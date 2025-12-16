#include <string>
#include <vector>
#include <cstring> 
#include <algorithm>
#include <thread>
#include <chrono>
#include <codecvt>

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
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif
#include <about.h>
#include <darkmode.h>
#include <render.h>
#include <options.h>
#include <HexData.h>
#include <searchdialog.h>
#include <menu.h>
#include <language.h>
#include <resource.h>
#include <notification.h>


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
long long cursorBytePos = -1;      
int cursorNibblePos = 0;         
long long selectionLength = 0;
bool caretVisible = true;
uintptr_t caretTimerId = 0;

#ifdef _WIN32
AppOptions g_options = { true, 16, false, false, "English" };
HWND g_hWnd = NULL;
std::wstring currentFilePathW;

#elif __APPLE__
AppOptions g_options = { true, 16, false };
NSWindow* g_nsWindow = nullptr;

#else  // Linux
AppOptions g_options = { true, 16, false,false,"English"};
Display* g_display = nullptr;
Window g_window = 0;
Atom wmDeleteMessage;
#endif

void LoadFile();
void LoadFileFromPath(const char* filepath);
void SaveFile();
void SaveFileAs();
void UpdateScrollbar(int windowWidth, int windowHeight);
size_t GetByteOffsetFromClick(int x, int y, int* outRow, int* outCol, int windowWidth, int windowHeight, int* outNibblePos);
int CalculateBytesPerLine(int windowWidth);

#ifdef _WIN32
void ShowOptionsDialog();
#endif

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
#ifdef _WIN32
  exitItem.callback = []() { PostQuitMessage(0); };
#elif __APPLE__
  exitItem.callback = []() { [NSApp terminate : nil] ; };
#else // Linux
  exitItem.callback = []() { /* handled by window close */ };
