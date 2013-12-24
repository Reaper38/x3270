/*
 * Copyright (c) 1996-2009, Paul Mattes.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Paul Mattes nor his contributors may be used
 *       to endorse or promote products derived from this software without
 *       specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY PAUL MATTES "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL PAUL MATTES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *	shortcut.c
 *		A Windows console-based 3270 Terminal Emulator
 *		Shell link creation
 */

#include <windows.h>
#include <shlobj.h>
#include "shlobj_missing.h"
#include <stdio.h>

#include "shortcutc.h"
#include "winversc.h"

// CreateLink - uses the shell's IShellLink and IPersistFile interfaces 
//   to create and store a shortcut to the specified object. 
// Returns the result of calling the member functions of the interfaces. 
// lpszPathObj - address of a buffer containing the path of the object 
// lpszPathLink - address of a buffer containing the path where the 
//   shell link is to be stored 
// lpszDesc - address of a buffer containing the description of the 
//   shell link 
HRESULT
CreateLink(LPCSTR lpszPathObj, LPSTR lpszPathLink, LPSTR lpszDesc,
    LPSTR lpszArgs, LPSTR lpszDir, int rows, int cols, wchar_t *font,
    int pointsize, int codepage)
{
	HRESULT			hres;
	int	 		initialized;
	IShellLink*		psl = NULL; 
	IShellLinkDataList* 	psldl = NULL; 
	IPersistFile*		ppf = NULL;
	NT_CONSOLE_PROPS	p;
	WORD			wsz[MAX_PATH];

	hres = CoInitialize(NULL);
	if (!SUCCEEDED(hres)) {
	    	fprintf(stderr, "CoInitialize failed\n");
		goto out;
	}
	initialized = 1;

	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
	    &IID_IShellLink, (LPVOID *)&psl);

	if (!SUCCEEDED(hres)) {
	    	fprintf(stderr, "CoCreateInstance failed\n");
		goto out;
	}

	// Set the path to the shortcut target, and add the description.
	psl->lpVtbl->SetPath(psl, lpszPathObj);
	if (lpszDesc)
		psl->lpVtbl->SetDescription(psl, lpszDesc);
	if (lpszArgs)
		psl->lpVtbl->SetArguments(psl, lpszArgs);
	if (lpszDir)
		psl->lpVtbl->SetWorkingDirectory(psl, lpszDir);

	if (is_nt) {
		hres = psl->lpVtbl->QueryInterface(psl,
			&IID_IShellLinkDataList, (void **)&psldl);
		if (!SUCCEEDED(hres)) {
			fprintf(stderr, "QueryInterface(DataList) failed\n");
			goto out;
		}

		memset(&p, '\0', sizeof(NT_CONSOLE_PROPS));
#if defined(_MSC_VER) /*[*/
		p.cbSize = sizeof(p);
		p.dwSignature = NT_CONSOLE_PROPS_SIG;
#else /*][*/
		p.dbh.cbSize = sizeof(p);
		p.dbh.dwSignature = NT_CONSOLE_PROPS_SIG;
#endif /*]*/
		p.wFillAttribute = 7;	/* ? */
		p.wPopupFillAttribute = 245;	/* ? */
		p.dwScreenBufferSize.X = cols;
		p.dwScreenBufferSize.Y = 0x012c;
		p.dwWindowSize.X = cols;
		p.dwWindowSize.Y = rows;
		p.dwWindowOrigin.X = 0;
		p.dwWindowOrigin.Y = 0;
		p.nFont = 0;
		p.nInputBufferSize = 0;
		p.dwFontSize.X = 0;
		p.dwFontSize.Y = pointsize? pointsize: 12;
		p.uFontFamily = 54; /* ? */
		p.uFontWeight = 400; /* ? */
		wcscpy(p.FaceName, font);
		p.uCursorSize = /*0x19*/100;
		p.bFullScreen = 0;
		p.bQuickEdit = 0;
		p.bInsertMode = 1;
		p.bAutoPosition = 1;
		p.uHistoryBufferSize = 0x32;
		p.uNumberOfHistoryBuffers = 4;
		p.bHistoryNoDup = 0;
		p.ColorTable[0] = 0;
		p.ColorTable[1] =  0x00800000;
		p.ColorTable[2] =  0x00008000;
		p.ColorTable[3] =  0x00808000;
		p.ColorTable[4] =  0x00000080;
		p.ColorTable[5] =  0x00800080;
		p.ColorTable[6] =  0x00008080;
		p.ColorTable[7] =  0x00c0c0c0;
		p.ColorTable[8] =  0x00808080;
		p.ColorTable[9] =  0x00ff8000;
		p.ColorTable[10] = 0x0000ff00;
		p.ColorTable[11] = 0x00ffff00;
		p.ColorTable[12] = 0x000a0adc;
		p.ColorTable[13] = 0x00ff00ff;
		p.ColorTable[14] = 0x0000ffff;
		p.ColorTable[15] = 0x00ffffff;

		hres = psldl->lpVtbl->AddDataBlock(psldl, &p);
		if (!SUCCEEDED(hres)) {
			fprintf(stderr, "AddDataBlock(1) failed\n");
			goto out;
		}

		if (codepage) {
			NT_FE_CONSOLE_PROPS pfe;

			memset(&pfe, '\0', sizeof(pfe));
#if defined(_MSC_VER) /*[*/
			pfe.cbSize = sizeof(pfe);
			pfe.dwSignature = NT_FE_CONSOLE_PROPS_SIG;
#else /*][*/
			pfe.dbh.cbSize = sizeof(pfe);
			pfe.dbh.dwSignature = NT_FE_CONSOLE_PROPS_SIG;
#endif /*]*/
			pfe.uCodePage = codepage;

			hres = psldl->lpVtbl->AddDataBlock(psldl, &pfe);
			if (!SUCCEEDED(hres)) {
				fprintf(stderr, "AddDataBlock(2) failed\n");
				goto out;
			}
		}
	}

	// Query IShellLink for the IPersistFile interface for saving
	// the shortcut in persistent storage.
	hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile,
	    (void **)&ppf);

	if (!SUCCEEDED(hres)) {
	    	fprintf(stderr, "QueryInterface(Persist) failed\n");
		goto out;
	}

	// Ensure that the string is ANSI.
	MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

	// Save the link by calling IPersistFile::Save.
	hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
	if (!SUCCEEDED(hres)) {
	    	fprintf(stderr, "Save failed\n");
		goto out;
	}

