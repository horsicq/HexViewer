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

#include "options.h"

extern int g_RecentFileCount;
extern char g_RecentFiles[10][MAX_PATH_LEN];
static OptionsDialogData *g_dialogData = nullptr;

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

void SaveOptionsToFile(const AppOptions& options)
{
  char configPath[512];
  GetConfigPath(configPath, 512);

  char buf[256];

#ifdef _WIN32
  wchar_t wpath[512];
  MultiByteToWideChar(CP_UTF8, 0, configPath, -1, wpath, 512);
  CreateDirectoriesForPath(wpath);

  HANDLE hFile = CreateFileW(wpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
    FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
    return;

  DWORD written;

  StrCopy(buf, "[Options]\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "darkMode=");
  StrCopy(buf + 9, options.darkMode ? "1" : "0");
  StrCopy(buf + 10, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "bookmarkHighlights=");
  StrCopy(buf + 19, options.bookmarkHighlights ? "1" : "0");
  StrCopy(buf + 20, "\n");
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

  StrCopy(buf, "fontSize=");
  IntToStr(options.fontSize, buf + 9, 247);
  len = (int)StrLen(buf);
  StrCopy(buf + len, "\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  StrCopy(buf, "\n[RecentFiles]\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  for (int i = 0; i < g_RecentFileCount; i++)
  {
    StrCopy(buf, "File");

    char numStr[4];
    int num = i;
    int numLen = 0;
    if (num == 0)
    {
      numStr[numLen++] = '0';
    }
    else
    {
      int temp = num;
      int digits = 0;
      while (temp > 0)
      {
        temp /= 10;
        digits++;
      }
      for (int j = digits - 1; j >= 0; j--)
      {
        numStr[j] = '0' + (num % 10);
        num /= 10;
      }
      numLen = digits;
    }
    numStr[numLen] = '\0';

    StrCopy(buf + 4, numStr);
    int pos = 4 + numLen;
    buf[pos++] = '=';

    StrCopy(buf + pos, g_RecentFiles[i]);
    len = (int)StrLen(buf);
    buf[len++] = '\n';
    buf[len] = '\0';

    WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);
  }

  StrCopy(buf, "\n[Plugins]\n");
  WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);

  for (int i = 0; i < options.enabledPluginCount; i++)
  {
    StrCopy(buf, "enabled=");
    StrCopy(buf + 8, options.enabledPlugins[i]);
    len = (int)StrLen(buf);
    buf[len++] = '\n';
    buf[len] = '\0';

    WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);
  }

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

  StrCopy(buf, "[Options]\n");
  write(fd, buf, StrLen(buf));

  StrCopy(buf, "darkMode=");
  StrCopy(buf + 9, options.darkMode ? "1\n" : "0\n");
  write(fd, buf, StrLen(buf));

  StrCopy(buf, "bookmarkHighlights=");
  StrCopy(buf + 19, options.bookmarkHighlights ? "1\n" : "0\n");
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

  StrCopy(buf, "\n[RecentFiles]\n");
  write(fd, buf, StrLen(buf));

  for (int i = 0; i < g_RecentFileCount; i++)
  {
    StrCopy(buf, "File");

    char numStr[4];
    int num = i;
    int numLen = 0;
    if (num == 0)
    {
      numStr[numLen++] = '0';
    }
    else
    {
      int temp = num;
      int digits = 0;
      while (temp > 0)
      {
        temp /= 10;
        digits++;
      }
      for (int j = digits - 1; j >= 0; j--)
      {
        numStr[j] = '0' + (num % 10);
        num /= 10;
      }
      numLen = digits;
    }
    numStr[numLen] = '\0';

    StrCopy(buf + 4, numStr);
    int pos = 4 + numLen;
    buf[pos++] = '=';
    StrCopy(buf + pos, g_RecentFiles[i]);
    len = (int)StrLen(buf);
    buf[len++] = '\n';
    buf[len] = '\0';

    write(fd, buf, StrLen(buf));
  }

  StrCopy(buf, "\n[Plugins]\n");
  write(fd, buf, StrLen(buf));

  for (int i = 0; i < options.enabledPluginCount; i++)
  {
    StrCopy(buf, "enabled=");
    StrCopy(buf + 8, options.enabledPlugins[i]);
    len = (int)StrLen(buf);
    buf[len++] = '\n';
    buf[len] = '\0';

    write(fd, buf, StrLen(buf));
  }

  close(fd);
#endif
}

void LoadOptionsFromFile(AppOptions& options)
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
  char buf[8192];
  DWORD read;
  ReadFile(hFile, buf, sizeof(buf) - 1, &read, nullptr);
  buf[read] = 0;
  CloseHandle(hFile);
