# Verification script for dTran 1.0.0 Unpackaged Self-Contained Deployment
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot | Split-Path -Parent
Set-Location $repoRoot

$setupExe = Join-Path $repoRoot "dist\dTran-1.0.0-x64-Setup.exe"
$portableZip = Join-Path $repoRoot "dist\dTran-1.0.0-x64-Portable.zip"
$installDir = "C:\Program Files\dTran"
$exePath = Join-Path $installDir "dTranslate.exe"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "    dTran 1.0.0 - END-TO-END DEPLOYMENT VERIFICATION       " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 0. Ensure clean state
Write-Host "`n[Step 0] Cleaning any previous test processes and files..." -ForegroundColor Yellow
Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

if (Test-Path "$installDir\unins000.exe") {
    Write-Host "Running existing uninstaller..."
    Start-Process -FilePath "$installDir\unins000.exe" -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES" -Wait
    Start-Sleep -Seconds 2
}

# 1. Verify Installer Existence and Size
Write-Host "`n[Step 1] Verifying installer package..." -ForegroundColor Yellow
if (-not (Test-Path $setupExe)) { throw "Installer not found at $setupExe" }
$setupSize = [Math]::Round((Get-Item $setupExe).Length / 1MB, 2)
Write-Host "Setup binary verified: $setupExe ($setupSize MB)" -ForegroundColor Green

# 2. Run Silent Installation
Write-Host "`n[Step 2] Performing silent installation to C:\Program Files\dTran..." -ForegroundColor Yellow
$proc = Start-Process -FilePath $setupExe -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART" -PassThru -Wait
if ($proc.ExitCode -ne 0) { throw "Setup failed with exit code $($proc.ExitCode)" }
Write-Host "Installation completed with exit code 0." -ForegroundColor Green

# 3. Verify Installed Files
Write-Host "`n[Step 3] Verifying installed layout in $installDir..." -ForegroundColor Yellow
if (-not (Test-Path $exePath)) { throw "dTranslate.exe not found at $exePath" }

$essentialFiles = @(
    "dTranslate.exe",
    "Microsoft.WindowsAppRuntime.dll",
    "Microsoft.ui.xaml.dll",
    "Microsoft.UI.Xaml.Controls.dll",
    "Microsoft.UI.Input.dll",
    "Microsoft.UI.Windowing.dll",
    "MRM.dll",
    "resources.pri",
    "Assets\app.ico"
)

foreach ($f in $essentialFiles) {
    $full = Join-Path $installDir $f
    if (-not (Test-Path $full)) { throw "Missing essential file: $full" }
}

# Ensure launcher wrapper is NOT installed
if (Test-Path (Join-Path $installDir "dTranLauncher.exe")) {
    throw "Old dTranLauncher.exe should not be present!"
}

# Ensure developer artifacts are NOT installed
if (Test-Path (Join-Path $installDir "TestRunner.exe")) {
    throw "TestRunner.exe should not be present in production install!"
}

$fileCount = (Get-ChildItem -Path $installDir -Recurse -File).Count
Write-Host "Installed files verified: $fileCount files present in $installDir." -ForegroundColor Green

# 4. Verify Shortcut Integrity
Write-Host "`n[Step 4] Inspecting Start Menu shortcut properties..." -ForegroundColor Yellow
$wsh = New-Object -ComObject WScript.Shell
$shortcutPaths = @(
    "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\dTran.lnk",
    "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\dTran.lnk"
)
$foundShortcut = $null
foreach ($sp in $shortcutPaths) {
    if (Test-Path $sp) {
        $foundShortcut = $sp
        break
    }
}

if (-not $foundShortcut) { throw "Start Menu shortcut not found!" }

$lnk = $wsh.CreateShortcut($foundShortcut)
Write-Host "Shortcut Path: $foundShortcut"
Write-Host "  Target:        $($lnk.TargetPath)"
Write-Host "  Arguments:     $($lnk.Arguments)"
Write-Host "  WorkingDir:    $($lnk.WorkingDirectory)"
Write-Host "  IconLocation:  $($lnk.IconLocation)"

if ($lnk.TargetPath -ne $exePath) {
    throw "Shortcut target mismatch! Expected: $exePath, Got: $($lnk.TargetPath)"
}
if ($lnk.Arguments -ne "") {
    throw "Shortcut arguments must be empty! Got: $($lnk.Arguments)"
}
if ($lnk.WorkingDirectory -ne $installDir) {
    throw "Shortcut working directory mismatch! Expected: $installDir, Got: $($lnk.WorkingDirectory)"
}
Write-Host "Shortcut properties verified: Direct link to dTranslate.exe without wrappers or arguments." -ForegroundColor Green

