#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#endif

#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <time.h>
#include <sys/types.h>
#include <X11/Xutil.h>
#endif

#ifdef __APPLE__
#include <Cocoa/Cocoa.h>
#include <libproc.h>
#include <sys/types.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/vm_region.h>
#include <mach/vm_map.h>
#include <sys/sysctl.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "processdialog.h"

static ProcessDialogData* g_processDialogData = nullptr;


void RenderProcessDialog(ProcessDialogData* data, int windowWidth, int windowHeight)
{
  if (!data || !data->renderer)
    return;

  Theme theme = data->darkMode ? Theme::Dark() : Theme::Light();
  data->renderer->clear(theme.windowBackground);

  const int margin = 20;
  const int headerY = margin;
  const int rowHeight = 32;

  Color headerColor = theme.headerColor;
  data->renderer->drawText("PID", margin, headerY, headerColor);
  data->renderer->drawText("Process Name", margin + 80, headerY, headerColor);
  data->renderer->drawText("Architecture", margin + 320, headerY, headerColor);

  data->renderer->drawLine(margin, headerY + 20, windowWidth - margin, headerY + 20, theme.separator);

  int listStartY = headerY + 28;
  int listEndY = windowHeight - 70;
  int maxVisibleRows = (listEndY - listStartY) / rowHeight;

  int maxScroll = data->processes.count - maxVisibleRows;
  if (maxScroll < 0) maxScroll = 0;
  if (data->scrollOffset > maxScroll)
    data->scrollOffset = maxScroll;
  if (data->scrollOffset < 0)
    data->scrollOffset = 0;

  int rowY = listStartY;
  int visibleCount = 0;

  for (int i = data->scrollOffset; i < data->processes.count && visibleCount < maxVisibleRows; i++, visibleCount++)
  {
    ProcessEntry* e = &data->processes.entries[i];

    Rect rowRect(margin, rowY, windowWidth - margin * 2, rowHeight - 2);

    if (i == data->selectedIndex)
    {
      Color selectedBg = theme.controlCheck;
      selectedBg.a = 60;
      data->renderer->drawRoundedRect(rowRect, 6.0f, selectedBg, true);

      Color selectedBorder = theme.controlCheck;
      selectedBorder.a = 200;
      data->renderer->drawRoundedRect(rowRect, 6.0f, selectedBorder, false);
    }
    else if (i == data->hoveredIndex)
    {
      Color hoverBg = theme.menuHover;
      hoverBg.a = 80;
      data->renderer->drawRoundedRect(rowRect, 6.0f, hoverBg, true);
    }

    char pidBuf[32];
    IntToStr(e->pid, pidBuf, sizeof(pidBuf));
    Color textColor = (i == data->selectedIndex) ? theme.controlCheck : theme.textColor;
    data->renderer->drawText(pidBuf, margin, rowY + 8, textColor);

    data->renderer->drawText(e->name, margin + 80, rowY + 8, textColor);

    Rect archBadge(margin + 320, rowY + 6, 50, 20);
    bool isDark = (theme.windowBackground.r < 128);
    Color badgeBg = e->is64bit
      ? (isDark ? Color(50, 120, 200) : Color(70, 140, 220))
      : (isDark ? Color(180, 100, 50) : Color(220, 140, 80));
    badgeBg.a = (i == data->selectedIndex) ? 220 : 140;
    data->renderer->drawRoundedRect(archBadge, 4.0f, badgeBg, true);

    Color badgeText = Color(255, 255, 255);
    const char* archText = e->is64bit ? "x64" : "x86";
    data->renderer->drawText(archText, margin + 328, rowY + 9, badgeText);

    rowY += rowHeight;
  }

  if (data->processes.count > maxVisibleRows)
  {
    int scrollbarX = windowWidth - 15;
    int scrollbarY = listStartY;
    int scrollbarHeight = listEndY - listStartY;

    Rect trackRect(scrollbarX, scrollbarY, 8, scrollbarHeight);
    Color trackColor = theme.scrollbarBg;
    data->renderer->drawRoundedRect(trackRect, 4.0f, trackColor, true);

    float thumbRatio = (float)maxVisibleRows / (float)data->processes.count;
    int thumbHeight = (int)(scrollbarHeight * thumbRatio);
    if (thumbHeight < 30) thumbHeight = 30;

    float scrollRatio = (float)data->scrollOffset / (float)maxScroll;
    int thumbY = scrollbarY + (int)((scrollbarHeight - thumbHeight) * scrollRatio);

    Rect thumbRect(scrollbarX, thumbY, 8, thumbHeight);
    Color thumbColor = theme.scrollbarThumb;
    thumbColor.a = 200;
    data->renderer->drawRoundedRect(thumbRect, 4.0f, thumbColor, true);
  }

  data->renderer->drawLine(margin, listEndY + 8, windowWidth - margin, listEndY + 8, theme.separator);

  const int buttonWidth = 100;
  const int buttonHeight = 36;
  const int buttonY = windowHeight - margin - buttonHeight;

  {
    Rect r(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 1);
    ws.pressed = (data->pressedWidget == 1);
    data->renderer->drawModernButton(ws, theme, "Cancel");
  }

  {
    Rect r(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.enabled = (data->selectedIndex >= 0);
    ws.hovered = (data->hoveredWidget == 2);
    ws.pressed = (data->pressedWidget == 2);
    data->renderer->drawModernButton(ws, theme, "Open");
  }
}

void UpdateProcessDialogHover(ProcessDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  const int margin = 20;
  const int headerY = margin;
  const int rowHeight = 32;
  const int listStartY = headerY + 28;
  const int listEndY = windowHeight - 70;

  data->hoveredIndex = -1;
  data->hoveredWidget = -1;

  const int buttonWidth = 100;
  const int buttonHeight = 36;
  const int buttonY = windowHeight - margin - buttonHeight;

  Rect cancelRect(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, buttonHeight);
  if (x >= cancelRect.x && x <= cancelRect.x + cancelRect.width &&
    y >= cancelRect.y && y <= cancelRect.y + cancelRect.height)
  {
    data->hoveredWidget = 1;
    return;
  }

  Rect openRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  if (x >= openRect.x && x <= openRect.x + openRect.width &&
    y >= openRect.y && y <= openRect.y + openRect.height)
  {
    data->hoveredWidget = 2;
    return;
  }

  if (y < listStartY || y > listEndY)
    return;

  int relY = y - listStartY;
  int rowIndex = relY / rowHeight;
  int absoluteIndex = data->scrollOffset + rowIndex;

  if (absoluteIndex >= 0 && absoluteIndex < data->processes.count)
  {
    data->hoveredIndex = absoluteIndex;
  }
}

void HandleProcessDialogClick(ProcessDialogData* data)
{
  if (data->hoveredWidget == 1)
  {
    data->dialogResult = false;
    data->running = false;
  }
  else if (data->hoveredWidget == 2 && data->selectedIndex >= 0)
  {
    data->dialogResult = true;
    data->running = false;
  }
  else if (data->hoveredIndex >= 0)
  {
    data->selectedIndex = data->hoveredIndex;
  }
}


#ifdef _WIN32

bool IsProcessRecentlyAccessed(DWORD pid, FILETIME* outTime)
{
  HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!h)
    return false;

  FILETIME creation, exit, kernel, user;
  bool result = GetProcessTimes(h, &creation, &exit, &kernel, &user);

  if (result && outTime)
  {
    *outTime = creation;
  }

  CloseHandle(h);
  return result;
}