#else
  int fd = open(configPath, O_RDONLY);
  if (fd < 0)
    return;
  char buf[8192];
  ssize_t read = ::read(fd, buf, sizeof(buf) - 1);
  if (read < 0)
    read = 0;
  buf[read] = 0;
  close(fd);
#endif

  bool inRecentFilesSection = false;
  bool inPluginsSection = false;
  g_RecentFileCount = 0;
  options.enabledPluginCount = 0;

  int lineStart = 0;
  for (int i = 0; i <= (int)read; i++)
  {
    if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == 0)
    {
      if (i > lineStart)
      {
        char originalChar = buf[i];
        buf[i] = 0;
        char* line = buf + lineStart;

        while (*line == ' ' || *line == '\t')
          line++;

        if (*line == 0)
        {
          lineStart = i + 1;
          buf[i] = originalChar;
          continue;
        }

        if (line[0] == '[')
        {
          if (strEquals(line, "[Options]"))
          {
            inRecentFilesSection = false;
            inPluginsSection = false;
          }
          else if (strEquals(line, "[RecentFiles]"))
          {
            inRecentFilesSection = true;
            inPluginsSection = false;
          }
          else if (strEquals(line, "[Plugins]"))
          {
            inRecentFilesSection = false;
            inPluginsSection = true;
          }
          lineStart = i + 1;
          buf[i] = originalChar;
          continue;
        }

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
          char* key = line;
          char* val = line + eq + 1;

          if (inPluginsSection)
          {
            if (strEquals(key, "enabled") && val[0] != 0)
            {
              if (options.enabledPluginCount < 10)
              {
                StrCopy(options.enabledPlugins[options.enabledPluginCount], val);
                options.enabledPluginCount++;
              }
            }
          }
          else if (inRecentFilesSection)
          {
            if (key[0] == 'F' && key[1] == 'i' && key[2] == 'l' && key[3] == 'e')
            {
              int fileIndex = StrToInt(key + 4);

              if (fileIndex >= 0 && fileIndex < 10 && val[0] != 0)
              {
                StrCopy(g_RecentFiles[fileIndex], val);
                if (fileIndex >= g_RecentFileCount)
                {
                  g_RecentFileCount = fileIndex + 1;
                }
              }
            }
          }
          else
          {
            if (strEquals(key, "darkMode"))
              options.darkMode = strEquals(val, "1");

            else if (strEquals(key, "bookmarkHighlights"))
              options.bookmarkHighlights = strEquals(val, "1");

            else if (strEquals(key, "bytesPerLine"))
              options.defaultBytesPerLine = StrToInt(val);
            else if (strEquals(key, "autoReload"))
              options.autoReload = strEquals(val, "1");
            else if (strEquals(key, "language"))
              StrCopy(options.language, val);
            else if (strEquals(key, "fontSize"))
              options.fontSize = StrToInt(val);
          }
        }

        buf[i] = originalChar;
      }
      lineStart = i + 1;
    }
  }
}

bool IsPointInRect(int x, int y, const Rect &rect)
{
  return x >= rect.x && x <= rect.x + rect.width &&
         y >= rect.y && y <= rect.y + rect.height;
}

inline int NextY(int& y, int controlHeight, int spacing)
{
  y += controlHeight + spacing;
  return y;
}

