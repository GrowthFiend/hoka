#include "Database.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <variant>

Database::Database() : db(nullptr) {}

Database::~Database() {
  if (db) {
    sqlite3_close(db);
  }
}

bool Database::executePreparedQuery(const char* sql, 
                                   const std::vector<std::variant<std::string, int>>& params, 
                                   std::function<bool(sqlite3_stmt*)> processor) {
    if (!db) {
        std::cerr << "Database not initialized!" << std::endl;
        return false;
    }

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Биндим параметры с учетом типа
    for (size_t i = 0; i < params.size(); ++i) {
        std::visit([stmt, i](auto&& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::string>) {
                sqlite3_bind_text(stmt, i + 1, value.c_str(), -1, SQLITE_STATIC);
            } else if constexpr (std::is_same_v<T, int>) {
                sqlite3_bind_int(stmt, i + 1, value);
            }
        }, params[i]);
    }

    bool success = true;
    if (processor) {
        success = processor(stmt);
    } else {
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
            success = false;
        }
    }

    sqlite3_finalize(stmt);
    return success;
}

bool Database::openDatabase(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

bool Database::createTables() {
    return executePreparedQuery("CREATE TABLE IF NOT EXISTS key_statistics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "app_name TEXT NOT NULL,"
        "key_combination TEXT NOT NULL,"
        "press_count INTEGER DEFAULT 1,"
        "last_pressed TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE(app_name, key_combination)"
        ");", {});
}

std::vector<std::pair<std::string, int>> Database::fetchAppKeyData(const std::string& appName, int limit) {
    std::vector<std::pair<std::string, int>> data;

    auto processor = [&](sqlite3_stmt* stmt) -> bool {
        int rc;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const char *keyCombination =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            int pressCount = sqlite3_column_int(stmt, 1);
            
            if (keyCombination) {
                data.emplace_back(std::string(keyCombination), pressCount);
            }
        }
        return rc == SQLITE_DONE;
    };

    executePreparedQuery("SELECT key_combination, press_count "
                           "FROM key_statistics "
                           "WHERE app_name = ? "
                           "ORDER BY press_count DESC "
                           "LIMIT ?;", {appName, limit}, processor);
    return data;
}

std::string Database::formatStatisticsOutput(const std::vector<std::pair<std::string, int>>& data, 
                                           const std::string& appName) {
    std::stringstream ss;
    
    if (data.empty()) {
        ss << "No key presses recorded for " << appName << " yet.\n";
        return ss.str();
    }

    int rank = 1;
    int totalPresses = 0;
    
    for (const auto& [keyCombination, pressCount] : data) {
        totalPresses += pressCount;
        ss << std::setw(2) << rank << ". " << std::setw(25) << std::left
           << keyCombination << std::setw(6) << std::right << pressCount
           << " times\n";
        rank++;
    }

    ss << "\n" << std::string(40, '-') << "\n";
    ss << "Total combinations: " << data.size() << "\n";
    ss << "Total key presses: " << totalPresses << "\n";
    
    return ss.str();
}

bool Database::initialize() {
    if (!openDatabase("keypress_stats.db")) {
        return false;
    }
    
    if (!createTables()) {
        return false;
    }
    
    std::cout << "Database initialized successfully" << std::endl;
    return true;
}

void Database::updateKeyStatistics(const std::string &appName,
                                   const std::string &keyCombination) {
    executePreparedQuery("INSERT OR REPLACE INTO key_statistics (app_name, key_combination, "
        "press_count, last_pressed) "
        "VALUES (?, ?, COALESCE((SELECT press_count FROM key_statistics "
        "WHERE app_name = ? AND key_combination = ?) + 1, 1), "
        "CURRENT_TIMESTAMP);", {appName, keyCombination, appName, keyCombination});
}

std::string Database::getAppStatistics(const std::string &appName, int limit) {
    auto data = fetchAppKeyData(appName, limit);
    if (data.empty() && !db) {
        return "Database not initialized!";
    }
    return formatStatisticsOutput(data, appName);
}

std::vector<std::string> Database::getAllApps() {
    std::vector<std::string> apps;
    auto processor = [&](sqlite3_stmt* stmt) -> bool {
        int rc;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            const char *appName =
                reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
            if (appName) {
                apps.push_back(std::string(appName));
            }
        }
        return rc == SQLITE_DONE;
    };

    executePreparedQuery("SELECT DISTINCT app_name FROM key_statistics "
                           "ORDER BY app_name;", {}, processor);
    return apps;
}

bool Database::clearStatistics() {
    return executePreparedQuery("DELETE FROM key_statistics;", {});
}