#include <string>
#include <vector>
#include <cstring> 
#include <algorithm>
#include <thread>
#include <chrono>

#include "render.h"
#include "options.h"
#include "HexData.h"
#include "searchdialog.h"
#include "menu.h"
#include "language.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <windowsx.h>
#include <shellapi.h>
#include <dwmapi.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif
#include <about.h>
#include <darkmode.h>
#include <codecvt>

RenderManager* renderManager = nullptr;
HexData* hexData = nullptr;
MenuBar* menuBar = nullptr;
int scrollPos = 0;
int maxScrollPos = 0;
bool darkMode = true;
bool scrollbarHovered = false;
bool scrollbarPressed = false;
Rect scrollbarRect = {};
Rect thumbRect = {};
int dragStartY = 0;
int dragStartScroll = 0;

std::string currentFilePath;
size_t editingOffset = (size_t)-1;
std::string editBuffer;
int editingRow = -1;
int editingCol = -1;

#ifdef _WIN32
AppOptions g_options = { true, 16, false, false, "English" };
HWND g_hWnd = NULL;
std::wstring currentFilePathW;

#elif __APPLE__
AppOptions g_options = { true, 16, false };
NSWindow* g_nsWindow = nullptr;

#else  // Linux
AppOptions g_options = { true, 16, false, false, "English"};
Display* g_display = nullptr;
Window g_window = 0;
Atom wmDeleteMessage;
#endif

void LoadFile();
void LoadFileFromPath(const char* filepath);
void SaveFile();
void SaveFileAs();
void UpdateScrollbar(int windowWidth, int windowHeight);
size_t GetByteOffsetFromClick(int x, int y, int* outRow, int* outCol, int windowWidth, int windowHeight);
int CalculateBytesPerLine(int windowWidth);

#ifdef _WIN32
void ShowOptionsDialog();
#endif

#ifdef _WIN32
void RebuildMenuBar() {
  if (menuBar) {
    delete menuBar;
    menuBar = nullptr;
  }

  menuBar = new MenuBar();

  Menu fileMenu(Translations::T("File"));
  MenuItem openItem(Translations::T("Open"), MenuItemType::Normal);
  openItem.shortcut = "Ctrl+O";
  openItem.callback = []() { LoadFile(); };
  fileMenu.items.push_back(openItem);

  MenuItem saveItem(Translations::T("Save"), MenuItemType::Normal);
  saveItem.shortcut = "Ctrl+S";
  saveItem.callback = []() { SaveFile(); };
  fileMenu.items.push_back(saveItem);

  MenuItem exitItem(Translations::T("Exit"), MenuItemType::Normal);
  exitItem.callback = []() { PostQuitMessage(0); };
  fileMenu.items.push_back(exitItem);

  Menu searchMenu(Translations::T("Search"));
  MenuItem findReplaceItem(Translations::T("Find and Replace..."), MenuItemType::Normal);
  findReplaceItem.shortcut = "Ctrl+F";
  findReplaceItem.callback = []() {
    SearchDialogs::ShowFindReplaceDialog(g_hWnd, g_options.darkMode,
      [](const std::string& find, const std::string& replace) {
        MessageBoxA(g_hWnd,
          ("Find: " + find + "\nReplace: " + replace).c_str(),
          "Find & Replace", MB_OK);
      });
    };
  searchMenu.items.push_back(findReplaceItem);

  MenuItem goToItem(Translations::T("Go To..."), MenuItemType::Normal);
  goToItem.shortcut = "Ctrl+G";
  goToItem.callback = []() {
    SearchDialogs::ShowGoToDialog(g_hWnd, g_options.darkMode,
      [](int line) {
        MessageBoxA(g_hWnd,
          ("Go to line: " + std::to_string(line)).c_str(),
          "Go To", MB_OK);
      });
    };
  searchMenu.items.push_back(goToItem);

  Menu toolsMenu(Translations::T("Tools"));
  MenuItem optionsItem(Translations::T("Options..."), MenuItemType::Normal);
  optionsItem.callback = []() { ShowOptionsDialog(); };
  toolsMenu.items.push_back(optionsItem);

  Menu helpMenu(Translations::T("Help"));
  MenuItem aboutItem(Translations::T("About HexViewer"), MenuItemType::Normal);
  aboutItem.callback = []() {
    AboutDialog::Show(g_hWnd, g_options.darkMode);
    };
  helpMenu.items.push_back(aboutItem);

  menuBar->addMenu(fileMenu);
  menuBar->addMenu(searchMenu);
  menuBar->addMenu(toolsMenu);
  menuBar->addMenu(helpMenu);

  menuBar->setPosition(0, 0);
  menuBar->setHeight(24);
}

