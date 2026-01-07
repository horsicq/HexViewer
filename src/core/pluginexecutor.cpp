#include "pluginexecutor.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

static bool pythonInitialized = false;

typedef void (*PyErrPrintFunc)();
typedef void *(*PyErrOccurredFunc)();
typedef void (*PyErrClearFunc)();

static PyErrPrintFunc PyErr_Print = nullptr;
static PyErrOccurredFunc PyErr_Occurred = nullptr;
static PyErrClearFunc PyErr_Clear = nullptr;

typedef void (*PyInitFunc)();
typedef void (*PyFinalFunc)();
typedef int (*PyRunStringFunc)(const char *);
typedef void *(*PyImportFunc)(const char *);
typedef void *(*PyGetAttrFunc)(void *, const char *);
typedef void *(*PyCallFunc)(void *, void *);
typedef void *(*PyBytesFunc)(const char *, long long);
typedef void *(*PyLongFunc)(long long);
typedef void *(*PyTupleFunc)(long long);
typedef int (*PyTupleSetFunc)(void *, long long, void *);
typedef void (*PyDecRefFunc)(void *);
typedef long long (*PyListSizeFunc)(void *);
typedef void *(*PyListGetFunc)(void *, long long);
typedef void *(*PyDictGetFunc)(void *, const char *);
typedef char *(*PyStrFunc)(void *);
typedef void *(*PyUnicodeFromStringFunc)(const char *);

static PyInitFunc Py_Initialize = nullptr;
static PyFinalFunc Py_Finalize = nullptr;
static PyRunStringFunc PyRun_SimpleString = nullptr;
static PyImportFunc PyImport_ImportModule = nullptr;
static PyGetAttrFunc PyObject_GetAttrString = nullptr;
static PyCallFunc PyObject_CallObject = nullptr;
static PyBytesFunc PyBytes_FromStringAndSize = nullptr;
static PyLongFunc PyLong_FromLongLong = nullptr;
static PyTupleFunc PyTuple_New = nullptr;
static PyTupleSetFunc PyTuple_SetItem = nullptr;
static PyDecRefFunc Py_DecRef = nullptr;
static PyListSizeFunc PyList_Size = nullptr;
static PyListGetFunc PyList_GetItem = nullptr;
static PyDictGetFunc PyDict_GetItemString = nullptr;
static PyStrFunc PyUnicode_AsUTF8 = nullptr;
static PyUnicodeFromStringFunc PyUnicode_FromString = nullptr;

#ifdef _WIN32
static HMODULE pythonDLL = nullptr;
#else
static void *pythonLib = nullptr;
#endif

void GetPluginDirectory(char *outPath, int maxLen);

#ifdef _WIN32
HMODULE LoadPythonFromRegistry()
{
    const wchar_t *roots[] = {
        L"SOFTWARE\\Python\\PythonCore",
        L"SOFTWARE\\WOW6432Node\\Python\\PythonCore",
        nullptr
    };

    HKEY hRoot = nullptr;
    HKEY hVersion = nullptr;
    HKEY hInstallPath = nullptr;

    wchar_t subKey[256];
    wchar_t installPath[512];
    DWORD installPathSize;
    FILETIME ft;

    for (int r = 0; roots[r]; r++)
    {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, roots[r], 0, KEY_READ, &hRoot) != ERROR_SUCCESS &&
            RegOpenKeyExW(HKEY_CURRENT_USER, roots[r], 0, KEY_READ, &hRoot) != ERROR_SUCCESS)
        {
            continue;
        }

        DWORD index = 0;
        while (true)
        {
            DWORD subKeyLen = sizeof(subKey) / sizeof(wchar_t);
            LONG res = RegEnumKeyExW(hRoot, index++, subKey, &subKeyLen, nullptr, nullptr, nullptr, &ft);
            if (res != ERROR_SUCCESS)
                break;

            if (RegOpenKeyExW(hRoot, subKey, 0, KEY_READ, &hVersion) != ERROR_SUCCESS)
                continue;

            if (RegOpenKeyExW(hVersion, L"InstallPath", 0, KEY_READ, &hInstallPath) != ERROR_SUCCESS)
            {
                RegCloseKey(hVersion);
                continue;
            }

            installPathSize = sizeof(installPath);
            if (RegQueryValueExW(hInstallPath, nullptr, nullptr, nullptr,
                                 (LPBYTE)installPath, &installPathSize) == ERROR_SUCCESS)
            {
                wchar_t ver[16];
                int vi = 0;
                for (int si = 0; subKey[si] != 0; si++)
                {
                    if (subKey[si] != L'.')
                        ver[vi++] = subKey[si];
                }
                ver[vi] = 0;

                wchar_t dllPath[600];
                wsprintfW(dllPath, L"%spython%s.dll", installPath, ver);

                HMODULE h = LoadLibraryW(dllPath);
                if (h)
                {
                    RegCloseKey(hInstallPath);
                    RegCloseKey(hVersion);
                    RegCloseKey(hRoot);
                    return h;
                }
            }

            RegCloseKey(hInstallPath);
            RegCloseKey(hVersion);
        }

        RegCloseKey(hRoot);
    }

    return nullptr;
}

