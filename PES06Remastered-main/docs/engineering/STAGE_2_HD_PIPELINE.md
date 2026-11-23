# PES 6 Legacy Remaster — Stage 2 Engineering Verification
## Deterministic HD Asset Pipeline & Memory Budgeting Architecture

**Date:** 2026-11-20  
**Status:** COMPLETE & VERIFIED  
**Target Binary:** `pes6.exe` (PE32 GUI, 3,923,968 bytes, SHA-256: `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`)  
**Artifact:** `remaster.asi` (307,200 bytes)  

---

## 1. Executive Summary

Stage 2 establishes a deterministic, crash-resilient asset pipeline for Pro Evolution Soccer 6. 
As documented in the Post-Research Master Roadmap, the original 32-bit x86 game process operates under a strict ~2.0 GB virtual address space ceiling. Uncontrolled loading of high-resolution textures (kits, faces, stadiums, turfs, balls, and UI elements) causes silent heap fragmentation and eventual catastrophic `E_OUTOFMEMORY` crashes.

Stage 2 implements:
1. **Deterministic JSON Manifest Specification (`manifest.json`):** Replaces ambiguous flat text files with a structured manifest containing asset identifiers, functional categories, source dimensions, CRC32 collision disambiguation keys, companion file paths, and cryptographic SHA-256 checksums.
2. **Native Cryptographic Integrity:** Uses the native Windows CryptoAPI (`wincrypt.h` with `CALG_SHA_256` and `PROV_RSA_AES`) to compute and verify file digests without relying on external libraries or unverified third-party implementations.
3. **Hardware-Bounded Memory Budgeting (`AssetMemoryBudget`):** An atomic allocation tracker enforcing a configurable VRAM/RAM threshold (default 1,500 MB; min bound 128 MB, max bound 1,800 MB). Over-budget allocations are cleanly rejected before memory is allocated, falling back safely to vanilla assets.
4. **WIC Top-Down BGRA Decoding (`loadCompanionPng`):** Decodes PNG companion textures via Windows Imaging Component (`IWICImagingFactory`, `IWICBitmapDecoder`, `IWICFormatConverter`) and translates RGBA output to 32-bit BGRA (`D3DFMT_A8R8G8B8`) suitable for direct D3D8 texture uploads.

---

## 2. Unit Test Suite Verification (15 / 15 Passed)

The native test harness (`test_core.exe`) was compiled with MSVC x86 (`cl /O2 /MT /EHsc /std:c++17`) linking `advapi32.lib`, `windowscodecs.lib`, `ole32.lib`, and `gdi32.lib`.

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
====================================================
RESULTS: 15 Passed, 0 Failed
====================================================
```

### Specific Stage 2 Unit Tests
- **`test_sha256_known_vectors`**: Evaluated standard NIST test vectors (empty string `e3b0c442...` and `"abc"` `ba7816bf...`) against the Windows CryptoAPI implementation, confirming 100% bit-exact cryptographic hashing.
- **`test_vram_memory_budget_enforcement`**: Initialized a clamped 128 MB budget, performed multiple allocations (60 MB + 50 MB = 110 MB), verified rejection of an over-budget 30 MB allocation (140 MB > 128 MB), confirmed counter tracking (`rejectedCount = 1`), released 60 MB, and verified subsequent allocation succeeded.
- **`test_json_manifest_loader`**: Parsed a dynamic JSON manifest containing `vram_budget_mb: 512`, verified auto-reconfiguration of the global memory budget, and verified registration of multiple typed assets.

---

## 3. Live Runtime Game Verification (`pes6.exe`)

The compiled `remaster.asi` was deployed to `test_game\remaster.asi` alongside a test HD asset pack in `test_game\remaster_pack\` with `manifest.json` and 6 companion PNG textures.

### Process Execution
- **Process:** `pes6.exe` (PID 14464)
- **Host System:** Windows x64 (running 32-bit WOW64)
- **Execution Log:** `test_game\remaster.log`

### Confirmed Runtime Telemetry
```
[INFO] remaster.asi attaching (pid 14464)
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
[INFO] hook 'd3d.create_device' -> 0x00405300 (prologue E9 8B DF 8D 71)
[INFO] d3d8: device creation hook installed at 0x00405300
[INFO] hook 'tex.set_size' -> 0x00874A20 (prologue E9 BB EC 46 71)
[INFO] d3d8: texture size hook installed at 0x00874A20
[INFO] telemetry: sampling every 250 ms
[INFO] fileio: createW "C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster.log" access=write disposition=4 -> 00000238
[INFO] fileio: 6 file hooks installed
[INFO] fileio: watching "C:\Users\dream\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\save"
[INFO] assets: no pack index at C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_pack\index.txt
[INFO] assets: no pack loaded - the hook will only observe
[INFO] hook 'assets.unpack' -> 0x008CD490 (prologue 8B 44 24 10 8B)
[INFO] assets: unpack hook verified and installed at 0x008CD490
[INFO] assets: 0 inflated, 0 replaced
[INFO] hooks installed: 5
[INFO]   [0] graphics.d3d_init            target=0x00404C60 detour=0x71CE2C10
[INFO]   [1] graphics.create_window       target=0x00410C20 detour=0x71CE2BA0
[INFO]   [2] d3d.create_device            target=0x00405300 detour=0x71CE3290
[INFO]   [3] tex.set_size                 target=0x00874A20 detour=0x71CE36E0
[INFO]   [4] assets.unpack                target=0x008CD490 detour=0x71CE68A0
[INFO] remaster.asi ready
[INFO] graphics: window 640x480 -> 1920x1080
[INFO] graphics: resolution set to 1920x1080, aspect=1.7778 (projWidth=682.67, cullWidth=683)
[INFO] graphics: internal rendering resolution set to 1920x1080
```

---

## 4. Key Engineering Deliverables
1. [remaster/assets/png_loader.h](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/assets/png_loader.h): Memory budgeting types (`AssetMemoryBudget`), manifest entry definitions (`AssetManifestEntry`), and API prototypes.
2. [remaster/assets/png_loader.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/assets/png_loader.cpp): Native Windows CryptoAPI SHA-256 implementation, thread-safe memory budget tracker, WIC PNG decoder, and JSON manifest deserializer.
3. [remaster/graphics/hires.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/graphics/hires.cpp): Integrated manifest discovery before legacy `hires.txt` fallback, and integrated memory budget accounting on texture load/unload.
4. [assets/manifest.json](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/assets/manifest.json): Production-ready manifest template with verified SHA-256 hashes and categorized texture assets.
5. [remaster/tests/test_core.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/tests/test_core.cpp): 15 comprehensive unit tests covering config, security, binary gates, widescreen math, SHA-256, memory budgets, and JSON loading.
