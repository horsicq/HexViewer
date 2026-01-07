#define _DISABLE_STRING_ANNOTATION
#define _HAS_ALIGNED_NEW 0
#include "menu.h"
#include "render.h"

#ifdef _WIN32
#include <windows.h>
#define DEBUG_OUTPUT(msg) OutputDebugStringA(msg)
#else
#define DEBUG_OUTPUT(msg)
#endif

MenuItem::MenuItem()
    : label(nullptr), shortcut(nullptr), type(MenuItemType::Normal),
      enabled(true), checked(false), callback(nullptr),
      submenu(nullptr), submenuCount(0)
{
}

MenuItem::MenuItem(const char *lbl, MenuItemType t)
    : label(nullptr), shortcut(nullptr), type(t),
      enabled(true), checked(false), callback(nullptr),
      submenu(nullptr), submenuCount(0)
{
  if (lbl)
    label = StrDup(lbl);
}

MenuItem::~MenuItem()
{
  if (label)
    MemFree(label, StrLen(label) + 1);
  if (shortcut)
    MemFree(shortcut, StrLen(shortcut) + 1);
  if (submenu)
  {
    for (int i = 0; i < submenuCount; i++)
    {
      submenu[i].~MenuItem();
    }
    MemFree(submenu, submenuCount * sizeof(MenuItem));
  }
}

MenuItem::MenuItem(const MenuItem &other)
    : label(nullptr), shortcut(nullptr), type(other.type),
      enabled(other.enabled), checked(other.checked),
      callback(other.callback), submenu(nullptr), submenuCount(0)
{

  if (other.label)
    label = StrDup(other.label);
  if (other.shortcut)
    shortcut = StrDup(other.shortcut);

  if (other.submenu && other.submenuCount > 0)
  {
    submenuCount = other.submenuCount;
    submenu = (MenuItem *)PlatformAlloc(submenuCount * sizeof(MenuItem));

    for (int i = 0; i < submenuCount; i++)
    {
      submenu[i].label = other.submenu[i].label ? StrDup(other.submenu[i].label) : nullptr;
      submenu[i].shortcut = other.submenu[i].shortcut ? StrDup(other.submenu[i].shortcut) : nullptr;
      submenu[i].type = other.submenu[i].type;
      submenu[i].enabled = other.submenu[i].enabled;
      submenu[i].checked = other.submenu[i].checked;
      submenu[i].callback = other.submenu[i].callback;
      submenu[i].submenu = nullptr;
      submenu[i].submenuCount = 0;
    }
  }
}

MenuItem &MenuItem::operator=(const MenuItem &other)
{
  if (this != &other)
  {
    if (label)
      PlatformFree(label, StrLen(label) + 1);
    if (shortcut)
      PlatformFree(shortcut, StrLen(shortcut) + 1);
    if (submenu)
    {
      for (int i = 0; i < submenuCount; i++)
      {
        if (submenu[i].label)
          PlatformFree(submenu[i].label, StrLen(submenu[i].label) + 1);
        if (submenu[i].shortcut)
          PlatformFree(submenu[i].shortcut, StrLen(submenu[i].shortcut) + 1);
      }
      PlatformFree(submenu, submenuCount * sizeof(MenuItem));
    }

    label = other.label ? StrDup(other.label) : nullptr;
    shortcut = other.shortcut ? StrDup(other.shortcut) : nullptr;
    type = other.type;
    enabled = other.enabled;
    checked = other.checked;
    callback = other.callback;
    submenuCount = other.submenuCount;

    if (other.submenu && other.submenuCount > 0)
    {
      submenu = (MenuItem *)PlatformAlloc(submenuCount * sizeof(MenuItem));

      for (int i = 0; i < submenuCount; i++)
      {
        submenu[i].label = other.submenu[i].label ? StrDup(other.submenu[i].label) : nullptr;
        submenu[i].shortcut = other.submenu[i].shortcut ? StrDup(other.submenu[i].shortcut) : nullptr;
        submenu[i].type = other.submenu[i].type;
        submenu[i].enabled = other.submenu[i].enabled;
        submenu[i].checked = other.submenu[i].checked;
        submenu[i].callback = other.submenu[i].callback;
        submenu[i].submenu = nullptr;
        submenu[i].submenuCount = 0;
      }
    }
    else
    {
      submenu = nullptr;
    }
  }
  return *this;
}

