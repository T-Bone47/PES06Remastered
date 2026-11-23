#ifndef REMASTER_GRAPHICS_HIRES_H
#define REMASTER_GRAPHICS_H

namespace remaster {
namespace graphics {

// Artwork supplied at a higher resolution than the texture it replaces.
//
// The game asks for a texture of a fixed size and uploads its own artwork
// into it. The remaster can replace the contents of that texture with
// artwork supplied at a higher resolution, filtered down into the texture.
// The result is a properly supersampled image where the game previously had
// a low-resolution source that it stretched itself.
//
// Making the texture itself larger does not work: the game's two
// dimensional rendering is tied to the size of the texture it asked for.

// An image decoded into top-down 32 bit BGRA, which is the memory order of
// D3DFMT_A8R8G8B8.
struct HiResArt {
    unsigned int width;
    unsigned int height;
    unsigned char* pixels;   // width * height * 4, BGRA
};

// Reads the pack's index. Returns the number of entries found.
int loadHiresPack();

// Clears the pack entries (used for testing or reload).
void clearHiresPack();

// Registers an in-memory or pack entry for texture substitution.
bool addHiresEntry(unsigned int width, unsigned int height, unsigned int crc,
                   const char* file, const HiResArt* art = nullptr);

// The artwork for a texture of the given size and optional CRC32 hash,
// or null if the pack has none. Exact CRC match takes precedence over wildcard.
const HiResArt* findHiResArt(unsigned int width, unsigned int height, unsigned int crc = 0);

// Filters the artwork down into width x height and writes it into `out`,
// which is expected to hold at least width * height * 4 bytes. `pitch` is the
// distance between rows in `out`; a negative pitch means the rows run
// backwards, which D3D8 uses for bottom-up surfaces.
bool drawHiResArt(const HiResArt* art, unsigned int width, unsigned int height,
                  unsigned char* out, int pitch);

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_GRAPHICS_HIRES_H
