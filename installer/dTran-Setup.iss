; Inno Setup Script for dTran 1.0.0 (x64)
; Standard Windows 11 installation into C:\Program Files\dTran
; Pure unpackaged WinUI 3 desktop application (Self-Contained)

#define MyAppName "dTran"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "denb"
#define MyAppURL "https://github.com/denb/dTran"
#define MyAppExeName "dTranslate.exe"

[Setup]
AppId={{8B1A2C3D-4E5F-6A7B-8C9D-0E1F2A3B4C5D}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
DefaultGroupName={#MyAppName}
PrivilegesRequired=admin
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
UsedUserAreasWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\build\layout\x64\Release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Direct shortcuts to dTranslate.exe (standard Windows executable links, no explorer.exe shell redirection, no launcher wrapper)
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\Assets\app.ico"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; IconFilename: "{app}\Assets\app.ico"; Tasks: desktopicon

[Registry]
; Clean up Start with Windows registry value on uninstall if user had enabled it
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "dTranslate"; Flags: dontcreatekey uninsdeletevalue

[Run]
; Launch dTran as the original non-elevated user
Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: postinstall nowait skipifsilent runasoriginaluser

[Code]
// Full path to PowerShell executable on all Windows 10/11 systems
function GetPowerShellExe(): String;
begin
  Result := ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe');
end;

// Helper to terminate running instances and clean up legacy packages/shortcuts
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  // 1. Terminate running dTranslate processes
  Exec(GetPowerShellExe(), '-NoProfile -Command "Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  // 2. Remove legacy AppX package if previously registered from older versions
  Exec(GetPowerShellExe(), '-NoProfile -ExecutionPolicy Bypass -Command "Get-AppxPackage -Name dTranslate -ErrorAction SilentlyContinue | Remove-AppxPackage -ErrorAction SilentlyContinue"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);

  // 3. Remove legacy shortcuts if upgrading
  DeleteFile(ExpandConstant('{localappdata}\Programs\dTran\dTran.lnk'));
  DeleteFile(ExpandConstant('{userprograms}\dTran.lnk'));
  DeleteFile(ExpandConstant('{userdesktop}\dTran.lnk'));

  Result := True;
end;

function InitializeUninstall(): Boolean;
var
  ResultCode: Integer;
begin
  Exec(GetPowerShellExe(), '-NoProfile -Command "Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := True;
end;

// Clean leftover files in installation directory on uninstall
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    Exec('cmd.exe', '/c rd /s /q "' + ExpandConstant('{app}') + '"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;
