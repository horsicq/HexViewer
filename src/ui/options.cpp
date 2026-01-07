#include "options.h"

#ifdef _WIN32
#include <windows.h>
#include <appmodel.h>
#include <dwmapi.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#include <linux/limits.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

void DetectNative()
{
#ifdef BUILD_NATIVE
  GetIsNativeFlag() = true;
#else
#ifdef _WIN32
  UINT32 length = 0;
  LONG result = GetPackageFullName(nullptr, &length, nullptr);
  if (result == ERROR_SUCCESS || result == ERROR_INSUFFICIENT_BUFFER)
  {
    GetIsNativeFlag() = true;
    GetIsMsixFlag() = true;
    return;
  }

  wchar_t exePath[MAX_PATH];
  if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
  {
    for (int i = 0; exePath[i]; i++)
    {
      if (exePath[i] >= L'A' && exePath[i] <= L'Z')
      {
        exePath[i] = exePath[i] + (L'a' - L'A');
      }
    }

    bool inProgramFiles = false;
    for (int i = 0; exePath[i]; i++)
    {
      if (exePath[i] == L'p' && exePath[i + 1] == L'r' && exePath[i + 2] == L'o' &&
          exePath[i + 3] == L'g' && exePath[i + 4] == L'r' && exePath[i + 5] == L'a' &&
          exePath[i + 6] == L'm' && exePath[i + 7] == L' ' && exePath[i + 8] == L'f')
      {
        inProgramFiles = true;
        break;
      }
    }

    GetIsNativeFlag() = inProgramFiles;
  }
  else
  {
    GetIsNativeFlag() = false;
  }
#elif __APPLE__
  char path[4096];
  uint32_t size = sizeof(path);
  if (_NSGetExecutablePath(path, &size) == 0)
  {
    bool inApps = false;
    for (int i = 0; path[i]; i++)
    {
      if (path[i] == '/' && path[i + 1] == 'A' && path[i + 2] == 'p' &&
          path[i + 3] == 'p' && path[i + 4] == 'l')
      {
        inApps = true;
        break;
      }
    }
    GetIsNativeFlag() = inApps;
  }
  else
  {
    GetIsNativeFlag() = false;
  }
#else
  char buf[4096];
  ssize_t count = readlink("/proc/self/exe", buf, sizeof(buf));
  if (count != -1)
  {
    buf[count] = 0;
    GetIsNativeFlag() = (buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r');
  }
  else
  {
    GetIsNativeFlag() = false;
  }
#endif
#endif
}

void GetConfigPath(char *outPath, int maxLen)
{
#ifdef _WIN32
  wchar_t exePath[MAX_PATH];
  GetModuleFileNameW(nullptr, exePath, MAX_PATH);

  int lastSlash = -1;
  for (int i = 0; exePath[i]; i++)
  {
    if (exePath[i] == L'\\' || exePath[i] == L'/')
      lastSlash = i;
  }
  exePath[lastSlash] = 0;

  wchar_t exeDirLower[MAX_PATH];
  for (int i = 0; i <= lastSlash; i++)
  {
    exeDirLower[i] = exePath[i];
    if (exeDirLower[i] >= L'A' && exeDirLower[i] <= L'Z')
    {
      exeDirLower[i] = exeDirLower[i] + (L'a' - L'A');
    }
  }
  exeDirLower[lastSlash] = 0;

  bool inProgramFiles = false;
  for (int i = 0; exeDirLower[i]; i++)
  {
    if (exeDirLower[i] == L'p' && exeDirLower[i + 1] == L'r' && exeDirLower[i + 2] == L'o' &&
        exeDirLower[i + 3] == L'g' && exeDirLower[i + 4] == L'r' && exeDirLower[i + 5] == L'a' &&
        exeDirLower[i + 6] == L'm' && exeDirLower[i + 7] == L' ' && exeDirLower[i + 8] == L'f')
    {
      inProgramFiles = true;
      break;
    }
  }

  if (!inProgramFiles && !GetIsNativeFlag())
  {
    WideCharToMultiByte(CP_UTF8, 0, exePath, -1, outPath, maxLen - 30, nullptr, nullptr);
    int len = (int)StrLen(outPath);
    StrCopy(outPath + len, "\\hexviewer_config.ini");
    return;
  }

  wchar_t localAppData[MAX_PATH];
  DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
  if (len > 0 && len < MAX_PATH)
  {
    WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, outPath, maxLen - 50, nullptr, nullptr);
    int slen = (int)StrLen(outPath);
    StrCopy(outPath + slen, "\\HexViewer\\hexviewer_config.ini");
  }
  else
  {
    WideCharToMultiByte(CP_UTF8, 0, exePath, -1, outPath, maxLen - 30, nullptr, nullptr);
    int slen = (int)StrLen(outPath);
    StrCopy(outPath + slen, "\\hexviewer_config.ini");
  }
#else
  const char *home = getenv("HOME");
  if (home)
  {
    StrCopy(outPath, home);
    int len = (int)StrLen(outPath);
    StrCopy(outPath + len, "/.config/HexViewer/hexviewer_config.ini");
  }
  else
  {
    StrCopy(outPath, "hexviewer_config.ini");
  }
#endif
}

