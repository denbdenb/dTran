# dTran 1.2.5 — Multilingual Release & DeepL Web Integration

> Lightweight popup translator for Windows with Google Translate, Yandex Translate, DeepL Web, 10-language UI localization, and zero-heap memory architecture.

---

## What's New in v1.2.5

* **DeepL Web Translation Provider:** Integrated native C++ web client for DeepL, supporting 30+ languages alongside Google Translate and Yandex Translate without requiring API keys or third-party runtimes.
* **10-Language UI Localization:** Complete interface localization for **English**, **Русский**, **Español**, **Deutsch**, **Français**, **Português (Brasil)**, **Italiano**, **日本語**, **한국어**, and **简体中文** with instant dynamic switching.
* **Zero-Heap Memory Architecture:** Dictionaries are compiled into immutable static tables in the binary's `.rdata` section with binary search, guaranteeing zero dynamic heap allocations at startup.
* **Windows System Language Auto-Detection:** Automatically matches your Windows display language on first run with graceful fallback to English, respecting user preferences saved in `settings.json`.
* **Adaptive Screen OCR Toolbar:** Dynamically measures button text using Win32 GDI `DrawTextW` to prevent text truncation across all languages and DPI scales.
* **Settings Dialog Upgrades:** Updated version to 1.2.5 and enabled automatic text wrapping for smooth readability in all languages.

---

## Downloads

| Package | Size | Description |
|---|---|---|
| **[`dTran-1.2.5-x64-Setup.exe`](https://github.com/denbdenb/dTran/releases/download/v1.2.5/dTran-1.2.5-x64-Setup.exe)** | 28.49 MB | **Recommended:** Standard Windows installer for all Windows 10/11 x64 systems |
| **[`dTran-1.2.5-x64-Portable.zip`](https://github.com/denbdenb/dTran/releases/download/v1.2.5/dTran-1.2.5-x64-Portable.zip)** | 41.13 MB | Standalone portable archive (extract and run without installation) |
| **`SHA256SUMS.txt`** | 180 B | SHA-256 verification checksums |

---

## SHA-256 Checksums

```text
dde13ed42fc5e753df89243f3760de3aa9e4c5a93b1fd58c7507835dc6a423d0  dTran-1.2.5-x64-Setup.exe
7c0d3846b3ec93796eec54e8365ddb705babf452e8455f768cedcc327693f909  dTran-1.2.5-x64-Portable.zip
```
