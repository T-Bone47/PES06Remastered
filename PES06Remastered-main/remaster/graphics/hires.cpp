// remaster graphics subsystem - higher resolution artwork.
//
// The game asks for a texture of a fixed size and uploads its own artwork
// into it. This module holds artwork supplied at a higher resolution and
// filters it down into the texture, so the picture the game ends up with is
// a properly supersampled version of the supplied art rather than a low
// resolution source stretched by the hardware.
//
// Pack format, in the folder named by the "pack" setting:
//
//     hires.txt      one "<width>x<height> <file>" line per texture size
//     <file>         the artwork, as a PNG or as a raw BGRA dump
//
// The size in the index is the size of the *texture* the artwork replaces,
// not the size of the artwork itself. The artwork may be any size; it is
// filtered down to fit.
//
// Making the texture itself larger is a different thing and does not work:
// the game's two dimensional rendering is tied to the size of the texture it
// asked for, so enlarging the texture changes the picture rather than
// sharpening it.

#include "hires.h"
#include "../core/remaster.h"
#include "../assets/png_loader.h"

#include <wincodec.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace graphics {

#define MAX_HIRES 256

struct HiresEntry {
    unsigned int width;
    unsigned int height;
    unsigned int crc;
    char file[128];
    HiResArt art;
    int loaded;
    int tried;
};

static HiresEntry g_entries[MAX_HIRES];
static int g_count = 0;
static int g_loaded = 0;

// ---------------------------------------------------------------- decoding

// Decodes a PNG to top-down 32 bit BGRA using WIC.
static bool decodePng(const char* path, HiResArt* out) {
    bool ok = false;
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* conv = nullptr;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool didInit = SUCCEEDED(hrInit);

    do {
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&factory)))) {
            break;
        }
        wchar_t wide[512];
        MultiByteToWideChar(CP_ACP, 0, path, -1, wide, 512);
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
        if (FAILED(conv->GetSize(&w, &h)) || w == 0 || h == 0) {
            break;
        }
        unsigned char* rgba = (unsigned char*)malloc((size_t)w * h * 4);
        if (!rgba) {
            break;
        }
        if (FAILED(conv->CopyPixels(nullptr, w * 4, (UINT)((size_t)w * h * 4), rgba))) {
            free(rgba);
            break;
        }
        // WIC hands back R,G,B,A; D3DFMT_A8R8G8B8 is B,G,R,A in memory.
        unsigned char* bgra = (unsigned char*)malloc((size_t)w * h * 4);
        if (!bgra) {
            free(rgba);
            break;
        }
        for (size_t i = 0; i < (size_t)w * h; i++) {
            bgra[i * 4 + 0] = rgba[i * 4 + 2];
            bgra[i * 4 + 1] = rgba[i * 4 + 1];
            bgra[i * 4 + 2] = rgba[i * 4 + 0];
            bgra[i * 4 + 3] = rgba[i * 4 + 3];
        }
        free(rgba);
        out->width = w;
        out->height = h;
        out->pixels = bgra;
        ok = true;
    } while (false);

    if (conv) { conv->Release(); }
    if (frame) { frame->Release(); }
    if (decoder) { decoder->Release(); }
    if (factory) { factory->Release(); }
    if (didInit) { CoUninitialize(); }
    return ok;
}

// Reads a raw BGRA dump.
//
// The file starts with the four bytes "RAW1", the width and the height as
// 32 bit little endian values, and then width * height * 4 bytes of BGRA.
// Carrying the size in the file is what lets the artwork be larger than the
// texture it replaces, which is the point of the whole exercise.
static bool decodeRaw(const char* path, unsigned int width, unsigned int height, HiResArt* out) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    char magic[4] = { 0, 0, 0, 0 };
    unsigned int hdr[2] = { 0, 0 };
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "RAW1", 4) != 0 ||
        fread(hdr, 1, 8, f) != 8) {
        fclose(f);
        return false;
    }
    unsigned int pw = hdr[0];
    unsigned int ph = hdr[1];
    if (pw == 0 || ph == 0 || pw > 16384 || ph > 16384) {
        fclose(f);
        return false;
    }
    size_t need = (size_t)pw * ph * 4;
    unsigned char* pixels = (unsigned char*)malloc(need);
    if (!pixels) {
        fclose(f);
        return false;
    }
    size_t got = fread(pixels, 1, need, f);
    fclose(f);
    if (got != need) {
        free(pixels);
        return false;
    }
    (void)width;
    (void)height;
    out->width = pw;
    out->height = ph;
    out->pixels = pixels;
    return true;
}

