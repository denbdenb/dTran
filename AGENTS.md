# AGENTS.md — dTran project specification

This file is the **authoritative specification** for **dTran** (formerly dTranslate). Every contributor
(human or AI) must read it before working and must not silently weaken, remove or reinterpret any requirement.

## 0. Working with the user

The user is **not an experienced programmer**.

- Create files, folders, configs, build setup, git repo and code yourself. Never ask the user to create boilerplate.
- Perform safe setup automatically. Ask before anything needing admin rights (UAC), large downloads, or machine-wide changes.
- Use plain language. Don't assume knowledge of C++, Visual Studio, MSBuild, Git, WinUI, or the Windows SDK.

## 1. Product goal

A modern, ultralight, native Windows 11 system popup translation utility named **dTran**.

It is intentionally **NOT a plugin platform** and NOT a heavy multi-window assistant. Small, focused, fast, visually polished, resource-efficient.

Philosophy: **"Do a small number of things extremely well."**

Primary goals:
1. **Popup-first**: The popup translator is the exclusive primary user interface.
2. **Tray-resident & instant**: Starts quietly in the system tray without popping up windows; wakes instantly via global hotkey (`Ctrl+Alt+T`) or tray icon.
3. **No-config translation**: Google Translate and Yandex Translate work out of the box via reliable web endpoints without requiring API keys.
4. **In-place selection replacement**: Replace original text in the source application with translated text while safely preserving user clipboard.
5. **Screen OCR & TTS**: Windows OCR (`Windows.Media.Ocr`) snip-and-translate (`Ctrl+Alt+O`) and sequential chunked speech playback with instant abort.
6. **Low memory & fast startup**: Idle private memory < 20 MB; working set < 50 MB.

Non-goals:
- AI assistants, chatbots, LLM rewriting/summarization (completely purged).
- Dictionary/Wikipedia lookups (completely purged).
- Separate main window (completely purged).
- Generic plugin architectures, scripting runtimes, or non-native UI frameworks.

## 2. Technology stack

- **Language & Runtime**: C++20, C++/WinRT, WinUI 3 (Windows App SDK), native Win32.
- **Networking**: WinHTTP (`winhttp.h`).
- **Packaging**: MSIX (`runFullTrust`).
- **Build toolchain**: Visual Studio 2022 / MSVC / MSBuild.

Forbidden: Java, JVM, Kotlin, .NET runtime in the app, WPF, Electron, Chromium, WebView2 for UI, embedded JavaScript, Python, plugin architectures.

## 3. Architecture

Deliberately minimal:

```
dTran
├── WinUI 3 UI:
│   ├── QuickPopupWindow (Primary UI, editable source TextBox, inline History toggle)
│   ├── SettingsWindow (Single-page fixed-size 480x560, no sidebar)
│   └── SnippingOverlayWindow (Screen snip capture for OCR)
├── Windows Integration:
│   ├── Single Instance Mutex (dTran_SingleInstance_Mutex + IPC wakeup)
│   ├── Tray Icon (Shell_NotifyIconW + context menu)
│   ├── Global Hotkeys (Ctrl+Alt+T for Selection/Popup, Ctrl+Alt+O for Screen OCR)
│   ├── Selection Capture (UI Automation TextPattern + clipboard simulation fallback)
│   └── Selection Replacement (ReplaceSelection with clipboard backup & restore)
├── Translation Services:
│   ├── GoogleTranslateService (Web no-key endpoint with backoff & circuit-breaker)
│   ├── YandexTranslateService (Web no-key endpoint with fallback)
│   ├── GoogleTtsService (Chunked sequential playback with instant MCI alias halt)
│   └── WindowsOcrService (Windows.Media.Ocr engine)
├── Shared HTTP: WinHTTP wrapper (single engine)
└── Storage:
    ├── SettingsManager (Local JSON configuration)
    └── HistoryManager (Disk-backed translation history)
```

## 4. Supported services

- **Translation**: Google Translate, Yandex Translate.
- **Speech**: Google Text-to-Speech (web audio sequential chunking).
- **OCR**: Windows OCR (`Windows.Media.Ocr`).

