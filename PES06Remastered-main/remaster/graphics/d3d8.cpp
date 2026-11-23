// remaster graphics subsystem - D3D8 device and texture proxy.
//
// The game owns a Direct3D 8 device. Its pointer lives in a global, and the
// device's method table is the ordinary COM vtable. By patching that table
// the remaster can see every texture the game creates, and can replace the
// contents of the ones it makes larger.
//
// Evidence (pes6.exe sha256 0c7f804b8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f):
//
//   0x00F83EB8  the IDirect3D8 object
//   0x00F83E68  the IDirect3DDevice8 pointer
//   0x00F83EBC  D3DPRESENT_PARAMETERS (+0x00 width, +0x04 height, +0x0C format)
//   0x00405300  the game's device-creation routine
//   0x00874A20  void setTextureSize(int width, int height); writes the two
//                globals 0x011631F8 (width) and 0x011631FC (height)
//   0x0040E850  fills the texture descriptor: +0x14 width, +0x18 height,
//                +0x1C levels, +0x20 usage, +0x24 format
//   0x0040E700  calls IDirect3DDevice8::CreateTexture
//
// The vtable indices come from the DirectX 8.1 SDK header (d3d8.h) and were
// each cross-checked against a call site in the game:
//   IDirect3D8        +0x24 index  9 CheckDeviceType   (5 arguments at 0x403C10)
//                     +0x34 index 13 GetDeviceCaps     (3 arguments at 0x404D5D)
//                     +0x3C index 15 CreateDevice      (6 arguments at 0x405589)
//   IDirect3DDevice8  +0x90 index 36 Clear             (6 arguments at 0x4056A4)
//   IDirect3DTexture8 +0x38 index 14 GetLevelDesc
//                     +0x3C index 15 GetSurfaceLevel
//                     +0x40 index 16 LockRect
//                     +0x44 index 17 UnlockRect
//   IDirect3DSurface8 +0x24 index  9 LockRect
//                     +0x28 index 10 UnlockRect
//
// Two things about this game are worth recording, because both cost time:
//
//  * The game keeps its own copy of the device vtable in its own heap, so
//    that table can be patched in place. Substituting a private copy instead
//    makes the game call through a table the device was never built with and
//    it crashes on the next call.
//  * The game's two-dimensional rendering is tied to the size of the texture
//    it asked for, so making a texture larger does not simply give a higher
//    resolution picture - the sprite geometry changes with it. Enlarging is
//    therefore off by default and the asset pipeline is the supported route.

#include "d3d8.h"
#include "graphics.h"
#include "hires.h"
#include "postprocess.h"
#include "../core/remaster.h"

#include "MinHook.h"

#include <intrin.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace graphics {

// Game global holding the IDirect3DDevice8 pointer.
static const unsigned int G_DEVICE = 0x00F83E68;

// D3DPOOL_MANAGED. Spelled out rather than including d3d8.h, so the build
// does not depend on the DirectX SDK being present.
static const unsigned int POOL_MANAGED = 1;

// IDirect3DTexture8 vtable slots.
static const int VT_TEX_RELEASE = 2;
static const int VT_TEX_GETLEVELDESC = 14;
static const int VT_TEX_GETSURFACELEVEL = 15;
static const int VT_TEX_LOCKRECT = 16;
static const int VT_TEX_UNLOCKRECT = 17;

// IDirect3DSurface8 vtable slots.
static const int VT_SURF_LOCKRECT = 9;
static const int VT_SURF_UNLOCKRECT = 10;

// D3DLOCKED_RECT is {INT Pitch; void* pBits;} in the DirectX 8.1 SDK.
struct LockedRect {
    int Pitch;
    void* pBits;
};

// D3DSURFACE_DESC, with Width at +0x18 and Height at +0x1C.
struct SurfaceDesc {
    unsigned int Format;
    unsigned int Type;
    unsigned int Usage;
    unsigned int Pool;
    unsigned int Size;
    unsigned int MultiSampleType;
    unsigned int Width;
    unsigned int Height;
};

