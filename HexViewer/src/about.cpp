#include "about.h"
#ifdef _WIN32
#elif __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <unistd.h>
#endif

RenderManager* AboutDialog::renderer = nullptr;
bool AboutDialog::darkMode = true;
NativeWindow AboutDialog::parentWindow = 0;
int AboutDialog::hoveredButton = 0;
int AboutDialog::pressedButton = 0;
Rect AboutDialog::updateButtonRect = {};
Rect AboutDialog::closeButtonRect = {};

#ifdef __linux__
static Display* display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#elif __APPLE__
static NSWindow* nsWindow = nullptr;
static bool windowShouldClose = false;
#endif

void AboutDialog::Show(NativeWindow parent, bool isDarkMode) {
  darkMode = isDarkMode;
  parentWindow = parent;
  hoveredButton = 0;
  pressedButton = 0;

  int width = 500;
  int height = 400;

#ifdef _WIN32
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = DialogProc;
  wcex.hInstance = GetModuleHandle(nullptr);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"AboutDialogClass";

  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    L"AboutDialogClass",
    L"About HexViewer",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    (HWND)parentWindow,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr
  );

  if (!hWnd) return;

  ApplyDarkTitleBar(hWnd, isDarkMode);

  RECT parentRect;
  GetWindowRect((HWND)parentWindow, &parentRect);
  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
  SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

  renderer = new RenderManager();
  if (!renderer->initialize(hWnd)) {
    DestroyWindow(hWnd);
    delete renderer;
    return;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderer->resize(rc.right - rc.left, rc.bottom - rc.top);

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_QUIT || !IsWindow(hWnd)) {
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"AboutDialogClass", GetModuleHandle(nullptr));

#elif __APPLE__
  @autoreleasepool{
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;

    nsWindow = [[NSWindow alloc]initWithContentRect:frame
                                           styleMask : styleMask
                                             backing : NSBackingStoreBuffered
                                               defer : NO];

    [nsWindow setTitle:@"About HexViewer"] ;
    [nsWindow setLevel:NSFloatingWindowLevel] ;
    [nsWindow center] ;

    ApplyDarkTitleBar(nsWindow, true);
    ApplyDarkMenu([NSApp mainMenu], true);

    NSView* contentView = [[NSView alloc]initWithFrame:frame];
    [nsWindow setContentView:contentView] ;

    [nsWindow makeKeyAndOrderFront:nil] ;

    renderer = new RenderManager();
    if (!renderer->initialize((NativeWindow)nsWindow)) {
      [nsWindow close] ;
      nsWindow = nullptr;
      delete renderer;
      return;
    }

    renderer->resize(width, height);

    bool running = true;
    windowShouldClose = false;

    while (running && !windowShouldClose) {
      @autoreleasepool {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate : [NSDate distantPast]
                                               inMode : NSDefaultRunLoopMode
                                              dequeue : YES];

        if (event) {
          NSEventType eventType = [event type];

          switch (eventType) {
            case NSEventTypeLeftMouseDown: {
              NSPoint point = [event locationInWindow];
              OnMouseDown((int)point.x, (int)point.y);
              break;
            }

            case NSEventTypeLeftMouseUp: {
              NSPoint point = [event locationInWindow];
              if (OnMouseUp((int)point.x, (int)point.y)) {
                running = false;
              }
              break;
            }

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged: {
              NSPoint point = [event locationInWindow];
              OnMouseMove((int)point.x, (int)point.y);
              break;
            }

            default:
              [NSApp sendEvent:event] ;
              break;
          }
        }

        if (![nsWindow isVisible]) {
          running = false;
        }

        OnPaint();
        std::chrono::milliseconds(15); // ~60 FPS
      }
    }

    delete renderer;
    renderer = nullptr;
    [nsWindow close] ;
    nsWindow = nullptr;
  }

