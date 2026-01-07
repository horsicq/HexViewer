#if defined(_WIN32)
#include <windows.h>
#include <wininet.h>

#elif defined(__APPLE__)
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>
#include <thread>

#endif

#include <global.h>
#include <language.h>
#include <about.h>
#include "imageloader.h"
#include "resource.h"

RenderManager *AboutDialog::renderer = nullptr;
bool AboutDialog::darkMode = true;
NativeWindow AboutDialog::parentWindow = 0;
int AboutDialog::hoveredButton = 0;
int AboutDialog::pressedButton = 0;
Rect AboutDialog::updateButtonRect = {};
Rect AboutDialog::closeButtonRect = {};
bool AboutDialog::betaEnabled = false;
Rect AboutDialog::betaToggleRect = {};
bool AboutDialog::betaToggleHovered = false;

#ifdef _WIN32
static HBITMAP logoBitmap = nullptr;
static int logoWidth = 0;
static int logoHeight = 0;

struct UpdateCheckParams
{
  HWND parentWnd;
  bool checkBeta;
  char currentVersion[32];
  char releaseApiUrl[512];
};

static void CleanReleaseNotes(char *text)
{
  char *src = text;
  char *dst = text;

  while (*src)
  {
    if (src[0] == '\\' && src[1] == 'u')
    {
      char hex[5] = {src[2], src[3], src[4], src[5], 0};
      int code = StrHexToInt(hex);

      switch (code)
      {
      case 0x2022:
        *dst++ = '-';
        *dst++ = ' ';
        break;
      case 0x2013:
      case 0x2014:
        *dst++ = '-';
        break;
      case 0x00A0:
      case 0x2003:
      case 0x2002:
        *dst++ = ' ';
        break;
      default:
        if (code < 128)
          *dst++ = (char)code;
        break;
      }

      src += 6;
      continue;
    }

    unsigned char c0 = (unsigned char)src[0];
    unsigned char c1 = (unsigned char)src[1];
    unsigned char c2 = (unsigned char)src[2];

    if (c0 == 0xE2 && c1 == 0x80)
    {
      if (c2 == 0xA2)
      {
        *dst++ = '-';
        *dst++ = ' ';
        src += 3;
        continue;
      }
      if (c2 == 0x93)
      {
        *dst++ = '-';
        src += 3;
        continue;
      }
      if (c2 == 0x94)
      {
        *dst++ = '-';
        src += 3;
        continue;
      }
      if (c2 == 0x83)
      {
        *dst++ = ' ';
        src += 3;
        continue;
      }
    }

    if (c0 == 0xC2 && c1 == 0xA0)
    {
      *dst++ = ' ';
      src += 2;
      continue;
    }

    if (c0 >= 0xC0)
    {
      if (c0 < 0xE0)
        src += 2;
      else if (c0 < 0xF0)
        src += 3;
      else
        src += 4;
      continue;
    }

    *dst++ = *src++;
  }

  *dst = '\0';

  src = text;
  dst = text;
  bool lastSpace = false;

  while (*src)
  {
    if (*src == ' ')
    {
      if (!lastSpace)
        *dst++ = ' ';
      lastSpace = true;
    }
    else
    {
      *dst++ = *src;
      lastSpace = false;
    }
    src++;
  }

  *dst = '\0';
}

