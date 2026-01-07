#ifndef PLATFORM_DIE_H
#define PLATFORM_DIE_H

#ifdef _WIN32
#include <windows.h>
static bool g_DIEPathDebug = false;
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#endif

inline bool FindDIEPath(char *outPath, int outSize)
{
#ifdef _WIN32
    {
        const char *regPath = "Software\\Classes\\*\\shell\\Detect It Easy\\command";
        HKEY roots[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
        char buf[512];
        DWORD sz;

        for (int r = 0; r < 2; r++)
        {
            HKEY h;
            if (RegOpenKeyExA(roots[r], regPath, 0, KEY_READ, &h) == ERROR_SUCCESS)
            {
                sz = sizeof(buf);
                if (RegQueryValueExA(h, NULL, NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS)
                {
                    RegCloseKey(h);

                    int s = -1, e = -1;
                    for (DWORD i = 0; i < sz; i++)
                    {
                        if (buf[i] == '"')
                        {
                            if (s == -1) s = i + 1;
                            else { e = i; break; }
                        }
                    }

                    if (s != -1 && e != -1)
                    {
                        int j = 0;
                        for (int i = s; i < e && j < outSize - 1; i++)
                            outPath[j++] = buf[i];
                        outPath[j] = 0;
                        return true;
                    }
                }
                RegCloseKey(h);
            }
        }
    }

    {
        HKEY h;
        if (RegOpenKeyExA(
                HKEY_CURRENT_USER,
                "Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages",
                0, KEY_READ, &h) == ERROR_SUCCESS)
        {
            char name[256];
            DWORD len = sizeof(name);
            DWORD idx = 0;

            while (RegEnumKeyExA(h, idx, name, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                len = sizeof(name);
                idx++;

                if (name[0]=='D' && name[1]=='i' && name[2]=='e' && name[3]=='_')
                {
                    char path[512];
                    StrCopy(path, "C:\\Program Files\\WindowsApps\\");
                    StrCat(path, name);
                    StrCat(path, "\\Die\\Die.exe");

                    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ,
                                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                    if (f != INVALID_HANDLE_VALUE)
                    {
                        CloseHandle(f);

                        int j = 0;
                        while (path[j] && j < outSize - 1)
                            outPath[j] = path[j], j++;
                        outPath[j] = 0;

                        RegCloseKey(h);
                        return true;
                    }
                }
            }

            RegCloseKey(h);
        }
    }

    return false;
#elif __APPLE__
    const char *macPaths[] = {
        "/Applications/Detect It Easy.app/Contents/MacOS/die",
        "/usr/local/bin/die",
        "/opt/local/bin/die"};

    for (int i = 0; i < 3; i++)
    {
        FILE *f = fopen(macPaths[i], "rb");
        if (f)
        {
            fclose(f);

            int j = 0;
            for (; macPaths[i][j] && j < outSize - 1; j++)
                outPath[j] = macPaths[i][j];
            outPath[j] = 0;

            return true;
        }
    }

    return false;

#else
    const char *linuxPaths[] = {
        "/usr/bin/die",
        "/usr/local/bin/die",
        "/opt/die/die",
        "/snap/bin/die"};

    for (int i = 0; i < 4; i++)
    {
        FILE *f = fopen(linuxPaths[i], "rb");
        if (f)
        {
            fclose(f);

            int j = 0;
            for (; linuxPaths[i][j] && j < outSize - 1; j++)
                outPath[j] = linuxPaths[i][j];
            outPath[j] = 0;

            return true;
        }
    }

    char *pathEnv = getenv("PATH");
    if (pathEnv)
    {
        char temp[512];
        int ti = 0;

        for (int i = 0;; i++)
        {
            char c = pathEnv[i];

            if (c == ':' || c == 0)
            {
                temp[ti] = 0;

                char full[600];
                int fi = 0;

                for (int k = 0; temp[k] && fi < 580; k++)
                    full[fi++] = temp[k];

                full[fi++] = '/';
                full[fi++] = 'd';
                full[fi++] = 'i';
                full[fi++] = 'e';
                full[fi] = 0;

                FILE *f = fopen(full, "rb");
                if (f)
                {
                    fclose(f);

                    int j = 0;
                    for (; full[j] && j < outSize - 1; j++)
                        outPath[j] = full[j];
                    outPath[j] = 0;

                    return true;
                }

                ti = 0;
                if (c == 0)
                    break;
            }
            else
            {
                if (ti < 510)
                    temp[ti++] = c;
            }
        }
    }

    return false;

#endif
}

#endif