#elif __linux__
  display = XOpenDisplay(nullptr);
  if (!display) return;

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
    ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
  attrs.background_pixel = WhitePixel(display, screen);

  window = XCreateWindow(
    display, root,
    0, 0, width, height, 0,
    CopyFromParent, InputOutput, CopyFromParent,
    CWBackPixel | CWEventMask,
    &attrs
    );

  XStoreName(display, window, "About HexViewer");
  XSetIconName(display, window, "About HexViewer");

  ApplyDarkTitleBar(display, window, true);
  ApplyDarkMenu(display, true);

  Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom wmWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(display, window, wmWindowType, XA_ATOM, 32, PropModeReplace,
    (unsigned char*)&wmWindowTypeDialog, 1);

  XSizeHints* sizeHints = XAllocSizeHints();
  sizeHints->flags = PMinSize | PMaxSize;
  sizeHints->min_width = sizeHints->max_width = width;
  sizeHints->min_height = sizeHints->max_height = height;
  XSetWMNormalHints(display, window, sizeHints);
  XFree(sizeHints);

  wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  int screenWidth = DisplayWidth(display, screen);
  int screenHeight = DisplayHeight(display, screen);
  XMoveWindow(display, window, (screenWidth - width) / 2, (screenHeight - height) / 2);

  XMapWindow(display, window);
  XFlush(display);

  renderer = new RenderManager();
  if (!renderer->initialize((NativeWindow)window)) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    delete renderer;
    return;
  }

  renderer->resize(width, height);

  bool running = true;
  XEvent event;

  while (running) {
    while (XPending(display)) {
      XNextEvent(display, &event);

      switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          OnPaint();
        }
        break;

      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
          running = false;
        }
        break;

      case MotionNotify:
        OnMouseMove(event.xmotion.x, event.xmotion.y);
        break;

      case ButtonPress:
        if (event.xbutton.button == Button1) {
          OnMouseDown(event.xbutton.x, event.xbutton.y);
        }
        break;

      case ButtonRelease:
        if (event.xbutton.button == Button1) {
          if (OnMouseUp(event.xbutton.x, event.xbutton.y)) {
            running = false;
          }
        }
        break;
      }
    }

    usleep(16000);
  }

  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);
#endif
}

#ifdef _WIN32
LRESULT CALLBACK AboutDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    break;

  case WM_ERASEBKGND:
    return 1;

  case WM_PAINT:
    OnPaint(hWnd);
    return 0;

  case WM_MOUSEMOVE: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseMove(hWnd, x, y);
    break;
  }

  case WM_LBUTTONDOWN: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseDown(hWnd, x, y);
    break;
  }

  case WM_LBUTTONUP: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseUp(hWnd, x, y);
    break;
  }

  case WM_CLOSE:
    DestroyWindow(hWnd);
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void AboutDialog::OnPaint(HWND hWnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  if (!renderer) {
    EndPaint(hWnd, &ps);
    return;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;

  RenderContent(width, height);

  EndPaint(hWnd, &ps);
}

void AboutDialog::OnMouseMove(HWND hWnd, int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
    y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton) {
    InvalidateRect(hWnd, nullptr, FALSE);
  }
}

void AboutDialog::OnMouseDown(HWND hWnd, int x, int y) {
  pressedButton = hoveredButton;
  InvalidateRect(hWnd, nullptr, FALSE);
}

bool AboutDialog::OnMouseUp(HWND hWnd, int x, int y) {
  if (pressedButton == hoveredButton && pressedButton == 1) {
    UpdateInfo info;
    info.currentVersion = "1.0.0";
    info.latestVersion = "1.0.0";
    info.updateAvailable = true;
    info.releaseNotes =
      "New Features:\n"
      "Added cross-platform menu bar support\n"
      "Improved rendering performance\n"
      "Added dark mode for all dialogs\n"
      "\n"
      "Bug Fixes:\n"
      "Fixed scrollbar flickering on Linux\n"
      "Fixed memory leak in hex editor\n"
      "Improved file loading speed";
      info.downloadUrl = "https://github.com/horsicq/HexViewer/releases/latest";

    ShowWindow(hWnd, SW_HIDE);
    UpdateDialog::Show(parentWindow, info);
    DestroyWindow(hWnd);
    return true;
  }

  pressedButton = 0;
  InvalidateRect(hWnd, nullptr, FALSE);
  return false;
}

