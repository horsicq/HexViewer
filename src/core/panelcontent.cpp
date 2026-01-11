#include "panelcontent.h"
#include "platform_die.h"
#include "global.h"

extern MenuBar g_MenuBar;

#ifdef _WIN32
extern HWND g_Hwnd;
#endif

extern LeftPanelState g_LeftPanel;
extern BottomPanelState g_BottomPanel;
extern ChecksumResults g_Checksums;

extern char g_CurrentFilePath[260];
extern char g_DIEExecutablePath[260];
const int PANEL_TITLE_HEIGHT = 28;
extern HexData g_HexData;
BookmarksState g_Bookmarks = {{}, -1};
ByteStatistics g_ByteStats = {{0}, 0, 0, 0, 0, 0, 0.0, false};
DetectItEasyState g_DIEState = {false, "", "", ""};

extern long long cursorBytePos;
extern int cursorNibblePos;
extern int g_ScrollY;
extern int g_LinesPerPage;
void InvalidateWindow();

PatternSearchState g_PatternSearch = {"", -1, false};
ChecksumState g_Checksum = {false, false, false, false, true};
CompareState g_Compare = {"", false};

void PatternSearch_SetFocus()
{
    g_PatternSearch.hasFocus = true;
}

void PatternSearch_Run()
{
    g_PatternSearch.lastMatch = -1;
}

void PatternSearch_FindNext()
{
    uint8_t pattern[128];
    int patLen = ParseHexPattern(g_PatternSearch.searchPattern, pattern, 128);
    if (patLen <= 0)
        return;

    long long fileSize = (long long)g_HexData.getFileSize();
    if (fileSize <= 0)
        return;

    long long start = g_PatternSearch.lastMatch >= 0
                          ? g_PatternSearch.lastMatch + 1
                          : 0;

    for (long long i = start; i <= fileSize - patLen; i++)
    {
        bool match = true;
        for (int j = 0; j < patLen; j++)
        {
            if (g_HexData.getByte((size_t)(i + j)) != pattern[j])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            g_PatternSearch.lastMatch = i;

            cursorBytePos = i;
            cursorNibblePos = 0;

            long long line = i / 16;
            if (line < g_ScrollY || line >= g_ScrollY + g_LinesPerPage)
            {
                g_ScrollY = (int)line;

#ifdef _WIN32
                SetScrollPos(g_Hwnd, SB_VERT, g_ScrollY, TRUE);
#endif
            }

            InvalidateWindow();
            return;
        }
    }

    g_PatternSearch.lastMatch = -1;
}

void PatternSearch_FindPrev()
{
    uint8_t pattern[128];
    int patLen = ParseHexPattern(g_PatternSearch.searchPattern, pattern, 128);
    if (patLen <= 0)
        return;

    long long fileSize = (long long)g_HexData.getFileSize();
    if (fileSize <= 0)
        return;

    long long start = g_PatternSearch.lastMatch >= 0
                          ? g_PatternSearch.lastMatch - 1
                          : fileSize - patLen;

    for (long long i = start; i >= 0; i--)
    {
        bool match = true;
        for (int j = 0; j < patLen; j++)
        {
            if (g_HexData.getByte((size_t)(i + j)) != pattern[j])
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            g_PatternSearch.lastMatch = i;

            cursorBytePos = i;
            cursorNibblePos = 0;

            long long line = i / 16;
            if (line < g_ScrollY || line >= g_ScrollY + g_LinesPerPage)
            {
                g_ScrollY = (int)line;

#ifdef _WIN32
                SetScrollPos(g_Hwnd, SB_VERT, g_ScrollY, TRUE);
#endif
            }

            InvalidateWindow();
            return;
        }
    }

    g_PatternSearch.lastMatch = -1;
}

void Checksum_ToggleMD5()
{
    g_Checksum.md5 = !g_Checksum.md5;
}

void Checksum_ToggleSHA1()
{
    g_Checksum.sha1 = !g_Checksum.sha1;
}

