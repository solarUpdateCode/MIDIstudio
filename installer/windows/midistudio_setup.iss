[Setup]
AppName=MIDIstudio by solaraudio
AppVersion=1.0.5
AppPublisher=solaraudio
AppPublisherURL=https://github.com/solarUpdateCode/MIDIstudio
DefaultDirName={autopf}\solaraudio\MIDIstudio
DefaultGroupName=solaraudio\MIDIstudio
OutputBaseFilename=MIDIstudio-Setup-1.0.5-Win64
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Types]
Name: "full"; Description: "Full Installation (Standalone App + VST3 Plugin + Local MuScriptor Server)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "standalone"; Description: "MIDIstudio Standalone Desktop Application"; Types: full custom
Name: "vst3"; Description: "MIDIstudio VST3 Plugin (64-bit)"; Types: full custom
Name: "localserver"; Description: "Automatic Local MuScriptor Transcription Server (uv + muscriptor)"; Types: full custom

[Files]
Source: "..\..\build\MIDIstudio_artefacts\Release\Standalone\MIDIstudio.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\build\MIDIstudio_artefacts\Release\VST3\MIDIstudio.vst3\*"; DestDir: "{commoncf64}\VST3\MIDIstudio.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "setup_local_server.bat"; DestDir: "{app}"; Components: localserver; Flags: ignoreversion
Source: "start_local_server.bat"; DestDir: "{app}"; Components: localserver; Flags: ignoreversion
Source: "..\local_bridge.py"; DestDir: "{app}"; Components: localserver; Flags: ignoreversion

[Icons]
Name: "{group}\MIDIstudio"; Filename: "{app}\MIDIstudio.exe"; Components: standalone
Name: "{group}\Start MIDIstudio Local Server"; Filename: "{app}\start_local_server.bat"; Components: localserver
Name: "{autodesktop}\MIDIstudio"; Filename: "{app}\MIDIstudio.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut for MIDIstudio"; GroupDescription: "Additional icons:"; Components: standalone

[Run]
Filename: "{app}\setup_local_server.bat"; Description: "Setup local MuScriptor transcription server automatically"; Components: localserver; Flags: postinstall runhidden waituntilterminated skipifsilent
