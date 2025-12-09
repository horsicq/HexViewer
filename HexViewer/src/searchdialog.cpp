#include "render.h"
#include <functional>
#include <string>
#include "darkmode.h"

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#elif defined(__APPLE__)
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
typedef void* id;
typedef void* Class;
typedef void* SEL;
#elif defined(__linux__)
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif

namespace SearchDialogs {

  struct PlatformWindow {
#ifdef _WIN32
    HWND hwnd;
#elif defined(__APPLE__)
    id window;
    id view;
#elif defined(__linux__)
    Display* display;
    Window window;
    Atom wmDeleteWindow;
#endif
  };

  struct FindReplaceDialogData {
    std::string findText;
    std::string replaceText;
    int activeTextBox = 0;
    int hoveredWidget = -1;
    int pressedWidget = -1;
    bool dialogResult = false;
    bool running = true;
    RenderManager* renderer = nullptr;
    PlatformWindow platformWindow = {};
    std::function<void(const std::string&, const std::string&)> callback;
  };

  struct GoToDialogData {
    std::string lineNumberText;
    int activeTextBox = 0;
    int hoveredWidget = -1;
    int pressedWidget = -1;
    bool dialogResult = false;
    bool running = true;
    RenderManager* renderer = nullptr;
    PlatformWindow platformWindow = {};
    std::function<void(int)> callback;
  };

  static FindReplaceDialogData* g_findReplaceData = nullptr;
  static GoToDialogData* g_goToData = nullptr;

  inline bool IsPointInRect(int x, int y, const Rect& rect) {
    return x >= rect.x && x <= rect.x + rect.width &&
      y >= rect.y && y <= rect.y + rect.height;
  }


  void RenderFindReplaceDialog(FindReplaceDialogData* data, int windowWidth, int windowHeight) {
    if (!data || !data->renderer) return;

    data->renderer->beginFrame();
    Theme theme = data->renderer->getCurrentTheme();
    data->renderer->clear(theme.windowBackground);

    int margin = 20;
    int y = margin + 10;

    data->renderer->drawText("Find:", margin, y, theme.textColor);
    y += 28;

    Rect findBox(margin, y, windowWidth - margin * 2, 30);
    Color findBg = (data->activeTextBox == 0) ? Color(70, 70, 75) : Color(55, 55, 60);
    Color findBorder = (data->activeTextBox == 0) ? Color(0, 120, 215) : Color(90, 90, 95);

    data->renderer->drawRoundedRect(findBox, 4.0f, findBg, true);
    data->renderer->drawRoundedRect(findBox, 4.0f, findBorder, false);
    std::string findDisplay = data->findText + (data->activeTextBox == 0 ? "|" : "");
    data->renderer->drawText(findDisplay, findBox.x + 8, findBox.y + 8, Color(240, 240, 240));

    y += 48;

    data->renderer->drawText("Replace:", margin, y, theme.textColor);
    y += 28;

    Rect replaceBox(margin, y, windowWidth - margin * 2, 30);
    Color replaceBg = (data->activeTextBox == 1) ? Color(70, 70, 75) : Color(55, 55, 60);
    Color replaceBorder = (data->activeTextBox == 1) ? Color(0, 120, 215) : Color(90, 90, 95);

    data->renderer->drawRoundedRect(replaceBox, 4.0f, replaceBg, true);
    data->renderer->drawRoundedRect(replaceBox, 4.0f, replaceBorder, false);
    std::string replaceDisplay = data->replaceText + (data->activeTextBox == 1 ? "|" : "");
    data->renderer->drawText(replaceDisplay, replaceBox.x + 8, replaceBox.y + 8, Color(240, 240, 240));

    int buttonSpacing = 20;
    int buttonY = replaceBox.y + replaceBox.height + buttonSpacing;
    int buttonWidth = 90;

    Rect okButton(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, 30);
    Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

    WidgetState okState;
    okState.rect = okButton;
    okState.hovered = (data->hoveredWidget == 0);
    okState.pressed = (data->pressedWidget == 0);
    okState.enabled = true;
    data->renderer->drawModernButton(okState, theme, "Replace");

    WidgetState cancelState;
    cancelState.rect = cancelButton;
    cancelState.hovered = (data->hoveredWidget == 1);
    cancelState.pressed = (data->pressedWidget == 1);
    cancelState.enabled = true;
    data->renderer->drawModernButton(cancelState, theme, "Cancel");

    data->renderer->endFrame();
  }

