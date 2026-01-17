#pragma once

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include "global.h"

struct PatternSearchState;
struct ChecksumState;
struct CompareState;
struct DataInspectorValues;
struct FileInfoValues;

struct Rect
{
  int x, y, width, height;

  Rect(int x = 0, int y = 0, int w = 0, int h = 0)
      : x(x), y(y), width(w), height(h)
  {
  }

  bool contains(int px, int py) const
  {
    return px >= x && px < x + width && py >= y && py < y + height;
  }
};

struct Color
{
  uint8_t r, g, b, a;

  Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255)
      : r(r), g(g), b(b), a(a)
  {
  }

  static Color White() { return Color(255, 255, 255); }
  static Color Black() { return Color(0, 0, 0); }
  static Color Gray(uint8_t level) { return Color(level, level, level); }
  static Color Green() { return Color(100, 255, 100); }
  static Color DarkGreen() { return Color(0, 128, 0); }
  static Color Blue() { return Color(100, 100, 200, 128); }
  static Color Yellow() { return Color(255, 255, 0); }
};

struct Theme
{
  Color windowBackground;
  Color textColor;
  Color headerColor;
  Color separator;
  Color scrollbarBg;
  Color scrollbarThumb;
  Color disassemblyColor;

  Color buttonNormal;
  Color buttonHover;
  Color buttonPressed;
  Color buttonText;
  Color buttonDisabled;
  Color buttonColor;
  Color controlBorder;
  Color controlBackground;
  Color controlCheck;

  Color dropdownBackground;
  Color menuBackground;
  Color menuHover;
  Color menuBorder;
  Color disabledText;

  static Theme Dark()
  {
    Theme t;
    t.windowBackground = Color(32, 32, 32);
    t.textColor = Color(255, 255, 255);
    t.headerColor = Color(118, 185, 237);
    t.separator = Color(255, 255, 255, 20);

    t.scrollbarBg = Color(45, 45, 45, 100);
    t.scrollbarThumb = Color(90, 90, 90, 180);
    t.disassemblyColor = Color(144, 238, 144);

    t.buttonNormal = Color(255, 255, 255, 15);
    t.buttonHover = Color(255, 255, 255, 25);
    t.buttonPressed = Color(255, 255, 255, 10);
    t.buttonText = Color(255, 255, 255);
    t.buttonDisabled = Color(255, 255, 255, 8);

    t.controlBorder = Color(255, 255, 255, 30);
    t.controlBackground = Color(50, 50, 50, 200);
    t.controlCheck = Color(96, 150, 227);

    t.dropdownBackground = Color(44, 44, 44, 255);
    t.menuBackground = Color(44, 44, 44, 245);

    t.menuHover = Color(60, 60, 60, 255);
    t.menuBorder = Color(255, 255, 255, 15);
    t.disabledText = Color(255, 255, 255, 80);

    return t;
  }

  static Theme Light()
  {
    Theme t;
    t.windowBackground = Color(243, 243, 243);
    t.textColor = Color(0, 0, 0);
    t.headerColor = Color(0, 95, 184);
    t.separator = Color(0, 0, 0, 15);

    t.scrollbarBg = Color(240, 240, 240, 150);
    t.scrollbarThumb = Color(150, 150, 150, 180);
    t.disassemblyColor = Color(16, 124, 16);

    t.buttonNormal = Color(0, 0, 0, 8);
    t.buttonHover = Color(0, 0, 0, 12);
    t.buttonPressed = Color(0, 0, 0, 6);
    t.buttonText = Color(0, 0, 0);
    t.buttonDisabled = Color(0, 0, 0, 5);

    t.controlBorder = Color(0, 0, 0, 20);
    t.controlBackground = Color(255, 255, 255, 230);
    t.controlCheck = Color(0, 95, 184);

    t.dropdownBackground = Color(249, 249, 249, 255);

    t.menuBackground = Color(249, 249, 249, 250);
    t.menuHover = Color(0, 0, 0, 10);
    t.menuBorder = Color(0, 0, 0, 15);
    t.disabledText = Color(0, 0, 0, 90);

    return t;
  }
};

struct Point
{
  int X;
  int Y;
  Point(int x = 0, int y = 0) : X(x), Y(y) {}
};

struct PointF
{
  float X;
  float Y;
  PointF(float x = 0.0f, float y = 0.0f) : X(x), Y(y) {}
};

struct BytePositionInfo
{
  long long Index;
  int CharacterPosition;
  BytePositionInfo(long long idx = 0, int charPos = 0)
      : Index(idx), CharacterPosition(charPos)
  {
  }
};

struct LayoutMetrics
{
  float margin;
  float headerHeight;
  float lineHeight;
  float charWidth;
  float scrollbarWidth;

