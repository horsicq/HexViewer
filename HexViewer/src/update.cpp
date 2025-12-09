#include "update.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <shellapi.h>
#elif __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#endif

RenderManager* UpdateDialog::renderer = nullptr;
UpdateInfo UpdateDialog::currentInfo = {};
bool UpdateDialog::darkMode = true;
int UpdateDialog::hoveredButton = 0;
int UpdateDialog::pressedButton = 0;
int UpdateDialog::scrollOffset = 0;
bool UpdateDialog::scrollbarHovered = false;
bool UpdateDialog::scrollbarPressed = false;
int UpdateDialog::scrollDragStart = 0;
int UpdateDialog::scrollOffsetStart = 0;
Rect UpdateDialog::updateButtonRect = {};
Rect UpdateDialog::skipButtonRect = {};
Rect UpdateDialog::closeButtonRect = {};
Rect UpdateDialog::scrollbarRect = {};
Rect UpdateDialog::scrollThumbRect = {};

#ifdef __linux__
static Display* display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#elif __APPLE__
static NSWindow* nsWindow = nullptr;
static bool windowShouldClose = false;
#endif

bool UpdateDialog::Show(NativeWindow parent, const UpdateInfo& info) {
  currentInfo = info;
  hoveredButton = 0;
  pressedButton = 0;
  scrollOffset = 0;

  int width = 600;
  int height = 500;

#ifdef _WIN32
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = DialogProc;
  wcex.hInstance = GetModuleHandle(nullptr);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"UpdateDialogClass";

  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    L"UpdateDialogClass",
    info.updateAvailable ? L"Update Available" : L"No Updates Available",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    (HWND)parent,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr
  );

  if (!hWnd) return false;

  if (parent) {
    RECT parentRect;
    GetWindowRect((HWND)parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }

  renderer = new RenderManager();
  if (!renderer->initialize(hWnd)) {
    DestroyWindow(hWnd);
    delete renderer;
    return false;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderer->resize(rc.right - rc.left, rc.bottom - rc.top);

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);

  MSG msg;
  bool result = false;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_QUIT || !IsWindow(hWnd)) {
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"UpdateDialogClass", GetModuleHandle(nullptr));

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

    NSString* title = info.updateAvailable ?
                      @"Update Available" : @"No Updates Available";
    [nsWindow setTitle:title] ;
    [nsWindow setLevel:NSFloatingWindowLevel] ;
    [nsWindow center] ;

    NSView* contentView = [[NSView alloc]initWithFrame:frame];
    [nsWindow setContentView:contentView] ;

    [nsWindow makeKeyAndOrderFront:nil] ;

    renderer = new RenderManager();
    if (!renderer->initialize((NativeWindow)nsWindow)) {
      [nsWindow close] ;
      nsWindow = nullptr;
      delete renderer;
      return false;
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

            case NSEventTypeScrollWheel: {
              CGFloat deltaY = [event scrollingDeltaY];
              scrollOffset -= (int)(deltaY * 2);
              scrollOffset = std::max(0, scrollOffset);
              OnPaint();
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

        std::this_thread::sleep_for(std::chrono::milliseconds(16); // ~60 FPS
      }
    }

    delete renderer;
    renderer = nullptr;
    [nsWindow close] ;
    nsWindow = nullptr;
  }

#elif __linux__
  display = XOpenDisplay(nullptr);
  if (!display) return false;

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

  const char* title = info.updateAvailable ? "Update Available" : "No Updates Available";
  XStoreName(display, window, title);
  XSetIconName(display, window, title);

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
    return false;
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
        else if (event.xbutton.button == Button4) { // Scroll up
          scrollOffset -= 20;
          scrollOffset = std::max(0, scrollOffset);
          OnPaint();
        }
        else if (event.xbutton.button == Button5) { // Scroll down
          scrollOffset += 20;
          OnPaint();
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

    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#endif

  return true;
}

#ifdef _WIN32
LRESULT CALLBACK UpdateDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
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

  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    scrollOffset -= (delta > 0 ? 20 : -20);
    scrollOffset = std::max(0, scrollOffset);
    InvalidateRect(hWnd, nullptr, FALSE);
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

void UpdateDialog::OnPaint(HWND hWnd) {
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

void UpdateDialog::OnMouseMove(HWND hWnd, int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (currentInfo.updateAvailable) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered) {
    InvalidateRect(hWnd, nullptr, FALSE);
  }
}

void UpdateDialog::OnMouseDown(HWND hWnd, int x, int y) {
  pressedButton = hoveredButton;

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
    SetCapture(hWnd);
  }

  InvalidateRect(hWnd, nullptr, FALSE);
}

