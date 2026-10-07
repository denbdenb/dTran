; Inno Setup Script for dTran 1.0.0 (x64)
; Clean, lightweight, per-user Windows installer

#define MyAppName "dTran"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "denb"
#define MyAppURL "https://github.com/denb/dTran"
#define MyAppExeName "dTranslate.exe"
#define MyPackageFamily "dTranslate_4evqteexctg80"

[Setup]
AppId={{8B1A2C3D-4E5F-6A7B-8C9D-0E1F2A3B4C5D}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DisableProgramGroupPage=yes
DefaultGroupName={#MyAppName}
PrivilegesRequired=lowest
OutputDir=..\dist
OutputBaseFilename=dTran-1.0.0-x64-Setup
SetupIconFile=..\assets\app.ico
UninstallDisplayIcon={app}\Assets\app.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
CloseApplicationsFilter=dTranslate.exe
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build\layout\x64\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "explorer.exe"; Parameters: "shell:AppsFolder\{#MyPackageFamily}!App"; IconFilename: "{app}\Assets\app.ico"
Name: "{autodesktop}\{#MyAppName}"; Filename: "explorer.exe"; Parameters: "shell:AppsFolder\{#MyPackageFamily}!App"; IconFilename: "{app}\Assets\app.ico"; Tasks: desktopicon

[Run]
; Launch dTran on user request
Filename: "explorer.exe"; Parameters: "shell:AppsFolder\{#MyPackageFamily}!App"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[Code]
// Helper to terminate running instance before upgrade/uninstall
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  Exec('powershell.exe', '-NoProfile -Command "Get-Process dTranslate -ErrorAction SilentlyContinue | Stop-Process -Force"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := True;
end;

function InitializeUninstall(): Boolean;
var
  ResultCode: Integer;
begin
  Exec('powershell.exe', '-NoProfile -Command "Get-Process dTranslate -ErrorAction SilentlyContinue | Stop-Process -Force"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := True;
end;

// Register AppX package layout in Windows AppModel upon file extraction
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  ManifestPath: String;
begin
  if CurStep = ssPostInstall then
  begin
    ManifestPath := ExpandConstant('{app}') + '\AppxManifest.xml';
    Exec('powershell.exe', '-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "Add-AppxPackage -Register ''' + ManifestPath + ''' -ForceApplicationShutdown"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

// Unregister AppX package on uninstall before directory removal
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usUninstall then
  begin
    Exec('powershell.exe', '-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -Command "Get-AppxPackage -Name dTranslate -ErrorAction SilentlyContinue | Remove-AppxPackage"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;