#ifdef _WIN32
void CreateDirectoriesForPath(const wchar_t *path)
{
  wchar_t temp[MAX_PATH];
  int len = 0;
  while (path[len])
  {
    temp[len] = path[len];
    len++;
  }
  temp[len] = 0;

  for (int i = len - 1; i >= 0; i--)
  {
    if (temp[i] == L'\\' || temp[i] == L'/')
    {
      temp[i] = 0;
      CreateDirectoryW(temp, nullptr);
      break;
    }
  }
}
#endif

void SaveOptionsToFile(const AppOptions &options)
{
  char configPath[512];
  GetConfigPath(configPath, 512);

#ifdef _WIN32
  wchar_t wpath[512];
  MultiByteToWideChar(CP_UTF8, 0, configPath, -1, wpath, 512);
  CreateDirectoriesForPath(wpath);

  HANDLE hFile = CreateFileW(wpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  char buf[256];
  DWORD written;

  StrCopy(buf, "darkMode=");
  StrCopy(buf + 9, options.darkMode ? "1" : "0");
  StrCopy(buf + 10, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "bytesPerLine=");
  IntToStr(options.defaultBytesPerLine, buf + 13, 243);
  int len = (int)StrLen(buf);
  StrCopy(buf + len, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "autoReload=");
  StrCopy(buf + 11, options.autoReload ? "1" : "0");
  StrCopy(buf + 12, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "language=");
  StrCopy(buf + 9, options.language);
  len = (int)StrLen(buf);
  StrCopy(buf + len, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  CloseHandle(hFile);
#else
  char dirPath[512];
  StrCopy(dirPath, configPath);
  for (int i = (int)StrLen(dirPath) - 1; i >= 0; i--)
  {
    if (dirPath[i] == '/')
    {
      dirPath[i] = 0;
      mkdir(dirPath, 0755);
      break;
    }
  }

  int fd = open(configPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    return;

  char buf[256];

  StrCopy(buf, "darkMode=");
  StrCopy(buf + 9, options.darkMode ? "1\n" : "0\n");
  write(fd, buf, StrLen(buf));

  StrCopy(buf, "bytesPerLine=");
  IntToStr(options.defaultBytesPerLine, buf + 13, 243);
  int len = (int)StrLen(buf);
  StrCopy(buf + len, "\n");
  write(fd, buf, StrLen(buf));

  StrCopy(buf, "autoReload=");
  StrCopy(buf + 11, options.autoReload ? "1\n" : "0\n");
  write(fd, buf, StrLen(buf));

  StrCopy(buf, "language=");
  StrCopy(buf + 9, options.language);
  len = (int)StrLen(buf);
  StrCopy(buf + len, "\n");
  write(fd, buf, StrLen(buf));

  close(fd);
#endif
}

void LoadOptionsFromFile(AppOptions &options)
{
  char configPath[512];
  GetConfigPath(configPath, 512);

#ifdef _WIN32
  wchar_t wpath[512];
  MultiByteToWideChar(CP_UTF8, 0, configPath, -1, wpath, 512);

  HANDLE hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  char buf[1024];
  DWORD read;
  ReadFile(hFile, buf, 1023, &read, nullptr);
  buf[read] = 0;
  CloseHandle(hFile);
#else
  int fd = open(configPath, O_RDONLY);
  if (fd < 0)
    return;

  char buf[1024];
  ssize_t read = ::read(fd, buf, 1023);
  if (read < 0)
    read = 0;
  buf[read] = 0;
  close(fd);
#endif

  int lineStart = 0;
  for (int i = 0; i <= (int)read; i++)
  {
    if (buf[i] == '\n' || buf[i] == 0)
    {
      buf[i] = 0;
      char *line = buf + lineStart;

      int eq = -1;
      for (int j = 0; line[j]; j++)
      {
        if (line[j] == '=')
        {
          eq = j;
          break;
        }
      }

      if (eq > 0)
      {
        line[eq] = 0;
        char *key = line;
        char *val = line + eq + 1;

        if (StrEquals(key, "darkMode"))
          options.darkMode = StrEquals(val, "1");
        else if (StrEquals(key, "bytesPerLine"))
          options.defaultBytesPerLine = StrToInt(val);
        else if (StrEquals(key, "autoReload"))
          options.autoReload = StrEquals(val, "1");
        else if (StrEquals(key, "language"))
          StrCopy(options.language, val);
      }

      lineStart = i + 1;
    }
  }
}

static OptionsDialogData *g_dialogData = nullptr;

bool IsPointInRect(int x, int y, const Rect &rect)
{
  return x >= rect.x && x <= rect.x + rect.width &&
         y >= rect.y && y <= rect.y + rect.height;
}

void RenderOptionsDialog(OptionsDialogData *data, int windowWidth, int windowHeight)
{
  if (!data || !data->renderer)
    return;

  Theme theme = data->tempOptions.darkMode ? Theme::Dark() : Theme::Light();
  data->renderer->clear(theme.windowBackground);

  int margin = 20;
  int y = margin;
  int controlHeight = 25;
  int spacing = 10;

  data->renderer->drawText(Translations::T("Options"), margin, y, theme.headerColor);
  y += 35;

  Rect checkboxRect1(margin, y, 18, 18);
  WidgetState checkboxState1(checkboxRect1);
  checkboxState1.hovered = (data->hoveredWidget == 0);
  checkboxState1.pressed = (data->pressedWidget == 0);

  data->renderer->drawModernCheckbox(checkboxState1, theme, data->tempOptions.darkMode);
  data->renderer->drawText(Translations::T("Dark Mode"), margin + 28, y + 2, theme.textColor);
  y += controlHeight + spacing;

  Rect checkboxRect2(margin, y, 18, 18);
  WidgetState checkboxState2(checkboxRect2);
  checkboxState2.hovered = (data->hoveredWidget == 1);
  checkboxState2.pressed = (data->pressedWidget == 1);

  data->renderer->drawModernCheckbox(checkboxState2, theme, data->tempOptions.autoReload);
  data->renderer->drawText(Translations::T("Auto-reload modified file"), margin + 28, y + 2, theme.textColor);
  y += controlHeight + spacing;

  if (!GetIsNativeFlag())
  {
    Rect checkboxRect3(margin, y, 18, 18);
    WidgetState checkboxState3(checkboxRect3);
    checkboxState3.hovered = (data->hoveredWidget == 2);
    checkboxState3.pressed = (data->pressedWidget == 2);

    data->renderer->drawModernCheckbox(checkboxState3, theme, data->tempOptions.contextMenu);
    data->renderer->drawText(Translations::T("Add to context menu (right-click files)"), margin + 28, y + 2, theme.textColor);
    y += controlHeight + spacing * 2;
  }

  data->renderer->drawText(Translations::T("Language:"), margin, y, theme.textColor);
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
      data->dropdownScrollOffset);

  y += controlHeight + spacing * 2 + spacing * 8;

  data->renderer->drawText(
      Translations::T("Default bytes per line:"), margin, y, theme.textColor);
  y += controlHeight;

  Rect radioRect1(margin + 20, y, 16, 16);
  WidgetState radioState1(radioRect1);
  radioState1.hovered = (data->hoveredWidget == 3);
  radioState1.pressed = (data->pressedWidget == 3);

  data->renderer->drawModernRadioButton(radioState1, theme, data->tempOptions.defaultBytesPerLine == 8);
  data->renderer->drawText(Translations::T("8 bytes"), margin + 45, y + 1, theme.textColor);
  y += controlHeight;

  Rect radioRect2(margin + 20, y, 16, 16);
  WidgetState radioState2(radioRect2);
  radioState2.hovered = (data->hoveredWidget == 4);
  radioState2.pressed = (data->pressedWidget == 4);

  data->renderer->drawModernRadioButton(radioState2, theme, data->tempOptions.defaultBytesPerLine == 16);
  data->renderer->drawText(Translations::T("16 bytes"), margin + 45, y + 1, theme.textColor);

  int buttonWidth = 75;
  int buttonHeight = 25;
  int buttonY = windowHeight - margin - buttonHeight - 5;
  int buttonSpacing = 8;

  Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
  WidgetState okButtonState(okButtonRect);
  okButtonState.hovered = (data->hoveredWidget == 5);
  okButtonState.pressed = (data->pressedWidget == 5);

  data->renderer->drawModernButton(okButtonState, theme, Translations::T("OK"));

  Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  WidgetState cancelButtonState(cancelButtonRect);
  cancelButtonState.hovered = (data->hoveredWidget == 6);
  cancelButtonState.pressed = (data->pressedWidget == 6);

  data->renderer->drawModernButton(cancelButtonState, theme, Translations::T("Cancel"));
}

void UpdateHoverState(OptionsDialogData *data, int x, int y, int windowWidth, int windowHeight)
{
  int margin = 20;
  int startY = margin + 35;
  int controlHeight = 25;
  int spacing = 10;

  data->hoveredWidget = -1;
  data->hoveredDropdownItem = -1;

  Rect checkboxRect1(margin, startY, 18, 18);
  if (IsPointInRect(x, y, checkboxRect1))
  {
    data->hoveredWidget = 0;
    return;
  }
  startY += controlHeight + spacing;

  Rect checkboxRect2(margin, startY, 18, 18);
  if (IsPointInRect(x, y, checkboxRect2))
  {
    data->hoveredWidget = 1;
    return;
  }
  startY += controlHeight + spacing;

  if (!GetIsNativeFlag())
  {
    Rect checkboxRect3(margin, startY, 18, 18);
    if (IsPointInRect(x, y, checkboxRect3))
    {
      data->hoveredWidget = 2;
      return;
    }
    startY += controlHeight + spacing * 2;
  }

  startY += controlHeight;

  Rect dropdownRect(margin + 20, startY, 200, controlHeight);
  if (IsPointInRect(x, y, dropdownRect))
  {
    data->hoveredWidget = 7;
    if (!data->dropdownOpen)
      return;
  }

  if (data->dropdownOpen)
  {
    int itemHeight = 28;
    int maxVisibleItems = 3;
    int dropdownItemY = startY + controlHeight + 2;

    for (size_t i = 0; i < data->languages.size(); i++)
    {
      int visualIndex = (int)i - data->dropdownScrollOffset;
      if (visualIndex < 0 || visualIndex >= maxVisibleItems)
        continue;

      Rect itemRect(margin + 20, dropdownItemY + (visualIndex * itemHeight), 200, itemHeight);
      if (IsPointInRect(x, y, itemRect))
      {
        data->hoveredDropdownItem = (int)i;
        return;
      }
    }
  }

  startY += controlHeight + spacing * 2 + spacing * 8 + controlHeight;

  Rect radioRect1(margin + 20, startY, 16, 16);
  if (IsPointInRect(x, y, radioRect1))
  {
    data->hoveredWidget = 3;
    return;
  }
  startY += controlHeight;

  Rect radioRect2(margin + 20, startY, 16, 16);
  if (IsPointInRect(x, y, radioRect2))
  {
    data->hoveredWidget = 4;
    return;
  }

  int buttonWidth = 75;
  int buttonHeight = 25;
  int buttonY = windowHeight - margin - buttonHeight - 5;
  int buttonSpacing = 8;

  Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
  if (IsPointInRect(x, y, okButtonRect))
  {
    data->hoveredWidget = 5;
    return;
  }

  Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  if (IsPointInRect(x, y, cancelButtonRect))
  {
    data->hoveredWidget = 6;
  }
}

void HandleMouseClick(OptionsDialogData *data, int x, int y, int windowWidth, int windowHeight)
{
  if (data->dropdownOpen && data->hoveredDropdownItem >= 0)
  {
    data->selectedLanguage = data->hoveredDropdownItem;
    StrCopy(data->tempOptions.language, data->languages[data->selectedLanguage]);
    Translations::SetLanguage(data->tempOptions.language);
    data->dropdownOpen = false;
    return;
  }

  if (data->hoveredWidget == -1)
    return;

  switch (data->hoveredWidget)
  {
  case 0:
    data->tempOptions.darkMode = !data->tempOptions.darkMode;
    break;
  case 1:
    data->tempOptions.autoReload = !data->tempOptions.autoReload;
    break;
  case 2:
    if (!GetIsNativeFlag())
    {
      data->tempOptions.contextMenu = !data->tempOptions.contextMenu;
    }
    break;
  case 3:
    data->tempOptions.defaultBytesPerLine = 8;
    break;
  case 4:
    data->tempOptions.defaultBytesPerLine = 16;
    break;
  case 5:
  {
#ifdef _WIN32
    bool wasRegistered = ContextMenuRegistry::IsRegistered(UserRole::CurrentUser);
    bool shouldBeRegistered = data->tempOptions.contextMenu;

    if (shouldBeRegistered && !wasRegistered)
    {
      wchar_t exePath[MAX_PATH];
      GetModuleFileNameW(nullptr, exePath, MAX_PATH);

      for (int i = 0; exePath[i]; i++)
      {
        if (exePath[i] == L'/')
          exePath[i] = L'\\';
      }

      if (!ContextMenuRegistry::Register(exePath, UserRole::CurrentUser))
      {
        MessageBoxW(nullptr, L"Failed to register context menu.", L"Error", MB_OK | MB_ICONERROR);
        data->tempOptions.contextMenu = false;
      }
    }
    else if (!shouldBeRegistered && wasRegistered)
    {
      if (!ContextMenuRegistry::Unregister(UserRole::CurrentUser))
      {
        MessageBoxW(nullptr, L"Failed to unregister context menu.", L"Error", MB_OK | MB_ICONERROR);
        data->tempOptions.contextMenu = true;
      }
    }
#endif
    StrCopy(data->tempOptions.language, data->languages[data->selectedLanguage]);
    *data->originalOptions = data->tempOptions;
    data->dialogResult = true;
    data->running = false;
    break;
  }
  case 6:
    data->dialogResult = false;
    data->running = false;
    break;
  case 7:
    data->dropdownOpen = !data->dropdownOpen;
    if (!data->dropdownOpen)
      data->dropdownScrollOffset = 0;
    break;
  }
}

#ifdef _WIN32

LRESULT CALLBACK OptionsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  OptionsDialogData *data = g_dialogData;

  switch (msg)
  {
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (data && data->renderer)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);

      data->renderer->beginFrame();
      RenderOptionsDialog(data, rect.right, rect.bottom);

      data->renderer->endFrame(hdc);
    }

    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_MOUSEMOVE:
  {
    if (data)
    {
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

  case WM_MOUSEWHEEL:
  {
    if (data && data->dropdownOpen)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);

      int margin = 20;
      int startY = margin + 35 + 25 + 10 + 25 + 10;
      if (!GetIsNativeFlag())
        startY += 25 + 20;
      startY += 25;

      Rect dropdownRect(margin + 20, startY, 200, 25);

      if (data->mouseX >= dropdownRect.x && data->mouseX <= dropdownRect.x + dropdownRect.width &&
          data->mouseY >= dropdownRect.y)
      {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int maxVisibleItems = 3;
        int maxScroll = (int)data->languages.size() - maxVisibleItems;
        if (maxScroll < 0)
          maxScroll = 0;

        if (delta > 0)
        {
          data->dropdownScrollOffset--;
          if (data->dropdownScrollOffset < 0)
            data->dropdownScrollOffset = 0;
        }
        else
        {
          data->dropdownScrollOffset++;
          if (data->dropdownScrollOffset > maxScroll)
            data->dropdownScrollOffset = maxScroll;
        }

        UpdateHoverState(data, data->mouseX, data->mouseY, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
      }
    }
    break;
  }

  case WM_LBUTTONDOWN:
  {
    if (data)
    {
      data->mouseDown = true;
      data->pressedWidget = data->hoveredWidget;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONUP:
  {
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      data->mouseDown = false;

      if (data->pressedWidget == data->hoveredWidget || data->hoveredDropdownItem >= 0)
      {
        HandleMouseClick(data, x, y, rect.right, rect.bottom);
      }

      data->pressedWidget = -1;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_CLOSE:
    if (data)
    {
      data->dialogResult = false;
      data->running = false;
    }
    return 0;

  case WM_SIZE:
  {
    if (data && data->renderer)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      if (rect.right > 0 && rect.bottom > 0 && rect.right < 8192 && rect.bottom < 8192)
      {
        data->renderer->resize(rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
    }
    return 0;
  }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool OptionsDialog::Show(HWND parent, AppOptions &options)
{
  const wchar_t *className = L"CustomOptionsWindow";

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = OptionsWindowProc;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

  UnregisterClassW(className, wc.hInstance);
  if (!RegisterClassExW(&wc))
    return false;

  OptionsDialogData data = {};
  data.tempOptions = options;

  if (!GetIsNativeFlag())
  {
    data.tempOptions.contextMenu = ContextMenuRegistry::IsRegistered(UserRole::CurrentUser);
  }
  else
  {
    data.tempOptions.contextMenu = false;
  }

  data.originalOptions = &options;
  data.dialogResult = false;
  data.running = true;
  g_dialogData = &data;

  for (size_t i = 0; i < data.languages.size(); i++)
  {
    if (StrEquals(data.languages[i], options.language))
    {
      data.selectedLanguage = (int)i;
      break;
    }
  }

  int width = 400;
  int height = 480;

  RECT parentRect;
  GetWindowRect(parent, &parentRect);

  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

  HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, className, L"Options",
                              WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, parent, nullptr,
                              GetModuleHandleW(NULL), nullptr);

  if (!hwnd)
  {
    g_dialogData = nullptr;
    return false;
  }

  data.window = hwnd;

  if (data.tempOptions.darkMode)
  {
    BOOL dark = TRUE;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  }

  data.renderer = new RenderManager();
  if (!data.renderer)
  {
    DestroyWindow(hwnd);
    g_dialogData = nullptr;
    return false;
  }

  if (!data.renderer->initialize(hwnd))
  {
    delete data.renderer;
    DestroyWindow(hwnd);
    g_dialogData = nullptr;
    return false;
  }

  RECT clientRect;
  GetClientRect(hwnd, &clientRect);
  if (clientRect.right > 0 && clientRect.bottom > 0)
  {
    data.renderer->resize(clientRect.right, clientRect.bottom);
  }

  EnableWindow(parent, FALSE);
  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  MSG msg;
  while (data.running && GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (data.renderer)
  {
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

void ProcessOptionsEvent(OptionsDialogData *data, XEvent *event, int width, int height)
{
  switch (event->type)
  {
  case Expose:
    if (data->renderer)
    {
      data->renderer->beginFrame();
      RenderOptionsDialog(data, width, height);

      GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
      data->renderer->endFrame(gc);
      XFreeGC(data->display, gc);
    }
    break;

  case MotionNotify:
    data->mouseX = event->xmotion.x;
    data->mouseY = event->xmotion.y;
    UpdateHoverState(data, event->xmotion.x, event->xmotion.y, width, height);

    if (data->renderer)
    {
      data->renderer->beginFrame();
      RenderOptionsDialog(data, width, height);

      GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
      data->renderer->endFrame(gc);
      XFreeGC(data->display, gc);
    }
    break;

  case ButtonPress:
    if (event->xbutton.button == Button1)
    {
      data->mouseDown = true;
      data->pressedWidget = data->hoveredWidget;

      if (data->renderer)
      {
        data->renderer->beginFrame();
        RenderOptionsDialog(data, width, height);

        GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
        data->renderer->endFrame(gc);
        XFreeGC(data->display, gc);
      }
    }
    else if (event->xbutton.button == Button4)
    {
      if (data->dropdownOpen)
      {
        int maxVisibleItems = 3;
        int maxScroll = (int)data->languages.size() - maxVisibleItems;
        if (maxScroll < 0)
          maxScroll = 0;
        data->dropdownScrollOffset--;
        if (data->dropdownScrollOffset < 0)
          data->dropdownScrollOffset = 0;
        UpdateHoverState(data, data->mouseX, data->mouseY, width, height);

        if (data->renderer)
        {
          data->renderer->beginFrame();
          RenderOptionsDialog(data, width, height);

          GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
          data->renderer->endFrame(gc);
          XFreeGC(data->display, gc);
        }
      }
    }
    else if (event->xbutton.button == Button5)
    {
      if (data->dropdownOpen)
      {
        int maxVisibleItems = 3;
        int maxScroll = (int)data->languages.size() - maxVisibleItems;
        if (maxScroll < 0)
          maxScroll = 0;
        data->dropdownScrollOffset++;
        if (data->dropdownScrollOffset > maxScroll)
          data->dropdownScrollOffset = maxScroll;
        UpdateHoverState(data, data->mouseX, data->mouseY, width, height);

        if (data->renderer)
        {
          data->renderer->beginFrame();
          RenderOptionsDialog(data, width, height);

          GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
          data->renderer->endFrame(gc);
          XFreeGC(data->display, gc);
        }
      }
    }
    break;

  case ButtonRelease:
    if (event->xbutton.button == Button1)
    {
      data->mouseDown = false;
      if (data->pressedWidget == data->hoveredWidget || data->hoveredDropdownItem >= 0)
      {
        HandleMouseClick(data, event->xbutton.x, event->xbutton.y, width, height);
      }
      data->pressedWidget = -1;

      if (data->renderer)
      {
        data->renderer->beginFrame();
        RenderOptionsDialog(data, width, height);

        GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
        data->renderer->endFrame(gc);
        XFreeGC(data->display, gc);
      }
    }
    break;

  case ClientMessage:
    if ((Atom)event->xclient.data.l[0] == data->wmDeleteWindow)
    {
      data->dialogResult = false;
      data->running = false;
    }
    break;

  case ConfigureNotify:
    if (event->xconfigure.width != width || event->xconfigure.height != height)
    {
      if (data->renderer)
      {
        data->renderer->resize(event->xconfigure.width, event->xconfigure.height);

        data->renderer->beginFrame();
        RenderOptionsDialog(data, event->xconfigure.width, event->xconfigure.height);

        GC gc = XCreateGC(data->display, (Window)(uintptr_t)data->window, 0, nullptr);
        data->renderer->endFrame(gc);
        XFreeGC(data->display, gc);
      }
    }
    break;
  }
}

bool OptionsDialog::Show(NativeWindow parent, AppOptions &options)
{
  Display *display = XOpenDisplay(nullptr);
  if (!display)
    return false;

  int screen = DefaultScreen(display);
  Window rootWindow = RootWindow(display, screen);

  OptionsDialogData data = {};
  data.tempOptions = options;
  data.tempOptions.contextMenu = false;
  data.originalOptions = &options;
  data.dialogResult = false;
  data.running = true;
  data.display = display;
  g_dialogData = &data;

  for (size_t i = 0; i < data.languages.size(); i++)
  {
    if (StrEquals(data.languages[i], options.language))
    {
      data.selectedLanguage = (int)i;
      break;
    }
  }

  int width = 400;
  int height = 480;

  Window window = XCreateSimpleWindow(display, rootWindow, 100, 100, width, height, 1,
                                      BlackPixel(display, screen), WhitePixel(display, screen));

  data.window = (NativeWindow)(uintptr_t)window;

  XStoreName(display, window, "Options");

  Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom wmWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(display, window, wmWindowType, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&wmWindowTypeDialog, 1);

  Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(display, window, &wmDelete, 1);
  data.wmDeleteWindow = wmDelete;

  XSelectInput(display, window, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

  XMapWindow(display, window);
  XFlush(display);

  data.renderer = new RenderManager();
  if (!data.renderer || !data.renderer->initialize(window))
  {
    if (data.renderer)
      delete data.renderer;
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    g_dialogData = nullptr;
    return false;
  }

  data.renderer->resize(width, height);

  XEvent event;
  while (data.running)
  {
    XNextEvent(display, &event);
    ProcessOptionsEvent(&data, &event, width, height);
  }

  if (data.renderer)
    delete data.renderer;
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  g_dialogData = nullptr;

  return data.dialogResult;
}

#elif defined(__APPLE__)

bool OptionsDialog::Show(NativeWindow parent, AppOptions &options)
{
  (void)parent;
  (void)options;
  return false;
}

#endif
