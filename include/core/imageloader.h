#pragma once

#ifdef _WIN32
#include <windows.h>
#include <unknwn.h>       
#include <objbase.h>    
#include <gdiplus.h>
#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <stdlib.h>
#endif

#ifdef __linux__
#include <X11/Xlib.h>
#include <stdlib.h>
extern "C"
{
    extern const unsigned char _binary_about_png_start[];
    extern const unsigned char _binary_about_png_end[];
}
#endif

class ImageLoader
{
public:
    struct Buffer
    {
        unsigned char *data;
        unsigned int size;
        unsigned int capacity;
    };

    static Buffer *BufferCreate(unsigned int initialCap)
    {
#ifdef _WIN32
        Buffer *buf = (Buffer *)HeapAlloc(GetProcessHeap(), 0, sizeof(Buffer));
        if (!buf)
            return nullptr;

        buf->data = (unsigned char *)HeapAlloc(GetProcessHeap(), 0, initialCap);
        if (!buf->data)
        {
            HeapFree(GetProcessHeap(), 0, buf);
            return nullptr;
        }
#else
        Buffer *buf = (Buffer *)malloc(sizeof(Buffer));
        if (!buf)
            return nullptr;

        buf->data = (unsigned char *)malloc(initialCap);
        if (!buf->data)
        {
            free(buf);
            return nullptr;
        }
#endif

        buf->size = 0;
        buf->capacity = initialCap;
        return buf;
    }

    static void BufferFree(Buffer *buf)
    {
        if (!buf)
            return;
#ifdef _WIN32
        if (buf->data)
            HeapFree(GetProcessHeap(), 0, buf->data);
        HeapFree(GetProcessHeap(), 0, buf);
#else
        if (buf->data)
            free(buf->data);
        free(buf);
#endif
    }

    static bool BufferResize(Buffer *buf, unsigned int newSize)
    {
        if (newSize <= buf->capacity)
        {
            buf->size = newSize;
            return true;
        }

        unsigned int newCap = buf->capacity * 2;
        if (newCap < newSize)
            newCap = newSize;

#ifdef _WIN32
        unsigned char *newData = (unsigned char *)HeapReAlloc(GetProcessHeap(), 0, buf->data, newCap);
#else
        unsigned char *newData = (unsigned char *)realloc(buf->data, newCap);
#endif
        if (!newData)
            return false;

        buf->data = newData;
        buf->capacity = newCap;
        buf->size = newSize;
        return true;
    }

    static void MemCopy(void *dst, const void *src, unsigned int size)
    {
        unsigned char *d = (unsigned char *)dst;
        const unsigned char *s = (const unsigned char *)src;
        for (unsigned int i = 0; i < size; i++)
        {
            d[i] = s[i];
        }
    }

    static void MemSet(void *dst, unsigned char val, unsigned int size)
    {
        unsigned char *d = (unsigned char *)dst;
        for (unsigned int i = 0; i < size; i++)
        {
            d[i] = val;
        }
    }

    static int MemCmp(const void *a, const void *b, unsigned int size)
    {
        const unsigned char *pa = (const unsigned char *)a;
        const unsigned char *pb = (const unsigned char *)b;
        for (unsigned int i = 0; i < size; i++)
        {
            if (pa[i] != pb[i])
                return pa[i] - pb[i];
        }
        return 0;
    }

public:
#ifdef _WIN32
    static bool LoadPNG(const char *name, Buffer **outBuffer)
    {
        if (!outBuffer)
            return false;
        *outBuffer = nullptr;

        HMODULE hModule = GetModuleHandle(nullptr);
        if (!hModule)
            return false;

        bool isIntResource = ((unsigned long long)name < 0x10000);
        HRSRC hRes = nullptr;

        if (isIntResource)
        {
            hRes = FindResourceW(hModule, (LPCWSTR)name, (LPCWSTR)RT_RCDATA);
        }
        else
        {
            wchar_t wname[256];
            int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
            if (len <= 0)
                return false;

            hRes = FindResourceW(hModule, wname, (LPCWSTR)RT_RCDATA);
        }

        if (!hRes)
            return false;

        HGLOBAL hMem = LoadResource(hModule, hRes);
        if (!hMem)
            return false;

        DWORD size = SizeofResource(hModule, hRes);
        void *ptr = LockResource(hMem);
        if (!ptr || size == 0)
            return false;

        Buffer *buf = BufferCreate(size);
        if (!buf)
            return false;

        MemCopy(buf->data, ptr, size);
        buf->size = size;

        *outBuffer = buf;
        return true;
    }

    static HBITMAP LoadPNGDecoded(const char *name, int *outWidth, int *outHeight)
    {
        static bool gdiplusInitialized = false;
        static ULONG_PTR gdiplusToken = 0;

        if (!gdiplusInitialized)
        {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
            gdiplusInitialized = true;
        }

        Buffer *pngData = nullptr;
        if (!LoadPNG(name, &pngData))
            return nullptr;

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, pngData->size);
        if (!hMem)
        {
            BufferFree(pngData);
            return nullptr;
        }

