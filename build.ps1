# Builds dTranslate. Usage:  .\build.ps1            (Debug)
#                          .\build.ps1 -Configuration Release
param(
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Debug'
)
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'Visual Studio Build Tools not found. See docs/DEVELOPMENT.md (Requirements).'
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild `
    -find 'MSBuild\**\Bin\amd64\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild not found. See docs/DEVELOPMENT.md (Requirements).' }

Write-Host "Using $msbuild"
Stop-Process -Name dTranslate -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 200
& $msbuild dTranslate.sln -restore -m:2 -nologo -v:minimal `
    -p:PreferredToolArchitecture=x64 `
    -p:Configuration=$Configuration -p:Platform=x64
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit code $LASTEXITCODE)." }
Write-Host "Build OK: build\bin\x64\$Configuration" -ForegroundColor Green
