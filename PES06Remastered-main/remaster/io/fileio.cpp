// remaster file-I/O logger - a diagnostic, off by default.
//
// Hooks CreateFileA, CreateFileW, ReadFile and WriteFile and records what
// the game opens, in which direction, and how much it moves. This is how the
// option file's real path and size were found without guessing.
//
// It only observes. Nothing is written to the game's files, no file handle is
// closed or reopened, and no call is blocked. The hook only reads the
// arguments it was given and forwards the call unchanged.
//
// Switch: [fileio] enabled = 0 in remaster.ini.

#include "fileio.h"
#include "../core/remaster.h"

#include "MinHook.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <shlobj.h>

namespace remaster {
namespace fileio {

#define MAX_TRACKED 512

typedef HANDLE(__stdcall* FnCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef HANDLE(__stdcall* FnCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
typedef BOOL(__stdcall* FnReadFile)(HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL(__stdcall* FnWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef FILE*(__cdecl* FnFopen)(const char*, const char*);
typedef FILE*(__cdecl* FnWfopen)(const wchar_t*, const wchar_t*);

static FnCreateFileA g_origCreateFileA = 0;
static FnCreateFileW g_origCreateFileW = 0;
static FnReadFile g_origReadFile = 0;
static FnWriteFile g_origWriteFile = 0;
static FnFopen g_origFopen = 0;
static FnWfopen g_origWfopen = 0;

static int g_enabled = 0;
static volatile LONG g_creates = 0;
static volatile LONG g_reads = 0;
static volatile LONG g_writes = 0;
static volatile LONG g_inHook = 0;

struct Tracked {
    HANDLE handle;
    char path[MAX_PATH];
    int used;
};

static Tracked g_tracked[MAX_TRACKED];
static int g_trackNext = 0;

static void remember(HANDLE h, const char* path) {
    for (int i = 0; i < MAX_TRACKED; i++) {
        if (g_tracked[i].used && g_tracked[i].handle == h) {
            strncpy(g_tracked[i].path, path, MAX_PATH - 1);
            g_tracked[i].path[MAX_PATH - 1] = 0;
            return;
        }
    }
    Tracked& t = g_tracked[g_trackNext];
    g_trackNext = (g_trackNext + 1) % MAX_TRACKED;
    t.handle = h;
    t.used = 1;
    strncpy(t.path, path, MAX_PATH - 1);
    t.path[MAX_PATH - 1] = 0;
}

static const char* recall(HANDLE h) {
    for (int i = 0; i < MAX_TRACKED; i++) {
        if (g_tracked[i].used && g_tracked[i].handle == h) {
            return g_tracked[i].path;
        }
    }
    return "<unknown>";
}

static unsigned int crc32(const void* data, size_t n) {
    static unsigned int table[256];
    static int ready = 0;
    if (!ready) {
        for (unsigned int i = 0; i < 256; i++) {
            unsigned int c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        ready = 1;
    }
    const unsigned char* p = (const unsigned char*)data;
    unsigned int c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c = (c >> 8) ^ table[(c ^ p[i]) & 0xFF];
    }
    return c ^ 0xFFFFFFFFu;
}

static const char* accessName(DWORD a) {
    if (a & GENERIC_READ) {
        return "read";
    }
    if (a & GENERIC_WRITE) {
        return "write";
    }
    return "other";
}

static HANDLE __stdcall detourCreateFileA(LPCSTR path, DWORD access, DWORD share,
                                          LPSECURITY_ATTRIBUTES sa, DWORD disposition,
                                          DWORD flags, HANDLE tmpl) {
    HANDLE h = g_origCreateFileA(path, access, share, sa, disposition, flags, tmpl);
    if (g_enabled && path && InterlockedIncrement(&g_inHook) == 1) {
        remember(h, path);
        __try {
            log().write("INFO", "fileio: create \"%s\" access=%s disposition=%lu -> %p",
                        path, accessName(access), (unsigned long)disposition, (void*)h);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return h;
}

static HANDLE __stdcall detourCreateFileW(LPCWSTR path, DWORD access, DWORD share,
                                          LPSECURITY_ATTRIBUTES sa, DWORD disposition,
                                          DWORD flags, HANDLE tmpl) {
    HANDLE h = g_origCreateFileW(path, access, share, sa, disposition, flags, tmpl);
    if (g_enabled && path && InterlockedIncrement(&g_inHook) == 1) {
        char narrow[MAX_PATH];
        narrow[0] = 0;
        WideCharToMultiByte(CP_ACP, 0, path, -1, narrow, MAX_PATH, 0, 0);
        remember(h, narrow);
        __try {
            log().write("INFO", "fileio: createW \"%s\" access=%s disposition=%lu -> %p",
                        narrow, accessName(access), (unsigned long)disposition, (void*)h);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return h;
}

static BOOL __stdcall detourReadFile(HANDLE h, LPVOID buffer, DWORD toRead,
                                     LPDWORD read, LPOVERLAPPED overlapped) {
    BOOL ok = g_origReadFile(h, buffer, toRead, read, overlapped);
    if (g_enabled && ok && buffer && read && *read && InterlockedIncrement(&g_inHook) == 1) {
        unsigned int n = *read < 64 ? *read : 64;
        __try {
            log().write("INFO", "fileio: read \"%s\" %lu bytes, first %u hash=0x%08X",
                        recall(h), (unsigned long)*read, (unsigned)n,
                        crc32(buffer, n));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return ok;
}

static BOOL __stdcall detourWriteFile(HANDLE h, LPCVOID buffer, DWORD toWrite,
                                      LPDWORD written, LPOVERLAPPED overlapped) {
    BOOL ok = g_origWriteFile(h, buffer, toWrite, written, overlapped);
    if (g_enabled && ok && buffer && written && *written && InterlockedIncrement(&g_inHook) == 1) {
        unsigned int n = *written < 64 ? *written : 64;
        LARGE_INTEGER size;
        size.QuadPart = 0;
        GetFileSizeEx(h, &size);
        __try {
            log().write("INFO", "fileio: write \"%s\" %lu bytes (size now %lld), first %u hash=0x%08X",
                        recall(h), (unsigned long)*written, (long long)size.QuadPart,
                        (unsigned)n, crc32(buffer, n));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return ok;
}

// The game reaches its own files through the C runtime, not through
// CreateFile directly, so both are hooked.
static FILE* __cdecl detourFopen(const char* path, const char* mode) {
    FILE* f = g_origFopen(path, mode);
    if (g_enabled && path && InterlockedIncrement(&g_inHook) == 1) {
        InterlockedIncrement(&g_creates);
        __try {
            log().write("INFO", "fileio: fopen \"%s\" mode=\"%s\" -> %p", path, mode ? mode : "?", (void*)f);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return f;
}

static FILE* __cdecl detourWfopen(const wchar_t* path, const wchar_t* mode) {
    FILE* f = g_origWfopen(path, mode);
    if (g_enabled && path && InterlockedIncrement(&g_inHook) == 1) {
        InterlockedIncrement(&g_creates);
        char narrow[MAX_PATH];
        char narrowMode[16];
        narrow[0] = 0;
        narrowMode[0] = 0;
        WideCharToMultiByte(CP_ACP, 0, path, -1, narrow, MAX_PATH, 0, 0);
        if (mode) {
            WideCharToMultiByte(CP_ACP, 0, mode, -1, narrowMode, 16, 0, 0);
        }
        __try {
            log().write("INFO", "fileio: wfopen \"%s\" mode=\"%s\" -> %p", narrow, narrowMode, (void*)f);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
        InterlockedDecrement(&g_inHook);
    }
    return f;
}

// ---------------------------------------------------------------------------
// Folder poller
//
// The game decodes its own code at run time and resolves the C runtime
// entry points itself, so the CreateFile / fopen hooks never see its data
// files. The save path is known instead: the game builds it at run time from
//   "Pro Evolution Soccer 6" + "\KONAMI\"  ->  a global string at 0x00BC4990
// and appends "/save", giving
//
//   <Documents>\KONAMI\Pro Evolution Soccer 6\save\
//
// so the folder is watched directly. Every file that appears or changes is
// reported with its path, its size and a CRC32 of its first 64 bytes. This is
// read-only: the folder is listed and read, never written.

#define MAX_SEEN 64

struct SeenFile {
    char path[MAX_PATH];
    long long size;
    unsigned int hash;
    int used;
};

static SeenFile g_seen[MAX_SEEN];
static int g_seenCount = 0;
static char g_saveFolder[MAX_PATH] = { 0 };
static int g_folderReady = 0;

static unsigned int hashFirstBytes(const char* path, long long* sizeOut) {
    *sizeOut = -1;
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (h == INVALID_HANDLE_VALUE) {
        return 0;
    }
    LARGE_INTEGER li;
    unsigned int hash = 0;
    if (GetFileSizeEx(h, &li)) {
        *sizeOut = li.QuadPart;
        unsigned char buf[64];
        DWORD got = 0;
        if (ReadFile(h, buf, 64, &got, 0) && got) {
            hash = crc32(buf, got);
        }
    }
    CloseHandle(h);
    return hash;
}

static void buildSaveFolder() {
    if (g_folderReady) {
        return;
    }
    g_folderReady = 1;
    char documents[MAX_PATH];
    documents[0] = 0;
    if (SUCCEEDED(SHGetFolderPathA(0, CSIDL_PERSONAL, 0, 0, documents))) {
        snprintf(g_saveFolder, sizeof(g_saveFolder),
                 "%s\\KONAMI\\Pro Evolution Soccer 6\\save", documents);
    }
    else {
        snprintf(g_saveFolder, sizeof(g_saveFolder),
                 "%s\\KONAMI\\Pro Evolution Soccer 6\\save", documents);
    }
}

static void pollSaveFolder() {
    buildSaveFolder();
    if (!g_saveFolder[0]) {
        return;
    }
    WIN32_FIND_DATAA fd;
    char pattern[MAX_PATH];
    snprintf(pattern, sizeof(pattern), "%s\\*", g_saveFolder);
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        char full[MAX_PATH];
        snprintf(full, sizeof(full), "%s\\%s", g_saveFolder, fd.cFileName);
        long long size = -1;
        unsigned int hash = hashFirstBytes(full, &size);
        int known = 0;
        for (int i = 0; i < g_seenCount; i++) {
            if (strcmp(g_seen[i].path, full) == 0) {
                known = 1;
                if (g_seen[i].size != size || g_seen[i].hash != hash) {
                    g_seen[i].size = size;
                    g_seen[i].hash = hash;
                    __try {
                        log().write("INFO", "fileio: changed \"%s\" size=%lld hash64=0x%08X", full, size, hash);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                    }
                }
                break;
            }
        }
        if (!known && g_seenCount < MAX_SEEN) {
            SeenFile& s = g_seen[g_seenCount++];
            strncpy(s.path, full, MAX_PATH - 1);
            s.path[MAX_PATH - 1] = 0;
            s.size = size;
            s.hash = hash;
            __try {
                log().write("INFO", "fileio: found \"%s\" size=%lld hash64=0x%08X", full, size, hash);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

bool install() {
    if (!config().getBool("fileio", "enabled", false)) {
        log().write("INFO", "fileio: disabled by config");
        return true;
    }
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    if (!kernel) {
        log().write("WARN", "fileio: kernel32 not found");
        return false;
    }
    struct {
        const char* name;
        void** slot;
        void* detour;
    } hooks[] = {
        { "CreateFileA", (void**)&g_origCreateFileA, (void*)detourCreateFileA },
        { "CreateFileW", (void**)&g_origCreateFileW, (void*)detourCreateFileW },
        { "ReadFile", (void**)&g_origReadFile, (void*)detourReadFile },
        { "WriteFile", (void**)&g_origWriteFile, (void*)detourWriteFile },
    };
    int ok = 0;
    for (int i = 0; i < 4; i++) {
        void* target = (void*)GetProcAddress(kernel, hooks[i].name);
        if (!target) {
            continue;
        }
        if (MH_CreateHook(target, hooks[i].detour, hooks[i].slot) == MH_OK &&
            MH_EnableHook(target) == MH_OK) {
            ok++;
        }
    }
    // the C runtime entry points, which is how the game actually opens files
    HMODULE msvcrt = GetModuleHandleA("msvcrt.dll");
    if (!msvcrt) {
        msvcrt = LoadLibraryA("msvcrt.dll");
    }
    if (msvcrt) {
        void* fp = (void*)GetProcAddress(msvcrt, "fopen");
        if (fp && MH_CreateHook(fp, (void*)detourFopen, (void**)&g_origFopen) == MH_OK &&
            MH_EnableHook(fp) == MH_OK) {
            ok++;
        }
        void* wp = (void*)GetProcAddress(msvcrt, "_wfopen");
        if (wp && MH_CreateHook(wp, (void*)detourWfopen, (void**)&g_origWfopen) == MH_OK &&
            MH_EnableHook(wp) == MH_OK) {
            ok++;
        }
    }
    if (!ok) {
        log().write("WARN", "fileio: could not hook any of the file functions");
        return false;
    }
    g_enabled = 1;
    log().write("INFO", "fileio: %d file hooks installed", ok);
    buildSaveFolder();
    log().write("INFO", "fileio: watching \"%s\"", g_saveFolder);
    return true;
}

void poll() {
    if (g_enabled) {
        pollSaveFolder();
    }
}

void shutdown() {
    g_enabled = 0;
    log().write("INFO", "fileio: %ld creates, %ld reads, %ld writes",
                (long)g_creates, (long)g_reads, (long)g_writes);
}

}  // namespace fileio
}  // namespace remaster
