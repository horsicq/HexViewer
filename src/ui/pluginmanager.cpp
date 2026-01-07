#include "pluginmanager.h"
#include "options.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <dirent.h>
#include <unistd.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#endif

static PluginManagerData *g_pluginDialogData = nullptr;

void GetPluginDirectory(char *outPath, int maxLen)
{
#ifdef _WIN32
    if (GetIsMsixFlag())
    {
        wchar_t localAppData[MAX_PATH];
        DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (len > 0 && len < MAX_PATH)
        {
            WideCharToMultiByte(CP_UTF8, 0, localAppData, -1, outPath, maxLen - 50, nullptr, nullptr);
            int slen = (int)StrLen(outPath);
            StrCopy(outPath + slen, "\\HexViewer\\plugins");
        }
    }
    else
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(nullptr, exePath, MAX_PATH);

        int lastSlash = -1;
        for (int i = 0; exePath[i]; i++)
        {
            if (exePath[i] == L'\\' || exePath[i] == L'/')
                lastSlash = i;
        }
        exePath[lastSlash] = 0;

        WideCharToMultiByte(CP_UTF8, 0, exePath, -1, outPath, maxLen - 20, nullptr, nullptr);
        int len = (int)StrLen(outPath);
        StrCopy(outPath + len, "\\plugins");
    }
#else
    if (GetIsNativeFlag())
    {
        const char *home = getenv("HOME");
        if (home)
        {
            StrCopy(outPath, home);
            int len = (int)StrLen(outPath);
            StrCopy(outPath + len, "/.config/HexViewer/plugins");
        }
    }
    else
    {
        char exePath[4096];
#ifdef __APPLE__
        uint32_t size = sizeof(exePath);
        if (_NSGetExecutablePath(exePath, &size) == 0)
        {
#else
        ssize_t count = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
        if (count != -1)
        {
            exePath[count] = 0;
#endif
            int lastSlash = -1;
            for (int i = 0; exePath[i]; i++)
            {
                if (exePath[i] == '/')
                    lastSlash = i;
            }
            exePath[lastSlash] = 0;
            StrCopy(outPath, exePath);
            int len = (int)StrLen(outPath);
            StrCopy(outPath + len, "/plugins");
        }
    }
#endif
}

bool CheckPluginCapabilities(const char* pluginPath, PluginInfo* info) {
    if (!pluginPath || !info) return false;
    
    info->canDisassemble = false;
    info->canAnalyze = false;
    info->canTransform = false;
    
    if (!StrEquals(info->language, "python")) {
        return false;
    }
    
    if (!InitializePythonRuntime()) {
        StrCopy(info->description, "Python not installed");
        return false;
    }
    
    info->canDisassemble = CanPluginDisassemble(pluginPath);
    info->canAnalyze = CanPluginAnalyze(pluginPath);
    info->canTransform = CanPluginTransform(pluginPath);
    
    bool hasCapability = (info->canDisassemble || info->canAnalyze || info->canTransform);
    
    if (!hasCapability) {
        StrCopy(info->description, "No valid plugin functions");
    }
    
    return hasCapability;
}

void CreatePluginDirectory(const char *path)
{
#ifdef _WIN32
    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 512);
    CreateDirectoryW(wpath, nullptr);
#else
    mkdir(path, 0755);
#endif
}

bool PluginManager::ParsePluginManifest(const char *manifestPath, PluginInfo *info)
{
    if (!info)
        return false;

#ifdef _WIN32
    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, manifestPath, -1, wpath, 512);

    HANDLE hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    char buf[2048];
    DWORD read;
    ReadFile(hFile, buf, 2047, &read, nullptr);
    buf[read] = 0;
    CloseHandle(hFile);
#else
    int fd = open(manifestPath, O_RDONLY);
    if (fd < 0)
        return false;

    char buf[2048];
    ssize_t read = ::read(fd, buf, 2047);
    if (read < 0)
        read = 0;
    buf[read] = 0;
    close(fd);
