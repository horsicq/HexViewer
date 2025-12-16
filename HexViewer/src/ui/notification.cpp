#include "notification.h"
#include <string>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.ui.notifications.h>
#include <windows.data.xml.dom.h>
#include <ShlObj.h>
#include <Shobjidl.h>
#include <propvarutil.h>
#include <propkey.h>
#include <comdef.h>
#include <fstream>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::UI::Notifications;
using namespace ABI::Windows::Data::Xml::Dom;

#elif defined(__linux__)
#include <unistd.h>
#endif

AppNotification::AppNotification()
#ifdef _WIN32
  : m_shortcutCreated(false)
#endif
{
#ifdef _WIN32
  m_initialized = false;

  HRESULT hrCom = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hrCom) && hrCom != RPC_E_CHANGED_MODE) {
    return;
  }

  HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
  if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
    m_initialized = true;
  }
#endif
}
AppNotification::~AppNotification() {
#ifdef _WIN32
  if (m_initialized) {
    RoUninitialize();
  }
#endif
}

AppNotification& AppNotification::GetInstance() {
  static AppNotification instance;
  return instance;
}

bool AppNotification::IsAvailable() {
#ifdef _WIN32
  return true;
#elif defined(__linux__)
  return system("which notify-send > /dev/null 2>&1") == 0;
#elif defined(__APPLE__)
  return system("which osascript > /dev/null 2>&1") == 0;
#else
  return false;
#endif
}

void AppNotification::Show(const std::string& title,
  const std::string& message,
  const std::string& appId,
  NotificationIcon icon,
  int timeoutMs) {
#ifdef _WIN32
  GetInstance().ShowWindows(title, message, appId, icon, timeoutMs);
#elif defined(__linux__)
  GetInstance().ShowLinux(title, message, icon, timeoutMs);
#elif defined(__APPLE__)
  GetInstance().ShowMacOS(title, message);
#endif
}

#ifdef _WIN32

void WriteDebugLog(const std::string& message) {
  std::ofstream logFile("notification_debug.log", std::ios::app);
  if (logFile.is_open()) {
    logFile << message << std::endl;
    logFile.close();
  }
}

bool AppNotification::EnsureShortcutExists(const std::wstring& appId) {
  if (m_shortcutCreated) {
    WriteDebugLog("Shortcut already created");
    return true;
  }

  wchar_t exePath[MAX_PATH];
  if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0) {
    WriteDebugLog("Failed to get exe path");
    return false;
  }

  wchar_t startMenuPath[MAX_PATH];
  if (FAILED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, startMenuPath))) {
    WriteDebugLog("Failed to get start menu path");
    return false;
  }

  std::wstring shortcutPath = std::wstring(startMenuPath) + L"\\" + appId + L".lnk";

  DWORD attribs = GetFileAttributesW(shortcutPath.c_str());
  if (attribs != INVALID_FILE_ATTRIBUTES) {
    m_shortcutCreated = true;
    WriteDebugLog("Shortcut already exists at: " + std::string(shortcutPath.begin(), shortcutPath.end()));
    return true;
  }

  WriteDebugLog("Creating shortcut at: " + std::string(shortcutPath.begin(), shortcutPath.end()));

  IShellLinkW* pShellLink = NULL;
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
    IID_IShellLinkW, (LPVOID*)&pShellLink);

  if (SUCCEEDED(hr)) {
    pShellLink->SetPath(exePath);
    pShellLink->SetDescription(L"HexViewer Application");
    pShellLink->SetWorkingDirectory(L"");

    IPropertyStore* pPropertyStore = NULL;
    hr = pShellLink->QueryInterface(IID_IPropertyStore, (LPVOID*)&pPropertyStore);

    if (SUCCEEDED(hr)) {
      PROPVARIANT pv;
      PropVariantInit(&pv);
      pv.vt = VT_LPWSTR;
      pv.pwszVal = const_cast<wchar_t*>(appId.c_str());

      hr = pPropertyStore->SetValue(PKEY_AppUserModel_ID, pv);
      if (SUCCEEDED(hr)) {
        hr = pPropertyStore->Commit();
        WriteDebugLog("Set AppUserModelID successfully");
      }
      else {
        WriteDebugLog("Failed to set AppUserModelID");
      }
      pPropertyStore->Release();
    }

    IPersistFile* pPersistFile = NULL;
    hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);

    if (SUCCEEDED(hr)) {
      hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
      if (SUCCEEDED(hr)) {
        m_shortcutCreated = true;
        WriteDebugLog("Shortcut created successfully");
      }
      else {
        WriteDebugLog("Failed to save shortcut, HRESULT: " + std::to_string(hr));
      }
      pPersistFile->Release();
    }
    pShellLink->Release();
  }
  else {
    WriteDebugLog("Failed to create IShellLink, HRESULT: " + std::to_string(hr));
  }

  return m_shortcutCreated;
}

void AppNotification::ShowWindowsFallback(const std::string& title,
  const std::string& message,
  NotificationIcon icon) {
  WriteDebugLog("FALLBACK: " + title + " - " + message);

  std::string iconStr = "INFO";
  switch (icon) {
  case NotificationIcon::Warning: iconStr = "WARNING"; break;
  case NotificationIcon::Error: iconStr = "ERROR"; break;
  default: iconStr = "INFO"; break;
  }

  OutputDebugStringA(("[" + iconStr + "] " + title + ": " + message + "\n").c_str());
}

