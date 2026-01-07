#pragma once
#include "render.h"
#include "contextmenu.h"
#include "language.h"
#ifdef _WIN32
#include <windows.h>
#endif

struct AppOptions {
  bool darkMode;
  int defaultBytesPerLine;
  bool autoReload;
  bool contextMenu;
  char language[64];
  
  AppOptions()
    : darkMode(true),
    defaultBytesPerLine(16),
    autoReload(true),
    contextMenu(false) {
    language[0] = 'E';
    language[1] = 'n';
    language[2] = 'g';
    language[3] = 'l';
    language[4] = 'i';
    language[5] = 's';
    language[6] = 'h';
    language[7] = 0;
  }
  
  AppOptions(bool dark, int bpl, bool reload, bool ctx, const char* lang)
    : darkMode(dark),
    defaultBytesPerLine(bpl),
    autoReload(reload),
    contextMenu(ctx) {
    int i = 0;
    while (lang && lang[i] && i < 63) {
      language[i] = lang[i];
      i++;
    }
    language[i] = 0;
  }
  
  AppOptions(const AppOptions& other)
    : darkMode(other.darkMode),
    defaultBytesPerLine(other.defaultBytesPerLine),
    autoReload(other.autoReload),
    contextMenu(other.contextMenu) {
    for (int i = 0; i < 64; i++) {
      language[i] = other.language[i];
      if (other.language[i] == 0) break;
    }
  }
  
  AppOptions& operator=(const AppOptions& other) {
    if (this != &other) {
      darkMode = other.darkMode;
      defaultBytesPerLine = other.defaultBytesPerLine;
      autoReload = other.autoReload;
      contextMenu = other.contextMenu;
      
      for (int i = 0; i < 64; i++) {
        language[i] = other.language[i];
        if (other.language[i] == 0) break;
      }
    }
    return *this;
  }
};

inline bool& GetIsNativeFlag() {
  static bool g_isNative = false;
  return g_isNative;
}

inline bool& GetIsMsixFlag() {
  static bool g_isMsix = false;
  return g_isMsix;
}

inline bool IsMsixPackage() {
#ifdef _WIN32
  return GetIsMsixFlag();
#else
  return false;
#endif
}

void DetectNative();
void LoadOptionsFromFile(AppOptions& options);
void SaveOptionsToFile(const AppOptions& options);
bool IsPointInRect(int x, int y, const Rect& rect);

class OptionsDialog {
public:
#ifdef _WIN32
  static bool Show(HWND parent, AppOptions& options);
#else
  static bool Show(NativeWindow parent, AppOptions& options);
#endif
};

struct OptionsDialogData {
    AppOptions tempOptions;
    AppOptions* originalOptions;
    RenderManager* renderer;
    NativeWindow window;
    bool dialogResult;
    bool running;

    int mouseX;
    int mouseY;
    bool mouseDown;

    int hoveredWidget;
    int pressedWidget;

    bool dropdownOpen;
    int dropdownScrollOffset;
    int hoveredDropdownItem;
    Vector<char*> languages;
    int selectedLanguage;

#ifdef __linux__
  Display* display;
  Atom wmDeleteWindow;
#endif

  OptionsDialogData()
    : mouseX(0), mouseY(0), mouseDown(false),
    hoveredWidget(-1), pressedWidget(-1),
    renderer(nullptr),
#ifdef _WIN32
    window(nullptr),
#elif defined(__APPLE__)
    window(nullptr),
#elif defined(__linux__)
    window(0),
#endif
    dialogResult(false), running(true), originalOptions(nullptr),
    dropdownOpen(false), hoveredDropdownItem(-1), selectedLanguage(0),
    dropdownScrollOffset(0)
  {
#ifdef __linux__
    display = nullptr;
    wmDeleteWindow = 0;
#endif
    const char* langList[] = { "English", "Spanish", "French", "German", "Japanese",
      "Chinese", "Russian", "Italian", "Portuguese", "Korean", "Dutch", "Polish",
      "Turkish", "Swedish", "Arabic", "Hindi", "Czech", "Greek", "Danish",
      "Norwegian", "Finnish", "Vietnamese" };

    for (int i = 0; i < 22; i++) {
      size_t len = StrLen(langList[i]);
      char* s = (char*)PlatformAlloc(len + 1);
      StrCopy(s, langList[i]);
      languages.Add(s);
    }
  }
  
  ~OptionsDialogData() {
    for (size_t i = 0; i < languages.size(); i++) {
      if (languages[i]) {
        PlatformFree(languages[i], StrLen(languages[i]) + 1);
      }
    }
  }
};

void RenderOptionsDialog(OptionsDialogData* data, int windowWidth, int windowHeight);