#endif

    int lineStart = 0;
    for (int i = 0; i <= (int)read; i++)
    {
        if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == 0)
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

                if (StrEquals(key, "name"))
                    StrCopy(info->name, val);
                else if (StrEquals(key, "version"))
                    StrCopy(info->version, val);
                else if (StrEquals(key, "author"))
                    StrCopy(info->author, val);
                else if (StrEquals(key, "description"))
                    StrCopy(info->description, val);
                else if (StrEquals(key, "language"))
                    StrCopy(info->language, val);
            }

            lineStart = i + 1;
        }
    }

    return info->name[0] != 0;
}

void PluginManager::ScanDirectory(const char *path, PluginManagerData *data)
{
#ifdef _WIN32
    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 512);

    wchar_t searchPath[520];
    for (int i = 0; wpath[i]; i++)
        searchPath[i] = wpath[i];
    int len = 0;
    while (searchPath[len])
        len++;
    searchPath[len] = L'\\';
    searchPath[len + 1] = L'*';
    searchPath[len + 2] = 0;

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath, &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do
    {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        int nameLen = 0;
        while (findData.cFileName[nameLen]) nameLen++;
        
        bool isPython = false;
        bool isJavaScript = false;
        
        if (nameLen > 3) {
            if (findData.cFileName[nameLen-3] == L'.' &&
                findData.cFileName[nameLen-2] == L'p' &&
                findData.cFileName[nameLen-1] == L'y') {
                isPython = true;
            }
        }
        
        if (nameLen > 3) {
            if (findData.cFileName[nameLen-3] == L'.' &&
                findData.cFileName[nameLen-2] == L'j' &&
                findData.cFileName[nameLen-1] == L's') {
                isJavaScript = true;
            }
        }
        
        if (!isPython && !isJavaScript)
            continue;

        PluginInfo *info = (PluginInfo *)PlatformAlloc(sizeof(PluginInfo));
        MemSet(info, 0, sizeof(PluginInfo));

        WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, info->name, 128, nullptr, nullptr);
        
        if (isPython) {
            StrCopy(info->language, "python");
        } else {
            StrCopy(info->language, "javascript");
        }
        
        StrCopy(info->version, "1.0");
        StrCopy(info->author, "Unknown");
        
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, info->path, 512, nullptr, nullptr);
        int pathLen = (int)StrLen(info->path);
        info->path[pathLen] = '\\';
        WideCharToMultiByte(CP_UTF8, 0, findData.cFileName, -1, 
            info->path + pathLen + 1, 512 - pathLen - 1, nullptr, nullptr);

        info->enabled = false;
        info->loaded = false;
        
        info->canDisassemble = false;
        info->canAnalyze = false;
        info->canTransform = false;
        
        if (isPython) {
            GetPythonPluginInfo(info->path, info);
            if (CheckPluginCapabilities(info->path, info)) {
                if (info->canDisassemble && info->canAnalyze) {
                    StrCopy(info->description, "Disassembler & Analyzer");
                } else if (info->canDisassemble) {
                    StrCopy(info->description, "Disassembler Plugin");
                } else if (info->canAnalyze) {
                    StrCopy(info->description, "Analysis Plugin");
                } else if (info->canTransform) {
                    StrCopy(info->description, "Data Transform Plugin");
                } else {
                    StrCopy(info->description, "Python Plugin");
                }
            } else {
                StrCopy(info->description, "Python Plugin (error loading)");
            }
        } else {
            StrCopy(info->description, "JavaScript Plugin (not supported)");
        }
        
        data->plugins.push_back(info);

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    
#else
    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (entry->d_name[0] == '.')
            continue;

        char fullPath[512];
        StrCopy(fullPath, path);
        int len = (int)StrLen(fullPath);
        fullPath[len] = '/';
        StrCopy(fullPath + len + 1, entry->d_name);

        struct stat st;
        if (stat(fullPath, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        int nameLen = 0;
        while (entry->d_name[nameLen]) nameLen++;
        
        bool isPython = false;
        bool isJavaScript = false;
        
        if (nameLen > 3) {
            if (entry->d_name[nameLen-3] == '.' &&
                entry->d_name[nameLen-2] == 'p' &&
                entry->d_name[nameLen-1] == 'y') {
                isPython = true;
            }
        }
        
        if (nameLen > 3) {
            if (entry->d_name[nameLen-3] == '.' &&
                entry->d_name[nameLen-2] == 'j' &&
                entry->d_name[nameLen-1] == 's') {
                isJavaScript = true;
            }
        }
        
        if (!isPython && !isJavaScript)
            continue;

        PluginInfo *info = (PluginInfo *)PlatformAlloc(sizeof(PluginInfo));
        MemSet(info, 0, sizeof(PluginInfo));

        StrCopy(info->name, entry->d_name);
        
        if (isPython) {
            StrCopy(info->language, "python");
        } else {
            StrCopy(info->language, "javascript");
        }
        
        StrCopy(info->version, "1.0");
        StrCopy(info->author, "Unknown");
        
        StrCopy(info->path, fullPath);

        info->enabled = false;
        info->loaded = false;
        
        info->canDisassemble = false;
        info->canAnalyze = false;
        info->canTransform = false;
        
        if (isPython) {
            if (CheckPluginCapabilities(info->path, info)) {
                if (info->canDisassemble && info->canAnalyze) {
                    StrCopy(info->description, "Disassembler & Analyzer");
                } else if (info->canDisassemble) {
                    StrCopy(info->description, "Disassembler Plugin");
                } else if (info->canAnalyze) {
                    StrCopy(info->description, "Analysis Plugin");
                } else if (info->canTransform) {
                    StrCopy(info->description, "Data Transform Plugin");
                } else {
                    StrCopy(info->description, "Python Plugin");
                }
            } else {
                StrCopy(info->description, "Python Plugin (error loading)");
            }
        } else {
            StrCopy(info->description, "JavaScript Plugin (not supported)");
        }
        
        data->plugins.push_back(info);
    }

    closedir(dir);
#endif
}

