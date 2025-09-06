#include "KeyLogger.h"
#include <iostream>
#include <psapi.h>
#include <sstream>

// Инициализация статических членов - ОБЯЗАТЕЛЬНО в .cpp файле!
std::queue<KeyPressEvent> KeyLogger::eventQueue;
std::mutex KeyLogger::queueMutex;
KeyLogger *KeyLogger::instance = nullptr;

KeyLogger::KeyLogger() : keyboardHook(nullptr) {
  instance = this; // Сохраняем instance
}

KeyLogger::~KeyLogger() {
  stop();
  instance = nullptr;
}

// Статический метод-обработчик хука
LRESULT CALLBACK KeyLogger::keyboardProc(int nCode, WPARAM wParam,
                                         LPARAM lParam) {
  if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
    KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT *)lParam;

    // Получаем ID активного процесса
    DWORD processId;
    HWND foregroundWindow = GetForegroundWindow();
    if (foregroundWindow) {
      GetWindowThreadProcessId(foregroundWindow, &processId);
      std::string appName = getProcessName(processId);

      // Получаем состояние модификаторов
      bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
      bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
      bool altPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
      bool winPressed =
          (GetAsyncKeyState(VK_LWIN) | GetAsyncKeyState(VK_RWIN)) & 0x8000;

      // Формируем строку с комбинацией клавиш
      std::string keyCombination;

      // Исключаем модификаторы из основной клавиши
      std::string mainKey = virtualKeyToString(kbdStruct->vkCode);

      // Не добавляем модификатор, если он же является основной клавишей
      if (mainKey != "Ctrl" && ctrlPressed)
        keyCombination += "Ctrl+";
      if (mainKey != "Shift" && shiftPressed)
        keyCombination += "Shift+";
      if (mainKey != "Alt" && altPressed)
        keyCombination += "Alt+";
      if (mainKey != "Win" && winPressed)
        keyCombination += "Win+";

      keyCombination += mainKey;

      // Добавляем событие в глобальную очередь
      KeyPressEvent event{appName, keyCombination};

      // Используем защищенный доступ к очереди
      {
        std::lock_guard<std::mutex> lock(queueMutex);
        eventQueue.push(event);
      }

      // Вывод для отладки
      std::cout << "Event queued: " << appName << " - " << keyCombination
                << std::endl;
    }
  }
  return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool KeyLogger::start() {
  if (keyboardHook)
    return true;

  keyboardHook =
      SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);
  isRunning = (keyboardHook != nullptr);

  if (isRunning) {
    std::cout << "Keyboard hook started successfully" << std::endl;
  } else {
    DWORD error = GetLastError();
    std::cerr << "Failed to start keyboard hook! Error code: " << error
              << std::endl;
  }

  return isRunning;
}

void KeyLogger::stop() {
  if (keyboardHook) {
    UnhookWindowsHookEx(keyboardHook);
    keyboardHook = nullptr;
    isRunning = false;
    std::cout << "Keyboard hook stopped" << std::endl;
  }
}

bool KeyLogger::hasEvents() {
  std::lock_guard<std::mutex> lock(queueMutex);
  return !eventQueue.empty();
}

KeyPressEvent KeyLogger::popEvent() {
  std::lock_guard<std::mutex> lock(queueMutex);
  if (eventQueue.empty()) {
    return KeyPressEvent{"", ""};
  }

  KeyPressEvent event = eventQueue.front();
  eventQueue.pop();
  return event;
}

bool KeyLogger::isActive() const { return isRunning; }

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
  case VK_SPACE:
    return "Space";
  case VK_RETURN:
    return "Enter";
  case VK_BACK:
    return "Backspace";
  case VK_TAB:
    return "Tab";
  case VK_ESCAPE:
    return "Esc";
  case VK_LCONTROL:
  case VK_RCONTROL:
  case VK_CONTROL:
    return "Ctrl";
  case VK_LSHIFT:
  case VK_RSHIFT:
  case VK_SHIFT:
    return "Shift";
  case VK_LMENU:
  case VK_RMENU:
  case VK_MENU:
    return "Alt";
  case VK_LWIN:
  case VK_RWIN:
    return "Win";
  case VK_UP:
    return "↑";
  case VK_DOWN:
    return "↓";
  case VK_LEFT:
    return "←";
  case VK_RIGHT:
    return "→";
  case VK_INSERT:
    return "Insert";
  case VK_DELETE:
    return "Delete";
  case VK_HOME:
    return "Home";
  case VK_END:
    return "End";
  case VK_PRIOR:
    return "PageUp";
  case VK_NEXT:
    return "PageDown";
  case VK_ADD:
    return "+";
  case VK_SUBTRACT:
    return "-";
  case VK_MULTIPLY:
    return "*";
  case VK_DIVIDE:
    return "/";
  case VK_OEM_PERIOD:
    return ".";
  case VK_OEM_COMMA:
    return ",";
  case VK_OEM_1:
    return ";";
  case VK_OEM_2:
    return "/";
  case VK_OEM_3:
    return "`";
  case VK_OEM_4:
    return "[";
  case VK_OEM_5:
    return "\\";
  case VK_OEM_6:
    return "]";
  case VK_OEM_7:
    return "'";
  case VK_CAPITAL:
    return "CapsLock";
  case VK_NUMLOCK:
    return "NumLock";
  case VK_SCROLL:
    return "ScrollLock";
  case VK_PRINT:
    return "PrintScreen";
  case VK_PAUSE:
    return "Pause";
  case VK_APPS:
    return "Menu";
  case VK_SNAPSHOT:
    return "PrintScreen";
  case VK_VOLUME_MUTE:
    return "VolumeMute";
  case VK_VOLUME_DOWN:
    return "VolumeDown";
  case VK_VOLUME_UP:
    return "VolumeUp";
  case VK_MEDIA_NEXT_TRACK:
    return "NextTrack";
  case VK_MEDIA_PREV_TRACK:
    return "PrevTrack";
  case VK_MEDIA_STOP:
    return "MediaStop";
  case VK_MEDIA_PLAY_PAUSE:
    return "PlayPause";
  default:
    // Для неизвестных клавиш возвращаем код
    std::stringstream ss;
    ss << "VK_0x" << std::hex << vkCode;
    return ss.str();
  }
}