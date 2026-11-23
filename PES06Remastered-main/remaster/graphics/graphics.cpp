// remaster graphics subsystem - resolution, projection, and window management.
//
// Hooks and memory interventions:
//
//  FUN_00404C60  D3D init. Writes resolution globals (0x00F83FD8 width / 0x00F83FDC height).
//                We let it run, then overwrite resolution, projection matrix width (0x00F84090),
//                cull width (0x00F84064), and internal rendering resolution (0x011631F8/FC).
//
//  FUN_00410C20  CreateWindowExA wrapper. Takes client width/height as arguments.
//
//  Internal Resolution:
//                Pointers at 0x011631F8 (width) and 0x011631FC (height) define the
//                dimensions of the internal 3D scene render target before backbuffer
//                presentation (the Kingsley813 discovery).
//
//  Aspect Ratio Correction:
//                0x00F84090 (projWidth, float): 512.0f * (0.75f * aspect_ratio)
//                0x00F84064 (cullWidth, short): (short)(projWidth)
//                Eliminates horizontal stretching/squashing and expands FOV naturally.

#include "graphics.h"
#include "../core/remaster.h"

#ifndef REMASTER_NO_MINHOOK
#include "MinHook.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace remaster {
namespace graphics {

// Game globals (image base 0x00400000).
static const unsigned int G_WIDTH = 0x00F83FD8;
static const unsigned int G_HEIGHT = 0x00F83FDC;
static const unsigned int G_WIDTH2 = 0x00F83FE0;
static const unsigned int G_HEIGHT2 = 0x00F83FE4;
static const unsigned int G_CULL_WIDTH = 0x00F84064;
static const unsigned int G_PROJ_WIDTH = 0x00F84090;

// Internal rendering scene target globals.
static const unsigned int G_INTERNAL_WIDTH = 0x011631F8;
static const unsigned int G_INTERNAL_HEIGHT = 0x011631FC;

typedef int(__cdecl* FnD3DInit)(void*, void*);
typedef void*(__cdecl* FnCreateWindow)(void*, int, int);

static FnD3DInit g_origD3DInit = 0;
static FnCreateWindow g_origCreateWindow = 0;

static int g_width = 0;
static int g_height = 0;
static int g_internalWidth = 0;
static int g_internalHeight = 0;
static char g_aspectSetting[32] = "auto";

static QualitySettings g_qualitySettings = {
    PRESET_HD,
    1920,
    1080,
    16,
    true,
    true,
    1.10f,
    1.15f
};

QualityPreset parsePreset(const char* str) {
    if (!str || str[0] == '\0') return PRESET_HD;
    if (_stricmp(str, "classic") == 0) return PRESET_CLASSIC;
    if (_stricmp(str, "enhanced") == 0) return PRESET_ENHANCED;
    if (_stricmp(str, "hd") == 0) return PRESET_HD;
    if (_stricmp(str, "maximum") == 0) return PRESET_MAXIMUM;
    if (_stricmp(str, "ultra") == 0 || _stricmp(str, "4k") == 0 || _stricmp(str, "ultra_4k") == 0) return PRESET_ULTRA_4K;
    return PRESET_CUSTOM;
}

void setQualityPreset(QualityPreset preset) {
    g_qualitySettings.preset = preset;
    switch (preset) {
        case PRESET_CLASSIC:
            g_qualitySettings.internalWidth = 0;
            g_qualitySettings.internalHeight = 0;
            g_qualitySettings.anisotropicLevel = 0;
            g_qualitySettings.colorGrading = false;
            g_qualitySettings.modernHud = false;
            g_qualitySettings.contrast = 1.0f;
            g_qualitySettings.vibrance = 1.0f;
            break;
        case PRESET_ENHANCED:
            g_qualitySettings.internalWidth = 1280;
            g_qualitySettings.internalHeight = 720;
            g_qualitySettings.anisotropicLevel = 8;
            g_qualitySettings.colorGrading = true;
            g_qualitySettings.modernHud = false;
            g_qualitySettings.contrast = 1.05f;
            g_qualitySettings.vibrance = 1.08f;
            break;
        case PRESET_HD:
            g_qualitySettings.internalWidth = 1920;
            g_qualitySettings.internalHeight = 1080;
            g_qualitySettings.anisotropicLevel = 16;
            g_qualitySettings.colorGrading = true;
            g_qualitySettings.modernHud = true;
            g_qualitySettings.contrast = 1.10f;
            g_qualitySettings.vibrance = 1.15f;
            break;
        case PRESET_MAXIMUM:
            g_qualitySettings.internalWidth = 2560;
            g_qualitySettings.internalHeight = 1440;
            g_qualitySettings.anisotropicLevel = 16;
            g_qualitySettings.colorGrading = true;
            g_qualitySettings.modernHud = true;
            g_qualitySettings.contrast = 1.15f;
            g_qualitySettings.vibrance = 1.20f;
            break;
        case PRESET_ULTRA_4K:
            g_qualitySettings.internalWidth = 3840;
            g_qualitySettings.internalHeight = 2160;
            g_qualitySettings.anisotropicLevel = 16;
            g_qualitySettings.colorGrading = true;
            g_qualitySettings.modernHud = true;
            g_qualitySettings.contrast = 1.18f;
            g_qualitySettings.vibrance = 1.25f;
            break;
        case PRESET_CUSTOM:
        default:
            break;
    }
}

const QualitySettings& getQualitySettings() {
    return g_qualitySettings;
}

static float parseAspectRatio(const char* str, int width, int height) {
    if (!str || _stricmp(str, "auto") == 0 || str[0] == '\0') {
        if (width > 0 && height > 0) {
            return (float)width / (float)height;
        }
        return 4.0f / 3.0f;
    }
    if (_stricmp(str, "16:9") == 0) {
        return 16.0f / 9.0f;
    }
    if (_stricmp(str, "16:10") == 0) {
        return 16.0f / 10.0f;
    }
    if (_stricmp(str, "4:3") == 0) {
        return 4.0f / 3.0f;
    }
    if (_stricmp(str, "21:9") == 0) {
        return 64.0f / 27.0f;
    }
    if (_stricmp(str, "32:9") == 0) {
        return 32.0f / 9.0f;
    }
    const char* colon = strchr(str, ':');
    if (colon) {
        float num = (float)atof(str);
        float den = (float)atof(colon + 1);
        if (num > 0.0f && den > 0.0f) {
            return num / den;
        }
    }
    float val = (float)atof(str);
    if (val >= 0.5f && val <= 5.0f) {
        return val;
    }
    if (width > 0 && height > 0) {
        return (float)width / (float)height;
    }
    return 4.0f / 3.0f;
}

ProjectionState calculateProjection(int width, int height, const char* aspectSetting) {
    ProjectionState p;
    p.aspect = parseAspectRatio(aspectSetting, width, height);
    p.aspectFactor = (3.0f / 4.0f) * p.aspect;
    p.projWidth = 512.0f * p.aspectFactor;
    p.cullWidth = (short)(512.0f * p.aspectFactor + 0.5f);
    return p;
}

static void writeResolutionAndProjection(int w, int h, const char* aspectSetting) {
    if (w <= 0 || h <= 0) {
        return;
    }

    *(unsigned int*)G_WIDTH = (unsigned int)w;
    *(unsigned int*)G_HEIGHT = (unsigned int)h;
    *(unsigned int*)G_WIDTH2 = (unsigned int)w;
    *(unsigned int*)G_HEIGHT2 = (unsigned int)h;

    ProjectionState p = calculateProjection(w, h, aspectSetting);

    DWORD oldProtect = 0;
    if (VirtualProtect((void*)G_CULL_WIDTH, sizeof(short), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        *(short*)G_CULL_WIDTH = p.cullWidth;
        DWORD temp = 0;
        VirtualProtect((void*)G_CULL_WIDTH, sizeof(short), oldProtect, &temp);
    } else {
        *(short*)G_CULL_WIDTH = p.cullWidth;
    }

    if (VirtualProtect((void*)G_PROJ_WIDTH, sizeof(float), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        *(float*)G_PROJ_WIDTH = p.projWidth;
        DWORD temp = 0;
        VirtualProtect((void*)G_PROJ_WIDTH, sizeof(float), oldProtect, &temp);
    } else {
        *(float*)G_PROJ_WIDTH = p.projWidth;
    }

    log().write("INFO", "graphics: resolution set to %dx%d, aspect=%.4f (projWidth=%.2f, cullWidth=%d)",
                w, h, p.aspect, p.projWidth, (int)p.cullWidth);
}

static void writeInternalResolution(int intW, int intH) {
    if (intW <= 0 || intH <= 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect((void*)G_INTERNAL_WIDTH, sizeof(unsigned int) * 2, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        *(unsigned int*)G_INTERNAL_WIDTH = (unsigned int)intW;
        *(unsigned int*)G_INTERNAL_HEIGHT = (unsigned int)intH;
        DWORD temp = 0;
        VirtualProtect((void*)G_INTERNAL_WIDTH, sizeof(unsigned int) * 2, oldProtect, &temp);
        log().write("INFO", "graphics: internal rendering resolution set to %dx%d", intW, intH);
    } else {
        *(unsigned int*)G_INTERNAL_WIDTH = (unsigned int)intW;
        *(unsigned int*)G_INTERNAL_HEIGHT = (unsigned int)intH;
        log().write("INFO", "graphics: internal rendering resolution set to %dx%d", intW, intH);
    }
}

static int __cdecl detourD3DInit(void* self, void* arg2) {
    int result = g_origD3DInit(self, arg2);
    if (g_width > 0 && g_height > 0) {
        writeResolutionAndProjection(g_width, g_height, g_aspectSetting);
    }
    if (g_internalWidth > 0 && g_internalHeight > 0) {
        writeInternalResolution(g_internalWidth, g_internalHeight);
    }
    return result;
}

static void* __cdecl detourCreateWindow(void* hInstance, int width, int height) {
    if (g_width > 0 && g_height > 0) {
        log().write("INFO", "graphics: window %dx%d -> %dx%d", width, height, g_width, g_height);
        width = g_width;
        height = g_height;
    }
    return g_origCreateWindow(hInstance, width, height);
}

void onDeviceCreated() {
    if (g_width > 0 && g_height > 0) {
        writeResolutionAndProjection(g_width, g_height, g_aspectSetting);
    }
    if (g_internalWidth > 0 && g_internalHeight > 0) {
        writeInternalResolution(g_internalWidth, g_internalHeight);
    }
}

bool install() {
    bool ok = true;

    g_width = config().getInt("graphics", "width", 0);
    g_height = config().getInt("graphics", "height", 0);
    bool windowed = config().getBool("graphics", "windowed", true);

    const char* aspectCfg = config().get("graphics", "aspect_ratio", "auto");
    if (aspectCfg) {
        strncpy(g_aspectSetting, aspectCfg, sizeof(g_aspectSetting) - 1);
        g_aspectSetting[sizeof(g_aspectSetting) - 1] = '\0';
    }

    const char* presetStr = config().get("graphics", "preset", "hd");
    QualityPreset qp = parsePreset(presetStr);
    setQualityPreset(qp);
    log().write("INFO", "graphics: quality preset set to '%s' (preset=%d)", presetStr, (int)qp);

    int cfgIntW = config().getInt("graphics", "internal_resolution_width", -1);
    int cfgIntH = config().getInt("graphics", "internal_resolution_height", -1);
    if (cfgIntW > 0) g_qualitySettings.internalWidth = cfgIntW;
    if (cfgIntH > 0) g_qualitySettings.internalHeight = cfgIntH;

    g_internalWidth = g_qualitySettings.internalWidth;
    g_internalHeight = g_qualitySettings.internalHeight;
    if (g_internalWidth <= 0 && g_width > 0) {
        g_internalWidth = g_width;
    }
    if (g_internalHeight <= 0 && g_height > 0) {
        g_internalHeight = g_height;
    }

    if (!config().getBool("graphics", "enabled", true)) {
        log().write("INFO", "graphics: disabled in config");
        return true;
    }
    if (g_width <= 0 || g_height <= 0) {
        log().write("INFO", "graphics: no width/height configured - subsystem inactive");
        return true;
    }
    if (!windowed) {
        log().write("INFO", "graphics: windowed=0 requested but fullscreen is not "
                            "implemented yet - forcing windowed");
    }

    unsigned int initAddr = manifest().getAddr("d3d.init");
    unsigned int winAddr = manifest().getAddr("d3d.createwindow");

    if (!initAddr || !winAddr) {
        log().write("WARN", "graphics: manifest is missing d3d.init / d3d.createwindow");
        return false;
    }

#ifndef REMASTER_NO_MINHOOK
    // d3d.init - validate signature before installing hook
    if (!hooks().install("graphics.d3d_init", (void*)initAddr, (void*)detourD3DInit,
                         "sig.d3d.init", (void**)&g_origD3DInit)) {
        log().write("WARN", "graphics: failed to hook d3d.init (0x%08X)", initAddr);
        ok = false;
    }

    // d3d.createwindow - validate signature before installing hook
    if (!hooks().install("graphics.create_window", (void*)winAddr, (void*)detourCreateWindow,
                         "sig.d3d.createwindow", (void**)&g_origCreateWindow)) {
        log().write("WARN", "graphics: failed to hook d3d.createwindow (0x%08X)", winAddr);
        ok = false;
    }
#endif

    return ok;
}

}  // namespace graphics
}  // namespace remaster
