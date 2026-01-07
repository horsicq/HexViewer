#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#elif defined(__linux__)
#else
#error "Unsupported platform"
#endif

typedef unsigned long long size_t_custom;

#include "render.h"
#include "options.h"
#include "menu.h"
#include "hexdata.h"
#include "global.h"
#include "darkmode.h"
#include "panelcontent.h"
#include "about.h"
#include "pluginmanager.h"

void SaveOptionsToFile(const AppOptions &opts);
void LoadOptionsFromFile(AppOptions &opts);
void OnFileSaveAs();
void LinuxRedraw();

#if defined(_WIN32)

extern "C" void *__cdecl memset(void *dest, int c, size_t count);
#pragma function(memset)
extern "C" void *__cdecl memset(void *dest, int c, size_t count)
{
    char *bytes = (char *)dest;
    while (count--)
    {
        *bytes++ = (char)c;
    }
    return dest;
}

extern "C" void *__cdecl memcpy(void *dest, const void *src, size_t count);
#pragma function(memcpy)
extern "C" void *__cdecl memcpy(void *dest, const void *src, size_t count)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    while (count--)
    {
        *d++ = *s++;
    }
    return dest;
}

extern "C" int _fltused = 0;

void *__cdecl operator new(size_t_custom n)
{
    return HeapAlloc(GetProcessHeap(), 0, n);
}
void __cdecl operator delete(void *p)
{
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}
void *__cdecl operator new[](size_t_custom n)
{
    return HeapAlloc(GetProcessHeap(), 0, n);
}
void __cdecl operator delete[](void *p)
{
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}
void __cdecl operator delete(void *p, size_t_custom)
{
    if (p)
        HeapFree(GetProcessHeap(), 0, p);
}

extern "C" void __chkstk() {}
extern "C" int __cdecl atexit(void(__cdecl *)(void)) { return 0; }

typedef void(__cdecl *_PVFV)(void);
#pragma section(".CRT$XCA", long, read)
#pragma section(".CRT$XCZ", long, read)
__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = {0};
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = {0};

void RunConstructors()
{
    for (_PVFV *pf = __xc_a; pf < __xc_z; pf++)
    {
        if (*pf)
            (*pf)();
    }
}

#elif defined(__APPLE__) || defined(__linux__)

#include <stdlib.h>
#include <string.h>

void *operator new(std::size_t n)
{
    return malloc(n);
}

void operator delete(void *p) noexcept
{
    free(p);
}

void *operator new[](std::size_t n)
{
    return malloc(n);
}

void operator delete[](void *p) noexcept
{
    free(p);
}

void operator delete(void *p, std::size_t) noexcept
{
    free(p);
}

#endif

int g_SearchCaretX = 0;
int g_SearchCaretY = 0;
int g_SearchBoxXStart = 0;
bool caretVisible = true;
ScrollbarState g_MainScrollbar;
SelectionState g_Selection;
HexData g_HexData;
size_t editingOffset = (size_t)-1;
int g_ScrollY = 0;
int maxScrolls;
int g_LinesPerPage = 0;
int g_TotalLines = 0;
AppOptions g_Options;
MenuBar g_MenuBar;
bool darkmode = true;
#if defined(_WIN32)
HWND g_Hwnd = nullptr;
#define MAX_PATH_LEN MAX_PATH
#else
void *g_Hwnd = nullptr;
#define MAX_PATH_LEN 4096
#endif

RenderManager g_Renderer;
long long cursorBytePos = -1;
int cursorNibblePos = 0;
long long selectionLength = 0;
char g_CurrentFilePath[MAX_PATH_LEN] = {0};
LeftPanelState g_LeftPanel;
BottomPanelState g_BottomPanel;
ChecksumResults g_Checksums;

bool g_ResizingLeftPanel = false;
bool g_ResizingBottomPanel = false;
int g_ResizeStartX = 0;
int g_ResizeStartY = 0;
int g_ResizeStartWidth = 0;
int g_ResizeStartHeight = 0;

#if defined(_WIN32)
char *GetFileNameFromCmdLine()
{
    char *cmd = GetCommandLineA();
    bool in_quote = false;
    while (*cmd)
    {
        if (*cmd == '"')
            in_quote = !in_quote;
        else if (*cmd == ' ' && !in_quote)
        {
            cmd++;
            break;
        }
        cmd++;
    }
    while (*cmd == ' ')
        cmd++;
    if (*cmd == 0)
        return 0;
    if (*cmd == '"')
    {
        cmd++;
        char *end = cmd;
        while (*end && *end != '"')
            end++;
        *end = 0;
    }
    return cmd;
}

#elif defined(__APPLE__) || defined(__linux__)
static char *g_CommandLineFile = nullptr;

void SetCommandLineFile(char *filename)
{
    g_CommandLineFile = filename;
}

char *GetFileNameFromCmdLine()
{
    return g_CommandLineFile;
}

#endif

void CopyString(char *dest, const char *src, int maxLen)
{
    int i = 0;
    while (src[i] && i < maxLen - 1)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void OnNew()
{
    g_HexData.clear();
    g_CurrentFilePath[0] = '\0';
    g_ScrollY = 0;
    g_TotalLines = 0;
#if defined(_WIN32)
    InvalidateRect(g_Hwnd, NULL, FALSE);
#else
#endif
}

void OnFileOpen()
{
#if defined(_WIN32)
    if (!g_Hwnd)
        return;

    OPENFILENAMEA ofn;
    char szFile[260];
    memset(&ofn, 0, sizeof(ofn));
    memset(szFile, 0, sizeof(szFile));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_Hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter =
        "All Files (*.*)\0*.*\0"
        "Binary Files (*.bin)\0*.BIN\0"
        "Executable Files (*.exe)\0*.EXE\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
    {
        if (g_HexData.loadFile(ofn.lpstrFile))
        {
            StrCopy(g_CurrentFilePath, ofn.lpstrFile);
            g_TotalLines = (int)g_HexData.getHexLines().count;
            g_ScrollY = 0;
            RECT rc;
            GetClientRect(g_Hwnd, &rc);
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;

            g_Renderer.resize(w, h);

            g_LinesPerPage = (h - g_MenuBar.getHeight() - 40) / 16;
            if (g_LinesPerPage < 1)
                g_LinesPerPage = 1;

            InvalidateRect(g_Hwnd, 0, FALSE);
        }
        else
        {
            MessageBoxA(g_Hwnd, "Failed to open file.", "Error", MB_OK | MB_ICONERROR);
        }
    }

#elif defined(__APPLE__)
    @autoreleasepool
    {
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];

        if ([panel runModal] == NSModalResponseOK)
        {
            NSString *path = [[[panel URLs] objectAtIndex:0] path];
            const char *cpath = [path UTF8String];

            if (g_HexData.loadFile(cpath))
            {
                StrCopy(g_CurrentFilePath, cpath);
                g_TotalLines = (int)g_HexData.getHexLines().count;
                g_ScrollY = 0;
                g_Renderer.resize(g_Renderer.getWindowWidth(), g_Renderer.getWindowHeight());
            }
        }
    }

#elif defined(__linux__)
    FILE *fp = popen("zenity --file-selection 2>/dev/null", "r");
    if (!fp)
    {
        printf("Failed to open file dialog.\n");
        return;
    }

    char path[512] = {0};
    if (!fgets(path, sizeof(path), fp))
    {
        pclose(fp);
        printf("No file selected.\n");
        return;
    }
    pclose(fp);

    size_t len = StrLen(path);
    if (len > 0 && path[len - 1] == '\n')
        path[len - 1] = 0;

    if (g_HexData.loadFile(path))
    {
        StrCopy(g_CurrentFilePath, path);
        g_TotalLines = (int)g_HexData.getHexLines().count;
        g_ScrollY = 0;
        LinuxRedraw();
    }
    else
    {
        printf("Failed to open file: %s\n", path);
    }
#endif
}

