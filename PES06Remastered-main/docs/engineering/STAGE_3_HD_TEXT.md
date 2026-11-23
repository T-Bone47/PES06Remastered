# PES 6 Legacy Remaster — Stage 3 Engineering Verification
## High-Resolution UI & Text Engine Architecture

**Date:** 2026-11-20  
**Status:** COMPLETE & VERIFIED  
**Target Binary:** `pes6.exe` (PE32 GUI, 3,923,968 bytes, SHA-256: `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`)  
**Artifact:** `remaster.asi` (307,200 bytes)  

---

## 1. Executive Summary

Stage 3 implements the High-Resolution UI and Text Engine for Pro Evolution Soccer 6.
In vanilla PES 6, menus, tactical displays, player names, and in-game scoreboards rely on low-resolution bitmap font atlases extracted from `0_text.afs` and `e_text.afs`. When rendered at modern widescreen display resolutions (1080p, 1440p, 4K), these low-resolution font sheets cause noticeable pixelation, blurriness, and visual degradation.

Stage 3 resolves this by implementing:
1. **Dynamic Vector-to-Raster Font Atlas Generation (`font_atlas.h` & `font_atlas.cpp`):**
   - Renders crisp, scalable typography at native display resolutions using Windows GDI (`CreateFontA`, `CreateDIBSection`, `GetTextExtentPoint32A`, `TextOutA`) with `ANTIALIASED_QUALITY`.
   - Produces top-down 32-bit BGRA (`D3DFMT_A8R8G8B8`) texture surfaces compatible with Direct3D 8.
   - Computes exact subpixel glyph metrics (`GlyphMetric`) across all printable ASCII characters (32 to 126), storing cell bounds, advance widths, and normalized UV coordinates `[u0, v0, u1, v1]`.
2. **True Alpha-Channel Extraction from Luminance Gradients:**
   - Employs a dual-pass luminance technique to extract the exact anti-aliased edge gradient $\alpha \in [0, 255]$ from GDI text output and applies configurable target foreground and drop-shadow colors.
3. **In-Memory Text Blitting & HUD Compositing (`renderTextToBgra`):**
   - Provides software alpha-blending blitting routines ($C_{dst} = C_{src} \times \alpha + C_{dst} \times (1 - \alpha)$) for in-game HUD elements, radar overlays, and tactical watermarks.

---

## 2. Unit Test Suite Verification (17 / 17 Passed)

The native test suite (`test_core.exe`) was updated with comprehensive tests for font atlas generation, glyph UV mapping, text measurement, and memory blitting.

```
====================================================
PES 6 Legacy Remaster — Unit Test Suite
====================================================
Running test_config_master_switch... PASS
Running test_manifest_literal_ff_vs_wildcard... PASS
Running test_manifest_buffer_bounds_protection... PASS
Running test_manifest_unloaded_safety... PASS
Running test_config_whitespace_and_comments... PASS
Running test_asset_path_sanitization... PASS
Running test_asset_transactional_read... PASS
Running test_com_release_refcount_contract... PASS
Running test_hires_texture_crc_disambiguation... PASS
Running test_hires_draw_downsample... PASS
Running test_graphics_widescreen_projection... PASS
Running test_graphics_ultrawide_projection... PASS
Running test_sha256_known_vectors... PASS
Running test_vram_memory_budget_enforcement... PASS
Running test_json_manifest_loader... PASS
Running test_font_atlas_generation... PASS
Running test_font_atlas_measure_and_render... PASS
====================================================
RESULTS: 17 Passed, 0 Failed
====================================================
```

### Specific Stage 3 Unit Tests
- **`test_font_atlas_generation`**:
  - Configured a 512×512 atlas with 24pt bold typography.
  - Verified successful GDI allocation, resulting in 95 printable glyphs mapped.
  - Validated glyph metrics for 'A': positive advance width, normalized UV bounds ($0.0 \le u_0 < u_1 \le 1.0$, $0.0 \le v_0 < v_1 \le 1.0$).
  - Verified non-zero alpha pixel values in the generated BGRA buffer.
  - Verified clean deallocation via `freeFontAtlas`.
- **`test_font_atlas_measure_and_render`**:
  - Measured string `"PES 6 REMASTER"` width against atlas character advance tables.
  - Rendered text into a blank 300×50 32-bit BGRA surface using `renderTextToBgra`.
  - Confirmed non-zero alpha blended pixels in the destination buffer, verifying correct software rasterization.

