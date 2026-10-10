param(
    [string]$Token = $env:GITHUB_TOKEN,
    [string]$Repo = "denbdenb/dTran",
    [string]$Tag = "v1.2.5",
    [string]$Name = "dTran 1.2.5",
    [string]$NotesPath = "scripts\release_notes_v1.2.5.md"
)

if (-not $Token) {
    throw "GitHub token must be provided via -Token parameter or GITHUB_TOKEN environment variable."
}

$ErrorActionPreference = "Stop"

Write-Host "Publishing release $Tag for $Repo..." -ForegroundColor Cyan

$releaseNotes = Get-Content -Raw -Encoding UTF8 -Path $NotesPath

$payloadObj = [PSCustomObject]@{
    tag_name = $Tag
    target_commitish = "main"
    name = $Name
    body = $releaseNotes
    draft = $false
    prerelease = $false
}

$tempJson = Join-Path $PSScriptRoot "release_payload.json"
$payloadJson = $payloadObj | ConvertTo-Json -Depth 5
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($tempJson, $payloadJson, $utf8NoBom)

$createUrl = "https://api.github.com/repos/$Repo/releases"
Write-Host "Creating release at $createUrl..."

$createRes = & curl.exe -s -S -X POST `
    -H "Authorization: Bearer $Token" `
    -H "Accept: application/vnd.github+json" `
    -H "Content-Type: application/json" `
    --data-binary "@$tempJson" `
    "$createUrl"

Remove-Item -Force $tempJson -ErrorAction SilentlyContinue

$release = $createRes | ConvertFrom-Json
if (-not $release.id) {
    Write-Error "Failed to create release: $createRes"
    exit 1
}

Write-Host "Release created with ID: $($release.id)" -ForegroundColor Green
Write-Host "Release URL: $($release.html_url)" -ForegroundColor Green

$uploadBase = $release.upload_url -replace '\{\?name,label\}', ''

$assets = @(
    @{
        Path = "dist\dTran-1.2.5-x64-Setup.exe"
        Name = "dTran-1.2.5-x64-Setup.exe"
        ContentType = "application/octet-stream"
    },
    @{
        Path = "dist\dTran-1.2.5-x64-Portable.zip"
        Name = "dTran-1.2.5-x64-Portable.zip"
        ContentType = "application/zip"
    },
    @{
        Path = "dist\SHA256SUMS.txt"
        Name = "SHA256SUMS.txt"
        ContentType = "text/plain"
    }
)

foreach ($asset in $assets) {
    if (-not (Test-Path $asset.Path)) {
        throw "Asset file not found: $($asset.Path)"
    }
    
    $assetUrl = "$uploadBase`?name=$($asset.Name)"
    Write-Host "Uploading $($asset.Name) ($((Get-Item $asset.Path).Length) bytes)..." -ForegroundColor Yellow
    
    $uploadRes = & curl.exe -s -S -X POST `
        -H "Authorization: Bearer $Token" `
        -H "Accept: application/vnd.github+json" `
        -H "Content-Type: $($asset.ContentType)" `
        --data-binary "@$($asset.Path)" `
        "$assetUrl"
        
    $uploadJson = $uploadRes | ConvertFrom-Json
    if (-not $uploadJson.id) {
        Write-Error "Failed to upload $($asset.Name): $uploadRes"
        exit 1
    }
    
    Write-Host "Uploaded $($asset.Name) successfully (Asset ID: $($uploadJson.id))!" -ForegroundColor Green
}

Write-Host "`nAll release assets uploaded successfully!" -ForegroundColor Cyan
Write-Host "Release live at: $($release.html_url)" -ForegroundColor Green
