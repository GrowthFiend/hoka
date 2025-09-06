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
*   **A C++17 Compiler:** (e.g., MinGW)
*   **CMake:** Version 3.15+

### Steps

1.  **Clone the repository and its submodules:**
    ```bash
    git clone --recursive https://github.com/GrowthFiend/hoka.git
    cd Hoka
    ```

2.  **Configure the project with CMake:**
    This command will automatically handle all dependencies (like FLTK and SQLite) via vcpkg.
    ```bash
    cmake -B build -S .
    ```

3.  **Build the project:**
    ```bash
    cmake --build build --config Release
    ```
    The output executable will be generated in the `build/Release/` directory.


## 🔮 Roadmap

- [ ] Project setup and dependency management (CMake, vcpkg).
- [ ] Research and implement low-level Windows keylogging hooks.
- [ ] Design and create the local SQLite database schema.
- [ ] Develop the core application logic for logging and storing hotkeys.
- [ ] Build the primary user interface with FLTK.
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