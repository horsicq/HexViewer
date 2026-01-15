#pragma once
#include "render.h"
#include "hexdata.h"
#include <searchdialog.h>
#include "selectblockdialog.h"
#include <options.h>
#include "global.h"
#ifdef _WIN32
#include <windows.h>

enum class UserRole
{
  CurrentUser,
  AllUsers
};

class ContextMenuRegistry
{
public:
  static bool Register(const wchar_t *exePath, UserRole role = UserRole::CurrentUser);
  static bool Unregister(UserRole role = UserRole::CurrentUser);
  static bool IsRegistered(UserRole role = UserRole::CurrentUser);

private:
  static HKEY GetRootKey(UserRole role);
  static wchar_t *GetRegistryPath(UserRole role);
  static bool SetRegistryValue(HKEY hKey, const wchar_t *valueName, const wchar_t *data);
  static bool DeleteRegistryKey(HKEY hRootKey, const wchar_t *subKey);
};
#endif

enum ContextMenuAction
{
  ID_COPY = 100,
  ID_COPY_AS = 104,
  ID_COPY_HEX = 101,
  ID_COPY_TEXT = 102,
  ID_COPY_CARRAY = 103,
  ID_PASTE = 105,
  ID_SELECT_ALL = 106,
  ID_GOTO_OFFSET = 107,
  ID_FILL = 113,
  ID_FILL_ZEROS = 110,
  ID_FILL_FF = 111,
  ID_FILL_PATTERN = 112,
  ID_ADD_BOOKMARK = 114,
  ID_SELECT_BLOCK = 115
};

long long ParseNumber(const char* text, int numberFormat);

class AppContextMenu
{
public:
  AppContextMenu();

  void show(int x, int y);

  void hide();

  bool isVisible() const;

  const ContextMenuState &getState() const;

  void handleMouseMove(int x, int y, RenderManager *renderer);

  int handleClick(int x, int y, RenderManager *renderer);

  void executeAction(int actionId);


private:
  ContextMenuState state;
};
