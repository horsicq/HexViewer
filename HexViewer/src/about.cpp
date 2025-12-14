#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <unistd.h>
#endif

#include <fstream>
#include <language.h>
#include <about.h>
#include <resource.h>
#include <imageloader.h>

RenderManager* AboutDialog::renderer = nullptr;
bool AboutDialog::darkMode = true;
NativeWindow AboutDialog::parentWindow = 0;
int AboutDialog::hoveredButton = 0;
int AboutDialog::pressedButton = 0;
Rect AboutDialog::updateButtonRect = {};
Rect AboutDialog::closeButtonRect = {};

unsigned char* logoPixels = nullptr;
int logoPixelsWidth = 0;
int logoPixelsHeight = 0;

#ifdef _WIN32
static HBITMAP logoBitmap = nullptr;
int logoWidth = 0;
int logoHeight = 0;
#elif __APPLE__
static NSImage* logoImage = nil;
#elif __linux__
static Pixmap logoPixmap = 0;
static int logoWidth = 0;
static int logoHeight = 0;
static Display* display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#endif


void AboutDialog::Show(NativeWindow parent, bool isDarkMode) {
  darkMode = isDarkMode;
  parentWindow = parent;
  hoveredButton = 0;
  pressedButton = 0;

  int width = 550;
  int height = 480;

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

  std::vector<uint8_t> decoded;
  uint32_t decodeWidth = 0, decodeHeight = 0;

  constexpr int DESIRED_LOGO_WIDTH = 100;
  constexpr int DESIRED_LOGO_HEIGHT = 100;

  if (ImageLoader::LoadPNGDecoded(reinterpret_cast<const char*>(MAKEINTRESOURCE(IDB_LOGO)),
    decoded, decodeWidth, decodeHeight)) {
    if (decodeWidth != DESIRED_LOGO_WIDTH || decodeHeight != DESIRED_LOGO_HEIGHT) {
      std::vector<uint8_t> resized(DESIRED_LOGO_WIDTH * DESIRED_LOGO_HEIGHT * 4);

      auto clamp = [](int value, int minVal, int maxVal) {
        return (value < minVal) ? minVal : (value > maxVal ? maxVal : value);
        };

      for (int y = 0; y < DESIRED_LOGO_HEIGHT; ++y) {
        float srcY = float(y) * decodeHeight / DESIRED_LOGO_HEIGHT;
        int y0 = clamp(int(srcY), 0, int(decodeHeight) - 1);
        int y1 = clamp(y0 + 1, 0, int(decodeHeight) - 1);
        float fy = srcY - y0;

        for (int x = 0; x < DESIRED_LOGO_WIDTH; ++x) {
          float srcX = float(x) * decodeWidth / DESIRED_LOGO_WIDTH;
          int x0 = clamp(int(srcX), 0, int(decodeWidth) - 1);
          int x1 = clamp(x0 + 1, 0, int(decodeWidth) - 1);
          float fx = srcX - x0;

          for (int c = 0; c < 4; ++c) {
            float val =
              decoded[(y0 * decodeWidth + x0) * 4 + c] * (1 - fx) * (1 - fy) +
              decoded[(y0 * decodeWidth + x1) * 4 + c] * fx * (1 - fy) +
              decoded[(y1 * decodeWidth + x0) * 4 + c] * (1 - fx) * fy +
              decoded[(y1 * decodeWidth + x1) * 4 + c] * fx * fy;

            resized[(y * DESIRED_LOGO_WIDTH + x) * 4 + c] = uint8_t(val);
          }
        }
      }

      decoded = std::move(resized);
      decodeWidth = DESIRED_LOGO_WIDTH;
      decodeHeight = DESIRED_LOGO_HEIGHT;
    }

    logoBitmap = ImageLoader::CreateHBITMAP(decoded,
      int(decodeWidth),
      int(decodeHeight));
    logoWidth = int(decodeWidth);
    logoHeight = int(decodeHeight);
  }

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_QUIT || !IsWindow(hWnd)) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (logoBitmap) {
    DeleteObject(logoBitmap);
    logoBitmap = nullptr;
  }
  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"AboutDialogClass", GetModuleHandle(nullptr));

