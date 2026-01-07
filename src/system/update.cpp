#include "update.h"
#include "global.h"
#include "options.h"

#ifdef _WIN32
#include <shellapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <appmodel.h>
#else
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#endif

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <sys/stat.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <climits>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#endif

#include <language.h>

#ifdef _WIN32

class WinThread
{
private:
  HANDLE handle;
  DWORD threadId;
  static DWORD WINAPI ThreadProc(LPVOID param);
  void (*func)(void);

public:
  WinThread(void (*f)(void)) : handle(nullptr), threadId(0), func(f)
  {
    handle = CreateThread(nullptr, 0, ThreadProc, this, 0, &threadId);
  }

  ~WinThread()
  {
    if (handle)
    {
      WaitForSingleObject(handle, INFINITE);
      CloseHandle(handle);
    }
  }

  void Join()
  {
    if (handle)
    {
      WaitForSingleObject(handle, INFINITE);
    }
  }

  bool Joinable() const
  {
    return handle != nullptr;
  }
};

DWORD WINAPI WinThread::ThreadProc(LPVOID param)
{
  WinThread *thread = (WinThread *)param;
  if (thread && thread->func)
  {
    thread->func();
  }
  return 0;
}

void WinSleep(int milliseconds)
{
  Sleep(milliseconds);
}

#else

typedef std::string WinString;
typedef std::thread WinThread;

void WinSleep(int ms)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#endif

RenderManager *UpdateDialog::renderer = nullptr;
UpdateInfo UpdateDialog::currentInfo = {};
bool UpdateDialog::darkMode = true;
int UpdateDialog::hoveredButton = 0;
int UpdateDialog::pressedButton = 0;
int UpdateDialog::scrollOffset = 0;
bool UpdateDialog::scrollbarHovered = false;
bool UpdateDialog::scrollbarPressed = false;
int UpdateDialog::scrollDragStart = 0;
int UpdateDialog::scrollOffsetStart = 0;
Rect UpdateDialog::updateButtonRect = {};
Rect UpdateDialog::skipButtonRect = {};
Rect UpdateDialog::closeButtonRect = {};
Rect UpdateDialog::scrollbarRect = {};
Rect UpdateDialog::scrollThumbRect = {};

static DownloadState downloadState = DownloadState::Idle;
static float downloadProgress = 0.0f;
static WinString downloadStatus;
static WinString downloadedFilePath;
static Rect progressBarRect = {};
static WinThread *downloadThread = nullptr;
static bool betaEnabled = false;
static Rect betaToggleRect = {};
static bool betaToggleHovered = false;

#ifdef __linux__
static Display *display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#elif __APPLE__
static NSWindow *nsWindow = nullptr;
static bool windowShouldClose = false;
#endif

#ifdef _WIN32
WinString GetTempDownloadPath()
{
  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  char temp[MAX_PATH];
  WideCharToMultiByte(CP_UTF8, 0, tempPath, -1, temp, MAX_PATH, nullptr, nullptr);

  WinString result;
  result.Append(temp);
  result.Append("update_download");
  return result;
}
#else
std::string GetTempDownloadPath()
{
  return "/tmp/update_download";
}
#endif

#ifdef _WIN32
WinString ExtractJsonValue(const WinString &json, const char *key)
{
  WinString result;

  WinString searchKey;
  searchKey.Append("\"");
  searchKey.Append(key);
  searchKey.Append("\"");

  int pos = json.Find(searchKey.CStr());
  if (pos == -1)
    return result;

  const char *str = json.CStr();
  size_t len = json.Length();
  size_t colonPos = pos + searchKey.Length();
  while (colonPos < len && str[colonPos] != ':')
    colonPos++;
  if (colonPos >= len)
    return result;

  size_t quotePos = colonPos + 1;
  while (quotePos < len && str[quotePos] != '"')
    quotePos++;
  if (quotePos >= len)
    return result;

  size_t endPos = quotePos + 1;
  while (endPos < len && str[endPos] != '"')
    endPos++;
  if (endPos >= len)
    return result;

  return json.Substr(quotePos + 1, endPos - quotePos - 1);
}
#else
std::string ExtractJsonValue(const std::string &json, const std::string &key)
{
  std::string searchKey = "\"" + key + "\"";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos)
    return "";

  pos = json.find(":", pos);
  if (pos == std::string::npos)
    return "";

  pos = json.find("\"", pos);
  if (pos == std::string::npos)
    return "";

  size_t endPos = json.find("\"", pos + 1);
  if (endPos == std::string::npos)
    return "";

  return json.substr(pos + 1, endPos - pos - 1);
}
#endif