bool ProcessHasVisibleWindow(DWORD pid)
{
  EnumData data = { pid, false };

  EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
      EnumData* pData = (EnumData*)lParam;

      DWORD windowPid = 0;
      GetWindowThreadProcessId(hwnd, &windowPid);

      if (windowPid == pData->pid)
      {
        if (IsWindowVisible(hwnd))
        {
          LONG style = GetWindowLong(hwnd, GWL_STYLE);
          LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

          if ((style & WS_VISIBLE) && !(exStyle & WS_EX_TOOLWINDOW))
          {
            char title[256];
            if (GetWindowTextA(hwnd, title, sizeof(title)) > 0)
            {
              pData->hasWindow = true;
              return FALSE;
            }
          }
        }
      }

      return TRUE;
    }, (LPARAM)&data);

  return data.hasWindow;
}

void SortProcessPriorities(ProcessPriority* priorities, int count)
{
  for (int i = 1; i < count; i++)
  {
    ProcessPriority key = priorities[i];
    int j = i - 1;

    while (j >= 0)
    {
      bool shouldSwap = false;

      if (priorities[j].priority < key.priority)
      {
        shouldSwap = true;
      }
      else if (priorities[j].priority == key.priority && priorities[j].priority >= 2)
      {
        if (priorities[j].timestamp < key.timestamp)
        {
          shouldSwap = true;
        }
      }

      if (shouldSwap)
      {
        priorities[j + 1] = priorities[j];
        j--;
      }
      else
      {
        break;
      }
    }

    priorities[j + 1] = key;
  }
}

bool EnumerateProcesses(ProcessList* list)
{
  if (!list || !list->entries)
    return false;

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32 pe;
  memset(&pe, 0, sizeof(pe));
  pe.dwSize = sizeof(pe);

  if (!Process32First(snap, &pe))
  {
    CloseHandle(snap);
    return false;
  }

  ProcessEntry* tempEntries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);
  ProcessPriority* priorities = (ProcessPriority*)PlatformAlloc(sizeof(ProcessPriority) * 512);
  int tempCount = 0;

  FILETIME currentTime;
  GetSystemTimeAsFileTime(&currentTime);
  ULARGE_INTEGER currentTimeInt;
  currentTimeInt.LowPart = currentTime.dwLowDateTime;
  currentTimeInt.HighPart = currentTime.dwHighDateTime;

  do
  {
    if (tempCount >= 512)
      break;

    if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4)
      continue;

    if (pe.szExeFile[0] == '[')
      continue;

    ProcessEntry* e = &tempEntries[tempCount];
    e->pid = (int)pe.th32ProcessID;

    int i = 0;
    while (i < 259 && pe.szExeFile[i] != 0)
    {
      e->name[i] = (char)pe.szExeFile[i];
      i++;
    }
    e->name[i] = 0;

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)e->pid);
    if (!h)
    {
      h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)e->pid);
    }

    if (h)
    {
      BOOL wow64 = FALSE;
      if (IsWow64Process(h, &wow64))
      {
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
          e->is64bit = !wow64;
        }
        else
        {
          e->is64bit = false;
        }
      }
      else
      {
        e->is64bit = false;
      }

      CloseHandle(h);
    }
    else
    {
      e->is64bit = false;
    }

    priorities[tempCount].index = tempCount;
    priorities[tempCount].priority = 1;

    FILETIME processTime;
    if (IsProcessRecentlyAccessed(pe.th32ProcessID, &processTime))
    {
      ULARGE_INTEGER processTimeInt;
      processTimeInt.LowPart = processTime.dwLowDateTime;
      processTimeInt.HighPart = processTime.dwHighDateTime;

      priorities[tempCount].timestamp = processTimeInt.QuadPart;

      const ULONGLONG oneHour = 36000000000ULL;

      if ((currentTimeInt.QuadPart - processTimeInt.QuadPart) < oneHour)
      {
        priorities[tempCount].priority = 3;
      }
    }

    if (ProcessHasVisibleWindow(pe.th32ProcessID))
    {
      if (priorities[tempCount].priority < 2)
        priorities[tempCount].priority = 2;
    }

    tempCount++;

  } while (Process32Next(snap, &pe));

  CloseHandle(snap);

  SortProcessPriorities(priorities, tempCount);

  list->count = 0;
  for (int i = 0; i < tempCount && i < (int)list->capacity; i++)
  {
    int sourceIndex = priorities[i].index;
    list->entries[list->count] = tempEntries[sourceIndex];
    list->count++;
  }

  PlatformFree(tempEntries, sizeof(ProcessEntry) * 512);
  PlatformFree(priorities, sizeof(ProcessPriority) * 512);

  return true;
}

