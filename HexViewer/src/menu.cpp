#include "menu.h"
#include "render.h"
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#define DEBUG_OUTPUT(msg) OutputDebugStringA(msg)
#else
#include <cstdio>
#define DEBUG_OUTPUT(msg) fprintf(stderr, "%s", msg)
#endif

MenuBar::MenuBar()
  : x(0), y(0), height(24), openMenuIndex(-1), hoveredMenuIndex(-1), hoveredItemIndex(-1)
#ifdef _WIN32
  , hMenu(nullptr)
#elif __APPLE__
  , nativeMenuRef(nullptr)
#endif
{
}

MenuBar::~MenuBar() {
  destroyNativeMenu();
}

void MenuBar::addMenu(const Menu& menu) {
  menus.push_back(menu);
  calculateMenuBounds();
}

void MenuBar::clearMenus() {
  menus.clear();
  openMenuIndex = -1;
  hoveredMenuIndex = -1;
  hoveredItemIndex = -1;
}

Menu* MenuBar::getMenu(size_t index) {
  if (index < menus.size()) {
    return &menus[index];
  }
  return nullptr;
}

void MenuBar::setPosition(int px, int py) {
  x = px;
  y = py;
  calculateMenuBounds();
}

void MenuBar::render(RenderManager* renderer, int windowWidth) {
  if (!renderer) return;

  Theme theme = renderer->getCurrentTheme();

  Rect menuBarRect(x, y, windowWidth, height);
  renderer->drawRect(menuBarRect, theme.menuBackground, true);

  renderer->drawLine(x, y + height - 1, windowWidth, y + height - 1, theme.separator);

  int currentX = x + 10;
  for (size_t i = 0; i < menus.size(); i++) {
    Menu& menu = menus[i];

    int menuWidth = menu.title.length() * 8 + 20;
    menu.bounds = Rect(currentX, y, menuWidth, height);

    bool isHovered = (int)i == hoveredMenuIndex;
    bool isOpen = (int)i == openMenuIndex;

    if (isHovered || isOpen) {
      renderer->drawRect(menu.bounds, theme.menuHover, true);
    }

    int textX = currentX + 10;
    int textY = y + (height - 16) / 2;
    renderer->drawText(menu.title, textX, textY, theme.textColor);

    currentX += menuWidth;
  }

  if (openMenuIndex >= 0 && openMenuIndex < (int)menus.size()) {
    Menu& menu = menus[openMenuIndex];

    int dropdownX = menu.bounds.x;
    int dropdownY = menu.bounds.y + menu.bounds.height;
    int maxWidth = 200;

    int dropdownHeight = 0;
    for (const auto& item : menu.items) {
      if (item.type == MenuItemType::Separator) {
        dropdownHeight += 8;
      }
      else {
        dropdownHeight += 24;
      }
    }

    Rect shadowRect(dropdownX + 2, dropdownY + 2, maxWidth, dropdownHeight);
    renderer->drawRect(shadowRect, Color(0, 0, 0, 50), true);

    Rect dropdownRect(dropdownX, dropdownY, maxWidth, dropdownHeight);
    renderer->drawRect(dropdownRect, theme.menuBackground, true);
    renderer->drawRect(dropdownRect, theme.menuBorder, false);

    int itemY = dropdownY;
    for (size_t i = 0; i < menu.items.size(); i++) {
      const MenuItem& item = menu.items[i];

      if (item.type == MenuItemType::Separator) {
        int sepY = itemY + 4;
        renderer->drawLine(dropdownX + 5, sepY,
          dropdownX + maxWidth - 5, sepY,
          theme.separator);
        itemY += 8;
      }
      else {
        Rect itemRect(dropdownX, itemY, maxWidth, 24);
        if ((int)i == hoveredItemIndex && item.enabled) {
          renderer->drawRect(itemRect, theme.menuHover, true);
        }

        if (item.checked) {
          renderer->drawText("✓", dropdownX + 5, itemY + 4,
            item.enabled ? theme.textColor : theme.disabledText);
        }

        Color textColor = item.enabled ? theme.textColor : theme.disabledText;
        renderer->drawText(item.label, dropdownX + 25, itemY + 4, textColor);

        if (!item.shortcut.empty()) {
          int shortcutX = dropdownX + maxWidth - item.shortcut.length() * 8 - 10;
          renderer->drawText(item.shortcut, shortcutX, itemY + 4, theme.disabledText);
        }

        if (item.type == MenuItemType::Submenu && !item.submenu.empty()) {
          renderer->drawText(">", dropdownX + maxWidth - 20, itemY + 4, textColor);
        }

        itemY += 24;
      }
    }
  }
}