#elif __linux__

void AboutDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(500, 400);
  XFlush(display);
}

void AboutDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
    y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton) {
    OnPaint();
  }
}

void AboutDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;
  OnPaint();
}

bool AboutDialog::OnMouseUp(int x, int y) {
  if (pressedButton == hoveredButton && pressedButton == 1) {
    UpdateInfo info;
    info.currentVersion = "1.0.0";
    info.latestVersion = "1.0.0";
    info.updateAvailable = true;
    info.releaseNotes =
      "New Features:\n"
      "Added cross-platform menu bar support\n"
      "Improved rendering performance\n"
      "Added dark mode for all dialogs\n"
      "\n"
      "Bug Fixes:\n"
      "Fixed scrollbar flickering on Linux\n"
      "Fixed memory leak in hex editor\n"
      "Improved file loading speed";
    info.downloadUrl = "https://github.com/horsicq/HexViewer/releases/latest";

    UpdateDialog::Show(parentWindow, info);
    return true;
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#elif __APPLE__

void AboutDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(500, 400);

  @autoreleasepool {
    [[nsWindow contentView]setNeedsDisplay:YES];
  }
}

void AboutDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
    y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton) {
    OnPaint();
  }
}

void AboutDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;
  OnPaint();
}

bool AboutDialog::OnMouseUp(int x, int y) {
  if (pressedButton == hoveredButton && pressedButton == 1) {
    UpdateInfo info;
    info.currentVersion = "1.0.0";
    info.latestVersion = "1.0.0";
    info.updateAvailable = true;
    info.releaseNotes =
      "New Features:\n"
      "Added cross-platform menu bar support\n"
      "Improved rendering performance\n"
      "Added dark mode for all dialogs\n"
      "\n"
      "Bug Fixes:\n"
      "Fixed scrollbar flickering on Linux\n"
      "Fixed memory leak in hex editor\n"
      "Improved file loading speed";
    info.downloadUrl = "https://github.com/horsicq/HexViewer/releases/latest";

    UpdateDialog::Show(parentWindow, info);
    return true;
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#endif

void AboutDialog::RenderContent(int width, int height) {
  if (!renderer) return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();

  renderer->beginFrame();
  renderer->clear(theme.windowBackground);

  std::string version = "Version 1.2.0";
  int versionX = (width - (version.length() * 8)) / 2;
  renderer->drawText(version, versionX, 30, theme.textColor);

  std::string desc = "A modern cross-platform hex editor";
  int descX = (desc.length() * 8) / 2;
  renderer->drawText(desc, width / 2 - descX, 60, theme.disabledText);

  int featuresY = 100;
  renderer->drawText("Features:", 40, featuresY, theme.textColor);
  renderer->drawText("- Cross-platform support", 50, featuresY + 30, theme.disabledText);
  renderer->drawText("- Real-time hex editing", 50, featuresY + 55, theme.disabledText);
  renderer->drawText("- Dark mode support", 50, featuresY + 80, theme.disabledText);

  renderer->drawLine(0, height - 90, width, height - 90, theme.separator);

  int buttonY = height - 65;
  int buttonHeight = 40;
  int buttonWidth = 180;
  int buttonX = (width - buttonWidth) / 2;

  updateButtonRect = Rect(buttonX, buttonY, buttonWidth, buttonHeight);
  WidgetState updateState;
  updateState.rect = updateButtonRect;
  updateState.enabled = true;
  updateState.hovered = (hoveredButton == 1);
  updateState.pressed = (pressedButton == 1);
  renderer->drawModernButton(updateState, theme, "Check for Updates");

  std::string copyright = "2025 Hors";
  int copyrightX = (width - (copyright.length() * 8)) / 2;
  renderer->drawText(copyright, copyrightX, height - 15, theme.disabledText);

  renderer->endFrame();
}