void Checksum_ToggleSHA256()
{
    g_Checksum.sha256 = !g_Checksum.sha256;
}

void Checksum_ToggleCRC32()
{
    g_Checksum.crc32 = !g_Checksum.crc32;
}

void Checksum_SetModeEntireFile()
{
    g_Checksum.entireFile = true;
}

void Checksum_SetModeSelection()
{
    g_Checksum.entireFile = false;
}

void Checksum_Compare()
{
}

void Checksum_Compute()
{
}

void Compare_OpenFileDialog()
{
    g_Compare.fileLoaded = true;
}

void Compare_Run()
{
    if (!g_Compare.fileLoaded)
        return;
}

static int ParseHexPattern(const char *text, uint8_t *out, int maxOut)
{
    int count = 0;
    int i = 0;

    while (text[i] && count < maxOut)
    {
        while (text[i] == ' ')
            i++;

        if (!text[i] || !text[i + 1])
            break;

        char a = text[i];
        char b = text[i + 1];

        auto hex = [](char c) -> int
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        };

        int hi = hex(a);
        int lo = hex(b);
        if (hi < 0 || lo < 0)
            break;

        out[count++] = (uint8_t)((hi << 4) | lo);

        i += 2;
    }

    return count;
}

void Bookmarks_Add(long long byteOffset, const char *name, Color color)
{
    if (Bookmarks_FindAtOffset(byteOffset) >= 0)
    {
        return;
    }

    Bookmark bm;
    bm.byteOffset = byteOffset;
    StrCopy(bm.name, name);
    bm.color = color;
    bm.description[0] = '\0';

    g_Bookmarks.bookmarks.push_back(bm);
    InvalidateWindow();
}

void Bookmarks_Remove(int index)
{
    if (index >= 0 && index < (int)g_Bookmarks.bookmarks.size())
    {
        g_Bookmarks.bookmarks.remove(index);

        if (g_Bookmarks.selectedIndex == index)
        {
            g_Bookmarks.selectedIndex = -1;
        }
        else if (g_Bookmarks.selectedIndex > index)
        {
            g_Bookmarks.selectedIndex--;
        }
        InvalidateWindow();
    }
}

void Bookmarks_JumpTo(int index)
{
    if (index >= 0 && index < (int)g_Bookmarks.bookmarks.size())
    {
        cursorBytePos = g_Bookmarks.bookmarks[index].byteOffset;
        cursorNibblePos = 0;
        g_Bookmarks.selectedIndex = index;

        long long line = cursorBytePos / 16;
        if (line < g_ScrollY || line >= g_ScrollY + g_LinesPerPage)
        {
            g_ScrollY = (int)line;

#ifdef _WIN32
            extern HWND g_Hwnd;
            SetScrollPos(g_Hwnd, SB_VERT, g_ScrollY, TRUE);
#endif
        }

        InvalidateWindow();
    }
}

void Bookmarks_Clear()
{
    g_Bookmarks.bookmarks.clear();
    g_Bookmarks.selectedIndex = -1;
    InvalidateWindow();
}

int Bookmarks_FindAtOffset(long long byteOffset)
{
    for (int i = 0; i < (int)g_Bookmarks.bookmarks.size(); i++)
    {
        if (g_Bookmarks.bookmarks[i].byteOffset == byteOffset)
        {
            return i;
        }
    }
    return -1;
}

