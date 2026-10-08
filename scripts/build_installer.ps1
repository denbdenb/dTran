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
Write-Host "`n[1/7] Building dTran Release x64..." -ForegroundColor Yellow
& powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release
if ($LASTEXITCODE -ne 0) { throw "Release build failed with exit code $LASTEXITCODE" }

# 2. Compile Lightweight dTranLauncher.exe
Write-Host "`n[2/7] Compiling dTranLauncher.exe..." -ForegroundColor Yellow
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    $vcvars = Get-ChildItem "C:\Program Files*\Microsoft Visual Studio\*\*\VC\Auxiliary\Build\vcvars64.bat" | Select-Object -First 1 -ExpandProperty FullName
}
cmd /c "call `"$vcvars`" && rc /fo build\obj\dTranLauncher.res src\Launcher\dTranLauncher.rc && cl /nologo /O2 /std:c++20 /EHsc /W4 src\Launcher\dTranLauncher.cpp build\obj\dTranLauncher.res /link /SUBSYSTEM:WINDOWS /OUT:build\bin\x64\Release\dTranLauncher.exe"
if ($LASTEXITCODE -ne 0) { throw "dTranLauncher compilation failed with exit code $LASTEXITCODE" }
Write-Host "dTranLauncher.exe compiled successfully." -ForegroundColor Green

# 3. Run Test Suite via build_test_runner.ps1
Write-Host "`n[3/7] Running automated test suite..." -ForegroundColor Yellow
& powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1
if ($LASTEXITCODE -ne 0) { throw "Automated test suite failed with exit code $LASTEXITCODE" }

# 4. Create Clean Staging Layout
Write-Host "`n[4/7] Staging runtime files in build\layout\x64\Release..." -ForegroundColor Yellow
$binDir = Join-Path $repoRoot "build\bin\x64\Release"
$stageDir = Join-Path $repoRoot "build\layout\x64\Release"
$distDir = Join-Path $repoRoot "dist"

if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
if (-not (Test-Path $distDir)) { New-Item -ItemType Directory -Path $distDir | Out-Null }
New-Item -ItemType Directory -Path $stageDir | Out-Null
New-Item -ItemType Directory -Path "$stageDir\Assets" | Out-Null

$payloadFiles = @(
    "dTranslate.exe",
    "dTranLauncher.exe",
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

# 5. Build MSIX Package (app payload only, dependencies referenced via manifest)
Write-Host "`n[5/7] Building MSIX package via makeappx.exe..." -ForegroundColor Yellow
$makeappx = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\makeappx.exe"
if (-not (Test-Path $makeappx)) {
    $makeappx = Get-ChildItem "C:\Program Files (x86)\Windows Kits\10\bin" -Recurse -Filter "makeappx.exe" -ErrorAction SilentlyContinue | Select-Object -First 1 -ExpandProperty FullName
}
$msixOut = Join-Path $distDir "dTran-1.0.0-x64.msix"
& $makeappx pack /d $stageDir /p $msixOut /o
if ($LASTEXITCODE -ne 0) { throw "makeappx failed with exit code $LASTEXITCODE" }
$msixSize = [Math]::Round((Get-Item $msixOut).Length / 1MB, 2)
Write-Host "MSIX generated: $msixOut ($msixSize MB)" -ForegroundColor Green

# Stage runtime framework dependencies for the Inno Setup installer
Write-Host "`nStaging framework dependencies for Inno Setup installer..." -ForegroundColor Yellow
$depDir = Join-Path $stageDir "Dependencies"
New-Item -ItemType Directory -Path $depDir -Force | Out-Null

$winAppMsix = "C:\Users\denb\.nuget\packages\microsoft.windowsappsdk.runtime\1.8.260921001\tools\MSIX\win10-x64\Microsoft.WindowsAppRuntime.1.8.msix"
if (Test-Path $winAppMsix) {
    Copy-Item $winAppMsix $depDir -Force
    Write-Host "  Staged Microsoft.WindowsAppRuntime.1.8.msix" -ForegroundColor Green
} else {
    Write-Warning "  Microsoft.WindowsAppRuntime.1.8.msix not found in NuGet cache!"
}

$vcLibAppx = "C:\Program Files (x86)\Microsoft SDKs\Windows Kits\10\ExtensionSDKs\Microsoft.VCLibs.Desktop\14.0\Appx\Retail\x64\Microsoft.VCLibs.x64.14.00.Desktop.appx"
if (Test-Path $vcLibAppx) {
    Copy-Item $vcLibAppx $depDir -Force
    Write-Host "  Staged Microsoft.VCLibs.x64.14.00.Desktop.appx" -ForegroundColor Green
}

# 6. Compile Inno Setup Installer
Write-Host "`n[6/7] Compiling Setup.exe via Inno Setup ISCC..." -ForegroundColor Yellow
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

# 7. Generate Layout ZIP and Checksums
Write-Host "`n[7/7] Creating package archive and computing SHA-256 checksums..." -ForegroundColor Yellow
$zipOut = Join-Path $distDir "dTran-1.0.0-x64-Layout.zip"
if (Test-Path $zipOut) { Remove-Item $zipOut -Force }
# Archive core layout (without heavy dependencies)
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
Get-Content $checksumFile

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "       dTran 1.0.0 - BUILD COMPLETED SUCCESSFULLY         " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
