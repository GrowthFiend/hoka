#pragma once
#include "KeyPress.h"
#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

class KeyStatistics {
private:
  std::vector<KeyPress> keyPressHistory;
  size_t maxHistorySize;

public:
  // Конструктор с настройкой размера истории
  explicit KeyStatistics(size_t maxSize = 1000) : maxHistorySize(maxSize) {}

  // Добавление нового нажатия
  void addKeyPress(const KeyPress &keyPress) {
    keyPressHistory.push_back(keyPress);

    // Ограничиваем размер истории
    if (keyPressHistory.size() > maxHistorySize) {
      keyPressHistory.erase(keyPressHistory.begin());
    }
  }

  void addKeyPress(const std::string &appName,
                   const std::string &keyCombination) {
    addKeyPress(KeyPress(appName, keyCombination));
  }

  // Получение всей истории
  const std::vector<KeyPress> &getHistory() const { return keyPressHistory; }

  // Получение последних N нажатий
  std::vector<KeyPress> getRecentPresses(size_t count = 10) const {
    size_t startIndex =
        (keyPressHistory.size() > count) ? keyPressHistory.size() - count : 0;

    return std::vector<KeyPress>(keyPressHistory.begin() + startIndex,
                                 keyPressHistory.end());
  }

  // Статистика по приложениям
  std::map<std::string, int> getAppUsageStats() const {
    std::map<std::string, int> appStats;

    for (const auto &press : keyPressHistory) {
      appStats[press.appName]++;
    }

    return appStats;
  }

  // Статистика по клавишам (все приложения)
  std::map<std::string, int> getKeyUsageStats() const {
    std::map<std::string, int> keyStats;

    for (const auto &press : keyPressHistory) {
      keyStats[press.keyCombination]++;
    }

    return keyStats;
  }

  // Статистика по клавишам для конкретного приложения
  std::map<std::string, int> getAppKeyStats(const std::string &appName) const {
    std::map<std::string, int> appKeyStats;

    for (const auto &press : keyPressHistory) {
      if (press.appName == appName) {
        appKeyStats[press.keyCombination]++;
      }
    }

    return appKeyStats;
  }

  // Топ N самых используемых приложений
  std::vector<std::pair<std::string, int>> getTopApps(size_t limit = 10) const {
    auto appStats = getAppUsageStats();

    std::vector<std::pair<std::string, int>> sortedApps(appStats.begin(),
                                                        appStats.end());

    std::sort(sortedApps.begin(), sortedApps.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (sortedApps.size() > limit) {
      sortedApps.resize(limit);
    }

    return sortedApps;
  }

  // Топ N самых используемых клавиш (во всех приложениях)
  std::vector<std::pair<std::string, int>> getTopKeys(size_t limit = 10) const {
    auto keyStats = getKeyUsageStats();

    std::vector<std::pair<std::string, int>> sortedKeys(keyStats.begin(),
                                                        keyStats.end());

    std::sort(sortedKeys.begin(), sortedKeys.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (sortedKeys.size() > limit) {
      sortedKeys.resize(limit);
    }

    return sortedKeys;
  }

  // Топ N самых используемых клавиш для конкретного приложения
  std::vector<std::pair<std::string, int>>
  getTopKeysForApp(const std::string &appName, size_t limit = 10) const {

    auto appKeyStats = getAppKeyStats(appName);

    std::vector<std::pair<std::string, int>> sortedKeys(appKeyStats.begin(),
                                                        appKeyStats.end());

    std::sort(sortedKeys.begin(), sortedKeys.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (sortedKeys.size() > limit) {
      sortedKeys.resize(limit);
    }

    return sortedKeys;
  }

  // Поиск по приложению
  std::vector<KeyPress> findPressesByApp(const std::string &appName) const {
    std::vector<KeyPress> result;

    for (const auto &press : keyPressHistory) {
      if (press.appName == appName) {
        result.push_back(press);
      }
    }

    return result;
  }

  // Поиск по клавише
  std::vector<KeyPress>
  findPressesByKey(const std::string &keyCombination) const {
    std::vector<KeyPress> result;

    for (const auto &press : keyPressHistory) {
      if (press.keyCombination == keyCombination) {
        result.push_back(press);
      }
    }

    return result;
  }

  // Получение временного диапазона
  std::pair<unsigned long, unsigned long> getTimeRange() const {
    if (keyPressHistory.empty()) {
      return {0, 0};
    }

    auto minTime = keyPressHistory.front().timestamp;
    auto maxTime = keyPressHistory.back().timestamp;

    return {minTime, maxTime};
  }

  // Очистка истории
  void clearHistory() { keyPressHistory.clear(); }

  // Получение количества записей
  size_t getCount() const { return keyPressHistory.size(); }

  // Проверка на пустоту
  bool isEmpty() const { return keyPressHistory.empty(); }

  // Установка максимального размера истории
  void setMaxHistorySize(size_t newSize) {
    maxHistorySize = newSize;

    // Обрезаем историю если нужно
    if (keyPressHistory.size() > maxHistorySize) {
      keyPressHistory.erase(keyPressHistory.begin(),
                            keyPressHistory.begin() +
                                (keyPressHistory.size() - maxHistorySize));
    }
  }

  // Экспорт статистики в текстовом виде
  std::string exportStats() const {
    std::stringstream ss;

    ss << "Key Press Statistics\n";
    ss << "====================\n\n";

    ss << "Total presses: " << getCount() << "\n\n";

    auto timeRange = getTimeRange();
    ss << "Time range: " << timeRange.first << " - " << timeRange.second
       << "\n\n";

    ss << "Top Applications:\n";
    auto topApps = getTopApps(5);
    for (const auto &[app, count] : topApps) {
      ss << "  " << app << ": " << count << " presses\n";
    }

    ss << "\nTop Keys:\n";
    auto topKeys = getTopKeys(5);
    for (const auto &[key, count] : topKeys) {
      ss << "  " << key << ": " << count << " presses\n";
    }

    return ss.str();
  }
};