  void RenderGoToDialog(GoToDialogData* data, int windowWidth, int windowHeight) {
    if (!data || !data->renderer) return;

    data->renderer->beginFrame();
    Theme theme = data->renderer->getCurrentTheme();
    data->renderer->clear(theme.windowBackground);

    int margin = 20;
    int y = margin;

    data->renderer->drawText("Offset:", margin, y + 8, theme.textColor);

    Rect offsetBox(80, y, windowWidth - 100, 30);
    Color offsetBg = (data->activeTextBox == 0) ? Color(70, 70, 75) : Color(55, 55, 60);
    Color offsetBorder = (data->activeTextBox == 0) ? Color(0, 120, 215) : Color(90, 90, 95);

    data->renderer->drawRoundedRect(offsetBox, 4.0f, offsetBg, true);
    data->renderer->drawRoundedRect(offsetBox, 4.0f, offsetBorder, false);
    std::string offsetDisplay = data->lineNumberText + (data->activeTextBox == 0 ? "|" : "");
    data->renderer->drawText(offsetDisplay, offsetBox.x + 8, offsetBox.y + 8, Color(240, 240, 240));

    int buttonY = offsetBox.y + offsetBox.height + 30;
    int buttonWidth = 90;
    int buttonSpacing = 10;

    Rect okButton(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, 30);
    Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

    WidgetState okState;
    okState.rect = okButton;
    okState.hovered = (data->hoveredWidget == 0);
    okState.pressed = (data->pressedWidget == 0);
    okState.enabled = true;
    data->renderer->drawModernButton(okState, theme, "Go");

    WidgetState cancelState;
    cancelState.rect = cancelButton;
    cancelState.hovered = (data->hoveredWidget == 1);
    cancelState.pressed = (data->pressedWidget == 1);
    cancelState.enabled = true;
    data->renderer->drawModernButton(cancelState, theme, "Cancel");

    data->renderer->endFrame();
  }

  void UpdateFindReplaceHover(FindReplaceDialogData* data, int x, int y, int windowWidth, int windowHeight) {
    int margin = 20;
    int buttonY = windowHeight - margin - 45;
    int buttonWidth = 90;

    Rect okButton(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, 30);
    Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

    data->hoveredWidget = -1;
    if (IsPointInRect(x, y, okButton)) data->hoveredWidget = 0;
    else if (IsPointInRect(x, y, cancelButton)) data->hoveredWidget = 1;
  }

  void HandleFindReplaceClick(FindReplaceDialogData* data, int x, int y, int windowWidth, int windowHeight) {
    int margin = 20;
    int yStart = margin + 10 + 28;

    Rect findBox(margin, yStart, windowWidth - margin * 2, 30);
    yStart += 48 + 28;
    Rect replaceBox(margin, yStart, windowWidth - margin * 2, 30);

    if (IsPointInRect(x, y, findBox)) {
      data->activeTextBox = 0;
      return;
    }
    if (IsPointInRect(x, y, replaceBox)) {
      data->activeTextBox = 1;
      return;
    }

    if (data->hoveredWidget == 0) {
      data->dialogResult = true;
      data->running = false;
      if (data->callback) {
        data->callback(data->findText, data->replaceText);
      }
    }
    else if (data->hoveredWidget == 1) {
      data->dialogResult = false;
      data->running = false;
    }
  }

  void UpdateGoToHover(GoToDialogData* data, int x, int y, int windowWidth, int windowHeight) {
    int margin = 20;
    int lineBoxY = margin;
    int buttonY = lineBoxY + 30 + 30; // text box height = 30, spacing = 30
    int buttonWidth = 90;
    int buttonSpacing = 10;

    Rect okButton(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, 30);
    Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

    data->hoveredWidget = -1;
    if (IsPointInRect(x, y, okButton)) data->hoveredWidget = 0;
    else if (IsPointInRect(x, y, cancelButton)) data->hoveredWidget = 1;
  }

