#pragma once

#ifdef _WIN32
#include <windows.h>
#include <string>

enum class UserRole {
  CurrentUser,  // HKEY_CURRENT_USER (no admin needed)
  AllUsers      // HKEY_CLASSES_ROOT (requires admin)
};

class ContextMenuRegistry {
public:
  static bool Register(const wchar_t* exePath, UserRole role = UserRole::CurrentUser);

  static bool Unregister(UserRole role = UserRole::CurrentUser);

  static bool IsRegistered(UserRole role = UserRole::CurrentUser);

private:
  static HKEY GetRootKey(UserRole role);
  static std::wstring GetRegistryPath(UserRole role);
  static bool SetRegistryValue(HKEY hKey, const wchar_t* valueName, const wchar_t* data);
  static bool DeleteRegistryKey(HKEY hRootKey, const wchar_t* subKey);
};

#endif // _WIN32
