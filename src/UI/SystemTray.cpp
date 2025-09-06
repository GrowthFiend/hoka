#include "SystemTray.h"
#include <iostream>
#include <string>

// Статическая переменная
SystemTray *SystemTray::instance = nullptr;

SystemTray::SystemTray() : hwnd(nullptr), hIcon(nullptr), isVisible(false) {
  instance = this;
  WM_TRAYICON = RegisterWindowMessageW(L"Hoka_TrayIcon");

  // Инициализируем структуру NOTIFYICONDATAW
  ZeroMemory(&nid, sizeof(nid));
  nid.cbSize = sizeof(nid);
  nid.uVersion = NOTIFYICON_VERSION_4;
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
    hIcon = createDefaultIcon();
  }

  // Настраиваем структуру для системного трея
  nid.hWnd = hwnd;
  nid.uID = 1;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
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
    if (Shell_NotifyIconW(NIM_ADD, &nid)) {
      // Устанавливаем версию
      nid.uVersion = NOTIFYICON_VERSION_4;
      Shell_NotifyIconW(NIM_SETVERSION, &nid);

      isVisible = true;
      std::wcout << L"System tray icon added successfully" << std::endl;
    } else {
      std::cerr << "Failed to add system tray icon" << std::endl;
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

LRESULT CALLBACK SystemTray::TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                         LPARAM lParam) {
  if (!instance)
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  if (msg == instance->WM_TRAYICON) {
    switch (lParam) {
    case WM_LBUTTONDBLCLK:
      if (instance->onDoubleClickCallback) {
        instance->onDoubleClickCallback();
      }
      break;

    case WM_RBUTTONUP:
      instance->showContextMenu();
      break;
    }
    return 0;
  } else if (msg == WM_COMMAND) {
    switch (LOWORD(wParam)) {
    case ID_RESTORE:
      if (instance->onRestoreCallback) {
        instance->onRestoreCallback();
      }
      break;

    case ID_EXIT:
      if (instance->onExitCallback) {
        instance->onExitCallback();
      }
      break;
    }
    return 0;
  } else if (msg == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void SystemTray::showContextMenu() {
  POINT pt;
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();

  // Добавляем пункты меню с Unicode строками
  AppendMenuW(hMenu, MF_STRING, ID_RESTORE, L"Restore");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(hMenu, MF_STRING, ID_EXIT, L"Exit");

  // Делаем окно активным чтобы меню корректно закрывалось
  SetForegroundWindow(hwnd);

  // Показываем меню
  TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                 pt.x, pt.y, 0, hwnd, nullptr);

  // Отправляем сообщение чтобы меню исчезло после клика в другом месте
  PostMessageW(hwnd, WM_NULL, 0, 0);

  DestroyMenu(hMenu);
}

HICON SystemTray::loadIconFromResource() {
  // Пытаемся загрузить иконку из ресурсов
  HICON icon = (HICON)LoadImageW(GetModuleHandleW(nullptr),
                                 MAKEINTRESOURCEW(101), // ID иконки в ресурсах
                                 IMAGE_ICON, 16, 16,    // Размер для трея
                                 LR_DEFAULTCOLOR);

  if (!icon) {
    // Пытаемся загрузить стандартную иконку приложения
    icon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(32512));
  }

  return icon;
}

HICON SystemTray::createDefaultIcon() {
  // Создаем bitmap для иконки 16x16
  HDC hdc = GetDC(nullptr);
  HDC hdcMem = CreateCompatibleDC(hdc);
  HBITMAP hbm = CreateCompatibleBitmap(hdc, 16, 16);
  HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);

  // Рисуем простую иконку - синий квадрат с белой буквой H
  HBRUSH hBrush = CreateSolidBrush(RGB(0, 100, 200));
  RECT rect = {0, 0, 16, 16};
  FillRect(hdcMem, &rect, hBrush);

  SetTextColor(hdcMem, RGB(255, 255, 255));
  SetBkMode(hdcMem, TRANSPARENT);

  HFONT hFont =
      CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                  DEFAULT_PITCH | FF_SWISS, L"Arial");

  HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
  TextOutW(hdcMem, 3, 1, L"H", 1);

  // Восстанавливаем контекст
  SelectObject(hdcMem, hOldFont);
  SelectObject(hdcMem, hbmOld);

  // Очищаем ресурсы
  DeleteObject(hFont);
  DeleteObject(hBrush);
  DeleteDC(hdcMem);
  ReleaseDC(nullptr, hdc);

  // Создаем маску (полностью непрозрачную)
  HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, nullptr);

  // Создаем иконку
  ICONINFO ii = {};
  ii.fIcon = TRUE;
  ii.hbmColor = hbm;
  ii.hbmMask = hbmMask;

  HICON hIcon = CreateIconIndirect(&ii);

  // Очищаем
  DeleteObject(hbm);
  DeleteObject(hbmMask);

  return hIcon;
}