static DWORD WINAPI UpdateCheckThread(LPVOID lpParam)
{
  UpdateCheckParams *params = (UpdateCheckParams *)lpParam;

  UpdateInfo *info = (UpdateInfo *)PlatformAlloc(sizeof(UpdateInfo));
  if (!info)
  {
    PlatformFree(params);
    return 0;
  }

  info->currentVersion = params->currentVersion;
  info->releaseApiUrl = params->releaseApiUrl;
  info->updateAvailable = false;
  info->latestVersion = "Checking...";
  info->releaseNotes = "Please wait while we check for updates...";

  WinString response = HttpGet(info->releaseApiUrl);

  if (!response.IsEmpty())
  {
    char *releaseNameStr = nullptr;

    if (params->checkBeta)
    {
      int urlPos = response.Find("{\"url\"");
      if (urlPos >= 0)
      {
        const char *releaseStart = response.CStr() + urlPos;
        const char *releaseEnd = releaseStart;
        int braceCount = 0;
        while (*releaseEnd)
        {
          if (*releaseEnd == '{')
            braceCount++;
          if (*releaseEnd == '}')
          {
            braceCount--;
            if (braceCount == 0)
            {
              releaseEnd++;
              break;
            }
          }
          releaseEnd++;
        }

        int len = (int)(releaseEnd - releaseStart);
        char *firstRelease = (char *)PlatformAlloc(len + 1);
        if (firstRelease)
        {
          MemCopy(firstRelease, releaseStart, len);
          firstRelease[len] = '\0';

          WinString tempStr;
          tempStr.Append(firstRelease);

          WinString nameResult = ExtractJsonValue(tempStr, "name");
          if (!nameResult.IsEmpty())
          {
            size_t nameLen = nameResult.Length();
            releaseNameStr = (char *)PlatformAlloc(nameLen + 1);
            if (releaseNameStr)
            {
              MemCopy(releaseNameStr, nameResult.CStr(), nameLen);
              releaseNameStr[nameLen] = '\0';
            }
          }

          PlatformFree(firstRelease);
        }
      }
    }

    if (releaseNameStr)
    {
      const char *vPos = releaseNameStr;
      while (*vPos && *vPos != 'v')
        vPos++;

      if (*vPos == 'v')
      {
        vPos++;
        const char *spacePos = vPos;
        while (*spacePos && *spacePos != ' ')
          spacePos++;

        int versionLen = (int)(spacePos - vPos);
        if (versionLen > 0)
        {
          char *version = (char *)PlatformAlloc(versionLen + 1);
          if (version)
          {
            MemCopy(version, vPos, versionLen);
            version[versionLen] = '\0';
            info->latestVersion = version;
          }
        }
      }
      PlatformFree(releaseNameStr);
    }
    else
    {
      WinString tagName = ExtractJsonValue(response, "tag_name");
      if (!tagName.IsEmpty())
      {
        const char *tagStr = tagName.CStr();
        size_t tagLen = tagName.Length();

        if (tagStr[0] == 'v' && tagLen > 1)
        {
          char *version = (char *)PlatformAlloc(tagLen);
          if (version)
          {
            MemCopy(version, tagStr + 1, tagLen - 1);
            version[tagLen - 1] = '\0';
            info->latestVersion = version;
          }
        }
        else
        {
          char *version = (char *)PlatformAlloc(tagLen + 1);
          if (version)
          {
            MemCopy(version, tagStr, tagLen);
            version[tagLen] = '\0';
            info->latestVersion = version;
          }
        }
      }
    }

    WinString releaseNotes = ExtractJsonValue(response, "body");

    if (!releaseNotes.IsEmpty())
    {
      size_t len = releaseNotes.Length();
      char *notesCopy = (char *)PlatformAlloc(len + 1);

      if (notesCopy)
      {
        MemCopy(notesCopy, releaseNotes.CStr(), len);
        notesCopy[len] = '\0';

        CleanReleaseNotes(notesCopy);

        info->releaseNotes = notesCopy;
      }

      info->updateAvailable =
          !StrEquals(info->latestVersion, info->currentVersion) &&
          !StringIsEmpty(info->latestVersion);
    }
    else
    {
      info->updateAvailable = false;
      info->latestVersion = info->currentVersion;
      info->releaseNotes =
          "Unable to check for updates. Please try again later.";
    }

    UpdateDialog::Show(params->parentWnd, *info);

    PlatformFree(info);
    PlatformFree(params);
    return 0;
  }
}

#elif __APPLE__
unsigned char *logoPixels = nullptr;
int logoPixelsWidth = 0;
int logoPixelsHeight = 0;
static NSImage *logoImage = nil;
#elif __linux__
unsigned char *logoPixels = nullptr;
int logoPixelsWidth = 0;
int logoPixelsHeight = 0;
static Pixmap logoPixmap = 0;
static int logoWidth = 0;
static int logoHeight = 0;
static Display *display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#endif