Menu::Menu()
    : title(nullptr), items(nullptr), itemCount(0), itemCapacity(0),
      visible(false), selectedIndex(-1)
{
}

Menu::~Menu()
{
  if (title)
    MemFree(title, StrLen(title) + 1);
  if (items)
  {
    for (int i = 0; i < itemCount; i++)
    {
      items[i].~MenuItem();
    }
    MemFree(items, itemCapacity * sizeof(MenuItem));
  }
}

Menu::Menu(const Menu &other)
    : title(nullptr), items(nullptr), itemCount(0), itemCapacity(0),
      visible(other.visible), bounds(other.bounds), selectedIndex(other.selectedIndex)
{

  if (other.title)
    title = StrDup(other.title);

  if (other.items && other.itemCount > 0)
  {
    itemCount = other.itemCount;
    itemCapacity = other.itemCapacity;
    items = (MenuItem *)PlatformAlloc(itemCapacity * sizeof(MenuItem));

    for (int i = 0; i < itemCount; i++)
    {
      items[i].label = other.items[i].label ? StrDup(other.items[i].label) : nullptr;
      items[i].shortcut = other.items[i].shortcut ? StrDup(other.items[i].shortcut) : nullptr;
      items[i].type = other.items[i].type;
      items[i].enabled = other.items[i].enabled;
      items[i].checked = other.items[i].checked;
      items[i].callback = other.items[i].callback;
      items[i].submenu = nullptr;
      items[i].submenuCount = 0;
    }
  }
}

Menu &Menu::operator=(const Menu &other)
{
  if (this != &other)
  {
    if (title)
      PlatformFree(title, StrLen(title) + 1);
    if (items)
    {
      for (int i = 0; i < itemCount; i++)
      {
        if (items[i].label)
          PlatformFree(items[i].label, StrLen(items[i].label) + 1);
        if (items[i].shortcut)
          PlatformFree(items[i].shortcut, StrLen(items[i].shortcut) + 1);
      }
      PlatformFree(items, itemCapacity * sizeof(MenuItem));
    }

    title = other.title ? StrDup(other.title) : nullptr;
    visible = other.visible;
    bounds = other.bounds;
    selectedIndex = other.selectedIndex;
    itemCount = other.itemCount;
    itemCapacity = other.itemCapacity;

    if (other.items && other.itemCount > 0)
    {
      items = (MenuItem *)PlatformAlloc(itemCapacity * sizeof(MenuItem));

      for (int i = 0; i < itemCount; i++)
      {
        items[i].label = other.items[i].label ? StrDup(other.items[i].label) : nullptr;
        items[i].shortcut = other.items[i].shortcut ? StrDup(other.items[i].shortcut) : nullptr;
        items[i].type = other.items[i].type;
        items[i].enabled = other.items[i].enabled;
        items[i].checked = other.items[i].checked;
        items[i].callback = other.items[i].callback;
        items[i].submenu = nullptr;
        items[i].submenuCount = 0;
      }
    }
    else
    {
      items = nullptr;
    }
  }
  return *this;
}

