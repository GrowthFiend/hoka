# Hoka: Hot Key Analyzer

## 📖 Overview

**Hoka** (Hot Key Analyzer) is a personal Windows utility application currently in early development. Its mission is to help users optimize their workflow by providing deep insights into their hotkey usage across all software.

The application will run silently in the background, logging keyboard shortcuts used in different applications. By analyzing this data locally, Hoka will empower you to make informed decisions about remapping commands to more efficient and ergonomic key combinations, tailoring your software experience to your unique habits.

**Take control of your keyboard and boost your productivity.**

## ✨ Planned Features

*   **Global Hotkey Logging:** Capture keyboard shortcuts system-wide, associating them with the active application.
*   **Local Data Storage:** All data is stored securely and privately in a local SQLite database on your machine. No data is ever sent to the cloud.
*   **Usage Statistics Dashboard:** View your hotkey usage through intuitive charts and tables, identifying frequent, rare, and context-specific patterns.
*   **Personalized Insights:** Get data-driven suggestions for optimizing your hotkey layout based on your actual usage to improve ergonomics and efficiency.
*   **Lightweight & Fast:** Built with C++ and FLTK for a minimal footprint and snappy performance.

## 🚧 Project Status

**Early Development**

This project is in the very initial stages of development. Core architecture, logging functionality, and the user interface are currently being designed and implemented. Features, APIs, and installation processes are subject to change.

## 🛠️ Technology Stack (Planned)

*   **Language:** C++17
*   **GUI Toolkit:** FLTK (Fast Light Toolkit)
*   **Database:** SQLite (for local storage)
*   **Build System:** CMake
*   **Package Management:** vcpkg
*   **Platform:** Windows API (for low-level input hooks)

## 🚀 Building from Source (For Developers)

As the project is in early development, the build process is primarily intended for contributors.

### Prerequisites

*   **OS:** Windows 10 or 11
*   **Git**
*   **MinGW-w64 GCC** — the build is MinGW-only (vcpkg triplet `x64-mingw-dynamic`); Visual Studio is not needed and not supported. The tested toolchain is [WinLibs](https://winlibs.com/) GCC 15.2.0, POSIX threads, UCRT, *without* LLVM (`winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-*.zip`). WinLibs already bundles **CMake** and **Ninja**, so no separate CMake install is required.

### Setting up the toolchain

1.  Unpack the WinLibs archive so that the compiler ends up at `C:\mingw64\bin\g++.exe` (the archive contains a top-level `mingw64` folder — extract it into `C:\`). This is the path `CMakePresets.json` expects; if you put MinGW elsewhere, change the paths in the preset.
2.  Add `C:\mingw64\bin` to your user `PATH`. The presets add it for configure/build/test on their own, but running `hoka.exe` / `hoka_tests.exe` directly needs the MinGW runtime DLLs (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libwinpthread-1.dll`) from there.
3.  Open a new terminal and check: `g++ --version` and `cmake --version`.

### Steps

1.  **Clone the repository and its submodules:**
    ```bash
    git clone --recursive https://github.com/GrowthFiend/hoka.git
    cd hoka
    ```
    If you cloned without `--recursive`, run `git submodule update --init --recursive`.

2.  **Configure the project:**
    ```bash
    cmake --preset gcc-preset
    ```
    On the first run vcpkg bootstraps itself and builds FLTK, SQLite, GTest and their dependencies from source. This takes a long time (anywhere from several minutes to half an hour, mostly downloads — some upstream mirrors are slow and vcpkg retries them). Later configures reuse vcpkg's binary cache and are fast.

3.  **Build:**
    ```bash
    cmake --build --preset gcc-preset
    ```
    Executables are placed in `out/build/gcc-preset/`: `hoka.exe` (the app) and `hoka_tests.exe` (unit tests).

4.  **Run the tests:**
    ```bash
    ctest --preset gcc-preset
    ```
    The tests use their own temporary `hoka_test.db` and never touch the app's `keypress_stats.db`.

5.  **Run the app:** `hoka.exe` installs a global keyboard hook and should be run **as administrator**. It creates `keypress_stats.db` in the current working directory.


## 🔮 Roadmap

- [x] Project setup and dependency management (CMake, vcpkg).
- [x] Research and implement low-level Windows keylogging hooks.
- [x] Design and create the local SQLite database schema.
- [x] Develop the core application logic for logging and storing hotkeys.
- [x] Build the primary user interface with FLTK.
- [ ] Implement data analysis and visualization features.
- [ ] Alpha testing and refinement.

## 🤝 Contributing

As Hoka is in its early stages, ideas and contributions are highly welcome! Since the architecture is being defined, discussions on implementation approaches are valuable.

1.  Fork the Project.
2.  Create your Feature Branch (`git checkout -b feature/AmazingFeature`).
3.  Clearly communicate your plans and discuss them before major implementation.
4.  Commit your Changes (`git commit -m 'Add some AmazingFeature'`).
5.  Push to the Branch (`git push origin feature/AmazingFeature`).
6.  Open a Pull Request.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

*   FLTK team for the lightweight GUI toolkit.
*   SQLite developers for a robust embedded database solution.
*   GTest developers for a great framework.