# Performance

Memory is a first-class requirement. Targets (engineering goals, **not guarantees**):

| Metric | Target |
|---|---|
| Idle private memory | ≈ < 100 MB |
| With quick popup open | ≈ < 120 MB |
| Growth after 10 / 100 translations, and after repeated popup open/close | none significant |

## How to measure

Use a Release build (`.\build.ps1 -Configuration Release`, `.\run.ps1 -Configuration Release`).
Close other heavy apps. Use "Private bytes" (not "Working set") as the headline number.

| Measurement | Procedure |
|---|---|
| Cold startup | Reboot, wait 2 minutes, start the app, time to window visible (stopwatch / screen recording, or `Measure-Command` on the launch script) |
| Warm startup | Close app, start again immediately |
| Idle memory | Start, wait 60 s, read Private Bytes: `Get-Process dTranslate \| Select Name, @{n='PrivateMB';e={[int]($_.PrivateMemorySize64/1MB)}}, @{n='WorkingSetMB';e={[int]($_.WorkingSet64/1MB)}}` |
| After 10 / 100 translations | Translate 10 (then 100) distinct short texts; wait 30 s; read Private Bytes |
| Popup open/close | Open and Esc-close the popup 100 times; wait 30 s; read Private Bytes |
| CPU idle | `Get-Counter '\Process(dTranslate)\% Processor Time' -SampleInterval 5 -MaxSamples 12` with the app idle |
| CPU during translation | Same counter while translating a long text |
| Popup latency | Hotkey press → popup visible (screen recording at 60 fps, count frames); target: perceived as instant |

Also open the Windows Performance Monitor or Task Manager "Details" tab to check
thread count and handle count do not grow over a session.

## Results

Record each run: date, build config, git commit, machine, numbers.

| Date | Commit | Build | Idle MB (private) | Working Set MB | Handles | Notes |
|---|---|---|---|---|---|---|
| 2026-10-05 | 8f29c22 | Debug (x64) | 54.26 MB | 102.67 MB | 963 | All services compiled, instant launch |
| 2026-10-05 | 8f29c22 | Release (x64) | 43.22 MB | 92.95 MB | 824 | LTCG & WPO enabled, exceeds ideal <60MB budget |
