#ifndef DARKMODE_H
#define DARKMODE_H

#include <cstddef>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#endif

#ifdef __APPLE__
#include <AppKit/AppKit.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <cstring>
#endif

#ifdef _WIN32

inline void ApplyDarkTitleBar(HWND hWnd, bool dark)
{
  BOOL value = dark ? TRUE : FALSE;
  DwmSetWindowAttribute(hWnd, 19, &value, sizeof(value));
  DwmSetWindowAttribute(hWnd, 20, &value, sizeof(value));
}

inline void ApplyDarkMenu(HMENU hMenu, bool dark)
{
  MENUINFO mi = { sizeof(mi) };
  mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
  mi.hbrBack = dark ? CreateSolidBrush(RGB(32, 32, 32))
    : (HBRUSH)(COLOR_MENU + 1);
  SetMenuInfo(hMenu, &mi);
}

#elif defined(__APPLE__)

inline void ApplyDarkTitleBar(NSWindow* window, bool dark)
{
  if (!window) return;
  NSAppearanceName name = dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua;
  [window setAppearance : [NSAppearance appearanceNamed : name] ] ;
  [window setTitleVisibility : NSWindowTitleVisible] ;
  [window setTitlebarAppearsTransparent : NO] ;
}

inline void ApplyDarkMenu(NSMenu* menu, bool dark)
{
  if (!menu) return;
  NSAppearanceName name = dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua;
  NSAppearance* appearance = [NSAppearance appearanceNamed : name];
  [NSApp setAppearance : appearance] ;
  if ([menu respondsToSelector : @selector(setAppearance:)]) {
    [menu setAppearance : appearance] ;
  }
}

#elif defined(__linux__)

inline void ApplyDarkTitleBar(Display* dpy, Window win, bool dark)
{
  Atom atom = XInternAtom(dpy, "_GTK_THEME_VARIANT", False);
  if (atom == None) return;

  if (dark) {
    const char* value = "dark";
    XChangeProperty(dpy, win, atom, XA_STRING, 8,
      PropModeReplace,
      (unsigned char*)value,
      (int)strlen(value) + 1);
  }
  else {
    XDeleteProperty(dpy, win, atom);
  }
}

inline void ApplyDarkMenu(void*, bool) {}

#endif

#endif
