#include "SystemTray.h"
#include <iostream>
#include <string>


#define ID_RESTORE 1001
#define ID_EXIT 1002

// Статическая переменная
SystemTray *SystemTray::instance = nullptr;

SystemTray::SystemTray() : hwnd(nullptr), hIcon(nullptr), isVisible(false) {
  instance = this;
  WM_TRAYICON = RegisterWindowMessageW(L"Hoka_TrayIcon");

  // Инициализируем структуру NOTIFYICONDATAW
  ZeroMemory(&nid, sizeof(nid));
  nid.cbSize = sizeof(nid);
  // Пока не устанавливаем версию здесь
}

SystemTray::~SystemTray() {
  hide();
  if (hIcon) {
    DestroyIcon(hIcon);
  }
  if (hwnd) {
    DestroyWindow(hwnd);
  }
  instance = nullptr;
}

bool SystemTray::initialize(const std::wstring &tooltip) {
  // Регистрируем класс окна для трея
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = TrayWndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = L"HokaTrayWindow";
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

  if (!RegisterClassExW(&wc)) {
    DWORD error = GetLastError();
    if (error != ERROR_CLASS_ALREADY_EXISTS) {
      std::cerr << "Failed to register tray window class. Error: " << error
                << std::endl;
      return false;
    }
  }

  // Создаем скрытое окно для обработки сообщений трея
  hwnd = CreateWindowExW(0, L"HokaTrayWindow", L"Hoka Tray", 0, 0, 0, 0, 0,
                         nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

  if (!hwnd) {
    std::cerr << "Failed to create tray window" << std::endl;
    return false;
  }

  // Загружаем или создаем иконку
  hIcon = loadIconFromResource();
  if (!hIcon) {
    std::cerr << "Failed to load icon from resource"
                << std::endl;
  }

  // Настраиваем структуру для системного трея
  nid.hWnd = hwnd;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_TRAYICON;
  nid.hIcon = hIcon;

  // Копируем tooltip
  wcsncpy_s(nid.szTip, ARRAYSIZE(nid.szTip), tooltip.c_str(), _TRUNCATE);

  return true;
}

void SystemTray::show() {
  if (!hwnd)
    return;

  if (!isVisible) {
    // Сначала устанавливаем версию (для NOTIFYICON_VERSION_4)
    NOTIFYICONDATAW nid_version = {};
    nid_version.cbSize = sizeof(nid_version);
    nid_version.hWnd = hwnd;
    nid_version.uID = 1;
    nid_version.uVersion = NOTIFYICON_VERSION_4;
    
    if (!Shell_NotifyIconW(NIM_SETVERSION, &nid_version)) {
      std::cerr << "Failed to set notify icon version" << std::endl;
      // Пробуем использовать более старую версию
      nid.uVersion = NOTIFYICON_VERSION;
    } else {
      nid.uVersion = NOTIFYICON_VERSION_4;
    }

    // Теперь добавляем иконку
    if (Shell_NotifyIconW(NIM_ADD, &nid)) {
      isVisible = true;
      std::wcout << L"System tray icon added successfully" << std::endl;
    } else {
      DWORD error = GetLastError();
      std::cerr << "Failed to add system tray icon. Error: " << error << std::endl;
      
      // Пробуем без NOTIFYICON_VERSION_4
      nid.uVersion = 0; // Сбрасываем версию
      if (Shell_NotifyIconW(NIM_ADD, &nid)) {
        isVisible = true;
        std::wcout << L"System tray icon added with default version" << std::endl;
      }
    }
  }
}

void SystemTray::hide() {
  if (isVisible && hwnd) {
    Shell_NotifyIconW(NIM_DELETE, &nid);
    isVisible = false;
    std::wcout << L"System tray icon removed" << std::endl;
  }
}

void SystemTray::setTooltip(const std::wstring &tooltip) {
  if (!isVisible)
    return;

  wcsncpy_s(nid.szTip, ARRAYSIZE(nid.szTip), tooltip.c_str(), _TRUNCATE);
  nid.uFlags = NIF_TIP;
  Shell_NotifyIconW(NIM_MODIFY, &nid);
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
}

void SystemTray::showBalloon(const std::wstring &title,
                             const std::wstring &text, DWORD timeout) {
  if (!isVisible)
    return;

  nid.uFlags |= NIF_INFO;
  nid.dwInfoFlags = NIIF_INFO;
  nid.uTimeout = timeout;

  wcsncpy_s(nid.szInfoTitle, ARRAYSIZE(nid.szInfoTitle), title.c_str(),
            _TRUNCATE);
  wcsncpy_s(nid.szInfo, ARRAYSIZE(nid.szInfo), text.c_str(), _TRUNCATE);

  Shell_NotifyIconW(NIM_MODIFY, &nid);

  // Убираем флаг после показа
  nid.uFlags &= ~NIF_INFO;
}

LRESULT CALLBACK SystemTray::TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (!instance)
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  if (msg == instance->WM_TRAYICON) {
    if (lParam == WM_LBUTTONDBLCLK) {
      // Двойной клик - восстанавливаем
      if (instance->onDoubleClickCallback) {
        instance->onDoubleClickCallback();
      }
    }
    else if (lParam == WM_RBUTTONUP) {
      // Правый клик - показываем меню
      instance->showContextMenu();
    }
    return 0;
  }
  else if (msg == WM_COMMAND) {
    // Обрабатываем команды из меню
    std::wcout << L"WM_COMMAND received: 0x" << std::hex << LOWORD(wParam) << std::endl;
    
    switch (LOWORD(wParam)) {
    case ID_RESTORE:
      std::wcout << L"Restore command" << std::endl;
      if (instance->onRestoreCallback) {
        instance->onRestoreCallback();
      }
      break;

    case ID_EXIT:
      std::wcout << L"Exit command" << std::endl;
      if (instance->onExitCallback) {
        instance->onExitCallback();
      }
      break;
    }
    return 0;
  }
  else if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SystemTray::showContextMenu() {
  POINT pt;
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();

  // Добавляем пункты меню с правильными ID
  AppendMenuW(hMenu, MF_STRING, ID_RESTORE, L"Restore");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"Exit");

  // Делаем окно активным чтобы меню корректно закрывалось
  SetForegroundWindow(hwnd);

  // Показываем меню - важно использовать TPM_RETURNCMD
  UINT command = TrackPopupMenu(hMenu, 
                               TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN | TPM_RETURNCMD,
                               pt.x, pt.y, 0, hwnd, nullptr);

  // Если используем TPM_RETURNCMD, обрабатываем команду вручную
  if (command != 0) {
    PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(command, 0), 0);
  }

  // Отправляем сообщение чтобы меню исчезло после клика в другом месте
  PostMessageW(hwnd, WM_NULL, 0, 0);

  DestroyMenu(hMenu);
}

HICON SystemTray::loadIconFromResource() {
  // Пытаемся загрузить иконку из ресурсов
  HICON icon = (HICON)LoadImageW(GetModuleHandleW(nullptr),
                                 MAKEINTRESOURCEW(101), // ID иконки в ресурсах
                                 IMAGE_ICON, 32, 32,    // Размер для трея
                                 LR_DEFAULTCOLOR);

  if (!icon) {
    // Пытаемся загрузить стандартную иконку приложения
    icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(32512));
  }

  return icon;
}