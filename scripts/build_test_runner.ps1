$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsPath = & $vswhere -latest -products * -property installationPath
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'

$sources = @(
    "tests\TestRunner.cpp",
    "src\Translation\TextChunker.cpp",
    "src\Translation\LanguageCatalog.cpp",
    "src\Translation\GoogleTranslateService.cpp",
    "src\Translation\YandexTranslateService.cpp",
    "src\Audio\GoogleTtsService.cpp",
    "src\OCR\WindowsOcrService.cpp",
    "src\Networking\HttpClient.cpp",
    "src\Storage\SettingsManager.cpp",
    "src\Storage\HistoryManager.cpp",
    "src\Storage\LocalizationManager.cpp",
    "src\Windows\ClipboardHelper.cpp",
    "src\Windows\SelectionCapture.cpp"
) -join " "

$includes = "/I src\App /I `"src\App\Generated Files`" /I `"build\obj\x64\Release`" /I src\Translation /I src\Audio /I src\OCR /I src\Networking /I src\Storage /I src\Windows"
$libs = "winhttp.lib winmm.lib crypt32.lib windowsapp.lib shell32.lib user32.lib"
$out = "build\bin\x64\Release\TestRunner.exe"

$objDir = "build\obj\x64\Release\"
if (-not (Test-Path $objDir)) { New-Item -ItemType Directory -Path $objDir -Force | Out-Null }
$cmd = "`"$vcvars`" && cl /nologo /std:c++20 /EHsc /utf-8 /MD /DWIN32_LEAN_AND_MEAN /DWINRT_LEAN_AND_MEAN /DNDEBUG /Fo`"$objDir`" $includes $sources /link /out:$out $libs"

cmd.exe /c $cmd
if ($LASTEXITCODE -eq 0) {
    Write-Host "TestRunner built successfully!" -ForegroundColor Green
    & $out
} else {
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}