bool UpdateDialog::OnMouseUp(HWND hWnd, int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
    ReleaseCapture();
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1) {
      ShellExecuteA(nullptr, "open", currentInfo.downloadUrl.c_str(),
        nullptr, nullptr, SW_SHOWNORMAL);
      DestroyWindow(hWnd);
      return true;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      DestroyWindow(hWnd);
      return true;
    }
  }

  pressedButton = 0;
  InvalidateRect(hWnd, nullptr, FALSE);
  return false;
}

#elif __linux__

void UpdateDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(600, 500);
  XFlush(display);
}

void UpdateDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (currentInfo.updateAvailable) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered) {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1) {
      std::string cmd = "xdg-open \"" + currentInfo.downloadUrl + "\"";
      system(cmd.c_str());
      return true;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#elif __APPLE__

void UpdateDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(600, 500);

  @autoreleasepool {
    [[nsWindow contentView]setNeedsDisplay:YES];
  }
}

void UpdateDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (currentInfo.updateAvailable) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered) {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1) {
      @autoreleasepool {
        NSString* urlString = [NSString stringWithUTF8String:currentInfo.downloadUrl.c_str()];
        NSURL* url = [NSURL URLWithString:urlString];
        [[NSWorkspace sharedWorkspace]openURL:url];
      }
      return true;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#endif

void UpdateDialog::RenderContent(int width, int height) {
  if (!renderer) return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();

  renderer->beginFrame();
  renderer->clear(theme.windowBackground);

  Rect headerRect(0, 0, width, 80);
  renderer->drawRect(headerRect, theme.menuBackground, true);

  Rect iconRect(20, 20, 40, 40);
  renderer->drawRect(iconRect, Color(100, 150, 255), true);
  renderer->drawText("i", 35, 32, Color::White());

  std::string title = currentInfo.updateAvailable ?
    "Update Available!" : "You're up to date!";
  renderer->drawText(title, 80, 25, theme.textColor);

  std::string versionText = "Current: " + currentInfo.currentVersion;
  if (currentInfo.updateAvailable) {
    versionText += " ? Latest: " + currentInfo.latestVersion;
  }
  renderer->drawText(versionText, 80, 50, theme.disabledText);

  renderer->drawLine(0, 80, width, 80, theme.separator);

  if (currentInfo.updateAvailable && !currentInfo.releaseNotes.empty()) {
    renderer->drawText("What's New:", 20, 100, theme.textColor);

    std::istringstream stream(currentInfo.releaseNotes);
    std::string line;
    int y = 130 - scrollOffset;
    int maxY = height - 100;

    while (std::getline(stream, line) && y < maxY) {
      if (y > 80) {
        renderer->drawText("� " + line, 30, y, theme.disabledText);
      }
      y += 25;
    }

    int contentHeight = y + scrollOffset - 130;
    if (contentHeight > (maxY - 130)) {
      scrollbarRect = Rect(width - 15, 100, 10, maxY - 100);
      renderer->drawRect(scrollbarRect, theme.scrollbarBg, true);

      float visibleRatio = (float)(maxY - 130) / contentHeight;
      int thumbHeight = std::max(30, (int)(scrollbarRect.height * visibleRatio));
      int maxScroll = contentHeight - (maxY - 130);
      int thumbY = scrollbarRect.y + (int)((scrollbarRect.height - thumbHeight) *
        (scrollOffset / (float)maxScroll));

      scrollThumbRect = Rect(scrollbarRect.x, thumbY, scrollbarRect.width, thumbHeight);
      
    }
  }
  else if (!currentInfo.updateAvailable) {
    renderer->drawText("You have the latest version installed.", 20, 120, theme.textColor);
    renderer->drawText("Check back later for updates.", 20, 150, theme.disabledText);
  }

  int buttonY = height - 60;
  int buttonHeight = 40;

  if (currentInfo.updateAvailable) {
    updateButtonRect = Rect(width - 280, buttonY, 120, buttonHeight);
    WidgetState updateState;
    updateState.rect = updateButtonRect;
    updateState.enabled = true;
    updateState.hovered = (hoveredButton == 1);
    updateState.pressed = (pressedButton == 1);
    renderer->drawModernButton(updateState, theme, "Update Now");

    skipButtonRect = Rect(width - 150, buttonY, 100, buttonHeight);
    WidgetState skipState;
    skipState.rect = skipButtonRect;
    skipState.enabled = true;
    skipState.hovered = (hoveredButton == 2);
    skipState.pressed = (pressedButton == 2);
    renderer->drawModernButton(skipState, theme, "Skip");
  }
  else {
    closeButtonRect = Rect((width - 120) / 2, buttonY, 120, buttonHeight);
    WidgetState closeState;
    closeState.rect = closeButtonRect;
    closeState.enabled = true;
    closeState.hovered = (hoveredButton == 3);
    closeState.pressed = (pressedButton == 3);
    renderer->drawModernButton(closeState, theme, "Close");
  }

  renderer->endFrame();
}