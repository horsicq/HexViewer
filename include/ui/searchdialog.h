#pragma once
#include "render.h"
#include <functional>
#ifndef _WIN32
#include <string>
#endif

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
#ifdef _WIN32
  char findText[256];
  char replaceText[256];
#else
  std::string findText;
  std::string replaceText;
#endif
  bool caretVisible;
  int activeTextBox = 0;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  bool dialogResult = false;
  bool running = true;
  RenderManager* renderer = nullptr;
  PlatformWindow platformWindow = {};
#ifdef _WIN32
  void (*callback)(const char*, const char*) = nullptr;
  void* callbackUserData = nullptr;
#else
  std::function<void(const std::string&, const std::string&)> callback;
#endif
};

struct GoToDialogData {
#ifdef _WIN32
  char lineNumberText[256];
#else
  std::string lineNumberText;
#endif
  bool caretVisible;
  int activeTextBox = 0;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  bool dialogResult = false;
  bool running = true;
  RenderManager* renderer = nullptr;
  PlatformWindow platformWindow = {};
#ifdef _WIN32
  void (*callback)(int) = nullptr;
  void* callbackUserData = nullptr;
#else
  std::function<void(int)> callback;
#endif
};

namespace SearchDialogs {
  
#ifdef _WIN32
void ShowFindReplaceDialog(
    void* parentHandle,
    bool darkMode,
    void (*callback)(const char*, const char*),
    void* userData = nullptr
);

void ShowGoToDialog(
    void* parentHandle,
    bool darkMode,
    void (*callback)(int),
    void* userData = nullptr
);
#else

  void ShowFindReplaceDialog(void* parentHandle, bool darkMode,
    std::function<void(const std::string&, const std::string&)> callback);
  void ShowGoToDialog(void* parentHandle, bool darkMode,
    std::function<void(int)> callback);
#endif

#ifdef _WIN32
  void ShowInputDialog(void* parentHandle, const char* title, const char* prompt,
    const char* defaultText, bool darkMode,
    void (*callback)(const char*), void* userData);
#else
  void ShowInputDialog(void* parentHandle, const char* title, const char* prompt,
    const char* defaultText, bool darkMode,
    std::function<void(const std::string&)> callback);
#endif

}

struct InputDialogData
{
  bool caretVisible;
  uint64_t lastCaretToggleTime;

#ifdef _WIN32
  struct { HWND hwnd; } platformWindow;
  char inputText[256];
  void (*callback)(const char*);
  void* callbackUserData;
#else
  struct { Display* display; Window window; Atom wmDeleteWindow; } platformWindow;
  std::string inputText;
  std::function<void(const std::string&)> callback;
#endif

  RenderManager* renderer = nullptr;
  bool running = false;
  bool dialogResult = false;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  int activeTextBox = 0;
};