typedef HRESULT(__stdcall* FnCreateTexture)(void*, UINT, UINT, UINT, DWORD, DWORD, DWORD, void**);
typedef int(__cdecl* FnCreateDeviceEntry)(void);
typedef HRESULT(__stdcall* FnGetLevelDesc)(void*, UINT, SurfaceDesc*);
typedef ULONG(__stdcall* FnRelease)(void*);
typedef HRESULT(__stdcall* FnGetSurfaceLevel)(void*, UINT, void**);
typedef HRESULT(__stdcall* FnTexLockRect)(void*, UINT, LockedRect*, const void*, DWORD);
typedef HRESULT(__stdcall* FnTexUnlockRect)(void*, UINT);
typedef HRESULT(__stdcall* FnSurfLockRect)(void*, LockedRect*, const void*, DWORD);
typedef HRESULT(__stdcall* FnSurfUnlockRect)(void*);
typedef void(__cdecl* FnSetTextureSize)(int, int);

static HRESULT __stdcall detourCreateTexture(void*, UINT, UINT, UINT, DWORD, DWORD, DWORD, void**);
static void applyHiResArt(struct WatchedTexture*);
static HRESULT __stdcall detourGetLevelDesc(void*, UINT, SurfaceDesc*);
static ULONG __stdcall detourTexRelease(void*);
static HRESULT __stdcall detourGetSurfaceLevel(void*, UINT, void**);
static HRESULT __stdcall detourTexLockRect(void*, UINT, LockedRect*, const void*, DWORD);
static HRESULT __stdcall detourTexUnlockRect(void*, UINT);
static HRESULT __stdcall detourSurfLockRect(void*, LockedRect*, const void*, DWORD);
static HRESULT __stdcall detourSurfUnlockRect(void*);

static FnCreateTexture g_origCreateTexture = 0;
static FnCreateDeviceEntry g_origCreateDevice = 0;
static FnGetLevelDesc g_origGetLevelDesc = 0;
static FnRelease g_origTexRelease = 0;
static FnGetSurfaceLevel g_origGetSurfaceLevel = 0;
static FnTexLockRect g_origTexLockRect = 0;
static FnTexUnlockRect g_origTexUnlockRect = 0;
static FnSurfLockRect g_origSurfLockRect = 0;
static FnSurfUnlockRect g_origSurfUnlockRect = 0;
static FnSetTextureSize g_origSetTextureSize = 0;

// A texture the remaster is keeping an eye on.
struct WatchedTexture;

struct WatchedTexture {
    void* texture;
    void* surface;
    unsigned int width;
    unsigned int height;
    unsigned int scaledWidth;
    unsigned int scaledHeight;
    unsigned char* scratch;   // where the game's own upload is diverted
    size_t scratchSize;
    int scratchPitch;
    int locked;
};

#define MAX_WATCHED 512
static WatchedTexture g_watched[MAX_WATCHED];
static int g_watchedCount = 0;

static int g_textureCount = 0;
static int g_logTextures = 0;
static int g_logLocks = 0;
static int g_observeTextures = 0;
static int g_texScale = 1;
static int g_sizeCalls = 0;
static int g_hiresApplied = 0;
static int g_hiresCount = 0;
static int g_textureVtablePatched = 0;
static int g_surfaceVtablePatched = 0;

static unsigned int g_crcTable[256];
static int g_crcReady = 0;