void ByteStats_Compute(HexData &hexData)
{
    memSet(&g_ByteStats, 0, sizeof(ByteStatistics));

    uint64_t fileSize = (uint64_t)hexData.getFileSize();
    if (fileSize == 0)
    {
        g_ByteStats.computed = false;
        return;
    }

    for (uint64_t i = 0; i < fileSize; i++)
    {
        uint8_t b = hexData.getByte((size_t)i);
        g_ByteStats.histogram[b]++;
    }

    g_ByteStats.mostCommonCount = 0;
    g_ByteStats.leastCommonCount = (int)fileSize + 1;

    for (int i = 0; i < 256; i++)
    {
        int count = g_ByteStats.histogram[i];

        if (count > g_ByteStats.mostCommonCount)
        {
            g_ByteStats.mostCommonCount = count;
            g_ByteStats.mostCommonByte = i;
        }

        if (count > 0 && count < g_ByteStats.leastCommonCount)
        {
            g_ByteStats.leastCommonCount = count;
            g_ByteStats.leastCommonByte = i;
        }
    }

    g_ByteStats.nullByteCount = g_ByteStats.histogram[0];

    int distinct = 0;
    for (int i = 0; i < 256; i++)
    {
        if (g_ByteStats.histogram[i] > 0)
            distinct++;
    }

    double entropy = (double)distinct * (8.0 / 256.0);

    g_ByteStats.entropy = entropy;

    g_ByteStats.computed = true;
    InvalidateWindow();
}

void ByteStats_Clear()
{
    memSet(&g_ByteStats, 0, sizeof(ByteStatistics));
    g_ByteStats.computed = false;
}

void DIE_Analyze()
{
    g_DIEState.analyzed = true;
    StrCopy(g_DIEState.fileType, "Coming Soon");
    StrCopy(g_DIEState.compiler, "Coming Soon");
    StrCopy(g_DIEState.architecture, "Coming Soon");
    InvalidateWindow();
}

