#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/fl_ask.H>

#include <windows.h>
#include <psapi.h>
#include <sqlite3.h>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

// Структура для хранения информации о нажатии клавиши
struct KeyPress {
    std::string appName;
    std::string keyCombination;
    DWORD timestamp;
};

std::vector<KeyPress> keyHistory;
Fl_Multiline_Output* outputWidget = nullptr;
HHOOK keyboardHook = nullptr;
sqlite3* db = nullptr;

// Функция для инициализации базы данных SQLite
bool InitializeDatabase() {
    int rc = sqlite3_open("keypress_stats.db", &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Создаем таблицу для статистики нажатий
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS key_statistics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "app_name TEXT NOT NULL,"
        "key_combination TEXT NOT NULL,"
        "press_count INTEGER DEFAULT 1,"
        "last_pressed TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE(app_name, key_combination)"
        ");";

    char* errMsg = nullptr;
    rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

// Функция для обновления статистики в базе данных
void UpdateKeyStatistics(const std::string& appName, const std::string& keyCombination) {
    // Используем INSERT OR REPLACE для обновления счетчика:cite[1]:cite[4]
    const char* updateSQL = 
        "INSERT OR REPLACE INTO key_statistics (app_name, key_combination, press_count, last_pressed) "
        "VALUES (?, ?, COALESCE((SELECT press_count FROM key_statistics "
        "WHERE app_name = ? AND key_combination = ?) + 1, 1), CURRENT_TIMESTAMP);";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, updateSQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    // Привязываем параметры:cite[7]
    sqlite3_bind_text(stmt, 1, appName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, keyCombination.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, appName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, keyCombination.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

// Функция для получения топ-10 самых частых нажатий:cite[5]:cite[8]
std::string GetTopKeyPresses() {
    std::stringstream ss;
    const char* querySQL = 
        "SELECT app_name, key_combination, press_count "
        "FROM key_statistics "
        "ORDER BY press_count DESC, last_pressed DESC "
        "LIMIT 10;";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, querySQL, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "Error querying database";
    }

    ss << "Top 10 most frequent key presses:\n";
    ss << "---------------------------------\n";
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* appName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* keyCombination = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        int pressCount = sqlite3_column_int(stmt, 2);
        
        ss << appName << " - \"" << keyCombination << "\" (" << pressCount << " times)\n";
    }

    if (rc != SQLITE_DONE) {
        std::cerr << "Error during query: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return ss.str();
}

// Функция для получения имени процесса
std::string GetProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        char buffer[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, NULL, buffer, MAX_PATH) > 0) {
            CloseHandle(hProcess);
            return std::string(buffer);
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
}

// Функция для преобразования кода виртуальной клавиши в читаемый вид
std::string VirtualKeyToString(UINT vkCode) {
    // Буквы и цифры
    if ((vkCode >= 'A' && vkCode <= 'Z') || 
        (vkCode >= '0' && vkCode <= '9')) {
        return std::string(1, (char)vkCode);
    }

    // Специальные клавиши
    switch (vkCode) {
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab";
        case VK_ESCAPE: return "Esc";
        case VK_CONTROL: return "Ctrl";
        case VK_SHIFT: return "Shift";
        case VK_MENU: return "Alt";
        case VK_LWIN: case VK_RWIN: return "Win";
        case VK_UP: return "↑";
        case VK_DOWN: return "↓";
        case VK_LEFT: return "←";
        case VK_RIGHT: return "→";
        case VK_INSERT: return "Insert";
        case VK_DELETE: return "Delete";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "PageUp";
        case VK_NEXT: return "PageDown";
        case VK_ADD: return "+";
        case VK_SUBTRACT: return "-";
        case VK_MULTIPLY: return "*";
        case VK_DIVIDE: return "/";
        case VK_OEM_PERIOD: return ".";
        case VK_OEM_COMMA: return ",";
        case VK_OEM_1: return ";";
        case VK_OEM_2: return "/";
        case VK_OEM_3: return "`";
        case VK_OEM_4: return "[";
        case VK_OEM_5: return "\\";
        case VK_OEM_6: return "]";
        case VK_OEM_7: return "'";
        default: return "VK_" + std::to_string(vkCode);
    }
}

// Обработчик хука клавиатуры
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Получаем ID активного процесса
            DWORD processId;
            GetWindowThreadProcessId(GetForegroundWindow(), &processId);
            std::string appName = GetProcessName(processId);

            // Получаем состояние модификаторов
            bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
            bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
            bool altPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
            bool winPressed = (GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000;

            // Формируем строку с комбинацией клавиш
            std::string keyCombination;
            
            if (ctrlPressed) keyCombination += "Ctrl+";
            if (shiftPressed) keyCombination += "Shift+";
            if (altPressed) keyCombination += "Alt+";
            if (winPressed) keyCombination += "Win+";
            
            keyCombination += VirtualKeyToString(kbdStruct->vkCode);

            // Обновляем статистику в базе данных
            UpdateKeyStatistics(appName, keyCombination);

            // Добавляем запись в историю
            KeyPress newPress{appName, keyCombination, GetTickCount()};
            keyHistory.insert(keyHistory.begin(), newPress);

            // Ограничиваем историю 10 последними записями
            if (keyHistory.size() > 10) {
                keyHistory.pop_back();
            }

            // Обновляем отображение
            if (outputWidget) {
                std::stringstream ss;
                
                // Показываем последние 10 нажатий
                ss << "Last 10 key presses:\n";
                ss << "-------------------\n";
                for (const auto& press : keyHistory) {
                    ss << press.appName << " - \"" << press.keyCombination << "\"\n";
                }
                
                ss << "\n";
                ss << GetTopKeyPresses();
                
                outputWidget->value(ss.str().c_str());
                outputWidget->redraw();
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

// Функция для установки хука
void SetKeyboardHook() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (!keyboardHook) {
        fl_alert("Failed to set keyboard hook!");
    }
}

// Функция для снятия хука
void RemoveKeyboardHook() {
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }
}

// Обработчик закрытия окна
void window_callback(Fl_Widget* widget, void*) {
    RemoveKeyboardHook();
    if (db) {
        sqlite3_close(db);
    }
    exit(0);
}

int main(int argc, char** argv) {
    // Инициализируем базу данных
    if (!InitializeDatabase()) {
        fl_alert("Failed to initialize database!");
        return 1;
    }

    // Создаем главное окно
    Fl_Window* window = new Fl_Window(500, 600, "Hoka - Key Statistics Logger");
    window->callback(window_callback);

    // Создаем заголовок
    Fl_Box* title = new Fl_Box(10, 10, 480, 30, "Key Press Statistics");
    title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // Создаем область для вывода истории
    outputWidget = new Fl_Multiline_Output(10, 50, 480, 500);
    outputWidget->textsize(12);
    outputWidget->value("Waiting for key presses...\n\nPress keys in any application to see statistics.");

    window->end();
    window->show();

    // Устанавливаем хук клавиатуры
    SetKeyboardHook();

    // Запускаем главный цикл FLTK
    int result = Fl::run();

    // Снимаем хук перед выходом
    RemoveKeyboardHook();
    
    // Закрываем базу данных
    if (db) {
        sqlite3_close(db);
    }

    return result;
}