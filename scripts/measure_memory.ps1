$proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    Write-Host "dTranslate is not running. Launching via run.ps1..."
    & .\run.ps1 -Configuration Release
    Start-Sleep -Seconds 2
    $proc = Get-Process dTranslate -ErrorAction SilentlyContinue | Select-Object -First 1
}

if ($proc) {
    $ws = [math]::Round($proc.WorkingSet64 / 1MB, 2)
    $pm = [math]::Round($proc.PrivateMemorySize64 / 1MB, 2)
    $vm = [math]::Round($proc.VirtualMemorySize64 / 1MB, 2)
    $npm = [math]::Round($proc.NonpagedSystemMemorySize64 / 1KB, 2)
    $paged = [math]::Round($proc.PagedMemorySize64 / 1MB, 2)

    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "   dTranslate Process Memory Metrics      " -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "Process ID:          $($proc.Id)"
    Write-Host "Private Bytes (Commit): $pm MB" -ForegroundColor Green
    Write-Host "Working Set (Total):    $ws MB" -ForegroundColor Yellow
    Write-Host "Paged Memory:           $paged MB"
    Write-Host "Non-paged Pool:         $npm KB"
    Write-Host "Virtual Memory Size:    $vm MB"
    Write-Host "==========================================" -ForegroundColor Cyan
} else {
    Write-Host "Error: Could not find or launch dTranslate." -ForegroundColor Red
}
