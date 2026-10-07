# Automates the full Release build, test execution, packaging, and checksum generation for dTran 1.0.0
param(
    [ValidateSet('Release')] [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
$repoRoot = $PSScriptRoot | Split-Path -Parent
Set-Location $repoRoot

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       dTran 1.0.0 - FULL RELEASE AND INSTALLER BUILD     " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Build Solution via build.ps1
Write-Host "`n[1/6] Building dTran Release x64..." -ForegroundColor Yellow
& powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release
if ($LASTEXITCODE -ne 0) { throw "Release build failed with exit code $LASTEXITCODE" }

# 2. Run Test Suite via build_test_runner.ps1
Write-Host "`n[2/6] Running automated test suite..." -ForegroundColor Yellow
& powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1
if ($LASTEXITCODE -ne 0) { throw "Automated test suite failed with exit code $LASTEXITCODE" }

# 3. Create Clean Staging Layout
Write-Host "`n[3/6] Staging runtime files in build\layout\x64\Release..." -ForegroundColor Yellow
$binDir = Join-Path $repoRoot "build\bin\x64\Release"
$stageDir = Join-Path $repoRoot "build\layout\x64\Release"
$distDir = Join-Path $repoRoot "dist"

if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
if (-not (Test-Path $distDir)) { New-Item -ItemType Directory -Path $distDir | Out-Null }
New-Item -ItemType Directory -Path $stageDir | Out-Null
New-Item -ItemType Directory -Path "$stageDir\Assets" | Out-Null

$payloadFiles = @(
    "dTranslate.exe",
    "AppxManifest.xml",
    "resources.pri",
    "App.xbf",
    "QuickPopupWindow.xbf",
    "SettingsWindow.xbf",
    "Microsoft.Web.WebView2.Core.dll",
    "Microsoft.Web.WebView2.Core.winmd"
)

foreach ($f in $payloadFiles) {
    Copy-Item (Join-Path $binDir $f) $stageDir -Force
}
Copy-Item (Join-Path $binDir "Assets\*") "$stageDir\Assets" -Recurse -Force
Copy-Item (Join-Path $repoRoot "assets\app.ico") "$stageDir\Assets\app.ico" -Force
Write-Host "Staging layout verified." -ForegroundColor Green

# 4. Build MSIX Package
Write-Host "`n[4/6] Building MSIX package via makeappx.exe..." -ForegroundColor Yellow
$makeappx = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\makeappx.exe"
if (-not (Test-Path $makeappx)) {
    $makeappx = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Recurse -Filter "makeappx.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
$msixOut = Join-Path $distDir "dTran-1.0.0-x64.msix"
& $makeappx pack /d $stageDir /p $msixOut /o
if ($LASTEXITCODE -ne 0) { throw "makeappx failed with exit code $LASTEXITCODE" }
$msixSize = [Math]::Round((Get-Item $msixOut).Length / 1MB, 2)
Write-Host "MSIX generated: $msixOut ($msixSize MB)" -ForegroundColor Green

# 5. Compile Inno Setup Installer
Write-Host "`n[5/6] Compiling Setup.exe via Inno Setup ISCC..." -ForegroundColor Yellow
$iscc = "C:\Users\denb\AppData\Local\Programs\Inno Setup 6\ISCC.exe"
if (-not (Test-Path $iscc)) {
    $iscc = Get-ChildItem -Path "C:\Program Files (x86)\Inno Setup*", "C:\Program Files\Inno Setup*", "$env:LOCALAPPDATA\Programs\Inno Setup*" -Filter "ISCC.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $iscc -or -not (Test-Path $iscc)) {
    throw "Inno Setup compiler (ISCC.exe) not found."
}
$issFile = Join-Path $repoRoot "installer\dTran-Setup.iss"
& $iscc $issFile
if ($LASTEXITCODE -ne 0) { throw "ISCC compilation failed with exit code $LASTEXITCODE" }
$setupOut = Join-Path $distDir "dTran-1.0.0-x64-Setup.exe"
$setupSize = [Math]::Round((Get-Item $setupOut).Length / 1MB, 2)
Write-Host "Setup generated: $setupOut ($setupSize MB)" -ForegroundColor Green

# 6. Generate Layout ZIP and Checksums
Write-Host "`n[6/6] Creating package archive and computing SHA-256 checksums..." -ForegroundColor Yellow
$zipOut = Join-Path $distDir "dTran-1.0.0-x64-Layout.zip"
if (Test-Path $zipOut) { Remove-Item $zipOut -Force }
Compress-Archive -Path "$stageDir\*" -DestinationPath $zipOut -CompressionLevel Optimal
$zipSize = [Math]::Round((Get-Item $zipOut).Length / 1MB, 2)
Write-Host "Archive generated: $zipOut ($zipSize MB)" -ForegroundColor Green

$checksumFile = Join-Path $distDir "SHA256SUMS.txt"
$artifacts = @("dTran-1.0.0-x64-Setup.exe", "dTran-1.0.0-x64.msix", "dTran-1.0.0-x64-Layout.zip")
$lines = @()
foreach ($art in $artifacts) {
    $artPath = Join-Path $distDir $art
    if (Test-Path $artPath) {
        $hash = (Get-FileHash -Path $artPath -Algorithm SHA256).Hash.ToLowerInvariant()
        $lines += "$hash  $art"
    }
}
$lines | Set-Content -Path $checksumFile -Encoding utf8
Write-Host "`nChecksums written to:" $checksumFile -ForegroundColor Cyan
Get-Content $checksumFile | Write-Host

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "       BUILD, PACKAGING AND RELEASE ARTIFACTS COMPLETE     " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
