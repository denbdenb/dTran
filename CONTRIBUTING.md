# Contributing to dTran

Thank you for your interest in contributing to **dTran**! We welcome bug reports, documentation updates, and pull requests.

---

## 1. Prerequisites

To build and contribute to dTran on your developer workstation:
- **Operating System:** Windows 10 (version 19041+) or Windows 11 x64
- **Toolchain:** Visual Studio 2022 (Community or Build Tools) with:
  - Desktop development with C++
  - C++20 Standard Support (v143 toolset)
  - Windows 10/11 SDK (10.0.22621.0 or newer)
  - C++/WinRT Visual Studio Extension or CLI
- **Packaging Tools (Optional for installer generation):**
  - Inno Setup 6 (`winget install JRSoftware.InnoSetup`)
  - `makeappx.exe` (included in Windows SDK)

---

## 2. Getting Started

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally:
   ```cmd
   git clone https://github.com/<your-username>/dTran.git
   cd dTran
   ```
3. **Build the application**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release
   ```
4. **Run the automated test suite**:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\scripts\build_test_runner.ps1
   ```

---

## 3. Development Guidelines

- **Architecture:** Keep dTran lightweight and fast. The application relies on native WinUI 3, WinRT, and WinHTTP with no heavy framework dependencies.
- **Resource Footprint:** Preserve the small memory footprint (~9–15 MB idle in system tray). Avoid long-lived allocations or thread leaks.
- **Testing:** Ensure all automated tests in `tests/TestRunner.cpp` pass before submitting a pull request.
- **Coding Style:** Modern C++20 with standard naming conventions, RAII resource management, and clean exception safety.

---

## 4. Submitting a Pull Request

1. Create a feature branch: `git checkout -b feature/my-improvement`.
2. Commit your changes with clear, descriptive commit messages.
3. Push to your branch and open a Pull Request against `main`.
4. Verify that CI checks pass.