#endif
  fileMenu.items.push_back(exitItem);

  Menu searchMenu(Translations::T("Search"));
  MenuItem findReplaceItem(Translations::T("Find..."), MenuItemType::Normal);
  findReplaceItem.shortcut = "Ctrl+F";
  findReplaceItem.callback = []() {
#ifdef _WIN32
    SearchDialogs::ShowFindReplaceDialog(g_hWnd, g_options.darkMode,
      [](const std::string& find, const std::string& replace) {
        MessageBoxA(g_hWnd,
          ("Find: " + find + "\nReplace: " + replace).c_str(),
          "Find & Replace", MB_OK);
      });
#elif __APPLE__
    SearchDialogs::ShowFindReplaceDialog((void*)g_display, darkMode,
      [](const std::string& find, const std::string& replace) {
        fprintf(stderr, "Find: %s, Replace: %s\n", find.c_str(), replace.c_str());
      });
#else __linux__
    SearchDialogs::ShowFindReplaceDialog((void*)g_display, darkMode,
      [](const std::string& find, const std::string& replace) {
        fprintf(stderr, "Find: %s, Replace: %s\n", find.c_str(), replace.c_str());
      });
#endif
    };
  searchMenu.items.push_back(findReplaceItem);

  MenuItem goToItem(Translations::T("Go To..."), MenuItemType::Normal);
  goToItem.shortcut = "Ctrl+G";
  goToItem.callback = []() {
#ifdef _WIN32
    SearchDialogs::ShowGoToDialog(g_hWnd, g_options.darkMode,
      [](int line) {
        MessageBoxA(g_hWnd,
          ("Go to line: " + std::to_string(line)).c_str(),
          "Go To", MB_OK);
      });
#elif defined(__APPLE__) || defined(__linux__)
    SearchDialogs::ShowGoToDialog((void*)g_display, darkMode,
      [](int offset) {
        fprintf(stderr, "Go to offset: 0x%x\n", offset);
      });
#endif
    };
  searchMenu.items.push_back(goToItem);

  Menu toolsMenu(Translations::T("Tools"));
  MenuItem optionsItem(Translations::T("Options..."), MenuItemType::Normal);
  optionsItem.callback = []() {
    AppOptions oldOptions = g_options;

#ifdef _WIN32
    if (OptionsDialog::Show(g_hWnd, g_options)) {
      bool needsRedraw = false;

      if (oldOptions.darkMode != g_options.darkMode) {
        darkMode = g_options.darkMode;
        needsRedraw = true;
      }

      if (oldOptions.language != g_options.language) {
        Translations::SetLanguage(g_options.language);
        RebuildMenuBar();
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
#elif __APPLE__
    if (OptionsDialog::Show(g_nsWindow, g_options)) {
      bool needsRedraw = false;

      if (oldOptions.darkMode != g_options.darkMode) {
        darkMode = g_options.darkMode;
        needsRedraw = true;
      }

      if (oldOptions.language != g_options.language) {
        Translations::SetLanguage(g_options.language);
        RebuildMenuBar();
        needsRedraw = true;
      }

      if (oldOptions.defaultBytesPerLine != g_options.defaultBytesPerLine && !hexData->isEmpty()) {
        hexData->regenerateHexLines(g_options.defaultBytesPerLine);
        scrollPos = 0;
        needsRedraw = true;
      }

      SaveOptionsToFile(g_options);

      if (needsRedraw) {
      }
    }
#elif defined(__linux__)
    if (OptionsDialog::Show((NativeWindow)g_window, g_options)) {
      bool needsRedraw = false;

      if (oldOptions.darkMode != g_options.darkMode) {
        darkMode = g_options.darkMode;
        needsRedraw = true;
      }

      if (oldOptions.language != g_options.language) {
        Translations::SetLanguage(g_options.language);
        RebuildMenuBar();
        needsRedraw = true;
      }

      if (oldOptions.defaultBytesPerLine != g_options.defaultBytesPerLine && !hexData->isEmpty()) {
        hexData->regenerateHexLines(g_options.defaultBytesPerLine);
        scrollPos = 0;
        XWindowAttributes attrs;
        XGetWindowAttributes(g_display, g_window, &attrs);
        UpdateScrollbar(attrs.width, attrs.height);
        needsRedraw = true;
      }

      SaveOptionsToFile(g_options);

      if (needsRedraw) {
        XClearWindow(g_display, g_window);
        XEvent exposeEvent;
        memset(&exposeEvent, 0, sizeof(exposeEvent));
        exposeEvent.type = Expose;
        exposeEvent.xexpose.window = g_window;
        XSendEvent(g_display, g_window, False, ExposureMask, &exposeEvent);
        XFlush(g_display);
      }
    }
#endif
    };
  toolsMenu.items.push_back(optionsItem);

  Menu helpMenu(Translations::T("Help"));
  MenuItem aboutItem(Translations::T("About HexViewer"), MenuItemType::Normal);
  aboutItem.callback = []() {
#ifdef _WIN32
    AboutDialog::Show(g_hWnd, g_options.darkMode);
#elif __APPLE__
    AboutDialog::Show(g_nsWindow, darkMode);
#elif defined(__linux__)
    AboutDialog::Show(g_window, g_options.darkMode);
#endif
    };
  helpMenu.items.push_back(aboutItem);

  menuBar->addMenu(fileMenu);
  menuBar->addMenu(searchMenu);
  menuBar->addMenu(toolsMenu);
  menuBar->addMenu(helpMenu);

  menuBar->setPosition(0, 0);
  menuBar->setHeight(24);
}
#ifdef _WIN32
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
      RebuildMenuBar();
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
    caretTimerId = SetTimer(hWnd, 1, 500, NULL);
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
  case WM_TIMER: {
    if (wParam == 1 && cursorBytePos != -1) {
      caretVisible = !caretVisible;
      InvalidateRect(hWnd, NULL, FALSE);
    }
    break;
  }
  case WM_CHAR: {
    char ch = (char)wParam;

    if (editingOffset != (size_t)-1 && cursorBytePos != -1) {
      if (ch == VK_BACK) {
        if (cursorNibblePos == 1) {
          cursorNibblePos = 0;
        }
        else if (cursorBytePos > 0) {
          cursorBytePos--;
          editingOffset--;
          cursorNibblePos = 1;
          editingCol--;
          if (editingCol < 0) {
            editingCol = hexData->getCurrentBytesPerLine() - 1;
            editingRow--;
          }
        }
        editBuffer.clear();
        caretVisible = true;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }

      if (ch == VK_ESCAPE) {
        editingOffset = (size_t)-1;
        editBuffer.clear();
        editingRow = -1;
        editingCol = -1;
        cursorBytePos = -1;
        cursorNibblePos = 0;
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
      }

      if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
        if (ch >= 'a' && ch <= 'f') {
          ch = ch - 'a' + 'A';
        }

        uint8_t currentByte = 0;
        if (editingOffset < hexData->getFileSize()) {
          currentByte = hexData->readByte(editingOffset);
        }

        uint8_t newByte;

        if (cursorNibblePos == 0) {
          uint8_t newNibble = (ch >= 'A') ? (ch - 'A' + 10) : (ch - '0');
          newByte = (newNibble << 4) | (currentByte & 0x0F);

          hexData->editByte(editingOffset, newByte);
          cursorNibblePos = 1;
        }
        else {
          uint8_t newNibble = (ch >= 'A') ? (ch - 'A' + 10) : (ch - '0');
          newByte = (currentByte & 0xF0) | newNibble;

          hexData->editByte(editingOffset, newByte);

          if (editingOffset + 1 < hexData->getFileSize()) {
            editingOffset++;
            cursorBytePos++;
            cursorNibblePos = 0;
            editingCol++;

            if (editingCol >= hexData->getCurrentBytesPerLine()) {
              editingCol = 0;
              editingRow++;
            }
          }
        }

        editBuffer.clear();
        caretVisible = true;
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
      cursorBytePos = -1;
      cursorNibblePos = 0;
      InvalidateRect(hWnd, NULL, FALSE);
      return 0;
    }

    if (cursorBytePos != -1 && editingOffset != (size_t)-1) {
      bool moved = false;
      int bytesPerLine = hexData->getCurrentBytesPerLine();
      long long totalBytes = hexData->getFileSize();

      if (wParam == VK_RIGHT) {
        if (cursorNibblePos == 0) {
          cursorNibblePos = 1;
          moved = true;
        }
        else if (cursorBytePos + 1 < totalBytes) {
          cursorBytePos++;
          editingOffset++;
          cursorNibblePos = 0;
          editingCol++;
          if (editingCol >= bytesPerLine) {
            editingCol = 0;
            editingRow++;
          }
          moved = true;
        }
      }
      else if (wParam == VK_LEFT) {
        if (cursorNibblePos == 1) {
          cursorNibblePos = 0;
          moved = true;
        }
        else if (cursorBytePos > 0) {
          cursorBytePos--;
          editingOffset--;
          cursorNibblePos = 1;
          editingCol--;
          if (editingCol < 0) {
            editingCol = bytesPerLine - 1;
            editingRow--;
          }
          moved = true;
        }
      }
      else if (wParam == VK_DOWN && cursorBytePos + bytesPerLine < totalBytes) {
        cursorBytePos += bytesPerLine;
        editingOffset += bytesPerLine;
        editingRow++;
        moved = true;
      }
      else if (wParam == VK_UP && cursorBytePos >= bytesPerLine) {
        cursorBytePos -= bytesPerLine;
        editingOffset -= bytesPerLine;
        editingRow--;
        moved = true;
      }
      else if (wParam == VK_HOME) {
        cursorBytePos = (cursorBytePos / bytesPerLine) * bytesPerLine;
        editingOffset = cursorBytePos;
        cursorNibblePos = 0;
        editingCol = 0;
        moved = true;
      }
      else if (wParam == VK_END) {
        long long lineStart =
          (cursorBytePos / bytesPerLine) * bytesPerLine;

        long long lineEnd = std::clamp(
          lineStart + bytesPerLine - 1,
          0LL,
          totalBytes - 1
        );

        cursorBytePos = lineEnd;
        editingOffset = lineEnd;
        cursorNibblePos = 1;
        editingCol = lineEnd - lineStart;
        moved = true;
      }

      if (moved) {
        editBuffer.clear();
        caretVisible = true;
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
        scrollPos = std::clamp(scrollPos, 0, maxScrollPos);

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

    int row, col, nibblePos = 0;
    size_t offset = GetByteOffsetFromClick(x, y, &row, &col,
      rc.right - rc.left,
      rc.bottom - rc.top,
      &nibblePos);

    if (offset != (size_t)-1) {
      cursorBytePos = offset;
      cursorNibblePos = nibblePos;
      editingOffset = offset;
      editingRow = row;
      editingCol = col;
      editBuffer.clear();
      caretVisible = true;

      InvalidateRect(hWnd, NULL, FALSE);
    }
    else {
      if (editingOffset != (size_t)-1) {
        editingOffset = (size_t)-1;
        editBuffer.clear();
        editingRow = -1;
        editingCol = -1;
        cursorBytePos = -1;
        cursorNibblePos = 0;
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
      RECT rc;
      GetClientRect(hWnd, &rc);

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
        editBuffer,
        cursorBytePos,          
        cursorNibblePos,         
        hexData->getFileSize()); 

      if (menuBar) {
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
    scrollPos = std::clamp(scrollPos, 0, maxScrollPos);

    RECT rc;
    GetClientRect(hWnd, &rc);
    UpdateScrollbar(rc.right - rc.left, rc.bottom - rc.top);
    InvalidateRect(hWnd, NULL, FALSE);
    break;
  }
  case WM_DESTROY:
    if (caretTimerId) {
      KillTimer(hWnd, caretTimerId);
      caretTimerId = 0;
    }
    SearchDialogs::CleanupDialogs();
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

int WINAPI WinMain(
  HINSTANCE hInstance,
  HINSTANCE,
  LPSTR,
  int nCmdShow)
{
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEXW);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
  wcex.lpfnWndProc = WndProc;
  wcex.hInstance = hInstance;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);


  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"HexViewerClass";

  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowW(
    L"HexViewerClass",
    L"HexViewer",
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, 1024, 768,
    nullptr, nullptr,
    hInstance,
    nullptr
  );

  if (!hWnd) return FALSE;
  ApplyDarkTitleBar(hWnd, g_options.darkMode);

  renderManager = new RenderManager();
  if (!renderManager->initialize(hWnd)) {
    MessageBoxW(hWnd, L"Failed to initialize render manager!", L"Error",
      MB_OK | MB_ICONERROR);
    return FALSE;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderManager->resize(rc.right - rc.left, rc.bottom - rc.top);

  hexData = new HexData();
  RebuildMenuBar();

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  std::thread([]() {
    std::this_thread::sleep_for(std::chrono::seconds(2));

    UpdateInfo info;
    info.currentVersion = "1.0.0";
    info.releaseApiUrl = "https://api.github.com/repos/horsicq/HexViewer/releases/latest";

    std::string response = HttpGet(info.releaseApiUrl);

    if (!response.empty()) {
      std::string releaseName = ExtractJsonValue(response, "name");

      size_t vPos = releaseName.find("v");
      if (vPos != std::string::npos) {
        size_t spacePos = releaseName.find(" ", vPos);
        if (spacePos != std::string::npos) {
          info.latestVersion = releaseName.substr(vPos + 1, spacePos - vPos - 1);
        }
        else {
          info.latestVersion = releaseName.substr(vPos + 1);
        }
      }
      else {
        info.latestVersion = ExtractJsonValue(response, "tag_name");
        if (!info.latestVersion.empty() && info.latestVersion[0] == 'v') {
          info.latestVersion = info.latestVersion.substr(1);
        }
      }

      info.updateAvailable = (info.latestVersion != info.currentVersion &&
        !info.latestVersion.empty());

      if (info.updateAvailable) {
        AppNotification::Show(
          "Update Available!",
          "HexViewer " + info.latestVersion + " is now available.",
          "HexViewer",
          AppNotification::NotificationIcon::Info,
          10000  // Show for 10 seconds
        );
      }
    }
    }).detach();

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
  scrollPos = std::clamp(scrollPos, 0, maxScrollPos);

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

    Translations::Initialize();

    DetectNative();
    LoadOptionsFromFile(g_options);

    Translations::SetLanguage(g_options.language);

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

    RebuildMenuBar();

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

#elif defined(__linux__)
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
  Translations::Initialize();

  DetectNative();
  LoadOptionsFromFile(g_options);

  Translations::SetLanguage(g_options.language);

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

  RebuildMenuBar();

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
          int row, col, nibblePos = 0; 
          size_t offset = GetByteOffsetFromClick(event.xbutton.x, event.xbutton.y,
            &row, &col,
            attrs.width, attrs.height,
            &nibblePos); 
          if (offset != (size_t)-1) {
            cursorBytePos = offset;        
            cursorNibblePos = nibblePos;    
            editingOffset = offset;
            editingRow = row;
            editingCol = col;
            editBuffer.clear();
            needsRedraw = true;
          }
        }
        else if (event.xbutton.button == Button4) {
          scrollPos = std::clamp(scrollPos - 3, 0, INT_MAX);
          XWindowAttributes attrs;
          XGetWindowAttributes(g_display, g_window, &attrs);
          UpdateScrollbar(attrs.width, attrs.height);
          needsRedraw = true;
        }
        else if (event.xbutton.button == Button5) {
          scrollPos = std::clamp(scrollPos + 3, 0, maxScrollPos);
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
        editBuffer,
        cursorBytePos,     
        cursorNibblePos,
        hexData->getFileSize());

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
  bytesPerLine = std::clamp(bytesPerLine, 8, 16);
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
    thumbHeight = std::clamp(thumbHeight, 30, thumbHeight);
    int thumbTop = scrollbarRect.y + (int)((scrollbarHeight - thumbHeight) * scrollPos / maxScrollPos);

    thumbRect = Rect(scrollbarRect.x, thumbTop, scrollbarRect.width, thumbHeight);
  }
  else {
    thumbRect = scrollbarRect;
  }
}

size_t GetByteOffsetFromClick(int x, int y, int* outRow, int* outCol, int windowWidth, int windowHeight, int* outNibblePos) {
  LayoutMetrics layout;

#ifdef _WIN32
  HDC hdc = GetDC(g_hWnd);
  if (hdc) {
    HFONT oldFont = (HFONT)SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
    SIZE textSize;
    GetTextExtentPoint32A(hdc, "0", 1, &textSize);
    layout.charWidth = (float)textSize.cx;
    layout.lineHeight = (float)textSize.cy;
    SelectObject(hdc, oldFont);
    ReleaseDC(g_hWnd, hdc);
  }
  else {
    layout.charWidth = 9.6f;
    layout.lineHeight = 20.0f;
  }
#if defined(__APPLE__) || defined(__linux__)
    layout.charWidth  = 9.6f;
    layout.lineHeight = 20.0f;
#endif
#endif

  int menuBarHeight = menuBar ? menuBar->getHeight() : 0;

  float charWidth = layout.charWidth;  // Use measured width
  float offsetLabelWidth = 10 * charWidth;  // "00000000: " = 10 chars
  float hexStartX = layout.margin + offsetLabelWidth;

  float hexStartY = menuBarHeight + layout.margin + layout.headerHeight + 2.0f;

#ifdef _WIN32
  char debugMsg[256];
  snprintf(debugMsg, sizeof(debugMsg),
    "Click at (%d, %d), charWidth=%.1f, hexStartX=%.1f, hexStartY=%.1f\n",
    x, y, charWidth, hexStartX, hexStartY);
  OutputDebugStringA(debugMsg);
#endif

  if (y < hexStartY) return (size_t)-1;

  int lineIndex = (int)((y - hexStartY) / layout.lineHeight) + scrollPos;
  if (lineIndex < 0 || lineIndex >= (int)hexData->getHexLines().size()) {
    return (size_t)-1;
  }

  int bytesPerLine = hexData->getCurrentBytesPerLine();
  float hexAreaWidth = bytesPerLine * 3 * charWidth;  // Each byte = "XX " = 3 chars
  float hexEndX = hexStartX + hexAreaWidth;

#ifdef _WIN32
  snprintf(debugMsg, sizeof(debugMsg),
    "hexStartX=%.1f, hexEndX=%.1f, x=%d\n",
    hexStartX, hexEndX, x);
  OutputDebugStringA(debugMsg);
#endif

  if (x < hexStartX || x > hexEndX) return (size_t)-1;

  float relativeX = x - hexStartX;

  float byteGroupX = relativeX / (charWidth * 3);
  int byteInLine = (int)byteGroupX;

  float charInByte = (byteGroupX - byteInLine) * 3;  // 0-3 range
  int nibblePos = (charInByte < 1.0f) ? 0 : 1;

#ifdef _WIN32
  snprintf(debugMsg, sizeof(debugMsg),
    "relativeX=%.1f, byteInLine=%d, charInByte=%.2f, nibblePos=%d\n",
    relativeX, byteInLine, charInByte, nibblePos);
  OutputDebugStringA(debugMsg);
#endif

  if (byteInLine < 0 || byteInLine >= bytesPerLine) return (size_t)-1;

  size_t offset = (lineIndex * bytesPerLine) + byteInLine;
  if (offset >= hexData->getFileSize()) return (size_t)-1;

  if (outRow) *outRow = lineIndex - scrollPos;
  if (outCol) *outCol = byteInLine;
  if (outNibblePos) *outNibblePos = nibblePos;

#ifdef _WIN32
  snprintf(debugMsg, sizeof(debugMsg),
    "Final: offset=%zu, row=%d, col=%d, nibble=%d\n",
    offset, lineIndex - scrollPos, byteInLine, nibblePos);
  OutputDebugStringA(debugMsg);
#endif

  return offset;
}
