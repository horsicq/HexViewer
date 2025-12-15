#pragma once
#include <string>  
#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "dwmapi.lib")
typedef HWND NativeWindow;
#elif __APPLE__
typedef void* NativeWindow;
#else
typedef unsigned long NativeWindow;
#endif
struct AppOptions {
  bool darkMode;
  int defaultBytesPerLine;
  bool autoReload;
  bool contextMenu;
  std::string language;

  AppOptions()
    : darkMode(false),
    defaultBytesPerLine(16),
    autoReload(true),
    contextMenu(false),
    language("English") {
  }

  AppOptions(bool dark, int bpl, bool reload, bool ctx, const std::string& lang)
    : darkMode(dark),
    defaultBytesPerLine(bpl),
    autoReload(reload),
    contextMenu(ctx),
    language(lang) {
  }
};
inline bool g_isNative = false;
inline bool g_isMsix = false;
void DetectNative();
void LoadOptionsFromFile(AppOptions& options);
void SaveOptionsToFile(const AppOptions& options);
class OptionsDialog {
public:
#ifdef _WIN32
  static bool Show(HWND parent, AppOptions& options);
#else
  static bool Show(NativeWindow parent, AppOptions& options);
#endif
};