out:
	if (ppf != NULL)
		ppf->lpVtbl->Release(ppf);
	if (psldl != NULL)
		psldl->lpVtbl->Release(psldl);
	if (psl != NULL)
		psl->lpVtbl->Release(psl);

	if (initialized)
		CoUninitialize();

	return hres;
} 

/* Data for Piffle(). */
static unsigned char canned_pif[] = {
	0x00, 0x78, 0x55, 0x56, 0x56, 0x4d, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x80, 0x02, 0x00, 0x00, 0x43, 0x3a, 0x5c, 0x50,
	0x52, 0x4f, 0x47, 0x52, 0x41, 0x7e, 0x31, 0x5c,
	0x57, 0x43, 0x33, 0x32, 0x37, 0x30, 0x5c, 0x77,
	0x63, 0x33, 0x32, 0x37, 0x30, 0x2e, 0x65, 0x78,
	0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x10, 0x00, 0x43, 0x3a, 0x5c,
	0x50, 0x52, 0x4f, 0x47, 0x52, 0x41, 0x7e, 0x31,
	0x5c, 0x57, 0x43, 0x33, 0x32, 0x37, 0x30, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x76,
	0x6d, 0x2e, 0x77, 0x63, 0x33, 0x32, 0x37, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
	0xff, 0x19, 0x50, 0x00, 0x00, 0x07, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x4d, 0x49, 0x43, 0x52, 0x4f, 0x53, 0x4f,
	0x46, 0x54, 0x20, 0x50, 0x49, 0x46, 0x45, 0x58,
	0x00, 0x87, 0x01, 0x00, 0x00, 0x71, 0x01, 0x57,
	0x49, 0x4e, 0x44, 0x4f, 0x57, 0x53, 0x20, 0x33,
	0x38, 0x36, 0x20, 0x33, 0x2e, 0x30, 0x00, 0x05,
	0x02, 0x9d, 0x01, 0x68, 0x00, 0x80, 0x02, 0x00,
	0x00, 0x64, 0x00, 0x32, 0x00, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0x00, 0x00, 0x02, 0x10, 0x02,
	0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x76, 0x76,
	0x6d, 0x2e, 0x77, 0x63, 0x33, 0x32, 0x37, 0x30,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x49, 0x4e,
	0x44, 0x4f, 0x57, 0x53, 0x20, 0x56, 0x4d, 0x4d,
	0x20, 0x34, 0x2e, 0x30, 0x00, 0xff, 0xff, 0x1b,
	0x02, 0xac, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x43, 0x3a, 0x5c, 0x50, 0x72,
	0x6f, 0x67, 0x72, 0x61, 0x6d, 0x20, 0x46, 0x69,
	0x6c, 0x65, 0x73, 0x5c, 0x77, 0x63, 0x33, 0x32,
	0x37, 0x30, 0x5c, 0x77, 0x63, 0x33, 0x32, 0x37,
	0x30, 0x2e, 0x65, 0x78, 0x65, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x32, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x05, 0x00, 0x19, 0x00, 0x03,
	0x00, 0xc8, 0x00, 0xe8, 0x03, 0x02, 0x00, 0x0a,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x54, 0x65, 0x72,
	0x6d, 0x69, 0x6e, 0x61, 0x6c, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x75, 0x63,
	0x69, 0x64, 0x61, 0x20, 0x43, 0x6f, 0x6e, 0x73,
	0x6f, 0x6c, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	0x00, 0x00, 0x00, 0x50, 0x00, 0x32, 0x00, 0xe0,
	0x01, 0xf4, 0x01, 0xec, 0x01, 0x2f, 0x02, 0x16,
	0x00, 0x00, 0x00, 0x01, 0x00, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0x58, 0x00, 0x58,
	0x00, 0x44, 0x02, 0x87, 0x02, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00
};