static void initCrcTable() {
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

static void forgetWatched(void* texture) {
    for (int i = 0; i < g_watchedCount; i++) {
        if (g_watched[i].texture == texture) {
            if (g_watched[i].scratch) {
                free(g_watched[i].scratch);
            }
            g_watched[i] = g_watched[g_watchedCount - 1];
            g_watchedCount--;
            return;
        }
    }
}

static WatchedTexture* findWatched(void* texture) {
    for (int i = 0; i < g_watchedCount; i++) {
        if (g_watched[i].texture == texture) {
            return &g_watched[i];
        }
    }
    return 0;
}

static WatchedTexture* findWatchedBySurface(void* surface) {
    for (int i = 0; i < g_watchedCount; i++) {
        if (g_watched[i].surface == surface && g_watched[i].surface != 0) {
            return &g_watched[i];
        }
    }
    return 0;
}

// A vtable that belongs to d3d8.dll lives in read-only memory, so it has to
// be made writable before it can be patched. The patch is global for that
// table, so every detour checks whether the object is one of ours and passes
// everything else straight through.
static bool patchVTable(void** vtable, int slot, void* detour) {
    if (!vtable) {
        return false;
    }
    DWORD oldProtect = 0;
    if (!VirtualProtect(vtable, sizeof(void*) * (slot + 1), PAGE_READWRITE, &oldProtect)) {
        return false;
    }
    vtable[slot] = detour;
    DWORD ignored = 0;
    VirtualProtect(vtable, sizeof(void*) * (slot + 1), oldProtect, &ignored);
    return true;
}

// ---------------------------------------------------------------- device

typedef HRESULT(__stdcall* FnSetTextureStageState)(void*, DWORD, DWORD, DWORD);
static FnSetTextureStageState g_origSetTextureStageState = 0;

static HRESULT __stdcall detourSetTextureStageState(void* device, DWORD stage, DWORD type, DWORD value) {
    if (getQualitySettings().anisotropicLevel > 1) {
        // Enforce anisotropic filtering whenever game requests linear or point filtering on diffuse stages
        if (type == 16 /* D3DTSS_MAGFILTER */ || type == 17 /* D3DTSS_MINFILTER */) {
            value = 3; /* D3DTEXF_ANISOTROPIC */
        } else if (type == 21 /* D3DTSS_MAXANISOTROPY */) {
            value = (DWORD)getQualitySettings().anisotropicLevel;
        }
    }
    if (g_origSetTextureStageState) {
        return g_origSetTextureStageState(device, stage, type, value);
    }
    return S_OK;
}

static void installDeviceProxy(void* device) {
    if (!device) {
        return;
    }
    void** original = *(void***)device;
    if (!original) {
        return;
    }
    void* orig20 = original[20];   // IDirect3DDevice8::CreateTexture
    if (orig20) {
        g_origCreateTexture = (FnCreateTexture)orig20;
        patchVTable(original, 20, (void*)detourCreateTexture);
        log().write("INFO", "d3d8: device vtable patched, CreateTexture is slot 20");
    }

    void* orig59 = original[59];   // IDirect3DDevice8::SetTextureStageState
    if (orig59 && !g_origSetTextureStageState) {
        g_origSetTextureStageState = (FnSetTextureStageState)orig59;
        patchVTable(original, 59, (void*)detourSetTextureStageState);
        log().write("INFO", "d3d8: device vtable patched, SetTextureStageState is slot 59 (enforcing AF)");
    }
}

// Captures the return address of the *caller* of the function that calls this.
//
// _ReturnAddress() is not reliable in optimised code: it returned a value that
// landed in the middle of an instruction. This reads the stack directly
// instead, and a naked stub is used so the compiler cannot reorder or fold it.
//
// At entry:  [esp+0] = return address into our caller
//            [esp+4] = the first argument (the out pointer)
//            [esp+8] = our caller's own return address, i.e. the address in
//                       the game that called our caller.
static void* callerReturnAddress() {
    // _AddressOfReturnAddress() gives the address of this function's return
    // address slot; dereferencing it gives the address the caller will return
    // to, i.e. the instruction after the call in the game.
    void** slot = (void**)_AddressOfReturnAddress();
    return slot ? *slot : 0;
}

static HRESULT __stdcall detourCreateTexture(void* device, UINT width, UINT height, UINT levels,
                                             DWORD usage, DWORD format, DWORD pool, void** ppTexture) {
    // A scaled texture is created larger than the game asked for. The game
    // still uploads its own pixels into it, and the hi-res layer replaces them
    // once the upload is finished.
    //
    // Only D3DPOOL_MANAGED textures are scaled. The D3DPOOL_DEFAULT ones with
    // D3DUSAGE_DYNAMIC are the game's own render targets and working
    // buffers: the 3D scene is rendered into them at a fixed size, so making
    // them larger leaves the scene black.
    UINT realWidth = width;
    UINT realHeight = height;
    // Format guard: Only uncompressed 32-bit (A8R8G8B8 = 21, X8R8G8B8 = 22) or 24-bit (R8G8B8 = 20)
    // formats may have their dimensions scaled and diverted into 32-bit scratch buffers.
    // DXT compressed textures (DXT1=0x31545844..DXT5), 16-bit (23..26), and paletted (41) textures
    // MUST NOT be scaled, as writing 32-bit RGBA into them completely corrupts pitch and turf rendering!
    bool is32Bit = (format == 21 || format == 22 || format == 20);
    if (g_texScale > 1 && pool == POOL_MANAGED && is32Bit) {
        realWidth = (UINT)(width * g_texScale);
        realHeight = (UINT)(height * g_texScale);
    }
    HRESULT hr = g_origCreateTexture(device, realWidth, realHeight, levels, usage, format, pool, ppTexture);
    g_textureCount++;

    if (pool == POOL_MANAGED && ppTexture && *ppTexture) {
        WatchedTexture* existing = findWatched(*ppTexture);
        if (existing) {
            if (existing->scratch) { free(existing->scratch); }
            existing->scratch = 0;
            existing->scratchSize = 0;
            existing->surface = 0;
            existing->locked = 0;
        }
    }
    if (pool == POOL_MANAGED && ppTexture && *ppTexture && g_watchedCount < MAX_WATCHED) {
        WatchedTexture& w = g_watched[g_watchedCount++];
        w.texture = *ppTexture;
        w.surface = 0;
        w.width = width;
        w.height = height;
        w.scaledWidth = realWidth;
        w.scaledHeight = realHeight;
        w.scratch = 0;
        w.scratchSize = 0;
        w.scratchPitch = 0;
        w.locked = 0;

        void** vt = *(void***)(*ppTexture);
        if (vt && !g_textureVtablePatched) {
            // The originals are captured once. Every texture shares one
            // vtable, so capturing again would capture this file's own
            // detours and recurse for ever.
            g_origGetLevelDesc = (FnGetLevelDesc)vt[VT_TEX_GETLEVELDESC];
            g_origTexRelease = (FnRelease)vt[VT_TEX_RELEASE];
            patchVTable(vt, VT_TEX_RELEASE, (void*)detourTexRelease);
            g_origGetSurfaceLevel = (FnGetSurfaceLevel)vt[VT_TEX_GETSURFACELEVEL];
            g_origTexLockRect = (FnTexLockRect)vt[VT_TEX_LOCKRECT];
            g_origTexUnlockRect = (FnTexUnlockRect)vt[VT_TEX_UNLOCKRECT];
            patchVTable(vt, VT_TEX_GETLEVELDESC, (void*)detourGetLevelDesc);
            patchVTable(vt, VT_TEX_GETSURFACELEVEL, (void*)detourGetSurfaceLevel);
            patchVTable(vt, VT_TEX_LOCKRECT, (void*)detourTexLockRect);
            patchVTable(vt, VT_TEX_UNLOCKRECT, (void*)detourTexUnlockRect);
            g_textureVtablePatched = 1;
        }
        if (realWidth != width) {
            log().write("INFO", "d3d8: scaled texture %ux%u -> %ux%u",
                        (unsigned)width, (unsigned)height, (unsigned)realWidth, (unsigned)realHeight);
        }
    }

    if (g_logTextures) {
        // The caller's return address, read off the stack by a naked stub.
        // _ReturnAddress() is not reliable in optimised code.
        void* caller = callerReturnAddress();
        log().write("INFO", "d3d8: texture %ux%u -> %ux%u levels=%u usage=0x%X format=%u pool=%u hr=0x%08X caller=%p",
                    (unsigned)width, (unsigned)height, (unsigned)realWidth, (unsigned)realHeight,
                    (unsigned)levels, (unsigned)usage,
                    (unsigned)format, (unsigned)pool, (unsigned long)hr, caller);
    }
    return hr;
}

void applyAnisotropicFiltering(void* d3dDevice, int level) {
    if (!d3dDevice || level <= 1) {
        return;
    }
    void** vtable = *(void***)d3dDevice;
    if (!vtable || !vtable[59]) {
        return;
    }

    // Safely query hardware device capabilities (slot 7 = GetDeviceCaps)
    DWORD safeMaxAniso = 16;
    typedef HRESULT(__stdcall* FnGetDeviceCaps)(void*, void*);
    FnGetDeviceCaps getCaps = (FnGetDeviceCaps)vtable[7];
    if (getCaps) {
        unsigned char capsBuf[512] = { 0 };
        if (SUCCEEDED(getCaps(d3dDevice, capsBuf))) {
            // Offset 0x00E0 in D3DCAPS8 is MaxAnisotropy
            DWORD hwMax = *(DWORD*)(capsBuf + 0xE0);
            if (hwMax > 0 && hwMax < safeMaxAniso) {
                safeMaxAniso = hwMax;
            }
        }
    }
    if ((DWORD)level > safeMaxAniso) {
        level = (int)safeMaxAniso;
    }

    // Slot 59: IDirect3DDevice8::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
    typedef HRESULT(__stdcall* FnSetTextureStageState)(void*, DWORD, DWORD, DWORD);
    FnSetTextureStageState setTSS = (FnSetTextureStageState)vtable[59];

    // Stage 0: Main diffuse surface / turf sampler
    setTSS(d3dDevice, 0, 21 /* D3DTSS_MAXANISOTROPY */, (DWORD)level);
    setTSS(d3dDevice, 0, 16 /* D3DTSS_MAGFILTER */, 3 /* D3DTEXF_ANISOTROPIC */);
    setTSS(d3dDevice, 0, 17 /* D3DTSS_MINFILTER */, 3 /* D3DTEXF_ANISOTROPIC */);
    setTSS(d3dDevice, 0, 18 /* D3DTSS_MIPFILTER */, 2 /* D3DTEXF_LINEAR */);
    log().write("INFO", "d3d8: %dx hardware anisotropic filtering applied to sampler stage 0 (caps=%u)", level, safeMaxAniso);
}

// The game's device-creation routine. After it succeeds the device
// pointer is valid and can be proxied.
static int __cdecl detourCreateDeviceEntry(void) {
    int result = g_origCreateDevice();
    if (result == 0) {
        installDeviceProxy(*(void**)G_DEVICE);
        applyPostProcess(*(void**)G_DEVICE);
        applyAnisotropicFiltering(*(void**)G_DEVICE, getQualitySettings().anisotropicLevel);
        onDeviceCreated();
    }
    else {
        log().write("WARN", "d3d8: device creation returned %d - proxy not installed", result);
    }
    return result;
}

// ---------------------------------------------------------------- texture size

//   0x00874A20  void __cdecl setTextureSize(int width, int height)
//               { DAT_0116321A = 1; DAT_011631F8 = width; DAT_011631FC = height; }
//
// The game derives these from the asset it has just loaded, and FUN_00874FA0
// halves them when a texture cannot be allocated.
static void __cdecl detourSetTextureSize(int width, int height) {
    g_sizeCalls++;
    if (g_logLocks) {
        void* caller = callerReturnAddress();
        log().write("INFO", "texsize: %dx%d from %p", width, height, caller);
    }
    // The size is deliberately not scaled here. setTextureSize only sets the
    // game's global working-buffer size; every actual texture goes through
    // CreateTexture, and scaling in both places would scale twice.
    g_origSetTextureSize(width, height);
}

// ---------------------------------------------------------------- texture proxy

static ULONG __stdcall detourTexRelease(void* texture) {
    ULONG r = g_origTexRelease(texture);
    if (shouldForgetResource(r)) {
        forgetWatched(texture);
    }
    return r;
}

static HRESULT __stdcall detourGetLevelDesc(void* texture, UINT level, SurfaceDesc* pDesc) {
    HRESULT hr = g_origGetLevelDesc(texture, level, pDesc);    WatchedTexture* w = findWatched(texture);
    if (w && pDesc && SUCCEEDED(hr)) {
        // The game derives its geometry from the texture it gets back.
        // Reporting the size it asked for keeps that geometry unchanged.
        pDesc->Width = w->width;
        pDesc->Height = w->height;
    }
    if (g_logLocks) {
        log().write("INFO", "d3d8: GetLevelDesc tex=%p level=%u -> %ux%u",
                    texture, (unsigned)level, pDesc ? pDesc->Width : 0, pDesc ? pDesc->Height : 0);
    }
    return hr;
}

static HRESULT __stdcall detourGetSurfaceLevel(void* texture, UINT level, void** ppSurface) {
    HRESULT hr = g_origGetSurfaceLevel(texture, level, ppSurface);
    WatchedTexture* w = findWatched(texture);
    if (w && ppSurface && *ppSurface) {
        w->surface = *ppSurface;
        void** vt = *(void***)(*ppSurface);
        if (vt && !g_surfaceVtablePatched) {
            g_origSurfLockRect = (FnSurfLockRect)vt[VT_SURF_LOCKRECT];
            g_origSurfUnlockRect = (FnSurfUnlockRect)vt[VT_SURF_UNLOCKRECT];
            patchVTable(vt, VT_SURF_LOCKRECT, (void*)detourSurfLockRect);
            patchVTable(vt, VT_SURF_UNLOCKRECT, (void*)detourSurfUnlockRect);
            g_surfaceVtablePatched = 1;
        }
    }
    if (g_logLocks) {
        log().write("INFO", "d3d8: GetSurfaceLevel tex=%p level=%u surf=%p",
                    texture, (unsigned)level, ppSurface ? *ppSurface : 0);
    }
    return hr;
}

// Writes the enlarged texture's contents.
//
// The game uploads its own art, which is only as large as the texture it
// asked for. The upload is diverted into a scratch buffer so the real surface
// is left untouched, and the surface is then filled with the art scaled up to
// the full size of the enlarged texture.
static void fillScaledTexture(WatchedTexture* w) {
    if (!w->texture || !w->scratch) {
        return;
    }
    LockedRect lr;
    if (FAILED(g_origTexLockRect(w->texture, 0, &lr, 0, 0)) || !lr.pBits) {
        return;
    }

    const int srcW = (int)w->width;
    const int srcH = (int)w->height;
    const int dstW = (int)w->scaledWidth;
    const int dstH = (int)w->scaledHeight;

    int pitch = lr.Pitch;
    bool bottomUp = pitch < 0;
    if (bottomUp) {
        pitch = -pitch;
    }

    for (int y = 0; y < dstH; y++) {
        float fy = ((float)y + 0.5f) * (float)srcH / (float)dstH - 0.5f;
        int y0 = (int)floorf(fy);
        float wy = fy - (float)y0;
        int y1 = y0 + 1;
        if (y0 < 0) { y0 = 0; wy = 0.0f; }
        if (y1 > srcH - 1) { y1 = srcH - 1; }
        if (y1 < 0) { y1 = 0; }

        long offset = bottomUp ? -(long)y * (long)pitch : (long)y * (long)pitch;
        unsigned char* dst = (unsigned char*)lr.pBits + offset;
        const unsigned char* row0 = w->scratch + (size_t)y0 * (size_t)w->scratchPitch;
        const unsigned char* row1 = w->scratch + (size_t)y1 * (size_t)w->scratchPitch;

        for (int x = 0; x < dstW; x++) {
            float fx = ((float)x + 0.5f) * (float)srcW / (float)dstW - 0.5f;
            int x0 = (int)floorf(fx);
            float wx = fx - (float)x0;
            int x1 = x0 + 1;
            if (x0 < 0) { x0 = 0; wx = 0.0f; }
            if (x1 > srcW - 1) { x1 = srcW - 1; }
            if (x1 < 0) { x1 = 0; }

            const unsigned char* a = row0 + (size_t)x0 * 4;
            const unsigned char* b = row0 + (size_t)x1 * 4;
            const unsigned char* c = row1 + (size_t)x0 * 4;
            const unsigned char* e = row1 + (size_t)x1 * 4;
            unsigned char* o = dst + (size_t)x * 4;
            for (int ch = 0; ch < 4; ch++) {
                float top = (float)a[ch] + ((float)b[ch] - (float)a[ch]) * wx;
                float bot = (float)c[ch] + ((float)e[ch] - (float)c[ch]) * wx;
                o[ch] = (unsigned char)(top + (bot - top) * wy + 0.5f);
            }
        }
    }
    g_origTexUnlockRect(w->texture, 0);
}

// Reads back what the game uploaded and reports it. Matching that against
// the asset index tells us which archive entry ended up in which texture,
// which is what a higher resolution replacement needs.
static void observeTexture(WatchedTexture* w) {
    LockedRect lr;
    if (FAILED(g_origTexLockRect(w->texture, 0, &lr, 0, 0)) || !lr.pBits) {
        return;
    }
    // A surface can be stored bottom-up, in which case the pitch is negative
    // and the rows run backwards from pBits.
    int pitch = lr.Pitch;
    bool bottomUp = pitch < 0;
    if (bottomUp) {
        pitch = -pitch;
    }
    unsigned int crc = 0xFFFFFFFFu;
    for (unsigned int y = 0; y < w->scaledHeight; y++) {
        // The offset has to be computed as a signed value: a bottom-up
        // surface runs backwards from pBits.
        long offset = bottomUp ? -(long)y * (long)pitch : (long)y * (long)pitch;
        const unsigned char* row = (const unsigned char*)lr.pBits + offset;
        for (unsigned int x = 0; x < w->scaledWidth * 4; x++) {
            crc = (crc >> 8) ^ g_crcTable[(crc ^ row[x]) & 0xFF];
        }
    }
    crc ^= 0xFFFFFFFFu;
    g_origTexUnlockRect(w->texture, 0);
    log().write("INFO", "d3d8: uploaded %ux%u crc=0x%08X",
                (unsigned)w->scaledWidth, (unsigned)w->scaledHeight, crc);
}

static HRESULT __stdcall detourTexLockRect(void* texture, UINT level, LockedRect* pLocked,
                                           const void* pRect, DWORD flags) {
    WatchedTexture* w = findWatched(texture);
    HRESULT hr = g_origTexLockRect(texture, level, pLocked, pRect, flags);
    if (g_logLocks) {
        log().write("INFO", "d3d8: texture LockRect tex=%p level=%u scaled=%d flags=0x%X",
                    texture, (unsigned)level, (w && w->scaledWidth != w->width) ? 1 : 0, (unsigned)flags);
    }
    if (!w || FAILED(hr) || !pLocked) {
        return hr;
    }
    if (w->scaledWidth == w->width) {
        // Not enlarged: the upload goes straight to the surface, as the game expects.
        return hr;
    }
    // The game's upload is only as large as the texture it asked for, so it is
    // diverted into a scratch buffer. The real surface is filled afterwards.
    size_t need = (size_t)w->scaledWidth * w->scaledHeight * 4;
    size_t srcNeed = (size_t)w->width * w->height * 4;
    if (need < srcNeed) {
        need = srcNeed;
    }
    if (!w->scratch || w->scratchSize < need) {
        if (w->scratch) {
            free(w->scratch);
        }
        w->scratch = (unsigned char*)malloc(need ? need : 4);
        w->scratchSize = w->scratch ? need : 0;
    }
    if (!w->scratch) {
        return hr;
    }
    memset(w->scratch, 0, need);
    w->scratchPitch = (int)(w->width * 4);
    pLocked->pBits = w->scratch;
    pLocked->Pitch = w->scratchPitch;
    w->locked = 1;
    return hr;
}

// Computes CRC32 of uploaded texture content to disambiguate identical dimensions.
static unsigned int computeTextureCrc(WatchedTexture* w) {
    if (!w) {
        return 0;
    }
    if (w->scratch) {
        unsigned int crc = 0xFFFFFFFFu;
        for (unsigned int y = 0; y < w->height; y++) {
            const unsigned char* row = w->scratch + (size_t)y * (size_t)w->scratchPitch;
            for (unsigned int x = 0; x < w->width * 4; x++) {
                crc = (crc >> 8) ^ g_crcTable[(crc ^ row[x]) & 0xFF];
            }
        }
        return crc ^ 0xFFFFFFFFu;
    }
    LockedRect lr;
    if (FAILED(g_origTexLockRect(w->texture, 0, &lr, 0, 0)) || !lr.pBits) {
        return 0;
    }
    int pitch = lr.Pitch;
    bool bottomUp = pitch < 0;
    if (bottomUp) {
        pitch = -pitch;
    }
    unsigned int crc = 0xFFFFFFFFu;
    for (unsigned int y = 0; y < w->scaledHeight; y++) {
        long offset = bottomUp ? -(long)y * (long)pitch : (long)y * (long)pitch;
        const unsigned char* row = (const unsigned char*)lr.pBits + offset;
        for (unsigned int x = 0; x < w->scaledWidth * 4; x++) {
            crc = (crc >> 8) ^ g_crcTable[(crc ^ row[x]) & 0xFF];
        }
    }
    crc ^= 0xFFFFFFFFu;
    g_origTexUnlockRect(w->texture, 0);
    return crc;
}

// Replaces the contents of a texture with artwork supplied at a higher
// resolution, filtered down to the size the game asked for.
//
// This runs after the game has uploaded its own art, so the result is a
// supersampled image rather than a low resolution source stretched by the
// hardware.
static void applyHiResArt(WatchedTexture* w) {
    unsigned int crc = computeTextureCrc(w);
    const HiResArt* art = findHiResArt(w->width, w->height, crc);
    if (!art) {
        return;
    }
    // The real size is read back from the texture rather than taken from the
    // record, so a texture that has been released and re-created at the same
    // address cannot cause a write of the wrong length.
    SurfaceDesc desc;
    if (FAILED(g_origGetLevelDesc(w->texture, 0, &desc)) || desc.Width == 0 || desc.Height == 0) {
        return;
    }
    LockedRect lr;
    if (FAILED(g_origTexLockRect(w->texture, 0, &lr, 0, 0)) || !lr.pBits) {
        return;
    }
    // A row has to fit inside the surface's pitch, or the write would run off
    // the end of it.
    int pitch = lr.Pitch;
    bool bottomUp = pitch < 0;
    if (bottomUp) {
        pitch = -pitch;
    }
    if ((unsigned int)pitch < desc.Width * 4) {
        g_origTexUnlockRect(w->texture, 0);
        return;
    }
    bool ok = drawHiResArt(art, desc.Width, desc.Height, (unsigned char*)lr.pBits, lr.Pitch);
    g_origTexUnlockRect(w->texture, 0);
    if (ok) {
        g_hiresApplied++;
    }
}

static HRESULT __stdcall detourTexUnlockRect(void* texture, UINT level) {
    WatchedTexture* w = findWatched(texture);
    HRESULT hr = g_origTexUnlockRect(texture, level);
    if (w && w->locked) {
        w->locked = 0;
        fillScaledTexture(w);
        applyHiResArt(w);
    }
    else if (w) {
        // The game has finished uploading its own art. Replace the texture with
        // the higher resolution artwork from the pack, if there is any.
        applyHiResArt(w);
        if (g_observeTextures) {
            observeTexture(w);
        }
    }
    return hr;
}

static HRESULT __stdcall detourSurfLockRect(void* surface, LockedRect* pLocked,
                                            const void* pRect, DWORD flags) {
    if (g_logLocks) {
        log().write("INFO", "d3d8: surface LockRect surf=%p", surface);
    }
    return g_origSurfLockRect(surface, pLocked, pRect, flags);
}

static HRESULT __stdcall detourSurfUnlockRect(void* surface) {
    if (g_logLocks) {
        log().write("INFO", "d3d8: surface UnlockRect surf=%p", surface);
    }
    return g_origSurfUnlockRect(surface);
}

// ---------------------------------------------------------------- install

bool installD3D8() {
    g_logTextures = config().getBool("graphics", "log_textures", false);
    g_logLocks = config().getBool("graphics", "log_locks", false);
    g_observeTextures = config().getBool("graphics", "observe_textures", false);
    g_texScale = config().getInt("graphics", "texture_scale", 1);
    if (g_texScale < 1) {
        g_texScale = 1;
    }
    initCrcTable();
    g_hiresCount = loadHiresPack();

    unsigned int addr = manifest().getAddr("d3d.createdevice");
    if (!addr) {
        log().write("WARN", "d3d8: manifest has no d3d.createdevice entry");
        return false;
    }
    if (MH_CreateHook((void*)addr, (void*)detourCreateDeviceEntry, (LPVOID*)&g_origCreateDevice) != MH_OK ||
        MH_EnableHook((void*)addr) != MH_OK) {
        log().write("WARN", "d3d8: failed to hook the device creation routine (0x%08X)", addr);
        return false;
    }
    hooks().install("d3d.create_device", (void*)addr, (void*)detourCreateDeviceEntry,
                    manifest().get("sig.d3d.createdevice", 0));
    log().write("INFO", "d3d8: device creation hook installed at 0x%08X", addr);

    // The texture-size setter. It is what the game uses to decide how big a
    // texture is, so it is also the lever for higher resolution.
    unsigned int sizeAddr = manifest().getAddr("tex.set_size");
    if (!sizeAddr) {
        log().write("WARN", "d3d8: manifest has no tex.set_size entry");
        return true;
    }
    if (MH_CreateHook((void*)sizeAddr, (void*)detourSetTextureSize, (LPVOID*)&g_origSetTextureSize) != MH_OK ||
        MH_EnableHook((void*)sizeAddr) != MH_OK) {
        log().write("WARN", "d3d8: failed to hook the texture size setter (0x%08X)", sizeAddr);
        return true;
    }
    hooks().install("tex.set_size", (void*)sizeAddr, (void*)detourSetTextureSize,
                    manifest().get("sig.tex.set_size", 0));
    log().write("INFO", "d3d8: texture size hook installed at 0x%08X", sizeAddr);
    return true;
}

void reportD3D8() {
    log().write("INFO", "d3d8: %d textures created, %d size changes, %d hi-res replacements", g_textureCount, g_sizeCalls, g_hiresApplied);
}

void shutdownD3D8() {
    if (G_DEVICE && *(void**)G_DEVICE) {
        restorePostProcess(*(void**)G_DEVICE);
    }
}

}  // namespace graphics
}  // namespace remaster





