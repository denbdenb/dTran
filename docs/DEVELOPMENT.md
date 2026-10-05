# Development

## Requirements

All free. Windows 11 (x64) with:

| Requirement | Why | How to install |
|---|---|---|
| Visual Studio Build Tools 2022 with "Desktop development with C++" and the Windows 11 SDK (10.0.26100) | C++ compiler, MSBuild, Windows headers | `winget install Microsoft.VisualStudio.2022.BuildTools --override "--passive --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.Windows11SDK.26100 --includeRecommended"` (needs admin) |
| Developer Mode | Lets Windows install the dev build of the app | Settings → System → For developers → Developer Mode: On |
| Git | Version control | `winget install Git.Git` |
| Internet on first build | NuGet downloads the Windows App SDK and C++/WinRT | automatic |

The .NET SDK is *not* required to build the app (the app itself has no .NET runtime).

## Build and run

```powershell
.\build.ps1                       # Debug build
.\build.ps1 -Configuration Release
.\run.ps1                         # register the build for your user and start it
```

`build.ps1` finds MSBuild itself and restores NuGet packages automatically.
Visual Studio (any edition with the C++ workload) can also open `dTranslate.sln`.

## Troubleshooting

| Symptom | Fix |
|---|---|
| `Package registration failed` | Turn on Developer Mode (see above) |
| `0x80073CF9` / "Failed to reach state Staged" | Move the repository to a shorter path, e.g. `C:\src\dTranslate` |
| Build says Windows SDK not found | Re-run the Build Tools installer and tick "Windows 11 SDK" |

## Services and credentials

(Implemented in later phases; this is the design.)

- Each service has an **Enabled** switch and its settings in Settings.
- API keys are typed into Settings and stored in **Windows Credential Manager**
  (target names `dTranslate/<service>`), never in settings files, source, logs or Git.
- Non-secret settings live in `%LOCALAPPDATA%\dTranslate\settings.json`.
- Official APIs only: Google Cloud Translation, Yandex Cloud Translate,
  Gemini API, OpenAI API, Google Cloud Text-to-Speech, Wikimedia API; Reverso minimal.

## Automated tests

Tests will live in `tests/` (a plain console test executable with no third-party
framework, built by the same solution). They arrive together with the code they
cover (settings, language selection, HTTP errors, response parsing, history).

## Manual test procedures

Run after relevant changes. Record failures as issues.

| Area | Steps | Expected |
|---|---|---|
| Global hotkey | Start app, focus Notepad, press the hotkey | Popup appears; hotkey is not swallowed when app is not running |
| Selected text | Select a sentence in Notepad, Edge, Word; press hotkey | Popup shows the translation of the selection |
| Clipboard fallback | Select text in an app without UI Automation text support; press hotkey | Translation appears; your clipboard content is restored afterwards |
| Clipboard translation | Copy text; use "Translate clipboard" | Translation shown |
| Popup | Open, press Esc; open, click elsewhere; switch service; switch language; copy; speak | Esc closes; no focus theft from the source app; all actions work |
| Tray | Right-click the tray icon | Menu: Open, Translate clipboard, Settings, Exit; Exit removes the icon |
| OCR | Use OCR on an image with text | Recognized text appears; clear message on failure |
| TTS | Press speak on a translation | Audio plays; clear message without a key |
| Theme | Switch Light / Dark / System; change Windows theme | UI follows, no unreadable text |
| Settings persistence | Change settings, close, restart | Values persist; secrets not visible in settings.json |
| Offline | Disable network, translate | Friendly "no connection" message, UI stays responsive |

## Phase 1 checks

- `build.ps1` succeeds.
- `run.ps1` shows a window titled dTranslate with a Mica background and the text
  "Foundation shell is running."