void AboutDialog::Show(NativeWindow parent, bool isDarkMode)
{
  darkMode = isDarkMode;
  parentWindow = parent;
  hoveredButton = 0;
  pressedButton = 0;
  betaToggleHovered = false;

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
      L"About",
      WS_POPUP | WS_CAPTION | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT,
      width, height,
      (HWND)parentWindow,
      nullptr,
      GetModuleHandleW(nullptr),
      nullptr);
  if (!hWnd)
    return;

  ApplyDarkTitleBar(hWnd, isDarkMode);

  RECT parentRect;
  GetWindowRect((HWND)parentWindow, &parentRect);
  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
  SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

  renderer = new RenderManager();
  if (!renderer->initialize(hWnd))
  {
    DestroyWindow(hWnd);
    delete renderer;
    return;
  }
  RECT rc;
  GetClientRect(hWnd, &rc);
  renderer->resize(rc.right - rc.left, rc.bottom - rc.top);

  logoBitmap = ImageLoader::LoadPNGDecoded(
      reinterpret_cast<const char *>(MAKEINTRESOURCE(IDB_LOGO)),
      &logoWidth,
      &logoHeight);

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (msg.message == WM_QUIT || !IsWindow(hWnd))
      break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (logoBitmap)
  {
    DeleteObject(logoBitmap);
    logoBitmap = nullptr;
  }
  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"AboutDialogClass", GetModuleHandle(nullptr));

#elif __APPLE__
  @autoreleasepool
  {
    NSRect frame = NSMakeRect(0, 0, width, height);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
    NSWindow *nsWindow = [[NSWindow alloc] initWithContentRect:frame styleMask:styleMask backing:NSBackingStoreBuffered defer:NO];
    [nsWindow setTitle:@"About HexViewer"];
    [nsWindow setLevel:NSFloatingWindowLevel];
    [nsWindow center];

    renderer = new RenderManager();
    if (!renderer->initialize((NativeWindow)nsWindow))
    {
      [nsWindow close];
      delete renderer;
      return;
    }
    renderer->resize(width, height);

    logoImage = ImageLoader::LoadPNGDecoded("about.png");

    [nsWindow makeKeyAndOrderFront:nil];

    bool running = true;
    while (running)
    {
      @autoreleasepool
      {
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];
        if (event)
        {
          NSPoint point = [event locationInWindow];
          switch ([event type])
          {
          case NSEventTypeLeftMouseDown:
            OnMouseDown((int)point.x, (int)point.y);
            break;
          case NSEventTypeLeftMouseUp:
            if (OnMouseUp((int)point.x, (int)point.y))
              running = false;
            break;
          case NSEventTypeMouseMoved:
          case NSEventTypeLeftMouseDragged:
            OnMouseMove((int)point.x, (int)point.y);
            break;
          default:
            [NSApp sendEvent:event];
            break;
          }
        }
        if (![nsWindow isVisible])
          running = false;
        OnPaint();
      }
    }

    if (logoImage)
    {
      [logoImage release];
      logoImage = nullptr;
    }
    delete renderer;
    renderer = nullptr;
    [nsWindow close];
  }

