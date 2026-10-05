# AGENTS.md — dTranslate project specification

This file is the **authoritative specification** for dTranslate. Every contributor
(human or AI) must read it before working and must not silently weaken, remove or
reinterpret any requirement. Changes to the architecture require the process in
section 17 (Change control).

Visual reference: `docs/ui-concept.jpg` (three views: main window, quick popup,
settings window; the concept shows the name "QTranslate" — the product name is
**dTranslate**). If the image is unavailable, use the textual spec in section 9/10.

## 0. Working with the user

The user is **not an experienced programmer**.

- Create files, folders, configs, build setup, git repo and code yourself. Never ask
  the user to create project files, AGENTS.md, folders or boilerplate.
- Perform safe setup automatically. Ask before anything needing admin rights (UAC),
  large downloads, or machine-wide changes.
- If a dependency is missing: explain (1) what is missing, (2) why it is needed,
  (3) the simplest way to install it. Do not overwhelm with alternatives.
- Use plain language. Don't assume knowledge of C++, Visual Studio, MSBuild, Git,
  WinUI or the Windows SDK. Don't ask the user to copy files between folders.

## 1. Product goal

A modern, lightweight, native Windows 11 desktop translation application.

It is intentionally **NOT a plugin platform** and NOT a general extensible
translation framework. Small, focused, fast, visually polished, resource-efficient.

Philosophy: **"Do a small number of things extremely well."**

Primary goals: excellent Windows 11 integration; modern Fluent/WinUI 3 interface;
very low idle resource use; fast startup (including AutoStart); fast quick-translation
popup; keyboard-first; translate selected text in any Windows app; clipboard
integration; multiple fixed translation/AI services; compact settings.
Non-goals: unnecessary frameworks, speculative features, plugin ecosystem,
browser-based UI.

Priority order: 1 Correctness, 2 Native Windows UX, 3 Responsiveness, 4 Low memory,
5 Security, 6 Simplicity, 7 Extensibility (deliberately last). Never compromise
simplicity for hypothetical future requirements.

## 2. Technology stack

Use: C++20, C++/WinRT, WinUI 3, Windows App SDK, Windows SDK, MSIX deployment,
native Win32 interop where necessary, **WinHTTP** for networking, standard C++
facilities wherever practical. Build with Visual Studio / MSBuild (MSVC).
Prefer Microsoft/Windows native APIs over third-party libraries.

Forbidden: Java, JVM, Kotlin, .NET runtime in the app, WPF, Electron, Chromium,
WebView2 for the main UI, browser-based UI, embedded JavaScript runtime, Python
runtime, PascalScript, plugin systems, dynamic JAR/provider DLL loading, plugin
managers/manifests, dependency-injection containers, service locators, speculative
enterprise abstractions, unnecessary large third-party dependencies.

(The .NET SDK on the dev machine may be used only as a tool, e.g. `winapp`/NuGet
restore. It must not become a runtime dependency of the app.)

If a dependency is truly necessary, first document why in `docs/` and keep the
footprint minimal. No abstractions for theoretical future extensibility.

## 3. Architecture

Deliberately simple:

```
Application
├── WinUI 3 UI: Main Window, Quick Translation Popup, Settings, History, AI tools
├── Application Core: translation / AI / dictionary / TTS / OCR orchestration
├── Windows Integration: tray, global hotkeys, clipboard, selected-text capture, Win32
├── Services: Google Translate, Yandex Translate, Gemini, OpenAI, Reverso,
│             Wikipedia, Google TTS, Windows OCR
├── Shared HTTP: ONE common WinHTTP implementation
└── Local Storage: settings, API credentials, history
```

- NO generic plugin architecture. NO ProviderManager / PluginManager unless strictly
  necessary as an implementation detail that does not become an extensibility framework.
- Explicit service classes: `GoogleTranslateService`, `YandexTranslateService`,
  `GeminiService`, `OpenAIService`, `ReversoService`, `WikipediaService`,
  `GoogleTtsService`, `WindowsOcrService`.
- One shared HTTP implementation; no per-service permanent HTTP client/runtime.
- Services are lazy where practical; no network service is initialized until needed.
- Keep names boring and obvious. Forbidden style: `ServiceManager2`,
  `ProviderManagerFactory`, `UniversalTranslationBackend`, `AbstractProviderHost`,
  `MegaService`.

## 4. Fixed service set (exactly these)

- Translation: 1 Google Translate, 2 Yandex Translate
- AI: 3 Google Gemini, 4 OpenAI
- Dictionary/Reference: 5 Reverso, 6 Wikipedia
- Text to speech: 7 Google Cloud Text-to-Speech
- OCR: 8 Windows OCR (`Windows.Media.Ocr`)

No additional providers unless explicitly requested later. No "add arbitrary provider".

## 5. AI functionality

Gemini and OpenAI are AI backends, not separate translation engines.
Operations: **Translate, Rewrite, Improve, Summarize, Explain**.
UI lets the user pick the AI backend (Gemini | OpenAI) and the operation.
All five operations reuse **one** AI request infrastructure. All inference is remote
via API — **no local models**.

