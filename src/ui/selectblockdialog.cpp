#include "selectblockdialog.h"
#include <language.h>

#ifdef __linux__
#include <unistd.h>
#endif

static SelectBlockDialogData* g_selectBlockData = nullptr;

static bool IsPointInRect_SB(int x, int y, const Rect& rect)
{
  return x >= rect.x &&
    x <= rect.x + rect.width &&
    y >= rect.y &&
    y <= rect.y + rect.height;
}

void RenderSelectBlockDialog(SelectBlockDialogData* data, int windowWidth, int windowHeight)
{
  if (!data || !data->renderer)
    return;

#ifdef _WIN32
  HDC hdc = GetDC(data->platformWindow.hwnd);
#endif

  data->renderer->beginFrame();
  Theme theme = data->renderer->getCurrentTheme();
  data->renderer->clear(theme.windowBackground);

  int margin = 20;
  int y = margin;

  static DWORD lastBlinkTime = 0;
  static bool cursorVisible = true;
  DWORD currentTime = GetTickCount();

  if (currentTime - lastBlinkTime > 500)
  {
    cursorVisible = !cursorVisible;
    lastBlinkTime = currentTime;
  }

  data->renderer->drawText(Translations::T("Start-offset:"), margin, y + 8, theme.textColor);
  y += 28;

  Rect startBox(margin, y, windowWidth - margin * 2, 30);
  bool startActive = (data->activeTextBox == 0);

  Color startBg = startActive ? Color(70, 70, 75) : Color(55, 55, 60);
  Color startBorder = startActive ? Color(0, 120, 215) : Color(90, 90, 95);

  data->renderer->drawRoundedRect(startBox, 4.0f, startBg, true);
  data->renderer->drawRoundedRect(startBox, 4.0f, startBorder, false);

#ifdef _WIN32
  char startDisplay[256];
  StringCopy(startDisplay, data->startOffsetText, 256);
  if (startActive && cursorVisible)
    StringAppend(startDisplay, '|', 256);
  data->renderer->drawText(startDisplay, startBox.x + 8, startBox.y + 8, Color(240, 240, 240));
#else
  std::string startDisplay = data->startOffsetText;
  if (startActive && cursorVisible)
    startDisplay += "|";
  data->renderer->drawText(startDisplay.c_str(), startBox.x + 8, startBox.y + 8, Color(240, 240, 240));
#endif

  y += 50;
  WidgetState endRadio(Rect(margin, y + 2, 16, 16));
  endRadio.hovered = (data->hoveredWidget == 10);
  data->renderer->drawModernRadioButton(endRadio, theme, data->selectedMode == 0);

  data->renderer->drawText(Translations::T("End-offset:"), margin + 26, y, theme.textColor);
  y += 30;

  Rect endBox(margin, y, windowWidth - margin * 2, 30);
  bool endEnabled = (data->selectedMode == 0);
  bool endActive = (data->activeTextBox == 1 && endEnabled);

  Color endBg = endEnabled ? (endActive ? Color(70, 70, 75) : Color(55, 55, 60)) : Color(40, 40, 45);
  Color endBorder = endEnabled ? (endActive ? Color(0, 120, 215) : Color(90, 90, 95)) : Color(60, 60, 65);
  Color endText = endEnabled ? Color(240, 240, 240) : Color(120, 120, 120);

  data->renderer->drawRoundedRect(endBox, 4.0f, endBg, true);
  data->renderer->drawRoundedRect(endBox, 4.0f, endBorder, false);

#ifdef _WIN32
  char endDisplay[256];
  StringCopy(endDisplay, data->endOffsetText, 256);
  if (endActive && cursorVisible)
    StringAppend(endDisplay, '|', 256);
  data->renderer->drawText(endDisplay, endBox.x + 8, endBox.y + 8, endText);
#else
  std::string endDisplay = data->endOffsetText;
  if (endActive && cursorVisible)
    endDisplay += "|";
  data->renderer->drawText(endDisplay.c_str(), endBox.x + 8, endBox.y + 8, endText);
#endif

  y += 50;
  WidgetState lengthRadio(Rect(margin, y + 2, 16, 16));
  lengthRadio.hovered = (data->hoveredWidget == 11);
  data->renderer->drawModernRadioButton(lengthRadio, theme, data->selectedMode == 1);

  data->renderer->drawText(Translations::T("Length:"), margin + 26, y, theme.textColor);
  y += 30;

  Rect lengthBox(margin, y, windowWidth - margin * 2, 30);
  bool lengthEnabled = (data->selectedMode == 1);
  bool lengthActive = (data->activeTextBox == 2 && lengthEnabled);

  Color lengthBg = lengthEnabled ? (lengthActive ? Color(70, 70, 75) : Color(55, 55, 60)) : Color(40, 40, 45);
  Color lengthBorder = lengthEnabled ? (lengthActive ? Color(0, 120, 215) : Color(90, 90, 95)) : Color(60, 60, 65);
  Color lengthText = lengthEnabled ? Color(240, 240, 240) : Color(120, 120, 120);

  data->renderer->drawRoundedRect(lengthBox, 4.0f, lengthBg, true);
  data->renderer->drawRoundedRect(lengthBox, 4.0f, lengthBorder, false);

#ifdef _WIN32
  char lengthDisplay[256];
  StringCopy(lengthDisplay, data->lengthText, 256);
  if (lengthActive && cursorVisible)
    StringAppend(lengthDisplay, '|', 256);
  data->renderer->drawText(lengthDisplay, lengthBox.x + 8, lengthBox.y + 8, lengthText);
#else
  std::string lengthDisplay = data->lengthText;
  if (lengthActive && cursorVisible)
    lengthDisplay += "|";
  data->renderer->drawText(lengthDisplay.c_str(), lengthBox.x + 8, lengthBox.y + 8, lengthText);
#endif

  y += 55;
  int formatSpacing = 60;
  int formatY = y;

  WidgetState hexRadio(Rect(margin, formatY + 2, 16, 16));
  hexRadio.hovered = (data->hoveredWidget == 20);
  data->renderer->drawModernRadioButton(hexRadio, theme, data->selectedRadio == 0);
  data->renderer->drawText("hex", margin + 26, formatY, theme.textColor);

  WidgetState decRadio(Rect(margin + formatSpacing, formatY + 2, 16, 16));
  decRadio.hovered = (data->hoveredWidget == 21);
  data->renderer->drawModernRadioButton(decRadio, theme, data->selectedRadio == 1);
  data->renderer->drawText("dec", margin + formatSpacing + 26, formatY, theme.textColor);

  WidgetState octRadio(Rect(margin + formatSpacing * 2, formatY + 2, 16, 16));
  octRadio.hovered = (data->hoveredWidget == 22);
  data->renderer->drawModernRadioButton(octRadio, theme, data->selectedRadio == 2);
  data->renderer->drawText("oct", margin + formatSpacing * 2 + 26, formatY, theme.textColor);

  y += 70;
  int buttonY = windowHeight - margin - 25;
  int buttonWidth = 90;
  int buttonSpacing = 10;

  Rect okButton(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, 30);
  Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

  WidgetState okState(okButton);
  okState.hovered = (data->hoveredWidget == 0);
  okState.pressed = (data->pressedWidget == 0);
  okState.enabled = true;
  data->renderer->drawModernButton(okState, theme, Translations::T("OK"));

  WidgetState cancelState(cancelButton);
  cancelState.hovered = (data->hoveredWidget == 1);
  cancelState.pressed = (data->pressedWidget == 1);
  cancelState.enabled = true;
  data->renderer->drawModernButton(cancelState, theme, Translations::T("Cancel"));

#ifdef _WIN32
  data->renderer->endFrame(hdc);
  ReleaseDC(data->platformWindow.hwnd, hdc);
#else
  data->renderer->endFrame(data->renderer->getDrawContext());
#endif
}