void RenderOptionsDialog(OptionsDialogData* data, int windowWidth, int windowHeight)
{
  if (!data || !data->renderer)
    return;

  Theme theme = data->tempOptions.darkMode ? Theme::Dark() : Theme::Light();
  data->renderer->clear(theme.windowBackground);

  const int margin = 20;
  const int controlHeight = 25;
  const int controlSpacing = 10;
  const int sectionSpacing = 20;

  int y = margin;

#define NEXT_Y(spacing) (y += controlHeight + (spacing))

  data->renderer->drawText(Translations::T("Options"), margin, y, theme.headerColor);
  NEXT_Y(15);

  {
    Rect r(margin, y, 18, 18);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 0);
    ws.pressed = (data->pressedWidget == 0);

    data->renderer->drawModernCheckbox(ws, theme, data->tempOptions.darkMode);
    data->renderer->drawText(Translations::T("Dark Mode"), margin + 28, y + 2, theme.textColor);
    NEXT_Y(controlSpacing);
  }

  {
    Rect r(margin, y, 18, 18);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 9);
    ws.pressed = (data->pressedWidget == 9);

    data->renderer->drawModernCheckbox(ws, theme, data->tempOptions.bookmarkHighlights);
    data->renderer->drawText(Translations::T("Highlight bookmarks"), margin + 28, y + 2, theme.textColor);
    NEXT_Y(controlSpacing);
  }

  {
    Rect r(margin, y, 18, 18);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 1);
    ws.pressed = (data->pressedWidget == 1);

    data->renderer->drawModernCheckbox(ws, theme, data->tempOptions.autoReload);
    data->renderer->drawText(Translations::T("Auto-reload modified file"), margin + 28, y + 2, theme.textColor);
    NEXT_Y(controlSpacing);
  }


  if (!GetIsNativeFlag())
  {
    Rect r(margin, y, 18, 18);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 2);
    ws.pressed = (data->pressedWidget == 2);

    data->renderer->drawModernCheckbox(ws, theme, data->tempOptions.contextMenu);
    data->renderer->drawText(
      Translations::T("Add to context menu (right-click files)"),
      margin + 28, y + 2, theme.textColor);

    NEXT_Y(sectionSpacing);
  }

  data->renderer->drawText(Translations::T("Language:"), margin, y, theme.textColor);
  NEXT_Y(controlSpacing);

  int languageDropdownY = y;
  {
    Rect r(margin + 20, languageDropdownY, 200, controlHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 7);
    ws.pressed = (data->pressedWidget == 7);

    if (!data->dropdownOpen)
    {
      data->renderer->drawDropdown(
        ws,
        theme,
        data->languages[data->selectedLanguage],
        false,
        data->languages,
        data->selectedLanguage,
        data->hoveredDropdownItem,
        data->dropdownScrollOffset);
    }

    NEXT_Y(sectionSpacing);
  }

  data->renderer->drawText(Translations::T("Font Size:"), margin, y, theme.textColor);
  NEXT_Y(controlSpacing);

  int fontDropdownY = y;
  {
    char fontSizeLabel[32];
    IntToStr(data->tempOptions.fontSize, fontSizeLabel, 32);
    StrCat(fontSizeLabel, "pt");

    Rect r(margin + 20, fontDropdownY, 200, controlHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 8);
    ws.pressed = (data->pressedWidget == 8);

    if (!data->fontDropdownOpen)
    {
      data->renderer->drawDropdown(
        ws,
        theme,
        fontSizeLabel,
        false,
        data->fontSizes,
        data->selectedFontSize,
        data->hoveredFontDropdownItem,
        data->fontDropdownScrollOffset);
    }

    NEXT_Y(sectionSpacing);
  }
	y += sectionSpacing *2;

  data->renderer->drawText(
    Translations::T("Default bytes per line:"), margin, y, theme.textColor);
  NEXT_Y(controlSpacing);

  int radio8Y = y;
  {
    Rect r(margin + 20, radio8Y, 16, 16);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 3);
    ws.pressed = (data->pressedWidget == 3);

    data->renderer->drawModernRadioButton(ws, theme, data->tempOptions.defaultBytesPerLine == 8);
    data->renderer->drawText(Translations::T("8 bytes"), margin + 45, radio8Y + 1, theme.textColor);
    NEXT_Y(controlSpacing);
  }

  int radio16Y = y;
  {
    Rect r(margin + 20, radio16Y, 16, 16);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 4);
    ws.pressed = (data->pressedWidget == 4);

    data->renderer->drawModernRadioButton(ws, theme, data->tempOptions.defaultBytesPerLine == 16);
    data->renderer->drawText(Translations::T("16 bytes"), margin + 45, radio16Y + 1, theme.textColor);
  }

  const int buttonWidth = 75;
  const int buttonHeight = 25;
  const int buttonSpacing = 8;
  const int buttonY = windowHeight - margin - buttonHeight - 5;

  {
    Rect r(windowWidth - margin - buttonWidth * 2 - buttonSpacing,
      buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 5);
    ws.pressed = (data->pressedWidget == 5);
    data->renderer->drawModernButton(ws, theme, Translations::T("OK"));
  }

  {
    Rect r(windowWidth - margin - buttonWidth,
      buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 6);
    ws.pressed = (data->pressedWidget == 6);
    data->renderer->drawModernButton(ws, theme, Translations::T("Cancel"));
  }


  if (data->dropdownOpen)
  {
    Rect r(margin + 20, languageDropdownY, 200, controlHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 7);
    ws.pressed = (data->pressedWidget == 7);

    data->renderer->drawDropdown(
      ws,
      theme,
      data->languages[data->selectedLanguage],
      true,
      data->languages,
      data->selectedLanguage,
      data->hoveredDropdownItem,
      data->dropdownScrollOffset);
  }

  if (data->fontDropdownOpen)
  {
    char fontSizeLabel[32];
    IntToStr(data->tempOptions.fontSize, fontSizeLabel, 32);
    StrCat(fontSizeLabel, "pt");

    Rect r(margin + 20, fontDropdownY, 200, controlHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 8);
    ws.pressed = (data->pressedWidget == 8);

    data->renderer->drawDropdown(
      ws,
      theme,
      fontSizeLabel,
      true,
      data->fontSizes,
      data->selectedFontSize,
      data->hoveredFontDropdownItem,
      data->fontDropdownScrollOffset);
  }

