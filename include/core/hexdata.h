#ifndef HEXDATA_H
#define HEXDATA_H
#include <stdint.h>
#include <stddef.h>

#include "global.h"
#include "pluginexecutor.h"
#include "options.h"

#define MAX_PLUGINS 10

struct MemoryRegion
{
  uint64_t virtualAddress;
  size_t bufferOffset;
  size_t size;
};

class HexData
{
public:
  struct DisasmCache
  {
    size_t startOffset;
    size_t endOffset;
    bool valid;
  };
  Vector<DisasmCache> disasmRanges;
  Vector<MemoryRegion> memoryMap;
  HexData();
  ~HexData();

  ByteBuffer fileData;
  bool virtualAddressToOffset(uint64_t virtualAddress, size_t* outOffset) const;
  bool loadFile(const char* filepath);
  bool saveFile(const char* filepath);
  void clear();

  bool isRangeDisassembled(size_t startOffset, size_t endOffset);
  void disassembleRange(size_t offset, size_t size);
  void clearDisassemblyCache();

  const LineArray& getHexLines() const { return hexLines; }
  const LineArray& getDisassemblyLines() const { return disassemblyLines; }
  const SimpleString& getHeaderLine() const { return headerLine; }

  size_t getFileSize() const { return fileData.size; }
  bool isEmpty() const { return fileData.size == 0; }
  int getCurrentBytesPerLine() const { return currentBytesPerLine; }

  bool editByte(size_t offset, uint8_t newValue);
  uint8_t getByte(size_t offset) const;
  uint8_t readByte(size_t offset) const { return getByte(offset); }

  bool isModified() const { return modified; }
  void setModified(bool mod) { modified = mod; }

  void regenerateHexLines(int bytesPerLine);
  bool isProcessMemory;

  void setMemoryMap(const Vector<MemoryRegion>& map) {
    memoryMap = map;
    isProcessMemory = true;
  }

  const Vector<MemoryRegion>& getMemoryMap() const {
    return memoryMap;
  }

  bool isProcessMemoryDump() const {
    return isProcessMemory;
  }

  void clearMemoryMap() {
    memoryMap.clear();
    isProcessMemory = false;
  }
  void addPlugin(const char* pluginPath);
  void clearAllPlugins();
  bool hasPlugins() const;
  int getPluginCount() const { return pluginCount; }
  const char* getPluginPath(int index) const;

  void setDisassemblyPlugin(const char* pluginPath);
  void clearDisassemblyPlugin();
  bool hasDisassemblyPlugin() const;

  void generateDisassemblyFromPlugin(int bytesPerLine);
  void setArchitecture(int arch, int mode);

  PluginBookmarkArray* getPluginAnnotations() { return &pluginAnnotations; }
  const PluginBookmarkArray* getPluginAnnotations() const { return &pluginAnnotations; }
  void clearPluginAnnotations();
  void executeBookmarkPlugins();
  void convertDataToHex(int bytesPerLine);

private:
  char pluginPaths[MAX_PLUGINS][512];
  int pluginCount;
  bool usePlugins;

  char pluginPath[512];
  bool usePlugin;

  void generateHeader(int bytesPerLine);
  void generateDisassembly(int bytesPerLine);
  void disassembleInstruction(size_t offset, int& instructionLength, SimpleString& outInstr);
  bool initializeCapstone();
  void cleanupCapstone();

private:
  LineArray hexLines;
  LineArray disassemblyLines;
  SimpleString headerLine;
  int currentBytesPerLine;
  bool modified;
  bool capstoneInitialized;
  int currentArch;
  int currentMode;
  size_t csHandle;
  PluginBookmarkArray pluginAnnotations;
};
#endif
