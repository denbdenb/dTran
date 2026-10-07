# Changelog

All notable changes to **dTran** are documented in this file.

---

## [1.0.0] — 2026-10-07

### Added
- **Popup-First Architecture:** Lightweight system utility living in the Windows notification tray area with instant popup translation.
- **Dual Translation Engines:** Google Translate and Yandex Translate web engines with optional side-by-side comparison.
- **Translate Selected:** Global hotkey (`Ctrl+Alt+T`) grabs selected text from any active application via UI Automation or simulated clipboard copy.
- **Replace Selected:** Replaces highlighted source text in the target application with the translated output while preserving original clipboard state.
- **Screen OCR:** Screen snipping capture with Windows Media OCR (`Ctrl+Alt+O`) and inline text extraction.
- **Text-to-Speech:** Instant audio pronunciation using chunked sequential streaming and automatic source language detection.
- **Long-Text Support:** Sentence and paragraph chunker (`TextChunker`) preserving formatting across long inputs.
- **Character Counter:** Unicode code-point character counter with live tracking and locale formatting.
- **Translation History:** Disk-backed ring buffer storing recent translations with instant search, insertion, and clearing.
- **Windows Startup Integration:** Native `Windows.ApplicationModel.StartupTask` support ("Start with Windows") with Task Manager sync.
- **Modern Tray Menu:** WinUI 3 styled system tray menu supporting Windows 11 Light/Dark system themes and high DPI displays.
- **Bilingual Interface:** Complete English and Russian UI localization with instant runtime language switching.
- **Distribution & Installer:** Self-contained Inno Setup installer (`dTran-1.0.0-x64-Setup.exe`) and MSIX package (`dTran-1.0.0-x64.msix`).

### Changed
- **Memory Optimization:** Idle footprint reduced to **9–15 MB** in system tray.
- **Streamlined Design:** Completely removed legacy cloud AI (Gemini, OpenAI) and Dictionary (Reverso, Wikipedia) components for speed and privacy.
- **Single Instance:** Enforced single-instance application lifecycle with window wakeup IPC.
