// remaster assets subsystem - companion image loader, SHA-256 validation,
// and VRAM budget enforcement.
//
// Implements deterministic asset loading for high-resolution companion textures
// (kits, player models, pitch surfaces, balls, and UI elements) while strictly
// constraining memory consumption under a configurable 1.5 GB ceiling to protect
// the game's 32-bit (2.0 GB) virtual address space.

#include "png_loader.h"
#include "assets.h"
#include "../core/remaster.h"
#include "../graphics/hires.h"

#include <windows.h>
#include <wincrypt.h>
#include <wincodec.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace assets {

// ---------------------------------------------------------------- SHA-256 (Windows CryptoAPI)

void computeBufferSha256(const void* data, size_t len, char hexDigestOut[65]) {
    if (!hexDigestOut) return;
    hexDigestOut[0] = '\0';

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return;
    }
    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        if (len > 0) {
            CryptHashData(hHash, (const BYTE*)data, (DWORD)len, 0);
        }
        BYTE hash[32];
        DWORD hashLen = sizeof(hash);
        if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
            for (DWORD i = 0; i < hashLen; i++) {
                snprintf(hexDigestOut + (i * 2), 3, "%02x", hash[i]);
            }
            hexDigestOut[64] = '\0';
        }
        CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
}

bool computeFileSha256(const char* filePath, char hexDigestOut[65]) {
    if (!filePath || !hexDigestOut) {
        return false;
    }
    hexDigestOut[0] = '\0';

    FILE* f = fopen(filePath, "rb");
    if (!f) {
        return false;
    }

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        fclose(f);
        return false;
    }

    bool success = false;
    if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        BYTE buffer[16384];
        size_t bytesRead = 0;
        bool readOk = true;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) {
            if (!CryptHashData(hHash, buffer, (DWORD)bytesRead, 0)) {
                readOk = false;
                break;
            }
        }
        if (readOk) {
            BYTE hash[32];
            DWORD hashLen = sizeof(hash);
            if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                for (DWORD i = 0; i < hashLen; i++) {
                    snprintf(hexDigestOut + (i * 2), 3, "%02x", hash[i]);
                }
                hexDigestOut[64] = '\0';
                success = true;
            }
        }
        CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
    fclose(f);
    return success;
}

// ---------------------------------------------------------------- VRAM Budget

static size_t g_maxVramBytes = 1536u * 1024u * 1024u; // 1.5 GB default
static size_t g_allocatedVramBytes = 0;
static size_t g_peakVramBytes = 0;
static int g_budgetAssetCount = 0;
static int g_budgetRejectedCount = 0;

void initMemoryBudget(size_t maxMegabytes) {
    if (maxMegabytes < 128) {
        maxMegabytes = 128;
    }
    if (maxMegabytes > 1800) {
        maxMegabytes = 1800; // Keep headroom below 2.0 GB 32-bit limit
    }
    g_maxVramBytes = maxMegabytes * 1024u * 1024u;
    g_allocatedVramBytes = 0;
    g_peakVramBytes = 0;
    g_budgetAssetCount = 0;
    g_budgetRejectedCount = 0;
}

AssetMemoryBudget getMemoryBudget() {
    AssetMemoryBudget b;
    b.maxBytes = g_maxVramBytes;
    b.allocatedBytes = g_allocatedVramBytes;
    b.peakBytes = g_peakVramBytes;
    b.assetCount = g_budgetAssetCount;
    b.rejectedCount = g_budgetRejectedCount;
    return b;
}

bool requestMemoryAllocation(size_t bytes) {
    if (g_allocatedVramBytes + bytes > g_maxVramBytes) {
        g_budgetRejectedCount++;
        return false;
    }
    g_allocatedVramBytes += bytes;
    if (g_allocatedVramBytes > g_peakVramBytes) {
        g_peakVramBytes = g_allocatedVramBytes;
    }
    g_budgetAssetCount++;
    return true;
}

void releaseMemoryAllocation(size_t bytes) {
    if (bytes >= g_allocatedVramBytes) {
        g_allocatedVramBytes = 0;
    } else {
        g_allocatedVramBytes -= bytes;
    }
    if (g_budgetAssetCount > 0) {
        g_budgetAssetCount--;
    }
}

// ---------------------------------------------------------------- PNG Decoder

