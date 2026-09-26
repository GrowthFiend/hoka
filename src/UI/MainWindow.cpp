#include "MainWindow.h"
#include "Heatmap/Keyboards.h"
#include <FL/Fl.H>
#include <FL/Fl_Preferences.H>
#include <FL/fl_ask.H>
#include <algorithm>
#include <iostream>

namespace {
// Где хранится выбранная клавиатура между запусками
// (на Windows — %APPDATA%\hoka\hoka.prefs)
const char *const kPrefsVendor = "hoka";
const char *const kPrefsApplication = "hoka";
const char *const kPrefsKeyboard = "keyboard";

const char *const kSplitKeymapNote =
    "Для сплитов показан базовый слой default keymap QMK "
    "\xE2\x80\x94 у вас раскладка может отличаться.";
} // namespace

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
  statsTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  statsTitle->labelfont(FL_BOLD);
  statsTitle->labelsize(12);

  // Переключатель вида статистики (координаты задаёт updateLayout)
  btnViewText = new Fl_Radio_Round_Button(0, 55, 70, 25, "Текст");
  btnViewText->labelsize(12);
  btnViewText->value(1);
  btnViewText->callback(viewModeCallback, this);

  btnViewKeyboard = new Fl_Radio_Round_Button(0, 55, 100, 25, "Клавиатура");
  btnViewKeyboard->labelsize(12);
  btnViewKeyboard->callback(viewModeCallback, this);

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

  // Вид «Клавиатура»: выбор клавиатуры, флажок модификаторов и тепловая карта
  keyboardChoice = new Fl_Choice(0, 115, 100, 25, "Клавиатура:");
  keyboardChoice->labelsize(12);
  keyboardChoice->textsize(12);
  keyboardChoice->align(FL_ALIGN_LEFT);
  for (const heatmap::KeyboardLayout *keyboard : heatmap::allKeyboards()) {
    keyboardChoice->add(keyboard->name);
  }
  keyboardChoice->callback(keyboardChoiceCallback, this);

  chkModifiers = new Fl_Check_Button(0, 115, 185, 25, "Учитывать модификаторы");
  chkModifiers->labelsize(12);
  chkModifiers->value(1);
  chkModifiers->callback(modifiersCallback, this);

  heatmapView = new KeyboardHeatmap(0, 145, 100, 100);

  keyboardChoice->hide();
  chkModifiers->hide();
  heatmapView->hide();

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

  // Клавиатура, выбранная в прошлый раз
  {
    Fl_Preferences prefs(Fl_Preferences::USER, kPrefsVendor,
                         kPrefsApplication);
    char keyboardId[64];
    prefs.get(kPrefsKeyboard, keyboardId, heatmap::allKeyboards().front()->id,
              sizeof(keyboardId));
    selectKeyboard(keyboardId, false);
  }

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

  // В виде «Клавиатура» правой панели отдаём 2/3 ширины: карте нужно место.
  // В текстовом виде панели делят окно пополам, как раньше.
  int panelsW = w - 30;
  int rightW = keyboardMode ? panelsW * 2 / 3 : panelsW / 2;
  int leftW = keyboardMode ? panelsW - rightW : panelsW / 2;
  int rightX = 20 + leftW;
  int innerX = rightX + 5;
  int innerW = rightW - 10;

  titleBox->resize(10, 10, w - 20, 30);

  leftGroup->resize(10, 50, leftW, h - 120);
  recentTitle->resize(15, 55, leftW - 10, 25);
  recentActivity->resize(15, 85, leftW - 10, h - 170);

  rightGroup->resize(rightX, 50, rightW, h - 120);

  // Первая строка: заголовок слева, переключатель вида справа
  const int textButtonW = 70;
  const int keyboardButtonW = 100;
  btnViewText->resize(innerX + innerW - textButtonW - keyboardButtonW, 55,
                      textButtonW, 25);
  btnViewKeyboard->resize(innerX + innerW - keyboardButtonW, 55,
                          keyboardButtonW, 25);
  statsTitle->resize(innerX, 55,
                     std::max(0, innerW - textButtonW - keyboardButtonW), 25);

  appChoice->resize(innerX, 85, innerW, 25);
  appStats->resize(innerX, 115, innerW, h - 200);

  // Вид «Клавиатура»: выбор клавиатуры и флажок в одну строку, если
  // помещаются, иначе флажок переносится на следующую строку
  const int choiceLabelW = 80;
  const int checkW = 185;
  const int minChoiceW = 150;
  int heatmapY = 145;
  if (innerW >= choiceLabelW + minChoiceW + 10 + checkW) {
    keyboardChoice->resize(innerX + choiceLabelW, 115,
                           innerW - choiceLabelW - checkW - 10, 25);
    chkModifiers->resize(innerX + innerW - checkW, 115, checkW, 25);
  } else {
    keyboardChoice->resize(innerX + choiceLabelW, 115,
                           std::max(60, innerW - choiceLabelW), 25);
    chkModifiers->resize(innerX, 145, std::min(innerW, checkW), 25);
    heatmapY = 175;
  }
  // Низ карты совпадает с низом текстовой статистики
  heatmapView->resize(innerX, heatmapY, innerW,
                      std::max(0, (h - 85) - heatmapY));

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
  // Запоминаем выбранное приложение по имени: список пересоздаётся при
  // каждом нажатии, и без этого выбор сбрасывался на "Select an app..."
  std::string selected;
  if (appChoice->value() > 0 && appChoice->text()) {
    selected = appChoice->text();
  }

  appChoice->clear();
  appChoice->add("Select an app...");

  int selectedIndex = 0;
  for (size_t i = 0; i < availableApps.size(); ++i) {
    appChoice->add(availableApps[i].c_str());
    if (!selected.empty() && availableApps[i] == selected) {
      selectedIndex = static_cast<int>(i) + 1; // +1: первый пункт "Select an app..."
    }
  }

  appChoice->value(selectedIndex);
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

