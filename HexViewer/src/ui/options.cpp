#include "options.h"
#include "render.h"
#include "contextmenu.h"

#ifdef _WIN32
#include <windows.h>
#include <filesystem>
#include <appmodel.h>
#include <dwmapi.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#include <filesystem>
#else
#include <unistd.h>
#include <linux/limits.h>
#include <filesystem>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif
#include <cwctype>
#include <algorithm>
#include <fstream>
#include <language.h>

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
  std::vector<std::string> languages;
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
    languages = { "English", "Spanish", "French", "German", "Japanese", "Chinese", "Russian" }; // TODO: add more lang!!!
  }
};

void DetectNative() {
#ifdef BUILD_NATIVE
  g_isNative = true;
#else
#ifdef _WIN32
  UINT32 length = 0;
  LONG result = GetPackageFullName(nullptr, &length, nullptr);
  if (result == ERROR_SUCCESS || result == ERROR_INSUFFICIENT_BUFFER) {
    g_isNative = true;
    g_isMsix = true;
    return;
  }

  wchar_t exePath[MAX_PATH];
  if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
    std::filesystem::path path(exePath);
    std::wstring parentLower = path.parent_path().wstring();
    std::transform(parentLower.begin(), parentLower.end(), parentLower.begin(),
      [](wchar_t c) { return std::towlower(c); });

    if (parentLower.find(L"program files") != std::wstring::npos) {
      g_isNative = true;
    }
    else {
      g_isNative = false;
    }
  }
  else {
    g_isNative = false;
  }
#elif __APPLE__
  char path[PATH_MAX];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0) {
    std::filesystem::path exePath(path);
    if (exePath.parent_path().string().find("/Applications") != std::string::npos) {
      g_isNative = true;
    }
    else {
      g_isNative = false;
    }
  }
  else {
    g_isNative = false;
  }

#else // Linux / other Unix
  char buf[PATH_MAX];
  ssize_t count = readlink("/proc/self/exe", buf, sizeof(buf));
  if (count != -1) {
    std::filesystem::path exePath(std::string(buf, count));
    std::string parent = exePath.parent_path().string();
    if (parent.find("/usr/") == 0) {
      g_isNative = true;
    }
    else {
      g_isNative = false;
    }
  }
  else {
    g_isNative = false;
  }
#endif
#endif
}

std::filesystem::path GetConfigPath() {
#ifdef _WIN32
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);
  std::filesystem::path exeDir(exePath);
  exeDir = exeDir.parent_path();

  std::wstring exeDirLower = exeDir.wstring();
  std::transform(exeDirLower.begin(), exeDirLower.end(), exeDirLower.begin(),
    [](wchar_t c) { return std::towlower(c); });
  if (exeDirLower.find(L"program files") == std::wstring::npos && !g_isNative) {
    return exeDir / "hexviewer_config.ini";
  }

  wchar_t localAppData[MAX_PATH];
  DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
  if (len > 0 && len < MAX_PATH) {
    return std::filesystem::path(localAppData) / "HexViewer" / "hexviewer_config.ini";
  }

  return exeDir / "hexviewer_config.ini";
#else
  const char* home = getenv("HOME");
  if (home) {
    return std::filesystem::path(home) / ".config" / "HexViewer" / "hexviewer_config.ini";
  }
  return "hexviewer_config.ini"; // fallback
#endif
}

void SaveOptionsToFile(const AppOptions& options) {
  std::filesystem::path configPath = GetConfigPath();

  std::error_code ec;
  std::filesystem::create_directories(configPath.parent_path(), ec);

  std::ofstream file(configPath);
  if (file.is_open()) {
    file << "darkMode=" << (options.darkMode ? "1" : "0") << "\n";
    file << "bytesPerLine=" << options.defaultBytesPerLine << "\n";
    file << "autoReload=" << (options.autoReload ? "1" : "0") << "\n";
    file << "language=" << options.language << "\n";
    file.close();
  }
}