void Menu::addItem(const MenuItem &item)
{
  if (itemCount >= itemCapacity)
  {
    int newCapacity = itemCapacity == 0 ? 4 : itemCapacity * 2;
    MenuItem *newItems = (MenuItem *)PlatformAlloc(newCapacity * sizeof(MenuItem));

    for (int i = 0; i < newCapacity; i++)
    {
      newItems[i].label = nullptr;
      newItems[i].shortcut = nullptr;
      newItems[i].type = MenuItemType::Normal;
      newItems[i].enabled = true;
      newItems[i].checked = false;
      newItems[i].callback = nullptr;
      newItems[i].submenu = nullptr;
      newItems[i].submenuCount = 0;
    }

    for (int i = 0; i < itemCount; i++)
    {
      newItems[i].label = items[i].label;
      newItems[i].shortcut = items[i].shortcut;
      newItems[i].type = items[i].type;
      newItems[i].enabled = items[i].enabled;
      newItems[i].checked = items[i].checked;
      newItems[i].callback = items[i].callback;
      newItems[i].submenu = items[i].submenu;
      newItems[i].submenuCount = items[i].submenuCount;

      items[i].label = nullptr;
      items[i].shortcut = nullptr;
      items[i].submenu = nullptr;
    }

    if (items)
      PlatformFree(items, itemCapacity * sizeof(MenuItem));
    items = newItems;
    itemCapacity = newCapacity;
  }

  items[itemCount].label = item.label ? StrDup(item.label) : nullptr;
  items[itemCount].shortcut = item.shortcut ? StrDup(item.shortcut) : nullptr;
  items[itemCount].type = item.type;
  items[itemCount].enabled = item.enabled;
  items[itemCount].checked = item.checked;
  items[itemCount].callback = item.callback;
  items[itemCount].submenu = nullptr;
  items[itemCount].submenuCount = 0;

  itemCount++;
}

void Menu::clear()
{
  for (int i = 0; i < itemCount; i++)
  {
    items[i].~MenuItem();
  }
  itemCount = 0;
}

MenuBar::MenuBar()
    : menus(nullptr), menuCount(0), menuCapacity(0),
      x(0), y(0), height(32), openMenuIndex(-1),
      hoveredMenuIndex(-1), hoveredItemIndex(-1)
#ifdef _WIN32
      ,
      hMenu(nullptr)
#elif __APPLE__
      ,
      nativeMenuRef(nullptr)
#endif
{
}

MenuBar::~MenuBar()
{
  destroyNativeMenu();
  if (menus)
  {
    for (int i = 0; i < menuCount; i++)
    {
      menus[i].~Menu();
    }
    MemFree(menus, menuCapacity * sizeof(Menu));
  }
}

void MenuBar::addMenu(const Menu &menu)
{
  if (menuCount >= menuCapacity)
  {
    int newCapacity = menuCapacity == 0 ? 4 : menuCapacity * 2;
    Menu *newMenus = (Menu *)PlatformAlloc(newCapacity * sizeof(Menu));

    for (int i = 0; i < newCapacity; i++)
    {
      newMenus[i].title = nullptr;
      newMenus[i].items = nullptr;
      newMenus[i].itemCount = 0;
      newMenus[i].itemCapacity = 0;
      newMenus[i].visible = false;
      newMenus[i].bounds = Rect(0, 0, 0, 0);
      newMenus[i].selectedIndex = -1;
    }

    for (int i = 0; i < menuCount; i++)
    {
      newMenus[i].title = menus[i].title;
      newMenus[i].items = menus[i].items;
      newMenus[i].itemCount = menus[i].itemCount;
      newMenus[i].itemCapacity = menus[i].itemCapacity;
      newMenus[i].visible = menus[i].visible;
      newMenus[i].bounds = menus[i].bounds;
      newMenus[i].selectedIndex = menus[i].selectedIndex;

      menus[i].title = nullptr;
      menus[i].items = nullptr;
    }

    if (menus)
      PlatformFree(menus, menuCapacity * sizeof(Menu));
    menus = newMenus;
    menuCapacity = newCapacity;
  }

  menus[menuCount].title = menu.title ? StrDup(menu.title) : nullptr;
  menus[menuCount].visible = menu.visible;
  menus[menuCount].bounds = menu.bounds;
  menus[menuCount].selectedIndex = menu.selectedIndex;
  menus[menuCount].itemCount = menu.itemCount;
  menus[menuCount].itemCapacity = menu.itemCapacity;

  if (menu.items && menu.itemCount > 0)
  {
    menus[menuCount].itemCapacity = menu.itemCapacity;
    menus[menuCount].items = (MenuItem *)PlatformAlloc(menu.itemCapacity * sizeof(MenuItem));

    for (int i = 0; i < menu.itemCapacity; i++)
    {
      menus[menuCount].items[i].label = nullptr;
      menus[menuCount].items[i].shortcut = nullptr;
      menus[menuCount].items[i].type = MenuItemType::Normal;
      menus[menuCount].items[i].enabled = true;
      menus[menuCount].items[i].checked = false;
      menus[menuCount].items[i].callback = nullptr;
      menus[menuCount].items[i].submenu = nullptr;
      menus[menuCount].items[i].submenuCount = 0;
    }

    for (int i = 0; i < menu.itemCount; i++)
    {
      menus[menuCount].items[i].label = menu.items[i].label ? StrDup(menu.items[i].label) : nullptr;
      menus[menuCount].items[i].shortcut = menu.items[i].shortcut ? StrDup(menu.items[i].shortcut) : nullptr;
      menus[menuCount].items[i].type = menu.items[i].type;
      menus[menuCount].items[i].enabled = menu.items[i].enabled;
      menus[menuCount].items[i].checked = menu.items[i].checked;
      menus[menuCount].items[i].callback = menu.items[i].callback;
      menus[menuCount].items[i].submenu = nullptr;
      menus[menuCount].items[i].submenuCount = 0;
    }
  }
  else
  {
    menus[menuCount].items = nullptr;
  }

  menuCount++;
  calculateMenuBounds();
}