void ShowOptionsDialog() {
  AppOptions oldOptions = g_options;

  if (OptionsDialog::Show(g_hWnd, g_options)) {
    bool needsRedraw = false;

    if (oldOptions.darkMode != g_options.darkMode) {
      darkMode = g_options.darkMode;
      needsRedraw = true;
    }

    if (oldOptions.language != g_options.language) {
      Translations::SetLanguage(g_options.language);
      RebuildMenuBar();  // Rebuild menu with new translations
      needsRedraw = true;
    }

    if (oldOptions.defaultBytesPerLine != g_options.defaultBytesPerLine && !hexData->isEmpty()) {
      hexData->regenerateHexLines(g_options.defaultBytesPerLine);
      scrollPos = 0;
      RECT rc;
      GetClientRect(g_hWnd, &rc);
      UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
      needsRedraw = true;
    }

    SaveOptionsToFile(g_options);

    if (needsRedraw) {
      InvalidateRect(g_hWnd, NULL, FALSE);
    }
  }
}

void LoadFile() {
  OPENFILENAMEW ofn = {};
  WCHAR szFile[260] = {};

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_hWnd;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
  ofn.lpstrFilter = L"All Files\0*.*\0Binary Files\0*.bin;*.exe;*.dll\0Text Files\0*.txt\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (GetOpenFileNameW(&ofn) == TRUE) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::string path = conv.to_bytes(szFile);

    LoadFileFromPath(path.c_str());
  }

}

void SaveFileAs() {
  if (hexData->isEmpty()) {
    MessageBoxW(g_hWnd, L"No file loaded to save.", L"Save File", MB_OK | MB_ICONWARNING);
    return;
  }

  OPENFILENAMEW ofn = {};
  WCHAR szFile[260] = {};

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = g_hWnd;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile) / sizeof(WCHAR);
  ofn.lpstrFilter = L"All Files\0*.*\0Binary Files\0*.bin\0";
  ofn.nFilterIndex = 1;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

  if (GetSaveFileNameW(&ofn) == TRUE) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, szFile, -1, nullptr, 0, nullptr, nullptr);
    std::string path(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, szFile, -1, &path[0], size_needed, nullptr, nullptr);

    if (hexData->saveFile(path.c_str())) {
      currentFilePath = path;
      currentFilePathW = szFile;

      MessageBoxW(g_hWnd, L"File saved successfully!", L"Save File", MB_OK | MB_ICONINFORMATION);

      std::wstring title = L"HexViewer - " + currentFilePathW;
      SetWindowTextW(g_hWnd, title.c_str());
    }
    else {
      MessageBoxW(g_hWnd, L"Failed to save file.", L"Save File", MB_OK | MB_ICONERROR);
    }
  }

}