#elif __linux__
  display = XOpenDisplay(nullptr);
  if (!display)
    return;

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
                  (unsigned char *)&wmWindowTypeDialog, 1);

  XSizeHints *sizeHints = XAllocSizeHints();
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
  if (!renderer->initialize((NativeWindow)window))
  {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    delete renderer;
    return;
  }
  renderer->resize(width, height);

  ImageLoader::Buffer *decodedBuffer = nullptr;
  if (ImageLoader::LoadPNG("about.png", &decodedBuffer) && decodedBuffer)
  {
    logoWidth = 100;
    logoHeight = 100;
    logoPixelsWidth = logoWidth;
    logoPixelsHeight = logoHeight;

    logoPixels = new unsigned char[logoWidth * logoHeight * 4];

    for (int i = 0; i < logoWidth * logoHeight * 4; i += 4)
    {
      logoPixels[i] = 100;
      logoPixels[i + 1] = 150;
      logoPixels[i + 2] = 255;
      logoPixels[i + 3] = 255;
    }

    ImageLoader::BufferFree(decodedBuffer);

    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);

    logoPixmap = XCreatePixmap(display, window, logoWidth, logoHeight, depth);
    GC gc = XCreateGC(display, logoPixmap, 0, nullptr);

    Theme theme = isDarkMode ? Theme::Dark() : Theme::Light();
    uint8_t bgR = theme.windowBackground.r;
    uint8_t bgG = theme.windowBackground.g;
    uint8_t bgB = theme.windowBackground.b;

    XImage *ximage = XCreateImage(display, visual, depth, ZPixmap, 0, nullptr,
                                  logoWidth, logoHeight, 32, 0);
    ximage->data = (char *)malloc(logoHeight * ximage->bytes_per_line);

    for (int y = 0; y < logoHeight; y++)
    {
      for (int x = 0; x < logoWidth; x++)
      {
        int idx = (y * logoWidth + x) * 4;
        uint8_t r = logoPixels[idx];
        uint8_t g = logoPixels[idx + 1];
        uint8_t b = logoPixels[idx + 2];
        uint8_t a = logoPixels[idx + 3];

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
  while (running)
  {
    while (XPending(display))
    {
      XNextEvent(display, &event);
      switch (event.type)
      {
      case Expose:
        if (event.xexpose.count == 0)
          OnPaint();
        break;
      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteWindow)
          running = false;
        break;
      case MotionNotify:
        OnMouseMove(event.xmotion.x, event.xmotion.y);
        break;
      case ButtonPress:
        if (event.xbutton.button == Button1)
          OnMouseDown(event.xbutton.x, event.xbutton.y);
        break;
      case ButtonRelease:
        if (event.xbutton.button == Button1 && OnMouseUp(event.xbutton.x, event.xbutton.y))
          running = false;
        break;
      }
    }
    usleep(16000);
  }

  if (logoPixmap)
  {
    XFreePixmap(display, logoPixmap);
    logoPixmap = 0;
  }
  if (logoPixels)
  {
    delete[] logoPixels;
    logoPixels = nullptr;
  }
  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#endif
}

void AboutDialog::RenderContent(int width, int height)
{
  if (!renderer)
    return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();
  renderer->clear(theme.windowBackground);

  int logoPadding = 20;
  int logoSize = 100;

#ifdef _WIN32
#elif __APPLE__
  if (logoImage)
  {
    renderer->drawImage(logoImage, logoSize, logoSize, logoPadding, logoPadding);
  }
#elif __linux__
  if (logoPixmap)
  {
    renderer->drawX11Pixmap(logoPixmap, logoWidth, logoHeight, logoPadding, logoPadding);
  }
#endif

  int contentX = logoPadding + logoSize + 30;
  int contentY = logoPadding;

  char appName[] = "HexViewer";
  renderer->drawText(appName, contentX, contentY, theme.textColor);

  char version[64];
  StringCopy(version, Translations::T("Version"), sizeof(version));
  StrCat(version, " 1.0.0");
  renderer->drawText(version, contentX, contentY + 25, theme.disabledText);

  renderer->drawText(Translations::T("A modern cross-platform hex editor"), contentX, contentY + 50, theme.disabledText);

  int featuresY = logoPadding + logoSize + 30;
  renderer->drawText(Translations::T("Features:"), 40, featuresY, theme.textColor);

  char feature1[128];
  StringCopy(feature1, "- ", sizeof(feature1));
  StrCat(feature1, Translations::T("Cross-platform support"));
  renderer->drawText(feature1, 50, featuresY + 30, theme.disabledText);

  char feature2[128];
  StringCopy(feature2, "- ", sizeof(feature2));
  StrCat(feature2, Translations::T("Real-time hex editing"));
  renderer->drawText(feature2, 50, featuresY + 55, theme.disabledText);

  char feature3[128];
  StringCopy(feature3, "- ", sizeof(feature3));
  StrCat(feature3, Translations::T("Dark mode support"));
  renderer->drawText(feature3, 50, featuresY + 80, theme.disabledText);

  renderer->drawLine(0, height - 120, width, height - 120, theme.separator);

  int toggleY = height - 100;
  betaToggleRect = Rect(40, toggleY, 200, 25);

  Rect checkboxRect(betaToggleRect.x, betaToggleRect.y, 18, 18);
  Color checkboxBorder = betaToggleHovered ? Color(100, 150, 255) : theme.disabledText;
  renderer->drawRect(checkboxRect, theme.windowBackground, true);
  renderer->drawRect(checkboxRect, checkboxBorder, false);

  if (betaEnabled)
  {
    renderer->drawRect(Rect(checkboxRect.x + 3, checkboxRect.y + 3, 12, 12),
                       Color(100, 150, 255), true);
  }

  renderer->drawText(Translations::T("Include Beta Versions"),
                     betaToggleRect.x + 23, betaToggleRect.y + 2, theme.disabledText);

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

  char copyright[] = "(c) 2025 DiE team!";
  int copyrightX = (width - (StringLength(copyright) * 8)) / 2;
  renderer->drawText(copyright, copyrightX, height - 20, theme.disabledText);
}

