#include "render.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #define NATIVE_WINDOW_NULL nullptr
#elif __APPLE__
    #define NATIVE_WINDOW_NULL nullptr
#else
    #define NATIVE_WINDOW_NULL 0UL   // X11 Window is unsigned long
#endif

RenderManager::RenderManager()
  : window(NATIVE_WINDOW_NULL),
    windowWidth(0), windowHeight(0)
#ifdef _WIN32
  , hdc(nullptr), memDC(nullptr), memBitmap(nullptr), oldBitmap(nullptr),
    font(nullptr), pixels(nullptr)
#elif __APPLE__
  , context(nullptr), backBuffer(nullptr)
#else
  , display(nullptr), gc(nullptr), backBuffer(0), fontInfo(nullptr)
#endif
{
    currentTheme = Theme::Dark();
}


RenderManager::~RenderManager() {
  cleanup();
}

bool RenderManager::initialize(NativeWindow win) {
  window = win;

#ifdef _WIN32
  hdc = GetDC((HWND)window);
  if (!hdc) return false;

  memDC = CreateCompatibleDC(hdc);
  if (!memDC) {
    ReleaseDC((HWND)window, hdc);
    return false;
  }

  createFont();
  return true;

#elif __APPLE__
  return false;

#else
  display = XOpenDisplay(nullptr);
  if (!display) return false;

  gc = XCreateGC(display, window, 0, nullptr);
  if (!gc) {
    XCloseDisplay(display);
    return false;
  }

  fontInfo = XLoadQueryFont(display, "fixed");
  if (!fontInfo) {
    fontInfo = XLoadQueryFont(display, "*");
  }
  if (fontInfo) {
    XSetFont(display, gc, fontInfo->fid);
  }

  return true;
#endif
}

void RenderManager::cleanup() {
#ifdef _WIN32
  destroyFont();

  if (memBitmap) {
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    memBitmap = nullptr;
  }

  if (memDC) {
    DeleteDC(memDC);
    memDC = nullptr;
  }

  if (hdc) {
    ReleaseDC((HWND)window, hdc);
    hdc = nullptr;
  }

#elif __APPLE__
  if (backBuffer) {
    free(backBuffer);
    backBuffer = nullptr;
  }

#else
  if (backBuffer) {
    XFreePixmap(display, backBuffer);
    backBuffer = 0;
  }

  if (fontInfo) {
    XFreeFont(display, fontInfo);
    fontInfo = nullptr;
  }

  if (gc) {
    XFreeGC(display, gc);
    gc = nullptr;
  }

  if (display) {
    XCloseDisplay(display);
    display = nullptr;
  }
#endif
}

