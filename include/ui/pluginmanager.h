#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "render.h"
#include "plugintypes.h"
#include "pluginexecutor.h"
#include "options.h"

struct PluginManagerData {
    NativeWindow window;
    RenderManager* renderer;
    Vector<PluginInfo*> plugins;
    bool running;
    bool dialogResult;
    int hoveredWidget;
    int pressedWidget;
    int hoveredPlugin;
    int selectedPlugin;
    int scrollOffset;
    
    int mouseX;
    int mouseY;
    bool mouseDown;
    
#ifdef _WIN32
    HDC hdc;
#elif __linux__
    Display* display;
    Atom wmDeleteWindow;
#endif
};

class PluginManager {
public:

    static bool Show(NativeWindow parent);
    
    static void LoadPluginsFromDirectory(PluginManagerData* data);
    static void ScanDirectory(const char* path, PluginManagerData* data);
    static bool ParsePluginManifest(const char* manifestPath, PluginInfo* info);
    static void SavePluginStates(PluginManagerData* data);
    static void LoadPluginStates(PluginManagerData* data);
};

void RenderPluginManager(PluginManagerData* data, int windowWidth, int windowHeight);
void UpdatePluginHoverState(PluginManagerData* data, int x, int y, int windowWidth, int windowHeight);
void HandlePluginClick(PluginManagerData* data, int x, int y, int windowWidth, int windowHeight);
bool CheckPluginCapabilities(const char* pluginPath, PluginInfo* info);


#endif
