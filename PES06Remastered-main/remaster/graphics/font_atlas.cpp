// remaster graphics subsystem - high-resolution font atlas generator.
//
// Implements vector-to-raster HD font generation for Pro Evolution Soccer 6
// menus, scoreboard, and in-game tactical overlays. Generates 32-bit BGRA
// Direct3D 8-compatible font atlas sheets with subpixel antialiasing and
// precise UV coordinate mapping.

#include "font_atlas.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace remaster {
namespace graphics {

bool generateFontAtlas(const FontConfig& config, FontAtlas* outAtlas) {
    if (!outAtlas) return false;
    memset(outAtlas, 0, sizeof(FontAtlas));

    int w = config.atlasWidth > 0 ? config.atlasWidth : 1024;
    int h = config.atlasHeight > 0 ? config.atlasHeight : 1024;
    if (w < 256) w = 256;
    if (h < 256) h = 256;
    if (w > 4096) w = 4096;
    if (h > 4096) h = 4096;

    const char* face = (config.fontFace && config.fontFace[0]) ? config.fontFace : "Segoe UI";
    int ptSize = config.pointSize > 0 ? config.pointSize : 28;
    int weight = config.weight > 0 ? config.weight : FW_BOLD;

    HDC screenDc = GetDC(nullptr);
    if (!screenDc) return false;
    HDC memDc = CreateCompatibleDC(screenDc);
    if (!memDc) {
        ReleaseDC(nullptr, screenDc);
        return false;
    }

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* dibBits = nullptr;
    HBITMAP dibBmp = CreateDIBSection(memDc, &bmi, DIB_RGB_COLORS, &dibBits, nullptr, 0);
    if (!dibBmp || !dibBits) {
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        return false;
    }

    HGDIOBJ oldBmp = SelectObject(memDc, dibBmp);

    // Calculate font height in logical units
    int logPixelsY = GetDeviceCaps(screenDc, LOGPIXELSY);
    int fontHeight = -MulDiv(ptSize, logPixelsY, 72);

    HFONT hFont = CreateFontA(
        fontHeight, 0, 0, 0, weight,
        config.italic ? TRUE : FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face
    );
    if (!hFont) {
        // Fallback to Arial
        hFont = CreateFontA(
            fontHeight, 0, 0, 0, weight,
            config.italic ? TRUE : FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial"
        );
    }

    HGDIOBJ oldFont = SelectObject(memDc, hFont);

    // Clear DIB background to black (0x00000000)
    size_t totalBytes = (size_t)w * h * 4;
    memset(dibBits, 0, totalBytes);

    SetBkMode(memDc, TRANSPARENT);
    SetTextColor(memDc, RGB(255, 255, 255));

    // Glyph layout: printable ASCII 32 (' ') to 126 ('~')
    int padding = 4;
    int curX = padding;
    int curY = padding;
    int rowHeight = 0;

    TEXTMETRICA tm;
    GetTextMetricsA(memDc, &tm);
    rowHeight = tm.tmHeight + padding * 2;

    int mappedCount = 0;
    for (int c = 32; c <= 126; c++) {
        char ch = (char)c;
        char str[2] = { ch, '\0' };

        SIZE sz;
        GetTextExtentPoint32A(memDc, str, 1, &sz);

        int charW = sz.cx > 0 ? sz.cx : (tm.tmAveCharWidth);
        int cellW = charW + padding * 2;

        if (curX + cellW >= w - padding) {
            curX = padding;
            curY += rowHeight;
        }

        if (curY + rowHeight >= h - padding) {
            // Out of atlas space
            break;
        }

        // Draw character in pure white on black background
        TextOutA(memDc, curX + padding, curY + padding, str, 1);

        GlyphMetric& gm = outAtlas->glyphs[c];
        gm.character = ch;
        gm.x = curX;
        gm.y = curY;
        gm.width = cellW;
        gm.height = rowHeight;
        gm.advance = charW;
        gm.u0 = (float)curX / (float)w;
        gm.v0 = (float)curY / (float)h;
        gm.u1 = (float)(curX + cellW) / (float)w;
        gm.v1 = (float)(curY + rowHeight) / (float)h;

        curX += cellW;
        mappedCount++;
    }

    // Now extract alpha from grayscale luminance and apply target text color
    unsigned char* raw = (unsigned char*)dibBits;
    unsigned char* bgra = (unsigned char*)malloc(totalBytes);
    if (!bgra) {
        SelectObject(memDc, oldFont);
        SelectObject(memDc, oldBmp);
        DeleteObject(hFont);
        DeleteObject(dibBmp);
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        return false;
    }

    unsigned int targetR = (config.textColor >> 16) & 0xFF;
    unsigned int targetG = (config.textColor >> 8) & 0xFF;
    unsigned int targetB = (config.textColor) & 0xFF;
    if (config.textColor == 0) {
        // Default to solid white
        targetR = 255;
        targetG = 255;
        targetB = 255;
    }

    for (size_t i = 0; i < (size_t)w * h; i++) {
        unsigned char lum = raw[i * 4 + 0]; // R or B from white text
        if (lum > 0) {
            bgra[i * 4 + 0] = (unsigned char)((targetB * lum) / 255); // B
            bgra[i * 4 + 1] = (unsigned char)((targetG * lum) / 255); // G
            bgra[i * 4 + 2] = (unsigned char)((targetR * lum) / 255); // R
            bgra[i * 4 + 3] = lum;                                    // A
        } else {
            bgra[i * 4 + 0] = 0;
            bgra[i * 4 + 1] = 0;
            bgra[i * 4 + 2] = 0;
            bgra[i * 4 + 3] = 0;
        }
    }

    // Clean up GDI objects
    SelectObject(memDc, oldFont);
    SelectObject(memDc, oldBmp);
    DeleteObject(hFont);
    DeleteObject(dibBmp);
    DeleteDC(memDc);
    ReleaseDC(nullptr, screenDc);

    outAtlas->width = (unsigned int)w;
    outAtlas->height = (unsigned int)h;
    outAtlas->bgraPixels = bgra;
    outAtlas->totalBytes = totalBytes;
    outAtlas->glyphCount = mappedCount;

    return true;
}

void freeFontAtlas(FontAtlas* atlas) {
    if (!atlas) return;
    if (atlas->bgraPixels) {
        free(atlas->bgraPixels);
        atlas->bgraPixels = nullptr;
    }
    memset(atlas, 0, sizeof(FontAtlas));
}

const GlyphMetric* getGlyphMetric(const FontAtlas* atlas, char c) {
    if (!atlas || !atlas->bgraPixels) return nullptr;
    unsigned char idx = (unsigned char)c;
    if (idx < 32 || idx > 126) return nullptr;
    if (atlas->glyphs[idx].width == 0) return nullptr;
    return &atlas->glyphs[idx];
}

int measureTextWidth(const FontAtlas* atlas, const char* text) {
    if (!atlas || !text) return 0;
    int total = 0;
    for (const char* p = text; *p; p++) {
        const GlyphMetric* gm = getGlyphMetric(atlas, *p);
        if (gm) {
            total += gm->advance;
        } else if (*p == ' ') {
            const GlyphMetric* sp = getGlyphMetric(atlas, ' ');
            total += sp ? sp->advance : 8;
        }
    }
    return total;
}

bool renderTextToBgra(const FontAtlas* atlas, const char* text,
                      unsigned char* targetBgra, int targetWidth, int targetHeight,
                      int targetPitch, int startX, int startY) {
    if (!atlas || !atlas->bgraPixels || !text || !targetBgra) return false;
    if (targetWidth <= 0 || targetHeight <= 0 || targetPitch <= 0) return false;

    int penX = startX;
    int penY = startY;

    for (const char* p = text; *p; p++) {
        const GlyphMetric* gm = getGlyphMetric(atlas, *p);
        if (!gm) {
            if (*p == ' ') {
                const GlyphMetric* sp = getGlyphMetric(atlas, ' ');
                penX += sp ? sp->advance : 8;
            }
            continue;
        }

        // Blit glyph rect with alpha blending
        int gw = gm->width;
        int gh = gm->height;
        for (int gy = 0; gy < gh; gy++) {
            int dy = penY + gy;
            if (dy < 0 || dy >= targetHeight) continue;

            int sy = gm->y + gy;
            if (sy < 0 || sy >= (int)atlas->height) continue;

            const unsigned char* srcRow = atlas->bgraPixels + (size_t)sy * atlas->width * 4;
            unsigned char* dstRow = targetBgra + (size_t)dy * targetPitch;

            for (int gx = 0; gx < gw; gx++) {
                int dx = penX + gx;
                if (dx < 0 || dx >= targetWidth) continue;

                int sx = gm->x + gx;
                if (sx < 0 || sx >= (int)atlas->width) continue;

                const unsigned char* srcPixel = srcRow + (size_t)sx * 4;
                unsigned char* dstPixel = dstRow + (size_t)dx * 4;

                unsigned int srcA = srcPixel[3];
                if (srcA == 0) continue;

                if (srcA == 255) {
                    dstPixel[0] = srcPixel[0];
                    dstPixel[1] = srcPixel[1];
                    dstPixel[2] = srcPixel[2];
                    dstPixel[3] = 255;
                } else {
                    unsigned int invA = 255 - srcA;
                    dstPixel[0] = (unsigned char)((srcPixel[0] * srcA + dstPixel[0] * invA) / 255);
                    dstPixel[1] = (unsigned char)((srcPixel[1] * srcA + dstPixel[1] * invA) / 255);
                    dstPixel[2] = (unsigned char)((srcPixel[2] * srcA + dstPixel[2] * invA) / 255);
                    dstPixel[3] = (unsigned char)(srcA + (dstPixel[3] * invA) / 255);
                }
            }
        }

        penX += gm->advance;
    }

    return true;
}

}  // namespace graphics
}  // namespace remaster