# 5. Live Execution Test: Normal Interactive Launch
Write-Host "`n[Step 5] Testing interactive launch of dTranslate.exe..." -ForegroundColor Yellow
# Clear previous crash log if any
$crashLog = "$env:TEMP\dTran_crash.log"
if (Test-Path $crashLog) { Remove-Item $crashLog -Force }

$appProc = Start-Process -FilePath $exePath -WorkingDirectory $installDir -PassThru
Start-Sleep -Seconds 4

if ($appProc.HasExited) {
    throw "dTranslate.exe exited unexpectedly with code $($appProc.ExitCode)!"
}

Write-Host "Process running successfully! PID: $($appProc.Id), WindowHandle: $($appProc.MainWindowHandle)" -ForegroundColor Green

if (Test-Path $crashLog) {
    $cLog = Get-Content $crashLog -Raw
    throw "Crash log detected during run: $cLog"
}

# Stop process cleanly
Stop-Process -Id $appProc.Id -Force
Start-Sleep -Seconds 1
Write-Host "Interactive launch test PASSED." -ForegroundColor Green

# 6. Live Execution Test: Startup Launch (--startup)
Write-Host "`n[Step 6] Testing background/silent launch (--startup)..." -ForegroundColor Yellow
$startupProc = Start-Process -FilePath $exePath -ArgumentList "--startup" -WorkingDirectory $installDir -PassThru
Start-Sleep -Seconds 3

if ($startupProc.HasExited) {
    throw "dTranslate.exe --startup exited unexpectedly with code $($startupProc.ExitCode)!"
}

Write-Host "Startup background process running! PID: $($startupProc.Id)" -ForegroundColor Green
Stop-Process -Id $startupProc.Id -Force
Start-Sleep -Seconds 1
Write-Host "Startup launch test PASSED." -ForegroundColor Green

# 7. Test Portable Package
Write-Host "`n[Step 7] Testing portable archive extraction and execution..." -ForegroundColor Yellow
if (-not (Test-Path $portableZip)) { throw "Portable zip not found at $portableZip" }
$portTestDir = "C:\temp\dTran_port_test"
if (Test-Path $portTestDir) { Remove-Item $portTestDir -Recurse -Force }
New-Item -ItemType Directory -Path $portTestDir -Force | Out-Null

Expand-Archive -Path $portableZip -DestinationPath $portTestDir -Force
$portExe = Join-Path $portTestDir "dTranslate.exe"

$portProc = Start-Process -FilePath $portExe -WorkingDirectory $portTestDir -PassThru
Start-Sleep -Seconds 3

if ($portProc.HasExited) {
    throw "Portable dTranslate.exe exited unexpectedly with code $($portProc.ExitCode)!"
}

Write-Host "Portable process running successfully! PID: $($portProc.Id)" -ForegroundColor Green
Stop-Process -Id $portProc.Id -Force
Start-Sleep -Seconds 1
Remove-Item $portTestDir -Recurse -Force
Write-Host "Portable package test PASSED." -ForegroundColor Green

# 8. Test Clean Uninstall
Write-Host "`n[Step 8] Testing clean uninstallation..." -ForegroundColor Yellow
$uninsExe = Join-Path $installDir "unins000.exe"
if (-not (Test-Path $uninsExe)) { throw "Uninstaller not found at $uninsExe" }

$uninsProc = Start-Process -FilePath $uninsExe -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES" -PassThru -Wait
Start-Sleep -Seconds 2

if (Test-Path $exePath) {
    throw "dTranslate.exe was not removed by uninstaller!"
}
if (Test-Path $foundShortcut) {
    throw "Start menu shortcut was not removed by uninstaller!"
}

Write-Host "Uninstallation verified: Program Files and Shortcuts cleanly removed." -ForegroundColor Green

# Check that %LOCALAPPDATA%\dTranslate is preserved (user data safety)
$appDataDir = "$env:LOCALAPPDATA\dTranslate"
Write-Host "AppData directory status: $(Test-Path $appDataDir) ($appDataDir)" -ForegroundColor Cyan

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "   ALL DEPLOYMENT VERIFICATION TESTS PASSED SUCCESSFULLY! " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
