#pragma once
#include "render.h"
#include <functional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#elif defined(__APPLE__)
#include <objc/objc.h>
typedef void* id;
#elif defined(__linux__)
#include <X11/Xlib.h>
#endif

struct PlatformWindow {
#ifdef _WIN32
  HWND hwnd;
#elif defined(__APPLE__)
  id window;
  id view;
#elif defined(__linux__)
  Display* display;
  Window window;
  Atom wmDeleteWindow;
#endif
};

struct FindReplaceDialogData {
  std::string findText;
  std::string replaceText;
  int activeTextBox = 0;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  bool dialogResult = false;
  bool running = true;
  RenderManager* renderer = nullptr;
  PlatformWindow platformWindow = {};
  std::function<void(const std::string&, const std::string&)> callback;
};

struct GoToDialogData {
  std::string lineNumberText;
  int activeTextBox = 0;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  bool dialogResult = false;
  bool running = true;
  RenderManager* renderer = nullptr;
  PlatformWindow platformWindow = {};
  std::function<void(int)> callback;
};

namespace SearchDialogs {

  void CleanupDialogs();

  void ShowFindReplaceDialog(void* parentHandle, bool darkMode,
    std::function<void(const std::string&, const std::string&)> callback);

  void ShowGoToDialog(void* parentHandle, bool darkMode,
    std::function<void(int)> callback);

} // namespace SearchDialogs
