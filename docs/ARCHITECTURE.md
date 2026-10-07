# Architecture

Deliberately simple native architecture for **dTran**. Authoritative rules: [AGENTS.md](../AGENTS.md).

```
dTran (dTranslate.exe)  (one packaged WinUI 3 / C++/WinRT process)
├── UI              src/UI
│   ├── QuickPopup      Instant mouse-anchored popup for selected text / clipboard / typing (Primary UI)
│   ├── Settings        Single-page fixed-size 480x560 settings window (no sidebar)
│   └── SnippingOverlay Virtual-screen dimmed crosshair overlay for screen OCR
├── OCR             src/OCR
│   ├── ScreenSnipper   Virtual-screen BitBlt, rectangular crop, action toolbar
│   └── WindowsOcr      Windows.Media.Ocr engine integration for clipboard & snipped bitmaps
├── Translation     src/Translation
│   ├── LanguageCatalog 92 standard ISO languages, capability masks, text badges
│   ├── GoogleTranslate Web client (no key required, 429 backoff & circuit breaker)
│   ├── YandexTranslate Web client (no key required; browser instaserp endpoint + Mozhi/Trayslate fallback)
│   └── TranslationMgr  Dual-engine orchestration & side-by-side comparison
├── Audio (TTS)     src/Audio
│   └── GoogleTts       Sequential chunked MP3 audio playback with instant MCI alias halt
├── Networking      src/Networking
│   ├── HttpClient      Shared WinHTTP session, connection pooling, SSL/TLS, POST form/JSON
│   └── UrlEncoder      RFC 3986 UTF-8 query and form URL encoding
├── Storage         src/Storage
│   ├── SettingsManager JSON persistence in LocalAppData
│   └── HistoryManager  Bounded JSON translation history
└── Windows         src/Windows
    ├── TrayIcon        Shell_NotifyIconW with context menu
    ├── Hotkeys         RegisterHotKey (Ctrl+Alt+T, Ctrl+Alt+O)
    ├── Selection       UI Automation TextPattern + clipboard simulation capture
    └── Replacement     In-place text replacement in source window with clipboard backup/restore
```

## Architectural Principles

1. **Popup-First & System Tray Residence**:
   - The application starts quietly in the system tray. No separate main window exists.
   - Global hotkey `Ctrl+Alt+T` or tray click opens the popup translator.
   - Closing the popup with `[X]` hides it back to the tray; only the tray context menu exits the process.
   - Single-instance enforcement via named mutex (`dTran_SingleInstance_Mutex`) ensures a second launch wakes the running popup.

2. **No-Config Built-in Web Translation**:
   - Google Translate and Yandex Translate are fixed built-in engines requiring zero API keys or user configuration.
   - Both engines support automatic language detection, failover, and throttling backoff.

3. **In-Place Text Replacement**:
   - When text is selected in an external application and translated, clicking the Replace button (`PopupReplaceBtn`) pastes the translation directly into the target window over the selected text.
   - The user's previous clipboard contents are backed up and restored.

4. **Speech Playback with Immediate Abort**:
   - Google TTS handles long text by chunking along punctuation boundaries (<= 160 characters).
   - Audio is played sequentially via Windows MCI. Clicking the speaker icon during playback aborts instantly.

5. **Screen-Snipping OCR Workflow**:
   - `Ctrl+Alt+O` or the top OCR button launches the native `ScreenSnipper`.
   - Recognized text is processed by `Windows.Media.Ocr` and automatically populated in the source text field.

6. **Ultra-Lean Resource Profile**:
   - All legacy AI and Dictionary dependencies and code paths have been removed.
   - Idle background memory is ~14 MB Private Bytes / ~42 MB Working Set.
