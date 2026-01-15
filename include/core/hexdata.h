#ifndef HEXDATA_H
#define HEXDATA_H

#include <stdint.h>
#include <stddef.h>
#include "global.h"
#include "pluginexecutor.h"
#include "options.h"

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

    HexData();
    ~HexData();

    bool loadFile(const char *filepath);
    bool saveFile(const char *filepath);
    void clear();

    bool isRangeDisassembled(size_t startOffset, size_t endOffset);
    void disassembleRange(size_t offset, size_t size);
    void clearDisassemblyCache();

    const LineArray &getHexLines() const { return hexLines; }
    const LineArray &getDisassemblyLines() const { return disassemblyLines; }
    const SimpleString &getHeaderLine() const { return headerLine; }

    size_t getFileSize() const { return fileData.size; }
    bool isEmpty() const { return fileData.size == 0; }
    int getCurrentBytesPerLine() const { return currentBytesPerLine; }

    bool editByte(size_t offset, uint8_t newValue);
    uint8_t getByte(size_t offset) const;
    uint8_t readByte(size_t offset) const { return getByte(offset); }

    bool isModified() const { return modified; }
    void setModified(bool mod) { modified = mod; }

    void regenerateHexLines(int bytesPerLine);

    void setDisassemblyPlugin(const char *pluginPath);
    void clearDisassemblyPlugin();
    bool hasDisassemblyPlugin() const;
    void generateDisassemblyFromPlugin(int bytesPerLine);

    void setArchitecture(int arch, int mode);

    PluginBookmarkArray* getPluginAnnotations() { return &pluginAnnotations; }
    const PluginBookmarkArray* getPluginAnnotations() const { return &pluginAnnotations; }
    void clearPluginAnnotations();
    void executeBookmarkPlugins();

private:
    char pluginPath[512];
    bool usePlugin;

    void convertDataToHex(int bytesPerLine);
    void generateHeader(int bytesPerLine);
    void generateDisassembly(int bytesPerLine);

    void disassembleInstruction(size_t offset, int &instructionLength, SimpleString &outInstr);
    bool initializeCapstone();
    void cleanupCapstone();

private:
    ByteBuffer fileData;
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