void DIE_OpenInApplication()
{
#ifdef _WIN32
    if (!g_DIEExecutablePath[0])
    {
        MessageBoxA(nullptr, "DIE path is empty!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (!g_CurrentFilePath[0])
    {
        MessageBoxA(nullptr, "Current file path is empty!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    HINSTANCE result = ShellExecuteA(
        nullptr,
        "open",
        g_DIEExecutablePath,
        g_CurrentFilePath,
        nullptr,
        SW_SHOW
    );

    if ((int)result <= 32)
    {
        char errorMsg[256];
        StrCopy(errorMsg, "ShellExecute failed with code: ");
        char code[32];
        IntToStr((int)result, code, 32);
        StrCat(errorMsg, code);

        MessageBoxA(nullptr, errorMsg, "Error", MB_OK | MB_ICONERROR);
    }
#endif
}


bool HandleBottomPanelContentClick(int x, int y, int windowWidth, int windowHeight)
{
    if (!g_BottomPanel.visible)
        return false;

    Rect bottomBounds = GetBottomPanelBounds(
        g_BottomPanel, windowWidth, windowHeight,
        g_MenuBar.getHeight(), g_LeftPanel);

    int tabHeight = 32;
    bool isVertical = (g_BottomPanel.dockPosition == PanelDockPosition::Left ||
                       g_BottomPanel.dockPosition == PanelDockPosition::Right);

    int contentX = bottomBounds.x + 15;
    int contentY = bottomBounds.y + PANEL_TITLE_HEIGHT +
                   (isVertical ? (tabHeight * 4) : tabHeight) + 10;

    int contentWidth = bottomBounds.width - 30;
    int contentHeight = bottomBounds.height - (contentY - bottomBounds.y) - 10;

    switch (g_BottomPanel.activeTab)
    {
    case BottomPanelState::Tab::EntropyAnalysis:
        return false;

    case BottomPanelState::Tab::PatternSearch:
    {
        int cy = contentY;

        cy += 25;
        cy += 20;

        Rect searchBox(contentX, cy, 200, 28);
        if (IsPointInRect(x, y, searchBox))
        {
            g_PatternSearch.hasFocus = true;
            cursorBytePos = -1;
            InvalidateWindow();
            return true;
        }

        Rect findBtn(contentX + 210, cy, 80, 28);
        if (IsPointInRect(x, y, findBtn))
        {
            PatternSearch_Run();
            g_PatternSearch.hasFocus = false;
            InvalidateWindow();
            return true;
        }

        cy += 40;

        Rect prevBtn(contentX, cy, 120, 28);
        if (IsPointInRect(x, y, prevBtn))
        {
            PatternSearch_FindPrev();
            g_PatternSearch.hasFocus = false;
            InvalidateWindow();
            return true;
        }

        Rect nextBtn(contentX + 130, cy, 100, 28);
        if (IsPointInRect(x, y, nextBtn))
        {
            PatternSearch_FindNext();
            g_PatternSearch.hasFocus = false;
            InvalidateWindow();
            return true;
        }

        return false;
    }

    case BottomPanelState::Tab::Checksum:
    {
        int cy = contentY;
        cy += 25;

        Rect md5Check(contentX, cy, 16, 16);
        if (IsPointInRect(x, y, md5Check))
        {
            Checksum_ToggleMD5();
            InvalidateWindow();
            return true;
        }

        Rect sha1Check(contentX + 100, cy, 16, 16);
        if (IsPointInRect(x, y, sha1Check))
        {
            Checksum_ToggleSHA1();
            InvalidateWindow();
            return true;
        }

        Rect sha256Check(contentX + 200, cy, 16, 16);
        if (IsPointInRect(x, y, sha256Check))
        {
            Checksum_ToggleSHA256();
            InvalidateWindow();
            return true;
        }

        Rect crc32Check(contentX + 330, cy, 16, 16);
        if (IsPointInRect(x, y, crc32Check))
        {
            Checksum_ToggleCRC32();
            InvalidateWindow();
            return true;
        }

        cy += 35;

        Rect entireFileRadio(contentX, cy, 16, 16);
        if (IsPointInRect(x, y, entireFileRadio))
        {
            Checksum_SetModeEntireFile();
            InvalidateWindow();
            return true;
        }

        Rect selectionRadio(contentX + 150, cy, 16, 16);
        if (IsPointInRect(x, y, selectionRadio))
        {
            Checksum_SetModeSelection();
            InvalidateWindow();
            return true;
        }

        cy += 35;

        Rect compareBtn(contentX, cy, 100, 28);
        if (IsPointInRect(x, y, compareBtn))
        {
            Checksum_Compare();
            InvalidateWindow();
            return true;
        }

        Rect hashCalcBtn(contentX + 110, cy, 150, 28);
        if (IsPointInRect(x, y, hashCalcBtn))
        {
            Checksum_Compute();
            InvalidateWindow();
            return true;
        }

        if (IsPointInRect(x, y, bottomBounds))
        {
            return true;
        }

        return false;
    }

    case BottomPanelState::Tab::Compare:
    {
        int cy = contentY;
        cy += 25;
        cy += 30;

        Rect selectBtn(contentX, cy, 150, 32);
        if (IsPointInRect(x, y, selectBtn))
        {
            Compare_OpenFileDialog();
            Compare_Run();
            InvalidateWindow();
            return true;
        }

        if (IsPointInRect(x, y, bottomBounds))
        {
            return true;
        }

        return false;
    }
    }

    return false;
}

bool HandleLeftPanelContentClick(int x, int y, int windowWidth, int windowHeight)
{
    if (!g_LeftPanel.visible)
        return false;

    int menuBarHeight = 24;
    Rect panelBounds = GetLeftPanelBounds(g_LeftPanel, windowWidth, windowHeight, menuBarHeight);

    if (x < panelBounds.x || x > panelBounds.x + panelBounds.width ||
        y < panelBounds.y || y > panelBounds.y + panelBounds.height)
        return false;

    if (y < panelBounds.y + PANEL_TITLE_HEIGHT)
        return false;

    int contentX = panelBounds.x + 15;
    int currentY = panelBounds.y + PANEL_TITLE_HEIGHT + 10;
    int contentWidth = panelBounds.width - 30;

    int rowHeight = 16;
    int headerHeight = 18;
    int itemSpacing = 4;
    int sectionSpacing = 10;

    long long fileSize = (long long)g_HexData.getFileSize();

    currentY += headerHeight + sectionSpacing;
    currentY += (rowHeight + itemSpacing) * 2;
    currentY += 8;

    currentY += headerHeight + sectionSpacing;
    if (cursorBytePos >= 0 && cursorBytePos < fileSize)
    {
        currentY += (rowHeight + itemSpacing) * 5;
        currentY += 8;
    }
    else
    {
        currentY += rowHeight + itemSpacing;
    }

    currentY += headerHeight + sectionSpacing;

    if (g_Bookmarks.bookmarks.empty())
    {
        currentY += (rowHeight + itemSpacing) * 2;
    }
    else
    {
        for (size_t i = 0; i < g_Bookmarks.bookmarks.size() && i < 5; i++)
        {
            Rect bookmarkRect(contentX, currentY, contentWidth, rowHeight);

            if (x >= bookmarkRect.x && x <= bookmarkRect.x + bookmarkRect.width &&
                y >= bookmarkRect.y && y <= bookmarkRect.y + bookmarkRect.height)
            {
                cursorBytePos = g_Bookmarks.bookmarks[i].byteOffset;
                g_Bookmarks.selectedIndex = (int)i;
                return true;
            }

            currentY += rowHeight + itemSpacing;
        }
    }

    currentY += 8;

    currentY += headerHeight + sectionSpacing;

    if (!g_ByteStats.computed)
    {
        Rect computeRect(contentX, currentY, contentWidth, rowHeight);

        if (x >= computeRect.x && x <= computeRect.x + computeRect.width &&
            y >= computeRect.y && y <= computeRect.y + computeRect.height)
        {
            g_ByteStats.computed = true;

            for (int i = 0; i < 256; i++)
                g_ByteStats.histogram[i] = 0;

            for (size_t i = 0; i < g_HexData.getFileSize(); i++)
            {
                uint8_t byte = g_HexData.getByte(i);
                g_ByteStats.histogram[byte]++;
            }

            g_ByteStats.mostCommonByte = 0;
            g_ByteStats.mostCommonCount = g_ByteStats.histogram[0];
            g_ByteStats.leastCommonByte = 0;
            g_ByteStats.leastCommonCount = g_ByteStats.histogram[0];

            for (int i = 1; i < 256; i++)
            {
                if (g_ByteStats.histogram[i] > g_ByteStats.mostCommonCount)
                {
                    g_ByteStats.mostCommonByte = (uint8_t)i;
                    g_ByteStats.mostCommonCount = g_ByteStats.histogram[i];
                }
                if (g_ByteStats.histogram[i] < g_ByteStats.leastCommonCount)
                {
                    g_ByteStats.leastCommonByte = (uint8_t)i;
                    g_ByteStats.leastCommonCount = g_ByteStats.histogram[i];
                }
            }

            g_ByteStats.nullByteCount = g_ByteStats.histogram[0];

            g_ByteStats.entropy = 0.0;
            long long total = (long long)g_HexData.getFileSize();

            if (total > 0)
            {
                for (int i = 0; i < 256; i++)
                {
                    if (g_ByteStats.histogram[i] > 0)
                    {
                        double p = (double)g_ByteStats.histogram[i] / (double)total;
                        g_ByteStats.entropy -= p * fast_log2(p);
                    }
                }
            }

            return true;
        }

        currentY += rowHeight + itemSpacing;
    }
    else
    {
        currentY += rowHeight + itemSpacing;
    }

    currentY += 8;

    currentY += headerHeight + sectionSpacing;
    currentY += (rowHeight + itemSpacing) * 3;

    char diePath[260];
    bool dieFound = FindDIEPath(diePath, sizeof(diePath));

    if (dieFound)
    {
        currentY += 4;
        currentY += itemSpacing;

        Rect buttonRect(contentX, currentY, contentWidth, rowHeight + 10);

        if (x >= buttonRect.x && x <= buttonRect.x + buttonRect.width &&
            y >= buttonRect.y && y <= buttonRect.y + buttonRect.height)
        {
            DIE_OpenInApplication();
            return true;
        }
    }

    return false;
}
