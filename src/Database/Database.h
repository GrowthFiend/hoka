#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
private:
  sqlite3 *db;

public:
  Database();
  ~Database();

  // Основные методы
  bool initialize();
  void updateKeyStatistics(const std::string &appName,
                           const std::string &keyCombination);
  std::string getTopKeyPresses(int limit = 10);

  // Методы для работы с приложениями
  std::string getAppStatistics(const std::string &appName,
                               int limit = -1); // -1 = без ограничений
  std::vector<std::string> getAllApps();
  int getAppKeyPressCount(const std::string &appName);

  // Общая статистика
  int getTotalKeyPresses();
  bool clearStatistics();

  // Методы для отладки
  bool isConnected() const { return db != nullptr; }
};