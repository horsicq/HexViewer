#ifndef ABOUT_H
#define ABOUT_H

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
typedef HWND NativeWindow;
#elif defined(__APPLE__)
typedef void* NativeWindow;
#elif defined(__linux__)
#include <X11/Xlib.h>
typedef Window NativeWindow;
#endif

#include <render.h>
#include <update.h>
#include <darkmode.h>
#include <imageloader.h>

class AboutDialog {
public:
  static void Show(NativeWindow parent, bool isDarkMode);
private:
  static RenderManager* renderer;
  static bool darkMode;
  static NativeWindow parentWindow;
  static int hoveredButton;
  static int pressedButton;
  static Rect updateButtonRect;
  static Rect closeButtonRect;

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

#endif // ABOUT_H
