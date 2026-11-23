#include "postprocess.h"
#include "../core/remaster.h"

#include <math.h>
#include <string.h>

namespace remaster {
namespace graphics {

static PostProcessConfig g_config = {
    true,   // enabled
    1.10f,  // contrast (+10% rich contrast)
    1.15f,  // vibrance (+15% lush pitch and kit colors)
    0.02f,  // brightness (slight lift to shadow details)
    1.00f,  // gamma (neutral baseline)
    true    // modernHud
};

// IDirect3DDevice8::SetGammaRamp signature:
// void SetGammaRamp(DWORD Flags, const D3DGAMMARAMP* pRamp);
// Slot 18 in IDirect3DDevice8 vtable.
// Slot 19 = IDirect3DDevice8::GetGammaRamp(D3DGAMMARAMP* pRamp);
typedef void(__stdcall* FnSetGammaRamp)(void*, DWORD, const D3DGAMMARAMP*);
typedef void(__stdcall* FnGetGammaRamp)(void*, D3DGAMMARAMP*);

static D3DGAMMARAMP s_origRamp;
static bool s_hasOrigRamp = false;
static bool s_isApplied = false;

static float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void buildGammaRamp(D3DGAMMARAMP* ramp, float contrast, float vibrance, float brightness, float gamma) {
    if (!ramp) return;

    if (contrast < 0.5f) contrast = 0.5f;
    if (contrast > 2.0f) contrast = 2.0f;
    if (vibrance < 0.5f) vibrance = 0.5f;
    if (vibrance > 2.0f) vibrance = 2.0f;
    if (gamma < 0.5f) gamma = 0.5f;
    if (gamma > 2.0f) gamma = 2.0f;

    float invGamma = 1.0f / gamma;

    for (int i = 0; i < 256; ++i) {
        float x = (float)i / 255.0f;

        // 1. Lift brightness in mid-shadows without lifting black point
        float xBright = x + brightness * sinf(x * 3.14159265f);
        xBright = clamp01(xBright);

        // 2. Gamma exponent
        float xGamma = powf(xBright, invGamma);

        // 3. Contrast S-curve centered at 0.5
        float centered = xGamma - 0.5f;
        float xContrast = 0.5f + contrast * centered;
        // Soft curve rolloff
        if (xContrast < 0.0f) xContrast = xGamma * (1.0f - (1.0f - contrast) * 0.5f);
        if (xContrast > 1.0f) xContrast = 1.0f - (1.0f - xGamma) * (1.0f - (1.0f - contrast) * 0.5f);
        xContrast = clamp01(xContrast);

        // 4. Per-channel vibrance tuning
        // Green channel gets vibrant pitch enhancement in midtones
        float gMid = sinf(xContrast * 3.14159265f);
        float gVal = clamp01(xContrast + (vibrance - 1.0f) * 0.08f * gMid);

        // Red channel (team kits / warm tones)
        float rVal = clamp01(xContrast + (vibrance - 1.0f) * 0.04f * gMid);

        // Blue channel (sky / cool tones / contrast preservation)
        float bVal = clamp01(xContrast + (vibrance - 1.0f) * 0.03f * gMid);

        ramp->red[i] = (WORD)(rVal * 65535.0f + 0.5f);
        ramp->green[i] = (WORD)(gVal * 65535.0f + 0.5f);
        ramp->blue[i] = (WORD)(bVal * 65535.0f + 0.5f);
    }

    // Strict boundary enforcement (pure black at 0, pure white at 255)
    ramp->red[0] = 0;
    ramp->green[0] = 0;
    ramp->blue[0] = 0;

    ramp->red[255] = 65535;
    ramp->green[255] = 65535;
    ramp->blue[255] = 65535;

    // Enforce monotonic increase across all channels
    for (int i = 1; i < 256; ++i) {
        if (ramp->red[i] < ramp->red[i - 1]) ramp->red[i] = ramp->red[i - 1];
        if (ramp->green[i] < ramp->green[i - 1]) ramp->green[i] = ramp->green[i - 1];
        if (ramp->blue[i] < ramp->blue[i - 1]) ramp->blue[i] = ramp->blue[i - 1];
    }
}

bool initPostProcess() {
    g_config.enabled = config().getBool("graphics", "color_grading", true);
    g_config.contrast = config().getFloat("graphics", "contrast", 1.10f);
    g_config.vibrance = config().getFloat("graphics", "vibrance", 1.15f);
    g_config.brightness = config().getFloat("graphics", "brightness", 0.02f);
    g_config.gamma = config().getFloat("graphics", "gamma", 1.00f);
    g_config.modernHud = config().getBool("graphics", "modern_hud", true);

    log().write("INFO", "postprocess: initialized [enabled=%d, contrast=%.2f, vibrance=%.2f, gamma=%.2f, hud=%d]",
                g_config.enabled ? 1 : 0, g_config.contrast, g_config.vibrance, g_config.gamma, g_config.modernHud ? 1 : 0);
    return true;
}

bool applyPostProcess(void* d3dDevice) {
    if (!g_config.enabled || !d3dDevice) {
        return false;
    }

    void** vtable = *(void***)d3dDevice;
    if (!vtable) return false;

    // Slot 18 = SetGammaRamp, Slot 19 = GetGammaRamp
    FnSetGammaRamp setGammaRamp = (FnSetGammaRamp)vtable[18];
    FnGetGammaRamp getGammaRamp = (FnGetGammaRamp)vtable[19];
    if (!setGammaRamp) {
        log().write("WARN", "postprocess: SetGammaRamp slot 18 not available");
        return false;
    }

    // Save initial hardware gamma ramp if not already preserved
    if (!s_hasOrigRamp && getGammaRamp) {
        getGammaRamp(d3dDevice, &s_origRamp);
        s_hasOrigRamp = true;
        log().write("INFO", "postprocess: original hardware gamma ramp preserved for clean restoration");
    }

    D3DGAMMARAMP ramp;
    buildGammaRamp(&ramp, g_config.contrast, g_config.vibrance, g_config.brightness, g_config.gamma);

    // Direct3D 8 SetGammaRamp returns void
    setGammaRamp(d3dDevice, 0, &ramp);
    s_isApplied = true;
    log().write("INFO", "postprocess: hardware cinematic gamma ramp applied successfully");
    return true;
}

void restorePostProcess(void* d3dDevice) {
    if (!s_isApplied) return;

    if (d3dDevice) {
        void** vtable = *(void***)d3dDevice;
        if (vtable && vtable[18]) {
            FnSetGammaRamp setGammaRamp = (FnSetGammaRamp)vtable[18];
            if (s_hasOrigRamp) {
                setGammaRamp(d3dDevice, 0, &s_origRamp);
                log().write("INFO", "postprocess: restored original hardware gamma ramp to D3D8 device");
            } else {
                // Identity ramp fallback
                D3DGAMMARAMP identity;
                for (int i = 0; i < 256; ++i) {
                    WORD v = (WORD)((float)i / 255.0f * 65535.0f + 0.5f);
                    identity.red[i] = v;
                    identity.green[i] = v;
                    identity.blue[i] = v;
                }
                setGammaRamp(d3dDevice, 0, &identity);
                log().write("INFO", "postprocess: restored linear identity gamma ramp to D3D8 device");
            }
        }
    }

    // Fallback: GDI desktop ramp restoration in case of windowed / desktop lingering
    HDC hdc = GetDC(NULL);
    if (hdc) {
        if (s_hasOrigRamp) {
            SetDeviceGammaRamp(hdc, &s_origRamp);
        }
        ReleaseDC(NULL, hdc);
    }

    s_isApplied = false;
}

bool isPostProcessApplied() {
    return s_isApplied;
}

const PostProcessConfig& getPostProcessConfig() {
    return g_config;
}

void setPostProcessConfig(const PostProcessConfig& cfg) {
    g_config = cfg;
}

}  // namespace graphics
}  // namespace remaster