void LoadOptionsFromFile(AppOptions& options) {
  std::filesystem::path configPath = GetConfigPath();

  std::ifstream file(configPath);
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      size_t pos = line.find('=');
      if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "darkMode") options.darkMode = (value == "1");
        else if (key == "bytesPerLine") options.defaultBytesPerLine = std::stoi(value);
        else if (key == "autoReload") options.autoReload = (value == "1");
        else if (key == "language") options.language = value;
      }
    }
    file.close();
  }
}

static OptionsDialogData* g_dialogData = nullptr;

bool IsPointInRect(int x, int y, const Rect& rect) {
  return x >= rect.x && x <= rect.x + rect.width &&
    y >= rect.y && y <= rect.y + rect.height;
}

void RenderOptionsDialog(OptionsDialogData* data, int windowWidth, int windowHeight) {
  if (!data || !data->renderer) return;

  data->renderer->beginFrame();

  Theme theme = data->tempOptions.darkMode ? Theme::Dark() : Theme::Light();
  data->renderer->clear(theme.windowBackground);

  int margin = 20;
  int y = margin;
  int controlHeight = 25;
  int spacing = 10;

  data->renderer->drawText(Translations::T("Options").c_str(), margin, y, theme.headerColor);
  y += 35;

  Rect checkboxRect1(margin, y, 18, 18);
  WidgetState checkboxState1(checkboxRect1);
  checkboxState1.hovered = (data->hoveredWidget == 0);
  checkboxState1.pressed = (data->pressedWidget == 0);

  data->renderer->drawModernCheckbox(checkboxState1, theme, data->tempOptions.darkMode);
  data->renderer->drawText(Translations::T("Dark Mode").c_str(), margin + 28, y + 2, theme.textColor);
  y += controlHeight + spacing;

  Rect checkboxRect2(margin, y, 18, 18);
  WidgetState checkboxState2(checkboxRect2);
  checkboxState2.hovered = (data->hoveredWidget == 1);
  checkboxState2.pressed = (data->pressedWidget == 1);

  data->renderer->drawModernCheckbox(checkboxState2, theme, data->tempOptions.autoReload);
  data->renderer->drawText(Translations::T("Auto-reload modified file").c_str(), margin + 28, y + 2, theme.textColor);
  y += controlHeight + spacing;

  if (!g_isNative) {
    Rect checkboxRect3(margin, y, 18, 18);
    WidgetState checkboxState3(checkboxRect3);
    checkboxState3.hovered = (data->hoveredWidget == 2);
    checkboxState3.pressed = (data->pressedWidget == 2);

    data->renderer->drawModernCheckbox(checkboxState3, theme, data->tempOptions.contextMenu);
    data->renderer->drawText(Translations::T("Add to context menu (right-click files)").c_str(), margin + 28, y + 2, theme.textColor);
    y += controlHeight + spacing * 2;
  }

  data->renderer->drawText(Translations::T("Language:").c_str(), margin, y, theme.textColor);
  y += controlHeight;

  Rect dropdownRect(margin + 20, y, 200, controlHeight);
  WidgetState dropdownState(dropdownRect);
  dropdownState.hovered = (data->hoveredWidget == 7);
  dropdownState.pressed = (data->pressedWidget == 7);

  data->renderer->drawDropdown(
    dropdownState,
    theme,
    data->languages[data->selectedLanguage],
    data->dropdownOpen,
    data->languages,
    data->selectedLanguage,
    data->hoveredDropdownItem,
    data->dropdownScrollOffset
  );

  y += controlHeight + spacing * 2;

  y += spacing * 8; // Much more space!

  data->renderer->drawText(Translations::T("Default bytes per line:").c_str(), margin, y, theme.textColor);
  y += controlHeight;

  Rect radioRect1(margin + 20, y, 16, 16);
  WidgetState radioState1(radioRect1);
  radioState1.hovered = (data->hoveredWidget == 3);
  radioState1.pressed = (data->pressedWidget == 3);

  data->renderer->drawModernRadioButton(radioState1, theme, data->tempOptions.defaultBytesPerLine == 8);
  data->renderer->drawText(Translations::T("8 bytes").c_str(), margin + 45, y + 1, theme.textColor);
  y += controlHeight;

  Rect radioRect2(margin + 20, y, 16, 16);
  WidgetState radioState2(radioRect2);
  radioState2.hovered = (data->hoveredWidget == 4);
  radioState2.pressed = (data->pressedWidget == 4);

  data->renderer->drawModernRadioButton(radioState2, theme, data->tempOptions.defaultBytesPerLine == 16);
  data->renderer->drawText(Translations::T("16 bytes").c_str(), margin + 45, y + 1, theme.textColor);
  y += controlHeight + spacing * 2;

  int buttonWidth = 75;
  int buttonHeight = 25;
  int buttonY = windowHeight - margin - buttonHeight - 5;
  int buttonSpacing = 8;

  Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
  WidgetState okButtonState(okButtonRect);
  okButtonState.hovered = (data->hoveredWidget == 5);
  okButtonState.pressed = (data->pressedWidget == 5);

  data->renderer->drawModernButton(okButtonState, theme, Translations::T("OK").c_str());

  Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  WidgetState cancelButtonState(cancelButtonRect);
  cancelButtonState.hovered = (data->hoveredWidget == 6);
  cancelButtonState.pressed = (data->pressedWidget == 6);

  data->renderer->drawModernButton(cancelButtonState, theme, Translations::T("Cancel").c_str());

  data->renderer->endFrame();
}

