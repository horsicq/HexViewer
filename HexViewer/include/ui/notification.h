#ifndef APP_NOTIFICATION_H
#define APP_NOTIFICATION_H

#include <string>

class AppNotification {
public:
  enum class NotificationIcon {
    Deafult,
    Info,
    Warning,
    Error
  };

  static AppNotification& GetInstance();

  static bool IsAvailable();

  static void Show(const std::string& title,
    const std::string& message,
    const std::string& appId = "HexViewer",
    NotificationIcon icon = NotificationIcon::Info,
    int timeoutMs = 5000);

private:
  AppNotification();
  ~AppNotification();
  AppNotification(const AppNotification&) = delete;
  AppNotification& operator=(const AppNotification&) = delete;

#ifdef _WIN32
  bool m_initialized;
  bool m_shortcutCreated;
  bool EnsureShortcutExists(const std::wstring& appId);
  void ShowWindows(const std::string& title,
    const std::string& message,
    const std::string& appId,
    NotificationIcon icon,
    int timeoutMs);
  void ShowWindowsFallback(const std::string& title,
    const std::string& message,
    NotificationIcon icon);
  static std::wstring EscapeXml(const std::wstring& str);
#endif

#ifdef __linux__
  void ShowLinux(const std::string& title,
    const std::string& message,
    NotificationIcon icon,
    int timeoutMs);
#endif

#ifdef __APPLE__
  void ShowMacOS(const std::string& title,
    const std::string& message);
#endif
};

#endif // APP_NOTIFICATION_H
