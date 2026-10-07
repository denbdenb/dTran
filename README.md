# dTran

> Lightweight popup translator for Windows with Google Translate and Yandex Translate.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%2011%20%7C%2010%20x64-blue)](https://github.com/denb/dTran)
[![Release](https://img.shields.io/badge/Release-v1.0.0-green.svg)](https://github.com/denb/dTran/releases)

**dTran** is a fast, native Windows desktop translator that lives in your system tray. Highlight text in any application, press a global hotkey, read the translation in an instant popup window, and optionally replace the original selection in-place.

---

## Features

* **Google Translate & Yandex Translate:** Instant translation powered by dual web engines with automatic language detection.
* **Popup-First Workflow:** Stays unobtrusively in the system tray; press a hotkey or click the tray icon to open the lightweight popup.
* **Translate Selected (`Ctrl+Alt+T`):** Captures highlighted text in any active window (browsers, editors, documents) via UI Automation and clipboard fallback.
* **Replace Selected Text:** Translates and replaces the selected text directly inside the target application, safely restoring your clipboard afterwards.
* **Screen OCR (`Ctrl+Alt+O`):** Snip any portion of the screen to recognize text using native `Windows.Media.Ocr` and immediately translate it.
* **Text-to-Speech (TTS):** Clear audio pronunciation with automatic language detection and sequential streaming.
* **Long-Text Support:** Smart paragraph and sentence chunking (`TextChunker`) translates long passages without losing structure.
* **Side-by-Side Comparison:** Compare Google and Yandex translations in a single view with one click.
* **Translation History:** Fast, searchable disk-backed history buffer with instant copy and re-insert capabilities.
* **Bilingual UI:** Fully localized in **English** and **Русский** with instant runtime switching.
* **Start with Windows:** Integrated with native Windows Startup Tasks (`Windows.ApplicationModel.StartupTask`) and synchronized with Windows Task Manager.
* **Modern Windows 11 UI:** Built with WinUI 3 and Fluent Design, automatically matching system Light/Dark appearance.
* **Ultralight Memory Footprint:** Consumes only **~9–15 MB** of memory in the background.

---

## Installation

### Recommended (Installer)
1. Download **[`dTran-1.0.0-x64-Setup.exe`](https://github.com/denb/dTran/releases/latest)** from the latest GitHub Release.
2. Run the installer (installs per-user to your local profile without requiring administrator privileges).
3. dTran launches automatically in your system tray.
4. Highlight any text and press `Ctrl+Alt+T` to translate!

### Developer / Testing Package (MSIX)
* **`dTran-1.0.0-x64.msix`** is provided as an unsigned package for developers and testing in environments with Windows Developer Mode or custom certificate deployment. For end-users, please use the recommended **Setup.exe** installer above.

### Uninstallation
* To uninstall, open **Windows Settings** → **Apps** → **Installed apps** → search for **dTran** → click **Uninstall**, or run the uninstaller from the Start Menu.

---

## Supported Platforms

* **Windows 11 x64** (All editions, 21H2 and newer)
* **Windows 10 x64** (Build 19041 and newer)
* *Architecture:* x64

---

## Default Hotkeys

| Hotkey | Action |
|---|---|
| **`Ctrl + Alt + T`** | Translate selected text / Open dTran Popup |
| **`Ctrl + Alt + O`** | Screen OCR snip & translate |
| **`Esc`** | Dismiss popup back to system tray / Cancel snipping |

> *Note: Hotkeys can be customized or reset in the Settings dialog.*

---

## Architecture

dTran is engineered in pure **C++20** using **C++/WinRT** and **WinUI 3**. It does not use Electron, Node.js, .NET runtime, or WebView2 rendering.

```text
System Tray
    ↓
Popup Translator
    ├── Google Translate Engine
    ├── Yandex Translate Engine
    ├── Windows Media OCR
    ├── Google Text-to-Speech
    ├── Translation History Buffer
    └── Settings Dialog
```

* Cloud AI (Gemini, OpenAI) and Dictionary services were completely eliminated to maximize speed, maintain privacy, and achieve a minimal memory footprint.

---

## Privacy

* **Direct Provider Communication:** Translation requests are sent directly from your computer to Google and Yandex over HTTPS.
* **No Server Intermediary:** dTran does not run an external cloud backend, database, or analytics collector.
* **No Telemetry or Tracking:** Zero user telemetry, crash uploads, or trackers.
* **Local Data Storage:** Settings and Translation History are stored exclusively on your local machine in `%LOCALAPPDATA%`.

---

## Known Limitations

* **Language Support Differences:** While Google Translate supports over 100 languages, Yandex Translate supports a slightly smaller catalog. When a language unsupported by Yandex is chosen, dTran gracefully notifies the user and suggests switching to Google Translate.
* **Non-Selectable Controls:** Text inside certain hardened applications (e.g. elevated command prompts or protected games) cannot be read via UI Automation; in such cases, use the **Screen OCR** feature (`Ctrl+Alt+O`).

---

## Building from Source

### Prerequisites
* Windows 10/11 x64
* [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community or Build Tools) with:
  * Desktop development with C++
  * MSVC v143 toolset (C++20)
  * Windows 10/11 SDK (10.0.22621.0 or newer)
* [Inno Setup 6](https://www.innosetup.com/) (for building the setup installer)

### Build Steps

```powershell
# 1. Clone repository
git clone https://github.com/denb/dTran.git
cd dTran

# 2. Build Release x64 binary
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release

# 3. Run automated tests (133/133 tests)
powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1

# 4. Build distribution packages (Setup.exe + MSIX)
powershell -ExecutionPolicy Bypass -File .\scripts\build_installer.ps1
```

All distribution artifacts will be generated in the `dist/` directory.

---

## License

This project is licensed under the [MIT License](LICENSE).
Third-party notices and licenses are documented in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