void UpdateHoverState(OptionsDialogData* data, int x, int y, int windowWidth, int windowHeight) {
  int margin = 20;
  int startY = margin + 35;
  int controlHeight = 25;
  int spacing = 10;

  data->hoveredWidget = -1;
  data->hoveredDropdownItem = -1;

  Rect checkboxRect1(margin, startY, 18, 18);
  if (IsPointInRect(x, y, checkboxRect1)) {
    data->hoveredWidget = 0;
    return;
  }
  startY += controlHeight + spacing;

  Rect checkboxRect2(margin, startY, 18, 18);
  if (IsPointInRect(x, y, checkboxRect2)) {
    data->hoveredWidget = 1;
    return;
  }
  startY += controlHeight + spacing;

  if (!g_isNative) {
    Rect checkboxRect3(margin, startY, 18, 18);
    if (IsPointInRect(x, y, checkboxRect3)) {
      data->hoveredWidget = 2;
      return;
    }
    startY += controlHeight + spacing * 2;
  }

  startY += controlHeight;

  Rect dropdownRect(margin + 20, startY, 200, controlHeight);
  if (IsPointInRect(x, y, dropdownRect)) {
    data->hoveredWidget = 7;
    if (!data->dropdownOpen) {
      return;
    }
  }

  if (data->dropdownOpen) {
    int itemHeight = 28;
    int maxVisibleItems = 3;
    int dropdownItemY = startY + controlHeight + 2;

    for (size_t i = 0; i < data->languages.size(); i++) {
      int visualIndex = (int)i - data->dropdownScrollOffset;

      if (visualIndex < 0 || visualIndex >= maxVisibleItems) {
        continue;
      }

      Rect itemRect(margin + 20, dropdownItemY + (visualIndex * itemHeight), 200, itemHeight);
      if (IsPointInRect(x, y, itemRect)) {
        data->hoveredDropdownItem = i;
        return;
      }
    }
  }

  startY += controlHeight + spacing * 2 + controlHeight;

  Rect radioRect1(margin + 20, startY, 16, 16);
  if (IsPointInRect(x, y, radioRect1)) {
    data->hoveredWidget = 3;
    return;
  }
  startY += controlHeight;

  Rect radioRect2(margin + 20, startY, 16, 16);
  if (IsPointInRect(x, y, radioRect2)) {
    data->hoveredWidget = 4;
    return;
  }

  int buttonWidth = 75;
  int buttonHeight = 25;
  int buttonY = windowHeight - margin - buttonHeight - 5;
  int buttonSpacing = 8;

  Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
  if (IsPointInRect(x, y, okButtonRect)) {
    data->hoveredWidget = 5;
    return;
  }

  Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  if (IsPointInRect(x, y, cancelButtonRect)) {
    data->hoveredWidget = 6;
    return;
  }
}