bool MenuBar::handleMouseMove(int mx, int my) {
  int oldHoveredMenu = hoveredMenuIndex;
  hoveredMenuIndex = getMenuIndexAt(mx, my);

  if (openMenuIndex >= 0) {
    hoveredItemIndex = getMenuItemIndexAt(openMenuIndex, mx, my);

    if (hoveredMenuIndex >= 0 && hoveredMenuIndex != openMenuIndex) {
      openMenu(hoveredMenuIndex);
    }

    return true; // Consume event if menu is open
  }

  return oldHoveredMenu != hoveredMenuIndex;
}

bool MenuBar::handleMouseDown(int mx, int my) {
  int menuIndex = getMenuIndexAt(mx, my);

  if (menuIndex >= 0) {
    if (openMenuIndex == menuIndex) {
      closeMenu();
    }
    else {
      openMenu(menuIndex);
    }
    return true;
  }

  if (openMenuIndex >= 0) {
    int itemIndex = getMenuItemIndexAt(openMenuIndex, mx, my);
    if (itemIndex >= 0) {
      Menu& menu = menus[openMenuIndex];
      if (itemIndex < (int)menu.items.size()) {
        MenuItem& item = menu.items[itemIndex];
        if (item.enabled && item.type == MenuItemType::Normal) {
          executeMenuItem(item);
          closeMenu();
          return true;
        }
      }
    }

    closeMenu();
    return true;
  }

  return false;
}

bool MenuBar::handleMouseUp(int mx, int my) {
  return openMenuIndex >= 0;
}

bool MenuBar::handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt) {
  if (openMenuIndex >= 0) {
    if (keyCode == 27) { // ESC
      closeMenu();
      return true;
    }

    Menu& menu = menus[openMenuIndex];

    if (keyCode == 38) { // UP
      do {
        hoveredItemIndex--;
        if (hoveredItemIndex < 0) hoveredItemIndex = menu.items.size() - 1;
      } while (hoveredItemIndex >= 0 &&
        menu.items[hoveredItemIndex].type == MenuItemType::Separator);
      return true;
    }

    if (keyCode == 40) { // DOWN
      do {
        hoveredItemIndex++;
        if (hoveredItemIndex >= (int)menu.items.size()) hoveredItemIndex = 0;
      } while (hoveredItemIndex < (int)menu.items.size() &&
        menu.items[hoveredItemIndex].type == MenuItemType::Separator);
      return true;
    }

    if (keyCode == 13) { // ENTER
      if (hoveredItemIndex >= 0 && hoveredItemIndex < (int)menu.items.size()) {
        MenuItem& item = menu.items[hoveredItemIndex];
        if (item.enabled && item.type == MenuItemType::Normal) {
          executeMenuItem(item);
          closeMenu();
          return true;
        }
      }
    }
  }

  return false;
}

bool MenuBar::containsPoint(int px, int py) const {
  if (py >= y && py < y + height) {
    return true;
  }

  if (openMenuIndex >= 0 && openMenuIndex < (int)menus.size()) {
    const Menu& menu = menus[openMenuIndex];
    int dropdownY = y + height;

    int dropdownHeight = 0;
    for (const auto& item : menu.items) {
      dropdownHeight += (item.type == MenuItemType::Separator) ? 8 : 24;
    }

    if (px >= menu.bounds.x && px < menu.bounds.x + 200 &&
      py >= dropdownY && py < dropdownY + dropdownHeight) {
      return true;
    }
  }

  return false;
}

Rect MenuBar::getBounds(int windowWidth) const {
  return Rect(x, y, windowWidth, height);
}

void MenuBar::closeAllMenus() {
  closeMenu();
}

void MenuBar::openMenu(int menuIndex) {
  if (menuIndex >= 0 && menuIndex < (int)menus.size()) {
    openMenuIndex = menuIndex;
    hoveredItemIndex = -1;

    std::string debugMsg = "Opening menu: " + std::to_string(menuIndex) + "\n";
    DEBUG_OUTPUT(debugMsg.c_str());
  }
}

void MenuBar::closeMenu() {
  openMenuIndex = -1;
  hoveredItemIndex = -1;
}

int MenuBar::getMenuIndexAt(int mx, int my) const {
  if (my < y || my >= y + height) {
    return -1;
  }

  for (size_t i = 0; i < menus.size(); i++) {
    const Menu& menu = menus[i];
    if (mx >= menu.bounds.x && mx < menu.bounds.x + menu.bounds.width) {
      return (int)i;
    }
  }

  return -1;
}

int MenuBar::getMenuItemIndexAt(int menuIndex, int mx, int my) const {
  if (menuIndex < 0 || menuIndex >= (int)menus.size()) {
    return -1;
  }

  const Menu& menu = menus[menuIndex];
  int dropdownX = menu.bounds.x;
  int dropdownY = y + height;

  if (mx < dropdownX || mx >= dropdownX + 200) {
    return -1;
  }

  int itemY = dropdownY;
  for (size_t i = 0; i < menu.items.size(); i++) {
    const MenuItem& item = menu.items[i];
    int itemHeight = (item.type == MenuItemType::Separator) ? 8 : 24;

    if (my >= itemY && my < itemY + itemHeight) {
      return (int)i;
    }

    itemY += itemHeight;
  }

  return -1;
}