        void *pMem = GlobalLock(hMem);
        if (!pMem)
        {
            GlobalFree(hMem);
            BufferFree(pngData);
            return nullptr;
        }

        MemCopy(pMem, pngData->data, pngData->size);
        GlobalUnlock(hMem);
        BufferFree(pngData);

        IStream *pStream = nullptr;
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK)
        {
            GlobalFree(hMem);
            return nullptr;
        }

        Gdiplus::Bitmap *bitmap = Gdiplus::Bitmap::FromStream(pStream);
        pStream->Release();

        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        {
            if (bitmap)
                delete bitmap;
            return nullptr;
        }

        int width = bitmap->GetWidth();
        int height = bitmap->GetHeight();

        if (outWidth)
            *outWidth = width;
        if (outHeight)
            *outHeight = height;

        BITMAPV5HEADER bmi = {};
        bmi.bV5Size = sizeof(BITMAPV5HEADER);
        bmi.bV5Width = width;
        bmi.bV5Height = -height;
        bmi.bV5Planes = 1;
        bmi.bV5BitCount = 32;
        bmi.bV5Compression = BI_BITFIELDS;
        bmi.bV5RedMask = 0x00FF0000;
        bmi.bV5GreenMask = 0x0000FF00;
        bmi.bV5BlueMask = 0x000000FF;
        bmi.bV5AlphaMask = 0xFF000000;

        void *bits = nullptr;
        HDC hdc = GetDC(nullptr);
        HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        ReleaseDC(nullptr, hdc);

        if (hBitmap && bits)
        {
            Gdiplus::BitmapData bitmapData;
            Gdiplus::Rect rect(0, 0, width, height);

            if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead,
                                 PixelFormat32bppARGB, &bitmapData) == Gdiplus::Ok)
            {
                unsigned char *src = (unsigned char *)bitmapData.Scan0;
                unsigned char *dst = (unsigned char *)bits;

                for (int y = 0; y < height; y++)
                {
                    unsigned char *srcRow = src + y * bitmapData.Stride;
                    unsigned char *dstRow = dst + y * width * 4;

                    for (int x = 0; x < width; x++)
                    {
                        unsigned char b = srcRow[x * 4];
                        unsigned char g = srcRow[x * 4 + 1];
                        unsigned char r = srcRow[x * 4 + 2];
                        unsigned char a = srcRow[x * 4 + 3];

                        if (a < 255)
                        {
                            r = (r * a) / 255;
                            g = (g * a) / 255;
                            b = (b * a) / 255;
                        }

                        dstRow[x * 4] = b;
                        dstRow[x * 4 + 1] = g;
                        dstRow[x * 4 + 2] = r;
                        dstRow[x * 4 + 3] = a;
                    }
                }

                bitmap->UnlockBits(&bitmapData);
            }
        }

        delete bitmap;
        return hBitmap;
    }

    static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, int x, int y, int width, int height)
    {
        BITMAP bm;
        GetObject(hBitmap, sizeof(BITMAP), &bm);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);

        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, blend);

        SelectObject(hdcMem, hOld);
        DeleteDC(hdcMem);
    }
#endif

#ifdef __APPLE__
    static bool LoadPNG(const char *name, Buffer **outBuffer)
    {
        if (!outBuffer)
            return false;
        *outBuffer = nullptr;

        @autoreleasepool
        {
            NSString *nsName = [NSString stringWithUTF8String:name];
            NSString *base = [nsName stringByDeletingPathExtension];
            NSString *ext = [nsName pathExtension];
            NSBundle *bundle = [NSBundle mainBundle];
            NSString *path = [bundle pathForResource:base ofType:ext];
            if (!path)
                return false;

            NSData *data = [NSData dataWithContentsOfFile:path];
            if (!data)
                return false;

            Buffer *buf = BufferCreate([data length]);
            if (!buf)
                return false;

            MemCopy(buf->data, [data bytes], [data length]);
            buf->size = [data length];

            *outBuffer = buf;
            return true;
        }
    }

    static NSImage *LoadPNGDecoded(const char *name)
    {
        @autoreleasepool
        {
            NSString *nsName = [NSString stringWithUTF8String:name];
            NSString *base = [nsName stringByDeletingPathExtension];
            NSString *ext = [nsName pathExtension];
            NSBundle *bundle = [NSBundle mainBundle];
            NSString *path = [bundle pathForResource:base ofType:ext];
            if (!path)
                return nullptr;

            return [[NSImage alloc] initWithContentsOfFile:path];
        }
    }
#endif

#ifdef __linux__
    static bool LoadPNG(const char *name, Buffer **outBuffer)
    {
        if (!outBuffer)
            return false;
        *outBuffer = nullptr;

        unsigned int size = _binary_about_png_end - _binary_about_png_start;
        if (size == 0)
            return false;

        Buffer *buf = BufferCreate(size);
        if (!buf)
            return false;

        MemCopy(buf->data, _binary_about_png_start, size);
        buf->size = size;

        *outBuffer = buf;
        return true;
    }
#endif
};