#endif

bool InitializePythonRuntime()
{
    if (pythonInitialized)
        return true;

#ifdef _WIN32
    HMODULE LoadPythonFromRegistry();
    pythonDLL = LoadPythonFromRegistry();
    if (!pythonDLL)
    {
        MessageBoxA(NULL,
                    "Could not load Python DLL from registry!\n\n"
                    "Please install Python 3.8+ from python.org\n"
                    "Make sure to check 'Add Python to PATH' during installation.",
                    "Python Error",
                    MB_OK | MB_ICONERROR);
        return false;
    }

    Py_Initialize = (PyInitFunc)GetProcAddress(pythonDLL, "Py_Initialize");
    Py_Finalize = (PyFinalFunc)GetProcAddress(pythonDLL, "Py_Finalize");
    PyRun_SimpleString = (PyRunStringFunc)GetProcAddress(pythonDLL, "PyRun_SimpleString");
    PyImport_ImportModule = (PyImportFunc)GetProcAddress(pythonDLL, "PyImport_ImportModule");
    PyObject_GetAttrString = (PyGetAttrFunc)GetProcAddress(pythonDLL, "PyObject_GetAttrString");
    PyObject_CallObject = (PyCallFunc)GetProcAddress(pythonDLL, "PyObject_CallObject");
    PyBytes_FromStringAndSize = (PyBytesFunc)GetProcAddress(pythonDLL, "PyBytes_FromStringAndSize");
    PyLong_FromLongLong = (PyLongFunc)GetProcAddress(pythonDLL, "PyLong_FromLongLong");
    PyTuple_New = (PyTupleFunc)GetProcAddress(pythonDLL, "PyTuple_New");
    PyTuple_SetItem = (PyTupleSetFunc)GetProcAddress(pythonDLL, "PyTuple_SetItem");
    Py_DecRef = (PyDecRefFunc)GetProcAddress(pythonDLL, "Py_DecRef");
    PyList_Size = (PyListSizeFunc)GetProcAddress(pythonDLL, "PyList_Size");
    PyList_GetItem = (PyListGetFunc)GetProcAddress(pythonDLL, "PyList_GetItem");
    PyDict_GetItemString = (PyDictGetFunc)GetProcAddress(pythonDLL, "PyDict_GetItemString");
    PyUnicode_AsUTF8 = (PyStrFunc)GetProcAddress(pythonDLL, "PyUnicode_AsUTF8");
    PyUnicode_FromString = (PyUnicodeFromStringFunc)GetProcAddress(pythonDLL, "PyUnicode_FromString");

    PyErr_Print = (PyErrPrintFunc)GetProcAddress(pythonDLL, "PyErr_Print");
    PyErr_Occurred = (PyErrOccurredFunc)GetProcAddress(pythonDLL, "PyErr_Occurred");
    PyErr_Clear = (PyErrClearFunc)GetProcAddress(pythonDLL, "PyErr_Clear");

#else
    const char *libNames[] = {
        "libpython3.12.so", "libpython3.11.so", "libpython3.10.so",
        "libpython3.9.so", "libpython3.so", nullptr};

    for (int i = 0; libNames[i]; i++)
    {
        pythonLib = dlopen(libNames[i], RTLD_NOW | RTLD_GLOBAL);
        if (pythonLib)
            break;
    }

    if (!pythonLib)
        return false;

    Py_Initialize = (PyInitFunc)dlsym(pythonLib, "Py_Initialize");
    Py_Finalize = (PyFinalFunc)dlsym(pythonLib, "Py_Finalize");
    PyRun_SimpleString = (PyRunStringFunc)dlsym(pythonLib, "PyRun_SimpleString");
    PyImport_ImportModule = (PyImportFunc)dlsym(pythonLib, "PyImport_ImportModule");
    PyObject_GetAttrString = (PyGetAttrFunc)dlsym(pythonLib, "PyObject_GetAttrString");
    PyObject_CallObject = (PyCallFunc)dlsym(pythonLib, "PyObject_CallObject");
    PyBytes_FromStringAndSize = (PyBytesFunc)dlsym(pythonLib, "PyBytes_FromStringAndSize");
    PyLong_FromLongLong = (PyLongFunc)dlsym(pythonLib, "PyLong_FromLongLong");
    PyTuple_New = (PyTupleFunc)dlsym(pythonLib, "PyTuple_New");
    PyTuple_SetItem = (PyTupleSetFunc)dlsym(pythonLib, "PyTuple_SetItem");
    Py_DecRef = (PyDecRefFunc)dlsym(pythonLib, "Py_DecRef");
    PyList_Size = (PyListSizeFunc)dlsym(pythonLib, "PyList_Size");
    PyList_GetItem = (PyListGetFunc)dlsym(pythonLib, "PyList_GetItem");
    PyDict_GetItemString = (PyDictGetFunc)dlsym(pythonLib, "PyDict_GetItemString");
    PyUnicode_AsUTF8 = (PyStrFunc)dlsym(pythonLib, "PyUnicode_AsUTF8");
    PyUnicode_FromString = (PyUnicodeFromStringFunc)dlsym(pythonLib, "PyUnicode_FromString");

    PyErr_Print = (PyErrPrintFunc)dlsym(pythonLib, "PyErr_Print");
    PyErr_Occurred = (PyErrOccurredFunc)dlsym(pythonLib, "PyErr_Occurred");
    PyErr_Clear = (PyErrClearFunc)dlsym(pythonLib, "PyErr_Clear");
#endif

    if (!Py_Initialize)
        return false;

    Py_Initialize();

    char pluginDir[512];
    GetPluginDirectory(pluginDir, 512);

    char cmd[1024];
    int pos = 0;
    const char *prefix = "import sys; sys.path.insert(0, '";
    while (*prefix)
        cmd[pos++] = *prefix++;

    for (int i = 0; pluginDir[i]; i++)
    {
        if (pluginDir[i] == '\\')
        {
            cmd[pos++] = '\\';
            cmd[pos++] = '\\';
        }
        else
        {
            cmd[pos++] = pluginDir[i];
        }
    }
    cmd[pos++] = '\'';
    cmd[pos++] = ')';
    cmd[pos] = '\0';

    PyRun_SimpleString(cmd);

    pythonInitialized = true;
    return true;
}