void HandleMouseClick(OptionsDialogData* data, int x, int y, int windowWidth, int windowHeight) {
  if (data->dropdownOpen && data->hoveredDropdownItem >= 0) {
    data->selectedLanguage = data->hoveredDropdownItem;
    data->tempOptions.language = data->languages[data->selectedLanguage];
    Translations::SetLanguage(data->tempOptions.language);
    data->dropdownOpen = false;
    return;
  }

  if (data->hoveredWidget == -1) return;

  switch (data->hoveredWidget) {
  case 0: // Dark Mode checkbox
    data->tempOptions.darkMode = !data->tempOptions.darkMode;
    break;

  case 1: // Auto Reload checkbox
    data->tempOptions.autoReload = !data->tempOptions.autoReload;
    break;

  case 2: // Context Menu checkbox
    if (!g_isNative) {
      data->tempOptions.contextMenu = !data->tempOptions.contextMenu;
    }
    break;

  case 3: // 8 bytes radio
    data->tempOptions.defaultBytesPerLine = 8;
    break;

  case 4: // 16 bytes radio
    data->tempOptions.defaultBytesPerLine = 16;
    break;

  case 5: { // OK button
#ifdef _WIN32
    bool wasRegistered = ContextMenuRegistry::IsRegistered(UserRole::CurrentUser);
    bool shouldBeRegistered = data->tempOptions.contextMenu;

    if (shouldBeRegistered && !wasRegistered) {
      wchar_t exePath[MAX_PATH];
      GetModuleFileNameW(nullptr, exePath, MAX_PATH);

      for (int i = 0; exePath[i] != L'\0'; i++) {
        if (exePath[i] == L'/') {
          exePath[i] = L'\\';
        }
      }

      if (!ContextMenuRegistry::Register(exePath, UserRole::CurrentUser)) {
        MessageBoxW(nullptr,
          L"Failed to register context menu.",
          L"Error", MB_OK | MB_ICONERROR);
        data->tempOptions.contextMenu = false;
      }
    }
    else if (!shouldBeRegistered && wasRegistered) {
      if (!ContextMenuRegistry::Unregister(UserRole::CurrentUser)) {
        MessageBoxW(nullptr,
          L"Failed to unregister context menu.",
          L"Error", MB_OK | MB_ICONERROR);
        data->tempOptions.contextMenu = true;
      }
    }
#endif
    data->tempOptions.language = data->languages[data->selectedLanguage];
    *data->originalOptions = data->tempOptions;
    data->dialogResult = true;
    data->running = false;
    break;
  }

  case 6: // Cancel button
    data->dialogResult = false;
    data->running = false;
    break;

  case 7: // Dropdown
    data->dropdownOpen = !data->dropdownOpen;
    if (!data->dropdownOpen) {
      data->dropdownScrollOffset = 0;
    }
    break;
  }
}

#ifdef _WIN32

