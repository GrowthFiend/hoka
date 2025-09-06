#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista или выше
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <functional>
#include <windows.h>
#include <shellapi.h>
#include <string>


class SystemTray {
public:
  SystemTray();
  ~SystemTray();

  bool initialize(const std::wstring &tooltip = L"Hoka Application");
  void show();
  void hide();
  void setTooltip(const std::wstring &tooltip);
  void showBalloon(const std::wstring &title, const std::wstring &text,
                   DWORD timeout = 3000);

  // Callbacks
  std::function<void()> onDoubleClickCallback;
  std::function<void()> onRestoreCallback;
  std::function<void()> onExitCallback;

private:
  HWND hwnd;
  HICON hIcon;
  NOTIFYICONDATAW nid;
  UINT WM_TRAYICON;
  bool isVisible;

  static SystemTray *instance;

  // Window procedure and helper methods
  static LRESULT CALLBACK TrayWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam);
  void showContextMenu();
  HICON loadIconFromResource();
  HICON createDefaultIcon();

  // Context menu commands
  static const UINT ID_RESTORE = 1001;
  static const UINT ID_EXIT = 1002;

  // Prevent copying
  SystemTray(const SystemTray &) = delete;
  SystemTray &operator=(const SystemTray &) = delete;
};

#endif // SYSTEMTRAY_H