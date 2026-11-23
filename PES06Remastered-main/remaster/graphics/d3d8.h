#ifndef REMASTER_GRAPHICS_D3D8_H
#define REMASTER_GRAPHICS_D3D8_H

namespace remaster {
namespace graphics {

// Installs the D3D8 device proxy, which observes texture creation.
// Returns false if the hook could not be installed; the game is unaffected.
bool installD3D8();

// Determines whether a COM resource should be evicted upon Release (only when ref count reaches 0).
inline bool shouldForgetResource(unsigned long refCount) {
    return refCount == 0;
}

// Logs how many textures the game created.
void reportD3D8();

// Applies hardware anisotropic filtering to improve pitch and ground clarity at steep camera angles.
void applyAnisotropicFiltering(void* d3dDevice, int level);

// Restores hardware gamma and cleans up device proxies upon exit.
void shutdownD3D8();

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_D3D8_H
