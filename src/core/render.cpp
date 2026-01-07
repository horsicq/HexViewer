#include "render.h"
#include "panelcontent.h"
#include "hexdata.h"
#include "platfrom_die.h"

extern HexData g_HexData;
char g_DIEExecutablePath[260];

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
      ,
      hdc(nullptr), memDC(nullptr), memBitmap(nullptr), oldBitmap(nullptr),
      font(nullptr), pixels(nullptr)
#elif __APPLE__
      ,
      context(nullptr), backBuffer(nullptr)
#else
      ,
      display(nullptr), gc(nullptr), backBuffer(0), fontInfo(nullptr)
#endif
{
  currentTheme = Theme::Dark();
}

RenderManager::~RenderManager()
{
  cleanup();
}

bool RenderManager::initialize(NativeWindow win)
{
  window = win;

#ifdef _WIN32
  hdc = GetDC((HWND)window);
  if (!hdc)
    return false;

  memDC = CreateCompatibleDC(hdc);
  if (!memDC)
  {
    ReleaseDC((HWND)window, hdc);
    return false;
  }

  createFont();
  return true;

#elif __APPLE__
  return false;

#else
  display = XOpenDisplay(nullptr);
  if (!display)
    return false;

  Window x11Window = (Window)(uintptr_t)win;
  gc = XCreateGC(display, x11Window, 0, nullptr);
  if (!gc)
  {
    XCloseDisplay(display);
    return false;
  }

  fontInfo = XLoadQueryFont(display, "fixed");
  if (!fontInfo)
  {
    fontInfo = XLoadQueryFont(display, "*");
  }
  if (fontInfo)
  {
    XSetFont(display, gc, fontInfo->fid);
  }

  return true;
#endif
}

void RenderManager::cleanup()
{
#ifdef _WIN32
  destroyFont();

  if (memBitmap)
  {
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    memBitmap = nullptr;
  }

  if (memDC)
  {
    DeleteDC(memDC);
    memDC = nullptr;
  }

  if (hdc)
  {
    ReleaseDC((HWND)window, hdc);
    hdc = nullptr;
  }

#elif __APPLE__
  if (backBuffer)
  {
    PlatformFree(backBuffer);
    backBuffer = nullptr;
  }

#else
  if (backBuffer)
  {
    XFreePixmap(display, backBuffer);
    backBuffer = 0;
  }

  if (fontInfo)
  {
    XFreeFont(display, fontInfo);
    fontInfo = nullptr;
  }

  if (gc)
  {
    XFreeGC(display, gc);
    gc = nullptr;
  }

  if (display)
  {
    XCloseDisplay(display);
    display = nullptr;
  }
#endif
}

void RenderManager::resize(int width, int height)
{
#ifdef _WIN32
  if (!memDC)
    return;
#elif __APPLE__
  if (!context)
    return;
#else
  if (!display || !window)
    return;
#endif

  if (width <= 0 || height <= 0)
    return;
  if (width > 8192 || height > 8192)
    return;

  windowWidth = width;
  windowHeight = height;

#ifdef _WIN32
  if (memBitmap)
  {
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
  }

  MemSet(&bitmapInfo, 0, sizeof(BITMAPINFO));
  bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = -height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;

  memBitmap = CreateDIBSection(memDC, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
  oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);

#elif __APPLE__
  if (backBuffer)
  {
    PlatformFree(backBuffer);
  }
  backBuffer = PlatformAlloc(width * height * 4);

#else
  if (backBuffer)
  {
    XFreePixmap(display, backBuffer);
  }
  Window x11Window = (Window)(uintptr_t)this->window;
  backBuffer = XCreatePixmap(display, x11Window, width, height,
                             DefaultDepth(display, DefaultScreen(display)));

#endif
}

void RenderManager::UpdateHexMetrics(int leftPanelWidth, int menuBarHeight)
{
  LayoutMetrics layout;

#ifdef _WIN32
  if (memDC)
  {
    SIZE textSize;
    if (GetTextExtentPoint32A(memDC, "0", 1, &textSize))
    {
      layout.charWidth = (float)textSize.cx;
      layout.lineHeight = (float)textSize.cy;
    }
  }
#elif __APPLE__
  layout.charWidth = 9.6f;
  layout.lineHeight = 20.0f;
#else
  if (fontInfo)
  {
    layout.charWidth = (float)fontInfo->max_bounds.width;
    layout.lineHeight = (float)(fontInfo->ascent + fontInfo->descent);
  }
#endif

  layout.margin = 10.0f;
  layout.headerHeight = layout.lineHeight;

  _charWidth = (int)layout.charWidth;
  _charHeight = (int)layout.lineHeight;

  _hexAreaX = leftPanelWidth + (int)(layout.margin + (10 * layout.charWidth));
  _hexAreaY = menuBarHeight + (int)(layout.margin + layout.headerHeight + 2);
}

bool RenderManager::IsPointInHexArea(int mouseX, int mouseY, int leftPanelWidth,
                                     int menuBarHeight, int windowWidth, int windowHeight)
{
  int hexAreaLeft = _hexAreaX;
  int hexAreaTop = _hexAreaY;

  int hexAreaRight = _hexAreaX + (16 * 3 * _charWidth);

  int hexAreaBottom = _hexAreaY + (_visibleLines * _charHeight);

  return (mouseX >= hexAreaLeft && mouseX <= hexAreaRight &&
          mouseY >= hexAreaTop && mouseY <= hexAreaBottom);
}

