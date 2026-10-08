# Diagnostic Script for dTran Package Identity and Activation
# Usage: powershell -ExecutionPolicy Bypass -File scripts\diagnose_package_activation.ps1

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "       dTran PACKAGE ACTIVATION & IDENTITY DIAGNOSTIC     " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Search installed AppX packages for dTran / dTranslate
Write-Host "`n[1] Querying Windows AppModel for dTran package..." -ForegroundColor Yellow
$pkg = Get-AppxPackage | Where-Object { $_.Name -like "*dTranslate*" -or $_.Name -like "*dTran*" }

if (-not $pkg) {
    Write-Host "`n[FAIL] No package matching '*dTranslate*' or '*dTran*' found!" -ForegroundColor Red
    Write-Host "Status: The application package is NOT registered in Windows AppModel." -ForegroundColor Red
    Write-Host "Package            : NOT INSTALLED"
    Write-Host "Package Full Name  : NONE"
    Write-Host "Package Family Name: NONE"
    Write-Host "Application Name   : NONE"
    Write-Host "Application Id     : NONE"
    Write-Host "AUMID              : NONE"
    Write-Host "Install Location   : NONE"
    Write-Host "Executable         : NONE"
    Write-Host "Manifest           : NONE"
} else {
    Write-Host "[OK] Package found in Windows AppModel." -ForegroundColor Green

    $manifestPath = Join-Path $pkg.InstallLocation "AppxManifest.xml"
    $appId = "App"
    $executable = "dTranslate.exe"
    $appName = "dTran"

    if (Test-Path $manifestPath) {
        try {
            [xml]$xml = Get-Content -Path $manifestPath -Raw
            $appNode = $xml.Package.Applications.Application | Select-Object -First 1
            if ($appNode) {
                $appId = $appNode.Id
                $executable = $appNode.Executable
                if ($appNode.VisualElements -and $appNode.VisualElements.DisplayName) {
                    $appName = $appNode.VisualElements.DisplayName
                }
            }
        } catch {
            Write-Warning "Could not parse manifest XML: $_"
        }
    }

    $actualAumid = "$($pkg.PackageFamilyName)!$appId"

    Write-Host "`n--- PACKAGE IDENTITY DETAILS ---" -ForegroundColor Cyan
    Write-Host "Package            : $($pkg.Name)"
    Write-Host "Package Full Name  : $($pkg.PackageFullName)"
    Write-Host "Package Family Name: $($pkg.PackageFamilyName)"
    Write-Host "Application Name   : $appName"
    Write-Host "Application Id     : $appId"
    Write-Host "AUMID              : $actualAumid"
    Write-Host "Install Location   : $($pkg.InstallLocation)"
    Write-Host "Executable         : $executable"
    Write-Host "Manifest           : $manifestPath"
    Write-Host "Publisher          : $($pkg.Publisher)"
    Write-Host "Architecture       : $($pkg.Architecture)"
    Write-Host "Status             : $($pkg.Status)"
    Write-Host "IsDevelopmentMode  : $($pkg.IsDevelopmentMode)"
}

# 2. Check AppsFolder / Start Menu registration
Write-Host "`n[2] Checking Start Menu / shell:AppsFolder..." -ForegroundColor Yellow
$startApps = Get-StartApps | Where-Object { $_.Name -like "*dTran*" -or $_.AppID -like "*dTranslate*" }
if ($startApps) {
    Write-Host "[OK] Found in StartApps:" -ForegroundColor Green
    foreach ($sa in $startApps) {
        Write-Host "  - Name: $($sa.Name) | AppID: $($sa.AppID)"
    }
} else {
    Write-Host "[WARN] Not found in Get-StartApps." -ForegroundColor Yellow
}

