# dTran 1.0.0 — Public Release

> Fast, native Windows desktop translator powered by Google Translate and Yandex Translate with WinUI 3, OCR, TTS, and Tray integration.

---

## What's New in v1.0.0

* **Dual Translation Engines:** Google Translate and Yandex Translate with instant language autodetection.
* **Popup-First Architecture:** Unobtrusive system tray presence with instant hotkey popup (`Ctrl+Alt+T`).
* **Translate Selected:** Capture and translate selected text across any application (browsers, IDEs, documents).
* **Replace Selection:** In-place replacement of selected text directly in the active editor.
* **Screen OCR (`Ctrl+Alt+O`):** Hardware-accelerated snipping tool using native `Windows.Media.Ocr` with copy and translate actions.
* **Text-to-Speech (TTS):** Clear streaming audio pronunciation with automatic language detection.
* **Long Text Translation:** Smart paragraph- and sentence-level chunking (`TextChunker`) without buffer truncation.
* **Translation Comparison:** Compare Google and Yandex outputs side-by-side with one click.
* **History & Search:** Full offline history manager with instant search, copy, and re-insertion.
* **Localization:** Bilingual UI supporting **English** and **Русский** with instant runtime switching.
* **Windows 11 Fluent Design:** Modern WinUI 3 interface with dynamic Light/Dark theme adaptation and seamless tray menu.
* **Start with Windows:** Native Windows Startup Task integration with Task Manager synchronization.
* **Ultralight Memory Footprint:** Approximately **9–15 MB** in tray/idle mode.
* **Unpackaged Self-Contained Deployment:** Standard Inno Setup installer into `C:\Program Files\dTran` and portable standalone zip without requiring Windows Developer Mode.

---

## Downloads

| Package | Size | Description |
|---|---|---|
| **[`dTran-1.0.0-x64-Setup.exe`](https://github.com/denbdenb/dTran/releases/download/v1.0.0/dTran-1.0.0-x64-Setup.exe)** | ~28.4 MB | **Recommended:** Standard Windows installer for all Windows 10/11 x64 systems |
| **[`dTran-1.0.0-x64-Portable.zip`](https://github.com/denbdenb/dTran/releases/download/v1.0.0/dTran-1.0.0-x64-Portable.zip)** | ~41.0 MB | Standalone portable archive (extract and run without installation) |
| **`SHA256SUMS.txt`** | 280 B | SHA-256 verification checksums |

---

## SHA-256 Checksums

```text
238854e6a8c2cbc444b27227abf64d1b8b50bab4ac56a3a8ad11157be4a0fbda  dTran-1.0.0-x64-Setup.exe
d2ddb7505802290e6f5c3f23953e7ef01f093785e4c9024039c17afd5386d627  dTran-1.0.0-x64-Portable.zip
```
