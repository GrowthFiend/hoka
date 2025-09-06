#pragma once
#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <windows.h>

struct KeyPressEvent {
  std::string appName;
  std::string keyCombination;
};

class KeyLogger {
private:
  HHOOK keyboardHook;
  std::atomic<bool> isRunning{false};

  // Статические члены для глобального доступа - только объявления!
  static std::queue<KeyPressEvent> eventQueue;
  static std::mutex queueMutex;
  static KeyLogger *instance;

  static LRESULT CALLBACK keyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

public:
  KeyLogger();
  ~KeyLogger();

  bool start();
  void stop();
  bool hasEvents();
  KeyPressEvent popEvent();
  bool isActive() const;

  // Статические utility-методы
  static std::string getProcessName(DWORD processId);
  static std::string virtualKeyToString(UINT vkCode);
};