void MenuBar::clearMenus()
{
  for (int i = 0; i < menuCount; i++)
  {
    menus[i].~Menu();
  }
  menuCount = 0;
  openMenuIndex = -1;
  hoveredMenuIndex = -1;
  hoveredItemIndex = -1;
}

Menu *MenuBar::getMenu(size_t index)
{
  if (index < (size_t)menuCount)
  {
    return &menus[index];
  }
  return nullptr;
}

void MenuBar::setPosition(int px, int py)
{
  x = px;
  y = py;
  calculateMenuBounds();
}

void MenuBar::render(RenderManager *renderer, int windowWidth)
{
  if (!renderer)
    return;

  Theme theme = renderer->getCurrentTheme();

  int charWidth = renderer->getCharWidth();
  int charHeight = renderer->getCharHeight();

  Rect menuBarRect(x, y, windowWidth, height);
  renderer->drawRect(menuBarRect, theme.menuBackground, true);
  renderer->drawLine(x, y + height - 1, windowWidth, y + height - 1, theme.separator);

  int currentX = x + 15;
  for (int i = 0; i < menuCount; i++)
  {
    Menu &menu = menus[i];

    int titleLen = menu.title ? StrLen(menu.title) : 0;
    int menuWidth = titleLen * charWidth + 30;
    menu.bounds = Rect(currentX, y, menuWidth, height);

    bool isHovered = i == hoveredMenuIndex;
    bool isOpen = i == openMenuIndex;

    if (isHovered || isOpen)
    {
      renderer->drawRect(menu.bounds, theme.menuHover, true);
    }

    int textX = currentX + 15;
    int textY = y + (height - charHeight) / 2;
    if (menu.title)
    {
      renderer->drawText(menu.title, textX, textY, theme.textColor);
    }

    currentX += menuWidth;
  }

  if (openMenuIndex >= 0 && openMenuIndex < menuCount)
  {
    Menu &menu = menus[openMenuIndex];

    int dropdownX = menu.bounds.x;
    int dropdownY = menu.bounds.y + menu.bounds.height;
    int maxWidth = 250;

    int dropdownHeight = 8;
    for (int i = 0; i < menu.itemCount; i++)
    {
      dropdownHeight += (menu.items[i].type == MenuItemType::Separator) ? 8 : 32;
    }
    dropdownHeight += 8;

    Rect shadowRect(dropdownX + 3, dropdownY + 3, maxWidth, dropdownHeight);
    renderer->drawRect(shadowRect, Color(0, 0, 0, 80), true);

    Rect dropdownRect(dropdownX, dropdownY, maxWidth, dropdownHeight);
    renderer->drawRect(dropdownRect, theme.menuBackground, true);
    renderer->drawRect(dropdownRect, theme.menuBorder, false);

    int itemY = dropdownY + 8;
    for (int i = 0; i < menu.itemCount; i++)
    {
      const MenuItem &item = menu.items[i];

      if (item.type == MenuItemType::Separator)
      {
        int sepY = itemY + 4;
        renderer->drawLine(dropdownX + 10, sepY, dropdownX + maxWidth - 10, sepY, theme.separator);
        itemY += 8;
      }
      else
      {
        Rect itemRect(dropdownX, itemY, maxWidth, 32);
        if (i == hoveredItemIndex && item.enabled)
        {
          renderer->drawRoundedRect(itemRect, 3.0f, theme.menuHover, true);
        }

        if (item.checked)
        {
          renderer->drawText("✓", dropdownX + 10, itemY + (32 - charHeight) / 2,
                             item.enabled ? theme.textColor : theme.disabledText);
        }

        Color textColor = item.enabled ? theme.textColor : theme.disabledText;
        if (item.label)
        {
          int labelX = dropdownX + (item.checked ? 35 : 15);
          int labelY = itemY + (32 - charHeight) / 2;
          renderer->drawText(item.label, labelX, labelY, textColor);
        }

        if (item.shortcut)
        {
          int shortcutLen = StrLen(item.shortcut);
          int shortcutX = dropdownX + maxWidth - (shortcutLen * charWidth) - 15;
          int shortcutY = itemY + (32 - charHeight) / 2;
          renderer->drawText(item.shortcut, shortcutX, shortcutY, theme.disabledText);
        }

        if (item.type == MenuItemType::Submenu && item.submenuCount > 0)
        {
          int arrowX = dropdownX + maxWidth - 20;
          int arrowY = itemY + (32 - charHeight) / 2;
          renderer->drawText(">", arrowX, arrowY, textColor);
        }

        itemY += 32;
      }
    }
  }
}

