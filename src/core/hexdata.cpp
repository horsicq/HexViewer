#include "hexdata.h"
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <stdlib.h>
    #include <string.h>
#endif


static bool read_file_all(const char* path, ByteBuffer* outBuffer) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER liSize;
    if (!GetFileSizeEx(hFile, &liSize)) {
        CloseHandle(hFile);
        return false;
    }

    if (liSize.QuadPart < 0 || liSize.QuadPart > 0x7FFFFFFFLL) {
        CloseHandle(hFile);
        return false;
    }

    size_t size = (size_t)liSize.QuadPart;
    if (!bb_resize(outBuffer, size)) {
        CloseHandle(hFile);
        return false;
    }

    DWORD readBytes = 0;
    if (!ReadFile(hFile, outBuffer->data, (DWORD)size, &readBytes, NULL) ||
        readBytes != size) {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);
    return true;
#else
    int fd = open(path, O_RDONLY);
    if (fd < 0) return false;

    struct stat st;
    if (fstat(fd, &st) != 0) {
        close(fd);
        return false;
    }

    if (st.st_size < 0) {
        close(fd);
        return false;
    }

    size_t size = (size_t)st.st_size;
    if (!bb_resize(outBuffer, size)) {
        close(fd);
        return false;
    }

    size_t totalRead = 0;
    while (totalRead < size) {
        ssize_t r = read(fd, outBuffer->data + totalRead, size - totalRead);
        if (r <= 0) {
            close(fd);
            return false;
        }
        totalRead += (size_t)r;
    }

    close(fd);
    return true;
#endif
}

static bool write_file_all(const char* path, const uint8_t* data, size_t size) {
#ifdef _WIN32
    HANDLE hFile = CreateFileA(path,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    if (!WriteFile(hFile, data, (DWORD)size, &written, NULL) ||
        written != size) {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);
    return true;
#else
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return false;

    size_t totalWritten = 0;
    while (totalWritten < size) {
        ssize_t w = write(fd, data + totalWritten, size - totalWritten);
        if (w <= 0) {
            close(fd);
            return false;
        }
        totalWritten += (size_t)w;
    }

    close(fd);
    return true;
#endif
}


static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}


HexData::HexData()
    : currentBytesPerLine(16),
      modified(false),
      capstoneInitialized(false),
      currentArch(0),
      currentMode(0),
      csHandle(0),
      usePlugin(false) {
    bb_init(&fileData);
    la_init(&hexLines);
    la_init(&disassemblyLines);
    ss_init(&headerLine);
    pluginPath[0] = '\0';
}

HexData::~HexData() {
    clear();
    bb_free(&fileData);
    la_free(&hexLines);
    la_free(&disassemblyLines);
    ss_free(&headerLine);
}

void HexData::setDisassemblyPlugin(const char* path) {
    if (!path) return;
    
    extern bool CanPluginDisassemble(const char* pluginPath);
    if (!CanPluginDisassemble(path)) {
        return;
    }
    
    int i = 0;
    while (path[i] && i < 511) {
        pluginPath[i] = path[i];
        i++;
    }
    pluginPath[i] = '\0';
    
    usePlugin = true;
    
    if (!bb_empty(&fileData)) {
        convertDataToHex(currentBytesPerLine);
    }
}

void HexData::clearDisassemblyPlugin() {
    usePlugin = false;
    pluginPath[0] = '\0';
    
    if (!bb_empty(&fileData)) {
        convertDataToHex(currentBytesPerLine);
    }
}

bool HexData::hasDisassemblyPlugin() const {
    return usePlugin && pluginPath[0] != '\0';
}

void HexData::generateDisassemblyFromPlugin(int bytesPerLine) {
    la_clear(&disassemblyLines);
    
    if (bb_empty(&fileData) || !usePlugin) {
        #ifdef _WIN32
        MessageBoxA(NULL, "generateDisassemblyFromPlugin: No data or no plugin!", "Debug", MB_OK);
        #endif
        return;
    }
    
    #ifdef _WIN32
    MessageBoxA(NULL, pluginPath, "Debug - Plugin Path", MB_OK);
    #endif
    
    extern bool ExecutePythonDisassembly(
        const char* pluginPath,
        const uint8_t* data,
        size_t dataSize,
        size_t offset,
        LineArray* outLines);
    
    int linesGenerated = 0;
    for (size_t i = 0; i < fileData.size; i += (size_t)bytesPerLine) {
        SimpleString line;
        ss_init(&line);
        
        size_t remaining = fileData.size - i;
        size_t chunkSize = remaining < (size_t)bytesPerLine ? remaining : (size_t)bytesPerLine;
        
        LineArray tempLines;
        la_init(&tempLines);
        
        if (ExecutePythonDisassembly(
            pluginPath,
            fileData.data + i,
            chunkSize,
            i,
            &tempLines)) {
            
            if (tempLines.count > 0 && tempLines.lines[0].length > 0) {
                ss_append_cstr(&line, tempLines.lines[0].data);
                linesGenerated++;
            }
        }
        
        la_free(&tempLines);
        
        la_push_back(&disassemblyLines, &line);
        ss_free(&line);
    }
    
    #ifdef _WIN32
    if (linesGenerated > 0) {
        MessageBoxA(NULL, "Disassembly lines generated successfully!", "Debug - Success", MB_OK);
    } else {
        MessageBoxA(NULL, "No disassembly lines were generated!", "Debug - Warning", MB_OK);
    }
    #endif
}

