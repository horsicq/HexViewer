#ifndef GLOBAL_H
#define GLOBAL_H

#include <stddef.h>
#include <stdint.h>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#define MAX_PATH_LEN MAX_PATH
#else
#define MAX_PATH_LEN 4096
#endif


inline void* sys_alloc(size_t size)
{
#ifdef _WIN32
    return HeapAlloc(GetProcessHeap(), 0, size);
#else
    return malloc(size);
#endif
}

inline void* sys_realloc(void* ptr, size_t size)
{
#ifdef _WIN32
    if (!ptr)
        return HeapAlloc(GetProcessHeap(), 0, size);
    if (size == 0) {
        HeapFree(GetProcessHeap(), 0, ptr);
        return NULL;
    }
    return HeapReAlloc(GetProcessHeap(), 0, ptr, size);
#else
    return realloc(ptr, size);
#endif
}

inline void sys_free(void* ptr)
{
#ifdef _WIN32
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
#else
    if (ptr)
        free(ptr);
#endif
}

#ifdef _WIN32

inline void* PlatformAlloc(size_t size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

inline void PlatformFree(void* ptr, size_t = 0)
{
    if (ptr)
        HeapFree(GetProcessHeap(), 0, ptr);
}

#elif defined(__APPLE__)

inline void* PlatformAlloc(size_t size)
{
    return malloc(size);
}

inline void PlatformFree(void* ptr, size_t = 0)
{
    if (ptr)
        free(ptr);
}

#else

inline void* PlatformAlloc(size_t size)
{
    return malloc(size);
}

inline void PlatformFree(void* ptr, size_t = 0)
{
    if (ptr)
        free(ptr);
}

#endif

extern int g_SearchCaretX;
extern int g_SearchCaretY;
extern bool caretVisible;

inline void* memSet(void* dest, int val, size_t count)
{
    unsigned char* p = (unsigned char*)dest;
    while (count--) {
        *p++ = (unsigned char)val;
    }
    return dest;
}

inline char* StrCopy(char* dest, const char* src)
{
    char* original = dest;
    while ((*dest++ = *src++)) {
    }
    return original;
}

inline int StringLength(const char *str)
{
    int len = 0;
    while (str[len])
        len++;
    return len;
}

inline void StringCopy(char *dest, const char *src, int maxLen)
{
    int i = 0;
    while (src[i] && i < maxLen - 1)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
}

inline int StrHexToInt(const char* hex)
{
    int value = 0;
    for (int i = 0; i < 4; i++)
    {
        char c = hex[i];
        int digit = 0;

        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else
            return value;

        value = (value << 4) | digit;
    }
    return value;
}


inline void StringAppend(char *dest, char ch, int maxLen)
{
    int len = StringLength(dest);
    if (len < maxLen - 1)
    {
        dest[len] = ch;
        dest[len + 1] = 0;
    }
}

inline void StringRemoveLast(char *str)
{
    int len = StringLength(str);
    if (len > 0)
    {
        str[len - 1] = 0;
    }
}

inline bool StringIsEmpty(const char *str)
{
    return str[0] == 0;
}

inline int StringToInt(const char *str)
{
    int result = 0;
    int i = 0;
    bool isHex = false;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        isHex = true;
        i = 2;
    }

    while (str[i])
    {
        if (isHex)
        {
            result *= 16;
            if (str[i] >= '0' && str[i] <= '9')
                result += str[i] - '0';
            else if (str[i] >= 'a' && str[i] <= 'f')
                result += str[i] - 'a' + 10;
            else if (str[i] >= 'A' && str[i] <= 'F')
                result += str[i] - 'A' + 10;
        }
        else
        {
            if (str[i] >= '0' && str[i] <= '9')
            {
                result = result * 10 + (str[i] - '0');
            }
        }
        i++;
    }
    return result;
}

inline void IntToString(int value, char* out, int maxLen)
{
    if (!out || maxLen <= 1)
        return;

    unsigned int v;
    int pos = 0;

    if (value < 0)
    {
        if (maxLen < 2) { out[0] = '\0'; return; }
        out[pos++] = '-';
        v = (unsigned int)(-value);
    }
    else
    {
        v = (unsigned int)value;
    }

    char temp[32];
    int tpos = 0;

    do {
        temp[tpos++] = '0' + (v % 10);
        v /= 10;
    } while (v && tpos < 31);

    for (int i = tpos - 1; i >= 0 && pos < maxLen - 1; i--)
        out[pos++] = temp[i];

    out[pos] = '\0';
}