void SaveFile() {
  if (hexData->isEmpty()) {
    MessageBoxW(g_hWnd, L"No file loaded to save.", L"Save File", MB_OK | MB_ICONWARNING);
    return;
  }

  if (currentFilePath.empty()) {
    SaveFileAs();
    return;
  }

  if (hexData->saveFile(currentFilePath.c_str())) {
    MessageBoxW(g_hWnd, L"File saved successfully!", L"Save File", MB_OK | MB_ICONINFORMATION);
  }
  else {
    MessageBoxW(g_hWnd, L"Failed to save file.", L"Save File", MB_OK | MB_ICONERROR);
  }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    g_hWnd = hWnd;
    DragAcceptFiles(hWnd, TRUE);
    break;

  case WM_ERASEBKGND:
    return 1;

  case WM_DROPFILES: {
    HDROP hDrop = (HDROP)wParam;
    WCHAR filepath[MAX_PATH];
    UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

    if (fileCount > 0) {
      if (DragQueryFileW(hDrop, 0, filepath, MAX_PATH) > 0) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        std::string path = conv.to_bytes(filepath);

        LoadFileFromPath(path.c_str());
      }
      if (fileCount > 1) {
        MessageBoxW(hWnd, L"Only the first file was loaded.", L"Multiple Files", MB_OK | MB_ICONINFORMATION);
      }
    }

    DragFinish(hDrop);
    break;
  }
  case WM_COMMAND: {
    int wmId = LOWORD(wParam);
    switch (wmId) {
    case 1: LoadFile(); break;
    case 2: SaveFile(); break;
    case 3: SaveFileAs(); break;
    case 4: PostQuitMessage(0); break;
    case 5: ShowOptionsDialog(); break;
    }
    break;
  }
  case WM_CHAR: {
    char ch = (char)wParam;

    if (editingOffset != (size_t)-1) {
      if (ch == VK_BACK && !editBuffer.empty()) {
        editBuffer.pop_back();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }

      if (ch == VK_ESCAPE) {
        editingOffset = (size_t)-1;
        editBuffer.clear();
        editingRow = -1;
        editingCol = -1;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }

      if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
        if (ch >= 'a' && ch <= 'f') {
          ch = ch - 'a' + 'A';
        }

        editBuffer += ch;

        if (editBuffer.length() == 2) {
          uint8_t newValue = (uint8_t)strtol(editBuffer.c_str(), nullptr, 16);
          hexData->editByte(editingOffset, newValue);

          editingOffset++;
          if (editingOffset >= hexData->getFileSize()) {
            editingOffset = (size_t)-1;
            editingRow = -1;
            editingCol = -1;
          }
          else {
            editingCol++;
            if (editingCol >= hexData->getCurrentBytesPerLine()) {
              editingCol = 0;
              editingRow++;
            }
          }
          editBuffer.clear();
        }
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }
    }
    break;
  }
  case WM_KEYDOWN: {
    bool ctrlPressed = GetKeyState(VK_CONTROL) & 0x8000;

    if (ctrlPressed && wParam == 'S') {
      SaveFile();
      return 0;
    }

    if (ctrlPressed && wParam == 'O') {
      LoadFile();
      return 0;
    }

    if (ctrlPressed && wParam == 'F') {
      SearchDialogs::ShowFindReplaceDialog(g_hWnd, g_options.darkMode,
        [](const std::string& find, const std::string& replace) {
          MessageBoxA(g_hWnd,
            ("Find: " + find + "\nReplace: " + replace).c_str(),
            "Find & Replace", MB_OK);
        });
      return 0;
    }

    if (ctrlPressed && wParam == 'G') {
      SearchDialogs::ShowGoToDialog(g_hWnd, g_options.darkMode,
        [](int offset) {
          MessageBoxA(g_hWnd,
            ("Go to offset: 0x" + std::to_string(offset)).c_str(),
            "Go To", MB_OK);
        });
      return 0;
    }

    if (wParam == VK_ESCAPE && editingOffset != (size_t)-1) {
      editingOffset = (size_t)-1;
      editBuffer.clear();
      editingRow = -1;
      editingCol = -1;
      InvalidateRect(hWnd, NULL, FALSE);
      return 0;
    }

    if (editingOffset != (size_t)-1) {
      bool moved = false;
      int bytesPerLine = hexData->getCurrentBytesPerLine();

      if (wParam == VK_RIGHT && editingOffset + 1 < hexData->getFileSize()) {
        editingOffset++;
        editingCol++;
        if (editingCol >= bytesPerLine) {
          editingCol = 0;
          editingRow++;
        }
        moved = true;
      }
      else if (wParam == VK_LEFT && editingOffset > 0) {
        editingOffset--;
        editingCol--;
        if (editingCol < 0) {
          editingCol = bytesPerLine - 1;
          editingRow--;
        }
        moved = true;
      }
      else if (wParam == VK_DOWN && editingOffset + bytesPerLine < hexData->getFileSize()) {
        editingOffset += bytesPerLine;
        editingRow++;
        moved = true;
      }
      else if (wParam == VK_UP && editingOffset >= (size_t)bytesPerLine) {
        editingOffset -= bytesPerLine;
        editingRow--;
        moved = true;
      }

      if (moved) {
        editBuffer.clear();
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }
    }
    break;
  }
  case WM_SIZE: {
    RECT rc;
    GetClientRect(hWnd, &rc);
    if (renderManager) {
      renderManager->resize(rc.right - rc.left, rc.bottom - rc.top);
      UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
    }
    InvalidateRect(hWnd, NULL, FALSE);
    break;
  }
  case WM_MOUSEMOVE: {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    if (menuBar) {
      RECT rc;
      int windowWidth = 0, windowHeight = 0;
      GetClientRect(hWnd, &rc);
      windowWidth = rc.right - rc.left;
      windowHeight = rc.bottom - rc.top;

      bool stateChanged = menuBar->handleMouseMove(x, y);
      if (stateChanged) {
        Rect menuBounds = menuBar->getBounds(windowWidth);
        InvalidateRect(hWnd, (RECT*)&menuBounds, FALSE);
      }
    }

    bool wasScrollbarHovered = scrollbarHovered;
    scrollbarHovered = (maxScrollPos > 0 &&
      x >= scrollbarRect.x && x <= scrollbarRect.x + scrollbarRect.width &&
      y >= scrollbarRect.y && y <= scrollbarRect.y + scrollbarRect.height);

    if (scrollbarPressed && maxScrollPos > 0) {
      int deltaY = y - dragStartY;
      float scrollRange = scrollbarRect.height - thumbRect.height;
      if (scrollRange > 0) {
        int scrollDelta = (int)(deltaY * maxScrollPos / scrollRange);
        scrollPos = dragStartScroll + scrollDelta;
        scrollPos = std::max(0, std::min(scrollPos, maxScrollPos));

        RECT rc;
        GetClientRect(hWnd, &rc);
        UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
        InvalidateRect(hWnd, NULL, FALSE);
      }
    }

    if (wasScrollbarHovered != scrollbarHovered) {
      InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
  }
  case WM_LBUTTONDOWN: {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);
    if (menuBar && menuBar->handleMouseDown(x, y)) {
      InvalidateRect(hWnd, NULL, FALSE);
      break;
    }

    RECT rc;
    GetClientRect(hWnd, &rc);

    int row, col;
    size_t offset = GetByteOffsetFromClick(x, y, &row, &col, rc.right - rc.left, rc.bottom - rc.top);
    if (offset != (size_t)-1) {
      editingOffset = offset;
      editingRow = row;
      editingCol = col;
      editBuffer.clear();
      InvalidateRect(hWnd, NULL, FALSE);
    }
    else {
      if (editingOffset != (size_t)-1) {
        editingOffset = (size_t)-1;
        editBuffer.clear();
        editingRow = -1;
        editingCol = -1;
        InvalidateRect(hWnd, NULL, FALSE);
      }

      if (maxScrollPos > 0 &&
        x >= thumbRect.x && x <= thumbRect.x + thumbRect.width &&
        y >= thumbRect.y && y <= thumbRect.y + thumbRect.height) {
        scrollbarPressed = true;
        dragStartY = y;
        dragStartScroll = scrollPos;
        SetCapture(hWnd);
      }
    }
    break;
  }
  case WM_LBUTTONUP: {
    int x = GET_X_LPARAM(lParam);
    int y = GET_Y_LPARAM(lParam);

    break;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    if (renderManager && hexData) {
      renderManager->renderHexViewer(
        hexData->getHexLines(),
        hexData->getHeaderLine(),
        scrollPos,
        maxScrollPos,
        scrollbarHovered,
        scrollbarPressed,
        scrollbarRect,
        thumbRect,
        darkMode,
        editingRow,
        editingCol,
        editBuffer);

      if (menuBar) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        menuBar->render(renderManager, rc.right - rc.left);
      }

      renderManager->endFrame();
    }

    EndPaint(hWnd, &ps);
    break;
  }
  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    scrollPos -= (delta > 0 ? 3 : -3);
    scrollPos = std::max(0, std::min(scrollPos, maxScrollPos));

    RECT rc;
    GetClientRect(hWnd, &rc);
    UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(hWnd, NULL, FALSE);
    break;
  }
  case WM_DESTROY:
    SearchDialogs::CleanupDialogs();
    if (menuBar) {
      delete menuBar;
      menuBar = nullptr;
    }
    if (menuBar) {
      delete menuBar;
      menuBar = nullptr;
    }
    if (renderManager) {
      delete renderManager;
      renderManager = nullptr;
    }
    if (hexData) {
      delete hexData;
      hexData = nullptr;
    }
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASSEXW wcex = {};

  Translations::Initialize();

  DetectNative();
  LoadOptionsFromFile(g_options);

  Translations::SetLanguage(g_options.language);

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"HexViewerClass";
  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowW(L"HexViewerClass", L"HexViewer", WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, 1024, 768, nullptr, nullptr, hInstance, nullptr);

  if (!hWnd) return FALSE;

  ApplyDarkTitleBar(hWnd, g_options.darkMode);

  renderManager = new RenderManager();
  if (!renderManager->initialize(hWnd)) {
    MessageBoxW(hWnd, L"Failed to initialize render manager!", L"Error", MB_OK | MB_ICONERROR);
    return FALSE;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderManager->resize(rc.right - rc.left, rc.bottom - rc.top);
  hexData = new HexData();

  RebuildMenuBar();

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (argv && argc > 1) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    std::string path = conv.to_bytes(argv[1]);

    LoadFileFromPath(path.c_str());
  }
  LocalFree(argv);

  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int)msg.wParam;
}
#elif __APPLE__

