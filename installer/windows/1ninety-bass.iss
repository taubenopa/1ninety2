; Inno Setup Script - 1ninety-bass by taubenopa
#define AppName    "1ninety-bass"
#define AppVersion "1.0.0"
#define Publisher  "taubenopa"
#define BuildDir   "..\..\build\NinetyBass_artefacts\Release\VST3"

[Setup]
AppId={{B7D3F0A2-1E90-4C77-9A11-1A2B3C4D5E6F}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#Publisher}
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DirExistsWarning=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\..\dist
OutputBaseFilename=1ninety-bass-{#AppVersion}-Windows-Setup
SetupIconFile=..\..\assets\icon.ico
WizardImageFile=wizard_large.bmp
WizardSmallImageFile=wizard_small.bmp
WizardStyle=modern
Compression=lzma2
SolidCompression=yes
UninstallDisplayName={#AppName} (VST3)
UninstallDisplayIcon={commoncf64}\VST3\1ninety-bass.vst3\Contents\x86_64-win\1ninety-bass.vst3

[Messages]
WelcomeLabel1=Willkommen bei 1ninety-bass
WelcomeLabel2=Dieser Assistent installiert das VST3-Plugin 1ninety-bass von taubenopa nach:%n%n%CommonProgramFiles%\VST3%n%nSchliesse bitte deine DAW, bevor du fortfaehrst.

[Files]
Source: "{#BuildDir}\1ninety-bass.vst3\*"; DestDir: "{commoncf64}\VST3\1ninety-bass.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\1ninety-bass.vst3"