void ShutdownPythonRuntime()
{
    if (pythonInitialized && Py_Finalize)
    {
        Py_Finalize();
        pythonInitialized = false;
    }

#ifdef _WIN32
    if (pythonDLL)
    {
        FreeLibrary(pythonDLL);
        pythonDLL = nullptr;
    }
#else
    if (pythonLib)
    {
        dlclose(pythonLib);
        pythonLib = nullptr;
    }
#endif
}

bool GetPythonPluginInfo(const char *pluginPath, PluginInfo *info)
{
    if (!pythonInitialized)
    {
        if (!InitializePythonRuntime())
            return false;
    }

    char moduleName[256];
    ExtractModuleName(pluginPath, moduleName, 256);

    void *pModule = PyImport_ImportModule(moduleName);
    if (!pModule)
    {
        if (PyErr_Clear)
            PyErr_Clear();
        return false;
    }

    void *pFunc = PyObject_GetAttrString(pModule, "get_info");
    if (!pFunc)
    {
        Py_DecRef(pModule);
        return false;
    }

    void *pResult = PyObject_CallObject(pFunc, nullptr);
    
    if (pResult && PyDict_GetItemString)
    {
        void *pName = PyDict_GetItemString(pResult, "name");
        void *pVersion = PyDict_GetItemString(pResult, "version");
        void *pAuthor = PyDict_GetItemString(pResult, "author");
        void *pDesc = PyDict_GetItemString(pResult, "description");

        if (pName && PyUnicode_AsUTF8)
        {
            char *name = PyUnicode_AsUTF8(pName);
            if (name)
                StrCopy(info->name, name);
        }

        if (pVersion && PyUnicode_AsUTF8)
        {
            char *version = PyUnicode_AsUTF8(pVersion);
            if (version)
                StrCopy(info->version, version);
        }

        if (pAuthor && PyUnicode_AsUTF8)
        {
            char *author = PyUnicode_AsUTF8(pAuthor);
            if (author)
                StrCopy(info->author, author);
        }

        if (pDesc && PyUnicode_AsUTF8)
        {
            char *desc = PyUnicode_AsUTF8(pDesc);
            if (desc)
                StrCopy(info->description, desc);
        }

        Py_DecRef(pResult);
    }

    Py_DecRef(pFunc);
    Py_DecRef(pModule);

    return true;
}

static void ExtractModuleName(const char *pluginPath, char *moduleName, int maxLen)
{
    const char *lastSlash = pluginPath;
    for (const char *p = pluginPath; *p; p++)
    {
        if (*p == '\\' || *p == '/')
            lastSlash = p + 1;
    }

    int i = 0;
    while (lastSlash[i] && lastSlash[i] != '.' && i < maxLen - 1)
    {
        moduleName[i] = lastSlash[i];
        i++;
    }
    moduleName[i] = '\0';
}