void AppNotification::ShowWindows(const std::string& title,
  const std::string& message,
  const std::string& appId,
  NotificationIcon icon,
  int timeoutMs) {

  WriteDebugLog("=== ShowWindows called ===");
  WriteDebugLog("Title: " + title);
  WriteDebugLog("Message: " + message);
  WriteDebugLog("AppId: " + appId);

  if (!m_initialized) {
    WriteDebugLog("ERROR: WinRT not initialized");
    ShowWindowsFallback(title, message, icon);
    return;
  }

  std::wstring wTitle(title.begin(), title.end());
  std::wstring wMessage(message.begin(), message.end());
  std::wstring wAppId(appId.begin(), appId.end());

  if (!EnsureShortcutExists(wAppId)) {
    WriteDebugLog("WARNING: Could not ensure shortcut exists, trying anyway...");
  }

  HRESULT hr = S_OK;

  ComPtr<IToastNotificationManagerStatics> toastStatics;
  hr = Windows::Foundation::GetActivationFactory(
    HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(),
    &toastStatics);
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to get ToastNotificationManagerStatics, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }
  WriteDebugLog("Got ToastNotificationManagerStatics");

  ComPtr<IXmlDocument> toastXml;
  hr = toastStatics->GetTemplateContent(ToastTemplateType_ToastText02, &toastXml);
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to get template, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }
  WriteDebugLog("Got template XML");

  ComPtr<IXmlNodeList> textNodes;
  toastXml->GetElementsByTagName(HStringReference(L"text").Get(), &textNodes);

  if (textNodes) {
    ComPtr<IXmlNode> titleNode;
    textNodes->Item(0, &titleNode);
    if (titleNode) {
      ComPtr<IXmlText> titleText;
      ComPtr<IXmlDocument> xmlDoc;
      titleNode->get_OwnerDocument(&xmlDoc);
      if (xmlDoc) {
        xmlDoc->CreateTextNode(HStringReference(wTitle.c_str()).Get(), &titleText);
        ComPtr<IXmlNode> titleTextNode;
        titleText.As(&titleTextNode);
        ComPtr<IXmlNode> appendedChild;
        titleNode->AppendChild(titleTextNode.Get(), &appendedChild);
        WriteDebugLog("Set title text");
      }
    }

    ComPtr<IXmlNode> messageNode;
    textNodes->Item(1, &messageNode);
    if (messageNode) {
      ComPtr<IXmlText> messageText;
      ComPtr<IXmlDocument> xmlDoc;
      messageNode->get_OwnerDocument(&xmlDoc);
      if (xmlDoc) {
        xmlDoc->CreateTextNode(HStringReference(wMessage.c_str()).Get(), &messageText);
        ComPtr<IXmlNode> messageTextNode;
        messageText.As(&messageTextNode);
        ComPtr<IXmlNode> appendedChild;
        messageNode->AppendChild(messageTextNode.Get(), &appendedChild);
        WriteDebugLog("Set message text");
      }
    }
  }

  ComPtr<IToastNotificationFactory> toastFactory;
  hr = Windows::Foundation::GetActivationFactory(
    HStringReference(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(),
    &toastFactory);
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to get ToastNotificationFactory, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }
  WriteDebugLog("Got ToastNotificationFactory");

  ComPtr<IToastNotification> toast;
  hr = toastFactory->CreateToastNotification(toastXml.Get(), &toast);
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to create toast notification, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }
  WriteDebugLog("Created toast notification");

  ComPtr<IToastNotifier> notifier;
  hr = toastStatics->CreateToastNotifierWithId(
    HStringReference(wAppId.c_str()).Get(),
    &notifier);
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to create notifier, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }
  WriteDebugLog("Created notifier");

  hr = notifier->Show(toast.Get());
  if (FAILED(hr)) {
    WriteDebugLog("ERROR: Failed to show toast, HRESULT: " + std::to_string(hr));
    ShowWindowsFallback(title, message, icon);
    return;
  }

  WriteDebugLog("SUCCESS: Toast notification shown!");
}

std::wstring AppNotification::EscapeXml(const std::wstring& str) {
  std::wstring result;
  for (wchar_t c : str) {
    switch (c) {
    case L'&': result += L"&amp;"; break;
    case L'<': result += L"&lt;"; break;
    case L'>': result += L"&gt;"; break;
    case L'"': result += L"&quot;"; break;
    case L'\'': result += L"&apos;"; break;
    default: result += c; break;
    }
  }
  return result;
}
#endif

#ifdef __linux__
void AppNotification::ShowLinux(const std::string& title,
  const std::string& message,
  NotificationIcon icon,
  int timeoutMs) {
  std::string iconStr;
  switch (icon) {
  case NotificationIcon::Info:    iconStr = "dialog-information"; break;
  case NotificationIcon::Warning: iconStr = "dialog-warning"; break;
  case NotificationIcon::Error:   iconStr = "dialog-error"; break;
  default:                        iconStr = "dialog-information"; break;
  }

  std::ostringstream cmd;
  cmd << "notify-send -t " << timeoutMs
    << " -i " << iconStr
    << " \"" << title << "\" \"" << message << "\"";
  system(cmd.str().c_str());
}
#endif

#ifdef __APPLE__
void AppNotification::ShowMacOS(const std::string& title,
  const std::string& message) {
  std::ostringstream cmd;
  cmd << "osascript -e 'display notification \""
    << message << "\" with title \"" << title << "\"'";
  system(cmd.str().c_str());
}
#endif