NSWindow* g_nsWindow = nullptr;

void LoadFile() {
  NSOpenPanel* panel = [NSOpenPanel openPanel];
  [panel setCanChooseFiles : YES] ;
  [panel setCanChooseDirectories : NO] ;
  [panel setAllowsMultipleSelection : NO] ;

  if ([panel runModal] == NSModalResponseOK) {
    NSURL* url = [[panel URLs]objectAtIndex:0];
    const char* filepath = [[url path]UTF8String];
    LoadFileFromPath(filepath);
  }
}

void SaveFile() {
  if (hexData->isEmpty()) {
    NSAlert* alert = [[NSAlert alloc]init];
    [alert setMessageText:@"No file loaded to save."] ;
    [alert setInformativeText:@"Please load a file first."] ;
    [alert runModal] ;
    return;
  }

  if (currentFilePath.empty()) {
    SaveFileAs();
    return;
  }

  hexData->saveFile(currentFilePath.c_str());
}

void SaveFileAs() {
  if (hexData->isEmpty()) {
    NSAlert* alert = [[NSAlert alloc]init];
    [alert setMessageText:@"No file loaded to save."] ;
    [alert setInformativeText:@"Please load a file first."] ;
    [alert runModal] ;
    return;
  }

  NSSavePanel* panel = [NSSavePanel savePanel];

  if ([panel runModal] == NSModalResponseOK) {
    NSURL* url = [panel URL];
    const char* filepath = [[url path]UTF8String];
    if (hexData->saveFile(filepath)) {
      currentFilePath = filepath;
      [g_nsWindow setTitle:[NSString stringWithFormat:@"HexViewer - %s", filepath] ] ;
    }
  }
}

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property(nonatomic, assign) bool* running;
@property(nonatomic, assign) bool* needsRedraw;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed : (NSApplication*)sender {
  return YES;
}

- (void)windowWillClose : (NSNotification*)notification {
  if (self.running) {
    *self.running = false;
  }
}