void UpdateSelectBlockHover(SelectBlockDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  int margin = 20;
  int buttonY = windowHeight - margin - 35;
  int buttonWidth = 90;
  int buttonSpacing = 10;

  Rect okButton(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, 30);
  Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

  data->hoveredWidget = -1;
  if (IsPointInRect_SB(x, y, okButton))
    data->hoveredWidget = 0;
  else if (IsPointInRect_SB(x, y, cancelButton))
    data->hoveredWidget = 1;
}

void HandleSelectBlockClick(SelectBlockDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  int margin = 20;
  int radioSize = 16;
  int y_pos = margin + 28 + 45;

  Rect endRadioArea(margin, y_pos, 100, 20);
  if (IsPointInRect_SB(x, y, endRadioArea))
  {
    data->selectedMode = 0;
    data->activeTextBox = 1;
    return;
  }

  y_pos += 28;
  Rect endBox(margin, y_pos, windowWidth - margin * 2, 30);
  if (IsPointInRect_SB(x, y, endBox) && data->selectedMode == 0)
  {
    data->activeTextBox = 1;
    return;
  }

  y_pos += 45;
  Rect lengthRadioArea(margin, y_pos, 80, 20);
  if (IsPointInRect_SB(x, y, lengthRadioArea))
  {
    data->selectedMode = 1;
    data->activeTextBox = 2;
    return;
  }

  y_pos += 28;
  Rect lengthBox(margin, y_pos, windowWidth - margin * 2, 30);
  if (IsPointInRect_SB(x, y, lengthBox) && data->selectedMode == 1)
  {
    data->activeTextBox = 2;
    return;
  }

  Rect startBox(margin, margin + 28, windowWidth - margin * 2, 30);
  if (IsPointInRect_SB(x, y, startBox))
  {
    data->activeTextBox = 0;
    return;
  }

  int formatRadioY = y_pos + 45;
  int formatSpacing = 80;

  Rect hexRadioArea(margin, formatRadioY, 60, 20);
  if (IsPointInRect_SB(x, y, hexRadioArea))
  {
    data->selectedRadio = 0;
    return;
  }

  Rect decRadioArea(margin + formatSpacing, formatRadioY, 60, 20);
  if (IsPointInRect_SB(x, y, decRadioArea))
  {
    data->selectedRadio = 1;
    return;
  }

  Rect octRadioArea(margin + formatSpacing * 2, formatRadioY, 60, 20);
  if (IsPointInRect_SB(x, y, octRadioArea))
  {
    data->selectedRadio = 2;
    return;
  }

  if (data->hoveredWidget == 0)
  {
    data->dialogResult = true;
    data->running = false;
    if (data->callback)
    {
#ifdef _WIN32
      data->callback(data->startOffsetText,
        data->selectedMode == 0 ? data->endOffsetText : data->lengthText,
        data->selectedMode == 1, data->selectedRadio);
#else
      data->callback(data->startOffsetText,
        data->selectedMode == 0 ? data->endOffsetText : data->lengthText,
        data->selectedMode == 1, data->selectedRadio);
#endif
    }
  }
  else if (data->hoveredWidget == 1)
  {
    data->dialogResult = false;
    data->running = false;
  }
}