void RenderManager::createFont()
{
#ifdef _WIN32
  font = CreateFontA(
      16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
      DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

  if (font && memDC)
  {
    SelectObject(memDC, font);
    SIZE textSize;
    GetTextExtentPoint32A(memDC, "0", 1, &textSize);
  }
#endif
}

void RenderManager::destroyFont()
{
#ifdef _WIN32
  if (font)
  {
    DeleteObject(font);
    font = nullptr;
  }
#endif
}

void RenderManager::beginFrame()
{
}

void RenderManager::endFrame(NativeDrawContext ctx)
{
#ifdef _WIN32
  if (!ctx || !memDC)
    return;

  BitBlt(ctx, 0, 0, windowWidth, windowHeight, memDC, 0, 0, SRCCOPY);

#elif __APPLE__
  if (!context)
    return;

  CGContextDrawImage(ctx,
                     CGRectMake(0, 0, windowWidth, windowHeight),
                     (CGImageRef)backBuffer);

#else
  if (!display || !gc)
    return;

  Window x11Window = (Window)(uintptr_t)window;

  XCopyArea(display, backBuffer, x11Window, gc,
            0, 0, windowWidth, windowHeight, 0, 0);

  XFlush(display);
#endif
}

void RenderManager::clear(const Color &color)
{
#ifdef _WIN32
  RECT rect = {0, 0, windowWidth, windowHeight};
  HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
  FillRect(memDC, &rect, brush);
  DeleteObject(brush);

#elif __APPLE__
  if (backBuffer)
  {
    unsigned int *buf = (unsigned int *)backBuffer;
    unsigned int colorValue = (color.r << 16) | (color.g << 8) | color.b;
    for (int i = 0; i < windowWidth * windowHeight; i++)
    {
      buf[i] = colorValue;
    }
  }

#else
  XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
  XFillRectangle(display, backBuffer, gc, 0, 0, windowWidth, windowHeight);
#endif
}

void RenderManager::setColor(const Color &color)
{
#ifdef _WIN32
  SetTextColor(memDC, RGB(color.r, color.g, color.b));
  SetBkMode(memDC, TRANSPARENT);
#elif __APPLE__
#else
  XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
#endif
}

void RenderManager::drawRect(const Rect &rect, const Color &color, bool filled)
{
#ifdef _WIN32
  if (filled)
  {
    RECT r = {rect.x, rect.y, rect.x + rect.width, rect.y + rect.height};
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    FillRect(memDC, &r, brush);
    DeleteObject(brush);
  }
  else
  {
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
  XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
  if (filled)
  {
    XFillRectangle(display, backBuffer, gc, rect.x, rect.y, rect.width, rect.height);
  }
  else
  {
    XDrawRectangle(display, backBuffer, gc, rect.x, rect.y, rect.width, rect.height);
  }
#endif
}

void RenderManager::drawLine(int x1, int y1, int x2, int y2, const Color &color)
{
#ifdef _WIN32
  HPEN pen = CreatePen(PS_SOLID, 1, RGB(color.r, color.g, color.b));
  HPEN oldPen = (HPEN)SelectObject(memDC, pen);
  MoveToEx(memDC, x1, y1, nullptr);
  LineTo(memDC, x2, y2);
  SelectObject(memDC, oldPen);
  DeleteObject(pen);
#elif __APPLE__
#else
  XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
  XDrawLine(display, backBuffer, gc, x1, y1, x2, y2);
#endif
}

void RenderManager::drawText(const char *text, int x, int y, const Color &color)
{
  if (!text)
    return;

#ifdef _WIN32
  setColor(color);
  TextOutA(memDC, x, y, text, (int)StrLen(text));
#elif __APPLE__
#else
  XSetForeground(display, gc, (color.r << 16) | (color.g << 8) | color.b);
  XDrawString(display, backBuffer, gc, x, y + 12, text, StrLen(text));
#endif
}

void RenderManager::drawRoundedRect(const Rect &rect, float radius, const Color &color, bool filled)
{
#ifdef _WIN32
  if (filled)
  {
    HRGN rgn = CreateRoundRectRgn(rect.x, rect.y, rect.x + rect.width,
                                  rect.y + rect.height, (int)radius * 2, (int)radius * 2);
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    FillRgn(memDC, rgn, brush);
    DeleteObject(brush);
    DeleteObject(rgn);
  }
  else
  {
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

const int PANEL_TITLE_HEIGHT = 28;

Rect GetLeftPanelBounds(const LeftPanelState &state, int windowWidth, int windowHeight, int menuBarHeight)
{
  switch (state.dockPosition)
  {
  case PanelDockPosition::Left:
    return Rect(0, menuBarHeight, state.width, windowHeight - menuBarHeight);
  case PanelDockPosition::Right:
    return Rect(windowWidth - state.width, menuBarHeight, state.width, windowHeight - menuBarHeight);
  case PanelDockPosition::Top:
    return Rect(0, menuBarHeight, windowWidth, state.height);
  case PanelDockPosition::Bottom:
    return Rect(0, windowHeight - state.height, windowWidth, state.height);
  case PanelDockPosition::Floating:
    return Rect(state.floatingX, state.floatingY, state.floatingWidth, state.floatingHeight);
  }
  return Rect(0, 0, 0, 0);
}

Rect GetBottomPanelBounds(const BottomPanelState &state, int windowWidth, int windowHeight,
                          int menuBarHeight, const LeftPanelState &leftPanel)
{
  int leftOffset = (leftPanel.visible && leftPanel.dockPosition == PanelDockPosition::Left) ? leftPanel.width : 0;
  int rightOffset = (leftPanel.visible && leftPanel.dockPosition == PanelDockPosition::Right) ? leftPanel.width : 0;
  int topOffset = (leftPanel.visible && leftPanel.dockPosition == PanelDockPosition::Top) ? leftPanel.height : 0;

  switch (state.dockPosition)
  {
  case PanelDockPosition::Left:
    return Rect(leftOffset, menuBarHeight + topOffset, state.width, windowHeight - menuBarHeight - topOffset);
  case PanelDockPosition::Right:
    return Rect(windowWidth - state.width - rightOffset, menuBarHeight + topOffset,
                state.width, windowHeight - menuBarHeight - topOffset);
  case PanelDockPosition::Top:
    return Rect(leftOffset, menuBarHeight + topOffset, windowWidth - leftOffset - rightOffset, state.height);
  case PanelDockPosition::Bottom:
    return Rect(leftOffset, windowHeight - state.height,
                windowWidth - leftOffset - rightOffset, state.height);
  case PanelDockPosition::Floating:
    return Rect(state.floatingX, state.floatingY, state.floatingWidth, state.floatingHeight);
  }
  return Rect(0, 0, 0, 0);
}

bool IsInPanelTitleBar(int mouseX, int mouseY, const Rect &panelBounds)
{
  return mouseX >= panelBounds.x &&
         mouseX <= panelBounds.x + panelBounds.width &&
         mouseY >= panelBounds.y &&
         mouseY <= panelBounds.y + PANEL_TITLE_HEIGHT;
}

PanelDockPosition GetDockPositionFromMouse(int mouseX, int mouseY, int windowWidth, int windowHeight, int menuBarHeight)
{
  const int DOCK_THRESHOLD = 50;

  if (mouseX < DOCK_THRESHOLD)
    return PanelDockPosition::Left;
  if (mouseX > windowWidth - DOCK_THRESHOLD)
    return PanelDockPosition::Right;
  if (mouseY < menuBarHeight + DOCK_THRESHOLD)
    return PanelDockPosition::Top;
  if (mouseY > windowHeight - DOCK_THRESHOLD)
    return PanelDockPosition::Bottom;

  return PanelDockPosition::Floating;
}

void RenderManager::drawModernButton(const WidgetState &state, const Theme &theme, const char *label)
{
  float radius = 6.0f;

  Color fillColor = theme.buttonNormal;
  if (!state.enabled)
  {
    fillColor = theme.buttonDisabled;
  }
  else if (state.pressed)
  {
    fillColor = theme.buttonPressed;
  }
  else if (state.hovered)
  {
    fillColor = theme.buttonHover;
  }

  drawRoundedRect(state.rect, radius, fillColor, true);

  if (state.enabled)
  {
    Color borderColor = state.pressed ? Color(fillColor.r - 20, fillColor.g - 20, fillColor.b - 20) : Color(fillColor.r - 10, fillColor.g - 10, fillColor.b - 10);
    drawRoundedRect(state.rect, radius, borderColor, false);
  }

  int textWidth = StrLen(label) * 8;
  int textHeight = 16;
  int textX = state.rect.x + (state.rect.width - textWidth) / 2;
  int textY = state.rect.y + (state.rect.height - textHeight) / 2;

  Color textColor = state.enabled ? theme.buttonText : Color(theme.buttonText.r / 2, theme.buttonText.g / 2, theme.buttonText.b / 2);
  drawText(label, textX, textY, textColor);
}

void RenderManager::drawModernCheckbox(const WidgetState &state, const Theme &theme, bool checked)
{
  float radius = 3.0f;
  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered)
  {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  if (checked)
  {
    bgColor = theme.controlCheck;
    borderColor = theme.controlCheck;
  }

  drawRoundedRect(state.rect, radius, bgColor, true);

  if (!checked || state.hovered)
  {
    drawRoundedRect(state.rect, radius, borderColor, false);
  }

  if (checked)
  {
    Color checkColor = Color(255, 255, 255);
    int cx = state.rect.x + 4;
    int cy = state.rect.y + 9;
    drawLine(cx, cy, cx + 3, cy + 4, checkColor);
    drawLine(cx + 1, cy, cx + 4, cy + 4, checkColor);
    drawLine(cx + 3, cy + 4, cx + 10, cy - 4, checkColor);
    drawLine(cx + 4, cy + 4, cx + 11, cy - 4, checkColor);
  }
}

void RenderManager::drawModernRadioButton(const WidgetState &state, const Theme &theme, bool selected)
{
#ifdef _WIN32
  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered)
  {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  int centerX = state.rect.x + state.rect.width / 2;
  int centerY = state.rect.y + state.rect.height / 2;
  int outerRadius = state.rect.width / 2;

  HBRUSH bgBrush = CreateSolidBrush(RGB(bgColor.r, bgColor.g, bgColor.b));
  HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(borderColor.r, borderColor.g, borderColor.b));

  HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, bgBrush);
  HPEN oldPen = (HPEN)SelectObject(memDC, borderPen);

  Ellipse(memDC, centerX - outerRadius, centerY - outerRadius,
          centerX + outerRadius, centerY + outerRadius);

  SelectObject(memDC, oldBrush);
  SelectObject(memDC, oldPen);
  DeleteObject(bgBrush);
  DeleteObject(borderPen);

  if (selected)
  {
    int innerRadius = outerRadius - 4;
    Color dotColor = theme.controlCheck;

    HBRUSH dotBrush = CreateSolidBrush(RGB(dotColor.r, dotColor.g, dotColor.b));
    HPEN dotPen = CreatePen(PS_SOLID, 1, RGB(dotColor.r, dotColor.g, dotColor.b));

    oldBrush = (HBRUSH)SelectObject(memDC, dotBrush);
    oldPen = (HPEN)SelectObject(memDC, dotPen);

    Ellipse(memDC, centerX - innerRadius, centerY - innerRadius,
            centerX + innerRadius, centerY + innerRadius);

    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldPen);
    DeleteObject(dotBrush);
    DeleteObject(dotPen);
  }

#elif __APPLE__
#else
  Color bgColor = theme.controlBackground;
  Color borderColor = theme.controlBorder;

  if (state.hovered)
  {
    borderColor = Color(borderColor.r + 30, borderColor.g + 30, borderColor.b + 30);
  }

  int centerX = state.rect.x + state.rect.width / 2;
  int centerY = state.rect.y + state.rect.height / 2;
  int outerRadius = state.rect.width / 2;

  XSetForeground(display, gc, (bgColor.r << 16) | (bgColor.g << 8) | bgColor.b);
  XFillArc(display, backBuffer, gc, state.rect.x, state.rect.y,
           state.rect.width, state.rect.height, 0, 360 * 64);

  XSetForeground(display, gc, (borderColor.r << 16) | (borderColor.g << 8) | borderColor.b);
  XDrawArc(display, backBuffer, gc, state.rect.x, state.rect.y,
           state.rect.width, state.rect.height, 0, 360 * 64);

  if (selected)
  {
    int innerRadius = outerRadius - 4;
    Color dotColor = theme.controlCheck;

    XSetForeground(display, gc, (dotColor.r << 16) | (dotColor.g << 8) | dotColor.b);
    XFillArc(display, backBuffer, gc, centerX - innerRadius, centerY - innerRadius,
             innerRadius * 2, innerRadius * 2, 0, 360 * 64);
  }
#endif
}

void RenderManager::drawDropdown(
    const WidgetState &state,
    const Theme &theme,
    const char *selectedText,
    bool isOpen,
    const Vector<char *> &items,
    int selectedIndex,
    int hoveredIndex,
    int scrollOffset)
{
  float radius = 4.0f;

  Color bgColor = theme.controlBackground;
  bgColor.a = 255;

  drawRect(state.rect, bgColor, true);

  Color borderColor = theme.controlBorder;

  if (state.hovered && !isOpen)
  {
    borderColor = Color(
        Clamp(borderColor.r + 30, 0, 255),
        Clamp(borderColor.g + 30, 0, 255),
        Clamp(borderColor.b + 30, 0, 255));
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

  if (isOpen)
  {
    drawLine(arrowX + 4, arrowY - 3, arrowX, arrowY + 2, arrowColor);
    drawLine(arrowX + 4, arrowY - 3, arrowX + 8, arrowY + 2, arrowColor);
    drawLine(arrowX, arrowY + 2, arrowX + 8, arrowY + 2, arrowColor);
  }
  else
  {
    drawLine(arrowX, arrowY - 2, arrowX + 8, arrowY - 2, arrowColor);
    drawLine(arrowX, arrowY - 2, arrowX + 4, arrowY + 3, arrowColor);
    drawLine(arrowX + 8, arrowY - 2, arrowX + 4, arrowY + 3, arrowColor);
  }

  if (isOpen && !items.empty())
  {
    int itemHeight = 28;
    int maxVisibleItems = 3;

    int totalItems = (int)items.size();

    int maxScroll = Clamp(totalItems - maxVisibleItems, 0, 0x7FFFFFFF);
    scrollOffset = Clamp(scrollOffset, 0, maxScroll);
    int remaining = totalItems - scrollOffset;
    int visibleItems = Clamp(remaining, 0, maxVisibleItems);

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

    for (int visualIndex = 0; visualIndex < visibleItems; visualIndex++)
    {
      size_t actualIndex = scrollOffset + visualIndex;

      Rect itemRect(
          state.rect.x + 1,
          listY + (visualIndex * itemHeight) + 1,
          state.rect.width - 2,
          itemHeight - 1);

      bool isSelected = ((int)actualIndex == selectedIndex);
      bool isHovered = ((int)actualIndex == hoveredIndex);

      Color itemBg;

      if (isHovered)
      {
        itemBg = theme.menuHover;
      }
      else if (isSelected)
      {
        itemBg = Color(
            (theme.controlCheck.r + theme.menuBackground.r) / 2,
            (theme.controlCheck.g + theme.menuBackground.g) / 2,
            (theme.controlCheck.b + theme.menuBackground.b) / 2);
      }
      else
      {
        itemBg = listBg;
      }

      itemBg.a = 255;

      drawRect(itemRect, itemBg, true);

      int itemTextX = itemRect.x + 10;
      int itemTextY = itemRect.y + (itemRect.height / 2) - 6;

      Color textColor = theme.textColor;
      if (isSelected)
      {
        textColor = Color(
            Clamp(textColor.r + 20, 0, 255),
            Clamp(textColor.g + 20, 0, 255),
            Clamp(textColor.b + 20, 0, 255));
      }

      drawText(items[actualIndex], itemTextX, itemTextY, textColor);

      if (isSelected)
      {
        int checkX = itemRect.x + itemRect.width - 20;
        int checkY = itemRect.y + itemRect.height / 2;
        Color checkColor = theme.controlCheck;

        drawLine(checkX, checkY, checkX + 2, checkY + 3, checkColor);
        drawLine(checkX + 2, checkY + 3, checkX + 6, checkY - 2, checkColor);
        drawLine(checkX + 1, checkY, checkX + 3, checkY + 3, checkColor);
        drawLine(checkX + 3, checkY + 3, checkX + 7, checkY - 2, checkColor);
      }

      if (visualIndex < visibleItems - 1)
      {
        Color separatorColor = Color(
            theme.separator.r,
            theme.separator.g,
            theme.separator.b,
            100);
        drawLine(
            itemRect.x + 8,
            itemRect.y + itemRect.height,
            itemRect.x + itemRect.width - 8,
            itemRect.y + itemRect.height,
            separatorColor);
      }
    }

    if (scrollOffset > 0)
    {
      int arrowTopX = state.rect.x + state.rect.width / 2 - 4;
      int arrowTopY = listY + 8;
      Color scrollHint = theme.textColor;
      drawText("^", arrowTopX, arrowTopY - 4, scrollHint);
    }

    if (scrollOffset < maxScroll)
    {
      int arrowBottomX = state.rect.x + state.rect.width / 2 - 4;
      int arrowBottomY = listY + listHeight - 12;
      Color scrollHint = theme.textColor;
      drawText("v", arrowBottomX, arrowBottomY, scrollHint);
    }
  }
}

PointF RenderManager::GetBytePointF(long long byteIndex)
{
  Point gp = GetGridBytePoint(byteIndex);
  return GetBytePointF(gp);
}

PointF RenderManager::GetBytePointF(Point gp)
{
  float x = (3.0f * _charWidth) * gp.X + _hexAreaX;
  float y = gp.Y * _charHeight + _hexAreaY;
  return PointF(x, y);
}

Point RenderManager::GetGridBytePoint(long long byteIndex)
{
  int row = (int)Floor((double)byteIndex / (double)_bytesPerLine);
  int column = (int)(byteIndex % _bytesPerLine);
  return Point(column, row);
}

BytePositionInfo RenderManager::GetHexBytePositionInfo(Point screenPoint)
{
  int relX = screenPoint.X - _hexAreaX;
  int relY = screenPoint.Y - _hexAreaY;

  if (relX < 0)
  {
    return BytePositionInfo(-1, 0);
  }

  int row = relY / _charHeight;
  if (row < 0)
    row = 0;

  int charX = relX / _charWidth;

  if (charX < 0)
    charX = 0;

  int column = charX / 3;
  int byteCharPos = charX % 3;

  if (byteCharPos >= 2)
  {
    byteCharPos = 1;
  }

  if (column >= _bytesPerLine)
  {
    column = _bytesPerLine - 1;
    byteCharPos = 1;
  }
  if (column < 0)
  {
    column = 0;
    byteCharPos = 0;
  }

  long long bytePos = _startByte + ((long long)row * _bytesPerLine) + column;

  extern HexData g_HexData;
  long long maxPos = (long long)g_HexData.getFileSize() - 1;
  if (maxPos < 0)
  {
    maxPos = 0;
    bytePos = 0;
    byteCharPos = 0;
  }

  if (bytePos < 0)
  {
    bytePos = 0;
    byteCharPos = 0;
  }
  if (bytePos > maxPos)
  {
    bytePos = maxPos;
    byteCharPos = 1;
  }

  return BytePositionInfo(bytePos, byteCharPos);
}

void RenderManager::UpdateCaret()
{
}

CaretInfo RenderManager::GetCaretPosition()
{
  CaretInfo ci;

  if (g_PatternSearch.hasFocus)
  {
    ci.x = g_SearchCaretX;
    ci.y = g_SearchCaretY;
    ci.width = 2;
    ci.height = _charHeight;
    return ci;
  }

  PointF p = GetBytePointF(_bytePos - _startByte);
  p.X += _byteCharacterPos * _charWidth;

  ci.x = (int)p.X;
  ci.y = (int)p.Y;
  ci.width = 2;
  ci.height = _charHeight;
  return ci;
}

void RenderManager::DrawCaret()
{
  if (!caretVisible)
    return;

  CaretInfo ci = GetCaretPosition();
  drawRect(Rect(ci.x, ci.y, ci.width, ci.height), currentTheme.textColor, true);
}

long long RenderManager::ScreenToByteIndex(int mouseX, int mouseY)
{
  BytePositionInfo info = GetHexBytePositionInfo(Point(mouseX, mouseY));

  extern HexData g_HexData;
  if (info.Index < 0)
    return 0;
  if (info.Index >= (long long)g_HexData.getFileSize())
  {
    return g_HexData.getFileSize() - 1;
  }

  return info.Index;
}

#ifdef _WIN32
void RenderManager::drawBitmap(void *hBitmap, int width, int height, int x, int y)
{
  if (!memDC || !hBitmap)
    return;
  HDC tempDC = CreateCompatibleDC(memDC);
  HBITMAP oldBmp = (HBITMAP)SelectObject(tempDC, (HBITMAP)hBitmap);
  BitBlt(memDC, x, y, width, height, tempDC, 0, 0, SRCCOPY);
  SelectObject(tempDC, oldBmp);
  DeleteDC(tempDC);
}
#endif

#ifdef __APPLE__
void RenderManager::drawImage(void *image, int width, int height, int x, int y)
{
  if (!backBuffer || !image)
    return;
}
#endif

#ifdef __linux__
void RenderManager::drawX11Pixmap(Pixmap pixmap, int width, int height, int x, int y)
{
  if (!display || !backBuffer || !pixmap)
    return;
  GC tempGC = XCreateGC(display, backBuffer, 0, nullptr);
  XCopyArea(display, pixmap, backBuffer, tempGC, 0, 0, width, height, x, y);
  XFreeGC(display, tempGC);
}
#endif

int MeasureTextHeight(const char *text)
{
  return 16;
}

void RenderManager::drawProgressBar(const Rect &rect, float progress, const Theme &theme)
{
  progress = Clamp(progress, 0.0f, 1.0f);

  float radius = 4.0f;

  Color bgColor(50, 50, 50);
  if (theme.windowBackground.r > 128)
  {
    bgColor = Color(220, 220, 220);
  }
  drawRoundedRect(rect, radius, bgColor, true);

  int fillWidth = (int)(rect.width * progress);
  if (fillWidth > 2)
  {
    Rect fillRect(rect.x, rect.y, fillWidth, rect.height);
    Color fillColor(100, 150, 255);
    drawRoundedRect(fillRect, radius, fillColor, true);

    if (rect.height > 10)
    {
      Rect highlightRect(rect.x + 2, rect.y + 2, fillWidth - 4, rect.height / 3);
      Color highlightColor(
          Clamp(fillColor.r + 40, 0, 255),
          Clamp(fillColor.g + 40, 0, 255),
          Clamp(fillColor.b + 40, 0, 255),
          150);
      if (highlightRect.width > 0)
      {
        drawRect(highlightRect, highlightColor, true);
      }
    }
  }

  Color borderColor(80, 80, 80);
  if (theme.windowBackground.r > 128)
  {
    borderColor = Color(180, 180, 180);
  }
  drawRoundedRect(rect, radius, borderColor, false);
}

void RenderManager::drawLeftPanel(
    const LeftPanelState &state,
    const Theme &theme,
    int windowHeight,
    const Rect &panelBounds)
{
  if (!state.visible)
    return;

  Color panelBg(
      theme.windowBackground.r - 10,
      theme.windowBackground.g - 10,
      theme.windowBackground.b - 10);

  if (theme.windowBackground.r < 50)
    panelBg = Color(35, 35, 40);

  drawRect(panelBounds, panelBg, true);

  Rect titleBar(panelBounds.x, panelBounds.y, panelBounds.width, PANEL_TITLE_HEIGHT);
  Color titleBg = state.dragging ? theme.controlCheck
                                 : Color(panelBg.r + 15, panelBg.g + 15, panelBg.b + 15);
  drawRect(titleBar, titleBg, true);

  const char *dockText = "";
  switch (state.dockPosition)
  {
  case PanelDockPosition::Left:
    dockText = " [Left]";
    break;
  case PanelDockPosition::Right:
    dockText = " [Right]";
    break;
  case PanelDockPosition::Top:
    dockText = " [Top]";
    break;
  case PanelDockPosition::Bottom:
    dockText = " [Bottom]";
    break;
  case PanelDockPosition::Floating:
    dockText = " [Float]";
    break;
  }

  char titleText[64];
  StrCopy(titleText, "File Explorer");
  StrCat(titleText, dockText);
  drawText(titleText, panelBounds.x + 10, panelBounds.y + 7, theme.textColor);

  drawRect(panelBounds, theme.separator, false);

  int contentX = panelBounds.x + 15;
  int currentY = panelBounds.y + PANEL_TITLE_HEIGHT + 10;
  int contentWidth = panelBounds.width - 30;

  int rowHeight = 16;
  int headerHeight = 18;
  int itemSpacing = 4;
  int sectionSpacing = 10;

  char buf[256];
  extern HexData g_HexData;
  extern long long cursorBytePos;
  extern BookmarksState g_Bookmarks;
  extern ByteStatistics g_ByteStats;
  extern DetectItEasyState g_DIEState;

  long long fileSize = (long long)g_HexData.getFileSize();

  Color faded(
      theme.textColor.r - 40,
      theme.textColor.g - 40,
      theme.textColor.b - 40);

  Color halfText(
      theme.textColor.r / 2,
      theme.textColor.g / 2,
      theme.textColor.b / 2);

  Color thirdText(
      theme.textColor.r / 3,
      theme.textColor.g / 3,
      theme.textColor.b / 3);

  drawText("File Information", contentX, currentY, theme.headerColor);
  currentY += headerHeight + sectionSpacing;

  if (fileSize < 1024)
  {
    ItoaDec(fileSize, buf, 256);
    StrCat(buf, " bytes");
  }
  else if (fileSize < 1024 * 1024)
  {
    ItoaDec(fileSize / 1024, buf, 256);
    StrCat(buf, " KB");
  }
  else
  {
    ItoaDec(fileSize / (1024 * 1024), buf, 256);
    StrCat(buf, " MB");
  }
  drawText("Size:", contentX, currentY, faded);
  drawText(buf, contentX + 85, currentY, theme.textColor);
  currentY += rowHeight + itemSpacing;

  const char *typeStr = "Unknown";
  if (fileSize >= 4)
  {
    uint8_t b0 = g_HexData.getByte(0);
    uint8_t b1 = g_HexData.getByte(1);
    uint8_t b2 = g_HexData.getByte(2);
    uint8_t b3 = g_HexData.getByte(3);

    typeStr = "Binary Data";
    if (b0 == 0x4D && b1 == 0x5A)
      typeStr = "PE Executable";
    else if (b0 == 0x7F && b1 == 'E' && b2 == 'L' && b3 == 'F')
      typeStr = "ELF Executable";
    else if (b0 == 0x89 && b1 == 'P' && b2 == 'N' && b3 == 'G')
      typeStr = "PNG Image";
    else if (b0 == 0xFF && b1 == 0xD8 && b2 == 0xFF)
      typeStr = "JPEG Image";
  }
  drawText("Type:", contentX, currentY, faded);
  drawText(typeStr, contentX + 85, currentY, Color(100, 200, 150));
  currentY += rowHeight + itemSpacing;

  currentY += 8;

  drawText("Data Inspector", contentX, currentY, theme.headerColor);
  currentY += headerHeight + sectionSpacing;

  if (cursorBytePos >= 0 && cursorBytePos < fileSize)
  {
    uint8_t byteVal = g_HexData.getByte((size_t)cursorBytePos);

    StrCopy(buf, "0x");
    ItoaHex(cursorBytePos, buf + 2, 254);
    drawText("Offset:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, Color(100, 150, 255));
    currentY += rowHeight + itemSpacing;

    ItoaDec(byteVal, buf, 256);
    drawText("Uint8:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, theme.textColor);
    currentY += rowHeight + itemSpacing;

    ItoaDec((int8_t)byteVal, buf, 256);
    drawText("Int8:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, theme.textColor);
    currentY += rowHeight + itemSpacing;

    StrCopy(buf, "0x");
    ByteToHex(byteVal, buf + 2);
    buf[4] = 0;
    drawText("Hex:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, theme.textColor);
    currentY += rowHeight + itemSpacing;

    if (byteVal >= 32 && byteVal < 127)
    {
      buf[0] = '\'';
      buf[1] = (char)byteVal;
      buf[2] = '\'';
      buf[3] = 0;
    }
    else
    {
      StrCopy(buf, ".");
    }
    drawText("ASCII:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, theme.textColor);
    currentY += rowHeight + itemSpacing;

    currentY += 8;
  }
  else
  {
    drawText("Move cursor to view data", contentX, currentY, halfText);
    currentY += rowHeight + itemSpacing;
  }

  drawText("Bookmarks", contentX, currentY, theme.headerColor);
  currentY += headerHeight + sectionSpacing;

  if (g_Bookmarks.bookmarks.empty())
  {
    drawText("No bookmarks", contentX, currentY, halfText);
    currentY += rowHeight + itemSpacing;
    drawText("Ctrl+B to add", contentX, currentY, thirdText);
    currentY += rowHeight + itemSpacing;
  }
  else
  {
    for (size_t i = 0; i < g_Bookmarks.bookmarks.size() && i < 5; i++)
    {
      const Bookmark &bm = g_Bookmarks.bookmarks[i];
      bool selected = (g_Bookmarks.selectedIndex == (int)i);

      Rect colorBox(contentX, currentY + 3, 10, 10);
      drawRect(colorBox, bm.color, true);

      if (selected)
      {
        Color selBg = theme.controlCheck;
        selBg.a = 40;
        Rect selRect(contentX, currentY, contentWidth, rowHeight);
        drawRect(selRect, selBg, true);
      }

      int textX = contentX + 15;
      drawText(bm.name, textX, currentY, theme.textColor);
      currentY += rowHeight + itemSpacing;
    }
  }

  currentY += 8;

  drawText("Byte Statistics", contentX, currentY, theme.headerColor);
  currentY += headerHeight + sectionSpacing;

  if (!g_ByteStats.computed)
  {
    drawText("Click to compute", contentX, currentY, halfText);
    currentY += rowHeight + itemSpacing;
  }
  else
  {
    StrCopy(buf, "0x");
    ByteToHex(g_ByteStats.mostCommonByte, buf + 2);
    buf[4] = 0;
    drawText("Most Common:", contentX, currentY, faded);
    drawText(buf, contentX + 85, currentY, theme.textColor);
    currentY += rowHeight + itemSpacing;
  }

  currentY += 8;

  drawText("Detect it Easy", contentX, currentY, theme.headerColor);
  currentY += headerHeight + sectionSpacing;

  drawText("File Type:", contentX, currentY, faded);
  drawText(g_DIEState.fileType[0] ? g_DIEState.fileType : "Coming Soon",
           contentX + 85, currentY, Color(100, 200, 150));
  currentY += rowHeight + itemSpacing;

  drawText("Compiler:", contentX, currentY, faded);
  drawText(g_DIEState.compiler[0] ? g_DIEState.compiler : "Coming Soon",
           contentX + 85, currentY, Color(100, 200, 150));
  currentY += rowHeight + itemSpacing;

  drawText("Arch:", contentX, currentY, faded);
  drawText(g_DIEState.architecture[0] ? g_DIEState.architecture : "Coming Soon",
           contentX + 85, currentY, Color(100, 200, 150));
  currentY += rowHeight + itemSpacing;

  char diePath[260];
  bool dieFound = FindDIEPath(diePath, sizeof(diePath));

  if (dieFound)
  {
    StrCopy(g_DIEExecutablePath, diePath);

    currentY += 4;
    
    Rect separatorRect(contentX, currentY, contentWidth, 1);
    drawRect(separatorRect, theme.separator, true);
    currentY += itemSpacing;

    Rect buttonRect(contentX, currentY, contentWidth, rowHeight + 10);
    WidgetState ws;
    ws.rect = buttonRect;
    ws.enabled = true;
    ws.hovered = false;
    ws.pressed = false;
    drawModernButton(ws, theme, "Open in DIE");
    currentY += rowHeight + 10 + itemSpacing;
  }

  if (state.dockPosition == PanelDockPosition::Floating)
  {
    Rect resizeHandle(
        panelBounds.x + panelBounds.width - 10,
        panelBounds.y + panelBounds.height - 10,
        10, 10);
    Color handleColor = theme.controlCheck;
    handleColor.a = state.resizing ? 150 : 30;
    drawRect(resizeHandle, handleColor, true);
  }
  else if (state.dockPosition == PanelDockPosition::Left)
  {
    Rect resizeHandle(
        panelBounds.x + panelBounds.width - 3,
        panelBounds.y + PANEL_TITLE_HEIGHT,
        6,
        panelBounds.height - PANEL_TITLE_HEIGHT);
    Color handleColor = theme.controlCheck;
    handleColor.a = state.resizing ? 150 : 30;
    drawRect(resizeHandle, handleColor, true);
  }
  else if (state.dockPosition == PanelDockPosition::Right)
  {
    Rect resizeHandle(
        panelBounds.x,
        panelBounds.y + PANEL_TITLE_HEIGHT,
        6,
        panelBounds.height - PANEL_TITLE_HEIGHT);
    Color handleColor = theme.controlCheck;
    handleColor.a = state.resizing ? 150 : 30;
    drawRect(resizeHandle, handleColor, true);
  }
}

void RenderManager::drawBottomPanel(
    const BottomPanelState &state,
    const Theme &theme,
    const ChecksumResults &checksums,
    int windowWidth,
    int windowHeight,
    const Rect &panelBounds)
{
  if (!state.visible)
    return;

  Color panelBg = (theme.windowBackground.r < 50)
                      ? Color(35, 35, 40)
                      : Color(theme.windowBackground.r - 10,
                              theme.windowBackground.g - 10,
                              theme.windowBackground.b - 10);

  drawRect(panelBounds, panelBg, true);

  Rect titleBar(panelBounds.x, panelBounds.y, panelBounds.width, PANEL_TITLE_HEIGHT);
  Color titleBg = state.dragging ? theme.controlCheck
                                 : Color(panelBg.r + 15, panelBg.g + 15, panelBg.b + 15);
  drawRect(titleBar, titleBg, true);

  const char *dockText = "";
  switch (state.dockPosition)
  {
  case PanelDockPosition::Left:
    dockText = " [Left]";
    break;
  case PanelDockPosition::Right:
    dockText = " [Right]";
    break;
  case PanelDockPosition::Top:
    dockText = " [Top]";
    break;
  case PanelDockPosition::Bottom:
    dockText = " [Bottom]";
    break;
  case PanelDockPosition::Floating:
    dockText = " [Float]";
    break;
  }

  char titleText[64];
  StrCopy(titleText, "Analysis Panel");
  StrCat(titleText, dockText);
  drawText(titleText, panelBounds.x + 10, panelBounds.y + 7, theme.textColor);

  drawRect(panelBounds, theme.separator, false);

  int tabHeight = 32;
  bool isVertical = (state.dockPosition == PanelDockPosition::Left ||
                     state.dockPosition == PanelDockPosition::Right);

  const char *tabLabels[] = {
      isVertical ? "Entropy" : "Entropy Analysis",
      isVertical ? "Search" : "Hex Pattern Search",
      "Checksum",
      "Compare"};

  BottomPanelState::Tab tabs[] = {
      BottomPanelState::Tab::EntropyAnalysis,
      BottomPanelState::Tab::PatternSearch,
      BottomPanelState::Tab::Checksum,
      BottomPanelState::Tab::Compare};

  if (isVertical)
  {
    int y = panelBounds.y + PANEL_TITLE_HEIGHT + 5;
    int w = panelBounds.width - 10;

    for (int i = 0; i < 4; i++)
    {
      Rect r(panelBounds.x + 5, y, w, tabHeight - 5);

      if (tabs[i] == state.activeTab)
      {
        Color c(panelBg.r + 20, panelBg.g + 20, panelBg.b + 20);
        drawRoundedRect(r, 3, c, true);
      }

      drawText(tabLabels[i], panelBounds.x + 15, y + 6,
               tabs[i] == state.activeTab ? theme.controlCheck : theme.textColor);

      y += tabHeight;
    }
  }
  else
  {
    int y = panelBounds.y + PANEL_TITLE_HEIGHT + 5;
    int x = panelBounds.x + 10;

    for (int i = 0; i < 4; i++)
    {
      int w = StrLen(tabLabels[i]) * 8 + 20;
      Rect r(x, y, w, tabHeight - 5);

      if (tabs[i] == state.activeTab)
      {
        Color c(panelBg.r + 20, panelBg.g + 20, panelBg.b + 20);
        drawRoundedRect(r, 3, c, true);
      }

      drawText(tabLabels[i], x + 10, y + 6,
               tabs[i] == state.activeTab ? theme.controlCheck : theme.textColor);

      x += w + 5;
    }
  }

  int contentX = panelBounds.x + 15;
  int contentY = panelBounds.y + PANEL_TITLE_HEIGHT +
                 (isVertical ? (tabHeight * 4) : tabHeight) + 10;

  int contentWidth = panelBounds.width - 30;
  int contentHeight = panelBounds.height - (contentY - panelBounds.y) - 10;

  switch (state.activeTab)
  {
  case BottomPanelState::Tab::EntropyAnalysis:
  {
    drawText("Entropy Analysis", contentX, contentY, theme.headerColor);
    contentY += 25;

    Rect graph(contentX, contentY, contentWidth - 15, contentHeight - 30);
    drawRect(graph, Color(20, 20, 25), true);
    drawRect(graph, theme.controlBorder, false);

    int barCount = 50;
    int barWidth = graph.width / barCount;

    for (int i = 0; i < barCount; i++)
    {
      int h = (i * 7 + (i * i) % 31) % (graph.height - 10);
      Rect bar(graph.x + i * barWidth,
               graph.y + graph.height - h - 5,
               barWidth - 2,
               h);
      drawRect(bar, Color(70, 130, 180), true);
    }
    break;
  }

  case BottomPanelState::Tab::PatternSearch:
  {
    drawText("Hex Pattern Search", contentX, contentY, theme.headerColor);
    contentY += 25;

    drawText("Pattern:", contentX, contentY, theme.textColor);
    contentY += 20;

    Rect searchBox(contentX, contentY, 200, 28);
    drawRect(searchBox, theme.controlBackground, true);
    drawRoundedRect(searchBox, 3, theme.controlBorder, false);

    drawText(g_PatternSearch.searchPattern,
             contentX + 8,
             contentY + 6,
             theme.textColor);

    extern bool caretVisible;
    if (g_PatternSearch.hasFocus && caretVisible)
    {
      int caretX = contentX + 8 + (int)StrLen(g_PatternSearch.searchPattern) * 8;
      Rect caret(caretX, contentY + 4, 2, 20);
      drawRect(caret, theme.textColor, true);
    }

    WidgetState btn;
    btn.enabled = true;

    btn.rect = Rect(contentX + 210, contentY, 80, 28);
    drawModernButton(btn, theme, "Find");

    contentY += 40;

    btn.rect = Rect(contentX, contentY, 120, 28);
    drawModernButton(btn, theme, "Find Previous");

    btn.rect = Rect(contentX + 130, contentY, 100, 28);
    drawModernButton(btn, theme, "Find Next");

    break;
  }

  case BottomPanelState::Tab::Checksum:
  {
    drawText("Checksum", contentX, contentY, theme.headerColor);
    contentY += 25;

    int y = contentY;

    WidgetState chk;
    chk.enabled = true;

    chk.rect = Rect(contentX, y, 16, 16);
    drawModernCheckbox(chk, theme, g_Checksum.md5);
    drawText("MD5", contentX + 22, y, theme.textColor);

    chk.rect = Rect(contentX + 100, y, 16, 16);
    drawModernCheckbox(chk, theme, g_Checksum.sha1);
    drawText("SHA-1", contentX + 122, y, theme.textColor);

    chk.rect = Rect(contentX + 200, y, 16, 16);
    drawModernCheckbox(chk, theme, g_Checksum.sha256);
    drawText("SHA-256", contentX + 222, y, theme.textColor);

    chk.rect = Rect(contentX + 330, y, 16, 16);
    drawModernCheckbox(chk, theme, g_Checksum.crc32);
    drawText("CRC32", contentX + 352, y, theme.textColor);

    contentY += 35;

    WidgetState radio;
    radio.enabled = true;

    radio.rect = Rect(contentX, contentY, 16, 16);
    drawModernRadioButton(radio, theme, g_Checksum.entireFile);
    drawText("Entire File", contentX + 22, contentY, theme.textColor);

    radio.rect = Rect(contentX + 150, contentY, 16, 16);
    drawModernRadioButton(radio, theme, !g_Checksum.entireFile);
    drawText("Selection", contentX + 172, contentY, theme.textColor);

    contentY += 35;

    WidgetState btn;
    btn.enabled = true;

    btn.rect = Rect(contentX, contentY, 100, 28);
    drawModernButton(btn, theme, "Compare");

    btn.rect = Rect(contentX + 110, contentY, 150, 28);
    drawModernButton(btn, theme, "Hash Calculator");

    break;
  }

  case BottomPanelState::Tab::Compare:
  {
    drawText("Compare Files", contentX, contentY, theme.headerColor);
    contentY += 25;

    drawText("Compare current file with another file",
             contentX, contentY, theme.textColor);
    contentY += 30;

    WidgetState btn;
    btn.enabled = true;
    btn.rect = Rect(contentX, contentY, 150, 32);
    drawModernButton(btn, theme, "Select File to Compare");

    break;
  }
  }

  if (state.dockPosition == PanelDockPosition::Floating)
  {
    Rect r(panelBounds.x + panelBounds.width - 10,
           panelBounds.y + panelBounds.height - 10,
           10, 10);
    Color c = theme.controlCheck;
    c.a = state.resizing ? 150 : 30;
    drawRect(r, c, true);
  }
  else if (state.dockPosition == PanelDockPosition::Bottom)
  {
    Rect r(panelBounds.x, panelBounds.y, panelBounds.width, 6);
    Color c = theme.controlCheck;
    c.a = state.resizing ? 150 : 30;
    drawRect(r, c, true);
  }
}

bool RenderManager::isLeftPanelResizeHandle(int mouseX, int mouseY, const LeftPanelState &state)
{
  if (!state.visible)
    return false;

  int menuBarHeight = 24;
  return mouseX >= state.width - 3 &&
         mouseX <= state.width + 3 &&
         mouseY >= menuBarHeight;
}

bool RenderManager::isBottomPanelResizeHandle(int mouseX, int mouseY,
                                              const BottomPanelState &state,
                                              int leftPanelWidth)
{
  if (!state.visible)
    return false;

  int panelY = windowHeight - state.height;
  return mouseX >= leftPanelWidth &&
         mouseY >= panelY - 3 &&
         mouseY <= panelY + 3;
}

int RenderManager::getSectionHeaderY(const LeftPanelState &state, int sectionIndex, int menuBarHeight)
{
  int y = menuBarHeight + 40;

  for (int i = 0; i < sectionIndex; i++)
  {
    y += 28;
    if (state.sections[i].expanded)
    {
      y += state.sections[i].items.size() * 24 + 5;
    }
  }

  return y;
}

int RenderManager::getItemY(const LeftPanelState &state, int sectionIndex, int itemIndex, int menuBarHeight)
{
  int y = getSectionHeaderY(state, sectionIndex, menuBarHeight) + 28;
  y += itemIndex * 24;
  return y;
}

void RenderManager::drawContextMenu(
    const ContextMenuState &state,
    const Theme &theme)
{
  if (!state.visible || state.items.empty())
    return;

  int itemHeight = 24;
  int separatorHeight = 8;
  int padding = 8;
  int shortcutOffset = 120;
  int submenuArrowOffset = 10;

  int totalHeight = padding * 2;
  for (size_t i = 0; i < state.items.size(); i++)
  {
    totalHeight += state.items[i].separator ? separatorHeight : itemHeight;
  }

  int menuWidth = state.width > 0 ? state.width : 200;

  Rect menuRect(state.x, state.y, menuWidth, totalHeight);

  Rect shadowRect(state.x + 4, state.y + 4, menuWidth, totalHeight);
  Color shadowColor(0, 0, 0, 100);
  drawRect(shadowRect, shadowColor, true);

  Color menuBg = theme.menuBackground;
  menuBg.a = 255;
  drawRoundedRect(menuRect, 4.0f, menuBg, true);

  Color menuBorder = theme.menuBorder;
  drawRoundedRect(menuRect, 4.0f, menuBorder, false);

  int currentY = state.y + padding;
  for (size_t i = 0; i < state.items.size(); i++)
  {
    const ContextMenuItem &item = state.items[i];

    if (item.separator)
    {
      int sepY = currentY + separatorHeight / 2;
      Color sepColor = theme.separator;
      drawLine(
          state.x + padding,
          sepY,
          state.x + menuWidth - padding,
          sepY,
          sepColor);
      currentY += separatorHeight;
      continue;
    }

    Rect itemRect(
        state.x + 1,
        currentY,
        menuWidth - 2,
        itemHeight);

    if ((int)i == state.hoveredIndex && item.enabled)
    {
      Color hoverBg = theme.menuHover;
      hoverBg.a = 255;
      drawRect(itemRect, hoverBg, true);
    }

    if (item.checked)
    {
      int checkX = state.x + padding;
      int checkY = currentY + itemHeight / 2;
      Color checkColor = item.enabled ? theme.controlCheck : Color(theme.controlCheck.r / 2, theme.controlCheck.g / 2, theme.controlCheck.b / 2);

      drawLine(checkX, checkY, checkX + 2, checkY + 3, checkColor);
      drawLine(checkX + 2, checkY + 3, checkX + 6, checkY - 2, checkColor);
      drawLine(checkX + 1, checkY, checkX + 3, checkY + 3, checkColor);
      drawLine(checkX + 3, checkY + 3, checkX + 7, checkY - 2, checkColor);
    }

    int textX = state.x + padding + (item.checked ? 20 : 10);
    int textY = currentY + (itemHeight / 2) - 6;

    Color textColor = item.enabled ? theme.textColor : Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2);

    drawText(item.text, textX, textY, textColor);

    if (item.shortcut && item.shortcut[0])
    {
      int shortcutX = state.x + shortcutOffset;
      Color shortcutColor = item.enabled ? Color(theme.textColor.r - 40, theme.textColor.g - 40, theme.textColor.b - 40) : Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2);
      drawText(item.shortcut, shortcutX, textY, shortcutColor);
    }

    if (!item.submenu.empty())
    {
      int arrowX = state.x + menuWidth - submenuArrowOffset - 8;
      int arrowY = currentY + itemHeight / 2;
      Color arrowColor = item.enabled ? theme.textColor : Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2);

      drawLine(arrowX, arrowY - 3, arrowX + 4, arrowY, arrowColor);
      drawLine(arrowX, arrowY + 3, arrowX + 4, arrowY, arrowColor);
    }

    currentY += itemHeight;
  }

  if (state.openSubmenuIndex >= 0 &&
      state.openSubmenuIndex < (int)state.items.size() &&
      !state.items[state.openSubmenuIndex].submenu.empty())
  {

    int submenuY = state.y + padding;
    for (int i = 0; i < state.openSubmenuIndex; i++)
    {
      submenuY += state.items[i].separator ? separatorHeight : itemHeight;
    }

    ContextMenuState submenuState;
    submenuState.visible = true;
    submenuState.x = state.x + menuWidth - 2;
    submenuState.y = submenuY;
    submenuState.width = menuWidth;
    submenuState.hoveredIndex = -1;
    submenuState.items = state.items[state.openSubmenuIndex].submenu;
    submenuState.openSubmenuIndex = -1;

    drawContextMenu(submenuState, theme);
  }
}

bool RenderManager::isPointInContextMenu(
    int mouseX, int mouseY,
    const ContextMenuState &state)
{
  if (!state.visible)
    return false;

  int itemHeight = 24;
  int separatorHeight = 8;
  int padding = 8;

  int totalHeight = padding * 2;
  for (size_t i = 0; i < state.items.size(); i++)
  {
    totalHeight += state.items[i].separator ? separatorHeight : itemHeight;
  }

  int menuWidth = state.width > 0 ? state.width : 200;

  return mouseX >= state.x &&
         mouseX <= state.x + menuWidth &&
         mouseY >= state.y &&
         mouseY <= state.y + totalHeight;
}

int RenderManager::getContextMenuHoveredItem(
    int mouseX, int mouseY,
    const ContextMenuState &state)
{
  if (!state.visible)
    return -1;
  if (!isPointInContextMenu(mouseX, mouseY, state))
    return -1;

  int itemHeight = 24;
  int separatorHeight = 8;
  int padding = 8;

  int relativeY = mouseY - state.y - padding;
  int currentY = 0;

  for (size_t i = 0; i < state.items.size(); i++)
  {
    const ContextMenuItem &item = state.items[i];
    int height = item.separator ? separatorHeight : itemHeight;

    if (relativeY >= currentY && relativeY < currentY + height)
    {
      return item.separator ? -1 : (int)i;
    }

    currentY += height;
  }

  return -1;
}

void RenderManager::updateScrollbarMetrics(
    ScrollbarState &state,
    int x, int y, int width, int height,
    float contentSize, float viewportSize,
    bool vertical)
{
  state.visible = contentSize > viewportSize;

  if (!state.visible)
  {
    state.thumbSize = 1.0f;
    return;
  }

  state.thumbSize = viewportSize / contentSize;
  if (state.thumbSize > 1.0f)
    state.thumbSize = 1.0f;
  if (state.thumbSize < 0.05f)
    state.thumbSize = 0.05f;

  if (vertical)
  {
    state.trackX = x;
    state.trackY = y;
    state.trackWidth = width;
    state.trackHeight = height;

    int scrollbarWidth = 8;
    if (state.hovered || state.pressed)
      scrollbarWidth = 12;

    state.trackX = x + width - scrollbarWidth;
    state.trackWidth = scrollbarWidth;

    int availableHeight = height - 4;
    int thumbHeight = (int)(availableHeight * state.thumbSize);
    if (thumbHeight < 20)
      thumbHeight = 20;

    int maxThumbTravel = availableHeight - thumbHeight;

    int thumbY = y + 2 + (int)(maxThumbTravel * state.position);

    state.thumbX = state.trackX + 2;
    state.thumbY = thumbY;
    state.thumbWidth = scrollbarWidth - 3;
    state.thumbHeight = thumbHeight;
  }
  else
  {
    state.trackX = x;
    state.trackY = y;
    state.trackWidth = width;
    state.trackHeight = height;

    int scrollbarHeight = 8;
    if (state.hovered || state.pressed)
      scrollbarHeight = 12;

    state.trackY = y + height - scrollbarHeight;
    state.trackHeight = scrollbarHeight;

    int availableWidth = width - 4;
    int thumbWidth = (int)(availableWidth * state.thumbSize);
    if (thumbWidth < 20)
      thumbWidth = 20;

    int maxThumbTravel = availableWidth - thumbWidth;

    int thumbX = x + 2 + (int)(maxThumbTravel * state.position);

    state.thumbX = thumbX;
    state.thumbY = state.trackY + 2;
    state.thumbWidth = thumbWidth;
    state.thumbHeight = scrollbarHeight - 3;
  }
}

void RenderManager::drawModernScrollbar(
    const ScrollbarState &state,
    const Theme &theme,
    bool vertical)
{
  if (!state.visible)
    return;

  Color trackColor = theme.scrollbarBg;
  trackColor.a = 0;

  Color thumbColor = theme.scrollbarThumb;
  thumbColor.a = 128;

  if (state.hovered || state.pressed)
  {
    trackColor.a = 30;
    thumbColor.a = 180;
  }

  if (state.pressed)
  {
    thumbColor.a = 220;
  }

  if (state.hovered || state.pressed)
  {
    Rect trackRect(state.trackX, state.trackY,
                   state.trackWidth, state.trackHeight);
    drawRoundedRect(trackRect, 6.0f, trackColor, true);
  }

  Rect thumbRect(state.thumbX, state.thumbY,
                 state.thumbWidth, state.thumbHeight);

  Color finalThumbColor = thumbColor;
  if (state.thumbHovered && !state.pressed)
  {
    finalThumbColor.r = (uint8_t)(thumbColor.r * 1.2f > 255 ? 255 : thumbColor.r * 1.2f);
    finalThumbColor.g = (uint8_t)(thumbColor.g * 1.2f > 255 ? 255 : thumbColor.g * 1.2f);
    finalThumbColor.b = (uint8_t)(thumbColor.b * 1.2f > 255 ? 255 : thumbColor.b * 1.2f);
  }

  float radius = (float)state.thumbWidth / 2.0f;
  if (!vertical)
    radius = (float)state.thumbHeight / 2.0f;

  drawRoundedRect(thumbRect, radius, finalThumbColor, true);
}

bool RenderManager::isPointInScrollbarThumb(
    int mouseX, int mouseY,
    const ScrollbarState &state)
{
  if (!state.visible)
    return false;

  return mouseX >= state.thumbX &&
         mouseX < state.thumbX + state.thumbWidth &&
         mouseY >= state.thumbY &&
         mouseY < state.thumbY + state.thumbHeight;
}

bool RenderManager::isPointInScrollbarTrack(
    int mouseX, int mouseY,
    const ScrollbarState &state)
{
  if (!state.visible)
    return false;

  return mouseX >= state.trackX &&
         mouseX < state.trackX + state.trackWidth &&
         mouseY >= state.trackY &&
         mouseY < state.trackY + state.trackHeight;
}

float RenderManager::getScrollbarPositionFromMouse(
    int mouseY,
    const ScrollbarState &state,
    bool vertical)
{
  if (!state.visible)
    return 0.0f;

  if (vertical)
  {
    int maxThumbTravel = state.trackHeight - state.thumbHeight - 4;
    if (maxThumbTravel <= 0)
      return 0.0f;

    int thumbTop = mouseY - state.trackY - 2;

    float position = (float)thumbTop / (float)maxThumbTravel;

    if (position < 0.0f)
      position = 0.0f;
    if (position > 1.0f)
      position = 1.0f;

    return position;
  }
  else
  {
    int maxThumbTravel = state.trackWidth - state.thumbWidth - 4;
    if (maxThumbTravel <= 0)
      return 0.0f;

    int thumbLeft = mouseY - state.trackX - 2;

    float position = (float)thumbLeft / (float)maxThumbTravel;

    if (position < 0.0f)
      position = 0.0f;
    if (position > 1.0f)
      position = 1.0f;

    return position;
  }
}

void RenderManager::drawDataInspectorContent(
    int contentX, int &contentY, int panelWidth,
    const DataInspectorValues &vals,
    const Theme &theme)
{
  if (!vals.hasData)
  {
    drawText("Move cursor to view data", contentX, contentY,
             Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2));
    contentY += 20;
    return;
  }

  char buf[256];
  int labelWidth = 85;
  int valueX = contentX + labelWidth;
  Color labelColor = Color(theme.textColor.r - 40, theme.textColor.g - 40, theme.textColor.b - 40);

  StrCopy(buf, "Offset: ");
  ItoaHex(vals.byteOffset, buf + StrLen(buf), 256 - StrLen(buf));
  drawText(buf, contentX, contentY, Color(100, 150, 255));
  contentY += 20;

  drawLine(contentX, contentY, contentX + panelWidth - 30, contentY, theme.separator);
  contentY += 8;

  drawText("Uint8:", contentX, contentY, labelColor);
  ItoaDec(vals.uint8Val, buf, 256);
  StrCat(buf, " (0x");
  char hexBuf[8];
  ByteToHex(vals.uint8Val, hexBuf);
  hexBuf[2] = ')';
  hexBuf[3] = 0;
  StrCat(buf, hexBuf);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Int8:", contentX, contentY, labelColor);
  ItoaDec(vals.int8Val, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Binary:", contentX, contentY, labelColor);
  drawText(vals.binaryStr, valueX, contentY, Color(150, 200, 150));
  contentY += 16;

  drawText("ASCII:", contentX, contentY, labelColor);
  if (vals.isASCIIPrintable)
  {
    buf[0] = '\'';
    buf[1] = vals.asciiChar;
    buf[2] = '\'';
    buf[3] = 0;
  }
  else
  {
    StrCopy(buf, ".");
  }
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 18;

  drawLine(contentX, contentY, contentX + panelWidth - 30, contentY, theme.separator);
  contentY += 8;

  drawText("Uint16 LE:", contentX, contentY, labelColor);
  ItoaDec(vals.uint16LE, buf, 256);
  StrCat(buf, " (0x");
  for (int i = 0; i < 4; i++)
  {
    int nibble = (vals.uint16LE >> (12 - i * 4)) & 0xF;
    buf[StrLen(buf)] = IntToHexChar(nibble);
    buf[StrLen(buf) + 1] = 0;
  }
  StrCat(buf, ")");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Int16 LE:", contentX, contentY, labelColor);
  ItoaDec(vals.int16LE, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 18;

  drawLine(contentX, contentY, contentX + panelWidth - 30, contentY, theme.separator);
  contentY += 8;

  drawText("Uint32 LE:", contentX, contentY, labelColor);
  ItoaDec(vals.uint32LE, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Int32 LE:", contentX, contentY, labelColor);
  ItoaDec(vals.int32LE, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Float LE:", contentX, contentY, labelColor);
  StrCopy(buf, "<float>");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 18;

  drawLine(contentX, contentY, contentX + panelWidth - 30, contentY, theme.separator);
  contentY += 8;

  drawText("Uint64 LE:", contentX, contentY, labelColor);
  ItoaDec(vals.uint64LE, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Double LE:", contentX, contentY, labelColor);
  StrCopy(buf, "<double>");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;
}

void RenderManager::drawFileInfoContent(
    int contentX, int &contentY, int panelWidth,
    const FileInfoValues &info,
    const Theme &theme)
{
  char buf[256];
  int labelWidth = 70;
  int valueX = contentX + labelWidth;
  Color labelColor = Color(theme.textColor.r - 40, theme.textColor.g - 40, theme.textColor.b - 40);

  drawText("Size:", contentX, contentY, labelColor);
  drawText(info.fileSizeFormatted, valueX, contentY, theme.textColor);
  contentY += 18;

  drawText("Type:", contentX, contentY, labelColor);
  drawText(info.fileType, valueX, contentY, Color(100, 200, 150));
  contentY += 18;

  drawText("Format:", contentX, contentY, labelColor);
  ItoaDec(info.fileSize, buf, 256);
  StrCat(buf, " bytes");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 18;
}

void RenderManager::drawBookmarksContent(
    int contentX, int &contentY, int panelWidth,
    const Theme &theme)
{
  extern BookmarksState g_Bookmarks;

  if (g_Bookmarks.bookmarks.empty())
  {
    drawText("No bookmarks", contentX, contentY,
             Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2));
    contentY += 18;

    drawText("Ctrl+B to add bookmark", contentX, contentY + 5,
             Color(theme.textColor.r / 3, theme.textColor.g / 3, theme.textColor.b / 3));
    contentY += 18;
    return;
  }

  for (size_t i = 0; i < g_Bookmarks.bookmarks.size(); i++)
  {
    const Bookmark &bm = g_Bookmarks.bookmarks[i];

    Rect colorBox(contentX, contentY + 3, 10, 10);
    drawRect(colorBox, bm.color, true);

    char buf[128];
    StrCopy(buf, bm.name);
    StrCat(buf, " (");
    char hexBuf[32];
    ItoaHex(bm.byteOffset, hexBuf, 32);
    StrCat(buf, hexBuf);
    StrCat(buf, ")");

    Color textColor = ((int)i == g_Bookmarks.selectedIndex)
                          ? Color(100, 150, 255)
                          : theme.textColor;

    drawText(buf, contentX + 15, contentY, textColor);
    contentY += 18;
  }
}

void RenderManager::drawByteStatsContent(
    int contentX, int &contentY, int panelWidth,
    const Theme &theme)
{
  extern ByteStatistics g_ByteStats;

  if (!g_ByteStats.computed)
  {
    drawText("Click to compute statistics", contentX, contentY,
             Color(theme.textColor.r / 2, theme.textColor.g / 2, theme.textColor.b / 2));
    contentY += 20;
    return;
  }

  char buf[256];
  int labelWidth = 110;
  int valueX = contentX + labelWidth;
  Color labelColor = Color(theme.textColor.r - 40, theme.textColor.g - 40, theme.textColor.b - 40);

  drawText("Entropy:", contentX, contentY, labelColor);
  StrCopy(buf, "<entropy> bits");

  Color entropyColor = theme.textColor;
  if (g_ByteStats.entropy > 7.5)
  {
    entropyColor = Color(255, 100, 100);
  }
  else if (g_ByteStats.entropy < 3.0)
  {
    entropyColor = Color(100, 255, 100);
  }

  drawText(buf, valueX, contentY, entropyColor);
  contentY += 18;

  drawText("Most Common:", contentX, contentY, labelColor);
  StrCopy(buf, "0x");
  ByteToHex(g_ByteStats.mostCommonByte, buf + 2);
  buf[4] = ' ';
  buf[5] = '(';
  ItoaDec(g_ByteStats.mostCommonCount, buf + 6, 250);
  StrCat(buf, ")");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Least Common:", contentX, contentY, labelColor);
  StrCopy(buf, "0x");
  ByteToHex(g_ByteStats.leastCommonByte, buf + 2);
  buf[4] = ' ';
  buf[5] = '(';
  ItoaDec(g_ByteStats.leastCommonCount, buf + 6, 250);
  StrCat(buf, ")");
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 16;

  drawText("Null Bytes:", contentX, contentY, labelColor);
  ItoaDec(g_ByteStats.nullByteCount, buf, 256);
  drawText(buf, valueX, contentY, theme.textColor);
  contentY += 18;

  contentY += 5;
  drawText("Byte Distribution:", contentX, contentY, labelColor);
  contentY += 18;

  int histWidth = panelWidth - 30;
  int histHeight = 60;
  Rect histRect(contentX, contentY, histWidth, histHeight);

  drawRect(histRect, Color(20, 20, 25), true);
  drawRect(histRect, theme.controlBorder, false);

  int barCount = Clamp(histWidth / 2, 32, 256);
  int bytesPerBar = 256 / barCount;
  int barWidth = histWidth / barCount;

  int maxCount = 0;
  for (int i = 0; i < 256; i++)
  {
    if (g_ByteStats.histogram[i] > maxCount)
    {
      maxCount = g_ByteStats.histogram[i];
    }
  }

  if (maxCount > 0)
  {
    for (int i = 0; i < barCount; i++)
    {
      int sum = 0;
      for (int j = 0; j < bytesPerBar; j++)
      {
        sum += g_ByteStats.histogram[i * bytesPerBar + j];
      }

      int barHeight = (sum * (histHeight - 10)) / maxCount;
      if (barHeight > 0)
      {
        Rect bar(contentX + i * barWidth + 1,
                 contentY + histHeight - barHeight - 5,
                 barWidth - 2,
                 barHeight);

        Color barColor(70, 130, 180);
        if (i * bytesPerBar == 0)
        {
          barColor = Color(255, 100, 100);
        }

        drawRect(bar, barColor, true);
      }
    }
  }

  contentY += histHeight + 5;
}

void RenderManager::renderHexViewer(
    const Vector<char *> &hexLines,
    const char *headerLine,
    int scrollPos,
    int maxScrollPos,
    bool scrollbarHovered,
    bool scrollbarPressed,
    const Rect &scrollbarRect,
    const Rect &thumbRect,
    bool darkMode,
    int editingRow,
    int editingCol,
    const char *editBuffer,
    long long cursorBytePos,
    int cursorNibblePos,
    long long totalBytes,
    int leftPanelWidth,
    int effectiveWindowHeight)
{
  currentTheme = darkMode ? Theme::Dark() : Theme::Light();
  LayoutMetrics layout;

#ifdef _WIN32
  if (memDC)
  {
    SIZE textSize;
    if (GetTextExtentPoint32A(memDC, "0", 1, &textSize))
    {
      layout.charWidth = (float)textSize.cx;
      layout.lineHeight = (float)textSize.cy;
    }
    else
    {
      layout.charWidth = 8.0f;
      layout.lineHeight = 16.0f;
    }
  }
  else
  {
    layout.charWidth = 8.0f;
    layout.lineHeight = 16.0f;
  }
#elif __APPLE__
  layout.charWidth = 9.6f;
  layout.lineHeight = 20.0f;
#else
  if (fontInfo)
  {
    layout.charWidth = (float)fontInfo->max_bounds.width;
    layout.lineHeight = (float)(fontInfo->ascent + fontInfo->descent);
  }
  else
  {
    layout.charWidth = 9.6f;
    layout.lineHeight = 20.0f;
  }
#endif

  layout.margin = 10.0f;
  layout.headerHeight = layout.lineHeight;
  layout.scrollbarWidth = 16.0f;

  _bytesPerLine = 16;
  _startByte = scrollPos * _bytesPerLine;
  _bytePos = cursorBytePos;
  _byteCharacterPos = cursorNibblePos;
  _charWidth = (int)layout.charWidth;
  _charHeight = (int)layout.lineHeight;

  layout.charWidth = (float)_charWidth;
  layout.lineHeight = (float)_charHeight;

  int workingHeight = (effectiveWindowHeight > 0) ? effectiveWindowHeight : windowHeight;

  int menuBarHeight = 24;

  Rect contentArea(leftPanelWidth, menuBarHeight,
                   windowWidth - leftPanelWidth,
                   workingHeight - menuBarHeight);
  drawRect(contentArea, currentTheme.windowBackground, true);

  int disasmColumnWidth = 300;

  if (headerLine && headerLine[0])
  {
    drawText(headerLine,
             leftPanelWidth + (int)layout.margin,
             menuBarHeight + (int)layout.margin,
             currentTheme.headerColor);

    int disasmX = windowWidth - (int)layout.scrollbarWidth - disasmColumnWidth + 10;
    drawText("Disassembly",
             disasmX,
             menuBarHeight + (int)layout.margin,
             currentTheme.disassemblyColor);

    drawLine(leftPanelWidth + (int)layout.margin,
             menuBarHeight + (int)(layout.margin + layout.headerHeight),
             windowWidth - (int)layout.scrollbarWidth,
             menuBarHeight + (int)(layout.margin + layout.headerHeight),
             currentTheme.separator);
  }

  int separatorX = windowWidth - (int)layout.scrollbarWidth - disasmColumnWidth;
  drawLine(separatorX,
           menuBarHeight + (int)(layout.margin + layout.headerHeight),
           separatorX,
           workingHeight - (int)layout.margin,
           currentTheme.separator);

  int contentY = _hexAreaY;
  int contentHeight = workingHeight - contentY - (int)layout.margin;
  if (contentHeight < 0)
    contentHeight = 0;

  size_t maxVisibleLines = (size_t)(contentHeight / layout.lineHeight);
  _visibleLines = (int)maxVisibleLines;

  size_t startLine = (size_t)scrollPos;
  size_t endLine = Clamp(startLine + maxVisibleLines, (size_t)0, hexLines.size());

  extern HexData g_HexData;
  const LineArray &disasmLines = g_HexData.getDisassemblyLines();

  extern SelectionState g_Selection;
  if (g_Selection.active)
  {
    long long selMin, selMax;
    g_Selection.getRange(selMin, selMax);

    long long firstLine = selMin / _bytesPerLine;
    long long lastLine = selMax / _bytesPerLine;

    Color highlightColor = currentTheme.controlCheck;
    highlightColor.a = 80;

    for (long long line = firstLine; line <= lastLine; line++)
    {
      if (line < (long long)startLine)
        continue;
      if (line >= (long long)endLine)
        break;

      int displayLine = (int)(line - startLine);
      int yPos = contentY + displayLine * _charHeight;

      long long lineStart = line * _bytesPerLine;
      long long lineEnd = lineStart + _bytesPerLine - 1;

      long long drawStart = (lineStart < selMin) ? selMin : lineStart;
      long long drawEnd = (lineEnd > selMax) ? selMax : lineEnd;

      int startCol = (int)(drawStart % _bytesPerLine);
      int endCol = (int)(drawEnd % _bytesPerLine);

      int xStart = _hexAreaX + (startCol * 3 * _charWidth);
      int xEnd = _hexAreaX + ((endCol + 1) * 3 * _charWidth) - _charWidth;

      Rect highlightRect(xStart, yPos, xEnd - xStart, _charHeight);
      drawRect(highlightRect, highlightColor, true);

      int asciiAreaX = _hexAreaX + (16 * 3 * _charWidth) + (1 * _charWidth);
      int asciiStart = asciiAreaX + (startCol * _charWidth);
      int asciiEnd = asciiAreaX + ((endCol + 1) * _charWidth);

      Rect asciiHighlight(asciiStart, yPos, asciiEnd - asciiStart, _charHeight);
      drawRect(asciiHighlight, highlightColor, true);
    }
  }

  for (size_t i = startLine; i < endLine; i++)
  {
    int y = contentY + (int)((i - startLine) * layout.lineHeight);
    const char *line = hexLines[i];

    drawText(line,
             leftPanelWidth + (int)layout.margin,
             y,
             currentTheme.textColor);

    if (i < disasmLines.count && disasmLines.lines[i].data != nullptr)
    {
      if (disasmLines.lines[i].length > 0)
      {
        int disasmX = separatorX + 10;
        drawText(disasmLines.lines[i].data, disasmX, y, currentTheme.disassemblyColor);
      }
    }
  }

  if (_bytePos >= _startByte &&
      _bytePos < _startByte + (_bytesPerLine * _visibleLines))
  {
    DrawCaret();
  }

  if (maxScrollPos > 0)
  {
    extern ScrollbarState g_MainScrollbar;

    if (maxScrollPos > 0)
    {
      g_MainScrollbar.position = (float)scrollPos / (float)maxScrollPos;
    }
    else
    {
      g_MainScrollbar.position = 0.0f;
    }

    if (g_MainScrollbar.position < 0.0f)
      g_MainScrollbar.position = 0.0f;
    if (g_MainScrollbar.position > 1.0f)
      g_MainScrollbar.position = 1.0f;

    g_MainScrollbar.hovered = scrollbarHovered;
    g_MainScrollbar.pressed = scrollbarPressed;
    g_MainScrollbar.thumbHovered = scrollbarHovered;

    int totalContentHeight = (int)hexLines.size() * _charHeight;
    int viewportHeight = contentHeight;

    int scrollbarX = windowWidth - 16;
    int scrollbarY = menuBarHeight + (int)(layout.margin + layout.headerHeight);
    int scrollbarWidth = 16;
    int scrollbarHeight = contentHeight;

    updateScrollbarMetrics(
        g_MainScrollbar,
        scrollbarX, scrollbarY,
        scrollbarWidth, scrollbarHeight,
        (float)totalContentHeight, (float)viewportHeight,
        true);

    drawModernScrollbar(g_MainScrollbar, currentTheme, true);
  }
}
