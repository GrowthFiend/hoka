#pragma once
#include "SystemTray.h" // Include full header instead of forward declaration
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Window.H>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h>

class MainWindow : public Fl_Window {
private:
  bool shouldCloseToTray = true;
  // UI elements
  Fl_Box *titleBox;
  Fl_Group *leftGroup;
  Fl_Group *rightGroup;

  // Left panel - Recent activity
  Fl_Box *recentTitle;
  Fl_Multiline_Output *recentActivity;

  // Right panel - App statistics
  Fl_Box *statsTitle;
  Fl_Choice *appChoice;
  Fl_Multiline_Output *appStats;

  // Bottom controls
  Fl_Button *btnClear;
  Fl_Button *btnRefresh;
  Fl_Button *btnExport;

  // Status bar
  Fl_Box *statusBox;

  // Data storage
  std::vector<std::string> recentKeys;
  std::vector<std::string> availableApps;

  // Callbacks
  std::function<void()> onCloseCallback;
  std::function<void()> onClearCallback;
  std::function<void()> onExportCallback;
  std::function<void()> onRefreshCallback;
  std::function<void(const std::string &)> onAppSelectedCallback;

  // Static callback methods
  static void windowCallback(Fl_Widget *widget, void *data);
  static void clearCallback(Fl_Widget *widget, void *data);
  static void exportCallback(Fl_Widget *widget, void *data);
  static void refreshCallback(Fl_Widget *widget, void *data);
  static void appChoiceCallback(Fl_Widget *widget, void *data);

  SystemTray *systemTray = nullptr;
  bool isMinimizedToTray = false;

  void updateLayout();          // Private helper
  void updateAppChoiceWidget(); // Declare the method

public:
  MainWindow(int width, int height, const char *title);

  void show() override;
  void hide() override;
  void resize(int x, int y, int w, int h) override;

  void minimizeToTray();
  void restoreFromTray();

  // Recent activity management
  void addRecentKeyPress(const std::string &appName,
                         const std::string &keyCombination);
  void clearRecentActivity();

  // App statistics management
  void updateAppList(const std::vector<std::string> &apps);
  void updateAppStatistics(const std::string &appName,
                           const std::string &statsText);
  std::string getSelectedApp() const;

  // Status and notifications
  void setStatus(const std::string &status);
  void showNotification(const std::string &message);
  void showError(const std::string &errorMessage);

  // Callback setters
  void setOnCloseCallback(std::function<void()> callback);
  void setOnClearCallback(std::function<void()> callback);
  void setOnExportCallback(std::function<void()> callback);
  void setOnRefreshCallback(std::function<void()> callback);
  void
  setOnAppSelectedCallback(std::function<void(const std::string &)> callback);

  // Helper methods
  void refreshUI();

  void setSystemTray(SystemTray *tray) { systemTray = tray; }
  int handle(int event) override;
  void setCloseToTray(bool enable) { shouldCloseToTray = enable; }
  bool getCloseToTray() const { return shouldCloseToTray; }
};