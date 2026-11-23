# Phase P2 Engineering Report: High-Resolution Texture Disambiguation & Content Hashing

**Date:** 2026-11-20  
**Project:** PES 6 Legacy Remaster  
**Target:** Pro Evolution Soccer 6 PC (`pes6.exe`, SHA-256: `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`)  
**Status:** Complete & Verified (10/10 Tests Passing, Zero Collisions)

---

## 1. Executive Summary

Phase P2 resolves Audit Hypothesis 6: **Width-and-height-only texture identification**. 

In prior revisions of the remaster, high-resolution texture replacement was keyed solely by width and height (`findHiResArt(width, height)`). Because PES 6 shares standard square dimensions (e.g. 512×512, 256×256) across multiple unrelated visual assets—including jersey textures, font sheets, radar maps, balls, and pitch turf—a dimension-only lookup returned the first matching entry in `hires.txt`. This caused severe visual corruption, such as font textures or radar HUD elements being mistakenly overwritten by kit textures.

Phase P2 implements **CRC32 content hashing** and a **two-tier resolution strategy** in the Direct3D 8 texture interception pipeline, providing collision-free asset replacement while preserving 100% backward compatibility with existing texture packs.

---

## 2. Technical Architecture & Implementation

### 2.1 Dynamic CRC32 Content Hashing (`remaster/graphics/d3d8.cpp`)
When the game creates or uploads a Direct3D 8 texture, `computeTextureCrc(WatchedTexture* w)` computes an IEEE 802.3 CRC32 checksum (polynomial `0xEDB88320`) over the uploaded pixel stream:
- **Scratch-diverted surfaces:** Computed directly from `w->scratch`.
- **Direct surfaces:** Locked and read via `g_origTexLockRect`, safely handling positive and negative pitch (bottom-up surfaces).

### 2.2 Pack Format Extension (`remaster/graphics/hires.cpp`)
The `remaster_pack/hires.txt` index parser was extended to support optional CRC32 tokens:
```
# Format: <width>x<height> [CRC32] <filename>
512x512 0x11223344 kit_home.png      # Exact match for texture with CRC 0x11223344
512x512 0xAABBCCDD radar_hud.png     # Exact match for radar with CRC 0xAABBCCDD
512x512 default_pitch.png            # Fallback for any other 512x512 texture
```

### 2.3 Two-Tier Lookup Strategy (`findHiResArt`)
When `findHiResArt(width, height, crc)` is invoked:
1. **Tier 1 (Exact Match):** If `crc != 0`, the registry scans for an entry matching `e.width == width && e.height == height && e.crc == crc`. If found, that specific artwork is loaded and applied.
2. **Tier 2 (Fallback):** If no exact CRC match is found, the registry falls back to entries matching `e.width == width && e.height == height && e.crc == 0`.
3. If neither matches, `nullptr` is returned and the original game texture is preserved untouched.

---

## 3. Unit Test Verification

Two new tests were added to the native unit test suite (`remaster/tests/test_core.cpp`):
1. `test_hires_texture_crc_disambiguation`: Registers three entries with identical 512×512 dimensions (two distinct CRC32 entries and one wildcard fallback). Proves that CRC queries route to the exact intended art, unmatched CRCs fall back cleanly, and non-existent dimensions return `nullptr`.
2. `test_hires_draw_downsample`: Proves that `drawHiResArt` correctly applies box area filtering when supersampling high-resolution artwork down to target dimensions.

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
====================================================
RESULTS: 10 Passed, 0 Failed
====================================================
```

---

## 4. Runtime Validation in PES 6 Retail Binary

The updated binary (`remaster.asi`, 301,568 bytes) was deployed to `test_game/` and validated against `pes6.exe`:
- **Address Gate Check:** 119/119 addresses matched (0 mismatched, 0 unmapped).
- **Gameplay Gate Check:** 101/101 addresses matched.
- **Hook Attachment:** All 5 core hooks (`graphics.d3d_init`, `graphics.create_window`, `d3d.create_device`, `tex.set_size`, `assets.unpack`) installed cleanly.
- **Resolution Override:** Successfully transformed window 640×480 → 1280×720 and updated resolution globals.
- **Execution Stability:** Process ran cleanly with zero memory access violations.