bool ReadProcessMemoryData(int pid, HexData* hexData)
{
  if (!hexData)
    return false;

  HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
  if (!hProcess)
  {
    return false;
  }

  MEMORY_BASIC_INFORMATION mbi;
  uint8_t* address = 0;

  hexData->clear();

  ByteBuffer tempBuffer;
  bb_init(&tempBuffer);

  while (VirtualQueryEx(hProcess, address, &mbi, sizeof(mbi)) == sizeof(mbi))
  {
    if (mbi.State == MEM_COMMIT &&
      (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
    {
      size_t oldSize = tempBuffer.size;
      size_t newSize = oldSize + mbi.RegionSize;

      if (bb_resize(&tempBuffer, newSize))
      {
        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, mbi.BaseAddress, tempBuffer.data + oldSize, mbi.RegionSize, &bytesRead))
        {
          bb_resize(&tempBuffer, oldSize + bytesRead);
        }
        else
        {
          bb_resize(&tempBuffer, oldSize);
        }
      }
    }

    address = (uint8_t*)mbi.BaseAddress + mbi.RegionSize;

    if ((size_t)address >= 0x7FFFFFFF)
      break;
  }

  CloseHandle(hProcess);

  if (tempBuffer.size > 0)
  {
    bb_resize(&hexData->fileData, tempBuffer.size);
    for (size_t i = 0; i < tempBuffer.size; i++)
    {
      hexData->fileData.data[i] = tempBuffer.data[i];
    }
    hexData->convertDataToHex(16);
    bb_free(&tempBuffer);
    return true;
  }

  bb_free(&tempBuffer);
  return false;
}

