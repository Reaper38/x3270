/*
 * Copyright (c) 1994-2019 Paul Mattes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the names of Paul Mattes nor the names of his contributors
 *       may be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	print_screen.c
 *		Screen printing functions.
 */

#include "globals.h"

#include "appres.h"
#include "3270ds.h"
#include "ctlr.h"

#include "ctlrc.h"

#include <errno.h>
#include <assert.h>

#include "resources.h"

#include "actions.h"
#include "codepage.h"
#include "fprint_screen.h"
#include "popups.h"
#include "print_screen.h"
#include "print_gui.h"
#include "product.h"
#include "task.h"
#include "toggles.h"
#include "trace.h"
#include "unicodec.h"
#include "utils.h"

#if defined(_WIN32) /*[*/
# include <fcntl.h>
# include <sys/stat.h>
# include "w3misc.h"
# include "winprint.h"
#endif /*]*/

/* Typedefs */

/* Saved context for a suspended PrintText(). */
typedef struct {
    FILE *f;		/* temporary file */
    ptype_t ptype;	/* print type */
    unsigned opts;	/* options */
    const char *caption; /* caption text */
    const char *name;	/* printer name */
    char *temp_name;	/* temporary file name */
} printtext_t;

/* Globals */

/* Statics */

/* Print Text popup */

#if !defined(_WIN32) /*[*/
/* Termination code for print text process. */
static void
print_text_done(FILE *f)
{
    int status;

    status = pclose(f);
    if (status) {
	popup_an_error("Print program exited with status %d.",
		(status & 0xff00) > 8);
    } else {
	if (appres.interactive.do_confirms) {
	    popup_an_info("Screen image printed.");
	}
    }
}
#endif /*]*/

/**
 * Default caption.
 *
 * @return caption text
 */
char *
default_caption(void)
{
    static char *r = NULL;

#if !defined(_WIN32) /*[*/
    /* Unix version: username@host %T% */
    char hostname[132];
    char *user;

    gethostname(hostname, sizeof(hostname));
    hostname[sizeof(hostname) - 1] = '\0';
    user = getenv("USER");
    if (user == NULL) {
	user = "(unknown)";
    }

    if (r != NULL) {
	Free(r);
    }
    r = xs_buffer("%s @ %s %%T%%", user, hostname);

#else /*][*/

    char *username;
    char *computername;
    char *userdomain;
    char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    username = getenv("USERNAME");
    if (username == NULL) {
	username = "(unknown)";
    }
    computername = getenv("COMPUTERNAME");
    if (computername == NULL) {
	DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

	if (GetComputerName(ComputerName, &size)) {
	    computername = ComputerName;
	} else {
	    computername = "(unknown)";
	}
    }
    userdomain = getenv("USERDOMAIN");
    if (userdomain == NULL) {
	userdomain = "(unknown)";
    }

    if (r != NULL) {
	Free(r);
    }
    if (strcasecmp(userdomain, computername)) {
	r = xs_buffer("%s\\%s @ %s %%T%%", userdomain, username, computername);
    } else {
	r = xs_buffer("%s @ %s %%T%%", username, computername);
    }
#endif

    return r;
}

/* Extended-wait continue function for PrintText(). */
static void
printtext_continue(void *context, bool cancel)
{
    printtext_t *pt = (printtext_t *)context;
    fps_status_t status;

    if (cancel) {
	vtrace("PrintText canceled\n");
	fclose(pt->f);
	if (pt->temp_name != NULL) {
	    unlink(pt->temp_name);
	    Free(pt->temp_name);
	}
	Free(pt);
	return;
    }

    status = fprint_screen(pt->f, pt->ptype, pt->opts | FPS_DIALOG_COMPLETE,
	    pt->caption, pt->name, NULL);
    switch (status) {
    case FPS_STATUS_SUCCESS:
    case FPS_STATUS_SUCCESS_WRITTEN:
	vtrace("PrintText: printing succeeded.\n");
	break;
    case FPS_STATUS_ERROR:
	popup_an_error("Screen print failed");
	/* fall through */
    case FPS_STATUS_CANCEL:
	if (status == FPS_STATUS_CANCEL) {
	    vtrace("PrintText: printing canceled.\n");
	}
	break;
    case FPS_STATUS_WAIT:
	/* Can't happen. */
	assert(status != FPS_STATUS_WAIT);
	break;
    }

    fclose(pt->f);
    if (pt->temp_name != NULL) {
	unlink(pt->temp_name);
	Free(pt->temp_name);
    }
    Free(pt);
}

