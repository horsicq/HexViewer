#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <climits>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

struct Rect {
  int x, y, width, height;

  Rect(int x = 0, int y = 0, int w = 0, int h = 0)
    : x(x), y(y), width(w), height(h) {
  }

  bool contains(int px, int py) const {
    return px >= x && px < x + width && py >= y && py < y + height;
  }
};

struct Color {
  uint8_t r, g, b, a;

  Color(uint8_t r = 255, uint8_t g = 255, uint8_t b = 255, uint8_t a = 255)
    : r(r), g(g), b(b), a(a) {
  }

  static Color White() { return Color(255, 255, 255); }
  static Color Black() { return Color(0, 0, 0); }
  static Color Gray(uint8_t level) { return Color(level, level, level); }
  static Color Green() { return Color(100, 255, 100); }
  static Color DarkGreen() { return Color(0, 128, 0); }
  static Color Blue() { return Color(100, 100, 200, 128); }
  static Color Yellow() { return Color(255, 255, 0); }
};

struct Theme {
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

  Color controlBorder;
  Color controlBackground;
  Color controlCheck;

  Color menuBackground;
  Color menuHover;
  Color menuBorder;
  Color disabledText;

  static Theme Dark() {
    Theme t;
    t.windowBackground = Color(30, 30, 30);
    t.textColor = Color(220, 220, 220);
    t.headerColor = Color(100, 149, 237);
    t.separator = Color(70, 70, 70);
    t.scrollbarBg = Color(45, 45, 45);
    t.scrollbarThumb = Color(90, 90, 90);
    t.disassemblyColor = Color(144, 238, 144);

    t.buttonNormal = Color(50, 50, 50);
    t.buttonHover = Color(70, 70, 70);
    t.buttonPressed = Color(35, 35, 35);
    t.buttonText = Color(220, 220, 220);
    t.buttonDisabled = Color(40, 40, 40);

    t.controlBorder = Color(100, 100, 100);
    t.controlBackground = Color(45, 45, 45);
    t.controlCheck = Color(100, 149, 237);

    t.menuBackground = Color(45, 45, 48);
    t.menuHover = Color(62, 62, 66);
    t.menuBorder = Color(63, 63, 70);
    t.disabledText = Color(128, 128, 128);

    return t;
  }

  static Theme Light() {
    Theme t;
    t.windowBackground = Color(255, 255, 255);
    t.textColor = Color(0, 0, 0);
    t.headerColor = Color(0, 102, 204);
    t.separator = Color(200, 200, 200);
    t.scrollbarBg = Color(240, 240, 240);
    t.scrollbarThumb = Color(180, 180, 180);
    t.disassemblyColor = Color(0, 128, 0);

    t.buttonNormal = Color(240, 240, 240);
    t.buttonHover = Color(225, 225, 225);
    t.buttonPressed = Color(200, 200, 200);
    t.buttonText = Color(0, 0, 0);
    t.buttonDisabled = Color(245, 245, 245);

    t.controlBorder = Color(180, 180, 180);
    t.controlBackground = Color(255, 255, 255);
    t.controlCheck = Color(0, 102, 204);

    t.menuBackground = Color(240, 240, 240);
    t.menuHover = Color(225, 225, 225);
    t.menuBorder = Color(200, 200, 200);
    t.disabledText = Color(160, 160, 160);

    return t;
  }
};

struct Point {
  int X;
  int Y;
  Point(int x = 0, int y = 0) : X(x), Y(y) {}
};

struct PointF {
  float X;
  float Y;
  PointF(float x = 0.0f, float y = 0.0f) : X(x), Y(y) {}
};

struct BytePositionInfo {
  long long Index;
  int CharacterPosition;
  BytePositionInfo(long long idx = 0, int charPos = 0)
    : Index(idx), CharacterPosition(charPos) {
  }
};

struct LayoutMetrics {
  float margin = 20.0f;       
  float headerHeight = 20.0f;
  float lineHeight = 20.0f;
  float charWidth = 9.6f;     
  float scrollbarWidth = 20.0f;

  LayoutMetrics()
    : margin(20.0f),
    headerHeight(20.0f),
    lineHeight(20.0f),
    charWidth(9.6f),
    scrollbarWidth(20.0f) {
  }
};


struct WidgetState {
  Rect rect;
  bool hovered;
  bool pressed;
  bool enabled;

  WidgetState() : hovered(false), pressed(false), enabled(true) {}
  WidgetState(const Rect& r) : rect(r), hovered(false), pressed(false), enabled(true) {}
  WidgetState(const Rect& r, bool h, bool p, bool e) : rect(r), hovered(h), pressed(p), enabled(e) {}
};


#ifdef _WIN32
typedef HWND NativeWindow;
#elif __APPLE__
typedef void* NativeWindow;  
#else
typedef Window NativeWindow;
#endif

class RenderManager {
public:
  RenderManager();
  ~RenderManager();

  

  bool initialize(NativeWindow window);
  void cleanup();
  void resize(int width, int height);

  void beginFrame();
  void endFrame();

  void clear(const Color& color);
  void drawRect(const Rect& rect, const Color& color, bool filled = true);
  void drawLine(int x1, int y1, int x2, int y2, const Color& color);
  void drawText(const std::string& text, int x, int y, const Color& color);
  void drawRoundedRect(const Rect& rect, float radius, const Color& color, bool filled);
  void drawModernButton(const WidgetState& state, const Theme& theme, const std::string& label);
  void drawModernCheckbox(const WidgetState& state, const Theme& theme, bool checked);
  void drawModernRadioButton(const WidgetState& state, const Theme& theme, bool selected);
  Point GetGridBytePoint(long long byteIndex);
  PointF GetBytePointF(long long byteIndex);
  PointF GetBytePointF(Point gridPoint);
  BytePositionInfo GetHexBytePositionInfo(Point screenPoint);
  void drawProgressBar(const Rect& rect, float progress, const Theme& theme);
  void UpdateCaret();
  void DrawCaret();
  long long ScreenToByteIndex(int mouseX, int mouseY);

#ifdef _WIN32
  void drawBitmap(void* hBitmap, int width, int height, int x, int y);
#elif __APPLE__
  void drawImage(void* nsImage, int width, int height, int x, int y);
#else
  void drawX11Pixmap(Pixmap pixmap, int width, int height, int x, int y);
#endif


  void drawDropdown(
    const WidgetState& state,
    const Theme& theme,
    const std::string& selectedText,
    bool isOpen,
    const std::vector<std::string>& items,
    int selectedIndex,
    int hoveredIndex,
    int scrollOffset);

  void renderHexViewer(
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
    long long totalBytes);          


  Theme getCurrentTheme() const { return currentTheme; }

  int getWindowWidth() const { return windowWidth; }
  int getWindowHeight() const { return windowHeight; }

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
  void* pixels;
  BITMAPINFO bitmapInfo;
#elif __APPLE__
  CGContextRef context;
  void* backBuffer;
#else
  Display* display;
  GC gc;
  Pixmap backBuffer;
  XFontStruct* fontInfo;
#endif

  void createFont();
  void destroyFont();
  void setColor(const Color& color);
};