LRESULT CALLBACK ProcessDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ProcessDialogData* data = g_processDialogData;

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
      RenderProcessDialog(data, rect.right, rect.bottom);
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
      UpdateProcessDialogHover(data, x, y, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_MOUSEWHEEL:
  {
    if (data)
    {
      int delta = GET_WHEEL_DELTA_WPARAM(wParam);

      RECT rect;
      GetClientRect(hwnd, &rect);
      int maxVisibleRows = (rect.bottom - 118) / 32;
      int maxScroll = data->processes.count - maxVisibleRows;
      if (maxScroll < 0) maxScroll = 0;

      int scrollLines = (delta > 0) ? -3 : 3;
      data->scrollOffset += scrollLines;

      if (data->scrollOffset < 0)
        data->scrollOffset = 0;
      if (data->scrollOffset > maxScroll)
        data->scrollOffset = maxScroll;

      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    if (data)
    {
      data->pressedWidget = data->hoveredWidget;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONUP:
  {
    if (data)
    {
      if (data->pressedWidget == data->hoveredWidget || data->hoveredIndex >= 0)
      {
        HandleProcessDialogClick(data);
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

bool ShowProcessDialog(HWND parent, AppOptions& options)
{
  const wchar_t* className = L"ProcessDialogWindow";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = ProcessDialogProc;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

  UnregisterClassW(className, wc.hInstance);
  if (!RegisterClassExW(&wc))
    return false;

  ProcessDialogData data = {};
  data.darkMode = options.darkMode;
  data.dialogResult = false;
  data.running = true;
  data.selectedIndex = -1;
  data.hoveredIndex = -1;
  data.hoveredWidget = -1;
  data.pressedWidget = -1;
  data.scrollOffset = 0;

  g_processDialogData = &data;

  data.processes.capacity = 512;
  data.processes.count = 0;
  data.processes.entries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);

  if (!data.processes.entries)
  {
    g_processDialogData = nullptr;
    return false;
  }

  for (int i = 0; i < 512; i++)
  {
    data.processes.entries[i].pid = 0;
    data.processes.entries[i].name[0] = '\0';
    data.processes.entries[i].is64bit = false;
  }

  if (!EnumerateProcesses(&data.processes))
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  int width = 500;
  int height = 600;

  RECT parentRect;
  GetWindowRect(parent, &parentRect);
  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

  HWND hwnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    className,
    L"Select Process",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    x, y, width, height,
    parent,
    nullptr,
    GetModuleHandleW(NULL),
    nullptr);

  if (!hwnd)
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  data.window = hwnd;

  if (data.darkMode)
  {
    BOOL dark = TRUE;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  }

  data.renderer = new RenderManager();
  if (!data.renderer || !data.renderer->initialize(hwnd))
  {
    if (data.renderer)
      delete data.renderer;
    DestroyWindow(hwnd);
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
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

  int selectedPid = -1;
  if (data.dialogResult && data.selectedIndex >= 0 && data.selectedIndex < data.processes.count)
  {
    selectedPid = data.processes.entries[data.selectedIndex].pid;
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

  PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
  g_processDialogData = nullptr;

  if (selectedPid > 0)
  {
    extern HexData g_HexData;
    extern char g_CurrentFilePath[MAX_PATH_LEN];
    extern int g_TotalLines;
    extern int g_ScrollY;

    if (ReadProcessMemoryData(selectedPid, &g_HexData))
    {
      StrCopy(g_CurrentFilePath, "[Process Memory - PID ");
      char pidBuf[32];
      IntToStr(selectedPid, pidBuf, sizeof(pidBuf));
      StrCat(g_CurrentFilePath, pidBuf);
      StrCat(g_CurrentFilePath, "]");

      g_TotalLines = (int)g_HexData.getHexLines().count;
      g_ScrollY = 0;

      InvalidateRect(parent, NULL, FALSE);
    }
    else
    {
      MessageBoxA(parent, "Failed to read process memory. Make sure you have sufficient privileges.",
        "Error", MB_OK | MB_ICONERROR);
    }
  }

  return data.dialogResult;
}

#endif


#ifdef __linux__

bool IsProcessRecentlyAccessed(int pid, time_t* outTime)
{
  char statPath[256];
  snprintf(statPath, sizeof(statPath), "/proc/%d/stat", pid);

  FILE* f = fopen(statPath, "r");
  if (!f)
    return false;

  unsigned long long starttime;
  char comm[256];
  char state;
  int ppid;

  if (fscanf(f, "%*d %s %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %*u %*u %*d %*d %*d %*d %*d %*d %llu",
    comm, &state, &ppid, &starttime) < 4)
  {
    fclose(f);
    return false;
  }

  fclose(f);

  time_t procStartTime = starttime / sysconf(_SC_CLK_TCK);

  if (outTime)
    *outTime = procStartTime;

  return true;
}

bool ProcessHasVisibleWindow(int pid)
{
  Display* display = XOpenDisplay(NULL);
  if (!display)
    return false;

  Window root = DefaultRootWindow(display);
  Atom clientListAtom = XInternAtom(display, "_NET_CLIENT_LIST", False);
  Atom pidAtom = XInternAtom(display, "_NET_WM_PID", False);

  Atom actualType;
  int format;
  unsigned long numItems, bytesAfter;
  unsigned char* data = NULL;

  if (XGetWindowProperty(display, root, clientListAtom, 0, ~0L, False,
    XA_WINDOW, &actualType, &format, &numItems,
    &bytesAfter, &data) != Success)
  {
    XCloseDisplay(display);
    return false;
  }

  Window* windowList = (Window*)data;
  bool hasWindow = false;

  for (unsigned long i = 0; i < numItems; i++)
  {
    unsigned char* pidData = NULL;
    unsigned long pidItems;

    if (XGetWindowProperty(display, windowList[i], pidAtom, 0, 1, False,
      XA_CARDINAL, &actualType, &format, &pidItems,
      &bytesAfter, &pidData) == Success)
    {
      if (pidItems > 0)
      {
        int windowPid = *(int*)pidData;
        if (windowPid == pid)
        {
          XWindowAttributes attrs;
          if (XGetWindowAttributes(display, windowList[i], &attrs))
          {
            if (attrs.map_state == IsViewable)
            {
              hasWindow = true;
              XFree(pidData);
              break;
            }
          }
        }
      }
      XFree(pidData);
    }
  }

  XFree(data);
  XCloseDisplay(display);

  return hasWindow;
}

int CompareProcessPriority(const void* a, const void* b)
{
  const ProcessPriority* pa = (const ProcessPriority*)a;
  const ProcessPriority* pb = (const ProcessPriority*)b;

  if (pa->priority != pb->priority)
    return pb->priority - pa->priority;

  if (pa->priority >= 2)
  {
    if (pa->timestamp > pb->timestamp)
      return -1;
    else if (pa->timestamp < pb->timestamp)
      return 1;
  }

  return 0;
}

bool EnumerateProcesses(ProcessList* list)
{

  if (!list || !list->entries)
  {
    return false;
  }

  DIR* procDir = opendir("/proc");
  if (!procDir)
  {
    return false;
  }

  ProcessEntry* tempEntries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);
  ProcessPriority* priorities = (ProcessPriority*)PlatformAlloc(sizeof(ProcessPriority) * 512);

  if (!tempEntries || !priorities)
  {

    if (tempEntries) PlatformFree(tempEntries, sizeof(ProcessEntry) * 512);
    if (priorities) PlatformFree(priorities, sizeof(ProcessPriority) * 512);
    closedir(procDir);
    return false;
  }


  int tempCount = 0;
  time_t currentTime = time(NULL);
  struct dirent* entry;

  while ((entry = readdir(procDir)) != NULL && tempCount < 512)
  {
    int pid = atoi(entry->d_name);
    if (pid <= 0)
      continue;


    char cmdlinePath[256];
    snprintf(cmdlinePath, sizeof(cmdlinePath), "/proc/%d/cmdline", pid);

    FILE* cmdFile = fopen(cmdlinePath, "r");
    if (!cmdFile)
    {
      continue;
    }

    char cmdline[260];
    size_t len = fread(cmdline, 1, sizeof(cmdline) - 1, cmdFile);
    fclose(cmdFile);
    cmdline[len] = '\0';

    if (len == 0)
    {
      continue;
    }

    ProcessEntry* e = &tempEntries[tempCount];
    e->pid = pid;

    const char* procName = cmdline;
    const char* lastSlash = NULL;
    for (size_t i = 0; i < len && cmdline[i]; i++)
    {
      if (cmdline[i] == '/')
        lastSlash = &cmdline[i];
    }
    if (lastSlash)
      procName = lastSlash + 1;

    int i = 0;
    while (i < 259 && procName[i] != 0)
    {
      e->name[i] = procName[i];
      i++;
    }
    e->name[i] = 0;

    char exePath[256];
    char exeLink[512];
    snprintf(exePath, sizeof(exePath), "/proc/%d/exe", pid);
    ssize_t linkLen = readlink(exePath, exeLink, sizeof(exeLink) - 1);

    e->is64bit = false;
    if (linkLen > 0)
    {
      exeLink[linkLen] = '\0';

      FILE* exeFile = fopen(exeLink, "rb");
      if (exeFile)
      {
        unsigned char elfHeader[5];
        if (fread(elfHeader, 1, 5, exeFile) == 5)
        {
          if (elfHeader[0] == 0x7F && elfHeader[1] == 'E' &&
            elfHeader[2] == 'L' && elfHeader[3] == 'F')
          {
            e->is64bit = (elfHeader[4] == 2);
          }
        }
        fclose(exeFile);
      }
    }

    priorities[tempCount].index = tempCount;
    priorities[tempCount].priority = 1;

    time_t processTime;
    if (IsProcessRecentlyAccessed(pid, &processTime))
    {
      priorities[tempCount].timestamp = (uint64_t)processTime;

      if ((currentTime - processTime) < 3600)
        priorities[tempCount].priority = 3;
    }

    tempCount++;
  }

  closedir(procDir);


  qsort(priorities, tempCount, sizeof(ProcessPriority), CompareProcessPriority);

  list->count = 0;
  for (int i = 0; i < tempCount && i < (int)list->capacity; i++)
  {
    int sourceIndex = priorities[i].index;
    list->entries[list->count] = tempEntries[sourceIndex];
    list->count++;
  }


  PlatformFree(tempEntries, sizeof(ProcessEntry) * 512);
  PlatformFree(priorities, sizeof(ProcessPriority) * 512);


  return true;
}

bool ReadProcessMemoryData(int pid, HexData* hexData)
{
  if (!hexData)
    return false;

  hexData->clear();

  char mapsPath[256];
  snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);

  FILE* mapsFile = fopen(mapsPath, "r");
  if (!mapsFile)
  {
    return false;
  }

  ByteBuffer tempBuffer;
  bb_init(&tempBuffer);

  char line[512];
  while (fgets(line, sizeof(line), mapsFile))
  {
    unsigned long long startAddr, endAddr;
    char perms[5];

    if (sscanf(line, "%llx-%llx %4s", &startAddr, &endAddr, perms) != 3)
      continue;

    if (perms[0] != 'r')
      continue;

    size_t regionSize = (size_t)(endAddr - startAddr);

    if (regionSize > 100 * 1024 * 1024)
      continue;

    size_t oldSize = tempBuffer.size;
    size_t newSize = oldSize + regionSize;

    if (!bb_resize(&tempBuffer, newSize))
    {
      continue;
    }

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = tempBuffer.data + oldSize;
    local[0].iov_len = regionSize;
    remote[0].iov_base = (void*)startAddr;
    remote[0].iov_len = regionSize;

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);

    if (nread > 0)
    {
      bb_resize(&tempBuffer, oldSize + (size_t)nread);
    }
    else
    {
      bb_resize(&tempBuffer, oldSize);
    }
  }

  fclose(mapsFile);

  if (tempBuffer.size > 0)
  {
    bb_resize(&hexData->fileData, tempBuffer.size);
    for (size_t i = 0; i < tempBuffer.size; i++)
    {
      hexData->fileData.data[i] = tempBuffer.data[i];
    }
    hexData->convertDataToHex(16);
    bb_free(&tempBuffer);
    return true;
  }

  bb_free(&tempBuffer);
  return false;
}

bool ShowProcessDialog(NativeWindow parent, AppOptions& options)
{
  extern Display* g_display;

  if (!g_display)
  {
    return false;
  }

  ProcessDialogData data = {};
  data.darkMode = options.darkMode;
  data.dialogResult = false;
  data.running = true;
  data.selectedIndex = -1;
  data.hoveredIndex = -1;
  data.hoveredWidget = -1;
  data.pressedWidget = -1;
  data.scrollOffset = 0;

  g_processDialogData = &data;

  data.processes.capacity = 512;
  data.processes.count = 0;
  data.processes.entries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);

  if (!data.processes.entries)
  {
    g_processDialogData = nullptr;
    return false;
  }

  for (int i = 0; i < 512; i++)
  {
    data.processes.entries[i].pid = 0;
    data.processes.entries[i].name[0] = '\0';
    data.processes.entries[i].is64bit = false;
  }

  if (!EnumerateProcesses(&data.processes))
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  int width = 500;
  int height = 600;

  Window parentWindow = (Window)parent;
  int screen = DefaultScreen(g_display);

  XWindowAttributes parentAttrs;
  if (!XGetWindowAttributes(g_display, parentWindow, &parentAttrs))
  {
    parentAttrs.x = 100;
    parentAttrs.y = 100;
    parentAttrs.width = 800;
    parentAttrs.height = 600;
  }

  int x = parentAttrs.x + (parentAttrs.width - width) / 2;
  int y = parentAttrs.y + (parentAttrs.height - height) / 2;

  XSetWindowAttributes attrs;
  attrs.background_pixel = WhitePixel(g_display, screen);
  attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
    ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
  attrs.override_redirect = False;

  Window dialog = XCreateWindow(
    g_display,
    RootWindow(g_display, screen),
    x, y, width, height,
    2,
    DefaultDepth(g_display, screen),
    InputOutput,
    DefaultVisual(g_display, screen),
    CWBackPixel | CWEventMask | CWOverrideRedirect,
    &attrs);

  if (!dialog)
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  XStoreName(g_display, dialog, "Select Process");

  XSizeHints* sizeHints = XAllocSizeHints();
  if (sizeHints)
  {
    sizeHints->flags = PMinSize | PMaxSize;
    sizeHints->min_width = width;
    sizeHints->min_height = height;
    sizeHints->max_width = width;
    sizeHints->max_height = height;
    XSetWMNormalHints(g_display, dialog, sizeHints);
    XFree(sizeHints);
  }

  XSetTransientForHint(g_display, dialog, parentWindow);

  Atom wmDelete = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(g_display, dialog, &wmDelete, 1);

  data.window = (NativeWindow)dialog;

  XMapWindow(g_display, dialog);
  XRaiseWindow(g_display, dialog);
  XSync(g_display, False);

  XEvent mapEvent;
  while (true)
  {
    XNextEvent(g_display, &mapEvent);
    if (mapEvent.type == MapNotify && mapEvent.xmap.window == dialog)
    {
      break;
    }
  }

  data.renderer = new RenderManager();
  if (!data.renderer || !data.renderer->initialize((NativeWindow)dialog))
  {
    if (data.renderer)
      delete data.renderer;

    XDestroyWindow(g_display, dialog);
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  data.renderer->resize(width, height);

  XSetInputFocus(g_display, dialog, RevertToParent, CurrentTime);

  data.renderer->beginFrame();
  RenderProcessDialog(&data, width, height);
  GC gc = XCreateGC(g_display, dialog, 0, nullptr);
  data.renderer->endFrame(gc);
  XFreeGC(g_display, gc);

  XFlush(g_display);

  XEvent event;
  bool needsRedraw = false;

  while (data.running)
  {
    if (XCheckWindowEvent(g_display, dialog,
      ExposureMask | KeyPressMask | ButtonPressMask |
      ButtonReleaseMask | PointerMotionMask | StructureNotifyMask,
      &event) ||
      XCheckTypedWindowEvent(g_display, dialog, ClientMessage, &event))
    {
      needsRedraw = false;

      switch (event.type)
      {
      case Expose:
        if (event.xexpose.count == 0)
        {
          needsRedraw = true;
        }
        break;

      case MotionNotify:
        UpdateProcessDialogHover(&data, event.xmotion.x, event.xmotion.y, width, height);
        needsRedraw = true;
        break;

      case ButtonPress:
        if (event.xbutton.button == Button1)
        {
          data.pressedWidget = data.hoveredWidget;
          needsRedraw = true;
        }
        else if (event.xbutton.button == Button4)
        {
          int maxVisibleRows = (height - 118) / 32;
          int maxScroll = data.processes.count - maxVisibleRows;
          if (maxScroll < 0) maxScroll = 0;

          data.scrollOffset -= 3;
          if (data.scrollOffset < 0) data.scrollOffset = 0;
          needsRedraw = true;
        }
        else if (event.xbutton.button == Button5)
        {
          int maxVisibleRows = (height - 118) / 32;
          int maxScroll = data.processes.count - maxVisibleRows;
          if (maxScroll < 0) maxScroll = 0;

          data.scrollOffset += 3;
          if (data.scrollOffset > maxScroll) data.scrollOffset = maxScroll;
          needsRedraw = true;
        }
        break;

      case ButtonRelease:
        if (event.xbutton.button == Button1)
        {
          if (data.pressedWidget == data.hoveredWidget || data.hoveredIndex >= 0)
            HandleProcessDialogClick(&data);

          data.pressedWidget = -1;
          needsRedraw = true;
        }
        break;

      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDelete)
        {
          data.dialogResult = false;
          data.running = false;
        }
        break;

      case KeyPress:
      {
        KeySym key = XLookupKeysym(&event.xkey, 0);
        if (key == XK_Escape)
        {
          data.dialogResult = false;
          data.running = false;
        }
      }
      break;
      }

      if (needsRedraw)
      {
        data.renderer->beginFrame();
        RenderProcessDialog(&data, width, height);
        GC gc = XCreateGC(g_display, dialog, 0, nullptr);
        data.renderer->endFrame(gc);
        XFreeGC(g_display, gc);
      }
    }
    else
    {
      usleep(10000);
    }
  }

  int selectedPid = -1;
  if (data.dialogResult &&
    data.selectedIndex >= 0 &&
    data.selectedIndex < data.processes.count)
  {
    selectedPid = data.processes.entries[data.selectedIndex].pid;
  }

  if (data.renderer)
  {
    delete data.renderer;
    data.renderer = nullptr;
  }

  XDestroyWindow(g_display, dialog);
  XFlush(g_display);

  PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
  g_processDialogData = nullptr;

  if (selectedPid > 0)
  {
    extern HexData g_HexData;
    extern char g_CurrentFilePath[MAX_PATH_LEN];
    extern int g_TotalLines;
    extern int g_ScrollY;
    extern void LinuxRedraw();

    if (ReadProcessMemoryData(selectedPid, &g_HexData))
    {
      StrCopy(g_CurrentFilePath, "[Process Memory - PID ");
      char pidBuf[32];
      IntToStr(selectedPid, pidBuf, sizeof(pidBuf));
      StrCat(g_CurrentFilePath, pidBuf);
      StrCat(g_CurrentFilePath, "]");

      g_TotalLines = (int)g_HexData.getHexLines().count;
      g_ScrollY = 0;

      LinuxRedraw();
    }
    else
    {
      printf("[ShowProcessDialog] ERROR: Failed to read process memory\n"); fflush(stdout);
    }
  }

  return data.dialogResult;
}

#endif


#ifdef __APPLE__

bool IsProcessRecentlyAccessed(int pid, time_t* outTime)
{
  struct proc_bsdinfo procInfo;
  int ret = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &procInfo, sizeof(procInfo));

  if (ret <= 0)
    return false;

  if (outTime)
    *outTime = procInfo.pbi_start_tvsec;

  return true;
}

bool ProcessHasVisibleWindow(int pid)
{
  CFArrayRef windowList = CGWindowListCopyWindowInfo(
    kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
    kCGNullWindowID);

  if (!windowList)
    return false;

  bool hasWindow = false;
  CFIndex count = CFArrayGetCount(windowList);

  for (CFIndex i = 0; i < count; i++)
  {
    CFDictionaryRef windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);

    CFNumberRef pidNum = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowOwnerPID);
    if (!pidNum)
      continue;

    int windowPid;
    CFNumberGetValue(pidNum, kCFNumberIntType, &windowPid);

    if (windowPid == pid)
    {
      CFStringRef windowName = (CFStringRef)CFDictionaryGetValue(windowInfo, kCGWindowName);
      if (windowName && CFStringGetLength(windowName) > 0)
      {
        hasWindow = true;
        break;
      }
    }
  }

  CFRelease(windowList);
  return hasWindow;
}