bool MenuBar::handleMouseMove(int mx, int my)
{
  int oldHoveredMenu = hoveredMenuIndex;
  hoveredMenuIndex = getMenuIndexAt(mx, my);

  if (openMenuIndex >= 0)
  {
    hoveredItemIndex = getMenuItemIndexAt(openMenuIndex, mx, my);

    if (hoveredMenuIndex >= 0 && hoveredMenuIndex != openMenuIndex)
    {
      openMenu(hoveredMenuIndex);
    }

    return true;
  }

  return oldHoveredMenu != hoveredMenuIndex;
}

bool MenuBar::handleMouseDown(int mx, int my)
{
  int menuIndex = getMenuIndexAt(mx, my);

  if (menuIndex >= 0)
  {
    if (openMenuIndex == menuIndex)
    {
      closeMenu();
    }
    else
    {
      openMenu(menuIndex);
    }
    return true;
  }

  if (openMenuIndex >= 0)
  {
    Menu &menu = menus[openMenuIndex];
    int dropdownX = menu.bounds.x;
    int dropdownY = y + height;
    int maxWidth = 250;

    int dropdownHeight = 8;
    for (int i = 0; i < menu.itemCount; i++)
    {
      dropdownHeight += (menu.items[i].type == MenuItemType::Separator) ? 8 : 32;
    }
    dropdownHeight += 8;

    if (mx >= dropdownX && mx < dropdownX + maxWidth &&
        my >= dropdownY && my < dropdownY + dropdownHeight)
    {

      int itemIndex = getMenuItemIndexAt(openMenuIndex, mx, my);

      if (itemIndex >= 0)
      {
        MenuItem &item = menu.items[itemIndex];

        if (item.enabled && item.type == MenuItemType::Normal)
        {
          executeMenuItem(item);
          closeMenu();
        }
      }

      return true;
    }

    closeMenu();
    return true;
  }

  return false;
}

bool MenuBar::handleMouseUp(int mx, int my)
{
  return openMenuIndex >= 0;
}

bool MenuBar::handleKeyPress(int keyCode, bool ctrl, bool shift, bool alt)
{
  if (openMenuIndex >= 0)
  {
    if (keyCode == 27)
    {
      closeMenu();
      return true;
    }

    Menu &menu = menus[openMenuIndex];

    if (keyCode == 38)
    {
      do
      {
        hoveredItemIndex--;
        if (hoveredItemIndex < 0)
          hoveredItemIndex = menu.itemCount - 1;
      } while (hoveredItemIndex >= 0 && menu.items[hoveredItemIndex].type == MenuItemType::Separator);
      return true;
    }

    if (keyCode == 40)
    {
      do
      {
        hoveredItemIndex++;
        if (hoveredItemIndex >= menu.itemCount)
          hoveredItemIndex = 0;
      } while (hoveredItemIndex < menu.itemCount &&
               menu.items[hoveredItemIndex].type == MenuItemType::Separator);
      return true;
    }

    if (keyCode == 13)
    {
      if (hoveredItemIndex >= 0 && hoveredItemIndex < menu.itemCount)
      {
        MenuItem &item = menu.items[hoveredItemIndex];
        if (item.enabled && item.type == MenuItemType::Normal)
        {
          executeMenuItem(item);
          closeMenu();
          return true;
        }
      }
    }
  }

  return false;
}

