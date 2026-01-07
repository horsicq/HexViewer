#ifndef UPDATE_H
#define UPDATE_H

#include "render.h"

#ifdef _WIN32
#include <windows.h>
typedef HWND NativeWindow;
#elif defined(__APPLE__)
typedef void* NativeWindow;
#elif defined(__linux__)
#include <X11/Xlib.h>
typedef Window NativeWindow;
#endif

#ifndef _WIN32
#include <string>
#endif

extern bool g_isNative;

#ifdef _WIN32
class WinString;
#endif

struct UpdateInfo {
  bool updateAvailable;
#ifdef _WIN32
  const char* currentVersion;
  const char* latestVersion;
  const char* releaseNotes;
  const char* downloadUrl;
  const char* releaseApiUrl;
#else
  std::string currentVersion;
  std::string latestVersion;
  std::string releaseNotes;
  std::string downloadUrl;
  std::string releaseApiUrl;
#endif
  bool betaPreference = false;
};

enum class DownloadState {
  Idle,
  Connecting,
  Downloading,
  Installing,
  Complete,
  Error
};

#ifdef _WIN32
class WinString;
WinString ExtractJsonValue(const WinString& json, const char* key);
bool ExtractJsonBool(const WinString& json, const char* key);
WinString HttpGet(const char* url);
WinString GetAssetDownloadUrl(const char* releaseApiUrl, bool includeBeta);
#else
std::string ExtractJsonValue(const std::string& json, const std::string& key);
bool ExtractJsonBool(const std::string& json, const std::string& key);
std::string HttpGet(const std::string& url);
std::string GetAssetDownloadUrl(const std::string& releaseApiUrl, bool includeBeta);
#endif

class UpdateDialog {
public:
  static bool Show(NativeWindow parent, const UpdateInfo& info);
  static UpdateInfo currentInfo;

private:
  static RenderManager* renderer;
  static bool darkMode;
  static int hoveredButton;
  static int pressedButton;
  static int scrollOffset;
  static bool scrollbarHovered;
  static bool scrollbarPressed;
  static int scrollDragStart;
  static int scrollOffsetStart;
  static Rect updateButtonRect;
  static Rect skipButtonRect;
  static Rect closeButtonRect;
  static Rect scrollbarRect;
  static Rect scrollThumbRect;

  static void RenderContent(int width, int height);

#ifdef _WIN32
  static LRESULT CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
  static void OnPaint(HWND hWnd);
  static void OnMouseMove(HWND hWnd, int x, int y);
  static void OnMouseDown(HWND hWnd, int x, int y);
  static bool OnMouseUp(HWND hWnd, int x, int y);
#elif defined(__linux__)
  static void OnPaint();
  static void OnMouseMove(int x, int y);
  static void OnMouseDown(int x, int y);
  static bool OnMouseUp(int x, int y);
#elif defined(__APPLE__)
  static void OnPaint();
  static void OnMouseMove(int x, int y);
  static void OnMouseDown(int x, int y);
  static bool OnMouseUp(int x, int y);
#endif
};

#endif
