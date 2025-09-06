#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H> // For fl_xid
#include <chrono>
#include <fstream> // For ofstream
#include <iostream>
#include <thread>
#include <windows.h>

#include "Database/Database.h"
#include "KeyLogger/KeyLogger.h"
#include "UI/MainWindow.h"
#include "UI/SystemTray.h"

int main() {
  // Инициализация базы данных
  Database db;
  if (!db.initialize()) {
    std::cerr << "Failed to initialize database" << std::endl;
    return 1;
  }

  // Инициализация кейлоггера
  KeyLogger logger;
  if (!logger.start()) {
    std::cerr << "Failed to start key logger" << std::endl;
    return 1;
  }

  // Инициализация системного трея
  SystemTray tray;
  if (!tray.initialize(L"Hoka Key Analyzer")) {
    std::cerr << "Failed to initialize system tray" << std::endl;
    return 1;
  }
  tray.show();

  // Инициализация главного окна
  MainWindow window(800, 600, "Hoka Key Analyzer");
  window.size_range(400, 300, 0,
                    0); // Делаем окно resizable (минимальный размер 400x300)

  // Добавляем кнопку для полноэкранного режима (в нижней части окна)
  Fl_Button *btnFullScreen =
      new Fl_Button(280, window.h() - 60, 100, 30, "Full Screen");
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

  // Устанавливаем иконку для окна (из ресурсов)
  HICON hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101),
                                 IMAGE_ICON, 32, 32, // Размер для окна
                                 LR_DEFAULTCOLOR);

  HICON hIconSmall = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(101), IMAGE_ICON,
                       16, 16, // Размер для маленькой иконки
                       LR_DEFAULTCOLOR);

  if (hIcon) {
    HWND hwnd = fl_xid(&window);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
  }

  // Связываем SystemTray с MainWindow
  window.setSystemTray(&tray);

  // Устанавливаем callbacks для трея
  tray.onRestoreCallback = [&window]() { window.restoreFromTray(); };
  tray.onDoubleClickCallback = [&window]() { window.restoreFromTray(); };
  tray.onExitCallback = ([&logger, &db, &tray, &window]() {
    logger.stop();
    tray.hide();
    window.hide();
    Fl::awake([](void *) { exit(0); }, nullptr);
  });

  // Устанавливаем callbacks для окна
  window.setOnCloseCallback([&window]() {
    window.hide(); // Вызываем hide(), который теперь сворачивает в трей
  });
  window.setOnRefreshCallback([&db, &window]() {
    auto apps = db.getAllApps();
    window.updateAppList(apps);
    std::string topKeys = db.getTopKeyPresses(10);
    // Можно обновить recent activity или другие части UI
  });
  window.setOnAppSelectedCallback([&db, &window](const std::string &app) {
    std::string stats = db.getAppStatistics(app);
    window.updateAppStatistics(app, stats);
  });
  window.setOnClearCallback([&db, &window]() {
    if (db.clearStatistics()) {
      window.clearRecentActivity();
      window.updateAppStatistics("", "");
      window.setStatus("Statistics cleared");
    }
  });
  window.setOnExportCallback([&db, &window]() {
    std::ofstream exportFile("hoka_stats.txt");
    if (exportFile) {
      exportFile << db.getTopKeyPresses(100); // Экспорт топа
      auto apps = db.getAllApps();
      for (const auto &app : apps) {
        exportFile << "\nStatistics for " << app << ":\n";
        exportFile << db.getAppStatistics(app);
      }
      exportFile.close();
      window.showNotification("Exported to hoka_stats.txt");
    } else {
      window.showError("Failed to export");
    }
  });

  // Показываем окно
  window.show(); // Call show() without arguments
  Fl::add_handler([](int event) -> int {
    if (event == FL_CLOSE) {
      // Проверяем, какое окно пытаются закрыть
      Fl_Window *win = Fl::first_window();
      if (win && win->visible()) {
        MainWindow *mainWin = dynamic_cast<MainWindow *>(win);
        if (mainWin) {
          mainWin->minimizeToTray();
          return 1; // Блокируем стандартную обработку закрытия
        }
      }
    }
    return 0;
  });
  // Основной цикл: обработка событий кейлоггера
  while (Fl::wait()) {
    if (logger.hasEvents()) {
      KeyPressEvent event = logger.popEvent();
      if (!event.appName.empty() && !event.keyCombination.empty()) {
        db.updateKeyStatistics(event.appName, event.keyCombination);
        window.addRecentKeyPress(event.appName, event.keyCombination);
        // Обновляем статус в трее
        std::wstring w_appName(event.appName.begin(), event.appName.end());
        std::wstring w_keyCombo(event.keyCombination.begin(), event.keyCombination.end());
        tray.setTooltip(L"Hoka - Last: " + w_appName + L" → " + w_keyCombo);
      }
    }
    // Небольшая задержка, чтобы не нагружать CPU
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Cleanup
  logger.stop();
  tray.hide();

  return Fl::run(); // Use Fl::run() for the event loop
}