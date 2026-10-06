# Architecture

Deliberately simple native architecture. Authoritative rules: [AGENTS.md](../AGENTS.md).

```
dTranslate.exe  (one packaged WinUI 3 / C++/WinRT process)
├── UI              src/UI
│   ├── MainWindow      Mica, collapsible sidebar, simplified translation cards, OCR snipping button
│   ├── QuickPopup      Instant mouse-anchored popup for selected text / clipboard
│   ├── Settings        General, Translation, AI, Hotkeys, About tabs
│   ├── AI              Rewrite, Improve, Summarize, Explain operations
│   ├── Dictionary      Reverso Context and Wikipedia lookup
│   └── History         Disk-backed search and copy
├── OCR             src/OCR
│   ├── ScreenSnipper   Virtual-screen BitBlt, dimmed crosshair overlay, rectangular crop, action toolbar
│   └── WindowsOcr      Windows.Media.Ocr engine integration for clipboard & snipped bitmaps
├── Translation     src/Translation
│   ├── LanguageCatalog 92 standard ISO languages, capability masks, text badges
│   ├── GoogleTranslate Web client (no key required, 429 backoff & circuit breaker)
│   ├── YandexTranslate Web client (no key required; browser instaserp endpoint + Mozhi/Trayslate fallback)
│   └── TranslationMgr  Service orchestration & multi-engine fallback
├── AI Services     src/AI
│   ├── GeminiService   Google Gemini API (gemini-2.5-flash) with Windows Credential Manager storage
│   └── OpenAIService   OpenAI API (gpt-4o-mini) with Windows Credential Manager storage
├── Audio (TTS)     src/Audio
│   └── GoogleTts       Direct streaming MP3 audio playback
├── Networking      src/Networking
│   ├── HttpClient      Shared WinHTTP session, connection pooling, SSL/TLS, POST form/JSON
│   └── UrlEncoder      RFC 3986 UTF-8 query and form URL encoding
├── Storage         src/Storage
│   ├── CredentialStore Windows Credential Manager API (wincred.h) for AI secrets
│   ├── SettingsManager JSON persistence in LocalAppData
│   └── HistoryManager  Bounded JSON translation history
└── Windows         src/Windows
    ├── TrayIcon        Shell_NotifyIconW with context menu
    ├── Hotkeys         RegisterHotKey (Ctrl+Alt+T, Ctrl+Alt+D, Ctrl+Alt+O)
    ├── Selection       SendInput Ctrl+C / clipboard capture
    └── Clipboard       Win32 clipboard access
```

## Architectural Principles

1. **No-Config Built-in Web Translation**:
   - Google Translate and Yandex Translate are fixed built-in engines requiring zero API keys or user configuration.
   - Yandex uses the lightweight browser endpoint `https://api.browser.yandex.com/instaserp/translate` (POST form) with fallbacks.
   - Built-in engines have no enable/disable toggles; they are selectable via `[ Google ] [ Yandex ] [ Gemini ] [ OpenAI ]`.
2. **AI Services via Windows Credential Manager**:
   - Gemini and OpenAI require API keys stored securely encrypted via Windows Credential Manager (`wincred.h`).
   - Settings → AI handles credential configuration, model selection, and connection testing.
3. **Built-in Speech Action**:
   - Google TTS is an inline speech action (🔊 button) on source and translation cards, not a separate service card or switch.
4. **Screen-Snipping OCR Workflow**:
   - Top-right `[ ⛶ ]` button and `Ctrl+Alt+O` hotkey launch the native `ScreenSnipper`.
   - Dimmed crosshair overlay with rubberband selection and floating action bar: `[ Translate ] [ Copy Text ] [ Cancel ]`.
   - Recognized text is processed by `Windows.Media.Ocr` and automatically translated with the active engine.
5. **Zero Plugins, Zero WebViews**:
   - Entirely native C++20 and C++/WinRT with WinUI 3 controls.
   - Strictly obeys the memory footprint budget (< 100 MB target, measured at ~44 MB private bytes).