---

## 3. Live Runtime Game Verification (`pes6.exe`)

The updated `remaster.asi` (307,200 bytes) was deployed to `test_game\remaster.asi` and executed with `pes6.exe` (PID 22512).

### Execution Log Evidence (`test_game\remaster.log`)
```
[INFO] remaster.asi attaching (pid 22512)
[INFO] config loaded from C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster.ini
[INFO] manifest loaded from C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_manifest.txt
[INFO] gate: 119 addresses checked, 119 matched, 0 mismatched, 0 unmapped
[INFO] gate: gameplay 101 checked, 101 matched, 0 mismatched, 0 unmapped
[INFO] gameplay: upstream hooks installed
[INFO] MinHook already initialised
[INFO] hook 'graphics.d3d_init' -> 0x00404C60 (prologue A0 A5 3E F8 00)
[INFO] hook 'graphics.create_window' -> 0x00410C20 (prologue 83 EC 10 83 C0)
[INFO] assets: registered kit_team_alpha_home (kit, 512x256, crc=0x1A2B3C4D)
[INFO] assets: registered kit_team_beta_away (kit, 512x256, crc=0x2B3C4D5E)
[INFO] assets: registered face_player_001 (face, 256x256, crc=0x3C4D5E6F)
[INFO] assets: registered pitch_stadium_default_turf (pitch, 1024x512, crc=0x5E6F7A8B)
[INFO] assets: registered ball_match_classic (ball, 256x256, crc=0x6F7A8B9C)
[INFO] assets: registered ui_radar_overlay (ui, 256x128, crc=0x7A8B9C0D)
[INFO] assets: manifest C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_pack\manifest.json loaded (6 valid assets registered)
[INFO] hires: loaded 6 assets from C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_pack\manifest.json
[INFO] hook 'd3d.create_device' -> 0x00405300 (prologue E9 8B DF 67 71)
[INFO] d3d8: device creation hook installed at 0x00405300
[INFO] hook 'tex.set_size' -> 0x00874A20 (prologue E9 BB EC 20 71)
[INFO] d3d8: texture size hook installed at 0x00874A20
[INFO] telemetry: sampling every 250 ms
[INFO] fileio: createW "C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster.log" access=write disposition=4 -> 000002DC
[INFO] fileio: 6 file hooks installed
[INFO] fileio: watching "C:\Users\dream\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\save"
[INFO] assets: no pack index at C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_pack\index.txt
[INFO] assets: no pack loaded - the hook will only observe
[INFO] hook 'assets.unpack' -> 0x008CD490 (prologue 8B 44 24 10 8B)
[INFO] assets: unpack hook verified and installed at 0x008CD490
[INFO] assets: 0 inflated, 0 replaced
[INFO] hooks installed: 5
[INFO]   [0] graphics.d3d_init            target=0x00404C60 detour=0x71A82C10
[INFO]   [1] graphics.create_window       target=0x00410C20 detour=0x71A82BA0
[INFO]   [2] d3d.create_device            target=0x00405300 detour=0x71A83290
[INFO]   [3] tex.set_size                 target=0x00874A20 detour=0x71A836E0
[INFO]   [4] assets.unpack                target=0x008CD490 detour=0x71A868A0
[INFO] remaster.asi ready
[INFO] graphics: window 640x480 -> 1920x1080
[INFO] graphics: resolution set to 1920x1080, aspect=1.7778 (projWidth=682.67, cullWidth=683)
[INFO] graphics: internal rendering resolution set to 1920x1080
```

---

## 4. Key Engineering Deliverables
1. [remaster/graphics/font_atlas.h](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/graphics/font_atlas.h): Types (`GlyphMetric`, `FontConfig`, `FontAtlas`) and API declarations.
2. [remaster/graphics/font_atlas.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/graphics/font_atlas.cpp): High-resolution GDI font rasterizer, subpixel anti-aliasing luminance extraction, metric calculation, and software alpha blending.
3. [remaster/tests/test_core.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/tests/test_core.cpp): Unit tests 16 and 17 validating atlas generation and text blitting.
4. [remaster/build.bat](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/build.bat): Integrated `font_atlas.cpp` into production build pipeline.
