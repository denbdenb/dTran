$ErrorActionPreference = 'Stop'

Write-Host "=== VERIFYING SHORTCUT & INSTALLER FIX ===" -ForegroundColor Cyan

# 1. Stop any running dTranslate process
Get-Process dTranslate -ErrorAction SilentlyContinue | Stop-Process -Force

# 2. Run new Setup.exe
$setup = (Resolve-Path "dist\dTran-1.0.0-x64-Setup.exe").Path
Write-Host "Running Setup silently from: $setup" -ForegroundColor Cyan
$proc = Start-Process -FilePath $setup -ArgumentList "/VERYSILENT /SUPPRESSMSGBOXES" -PassThru -Wait
Write-Host "Setup Exit Code: $($proc.ExitCode)" -ForegroundColor Green
if ($proc.ExitCode -ne 0) {
    Write-Error "Setup failed with exit code $($proc.ExitCode)"
}

Start-Sleep -Seconds 2

# 3. Inspect Start Menu shortcut
$sh = New-Object -ComObject WScript.Shell
$lnkPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\dTran.lnk"
if (-not (Test-Path $lnkPath)) {
    Write-Error "Start Menu shortcut not found at: $lnkPath"
}
$lnk = $sh.CreateShortcut($lnkPath)
Write-Host "`n[1] Start Menu Shortcut Properties:" -ForegroundColor Yellow
Write-Host "    TargetPath:       '$($lnk.TargetPath)'"
Write-Host "    Arguments:        '$($lnk.Arguments)'"
Write-Host "    WorkingDirectory: '$($lnk.WorkingDirectory)'"
Write-Host "    IconLocation:     '$($lnk.IconLocation)'"

# Verify strict separation
if ($lnk.TargetPath -match "shell:AppsFolder") {
    Write-Error "FAIL: TargetPath incorrectly contains Arguments!"
} else {
    Write-Host "    PASS: TargetPath is strictly separated from Arguments!" -ForegroundColor Green
}

if ($lnk.Arguments -notmatch "shell:AppsFolder\\dTranslate_4evqteexctg80!App") {
    Write-Error "FAIL: Arguments does not match actual AUMID!"
} else {
    Write-Host "    PASS: Arguments matches actual AUMID (shell:AppsFolder\dTranslate_4evqteexctg80!App)!" -ForegroundColor Green
}

# 4. Test Launching via Shortcut
Write-Host "`n[2] Testing Launch via Shortcut ($lnkPath)..." -ForegroundColor Yellow
Start-Process $lnkPath
Start-Sleep -Seconds 3
$app1 = Get-Process dTranslate -ErrorAction SilentlyContinue
if ($app1) {
    Write-Host "    PASS: Application started successfully via shortcut! PID: $($app1.Id)" -ForegroundColor Green
    Stop-Process -Id $app1.Id -Force
    Start-Sleep -Seconds 1
} else {
    Write-Error "FAIL: Application did NOT start via shortcut!"
}

# 5. Test Launching <install directory>\dTranslate.exe directly
$installedExe = "$env:LOCALAPPDATA\Programs\dTran\dTranslate.exe"
Write-Host "`n[3] Testing Direct Launch of $installedExe..." -ForegroundColor Yellow
Start-Process $installedExe
Start-Sleep -Seconds 3
$app2 = Get-Process dTranslate -ErrorAction SilentlyContinue
if ($app2) {
    Write-Host "    PASS: Application started successfully via direct EXE launch! PID: $($app2.Id)" -ForegroundColor Green
    $wsMB = [Math]::Round($app2.WorkingSet64 / 1MB, 2)
    Write-Host "    Working Set: $wsMB MB"
} else {
    Write-Error "FAIL: Application did NOT start via direct EXE launch!"
}

Write-Host "`n=== ALL SHORTCUT & LAUNCH TESTS PASSED! ===" -ForegroundColor Green