#elif __APPLE__
  @autoreleasepool{
      NSRect frame = NSMakeRect(0, 0, width, height);
      NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
      NSWindow* nsWindow = [[NSWindow alloc]initWithContentRect:frame styleMask : styleMask backing : NSBackingStoreBuffered defer : NO];
      [nsWindow setTitle:@"About HexViewer"] ;
      [nsWindow setLevel:NSFloatingWindowLevel] ;
      [nsWindow center] ;

      renderer = new RenderManager();
      if (!renderer->initialize((NativeWindow)nsWindow)) {
          [nsWindow close] ;
          delete renderer;
          return;
      }
      renderer->resize(width, height);

      std::vector<uint8_t> logoData;
      if (ImageLoader::LoadPNG("about.png", logoData)) {
          NSData* data = [NSData dataWithBytes:logoData.data() length : logoData.size()];
          logoImage = [[NSImage alloc]initWithData:data];
      }

      [nsWindow makeKeyAndOrderFront:nil];

      bool running = true;
      while (running) {
          @autoreleasepool {
              NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                untilDate : [NSDate distantPast]
                                                   inMode : NSDefaultRunLoopMode
                                                  dequeue : YES];
              if (event) {
                  NSPoint point = [event locationInWindow];
                  switch ([event type]) {
                      case NSEventTypeLeftMouseDown: OnMouseDown((int)point.x,(int)point.y); break;
                      case NSEventTypeLeftMouseUp: if (OnMouseUp((int)point.x,(int)point.y)) running = false; break;
                      case NSEventTypeMouseMoved:
                      case NSEventTypeLeftMouseDragged: OnMouseMove((int)point.x,(int)point.y); break;
                      default: [NSApp sendEvent:event] ; break;
                  }
              }
              if (![nsWindow isVisible]) running = false;
              OnPaint();
          }
      }

      if (logoImage) {
          [logoImage release] ;
          logoImage = nullptr;
      }
      delete renderer;
      renderer = nullptr;
      [nsWindow close] ;
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

  window = XCreateWindow(display, root, 0, 0, width, height, 0, CopyFromParent,
    InputOutput, CopyFromParent, CWBackPixel | CWEventMask, &attrs);
  XStoreName(display, window, "About HexViewer");

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

  std::vector<uint8_t> decoded;
  uint32_t decodeWidth = 0, decodeHeight = 0;

  constexpr int DESIRED_LOGO_WIDTH = 100;
  constexpr int DESIRED_LOGO_HEIGHT = 100;

  if (ImageLoader::LoadPNGDecoded("about.png", decoded, decodeWidth, decodeHeight)) {
    if (decodeWidth != DESIRED_LOGO_WIDTH || decodeHeight != DESIRED_LOGO_HEIGHT) {
      std::vector<uint8_t> resized(DESIRED_LOGO_WIDTH * DESIRED_LOGO_HEIGHT * 4);

      auto clamp = [](int value, int minVal, int maxVal) {
        return (value < minVal) ? minVal : (value > maxVal ? maxVal : value);
        };

      for (int y = 0; y < DESIRED_LOGO_HEIGHT; ++y) {
        float srcY = float(y) * decodeHeight / DESIRED_LOGO_HEIGHT;
        int y0 = clamp(int(srcY), 0, int(decodeHeight) - 1);
        int y1 = clamp(y0 + 1, 0, int(decodeHeight) - 1);
        float fy = srcY - y0;

        for (int x = 0; x < DESIRED_LOGO_WIDTH; ++x) {
          float srcX = float(x) * decodeWidth / DESIRED_LOGO_WIDTH;
          int x0 = clamp(int(srcX), 0, int(decodeWidth) - 1);
          int x1 = clamp(x0 + 1, 0, int(decodeWidth) - 1);
          float fx = srcX - x0;

          for (int c = 0; c < 4; ++c) {
            float val =
              decoded[(y0 * decodeWidth + x0) * 4 + c] * (1 - fx) * (1 - fy) +
              decoded[(y0 * decodeWidth + x1) * 4 + c] * fx * (1 - fy) +
              decoded[(y1 * decodeWidth + x0) * 4 + c] * (1 - fx) * fy +
              decoded[(y1 * decodeWidth + x1) * 4 + c] * fx * fy;

            resized[(y * DESIRED_LOGO_WIDTH + x) * 4 + c] = uint8_t(val);
          }
        }
      }

      decoded = std::move(resized);
      decodeWidth = DESIRED_LOGO_WIDTH;
      decodeHeight = DESIRED_LOGO_HEIGHT;
    }

    logoWidth = int(decodeWidth);
    logoHeight = int(decodeHeight);

    logoPixelsWidth = logoWidth;
    logoPixelsHeight = logoHeight;
    logoPixels = new unsigned char[decoded.size()];
    std::copy(decoded.begin(), decoded.end(), logoPixels);

    Visual* visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);

    logoPixmap = XCreatePixmap(display, window, logoWidth, logoHeight, depth);
    GC gc = XCreateGC(display, logoPixmap, 0, nullptr);

    Theme theme = isDarkMode ? Theme::Dark() : Theme::Light();
    uint8_t bgR = theme.windowBackground.r;
    uint8_t bgG = theme.windowBackground.g;
    uint8_t bgB = theme.windowBackground.b;

    XImage* ximage = XCreateImage(display, visual, depth, ZPixmap, 0, nullptr,
      logoWidth, logoHeight, 32, 0);
    ximage->data = (char*)malloc(logoHeight * ximage->bytes_per_line);

    for (int y = 0; y < logoHeight; y++) {
      for (int x = 0; x < logoWidth; x++) {
        int idx = (y * logoWidth + x) * 4;
        uint8_t r = decoded[idx];
        uint8_t g = decoded[idx + 1];
        uint8_t b = decoded[idx + 2];
        uint8_t a = decoded[idx + 3];

        r = (r * a + bgR * (255 - a)) / 255;
        g = (g * a + bgG * (255 - a)) / 255;
        b = (b * a + bgB * (255 - a)) / 255;

        unsigned long pixel = (r << 16) | (g << 8) | b;
        XPutPixel(ximage, x, y, pixel);
      }
    }

    XPutImage(display, logoPixmap, gc, ximage, 0, 0, 0, 0, logoWidth, logoHeight);

    XFreeGC(display, gc);
    free(ximage->data);
    ximage->data = nullptr;
    XDestroyImage(ximage);
  }

  bool running = true;
  XEvent event;
  while (running) {
    while (XPending(display)) {
      XNextEvent(display, &event);
      switch (event.type) {
      case Expose: if (event.xexpose.count == 0) OnPaint(); break;
      case ClientMessage: if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) running = false; break;
      case MotionNotify: OnMouseMove(event.xmotion.x, event.xmotion.y); break;
      case ButtonPress: if (event.xbutton.button == Button1) OnMouseDown(event.xbutton.x, event.xbutton.y); break;
      case ButtonRelease: if (event.xbutton.button == Button1 && OnMouseUp(event.xbutton.x, event.xbutton.y)) running = false; break;
      }
    }
    usleep(16000);
  }

  if (logoPixmap) {
    XFreePixmap(display, logoPixmap);
    logoPixmap = 0;
  }
  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#endif
}

