#include <gtest/gtest.h>
#include <algorithm> // Добавлено для std::find
#include <cstdio>
#include <memory>
#include "Database/Database.h"

// Отдельный файл БД, чтобы тесты не трогали keypress_stats.db приложения
static const char *kTestDbPath = "hoka_test.db";

// Test fixture for Database tests
class DatabaseTest : public ::testing::Test {
protected:
    std::unique_ptr<Database> db;

    void SetUp() override {
        std::remove(kTestDbPath); // Каждый тест начинает с пустой БД
        db = std::make_unique<Database>();
        ASSERT_TRUE(db->initialize(kTestDbPath)) << "Failed to initialize database";
    }

    void TearDown() override {
        db.reset(); // Закрываем соединение, иначе Windows не даст удалить файл
        std::remove(kTestDbPath);
    }
};

// Test case for adding and retrieving key statistics
TEST_F(DatabaseTest, UpdateAndGetKeyStatistics) {
    std::string appName = "testApp";
    std::string keyCombo = "Ctrl+S";

    db->updateKeyStatistics(appName, keyCombo);

    std::string stats = db->getAppStatistics(appName);
    EXPECT_FALSE(stats.empty()) << "Statistics should not be empty";
    EXPECT_NE(stats.find(keyCombo), std::string::npos) << "Key combo not found in stats";
}

// Test case for clearing statistics
TEST_F(DatabaseTest, ClearStatistics) {
    db->updateKeyStatistics("testApp", "Ctrl+C");
    
    // Проверьте, что данные действительно добавились
    std::string before = db->getAppStatistics("testApp");
    std::cout << "Before clear: " << before << std::endl;
    
    EXPECT_TRUE(db->clearStatistics()) << "Failed to clear statistics";
    
    // Проверьте количество записей после очистки
    std::string stats = db->getAppStatistics("testApp");
    std::cout << "After clear: " << stats << std::endl;
    
    EXPECT_TRUE(stats.empty() || stats == "No key presses recorded for testApp yet.\n") << "Stats not cleared";
}

// Test case for retrieving all apps
TEST_F(DatabaseTest, GetAllApps) {
    db->updateKeyStatistics("app1", "Ctrl+A");
    db->updateKeyStatistics("app2", "Ctrl+B");
    auto apps = db->getAllApps();
    EXPECT_EQ(apps.size(), 2) << "Expected 2 apps";
    EXPECT_TRUE(std::find(apps.begin(), apps.end(), std::string("app1")) != apps.end()) << "app1 not found";
    EXPECT_TRUE(std::find(apps.begin(), apps.end(), std::string("app2")) != apps.end()) << "app2 not found";
}
// Пары (комбинация, число нажатий) для тепловой карты
TEST_F(DatabaseTest, GetAppKeyCounts) {
    db->updateKeyStatistics("app1", "Ctrl+S");
    db->updateKeyStatistics("app1", "Ctrl+S");
    db->updateKeyStatistics("app1", "Ctrl++");
    db->updateKeyStatistics("app2", "A");

    auto counts = db->getAppKeyCounts("app1");
    ASSERT_EQ(counts.size(), 2u);
    // По убыванию числа нажатий
    EXPECT_EQ(counts[0].first, "Ctrl+S");
    EXPECT_EQ(counts[0].second, 2);
    EXPECT_EQ(counts[1].first, "Ctrl++");
    EXPECT_EQ(counts[1].second, 1);

    EXPECT_TRUE(db->getAppKeyCounts("no-such-app").empty());
}
