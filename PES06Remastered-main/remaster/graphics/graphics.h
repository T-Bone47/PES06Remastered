#ifndef REMASTER_GRAPHICS_H
#define REMASTER_GRAPHICS_H

namespace remaster {
namespace graphics {

struct ProjectionState {
    float aspect;
    float aspectFactor;
    float projWidth;
    short cullWidth;
};

enum QualityPreset {
    PRESET_CLASSIC = 0,
    PRESET_ENHANCED = 1,
    PRESET_HD = 2,
    PRESET_MAXIMUM = 3,
    PRESET_ULTRA_4K = 4,
    PRESET_CUSTOM = 5
};

struct QualitySettings {
    QualityPreset preset;
    int internalWidth;
    int internalHeight;
    int anisotropicLevel;
    bool colorGrading;
    bool modernHud;
    float contrast;
    float vibrance;
};

// Calculates projection parameters from display dimensions and aspect ratio setting.
ProjectionState calculateProjection(int width, int height, const char* aspectSetting);

// Quality preset accessors
const QualitySettings& getQualitySettings();
QualityPreset parsePreset(const char* str);
void setQualityPreset(QualityPreset preset);

// Installs the graphics hooks (resolution globals, window creation, projection, internal resolution).
bool install();

// Called when Direct3D device creation succeeds to ensure internal resolution and projection persist.
void onDeviceCreated();

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_H
