#include "KeyLogger.h"
#include <iostream>
#include <psapi.h>
#include <sstream>

// Инициализация статического члена
KeyLogger* KeyLogger::instance = nullptr;

KeyLogger::KeyLogger() : keyboardHook(nullptr) {
    instance = this;
}

KeyLogger::~KeyLogger() {
    stop();
    instance = nullptr;
}

bool KeyLogger::start(KeyEventCallback callback) {
    if (isRunning) {
        return true;
    }
    
    if (callback) {
        eventCallback = callback;
    }
    
    // Устанавливаем hook
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, 
                                   GetModuleHandle(NULL), 0);
    
    if (!keyboardHook) {
        DWORD error = GetLastError();
        std::cerr << "Failed to start keyboard hook! Error code: " << error << std::endl;
        return false;
    }
    
    // Запускаем поток обработки
    shouldStop = false;
    isRunning = true;
    processingThread = std::thread(&KeyLogger::processEvents, this);
    
    std::cout << "KeyLogger started successfully" << std::endl;
    return true;
}

void KeyLogger::stop() {
    if (!isRunning) {
        return;
    }
    
    // Останавливаем hook
    if (keyboardHook) {
        UnhookWindowsHookEx(keyboardHook);
        keyboardHook = nullptr;
    }
    
    // Останавливаем поток обработки
    shouldStop = true;
    queueCondition.notify_all();
    
    if (processingThread.joinable()) {
        processingThread.join();
    }
    
    isRunning = false;
    std::cout << "KeyLogger stopped" << std::endl;
}

void KeyLogger::setEventCallback(KeyEventCallback callback) {
    eventCallback = callback;
}

bool KeyLogger::isActive() const {
    return isRunning;
}

void KeyLogger::addEvent(const KeyPressEvent& event) {
    std::unique_lock<std::mutex> lock(queueMutex);
    eventQueue.push(event);
    queueCondition.notify_one();
}

void KeyLogger::processEvents() {
    while (!shouldStop) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // Ждем события или сигнала остановки
        queueCondition.wait(lock, [this] { 
            return !eventQueue.empty() || shouldStop; 
        });
        
        // Обрабатываем все события в очереди
        while (!eventQueue.empty() && !shouldStop) {
            KeyPressEvent event = eventQueue.front();
            eventQueue.pop();
            
            // Освобождаем mutex перед вызовом callback
            lock.unlock();
            
            // Вызываем callback если установлен
            if (eventCallback) {
                try {
                    eventCallback(event);
                } catch (const std::exception& e) {
                    std::cerr << "Exception in event callback: " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown exception in event callback" << std::endl;
                }
            }
            
            std::cout << "Processed event: " << event.appName 
                     << " - " << event.keyCombination << std::endl;
            
            lock.lock();
        }
    }
}

LRESULT CALLBACK KeyLogger::keyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && instance && 
        (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {
        
        KBDLLHOOKSTRUCT* kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        
        // Получаем активное окно и процесс
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow) {
            DWORD processId;
            GetWindowThreadProcessId(foregroundWindow, &processId);
            std::string appName = getProcessName(processId);
            
            // Получаем состояние модификаторов
            bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
            bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
            bool altPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
            bool winPressed = (GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000;
            
            // Формируем комбинацию клавиш
            std::string mainKey = virtualKeyToString(kbdStruct->vkCode);
            
            // Исключаем одиночные нажатия модификаторов
            if (mainKey == "Ctrl" || mainKey == "Shift" || 
                mainKey == "Alt" || mainKey == "Win") {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }
            
            std::string keyCombination;
            
            // Добавляем модификаторы
            if (mainKey != "Ctrl" && ctrlPressed) keyCombination += "Ctrl+";
            if (mainKey != "Shift" && shiftPressed) keyCombination += "Shift+";
            if (mainKey != "Alt" && altPressed) keyCombination += "Alt+";
            if (mainKey != "Win" && winPressed) keyCombination += "Win+";
            
            keyCombination += mainKey;
            
            // Добавляем событие в очередь
            KeyPressEvent event{appName, keyCombination};
            instance->addEvent(event);
        }
    }
    
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

std::string KeyLogger::getProcessName(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                 FALSE, processId);
    if (hProcess) {
        char buffer[MAX_PATH];
        DWORD size = sizeof(buffer);
        if (GetModuleBaseNameA(hProcess, NULL, buffer, size) > 0) {
            CloseHandle(hProcess);
            return std::string(buffer);
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
}

std::string KeyLogger::virtualKeyToString(UINT vkCode) {
    // Буквы и цифры
    if ((vkCode >= 'A' && vkCode <= 'Z') || (vkCode >= '0' && vkCode <= '9')) {
        return std::string(1, (char)vkCode);
    }
    
    // Функциональные клавиши
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return "F" + std::to_string(vkCode - VK_F1 + 1);
    }
    
    // Специальные клавиши
    switch (vkCode) {
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_BACK: return "Backspace";
        case VK_TAB: return "Tab";
        case VK_ESCAPE: return "Esc";
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_CONTROL: return "Ctrl";
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_SHIFT: return "Shift";
        case VK_LMENU:
        case VK_RMENU:
        case VK_MENU: return "Alt";
        case VK_LWIN:
        case VK_RWIN: return "Win";
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
        case VK_CAPITAL: return "CapsLock";
        case VK_NUMLOCK: return "NumLock";
        case VK_SCROLL: return "ScrollLock";
        case VK_PRINT: return "PrintScreen";
        case VK_PAUSE: return "Pause";
        case VK_APPS: return "Menu";
        case VK_SNAPSHOT: return "PrintScreen";
        case VK_VOLUME_MUTE: return "VolumeMute";
        case VK_VOLUME_DOWN: return "VolumeDown";
        case VK_VOLUME_UP: return "VolumeUp";
        case VK_MEDIA_NEXT_TRACK: return "NextTrack";
        case VK_MEDIA_PREV_TRACK: return "PrevTrack";
        case VK_MEDIA_STOP: return "MediaStop";
        case VK_MEDIA_PLAY_PAUSE: return "PlayPause";
        default: {
            std::stringstream ss;
            ss << "VK_0x" << std::hex << vkCode;
            return ss.str();
        }
    }
}