/* Print or save the contents of the screen as text. */
bool
PrintText_action(ia_t ia, unsigned argc, const char **argv)
{
    unsigned i;
    const char *name = NULL;
    bool secure = appres.secure;
    ptype_t ptype = P_TEXT;
    bool use_file = false;
    bool use_string = false;
    bool replace = false;
    char *temp_name = NULL;
    unsigned opts = FPS_EVEN_IF_EMPTY;
    const char *caption = NULL;
    FILE *f;
    int fd = -1;
    fps_status_t status;
    printtext_t *pt;

    if (!product_has_display()) {
	opts |= FPS_NO_DIALOG;
    }

    action_debug("PrintText", ia, argc, argv);

    /*
     * Pick off optional arguments:
     *  file     directs the output to a file instead of a command;
     *  	      must be the last keyword
     *  html     generates HTML output instead of ASCII text (and implies
     *            'file')
     *  rtf      generates RTF output instead of ASCII text (and implies
     *            'file')
     *  gdi      prints to a GDI printer (Windows only)
     *  nodialog skip print dialog (Windows only)
     *            this is the default for ws3270
     *  dialog	 use print dialog (Windows only)
     *            this is the default for wc3270
     *  replace  replace the file
     *  append   append to the file, if it exists (default)
     *  wordpad  prints via WordPad (wc3270 only)
     *  modi     print modified fields in italics
     *  caption "text"
     *           Adds caption text above the screen
     *           %T% is replaced by a timestamp
     *  secure   disables the pop-up dialog, if this action is invoked from
     *            a keymap (x3270 only)
     *  command  directs the output to a command (this is the default, but
     *            allows the command to be one of the other keywords);
     *  	      must be the last keyword
     *  string   returns the data as a string, allowed only from scripts
     */
    for (i = 0; i < argc; i++) {
	if (!strcasecmp(argv[i], "file")) {
	    use_file = true;
	    i++;
	    break;
	} else if (!strcasecmp(argv[i], "html")) {
	    ptype = P_HTML;
	    use_file = true;
	} else if (!strcasecmp(argv[i], "rtf")) {
	    ptype = P_RTF;
	    use_file = true;
	} else if (!strcasecmp(argv[i], "replace")) {
	    replace = true;
	} else if (!strcasecmp(argv[i], "append")) {
	    replace = false;
	}
#if defined(_WIN32) /*[*/
	else if (!strcasecmp(argv[i], "gdi")) {
	    ptype = P_GDI;
	} else if (!strcasecmp(argv[i], "wordpad")) {
	    ptype = P_RTF;
	} else if (!strcasecmp(argv[i], "nodialog")) {
	    opts |= FPS_NO_DIALOG;
	} else if (!strcasecmp(argv[i], "dialog")) {
	    opts &= ~FPS_NO_DIALOG;
	}
#endif /*]*/
	else if (!strcasecmp(argv[i], "secure")) {
	    secure = true;
	} else if (!strcasecmp(argv[i], "command")) {
	    if ((ptype != P_TEXT) || use_file) {
		popup_an_error("PrintText: contradictory options");
		return false;
	    }
	    i++;
	    break;
	} else if (!strcasecmp(argv[i], "string")) {
	    use_string = true;
	    use_file = true;
	} else if (!strcasecmp(argv[i], "modi")) {
	    opts |= FPS_MODIFIED_ITALIC;
	} else if (!strcasecmp(argv[i], "caption")) {
	    if (i == argc - 1) {
		popup_an_error("PrintText: mising caption parameter");
		return false;
	    }
	    caption = argv[++i];
	} else {
	    break;
	}
    }

    switch (argc - i) {
    case 0:
	/* Use the default. */
	if (!use_file) {
#if !defined(_WIN32) /*[*/
	    name = get_resource(ResPrintTextCommand);
#else /*][*/
	    name = get_resource(ResPrinterName); /* XXX */
#endif /*]*/
	}
	break;
    case 1:
	if (use_string) {
	    popup_an_error("PrintText: extra arguments or invalid option(s)");
	    return false;
	}
	name = argv[i];
	break;
    default:
	popup_an_error("PrintText: extra arguments or invalid option(s)");
	return false;
    }

#if defined(_WIN32) /*[*/
    /* On Windows, use GDI as the default. */
    if (!use_string && !use_file && ptype == P_TEXT) {
	ptype = P_GDI;
    }
#endif /*]*/

    if (name != NULL && name[0] == '@') {
	/*
	 * Starting the PrintTextCommand resource value with '@'
	 * suppresses the pop-up dialog, as does setting the 'secure'
	 * resource.
	 */
	secure = true;
	name++;
    }
    if (!use_file && (name == NULL || !*name)) {
#if !defined(_WIN32) /*[*/
	name = "lpr";
#else /*][*/
	name = NULL;
#endif /*]*/
    }

    /* See if the GUI wants to handle it. */
    if (!secure && print_text_gui(use_file)) {
	return true;
    }

    /* Do the real work. */
    if (use_file) {
	if (use_string) {
#if defined(_WIN32) /*[*/
	    fd = win_mkstemp(&temp_name, ptype);
#else /*][*/
	    temp_name = NewString("/tmp/x3hXXXXXX");
	    fd = mkstemp(temp_name);
#endif /*]*/
	    if (fd < 0) {
		popup_an_errno(errno, "mkstemp");
		return false;
	    }
	    f = fdopen(fd, "w+");
	    vtrace("PrintText: using '%s'\n", temp_name);
	} else {
	    if (name == NULL || !*name) {
		popup_an_error("PrintText: missing filename");
		return false;
	    }
	    f = fopen(name, replace? "w": "a");
	}
    } else {
#if !defined(_WIN32) /*[*/
	f = popen(name, "w");
#else /*][*/
	fd = win_mkstemp(&temp_name, ptype);
	if (fd < 0) {
	    popup_an_errno(errno, "mkstemp");
	    return false;
	}
	if (ptype == P_GDI) {
	    f = fdopen(fd, "wb+");
	} else {
	    f = fdopen(fd, "w+");
	}
	vtrace("PrintText: using '%s'\n", temp_name);
#endif /*]*/
    }
    if (f == NULL) {
	popup_an_errno(errno, "PrintText: %s", name);
	if (fd >= 0) {
	    close(fd);
	}
	if (temp_name) {
	    unlink(temp_name);
	    Free(temp_name);
	}
	return false;
    }

    /* Captions look nice on GDI, so create a default one. */
    if (ptype == P_GDI && caption == NULL) {
	caption = default_caption();
    }

    pt = (printtext_t *)Calloc(1, sizeof(printtext_t));
    status = fprint_screen(f, ptype, opts, caption, name, pt);
    switch (status) {
    case FPS_STATUS_SUCCESS:
    case FPS_STATUS_SUCCESS_WRITTEN:
	vtrace("PrintText: printing succeeded.\n");
	Free(pt);
	break;
    case FPS_STATUS_ERROR:
	popup_an_error("Screen print failed.");
	/* fall through */
    case FPS_STATUS_CANCEL:
	if (status == FPS_STATUS_CANCEL) {
	    vtrace("PrintText: printing canceled.\n");
	}
	Free(pt);
	fclose(f);
	if (temp_name) {
	    unlink(temp_name);
	    Free(temp_name);
	}
	return false;
    case FPS_STATUS_WAIT:
	/* Waiting for asynchronous activity. */
	assert(ptype == P_GDI);
	pt->f = f;
	pt->ptype = ptype;
	pt->opts = opts;
	pt->caption = caption;
	pt->name = name;
	pt->temp_name = temp_name;
	task_xwait(pt, printtext_continue, "printing");
	return true;
    }

    if (use_string) {
	char buf[8192];

	/* Print to string. */
	fflush(f);
	rewind(f);
	while (fgets(buf, sizeof(buf), f) != NULL) {
	    action_output("%s", buf);
	}
	fclose(f);
	unlink(temp_name);
	Free(temp_name);
	return true;
    }

    if (use_file) {
	/* Print to specified file. */
	fclose(f);
	return true;
    }

    /* Print to printer. */
#if !defined(_WIN32) /*[*/
    print_text_done(f);
#else /*][*/
    fclose(f);
    if (ptype == P_RTF) {
	if (product_has_display()) {
	    /* Run WordPad to print the file, asynchronusly. */
	    start_wordpad_async("PrintText", temp_name, name);
	} else {
	    /* Run WordPad to print the file, synchronusly. */
	    start_wordpad_sync("PrintText", temp_name, name);
	    unlink(temp_name);
	}
    } else if (ptype == P_GDI) {
	/* All done with the temp file. */
	unlink(temp_name);
    }
    if (appres.interactive.do_confirms) {
	popup_an_info("Screen image printing.\n");
    }
#endif /*]*/
    Free(temp_name);
    return true;
}

