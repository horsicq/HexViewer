#ifndef PLUGINEXECUTOR_H
#define PLUGINEXECUTOR_H

#include "global.h"
#include "plugintypes.h"

struct LineArray;
struct PluginInfo;

struct PluginBookmark {
  uint64_t offset;
  char label[128];
  char description[256];
  char pluginSource[128];
};

struct PluginBookmarkArray {
  PluginBookmark* bookmarks;
  size_t count;
  size_t capacity;
};

bool InitializePythonRuntime();
void ShutdownPythonRuntime();

bool CanPluginDisassemble(const char* pluginPath);
bool CanPluginAnalyze(const char* pluginPath);
bool CanPluginTransform(const char* pluginPath);
void GetPluginDirectory(char* outPath, int maxLen);
bool GetPythonPluginInfo(const char* pluginPath, PluginInfo* info);
static void ExtractModuleName(const char* pluginPath, char* moduleName, int maxLen);

void pba_init(PluginBookmarkArray* arr);
void pba_push_back(PluginBookmarkArray* arr, const PluginBookmark* bookmark);
void pba_free(PluginBookmarkArray* arr);
bool CanPluginGenerateBookmarks(const char* pluginPath);

bool ExecutePluginBookmarks(
  const char* pluginPath,
  const uint8_t* data,
  size_t dataSize,
  PluginBookmarkArray* outBookmarks);

bool ExecutePythonDisassembly(
  const char* pluginPath,
  const uint8_t* data,
  size_t dataSize,
  size_t offset,
  LineArray* outLines);

#endif