bool MenuBar::containsPoint(int px, int py) const
{
  if (py >= y && py < y + height)
  {
    return true;
  }

  if (openMenuIndex >= 0 && openMenuIndex < menuCount)
  {
    const Menu &menu = menus[openMenuIndex];
    int dropdownY = y + height;

    int dropdownHeight = 8;
    for (int i = 0; i < menu.itemCount; i++)
    {
      dropdownHeight += (menu.items[i].type == MenuItemType::Separator) ? 8 : 32;
    }
    dropdownHeight += 8;

    if (px >= menu.bounds.x && px < menu.bounds.x + 250 &&
        py >= dropdownY && py < dropdownY + dropdownHeight)
    {
      return true;
    }
  }

  return false;
}

Rect MenuBar::getBounds(int windowWidth) const
{
  return Rect(x, y, windowWidth, height);
}

void MenuBar::closeAllMenus()
{
  closeMenu();
}

void MenuBar::openMenu(int menuIndex)
{
  if (menuIndex >= 0 && menuIndex < menuCount)
  {
    openMenuIndex = menuIndex;
    hoveredItemIndex = -1;

    char debugMsg[64];
    const char *prefix = "Opening menu: ";
    StrCopy(debugMsg, prefix);

    int num = menuIndex;
    char numStr[16];
    int idx = 0;
    if (num == 0)
    {
      numStr[idx++] = '0';
    }
    else
    {
      int temp = num;
      int len = 0;
      while (temp > 0)
      {
        temp /= 10;
        len++;
      }
      for (int i = len - 1; i >= 0; i--)
      {
        numStr[i] = '0' + (num % 10);
        num /= 10;
      }
      idx = len;
    }
    numStr[idx++] = '\n';
    numStr[idx] = '\0';

    int prefixLen = 0;
    while (debugMsg[prefixLen])
      prefixLen++;
    StrCopy(debugMsg + prefixLen, numStr);

    DEBUG_OUTPUT(debugMsg);
  }
}

void MenuBar::closeMenu()
{
  openMenuIndex = -1;
  hoveredItemIndex = -1;
}

int MenuBar::getMenuIndexAt(int mx, int my) const
{
  if (my < y || my >= y + height)
  {
    return -1;
  }

  for (int i = 0; i < menuCount; i++)
  {
    const Menu &menu = menus[i];
    if (mx >= menu.bounds.x && mx < menu.bounds.x + menu.bounds.width)
    {
      return i;
    }
  }

  return -1;
}

int MenuBar::getMenuItemIndexAt(int menuIndex, int mx, int my) const
{
  if (menuIndex < 0 || menuIndex >= menuCount)
  {
    return -1;
  }

  const Menu &menu = menus[menuIndex];
  int dropdownX = menu.bounds.x;
  int dropdownY = y + height;
  int maxWidth = 250;

  int itemY = dropdownY + 8;

  for (int i = 0; i < menu.itemCount; i++)
  {
    const MenuItem &item = menu.items[i];
    int itemHeight = (item.type == MenuItemType::Separator) ? 8 : 32;

    if (mx >= dropdownX && mx < dropdownX + maxWidth &&
        my >= itemY && my < itemY + itemHeight)
    {

      if (item.type != MenuItemType::Separator)
        return i;

      return -1;
    }

    itemY += itemHeight;
  }

  return -1;
}

void MenuBar::calculateMenuBounds()
{
  int currentX = x + 15;
  for (int i = 0; i < menuCount; i++)
  {
    Menu &menu = menus[i];
    int titleLen = menu.title ? StrLen(menu.title) : 0;
    int menuWidth = titleLen * 9 + 30;
    menu.bounds = Rect(currentX, y, menuWidth, height);
    currentX += menuWidth;
  }
}

