# Package Registration and Dependency Provisioning Script for dTran
param(
    [Parameter(Mandatory=$true)]
    [string]$InstallDir
)

$ErrorActionPreference = 'Stop'

Write-Host "=========================================================="
Write-Host "  dTran Deployment & AppModel Registration"
Write-Host "  InstallDir: $InstallDir"
Write-Host "=========================================================="

# 1. Grant Read and Execute permissions to ALL APPLICATION PACKAGES
# This allows Windows AppModel to access assets and binaries in Program Files
try {
    Write-Host "[1/4] Configuring folder ACL for ALL APPLICATION PACKAGES..."
    & icacls "$InstallDir" /grant "*S-1-15-2-1:(OI)(CI)RX" /grant "Administrators:(OI)(CI)F" /grant "Users:(OI)(CI)RX" /T /Q | Out-Null
    Write-Host "      Folder permissions configured successfully."
} catch {
    Write-Warning "Failed to set ACL via icacls: $_"
}

# 2. Check and provision Windows App Runtime 1.8
try {
    Write-Host "[2/4] Verifying Windows App Runtime 1.8 framework..."
    $winAppSdk = Get-AppxPackage -Name "Microsoft.WindowsAppRuntime.1.8" -ErrorAction SilentlyContinue
    if (-not $winAppSdk) {
        $depMsix = Join-Path $InstallDir "Dependencies\Microsoft.WindowsAppRuntime.1.8.msix"
        if (Test-Path $depMsix) {
            Write-Host "      Installing Windows App Runtime 1.8 from $depMsix..."
            Add-AppxPackage -Path $depMsix -ForceApplicationShutdown
            Write-Host "      Windows App Runtime 1.8 installed successfully."
        } else {
            Write-Warning "      Microsoft.WindowsAppRuntime.1.8 is not installed and bundled package was not found."
        }
    } else {
        Write-Host "      Windows App Runtime 1.8 is already installed."
    }
} catch {
    Write-Warning "Error provisioning Windows App Runtime: $_"
}

# 3. Check and provision VCLibs 140 UWPDesktop
try {
    Write-Host "[3/4] Verifying VCLibs framework..."
    $vcLibs = Get-AppxPackage -Name "Microsoft.VCLibs.140.00.UWPDesktop" -ErrorAction SilentlyContinue
    if (-not $vcLibs) {
        $depVc = Join-Path $InstallDir "Dependencies\Microsoft.VCLibs.x64.14.00.Desktop.appx"
        if (Test-Path $depVc) {
            Write-Host "      Installing VCLibs Desktop from $depVc..."
            Add-AppxPackage -Path $depVc -ForceApplicationShutdown
            Write-Host "      VCLibs Desktop installed successfully."
        }
    } else {
        Write-Host "      VCLibs Desktop is already installed."
    }
} catch {
    Write-Warning "Error provisioning VCLibs: $_"
}

# 4. Register dTranslate package layout in Windows AppModel
Write-Host "[4/4] Registering dTranslate AppxManifest.xml..."
$manifestPath = Join-Path $InstallDir "AppxManifest.xml"
if (-not (Test-Path $manifestPath)) {
    throw "AppxManifest.xml not found at: $manifestPath"
}

Add-AppxPackage -Register $manifestPath -ForceApplicationShutdown

# 5. Validate that package is present and readable
$pkg = Get-AppxPackage -Name "dTranslate" -ErrorAction SilentlyContinue
if (-not $pkg) {
    throw "Package registration failed: 'dTranslate' was not found in Windows AppModel after registration."
}

Write-Host "=========================================================="
Write-Host "  Registration SUCCESSFUL!"
Write-Host "  PackageFullName : $($pkg.PackageFullName)"
Write-Host "  InstallLocation : $($pkg.InstallLocation)"
Write-Host "=========================================================="
exit 0
