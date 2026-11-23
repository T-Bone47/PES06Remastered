// remaster assets subsystem - AFS asset replacement.
//
// The game reads its assets from dat/<lang>_text.afs, inflating each entry
// with Konami's unzlib. Replacing the *compressed* bytes is not viable: the
// game rejects a modified archive outright, so the substitution has to happen
// after inflation.
//
// This hook sits on the inflate routine. The game inflates normally; we then
// look at the inflated buffer, identify it by CRC32, and - only if we have a
// replacement for exactly that asset - copy the replacement over it.
//
// Safety: if the CRC does not match, nothing is touched. A wrong guess
// therefore cannot corrupt the game; it just does nothing.
//
// Pack layout, next to the game executable:
//
//     remaster_pack/index.txt     "<crc32 hex> <filename>" per line
//     remaster_pack/<files>        the replacement asset, already inflated
//
// Replacement files hold the *inflated* form of the asset.

#include "assets.h"
#include "../core/remaster.h"

#ifndef REMASTER_NO_MINHOOK
#include "MinHook.h"
#endif

#include <stdio.h>
#include <string.h>

namespace remaster {
namespace assets {

#define MAX_PACK_ENTRIES 256

typedef int(__cdecl* FnUnpack)(void*, void*, void*, void*, int*);

static FnUnpack g_origUnpack = 0;

struct PackEntry {
    unsigned int crc;
    char file[128];
    unsigned int size;
};

static int g_logAll = 0;
static PackEntry g_pack[MAX_PACK_ENTRIES];
static int g_packCount = 0;
static int g_loaded = 0;

// ---------------------------------------------------------------- crc32

static unsigned int g_crcTable[256];
static int g_crcReady = 0;

static void initCrc() {
    if (g_crcReady) {
        return;
    }
    for (unsigned int i = 0; i < 256; i++) {
        unsigned int c = i;
        for (int k = 0; k < 8; k++) {
            c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        g_crcTable[i] = c;
    }
    g_crcReady = 1;
}

static unsigned int crc32(const void* data, size_t len) {
    initCrc();
    const unsigned char* p = (const unsigned char*)data;
    unsigned int c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        c = g_crcTable[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}

// ---------------------------------------------------------------- pack

static bool loadPack() {
    if (g_loaded) {
        return g_packCount > 0;
    }
    g_loaded = 1;
    g_logAll = config().getBool("assets", "log_all", false);

    char* indexPath = gamePath("remaster_pack\\index.txt");
    if (!indexPath) {
        return false;
    }
    FILE* f = fopen(indexPath, "r");
    if (!f) {
        log().write("INFO", "assets: no pack index at %s", indexPath);
        return false;
    }
    char line[512];
    while (fgets(line, sizeof(line), f) && g_packCount < MAX_PACK_ENTRIES) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == 0 || *p == '#' || *p == '\n' || *p == '\r') {
            continue;
        }
        unsigned int crc = (unsigned int)strtoul(p, &p, 16);
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        char* nl = strpbrk(p, "\r\n");
        if (nl) {
            *nl = 0;
        }
        if (!*p) {
            continue;
        }
        PackEntry& e = g_pack[g_packCount];
        e.crc = crc;
        strncpy(e.file, p, sizeof(e.file) - 1);
        e.file[sizeof(e.file) - 1] = 0;
        e.size = 0;
        g_packCount++;
    }
    fclose(f);
    log().write("INFO", "assets: pack index loaded, %d entries", g_packCount);
    return g_packCount > 0;
}

static const PackEntry* findPack(unsigned int crc) {
    for (int i = 0; i < g_packCount; i++) {
        if (g_pack[i].crc == crc) {
            return &g_pack[i];
        }
    }
    return 0;
}

bool isPathSafe(const char* file) {
    if (!file || !*file) {
        return false;
    }
    // Reject absolute paths, drive letters, UNC paths
    if (file[0] == '/' || file[0] == '\\' || strchr(file, ':') != nullptr) {
        return false;
    }
    // Reject directory traversal
    if (strstr(file, "..") != nullptr) {
        return false;
    }
    return true;
}

bool replaceAssetTransactional(void* dest, unsigned int destSize, const char* fullPath, unsigned int* bytesWritten) {
    if (!dest || destSize == 0 || !fullPath || !*fullPath) {
        return false;
    }
    FILE* f = fopen(fullPath, "rb");
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || (unsigned int)n > destSize) {
        fclose(f);
        return false;
    }
    // Read into local scratch buffer first to guarantee transactional replacement
    void* temp = malloc((size_t)n);
    if (!temp) {
        fclose(f);
        return false;
    }
    size_t got = fread(temp, 1, (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) {
        // Read failed or partial read; do not touch destination buffer!
        free(temp);
        return false;
    }
    memcpy(dest, temp, (size_t)n);
    free(temp);
    if (bytesWritten) {
        *bytesWritten = (unsigned int)got;
    }
    return true;
}

// ---------------------------------------------------------------- hook

static int g_replaced = 0;
static int g_checked = 0;

static int __cdecl detourUnpack(void* a1, void* dest, void* a2, void* a3, int* sizeOut) {
    int ret = g_origUnpack(a1, dest, a2, a3, sizeOut);
    if (!dest || !sizeOut) {
        return ret;
    }
    unsigned int size = (unsigned int)(*sizeOut);
    if (size == 0 || size > (32u << 20)) {
        return ret;
    }
    g_checked++;
    unsigned int crc = crc32(dest, size);
    if (g_logAll) {
        char head[80];
        unsigned int n = size < 12 ? size : 12;
        for (unsigned int i = 0; i < n; i++) {
            sprintf(head + i * 2, "%02x", ((unsigned char*)dest)[i]);
        }
        head[n * 2] = 0;
        log().write("INFO", "assets: inflated size=%u crc=0x%08X head=%s", size, crc, head);
    }
    const PackEntry* e = findPack(crc);
    if (!e) {
        return ret;
    }
    if (!isPathSafe(e->file)) {
        log().write("WARN", "assets: unsafe path in pack entry: %s", e->file);
        return ret;
    }

    char path[MAX_PATH * 2];
    snprintf(path, sizeof(path), "remaster_pack\\%s", e->file);
    char* full = gamePath(path);
    if (!full) {
        return ret;
    }
    unsigned int written = 0;
    if (replaceAssetTransactional(dest, size, full, &written)) {
        g_replaced++;
        log().write("INFO", "assets: replaced crc 0x%08X with %s (%u bytes)", crc, e->file, written);
    } else {
        log().write("WARN", "assets: transactional replacement failed for crc 0x%08X: %s", crc, e->file);
    }
    return ret;
}

bool install() {
    if (!config().getBool("assets", "enabled", false)) {
        log().write("INFO", "assets: disabled by config");
        return true;
    }
    if (!loadPack()) {
        log().write("INFO", "assets: no pack loaded - the hook will only observe");
    }

    unsigned int addr = manifest().getAddr("afs.unpack");
    if (!addr) {
        log().write("WARN", "assets: manifest has no afs.unpack entry");
        return false;
    }
    if (!hooks().install("assets.unpack", (void*)addr, (void*)detourUnpack, "sig.afs.unpack", (void**)&g_origUnpack)) {
        log().write("WARN", "assets: failed to install afs.unpack hook at 0x%08X", addr);
        return false;
    }
    log().write("INFO", "assets: unpack hook verified and installed at 0x%08X", addr);
    return true;
}

void report() {
    log().write("INFO", "assets: %d inflated, %d replaced", g_checked, g_replaced);
}

}  // namespace assets
}  // namespace remaster