typedef struct {
    	unsigned char data[2];
} ua16_t;

typedef struct {
    	unsigned char data[4];
} ua32_t;

typedef struct {
    	char section_name[16];			/* NULL terminated */
	ua16_t next_off;
	ua16_t data_off;
	ua16_t data_len;
} pifsh_t;

typedef struct {
    	struct basic {
	    	unsigned char unused1;
		unsigned char checksum;
		char window_title[30];		/* blank padded */	/*+*/
		ua16_t max_mem;
		ua16_t min_mem;
		char program_pathname[63];	/* NULL terminated */	/*+*/
		ua16_t flags1;
		char working_dir[64];		/* NULL terminated */	/*+*/
		char parameters[64];		/* NULL terminated */	/*+*/
		unsigned char video_mode;
		unsigned char video_pages;
		unsigned char int_first;
		unsigned char int_last;
		unsigned char screen_height;
		unsigned char screen_width;
		unsigned char pos_x;
		unsigned char pos_y;
		ua16_t video_page;
		unsigned char unused2[128];
		ua16_t flags2;
	} basic;
	pifsh_t pifex_header;
	pifsh_t win386_header;
	struct win386 {
	    	ua16_t max_mem;
		ua16_t req_mem;
		ua16_t active_pri;
		ua16_t bg_pri;
		ua16_t max_ems;
		ua16_t req_emx;
		ua16_t max_xms;
		ua16_t req_xms;
		ua32_t flags1;
		ua16_t flags2;
		ua16_t unknown1;
		ua16_t shortcut_scan;
		ua16_t shortcut_mod;
		ua16_t use_shortcut;
		ua16_t shortcut_ext;
		ua16_t unknown2;
		ua16_t unknown3;
		ua32_t unknown4;
		char parameters[64];		/* NULL terminated */	/*+*/
	} win386;
	pifsh_t vmm_header;
	struct vmm {
	    	unsigned char unknown1[88];
		char icon_file[80];		/* NULL terminated */	/*+*/
		ua16_t icon_number;					/*+*/
		ua16_t flags1;
		unsigned char unknown2[10];
		ua16_t pri;
		ua16_t flags2;
		unsigned char unknown3[8];
		ua16_t text_lines;
		ua16_t flags3;
		unsigned char unknown4[16];
		ua16_t flags4;
		unsigned char unknown5[6];
		ua16_t flags5;
		ua16_t unknown6;
		ua16_t raster_horiz;
		ua16_t raster_vert;
		ua16_t font_horiz;
		ua16_t font_vert;
		char raster_font[32];		/* NULL terminated */
		char tt_font[32];		/* NULL terminated */
		ua16_t unknown7;
		ua16_t flags6;
		ua16_t flags7;
		ua16_t screen_width;
		ua16_t screen_height;		/* 25, 43 or 50 */	/*+*/
		ua16_t client_width;
		ua16_t client_height;
		ua16_t window_width;
		ua16_t window_height;
		ua16_t unknown8;
		ua16_t flags8;
		ua16_t flags9;
		ua16_t unknown9;
		ua16_t unknown10;
		ua16_t right_max;
		ua16_t bottom_max;
		ua16_t win_left;
		ua16_t win_top;
		ua16_t right_normal;
		ua16_t bottom_normal;
		ua32_t unknown11;
		char batchfile[80];		/* NULL terminated */
		ua16_t env_mem;
		ua16_t dpmi_mem;
		ua16_t unknown12;
	} vmm;
} w98pif_t;