void RenderManager::resize(int width, int height) {
#ifdef _WIN32
    if (!memDC) return;  // Not initialized yet
#elif __APPLE__
    if (!context) return;  // Not initialized yet
#else
    if (!display || !window) return;  // Not initialized yet
#endif

    if (width <= 0 || height <= 0) return;
    if (width > 8192 || height > 8192) return;

  windowWidth = width;
  windowHeight = height;
#ifdef _WIN32
  if (memBitmap) {
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
  }

  memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
  bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = -height; // Top-down
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  memBitmap = CreateDIBSection(memDC, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
  oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

#elif __APPLE__
  if (backBuffer) {
    free(backBuffer);
  }
  backBuffer = malloc(width * height * 4);

#else
  if (backBuffer) {
    XFreePixmap(display, backBuffer);
  }
  backBuffer = XCreatePixmap(display, window, width, height,
    DefaultDepth(display, DefaultScreen(display)));
#endif
}

void RenderManager::createFont() {
#ifdef _WIN32
  font = CreateFontA(
    16,                        // Height
    0,                         // Width
    0,                         // Escapement
    0,                         // Orientation
    FW_NORMAL,                 // Weight
    FALSE,                     // Italic
    FALSE,                     // Underline
    FALSE,                     // StrikeOut
    DEFAULT_CHARSET,           // CharSet
    OUT_DEFAULT_PRECIS,        // OutPrecision
    CLIP_DEFAULT_PRECIS,       // ClipPrecision
    DEFAULT_QUALITY,           // Quality
    FIXED_PITCH | FF_MODERN,   // PitchAndFamily
    "Consolas"                 // FaceName
  );

  if (font && memDC) {
    SelectObject(memDC, font);
  }
#endif
}

void RenderManager::destroyFont() {
#ifdef _WIN32
  if (font) {
    DeleteObject(font);
    font = nullptr;
  }
#endif
}

void RenderManager::beginFrame() {
}

void RenderManager::endFrame() {
#ifdef _WIN32
  BitBlt(hdc, 0, 0, windowWidth, windowHeight, memDC, 0, 0, SRCCOPY);

#elif __APPLE__

#else
  XCopyArea(display, backBuffer, window, gc, 0, 0,
    windowWidth, windowHeight, 0, 0);
  XFlush(display);
#endif
}

void RenderManager::clear(const Color& color) {
#ifdef _WIN32
  RECT rect = { 0, 0, windowWidth, windowHeight };
  HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
  FillRect(memDC, &rect, brush);
  DeleteObject(brush);

#elif __APPLE__
  if (backBuffer) {
    uint32_t* buf = (uint32_t*)backBuffer;
    uint32_t colorValue = (color.r << 16) | (color.g << 8) | color.b;
    for (int i = 0; i < windowWidth * windowHeight; i++) {
      buf[i] = colorValue;
    }
  }

#else
  XSetForeground(display, gc,
    (color.r << 16) | (color.g << 8) | color.b);
  XFillRectangle(display, backBuffer, gc, 0, 0, windowWidth, windowHeight);
#endif
}

void RenderManager::setColor(const Color& color) {
#ifdef _WIN32
  SetTextColor(memDC, RGB(color.r, color.g, color.b));
  SetBkMode(memDC, TRANSPARENT);

#elif __APPLE__

#else
  XSetForeground(display, gc,
    (color.r << 16) | (color.g << 8) | color.b);
#endif
}

void RenderManager::drawRect(const Rect& rect, const Color& color, bool filled) {
#ifdef _WIN32
  if (filled) {
    RECT r = { rect.x, rect.y, rect.x + rect.width, rect.y + rect.height };
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    FillRect(memDC, &r, brush);
    DeleteObject(brush);
  }
  else {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(color.r, color.g, color.b));
    HPEN oldPen = (HPEN)SelectObject(memDC, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));

    Rectangle(memDC, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);

    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(pen);
  }

#elif __APPLE__

#else
  XSetForeground(display, gc,
    (color.r << 16) | (color.g << 8) | color.b);
  if (filled) {
    XFillRectangle(display, backBuffer, gc,
      rect.x, rect.y, rect.width, rect.height);
  }
  else {
    XDrawRectangle(display, backBuffer, gc,
      rect.x, rect.y, rect.width, rect.height);
  }
#endif
}

void RenderManager::drawLine(int x1, int y1, int x2, int y2, const Color& color) {
#ifdef _WIN32
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(color.r, color.g, color.b));
  HPEN oldPen = (HPEN)SelectObject(memDC, pen);

  MoveToEx(memDC, x1, y1, nullptr);
  LineTo(memDC, x2, y2);

  SelectObject(memDC, oldPen);
  DeleteObject(pen);

#elif __APPLE__

#else
  XSetForeground(display, gc,
    (color.r << 16) | (color.g << 8) | color.b);
  XDrawLine(display, backBuffer, gc, x1, y1, x2, y2);
#endif
}

void RenderManager::drawText(const std::string& text, int x, int y, const Color& color) {
#ifdef _WIN32
  setColor(color);
  TextOutA(memDC, x, y, text.c_str(), (int)text.length());

#elif __APPLE__

#else
  XSetForeground(display, gc,
    (color.r << 16) | (color.g << 8) | color.b);
  XDrawString(display, backBuffer, gc, x, y + 12,
    text.c_str(), text.length());
#endif
}


void RenderManager::drawRoundedRect(const Rect& rect, float radius, const Color& color, bool filled) {
#ifdef _WIN32
  if (filled) {
    HRGN rgn = CreateRoundRectRgn(
      rect.x, rect.y,
      rect.x + rect.width, rect.y + rect.height,
      (int)radius * 2, (int)radius * 2
    );

    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    FillRgn(memDC, rgn, brush);
    DeleteObject(brush);
    DeleteObject(rgn);
  }
  else {
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(color.r, color.g, color.b));
    HPEN oldPen = (HPEN)SelectObject(memDC, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));

    RoundRect(memDC, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height,
      (int)radius * 2, (int)radius * 2);

    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(pen);
  }
#elif __APPLE__
#else
  drawRect(rect, color, filled);