void PluginManager::LoadPluginsFromDirectory(PluginManagerData *data)
{
    char pluginDir[512];
    GetPluginDirectory(pluginDir, 512);

    CreatePluginDirectory(pluginDir);

    ScanDirectory(pluginDir, data);

    LoadPluginStates(data);
}

void PluginManager::SavePluginStates(PluginManagerData *data)
{
    char configPath[512];
    GetPluginDirectory(configPath, 512);
    int len = (int)StrLen(configPath);
    StrCopy(configPath + len, "/enabled_plugins.txt");

#ifdef _WIN32
    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, configPath, -1, wpath, 512);

    HANDLE hFile = CreateFileW(wpath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    for (size_t i = 0; i < data->plugins.size(); i++)
    {
        if (data->plugins[i]->enabled)
        {
            char buf[256];
            StrCopy(buf, data->plugins[i]->name);
            int bufLen = (int)StrLen(buf);
            StrCopy(buf + bufLen, "\n");

            DWORD written;
            WriteFile(hFile, buf, (DWORD)StrLen(buf), &written, nullptr);
        }
    }

    CloseHandle(hFile);
#else
    int fd = open(configPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return;

    for (size_t i = 0; i < data->plugins.size(); i++)
    {
        if (data->plugins[i]->enabled)
        {
            char buf[256];
            StrCopy(buf, data->plugins[i]->name);
            int bufLen = (int)StrLen(buf);
            StrCopy(buf + bufLen, "\n");
            write(fd, buf, StrLen(buf));
        }
    }

    close(fd);
#endif
}

void PluginManager::LoadPluginStates(PluginManagerData *data)
{
    char configPath[512];
    GetPluginDirectory(configPath, 512);
    int len = (int)StrLen(configPath);
    StrCopy(configPath + len, "/enabled_plugins.txt");

#ifdef _WIN32
    wchar_t wpath[512];
    MultiByteToWideChar(CP_UTF8, 0, configPath, -1, wpath, 512);

    HANDLE hFile = CreateFileW(wpath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    char buf[2048];
    DWORD read;
    ReadFile(hFile, buf, 2047, &read, nullptr);
    buf[read] = 0;
    CloseHandle(hFile);
#else
    int fd = open(configPath, O_RDONLY);
    if (fd < 0)
        return;

    char buf[2048];
    ssize_t read = ::read(fd, buf, 2047);
    if (read < 0)
        read = 0;
    buf[read] = 0;
    close(fd);
#endif

    int lineStart = 0;
    for (int i = 0; i <= (int)read; i++)
    {
        if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == 0)
        {
            buf[i] = 0;
            char *pluginName = buf + lineStart;

            for (size_t j = 0; j < data->plugins.size(); j++)
            {
                if (StrEquals(data->plugins[j]->name, pluginName))
                {
                    data->plugins[j]->enabled = true;
                    break;
                }
            }

            lineStart = i + 1;
        }
    }
}

void RenderPluginManager(PluginManagerData *data, int windowWidth, int windowHeight)
{
    if (!data || !data->renderer)
        return;

    Theme theme = Theme::Dark();
    data->renderer->clear(theme.windowBackground);

    int margin = 20;
    int y = margin;

    data->renderer->drawText("Plugin Manager", margin, y, theme.headerColor);
    y += 35;

    int listHeight = windowHeight - 150;
    Rect listRect(margin, y, windowWidth - margin * 2, listHeight);

    Color listBg(30, 30, 35);
    data->renderer->drawRoundedRect(listRect, 4.0f, listBg, true);
    data->renderer->drawRoundedRect(listRect, 4.0f, theme.controlBorder, false);

    int itemY = y + 10;
    int itemHeight = 60;
    int maxVisibleItems = (listHeight - 20) / itemHeight;

    int startIndex = data->scrollOffset;
    int endIndex = Clamp(startIndex + maxVisibleItems, 0, (int)data->plugins.size());

    for (int i = startIndex; i < endIndex; i++)
    {
        PluginInfo *plugin = data->plugins[i];

        Rect itemRect(margin + 10, itemY, windowWidth - margin * 2 - 20, itemHeight - 5);

        bool isHovered = (i == data->hoveredPlugin);
        bool isSelected = (i == data->selectedPlugin);

        Color itemBg = listBg;
        if (isSelected)
        {
            itemBg = Color(50, 50, 60);
        }
        else if (isHovered)
        {
            itemBg = Color(40, 40, 50);
        }

        data->renderer->drawRoundedRect(itemRect, 3.0f, itemBg, true);

        Rect checkboxRect(itemRect.x + 10, itemRect.y + 20, 18, 18);
        WidgetState checkboxState(checkboxRect);
        checkboxState.hovered = isHovered;

        data->renderer->drawModernCheckbox(checkboxState, theme, plugin->enabled);

        int textX = itemRect.x + 40;
        int textY = itemRect.y + 5;

        data->renderer->drawText(plugin->name, textX, textY, theme.textColor);
        textY += 18;

        char versionAuthor[128];
        StrCopy(versionAuthor, "v");
        StrCat(versionAuthor, plugin->version);
        StrCat(versionAuthor, " by ");
        StrCat(versionAuthor, plugin->author);

        Color subColor(150, 150, 150);
        data->renderer->drawText(versionAuthor, textX, textY, subColor);
        textY += 16;

        Color descColor(120, 120, 120);
        data->renderer->drawText(plugin->description, textX, textY, descColor);

        int badgeX = itemRect.x + itemRect.width - 80;
        int badgeY = itemRect.y + 10;
        Rect badgeRect(badgeX, badgeY, 70, 20);

        Color badgeColor = StrEquals(plugin->language, "python")
                               ? Color(60, 90, 150)
                               : Color(240, 180, 40);

        data->renderer->drawRoundedRect(badgeRect, 3.0f, badgeColor, true);
        data->renderer->drawText(plugin->language, badgeX + 8, badgeY + 3, Color(255, 255, 255));

        itemY += itemHeight;
    }

    int buttonWidth = 100;
    int buttonHeight = 30;
    int buttonY = windowHeight - margin - buttonHeight - 10;
    int buttonSpacing = 10;

    Rect reloadButtonRect(margin, buttonY, buttonWidth, buttonHeight);
    WidgetState reloadButtonState(reloadButtonRect);
    reloadButtonState.hovered = (data->hoveredWidget == 0);
    reloadButtonState.pressed = (data->pressedWidget == 0);
    data->renderer->drawModernButton(reloadButtonState, theme, "Reload");

    Rect settingsButtonRect(margin + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);
    WidgetState settingsButtonState(settingsButtonRect);
    settingsButtonState.enabled = (data->selectedPlugin >= 0);
    settingsButtonState.hovered = (data->hoveredWidget == 1);
    settingsButtonState.pressed = (data->pressedWidget == 1);
    data->renderer->drawModernButton(settingsButtonState, theme, "Settings");

    Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
    WidgetState okButtonState(okButtonRect);
    okButtonState.hovered = (data->hoveredWidget == 2);
    okButtonState.pressed = (data->pressedWidget == 2);
    data->renderer->drawModernButton(okButtonState, theme, "OK");

    Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
    WidgetState cancelButtonState(cancelButtonRect);
    cancelButtonState.hovered = (data->hoveredWidget == 3);
    cancelButtonState.pressed = (data->pressedWidget == 3);
    data->renderer->drawModernButton(cancelButtonState, theme, "Cancel");
}

void UpdatePluginHoverState(PluginManagerData *data, int x, int y, int windowWidth, int windowHeight)
{
    data->hoveredWidget = -1;
    data->hoveredPlugin = -1;

    int margin = 20;
    int listY = margin + 35;
    int listHeight = windowHeight - 150;

    int itemY = listY + 10;
    int itemHeight = 60;
    int maxVisibleItems = (listHeight - 20) / itemHeight;

    int startIndex = data->scrollOffset;
    int endIndex = Clamp(startIndex + maxVisibleItems, 0, (int)data->plugins.size());

    for (int i = startIndex; i < endIndex; i++)
    {
        Rect itemRect(margin + 10, itemY, windowWidth - margin * 2 - 20, itemHeight - 5);
        if (IsPointInRect(x, y, itemRect))
        {
            data->hoveredPlugin = i;
            return;
        }
        itemY += itemHeight;
    }

    int buttonWidth = 100;
    int buttonHeight = 30;
    int buttonY = windowHeight - margin - buttonHeight - 10;
    int buttonSpacing = 10;

    Rect reloadButtonRect(margin, buttonY, buttonWidth, buttonHeight);
    if (IsPointInRect(x, y, reloadButtonRect))
    {
        data->hoveredWidget = 0;
        return;
    }

    Rect settingsButtonRect(margin + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);
    if (IsPointInRect(x, y, settingsButtonRect) && data->selectedPlugin >= 0)
    {
        data->hoveredWidget = 1;
        return;
    }

    Rect okButtonRect(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, buttonHeight);
    if (IsPointInRect(x, y, okButtonRect))
    {
        data->hoveredWidget = 2;
        return;
    }

    Rect cancelButtonRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
    if (IsPointInRect(x, y, cancelButtonRect))
    {
        data->hoveredWidget = 3;
    }
}

void HandlePluginClick(PluginManagerData *data, int x, int y, int windowWidth, int windowHeight)
{
    if (data->hoveredPlugin >= 0)
    {
        data->selectedPlugin = data->hoveredPlugin;

        int margin = 20;
        int listY = margin + 35;
        int itemHeight = 60;
        int itemY = listY + 10 + (data->hoveredPlugin - data->scrollOffset) * itemHeight;

        Rect checkboxRect(margin + 20, itemY + 20, 18, 18);
        if (IsPointInRect(x, y, checkboxRect))
        {
            data->plugins[data->hoveredPlugin]->enabled =
                !data->plugins[data->hoveredPlugin]->enabled;
            
            if (data->plugins[data->hoveredPlugin]->enabled && 
                data->plugins[data->hoveredPlugin]->canDisassemble)
            {
                extern HexData g_HexData;
                g_HexData.setDisassemblyPlugin(data->plugins[data->hoveredPlugin]->path);
                
                #ifdef _WIN32
                MessageBoxA(NULL, 
                    "Disassembly plugin activated!\nReload your file to see disassembly.", 
                    "Plugin Activated", 
                    MB_OK | MB_ICONINFORMATION);
                #endif
            }
            else if (!data->plugins[data->hoveredPlugin]->enabled)
            {
                extern HexData g_HexData;
                g_HexData.clearDisassemblyPlugin();
            }
        }
        return;
    }

    if (data->hoveredWidget == -1)
        return;

    switch (data->hoveredWidget)
    {
    case 0:
        for (size_t i = 0; i < data->plugins.size(); i++)
        {
            PlatformFree(data->plugins[i]);
        }
        data->plugins.clear();
        PluginManager::LoadPluginsFromDirectory(data);
        data->selectedPlugin = -1;
        break;

    case 1:
        break;

    case 2:
        PluginManager::SavePluginStates(data);
        data->dialogResult = true;
        data->running = false;
        break;

    case 3:
        data->dialogResult = false;
        data->running = false;
        break;
    }
}

#ifdef _WIN32

LRESULT CALLBACK PluginWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PluginManagerData *data = g_pluginDialogData;

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
            RenderPluginManager(data, rect.right, rect.bottom);
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
            UpdatePluginHoverState(data, x, y, rect.right, rect.bottom);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        if (data)
        {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int maxScroll = (int)data->plugins.size() - 5;
            if (maxScroll < 0)
                maxScroll = 0;

            if (delta > 0)
            {
                data->scrollOffset--;
                if (data->scrollOffset < 0)
                    data->scrollOffset = 0;
            }
            else
            {
                data->scrollOffset++;
                if (data->scrollOffset > maxScroll)
                    data->scrollOffset = maxScroll;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
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

            if (data->pressedWidget == data->hoveredWidget || data->hoveredPlugin >= 0)
            {
                HandlePluginClick(data, x, y, rect.right, rect.bottom);
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
            if (rect.right > 0 && rect.bottom > 0)
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

bool PluginManager::Show(NativeWindow parent)
{
    const wchar_t *className = L"PluginManagerWindow";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = PluginWindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    UnregisterClassW(className, wc.hInstance);
    if (!RegisterClassExW(&wc))
        return false;

    PluginManagerData data = {};
    data.dialogResult = false;
    data.running = true;
    data.hoveredWidget = -1;
    data.hoveredPlugin = -1;
    data.selectedPlugin = -1;
    data.scrollOffset = 0;
    g_pluginDialogData = &data;

    int width = 600;
    int height = 500;

    RECT parentRect;
    GetWindowRect((HWND)parent, &parentRect);

    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, className, L"Plugin Manager",
                                WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height, (HWND)parent, nullptr,
                                GetModuleHandleW(NULL), nullptr);

    if (!hwnd)
    {
        g_pluginDialogData = nullptr;
        return false;
    }

    data.window = hwnd;

    BOOL dark = TRUE;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

    data.renderer = new RenderManager();
    if (!data.renderer)
    {
        DestroyWindow(hwnd);
        g_pluginDialogData = nullptr;
        return false;
    }

    if (!data.renderer->initialize(hwnd))
    {
        delete data.renderer;
        DestroyWindow(hwnd);
        g_pluginDialogData = nullptr;
        return false;
    }

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    if (clientRect.right > 0 && clientRect.bottom > 0)
    {
        data.renderer->resize(clientRect.right, clientRect.bottom);
    }

    LoadPluginsFromDirectory(&data);

    EnableWindow((HWND)parent, FALSE);
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

    for (size_t i = 0; i < data.plugins.size(); i++)
    {
        PlatformFree(data.plugins[i]);
    }

    EnableWindow((HWND)parent, TRUE);
    SetForegroundWindow((HWND)parent);
    DestroyWindow(hwnd);
    UnregisterClassW(className, GetModuleHandleW(NULL));

    g_pluginDialogData = nullptr;
    return data.dialogResult;
}

#elif defined(__linux__)

bool PluginManager::Show(NativeWindow parent)
{
    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return false;

    int screen = DefaultScreen(display);
    Window rootWindow = RootWindow(display, screen);

    PluginManagerData data = {};
    data.dialogResult = false;
    data.running = true;
    data.display = display;
    data.hoveredWidget = -1;
    data.hoveredPlugin = -1;
    data.selectedPlugin = -1;
    data.scrollOffset = 0;
    g_pluginDialogData = &data;


    int width = 600;
    int height = 500;

    Window window = XCreateSimpleWindow(display, rootWindow, 100, 100, width, height, 1,
                                        BlackPixel(display, screen), WhitePixel(display, screen));

    data.window = (NativeWindow)(uintptr_t)window;

    XStoreName(display, window, "Plugin Manager");

    Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &wmDelete, 1);
    data.wmDeleteWindow = wmDelete;

    XSelectInput(display, window, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

    XMapWindow(display, window);
    XFlush(display);

    data.renderer = new RenderManager();
    if (!data.renderer || !data.renderer->initialize((NativeWindow)(uintptr_t)window))
    {
        if (data.renderer)
            delete data.renderer;
        XDestroyWindow(display, window);
        XCloseDisplay(display);
        g_pluginDialogData = nullptr;
        return false;
    }

    data.renderer->resize(width, height);
    LoadPluginsFromDirectory(&data);
    XEvent event;
    while (data.running)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            data.renderer->beginFrame();
            RenderPluginManager(&data, width, height);
            GC gc = XCreateGC(display, window, 0, nullptr);
            data.renderer->endFrame(gc);
            XFreeGC(display, gc);
        }
        else if (event.type == MotionNotify)
        {
            data.mouseX = event.xmotion.x;
            data.mouseY = event.xmotion.y;
            UpdatePluginHoverState(&data, event.xmotion.x, event.xmotion.y, width, height);

            data.renderer->beginFrame();
            RenderPluginManager(&data, width, height);
            GC gc = XCreateGC(display, window, 0, nullptr);
            data.renderer->endFrame(gc);
            XFreeGC(display, gc);
        }
        else if (event.type == ButtonPress)
        {
            if (event.xbutton.button == Button1)
            {
                data.mouseDown = true;
                data.pressedWidget = data.hoveredWidget;
            }
            else if (event.xbutton.button == Button4 || event.xbutton.button == Button5)
            {
                int maxScroll = (int)data.plugins.size() - 5;
                if (maxScroll < 0)
                    maxScroll = 0;

                if (event.xbutton.button == Button4)
                {
                    data.scrollOffset--;
                    if (data.scrollOffset < 0)
                        data.scrollOffset = 0;
                }
                else
                {
                    data.scrollOffset++;
                    if (data.scrollOffset > maxScroll)
                        data.scrollOffset = maxScroll;
                }
            }

            data.renderer->beginFrame();
            RenderPluginManager(&data, width, height);
            GC gc = XCreateGC(display, window, 0, nullptr);
            data.renderer->endFrame(gc);
            XFreeGC(display, gc);
        }
        else if (event.type == ButtonRelease && event.xbutton.button == Button1)
        {
            data.mouseDown = false;
            if (data.pressedWidget == data.hoveredWidget || data.hoveredPlugin >= 0)
            {
                HandlePluginClick(&data, event.xbutton.x, event.xbutton.y, width, height);
            }
            data.pressedWidget = -1;

            data.renderer->beginFrame();
            RenderPluginManager(&data, width, height);
            GC gc = XCreateGC(display, window, 0, nullptr);
            data.renderer->endFrame(gc);
            XFreeGC(display, gc);
        }
        else if (event.type == ClientMessage)
        {
            if ((Atom)event.xclient.data.l[0] == data.wmDeleteWindow)
            {
                data.dialogResult = false;
                data.running = false;
            }
        }
    }

    if (data.renderer)
        delete data.renderer;

    for (size_t i = 0; i < data.plugins.size(); i++)
    {
        PlatformFree(data.plugins[i]);
    }

    XDestroyWindow(display, window);
    XCloseDisplay(display);
    g_pluginDialogData = nullptr;

    return data.dialogResult;
}

#elif defined(__APPLE__)

bool PluginManager::Show(NativeWindow parent)
{
    (void)parent;
    return false;
}

#endif