## 6. Translation behavior

Support: source language, target language, automatic source detection, swap,
recent language pairs, configurable primary translation service, optional
Google-vs-Yandex comparison, translation of typed text / clipboard text / text
selected in other applications.

Requests are asynchronous; the UI must never freeze on the network. Cancellation
where reasonably practical. Handle: timeout, HTTP failure, invalid credentials,
API limits, malformed response, unavailable service, empty response — shown in
human-friendly language. No raw exceptions to ordinary users (diagnostics only).

## 7. Windows integration (required)

1 System tray icon (`Shell_NotifyIcon`), 2 Global hotkey (`RegisterHotKey`),
3 Clipboard integration, 4 Selected-text translation, 5 Quick translation popup,
6 Windows OCR.

Selected-text capture: use the least intrusive reliable method (e.g. UI Automation
`TextPattern`) with a **clipboard-based fallback** (simulate copy, restore the user's
clipboard). Do not assume every app exposes selection the same way.

## 8. Quick Translation Popup (most important feature)

Appears near the selected text when possible; compact; Windows 11 consistent;
keyboard friendly; quick translate; copy; TTS; switch service; switch language;
**Escape closes**; avoids focus stealing; feels instant. Must NOT become a
miniature full application — keep it visually and architecturally simple.

## 9. Main window

Left sidebar: Translate, Dictionary, AI, History. Bottom: Theme/appearance, Settings.
Main content, top to bottom: language selectors `[Source] [Swap] [Target]`; source
text panel (with character counter); primary **Translate** button; translation
result panel (TTS / copy / more); service selection (Google, Yandex, Gemini,
OpenAI); utility actions (Dictionary, Wikipedia, TTS, OCR).

Visual style: Windows 11 / Fluent; light (dark supported); clean; compact; subtle
rounded corners; subtle shadows; restrained accent color; excellent typography;
clear hierarchy; keyboard friendly; not overloaded. Mica where appropriate, no
overuse of translucency. Must not imitate a web app or a web dashboard.

## 10. Settings window

Modern Windows 11 settings experience. Sections: **General, Translation, AI,
Dictionaries, Speech, OCR, Hotkeys, About**. Each service has Enabled toggle plus
simple configuration (Google/Yandex: API config; primary-translator choice;
Gemini/OpenAI: API key + default model; Reverso: config if needed; Wikipedia:
enabled; Google TTS: voice + language; Windows OCR: enabled). Save / Cancel buttons.
No plugin-management UI, no "extensions" page.

## 11. Local storage

Settings stored locally: source/target language, primary translator, enabled
services, theme, hotkeys, AI settings, TTS settings, app preferences.
**API secrets never in plain-text config.** Use Windows Credential Manager / DPAPI.
Never hard-code keys; never commit keys; never put secrets in Git. Provide a clear
UI for entering API keys.

## 12. History

Intentionally bounded. No unlimited in-memory history; disk-backed; only a small
recent working set in memory; never load thousands of translations at startup.
User can: view, search, reopen, copy, delete an item, clear history.

## 13. Performance (first-class requirement)

Avoid: multiple HTTP engines, multiple persistent network clients, plugin class
loaders, large runtimes, browser processes, unnecessary background threads,
large permanent caches, unbounded collections, loading all history at startup,
loading all service implementations, eager initialization of every service,
unnecessary timers, unnecessary polling.

Engineering targets: idle private memory ≈ **< 100 MB** if realistically
achievable; quick popup ≈ **< 120 MB**; no obvious unbounded growth after repeated
translations. Measure and document actual results (`docs/PERFORMANCE.md`); do not
present targets as guarantees. If memory grows significantly, investigate before
adding features; do not just raise limits or suppress diagnostics.

## 14. Startup

Start → Windows/WinUI init → load settings → init tray and core Windows
integration → show UI → services initialize only when required. No network
connections at startup. Never call Google/Yandex/Gemini/OpenAI merely because the
app launched.

## 15. UI principles, accessibility, localization

Prioritize readability, hierarchy, speed, compactness, accessibility, keyboard
navigation, native Windows behavior, consistent spacing and iconography.
Avoid excessive gradients/shadows/giant cards/empty space/animation/decoration,
web-style navigation, excessive modal dialogs. Animations subtle, never harming
responsiveness.

Accessibility: keyboard navigation, visible focus, sensible tab order, readable
text, high-contrast, Windows light/dark mode, reduced motion; never rely on color
alone for state. Set `AutomationProperties.Name` on icon-only controls.

Localization: English and Russian UI initially. Simple resource-based layer
(`.resw`), additional languages addable without redesign. No big framework.

## 16. Networking, APIs, security, errors, logging

**Networking:** one shared WinHTTP layer: GET, POST, headers, JSON in/out, timeout,
cancellation where practical, status-code handling, retry only where appropriate,
TLS via Windows, proxy support if practical.