#endif
}

void RenderManager::drawModernButton(const WidgetState& state, const Theme& theme, const std::string& label) {
  float radius = 6.0f;

  Color fillColor = theme.buttonNormal;
  if (!state.enabled) {
    fillColor = theme.buttonDisabled;
  }
  else if (state.pressed) {
    fillColor = theme.buttonPressed;
  }
  else if (state.hovered) {
    fillColor = theme.buttonHover;
  }

  drawRoundedRect(state.rect, radius, fillColor, true);

  if (state.enabled) {
    Color borderColor = state.pressed ?
      Color(fillColor.r - 20, fillColor.g - 20, fillColor.b - 20) :
      Color(fillColor.r - 10, fillColor.g - 10, fillColor.b - 10);
    drawRoundedRect(state.rect, radius, borderColor, false);
  }

  int textWidth = label.length() * 8; // Approximate character width
  int textHeight = 16; // Approximate font height
  int textX = state.rect.x + (state.rect.width - textWidth) / 2;
  int textY = state.rect.y + (state.rect.height - textHeight) / 2;

  Color textColor = state.enabled ? theme.buttonText :
    Color(theme.buttonText.r / 2, theme.buttonText.g / 2, theme.buttonText.b / 2);
  drawText(label, textX, textY, textColor);
}

void RenderManager::drawModernCheckbox(const WidgetState& state, const Theme& theme, bool checked) {
  float radius = 3.0f;

  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered) {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  if (checked) {
    bgColor = theme.controlCheck;
    borderColor = theme.controlCheck;
  }

  drawRoundedRect(state.rect, radius, bgColor, true);

  if (!checked || state.hovered) {
    drawRoundedRect(state.rect, radius, borderColor, false);
  }

  if (checked) {
    Color checkColor = Color(255, 255, 255); // White checkmark
    int cx = state.rect.x + 4;
    int cy = state.rect.y + 9;

    drawLine(cx, cy, cx + 3, cy + 4, checkColor);
    drawLine(cx + 1, cy, cx + 4, cy + 4, checkColor); // Thicker
    drawLine(cx + 3, cy + 4, cx + 10, cy - 4, checkColor);
    drawLine(cx + 4, cy + 4, cx + 11, cy - 4, checkColor); // Thicker
  }
}


void RenderManager::drawModernRadioButton(const WidgetState& state, const Theme& theme, bool selected) {
#ifdef _WIN32
  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered) {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  int centerX = state.rect.x + state.rect.width / 2;
  int centerY = state.rect.y + state.rect.height / 2;
  int outerRadius = state.rect.width / 2;

  HBRUSH bgBrush = CreateSolidBrush(RGB(bgColor.r, bgColor.g, bgColor.b));
  HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(borderColor.r, borderColor.g, borderColor.b));

  HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, bgBrush);
  HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);

  Ellipse(memDC,
    centerX - outerRadius,
    centerY - outerRadius,
    centerX + outerRadius,
    centerY + outerRadius);

  SelectObject(memDC, oldBrush);
  SelectObject(memDC, oldPen);
  DeleteObject(bgBrush);
  DeleteObject(borderPen);

  if (selected) {
    int innerRadius = outerRadius - 4;
    Color dotColor = theme.controlCheck;

    HBRUSH dotBrush = CreateSolidBrush(RGB(dotColor.r, dotColor.g, dotColor.b));
    HPEN dotPen = CreatePen(PS_SOLID, 1, RGB(dotColor.r, dotColor.g, dotColor.b));

    oldBrush = (HBRUSH)SelectObject(memDC, dotBrush);
    oldPen = (HPEN)SelectObject(memDC, dotPen);

    Ellipse(memDC,
      centerX - innerRadius,
      centerY - innerRadius,
      centerX + innerRadius,
      centerY + innerRadius);

    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(dotBrush);
    DeleteObject(dotPen);
  }