/*
 * The Windows 98 version of CreateLink.
 * Creates a .PIF file instead, using secret undocumented recipes.
 */
HRESULT
Piffle(char *title, LPCSTR lpszPathObj, LPSTR lpszPathLink, LPSTR lpszDesc,
    LPSTR lpszArgs, LPSTR lpszDir, int rows, int cols, char *font)
{
    	w98pif_t pif;
	int sl;
	FILE *f;
	unsigned short screen_height;
	LPSTR args = lpszArgs? lpszArgs: "";

	/* Start with something that is mostly right. */
	memcpy(&pif, canned_pif, sizeof(w98pif_t));

	/* Fill in the fields of interest. */
	if (rows > 43)
	    	screen_height = 50;
	else if (rows > 25)
	    	screen_height = 43;
	else
	    	screen_height = 25;

	strncpy(title, pif.basic.window_title, sizeof(pif.basic.window_title));
	pif.basic.screen_height = (unsigned char)screen_height; /* XXX? */
	sl = strlen(title);
	if (sl < sizeof(pif.basic.window_title))
	    	memset(pif.basic.window_title + sl, ' ',
			sizeof(pif.basic.window_title - sl));
	strncpy(pif.basic.program_pathname, lpszPathObj,
		sizeof(pif.basic.program_pathname));
	strncpy(pif.basic.working_dir, lpszDir, sizeof(pif.basic.working_dir));
	strncpy(pif.basic.parameters, args, sizeof(pif.basic.parameters));
	strncpy(pif.win386.parameters, args, sizeof(pif.win386.parameters));
	strncpy(pif.vmm.icon_file, lpszPathObj, sizeof(pif.vmm.icon_file));
	pif.vmm.text_lines.data[0] = screen_height & 0x00ff;
	pif.vmm.text_lines.data[1] = (screen_height & 0xff00) >> 8;
	pif.vmm.screen_height.data[0] = screen_height & 0x00ff;
	pif.vmm.screen_height.data[1] = (screen_height & 0xff00) >> 8;
	pif.vmm.client_width.data[0] = 0;
	pif.vmm.client_width.data[1] = 0;
	pif.vmm.client_height.data[0] = 0;
	pif.vmm.client_height.data[1] = 0;
	pif.vmm.window_width.data[0] = 0;
	pif.vmm.window_width.data[1] = 0;
	pif.vmm.window_height.data[0] = 0;
	pif.vmm.window_height.data[1] = 0;
	strcpy(pif.vmm.tt_font, font);

	/* Create the file. */
	f = fopen(lpszPathLink, "wb");
	if (f == NULL)
	    	return S_FALSE;
	if (fwrite(&pif, sizeof(pif), 1, f) == 0) {
	    	fclose(f);
		return S_FALSE;
	}
	fclose(f);
	return S_OK;
}