int CompareProcessPriority(const void* a, const void* b)
{
  const ProcessPriority* pa = (const ProcessPriority*)a;
  const ProcessPriority* pb = (const ProcessPriority*)b;

  if (pa->priority != pb->priority)
    return pb->priority - pa->priority;

  if (pa->priority >= 2)
  {
    if (pa->timestamp > pb->timestamp)
      return -1;
    else if (pa->timestamp < pb->timestamp)
      return 1;
  }

  return 0;
}

bool EnumerateProcesses(ProcessList* list)
{
  if (!list || !list->entries)
    return false;

  int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
  size_t size;

  if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0)
    return false;

  struct kinfo_proc* procList = (struct kinfo_proc*)malloc(size);
  if (!procList)
    return false;

  if (sysctl(mib, 4, procList, &size, NULL, 0) < 0)
  {
    free(procList);
    return false;
  }

  int procCount = (int)(size / sizeof(struct kinfo_proc));

  ProcessEntry* tempEntries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);
  ProcessPriority* priorities = (ProcessPriority*)PlatformAlloc(sizeof(ProcessPriority) * 512);
  int tempCount = 0;

  time_t currentTime = time(NULL);

  for (int i = 0; i < procCount && tempCount < 512; i++)
  {
    int pid = procList[i].kp_proc.p_pid;
    if (pid <= 0)
      continue;

    ProcessEntry* e = &tempEntries[tempCount];
    e->pid = pid;

    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) <= 0)
    {
      int j = 0;
      while (j < 259 && procList[i].kp_proc.p_comm[j] != 0)
      {
        e->name[j] = procList[i].kp_proc.p_comm[j];
        j++;
      }
      e->name[j] = 0;
    }
    else
    {
      const char* name = pathbuf;
      for (int j = 0; pathbuf[j]; j++)
      {
        if (pathbuf[j] == '/')
          name = &pathbuf[j + 1];
      }

      int j = 0;
      while (j < 259 && name[j] != 0)
      {
        e->name[j] = name[j];
        j++;
      }
      e->name[j] = 0;
    }

    if (e->name[0] == 0)
      continue;

    cpu_type_t cpuType;
    size_t cpuTypeSize = sizeof(cpuType);
    int mib2[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, pid };

    e->is64bit = false;
    if (sysctl(mib2, 4, NULL, &size, NULL, 0) == 0)
    {
      struct kinfo_proc kp;
      size = sizeof(kp);
      if (sysctl(mib2, 4, &kp, &size, NULL, 0) == 0)
      {
        e->is64bit = (kp.kp_proc.p_flag & P_LP64) != 0;
      }
    }

    priorities[tempCount].index = tempCount;
    priorities[tempCount].priority = 1;

    time_t processTime;
    if (IsProcessRecentlyAccessed(pid, &processTime))
    {
      priorities[tempCount].timestamp = (uint64_t)processTime;

      if ((currentTime - processTime) < 3600)
      {
        priorities[tempCount].priority = 3;
      }
    }

    if (ProcessHasVisibleWindow(pid))
    {
      if (priorities[tempCount].priority < 2)
        priorities[tempCount].priority = 2;
    }

    tempCount++;
  }

  free(procList);

  qsort(priorities, tempCount, sizeof(ProcessPriority), CompareProcessPriority);

  list->count = 0;
  for (int i = 0; i < tempCount && i < (int)list->capacity; i++)
  {
    int sourceIndex = priorities[i].index;
    list->entries[list->count] = tempEntries[sourceIndex];
    list->count++;
  }

  PlatformFree(tempEntries, sizeof(ProcessEntry) * 512);
  PlatformFree(priorities, sizeof(ProcessPriority) * 512);

  return true;
}