void MenuBar::executeMenuItem(MenuItem &item)
{
  if (item.callback)
  {
    item.callback();
  }
}

void MenuBar::createNativeMenu()
{
#ifdef _WIN32
#elif __APPLE__
#endif
}

void MenuBar::destroyNativeMenu()
{
#ifdef _WIN32
  if (hMenu)
  {
    DestroyMenu((HMENU)hMenu);
    hMenu = nullptr;
  }
#endif
}

namespace MenuHelper
{
  Menu createFileMenu(MenuCallback onNew, MenuCallback onOpen,
                      MenuCallback onSave, MenuCallback onExit)
  {
    Menu menu;

    menu.title = StrDup("File");
    MenuItem newItem;
    newItem.label = StrDup("New");
    newItem.shortcut = StrDup("Ctrl+N");
    newItem.type = MenuItemType::Normal;
    newItem.callback = onNew;
    menu.addItem(newItem);

    MenuItem openItem;
    openItem.label = StrDup("Open...");
    openItem.shortcut = StrDup("Ctrl+O");
    openItem.type = MenuItemType::Normal;
    openItem.callback = onOpen;
    menu.addItem(openItem);

    MenuItem saveItem;
    saveItem.label = StrDup("Save");
    saveItem.shortcut = StrDup("Ctrl+S");
    saveItem.type = MenuItemType::Normal;
    saveItem.callback = onSave;
    menu.addItem(saveItem);

    MenuItem sepItem;
    sepItem.type = MenuItemType::Separator;
    menu.addItem(sepItem);

    MenuItem exitItem;
    exitItem.label = StrDup("Exit");
    exitItem.shortcut = StrDup("Alt+F4");
    exitItem.type = MenuItemType::Normal;
    exitItem.callback = onExit;
    menu.addItem(exitItem);

    return menu;
  }

  Menu createSearchMenu(MenuCallback onFindReplace, MenuCallback onGoTo)
  {
    Menu menu;
    menu.title = StrDup("Search");

    MenuItem findReplaceItem;
    findReplaceItem.label = StrDup("Find and Replace...");
    findReplaceItem.shortcut = StrDup("Ctrl+F");
    findReplaceItem.type = MenuItemType::Normal;
    findReplaceItem.callback = onFindReplace;
    menu.addItem(findReplaceItem);

    MenuItem goToItem;
    goToItem.label = StrDup("Go To...");
    goToItem.shortcut = StrDup("Ctrl+G");
    goToItem.type = MenuItemType::Normal;
    goToItem.callback = onGoTo;
    menu.addItem(goToItem);

    return menu;
  }

 Menu createToolsMenu(MenuCallback onOptions, MenuCallback onPlugins)
{
    Menu toolsMenu;
    toolsMenu.title = StrDup("Tools");

    MenuItem optionsItem;
    optionsItem.label = StrDup("Options");
    optionsItem.type = MenuItemType::Normal;
    optionsItem.callback = onOptions;
    toolsMenu.addItem(optionsItem);

    MenuItem pluginsItem;
    pluginsItem.label = StrDup("Plugins...");
    pluginsItem.type = MenuItemType::Normal;
    pluginsItem.callback = onPlugins;
    toolsMenu.addItem(pluginsItem);

    return toolsMenu;
}


  Menu createHelpMenu(MenuCallback onAbout, MenuCallback onDocumentation)
  {
    Menu menu;
    menu.title = StrDup("Help");

    MenuItem docItem;
    docItem.label = StrDup("Documentation");
    docItem.shortcut = StrDup("F1");
    docItem.type = MenuItemType::Normal;
    docItem.callback = onDocumentation;
    menu.addItem(docItem);

    MenuItem sepItem;
    sepItem.type = MenuItemType::Separator;
    menu.addItem(sepItem);

    MenuItem aboutItem;
    aboutItem.label = StrDup("About");
    aboutItem.type = MenuItemType::Normal;
    aboutItem.callback = onAbout;
    menu.addItem(aboutItem);

    return menu;
  }
}