// ---------------------------------------------------------------- pack

int loadHiresPack() {
    if (g_loaded) {
        return g_count;
    }
    g_loaded = 1;

    char* folder = gamePath("remaster_pack");
    if (!folder) {
        return 0;
    }

    // Pass 1: check for structured JSON manifest (manifest.json)
    char jsonPath[512];
    snprintf(jsonPath, sizeof(jsonPath), "%s\\manifest.json", folder);
    FILE* fj = fopen(jsonPath, "rb");
    if (fj) {
        fclose(fj);
        int loadedFromJson = assets::loadAssetManifest(jsonPath);
        if (loadedFromJson > 0) {
            log().write("INFO", "hires: loaded %d assets from %s", loadedFromJson, jsonPath);
            return g_count;
        }
    }

    // Pass 2: fallback to legacy hires.txt index
    char path[512];
    snprintf(path, sizeof(path), "%s\\hires.txt", folder);
    FILE* f = fopen(path, "r");
    if (!f) {
        log().write("INFO", "hires: no index at %s", path);
        return 0;
    }

    char line[512];
    while (fgets(line, sizeof(line), f) && g_count < MAX_HIRES) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == 0 || *p == '#' || *p == '\n' || *p == '\r') {
            continue;
        }
        unsigned int w = 0, h = 0;
        int n = 0;
        if (sscanf(p, "%ux%u %n", &w, &h, &n) != 2 || n == 0) {
            continue;
        }
        char* rest = p + n;
        while (*rest == ' ' || *rest == '\t') {
            rest++;
        }
        char* nl = strpbrk(rest, "\r\n");
        if (nl) {
            *nl = 0;
        }
        if (!*rest || w == 0 || h == 0) {
            continue;
        }

        // Optional CRC32: e.g. "512x512 0x1A2B3C4D file.png" or "512x512 file.png"
        unsigned int crc = 0;
        char* name = rest;
        if ((rest[0] == '0' && (rest[1] == 'x' || rest[1] == 'X')) ||
            (isxdigit((unsigned char)rest[0]) && isxdigit((unsigned char)rest[1]))) {
            char* endp = nullptr;
            unsigned long v = strtoul(rest, &endp, 16);
            if (endp && (*endp == ' ' || *endp == '\t')) {
                crc = (unsigned int)v;
                name = endp;
                while (*name == ' ' || *name == '\t') {
                    name++;
                }
            }
        }

        addHiresEntry(w, h, crc, name, nullptr);
    }
    fclose(f);
    log().write("INFO", "hires: %d entries in the pack index", g_count);
    return g_count;
}

void clearHiresPack() {
    for (int i = 0; i < g_count; i++) {
        if (g_entries[i].art.pixels) {
            size_t bytes = (size_t)g_entries[i].art.width * g_entries[i].art.height * 4;
            assets::releaseMemoryAllocation(bytes);
            free(g_entries[i].art.pixels);
            g_entries[i].art.pixels = nullptr;
        }
    }
    g_count = 0;
    g_loaded = 0;
}

bool addHiresEntry(unsigned int width, unsigned int height, unsigned int crc,
                   const char* file, const HiResArt* art) {
    if (g_count >= MAX_HIRES || !file || !*file || width == 0 || height == 0) {
        return false;
    }
    HiresEntry& e = g_entries[g_count++];
    e.width = width;
    e.height = height;
    e.crc = crc;
    strncpy(e.file, file, sizeof(e.file) - 1);
    e.file[sizeof(e.file) - 1] = 0;
    e.art.width = 0;
    e.art.height = 0;
    e.art.pixels = nullptr;
    e.loaded = 0;
    e.tried = 0;

    if (art && art->pixels) {
        e.art.width = art->width;
        e.art.height = art->height;
        size_t bytes = (size_t)art->width * art->height * 4;
        e.art.pixels = (unsigned char*)malloc(bytes);
        if (e.art.pixels) {
            memcpy(e.art.pixels, art->pixels, bytes);
            e.loaded = 1;
            e.tried = 1;
        }
    }
    return true;
}

