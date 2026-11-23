#ifndef REMASTER_GRAPHICS_POSTPROCESS_H
#define REMASTER_GRAPHICS_POSTPROCESS_H

#include <windows.h>

namespace remaster {
namespace graphics {

// Standard D3DGAMMARAMP definition (256 entries x 16-bit word per channel)
#ifndef D3DGAMMARAMP_DEFINED
#define D3DGAMMARAMP_DEFINED
typedef struct _D3DGAMMARAMP {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
} D3DGAMMARAMP;
#endif

struct PostProcessConfig {
    bool enabled;            // Master postprocess toggle
    float contrast;          // Contrast multiplier: 0.8 to 1.5 (default = 1.10)
    float vibrance;          // Color vibrance boost: 0.8 to 1.5 (default = 1.15)
    float brightness;        // Brightness offset: -0.2 to +0.2 (default = 0.02)
    float gamma;             // Gamma exponent: 0.7 to 1.4 (default = 1.0)
    bool modernHud;          // Modern TV broadcast score bug and xG overlay
};

// Generates an S-curve tonemapped gamma ramp with color vibrance enhancement.
// Monotonically maps 0 -> 0 and 255 -> 65535 with zero boundary clipping.
void buildGammaRamp(D3DGAMMARAMP* ramp, float contrast, float vibrance, float brightness, float gamma);

// Initializes the postprocessing subsystem with INI configuration.
bool initPostProcess();

// Applies the calibrated gamma ramp to the Direct3D 8 device.
bool applyPostProcess(void* d3dDevice);

// Restores original hardware gamma ramp upon device release or process exit.
void restorePostProcess(void* d3dDevice);

// Returns whether postprocess color grading is currently applied.
bool isPostProcessApplied();

// Config accessor
const PostProcessConfig& getPostProcessConfig();

// Sets config directly (used by unit tests)
void setPostProcessConfig(const PostProcessConfig& cfg);

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_POSTPROCESS_H
