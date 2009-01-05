; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=wc3270
AppVerName=wc3270 3.3.9a5
AppPublisher=Paul Mattes
AppPublisherURL=http://x3270.bgp.nu
AppSupportURL=http://x3270.bgp.nu
AppUpdatesURL=http://x3270.bgp.nu
AppCopyright=Copyright (C) 1989-2009 by Paul Mattes, GTRC and others
WizardSmallImageFile=x3270-icon2.bmp
DefaultDirName={pf}\wc3270
DisableDirPage=no
DefaultGroupName=wc3270
AllowNoIcons=yes
OutputBaseFilename=wc3270-3.3.9a5-setup
OutputDir=\\Melville\pdm\psrc\x3270\Release\Inno\wc3270
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
MinVersion=4.0,5.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\wc3270.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\w3n4.dll"; DestDir: "{app}"; Flags: ignoreversion; OnlyBelowVersion: 0.0,5.01
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\w3n46.dll"; DestDir: "{app}"; Flags: ignoreversion; MinVersion: 0.0,5.01
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\shf.dll"; DestDir: "{app}"; Flags: ignoreversion; MinVersion: 0.0,5.0
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wpr3287\wpr3287.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\catf.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\mkshort.exe"; DestDir: "{app}"; Flags: ignoreversion deleteafterinstall
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\ead3270.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Release\Inno\wc3270\wc3270wiz.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Bugs.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Build.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\FAQ.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Intro.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Keymap.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Lineage.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\ReleaseNotes.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\README.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\wc3270-man.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\html\Wishlist.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wpr3287\html\wpr3287-man.html"; DestDir: "{app}\html"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "\\Melville\pdm\psrc\x3270\Source\3.3svn\wc3270\README.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Dirs]
Name: "{userappdata}\wc3270";

[Code]
function myHelp(Param: String): String;
begin
; result := '/c start /b ' + GetShortName(ExpandConstant('{app}') + '\html\README.html');
 result := '/c start ' + GetShortName(ExpandConstant('{app}') + '\html\README.html');
end;

[Icons]
Name: "{group}\New wc3270 Session"; Filename: "{app}\wc3270wiz.exe"; WorkingDir: "{app}"
Name: "{group}\Run wc3270"; Filename: "{app}\wc3270.exe"; WorkingDir: "{app}"
Name: "{group}\wc3270 Documentation"; Filename: "{app}\html\README.html"
Name: "{group}\wc3270 Explore AppData"; Filename: "{app}\ead3270.exe"; Flags: runminimized; IconFilename: "{app}\wc3270.exe"

[Registry]
Root: HKCR; Subkey: ".wc3270"; ValueType: string; ValueName: ""; ValueData: "wc3270"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "wc3270"; ValueType: string; ValueName: ""; ValueData: "wc3270 Emulator Session"; Flags: uninsdeletekey
Root: HKCR; Subkey: "wc3270\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\wc3270.exe,0"
Root: HKCR; Subkey: "wc3270\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\wc3270"" ""%1"""

[Run]
Filename: "{app}\wc3270wiz.exe"; Description: "{cm:LaunchProgram,wc3270 New Session Wizard}"; Flags: nowait postinstall skipifsilent
Filename: "{cmd}"; Parameters: {code:MyHelp}; Description: "{cm:LaunchProgram,Online Documentation}"; Flags: nowait postinstall skipifsilent
Filename: "{app}\mkshort.exe"; Parameters: """{app}"" wc3270.exe ""wc3270"""; Flags: nowait skipifsilent runhidden; Tasks: desktopicon
Filename: "{app}\mkshort.exe"; Parameters: """{app}"" wc3270wiz.exe ""New wc3270 Session"""; Flags: nowait skipifsilent runhidden; Tasks: desktopicon

[UninstallRun]
Filename: "{cmd}"; Parameters: "/c erase ""{userdesktop}\wc3270.lnk"""; Tasks: desktopicon; Flags: runhidden; MinVersion: 0,4.0
Filename: "{cmd}"; Parameters: "/c erase ""{userdesktop}\New wc3270 Session.lnk"""; Tasks: desktopicon; Flags: runhidden; MinVersion: 0,4.0
Filename: "{cmd}"; Parameters: "/c erase ""{userdesktop}\wc3270.pif"""; Tasks: desktopicon; Flags: runhidden; MinVersion: 4.0,0
Filename: "{cmd}"; Parameters: "/c erase ""{userdesktop}\New wc3270 Session.pif"""; Tasks: desktopicon; Flags: runhidden; MinVersion: 4.0,0