- (void)windowDidResize:(NSNotification*)notification {
  if (self.needsRedraw) {
    *self.needsRedraw = true;
  }
}

@end

@interface HexView : NSOpenGLView
@property(nonatomic, assign) bool* needsRedraw;
@end

@implementation HexView

- (void)drawRect:(NSRect)dirtyRect {
  if (renderManager && hexData) {
    NSRect bounds = [self bounds];

    renderManager->renderHexViewer(
      hexData->getHexLines(),
      hexData->getHeaderLine(),
      scrollPos,
      maxScrollPos,
      scrollbarHovered,
      scrollbarPressed,
      scrollbarRect,
      thumbRect,
      darkMode,
      editingRow,
      editingCol,
      editBuffer);

    if (menuBar) {
      menuBar->render(renderManager, bounds.size.width);
    }

    renderManager->endFrame();
    [[self openGLContext]flushBuffer];
  }
}

- (void)mouseMoved:(NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView : nil];
  NSRect bounds = [self bounds];
  int x = (int)location.x;
  int y = (int)(bounds.size.height - location.y);

  if (menuBar) {
    bool stateChanged = menuBar->handleMouseMove(x, y);
    if (stateChanged && self.needsRedraw) {
      *self.needsRedraw = true;
      [self setNeedsDisplay:YES] ;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (maxScrollPos > 0 &&
    x >= scrollbarRect.x &&
    x <= scrollbarRect.x + scrollbarRect.width &&
    y >= scrollbarRect.y &&
    y <= scrollbarRect.y + scrollbarRect.height);

  if (wasScrollbarHovered != scrollbarHovered && self.needsRedraw) {
    *self.needsRedraw = true;
    [self setNeedsDisplay:YES] ;
  }
}

- (void)mouseDown:(NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView : nil];
  NSRect bounds = [self bounds];
  int x = (int)location.x;
  int y = (int)(bounds.size.height - location.y);

  if (menuBar && menuBar->handleMouseDown(x, y)) {
    if (self.needsRedraw) {
      *self.needsRedraw = true;
      [self setNeedsDisplay:YES] ;
    }
    return;
  }

  int row, col;
  size_t offset = GetByteOffsetFromClick(x, y, &row, &col,
    bounds.size.width, bounds.size.height);
  if (offset != (size_t)-1) {
    editingOffset = offset;
    editingRow = row;
    editingCol = col;
    editBuffer.clear();
    if (self.needsRedraw) {
      *self.needsRedraw = true;
      [self setNeedsDisplay:YES] ;
    }
  }
}

- (void)scrollWheel:(NSEvent*)event {
  CGFloat deltaY = [event deltaY];
  scrollPos -= (int)(deltaY * 3);
  scrollPos = std::max(0, std::min(scrollPos, maxScrollPos));

  NSRect bounds = [self bounds];
  UpdateScrollbar(bounds.size.width, bounds.size.height);

  if (self.needsRedraw) {
    *self.needsRedraw = true;
    [self setNeedsDisplay:YES] ;
  }
}

- (void)keyDown:(NSEvent*)event {
  NSString* chars = [event charactersIgnoringModifiers];
  if ([chars length] > 0) {
    unichar key = [chars characterAtIndex:0];
    BOOL cmdPressed = ([event modifierFlags] & NSEventModifierFlagCommand) != 0;

    if (cmdPressed && (key == 's' || key == 'S')) {
      SaveFile();
      return;
    }
    if (cmdPressed && (key == 'o' || key == 'O')) {
      LoadFile();
      return;
    }
  }
}

- (BOOL)acceptsFirstResponder {
  return YES;
}

@end

