#pragma once
#include <sqlite3.h>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <utility>

class Database {
private:
  sqlite3 *db;

  // Общий вспомогательный метод для разных запросов
  bool executePreparedQuery(const char* sql, 
                           const std::vector<std::variant<std::string, int>>& params, 
                           std::function<bool(sqlite3_stmt*)> processor = nullptr);
  
  // Методы для инициализации
  bool openDatabase(const std::string& dbPath);
  bool createTables();
  
  // Методы работы с данными
  std::vector<std::pair<std::string, int>> fetchAppKeyData(const std::string& appName, int limit);
  std::string formatStatisticsOutput(const std::vector<std::pair<std::string, int>>& data, 
                                    const std::string& appName);

public:
  Database();
  ~Database();

  // Основные модифицирующие методы
  bool initialize();
  void updateKeyStatistics(const std::string &appName,
                           const std::string &keyCombination);
  bool clearStatistics();

  // Немодифицирующие методы для работы с приложениями
  bool isConnected() const { return db != nullptr; }
  std::string getAppStatistics(const std::string &appName,
                               int limit = -1); // -1 = без ограничений
  std::vector<std::string> getAllApps();
};