inline size_t StrLen(const char* str)
{
    const char* s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

inline char* AllocString(const char* str)
{
    if (!str)
        return nullptr;

    size_t len = StrLen(str);
    char* result = (char*)PlatformAlloc(len + 1);
    if (!result)
        return nullptr;

    StrCopy(result, str);
    return result;
}

template<typename T>
inline T Clamp(T value, T min, T max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

template<typename T>
inline T Min(T a, T b)
{
    return (a < b) ? a : b;
}

template<typename T>
inline T Max(T a, T b)
{
    return (a > b) ? a : b;
}

inline double Floor(double x)
{
    long long i = (long long)x;
    return (double)(i - (x < i));
}

inline bool StrEquals(const char* a, const char* b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    while (*a && *b)
    {
        if (*a != *b)
            return false;
        a++;
        b++;
    }
    return *a == *b;
}

inline void SizeToString(size_t value, char* out, int maxLen)
{
    char temp[32];
    int tpos = 0;

    size_t v = value;

    if (v == 0) {
        if (maxLen > 1) {
            out[0] = '0';
            out[1] = 0;
        }
        return;
    }

    while (v && tpos < 31) {
        temp[tpos++] = '0' + (v % 10);
        v /= 10;
    }

    int pos = 0;
    for (int i = tpos - 1; i >= 0 && pos < maxLen - 1; i--)
        out[pos++] = temp[i];

    out[pos] = 0;
}


inline void* MemAlloc(size_t size) {
    return PlatformAlloc(size);
}

inline void memFree(void* ptr, size_t size = 0) {
    PlatformFree(ptr, size);
}

inline int StrToInt(const char* s)
{
    if (!s) return 0;

    while (*s == ' ' || *s == '\t')
        s++;

    int sign = 1;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }

    int value = 0;
    while (*s >= '0' && *s <= '9')
    {
        value = value * 10 + (*s - '0');
        s++;
    }

    return value * sign;
}

inline void IntToStr(int value, char* buffer, size_t bufferSize)
{
    if (!buffer || bufferSize == 0)
        return;

    if (value == 0)
    {
        if (bufferSize >= 2)
        {
            buffer[0] = '0';
            buffer[1] = '\0';
        }
        return;
    }

    bool negative = false;
    unsigned int v;

    if (value < 0)
    {
        negative = true;
        v = (unsigned int)(-value);
    }
    else
    {
        v = (unsigned int)value;
    }

    char temp[32];
    size_t idx = 0;

    while (v > 0 && idx < sizeof(temp))
    {
        temp[idx++] = (char)('0' + (v % 10));
        v /= 10;
    }

    if (negative && idx < sizeof(temp))
        temp[idx++] = '-';

    size_t out = 0;
    if (idx >= bufferSize)
        idx = bufferSize - 1;

    while (idx > 0)
        buffer[out++] = temp[--idx];

    buffer[out] = '\0';
}

inline size_t WcsLen(const wchar_t* str)
{
    const wchar_t* s = str;
    while (*s) s++;
    return (size_t)(s - str);
}

inline wchar_t* WcsCopy(wchar_t* dest, const wchar_t* src)
{
    wchar_t* original = dest;
    while ((*dest++ = *src++)) {
    }
    return original;
}

inline void MemCopy(void* dest, const void* src, size_t n)
{
    if (!dest || !src || n == 0)
        return;

    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    if (d > s && d < s + n) {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    } else {
        while (n--) *d++ = *s++;
    }
}

inline void ByteToHex(uint8_t b, char out[2])
{
    const char* hex = "0123456789ABCDEF";
    out[0] = hex[(b >> 4) & 0xF];
    out[1] = hex[b & 0xF];
}

inline bool IsXDigit(char c)
{
    return (c >= '0' && c <= '9') ||
           (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

inline int HexDigitToInt(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return 0;
}

inline long long ClampLL(long long v, long long lo, long long hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

inline int ClampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

inline char IntToHexChar(int digit)
{
    if (digit < 10) return (char)('0' + digit);
    return (char)('A' + (digit - 10));
}

inline wchar_t* AllocWideString(const wchar_t* str)
{
    if (!str)
        return nullptr;

    size_t len = WcsLen(str);
    wchar_t* result = (wchar_t*)PlatformAlloc((len + 1) * sizeof(wchar_t));
    if (!result)
        return nullptr;

    WcsCopy(result, str);
    return result;
}

static double fast_log2(double x)
{
    union { double d; uint64_t i; } u = { x };

    int exp = (int)((u.i >> 52) & 0x7FF) - 1023;

    u.i &= ~(uint64_t)0x7FF << 52;
    u.i |= (uint64_t)1023 << 52;

    double m = u.d - 1.0;

    double m2 = m * m;
    double m3 = m2 * m;

    double approx = m - 0.5 * m2 + (1.0 / 3.0) * m3;

    return (double)exp + approx;
}


inline void StrCat(char* dest, const char* src)
{
    if (!dest || !src)
        return;

    while (*dest)
        dest++;

    while (*src)
        *dest++ = *src++;

    *dest = '\0';
}

inline int StrCompareIgnoreCase(const char* a, const char* b)
{
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && *b)
    {
        unsigned char ca = *a;
        unsigned char cb = *b;

        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;

        if (ca != cb)
            return (int)ca - (int)cb;

        ++a;
        ++b;
    }

    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

inline int StrCompare(const char* a, const char* b)
{
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && (*a == *b))
    {
        ++a;
        ++b;
    }

    return (int)( (unsigned char)*a ) - (int)( (unsigned char)*b );
}


inline void ItoaDec(long long value, char* out, int max) {
    char buf[32];
    int i = 0;

    bool neg = value < 0;
    unsigned long long v = neg ? -value : value;

    do {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    } while (v && i < 31);

    if (neg && i < 31)
        buf[i++] = '-';

    int j = 0;
    while (i-- && j < max - 1)
        out[j++] = buf[i];
    out[j] = 0;
}

inline void ItoaHex(unsigned long long value, char* out, int max) {
    static const char* hex = "0123456789ABCDEF";

    if (max < 3) { if (max > 0) out[0] = 0; return; }

    out[0] = '0';
    out[1] = 'x';
    int pos = 2;

    bool started = false;
    for (int i = 60; i >= 0; i -= 4) {
        unsigned digit = (value >> i) & 0xF;
        if (digit || started || i == 0) {
            if (pos < max - 1)
                out[pos++] = hex[digit];
            started = true;
        }
    }
    out[pos] = 0;
}

template<typename T>
class Vector {
private:
    T* data;
    size_t count;
    size_t capacity;
public:
    Vector() : data(nullptr), count(0), capacity(0) {}
    ~Vector()
    {
        if (data)
            PlatformFree(data, capacity * sizeof(T));
    }
    void push_back(const T& item)
    {
        if (count >= capacity)
        {
            size_t newCapacity = (capacity == 0) ? 4 : capacity * 2;
            T* newData = (T*)PlatformAlloc(newCapacity * sizeof(T));
            for (size_t i = 0; i < count; i++)
                newData[i] = data[i];
            if (data)
                PlatformFree(data, capacity * sizeof(T));
            data = newData;
            capacity = newCapacity;
        }
        data[count++] = item;
    }
    void Add(const T& item) { push_back(item); }
    
    void remove(size_t index)
    {
        if (index >= count)
            return;
        
        for (size_t i = index; i < count - 1; i++)
        {
            data[i] = data[i + 1];
        }
        
        count--;
    }
    
    T& operator[](size_t index) { return data[index]; }
    const T& operator[](size_t index) const { return data[index]; }
    size_t size() const { return count; }
    size_t Count() const { return count; }
    bool empty() const { return count == 0; }
    void clear() { count = 0; }
};

inline char* StrDup(const char* s) {
    if (!s) return nullptr;
    size_t len = StrLen(s);
    char* result = (char*)PlatformAlloc(len + 1);
    StrCopy(result, s);
    return result;
}

struct SimpleString
{
    char *data;
    size_t length;
    size_t capacity;
};

struct ByteBuffer
{
    uint8_t *data;
    size_t size;
    size_t capacity;
};

struct LineArray
{
    SimpleString *lines;
    size_t count;
    size_t capacity;
};

inline void mem_copy(void* dst, const void* src, size_t n) {
    unsigned char*       d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
}

inline void mem_set(void* dst, int value, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    unsigned char v = (unsigned char)value;
    while (n--) {
        *d++ = v;
    }
}

inline void ss_init(SimpleString* s) {
    s->data     = NULL;
    s->length   = 0;
    s->capacity = 0;
}

inline void ss_free(SimpleString* s) {
    if (s->data) {
        sys_free(s->data);
        s->data = NULL;
    }
    s->length   = 0;
    s->capacity = 0;
}

inline bool ss_reserve(SimpleString* s, size_t newCap) {
    if (newCap <= s->capacity) return true;
    size_t allocSize = newCap + 1;
    char* newData = (char*)sys_realloc(s->data, allocSize);
    if (!newData) return false;
    s->data     = newData;
    s->capacity = newCap;
    if (s->length == 0) s->data[0] = '\0';
    return true;
}

inline bool ss_append_char(SimpleString* s, char c) {
    if (s->length + 1 >= s->capacity) {
        size_t newCap = s->capacity ? s->capacity * 2 : 32;
        if (!ss_reserve(s, newCap)) return false;
    }
    s->data[s->length++] = c;
    s->data[s->length]   = '\0';
    return true;
}

inline void ss_clear(SimpleString* s) {
    s->length = 0;
    if (s->data) s->data[0] = '\0';
}

inline bool ss_append_cstr(SimpleString* s, const char* str) {
    if (!str) return true;
    size_t len = 0;
    while (str[len] != '\0') ++len;

    if (s->length + len >= s->capacity) {
        size_t newCap = s->capacity ? s->capacity : 32;
        while (s->length + len >= newCap) newCap *= 2;
        if (!ss_reserve(s, newCap)) return false;
    }

    mem_copy(s->data + s->length, str, len);
    s->length += len;
    s->data[s->length] = '\0';
    return true;
}

inline bool la_reserve(LineArray* a, size_t newCap) {
    if (newCap <= a->capacity) return true;
    SimpleString* newLines =
        (SimpleString*)sys_realloc(a->lines, newCap * sizeof(SimpleString));
    if (!newLines) return false;

    for (size_t i = a->capacity; i < newCap; ++i) {
        ss_init(&newLines[i]);
    }

    a->lines    = newLines;
    a->capacity = newCap;
    return true;
}

inline bool la_push_back(LineArray* a, const SimpleString* s) {
    if (a->count >= a->capacity) {
        size_t newCap = a->capacity ? a->capacity * 2 : 16;
        if (!la_reserve(a, newCap)) return false;
    }

    SimpleString* dst = &a->lines[a->count];
    ss_clear(dst);
    if (s->length > 0) {
        if (!ss_reserve(dst, s->length)) return false;
        ss_append_cstr(dst, s->data);
    }
    ++a->count;
    return true;
}


inline bool la_push_back_cstr(LineArray* a, const char* str) {
    SimpleString tmp;
    ss_init(&tmp);
    ss_append_cstr(&tmp, str);
    bool ok = la_push_back(a, &tmp);
    ss_free(&tmp);
    return ok;
}


inline bool ss_append_dec2(SimpleString* s, unsigned int value) {
    char buf[3];
    buf[0] = (char)('0' + ((value / 10) % 10));
    buf[1] = (char)('0' + (value % 10));
    buf[2] = '\0';
    return ss_append_cstr(s, buf);
}

inline bool ss_append_hex8(SimpleString* s, unsigned int value) {
    char buf[9];
    for (int i = 7; i >= 0; --i) {
        unsigned int nibble = value & 0xF;
        buf[i] = (char)(nibble < 10 ? ('0' + nibble) : ('A' + (nibble - 10)));
        value >>= 4;
    }
    buf[8] = '\0';
    return ss_append_cstr(s, buf);
}

inline bool ss_append_hex2(SimpleString* s, unsigned int value) {
    char buf[3];
    for (int i = 1; i >= 0; --i) {
        unsigned int nibble = value & 0xF;
        buf[i] = (char)(nibble < 10 ? ('0' + nibble) : ('A' + (nibble - 10)));
        value >>= 4;
    }
    buf[2] = '\0';
    return ss_append_cstr(s, buf);
}


inline void bb_init(ByteBuffer* b) {
    b->data     = NULL;
    b->size     = 0;
    b->capacity = 0;
}

inline void bb_free(ByteBuffer* b) {
    if (b->data) {
        sys_free(b->data);
        b->data = NULL;
    }
    b->size     = 0;
    b->capacity = 0;
}

inline bool bb_resize(ByteBuffer* b, size_t newSize) {
    if (newSize > b->capacity) {
        size_t newCap = b->capacity ? b->capacity : 256;
        while (newCap < newSize) newCap *= 2;
        uint8_t* newData = (uint8_t*)sys_realloc(b->data, newCap);
        if (!newData) return false;
        b->data     = newData;
        b->capacity = newCap;
    }
    b->size = newSize;
    return true;
}

inline bool bb_empty(const ByteBuffer* b) {
    return b->size == 0;
}


inline void la_init(LineArray* a) {
    a->lines    = NULL;
    a->count    = 0;
    a->capacity = 0;
}

inline void la_clear(LineArray* a) {
    if (a->lines) {
        for (size_t i = 0; i < a->count; ++i) {
            ss_free(&a->lines[i]);
        }
    }
    a->count = 0;
}

inline void la_free(LineArray* a) {
    la_clear(a);
    if (a->lines) {
        sys_free(a->lines);
        a->lines = NULL;
    }
    a->capacity = 0;
}



#ifdef _WIN32
class WinString {
private:
    char* data;
    size_t len;
    size_t cap;

public:
    WinString() : data(nullptr), len(0), cap(0) {}
    
    ~WinString() {
        if (data) PlatformFree(data);
    }

    WinString(const WinString& other) : data(nullptr), len(0), cap(0) {
        if (other.len > 0) {
            Reserve(other.len);
            MemCopy(data, other.data, other.len);
            len = other.len;
            data[len] = 0;
        }
    }

    WinString& operator=(const WinString& other) {
        if (this != &other) {
            Clear();
            if (other.len > 0) {
                Reserve(other.len);
                MemCopy(data, other.data, other.len);
                len = other.len;
                data[len] = 0;
            }
        }
        return *this;
    }

    void Reserve(size_t newCap) {
        if (newCap <= cap) return;
        char* newData = (char*)PlatformAlloc(newCap + 1);
        if (data) {
            MemCopy(newData, data, len);
            PlatformFree(data);
        }
        data = newData;
        cap = newCap;
        data[len] = 0;
    }

    void Append(const char* str) {
        if (!str) return;
        size_t slen = StrLen(str);
        if (len + slen > cap) {
            Reserve((len + slen) * 2);
        }
        MemCopy(data + len, str, slen);
        len += slen;
        data[len] = 0;
    }

    void Append(char c) {
        if (len + 1 > cap) {
            Reserve((cap == 0 ? 16 : cap * 2));
        }
        data[len++] = c;
        data[len] = 0;
    }

    const char* CStr() const { return data ? data : ""; }
    size_t Length() const { return len; }
    bool IsEmpty() const { return len == 0; }
    
    void Clear() { 
        if (data) PlatformFree(data);
        data = nullptr;
        len = 0;
        cap = 0;
    }

    int Find(const char* needle) const {
        if (!data || !needle) return -1;
        size_t nlen = StrLen(needle);
        if (nlen == 0 || nlen > len) return -1;
        
        for (size_t i = 0; i <= len - nlen; i++) {
            bool match = true;
            for (size_t j = 0; j < nlen; j++) {
                if (data[i + j] != needle[j]) {
                    match = false;
                    break;
                }
            }
            if (match) return (int)i;
        }
        return -1;
    }

    WinString Substr(size_t start, size_t length) const {
        WinString result;
        if (!data || start >= len) return result;
        if (start + length > len) length = len - start;
        result.Reserve(length);
        MemCopy(result.data, data + start, length);
        result.len = length;
        result.data[length] = 0;
        return result;
    }
};
#endif
#endif
