#ifndef REMASTER_GRAPHICS_H
#define REMASTER_GRAPHICS_H

namespace remaster {
namespace graphics {

// Installs the graphics hooks. Returns false if a hook failed, but a false
// result never aborts the rest of the remaster.
bool install();

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_H
