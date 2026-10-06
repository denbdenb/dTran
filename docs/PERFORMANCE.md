# Performance

Memory is a first-class requirement. Targets (engineering goals, **not guarantees**):

| Metric | Target |
|---|---|
| Idle private memory | ≈ < 100 MB (budget), < 60 MB (ideal) |
| With quick popup open | ≈ < 120 MB |
| Growth after 10 / 100 translations, and after repeated popup open/close | none significant |

## How to measure

Use a Release build (`.\build.ps1 -Configuration Release`, `.\run.ps1 -Configuration Release`).
Close other heavy apps. Use "Private bytes" (not "Working set") as the headline number.

| Measurement | Procedure |
|---|---|
| Cold startup | Reboot, wait 2 minutes, start the app, time to window visible (stopwatch / screen recording, or `Measure-Command` on the launch script) |
| Warm startup | Close app, start again immediately |
| Idle memory | Start, wait 60 s, read Private Bytes: `Get-Process dTranslate | Select-Object Id, ProcessName, WorkingSet64, PrivateMemorySize64` |
| After 10 / 100 translations | Translate 10 (then 100) distinct short texts; wait 30 s; read Private Bytes |
| Popup open/close | Open and Esc-close the popup 100 times; wait 30 s; read Private Bytes |
| CPU idle | `Get-Counter '\Process(dTranslate)\% Processor Time' -SampleInterval 5 -MaxSamples 12` with the app idle |
| CPU during translation | Same counter while translating a long text |
| Popup latency | Hotkey press → popup visible (screen recording at 60 fps, count frames); target: perceived as instant |

Also open the Windows Performance Monitor or Task Manager "Details" tab to check
thread count and handle count do not grow over a session.

## Results

Record each run: date, build config, git commit, machine, numbers.

| Date | Commit | Build | Idle MB (private) | Working Set MB | Notes |
|---|---|---|---|---|---|
| 2026-10-05 | 8f29c22 | Debug (x64) | 54.26 MB | 102.67 MB | All services compiled, instant launch |
| 2026-10-05 | 8f29c22 | Release (x64) | 43.22 MB | 92.95 MB | LTCG & WPO enabled, exceeds ideal <60MB budget |
| 2026-10-06 | latest | Release (x64) | 44.02 MB | 89.66 MB | Final UX revision: No-key Yandex web endpoint, Screen Snipping OCR, collapsible sidebar |
| 2026-10-06 | latest | Release (x64) | 42.24 MB | 91.41 MB | Stability pass: Circuit breaker, 92-lang catalog, High-DPI Snipper, model discovery |
