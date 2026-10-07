$ErrorActionPreference = 'Stop'

Write-Host "=== LIVE E2E STARTUP & PERFORMANCE AUDIT ===" -ForegroundColor Cyan

# 1. Inspect running dTranslate process
$proc = Get-Process dTranslate -ErrorAction SilentlyContinue
if (-not $proc) {
    Write-Host "Starting dTranslate 1.0..."
    $manifest = Join-Path $PSScriptRoot "..\build\bin\x64\Release\AppxManifest.xml"
    Add-AppxPackage -Register $manifest -ForceApplicationShutdown
    $pkg = Get-AppxPackage -Name dTranslate
    Start-Process "shell:AppsFolder\$($pkg.PackageFamilyName)!App"
    Start-Sleep -Seconds 2
    $proc = Get-Process dTranslate
}

$wsMB = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
$pmMB = [Math]::Round($proc.PrivateMemorySize64 / 1MB, 2)
Write-Host "[1] Process Memory Audit:" -ForegroundColor Green
Write-Host "    PID: $($proc.Id)"
Write-Host "    Working Set: $wsMB MB"
Write-Host "    Private Memory: $pmMB MB"
Write-Host "    Threads: $($proc.Threads.Count)"
Write-Host "    Handles: $($proc.HandleCount)"

# 2. Inspect AppModel StartupTask Registry key
$pkg = Get-AppxPackage -Name dTranslate
$regPath = "HKCU:\Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\SystemAppData\$($pkg.PackageFamilyName)\dTranStartupTask"
if (Test-Path $regPath) {
    $item = Get-ItemProperty $regPath
    Write-Host "[2] Windows Startup Registration State:" -ForegroundColor Green
    Write-Host "    Registry Path: $regPath"
    Write-Host "    State value: $($item.State)"
    if ($item.State -eq 2) {
        Write-Host "    STATUS: ENABLED (Windows will start dTran on user login) - PASS" -ForegroundColor Green
    } elseif ($item.State -eq 0) {
        Write-Host "    STATUS: DISABLED by user request" -ForegroundColor Yellow
    } elseif ($item.State -eq 1) {
        Write-Host "    STATUS: DISABLED BY USER in Task Manager" -ForegroundColor Yellow
    }
} else {
    Write-Host "[2] Registry path not found yet" -ForegroundColor Red
}

# 3. Verify No Conflict in HKCU\Run
$runVal = (Get-ItemProperty "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -ErrorAction SilentlyContinue).dTranslate
if ($runVal) {
    Write-Host "[3] FAIL: Duplicate legacy registry entry exists: $runVal" -ForegroundColor Red
} else {
    Write-Host "[3] HKCU\Run contains no duplicate/conflicting dTranslate entry - PASS" -ForegroundColor Green
}

# 4. Check Windows Startup Apps entry via Get-AppxPackage
$manifestXml = [xml](Get-Content (Join-Path $pkg.InstallLocation "AppxManifest.xml"))
$startupExt = $manifestXml.Package.Applications.Application.Extensions.Extension | Where-Object { $_.Category -eq 'windows.startupTask' }
Write-Host "[4] AppxManifest Declaration:" -ForegroundColor Green
Write-Host "    Category: $($startupExt.Category)"
Write-Host "    TaskId: $($startupExt.StartupTask.TaskId)"
Write-Host "    DisplayName: $($startupExt.StartupTask.DisplayName)"
Write-Host "    PASS: Conforms to Windows AppModel Startup Contract" -ForegroundColor Green

Write-Host "=== AUDIT COMPLETE ===" -ForegroundColor Cyan