void OnFileSave()
{
    if (g_CurrentFilePath[0] == '\0')
    {
        OnFileSaveAs();
        return;
    }

    if (g_HexData.saveFile(g_CurrentFilePath))
    {
#if defined(_WIN32)
        MessageBoxA(g_Hwnd, "File saved successfully.", "Info", MB_OK | MB_ICONINFORMATION);
#else
#endif
    }
    else
    {
#if defined(_WIN32)
        MessageBoxA(g_Hwnd, "Failed to save file.", "Error", MB_OK | MB_ICONERROR);
#else
#endif
    }
}

void OnFileSaveAs()
{
#if defined(_WIN32)
    if (!g_Hwnd)
        return;

    char szFile[260];
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    memset(szFile, 0, sizeof(szFile));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_Hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0Binary Files (*.bin)\0*.BIN\0";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn))
    {
        if (g_HexData.saveFile(ofn.lpstrFile))
        {
            CopyString(g_CurrentFilePath, ofn.lpstrFile, MAX_PATH_LEN);
            MessageBoxA(g_Hwnd, "File saved successfully.", "Info", MB_OK | MB_ICONINFORMATION);
        }
        else
        {
            MessageBoxA(g_Hwnd, "Failed to write file.", "Error", MB_OK | MB_ICONERROR);
        }
    }
#else
#endif
}

void OnFileExit()
{
#if defined(_WIN32)
    PostQuitMessage(0);
#else
#endif
}

void OnOptionsDialog()
{
#if defined(_WIN32)
    if (g_Hwnd && OptionsDialog::Show(g_Hwnd, g_Options))
    {
        SaveOptionsToFile(g_Options);
        InvalidateRect(g_Hwnd, 0, FALSE);
    }
#else
    if (OptionsDialog::Show((NativeWindow)(uintptr_t)g_Hwnd, g_Options))
    {
        SaveOptionsToFile(g_Options);
        LinuxRedraw();
    }
#endif
}

void OnPluginsDialog()
{
#if defined(_WIN32)
    if (g_Hwnd && PluginManager::Show(g_Hwnd))
    {
        InvalidateRect(g_Hwnd, 0, FALSE);
    }
#else
    if (PluginManager::Show((NativeWindow)(uintptr_t)g_Hwnd))
    {
        LinuxRedraw();
    }

#endif
}

void OnHelp() {}

#if defined(_WIN32)
void OnCopy() { MessageBoxA(g_Hwnd, "Copy not implemented.", "Info", MB_OK); }
void OnPaste() { MessageBoxA(g_Hwnd, "Paste not implemented.", "Info", MB_OK); }
void OnFind() { MessageBoxA(g_Hwnd, "Find not implemented.", "Info", MB_OK); }
#else
void OnCopy() {}
void OnPaste() {}
void OnFind() {}
#endif

void OnFindReplace()
{
#if defined(_WIN32)
    SearchDialogs::ShowFindReplaceDialog(
        g_Hwnd,
        g_Options.darkMode,
        [](const char *find, const char *replace)
        {
            char buf[512];
            buf[0] = '\0';

            CopyString(buf, "Find: ", sizeof(buf));
            int len = StrLen(buf);

            CopyString(buf + len, find, sizeof(buf) - len);
            len = StrLen(buf);

            CopyString(buf + len, "\nReplace: ", sizeof(buf) - len);
            len = StrLen(buf);

            CopyString(buf + len, replace, sizeof(buf) - len);

            MessageBoxA(g_Hwnd, buf, "Find & Replace", MB_OK);
        },
        nullptr);
#else
    SearchDialogs::ShowFindReplaceDialog(
        g_Hwnd,
        g_Options.darkMode,
        [](const std::string &find, const std::string &replace)
        {
            printf("Find: %s\nReplace: %s\n", find.c_str(), replace.c_str());
        });
    LinuxRedraw();
#endif
}
void OnGoTo()
{
#if defined(_WIN32)
    SearchDialogs::ShowGoToDialog(
        g_Hwnd,
        g_Options.darkMode,
        [](int line)
        {
            char num[32];
            IntToString(line, num, sizeof(num));

            char buf[128];
            CopyString(buf, "Go to line: ", sizeof(buf));
            CopyString(buf + StrLen(buf), num, sizeof(buf) - StrLen(buf));

            MessageBoxA(g_Hwnd, buf, "Go To", MB_OK);
        },
        nullptr);
#else
    SearchDialogs::ShowGoToDialog(
        g_Hwnd,
        g_Options.darkMode,
        [](int line)
        {
            printf("Go to line: %d\n", line);
        });
    LinuxRedraw();
#endif
}

void OnToggleDarkMode()
{
    g_Options.darkMode = !g_Options.darkMode;
    SaveOptionsToFile(g_Options);
#if defined(_WIN32)
    InvalidateRect(g_Hwnd, NULL, FALSE);
#else
#endif
}

#if defined(_WIN32)
void OnZoomIn() { MessageBoxA(g_Hwnd, "Zoom In not implemented.", "Info", MB_OK); }
void OnZoomOut() { MessageBoxA(g_Hwnd, "Zoom Out not implemented.", "Info", MB_OK); }
void OnAbout()
{
#ifdef _WIN32
    AboutDialog::Show(g_Hwnd, darkmode);
#elif defined(__APPLE__)
    AboutDialog::Show(g_nsWindow, g_darkMode);
#elif defined(__linux__)
    AboutDialog::Show(g_window, g_darkMode);
#endif
}
void OnDocumentation()
{
    MessageBoxA(g_Hwnd, "Documentation not available.", "Info", MB_OK);
}
#else
void OnZoomIn() {}
void OnZoomOut() {}
void OnAbout() {}
void OnDocumentation() {}
#endif