bool ReadProcessMemoryData(int pid, HexData* hexData)
{
  if (!hexData)
    return false;

  hexData->clear();

  mach_port_t task;
  kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

  if (kr != KERN_SUCCESS)
  {
    return false;
  }

  ByteBuffer tempBuffer;
  bb_init(&tempBuffer);

  mach_vm_address_t address = 0;
  mach_vm_size_t size = 0;
  vm_region_basic_info_data_64_t info;
  mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
  mach_port_t object_name;

  while (true)
  {
    kr = mach_vm_region(task, &address, &size, VM_REGION_BASIC_INFO_64,
      (vm_region_info_t)&info, &count, &object_name);

    if (kr != KERN_SUCCESS)
      break;

    if (info.protection & VM_PROT_READ)
    {
      if (size > 100 * 1024 * 1024)
      {
        address += size;
        continue;
      }

      size_t oldSize = tempBuffer.size;
      size_t newSize = oldSize + (size_t)size;

      if (bb_resize(&tempBuffer, newSize))
      {
        mach_vm_size_t bytesRead = 0;
        kr = mach_vm_read_overwrite(task, address, size,
          (mach_vm_address_t)(tempBuffer.data + oldSize),
          &bytesRead);

        if (kr == KERN_SUCCESS && bytesRead > 0)
        {
          bb_resize(&tempBuffer, oldSize + (size_t)bytesRead);
        }
        else
        {
          bb_resize(&tempBuffer, oldSize);
        }
      }
    }

    address += size;
  }

  mach_port_deallocate(mach_task_self(), task);

  if (tempBuffer.size > 0)
  {
    bb_resize(&hexData->fileData, tempBuffer.size);
    for (size_t i = 0; i < tempBuffer.size; i++)
    {
      hexData->fileData.data[i] = tempBuffer.data[i];
    }
    hexData->convertDataToHex(16);
    bb_free(&tempBuffer);
    return true;
  }

  bb_free(&tempBuffer);
  return false;
}