void HandleSelectBlockChar(SelectBlockDialogData* data, char ch, int windowWidth, int windowHeight)
{
  if (ch == '\b' || ch == 8)
  {
#ifdef _WIN32
    if (data->activeTextBox == 0 && !StringIsEmpty(data->startOffsetText))
    {
      StringRemoveLast(data->startOffsetText);
    }
    else if (data->activeTextBox == 1 && data->selectedMode == 0 && !StringIsEmpty(data->endOffsetText))
    {
      StringRemoveLast(data->endOffsetText);
    }
    else if (data->activeTextBox == 2 && data->selectedMode == 1 && !StringIsEmpty(data->lengthText))
    {
      StringRemoveLast(data->lengthText);
    }
#else
    if (data->activeTextBox == 0 && !data->startOffsetText.empty())
    {
      data->startOffsetText.pop_back();
    }
    else if (data->activeTextBox == 1 && data->selectedMode == 0 && !data->endOffsetText.empty())
    {
      data->endOffsetText.pop_back();
    }
    else if (data->activeTextBox == 2 && data->selectedMode == 1 && !data->lengthText.empty())
    {
      data->lengthText.pop_back();
    }
#endif
  }
  else if (ch == '\r' || ch == '\n')
  {
    data->hoveredWidget = 0;
    HandleSelectBlockClick(data, 0, 0, windowWidth, windowHeight);
  }
  else if (ch == 27)
  {
    data->dialogResult = false;
    data->running = false;
  }
  else if ((ch >= '0' && ch <= '9') ||
    (ch >= 'A' && ch <= 'F') ||
    (ch >= 'a' && ch <= 'f') ||
    (ch == 'x' || ch == 'X'))
  {
#ifdef _WIN32
    if (data->activeTextBox == 0)
    {
      StringAppend(data->startOffsetText, ch, 256);
    }
    else if (data->activeTextBox == 1 && data->selectedMode == 0)
    {
      StringAppend(data->endOffsetText, ch, 256);
    }
    else if (data->activeTextBox == 2 && data->selectedMode == 1)
    {
      StringAppend(data->lengthText, ch, 256);
    }
#else
    if (data->activeTextBox == 0)
    {
      data->startOffsetText += ch;
    }
    else if (data->activeTextBox == 1 && data->selectedMode == 0)
    {
      data->endOffsetText += ch;
    }
    else if (data->activeTextBox == 2 && data->selectedMode == 1)
    {
      data->lengthText += ch;
    }
#endif
  }
}