/*
 * ScreenTrace(On)
 * ScreenTrace(On,filename)			 backwards-compatible
 * ScreenTrace(On,File,filename)		 preferred
 * ScreenTrace(On,Printer)
 * ScreenTrace(On,Printer,"print command")	 Unix
 * ScreenTrace(On,Printer[,Gdi[,Dialog]|WordPad],printername) Windows
 * ScreenTrace(Off)
 */
static bool
ScreenTrace_action(ia_t ia, unsigned argc, const char **argv)
{
    bool on = false;
#if defined(_WIN32) /*[*/
    bool is_file = false;
#endif /*]*/
    tss_t how = TSS_FILE;
    ptype_t ptype = P_TEXT;
    const char *name = NULL;
    unsigned px;
    unsigned opts = product_has_display()? 0 : FPS_NO_DIALOG;

    action_debug("ScreenTrace", ia, argc, argv);

    if (argc == 0) {
	how = trace_get_screentrace_how();
	if (toggled(SCREEN_TRACE)) {
	    action_output("Screen tracing is enabled, %s \"%s\".",
		    (how == TSS_FILE)? "file":
#if !defined(_WIN32) /*[*/
		    "with print command",
#else /*]*/
		    "to printer",
#endif /*]*/
		    trace_get_screentrace_name());
	} else {
	    action_output("Screen tracing is disabled.");
	}
	return true;
    }

    if (!strcasecmp(argv[0], "Off")) {
	if (!toggled(SCREEN_TRACE)) {
	    popup_an_error("Screen tracing is already disabled.");
	    return false;
	}
	on = false;
	if (argc > 1) {
	    popup_an_error("ScreenTrace(): Too many arguments for 'Off'");
	    return false;
	}
	goto toggle_it;
    }
    if (strcasecmp(argv[0], "On")) {
	popup_an_error("ScreenTrace(): Must be 'On' or 'Off'");
	return false;
    }

    /* Process 'On'. */
    if (toggled(SCREEN_TRACE)) {
	popup_an_error("Screen tracing is already enabled.");
	return true;
    }

    on = true;
    px = 1;

    if (px >= argc) {
	/*
	 * No more parameters. Trace to a file, and generate the name.
	 */
	goto toggle_it;
    }
    if (!strcasecmp(argv[px], "File")) {
	px++;
#if defined(_WIN32) /*[*/
	is_file = true;
#endif /*]*/
    } else if (!strcasecmp(argv[px], "Printer")) {
	px++;
	how = TSS_PRINTER;
#if defined(WIN32) /*[*/
	ptype = P_GDI;
#endif /*]*/
    }
#if defined(_WIN32) /*[*/
    if (px < argc && !strcasecmp(argv[px], "Gdi")) {
	if (is_file) {
	    popup_an_error("ScreenTrace(): Cannot specify 'File' and 'Gdi'.");
	    return false;
	}
	px++;
	how = TSS_PRINTER;
	ptype = P_GDI;
	if (px < argc && !strcasecmp(argv[px], "Dialog")) {
	    px++;
	    opts &= ~FPS_NO_DIALOG;
	}
    } else if (px < argc && !strcasecmp(argv[px], "WordPad")) {
	if (is_file) {
	    popup_an_error("ScreenTrace(): Cannot specify 'File' and "
		    "'WordPad'.");
	    return false;
	}
	px++;
	how = TSS_PRINTER;
	ptype = P_RTF;
    }
#endif /*]*/
    if (px < argc) {
	name = argv[px];
	px++;
    }
    if (px < argc) {
	popup_an_error("ScreenTrace(): Too many arguments.");
	return false;
    }
    if (how == TSS_PRINTER && name == NULL) {
#if !defined(_WIN32) /*[*/
	name = get_resource(ResPrintTextCommand);
#else /*][*/
	name = get_resource(ResPrinterName);
#endif /*]*/
    }

toggle_it:
    if ((on && !toggled(SCREEN_TRACE)) || (!on && toggled(SCREEN_TRACE))) {
	if (on) {
	    trace_set_screentrace_file(how, ptype, opts, name);
	}
	do_toggle(SCREEN_TRACE);
    }
    if (on && !toggled(SCREEN_TRACE)) {
	return true;
    }

    name = trace_get_screentrace_name();
    if (name != NULL) {
	if (on) {
	    if (how == TSS_FILE) {
		if (ia_cause == IA_COMMAND) {
		    action_output("Trace file is %s.", name);
		} else {
		    popup_an_info("Trace file is %s.", name);
		}
	    } else {
		if (ia_cause == IA_COMMAND) {
		    action_output("Tracing to printer.");
		} else {
		    popup_an_info("Tracing to printer.");
		}
	    }
	} else {
	    if (trace_get_screentrace_last_how() == TSS_FILE) {
		if (ia_cause == IA_COMMAND) {
		    action_output("Tracing complete. Trace file is %s.", name);
		} else {
		    popup_an_info("Tracing complete. Trace file is %s.", name);
		}
	    } else {
		if (ia_cause == IA_COMMAND) {
		    action_output("Tracing to printer complete.");
		} else {
		    popup_an_info("Tracing to printer complete.");
		}
	    }
	}
    }
    return true;
}

/**
 * Print screen module registration.
 */
void
print_screen_register(void)
{
    static action_table_t print_text_actions[] = {
	{ "PrintText",		PrintText_action,	ACTION_KE },
	{ "ScreenTrace",	ScreenTrace_action,	ACTION_KE }
    };

    /* Register the actions. */
    register_actions(print_text_actions, array_count(print_text_actions));
}
