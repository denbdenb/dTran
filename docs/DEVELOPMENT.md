# Development

## Requirements

Windows 11 (x64) with:

| Requirement | Why | How to install |
|---|---|---|
| Visual Studio Build Tools 2022 with "Desktop development with C++" and Windows 11 SDK (10.0.26100) | C++ compiler, MSBuild, Windows App SDK headers | `winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --includeRecommended"` (needs admin) |
| Developer Mode | Lets Windows register packaged MSIX apps | Settings → System → For developers → Developer Mode: On |
| Git | Version control | `winget install Git.Git` |

## Build and Run

```powershell
# Build Debug or Release
.\build.ps1 -Configuration Release

# Run automated tests (TestRunner)
powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1 -Configuration Release

# Register and launch dTran
.\run.ps1 -Configuration Release

# Check live memory footprint
powershell -ExecutionPolicy Bypass -File .\scripts\measure_memory.ps1
```

## Testing

The project includes an automated test runner at `tests/TestRunner.cpp` covering:
1. Language Catalog & ISO-639 mapping (BCP-47 tags, flags, capability filtering)
2. Google TTS sequential chunking and immediate alias stop control
3. Windows OCR capabilities and BCP-47 locale matching
4. Live translation engine queries for Google Translate and Yandex Translate
5. Settings persistence and live observer events
6. HistoryManager storage, search, and bound eviction
7. ClipboardHelper and Selection Replacement safety

Execute via:
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1 -Configuration Release
```
All 35 tests pass deterministically.
