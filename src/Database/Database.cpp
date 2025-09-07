#include "Database.h"
#include <iomanip>
#include <iostream>
#include <sstream>


Database::Database() : db(nullptr) {}

Database::~Database() {
  if (db) {
    sqlite3_close(db);
  }
}

bool Database::initialize() {
  int rc = sqlite3_open("keypress_stats.db", &db);
  if (rc != SQLITE_OK) {
    std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
    return false;
  }

  // Создаем таблицу для статистики нажатий
  const char *createTableSQL =
      "CREATE TABLE IF NOT EXISTS key_statistics ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "app_name TEXT NOT NULL,"
      "key_combination TEXT NOT NULL,"
      "press_count INTEGER DEFAULT 1,"
      "last_pressed TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
      "UNIQUE(app_name, key_combination)"
      ");";

  char *errMsg = nullptr;
  rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  std::cout << "Database initialized successfully" << std::endl;
  return true;
}

void Database::updateKeyStatistics(const std::string &appName,
                                   const std::string &keyCombination) {
  if (!db) {
    std::cerr << "Database not initialized!" << std::endl;
    return;
  }

  // Используем INSERT OR REPLACE для обновления счетчика
  const char *updateSQL =
      "INSERT OR REPLACE INTO key_statistics (app_name, key_combination, "
      "press_count, last_pressed) "
      "VALUES (?, ?, COALESCE((SELECT press_count FROM key_statistics "
      "WHERE app_name = ? AND key_combination = ?) + 1, 1), "
      "CURRENT_TIMESTAMP);";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, updateSQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db)
              << std::endl;
    return;
  }

  // Привязываем параметры
  sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, keyCombination.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, appName.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 4, keyCombination.c_str(), -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db)
              << std::endl;
  }

  sqlite3_finalize(stmt);
}

std::string Database::getAppStatistics(const std::string &appName, int limit) {
  if (!db)
    return "Database not initialized!";

  std::stringstream ss;
  const char *querySQL = "SELECT key_combination, press_count "
                         "FROM key_statistics "
                         "WHERE app_name = ? "
                         "ORDER BY press_count DESC "
                         "LIMIT ?;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    return "Error querying database";
  }

  sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);

  bool hasData = false;
  int rank = 1;
  int totalPresses = 0;

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    hasData = true;
    const char *keyCombination =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    int pressCount = sqlite3_column_int(stmt, 1);
    totalPresses += pressCount;

    // Форматируем вывод с выравниванием
    ss << std::setw(2) << rank << ". " << std::setw(25) << std::left
       << keyCombination << std::setw(6) << std::right << pressCount
       << " times\n";
    rank++;
  }

  if (!hasData) {
    ss << "No key presses recorded for " << appName << " yet.\n";
  } else {
    // Добавляем общую статистику в конец
    ss << "\n" << std::string(40, '-') << "\n";
    ss << "Total combinations: " << (rank - 1) << "\n";
    ss << "Total key presses: " << totalPresses << "\n";
  }

  sqlite3_finalize(stmt);
  return ss.str();
}

std::vector<std::string> Database::getAllApps() {
  std::vector<std::string> apps;
  if (!db)
    return apps;

  const char *querySQL = "SELECT DISTINCT app_name FROM key_statistics "
                         "ORDER BY app_name;";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    return apps;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const char *appName =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    if (appName) {
      apps.push_back(std::string(appName));
    }
  }

  sqlite3_finalize(stmt);
  return apps;
}

int Database::getTotalKeyPresses() {
  if (!db)
    return -1;

  const char *querySQL = "SELECT SUM(press_count) FROM key_statistics;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    return -1;
  }

  int total = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    total = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return total;
}

int Database::getAppKeyPressCount(const std::string &appName) {
  if (!db)
    return -1;

  const char *querySQL =
      "SELECT SUM(press_count) FROM key_statistics WHERE app_name = ?;";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    return -1;
  }

  sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_STATIC);

  int total = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    total = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return total;
}

bool Database::clearStatistics() {
  if (!db)
    return false;

  const char *deleteSQL = "DELETE FROM key_statistics;";
  char *errMsg = nullptr;
  int rc = sqlite3_exec(db, deleteSQL, nullptr, nullptr, &errMsg);

  if (rc != SQLITE_OK) {
    std::cerr << "Failed to clear statistics: " << errMsg << std::endl;
    sqlite3_free(errMsg);
    return false;
  }

  return true;
}