void MainWindow::updateAppKeyCounts(
    const std::string &appName,
    const std::vector<std::pair<std::string, int>> &keyCounts) {
  heatmapView->setKeyCounts(appName, keyCounts);
}

void MainWindow::setKeyboardMode(bool enable) {
  keyboardMode = enable;
  btnViewText->value(enable ? 0 : 1);
  btnViewKeyboard->value(enable ? 1 : 0);
  if (enable) {
    appStats->hide();
    keyboardChoice->show();
    chkModifiers->show();
    heatmapView->show();
  } else {
    keyboardChoice->hide();
    chkModifiers->hide();
    heatmapView->hide();
    appStats->show();
  }
  updateLayout();
}

void MainWindow::selectKeyboard(const std::string &keyboardId, bool remember) {
  const auto &keyboards = heatmap::allKeyboards();
  size_t index = 0; // неизвестный id — первая клавиатура списка
  for (size_t i = 0; i < keyboards.size(); ++i) {
    if (keyboardId == keyboards[i]->id) {
      index = i;
      break;
    }
  }
  const heatmap::KeyboardLayout *keyboard = keyboards[index];
  keyboardChoice->value(static_cast<int>(index));
  keyboardChoice->tooltip(keyboard->isSplit ? kSplitKeymapNote : nullptr);
  heatmapView->setKeyboard(keyboard);

  if (remember) {
    Fl_Preferences prefs(Fl_Preferences::USER, kPrefsVendor,
                         kPrefsApplication);
    prefs.set(kPrefsKeyboard, keyboard->id);
    prefs.flush();
  }
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

void MainWindow::clearCallback(Fl_Widget *, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  if (window->onClearCallback) {
    window->onClearCallback();
  }
  window->clearRecentActivity();
  window->updateAppStatistics("", "");
  window->updateAppKeyCounts("", {});
  window->setStatus("Statistics cleared");
}

void MainWindow::exportCallback(Fl_Widget *, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  if (window->onExportCallback) {
    window->onExportCallback();
  }
}

void MainWindow::appChoiceCallback(Fl_Widget *, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  std::string selectedApp = window->getSelectedApp();

  if (!selectedApp.empty() && window->onAppSelectedCallback) {
    window->onAppSelectedCallback(selectedApp);
  }
}

void MainWindow::viewModeCallback(Fl_Widget *widget, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  window->setKeyboardMode(widget == window->btnViewKeyboard);
}

void MainWindow::keyboardChoiceCallback(Fl_Widget *, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  const auto &keyboards = heatmap::allKeyboards();
  int index = window->keyboardChoice->value();
  if (index >= 0 && index < static_cast<int>(keyboards.size())) {
    window->selectKeyboard(keyboards[index]->id, true);
  }
}

void MainWindow::modifiersCallback(Fl_Widget *, void *data) {
  MainWindow *window = static_cast<MainWindow *>(data);
  window->heatmapView->setIncludeModifiers(window->chkModifiers->value() != 0);
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