LRESULT CALLBACK OptionsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  OptionsDialogData* data = g_dialogData;

  switch (msg) {
  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    if (data && data->renderer) {
      RECT rect;
      GetClientRect(hwnd, &rect);
      RenderOptionsDialog(data, rect.right, rect.bottom);
    }
    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_MOUSEMOVE: {
    if (data) {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      data->mouseX = x;
      data->mouseY = y;
      UpdateHoverState(data, x, y, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }
  case WM_MOUSEWHEEL: {
    if (data && data->dropdownOpen) {
      RECT rect;
      GetClientRect(hwnd, &rect);

      int margin = 20;
      int startY = margin + 35 + 25 + 10 + 25 + 10; // Calculate dropdown Y position
      if (!g_isNative) {
        startY += 25 + 10 * 2;
      }
      startY += 25; // Skip "Language:" label

      Rect dropdownRect(margin + 20, startY, 200, 25);

      if (data->mouseX >= dropdownRect.x &&
        data->mouseX <= dropdownRect.x + dropdownRect.width &&
        data->mouseY >= dropdownRect.y) {

        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int maxVisibleItems = 3;
        int maxScroll = std::clamp((int)data->languages.size() - maxVisibleItems, 0, INT_MAX);

        if (delta > 0) {
          data->dropdownScrollOffset = std::clamp(data->dropdownScrollOffset - 1, 0, maxScroll);
        }
        else {
          data->dropdownScrollOffset = std::clamp(data->dropdownScrollOffset + 1, 0, maxScroll);
        }

        UpdateHoverState(data, data->mouseX, data->mouseY, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
      }
    }
    break;
  }

  case WM_LBUTTONDOWN: {
    if (data) {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      data->mouseDown = true;
      data->pressedWidget = data->hoveredWidget;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONUP: {
    if (data) {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      data->mouseDown = false;

      if (data->pressedWidget == data->hoveredWidget || data->hoveredDropdownItem >= 0) {
        HandleMouseClick(data, x, y, rect.right, rect.bottom);
      }

      data->pressedWidget = -1;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_CLOSE:
    if (data) {
      data->dialogResult = false;
      data->running = false;
    }
    return 0;

  case WM_SIZE: {
    if (data && data->renderer) {
      RECT rect;
      GetClientRect(hwnd, &rect);
      if (rect.right > 0 && rect.bottom > 0 && rect.right < 8192 && rect.bottom < 8192) {
        data->renderer->resize(rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
    }
    return 0;
  }
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool OptionsDialog::Show(HWND parent, AppOptions& options)
{
  const wchar_t* className = L"CustomOptionsWindow";

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = OptionsWindowProc;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

  UnregisterClassW(className, wc.hInstance);
  if (!RegisterClassExW(&wc)) {
    return false;
  }

  OptionsDialogData data = {};
  data.tempOptions = options;

  if (!g_isNative) {
    data.tempOptions.contextMenu = ContextMenuRegistry::IsRegistered(UserRole::CurrentUser);
  }
  else {
    data.tempOptions.contextMenu = false;
  }

  data.originalOptions = &options;
  data.dialogResult = false;
  data.running = true;
  g_dialogData = &data;

  for (size_t i = 0; i < data.languages.size(); i++) {
    if (data.languages[i] == options.language) {
      data.selectedLanguage = i;
      break;
    }
  }
  int width = 400;
  int height = 480; // Increased height for dropdown with full list

  RECT parentRect;
  GetWindowRect(parent, &parentRect);

  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

  HWND hwnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    className,
    L"Options",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    x, y, width, height,
    parent,
    nullptr,
    GetModuleHandleW(NULL),
    nullptr
  );

  if (!hwnd) {
    g_dialogData = nullptr;
    return false;
  }

  data.window = hwnd;

  if (data.tempOptions.darkMode) {
    BOOL dark = TRUE;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  }

  data.renderer = new RenderManager();
  if (!data.renderer) {
    DestroyWindow(hwnd);
    g_dialogData = nullptr;
    return false;
  }

  if (!data.renderer->initialize(hwnd)) {
    delete data.renderer;
    DestroyWindow(hwnd);
    g_dialogData = nullptr;
    return false;
  }

  RECT clientRect;
  GetClientRect(hwnd, &clientRect);

  if (clientRect.right > 0 && clientRect.bottom > 0) {
    data.renderer->resize(clientRect.right, clientRect.bottom);
  }

  EnableWindow(parent, FALSE);

  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  MSG msg;
  while (data.running && GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (data.renderer) {
    delete data.renderer;
    data.renderer = nullptr;
  }

  EnableWindow(parent, TRUE);
  SetForegroundWindow(parent);
  DestroyWindow(hwnd);
  UnregisterClassW(className, GetModuleHandleW(NULL));

  g_dialogData = nullptr;

  return data.dialogResult;
}

#elif defined(__linux__)

void ProcessOptionsEvent(OptionsDialogData* data, XEvent* event, int width, int height) {
  switch (event->type) {
  case Expose:
    RenderOptionsDialog(data, width, height);
    break;

  case MotionNotify:
    data->mouseX = event->xmotion.x;
    data->mouseY = event->xmotion.y;
    UpdateHoverState(data, event->xmotion.x, event->xmotion.y, width, height);
    RenderOptionsDialog(data, width, height);
    break;

  case ButtonPress:
    if (event->xbutton.button == Button1) {
      data->mouseDown = true;
      data->pressedWidget = data->hoveredWidget;
      RenderOptionsDialog(data, width, height);
    }
    else if (event->xbutton.button == Button4) { // Scroll up
      if (data->dropdownOpen) {
        int maxVisibleItems = 3;
        int maxScroll = std::clamp((int)data->languages.size() - maxVisibleItems, 0, INT_MAX);
        data->dropdownScrollOffset = std::clamp(data->dropdownScrollOffset - 1, 0, maxScroll);
        UpdateHoverState(data, data->mouseX, data->mouseY, width, height);
        RenderOptionsDialog(data, width, height);
      }
    }
    else if (event->xbutton.button == Button5) { // Scroll down
      if (data->dropdownOpen) {
        int maxVisibleItems = 3;
        int maxScroll = std::clamp((int)data->languages.size() - maxVisibleItems, 0, INT_MAX);
        data->dropdownScrollOffset = std::clamp(data->dropdownScrollOffset + 1, 0, maxScroll);
        UpdateHoverState(data, data->mouseX, data->mouseY, width, height);
        RenderOptionsDialog(data, width, height);
      }
    }
    break;

  case ButtonRelease:
    if (event->xbutton.button == Button1) {
      data->mouseDown = false;

      if (data->pressedWidget == data->hoveredWidget || data->hoveredDropdownItem >= 0) {
        HandleMouseClick(data, event->xbutton.x, event->xbutton.y, width, height);
      }

      data->pressedWidget = -1;
      RenderOptionsDialog(data, width, height);
    }
    break;

  case ClientMessage:
    if ((Atom)event->xclient.data.l[0] == data->wmDeleteWindow) {
      data->dialogResult = false;
      data->running = false;
    }
    break;

  case ConfigureNotify:
    if (event->xconfigure.width != width || event->xconfigure.height != height) {
      if (data->renderer) {
        data->renderer->resize(event->xconfigure.width, event->xconfigure.height);
        RenderOptionsDialog(data, event->xconfigure.width, event->xconfigure.height);
      }
    }
    break;
  }
}

bool OptionsDialog::Show(NativeWindow parent, AppOptions& options) {
  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    return false;
  }

  int screen = DefaultScreen(display);
  Window rootWindow = RootWindow(display, screen);

  OptionsDialogData data = {};
  data.tempOptions = options;

  if (!g_isNative) {
    data.tempOptions.contextMenu = false;
  }
  else {
    data.tempOptions.contextMenu = false;
  }

  data.originalOptions = &options;
  data.dialogResult = false;
  data.running = true;
  data.display = display;
  g_dialogData = &data;

  for (size_t i = 0; i < data.languages.size(); i++) {
    if (data.languages[i] == options.language) {
      data.selectedLanguage = i;
      break;
    }
  }

  int width = 400;
  int height = 480;

  Window window = XCreateSimpleWindow(
    display, rootWindow,
    100, 100, width, height, 1,
    BlackPixel(display, screen),
    WhitePixel(display, screen)
  );

  data.window = window;

  XStoreName(display, window, "Options");

  Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom wmWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(display, window, wmWindowType, XA_ATOM, 32, PropModeReplace,
    (unsigned char*)&wmWindowTypeDialog, 1);

  Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(display, window, &wmDelete, 1);
  data.wmDeleteWindow = wmDelete;

  XSelectInput(display, window,
    ExposureMask | ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask | StructureNotifyMask);

  XMapWindow(display, window);
  XFlush(display);

  data.renderer = new RenderManager();
  if (!data.renderer || !data.renderer->initialize(window)) {
    if (data.renderer) delete data.renderer;
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    g_dialogData = nullptr;
    return false;
  }

  data.renderer->resize(width, height);

  XEvent event;
  while (data.running) {
    XNextEvent(display, &event);
    ProcessOptionsEvent(&data, &event, width, height);
  }

  if (data.renderer) {
    delete data.renderer;
  }

  XDestroyWindow(display, window);
  XCloseDisplay(display);
  g_dialogData = nullptr;

  return data.dialogResult;
}

#elif defined(__APPLE__)

bool OptionsDialog::Show(NativeWindow parent, AppOptions& options) {
  (void)parent;
  (void)options;
  return false;
}

#endif