#ifdef _WIN32
bool ExtractJsonBool(const WinString &json, const char *key)
{
  WinString searchKey;
  searchKey.Append("\"");
  searchKey.Append(key);
  searchKey.Append("\"");

  int pos = json.Find(searchKey.CStr());
  if (pos == -1)
    return false;

  const char *str = json.CStr();
  size_t len = json.Length();
  size_t colonPos = pos + searchKey.Length();
  while (colonPos < len && str[colonPos] != ':')
    colonPos++;
  if (colonPos >= len)
    return false;

  size_t truePos = colonPos;
  while (truePos < len - 4)
  {
    if (str[truePos] == 't' && str[truePos + 1] == 'r' &&
        str[truePos + 2] == 'u' && str[truePos + 3] == 'e')
    {
      size_t checkPos = truePos + 4;
      while (checkPos < len && (str[checkPos] == ' ' || str[checkPos] == '\t'))
        checkPos++;
      if (checkPos >= len || str[checkPos] == ',' || str[checkPos] == '}')
      {
        return true;
      }
    }
    truePos++;
    if (str[truePos] == ',' || str[truePos] == '}')
      break;
  }

  return false;
}
#else
bool ExtractJsonBool(const std::string &json, const std::string &key)
{
  std::string searchKey = "\"" + key + "\"";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos)
    return false;

  pos = json.find(":", pos);
  if (pos == std::string::npos)
    return false;

  size_t truePos = json.find("true", pos);
  size_t falsePos = json.find("false", pos);
  size_t commaPos = json.find(",", pos);
  size_t bracePos = json.find("}", pos);

  size_t endPos = std::min(commaPos, bracePos);

  if (truePos != std::string::npos && truePos < endPos)
  {
    return true;
  }
  return false;
}
#endif

#ifdef _WIN32
WinString HttpGet(const char *url)
{
  WinString result;

  HINTERNET hInternet = InternetOpenA("UpdaterAgent/1.0",
                                      INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet)
    return result;

  DWORD timeout = 5000;
  InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
  InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

  HINTERNET hConnect = InternetOpenUrlA(hInternet, url, nullptr, 0,
                                        INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
  if (!hConnect)
  {
    InternetCloseHandle(hInternet);
    return result;
  }

  char buffer[4096];
  DWORD bytesRead;

  while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
  {
    for (DWORD i = 0; i < bytesRead; i++)
    {
      result.Append(buffer[i]);
    }
  }

  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return result;
}
#else
std::string HttpGet(const std::string &url)
{
  std::string command = "curl --max-time 5 -L -s \"" + url + "\"";
  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe)
    return "";

  std::string result;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe))
  {
    result += buffer;
  }
  pclose(pipe);
  return result;
}
#endif

