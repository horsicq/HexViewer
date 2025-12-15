#include "render.h"
#include <cstring>
#include <algorithm>

#ifdef _WIN32
    #define NATIVE_WINDOW_NULL nullptr
#elif __APPLE__
    #define NATIVE_WINDOW_NULL nullptr
#else
    #define NATIVE_WINDOW_NULL 0UL
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
    0,                         // Width (0 = default)
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
    FIXED_PITCH | FF_MODERN,   // PitchAndFamily - IMPORTANT: FIXED_PITCH!
    "Consolas"                 // FaceName
  );

  if (font && memDC) {
    SelectObject(memDC, font);

    SIZE textSize;
    GetTextExtentPoint32A(memDC, "0", 1, &textSize);

    char debugMsg[128];
    snprintf(debugMsg, sizeof(debugMsg),
      "Font created: charWidth=%d, charHeight=%d\n",
      textSize.cx, textSize.cy);
    OutputDebugStringA(debugMsg);
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

  int textWidth = label.length() * 8; 
  int textHeight = 16; 
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
    Color checkColor = Color(255, 255, 255);
    int cx = state.rect.x + 4;
    int cy = state.rect.y + 9;

    drawLine(cx, cy, cx + 3, cy + 4, checkColor);
    drawLine(cx + 1, cy, cx + 4, cy + 4, checkColor); 
    drawLine(cx + 3, cy + 4, cx + 10, cy - 4, checkColor);
    drawLine(cx + 4, cy + 4, cx + 11, cy - 4, checkColor); 
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

void RenderManager::drawDropdown(
  const WidgetState& state,
  const Theme& theme,
  const std::string& selectedText,
  bool isOpen,
  const std::vector<std::string>& items,
  int selectedIndex,
  int hoveredIndex,
  int scrollOffset)
{
  float radius = 4.0f;

  Color bgColor = theme.controlBackground;
  bgColor.a = 255;

  drawRect(state.rect, bgColor, true);

  Color borderColor = theme.controlBorder;

  if (state.hovered && !isOpen) {
    borderColor = Color(
      std::clamp(borderColor.r + 30, 0, 255),
      std::clamp(borderColor.g + 30, 0, 255),
      std::clamp(borderColor.b + 30, 0, 255)
    );
  }

  drawRect(state.rect, bgColor, true);
  drawRoundedRect(state.rect, radius, bgColor, true);
  drawRoundedRect(state.rect, radius, borderColor, false);

  int textX = state.rect.x + 10;
  int textY = state.rect.y + (state.rect.height / 2) - 6;
  drawText(selectedText, textX, textY, theme.textColor);

  int arrowX = state.rect.x + state.rect.width - 18;
  int arrowY = state.rect.y + (state.rect.height / 2);

  Color arrowColor = theme.textColor;

  if (isOpen) {
    drawLine(arrowX + 4, arrowY - 3, arrowX, arrowY + 2, arrowColor);
    drawLine(arrowX + 4, arrowY - 3, arrowX + 8, arrowY + 2, arrowColor);
    drawLine(arrowX, arrowY + 2, arrowX + 8, arrowY + 2, arrowColor);
  }
  else {
    drawLine(arrowX, arrowY - 2, arrowX + 8, arrowY - 2, arrowColor);
    drawLine(arrowX, arrowY - 2, arrowX + 4, arrowY + 3, arrowColor);
    drawLine(arrowX + 8, arrowY - 2, arrowX + 4, arrowY + 3, arrowColor);
  }

  if (isOpen && !items.empty()) {
    int itemHeight = 28;
    int maxVisibleItems = 3;

    int totalItems = (int)items.size();

    int maxScroll = std::clamp(totalItems - maxVisibleItems, 0, INT_MAX);
    scrollOffset = std::clamp(scrollOffset, 0, maxScroll);
    int remaining = totalItems - scrollOffset;
    int visibleItems = std::clamp(remaining, 0, maxVisibleItems);

    int listHeight = itemHeight * visibleItems;
    int listY = state.rect.y + state.rect.height + 2;

    Rect clearRect(state.rect.x - 5, listY - 5, state.rect.width + 10, listHeight + 10);
    Color windowBg = theme.windowBackground;
    windowBg.a = 255;
    drawRect(clearRect, windowBg, true);

    Rect shadowRect(state.rect.x + 3, listY + 3, state.rect.width, listHeight);
    Color shadowColor(0, 0, 0, 80);
    drawRect(shadowRect, shadowColor, true);

    Rect listRect(state.rect.x, listY, state.rect.width, listHeight);

    Color listBg = theme.menuBackground;
    listBg.a = 255;

    drawRect(listRect, listBg, true);
    drawRoundedRect(listRect, radius, listBg, true);

    Color listBorder = theme.menuBorder;
    drawRoundedRect(listRect, radius, listBorder, false);

    for (int visualIndex = 0; visualIndex < visibleItems; visualIndex++) {
      size_t actualIndex = scrollOffset + visualIndex;

      Rect itemRect(
        state.rect.x + 1,
        listY + (visualIndex * itemHeight) + 1,
        state.rect.width - 2,
        itemHeight - 1
      );

      bool isSelected = ((int)actualIndex == selectedIndex);
      bool isHovered = ((int)actualIndex == hoveredIndex);

      Color itemBg;

      if (isHovered) {
        itemBg = theme.menuHover;
      }
      else if (isSelected) {
        itemBg = Color(
          (theme.controlCheck.r + theme.menuBackground.r) / 2,
          (theme.controlCheck.g + theme.menuBackground.g) / 2,
          (theme.controlCheck.b + theme.menuBackground.b) / 2
        );
      }
      else {
        itemBg = listBg;
      }

      itemBg.a = 255;

      drawRect(itemRect, itemBg, true);

      int itemTextX = itemRect.x + 10;
      int itemTextY = itemRect.y + (itemRect.height / 2) - 6;

      Color textColor = theme.textColor;
      if (isSelected) {
        textColor = Color(
          std::clamp(textColor.r + 20, 0, 255),
          std::clamp(textColor.g + 20, 0, 255),
          std::clamp(textColor.b + 20, 0, 255)
        );
      }

      drawText(items[actualIndex], itemTextX, itemTextY, textColor);

      if (isSelected) {
        int checkX = itemRect.x + itemRect.width - 20;
        int checkY = itemRect.y + itemRect.height / 2;
        Color checkColor = theme.controlCheck;

        drawLine(checkX, checkY, checkX + 2, checkY + 3, checkColor);
        drawLine(checkX + 2, checkY + 3, checkX + 6, checkY - 2, checkColor);
        drawLine(checkX + 1, checkY, checkX + 3, checkY + 3, checkColor);
        drawLine(checkX + 3, checkY + 3, checkX + 7, checkY - 2, checkColor);
      }

      if (visualIndex < visibleItems - 1) {
        Color separatorColor = Color(
          theme.separator.r,
          theme.separator.g,
          theme.separator.b,
          100
        );
        drawLine(
          itemRect.x + 8,
          itemRect.y + itemRect.height,
          itemRect.x + itemRect.width - 8,
          itemRect.y + itemRect.height,
          separatorColor
        );
      }
    }

    if (scrollOffset > 0) {
      int arrowTopX = state.rect.x + state.rect.width / 2 - 4;
      int arrowTopY = listY + 8;
      Color scrollHint = theme.textColor;
      drawText("^", arrowTopX, arrowTopY - 4, scrollHint);
    }

    if (scrollOffset < maxScroll) {
      int arrowBottomX = state.rect.x + state.rect.width / 2 - 4;
      int arrowBottomY = listY + listHeight - 12;
      Color scrollHint = theme.textColor;
      drawText("v", arrowBottomX, arrowBottomY, scrollHint);
    }
  }
}

PointF RenderManager::GetBytePointF(long long byteIndex) {
  Point gp = GetGridBytePoint(byteIndex);
  return GetBytePointF(gp);
}

PointF RenderManager::GetBytePointF(Point gp) {
  float x = (3.0f * _charWidth) * gp.X + _hexAreaX;
  float y = gp.Y * _charHeight + _hexAreaY;
  return PointF(x, y);
}

Point RenderManager::GetGridBytePoint(long long byteIndex) {
  int row = static_cast<int>(floor(static_cast<double>(byteIndex) /
    static_cast<double>(_bytesPerLine)));
  int column = static_cast<int>(byteIndex % _bytesPerLine);
  return Point(column, row);
}

BytePositionInfo RenderManager::GetHexBytePositionInfo(Point screenPoint) {
  int relX = screenPoint.X - _hexAreaX;
  int relY = screenPoint.Y - _hexAreaY;
  int charX = relX / _charWidth;
  int charY = relY / _charHeight;
  int column = charX / 3;
  int row = charY;

  long long bytePos = _startByte + (row * _bytesPerLine) + column;
  int byteCharPos = charX % 3;
  if (byteCharPos > 1) {
    byteCharPos = 1;
  }

  return BytePositionInfo(bytePos, byteCharPos);
}

void RenderManager::DrawCaret() {
  if (_bytePos < 0) return;
  extern bool caretVisible;
  if (!caretVisible) return;

  long long relativePos = _bytePos - _startByte;
  if (relativePos < 0 || relativePos >= (_bytesPerLine * _visibleLines)) {
    return;
  }

  PointF caretPos = GetBytePointF(relativePos);

  caretPos.X += _byteCharacterPos * _charWidth;
  int caretWidth = 2;
  int caretHeight = _charHeight;

  Rect caretRect(
    static_cast<int>(caretPos.X),
    static_cast<int>(caretPos.Y),
    caretWidth,
    caretHeight
  );

  Color caretColor = currentTheme.textColor;
  caretColor.a = 255;

  drawRect(caretRect, caretColor, true);
}

long long RenderManager::ScreenToByteIndex(int mouseX, int mouseY) {
  BytePositionInfo info = GetHexBytePositionInfo(Point(mouseX, mouseY));
  return info.Index;
}

#ifdef _WIN32
void RenderManager::drawBitmap(void* hBitmap, int width, int height, int x, int y) {
  if (!memDC || !hBitmap) return;
  HDC tempDC = CreateCompatibleDC(memDC);
  HBITMAP oldBmp = (HBITMAP)SelectObject(tempDC, (HBITMAP)hBitmap);
  BitBlt(memDC, x, y, width, height, tempDC, 0, 0, SRCCOPY);
  SelectObject(tempDC, oldBmp);
  DeleteDC(tempDC);
}
#endif
#ifdef __APPLE__
void RenderManager::drawImage(NSImage* image, int width, int height, int x, int y) {
  if (!backBuffer || !image) return;
  [image drawInRect : NSMakeRect(x, y, width, height)
    fromRect : NSZeroRect
    operation : NSCompositingOperationSourceOver
    fraction : 1.0] ;
}
#endif
#ifdef __linux__
void RenderManager::drawX11Pixmap(Pixmap pixmap, int width, int height, int x, int y) {
  if (!display || !backBuffer || !pixmap) return;
  GC tempGC = XCreateGC(display, backBuffer, 0, nullptr);
  XCopyArea(display, pixmap, backBuffer, tempGC, 0, 0, width, height, x, y);
  XFreeGC(display, tempGC);
}
#endif


void RenderManager::drawProgressBar(const Rect& rect, float progress, const Theme& theme) {
  progress = std::clamp(progress, 0.0f, 1.0f);

  float radius = 4.0f;

  Color bgColor(50, 50, 50);
  if (theme.windowBackground.r > 128) { // Light theme
    bgColor = Color(220, 220, 220);
  }
  drawRoundedRect(rect, radius, bgColor, true);

  int fillWidth = (int)(rect.width * progress);
  if (fillWidth > 2) { // Only draw if there's visible progress
    Rect fillRect(rect.x, rect.y, fillWidth, rect.height);

    Color fillColor(100, 150, 255); // Blue progress color

    drawRoundedRect(fillRect, radius, fillColor, true);

    if (rect.height > 10) {
      Rect highlightRect(rect.x + 2, rect.y + 2, fillWidth - 4, rect.height / 3);
      Color highlightColor(
        std::clamp(fillColor.r + 40, 0, 255),
        std::clamp(fillColor.g + 40, 0, 255),
        std::clamp(fillColor.b + 40, 0, 255),
        150
      );
      if (highlightRect.width > 0) {
        drawRect(highlightRect, highlightColor, true);
      }
    }
  }

  Color borderColor(80, 80, 80);
  if (theme.windowBackground.r > 128) { // Light theme
    borderColor = Color(180, 180, 180);
  }
  drawRoundedRect(rect, radius, borderColor, false);
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
  const std::string& editBuffer,
  long long cursorBytePos,
  int cursorNibblePos,
  long long totalBytes)
{
  currentTheme = darkMode ? Theme::Dark() : Theme::Light();
  LayoutMetrics layout;

#ifdef _WIN32
  if (memDC) {
    SIZE textSize;
    if (GetTextExtentPoint32A(memDC, "0", 1, &textSize)) {
      layout.charWidth = (float)textSize.cx;
      layout.lineHeight = (float)textSize.cy;
    }
    else {
      layout.charWidth = 8.0f;
      layout.lineHeight = 16.0f;
    }
  }
  else {
    layout.charWidth = 8.0f;
    layout.lineHeight = 16.0f;
  }
#elif __APPLE__
  layout.charWidth = 9.6f;
  layout.lineHeight = 20.0f;
#else
  if (fontInfo) {
    layout.charWidth = (float)fontInfo->max_bounds.width;
    layout.lineHeight = (float)(fontInfo->ascent + fontInfo->descent);
  }
  else {
    layout.charWidth = 9.6f;
    layout.lineHeight = 20.0f;
  }
#endif

  _bytesPerLine = 16;
  _startByte = scrollPos * _bytesPerLine;
  _bytePos = cursorBytePos;
  _byteCharacterPos = cursorNibblePos;
  _charWidth = (int)layout.charWidth;
  _charHeight = (int)layout.lineHeight;

  beginFrame();
  int menuBarHeight = 24;

  Rect contentArea(0, menuBarHeight, windowWidth, windowHeight - menuBarHeight);
  drawRect(contentArea, currentTheme.windowBackground, true);

  int disasmColumnWidth = 300;

  _hexAreaX = (int)(layout.margin + (10 * layout.charWidth));
  _hexAreaY = menuBarHeight + (int)(layout.margin + layout.headerHeight + 2);

  if (!headerLine.empty()) {
    drawText(headerLine, (int)layout.margin, menuBarHeight + (int)layout.margin, currentTheme.headerColor);

    int disasmX = windowWidth - (int)layout.scrollbarWidth - disasmColumnWidth + 10;
    drawText("Disassembly", disasmX, menuBarHeight + (int)layout.margin, currentTheme.disassemblyColor);

    drawLine((int)layout.margin,
      menuBarHeight + (int)(layout.margin + layout.headerHeight),
      windowWidth - (int)layout.scrollbarWidth,
      menuBarHeight + (int)(layout.margin + layout.headerHeight),
      currentTheme.separator);
  }

  int separatorX = windowWidth - (int)layout.scrollbarWidth - disasmColumnWidth;
  drawLine(separatorX,
    menuBarHeight + (int)(layout.margin + layout.headerHeight),
    separatorX,
    windowHeight - (int)layout.margin,
    currentTheme.separator);

  int contentY = _hexAreaY;
  int contentHeight = windowHeight - contentY - (int)layout.margin;
  size_t maxVisibleLines = (size_t)(contentHeight / layout.lineHeight);
  _visibleLines = (int)maxVisibleLines;

  size_t startLine = scrollPos;
  size_t endLine = std::clamp(
    startLine + maxVisibleLines,
    size_t{ 0 },
    hexLines.size()
  );

  for (size_t i = startLine; i < endLine; i++) {
    int y = contentY + (int)((i - startLine) * layout.lineHeight);
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

    drawText(hexPart, (int)layout.margin, y, currentTheme.textColor);
    if (!disasmPart.empty()) {
      int disasmX = separatorX + 10;
      drawText(disasmPart, disasmX, y, currentTheme.disassemblyColor);
    }
  }
  if (_bytePos >= _startByte && _bytePos < _startByte + (_bytesPerLine * _visibleLines)) {
    DrawCaret();
  }
  if (_selectionLength > 0) {
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