# 3. Check Windows App SDK and VCLibs Dependencies
Write-Host "`n[3] Checking Framework Dependencies..." -ForegroundColor Yellow
$winAppSdk = Get-AppxPackage -Name "*WindowsAppRuntime.1.8*" -ErrorAction SilentlyContinue
if ($winAppSdk) {
    Write-Host "  [OK] Windows App Runtime 1.8: $($winAppSdk.PackageFullName)" -ForegroundColor Green
} else {
    Write-Host "  [FAIL] Windows App Runtime 1.8 is NOT INSTALLED!" -ForegroundColor Red
}

$vcLibsDesktop = Get-AppxPackage -Name "*VCLibs.140.00.UWPDesktop*" -ErrorAction SilentlyContinue
if ($vcLibsDesktop) {
    Write-Host "  [OK] VCLibs 140 UWPDesktop  : $($vcLibsDesktop.PackageFullName)" -ForegroundColor Green
} else {
    Write-Host "  [WARN] VCLibs 140 UWPDesktop is NOT INSTALLED!" -ForegroundColor Yellow
}

# 4. Test Direct Windows Application Activation (IApplicationActivationManager)
Write-Host "`n[4] Testing IApplicationActivationManager::ActivateApplication..." -ForegroundColor Yellow
if ($pkg -and $actualAumid) {
    try {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

namespace WinDiag {
    [ComImport, Guid("2e941141-7f97-4756-ba1d-9decde894a3d"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IApplicationActivationManager
    {
        int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, [MarshalAs(UnmanagedType.LPWStr)] string arguments, int options, out uint processId);
        int ActivateForFile([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, IntPtr itemArray, [MarshalAs(UnmanagedType.LPWStr)] string verb, out uint processId);
        int ActivateForProtocol([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, IntPtr itemArray, out uint processId);
    }

    [ComImport, Guid("45BA127D-10A8-46EA-8AB7-56EA9078943C")]
    public class ApplicationActivationManager : IApplicationActivationManager
    {
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        public extern int ActivateApplication([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, [MarshalAs(UnmanagedType.LPWStr)] string arguments, int options, out uint processId);
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        public extern int ActivateForFile([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, IntPtr itemArray, [MarshalAs(UnmanagedType.LPWStr)] string verb, out uint processId);
        [System.Runtime.CompilerServices.MethodImpl(System.Runtime.CompilerServices.MethodImplOptions.InternalCall)]
        public extern int ActivateForProtocol([MarshalAs(UnmanagedType.LPWStr)] string appUserModelId, IntPtr itemArray, out uint processId);
    }

    public static class AppTester {
        public static Tuple<int, uint> TestActivate(string aumid) {
            var mgr = (IApplicationActivationManager)new ApplicationActivationManager();
            uint pid = 0;
            int hr = mgr.ActivateApplication(aumid, "", 0, out pid);
            return new Tuple<int, uint>(hr, pid);
        }
    }
}
"@ -ErrorAction SilentlyContinue

        $res = [WinDiag.AppTester]::TestActivate($actualAumid)
        $hr = $res.Item1
        $targetPid = $res.Item2

        if ($hr -eq 0) {
            Write-Host "  [OK] ActivateApplication SUCCEEDED! HRESULT: 0x00000000 | Started PID: $targetPid" -ForegroundColor Green
            # Give it a moment, verify process
            Start-Sleep -Milliseconds 500
            $proc = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
            if ($proc) {
                Write-Host "  [OK] Process is alive: $($proc.ProcessName) (PID: $targetPid, WorkingSet: $([Math]::Round($proc.WorkingSet64/1MB, 2)) MB)" -ForegroundColor Green
            }
        } else {
            Write-Host "  [FAIL] ActivateApplication failed! HRESULT: 0x$($hr.ToString('X8'))" -ForegroundColor Red
        }
    } catch {
        Write-Host "  [FAIL] COM activation invocation error: $_" -ForegroundColor Red
    }
} else {
    Write-Host "  [SKIP] Cannot test activation because package is not installed." -ForegroundColor Yellow
}

Write-Host "`n==========================================================" -ForegroundColor Cyan
Write-Host "                  DIAGNOSTIC COMPLETED                    " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