All AI (Gemini, OpenAI) and Dictionary (Reverso, Wikipedia) components are completely removed from the codebase.

## 5. UI & interaction rules

### QuickPopupWindow (Primary UI)
- **Top Language Bar**: `[Source Combo] [Swap Btn] [Target Combo] [Restore Defaults (icon-only)] [Spacer] [OCR (icon-only)]` calibrated to match the exact width of the text cards.
- **Source Input**: Fully editable multiline `TextBox` (`PopupSourceTextBox`). Supports typing, keyboard navigation, copy/paste, and automatic population from selection capture.
- **Result Output Actions**: Strictly ordered as `[ TTS ] [ Replace selected text ] [ Copy ]` (Replace and Copy adjacent). Tooltip: "Replace selected text in active window" / "Заменить выделенный текст в активном окне".
- **Replace Source Text**: `PopupReplaceBtn` replaces the selected text in the active target window with the translated text and restores the user's prior clipboard. Disabled when no selection context exists.
- **Bottom Toolbar**: Left: `Google | Yandex` (icon + service name, localized as "Google" and "Yandex" / "Яндекс"). Right: `History | Settings` (icon-only, right-aligned to match text card edge).
- **Google Vector Icon**: Rendered with `Viewbox Width="16" Height="16"` enclosing `Canvas Width="18" Height="18"` to eliminate bottom clipping.
- **Action Icons**: Enlarged to `FontSize="13"`.
- **Inline History**: Clicking the History icon toggles an inline search & list view directly within the popup card area; pressing Esc or clicking History returns to translation view.
- **Window Lifetime**: Clicking `[X]` cancels window destruction and hides the window back to the system tray (`args.Cancel(true); AppWindow().Hide();`). The process exits only when the user clicks "Exit" in the tray context menu.
- **App & Window Icons**: Windows PE header Resource ID 1 (`app.rc`) and `WindowsIntegration::SetWindowAppIcon(hwnd)` ensure the branded blue icon appears in Explorer, Taskbar previews, Alt+Tab, and window headers.

### SettingsWindow (True Dialog Box)
- **Modal Dialog Chrome**: Non-resizable (`IsResizable(false)`), non-minimizable (`IsMinimizable(false)`), non-maximizable (`IsMaximizable(false)`). Close button (X) only. Dismisses on Esc and Close (X).
- **Size**: Scaled proportional dialog (500x640 DIPs). All controls fit naturally without scrollbars on standard displays, with `ScrollViewer` as an overflow safety net.
- **Sections**:
  - General: Theme selector (System default / Light / Dark with live preview), `Start with Windows` (right-aligned ToggleSwitch), `Application language` (ComboBox with English and Русский).
  - Languages & Translation: Default Source & Target, `Compare Google and Yandex translations` (right-aligned ToggleSwitch).
  - Hotkeys: Independent key combination boxes with independent Reset buttons for Translate (`Ctrl+Alt+T`) and OCR (`Ctrl+Alt+O`).
  - About: Displays `dTran v1.0 beta1`, `Lightweight system translator for Google & Yandex`, and `Created with ❤️ by denb`.

### Tray Menu & Windows Integration
- **Tray Context Menu**: Strictly: Translate Clipboard, Screen OCR, Separator, Settings, Separator, Exit. (Removed "Open dTran"; left click directly opens popup).
- **Menu Glyphs**: Modern 16x16 anti-aliased font glyph bitmaps (`\uE774`, `\uEE6F`, `\uE713`, `\uE7E8`) adapting to system light/dark menu text color.
- **Dynamic Localization**: All labels, tooltips, placeholders, and tray items switch instantly via `LocalizationManager` without requiring application restart.

## 6. Performance & resource standards

- Idle background memory: Private Bytes ~14.5 MB; Working Set < 30 MB.
- Active popup memory: Private Bytes < 65 MB; Working Set < 70 MB.
- Immediate working set trimming upon hiding to system tray.
- Zero CPU usage when idle in the system tray.
- Instant wakeup upon hotkey activation.