  void HandleGoToClick(GoToDialogData* data, int x, int y, int windowWidth, int windowHeight) {
    int margin = 20;
    int lineBoxY = margin;
    int buttonY = lineBoxY + 30 + 30;
    int buttonWidth = 90;
    int buttonSpacing = 10;

    Rect okButton(windowWidth - margin - buttonWidth * 2 - buttonSpacing, buttonY, buttonWidth, 30);
    Rect cancelButton(windowWidth - margin - buttonWidth, buttonY, buttonWidth, 30);

    if (IsPointInRect(x, y, okButton)) {
      data->dialogResult = true;
      data->running = false;
      if (data->callback) {
        try {
          int line = std::stoi(data->lineNumberText);
          data->callback(line);
        }
        catch (...) {}
      }
    }
    else if (IsPointInRect(x, y, cancelButton)) {
      data->dialogResult = false;
      data->running = false;
    }
  }

  void HandleFindReplaceChar(FindReplaceDialogData* data, char ch, int windowWidth, int windowHeight) {
    if (ch == '\b' || ch == 8) {
      if (data->activeTextBox == 0 && !data->findText.empty()) {
        data->findText.pop_back();
      }
      else if (data->activeTextBox == 1 && !data->replaceText.empty()) {
        data->replaceText.pop_back();
      }
    }
    else if (ch == '\r' || ch == '\n') {
      data->hoveredWidget = 0;
      HandleFindReplaceClick(data, 0, 0, windowWidth, windowHeight);
    }
    else if (ch == 27) {
      data->dialogResult = false;
      data->running = false;
    }
    else if (ch >= 32 && ch < 127) {
      if (data->activeTextBox == 0) {
        data->findText += ch;
      }
      else if (data->activeTextBox == 1) {
        data->replaceText += ch;
      }
    }
  }

  void HandleGoToChar(GoToDialogData* data, char ch, int windowWidth, int windowHeight) {
    if (ch == '\b' || ch == 8) {
      if (!data->lineNumberText.empty()) {
        data->lineNumberText.pop_back();
      }
    }
    else if (ch == '\r' || ch == '\n') {
      data->hoveredWidget = 0;
      HandleGoToClick(data, 0, 0, windowWidth, windowHeight);
    }
    else if (ch == 27) {
      data->dialogResult = false;
      data->running = false;
    }
    else if ((ch >= '0' && ch <= '9') ||
      (ch >= 'A' && ch <= 'F') ||
      (ch >= 'a' && ch <= 'f') ||
      (ch == 'x' || ch == 'X')) {
      data->lineNumberText += ch;
    }
  }

#ifdef _WIN32

  LRESULT CALLBACK FindReplaceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    FindReplaceDialogData* data = g_findReplaceData;

    switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hwnd, &ps);
      if (data && data->renderer) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        RenderFindReplaceDialog(data, rect.right, rect.bottom);
      }
      EndPaint(hwnd, &ps);
      return 0;
    }

    case WM_CHAR: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        HandleFindReplaceChar(data, (char)wParam, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_KEYDOWN: {
      if (data && wParam == VK_TAB) {
        data->activeTextBox = (data->activeTextBox + 1) % 2;
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
      }
      break;
    }

    case WM_MOUSEMOVE: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        UpdateFindReplaceHover(data, x, y, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONDOWN: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        data->pressedWidget = data->hoveredWidget;

        int margin = 20;
        int yStart = margin + 10 + 28;
        Rect findBox(margin, yStart, rect.right - margin * 2, 30);
        yStart += 48 + 28;
        Rect replaceBox(margin, yStart, rect.right - margin * 2, 30);

        if (IsPointInRect(x, y, findBox)) data->activeTextBox = 0;
        else if (IsPointInRect(x, y, replaceBox)) data->activeTextBox = 1;

        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONUP: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (data->pressedWidget == data->hoveredWidget && data->hoveredWidget != -1) {
          HandleFindReplaceClick(data, x, y, rect.right, rect.bottom);
        }

        data->pressedWidget = -1;
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_CLOSE:
      if (data) {
        data->dialogResult = false;
        data->running = false;
      }
      return 0;

    case WM_SIZE: {
      if (data && data->renderer) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        if (rect.right > 0 && rect.bottom > 0) {
          data->renderer->resize(rect.right, rect.bottom);
          InvalidateRect(hwnd, NULL, FALSE);
        }
      }
      return 0;
    }
  }

    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  LRESULT CALLBACK GoToWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GoToDialogData* data = g_goToData;

    switch (msg) {
    case WM_PAINT: {
      PAINTSTRUCT ps;
      BeginPaint(hwnd, &ps);
      if (data && data->renderer) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        RenderGoToDialog(data, rect.right, rect.bottom);
      }
      EndPaint(hwnd, &ps);
      return 0;
    }

    case WM_CHAR: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        HandleGoToChar(data, (char)wParam, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        UpdateGoToHover(data, x, y, rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONDOWN: {
      if (data) {
        data->pressedWidget = data->hoveredWidget;
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_LBUTTONUP: {
      if (data) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (data->pressedWidget == data->hoveredWidget && data->hoveredWidget != -1) {
          HandleGoToClick(data, x, y, rect.right, rect.bottom);
        }

        data->pressedWidget = -1;
        InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }

    case WM_CLOSE:
      if (data) {
        data->dialogResult = false;
        data->running = false;
      }
      return 0;

    case WM_SIZE: {
      if (data && data->renderer) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        if (rect.right > 0 && rect.bottom > 0) {
          data->renderer->resize(rect.right, rect.bottom);
          InvalidateRect(hwnd, NULL, FALSE);
        }
      }
      return 0;
    }
  }

    return DefWindowProc(hwnd, msg, wParam, lParam);
  }

#elif defined(__linux__)

  void ProcessFindReplaceEvent(FindReplaceDialogData* data, XEvent* event, int width, int height) {
    switch (event->type) {
    case Expose:
      RenderFindReplaceDialog(data, width, height);
      break;

    case KeyPress: {
      char buf[32];
      KeySym keysym;
      int len = XLookupString(&event->xkey, buf, sizeof(buf), &keysym, nullptr);

      if (keysym == XK_Tab) {
        data->activeTextBox = (data->activeTextBox + 1) % 2;
        RenderFindReplaceDialog(data, width, height);
      }
      else if (len > 0) {
        HandleFindReplaceChar(data, buf[0], width, height);
        RenderFindReplaceDialog(data, width, height);
      }
      break;
    }

    case MotionNotify: {
      UpdateFindReplaceHover(data, event->xmotion.x, event->xmotion.y, width, height);
      RenderFindReplaceDialog(data, width, height);
      break;
    }

    case ButtonPress:
      if (event->xbutton.button == Button1) {
        data->pressedWidget = data->hoveredWidget;

        int margin = 20;
        int yStart = margin + 10 + 28;
        Rect findBox(margin, yStart, width - margin * 2, 30);
        yStart += 48 + 28;
        Rect replaceBox(margin, yStart, width - margin * 2, 30);

        if (IsPointInRect(event->xbutton.x, event->xbutton.y, findBox))
          data->activeTextBox = 0;
        else if (IsPointInRect(event->xbutton.x, event->xbutton.y, replaceBox))
          data->activeTextBox = 1;

        RenderFindReplaceDialog(data, width, height);
      }
      break;

    case ButtonRelease:
      if (event->xbutton.button == Button1) {
        if (data->pressedWidget == data->hoveredWidget && data->hoveredWidget != -1) {
          HandleFindReplaceClick(data, event->xbutton.x, event->xbutton.y, width, height);
        }
        data->pressedWidget = -1;
        RenderFindReplaceDialog(data, width, height);
      }
      break;

    case ClientMessage:
      if ((Atom)event->xclient.data.l[0] == data->platformWindow.wmDeleteWindow) {
        data->dialogResult = false;
        data->running = false;
      }
      break;

    case ConfigureNotify:
      if (event->xconfigure.width != width || event->xconfigure.height != height) {
        data->renderer->resize(event->xconfigure.width, event->xconfigure.height);
        RenderFindReplaceDialog(data, event->xconfigure.width, event->xconfigure.height);
      }
      break;
    }
  }

  void ProcessGoToEvent(GoToDialogData* data, XEvent* event, int width, int height) {
    switch (event->type) {
    case Expose:
      RenderGoToDialog(data, width, height);
      break;

    case KeyPress: {
      char buf[32];
      KeySym keysym;
      int len = XLookupString(&event->xkey, buf, sizeof(buf), &keysym, nullptr);

      if (len > 0) {
        HandleGoToChar(data, buf[0], width, height);
        RenderGoToDialog(data, width, height);
      }
      break;
    }

    case MotionNotify: {
      UpdateGoToHover(data, event->xmotion.x, event->xmotion.y, width, height);
      RenderGoToDialog(data, width, height);
      break;
    }

    case ButtonPress:
      if (event->xbutton.button == Button1) {
        data->pressedWidget = data->hoveredWidget;
        RenderGoToDialog(data, width, height);
      }
      break;

    case ButtonRelease:
      if (event->xbutton.button == Button1) {
        if (data->pressedWidget == data->hoveredWidget && data->hoveredWidget != -1) {
          HandleGoToClick(data, event->xbutton.x, event->xbutton.y, width, height);
        }
        data->pressedWidget = -1;
        RenderGoToDialog(data, width, height);
      }
      break;

    case ClientMessage:
      if ((Atom)event->xclient.data.l[0] == data->platformWindow.wmDeleteWindow) {
        data->dialogResult = false;
        data->running = false;
      }
      break;

    case ConfigureNotify:
      if (event->xconfigure.width != width || event->xconfigure.height != height) {
        data->renderer->resize(event->xconfigure.width, event->xconfigure.height);
        RenderGoToDialog(data, event->xconfigure.width, event->xconfigure.height);
      }
      break;
    }
  }

#elif defined(__APPLE__)

  extern "C" id objc_msgSend(id, SEL, ...);
  extern "C" SEL sel_registerName(const char*);
  extern "C" Class objc_getClass(const char*);
  extern "C" id objc_allocateClassPair(Class, const char*, size_t);
  extern "C" void objc_registerClassPair(Class);
  extern "C" bool class_addMethod(Class, SEL, void*, const char*);

  void FindReplaceDrawRect(id self, SEL _cmd, void* dirtyRect) {
    FindReplaceDialogData* data = g_findReplaceData;
    if (!data || !data->renderer) return;

    id window = ((id(*)(id, SEL))objc_msgSend)(self, sel_registerName("window"));
    id contentView = ((id(*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

    double width = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.width;
    double height = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.height;

    RenderFindReplaceDialog(data, (int)width, (int)height);
  }

  void FindReplaceMouseMoved(id self, SEL _cmd, id event) {
    FindReplaceDialogData* data = g_findReplaceData;
    if (!data) return;

    id window = ((id(*)(id, SEL))objc_msgSend)(self, sel_registerName("window"));
    id contentView = ((id(*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

    double width = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.width;
    double height = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.height;

    double x = ((double(*)(id, SEL))objc_msgSend)(event, sel_registerName("locationInWindow")).x;
    double y = ((double(*)(id, SEL))objc_msgSend)(event, sel_registerName("locationInWindow")).y;

    UpdateFindReplaceHover(data, (int)x, (int)(height - y), (int)width, (int)height);
    ((void(*)(id, SEL))objc_msgSend)(self, sel_registerName("setNeedsDisplay:"), true);
  }

  void FindReplaceMouseDown(id self, SEL _cmd, id event) {
    FindReplaceDialogData* data = g_findReplaceData;
    if (!data) return;

    data->pressedWidget = data->hoveredWidget;
    ((void(*)(id, SEL))objc_msgSend)(self, sel_registerName("setNeedsDisplay:"), true);
  }

  void FindReplaceMouseUp(id self, SEL _cmd, id event) {
    FindReplaceDialogData* data = g_findReplaceData;
    if (!data) return;

    id window = ((id(*)(id, SEL))objc_msgSend)(self, sel_registerName("window"));
    id contentView = ((id(*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

    double width = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.width;
    double height = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.height;

    double x = ((double(*)(id, SEL))objc_msgSend)(event, sel_registerName("locationInWindow")).x;
    double y = ((double(*)(id, SEL))objc_msgSend)(event, sel_registerName("locationInWindow")).y;

    if (data->pressedWidget == data->hoveredWidget && data->hoveredWidget != -1) {
      HandleFindReplaceClick(data, (int)x, (int)(height - y), (int)width, (int)height);
    }

    data->pressedWidget = -1;
    ((void(*)(id, SEL))objc_msgSend)(self, sel_registerName("setNeedsDisplay:"), true);
  }

  void FindReplaceKeyDown(id self, SEL _cmd, id event) {
    FindReplaceDialogData* data = g_findReplaceData;
    if (!data) return;

    id characters = ((id(*)(id, SEL))objc_msgSend)(event, sel_registerName("characters"));
    const char* str = ((const char* (*)(id, SEL))objc_msgSend)(characters, sel_registerName("UTF8String"));

    id window = ((id(*)(id, SEL))objc_msgSend)(self, sel_registerName("window"));
    id contentView = ((id(*)(id, SEL))objc_msgSend)(window, sel_registerName("contentView"));

    double width = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.width;
    double height = ((double(*)(id, SEL))objc_msgSend)(contentView, sel_registerName("bounds")).size.height;

    if (str && str[0]) {
      HandleFindReplaceChar(data, str[0], (int)width, (int)height);
      ((void(*)(id, SEL))objc_msgSend)(self, sel_registerName("setNeedsDisplay:"), true);
    }
  }

#endif
  void ShowFindReplaceDialog(void* parentHandle, bool darkMode,
    std::function<void(const std::string&, const std::string&)> callback) {

#ifdef _WIN32
    HWND parent = (HWND)parentHandle;
    const wchar_t* className = L"FindReplaceDialogClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = FindReplaceWindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    UnregisterClassW(className, wc.hInstance);
    RegisterClassExW(&wc);

    FindReplaceDialogData data = {};
    data.callback = callback;
    g_findReplaceData = &data;

    int width = 400;
    int height = 260;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

    HWND hwnd = CreateWindowExW(
      WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
      className, L"Find and Replace",
      WS_POPUP | WS_CAPTION | WS_SYSMENU,
      x, y, width, height,
      parent, nullptr, GetModuleHandleW(NULL), nullptr);

    if (!hwnd) {
      g_findReplaceData = nullptr;
      return;
    }

    if (darkMode) {
      BOOL dark = TRUE;
      DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    }

    data.renderer = new RenderManager();
    if (!data.renderer->initialize(hwnd)) {
      delete data.renderer;
      DestroyWindow(hwnd);
      g_findReplaceData = nullptr;
      return;
    }

    data.platformWindow.hwnd = hwnd;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    if (clientRect.right > 0 && clientRect.bottom > 0) {
      data.renderer->resize(clientRect.right, clientRect.bottom);
    }

    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (data.running && GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    delete data.renderer;
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    DestroyWindow(hwnd);
    UnregisterClassW(className, GetModuleHandleW(NULL));
    g_findReplaceData = nullptr;

#elif defined(__linux__)
    Window parentWindow = (Window)(uintptr_t)parentHandle;
    Display* parentDisplay = XOpenDisplay(nullptr);
    if (!parentDisplay) {
      fprintf(stderr, "Failed to open display\n");
      return;
    }

    int screen = DefaultScreen(parentDisplay);
    Window rootWindow = RootWindow(parentDisplay, screen);

    FindReplaceDialogData data = {};
    data.callback = callback;
    g_findReplaceData = &data;

    int width = 400;
    int height = 260;

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
    XStoreName(parentDisplay, window, "Find and Replace");

    if (parentWindow != 0) {
      XSetTransientForHint(parentDisplay, window, parentWindow);
    }

    XMapWindow(parentDisplay, window);
    XFlush(parentDisplay);
    ApplyDarkTitleBar(parentDisplay, window, darkMode);
    data.platformWindow.display = parentDisplay;
    data.platformWindow.window = window;
    data.platformWindow.wmDeleteWindow = wmDelete;

    data.renderer = new RenderManager();
    if (!data.renderer->initialize(window)) {
      delete data.renderer;
      XDestroyWindow(parentDisplay, window);
      XCloseDisplay(parentDisplay);
      g_findReplaceData = nullptr;
      return;
    }

    data.renderer->resize(width, height);

    XEvent event;
    while (data.running) {
      while (XPending(parentDisplay)) {
        XNextEvent(parentDisplay, &event);
        ProcessFindReplaceEvent(&data, &event, width, height);
      }
      usleep(1000);  // Small sleep to prevent busy-waiting
    }

    delete data.renderer;
    XDestroyWindow(parentDisplay, window);
    XCloseDisplay(parentDisplay);
    g_findReplaceData = nullptr;

#elif defined(__APPLE__)
    (void)parentHandle;
    (void)darkMode;
    (void)callback;
#endif
  }

  void ShowGoToDialog(void* parentHandle, bool darkMode,
    std::function<void(int)> callback) {

#ifdef _WIN32
    HWND parent = (HWND)parentHandle;
    const wchar_t* className = L"GoToDialogClass";

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = GoToWindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    UnregisterClassW(className, wc.hInstance);
    RegisterClassExW(&wc);

    GoToDialogData data = {};
    data.callback = callback;
    g_goToData = &data;

    int width = 380;
    int height = 160;

    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

    HWND hwnd = CreateWindowExW(
      WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
      className, L"Go To Line",
      WS_POPUP | WS_CAPTION | WS_SYSMENU,
      x, y, width, height,
      parent, nullptr, GetModuleHandleW(NULL), nullptr);

    if (!hwnd) {
      g_goToData = nullptr;
      return;
    }

    if (darkMode) {
      BOOL dark = TRUE;
      DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
    }

    data.renderer = new RenderManager();
    if (!data.renderer->initialize(hwnd)) {
      delete data.renderer;
      DestroyWindow(hwnd);
      g_goToData = nullptr;
      return;
    }

    data.platformWindow.hwnd = hwnd;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    if (clientRect.right > 0 && clientRect.bottom > 0) {
      data.renderer->resize(clientRect.right, clientRect.bottom);
    }

    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (data.running && GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    delete data.renderer;
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    DestroyWindow(hwnd);
    UnregisterClassW(className, GetModuleHandleW(NULL));
    g_goToData = nullptr;

#elif defined(__linux__)
    Window parentWindow = (Window)(uintptr_t)parentHandle;
    Display* parentDisplay = XOpenDisplay(nullptr);
    if (!parentDisplay) {
      fprintf(stderr, "Failed to open display\n");
      return;
    }

    int screen = DefaultScreen(parentDisplay);
    Window rootWindow = RootWindow(parentDisplay, screen);

    GoToDialogData data = {};
    data.callback = callback;
    g_goToData = &data;

    int width = 380;
    int height = 160;

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
    XStoreName(parentDisplay, window, "Go To Line");

    if (parentWindow != 0) {
      XSetTransientForHint(parentDisplay, window, parentWindow);
    }

    XMapWindow(parentDisplay, window);
    XFlush(parentDisplay);

    ApplyDarkTitleBar(parentDisplay, window, darkMode);

    data.platformWindow.display = parentDisplay;
    data.platformWindow.window = window;
    data.platformWindow.wmDeleteWindow = wmDelete;

    data.renderer = new RenderManager();
    if (!data.renderer->initialize(window)) {
      delete data.renderer;
      XDestroyWindow(parentDisplay, window);
      XCloseDisplay(parentDisplay);
      g_goToData = nullptr;
      return;
    }

    data.renderer->resize(width, height);

    XEvent event;
    while (data.running) {
      while (XPending(parentDisplay)) {
        XNextEvent(parentDisplay, &event);
        ProcessGoToEvent(&data, &event, width, height);
      }
      usleep(1000);  // Small sleep to prevent busy-waiting
    }

    delete data.renderer;
    XDestroyWindow(parentDisplay, window);
    XCloseDisplay(parentDisplay);
    g_goToData = nullptr;
#elif defined(__APPLE__)
    (void)parentHandle;
    (void)darkMode;
    (void)callback;
#endif
  }


} // namespace SearchDialogs