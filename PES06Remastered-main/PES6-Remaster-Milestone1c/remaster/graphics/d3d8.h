#ifndef REMASTER_GRAPHICS_D3D8_H
#define REMASTER_GRAPHICS_D3D8_H

namespace remaster {
namespace graphics {

// Installs the D3D8 device proxy, which observes texture creation.
// Returns false if the hook could not be installed; the game is unaffected.
bool installD3D8();

// Logs how many textures the game created.
void reportD3D8();

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_D3D8_H
