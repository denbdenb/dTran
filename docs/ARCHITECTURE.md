# Architecture

Deliberately simple. Authoritative rules: [AGENTS.md](../AGENTS.md).

```
dTranslate.exe  (one packaged WinUI 3 / C++/WinRT process)
├── UI              src/UI/{MainWindow,QuickPopup,Settings,History,Dictionary,AI}
├── Core            src/Core         orchestration: translate / AI / dictionary / TTS / OCR
├── Windows         src/Windows      tray, global hotkeys, clipboard, selected-text capture
├── Services        src/Translation  GoogleTranslateService, YandexTranslateService
│                   src/AI           GeminiService, OpenAIService (one shared AI request path)
│                   src/Dictionary   ReversoService, WikipediaService
│                   src/Speech       GoogleTtsService
│                   src/OCR          WindowsOcrService
├── Networking      src/Networking   ONE shared WinHTTP implementation
├── Storage         src/Storage      settings (local file) + credentials (Credential Manager)
├── History         src/History      bounded, disk-backed
└── App             src/App          entry point, project file, manifests
```

## Rules of thumb

- Explicit service classes, no plugin/provider-manager layer, no DI container.
- Services are created lazily, the first time they are used.
- No network access at startup.
- One WinHTTP session shared by every service.
- History is read from disk on demand; only a small recent window lives in memory.

## Current state (Phase 1)

Only `src/App` and `src/UI/MainWindow` contain code: an application class and a
placeholder window (Mica backdrop, custom title bar). Other folders are
placeholders for later phases.

## Build layout

- `src/App/dTranslate.vcxproj` is the only project; the XAML and sources in
  `src/UI/**` are compiled into it by relative path.
- Output goes to `build/` (git-ignored): `build/bin/x64/<Config>` (package layout)
  and `build/obj/...` (intermediates).
- NuGet packages (only two): `Microsoft.Windows.CppWinRT`, `Microsoft.WindowsAppSDK`.
- Packaged as MSIX (single-project). Dev runs register the loose layout via `run.ps1`.

## Dependencies

| Dependency | Why | Memory/runtime impact |
|---|---|---|
| Windows App SDK 1.8 (framework package) | WinUI 3 UI, Mica, windowing | Shared framework package; required by WinUI 3 |
| C++/WinRT | Header-only projection of WinRT APIs | None at runtime |