#undef NEXT_Y
}

void UpdateHoverState(OptionsDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  const int margin = 20;
  const int controlHeight = 25;
  const int controlSpacing = 10;
  const int sectionSpacing = 20;

  int startY = margin;

  data->hoveredWidget = -1;
  data->hoveredDropdownItem = -1;
  data->hoveredFontDropdownItem = -1;

#define NEXT_Y(spacing) (startY += controlHeight + (spacing))

  NEXT_Y(15);

  {
    Rect r(margin, startY, 18, 18);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 0; return; }
    NEXT_Y(controlSpacing);
  }

  {
    Rect r(margin, startY, 18, 18);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 9; return; }
    NEXT_Y(controlSpacing);
  }

  {
    Rect r(margin, startY, 18, 18);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 1; return; }
    NEXT_Y(controlSpacing);
  }


  if (!GetIsNativeFlag())
  {
    Rect r(margin, startY, 18, 18);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 2; return; }
    NEXT_Y(sectionSpacing);
  }

  NEXT_Y(controlSpacing);

  {
    Rect dropdownRect(margin + 20, startY, 200, controlHeight);
    if (IsPointInRect(x, y, dropdownRect))
    {
      data->hoveredWidget = 7;
      if (!data->dropdownOpen)
      {
        return;
      }
    }

    if (data->dropdownOpen)
    {
      int itemHeight = 32;
      int maxVisible = 5;
      int dropdownItemY = startY + controlHeight + 4;

      for (size_t i = 0; i < data->languages.size(); i++)
      {
        int visualIndex = (int)i - data->dropdownScrollOffset;
        if (visualIndex < 0 || visualIndex >= maxVisible)
          continue;

        Rect itemRect(
          margin + 20,
          dropdownItemY + visualIndex * itemHeight + 4,
          200,
          itemHeight - 4);

        if (IsPointInRect(x, y, itemRect))
        {
          data->hoveredDropdownItem = (int)i;
          return;
        }
      }
    }

    NEXT_Y(sectionSpacing);
  }

  NEXT_Y(controlSpacing);

  {
    Rect fontDropdownRect(margin + 20, startY, 200, controlHeight);
    if (IsPointInRect(x, y, fontDropdownRect))
    {
      data->hoveredWidget = 8;
      if (!data->fontDropdownOpen)
      {
        return;
      }
    }

    if (data->fontDropdownOpen)
    {
      int itemHeight = 32;
      int maxVisible = 5;
      int fontDropdownItemY = startY + controlHeight + 4;

      for (size_t i = 0; i < data->fontSizes.size(); i++)
      {
        int visualIndex = (int)i - data->fontDropdownScrollOffset;
        if (visualIndex < 0 || visualIndex >= maxVisible)
          continue;

        Rect itemRect(
          margin + 20,
          fontDropdownItemY + visualIndex * itemHeight + 4,
          200,
          itemHeight - 4);

        if (IsPointInRect(x, y, itemRect))
        {
          data->hoveredFontDropdownItem = (int)i;
          return;
        }
      }
    }

    NEXT_Y(sectionSpacing);
  }

  NEXT_Y(controlSpacing);

  {
    Rect r(margin + 20, startY, 16, 16);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 3; return; }
    NEXT_Y(controlSpacing);
  }

  {
    Rect r(margin + 20, startY, 16, 16);
    if (IsPointInRect(x, y, r)) { data->hoveredWidget = 4; return; }
  }

  const int buttonWidth = 75;
  const int buttonHeight = 25;
  const int buttonSpacing = 8;
  const int buttonY = windowHeight - margin - buttonHeight - 5;

  Rect okButtonRect(
    windowWidth - margin - buttonWidth * 2 - buttonSpacing,
    buttonY,
    buttonWidth,
    buttonHeight);

  if (IsPointInRect(x, y, okButtonRect))
  {
    data->hoveredWidget = 5;
    return;
  }

  Rect cancelButtonRect(
    windowWidth - margin - buttonWidth,
    buttonY,
    buttonWidth,
    buttonHeight);

  if (IsPointInRect(x, y, cancelButtonRect))
  {
    data->hoveredWidget = 6;
    return;
  }