int main(int argc, char** argv) {
  @autoreleasepool {
    [NSApplication sharedApplication] ;

    DetectNative();
    LoadOptionsFromFile(g_options);

    AppDelegate* delegate = [[AppDelegate alloc]init];
    bool running = true;
    bool needsRedraw = true;
    delegate.running = &running;
    delegate.needsRedraw = &needsRedraw;

    [NSApp setDelegate:delegate] ;

    NSRect frame = NSMakeRect(0, 0, 1024, 768);
    NSWindowStyleMask style = NSWindowStyleMaskTitled |
      NSWindowStyleMaskClosable |
      NSWindowStyleMaskMiniaturizable |
      NSWindowStyleMaskResizable;

    g_nsWindow = [[NSWindow alloc]initWithContentRect:frame
      styleMask : style
      backing : NSBackingStoreBuffered
      defer : NO];
    [g_nsWindow setTitle:@"HexViewer"] ;
    [g_nsWindow center] ;
    [g_nsWindow setDelegate:delegate] ;

    NSOpenGLPixelFormatAttribute attributes[] = {
      NSOpenGLPFADoubleBuffer,
      NSOpenGLPFADepthSize, 24,
      NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
      0
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc]
      initWithAttributes:attributes];
    HexView* glView = [[HexView alloc]initWithFrame:frame
      pixelFormat : pixelFormat];
    glView.needsRedraw = &needsRedraw;

    [g_nsWindow setContentView:glView] ;
    [g_nsWindow makeKeyAndOrderFront:nil] ;

    NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
      initWithRect:[glView bounds]
      options : (NSTrackingMouseMoved | NSTrackingActiveInKeyWindow |
        NSTrackingInVisibleRect)
      owner : glView
      userInfo : nil];
    [glView addTrackingArea:trackingArea] ;

    [[glView openGLContext]makeCurrentContext];

    renderManager = new RenderManager();
    if (!renderManager->initialize((void*)CFBridgingRetain(glView))) {
      fprintf(stderr, "Failed to initialize render manager\n");
      return 1;
    }

    renderManager->resize(1024, 768);
    hexData = new HexData();

    menuBar = new MenuBar();

    Menu fileMenu = MenuHelper::createFileMenu(
      []() { LoadFile(); },
      []() { LoadFile(); },
      []() { SaveFile(); },
      []() { [NSApp terminate:nil] ; }
      );

    Menu searchMenu("Search");
    MenuItem findReplaceItem("Find and Replace...", MenuItemType::Normal);
    findReplaceItem.shortcut = "Ctrl+F";
    findReplaceItem.callback = []() {
      SearchDialogs::ShowFindReplaceDialog((void*)g_display, darkMode,
        [](const std::string& find, const std::string& replace) {
          fprintf(stderr, "Find: %s, Replace: %s\n", find.c_str(), replace.c_str());
        });
      };
    searchMenu.items.push_back(findReplaceItem);

    MenuItem goToItem("Go To...", MenuItemType::Normal);
    goToItem.shortcut = "Ctrl+G";
    goToItem.callback = []() {
      SearchDialogs::ShowGoToDialog((void*)g_display, darkMode,
        [](int offset) {
          fprintf(stderr, "Go to offset: 0x%x\n", offset);
        });
      };
    searchMenu.items.push_back(goToItem);

    Menu helpMenu("Help");
    MenuItem aboutItem("About HexViewer", MenuItemType::Normal);
    aboutItem.callback = []() {
      AboutDialog::Show(g_nsWindow, darkMode);  // Change this!
      };
    helpMenu.items.push_back(aboutItem);

    menuBar->addMenu(fileMenu);
    menuBar->addMenu(searchMenu);
    menuBar->addMenu(helpMenu);

    menuBar->setPosition(0, 0);
    menuBar->setHeight(24);

    if (argc > 1) {
      LoadFileFromPath(argv[1]);
    }

    [NSApp run];

    if (menuBar) {
      delete menuBar;
      menuBar = nullptr;
    }
    delete renderManager;
    delete hexData;

    return 0;
  }
}

#else
void LoadFile() {
  FILE* fp = popen("zenity --file-selection", "r");
  if (fp) {
    char filepath[1024];
    if (fgets(filepath, sizeof(filepath), fp)) {
      filepath[strcspn(filepath, "\n")] = 0;
      LoadFileFromPath(filepath);
    }
    pclose(fp);
  }
}

void SaveFile() {
  if (hexData->isEmpty()) return;

  if (currentFilePath.empty()) {
    SaveFileAs();
    return;
  }

  hexData->saveFile(currentFilePath.c_str());
}

void SaveFileAs() {
  if (hexData->isEmpty()) return;

  FILE* fp = popen("zenity --file-selection --save", "r");
  if (fp) {
    char filepath[1024];
    if (fgets(filepath, sizeof(filepath), fp)) {
      filepath[strcspn(filepath, "\n")] = 0;
      if (hexData->saveFile(filepath)) {
        currentFilePath = filepath;
      }
    }
    pclose(fp);
  }
}

