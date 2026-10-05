# Registers the freshly built dTranslate package for the current user and starts it.
# Usage:  .\run.ps1            (Debug build)
#         .\run.ps1 -Configuration Release
param(
    [ValidateSet('Debug', 'Release')] [string]$Configuration = 'Debug'
)
$ErrorActionPreference = 'Stop'
$layout = Join-Path $PSScriptRoot "build\bin\x64\$Configuration"
$manifest = Join-Path $layout 'AppxManifest.xml'
if (-not (Test-Path $manifest)) { throw "Nothing built yet. Run .\build.ps1 -Configuration $Configuration first." }

Get-Process dTranslate -ErrorAction SilentlyContinue | Stop-Process -Force
Add-AppxPackage -Register $manifest -ForceApplicationShutdown
$pkg = Get-AppxPackage -Name dTranslate
if (-not $pkg) { throw 'Package registration failed. Is Developer Mode enabled?' }
Start-Process "shell:AppsFolder\$($pkg.PackageFamilyName)!App"
Write-Host "Started dTranslate $($pkg.Version)" -ForegroundColor Green
