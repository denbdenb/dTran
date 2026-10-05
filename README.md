# dTranslate

A small, fast, native **Windows 11** translator. Select text in any app, press a
hotkey, read the translation in a compact popup.

- Native C++20 / WinUI 3 (no .NET runtime, no Electron, no browser engine)
- Google Translate and Yandex Translate; Gemini and OpenAI for AI
  (translate, rewrite, improve, summarize, explain)
- Dictionary (Reverso, Wikipedia), text-to-speech (Google), OCR (Windows)
- Built for low memory use and instant startup
- Not a plugin platform: a fixed set of services, done well

UI concept: [docs/ui-concept.jpg](docs/ui-concept.jpg)

## Status

Phase 1 of 20 — **foundation and WinUI 3 shell** (empty Mica window).
Features are added phase by phase; see the phase list in [AGENTS.md](AGENTS.md).

## Build and run

Requirements and details: [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).

```powershell
.\build.ps1      # build (Debug)
.\run.ps1        # install the dev build for your user and start it
```

## Documentation

| File | Contents |
|---|---|
| [AGENTS.md](AGENTS.md) | The authoritative project specification and rules |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | How the app is structured |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) | Requirements, build, run, tests, credentials, manual tests |
| [docs/PERFORMANCE.md](docs/PERFORMANCE.md) | How memory/CPU/latency are measured, and results |

## Security

API keys are entered in the app's Settings and stored with Windows Credential
Manager. They are never written to source code, config files, logs or Git.