int main(int argc, char** argv) {
  DetectNative();
  LoadOptionsFromFile(g_options);

  g_display = XOpenDisplay(nullptr);
  if (!g_display) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  int screen = DefaultScreen(g_display);
  g_window = XCreateSimpleWindow(g_display, RootWindow(g_display, screen),
    0, 0, 1024, 768, 1,
    BlackPixel(g_display, screen),
    WhitePixel(g_display, screen));

  XStoreName(g_display, g_window, "HexViewer");
  XSelectInput(g_display, g_window, ExposureMask | KeyPressMask | ButtonPressMask |
    ButtonReleaseMask | PointerMotionMask | StructureNotifyMask);

  wmDeleteMessage = XInternAtom(g_display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(g_display, g_window, &wmDeleteMessage, 1);

  XMapWindow(g_display, g_window);

  renderManager = new RenderManager();
  if (!renderManager->initialize(g_window)) {
    fprintf(stderr, "Failed to initialize render manager\n");
    return 1;
  }

  renderManager->resize(1024, 768);
  hexData = new HexData();

  menuBar = new MenuBar();

  Menu fileMenu = MenuHelper::createFileMenu(
    []() { LoadFile(); },
    []() { LoadFile(); },
    []() { SaveFile(); },
    []() {}
    );

  Menu searchMenu("Search");
  MenuItem findReplaceItem("Find and Replace...", MenuItemType::Normal);
  findReplaceItem.shortcut = "Ctrl+F";
  findReplaceItem.callback = []() {
    SearchDialogs::ShowFindReplaceDialog((void*)g_display, darkMode,  // Pass display, not window
      [](const std::string& find, const std::string& replace) {
        fprintf(stderr, "Find: %s, Replace: %s\n", find.c_str(), replace.c_str());
      });
    };
  searchMenu.items.push_back(findReplaceItem);

  MenuItem goToItem("Go To...", MenuItemType::Normal);
  goToItem.shortcut = "Ctrl+G";
  goToItem.callback = []() {
    SearchDialogs::ShowGoToDialog((void*)g_display, darkMode,  // Pass display, not window
      [](int offset) {
        fprintf(stderr, "Go to offset: 0x%x\n", offset);
      });
    };
  searchMenu.items.push_back(goToItem);

  Menu toolsMenu("Tools");
  MenuItem optionsItem("Options...", MenuItemType::Normal);
  optionsItem.callback = []() {
    OptionsDialog::Show((NativeWindow)g_window, g_options);
    };
  toolsMenu.items.push_back(optionsItem);

  Menu helpMenu("Help");
  MenuItem aboutItem("About HexViewer", MenuItemType::Normal);
  aboutItem.callback = []() {
    AboutDialog::Show(g_window, g_options.darkMode);
    };
  helpMenu.items.push_back(aboutItem);

  menuBar->addMenu(fileMenu);
  menuBar->addMenu(searchMenu);
  menuBar->addMenu(toolsMenu);
  menuBar->addMenu(helpMenu);

  menuBar->setPosition(0, 0);
  menuBar->setHeight(24);

  bool running = true;
  bool needsRedraw = true;

  while (running) {
    while (XPending(g_display)) {
      XEvent event;
      XNextEvent(g_display, &event);

      switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          needsRedraw = true;
        }
        break;

      case ConfigureNotify:
        if (renderManager) {
          renderManager->resize(event.xconfigure.width, event.xconfigure.height);
          UpdateScrollbar(event.xconfigure.width, event.xconfigure.height);
          needsRedraw = true;
        }
        break;

      case MotionNotify: {
        if (menuBar) {
          bool stateChanged = menuBar->handleMouseMove(event.xmotion.x, event.xmotion.y);
          if (stateChanged) {
            needsRedraw = true;
          }
        }

        bool wasScrollbarHovered = scrollbarHovered;
        scrollbarHovered = (maxScrollPos > 0 &&
          event.xmotion.x >= scrollbarRect.x &&
          event.xmotion.x <= scrollbarRect.x + scrollbarRect.width &&
          event.xmotion.y >= scrollbarRect.y &&
          event.xmotion.y <= scrollbarRect.y + scrollbarRect.height);

        if (wasScrollbarHovered != scrollbarHovered) {
          needsRedraw = true;
        }
        break;
      }

      case KeyPress: {
        KeySym key = XLookupKeysym(&event.xkey, 0);

        if (key == XK_q || key == XK_Escape) {
          running = false;
        }
        else if (key == XK_o && (event.xkey.state & ControlMask)) {
          LoadFile();
        }
        else if (key == XK_s && (event.xkey.state & ControlMask)) {
          SaveFile();
        }
        break;
      }

      case ButtonPress:
        if (event.xbutton.button == Button1) {
          if (menuBar && menuBar->handleMouseDown(event.xbutton.x, event.xbutton.y)) {
            needsRedraw = true;
            break;
          }

          XWindowAttributes attrs;
          XGetWindowAttributes(g_display, g_window, &attrs);
          int row, col;
          size_t offset = GetByteOffsetFromClick(event.xbutton.x, event.xbutton.y,
            &row, &col, attrs.width, attrs.height);
          if (offset != (size_t)-1) {
            editingOffset = offset;
            editingRow = row;
            editingCol = col;
            editBuffer.clear();
            needsRedraw = true;
          }
        }
        else if (event.xbutton.button == Button4) {
          scrollPos = std::max(0, scrollPos - 3);
          XWindowAttributes attrs;
          XGetWindowAttributes(g_display, g_window, &attrs);
          UpdateScrollbar(attrs.width, attrs.height);
          needsRedraw = true;
        }
        else if (event.xbutton.button == Button5) {
          scrollPos = std::min(maxScrollPos, scrollPos + 3);
          XWindowAttributes attrs;
          XGetWindowAttributes(g_display, g_window, &attrs);
          UpdateScrollbar(attrs.width, attrs.height);
          needsRedraw = true;
        }
        break;

      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteMessage) {
          running = false;
        }
        break;
      }
    }

    if (needsRedraw && renderManager && hexData) {
      renderManager->renderHexViewer(
        hexData->getHexLines(),
        hexData->getHeaderLine(),
        scrollPos,
        maxScrollPos,
        scrollbarHovered,
        scrollbarPressed,
        scrollbarRect,
        thumbRect,
        darkMode,
        editingRow,
        editingCol,
        editBuffer);

      if (menuBar) {
        XWindowAttributes attrs;
        XGetWindowAttributes(g_display, g_window, &attrs);
        menuBar->render(renderManager, attrs.width);
      }

      renderManager->endFrame();
      needsRedraw = false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  if (menuBar) {
    delete menuBar;
    menuBar = nullptr;
  }
  delete renderManager;
  delete hexData;
  XDestroyWindow(g_display, g_window);
  XCloseDisplay(g_display);

  return 0;
}
#endif

