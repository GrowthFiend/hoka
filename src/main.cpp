#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <windows.h>
#include "Database/Database.h"
#include "KeyLogger/KeyLogger.h"
#include "UI/MainWindow.h"
#include "UI/SystemTray.h"

void timerCallback(void *data) {
    auto *ctx = static_cast<std::tuple<KeyLogger*, Database*, MainWindow*, SystemTray*>*>(data);
    KeyLogger* logger = std::get<0>(*ctx);
    Database* db = std::get<1>(*ctx);
    MainWindow* window = std::get<2>(*ctx);
    SystemTray* tray = std::get<3>(*ctx);

    if (logger->hasEvents()) {
        KeyPressEvent event = logger->popEvent();
        if (!event.appName.empty() && !event.keyCombination.empty()) {
            db->updateKeyStatistics(event.appName, event.keyCombination);
            window->addRecentKeyPress(event.appName, event.keyCombination);
            std::wstring w_appName(event.appName.begin(), event.appName.end());
            std::wstring w_keyCombo(event.keyCombination.begin(), event.keyCombination.end());
            tray->setTooltip(L"Hoka - Last: " + w_appName + L" → " + w_keyCombo);
            window->updateAppList(db->getAllApps());
            std::string selectedApp = window->getSelectedApp();
            if (!selectedApp.empty()) {
                std::string stats = db->getAppStatistics(selectedApp);
                window->updateAppStatistics(selectedApp, stats);
            }
        }
    }

    if (window->visible() || tray->isVisible) {
        Fl::repeat_timeout(0.01, timerCallback, data); // Проверять каждые 10 мс
    } else {
        logger->stop();
        tray->hide();
        Fl::awake([](void*) { exit(0); }, nullptr);
    }
}

int main() {
    std::cout << "Starting Hoka..." << std::endl;

    Database db;
    if (!db.initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return 1;
    }
    std::cout << "Database initialized" << std::endl;

    KeyLogger logger;
    if (!logger.start()) {
        std::cerr << "Failed to start key logger" << std::endl;
        return 1;
    }
    std::cout << "KeyLogger started" << std::endl;

    SystemTray tray;
    if (!tray.initialize(L"Hoka Key Analyzer")) {
        std::cerr << "Failed to initialize system tray" << std::endl;
        return 1;
    }
    tray.show();
    std::cout << "SystemTray initialized and shown" << std::endl;

    MainWindow window(800, 600, "Hoka Key Analyzer");
    window.size_range(400, 300, 0, 0);
    std::cout << "MainWindow created" << std::endl;

    // Настройка кнопки Full Screen
    Fl_Button *btnFullScreen = new Fl_Button(280, window.h() - 60, 100, 30, "Full Screen");
    btnFullScreen->callback(
        [](Fl_Widget *, void *data) {
            MainWindow *w = static_cast<MainWindow *>(data);
            if (w->fullscreen_active()) {
                w->fullscreen_off(w->x(), w->y(), w->w(), w->h());
            } else {
                w->fullscreen();
            }
        },
        &window);

    window.setSystemTray(&tray);
    window.setCloseToTray(true);

    tray.onRestoreCallback = [&window]() { window.restoreFromTray(); };
    tray.onDoubleClickCallback = [&window]() { window.restoreFromTray(); };
    tray.onExitCallback = ([&logger, &db, &tray, &window]() {
        logger.stop();
        tray.hide();
        window.hide();
        Fl::awake([](void *) { exit(0); }, nullptr);
    });


    window.setOnAppSelectedCallback([&db, &window](const std::string &app) {
        std::cout << "AppSelectedCallback: selected app = " << app << std::endl;
        std::string stats = db.getAppStatistics(app);
        std::cout << "AppSelectedCallback: stats = " << stats << std::endl;
        window.updateAppStatistics(app, stats);
    });
    window.setOnClearCallback([&db, &window]() {
        if (db.clearStatistics()) {
            std::cout << "ClearCallback: statistics cleared" << std::endl;
            window.clearRecentActivity();
            window.updateAppStatistics("", "");
            window.setStatus("Statistics cleared");
        }
    });
    window.setOnExportCallback([&db, &window]() {
        std::ofstream exportFile("hoka_stats.txt");
        if (exportFile) {
            exportFile << db.getTopKeyPresses(100);
            auto apps = db.getAllApps();
            for (const auto &app : apps) {
                exportFile << "\nStatistics for " << app << ":\n";
                exportFile << db.getAppStatistics(app);
            }
            exportFile.close();
            window.showNotification("Exported to hoka_stats.txt");
            std::cout << "ExportCallback: exported to hoka_stats.txt" << std::endl;
        } else {
            window.showError("Failed to export");
            std::cout << "ExportCallback: failed to export" << std::endl;
        }
    });

    window.show();
    std::cout << "MainWindow shown" << std::endl;
    
    auto ctx = std::make_tuple(&logger, &db, &window, &tray);
    Fl::add_timeout(0.01, timerCallback, &ctx);

    HICON hIconBig = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101));
    HICON hIconSmall = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101));
    if (hIconBig && hIconSmall) {
        HWND hwnd = fl_xid(&window);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    } else {
        MessageBoxW(NULL, L"Failed to Load Icon", L"Error", MB_ICONERROR);
    }

    return Fl::run();
}