#undef NEXT_Y
}

void HandleMouseClick(OptionsDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  if (data->dropdownOpen && data->hoveredDropdownItem >= 0)
  {
    data->selectedLanguage = data->hoveredDropdownItem;
    StrCopy(data->tempOptions.language, data->languages[data->selectedLanguage]);
    Translations::SetLanguage(data->tempOptions.language);
    data->dropdownOpen = false;
    return;
  }

  if (data->fontDropdownOpen && data->hoveredFontDropdownItem >= 0)
  {
    data->selectedFontSize = data->hoveredFontDropdownItem;

    int actualSizes[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24 };
    if (data->selectedFontSize >= 0 && data->selectedFontSize < 13)
    {
      data->tempOptions.fontSize = actualSizes[data->selectedFontSize];
    }

    data->fontDropdownOpen = false;
    return;
  }

  if (data->hoveredWidget == -1)
    return;

  switch (data->hoveredWidget)
  {
  case 0:
    data->tempOptions.darkMode = !data->tempOptions.darkMode;
    break;

  case 9:
    data->tempOptions.bookmarkHighlights = !data->tempOptions.bookmarkHighlights;
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

    if (data->dropdownOpen)
      data->fontDropdownOpen = false;
    break;

  case 8:
    data->fontDropdownOpen = !data->fontDropdownOpen;
    if (!data->fontDropdownOpen)
      data->fontDropdownScrollOffset = 0;

    if (data->fontDropdownOpen)
      data->dropdownOpen = false;
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
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int margin = 20;

      int startY = margin + 15;
      startY += 25 + 10;
      startY += 25 + 10;

      if (!GetIsNativeFlag())
        startY += 25 + 20;

      startY += 25;

      int languageDropdownY = startY;
      int fontDropdownY = languageDropdownY + 20 + 25;

      int delta = GET_WHEEL_DELTA_WPARAM(wParam);

      if (data->dropdownOpen)
      {
        Rect dropdownRect(margin + 20, languageDropdownY, 200, 25);

        if (data->mouseX >= dropdownRect.x &&
          data->mouseX <= dropdownRect.x + dropdownRect.width &&
          data->mouseY >= dropdownRect.y)
        {
          int maxVisibleItems = 5;
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

      if (data->fontDropdownOpen)
      {
        Rect fontDropdownRect(margin + 20, fontDropdownY, 200, 25);

        if (data->mouseX >= fontDropdownRect.x &&
          data->mouseX <= fontDropdownRect.x + fontDropdownRect.width &&
          data->mouseY >= fontDropdownRect.y)
        {
          int maxVisibleItems = 5;
          int maxScroll = (int)data->fontSizes.size() - maxVisibleItems;
          if (maxScroll < 0)
            maxScroll = 0;

          if (delta > 0)
          {
            data->fontDropdownScrollOffset--;
            if (data->fontDropdownScrollOffset < 0)
              data->fontDropdownScrollOffset = 0;
          }
          else
          {
            data->fontDropdownScrollOffset++;
            if (data->fontDropdownScrollOffset > maxScroll)
              data->fontDropdownScrollOffset = maxScroll;
          }

          UpdateHoverState(data, data->mouseX, data->mouseY, rect.right, rect.bottom);
          InvalidateRect(hwnd, NULL, FALSE);
          return 0;
        }
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
  if (!RegisterClassExW(&wc))
    return false;
  OptionsDialogData data = {};
  data.tempOptions = options;
  data.tempOptions.bookmarkHighlights = options.bookmarkHighlights;
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
    if (strEquals(data.languages[i], options.language))
    {
      data.selectedLanguage = (int)i;
      break;
    }
  }

  int actualSizes[] = { 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24 };
  data.selectedFontSize = 6;
  for (int i = 0; i < 13; i++)
  {
    if (actualSizes[i] == options.fontSize)
    {
      data.selectedFontSize = i;
      break;
    }
  }

  int width = 400;
  int height = 600;
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
  data.tempOptions.bookmarkHighlights = options.bookmarkHighlights;
  data.tempOptions.contextMenu = false;
  data.originalOptions = &options;
  data.dialogResult = false;
  data.running = true;
  data.display = display;
  g_dialogData = &data;

  for (size_t i = 0; i < data.languages.size(); i++)
  {
    if (strEquals(data.languages[i], options.language))
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
