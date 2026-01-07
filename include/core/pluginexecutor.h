#ifndef PLUGINEXECUTOR_H
#define PLUGINEXECUTOR_H

#include "global.h"
#include "hexdata.h"
#include "plugintypes.h"

bool InitializePythonRuntime();
void ShutdownPythonRuntime();

bool CanPluginDisassemble(const char* pluginPath);
bool CanPluginAnalyze(const char* pluginPath);
bool CanPluginTransform(const char* pluginPath);
void GetPluginDirectory(char *outPath, int maxLen);
bool GetPythonPluginInfo(const char *pluginPath, PluginInfo *info);
static void ExtractModuleName(const char *pluginPath, char *moduleName, int maxLen);

bool ExecutePythonDisassembly(
    const char* pluginPath,
    const uint8_t* data,
    size_t dataSize,
    size_t offset,
    LineArray* outLines);

#endif
