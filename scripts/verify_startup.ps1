$ErrorActionPreference = 'Stop'

Write-Host "=== VERIFYING DTRAN STARTUP INTEGRATION ===" -ForegroundColor Cyan

# 1. Check Package
$pkg = Get-AppxPackage -Name dTranslate
if (-not $pkg) {
    Write-Error "Package dTranslate not installed!"
}
Write-Host "[1] Package Full Name:" $pkg.PackageFullName
Write-Host "    Package Family Name:" $pkg.PackageFamilyName
Write-Host "    Version:" $pkg.Version

# 2. Check Manifest for StartupTask
$manifestPath = Join-Path $pkg.InstallLocation "AppxManifest.xml"
[xml]$xml = Get-Content $manifestPath
$startupNode = $xml.Package.Applications.Application.Extensions.Extension | Where-Object { $_.Category -eq 'windows.startupTask' }
if ($startupNode) {
    Write-Host "[2] StartupTask found in registered manifest!" -ForegroundColor Green
    Write-Host "    TaskId:" $startupNode.StartupTask.TaskId
    Write-Host "    DisplayName:" $startupNode.StartupTask.DisplayName
    Write-Host "    Enabled:" $startupNode.StartupTask.Enabled
} else {
    Write-Error "StartupTask NOT found in registered manifest!"
}

# 3. Check Windows Registry Registration under AppModel
$regPath = "HKCU:\Software\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\AppModel\SystemAppData\$($pkg.PackageFamilyName)\$($startupNode.StartupTask.TaskId)"
if (Test-Path $regPath) {
    $item = Get-ItemProperty $regPath
    Write-Host "[3] Windows AppModel StartupTask Registry Entry:" -ForegroundColor Green
    Write-Host "    Path: $regPath"
    Write-Host "    State: $($item.State) (2 = Enabled, 0 = Disabled, 1 = DisabledByUser)"
    Write-Host "    UserEnabledStartupOnce: $($item.UserEnabledStartupOnce)"
} else {
    Write-Host "[3] Windows AppModel StartupTask Registry Entry not found yet (will appear when toggled or first run)" -ForegroundColor Yellow
}

# 4. Check HKCU\Run (Must be clean)
$hkcuRun = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$val = (Get-ItemProperty $hkcuRun -ErrorAction SilentlyContinue).dTranslate
if ($val) {
    Write-Host "[4] WARNING: Stale HKCU\Run entry found: $val" -ForegroundColor Red
} else {
    Write-Host "[4] HKCU\Run is clean (no legacy/conflicting dTranslate entry) - PASS" -ForegroundColor Green
}

# 5. Check Process and LocalState Settings
$proc = Get-Process dTranslate -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "[5] Process dTranslate is running (PID: $($proc.Id))" -ForegroundColor Green
} else {
    Write-Host "[5] Process dTranslate is not currently running" -ForegroundColor Yellow
}

$localStateSettings = "$env:LOCALAPPDATA\Packages\$($pkg.PackageFamilyName)\LocalState\settings.json"
if (Test-Path $localStateSettings) {
    Write-Host "[6] LocalState settings.json contents:" -ForegroundColor Cyan
    Get-Content $localStateSettings | Out-String | Write-Host
} else {
    Write-Host "[6] LocalState settings.json does not exist yet" -ForegroundColor Yellow
}

Write-Host "=== VERIFICATION COMPLETE ===" -ForegroundColor Cyan