void AboutDialog::RenderContent(int width, int height) {
  if (!renderer) return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();
  renderer->beginFrame();
  renderer->clear(theme.windowBackground);

  int logoPadding = 20;
  int logoSize = 100;

#ifdef _WIN32
#elif __APPLE__
  if (logoImage) {
    renderer->drawImage(logoImage, logoSize, logoSize, logoPadding, logoPadding);
  }
#elif __linux__
  if (logoPixmap) {
    renderer->drawX11Pixmap(logoPixmap, logoWidth, logoHeight, logoPadding, logoPadding);
  }
#endif

  int contentX = logoPadding + logoSize + 30;
  int contentY = logoPadding;

  std::string appName = "HexViewer";
  renderer->drawText(appName, contentX, contentY, theme.textColor);

  std::string version = std::string(Translations::T("Version")) + " 1.0.0";
  renderer->drawText(version, contentX, contentY + 25, theme.disabledText);

  std::string desc = Translations::T("A modern cross-platform hex editor");
  renderer->drawText(desc, contentX, contentY + 50, theme.disabledText);

  int featuresY = logoPadding + logoSize + 30;
  renderer->drawText(Translations::T("Features:"), 40, featuresY, theme.textColor);
  renderer->drawText(std::string("- ") + Translations::T("Cross-platform support"), 50, featuresY + 30, theme.disabledText);
  renderer->drawText(std::string("- ") + Translations::T("Real-time hex editing"), 50, featuresY + 55, theme.disabledText);
  renderer->drawText(std::string("- ") + Translations::T("Dark mode support"), 50, featuresY + 80, theme.disabledText);

  renderer->drawLine(0, height - 90, width, height - 90, theme.separator);

  int buttonY = height - 60;
  int buttonHeight = 35;
  int buttonWidth = 160;
  int buttonX = (width - buttonWidth) / 2;

  updateButtonRect = Rect(buttonX, buttonY, buttonWidth, buttonHeight);
  WidgetState updateState;
  updateState.rect = updateButtonRect;
  updateState.enabled = true;
  updateState.hovered = (hoveredButton == 1);
  updateState.pressed = (pressedButton == 1);
  renderer->drawModernButton(updateState, theme, Translations::T("Check for Updates"));

  std::string copyright =  "\u00A9 2025 DiE team!";
  int copyrightX = (width - (copyright.length() * 8)) / 2;
  renderer->drawText(copyright, copyrightX, height - 20, theme.disabledText);

  renderer->endFrame();
}