void MenuBar::calculateMenuBounds() {
  int currentX = x + 10;
  for (auto& menu : menus) {
    int menuWidth = menu.title.length() * 8 + 20;
    menu.bounds = Rect(currentX, y, menuWidth, height);
    currentX += menuWidth;
  }
}

void MenuBar::executeMenuItem(MenuItem& item) {
  if (item.callback) {
    item.callback();
  }
}

void MenuBar::createNativeMenu() {
#ifdef _WIN32
#elif __APPLE__
#endif
}

void MenuBar::destroyNativeMenu() {
#ifdef _WIN32
  if (hMenu) {
    DestroyMenu(hMenu);
    hMenu = nullptr;
  }
#endif
}

namespace MenuHelper {
  Menu createFileMenu(
    std::function<void()> onNew,
    std::function<void()> onOpen,
    std::function<void()> onSave,
    std::function<void()> onExit)
  {
    Menu menu("File");

    MenuItem newItem("New", MenuItemType::Normal);
    newItem.shortcut = "Ctrl+N";
    newItem.callback = onNew;
    menu.items.push_back(newItem);

    MenuItem openItem("Open...", MenuItemType::Normal);
    openItem.shortcut = "Ctrl+O";
    openItem.callback = onOpen;
    menu.items.push_back(openItem);

    MenuItem saveItem("Save", MenuItemType::Normal);
    saveItem.shortcut = "Ctrl+S";
    saveItem.callback = onSave;
    menu.items.push_back(saveItem);

    menu.items.push_back(MenuItem("", MenuItemType::Separator));

    MenuItem exitItem("Exit", MenuItemType::Normal);
    exitItem.shortcut = "Alt+F4";
    exitItem.callback = onExit;
    menu.items.push_back(exitItem);

    return menu;
  }

  Menu createSearchMenu(
    std::function<void()> onFindReplace,
    std::function<void()> onGoTo)
  {
    Menu menu("Search");

    MenuItem findReplaceItem("Find and Replace...", MenuItemType::Normal);
    findReplaceItem.shortcut = "Ctrl+F";
    findReplaceItem.callback = onFindReplace;
    menu.items.push_back(findReplaceItem);

    MenuItem goToItem("Go To...", MenuItemType::Normal);
    goToItem.shortcut = "Ctrl+G";
    goToItem.callback = onGoTo;
    menu.items.push_back(goToItem);

    return menu;
  }

  Menu createEditMenu(
    std::function<void()> onCopy,
    std::function<void()> onPaste,
    std::function<void()> onFind)
  {
    Menu menu("Edit");

    MenuItem copyItem("Copy", MenuItemType::Normal);
    copyItem.shortcut = "Ctrl+C";
    copyItem.callback = onCopy;
    menu.items.push_back(copyItem);

    MenuItem pasteItem("Paste", MenuItemType::Normal);
    pasteItem.shortcut = "Ctrl+V";
    pasteItem.callback = onPaste;
    menu.items.push_back(pasteItem);

    menu.items.push_back(MenuItem("", MenuItemType::Separator));

    MenuItem findItem("Find...", MenuItemType::Normal);
    findItem.shortcut = "Ctrl+F";
    findItem.callback = onFind;
    menu.items.push_back(findItem);

    return menu;
  }

  Menu createViewMenu(
    std::function<void()> onToggleDarkMode,
    std::function<void()> onZoomIn,
    std::function<void()> onZoomOut)
  {
    Menu menu("View");

    MenuItem darkModeItem("Dark Mode", MenuItemType::Normal);
    darkModeItem.callback = onToggleDarkMode;
    menu.items.push_back(darkModeItem);

    menu.items.push_back(MenuItem("", MenuItemType::Separator));

    MenuItem zoomInItem("Zoom In", MenuItemType::Normal);
    zoomInItem.shortcut = "Ctrl++";

    MenuItem zoomOutItem("Zoom Out", MenuItemType::Normal);
    zoomOutItem.shortcut = "Ctrl+-";
    zoomOutItem.callback = onZoomOut;
    menu.items.push_back(zoomOutItem);

    return menu;
  }

  Menu createHelpMenu(
    std::function<void()> onAbout,
    std::function<void()> onDocumentation)
  {
    Menu menu("Help");

    MenuItem docItem("Documentation", MenuItemType::Normal);
    docItem.shortcut = "F1";
    docItem.callback = onDocumentation;
    menu.items.push_back(docItem);

    menu.items.push_back(MenuItem("", MenuItemType::Separator));

    MenuItem aboutItem("About", MenuItemType::Normal);
    aboutItem.callback = onAbout;
    menu.items.push_back(aboutItem);

    return menu;
  }
}