void HexData::generateDisassembly(int bytesPerLine) {
    if (usePlugin) {
        generateDisassemblyFromPlugin(bytesPerLine);
        return;
    }
    
    la_clear(&disassemblyLines);
}

bool HexData::initializeCapstone() {
    return false;
}

void HexData::cleanupCapstone() {
}

void HexData::setArchitecture(int /*arch*/, int /*mode*/) {
}

void HexData::disassembleInstruction(size_t /*offset*/,
                                     int& instructionLength,
                                     SimpleString& outInstr) {
    ss_clear(&outInstr);
    instructionLength = 1;
}

bool HexData::loadFile(const char* filepath) {
    if (!read_file_all(filepath, &fileData)) {
        la_clear(&hexLines);
        la_push_back_cstr(&hexLines, "Error: Failed to open or read file");
        return false;
    }

    convertDataToHex(16);
    modified = false;
    return true;
}

bool HexData::saveFile(const char* filepath) {
    if (!write_file_all(filepath, fileData.data, fileData.size)) {
        return false;
    }
    modified = false;
    return true;
}

bool HexData::editByte(size_t offset, uint8_t newValue) {
    if (offset >= fileData.size) return false;
    fileData.data[offset] = newValue;
    modified = true;
    regenerateHexLines(currentBytesPerLine);
    return true;
}

uint8_t HexData::getByte(size_t offset) const {
    if (offset >= fileData.size) return 0;
    return fileData.data[offset];
}

void HexData::clear() {
    bb_resize(&fileData, 0);
    la_clear(&hexLines);
    la_clear(&disassemblyLines);
    ss_clear(&headerLine);
    modified = false;
}

void HexData::regenerateHexLines(int bytesPerLine) {
    if (!bb_empty(&fileData)) {
        convertDataToHex(bytesPerLine);
    }
}

void HexData::generateHeader(int bytesPerLine) {
    ss_clear(&headerLine);
    ss_append_cstr(&headerLine, "Offset    ");
    for (int i = 0; i < bytesPerLine; ++i) {
        ss_append_dec2(&headerLine, (unsigned int)i);
        ss_append_char(&headerLine, ' ');
    }
    ss_append_char(&headerLine, ' ');
    ss_append_cstr(&headerLine, "Decoded text");
}

void HexData::convertDataToHex(int bytesPerLine) {
    la_clear(&hexLines);

    if (bb_empty(&fileData)) {
        la_push_back_cstr(&hexLines, "No data to display");
        ss_clear(&headerLine);
        return;
    }

    bytesPerLine = clamp_int(bytesPerLine, 8, 48);
    currentBytesPerLine = bytesPerLine;

    generateHeader(bytesPerLine);
    generateDisassembly(bytesPerLine);

    for (size_t i = 0; i < fileData.size; i += (size_t)bytesPerLine) {
        SimpleString line;
        ss_init(&line);

        ss_append_hex8(&line, (unsigned int)i);
        ss_append_cstr(&line, "  ");

        for (int j = 0; j < bytesPerLine; ++j) {
            size_t idx = i + (size_t)j;
            if (idx < fileData.size) {
                unsigned int v = fileData.data[idx];
                ss_append_hex2(&line, v);
                ss_append_char(&line, ' ');
            } else {
                ss_append_cstr(&line, "   ");
            }
        }

        ss_append_char(&line, ' ');

        for (int j = 0; j < bytesPerLine; ++j) {
            size_t idx = i + (size_t)j;
            if (idx >= fileData.size) break;
            
            uint8_t b = fileData.data[idx];
            
            if (b >= 32 && b != 127) {
                ss_append_char(&line, (char)b);
            } else {
                ss_append_char(&line, '.');
            }
        }

        ss_append_cstr(&line, "      ");

        la_push_back(&hexLines, &line);
        ss_free(&line);
    }
}