#ifdef _WIN32
void AboutDialog::OnPaint(HWND hWnd)
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  if (renderer)
  {
    RECT rc;
    GetClientRect(hWnd, &rc);

    renderer->beginFrame();
    RenderContent(rc.right - rc.left, rc.bottom - rc.top);
    renderer->endFrame(hdc);

    if (logoBitmap)
    {
      int logoPadding = 20;
      int targetSize = 100;

      int scaledWidth = targetSize;
      int scaledHeight = targetSize;

      if (logoWidth > logoHeight)
        scaledHeight = (targetSize * logoHeight) / logoWidth;
      else if (logoHeight > logoWidth)
        scaledWidth = (targetSize * logoWidth) / logoHeight;

      ImageLoader::DrawTransparentBitmap(hdc, logoBitmap,
                                         logoPadding, logoPadding,
                                         scaledWidth, scaledHeight);
    }
  }

  EndPaint(hWnd, &ps);
}

void AboutDialog::OnMouseMove(HWND hWnd, int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
  {
    betaToggleHovered = true;
  }

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
  {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton || oldBetaHovered != betaToggleHovered)
  {
    InvalidateRect(hWnd, nullptr, FALSE);
  }
}

void AboutDialog::OnMouseDown(HWND hWnd, int x, int y)
{
  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    InvalidateRect(hWnd, nullptr, FALSE);
    return;
  }

  pressedButton = hoveredButton;
  InvalidateRect(hWnd, nullptr, FALSE);
}

bool AboutDialog::OnMouseUp(HWND hWnd, int x, int y)
{
  if (pressedButton == hoveredButton && pressedButton == 1)
  {
    ShowWindow(hWnd, SW_HIDE);

    UpdateCheckParams *params = (UpdateCheckParams *)PlatformAlloc(sizeof(UpdateCheckParams));
    if (params)
    {
      params->parentWnd = parentWindow;
      params->checkBeta = betaEnabled;
      StringCopy(params->currentVersion, "1.0.0", sizeof(params->currentVersion));

      if (betaEnabled)
      {
        StringCopy(params->releaseApiUrl, "https://api.github.com/repos/horsicq/HexViewer/releases", sizeof(params->releaseApiUrl));
      }
      else
      {
        StringCopy(params->releaseApiUrl, "https://api.github.com/repos/horsicq/HexViewer/releases/latest", sizeof(params->releaseApiUrl));
      }

      HANDLE hThread = CreateThread(nullptr, 0, UpdateCheckThread, params, 0, nullptr);
      if (hThread)
      {
        CloseHandle(hThread);
      }
      else
      {
        PlatformFree(params);
      }
    }

    DestroyWindow(hWnd);
    return true;
  }

  pressedButton = 0;
  InvalidateRect(hWnd, nullptr, FALSE);
  return false;
}

LRESULT CALLBACK AboutDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
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

  return DefWindowProcW(hWnd, message, wParam, lParam);
}

#elif __linux__

void AboutDialog::OnPaint()
{
  if (!renderer)
    return;
  RenderContent(550, 480);
  XClearWindow(display, window);
  XSync(display, False);
}

