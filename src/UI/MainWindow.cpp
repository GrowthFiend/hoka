#include "MainWindow.h"
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <iostream>

MainWindow::MainWindow(int width, int height, const char *title)
    : Fl_Window(width, height, title) {

  color(FL_WHITE);
  begin();

  // Title
  titleBox = new Fl_Box(10, 10, width - 20, 30, "Hoka - Hot Key Analyzer");
  titleBox->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);
  titleBox->labelfont(FL_BOLD);
  titleBox->labelsize(16);

  // Left group - Recent Activity
  leftGroup = new Fl_Group(10, 50, (width - 30) / 2, height - 120);
  leftGroup->box(FL_BORDER_BOX);
  leftGroup->begin();

  recentTitle =
      new Fl_Box(15, 55, (width - 30) / 2 - 10, 25, "Recent Key Presses");
  recentTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  recentTitle->labelfont(FL_BOLD);
  recentTitle->labelsize(12);

  recentActivity =
      new Fl_Multiline_Output(15, 85, (width - 30) / 2 - 10, height - 170);
  recentActivity->textsize(11);
  recentActivity->value("Waiting for key presses...");

  leftGroup->end();

  // Right group - App Statistics
  rightGroup =
      new Fl_Group(20 + (width - 30) / 2, 50, (width - 30) / 2, height - 120);
  rightGroup->box(FL_BORDER_BOX);
  rightGroup->begin();

  statsTitle = new Fl_Box(25 + (width - 30) / 2, 55, (width - 30) / 2 - 10, 25,
                          "App Statistics");
  statsTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  statsTitle->labelfont(FL_BOLD);
  statsTitle->labelsize(12);

  // App selection dropdown
  appChoice =
      new Fl_Choice(25 + (width - 30) / 2, 85, (width - 30) / 2 - 10, 25, "");
  appChoice->callback(appChoiceCallback, this);
  appChoice->add("Select an app...");
  appChoice->value(0);

  // Statistics display
  appStats = new Fl_Multiline_Output(25 + (width - 30) / 2, 115,
                                     (width - 30) / 2 - 10, height - 200);
  appStats->textsize(11);
  appStats->value("Select an application to view statistics");

  rightGroup->end();

  // Bottom controls
  btnClear = new Fl_Button(10, height - 60, 80, 30, "Clear");
  btnClear->callback(clearCallback, this);

  btnExport = new Fl_Button(100, height - 60, 80, 30, "Export");
  btnExport->callback(exportCallback, this);

  // Status bar
  statusBox = new Fl_Box(10, height - 25, width - 20, 20,
                         "Ready - Start pressing keys to see activity");
  statusBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  statusBox->labelfont(FL_ITALIC);
  statusBox->labelsize(10);

  end();
  
  resizable(this); // Make window resizable
  updateLayout();  // Initial layout
}

void MainWindow::show() {
  Fl_Window::show();
  isMinimizedToTray = false;
}

void MainWindow::hide() {
  if (shouldCloseToTray) {
    minimizeToTray(); // Сворачиваем в трей вместо закрытия
  } else {
    Fl_Window::hide(); // Настоящее скрытие
  }
}

void MainWindow::minimizeToTray() {
  Fl_Window::hide();  // Call BASE hide() to avoid recursion
  isMinimizedToTray = true;
  if (systemTray) {
    systemTray->showBalloon(
        L"Hoka Key Analyzer",
        L"Application minimized to system tray.\nDouble-click icon to restore.",
        5000);
  }
}

void MainWindow::restoreFromTray() {
  show();
  isMinimizedToTray = false;
  Fl::focus(this);
}

void MainWindow::updateLayout() {
  // Recalculate positions on resize
  int w = this->w();
  int h = this->h();

  titleBox->resize(10, 10, w - 20, 30);

  leftGroup->resize(10, 50, (w - 30) / 2, h - 120);
  recentTitle->resize(15, 55, (w - 30) / 2 - 10, 25);
  recentActivity->resize(15, 85, (w - 30) / 2 - 10, h - 170);

  rightGroup->resize(20 + (w - 30) / 2, 50, (w - 30) / 2, h - 120);
  statsTitle->resize(25 + (w - 30) / 2, 55, (w - 30) / 2 - 10, 25);
  appChoice->resize(25 + (w - 30) / 2, 85, (w - 30) / 2 - 10, 25);
  appStats->resize(25 + (w - 30) / 2, 115, (w - 30) / 2 - 10, h - 200);

  btnClear->resize(10, h - 60, 80, 30);
  btnExport->resize(100, h - 60, 80, 30);

  statusBox->resize(10, h - 25, w - 20, 20);

  redraw();
}

void MainWindow::resize(int x, int y, int w, int h) {
  Fl_Window::resize(x, y, w, h);
  updateLayout();
}