#elif __APPLE__
#else
  Display* display = this->display;
  GC gc = this->gc;

  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered) {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  int centerX = state.rect.x + state.rect.width / 2;
  int centerY = state.rect.y + state.rect.height / 2;
  int outerRadius = state.rect.width / 2;

  XSetForeground(display, gc, (bgColor.r << 16) | (bgColor.g << 8) | bgColor.b);
  XFillArc(display, backBuffer, gc,
    state.rect.x, state.rect.y, state.rect.width, state.rect.height,
    0, 360 * 64);

  XSetForeground(display, gc, (borderColor.r << 16) | (borderColor.g << 8) | borderColor.b);
  XDrawArc(display, backBuffer, gc,
    state.rect.x, state.rect.y, state.rect.width, state.rect.height,
    0, 360 * 64);

  if (selected) {
    int innerRadius = outerRadius - 4;
    Color dotColor = theme.controlCheck;

    XSetForeground(display, gc, (dotColor.r << 16) | (dotColor.g << 8) | dotColor.b);
    XFillArc(display, backBuffer, gc,
      centerX - innerRadius, centerY - innerRadius,
      innerRadius * 2, innerRadius * 2,
      0, 360 * 64);
  }
#endif
}

void RenderManager::renderHexViewer(
  const std::vector<std::string>& hexLines,
  const std::string& headerLine,
  int scrollPos,
  int maxScrollPos,
  bool scrollbarHovered,
  bool scrollbarPressed,
  const Rect& scrollbarRect,
  const Rect& thumbRect,
  bool darkMode,
  int editingRow,
  int editingCol,
  const std::string& editBuffer)
{
  currentTheme = darkMode ? Theme::Dark() : Theme::Light();
  LayoutMetrics layout;

  beginFrame();
  int menuBarHeight = 24;

  Rect contentArea(0, menuBarHeight, windowWidth, windowHeight - menuBarHeight);
  drawRect(contentArea, currentTheme.windowBackground, true);

  int disasmColumnWidth = 300;
  int contentWidth = windowWidth - layout.scrollbarWidth - disasmColumnWidth - layout.margin * 2;

  if (!headerLine.empty()) {
    drawText(headerLine, layout.margin, menuBarHeight + layout.margin, currentTheme.headerColor);

    int disasmX = windowWidth - layout.scrollbarWidth - disasmColumnWidth + 10;
    drawText("Disassembly", disasmX, menuBarHeight + layout.margin, currentTheme.disassemblyColor);

    drawLine(layout.margin, menuBarHeight + layout.margin + layout.headerHeight,
      windowWidth - layout.scrollbarWidth, menuBarHeight + layout.margin + layout.headerHeight,
      currentTheme.separator);
  }

  int separatorX = windowWidth - layout.scrollbarWidth - disasmColumnWidth;
  drawLine(separatorX, menuBarHeight + layout.margin + layout.headerHeight,
    separatorX, windowHeight - layout.margin,
    currentTheme.separator);

  int contentY = menuBarHeight + layout.margin + layout.headerHeight + 2;
  int contentHeight = windowHeight - contentY - layout.margin;
  size_t maxVisibleLines = contentHeight / layout.lineHeight;
  size_t startLine = scrollPos;
  size_t endLine = std::min(startLine + maxVisibleLines, hexLines.size());

  for (size_t i = startLine; i < endLine; i++) {
    int y = contentY + ((i - startLine) * layout.lineHeight);
    const std::string& line = hexLines[i];

    size_t disasmStart = line.rfind("      ");
    std::string hexPart;
    std::string disasmPart;

    if (disasmStart != std::string::npos) {
      hexPart = line.substr(0, disasmStart);
      disasmPart = line.substr(disasmStart + 6);
    }
    else {
      hexPart = line;
    }

    drawText(hexPart, layout.margin, y, currentTheme.textColor);

    if (!disasmPart.empty()) {
      int disasmX = separatorX + 10;
      drawText(disasmPart, disasmX, y, currentTheme.disassemblyColor);
    }

    if (editingRow == (int)(i - startLine) && editingCol >= 0) {
      int offsetWidth = 10 * layout.charWidth;
      int byteX = layout.margin + offsetWidth + (editingCol * 3 * layout.charWidth);

      Rect highlight(byteX, y, 2 * layout.charWidth, layout.lineHeight);
      drawRect(highlight, Color::Blue(), true);

      if (!editBuffer.empty()) {
        drawText(editBuffer, byteX, y, Color::Yellow());
      }
    }
  }

  if (maxScrollPos > 0) {
    drawRect(scrollbarRect, currentTheme.scrollbarBg, true);

    Color thumbColor = currentTheme.scrollbarThumb;
    if (scrollbarHovered || scrollbarPressed) {
      thumbColor.a = 200;
    }
    drawRect(thumbRect, thumbColor, true);
  }

  endFrame();
}