void LoadFileFromPath(const char* filepath) {
  if (hexData->loadFile(filepath)) {
    currentFilePath = filepath;
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring wpath = conv.from_bytes(filepath);
    currentFilePathW = wpath;
    std::wstring title = L"HexViewer - " + currentFilePathW;
    SetWindowTextW(g_hWnd, title.c_str());
#endif
    scrollPos = 0;
#ifdef _WIN32
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(g_hWnd, NULL, TRUE);
#else
    XWindowAttributes attrs;
    XGetWindowAttributes(g_display, g_window, &attrs);
    UpdateScrollbar(attrs.width, attrs.height);
#endif
  }
}

int CalculateBytesPerLine(int windowWidth) {
  LayoutMetrics layout;
  int availableWidth = windowWidth - (int)(layout.margin * 2) - (int)layout.scrollbarWidth;
  float charWidth = 9.6f;
  int availableChars = (int)(availableWidth / charWidth);
  int bytesPerLine = (availableChars - 30) / 4;
  bytesPerLine = std::max(8, std::min(16, bytesPerLine));
  return (bytesPerLine > 12) ? 16 : 8;
}

void UpdateScrollbar(int windowWidth, int windowHeight) {
  LayoutMetrics layout;

  int menuBarHeight = menuBar ? menuBar->getHeight() : 0;

  int bytesPerLine = CalculateBytesPerLine(windowWidth);
  static int lastBytesPerLine = 16;

  if (bytesPerLine != lastBytesPerLine && !hexData->isEmpty()) {
    hexData->regenerateHexLines(bytesPerLine);
    lastBytesPerLine = bytesPerLine;
    scrollPos = 0;
  }

  float logRectTop = menuBarHeight + layout.margin + layout.headerHeight + 2.0f;
  float logRectBottom = windowHeight - layout.margin;

  const auto& hexLines = hexData->getHexLines();
  size_t maxVisibleLines = (size_t)((logRectBottom - logRectTop) / layout.lineHeight);
  maxScrollPos = (int)hexLines.size() > (int)maxVisibleLines ?
    (int)hexLines.size() - (int)maxVisibleLines : 0;

  if (scrollPos > maxScrollPos)
    scrollPos = maxScrollPos;

  scrollbarRect = Rect(
    windowWidth - (int)layout.margin - (int)layout.scrollbarWidth,
    menuBarHeight + (int)(layout.margin + layout.headerHeight), // ADD menuBarHeight
    (int)layout.scrollbarWidth,
    windowHeight - (int)layout.margin - menuBarHeight - (int)(layout.margin + layout.headerHeight)
    );

  if (maxScrollPos > 0 && hexLines.size() > 0) {
    float scrollbarHeight = scrollbarRect.height;
    int thumbHeight = (int)(scrollbarHeight * maxVisibleLines / hexLines.size());
    thumbHeight = std::max(thumbHeight, 30);
    int thumbTop = scrollbarRect.y + (int)((scrollbarHeight - thumbHeight) * scrollPos / maxScrollPos);

    thumbRect = Rect(scrollbarRect.x, thumbTop, scrollbarRect.width, thumbHeight);
  }
  else {
    thumbRect = scrollbarRect;
  }
}

size_t GetByteOffsetFromClick(int x, int y, int* outRow, int* outCol, int windowWidth, int windowHeight) {
  LayoutMetrics layout;

  int menuBarHeight = menuBar ? menuBar->getHeight() : 0;

  float charWidth = 9.6f;
  float offsetLabelWidth = 10 * charWidth;
  float hexStartX = layout.margin + offsetLabelWidth;

  float hexStartY = menuBarHeight + layout.margin + layout.headerHeight + 2.0f;

  if (y < hexStartY) return (size_t)-1;

  int lineIndex = (int)((y - hexStartY) / layout.lineHeight) + scrollPos;
  if (lineIndex < 0 || lineIndex >= (int)hexData->getHexLines().size()) {
    return (size_t)-1;
  }

  int bytesPerLine = hexData->getCurrentBytesPerLine();
  float hexAreaWidth = bytesPerLine * 3 * charWidth;
  float hexEndX = hexStartX + hexAreaWidth;

  if (x < hexStartX || x > hexEndX) return (size_t)-1;

  float relativeX = x - hexStartX;
  int byteInLine = (int)(relativeX / (charWidth * 3));

  if (byteInLine < 0 || byteInLine >= bytesPerLine) return (size_t)-1;

  size_t offset = (lineIndex * bytesPerLine) + byteInLine;
  if (offset >= hexData->getFileSize()) return (size_t)-1;

  if (outRow) *outRow = lineIndex - scrollPos;
  if (outCol) *outCol = byteInLine;

  return offset;
}
