# dTranslate — Conversation & Engineering History

**Conversation ID**: `55aa6f0f-92c9-460f-b1a6-b6ef02588dd5`  
**Workspace Path**: `C:\Users\denb\.gemini\antigravity\scratch\dTranslate`  
**Date**: October 6, 2026

---

## 1. How to Open This Project as an Active Workspace

If you want future sessions or conversations in Antigravity to be immediately attached to `dTranslate`:

1. In Antigravity IDE (or VS Code), go to **File -> Open Folder...** (`Файл -> Открыть папку...`).
2. Navigate to:
   ```
   C:\Users\denb\.gemini\antigravity\scratch\dTranslate
   ```
   *(Or move this folder to your preferred code directory like `C:\Projects\dTranslate` and open it there).*
3. When opened, Antigravity will automatically detect:
   * [`AGENTS.md`](../AGENTS.md) — The authoritative system specification and development guidelines.
   * `build.ps1` and `run.ps1` — One-click build and execution scripts.
   * Full source code in `src/` and test harness in `tests/`.

---

## 2. Engineering Milestones & Trajectory

### Pass 1: Architecture & Initial Native Foundation
* Built native Windows 11 C++20 / WinRT / WinUI 3 architecture.
* Integrated WinHTTP network client with SSL/TLS and automatic system proxy resolution.
* Integrated Windows Credential Manager (`CredentialStore`) for DPAPI-secured key storage.
* Implemented Windows integration: system tray icon (`Shell_NotifyIconW`), message-only window, global hotkeys (`RegisterHotKey`), selection capture (`SelectionCapture`), and clipboard fallback (`ClipboardHelper`).

### Pass 2: No-Config Web Engines & Throttling Resilience
* Established zero-configuration translation for:
  * **Google Translate Web Engine** (`translate.googleapis.com/translate_a/single`)
  * **Yandex Translate Web Engine** (`translate.yandex.net/api/v1/tr.json/translate` with dynamic SID / CSRF tokens)
* Added exponential backoff, circuit breaking, and endpoint health tracking (`GoogleEndpointHealth`, `YandexWebClient`) to handle HTTP 429 throttling without exposing errors on short bursts.

### Pass 3: Windows OCR & Screen Snipper
* Implemented on-screen area snipping (`ScreenSnipper`) with interactive drag rectangle and floating action toolbar.
* Connected Windows 11 native OCR engine (`Windows.Media.Ocr.OcrEngine`) with BCP-47 locale matching and fallback.
* Optimized GDI memory allocation in `ScreenSnipper`: eliminated full-screen bitmap re-allocation churn on every mouse move event.

### Pass 4: Dictionaries & Google TTS
* **Reverso Context Engine**: Direct extraction of real bilingual sentence examples and grammatical classifications.
* **Wikipedia Summary Engine**: REST summary fetching with redirect handling.
* **Google TTS Engine**: Sequential sentence-boundary chunking (<= 160 chars), multi-chunk background streaming, instant stop, and playback toggle (🔊/🔇).

### Pass 5: Final UI/UX Polish, Vectors & Performance
* Replaced text badges with 95 local vector SVG flag assets in `assets/flags/`.
* Replaced service labels with vector SVG emblems for Google, Yandex, Gemini, OpenAI, Reverso, and Wikipedia.
* Separated active session language pair from persistent settings defaults, adding dynamic "Restore default languages" button.
* Replaced AI model selection dropdowns in Settings with static verified models (`gemini-2.5-flash`, `gpt-4o-mini`), adding distinct detection for OpenAI quota depletion (`insufficient_quota`).
* Generated multi-resolution `assets/app.ico` (16x16 to 256x256) and wired it into the Windows notification tray.
* Verified 36/36 test cases in `TestRunner.exe` and verified process commit memory <= 43 MB.

---

## 3. Original Conversation Brain & Logs Reference

Full internal conversation logs, transcripts, and model thoughts are archived at:
* Local transcript: `C:\Users\denb\.gemini\antigravity\brain\55aa6f0f-92c9-460f-b1a6-b6ef02588dd5\.system_generated\logs\transcript.jsonl`
* Concept UI image: [`docs/ui-concept.jpg`](ui-concept.jpg)