bool ShowProcessDialog(NativeWindow parent, AppOptions& options)
{
  @autoreleasepool
  {
    ProcessDialogData data = {};
    data.darkMode = options.darkMode;
    data.dialogResult = false;
    data.running = true;
    data.selectedIndex = -1;
    data.hoveredIndex = -1;
    data.hoveredWidget = -1;
    data.pressedWidget = -1;
    data.scrollOffset = 0;

    g_processDialogData = &data;

    data.processes.capacity = 512;
    data.processes.count = 0;
    data.processes.entries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);

    if (!data.processes.entries)
    {
      g_processDialogData = nullptr;
      return false;
    }

    for (int i = 0; i < 512; i++)
    {
      data.processes.entries[i].pid = 0;
      data.processes.entries[i].name[0] = '\0';
      data.processes.entries[i].is64bit = false;
    }

    if (!EnumerateProcesses(&data.processes))
    {
      PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
      g_processDialogData = nullptr;
      return false;
    }

    int width = 500;
    int height = 600;

    NSWindow* parentWindow = (__bridge NSWindow*)parent;
    NSRect parentFrame = [parentWindow frame];

    NSRect frame = NSMakeRect(
      parentFrame.origin.x + (parentFrame.size.width - width) / 2,
      parentFrame.origin.y + (parentFrame.size.height - height) / 2,
      width, height);

    NSWindow* dialog = [[NSWindow alloc]
      initWithContentRect:frame
      styleMask : (NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
      backing : NSBackingStoreBuffered
      defer : NO];

    [dialog setTitle:@"Select Process"] ;
    [dialog setReleasedWhenClosed:NO] ;

    [parentWindow addChildWindow:dialog ordered : NSWindowAbove] ;

    data.window = (__bridge void*)dialog;

    data.renderer = new RenderManager();
    NSView* contentView = [dialog contentView];
    if (!data.renderer || !data.renderer->initialize((__bridge NativeWindow)contentView))
    {
      if (data.renderer)
        delete data.renderer;
      [dialog close] ;
      PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
      g_processDialogData = nullptr;
      return false;
    }

    data.renderer->resize(width, height);

    [dialog makeKeyAndOrderFront:nil] ;

    NSModalSession session = [NSApp beginModalSessionForWindow:dialog];
    while (data.running)
    {
      NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate : [NSDate distantPast]
                                             inMode : NSDefaultRunLoopMode
                                            dequeue : YES];

      if (event)
      {
        [NSApp sendEvent:event] ;

        if ([event type] == NSEventTypeKeyDown)
        {
          if ([event keyCode] == 53)
          {
            data.dialogResult = false;
            data.running = false;
          }
        }
      }

      if ([NSApp runModalSession : session] != NSModalResponseContinue)
        break;

      usleep(1000);
    }

    [NSApp endModalSession:session];

    int selectedPid = -1;
    if (data.dialogResult && data.selectedIndex >= 0 && data.selectedIndex < data.processes.count)
    {
      selectedPid = data.processes.entries[data.selectedIndex].pid;
    }

    if (data.renderer)
    {
      delete data.renderer;
      data.renderer = nullptr;
    }

    [parentWindow removeChildWindow:dialog];
    [dialog close] ;

    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;

    if (selectedPid > 0)
    {
      extern HexData g_HexData;
      extern char g_CurrentFilePath[MAX_PATH_LEN];
      extern int g_TotalLines;
      extern int g_ScrollY;

      if (ReadProcessMemoryData(selectedPid, &g_HexData))
      {
        StrCopy(g_CurrentFilePath, "[Process Memory - PID ");
        char pidBuf[32];
        IntToStr(selectedPid, pidBuf, sizeof(pidBuf));
        StrCat(g_CurrentFilePath, pidBuf);
        StrCat(g_CurrentFilePath, "]");

        g_TotalLines = (int)g_HexData.getHexLines().count;
        g_ScrollY = 0;

        [[parentWindow contentView]setNeedsDisplay:YES];
      }
      else
      {
        NSAlert* alert = [[NSAlert alloc]init];
        [alert setMessageText:@"Error"] ;
        [alert setInformativeText:@"Failed to read process memory. Make sure you have sufficient privileges."] ;
        [alert addButtonWithTitle:@"OK"] ;
        [alert runModal] ;
      }
    }

    return data.dialogResult;
  }
}

#endif