  LayoutMetrics()
      : margin(20.0f),
        headerHeight(20.0f),
        lineHeight(20.0f),
        charWidth(9.6f),
        scrollbarWidth(20.0f)
  {
  }
};

struct CaretInfo
{
  int x;
  int y;
  int width;
  int height;

  CaretInfo(int x = 0, int y = 0, int w = 2, int h = 0)
      : x(x), y(y), width(w), height(h) {}
};

enum class PanelDockPosition
{
  Left,
  Right,
  Top,
  Bottom,
  Floating
};

struct PanelSection
{
  char *title;
  bool expanded;
  Vector<char *> items;

  PanelSection() : title(nullptr), expanded(true) {}

  ~PanelSection()
  {
    if (title)
      PlatformFree(title, StrLen(title) + 1);
    for (size_t i = 0; i < items.size(); i++)
    {
      if (items[i])
        PlatformFree(items[i], StrLen(items[i]) + 1);
    }
  }
};

struct LeftPanelState
{
  bool visible;
  int width;
  int height;
  bool resizing;
  bool dragging;
  int dragOffsetX;
  int dragOffsetY;
  PanelDockPosition dockPosition;
  int floatingX;
  int floatingY;
  int floatingWidth;
  int floatingHeight;
  Vector<PanelSection> sections;
  int hoveredSection;
  int hoveredItem;

  LeftPanelState()
      : visible(false), width(250), height(200), resizing(false), dragging(false),
        dragOffsetX(0), dragOffsetY(0),
        dockPosition(PanelDockPosition::Left),
        floatingX(100), floatingY(100), floatingWidth(250), floatingHeight(400),
        hoveredSection(-1), hoveredItem(-1)
  {
  }
};

struct BottomPanelState
{
  enum class Tab
  {
    EntropyAnalysis,
    PatternSearch,
    Checksum,
    Compare
  };

  bool visible;
  int height;
  int width;
  bool resizing;
  bool dragging;
  int dragOffsetX;
  int dragOffsetY;
  PanelDockPosition dockPosition;
  int floatingX;
  int floatingY;
  int floatingWidth;
  int floatingHeight;
  Tab activeTab;
  char *searchPattern;
  Vector<long long> searchResults;
  int selectedResult;

  BottomPanelState()
      : visible(false), height(200), width(400), resizing(false), dragging(false),
        dragOffsetX(0), dragOffsetY(0),
        dockPosition(PanelDockPosition::Bottom),
        floatingX(100), floatingY(100), floatingWidth(600), floatingHeight(250),
        activeTab(Tab::EntropyAnalysis),
        searchPattern(nullptr), selectedResult(-1)
  {
  }

  ~BottomPanelState()
  {
    if (searchPattern)
      PlatformFree(searchPattern, StrLen(searchPattern) + 1);
  }
};

struct SelectionState
{
  bool active;
  bool dragging;
  long long startByte;
  long long endByte;

  SelectionState() : active(false), dragging(false), startByte(-1), endByte(-1) {}

  void getRange(long long &min, long long &max) const
  {
    if (startByte <= endByte)
    {
      min = startByte;
      max = endByte;
    }
    else
    {
      min = endByte;
      max = startByte;
    }
  }

  bool isSelected(long long bytePos) const
  {
    if (!active)
      return false;
    long long min, max;
    getRange(min, max);
    return bytePos >= min && bytePos <= max;
  }

  long long getLength() const
  {
    if (!active)
      return 0;
    long long min, max;
    getRange(min, max);
    return max - min + 1;
  }

  void clear()
  {
    active = false;
    dragging = false;
    startByte = -1;
    endByte = -1;
  }
};

Rect GetLeftPanelBounds(const LeftPanelState &state, int windowWidth, int windowHeight, int menuBarHeight);
Rect GetBottomPanelBounds(const BottomPanelState &state, int windowWidth, int windowHeight,
                          int menuBarHeight, const LeftPanelState &leftPanel);
bool IsInPanelTitleBar(int mouseX, int mouseY, const Rect &panelBounds);
PanelDockPosition GetDockPositionFromMouse(int mouseX, int mouseY, int windowWidth, int windowHeight, int menuBarHeight);

struct ContextMenuItem
{
  char *text;
  char *shortcut;
  bool enabled;
  bool checked;
  bool separator;
  int id;
  Vector<ContextMenuItem> submenu;

  ContextMenuItem()
      : text(nullptr), shortcut(nullptr),
        enabled(true), checked(false), separator(false), id(0)
  {
  }

  ContextMenuItem(const ContextMenuItem &other)
      : text(nullptr), shortcut(nullptr),
        enabled(other.enabled), checked(other.checked),
        separator(other.separator), id(other.id),
        submenu(other.submenu)
  {
    if (other.text)
    {
      text = AllocString(other.text);
    }
    if (other.shortcut)
    {
      shortcut = AllocString(other.shortcut);
    }
  }

