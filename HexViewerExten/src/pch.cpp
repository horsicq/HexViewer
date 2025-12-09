#include "pch.h"

void LogMessage(const std::wstring& message) {
#ifdef _DEBUG
    wchar_t desktopPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) {
        std::wstring logFilePath = std::wstring(desktopPath) + L"\\ServiceLog.txt";
        std::wofstream logFile(logFilePath, std::ios::app);
        if (logFile.is_open()) {
            logFile << message << std::endl;
            logFile.close();
        }
        else {
            fprint(stderr, "Failed to open log file.");
        }
    }
    else {
        fprintf(stderr, "Failed to get desktop directory.");
    }
#endif
}
