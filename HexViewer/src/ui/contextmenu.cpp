#ifdef _WIN32
#include <contextmenu.h>
#include <shlobj.h> 

HKEY ContextMenuRegistry::GetRootKey(UserRole role) {
  switch (role) {
  case UserRole::CurrentUser:
    return HKEY_CURRENT_USER;
  default:
    return HKEY_CURRENT_USER;
  }
}

std::wstring ContextMenuRegistry::GetRegistryPath(UserRole role) {
  switch (role) {
  case UserRole::CurrentUser:
    return L"Software\\Classes\\*\\shell\\HexViewer";
  default:
    return L"Software\\Classes\\*\\shell\\HexViewer";
  }
}

bool ContextMenuRegistry::SetRegistryValue(HKEY hKey, const wchar_t* valueName, const wchar_t* data) {
  LONG result = RegSetValueExW(
    hKey,
    valueName,
    0,
    REG_SZ,
    (const BYTE*)data,
    (DWORD)((wcslen(data) + 1) * sizeof(wchar_t))
  );
  return result == ERROR_SUCCESS;
}

bool ContextMenuRegistry::DeleteRegistryKey(HKEY hRootKey, const wchar_t* subKey) {
  LONG result = RegDeleteTreeW(hRootKey, subKey);
  return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

bool ContextMenuRegistry::Register(const wchar_t* exePath, UserRole role) {
  HKEY hKey = nullptr;
  HKEY hCommandKey = nullptr;
  bool success = false;

  HKEY rootKey = GetRootKey(role);
  std::wstring registryPath = GetRegistryPath(role);

  LONG result = RegCreateKeyExW(
    rootKey,
    registryPath.c_str(),
    0,
    nullptr,
    REG_OPTION_NON_VOLATILE,
    KEY_WRITE,
    nullptr,
    &hKey,
    nullptr
  );

  if (result != ERROR_SUCCESS) {
    return false;
  }

  if (!SetRegistryValue(hKey, nullptr, L"Open with Hex Viewer")) {
    RegCloseKey(hKey);
    return false;
  }

  std::wstring iconPath = std::wstring(exePath) + L",0";
  SetRegistryValue(hKey, L"Icon", iconPath.c_str());

  result = RegCreateKeyExW(
    hKey,
    L"command",
    0,
    nullptr,
    REG_OPTION_NON_VOLATILE,
    KEY_WRITE,
    nullptr,
    &hCommandKey,
    nullptr
  );

  if (result == ERROR_SUCCESS) {
    std::wstring command = std::wstring(L"\"") + exePath + L"\" \"%1\"";
    success = SetRegistryValue(hCommandKey, nullptr, command.c_str());
    RegCloseKey(hCommandKey);
  }

  RegCloseKey(hKey);

  if (success) {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  }

  return success;
}

bool ContextMenuRegistry::Unregister(UserRole role) {
  HKEY rootKey = GetRootKey(role);
  std::wstring registryPath = GetRegistryPath(role);

  bool success = DeleteRegistryKey(rootKey, registryPath.c_str());

  if (success) {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  }

  return success;
}

bool ContextMenuRegistry::IsRegistered(UserRole role) {
  HKEY hKey = nullptr;
  HKEY rootKey = GetRootKey(role);
  std::wstring registryPath = GetRegistryPath(role);

  LONG result = RegOpenKeyExW(
    rootKey,
    registryPath.c_str(),
    0,
    KEY_READ,
    &hKey
  );

  if (result == ERROR_SUCCESS) {
    RegCloseKey(hKey);
    return true;
  }

  return false;
}

#endif // _WIN32