#ifdef _WIN32
bool DownloadFile(const char *url, const char *outputPath,
                  void (*progressCallback)(float, const char *))
{

  HINTERNET hInternet = InternetOpenA("UpdaterAgent/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet)
    return false;

  HINTERNET hConnect = InternetOpenUrlA(hInternet, url, nullptr, 0,
                                        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
  if (!hConnect)
  {
    InternetCloseHandle(hInternet);
    return false;
  }

  DWORD fileSize = 0;
  DWORD bufferSize = sizeof(fileSize);
  HttpQueryInfoA(hConnect, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                 &fileSize, &bufferSize, nullptr);

  HANDLE hFile = CreateFileA(outputPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return false;
  }

  char buffer[8192];
  DWORD bytesRead;
  DWORD totalRead = 0;
  DWORD bytesWritten;

  while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
  {
    WriteFile(hFile, buffer, bytesRead, &bytesWritten, nullptr);
    totalRead += bytesRead;

    if (fileSize > 0 && progressCallback)
    {
      float progress = (float)totalRead / (float)fileSize;

      char status[256];
      char totalKB[32], fileKB[32];
      IntToStr(totalRead / 1024, totalKB, sizeof(totalKB));
      IntToStr(fileSize / 1024, fileKB, sizeof(fileKB));

      StringCopy(status, "Downloaded ", sizeof(status));
      StrCat(status, totalKB);
      StrCat(status, " KB / ");
      StrCat(status, fileKB);
      StrCat(status, " KB");

      progressCallback(progress, status);
    }
  }

  CloseHandle(hFile);
  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return true;
}
#else
bool DownloadFile(const std::string &url, const std::string &outputPath,
                  std::function<void(float, const std::string &)> progressCallback)
{
  std::string command = "curl -L \"" + url + "\" -o \"" + outputPath + "\"";
  int result = system(command.c_str());
  progressCallback(1.0f, "Download complete");
  return result == 0;
}
#endif

#ifdef _WIN32
WinString GetAssetDownloadUrl(const char *releaseApiUrl, bool includeBeta)
{
  WinString result;
  bool isMsix = IsMsixPackage();

  if (isMsix)
  {
    return result;
  }

  WinString modifiedUrl;
  modifiedUrl.Append(releaseApiUrl);

  if (includeBeta)
  {
    int pos = modifiedUrl.Find("/releases/latest");
    if (pos != -1)
    {
      modifiedUrl = modifiedUrl.Substr(0, pos);
      modifiedUrl.Append("/releases");
    }
  }

  WinString response = HttpGet(modifiedUrl.CStr());
  if (response.IsEmpty())
    return result;

  bool isNative = GetIsNativeFlag();
  const char *targetExtension;

  targetExtension = isNative ? ".exe" : ".zip";

  if (includeBeta)
  {
    int releaseStart = response.Find("{\"url\"");
    if (releaseStart != -1)
    {
      WinString firstRelease = response.Substr(releaseStart, response.Length() - releaseStart);
      int releaseEnd = firstRelease.Find("},");
      if (releaseEnd == -1)
      {
        releaseEnd = firstRelease.Find("}]");
      }
      if (releaseEnd != -1)
      {
        response = firstRelease.Substr(0, releaseEnd + 1);
      }
    }
  }

  int assetsPos = response.Find("\"assets\"");
  if (assetsPos == -1)
    return result;

  int currentPos = assetsPos;
  while (true)
  {
    WinString searchArea = response.Substr(currentPos, response.Length() - currentPos);
    int namePos = searchArea.Find("\"name\"");
    if (namePos == -1)
      break;

    WinString nameArea = searchArea.Substr(namePos, searchArea.Length() - namePos);
    WinString assetName = ExtractJsonValue(nameArea, "name");

    if (assetName.Find(targetExtension) != -1)
    {
      int urlPos = searchArea.Find("\"browser_download_url\"");
      if (urlPos != -1 && urlPos > namePos)
      {
        WinString urlArea = searchArea.Substr(urlPos, searchArea.Length() - urlPos);
        return ExtractJsonValue(urlArea, "browser_download_url");
      }
    }
    currentPos += namePos + 1;
    if (currentPos >= (int)response.Length())
      break;
  }

  return result;
}
#else
std::string GetAssetDownloadUrl(const std::string &releaseApiUrl, bool includeBeta)
{
  bool isMsix = IsMsixPackage();

  if (isMsix)
  {
    return "";
  }

  std::string modifiedUrl = releaseApiUrl;
  if (includeBeta)
  {
    size_t pos = modifiedUrl.find("/releases/latest");
    if (pos != std::string::npos)
    {
      modifiedUrl = modifiedUrl.substr(0, pos) + "/releases";
    }
  }

  std::string response = HttpGet(modifiedUrl);
  if (response.empty())
    return "";

  bool isNative = GetIsNativeFlag();
  std::string targetExtension;

#ifdef __APPLE__
  targetExtension = isNative ? ".dmg" : ".zip";
#elif __linux__
  targetExtension = isNative ? ".deb" : ".tar.gz";
#else
  targetExtension = ".zip";
#endif

  if (includeBeta)
  {
    size_t releaseStart = response.find("{\"url\"");
    if (releaseStart != std::string::npos)
    {
      std::string firstRelease = response.substr(releaseStart);
      size_t releaseEnd = firstRelease.find("},");
      if (releaseEnd == std::string::npos)
      {
        releaseEnd = firstRelease.find("}]");
      }
      if (releaseEnd != std::string::npos)
      {
        response = firstRelease.substr(0, releaseEnd + 1);
      }
    }
  }

  size_t assetsPos = response.find("\"assets\"");
  if (assetsPos == std::string::npos)
    return "";

  size_t currentPos = assetsPos;
  while (true)
  {
    size_t namePos = response.find("\"name\"", currentPos);
    if (namePos == std::string::npos)
      break;

    std::string assetName = ExtractJsonValue(response.substr(namePos), "name");
    if (assetName.find(targetExtension) != std::string::npos)
    {
      size_t urlPos = response.find("\"browser_download_url\"", namePos);
      if (urlPos != std::string::npos)
      {
        return ExtractJsonValue(response.substr(urlPos), "browser_download_url");
      }
    }
    currentPos = namePos + 1;
  }

  return "";
}
#endif

#ifdef _WIN32
void ProgressCallbackWrapper(float progress, const char *status)
{
  downloadProgress = progress;
  downloadStatus.Clear();
  downloadStatus.Append(status);
}

void DownloadFromGitHub()
{
  bool isMsix = IsMsixPackage();

  if (isMsix)
  {
    downloadState = DownloadState::Error;
    downloadStatus.Clear();
    downloadStatus.Append("MSIX packages update through Microsoft Store");
    return;
  }

  downloadState = DownloadState::Connecting;
  downloadStatus.Clear();
  downloadStatus.Append("Connecting to GitHub...");
  downloadProgress = 0.0f;

  WinString assetUrl = GetAssetDownloadUrl(UpdateDialog::currentInfo.releaseApiUrl, betaEnabled);
  if (assetUrl.IsEmpty())
  {
    downloadState = DownloadState::Error;
    downloadStatus.Clear();
    downloadStatus.Append("Failed to find download asset");
    return;
  }

  downloadState = DownloadState::Downloading;
  downloadStatus.Clear();
  downloadStatus.Append(betaEnabled ? "Downloading beta update..." : "Downloading update...");

  bool isNative = GetIsNativeFlag();
  const char *extension = isNative ? ".exe" : ".zip";

  downloadedFilePath = GetTempDownloadPath();
  downloadedFilePath.Append(extension);

  bool success = DownloadFile(assetUrl.CStr(), downloadedFilePath.CStr(), ProgressCallbackWrapper);

  if (!success)
  {
    downloadState = DownloadState::Error;
    downloadStatus.Clear();
    downloadStatus.Append("Download failed");
    return;
  }

  downloadState = DownloadState::Installing;
  downloadStatus.Clear();
  downloadStatus.Append("Installing update...");

  if (isNative)
  {
    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, downloadedFilePath.CStr(), -1, wPath, MAX_PATH);
    ShellExecuteW(nullptr, L"open", wPath, nullptr, nullptr, SW_SHOWNORMAL);
  }
  else
  {
    wchar_t wPath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, downloadedFilePath.CStr(), -1, wPath, MAX_PATH);

    wchar_t cmdLine[MAX_PATH * 2];
    cmdLine[0] = 0;
    WcsCopy(cmdLine, L"/select,\"");
    StrCat((char *)cmdLine, (char *)wPath);
    StrCat((char *)cmdLine, (char *)L"\"");

    ShellExecuteW(nullptr, L"open", L"explorer.exe", cmdLine, nullptr, SW_SHOWNORMAL);
  }

  downloadState = DownloadState::Complete;
  downloadStatus.Clear();
  downloadStatus.Append("Update installed successfully! Please restart.");
  downloadProgress = 1.0f;
}
#else
void DownloadFromGitHub()
{
  bool isMsix = IsMsixPackage();

  if (isMsix)
  {
    downloadState = DownloadState::Error;
    downloadStatus = "MSIX packages update through Microsoft Store";
    return;
  }

  downloadState = DownloadState::Connecting;
  downloadStatus = "Connecting to GitHub...";
  downloadProgress = 0.0f;

  std::string assetUrl = GetAssetDownloadUrl(UpdateDialog::currentInfo.releaseApiUrl, betaEnabled);
  if (assetUrl.empty())
  {
    downloadState = DownloadState::Error;
    downloadStatus = "Failed to find download asset";
    return;
  }

  downloadState = DownloadState::Downloading;
  downloadStatus = betaEnabled ? "Downloading beta update..." : "Downloading update...";

  bool isNative = GetIsNativeFlag();
  std::string extension = isNative ? ".exe" : ".zip";
#ifdef __APPLE__
  extension = isNative ? ".dmg" : ".zip";
#elif __linux__
  extension = isNative ? ".deb" : ".tar.gz";
#endif

  downloadedFilePath = GetTempDownloadPath() + extension;

  bool success = DownloadFile(assetUrl, downloadedFilePath,
                              [](float progress, const std::string &status)
                              {
                                downloadProgress = progress;
                                downloadStatus = status;
                              });

  if (!success)
  {
    downloadState = DownloadState::Error;
    downloadStatus = "Download failed";
    return;
  }

  downloadState = DownloadState::Installing;
  downloadStatus = "Installing update...";

#ifdef __APPLE__
  if (isNative)
  {
    std::string cmd = "open \"" + downloadedFilePath + "\"";
    system(cmd.c_str());
  }
  else
  {
    std::string cmd = "open -R \"" + downloadedFilePath + "\"";
    system(cmd.c_str());
  }
#else
  if (isNative)
  {
    std::string cmd = "xterm -e 'sudo dpkg -i \"" + downloadedFilePath + "\"'";
    system(cmd.c_str());
  }
  else
  {
    std::string cmd = "xdg-open \"$(dirname '" + downloadedFilePath + "')\"";
    system(cmd.c_str());
  }
#endif

  downloadState = DownloadState::Complete;
  downloadStatus = "Update installed successfully! Please restart.";
  downloadProgress = 1.0f;
}
#endif