#ifdef _WIN32
void AboutDialog::OnPaint(HWND hWnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  if (renderer) {
    RECT rc;
    GetClientRect(hWnd, &rc);

    RenderContent(rc.right - rc.left, rc.bottom - rc.top);

    if (logoBitmap) {
      int logoPadding = 20;
      ImageLoader::DrawTransparentBitmap(hdc, logoBitmap, logoPadding, logoPadding, logoWidth, logoHeight);
    }
  }

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
    info.latestVersion = "1.2.0";
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

LRESULT CALLBACK AboutDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_ERASEBKGND:
    return 1;

  case WM_PAINT:
    OnPaint(hWnd);
    return 0;

  case WM_MOUSEMOVE:
    OnMouseMove(hWnd, LOWORD(lParam), HIWORD(lParam));
    break;

  case WM_LBUTTONDOWN:
    OnMouseDown(hWnd, LOWORD(lParam), HIWORD(lParam));
    break;

  case WM_LBUTTONUP:
    OnMouseUp(hWnd, LOWORD(lParam), HIWORD(lParam));
    break;

  case WM_CLOSE:
    DestroyWindow(hWnd);
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

#elif __linux__

void AboutDialog::OnPaint() {
  if (!renderer) return;
  RenderContent(550, 480);
  XClearWindow(display, window);
  XSync(display, False);
}

void AboutDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  hoveredButton = 0;

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
    y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton) {
    XClearWindow(display, window);
    XEvent exposeEvent;
    memset(&exposeEvent, 0, sizeof(exposeEvent));
    exposeEvent.type = Expose;
    exposeEvent.xexpose.window = window;
    exposeEvent.xexpose.count = 0;
    XSendEvent(display, window, False, ExposureMask, &exposeEvent);
    XFlush(display);
  }
}

void AboutDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;
  XClearWindow(display, window);
  XEvent exposeEvent;
  memset(&exposeEvent, 0, sizeof(exposeEvent));
  exposeEvent.type = Expose;
  exposeEvent.xexpose.window = window;
  exposeEvent.xexpose.count = 0;
  XSendEvent(display, window, False, ExposureMask, &exposeEvent);
  XFlush(display);
}

bool AboutDialog::OnMouseUp(int x, int y) {
  if (pressedButton == hoveredButton && pressedButton == 1) {
    UpdateInfo info;
    info.currentVersion = "1.0.0";
    info.latestVersion = "1.2.0";
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
  XClearWindow(display, window);
  XEvent exposeEvent;
  memset(&exposeEvent, 0, sizeof(exposeEvent));
  exposeEvent.type = Expose;
  exposeEvent.xexpose.window = window;
  exposeEvent.xexpose.count = 0;
  XSendEvent(display, window, False, ExposureMask, &exposeEvent);
  XFlush(display);

  return false;
}

#elif __APPLE__

void AboutDialog::OnPaint() {
  if (!renderer) return;
  RenderContent(550, 480);
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
    info.latestVersion = "1.2.0";
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