static const HiResArt* loadEntryArt(HiresEntry& e) {
    if (!e.tried) {
        e.tried = 1;
        char path[512];
        char* folder = gamePath("remaster_pack");
        if (folder) {
            snprintf(path, sizeof(path), "%s\\%s", folder, e.file);
            const char* dot = strrchr(e.file, '.');
            bool isPng = dot && (_stricmp(dot, ".png") == 0);
            if (isPng) {
                unsigned int w = 0, h = 0;
                size_t bytes = 0;
                unsigned char* bgra = assets::loadCompanionPng(path, &w, &h, &bytes);
                if (bgra) {
                    e.art.width = w;
                    e.art.height = h;
                    e.art.pixels = bgra;
                    e.loaded = 1;
                }
            }
            else if (decodeRaw(path, e.width, e.height, &e.art)) {
                e.loaded = 1;
            }
            if (e.loaded) {
                log().write("INFO", "hires: %s is %ux%u (crc=0x%08X), for a %ux%u texture",
                            e.file, e.art.width, e.art.height, e.crc, e.width, e.height);
            }
            else {
                log().write("WARN", "hires: could not read %s", path);
            }
        }
    }
    return e.loaded ? &e.art : 0;
}

const HiResArt* findHiResArt(unsigned int width, unsigned int height, unsigned int crc) {
    if (g_count == 0) {
        return 0;
    }
    // 1. Pass 1: exact CRC match (disambiguation against width/height collisions)
    if (crc != 0) {
        for (int i = 0; i < g_count; i++) {
            HiresEntry& e = g_entries[i];
            if (e.width == width && e.height == height && e.crc != 0 && e.crc == crc) {
                return loadEntryArt(e);
            }
        }
    }
    // 2. Pass 2: wildcard fallback (crc == 0)
    for (int i = 0; i < g_count; i++) {
        HiresEntry& e = g_entries[i];
        if (e.width == width && e.height == height && e.crc == 0) {
            return loadEntryArt(e);
        }
    }
    return 0;
}

bool drawHiResArt(const HiResArt* art, unsigned int width, unsigned int height,
                  unsigned char* out, int pitch) {
    if (!art || !art->pixels || width == 0 || height == 0) {
        return false;
    }
    const int srcW = (int)art->width;
    const int srcH = (int)art->height;
    const int dstW = (int)width;
    const int dstH = (int)height;
    if (srcW <= 0 || srcH <= 0) {
        return false;
    }

    // An area average is the right filter when shrinking: every source pixel
    // contributes to the destination pixel it falls in.
    for (int dy = 0; dy < dstH; dy++) {
        int y0 = (int)((long)dy * srcH / dstH);
        int y1 = (int)(((long)(dy + 1) * srcH + dstH - 1) / dstH);
        if (y1 <= y0) {
            y1 = y0 + 1;
        }
        if (y1 > srcH) {
            y1 = srcH;
        }
        long rowOffset = pitch < 0 ? -(long)dy * -(long)pitch : (long)dy * (long)pitch;
        unsigned char* dst = out + rowOffset;

        for (int dx = 0; dx < dstW; dx++) {
            int x0 = (int)((long)dx * srcW / dstW);
            int x1 = (int)(((long)(dx + 1) * srcW + dstW - 1) / dstW);
            if (x1 <= x0) {
                x1 = x0 + 1;
            }
            if (x1 > srcW) {
                x1 = srcW;
            }
            unsigned int sum[4] = { 0, 0, 0, 0 };
            unsigned int count = 0;
            for (int sy = y0; sy < y1; sy++) {
                const unsigned char* row = art->pixels + (size_t)sy * (size_t)srcW * 4;
                for (int sx = x0; sx < x1; sx++) {
                    sum[0] += row[(size_t)sx * 4 + 0];
                    sum[1] += row[(size_t)sx * 4 + 1];
                    sum[2] += row[(size_t)sx * 4 + 2];
                    sum[3] += row[(size_t)sx * 4 + 3];
                    count++;
                }
            }
            if (!count) {
                count = 1;
            }
            unsigned char* o = dst + (size_t)dx * 4;
            for (int ch = 0; ch < 4; ch++) {
                o[ch] = (unsigned char)((sum[ch] + count / 2) / count);
            }
        }
    }
    return true;
}

}  // namespace graphics
}  // namespace remaster