unsigned char* loadCompanionPng(const char* fullPath, unsigned int* outWidth,
                                unsigned int* outHeight, size_t* outBytes) {
    if (!fullPath || !outWidth || !outHeight || !outBytes) {
        return nullptr;
    }
    *outWidth = 0;
    *outHeight = 0;
    *outBytes = 0;

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    (void)hrInit;

    unsigned char* bgra = nullptr;
    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) {
            break;
        }
        wchar_t wide[512];
        MultiByteToWideChar(CP_ACP, 0, fullPath, -1, wide, 512);
        if (FAILED(factory->CreateDecoderFromFilename(wide, nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnDemand, &decoder))) {
            break;
        }
        if (FAILED(decoder->GetFrame(0, &frame))) {
            break;
        }
        if (FAILED(factory->CreateFormatConverter(&conv))) {
            break;
        }
        if (FAILED(conv->Initialize(frame, GUID_WICPixelFormat32bppRGBA,
                                    WICBitmapDitherTypeNone, nullptr, 0.0,
                                    WICBitmapPaletteTypeCustom))) {
            break;
        }
        UINT w = 0, h = 0;
        if (FAILED(conv->GetSize(&w, &h)) || w == 0 || h == 0 || w > 8192 || h > 8192) {
            break;
        }
        size_t need = (size_t)w * h * 4;
        if (!requestMemoryAllocation(need)) {
            log().write("WARN", "assets: memory budget exceeded (%zu MB / %zu MB) - skipping %s",
                        g_allocatedVramBytes / (1024 * 1024), g_maxVramBytes / (1024 * 1024), fullPath);
            break;
        }

        unsigned char* rgba = (unsigned char*)malloc(need);
        if (!rgba) {
            releaseMemoryAllocation(need);
            break;
        }
        if (FAILED(conv->CopyPixels(nullptr, w * 4, (UINT)need, rgba))) {
            free(rgba);
            releaseMemoryAllocation(need);
            break;
        }

        bgra = (unsigned char*)malloc(need);
        if (!bgra) {
            free(rgba);
            releaseMemoryAllocation(need);
            break;
        }

        // Convert RGBA to BGRA in memory for Direct3D 8 D3DFMT_A8R8G8B8 compatibility
        for (size_t i = 0; i < (size_t)w * h; i++) {
            bgra[i * 4 + 0] = rgba[i * 4 + 2]; // B
            bgra[i * 4 + 1] = rgba[i * 4 + 1]; // G
            bgra[i * 4 + 2] = rgba[i * 4 + 0]; // R
            bgra[i * 4 + 3] = rgba[i * 4 + 3]; // A
        }
        free(rgba);

        *outWidth = w;
        *outHeight = h;
        *outBytes = need;
    } while (false);

    if (conv) conv->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();

    return bgra;
}

// ---------------------------------------------------------------- JSON Parser

static const char* skipWs(const char* p) {
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        p++;
    }
    return p;
}

static bool parseJsonString(const char*& p, char* out, size_t maxLen) {
    p = skipWs(p);
    if (*p != '\"') return false;
    p++;
    size_t idx = 0;
    while (*p && *p != '\"') {
        if (*p == '\\' && *(p + 1)) {
            p++;
        }
        if (idx + 1 < maxLen) {
            out[idx++] = *p;
        }
        p++;
    }
    out[idx] = '\0';
    if (*p == '\"') {
        p++;
        return true;
    }
    return false;
}

static bool parseJsonUint(const char*& p, unsigned int* outVal) {
    p = skipWs(p);
    if (*p == '0' && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        char* endp = nullptr;
        *outVal = (unsigned int)strtoul(p, &endp, 16);
        p = endp;
        return true;
    }
    if (*p >= '0' && *p <= '9') {
        char* endp = nullptr;
        *outVal = (unsigned int)strtoul(p, &endp, 10);
        p = endp;
        return true;
    }
    return false;
}

