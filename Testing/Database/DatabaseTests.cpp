#include <gtest/gtest.h>
#include <algorithm> // Добавлено для std::find
#include "Database/Database.h"

// Test fixture for Database tests
class DatabaseTest : public ::testing::Test {
protected:
    Database db;

    void SetUp() override {
        ASSERT_TRUE(db.initialize()) << "Failed to initialize database";
    }

    void TearDown() override {
        db.clearStatistics();
    }
};

// Test case for adding and retrieving key statistics
TEST_F(DatabaseTest, UpdateAndGetKeyStatistics) {
    std::string appName = "testApp";
    std::string keyCombo = "Ctrl+S";

    db.updateKeyStatistics(appName, keyCombo);

    std::string stats = db.getAppStatistics(appName);
    EXPECT_FALSE(stats.empty()) << "Statistics should not be empty";
    EXPECT_NE(stats.find(keyCombo), std::string::npos) << "Key combo not found in stats";
}

// Test case for clearing statistics
TEST_F(DatabaseTest, ClearStatistics) {
    db.updateKeyStatistics("testApp", "Ctrl+C");
    EXPECT_TRUE(db.clearStatistics()) << "Failed to clear statistics";
    std::string stats = db.getAppStatistics("testApp");
    EXPECT_TRUE(stats.empty() || stats == "No statistics available") << "Stats not cleared";
}

// Test case for retrieving all apps
TEST_F(DatabaseTest, GetAllApps) {
    db.updateKeyStatistics("app1", "Ctrl+A");
    db.updateKeyStatistics("app2", "Ctrl+B");
    auto apps = db.getAllApps();
    EXPECT_EQ(apps.size(), 2) << "Expected 2 apps";
    EXPECT_TRUE(std::find(apps.begin(), apps.end(), std::string("app1")) != apps.end()) << "app1 not found";
    EXPECT_TRUE(std::find(apps.begin(), apps.end(), std::string("app2")) != apps.end()) << "app2 not found";
}