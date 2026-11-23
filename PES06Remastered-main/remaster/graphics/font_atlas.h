#ifndef REMASTER_FONT_ATLAS_H
#define REMASTER_FONT_ATLAS_H

#include <stddef.h>

namespace remaster {
namespace graphics {

struct GlyphMetric {
    char character;
    int x;
    int y;
    int width;
    int height;
    int advance;
    float u0, v0;
    float u1, v1;
};

struct FontConfig {
    const char* fontFace;     // e.g. "Arial", "Segoe UI", "Tahoma"
    int pointSize;            // e.g. 24, 32, 48
    int weight;               // FW_NORMAL (400), FW_BOLD (700)
    bool italic;
    unsigned int textColor;   // 0xAARRGGBB
    unsigned int shadowColor; // 0xAARRGGBB (0 = no shadow)
    int shadowOffset;         // in pixels
    int atlasWidth;           // 1024, 2048
    int atlasHeight;          // 1024, 2048
};

struct FontAtlas {
    unsigned int width;
    unsigned int height;
    unsigned char* bgraPixels; // 32-bit BGRA (D3DFMT_A8R8G8B8)
    size_t totalBytes;
    int glyphCount;
    GlyphMetric glyphs[128];   // ASCII 0..127
};

// Generates an antialiased HD font atlas using Windows GDI DIB section.
bool generateFontAtlas(const FontConfig& config, FontAtlas* outAtlas);

// Releases memory allocated by generateFontAtlas.
void freeFontAtlas(FontAtlas* atlas);

// Looks up glyph metrics for a given character. Returns nullptr if unmapped.
const GlyphMetric* getGlyphMetric(const FontAtlas* atlas, char c);

// Measures text pixel width based on atlas glyph metrics.
int measureTextWidth(const FontAtlas* atlas, const char* text);

// Renders an antialiased string into a 32-bit BGRA target buffer with alpha blending.
bool renderTextToBgra(const FontAtlas* atlas, const char* text,
                      unsigned char* targetBgra, int targetWidth, int targetHeight,
                      int targetPitch, int startX, int startY);

}  // namespace graphics
}  // namespace remaster

#endif  // REMASTER_FONT_ATLAS_H