void MainWindow::addRecentKeyPress(const std::string &appName,
                                   const std::string &keyCombination) {
  std::string entry = appName + " → " + keyCombination;

  // Add to beginning of list
  recentKeys.insert(recentKeys.begin(), entry);

  // Keep only last 10 entries
  if (recentKeys.size() > 10) {
    recentKeys.resize(10);
  }

  // Update display
  std::string display;
  for (size_t i = 0; i < recentKeys.size(); ++i) {
    display += std::to_string(i + 1) + ". " + recentKeys[i] + "\n";
  }

  if (recentActivity) {
    recentActivity->value(display.c_str());
    recentActivity->redraw();
  }

  // Update status
  setStatus("Last: " + entry);

  // Add app to available apps if not already there
  if (std::find(availableApps.begin(), availableApps.end(), appName) ==
      availableApps.end()) {
    availableApps.push_back(appName);
    updateAppChoiceWidget();
  }

  // Auto-update app statistics if the current selected app matches or no app
  // selected
  std::string selectedApp = getSelectedApp();
  if ((selectedApp == appName || selectedApp.empty()) &&
      onAppSelectedCallback) {
    if (selectedApp.empty()) {
      // Select the app if none selected
      for (int i = 0; i < appChoice->size(); ++i) {
        if (std::string(appChoice->text(i)) == appName) {
          appChoice->value(i);
          break;
        }
      }
    }
    onAppSelectedCallback(appName);
  }

  // Update tray tooltip with latest activity
  if (systemTray && isMinimizedToTray) {
    std::wstring w_entry(entry.begin(), entry.end());
    systemTray->setTooltip(L"Hoka Key Analyzer - Last: " + w_entry);
  }
}

void MainWindow::updateAppChoiceWidget() {
  if (!appChoice)
    return;
  appChoice->clear();
  appChoice->add("Select an app...");

  for (const auto &app : availableApps) {
    appChoice->add(app.c_str());
  }

  appChoice->value(0);
  appChoice->redraw();
}

void MainWindow::clearRecentActivity() {
  recentKeys.clear();
  recentActivity->value("Waiting for key presses...");
  recentActivity->redraw();
}

void MainWindow::updateAppList(const std::vector<std::string> &apps) {
  availableApps = apps;
  updateAppChoiceWidget();
}

void MainWindow::updateAppStatistics(const std::string &appName,
                                     const std::string &statsText) {
  if (appName.empty() || statsText.empty()) {
    appStats->value("No statistics available");
  } else {
    std::string formattedStats = "Statistics for " + appName + ":\n";
    formattedStats += "================================\n";
    formattedStats += statsText;
    appStats->value(formattedStats.c_str());
  }
  appStats->redraw();
}

std::string MainWindow::getSelectedApp() const {
  if (appChoice->value() <= 0 ||
      appChoice->value() > (int)availableApps.size()) {
    return "";
  }
  return availableApps[appChoice->value() -
                       1]; // -1 because first item is "Select an app..."
}

void MainWindow::setStatus(const std::string &status) {
  if (statusBox) {
    statusBox->copy_label(status.c_str());
    statusBox->redraw();
  }
}

void MainWindow::showNotification(const std::string &message) {
  fl_message_title("Hoka Notification");
  fl_message("%s", message.c_str());
}

void MainWindow::showError(const std::string &errorMessage) {
  fl_alert("Error: %s", errorMessage.c_str());
  setStatus("Error: " + errorMessage);
}

void MainWindow::clearCallback(Fl_Widget *widget, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  if (window->onClearCallback) {
    window->onClearCallback();
  }
  window->clearRecentActivity();
  window->updateAppStatistics("", "");
  window->setStatus("Statistics cleared");
}

void MainWindow::exportCallback(Fl_Widget *widget, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  if (window->onExportCallback) {
    window->onExportCallback();
  }
}

void MainWindow::appChoiceCallback(Fl_Widget *widget, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  std::string selectedApp = window->getSelectedApp();

  if (!selectedApp.empty() && window->onAppSelectedCallback) {
    window->onAppSelectedCallback(selectedApp);
  }
}

// Callback setters
void MainWindow::setOnCloseCallback(std::function<void()> callback) {
  onCloseCallback = callback;
}

void MainWindow::setOnClearCallback(std::function<void()> callback) {
  onClearCallback = callback;
}

void MainWindow::setOnExportCallback(std::function<void()> callback) {
  onExportCallback = callback;
}

void MainWindow::setOnAppSelectedCallback(
    std::function<void(const std::string &)> callback) {
  onAppSelectedCallback = callback;
}

int MainWindow::handle(int event) {
    if (event == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
        return 1; // Игнорируем ESC
    }

    // Перехватываем событие закрытия окна
    if (event == FL_CLOSE) {
        if (shouldCloseToTray) {
            minimizeToTray(); // Сворачиваем в трей вместо закрытия
            return 1; // ВАЖНО: возвращаем 1, чтобы предотвратить стандартную обработку
        }
    }

    return Fl_Window::handle(event);
}