int loadAssetManifest(const char* jsonPath) {
    if (!jsonPath || !*jsonPath) {
        return 0;
    }
    FILE* f = fopen(jsonPath, "rb");
    if (!f) {
        return 0;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 1024 * 1024 * 4) { // 4 MB safety limit for manifest
        fclose(f);
        return 0;
    }

    char* content = (char*)malloc((size_t)sz + 1);
    if (!content) {
        fclose(f);
        return 0;
    }
    size_t n = fread(content, 1, (size_t)sz, f);
    fclose(f);
    content[n] = '\0';

    // Parse root budget if present
    const char* budgetPos = strstr(content, "\"vram_budget_mb\"");
    if (budgetPos) {
        budgetPos += 16;
        unsigned int mb = 0;
        const char* p = budgetPos;
        p = skipWs(p);
        if (*p == ':') p++;
        if (parseJsonUint(p, &mb) && mb >= 128) {
            initMemoryBudget(mb);
        }
    }

    // Parse assets array
    const char* assetsPos = strstr(content, "\"assets\"");
    if (!assetsPos) {
        free(content);
        return 0;
    }
    const char* p = strchr(assetsPos, '[');
    if (!p) {
        free(content);
        return 0;
    }
    p++;

    int registered = 0;
    while (*p && *p != ']') {
        p = skipWs(p);
        if (*p == '{') {
            p++;
            AssetManifestEntry entry;
            memset(&entry, 0, sizeof(entry));

            while (*p && *p != '}') {
                p = skipWs(p);
                char key[64];
                if (!parseJsonString(p, key, sizeof(key))) {
                    p++;
                    continue;
                }
                p = skipWs(p);
                if (*p == ':') p++;
                p = skipWs(p);

                if (_stricmp(key, "id") == 0) {
                    parseJsonString(p, entry.id, sizeof(entry.id));
                } else if (_stricmp(key, "category") == 0) {
                    parseJsonString(p, entry.category, sizeof(entry.category));
                } else if (_stricmp(key, "orig_width") == 0) {
                    parseJsonUint(p, &entry.origWidth);
                } else if (_stricmp(key, "orig_height") == 0) {
                    parseJsonUint(p, &entry.origHeight);
                } else if (_stricmp(key, "crc32") == 0) {
                    if (*p == '\"') {
                        char crcStr[32];
                        parseJsonString(p, crcStr, sizeof(crcStr));
                        entry.crc32 = (unsigned int)strtoul(crcStr, nullptr, 16);
                    } else {
                        parseJsonUint(p, &entry.crc32);
                    }
                } else if (_stricmp(key, "file") == 0) {
                    parseJsonString(p, entry.file, sizeof(entry.file));
                } else if (_stricmp(key, "sha256") == 0) {
                    parseJsonString(p, entry.sha256, sizeof(entry.sha256));
                } else {
                    // Skip unknown value
                    if (*p == '\"') {
                        char dummy[128];
                        parseJsonString(p, dummy, sizeof(dummy));
                    } else {
                        while (*p && *p != ',' && *p != '}') p++;
                    }
                }
                p = skipWs(p);
                if (*p == ',') p++;
            }
            if (*p == '}') p++;

            // Validate entry
            if (entry.origWidth > 0 && entry.origHeight > 0 && entry.file[0] != '\0') {
                if (!isPathSafe(entry.file)) {
                    log().write("WARN", "assets: rejected unsafe manifest path: %s", entry.file);
                    continue;
                }

                // Verify file presence
                char* folder = gamePath("remaster_pack");
                char fullPath[512];
                if (folder) {
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s", folder, entry.file);
                } else {
                    snprintf(fullPath, sizeof(fullPath), "%s", entry.file);
                }

                // Verify SHA-256 integrity if specified
                if (entry.sha256[0] != '\0') {
                    char actualSha[65];
                    if (computeFileSha256(fullPath, actualSha)) {
                        if (_stricmp(entry.sha256, actualSha) != 0) {
                            log().write("WARN", "assets: integrity mismatch for %s (expected %s, got %s)",
                                        entry.file, entry.sha256, actualSha);
                            continue;
                        }
                        entry.verified = true;
                    }
                }

                // Register into high-resolution texture engine
                if (graphics::addHiresEntry(entry.origWidth, entry.origHeight, entry.crc32, entry.file, nullptr)) {
                    registered++;
                    log().write("INFO", "assets: registered %s (%s, %ux%u, crc=0x%08X)",
                                entry.id[0] ? entry.id : entry.file,
                                entry.category[0] ? entry.category : "generic",
                                entry.origWidth, entry.origHeight, entry.crc32);
                }
            }
        } else {
            p++;
        }
        p = skipWs(p);
        if (*p == ',') p++;
    }

    free(content);
    log().write("INFO", "assets: manifest %s loaded (%d valid assets registered)", jsonPath, registered);
    return registered;
}

}  // namespace assets
}  // namespace remaster

