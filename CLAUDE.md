# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Hoka (Hot Key Analyzer) is a Windows-only C++17 desktop app that installs a global low-level keyboard hook, records which hotkey combinations are used in which foreground application, and stores counts in a local SQLite database. GUI is FLTK; it lives in the system tray. See [README.md](README.md) for the product framing.

## Build & test

Dependencies (FLTK, SQLite3, GTest) come from the **vcpkg submodule** — it must be initialized before configuring:

```bash
git submodule update --init --recursive
```

The build is **MinGW-only**: `CMakeLists.txt` hardcodes the vcpkg toolchain file and forces both the target *and host* triplet to `x64-mingw-dynamic`, so an MSVC generator will pull the wrong dependency binaries. The host triplet matters: without it vcpkg builds its helper ports for `x64-windows` and fails with "Unable to find a valid Visual Studio instance" on machines without VS.

Toolchain: WinLibs GCC 15.2.0 (POSIX, UCRT) unpacked to `C:\mingw64`; it bundles CMake and Ninja. The vcpkg mingw toolchain locates `x86_64-w64-mingw32-gcc` via `PATH` (not via `CMAKE_CXX_COMPILER`), so the configure preset prepends `C:/mingw64/bin` to `PATH` itself. If MinGW lives elsewhere, adjust the paths in [CMakePresets.json](CMakePresets.json). Use the presets:

```bash
cmake --preset gcc-preset
cmake --build --preset gcc-preset
```

vcpkg builds FLTK/SQLite/GTest (21 ports) from source on the first configure, which is slow — around 30 minutes on a fresh machine, mostly slow downloads from sourceware.org and MSYS2 mirrors that vcpkg retries. Expect it, don't assume the build hung. Subsequent configures restore from vcpkg's binary cache (`%LOCALAPPDATA%\vcpkg\archives`).

vcpkg's applocal step copies dependency DLLs (`libsqlite3.dll`, `libgtest.dll`) next to the executables, but the MinGW runtime DLLs (`libstdc++-6.dll` etc.) are not copied — running an `.exe` directly needs `C:\mingw64\bin` on `PATH`. The build/test presets inherit the configure preset's `PATH`.

Two targets are produced: `hoka` (the app) and `hoka_tests` (GTest).

### Running tests

```bash
ctest --preset gcc-preset                       # runs the hoka_tests target
./out/build/gcc-preset/hoka_tests.exe           # run directly, see per-test output
./out/build/gcc-preset/hoka_tests.exe --gtest_filter=DatabaseTest.UpdateAndGetKeyStatistics   # single test
```

## Architecture

`HokaApplication` in [src/main.cpp](src/main.cpp) owns four components and wires them together. Initialization order is load-bearing: Database → SystemTray → MainWindow → tray callbacks → KeyLogger.

The core data flow is a producer/consumer pipeline:

1. **KeyLogger** ([src/KeyLogger/KeyLogger.cpp](src/KeyLogger/KeyLogger.cpp)) installs a `WH_KEYBOARD_LL` global hook. The hook is a `static` function that reaches the object through a singleton `instance` pointer. On key-up it resolves the foreground process name via PSAPI, assembles a combination string (`Ctrl+`, `Shift+`, etc. + main key), and pushes a `KeyPressEvent` onto a mutex-guarded queue. Bare modifier presses are dropped.
2. A dedicated **processing thread** drains that queue and invokes the event callback set in `main.cpp` (`handleKeyEvent`).
3. `handleKeyEvent` writes the event to the **Database** and, if the window is visible, updates the FLTK UI and the tray tooltip.

Caveat worth knowing: `handleKeyEvent` runs on the KeyLogger's processing thread but touches FLTK widgets directly. FLTK is not thread-safe, so any new UI work triggered from key events lives in that same non-main-thread context.

**Database** ([src/Database/Database.cpp](src/Database/Database.cpp)): thin SQLite wrapper. One table `key_statistics` with `UNIQUE(app_name, key_combination)`; `updateKeyStatistics` does an `INSERT OR REPLACE ... COALESCE(press_count + 1, 1)` to increment counts. All queries go through `executePreparedQuery`, which binds `std::variant<std::string,int>` params. The DB file `keypress_stats.db` is opened relative to the **current working directory**, not an absolute path.

**UI**: `MainWindow` ([src/UI/MainWindow.cpp](src/UI/MainWindow.cpp)) is an `Fl_Window` (recent-activity panel + per-app stats). `SystemTray` ([src/UI/SystemTray.cpp](src/UI/SystemTray.cpp)) provides minimize/close-to-tray. `main.cpp` runs a custom loop (`while(!shouldExit) Fl::wait(0.1)`) instead of `Fl::run()` so the app keeps running while all windows are hidden in the tray.

`Models/` holds plain data structs (`KeyPress.h`, `KeyStatistics.h`).

## Gotchas

- **Requires administrator rights.** The MSVC branch of `CMakeLists.txt` embeds a `requireAdministrator` manifest; the low-level keyboard hook generally needs elevation to observe elevated foreground apps. Run the app elevated.
- **`hoka` is a GUI-subsystem (`WIN32`) executable**, so `std::cout`/`std::cerr` are invisible in a Release build. For MinGW the `Debug` build type switches `hoka` to the console subsystem, which is how you see the logging.
- **Tests are not isolated from the app's data.** `hoka_tests` uses the same `keypress_stats.db` in the working directory and each test's `TearDown` calls `clearStatistics()` (wipes the whole table). `GetAllApps` asserts an exact row count, so it will fail if run concurrently or against a pre-populated DB.
- Comments throughout the codebase are in Russian.
