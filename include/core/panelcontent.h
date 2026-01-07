#pragma once
#include "render.h"
#include "menu.h"
#include "options.h"

struct PatternSearchState
{
    char searchPattern[256];
    int lastMatch;
    bool hasFocus;
};

struct ChecksumState
{
    bool md5;
    bool sha1;
    bool sha256;
    bool crc32;
    bool entireFile;
};

struct CompareState
{
    char filePath[512];
    bool fileLoaded;
};

struct DataInspectorValues {
    long long byteOffset;
    
    uint8_t uint8Val;
    int8_t int8Val;
    uint16_t uint16LE;
    uint16_t uint16BE;
    int16_t int16LE;
    int16_t int16BE;
    uint32_t uint32LE;
    uint32_t uint32BE;
    int32_t int32LE;
    int32_t int32BE;
    uint64_t uint64LE;
    uint64_t uint64BE;
    int64_t int64LE;
    int64_t int64BE;
    
    float floatLE;
    float floatBE;
    double doubleLE;
    double doubleBE;
    
    char asciiChar;
    bool isASCIIPrintable;
    char utf8Str[8];
    
    char binaryStr[9];
    
    bool hasData;
};

struct FileInfoValues {
    long long fileSize;
    char fileSizeFormatted[32];
    char fileType[64];
    char createdDate[32];
    char modifiedDate[32];
    char md5Hash[33];
    bool md5Computed;
};

struct Bookmark {
    long long byteOffset;
    char name[64];
    Color color;
    char description[128];
};

struct BookmarksState {
    Vector<Bookmark> bookmarks;
    int selectedIndex;
};

struct ByteStatistics {
    int histogram[256];
    int mostCommonByte;
    int mostCommonCount;
    int leastCommonByte;
    int leastCommonCount;
    int nullByteCount;
    double entropy;
    bool computed;
};

extern PatternSearchState g_PatternSearch;
extern ChecksumState      g_Checksum;
extern CompareState       g_Compare;
extern BookmarksState     g_Bookmarks;
extern ByteStatistics     g_ByteStats;

void PatternSearch_SetFocus();
void PatternSearch_Run();
void PatternSearch_FindNext();
void PatternSearch_FindPrev();
static int ParseHexPattern(const char* text, uint8_t* out, int maxOut);

void Checksum_ToggleMD5();
void Checksum_ToggleSHA1();
void Checksum_ToggleSHA256();
void Checksum_ToggleCRC32();
void Checksum_SetModeEntireFile();
void Checksum_SetModeSelection();
void Checksum_Compare();
void Checksum_Compute();

void Compare_OpenFileDialog();
void Compare_Run();

bool HandleBottomPanelContentClick(int x, int y, int windowWidth, int windowHeight);

void Bookmarks_Add(long long byteOffset, const char* name, Color color);
void Bookmarks_Remove(int index);
void Bookmarks_JumpTo(int index);
void Bookmarks_Clear();
int Bookmarks_FindAtOffset(long long byteOffset);

void ByteStats_Compute(HexData& hexData);
void ByteStats_Clear();

void InitializeLeftPanelSections(Vector<PanelSection>& sections, HexData& hexData);
void InitializeDataInspectorSection(Vector<PanelSection>& sections, HexData& hexData);
void InitializeFileInfoSection(Vector<PanelSection>& sections, HexData& hexData);
void InitializeBookmarksSection(Vector<PanelSection>& sections);
void InitializeByteStatsSection(Vector<PanelSection>& sections);

bool HandleLeftPanelContentClick(int x, int y, int windowWidth, int windowHeight);