  ContextMenuItem &operator=(const ContextMenuItem &other)
  {
    if (this != &other)
    {
      if (text)
      {
        PlatformFree(text, StrLen(text) + 1);
        text = nullptr;
      }
      if (shortcut)
      {
        PlatformFree(shortcut, StrLen(shortcut) + 1);
        shortcut = nullptr;
      }

      if (other.text)
      {
        text = AllocString(other.text);
      }
      if (other.shortcut)
      {
        shortcut = AllocString(other.shortcut);
      }

      enabled = other.enabled;
      checked = other.checked;
      separator = other.separator;
      id = other.id;
      submenu = other.submenu;
    }
    return *this;
  }

  ~ContextMenuItem()
  {
    if (text)
    {
      PlatformFree(text, StrLen(text) + 1);
      text = nullptr;
    }
    if (shortcut)
    {
      PlatformFree(shortcut, StrLen(shortcut) + 1);
      shortcut = nullptr;
    }
  }
};

struct ContextMenuState
{
  bool visible;
  int x, y;
  int width;
  int hoveredIndex;
  Vector<ContextMenuItem> items;
  int openSubmenuIndex;

  ContextMenuState()
      : visible(false), x(0), y(0), width(0),
        hoveredIndex(-1), openSubmenuIndex(-1)
  {
  }
};

struct ChecksumResults
{
  char *md5;
  char *sha1;
  char *sha256;
  char *crc32;
  bool calculating;

  ChecksumResults()
      : md5(nullptr), sha1(nullptr), sha256(nullptr),
        crc32(nullptr), calculating(false) {}

  ~ChecksumResults()
  {
    if (md5)
      PlatformFree(md5, StrLen(md5) + 1);
    if (sha1)
      PlatformFree(sha1, StrLen(sha1) + 1);
    if (sha256)
      PlatformFree(sha256, StrLen(sha256) + 1);
    if (crc32)
      PlatformFree(crc32, StrLen(crc32) + 1);
  }
};

struct WidgetState
{
  Rect rect;
  bool hovered;
  bool pressed;
  bool enabled;
  bool checked;

  WidgetState() : hovered(false), pressed(false), enabled(true) {}
  WidgetState(const Rect &r) : rect(r), hovered(false), pressed(false), enabled(true) {}
  WidgetState(const Rect &r, bool h, bool p, bool e) : rect(r), hovered(h), pressed(p), enabled(e) {}
};

struct ScrollbarState
{
  float position;
  float thumbSize;
  bool hovered;
  bool thumbHovered;
  int dragOffsetY;
  bool pressed;
  bool visible;
  float animationProgress;
  int trackX, trackY;
  int trackWidth, trackHeight;
  int thumbX, thumbY;
  int thumbWidth, thumbHeight;

  ScrollbarState()
      : position(0.0f), thumbSize(0.3f), hovered(false),
        thumbHovered(false), dragOffsetY(0), pressed(false), visible(true),
        animationProgress(0.0f), trackX(0), trackY(0),
        trackWidth(0), trackHeight(0), thumbX(0), thumbY(0),
        thumbWidth(0), thumbHeight(0)
  {
  }
};

#ifdef _WIN32
typedef HDC NativeDrawContext;
#elif __APPLE__
typedef CGContextRef NativeDrawContext;
#else
typedef GC NativeDrawContext;
#endif

#ifdef _WIN32
typedef HWND NativeWindow;
#elif __APPLE__
typedef void *NativeWindow;
#else
typedef Window NativeWindow;
#endif

class RenderManager
{
public:
  int getHexAreaX() const { return _hexAreaX; }
  int getHexAreaY() const { return _hexAreaY; }
  RenderManager();
  ~RenderManager();

  bool initialize(NativeWindow window);
  void cleanup();
  void resize(int width, int height);
  int measureTextWidth(const char* text);

  void beginFrame();
  void endFrame(NativeDrawContext ctx);
  void clear(const Color &color);
  void drawRect(const Rect &rect, const Color &color, bool filled = true);
  void drawLine(int x1, int y1, int x2, int y2, const Color &color);
  void drawText(const char *text, int x, int y, const Color &color);
  void drawRoundedRect(const Rect &rect, float radius, const Color &color, bool filled);
  void drawModernButton(const WidgetState &state, const Theme &theme, const char *label);
  void drawModernCheckbox(const WidgetState &state, const Theme &theme, bool checked);
  void drawModernRadioButton(const WidgetState &state, const Theme &theme, bool selected);
  Point GetGridBytePoint(long long byteIndex);
  PointF GetBytePointF(long long byteIndex);
  PointF GetBytePointF(Point gridPoint);
  BytePositionInfo GetHexBytePositionInfo(Point screenPoint);
  void drawProgressBar(const Rect &rect, float progress, const Theme &theme);
  void UpdateCaret();
  void DrawCaret();
  long long ScreenToByteIndex(int mouseX, int mouseY);
  bool IsPointInHexArea(int mouseX, int mouseY, int leftPanelWidth,
                        int menuBarHeight, int windowWidth, int windowHeight);

