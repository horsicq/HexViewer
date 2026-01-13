#include "render.h"
#include "global.h"
#include "searchdialog.h"

struct SelectBlockDialogData
{
  PlatformWindow platformWindow;
  RenderManager* renderer = nullptr;
  bool running = false;
  bool dialogResult = false;
  int hoveredWidget = -1;
  int pressedWidget = -1;
  int activeTextBox = 0;
  int selectedRadio = 0;
  int selectedMode = 0;
#ifdef _WIN32
  char startOffsetText[256] = { 0 };
  char endOffsetText[256] = { 0 };
  char lengthText[256] = { 0 };
  void (*callback)(const char*, const char*, bool, int) = nullptr;
  void* callbackUserData = nullptr;
#else
  std::string startOffsetText;
  std::string endOffsetText;
  std::string lengthText;
  std::function<void(const std::string&, const std::string&, bool, int)> callback;
#endif
};

#ifdef _WIN32
void ShowSelectBlockDialog(
  void* parentHandle,
  bool darkMode,
  void (*callback)(const char*, const char*, bool, int),
  void* userData,
  long long initialStart = -1,
  long long initialEnd = -1);
#else
void ShowSelectBlockDialog(
  void* parentHandle,
  bool darkMode,
  std::function<void(const std::string&, const std::string&, bool, int)> callback,
  long long initialStart = -1,
  long long initialEnd = -1);
#endif