#if defined(_WIN32)

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
    {
        if (wParam == 1)
        {
            caretVisible = !caretVisible;
            if (g_PatternSearch.hasFocus || cursorBytePos >= 0)
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        return 0;
    }
    case WM_CREATE:
    {
        g_Renderer.initialize(hwnd);
        g_Hwnd = hwnd;
        ApplyDarkTitleBar(g_Hwnd, g_Options.darkMode);
        ShowScrollBar(hwnd, SB_VERT, FALSE);
        ShowScrollBar(hwnd, SB_HORZ, FALSE);
        cursorBytePos = -1;
        caretVisible = true;
        InvalidateRect(hwnd, NULL, FALSE);

        g_LeftPanel.visible = true;
        g_LeftPanel.width = 280;
        g_BottomPanel.visible = true;
        g_BottomPanel.height = 250;

        RECT rect;
        GetClientRect(hwnd, &rect);

        int w = rect.right;
        int h = rect.bottom;
        if (w < 800)
            w = 800;
        if (h < 600)
            h = 600;

        g_Renderer.resize(w, h);

        g_MenuBar.setPosition(0, 0);
        SetTimer(hwnd, 1, 500, nullptr);
        char pluginDir[512];
        GetPluginDirectory(pluginDir, 512);

        char configPath[600];
        StrCopy(configPath, pluginDir);
        int configLen = (int)StrLen(configPath);
        StrCopy(configPath + configLen, "\\enabled_plugins.txt");

        HANDLE hFile = CreateFileA(configPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            char buf[2048];
            DWORD bytesRead = 0;
            if (ReadFile(hFile, buf, sizeof(buf) - 1, &bytesRead, NULL))
            {
                buf[bytesRead] = '\0';

                int lineStart = 0;
                bool foundDisassembler = false;

                for (int i = 0; i <= (int)bytesRead && !foundDisassembler; i++)
                {
                    if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0')
                    {
                        if (i > lineStart)
                        {
                            buf[i] = '\0';
                            char *pluginName = buf + lineStart;

                            while (*pluginName == ' ' || *pluginName == '\t')
                                pluginName++;

                            if (*pluginName != '\0')
                            {
                                char fullPluginPath[600];
                                StrCopy(fullPluginPath, pluginDir);
                                int pathLen = (int)StrLen(fullPluginPath);
                                fullPluginPath[pathLen] = '\\';
                                StrCopy(fullPluginPath + pathLen + 1, pluginName);

                                extern bool CanPluginDisassemble(const char *);
                                if (CanPluginDisassemble(fullPluginPath))
                                {
                                    g_HexData.setDisassemblyPlugin(fullPluginPath);
                                    foundDisassembler = true;

                                    char msg[600];
                                    StrCopy(msg, "Auto-activated plugin: ");
                                    StrCat(msg, pluginName);
                                }
                            }
                        }
                        lineStart = i + 1;
                    }
                }
            }
            CloseHandle(hFile);
        }
        int leftPanelWidth = g_LeftPanel.visible ? g_LeftPanel.width : 0;
        g_Renderer.UpdateHexMetrics(leftPanelWidth, g_MenuBar.getHeight());
        return 0;
    }

    case WM_SIZE:
    {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);

        if (w < 800)
            w = 800;
        if (h < 600)
            h = 600;

        g_Renderer.resize(w, h);

        g_LinesPerPage = (h - g_MenuBar.getHeight() - 40) / 16;
        if (g_LinesPerPage < 1)
            g_LinesPerPage = 1;

        g_MenuBar.closeAllMenus();

        InvalidateRect(hwnd, NULL, FALSE);

        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int lines = delta / WHEEL_DELTA;
        int oldY = g_ScrollY;
        g_ScrollY -= lines * 3;

        int maxScroll = g_TotalLines - g_LinesPerPage;
        if (maxScroll < 0)
            maxScroll = 0;

        if (g_ScrollY < 0)
            g_ScrollY = 0;
        if (g_ScrollY > maxScroll)
            g_ScrollY = maxScroll;

        if (maxScroll > 0)
        {
            g_MainScrollbar.position = (float)g_ScrollY / (float)maxScroll;

            if (g_MainScrollbar.position < 0.0f)
                g_MainScrollbar.position = 0.0f;
            if (g_MainScrollbar.position > 1.0f)
                g_MainScrollbar.position = 1.0f;
        }

        if (g_ScrollY != oldY)
        {
            InvalidateRect(hwnd, 0, FALSE);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        RECT rect;
        GetClientRect(hwnd, &rect);
        int windowWidth = rect.right;
        int windowHeight = rect.bottom;

        static bool g_DebugScrollbarMsgShown = false;

        if (g_MainScrollbar.pressed)
        {
            int maxScroll = g_TotalLines - g_LinesPerPage;
            if (maxScroll < 0)
                maxScroll = 0;

            int desiredThumbTop = y - g_MainScrollbar.dragOffsetY;
            int relativeTop = desiredThumbTop - g_MainScrollbar.trackY - 2;
            int maxThumbTravel = g_MainScrollbar.trackHeight - g_MainScrollbar.thumbHeight - 4;

            if (maxThumbTravel > 0)
            {
                float newPos = (float)relativeTop / (float)maxThumbTravel;

                if (newPos < 0.0f)
                    newPos = 0.0f;
                if (newPos > 1.0f)
                    newPos = 1.0f;

                g_MainScrollbar.position = newPos;
                g_ScrollY = (int)(newPos * maxScroll);

                if (g_ScrollY < 0)
                    g_ScrollY = 0;
                if (g_ScrollY > maxScroll)
                    g_ScrollY = maxScroll;

                g_MainScrollbar.thumbY =
                    g_MainScrollbar.trackY + 2 + (int)(maxThumbTravel * newPos);
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_MainScrollbar.visible)
        {
            bool wasHovered = g_MainScrollbar.hovered;
            g_MainScrollbar.hovered = g_Renderer.isPointInScrollbarTrack(x, y, g_MainScrollbar);
            g_MainScrollbar.thumbHovered = g_Renderer.isPointInScrollbarThumb(x, y, g_MainScrollbar);

            if (wasHovered != g_MainScrollbar.hovered)
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }

        if (g_LeftPanel.dragging)
        {
            int newX = x - g_LeftPanel.dragOffsetX;
            int newY = y - g_LeftPanel.dragOffsetY;

            PanelDockPosition newDock = GetDockPositionFromMouse(
                x, y, windowWidth, windowHeight, g_MenuBar.getHeight());

            PanelDockPosition oldDock = g_LeftPanel.dockPosition;

            if (newDock == PanelDockPosition::Floating)
            {
                g_LeftPanel.dockPosition = PanelDockPosition::Floating;
                g_LeftPanel.floatingX = newX;
                g_LeftPanel.floatingY = newY;

                if (oldDock == PanelDockPosition::Top || oldDock == PanelDockPosition::Bottom)
                {
                    g_LeftPanel.floatingWidth = windowWidth / 2;
                    g_LeftPanel.floatingHeight = g_LeftPanel.height;
                }
                else
                {
                    g_LeftPanel.floatingWidth = g_LeftPanel.width;
                    g_LeftPanel.floatingHeight = windowHeight - g_MenuBar.getHeight();
                }
            }
            else if (newDock != oldDock)
            {
                g_LeftPanel.dockPosition = newDock;

                if (newDock == PanelDockPosition::Top || newDock == PanelDockPosition::Bottom)
                {
                    g_LeftPanel.height = 200;
                    g_LeftPanel.width = windowWidth;
                }
                else
                {
                    g_LeftPanel.width = 280;
                    g_LeftPanel.height = windowHeight - g_MenuBar.getHeight();
                }
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_BottomPanel.dragging)
        {
            int newX = x - g_BottomPanel.dragOffsetX;
            int newY = y - g_BottomPanel.dragOffsetY;

            PanelDockPosition newDock = GetDockPositionFromMouse(
                x, y, windowWidth, windowHeight, g_MenuBar.getHeight());

            PanelDockPosition oldDock = g_BottomPanel.dockPosition;

            if (newDock == PanelDockPosition::Floating)
            {
                g_BottomPanel.dockPosition = PanelDockPosition::Floating;
                g_BottomPanel.floatingX = newX;
                g_BottomPanel.floatingY = newY;

                if (oldDock == PanelDockPosition::Left || oldDock == PanelDockPosition::Right)
                {
                    g_BottomPanel.floatingWidth = g_BottomPanel.width;
                    g_BottomPanel.floatingHeight = windowHeight / 2;
                }
                else
                {
                    int leftOffset = (g_LeftPanel.visible &&
                                      g_LeftPanel.dockPosition == PanelDockPosition::Left)
                                         ? g_LeftPanel.width
                                         : 0;
                    g_BottomPanel.floatingWidth = windowWidth - leftOffset;
                    g_BottomPanel.floatingHeight = g_BottomPanel.height;
                }
            }
            else if (newDock != oldDock)
            {
                g_BottomPanel.dockPosition = newDock;

                if (newDock == PanelDockPosition::Left || newDock == PanelDockPosition::Right)
                {
                    g_BottomPanel.width = 300;
                    g_BottomPanel.height = windowHeight - g_MenuBar.getHeight();
                }
                else
                {
                    g_BottomPanel.height = 250;
                    int leftOffset = (g_LeftPanel.visible &&
                                      g_LeftPanel.dockPosition == PanelDockPosition::Left)
                                         ? g_LeftPanel.width
                                         : 0;
                    int rightOffset = (g_LeftPanel.visible &&
                                       g_LeftPanel.dockPosition == PanelDockPosition::Right)
                                          ? g_LeftPanel.width
                                          : 0;
                    g_BottomPanel.width = windowWidth - leftOffset - rightOffset;
                }
            }

            InvalidateRect(hwnd, NULL, FALSE);

            return 0;
        }

        if (g_ResizingLeftPanel)
        {
            int newWidth = g_ResizeStartWidth + (x - g_ResizeStartX);
            if (newWidth >= 200 && newWidth <= 500)
            {
                if (g_LeftPanel.dockPosition == PanelDockPosition::Left ||
                    g_LeftPanel.dockPosition == PanelDockPosition::Right ||
                    g_LeftPanel.dockPosition == PanelDockPosition::Floating)
                {
                    g_LeftPanel.width = newWidth;
                    if (g_LeftPanel.dockPosition == PanelDockPosition::Floating)
                    {
                        g_LeftPanel.floatingWidth = newWidth;
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        if (g_ResizingBottomPanel)
        {
            int newHeight = g_ResizeStartHeight - (y - g_ResizeStartY);
            if (newHeight >= 150 && newHeight <= windowHeight - 300)
            {
                if (g_BottomPanel.dockPosition == PanelDockPosition::Top ||
                    g_BottomPanel.dockPosition == PanelDockPosition::Bottom ||
                    g_BottomPanel.dockPosition == PanelDockPosition::Floating)
                {
                    g_BottomPanel.height = newHeight;
                    if (g_BottomPanel.dockPosition == PanelDockPosition::Floating)
                    {
                        g_BottomPanel.floatingHeight = newHeight;
                    }
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        if (g_Selection.dragging)
        {
            int leftPanelWidth = g_LeftPanel.visible ? g_LeftPanel.width : 0;
            if (g_Renderer.IsPointInHexArea(x, y, leftPanelWidth, g_MenuBar.getHeight(),
                                            windowWidth, windowHeight))
            {
                BytePositionInfo hoverInfo = g_Renderer.GetHexBytePositionInfo(Point(x, y));

                if (hoverInfo.Index >= 0 &&
                    hoverInfo.Index < (long long)g_HexData.getFileSize())
                {
                    g_Selection.endByte = hoverInfo.Index;
                    cursorBytePos = hoverInfo.Index;

                    long long cursorLine = hoverInfo.Index / 16;
                    if (cursorLine < g_ScrollY)
                    {
                        g_ScrollY = (int)cursorLine;
                    }
                    else if (cursorLine >= g_ScrollY + g_LinesPerPage)
                    {
                        g_ScrollY = (int)(cursorLine - g_LinesPerPage + 1);
                    }

                    InvalidateRect(hwnd, NULL, FALSE);
                }
            }
            return 0;
        }
        bool overLeftHandle = g_Renderer.isLeftPanelResizeHandle(x, y, g_LeftPanel);
        bool overBottomHandle = g_Renderer.isBottomPanelResizeHandle(x, y, g_BottomPanel,
                                                                     g_LeftPanel.visible ? g_LeftPanel.width : 0);

        if (overLeftHandle)
        {
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        }
        else if (overBottomHandle)
        {
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
        }
        else
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }

        if (g_MenuBar.handleMouseMove(x, y))
        {
            InvalidateRect(hwnd, NULL, FALSE);
        }

        return 0;
    }
    case WM_LBUTTONUP:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (g_Selection.dragging)
        {
            g_Selection.dragging = false;
            ReleaseCapture();

            if (g_Selection.startByte == g_Selection.endByte)
            {
                g_Selection.clear();
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (g_MainScrollbar.pressed)
        {
            g_MainScrollbar.pressed = false;
            g_MainScrollbar.thumbHovered = false;

            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_LeftPanel.dragging || g_BottomPanel.dragging)
        {
            g_LeftPanel.dragging = false;
            g_BottomPanel.dragging = false;

            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_ResizingLeftPanel || g_ResizingBottomPanel)
        {
            g_ResizingLeftPanel = false;
            g_ResizingBottomPanel = false;
            g_LeftPanel.resizing = false;
            g_BottomPanel.resizing = false;

            ReleaseCapture();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_MenuBar.handleMouseUp(x, y))
        {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        SetFocus(hwnd);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        RECT rect;
        GetClientRect(hwnd, &rect);
        int windowWidth = rect.right;
        int windowHeight = rect.bottom;
        int leftPanelWidth = g_LeftPanel.visible ? g_LeftPanel.width : 0;

        if (g_MainScrollbar.visible &&
            g_Renderer.isPointInScrollbarThumb(x, y, g_MainScrollbar))
        {
            g_MainScrollbar.pressed = true;
            g_MainScrollbar.thumbHovered = true;

            g_MainScrollbar.dragOffsetY = y - g_MainScrollbar.thumbY;

            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_MainScrollbar.visible && g_Renderer.isPointInScrollbarTrack(x, y, g_MainScrollbar))
        {
            float newPos = g_Renderer.getScrollbarPositionFromMouse(y, g_MainScrollbar, true);

            int maxScroll = g_TotalLines - g_LinesPerPage;
            if (maxScroll < 0)
                maxScroll = 0;

            g_ScrollY = (int)(newPos * maxScroll);

            if (g_ScrollY < 0)
                g_ScrollY = 0;
            if (g_ScrollY > maxScroll)
                g_ScrollY = maxScroll;

            g_MainScrollbar.position = newPos;

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        if (g_MenuBar.handleMouseDown(x, y))
        {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        if (g_BottomPanel.visible)
        {
            Rect bottomBounds = GetBottomPanelBounds(
                g_BottomPanel, windowWidth, windowHeight,
                g_MenuBar.getHeight(), g_LeftPanel);

            int tabHeight = 32;
            int tabStartY = bottomBounds.y + 28 + 5;

            bool isVertical =
                (g_BottomPanel.dockPosition == PanelDockPosition::Left ||
                 g_BottomPanel.dockPosition == PanelDockPosition::Right);

            BottomPanelState::Tab tabs[] = {
                BottomPanelState::Tab::EntropyAnalysis,
                BottomPanelState::Tab::PatternSearch,
                BottomPanelState::Tab::Checksum,
                BottomPanelState::Tab::Compare};

            if (isVertical)
            {
                int tabY = tabStartY;
                int tabWidth = bottomBounds.width - 10;

                for (int i = 0; i < 4; i++)
                {
                    if (x >= bottomBounds.x + 5 &&
                        x <= bottomBounds.x + 5 + tabWidth &&
                        y >= tabY &&
                        y <= tabY + tabHeight - 5)
                    {
                        g_BottomPanel.activeTab = tabs[i];
                        RECT rr = {
                            bottomBounds.x, bottomBounds.y,
                            bottomBounds.x + bottomBounds.width,
                            bottomBounds.y + bottomBounds.height};
                        InvalidateRect(hwnd, &rr, FALSE);
                        return 0;
                    }
                    tabY += tabHeight;
                }
            }
            else
            {
                if (y >= tabStartY && y <= tabStartY + tabHeight - 5)
                {
                    const char *tabLabels[] = {
                        "Entropy Analysis",
                        "Hex Pattern Search",
                        "Checksum",
                        "Compare"};

                    int tabX = bottomBounds.x + 10;

                    for (int i = 0; i < 4; i++)
                    {
                        int tabWidth = StrLen(tabLabels[i]) * 8 + 20;

                        if (x >= tabX && x <= tabX + tabWidth)
                        {
                            g_BottomPanel.activeTab = tabs[i];
                            RECT rr = {
                                bottomBounds.x, bottomBounds.y,
                                bottomBounds.x + bottomBounds.width,
                                bottomBounds.y + bottomBounds.height};
                            InvalidateRect(hwnd, &rr, FALSE);
                            return 0;
                        }

                        tabX += tabWidth + 5;
                    }
                }
            }
        }

        if (g_LeftPanel.visible)
        {
            Rect leftBounds = GetLeftPanelBounds(
                g_LeftPanel, windowWidth, windowHeight, g_MenuBar.getHeight());

            if (IsInPanelTitleBar(x, y, leftBounds))
            {
                g_LeftPanel.dragging = true;
                g_LeftPanel.dragOffsetX = x - leftBounds.x;
                g_LeftPanel.dragOffsetY = y - leftBounds.y;
                SetCapture(hwnd);
                RECT rr = {
                    leftBounds.x, leftBounds.y,
                    leftBounds.x + leftBounds.width,
                    leftBounds.y + leftBounds.height};
                InvalidateRect(hwnd, &rr, FALSE);
                return 0;
            }
        }

        if (g_BottomPanel.visible)
        {
            Rect bottomBounds = GetBottomPanelBounds(
                g_BottomPanel, windowWidth, windowHeight,
                g_MenuBar.getHeight(), g_LeftPanel);

            if (IsInPanelTitleBar(x, y, bottomBounds))
            {
                g_BottomPanel.dragging = true;
                g_BottomPanel.dragOffsetX = x - bottomBounds.x;
                g_BottomPanel.dragOffsetY = y - bottomBounds.y;
                SetCapture(hwnd);
                RECT rr = {
                    bottomBounds.x, bottomBounds.y,
                    bottomBounds.x + bottomBounds.width,
                    bottomBounds.y + bottomBounds.height};
                InvalidateRect(hwnd, &rr, FALSE);
                return 0;
            }
        }

        if (g_Renderer.isLeftPanelResizeHandle(x, y, g_LeftPanel))
        {
            g_ResizingLeftPanel = true;
            g_LeftPanel.resizing = true;
            g_ResizeStartX = x;
            g_ResizeStartWidth = g_LeftPanel.width;
            SetCapture(hwnd);
            Rect leftBounds = GetLeftPanelBounds(
                g_LeftPanel, windowWidth, windowHeight, g_MenuBar.getHeight());
            RECT rr = {
                leftBounds.x, leftBounds.y,
                leftBounds.x + leftBounds.width,
                leftBounds.y + leftBounds.height};
            InvalidateRect(hwnd, &rr, FALSE);
            return 0;
        }

        if (g_Renderer.isBottomPanelResizeHandle(
                x, y, g_BottomPanel,
                g_LeftPanel.visible ? g_LeftPanel.width : 0))
        {
            g_ResizingBottomPanel = true;
            g_BottomPanel.resizing = true;
            g_ResizeStartY = y;
            g_ResizeStartHeight = g_BottomPanel.height;
            SetCapture(hwnd);
            Rect bottomBounds = GetBottomPanelBounds(
                g_BottomPanel, windowWidth, windowHeight,
                g_MenuBar.getHeight(), g_LeftPanel);
            RECT rr = {
                bottomBounds.x, bottomBounds.y,
                bottomBounds.x + bottomBounds.width,
                bottomBounds.y + bottomBounds.height};
            InvalidateRect(hwnd, &rr, FALSE);
            return 0;
        }

        if (HandleBottomPanelContentClick(x, y, windowWidth, windowHeight))
        {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        g_Renderer.UpdateHexMetrics(leftPanelWidth, g_MenuBar.getHeight());

        if (g_Renderer.IsPointInHexArea(x, y, leftPanelWidth, g_MenuBar.getHeight(),
                                        windowWidth, windowHeight))
        {
            g_PatternSearch.hasFocus = false;

            BytePositionInfo clickInfo = g_Renderer.GetHexBytePositionInfo(Point(x, y));

            if (clickInfo.Index >= 0 &&
                clickInfo.Index < (long long)g_HexData.getFileSize())
            {
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

                if (shiftHeld && g_Selection.active)
                {
                    g_Selection.endByte = clickInfo.Index;
                }
                else
                {
                    g_Selection.startByte = clickInfo.Index;
                    g_Selection.endByte = clickInfo.Index;
                    g_Selection.active = true;
                    g_Selection.dragging = true;

                    SetCapture(hwnd);
                }

                cursorBytePos = clickInfo.Index;
                cursorNibblePos = clickInfo.CharacterPosition;
                caretVisible = true;
            }

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        g_PatternSearch.hasFocus = false;

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_CHAR:
    {
        if (g_PatternSearch.hasFocus)
        {
            char c = (char)wParam;

            if (c == 8)
            {
                size_t len = StrLen(g_PatternSearch.searchPattern);
                if (len > 0)
                {
                    g_PatternSearch.searchPattern[len - 1] = 0;

                    g_SearchCaretX -= g_Renderer.getCharWidth();
                    if (g_SearchCaretX < g_SearchBoxXStart)
                        g_SearchCaretX = g_SearchBoxXStart;
                }

                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            if (c >= 'a' && c <= 'f')
                c -= 32;

            if ((c >= '0' && c <= '9') ||
                (c >= 'A' && c <= 'F') ||
                c == ' ')
            {
                size_t len = StrLen(g_PatternSearch.searchPattern);
                if (len < (sizeof(g_PatternSearch.searchPattern) - 1))
                {
                    g_PatternSearch.searchPattern[len] = c;
                    g_PatternSearch.searchPattern[len + 1] = 0;

                    g_SearchCaretX += g_Renderer.getCharWidth();
                }

                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }

            return 0;
        }

        if (cursorBytePos >= 0 && cursorBytePos < (long long)g_HexData.getFileSize())
        {
            char c = (char)wParam;

            if (c >= 'a' && c <= 'f')
                c -= 32;

            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))
            {
                int nibbleValue = (c <= '9') ? (c - '0') : (c - 'A' + 10);

                uint8_t currentByte = g_HexData.getByte((size_t)cursorBytePos);
                uint8_t newByte;

                if (cursorNibblePos == 0)
                    newByte = (nibbleValue << 4) | (currentByte & 0x0F);
                else
                    newByte = (currentByte & 0xF0) | nibbleValue;

                g_HexData.editByte((size_t)cursorBytePos, newByte);

                if (cursorNibblePos == 0)
                {
                    cursorNibblePos = 1;
                }
                else
                {
                    if (cursorBytePos < (long long)g_HexData.getFileSize() - 1)
                    {
                        cursorBytePos++;
                        cursorNibblePos = 0;

                        long long cursorLine = cursorBytePos / 16;
                        if (cursorLine >= g_ScrollY + g_LinesPerPage)
                        {
                            g_ScrollY = (int)(cursorLine - g_LinesPerPage + 1);
                        }
                    }
                }

                caretVisible = true;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        break;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            if (g_Selection.active)
            {
                g_Selection.clear();
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
            if (g_PatternSearch.hasFocus)
            {
                g_PatternSearch.hasFocus = false;
                InvalidateRect(hwnd, NULL, FALSE);
                return 0;
            }
        }

        if (g_PatternSearch.hasFocus)
        {
            switch (wParam)
            {
            case VK_LEFT:
            case VK_RIGHT:
            case VK_UP:
            case VK_DOWN:
            case VK_DELETE:
                return 0;
            }
        }
        else
        {
            if (cursorBytePos >= 0 && g_HexData.getFileSize() > 0)
            {
                long long maxPos = (long long)g_HexData.getFileSize() - 1;
                bool moved = false;

                switch (wParam)
                {
                case VK_LEFT:
                    if (cursorNibblePos > 0)
                    {
                        cursorNibblePos = 0;
                    }
                    else if (cursorBytePos > 0)
                    {
                        cursorBytePos--;
                        cursorNibblePos = 1;
                    }
                    moved = true;
                    break;

                case VK_RIGHT:
                    if (cursorNibblePos < 1)
                    {
                        cursorNibblePos = 1;
                    }
                    else if (cursorBytePos < maxPos)
                    {
                        cursorBytePos++;
                        cursorNibblePos = 0;
                    }
                    moved = true;
                    break;

                case VK_UP:
                    if (cursorBytePos >= 16)
                    {
                        cursorBytePos -= 16;
                        moved = true;
                    }
                    break;

                case VK_DOWN:
                    if (cursorBytePos + 16 <= maxPos)
                    {
                        cursorBytePos += 16;
                        moved = true;
                    }
                    break;
                }

                if (moved)
                {
                    long long cursorLine = cursorBytePos / 16;
                    if (cursorLine < g_ScrollY)
                    {
                        g_ScrollY = (int)cursorLine;
                    }
                    else if (cursorLine >= g_ScrollY + g_LinesPerPage)
                    {
                        g_ScrollY = (int)(cursorLine - g_LinesPerPage + 1);
                    }

                    caretVisible = true;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
        }

        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (g_MenuBar.handleKeyPress((int)wParam, ctrl, shift, alt))
        {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }

        break;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        g_Renderer.beginFrame();

        g_Renderer.clear(
            g_Options.darkMode
                ? Theme::Dark().windowBackground
                : Theme::Light().windowBackground);

        RECT rect;
        GetClientRect(hwnd, &rect);
        int windowWidth = rect.right;
        int windowHeight = rect.bottom;
        int menuBarHeight = g_MenuBar.getHeight();

        Rect leftBounds = GetLeftPanelBounds(
            g_LeftPanel, windowWidth, windowHeight, menuBarHeight);

        Rect bottomBounds = GetBottomPanelBounds(
            g_BottomPanel, windowWidth, windowHeight,
            menuBarHeight, g_LeftPanel);

        Vector<char *> hexLines;
        const LineArray &lines = g_HexData.getHexLines();

        if (lines.count > 0)
        {
            for (int i = 0; i < (int)lines.count; i++)
            {
                const SimpleString *line = &lines.lines[i];
                char *buf = (char *)HeapAlloc(GetProcessHeap(), 0, line->length + 1);

                for (size_t j = 0; j < line->length; j++)
                    buf[j] = line->data[j];

                buf[line->length] = '\0';
                hexLines.push_back(buf);
            }
        }

        const SimpleString &header = g_HexData.getHeaderLine();
        const char *headerStr = header.data ? header.data : "No File Loaded";

        int effectiveWindowHeight = windowHeight;
        if (g_BottomPanel.visible && g_BottomPanel.dockPosition == PanelDockPosition::Bottom)
        {
            effectiveWindowHeight -= g_BottomPanel.height;
        }

        g_LinesPerPage = (effectiveWindowHeight - menuBarHeight - 40) / 16;
        if (g_LinesPerPage < 1)
            g_LinesPerPage = 1;
        int maxScrollPos = g_TotalLines - g_LinesPerPage;
        if (maxScrollPos < 0)
            maxScrollPos = 0;

        g_Renderer.renderHexViewer(
            hexLines,
            headerStr,
            g_ScrollY,
            maxScrollPos,
            g_MainScrollbar.hovered,
            g_MainScrollbar.pressed,
            Rect(0, 0, 0, 0),
            Rect(0, 0, 0, 0),
            g_Options.darkMode,
            -1,
            -1,
            "",
            cursorBytePos,
            cursorNibblePos,
            (long long)g_HexData.getFileSize(),
            g_LeftPanel.visible ? g_LeftPanel.width : 0,
            effectiveWindowHeight);

        for (size_t i = 0; i < hexLines.size(); i++)
            HeapFree(GetProcessHeap(), 0, hexLines[i]);

        if (g_LeftPanel.visible)
        {
            g_Renderer.drawLeftPanel(
                g_LeftPanel,
                g_Options.darkMode ? Theme::Dark() : Theme::Light(),
                windowHeight,
                leftBounds);
        }

        if (g_BottomPanel.visible)
        {
            g_Renderer.drawBottomPanel(
                g_BottomPanel,
                g_Options.darkMode ? Theme::Dark() : Theme::Light(),
                g_Checksums,
                windowWidth,
                windowHeight,
                bottomBounds);
        }

        g_MenuBar.render(&g_Renderer, windowWidth);

        g_Renderer.endFrame(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_KILLFOCUS:
    {
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        g_MainScrollbar.pressed = false;
        g_MainScrollbar.thumbHovered = false;
        g_LeftPanel.dragging = false;
        g_BottomPanel.dragging = false;
        g_ResizingLeftPanel = false;
        g_ResizingBottomPanel = false;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_DESTROY:
        if (GetCapture() == hwnd)
        {
            ReleaseCapture();
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

extern "C" void entry()
{
    RunConstructors();
    DetectNative();
    LoadOptionsFromFile(g_Options);

    g_MenuBar.setPosition(0, 0);
    g_MenuBar.addMenu(MenuHelper::createFileMenu(OnNew, OnFileOpen, OnFileSave, OnFileExit));
    g_MenuBar.addMenu(MenuHelper::createSearchMenu(OnFindReplace, OnGoTo));
    g_MenuBar.addMenu(MenuHelper::createToolsMenu(OnOptionsDialog, OnPluginsDialog));
    g_MenuBar.addMenu(MenuHelper::createHelpMenu(OnAbout, OnDocumentation));

    char *filename = GetFileNameFromCmdLine();
    if (filename)
    {
        if (g_HexData.loadFile(filename))
        {
            CopyString(g_CurrentFilePath, filename, MAX_PATH_LEN);
            g_TotalLines = (int)g_HexData.getHexLines().count;
        }
        else
        {
            MessageBoxA(0, "Failed to open file specified in command line.", "Error", MB_OK | MB_ICONERROR);
        }
    }

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleA(0);
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.lpszClassName = "HexViewClass";

    if (!RegisterClassA(&wc))
    {
        MessageBoxA(0, "RegisterClassA failed", "Error", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    HWND hwnd = CreateWindowExA(
        0,
        "HexViewClass",
        "HexViewer",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        2000, 900,
        0, 0,
        wc.hInstance,
        0);

    if (!hwnd)
    {
        MessageBoxA(0, "CreateWindowExA failed", "Error", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }
    ShowScrollBar(hwnd, SB_VERT, FALSE);
    ShowScrollBar(hwnd, SB_HORZ, FALSE);
    MSG msg;
    while (GetMessageA(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    SaveOptionsToFile(g_Options);
    ExitProcess(0);
}

#elif defined(__APPLE__)

int main(int argc, char **argv)
{
    RunConstructors();

    if (argc > 1)
    {
        SetCommandLineFile(argv[1]);
    }

    return 0;
}

#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <unistd.h>

Display *g_display = nullptr;
Window g_window = 0;
GC g_GC;
Atom g_WmDeleteWindow;

void UpdateLinuxScrollbar()
{
}

void HandleLinuxKeyPress(XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    bool ctrl = (event->state & ControlMask) != 0;
    bool shift = (event->state & ShiftMask) != 0;
    bool alt = (event->state & Mod1Mask) != 0;

    int vk = 0;
    switch (keysym)
    {
    case XK_Return:
        vk = 13;
        break;
    case XK_Escape:
        vk = 27;
        break;
    case XK_Left:
        vk = 37;
        break;
    case XK_Up:
        vk = 38;
        break;
    case XK_Right:
        vk = 39;
        break;
    case XK_Down:
        vk = 40;
        break;

    default:
        if (keysym >= XK_a && keysym <= XK_z)
            vk = 'A' + (keysym - XK_a);
        else if (keysym >= XK_A && keysym <= XK_Z)
            vk = keysym;
        break;
    }

    if (vk && g_MenuBar.handleKeyPress(vk, ctrl, shift, alt))
        return;

    if (g_PatternSearch.hasFocus)
    {
        char buf[8];
        KeySym sym;
        int len = XLookupString(event, buf, sizeof(buf), &sym, NULL);

        if (len > 0)
        {
            char ch = buf[0];

            if (ch == 8 || ch == 127)
            {
                int L = StrLen(g_PatternSearch.searchPattern);
                if (L > 0)
                    g_PatternSearch.searchPattern[L - 1] = '\0';

                LinuxRedraw();
                return;
            }

            if (ch == '\r' || ch == '\n')
            {
                PatternSearch_FindNext();
                LinuxRedraw();
                return;
            }

            if (ch >= 32 && ch < 127)
            {
                int L = StrLen(g_PatternSearch.searchPattern);
                if (L < 255)
                {
                    g_PatternSearch.searchPattern[L] = ch;
                    g_PatternSearch.searchPattern[L + 1] = '\0';
                }

                LinuxRedraw();
                return;
            }
        }
    }
}

void HandleLinuxResize(int width, int height)
{
    if (width < 800)
        width = 800;
    if (height < 600)
        height = 600;

    g_Renderer.resize(width, height);

    g_LinesPerPage = (height - g_MenuBar.getHeight() - 40) / 16;
    if (g_LinesPerPage < 1)
        g_LinesPerPage = 1;

    g_MenuBar.closeAllMenus();
}

void HandleLinuxMouseButton(XButtonEvent *event, bool pressed)
{
    int x = event->x;
    int y = event->y;

    XWindowAttributes attrs;
    XGetWindowAttributes(g_display, g_window, &attrs);
    int windowWidth = attrs.width;
    int windowHeight = attrs.height;

    if (pressed)
    {
        if (event->button == Button1)
        {
            Window root;
            int root_x, root_y, win_x, win_y;
            unsigned int mask;
            XQueryPointer(g_display, g_window, &root, &root,
                          &root_x, &root_y, &win_x, &win_y, &mask);

            if (g_MenuBar.handleMouseDown(x, y))
            {
                LinuxRedraw();
                return;
            }

            if (g_BottomPanel.visible)
            {
                Rect bottomBounds = GetBottomPanelBounds(
                    g_BottomPanel, windowWidth, windowHeight,
                    g_MenuBar.getHeight(), g_LeftPanel);

                int tabHeight = 32;
                int tabStartY = bottomBounds.y + 28 + 5;

                bool isVertical = (g_BottomPanel.dockPosition == PanelDockPosition::Left ||
                                   g_BottomPanel.dockPosition == PanelDockPosition::Right);

                BottomPanelState::Tab tabs[] = {
                    BottomPanelState::Tab::EntropyAnalysis,
                    BottomPanelState::Tab::PatternSearch,
                    BottomPanelState::Tab::Checksum,
                    BottomPanelState::Tab::Compare};

                if (isVertical)
                {
                    int tabY = tabStartY;
                    int tabWidth = bottomBounds.width - 10;

                    for (int i = 0; i < 4; i++)
                    {
                        if (x >= bottomBounds.x + 5 && x <= bottomBounds.x + 5 + tabWidth &&
                            y >= tabY && y <= tabY + tabHeight - 5)
                        {
                            g_BottomPanel.activeTab = tabs[i];
                            LinuxRedraw();
                            return;
                        }
                        tabY += tabHeight;
                    }
                }
                else
                {
                    if (y >= tabStartY && y <= tabStartY + tabHeight - 5)
                    {
                        const char *tabLabels[] = {
                            "Entropy Analysis",
                            "Hex Pattern Search",
                            "Checksum",
                            "Compare"};

                        int tabX = bottomBounds.x + 10;

                        for (int i = 0; i < 4; i++)
                        {
                            int tabWidth = StrLen(tabLabels[i]) * 8 + 20;

                            if (x >= tabX && x <= tabX + tabWidth)
                            {
                                g_BottomPanel.activeTab = tabs[i];
                                LinuxRedraw();
                                return;
                            }

                            tabX += tabWidth + 5;
                        }
                    }
                }

                if (IsInPanelTitleBar(x, y, bottomBounds))
                {
                    g_BottomPanel.dragging = true;
                    g_BottomPanel.dragOffsetX = x - bottomBounds.x;
                    g_BottomPanel.dragOffsetY = y - bottomBounds.y;
                    return;
                }

                if (HandleBottomPanelContentClick(x, y, windowWidth, windowHeight))
                {
                    LinuxRedraw();
                    return;
                }
            }

            if (g_LeftPanel.visible)
            {
                Rect leftBounds = GetLeftPanelBounds(
                    g_LeftPanel, windowWidth, windowHeight, g_MenuBar.getHeight());

                if (IsInPanelTitleBar(x, y, leftBounds))
                {
                    g_LeftPanel.dragging = true;
                    g_LeftPanel.dragOffsetX = x - leftBounds.x;
                    g_LeftPanel.dragOffsetY = y - leftBounds.y;
                    return;
                }
            }
        }
        else if (event->button == Button4)
        {
            g_ScrollY -= 3;
            if (g_ScrollY < 0)
                g_ScrollY = 0;
            LinuxRedraw();
        }
        else if (event->button == Button5)
        {
            g_ScrollY += 3;
            if (g_ScrollY > g_TotalLines - 1)
                g_ScrollY = g_TotalLines - 1;
            LinuxRedraw();
        }
    }
    else
    {
        if (event->button == Button1)
        {
            if (g_LeftPanel.dragging || g_BottomPanel.dragging)
            {
                g_LeftPanel.dragging = false;
                g_BottomPanel.dragging = false;
                LinuxRedraw();
                return;
            }

            if (g_ResizingLeftPanel || g_ResizingBottomPanel)
            {
                g_ResizingLeftPanel = false;
                g_ResizingBottomPanel = false;
                g_LeftPanel.resizing = false;
                g_BottomPanel.resizing = false;
                LinuxRedraw();
                return;
            }

            if (g_MenuBar.handleMouseUp(x, y))
            {
                LinuxRedraw();
                return;
            }
        }
    }
}

void HandleLinuxMouseMotion(XMotionEvent *event)
{
    int x = event->x;
    int y = event->y;

    XWindowAttributes attrs;
    XGetWindowAttributes(g_display, g_window, &attrs);
    int windowWidth = attrs.width;
    int windowHeight = attrs.height;

    if (g_LeftPanel.dragging)
    {
        int newX = x - g_LeftPanel.dragOffsetX;
        int newY = y - g_LeftPanel.dragOffsetY;

        PanelDockPosition newDock = GetDockPositionFromMouse(
            x, y, windowWidth, windowHeight, g_MenuBar.getHeight());

        PanelDockPosition oldDock = g_LeftPanel.dockPosition;

        if (newDock == PanelDockPosition::Floating)
        {
            g_LeftPanel.dockPosition = PanelDockPosition::Floating;
            g_LeftPanel.floatingX = newX;
            g_LeftPanel.floatingY = newY;

            if (oldDock == PanelDockPosition::Top || oldDock == PanelDockPosition::Bottom)
            {
                g_LeftPanel.floatingWidth = windowWidth / 2;
                g_LeftPanel.floatingHeight = g_LeftPanel.height;
            }
            else
            {
                g_LeftPanel.floatingWidth = g_LeftPanel.width;
                g_LeftPanel.floatingHeight = windowHeight - g_MenuBar.getHeight();
            }
        }
        else if (newDock != oldDock)
        {
            g_LeftPanel.dockPosition = newDock;

            if (newDock == PanelDockPosition::Top || newDock == PanelDockPosition::Bottom)
            {
                g_LeftPanel.height = 200;
                g_LeftPanel.width = windowWidth;
            }
            else
            {
                g_LeftPanel.width = 280;
                g_LeftPanel.height = windowHeight - g_MenuBar.getHeight();
            }
        }

        return;
    }

    if (g_BottomPanel.dragging)
    {
        int newX = x - g_BottomPanel.dragOffsetX;
        int newY = y - g_BottomPanel.dragOffsetY;

        PanelDockPosition newDock = GetDockPositionFromMouse(
            x, y, windowWidth, windowHeight, g_MenuBar.getHeight());

        PanelDockPosition oldDock = g_BottomPanel.dockPosition;

        if (newDock == PanelDockPosition::Floating)
        {
            g_BottomPanel.dockPosition = PanelDockPosition::Floating;
            g_BottomPanel.floatingX = newX;
            g_BottomPanel.floatingY = newY;

            if (oldDock == PanelDockPosition::Left || oldDock == PanelDockPosition::Right)
            {
                g_BottomPanel.floatingWidth = g_BottomPanel.width;
                g_BottomPanel.floatingHeight = windowHeight / 2;
            }
            else
            {
                int leftOffset = (g_LeftPanel.visible &&
                                  g_LeftPanel.dockPosition == PanelDockPosition::Left)
                                     ? g_LeftPanel.width
                                     : 0;
                g_BottomPanel.floatingWidth = windowWidth - leftOffset;
                g_BottomPanel.floatingHeight = g_BottomPanel.height;
            }
        }
        else if (newDock != oldDock)
        {
            g_BottomPanel.dockPosition = newDock;

            if (newDock == PanelDockPosition::Left || newDock == PanelDockPosition::Right)
            {
                g_BottomPanel.width = 300;
                g_BottomPanel.height = windowHeight - g_MenuBar.getHeight();
            }
            else
            {
                g_BottomPanel.height = 250;
                int leftOffset = (g_LeftPanel.visible &&
                                  g_LeftPanel.dockPosition == PanelDockPosition::Left)
                                     ? g_LeftPanel.width
                                     : 0;
                int rightOffset = (g_LeftPanel.visible &&
                                   g_LeftPanel.dockPosition == PanelDockPosition::Right)
                                      ? g_LeftPanel.width
                                      : 0;
                g_BottomPanel.width = windowWidth - leftOffset - rightOffset;
            }
        }

        return;
    }

    if (g_ResizingLeftPanel)
    {
        int newWidth = g_ResizeStartWidth + (x - g_ResizeStartX);
        if (newWidth >= 200 && newWidth <= 500)
        {
            if (g_LeftPanel.dockPosition == PanelDockPosition::Left ||
                g_LeftPanel.dockPosition == PanelDockPosition::Right ||
                g_LeftPanel.dockPosition == PanelDockPosition::Floating)
            {
                g_LeftPanel.width = newWidth;
                if (g_LeftPanel.dockPosition == PanelDockPosition::Floating)
                {
                    g_LeftPanel.floatingWidth = newWidth;
                }
            }
        }
        return;
    }

    if (g_ResizingBottomPanel)
    {
        int newHeight = g_ResizeStartHeight - (y - g_ResizeStartY);
        if (newHeight >= 150 && newHeight <= windowHeight - 300)
        {
            if (g_BottomPanel.dockPosition == PanelDockPosition::Top ||
                g_BottomPanel.dockPosition == PanelDockPosition::Bottom ||
                g_BottomPanel.dockPosition == PanelDockPosition::Floating)
            {
                g_BottomPanel.height = newHeight;
                if (g_BottomPanel.dockPosition == PanelDockPosition::Floating)
                {
                    g_BottomPanel.floatingHeight = newHeight;
                }
            }
        }
        return;
    }

    if (g_MenuBar.handleMouseMove(x, y))
    {
    }
}

void LinuxRedraw()
{
    XWindowAttributes attrs;
    XGetWindowAttributes(g_display, g_window, &attrs);
    int windowWidth = attrs.width;
    int windowHeight = attrs.height;
    int menuBarHeight = g_MenuBar.getHeight();

    g_Renderer.beginFrame();

    g_Renderer.clear(
        g_Options.darkMode
            ? Theme::Dark().windowBackground
            : Theme::Light().windowBackground);

    Rect leftBounds = GetLeftPanelBounds(
        g_LeftPanel, windowWidth, windowHeight, menuBarHeight);

    Rect bottomBounds = GetBottomPanelBounds(
        g_BottomPanel, windowWidth, windowHeight,
        menuBarHeight, g_LeftPanel);

    Vector<char *> hexLines;
    const LineArray &lines = g_HexData.getHexLines();

    if (lines.count > 0)
    {
        int startLine = g_ScrollY;
        int endLine = startLine + g_LinesPerPage + 1;
        if (endLine > (int)lines.count)
            endLine = (int)lines.count;

        for (int i = startLine; i < endLine; i++)
        {
            const SimpleString *line = &lines.lines[i];
            char *buf = (char *)malloc(line->length + 1);

            for (size_t j = 0; j < line->length; j++)
                buf[j] = line->data[j];

            buf[line->length] = '\0';
            hexLines.push_back(buf);
        }
    }

    const SimpleString &header = g_HexData.getHeaderLine();
    const char *headerStr = header.data ? header.data : "No File Loaded";

    g_Renderer.renderHexViewer(
        hexLines,
        headerStr,
        g_ScrollY,
        g_TotalLines,
        false,
        false,
        Rect(0, 0, 0, 0),
        Rect(0, 0, 0, 0),
        g_Options.darkMode,
        -1,
        -1,
        "",
        0LL,
        0,
        (long long)g_HexData.getFileSize(),
        g_LeftPanel.visible ? g_LeftPanel.width : 0);

    for (size_t i = 0; i < hexLines.size(); i++)
        free(hexLines[i]);

    if (g_LeftPanel.visible)
    {
        g_Renderer.drawLeftPanel(
            g_LeftPanel,
            g_Options.darkMode ? Theme::Dark() : Theme::Light(),
            windowHeight,
            leftBounds);
    }

    if (g_BottomPanel.visible)
    {
        g_Renderer.drawBottomPanel(
            g_BottomPanel,
            g_Options.darkMode ? Theme::Dark() : Theme::Light(),
            g_Checksums,
            windowWidth,
            windowHeight,
            bottomBounds);
    }

    g_MenuBar.render(&g_Renderer, windowWidth);

    g_Renderer.endFrame(g_GC);
}

int main(int argc, char **argv)
{
    DetectNative();
    LoadOptionsFromFile(g_Options);

    if (argc > 1)
    {
        SetCommandLineFile(argv[1]);
    }

    g_display = XOpenDisplay(nullptr);
    if (!g_display)
    {
        fprintf(stderr, "Cannot open X display\n");
        return 1;
    }

    int screen = DefaultScreen(g_display);
    Window root = RootWindow(g_display, screen);

    XSetWindowAttributes attrs;
    attrs.background_pixel = WhitePixel(g_display, screen);
    attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
                       ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;

    g_window = XCreateWindow(
        g_display, root,
        100, 100, 800, 600, 0,
        DefaultDepth(g_display, screen),
        InputOutput,
        DefaultVisual(g_display, screen),
        CWBackPixel | CWEventMask,
        &attrs);

    XStoreName(g_display, g_window, "NO-CRT Hex Viewer");

    g_WmDeleteWindow = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(g_display, g_window, &g_WmDeleteWindow, 1);

    g_GC = XCreateGC(g_display, g_window, 0, nullptr);

    XMapWindow(g_display, g_window);

    g_Renderer.initialize((NativeWindow)(uintptr_t)g_window);

    g_LeftPanel.visible = true;
    g_LeftPanel.width = 280;
    g_BottomPanel.visible = true;
    g_BottomPanel.height = 250;
    g_Renderer.resize(800, 600);
    g_MenuBar.setPosition(0, 0);

    g_MenuBar.addMenu(MenuHelper::createFileMenu(OnNew, OnFileOpen, OnFileSave, OnFileExit));
    g_MenuBar.addMenu(MenuHelper::createSearchMenu(OnFindReplace, OnGoTo));
    g_MenuBar.addMenu(MenuHelper::createToolsMenu(OnOptionsDialog, OnPluginsDialog));
    g_MenuBar.addMenu(MenuHelper::createHelpMenu(OnAbout, OnDocumentation));

    char *filename = GetFileNameFromCmdLine();
    if (filename)
    {
        if (g_HexData.loadFile(filename))
        {
            CopyString(g_CurrentFilePath, filename, MAX_PATH_LEN);
            g_TotalLines = (int)g_HexData.getHexLines().count;
        }
    }

    XEvent event;
    bool running = true;

    while (running)
    {
        while (XPending(g_display))
        {
            XNextEvent(g_display, &event);

            switch (event.type)
            {
            case Expose:
                if (event.xexpose.count == 0)
                    LinuxRedraw();
                break;

            case ConfigureNotify:
                HandleLinuxResize(event.xconfigure.width, event.xconfigure.height);
                LinuxRedraw();
                break;

            case KeyPress:
                HandleLinuxKeyPress(&event.xkey);
                LinuxRedraw();
                break;

            case ButtonPress:
                HandleLinuxMouseButton(&event.xbutton, true);
                LinuxRedraw();
                break;

            case ButtonRelease:
                HandleLinuxMouseButton(&event.xbutton, false);
                LinuxRedraw();
                break;

            case MotionNotify:
                HandleLinuxMouseMotion(&event.xmotion);
                LinuxRedraw();
                break;

            case ClientMessage:
                if ((Atom)event.xclient.data.l[0] == g_WmDeleteWindow)
                {
                    running = false;
                }
                break;
            }
        }

        usleep(1000);
    }

    SaveOptionsToFile(g_Options);
    XFreeGC(g_display, g_GC);
    XDestroyWindow(g_display, g_window);
    XCloseDisplay(g_display);

    return 0;
}

#endif
