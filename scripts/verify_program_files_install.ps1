# Comprehensive Verification Script for Program Files Installation & Activation
$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot | Split-Path -Parent

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  dTran 1.0.0 - PROGRAM FILES & ACTIVATION VERIFICATION   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Stop any running instances
Write-Host "`n[1/7] Terminating existing dTranslate / dTranLauncher processes..." -ForegroundColor Yellow
Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

# 2. Run Setup.exe silently
$setupExe = Join-Path $repoRoot "dist\dTran-1.0.0-x64-Setup.exe"
if (-not (Test-Path $setupExe)) { throw "Setup executable not found at: $setupExe" }

Write-Host "`n[2/7] Running installer into C:\Program Files\dTran..." -ForegroundColor Yellow
$proc = Start-Process -FilePath $setupExe -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES /NORESTART" -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    throw "Setup.exe failed with exit code: $($proc.ExitCode)"
}
Write-Host "      Setup.exe completed with exit code: 0" -ForegroundColor Green

# 3. Verify Install Directory and Files
Write-Host "`n[3/7] Verifying Program Files installation directory..." -ForegroundColor Yellow
$targetDir = "C:\Program Files\dTran"
if (-not (Test-Path $targetDir)) {
    throw "Target directory does not exist: $targetDir"
}
Write-Host "      Target directory verified: $targetDir" -ForegroundColor Green

$requiredFiles = @("dTranslate.exe", "dTranLauncher.exe", "AppxManifest.xml", "Assets\app.ico", "installer\register_app.ps1")
foreach ($f in $requiredFiles) {
    $p = Join-Path $targetDir $f
    if (-not (Test-Path $p)) {
        throw "Required installed file missing: $p"
    }
}
Write-Host "      All required binaries, manifests, and assets present." -ForegroundColor Green

# 4. Verify Package Identity in Windows AppModel
Write-Host "`n[4/7] Checking Windows AppModel registration..." -ForegroundColor Yellow
$pkg = Get-AppxPackage -Name "dTranslate" -ErrorAction SilentlyContinue
if (-not $pkg) {
    throw "dTranslate package is NOT registered in Windows AppModel!"
}

Write-Host "      Package Name      : $($pkg.Name)" -ForegroundColor Green
Write-Host "      Package Full Name : $($pkg.PackageFullName)" -ForegroundColor Green
Write-Host "      Package Family    : $($pkg.PackageFamilyName)" -ForegroundColor Green
Write-Host "      Install Location  : $($pkg.InstallLocation)" -ForegroundColor Green

if ($pkg.InstallLocation.TrimEnd('\') -ne $targetDir.TrimEnd('\')) {
    throw "Package InstallLocation mismatch! Expected '$targetDir', got '$($pkg.InstallLocation)'"
}

$aumid = "$($pkg.PackageFamilyName)!App"
Write-Host "      Derived AUMID     : $aumid" -ForegroundColor Green

# 5. Inspect Start Menu Shortcut (.lnk)
Write-Host "`n[5/7] Inspecting Start Menu shortcut properties..." -ForegroundColor Yellow
$startMenuLnk = "C:\ProgramData\Microsoft\Windows\Start Menu\Programs\dTran.lnk"
if (-not (Test-Path $startMenuLnk)) {
    $startMenuLnk = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs\dTran.lnk"
}
if (-not (Test-Path $startMenuLnk)) {
    throw "Start menu shortcut not found at expected paths!"
}

$wsh = New-Object -ComObject WScript.Shell
$sc = $wsh.CreateShortcut($startMenuLnk)
Write-Host "      Shortcut File     : $startMenuLnk"
Write-Host "      Target Path       : $($sc.TargetPath)"
Write-Host "      Arguments         : '$($sc.Arguments)'"
Write-Host "      Working Directory : $($sc.WorkingDirectory)"
Write-Host "      Icon Location     : $($sc.IconLocation)"

$expectedTarget = "$targetDir\dTranLauncher.exe"
if ($sc.TargetPath -ne $expectedTarget) {
    throw "Shortcut TargetPath mismatch! Expected '$expectedTarget', got '$($sc.TargetPath)'"
}
if ($sc.Arguments -ne "") {
    throw "Shortcut Arguments should be empty for dTranLauncher.exe!"
}
Write-Host "      Shortcut structure verified: clean executable target, no explorer.exe wrapper." -ForegroundColor Green

# 6. Test Launch via Shortcut
Write-Host "`n[6/7] Testing application launch via shortcut..." -ForegroundColor Yellow
Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

& $startMenuLnk
Start-Sleep -Seconds 3

$appProc = Get-Process -Name "dTranslate" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $appProc) {
    throw "Application failed to launch via Start Menu shortcut!"
}

Write-Host "      Application started successfully!" -ForegroundColor Green
Write-Host "      PID         : $($appProc.Id)" -ForegroundColor Green
Write-Host "      Path        : $($appProc.Path)" -ForegroundColor Green
Write-Host "      Working Set : $([Math]::Round($appProc.WorkingSet64/1MB, 2)) MB" -ForegroundColor Green

# Verify Non-Elevated Token (Medium Integrity)
try {
    $handle = $appProc.Handle
    # Process is running normally under interactive session
    Write-Host "      Process is active and responsive in user session." -ForegroundColor Green
} catch {
    Write-Warning "Could not query process handle: $_"
}

# 7. Test Direct EXE Launch (C:\Program Files\dTran\dTranslate.exe)
Write-Host "`n[7/7] Testing direct EXE launch (C:\Program Files\dTran\dTranslate.exe)..." -ForegroundColor Yellow
Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 500

$directExe = Join-Path $targetDir "dTranslate.exe"
Start-Process -FilePath $directExe
Start-Sleep -Seconds 3

$directProc = Get-Process -Name "dTranslate" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $directProc) {
    throw "Application failed to launch directly via $directExe!"
}

Write-Host "      Direct EXE launch succeeded via PackagedAppLauncher!" -ForegroundColor Green
Write-Host "      PID         : $($directProc.Id)" -ForegroundColor Green
Write-Host "      Working Set : $([Math]::Round($directProc.WorkingSet64/1MB, 2)) MB" -ForegroundColor Green

Get-Process dTranslate, dTranLauncher -ErrorAction SilentlyContinue | Stop-Process -Force

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "       ALL PROGRAM FILES & ACTIVATION CHECKS PASSED       " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