#ifdef _WIN32

LRESULT CALLBACK SelectBlockWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  SelectBlockDialogData* data = g_selectBlockData;

  switch (msg)
  {
  case WM_TIMER:
  {
    if (wParam == 1)
    {
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    BeginPaint(hwnd, &ps);
    if (data && data->renderer)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      RenderSelectBlockDialog(data, rect.right, rect.bottom);
    }
    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_CHAR:
  {
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      HandleSelectBlockChar(data, (char)wParam, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_KEYDOWN:
  {
    if (data && wParam == VK_TAB)
    {
      if (data->selectedMode == 0)
      {
        data->activeTextBox = (data->activeTextBox == 0) ? 1 : 0;
      }
      else
      {
        data->activeTextBox = (data->activeTextBox == 0) ? 2 : 0;
      }
      InvalidateRect(hwnd, NULL, FALSE);
      return 0;
    }
    break;
  }

  case WM_MOUSEMOVE:
  {
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      UpdateSelectBlockHover(data, x, y, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      data->pressedWidget = data->hoveredWidget;
      HandleSelectBlockClick(data, x, y, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONUP:
  {
    if (data)
    {
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

void ShowSelectBlockDialog(void* parentHandle, bool darkMode,
  void (*callback)(const char*, const char*, bool, int),
  void* userData,
  long long initialStart,
  long long initialLength)
{
  HWND parent = (HWND)parentHandle;
  const wchar_t* className = L"SelectBlockDialogClass";

  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = SelectBlockWindowProc;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;

  UnregisterClassW(className, wc.hInstance);
  RegisterClassExW(&wc);

  SelectBlockDialogData data = {};
  data.running = true;
  data.activeTextBox = 0;
  data.selectedRadio = 0;
  data.callback = callback;
  data.callbackUserData = userData;

  if (initialStart >= 0)
  {
    char hexStart[32];
    ItoaHex(initialStart, hexStart, 32);
    StringCopy(data.startOffsetText, hexStart, 256);

    if (initialLength > 0)
    {
      long long endOffset = initialStart + initialLength;

      char hexEnd[32];
      ItoaHex(endOffset, hexEnd, 32);
      StringCopy(data.endOffsetText, hexEnd, 256);

      char hexLength[32];
      ItoaHex(initialLength, hexLength, 32);
      StringCopy(data.lengthText, hexLength, 256);

      data.selectedMode = 0;
      data.activeTextBox = 1;
    }
    else
    {
      long long endOffset = initialStart + 1;

      char hexEnd[32];
      ItoaHex(endOffset, hexEnd, 32);
      StringCopy(data.endOffsetText, hexEnd, 256);

      data.lengthText[0] = '1';
      data.lengthText[1] = 0;

      data.selectedMode = 0;
      data.activeTextBox = 1;
    }

  }
  else
  {
    data.selectedMode = 0;
    data.startOffsetText[0] = 0;
    data.endOffsetText[0] = 0;
    data.lengthText[0] = '1';
    data.lengthText[1] = 0;
  }

  g_selectBlockData = &data;

  int width = 250;
  int height = 380;

  RECT parentRect;
  GetWindowRect(parent, &parentRect);
  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

  HWND hwnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    className, L"Select block",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    x, y, width, height,
    parent, nullptr, GetModuleHandleW(NULL), nullptr);

  if (!hwnd)
  {
    g_selectBlockData = nullptr;
    return;
  }

  if (darkMode)
  {
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
  }

  data.renderer = new RenderManager();
  if (!data.renderer->initialize(hwnd))
  {
    delete data.renderer;
    DestroyWindow(hwnd);
    g_selectBlockData = nullptr;
    return;
  }

  data.platformWindow.hwnd = hwnd;

  RECT clientRect;
  GetClientRect(hwnd, &clientRect);
  if (clientRect.right > 0 && clientRect.bottom > 0)
  {
    data.renderer->resize(clientRect.right, clientRect.bottom);
  }

  EnableWindow(parent, FALSE);
  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  SetTimer(hwnd, 1, 500, nullptr);

  MSG msg;
  while (data.running && GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  KillTimer(hwnd, 1);

  delete data.renderer;
  EnableWindow(parent, TRUE);
  SetForegroundWindow(parent);
  DestroyWindow(hwnd);
  UnregisterClassW(className, GetModuleHandleW(NULL));
  g_selectBlockData = nullptr;
}

#elif defined(__linux__)

void ProcessSelectBlockEvent(SelectBlockDialogData* data, XEvent* event, int width, int height)
{
  switch (event->type)
  {
  case Expose:
    RenderSelectBlockDialog(data, width, height);
    break;

  case KeyPress:
  {
    char buf[32];
    KeySym keysym;
    int len = XLookupString(&event->xkey, buf, sizeof(buf), &keysym, nullptr);

    if (keysym == XK_Tab)
    {
      if (data->selectedMode == 0)
        data->activeTextBox = (data->activeTextBox == 0) ? 1 : 0;
      else
        data->activeTextBox = (data->activeTextBox == 0) ? 2 : 0;
      RenderSelectBlockDialog(data, width, height);
    }
    else if (len > 0)
    {
      HandleSelectBlockChar(data, buf[0], width, height);
      RenderSelectBlockDialog(data, width, height);
    }
    break;
  }

  case MotionNotify:
  {
    UpdateSelectBlockHover(data, event->xmotion.x, event->xmotion.y, width, height);
    RenderSelectBlockDialog(data, width, height);
    break;
  }

  case ButtonPress:
    if (event->xbutton.button == Button1)
    {
      data->pressedWidget = data->hoveredWidget;
      HandleSelectBlockClick(data, event->xbutton.x, event->xbutton.y, width, height);
      RenderSelectBlockDialog(data, width, height);
    }
    break;

  case ButtonRelease:
    if (event->xbutton.button == Button1)
    {
      data->pressedWidget = -1;
      RenderSelectBlockDialog(data, width, height);
    }
    break;

  case ClientMessage:
    if ((Atom)event->xclient.data.l[0] == data->platformWindow.wmDeleteWindow)
    {
      data->dialogResult = false;
      data->running = false;
    }
    break;

  case ConfigureNotify:
    if (event->xconfigure.width != width || event->xconfigure.height != height)
    {
      data->renderer->resize(event->xconfigure.width, event->xconfigure.height);
      RenderSelectBlockDialog(data, event->xconfigure.width, event->xconfigure.height);
    }
    break;
  }
}

void ShowSelectBlockDialog(void* parentHandle, bool darkMode,
  std::function<void(const std::string&, const std::string&, bool, int)> callback,
  long long initialStart,
  long long initialLength)
{
  Window parentWindow = (Window)(uintptr_t)parentHandle;
  Display* parentDisplay = XOpenDisplay(nullptr);
  if (!parentDisplay)
  {
    fprintf(stderr, "Failed to open display\n");
    return;
  }

  int screen = DefaultScreen(parentDisplay);
  Window rootWindow = RootWindow(parentDisplay, screen);

  SelectBlockDialogData data = {};
  data.callback = callback;
  data.selectedRadio = 0;

  if (initialStart >= 0)
  {
    char hexStart[32];
    ItoaHex(initialStart, hexStart, 32);
    data.startOffsetText = hexStart;

    if (initialLength > 0)
    {
      long long endOffset = initialStart + initialLength;

      char hexEnd[32];
      ItoaHex(endOffset, hexEnd, 32);
      data.endOffsetText = hexEnd;

      char hexLength[32];
      ItoaHex(initialLength, hexLength, 32);
      data.lengthText = hexLength;

      data.selectedMode = 0;
      data.activeTextBox = 1;
    }
    else
    {
      data.selectedMode = 1;
      data.activeTextBox = 2;
      data.lengthText = "1";
      data.endOffsetText = "";
    }
  }
  else
  {
    data.selectedMode = 0;
    data.activeTextBox = 0;
    data.startOffsetText = "";
    data.endOffsetText = "";
    data.lengthText = "1";
  }

  g_selectBlockData = &data;

  int width = 250;
  int height = 340;

  Window window = XCreateSimpleWindow(parentDisplay, rootWindow,
    100, 100, width, height, 1,
    BlackPixel(parentDisplay, screen),
    WhitePixel(parentDisplay, screen));

  XSizeHints* sizeHints = XAllocSizeHints();
  sizeHints->flags = PPosition | PSize | PMinSize | PMaxSize;
  sizeHints->x = 100;
  sizeHints->y = 100;
  sizeHints->width = width;
  sizeHints->height = height;
  sizeHints->min_width = width;
  sizeHints->min_height = height;
  sizeHints->max_width = width;
  sizeHints->max_height = height;
  XSetWMNormalHints(parentDisplay, window, sizeHints);
  XFree(sizeHints);
  XSelectInput(parentDisplay, window,
    ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask | StructureNotifyMask);

  Atom wmDelete = XInternAtom(parentDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(parentDisplay, window, &wmDelete, 1);
  XStoreName(parentDisplay, window, "Select block");

  if (parentWindow != 0)
  {
    XSetTransientForHint(parentDisplay, window, parentWindow);
  }

  XMapWindow(parentDisplay, window);
  XFlush(parentDisplay);

  data.platformWindow.display = parentDisplay;
  data.platformWindow.window = window;
  data.platformWindow.wmDeleteWindow = wmDelete;

  data.renderer = new RenderManager();
  if (!data.renderer->initialize((NativeWindow)(uintptr_t)window))
  {
    delete data.renderer;
    XDestroyWindow(parentDisplay, window);
    XCloseDisplay(parentDisplay);
    g_selectBlockData = nullptr;
    return;
  }

  data.renderer->resize(width, height);

  XEvent event;
  while (data.running)
  {
    while (XPending(parentDisplay))
    {
      XNextEvent(parentDisplay, &event);
      ProcessSelectBlockEvent(&data, &event, width, height);
    }
    usleep(1000);
  }

  delete data.renderer;
  XDestroyWindow(parentDisplay, window);
  XCloseDisplay(parentDisplay);
  g_selectBlockData = nullptr;
}
#endif
