#pragma once
#include "render.h"
#include "global.h"
#include "options.h"

#ifdef _WIN32
struct EnumData
{
  DWORD pid;
  bool hasWindow;
};
#endif

struct ProcessPriority
{
  int index;
  int priority;
  uint64_t timestamp;
};

struct ProcessEntry
{
  int pid;
  char name[260];
  bool is64bit;
};

struct ProcessList
{
  ProcessEntry* entries;
  int count;
  int capacity;
};

struct ProcessDialogData
{
  NativeWindow window;
  RenderManager* renderer;
  ProcessList processes;
  bool darkMode;
  bool dialogResult;
  bool running;
  int selectedIndex;
  int hoveredIndex;
  int hoveredWidget;
  int pressedWidget;
  int scrollOffset;
};

#ifdef _WIN32
bool ShowProcessDialog(HWND parent, AppOptions& options);
#else
bool ShowProcessDialog(NativeWindow parent, AppOptions& options);
#endif