void StartDownload()
{
    if (downloadThread)
    {
#ifdef _WIN32
        if (downloadThread->Joinable())
            downloadThread->Join();
#else
        if (downloadThread->joinable())
            downloadThread->join();
#endif
        delete downloadThread;
        downloadThread = nullptr;
    }

#ifdef _WIN32
    downloadThread = new WinThread(DownloadFromGitHub);
#else
    downloadThread = new std::thread(DownloadFromGitHub);
#endif
}



bool UpdateDialog::Show(NativeWindow parent, const UpdateInfo &info)
{
  currentInfo = info;
  hoveredButton = 0;
  pressedButton = 0;
  scrollOffset = 0;
  downloadState = DownloadState::Idle;
  downloadProgress = 0.0f;
#ifdef _WIN32
  downloadStatus.Clear();
#else
  downloadStatus = "";
#endif
  betaEnabled = info.betaPreference;
  betaToggleHovered = false;

  int width = 600;
  int height = 500;

#ifdef _WIN32
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = DialogProc;
  wcex.hInstance = GetModuleHandle(nullptr);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"UpdateDialogClass";

  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowExW(
      WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
      L"UpdateDialogClass",
      info.updateAvailable ? L"Update Available" : L"No Updates Available",
      WS_POPUP | WS_CAPTION | WS_SYSMENU,
      CW_USEDEFAULT, CW_USEDEFAULT,
      width, height,
      (HWND)parent,
      nullptr,
      GetModuleHandle(nullptr),
      nullptr);

  if (!hWnd)
    return false;

  if (parent)
  {
    RECT parentRect;
    GetWindowRect((HWND)parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }

  renderer = new RenderManager();
  if (!renderer->initialize(hWnd))
  {
    DestroyWindow(hWnd);
    delete renderer;
    return false;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderer->resize(rc.right - rc.left, rc.bottom - rc.top);

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);

  MSG msg;
  bool result = false;
  while (GetMessage(&msg, nullptr, 0, 0))
  {
    if (msg.message == WM_QUIT || !IsWindow(hWnd))
    {
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (downloadThread)
  {
    if (downloadThread->Joinable())
    {
      downloadThread->Join();
    }
    delete downloadThread;
    downloadThread = nullptr;
  }

  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"UpdateDialogClass", GetModuleHandle(nullptr));

#elif __APPLE__
  @autoreleasepool
  {
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;

    nsWindow = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:styleMask
                                             backing:NSBackingStoreBuffered
                                               defer:NO];

    NSString *title = info.updateAvailable ? @"Update Available" : @"No Updates Available";
    [nsWindow setTitle:title];
    [nsWindow setLevel:NSFloatingWindowLevel];
    [nsWindow center];

    NSView *contentView = [[NSView alloc] initWithFrame:frame];
    [nsWindow setContentView:contentView];

    [nsWindow makeKeyAndOrderFront:nil];

    renderer = new RenderManager();
    if (!renderer->initialize((NativeWindow)nsWindow))
    {
      [nsWindow close];
      nsWindow = nullptr;
      delete renderer;
      return false;
    }

    renderer->resize(width, height);

    bool running = true;
    windowShouldClose = false;

    while (running && !windowShouldClose)
    {
      @autoreleasepool
      {
        NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];

        if (event)
        {
          NSEventType eventType = [event type];

          switch (eventType)
          {
          case NSEventTypeLeftMouseDown:
          {
            NSPoint point = [event locationInWindow];
            OnMouseDown((int)point.x, (int)point.y);
            break;
          }

          case NSEventTypeLeftMouseUp:
          {
            NSPoint point = [event locationInWindow];
            if (OnMouseUp((int)point.x, (int)point.y))
            {
              running = false;
            }
            break;
          }

          case NSEventTypeMouseMoved:
          case NSEventTypeLeftMouseDragged:
          {
            NSPoint point = [event locationInWindow];
            OnMouseMove((int)point.x, (int)point.y);
            break;
          }

          case NSEventTypeScrollWheel:
          {
            CGFloat deltaY = [event scrollingDeltaY];
            scrollOffset -= (int)(deltaY * 2);
            scrollOffset = ClampInt(scrollOffset, 0, INT_MAX);
            OnPaint();
            break;
          }

          default:
            [NSApp sendEvent:event];
            break;
          }
        }

        if (![nsWindow isVisible])
        {
          running = false;
        }

        OnPaint();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
    }

    if (downloadThread)
    {
      if (downloadThread->joinable())
      {
        downloadThread->join();
      }
      delete downloadThread;
      downloadThread = nullptr;
    }

    delete renderer;
    renderer = nullptr;
    [nsWindow close];
    nsWindow = nullptr;
  }

#elif __linux__
  display = XOpenDisplay(nullptr);
  if (!display)
    return false;

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
  attrs.background_pixel = WhitePixel(display, screen);

  window = XCreateWindow(
      display, root,
      0, 0, width, height, 0,
      CopyFromParent, InputOutput, CopyFromParent,
      CWBackPixel | CWEventMask,
      &attrs);

  const char *title = info.updateAvailable ? "Update Available" : "No Updates Available";
  XStoreName(display, window, title);
  XSetIconName(display, window, title);

  Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom wmWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(display, window, wmWindowType, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&wmWindowTypeDialog, 1);

  XSizeHints *sizeHints = XAllocSizeHints();
  sizeHints->flags = PMinSize | PMaxSize;
  sizeHints->min_width = sizeHints->max_width = width;
  sizeHints->min_height = sizeHints->max_height = height;
  XSetWMNormalHints(display, window, sizeHints);
  XFree(sizeHints);

  wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  int screenWidth = DisplayWidth(display, screen);
  int screenHeight = DisplayHeight(display, screen);
  XMoveWindow(display, window, (screenWidth - width) / 2, (screenHeight - height) / 2);

  XMapWindow(display, window);
  XFlush(display);

  renderer = new RenderManager();
  if (!renderer->initialize((NativeWindow)window))
  {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    delete renderer;
    return false;
  }

  renderer->resize(width, height);

  bool running = true;
  XEvent event;

  while (running)
  {
    while (XPending(display))
    {
      XNextEvent(display, &event);

      switch (event.type)
      {
      case Expose:
        if (event.xexpose.count == 0)
        {
          OnPaint();
        }
        break;

      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteWindow)
        {
          running = false;
        }
        break;

      case MotionNotify:
        OnMouseMove(event.xmotion.x, event.xmotion.y);
        break;

      case ButtonPress:
        if (event.xbutton.button == Button1)
        {
          OnMouseDown(event.xbutton.x, event.xbutton.y);
        }
        else if (event.xbutton.button == Button4)
        {
          scrollOffset -= 20;
          scrollOffset = ClampInt(scrollOffset, 0, INT_MAX);
          OnPaint();
        }
        else if (event.xbutton.button == Button5)
        {
          scrollOffset += 20;
          OnPaint();
        }
        break;

      case ButtonRelease:
        if (event.xbutton.button == Button1)
        {
          if (OnMouseUp(event.xbutton.x, event.xbutton.y))
          {
            running = false;
          }
        }
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  if (downloadThread)
  {
    if (downloadThread->joinable())
    {
      downloadThread->join();
    }
    delete downloadThread;
    downloadThread = nullptr;
  }

  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#endif

  return true;
}

#ifdef _WIN32
LRESULT CALLBACK UpdateDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CREATE:
    break;

  case WM_ERASEBKGND:
    return 1;

  case WM_PAINT:
    OnPaint(hWnd);
    return 0;

  case WM_MOUSEMOVE:
  {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseMove(hWnd, x, y);
    break;
  }

  case WM_LBUTTONDOWN:
  {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseDown(hWnd, x, y);
    break;
  }

  case WM_LBUTTONUP:
  {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseUp(hWnd, x, y);
    break;
  }

  case WM_MOUSEWHEEL:
  {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    scrollOffset -= (delta > 0 ? 20 : -20);
    scrollOffset = ClampInt(scrollOffset, 0, scrollOffset);
    InvalidateRect(hWnd, nullptr, FALSE);
    break;
  }

  case WM_CLOSE:
    DestroyWindow(hWnd);
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProcW(hWnd, message, wParam, lParam);
}

void UpdateDialog::OnPaint(HWND hWnd)
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  if (!renderer)
  {
    EndPaint(hWnd, &ps);
    return;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;

  RenderContent(width, height);
  renderer->endFrame(hdc);

  if (downloadState != DownloadState::Idle &&
      downloadState != DownloadState::Complete &&
      downloadState != DownloadState::Error)
  {
    InvalidateRect(hWnd, nullptr, FALSE);
  }

  EndPaint(hWnd, &ps);
}

void UpdateDialog::OnMouseMove(HWND hWnd, int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;
  bool isMsix = IsMsixPackage();

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix)
  {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
        y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
    {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix)
  {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
        y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
    {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
             y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height)
    {
      hoveredButton = 2;
    }
  }
  else if ((downloadState == DownloadState::Idle && currentInfo.updateAvailable && isMsix) ||
           (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) ||
           downloadState == DownloadState::Complete ||
           downloadState == DownloadState::Error)
  {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
        y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height)
    {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
                      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered)
  {
    InvalidateRect(hWnd, nullptr, FALSE);
  }
}

void UpdateDialog::OnMouseDown(HWND hWnd, int x, int y)
{
  pressedButton = hoveredButton;

  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    InvalidateRect(hWnd, nullptr, FALSE);
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height)
  {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
    SetCapture(hWnd);
  }

  InvalidateRect(hWnd, nullptr, FALSE);
}

bool UpdateDialog::OnMouseUp(HWND hWnd, int x, int y)
{
  if (scrollbarPressed)
  {
    scrollbarPressed = false;
    ReleaseCapture();
  }

  if (pressedButton == hoveredButton && pressedButton > 0)
  {
    if (pressedButton == 1 && downloadState == DownloadState::Idle)
    {
      StartDownload();
      InvalidateRect(hWnd, nullptr, FALSE);
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3 ||
             (pressedButton == 1 && (downloadState == DownloadState::Complete || downloadState == DownloadState::Error)))
    {
      DestroyWindow(hWnd);
      return true;
    }
  }

  pressedButton = 0;
  InvalidateRect(hWnd, nullptr, FALSE);
  return false;
}
#elif __linux__

void UpdateDialog::OnPaint()
{
  if (!renderer)
    return;

  RenderContent(600, 500);
  XFlush(display);
}

void UpdateDialog::OnMouseMove(int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable)
  {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
        y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
    {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable)
  {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
        y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
    {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
             y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height)
    {
      hoveredButton = 2;
    }
  }
  else if ((downloadState == DownloadState::Idle && currentInfo.updateAvailable) ||
           (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) ||
           downloadState == DownloadState::Complete ||
           downloadState == DownloadState::Error)
  {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
        y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height)
    {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
                      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered)
  {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y)
{
  pressedButton = hoveredButton;

  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    OnPaint();
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height)
  {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y)
{
  if (scrollbarPressed)
  {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0)
  {
    if (pressedButton == 1 && downloadState == DownloadState::Idle)
    {
      StartDownload();
      OnPaint();
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3 ||
             (pressedButton == 1 && (downloadState == DownloadState::Complete || downloadState == DownloadState::Error)))
    {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#elif __APPLE__

void UpdateDialog::OnPaint()
{
  if (!renderer)
    return;

  RenderContent(600, 500);

  @autoreleasepool
  {
    [[nsWindow contentView] setNeedsDisplay:YES];
  }
}

void UpdateDialog::OnMouseMove(int x, int y)
{
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable)
  {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
        y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height)
    {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable)
  {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
        y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height)
    {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
             y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height)
    {
      hoveredButton = 2;
    }
  }
  else if (downloadState == DownloadState::Idle && !currentInfo.updateAvailable)
  {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
        y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height)
    {
      hoveredButton = 3;
    }
  }
  else if (downloadState == DownloadState::Complete || downloadState == DownloadState::Error)
  {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
        y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height)
    {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
                      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered)
  {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y)
{
  pressedButton = hoveredButton;

  if (betaToggleHovered)
  {
    betaEnabled = !betaEnabled;
    OnPaint();
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
      y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height)
  {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y)
{
  if (scrollbarPressed)
  {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0)
  {
    if (pressedButton == 1 && downloadState == DownloadState::Idle)
    {
      StartDownload();
      OnPaint();
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3 ||
             (pressedButton == 1 && (downloadState == DownloadState::Complete || downloadState == DownloadState::Error)))
    {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}
#endif

void UpdateDialog::RenderContent(int width, int height)
{
  if (!renderer)
    return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();
  bool isMsix = IsMsixPackage();

  renderer->clear(theme.windowBackground);

  Rect headerRect(0, 0, width, 80);
  renderer->drawRect(headerRect, theme.menuBackground, true);

  Rect iconRect(20, 20, 40, 40);
  renderer->drawRect(iconRect, Color(100, 150, 255), true);
  renderer->drawText("i", 35, 32, Color::White());

  const char *title = currentInfo.updateAvailable ? Translations::T("Update Available!") : Translations::T("You're up to date!");
  renderer->drawText(title, 80, 25, theme.textColor);

  char versionText[512];
  StringCopy(versionText, Translations::T("Current:"), sizeof(versionText));
  StrCat(versionText, " ");
#ifdef _WIN32
    StrCat(versionText, currentInfo.currentVersion);
#else
    StrCat(versionText, currentInfo.currentVersion.c_str());
#endif
  if (currentInfo.updateAvailable)
  {
    StrCat(versionText, " → ");
    StrCat(versionText, Translations::T("Latest:"));
    StrCat(versionText, " ");
    #ifdef _WIN32
    StrCat(versionText, currentInfo.latestVersion);
    #else
    StrCat(versionText, currentInfo.latestVersion.c_str());
    #endif
  }
  renderer->drawText(versionText, 80, 50, theme.disabledText);

  renderer->drawLine(0, 80, width, 80, theme.separator);

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix)
  {
    betaToggleRect = Rect(20, 90, 200, 30);

    WidgetState cbState;
    cbState.rect = Rect(betaToggleRect.x, betaToggleRect.y + 5, 20, 20);
    cbState.enabled = true;
    cbState.hovered = betaToggleHovered;
    cbState.pressed = (pressedButton == 99);

    renderer->drawModernCheckbox(cbState, theme, betaEnabled);

    renderer->drawText(
        Translations::T("Include Beta Versions"),
        betaToggleRect.x + 30,
        betaToggleRect.y + 8,
        theme.textColor);
  }

  if (downloadState != DownloadState::Idle)
  {
    int progressY = 130;

    Color statusColor = theme.textColor;
    if (downloadState == DownloadState::Error)
    {
      statusColor = Color(255, 100, 100);
    }
    else if (downloadState == DownloadState::Complete)
    {
      statusColor = Color(100, 255, 100);
    }

#ifdef _WIN32
    renderer->drawText(downloadStatus.CStr(), 20, progressY, statusColor);
#else
    renderer->drawText(downloadStatus.c_str(), 20, progressY, statusColor);
#endif

    progressBarRect = Rect(20, progressY + 30, width - 40, 30);
    renderer->drawProgressBar(progressBarRect, downloadProgress, theme);

    char pctText[32];
    IntToStr((int)(downloadProgress * 100), pctText, sizeof(pctText));
    StrCat(pctText, "%");
    renderer->drawText(pctText, width / 2 - 15, progressY + 38, theme.textColor);
  }
  else if (currentInfo.updateAvailable && isMsix)
  {
    renderer->drawText(Translations::T("Microsoft Store Package Detected"), 20, 130, theme.textColor);
    renderer->drawText(Translations::T("This application is installed via the Microsoft Store."), 20, 160, theme.disabledText);
    renderer->drawText(Translations::T("Updates are managed automatically through the Store."), 20, 185, theme.disabledText);
    renderer->drawText(Translations::T("Please check the Microsoft Store for updates."), 20, 210, theme.disabledText);
  }
#ifdef _WIN32
    else if (currentInfo.updateAvailable &&
             currentInfo.releaseNotes &&
             currentInfo.releaseNotes[0])
#else
    else if (currentInfo.updateAvailable &&
             !currentInfo.releaseNotes.empty())
#endif
  {
    int headerY = 150;
    renderer->drawText(Translations::T("What's New:"), 20, headerY, theme.textColor);

#ifdef _WIN32
    const char* notes = currentInfo.releaseNotes;
#else
    const char* notes = currentInfo.releaseNotes.c_str();
#endif
    
    int y = headerY + 40 - scrollOffset;
    int maxY = height - 100;
    int maxLineWidth = width - 70;
    int charWidth = 8;
    int maxCharsPerLine = maxLineWidth / charWidth;

    char line[512];
    int linePos = 0;
    int lastSpacePos = -1;

    for (int i = 0; notes[i]; i++)
    {
      if (notes[i] == '\n')
      {
        line[linePos] = '\0';
        if (y > 80 && y < maxY)
        {
          renderer->drawText(line, 30, y, theme.disabledText);
        }
        y += 25;
        linePos = 0;
        lastSpacePos = -1;
        continue;
      }

      line[linePos] = notes[i];
      
      if (notes[i] == ' ')
      {
        lastSpacePos = linePos;
      }
      
      linePos++;

      if (linePos >= maxCharsPerLine - 1)
      {
        int breakPoint = linePos;
        
        if (lastSpacePos > 0 && lastSpacePos > linePos - 20)
        {
          breakPoint = lastSpacePos;
        }
        
        line[breakPoint] = '\0';
        if (y > 80 && y < maxY)
        {
          renderer->drawText(line, 30, y, theme.disabledText);
        }
        y += 25;

        int remaining = linePos - breakPoint - 1;
        if (remaining > 0 && breakPoint < linePos)
        {
          for (int j = 0; j < remaining; j++)
          {
            line[j] = line[breakPoint + 1 + j];
          }
          linePos = remaining;
        }
        else
        {
          linePos = 0;
        }
        lastSpacePos = -1;
      }
    }

    if (linePos > 0)
    {
      line[linePos] = '\0';
      if (y > 80 && y < maxY)
      {
        renderer->drawText(line, 30, y, theme.disabledText);
      }
      y += 25;
    }

    int contentHeight = y + scrollOffset - 160;
    if (contentHeight > (maxY - 160))
    {
      scrollbarRect = Rect(width - 15, 130, 10, maxY - 130);
      renderer->drawRect(scrollbarRect, theme.scrollbarBg, true);

      int viewHeight = maxY - 160;
      float visibleRatio = 0.0f;
      if (contentHeight > 0)
      {
        visibleRatio = (float)viewHeight / (float)contentHeight;
      }

      visibleRatio = Clamp(visibleRatio, 0.0f, 1.0f);
      int thumbHeight = (int)(scrollbarRect.height * visibleRatio);
      thumbHeight = ClampInt(thumbHeight, 30, scrollbarRect.height);

      int maxScroll = contentHeight - viewHeight;
      maxScroll = ClampInt(maxScroll, 0, INT_MAX);
      float scrollNorm = 0.0f;
      if (maxScroll > 0)
      {
        scrollNorm = (float)scrollOffset / (float)maxScroll;
      }
      scrollNorm = Clamp(scrollNorm, 0.0f, 1.0f);
      int thumbY =
          scrollbarRect.y +
          (int)((scrollbarRect.height - thumbHeight) * scrollNorm);

      scrollThumbRect = Rect(scrollbarRect.x, thumbY, scrollbarRect.width, thumbHeight);

      Color thumbColor = scrollbarHovered ? Color(100, 100, 100) : Color(80, 80, 80);
      renderer->drawRect(scrollThumbRect, thumbColor, true);
    }
  }
  else if (!currentInfo.updateAvailable)
  {
    renderer->drawText(Translations::T("You have the latest version installed."), 20, 150, theme.textColor);
    renderer->drawText(Translations::T("Check back later for updates."), 20, 180, theme.disabledText);
  }

  int buttonY = height - 60;
  int buttonHeight = 40;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix)
  {
    updateButtonRect = Rect(width - 280, buttonY, 120, buttonHeight);
    WidgetState updateState;
    updateState.rect = updateButtonRect;
    updateState.enabled = true;
    updateState.hovered = (hoveredButton == 1);
    updateState.pressed = (pressedButton == 1);
    const char *downloadText = betaEnabled ? Translations::T("Download Beta") : Translations::T("Download");
    renderer->drawModernButton(updateState, theme, downloadText);

    skipButtonRect = Rect(width - 150, buttonY, 100, buttonHeight);
    WidgetState skipState;
    skipState.rect = skipButtonRect;
    skipState.enabled = true;
    skipState.hovered = (hoveredButton == 2);
    skipState.pressed = (pressedButton == 2);
    renderer->drawModernButton(skipState, theme, Translations::T("Skip"));
  }
  else if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && isMsix)
  {
    closeButtonRect = Rect((width - 120) / 2, buttonY, 120, buttonHeight);
    WidgetState closeState;
    closeState.rect = closeButtonRect;
    closeState.enabled = true;
    closeState.hovered = (hoveredButton == 3);
    closeState.pressed = (pressedButton == 3);
    renderer->drawModernButton(closeState, theme, Translations::T("Close"));
  }
  else if (downloadState == DownloadState::Complete || downloadState == DownloadState::Error ||
           (downloadState == DownloadState::Idle && !currentInfo.updateAvailable))
  {
    closeButtonRect = Rect((width - 120) / 2, buttonY, 120, buttonHeight);
    WidgetState closeState;
    closeState.rect = closeButtonRect;
    closeState.enabled = true;
    closeState.hovered = (hoveredButton == 3);
    closeState.pressed = (pressedButton == 3);
    renderer->drawModernButton(closeState, theme, Translations::T("Close"));
  }
}