  void drawModernScrollbar(
      const ScrollbarState &state,
      const Theme &theme,
      bool vertical = true);

  void updateScrollbarMetrics(
      ScrollbarState &state,
      int x, int y, int width, int height,
      float contentSize, float viewportSize,
      bool vertical = true);

  bool isPointInScrollbarThumb(
      int mouseX, int mouseY,
      const ScrollbarState &state);

  bool isPointInScrollbarTrack(
      int mouseX, int mouseY,
      const ScrollbarState &state);

  float getScrollbarPositionFromMouse(
      int mouseY,
      const ScrollbarState &state,
      bool vertical = true);

  void drawByteStatsContent(
      int contentX, int &contentY, int panelWidth,
      const Theme &theme);

  void drawBookmarksContent(
      int contentX, int &contentY, int panelWidth,
      const Theme &theme);

  void drawDataInspectorContent(
      int contentX, int &contentY, int panelWidth,
      const DataInspectorValues &vals,
      const Theme &theme);

  void drawFileInfoContent(
      int contentX, int &contentY, int panelWidth,
      const FileInfoValues &info,
      const Theme &theme);

  CaretInfo GetCaretPosition();

  void UpdateHexMetrics(int leftPanelWidth, int menuBarHeight);

  int GetHexAreaX() const { return _hexAreaX; }
  int GetHexAreaY() const { return _hexAreaY; }
  int GetCharWidth() const { return _charWidth; }
  int GetCharHeight() const { return _charHeight; }
  NativeDrawContext getDrawContext() const
  {
#ifdef _WIN32
    return hdc;
#elif __APPLE__
    return context;
#else
    return gc;
#endif
  }
  int getCharWidth() const { return _charWidth; }
  int getCharHeight() const { return _charHeight; }

  void drawLeftPanel(
      const LeftPanelState &state,
      const Theme &theme,
      int windowHeight,
      const Rect &panelBounds);

  void drawBottomPanel(
      const BottomPanelState &state,
      const Theme &theme,
      const ChecksumResults &checksums,
      int windowWidth,
      int windowHeight,
      const Rect &panelBounds);

  bool isLeftPanelResizeHandle(int mouseX, int mouseY, const LeftPanelState &state);
  bool isBottomPanelResizeHandle(int mouseX, int mouseY, const BottomPanelState &state, int leftPanelWidth);

  int getContextMenuHoveredItem(
      int mouseX, int mouseY,
      const ContextMenuState &state);

  void drawContextMenu(
      const ContextMenuState &state,
      const Theme &theme);

  bool isPointInContextMenu(
      int mouseX, int mouseY,
      const ContextMenuState &state);

  int getSectionHeaderY(const LeftPanelState &state, int sectionIndex, int menuBarHeight);
  int getItemY(const LeftPanelState &state, int sectionIndex, int itemIndex, int menuBarHeight);

#ifdef _WIN32
  void drawBitmap(void *hBitmap, int width, int height, int x, int y);
#elif __APPLE__
  void drawImage(void *nsImage, int width, int height, int x, int y);
#else
  void drawX11Pixmap(Pixmap pixmap, int width, int height, int x, int y);
#endif

  void drawDropdown(
      const WidgetState &state,
      const Theme &theme,
      const char *selectedText,
      bool isOpen,
      const Vector<char *> &items,
      int selectedIndex,
      int hoveredIndex,
      int scrollOffset);

  void renderHexViewer(
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
      int effectiveWindowHeight = 0);

  Theme getCurrentTheme() const { return currentTheme; }

  int getWindowWidth() const { return windowWidth; }
  int getWindowHeight() const { return windowHeight; }
  void createFont();
  void destroyFont();

private:
  NativeWindow window;
  int windowWidth;
  int windowHeight;
  Theme currentTheme;
  long long _bytePos;
  int _byteCharacterPos;
  long long _startByte;
  long long _selectionLength;
  int _bytesPerLine;
  int _visibleLines;

  int _hexAreaX;
  int _hexAreaY;
  int _charWidth;
  int _charHeight;

#ifdef _WIN32
  HDC hdc;
  HDC memDC;
  HBITMAP memBitmap;
  HBITMAP oldBitmap;
  HFONT font;
  void *pixels;
  BITMAPINFO bitmapInfo;
#elif __APPLE__
  CGContextRef context;
  void *backBuffer;
#else
  Display *display;
  GC gc;
  Pixmap backBuffer;
  XFontStruct *fontInfo;
#endif

  void setColor(const Color &color);
};
