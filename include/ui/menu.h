#pragma once
#include "render.h"
#include "global.h"

typedef void (*MenuCallback)();

enum class MenuItemType {
    Normal,
    Separator,
    Submenu,
    Checkable
};

struct MenuItem {
    char* label;
    char* shortcut;
    MenuItemType type;
    bool enabled;
    bool checked;
    MenuCallback callback;
    MenuItem* submenu;
    int submenuCount;
    
    MenuItem();
    MenuItem(const char* lbl, MenuItemType t = MenuItemType::Normal);
    ~MenuItem();
    MenuItem(const MenuItem& other);
    MenuItem& operator=(const MenuItem& other);
};

struct Menu {
    char* title;
    MenuItem* items;
    int itemCount;
    int itemCapacity;
    bool visible;
    Rect bounds;
    int selectedIndex;
    
    Menu();
    ~Menu();
    Menu(const Menu& other);
    Menu& operator=(const Menu& other);
    
    void addItem(const MenuItem& item);
    void clear();
};

class MenuBar {
private:
    Menu* menus;
    int menuCount;
    int menuCapacity;
    int x, y, height;
    int openMenuIndex;
    int hoveredMenuIndex;
    int hoveredItemIndex;
    
#ifdef _WIN32
    void* hMenu;
#elif __APPLE__
    void* nativeMenuRef;
#endif

    void calculateMenuBounds();
    void executeMenuItem(MenuItem& item);
    void createNativeMenu();
    void destroyNativeMenu();
    int getMenuIndexAt(int mx, int my) const;
    int getMenuItemIndexAt(int menuIndex, int mx, int my) const;

public:
    MenuBar();
    ~MenuBar();
    
    void addMenu(const Menu& menu);
    void clearMenus();
    Menu* getMenu(size_t index);
    
    void setPosition(int px, int py);
    void render(RenderManager* renderer, int windowWidth);
    
    bool handleMouseMove(int mx, int my);
    bool handleMouseDown(int mx, int my);
    bool handleMouseUp(int mx, int my);
    bool handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt);
    
    bool containsPoint(int px, int py) const;
    Rect getBounds(int windowWidth) const;
    
    void closeAllMenus();
    void openMenu(int menuIndex);
    void closeMenu();
    
    int getHeight() const { return height; }
    bool isMenuOpen() const { return openMenuIndex >= 0; }
};

namespace MenuHelper {
    Menu createFileMenu(MenuCallback onNew, MenuCallback onOpen, MenuCallback onSave, MenuCallback onExit);
    Menu createSearchMenu(MenuCallback onFindReplace, MenuCallback onGoTo);
    Menu createHelpMenu(MenuCallback onAbout, MenuCallback onDocumentation);
    Menu createToolsMenu(MenuCallback onOptions, MenuCallback onPlugins);
}

