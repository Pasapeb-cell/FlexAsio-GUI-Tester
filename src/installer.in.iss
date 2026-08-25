; Note: this Inno Setup installer script is meant to run as part of
; installer.cmake. It will not work on its own.
;
; Inno Setup 6 or later is required for this script to work.

[Setup]
AppID=FlexASIO
AppName=FlexASIO GUI Tester
AppVerName=FlexASIO GUI Tester @FLEXASIO_VERSION@
AppVersion=@FLEXASIO_VERSION@
AppPublisher=Pasapeb-cell
AppPublisherURL=https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester
AppSupportURL=https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/issues
AppUpdatesURL=https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/releases
AppReadmeFile=https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/blob/main/README.md
AppContact=https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/issues
WizardStyle=modern

DefaultDirName={autopf}\FlexASIO
AppendDefaultDirName=no
ArchitecturesInstallIn64BitMode=x64

[Files]
Source:"install\x64-Release\bin\FlexASIO.dll"; DestDir: "{app}\x64"; Flags: ignoreversion regserver 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\FlexASIOGUI.exe"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\Qt6Core.dll"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\Qt6Gui.dll"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\Qt6Widgets.dll"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\portaudio.dll"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\PortAudioDevices.exe"; DestDir: "{app}\x64"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\bin\platforms\qwindows.dll"; DestDir: "{app}\x64\platforms"; Flags: ignoreversion 64bit; Check: Is64BitInstallMode
Source:"install\x64-Release\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: Is64BitInstallMode
Source:"install\x86-Release\bin\FlexASIO.dll"; DestDir: "{app}\x86"; Flags: ignoreversion regserver
Source:"install\x86-Release\bin\portaudio.dll"; DestDir: "{app}\x86"; Flags: ignoreversion
Source:"..\..\RELEASE_NOTES.md"; DestDir:"{app}"; Flags: ignoreversion
Source:"..\..\THIRD_PARTY_NOTICES.md"; DestDir:"{app}"; Flags: ignoreversion
Source:"..\..\LICENSE.txt"; DestDir:"{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\FlexASIO GUI Tester"; Filename: "{app}\x64\FlexASIOGUI.exe"; WorkingDir: "{app}\x64"

[Code]
function InitializeSetup(): Boolean;
begin
  if WizardSilent() then begin
    Result := True;
  end else begin
    Result := MsgBox('This installer deliberately replaces an existing FlexASIO installation and registers both x64 and x86 drivers. Continue only if that is intended.', mbConfirmation, MB_OKCANCEL) = IDOK;
  end;
end;

[Run]
Filename:"{tmp}\vc_redist.x64.exe"; Parameters:"/install /quiet /norestart"; StatusMsg:"Installing Microsoft Visual C++ Redistributable..."; Flags: waituntilterminated skipifsilent; Check: Is64BitInstallMode
Filename:"https://github.com/Pasapeb-cell/FlexAsio-GUI-Tester/blob/main/README.md"; Description:"Open README"; Flags: postinstall shellexec nowait skipifsilent