**APIs:** Use official supported APIs; do not scrape if an official API exists.
Google Translate (official Cloud Translation API), Yandex Translate (official
Yandex Cloud Translate API), Gemini (official Gemini API), OpenAI (official OpenAI
API), Google TTS (official Cloud Text-to-Speech API), Wikipedia (official Wikimedia
APIs), Reverso (minimum integration only; no generic framework around it).
Credentials are never embedded in source.

**Security:** never hard-code or commit secrets; never expose keys in logs,
screenshots, diagnostics; never print secret headers; logs redact sensitive data.
Provide a local diagnostic/log mode useful without exposing credentials.

**Errors:** gracefully handle no internet, timeout, invalid/expired key, quota,
rate limits, server errors, malformed response, unsupported language, empty input,
OCR failure, TTS failure. Tell the user what happened and what to do next, without
jargon.

**Logging:** lightweight; log lifecycle, high-level service requests,
success/failure, timing, key state transitions. Never log API keys, Authorization
headers, full sensitive user content, passwords. Offer a verbose-logging switch.

## 17. Change control and AI development rules

- Do not over-engineer. No abstractions "because they might be useful later".
- No plugin architecture. No unrequested providers. No unnecessary dependencies.
- Don't replace native Windows APIs with large frameworks without a concrete reason.
- Prefer simple explicit code, small classes, clear ownership, deterministic
  lifecycle, lazy initialization, one shared HTTP stack, native features.
- Between two valid solutions pick the simpler one with fewer runtime dependencies
  and lower memory use.
- Do not silently change the architecture. If a major decision proves technically
  invalid, document the problem and propose the smallest viable correction. Before
  introducing a major dependency/abstraction explain: why required, alternatives
  considered, memory/runtime implications, conflicts with this spec.
- Do not rewrite large parts of the project without a clear reason.

## 18. Project structure

```
/src  /App /Core /Windows /Translation /AI /Dictionary /Speech /OCR /Networking
      /Storage /History /UI/{MainWindow,QuickPopup,Settings,History,Dictionary,AI}
/tests  /assets  /docs  /build
```
Do not create unnecessary folders. Keep it easy for a human to understand.

## 19. Testing

Automated tests where practical; at minimum: settings serialization, language
selection, service configuration, HTTP error handling, translation response
parsing, AI response parsing, history storage, testable Windows-integration logic.
Manual test procedures (in `docs/DEVELOPMENT.md` or `docs/MANUAL_TESTS.md`) for:
global hotkey, selected-text translation, clipboard translation, popup behavior,
tray behavior, OCR, TTS, theme switching, settings persistence.

Performance procedure (`docs/PERFORMANCE.md`): cold startup, warm startup, idle
memory, memory after 10 / 100 translations, memory after repeated popup
open/close, CPU idle, CPU during translation, popup display latency. Document results.

## 20. Git and documentation

Git repo with a sensible `.gitignore`. Never commit API keys, secrets, local user
settings, credentials, unneeded build outputs, temp files. Meaningful commits, no
dozens of meaningless ones.

Docs (concise, not bloated): `README.md`, `AGENTS.md`, `docs/ARCHITECTURE.md`,
`docs/DEVELOPMENT.md`, `docs/PERFORMANCE.md` — covering what the app is,
architecture, build requirements, how to build/run, how services are configured,
how credentials are stored, how performance is measured.

## 21. Development workflow (phases)

0 Environment audit & foundation · 1 Working WinUI 3 shell · 2 Main window visual
design · 3 Quick popup · 4 Settings UI · 5 System tray · 6 Global hotkeys &
selection capture · 7 Clipboard · 8 Shared networking · 9 Google Translate ·
10 Yandex · 11 Gemini · 12 OpenAI · 13 Reverso · 14 Wikipedia · 15 Google TTS ·
16 Windows OCR · 17 History · 18 Performance optimization · 19 Security review ·
20 Release build & packaging.

After each major phase: build; run tests; run the app; fix errors before moving
on; re-check against this file; update docs when architecture changes.
Never skip build verification.

## 22. Final product vision

User selects text anywhere in Windows → presses a global hotkey → sees a compact
popup → reads the translation → copies it → hears it → changes language →
switches Google/Yandex/Gemini/OpenAI → opens the full app for advanced work.
The app feels fast, calm, compact, modern, reliable, native, unobtrusive.

## 23. Acceptance criteria (summary)

- Builds with MSBuild from a clean checkout using documented steps; runs on Windows 11.
- No forbidden technology present; no plugin/provider-manager abstractions.
- Exactly the 8 specified services; one shared WinHTTP layer; lazy service init.
- No network traffic at startup; no secrets in source, logs or Git.
- Idle/popup memory measured and documented against targets in section 13.
- Main window, popup and settings follow the concept image and section 9/10.
- English + Russian UI; light/dark; keyboard-accessible.
- Required automated tests pass; manual test procedures documented.
