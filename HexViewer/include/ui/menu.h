#ifndef MENU_H
#define MENU_H

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "render.h"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#else
#include <X11/Xlib.h>
#endif

struct Color;
struct Rect;
class RenderManager;

enum class MenuItemType {
  Normal,
  Separator,
  Submenu
};

struct MenuItem {
  std::string label;
  std::string shortcut;
  MenuItemType type;
  bool enabled;
  bool checked;
  std::function<void()> callback;
  std::vector<MenuItem> submenu;
  
  MenuItem(const std::string& lbl = "", MenuItemType t = MenuItemType::Normal)
    : label(lbl), type(t), enabled(true), checked(false) {}
};

struct Menu {
  std::string title;
  std::vector<MenuItem> items;
  bool visible;
  Rect bounds;        // now Rect is known
  int selectedIndex;

  Menu(const std::string& t = "")
    : title(t), visible(false), selectedIndex(-1) {
  }
};

class MenuBar {
public:
  MenuBar();
  ~MenuBar();
  
  void addMenu(const Menu& menu);
  void clearMenus();
  Menu* getMenu(size_t index);
  size_t getMenuCount() const { return menus.size(); }
  
  void setPosition(int x, int y);
  void setHeight(int h) { height = h; }
  int getHeight() const { return height; }
  
  void render(RenderManager* renderer, int windowWidth);
  
  bool handleMouseMove(int x, int y);
  bool handleMouseDown(int x, int y);
  bool handleMouseUp(int x, int y);
  bool handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt);
  
  bool isMenuOpen() const { return openMenuIndex >= 0; }
  bool containsPoint(int x, int y) const;
  Rect getBounds(int windowWidth) const;
  
  void closeAllMenus();
  
private:
  std::vector<Menu> menus;
  int x, y;
  int height;
  int openMenuIndex;
  int hoveredMenuIndex;
  int hoveredItemIndex;
  
  void openMenu(int menuIndex);
  void closeMenu();
  int getMenuIndexAt(int mx, int my) const;
  int getMenuItemIndexAt(int menuIndex, int mx, int my) const;
  void calculateMenuBounds();
  void executeMenuItem(MenuItem& item);
  
  void createNativeMenu();
  void destroyNativeMenu();
  
#ifdef _WIN32
  HMENU hMenu;
#elif __APPLE__
  void* nativeMenuRef;
#else
#endif
};

namespace MenuHelper {
  Menu createFileMenu(
    std::function<void()> onNew,
    std::function<void()> onOpen,
    std::function<void()> onSave,
    std::function<void()> onExit
  );

  Menu createEditMenu(
    std::function<void()> onCopy,
    std::function<void()> onPaste,
    std::function<void()> onFind
  );

  Menu createViewMenu(
    std::function<void()> onToggleDarkMode,
    std::function<void()> onZoomIn,
    std::function<void()> onZoomOut
  );

  Menu createHelpMenu(
    std::function<void()> onAbout,
    std::function<void()> onDocumentation
  );
  Menu createSearchMenu(
    std::function<void()> onFindReplace,
    std::function<void()> onGoTo
  );
}

#endif // MENU_H