void AboutDialog::OnMouseMove(int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
  {
    betaToggleHovered = true;
  }

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
  {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton || oldBetaHovered != betaToggleHovered)
  {
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

void AboutDialog::OnMouseDown(int x, int y)
{
  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    XClearWindow(display, window);
    XEvent exposeEvent;
    memset(&exposeEvent, 0, sizeof(exposeEvent));
    exposeEvent.type = Expose;
    exposeEvent.xexpose.window = window;
    exposeEvent.xexpose.count = 0;
    XSendEvent(display, window, False, ExposureMask, &exposeEvent);
    XFlush(display);
    return;
  }

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

bool AboutDialog::OnMouseUp(int x, int y)
{
  if (pressedButton == hoveredButton && pressedButton == 1)
  {
    UpdateInfo info;
    info.currentVersion = "1.0.0";

    if (betaEnabled)
    {
      info.releaseApiUrl = "https://api.github.com/repos/horsicq/HexViewer/releases";
    }
    else
    {
      info.releaseApiUrl = "https://api.github.com/repos/horsicq/HexViewer/releases/latest";
    }

    std::string response = HttpGet(info.releaseApiUrl);

    if (!response.empty())
    {
      if (betaEnabled)
      {
        size_t releaseStart = response.find("{\"url\"");
        if (releaseStart != std::string::npos)
        {
          std::string firstRelease = response.substr(releaseStart);
          size_t releaseEnd = firstRelease.find("},");
          if (releaseEnd == std::string::npos)
          {
            releaseEnd = firstRelease.find("}]");
          }
          if (releaseEnd != std::string::npos)
          {
            response = firstRelease.substr(0, releaseEnd + 1);
          }
        }
      }

      info.latestVersion = ExtractJsonValue(response, "tag_name");
      if (!info.latestVersion.empty() && info.latestVersion[0] == 'v')
      {
        info.latestVersion = info.latestVersion.substr(1);
      }

      info.releaseNotes = ExtractJsonValue(response, "body");
      info.updateAvailable = (info.latestVersion != info.currentVersion && !info.latestVersion.empty());
    }
    else
    {
      info.updateAvailable = false;
      info.latestVersion = info.currentVersion;
      info.releaseNotes = "Unable to check for updates. Please try again later.";
    }

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

void AboutDialog::OnPaint()
{
  if (!renderer)
    return;
  RenderContent(550, 480);
  @autoreleasepool
  {
    [[nsWindow contentView] setNeedsDisplay:YES];
  }
}

void AboutDialog::OnMouseMove(int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
  {
    betaToggleHovered = true;
  }

  if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
  {
    hoveredButton = 1;
  }

  if (oldHovered != hoveredButton || oldBetaHovered != betaToggleHovered)
  {
    OnPaint();
  }
}

void AboutDialog::OnMouseDown(int x, int y)
{
  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    OnPaint();
    return;
  }

  pressedButton = hoveredButton;
  OnPaint();
}

bool AboutDialog::OnMouseUp(int x, int y)
{
  if (pressedButton == hoveredButton && pressedButton == 1)
  {
    UpdateInfo info;
    info.currentVersion = "1.0.0";

    if (betaEnabled)
    {
      info.releaseApiUrl = "https://api.github.com/repos/horsicq/HexViewer/releases";
    }
    else
    {
      info.releaseApiUrl = "https://api.github.com/repos/horsicq/HexViewer/releases/latest";
    }

    std::string response = HttpGet(info.releaseApiUrl);

    if (!response.empty())
    {
      if (betaEnabled)
      {
        size_t releaseStart = response.find("{\"url\"");
        if (releaseStart != std::string::npos)
        {
          std::string firstRelease = response.substr(releaseStart);
          size_t releaseEnd = firstRelease.find("},");
          if (releaseEnd == std::string::npos)
          {
            releaseEnd = firstRelease.find("}]");
          }
          if (releaseEnd != std::string::npos)
          {
            response = firstRelease.substr(0, releaseEnd + 1);
          }
        }
      }

      info.latestVersion = ExtractJsonValue(response, "tag_name");
      if (!info.latestVersion.empty() && info.latestVersion[0] == 'v')
      {
        info.latestVersion = info.latestVersion.substr(1);
      }

      info.releaseNotes = ExtractJsonValue(response, "body");
      info.updateAvailable = (info.latestVersion != info.currentVersion && !info.latestVersion.empty());
    }
    else
    {
      info.updateAvailable = false;
      info.latestVersion = info.currentVersion;
      info.releaseNotes = "Unable to check for updates. Please try again later.";
    }

    UpdateDialog::Show(parentWindow, info);
    return true;
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#endif