bool CanPluginDisassemble(const char *pluginPath)
{
    if (!pythonInitialized)
    {
        if (!InitializePythonRuntime())
            return false;
    }

    char moduleName[256];
    ExtractModuleName(pluginPath, moduleName, 256);

    void *pModule = PyImport_ImportModule(moduleName);
    if (!pModule)
    {
        if (PyErr_Occurred && PyErr_Occurred())
        {
            if (PyErr_Print)
                PyErr_Print();
            if (PyErr_Clear)
                PyErr_Clear();
        }

        return false;
    }

    void *pFunc = PyObject_GetAttrString(pModule, "disassemble");
    bool canDisasm = (pFunc != nullptr);

    if (!canDisasm)
    {
       
    }

    if (pFunc)
        Py_DecRef(pFunc);

    Py_DecRef(pModule);
    return canDisasm;
}


bool CanPluginAnalyze(const char *pluginPath)
{
    if (!pythonInitialized)
    {
        if (!InitializePythonRuntime())
            return false;
    }

    char moduleName[256];
    ExtractModuleName(pluginPath, moduleName, 256);

    void *pModule = PyImport_ImportModule(moduleName);
    if (!pModule)
    {
        if (PyErr_Clear)
            PyErr_Clear();
        return false;
    }

    void *pFunc = PyObject_GetAttrString(pModule, "analyze");
    bool canAnalyze = (pFunc != nullptr);

    if (pFunc)
        Py_DecRef(pFunc);
    Py_DecRef(pModule);

    return canAnalyze;
}

bool CanPluginTransform(const char *pluginPath)
{
    if (!pythonInitialized)
    {
        if (!InitializePythonRuntime())
            return false;
    }

    char moduleName[256];
    ExtractModuleName(pluginPath, moduleName, 256);

    void *pModule = PyImport_ImportModule(moduleName);
    if (!pModule)
    {
        if (PyErr_Clear)
            PyErr_Clear();
        return false;
    }

    void *pFunc = PyObject_GetAttrString(pModule, "transform");
    bool canTransform = (pFunc != nullptr);

    if (pFunc)
        Py_DecRef(pFunc);
    Py_DecRef(pModule);

    return canTransform;
}

bool ExecutePythonDisassembly(
    const char *pluginPath,
    const uint8_t *data,
    size_t dataSize,
    size_t offset,
    LineArray *outLines)
{
    if (!pythonInitialized)
    {
        if (!InitializePythonRuntime())
            return false;
    }

    char moduleName[256];
    ExtractModuleName(pluginPath, moduleName, 256);

    void *pModule = PyImport_ImportModule(moduleName);
    if (!pModule)
        return false;

    void *pFunc = PyObject_GetAttrString(pModule, "disassemble");
    if (!pFunc)
    {
        Py_DecRef(pModule);
        return false;
    }

    void *pArgs = PyTuple_New(4);

    void *pData = PyBytes_FromStringAndSize((const char *)data, (long long)dataSize);
    PyTuple_SetItem(pArgs, 0, pData);

    void *pOffset = PyLong_FromLongLong((long long)offset);
    PyTuple_SetItem(pArgs, 1, pOffset);

    void *pArch = PyUnicode_FromString("x64");
    PyTuple_SetItem(pArgs, 2, pArch);

    void *pMaxInst = PyLong_FromLongLong(1);
    PyTuple_SetItem(pArgs, 3, pMaxInst);

    void *pResult = PyObject_CallObject(pFunc, pArgs);

    if (pResult && PyList_Size)
    {
        long long listSize = PyList_Size(pResult);

        for (long long j = 0; j < listSize; j++)
        {
            void *pItem = PyList_GetItem(pResult, j);

            void *pMnem = PyDict_GetItemString(pItem, "mnemonic");
            void *pOps = PyDict_GetItemString(pItem, "operands");

            if (pMnem && PyUnicode_AsUTF8)
            {
                char *mnem = PyUnicode_AsUTF8(pMnem);
                char *ops = pOps ? PyUnicode_AsUTF8(pOps) : nullptr;

                SimpleString line;
                extern void ss_init(SimpleString *);
                extern bool ss_append_cstr(SimpleString *, const char *);
                extern bool ss_append_char(SimpleString *, char);
                extern void ss_free(SimpleString *);
                extern bool la_push_back(LineArray *, const SimpleString *);

                ss_init(&line);
                ss_append_cstr(&line, mnem);
                if (ops && ops[0])
                {
                    ss_append_char(&line, ' ');
                    ss_append_cstr(&line, ops);
                }

                la_push_back(outLines, &line);
                ss_free(&line);
            }
        }

        Py_DecRef(pResult);
    }

    Py_DecRef(pArgs);
    Py_DecRef(pFunc);
    Py_DecRef(pModule);

    return outLines->count > 0;
}
