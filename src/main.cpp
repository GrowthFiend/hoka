#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include <fstream>
#include <iostream>
#include <memory>
#include <windows.h>
#include "Database/Database.h"
#include "KeyLogger/KeyLogger.h"
#include "UI/MainWindow.h"
#include "UI/SystemTray.h"

class HokaApplication {
private:
    std::unique_ptr<Database> db;
    std::unique_ptr<KeyLogger> logger;
    std::unique_ptr<MainWindow> window;
    std::unique_ptr<SystemTray> tray;
    
    bool initializeDatabase() {
        db = std::make_unique<Database>();
        if (!db->initialize()) {
            std::cerr << "Failed to initialize database" << std::endl;
            return false;
        }
        std::cout << "Database initialized" << std::endl;
        return true;
    }
    
    bool initializeSystemTray() {
        tray = std::make_unique<SystemTray>();
        if (!tray->initialize(L"Hoka Key Analyzer")) {
            std::cerr << "Failed to initialize system tray" << std::endl;
            return false;
        }
        tray->show();
        std::cout << "SystemTray initialized and shown" << std::endl;
        return true;
    }
    
    void initializeMainWindow() {
        window = std::make_unique<MainWindow>(800, 600, "Hoka Key Analyzer");
        window->size_range(400, 300, 0, 0);
        
        // Настройка кнопки Full Screen
        setupFullScreenButton();
        
        // Настройка взаимодействия с системным лотком
        window->setSystemTray(tray.get());
        window->setCloseToTray(true);
        
        // Настройка callbacks для окна
        setupWindowCallbacks();
        
        window->show();
        
        // Установка иконки
        setupWindowIcon();
        
        std::cout << "MainWindow created and shown" << std::endl;
    }
    
    void setupFullScreenButton() {
        Fl_Button* btnFullScreen = new Fl_Button(280, window->h() - 60, 100, 30, "Full Screen");
        btnFullScreen->callback([](Fl_Widget*, void* data) {
            MainWindow* w = static_cast<MainWindow*>(data);
            if (w->fullscreen_active()) {
                w->fullscreen_off(w->x(), w->y(), w->w(), w->h());
            } else {
                w->fullscreen();
            }
        }, window.get());
    }
    
    void setupWindowCallbacks() {
        // Callback для выбора приложения
        window->setOnAppSelectedCallback([this](const std::string& app) {
            std::cout << "AppSelectedCallback: selected app = " << app << std::endl;
            std::string stats = db->getAppStatistics(app);
            std::cout << "AppSelectedCallback: stats = " << stats << std::endl;
            window->updateAppStatistics(app, stats);
        });
        
        // Callback для очистки статистики
        window->setOnClearCallback([this]() {
            if (db->clearStatistics()) {
                std::cout << "ClearCallback: statistics cleared" << std::endl;
                window->clearRecentActivity();
                window->updateAppStatistics("", "");
                window->setStatus("Statistics cleared");
            }
        });
        
        // Callback для экспорта
        window->setOnExportCallback([this]() {
            exportStatistics();
        });
    }
    
    void setupSystemTrayCallbacks() {
        tray->onRestoreCallback = [this]() { 
            window->restoreFromTray(); 
        };
        
        tray->onDoubleClickCallback = [this]() { 
            window->restoreFromTray(); 
        };
        
        tray->onExitCallback = [this]() {
            shutdown();
        };
    }
    
    void setupWindowIcon() {
        HICON hIconBig = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101));
        HICON hIconSmall = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101));
        
        if (hIconBig && hIconSmall) {
            HWND hwnd = fl_xid(window.get());
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
        } else {
            MessageBoxW(NULL, L"Failed to Load Icon", L"Error", MB_ICONERROR);
        }
    }
    
    bool initializeKeyLogger() {
        logger = std::make_unique<KeyLogger>();
        
        // Устанавливаем callback для обработки событий клавиатуры
        auto keyEventCallback = [this](const KeyPressEvent& event) {
            handleKeyEvent(event);
        };
        
        if (!logger->start(keyEventCallback)) {
            std::cerr << "Failed to start key logger" << std::endl;
            return false;
        }
        
        std::cout << "KeyLogger started" << std::endl;
        return true;
    }
    
    void handleKeyEvent(const KeyPressEvent& event) {
        if (event.appName.empty() || event.keyCombination.empty()) {
            return;
        }
        
        // Обновляем статистику в базе данных
        db->updateKeyStatistics(event.appName, event.keyCombination);
        
        // Обновляем UI только если окно видимо (для производительности)
        if (window->visible()) {
            // Добавляем событие в список недавних нажатий
            window->addRecentKeyPress(event.appName, event.keyCombination);
            
            // Обновляем список приложений
            window->updateAppList(db->getAllApps());
            
            // Обновляем статистику выбранного приложения
            std::string selectedApp = window->getSelectedApp();
            if (!selectedApp.empty()) {
                std::string stats = db->getAppStatistics(selectedApp);
                window->updateAppStatistics(selectedApp, stats);
            }
        }
        
        // Обновляем tooltip в системном лотке
        updateSystemTrayTooltip(event);
    }
    
    void updateSystemTrayTooltip(const KeyPressEvent& event) {
        std::wstring w_appName(event.appName.begin(), event.appName.end());
        std::wstring w_keyCombo(event.keyCombination.begin(), event.keyCombination.end());
        tray->setTooltip(L"Hoka - Last: " + w_appName + L" → " + w_keyCombo);
    }
    
    void exportStatistics() {
        std::ofstream exportFile("hoka_stats.txt");
        if (exportFile) {
            auto apps = db->getAllApps();
            for (const auto& app : apps) {
                exportFile << "\nStatistics for " << app << ":\n";
                exportFile << db->getAppStatistics(app);
            }
            exportFile.close();
            window->showNotification("Exported to hoka_stats.txt");
            std::cout << "ExportCallback: exported to hoka_stats.txt" << std::endl;
        } else {
            window->showError("Failed to export");
            std::cout << "ExportCallback: failed to export" << std::endl;
        }
    }
    std::atomic<bool> shouldExit{false};
    void shutdown() {
        std::cout << "Shutting down application..." << std::endl;
        shouldExit = true;
    
        if (logger) {
            logger->stop();
        }
    
        if (tray) {
            tray->hide();
        }
    
        if (window) {
            window->hide();
        }
    
        // Принудительно завершаем FLTK
        exit(0);
    }

public:
    bool initialize() {
        std::cout << "Starting Hoka..." << std::endl;
        
        // Инициализируем компоненты в правильном порядке
        if (!initializeDatabase()) return false;
        if (!initializeSystemTray()) return false;
        
        initializeMainWindow();
        setupSystemTrayCallbacks();
        
        if (!initializeKeyLogger()) return false;
        
        return true;
    }
    
    int run() {
    // Предотвращаем автоматический выход когда все окна скрыты
        while (!shouldExit) {
            Fl::wait(0.1);
            if (!logger || !logger->isActive()) {
                break; // Выходим только когда приложение действительно завершается
            }
        }
        return 0;
    }
    
    ~HokaApplication() {
        shutdown();
    }
};

int main() {
    HokaApplication app;
    
    if (!app.initialize()) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }
    
    std::cout << "Application initialized successfully" << std::endl;
    return app.run();
}