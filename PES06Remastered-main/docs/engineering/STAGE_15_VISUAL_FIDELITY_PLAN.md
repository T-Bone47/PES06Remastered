# STAGE 15 — VISUAL FIDELITY MASTER PLAN & ASSET ENHANCEMENT ROADMAP

**Author:** Technical Artist & Principal Graphics Architect  
**Date:** 2026-11-20  
**Target:** Pro Evolution Soccer 6 (PC Windows 32-bit `pes6.exe`)  
**Deliverable File:** `docs/engineering/STAGE_15_VISUAL_FIDELITY_PLAN.md`  
**Classification:** **ENGINEERING EXECUTION ROADMAP**

---

## 1. STRATEGIC OBJECTIVE

Stage 15 executes the definitive visual transformation for PES 6 Legacy Remaster. Informed by the Stage 14 independent audit and real-game visual comparison, Stage 15 focuses exclusively on **tangible, measurable improvements to in-game visuals** rather than abstract codebase expansions.

Our execution mandate:
- **Maximum Impact:** Target the surfaces that occupy $> 75\%$ of the player's screen time during match play (pitch turf, player kits, faces, and broadcast presentation).
- **Technical Rigor:** Use verified tools (DirectXTex, AMD Compressonator, Real-ESRGAN, DxWrapper) in their optimal roles.
- **Strict Compatibility:** Zero regressions in gameplay responsiveness, 60 FPS frametime stability, and zero disk alterations to `pes6.exe`.

---

## 2. THE THREE HIGHEST-IMPACT VISUAL INITIATIVES

```
+-----------------------------------------------------------------------------------------------------------------------+
|                                           STAGE 15 CORE VISUAL INITIATIVES                                            |
+---+-----------------------------------+-----------------------------------------+-------------------------------------+
| # | Initiative                        | Target Deliverables                     | Tooling & Pipeline                  |
+---+-----------------------------------+-----------------------------------------+-------------------------------------+
| 1 | Ultra-HD Pitch & Grass System     | 2048x2048 procedural grass textures     | DirectXTex (texconv) + 16x AF       |
| 2 | Photorealistic Face & Kit Pipeline| 1024px uncompressed 32-bit player art   | Real-ESRGAN + Compressonator (SSIM) |
| 3 | Translation Wrapper Post-Processing| D3D8-to-D3D9 bridge, 4x MSAA, Windowed LUT| DxWrapper (chained via dsound.dll)  |
+---+-----------------------------------+-----------------------------------------+-------------------------------------+
```

---

### Initiative 1: Ultra-HD Pitch & Turf Restoration

#### Problem Statement
In vanilla PES 6, pitch turf is rendered using low-resolution ($256 \times 256$ or $512 \times 512$) repeating textures. Beyond the penalty area, default bilinear filtering blends the grass into a flat, monochromatic green blur.

#### Implementation Architecture
1. **Curated Procedural Grass Master (`remaster_pack/pitch/`):**
   - Author a high-definition $2048 \times 2048$ seamless turf surface featuring authentic grass blade tufts, subtle tonal variation, and contact shadows.
   - Pre-bake micro-contrast highlights into the diffuse texture to simulate directional sunlight response without requiring dynamic geometry shaders.
2. **Offline Processing via DirectXTex (`texconv.exe`):**
   - Generate full mipmap chains with Kaiser filtering to eliminate high-frequency scintillation at shallow broadcast camera angles:
     ```cmd
     texconv.exe -w 2048 -h 2048 -m 11 -f B8G8R8A8_UNORM -if KAISER -y -o remaster_pack\pitch turf_master.png
     ```
3. **Continuous 16x Anisotropic Filtering Enforcement:**
   - Leveraging our Stage 14 vtable slot 59 hook in `d3d8.cpp`, the Direct3D 8 runtime permanently samples the turf using 16x hardware anisotropic filtering, keeping the turf razor-sharp from the center circle all the way to the touchlines.

---

### Initiative 2: High-Resolution Player Faces & Team Kits

#### Problem Statement
Vanilla face textures ($128 \times 128$) and kit textures ($512 \times 256$) suffer from severe pixelation, posterization, and blocky compression artifacts when zoomed in during broadcast transitions and goal celebrations.

#### Implementation Architecture
1. **Deep-Learning AI Super-Resolution (Real-ESRGAN):**
   - Process original legacy face and kit textures through `RealESRGAN_x4plus` to reconstruct fine facial features (eyes, stubble, skin pores) and fabric textures without introducing cartoon blur.
2. **Automated Quality Assurance via AMD Compressonator:**
   - Compute SSIM (Structural Similarity Index) against original textures to guarantee that player likenesses and official team colors are preserved with $> 0.95$ fidelity:
     ```cmd
     compressonatorcli.exe -log ssim_report.txt -analysis original_kit.png upscaled_kit.png
     ```
3. **Two-Pass CRC32 Ingestion:**
   - Register upscaled assets in `remaster_pack/manifest.json`. The engine detects the texture upload at runtime, matches its content CRC32, and replaces the low-res surface via `hires.cpp`.

---

### Initiative 3: Translation Wrapper & Post-Processing (DxWrapper)

#### Problem Statement
Under modern Windows 10 and Windows 11 DWM (Desktop Window Manager), native Direct3D 8 applications cannot reliably set windowed hardware gamma ramps or enable modern edge anti-aliasing (MSAA/SMAA).

#### Implementation Architecture
1. **Clean Loader Chaining (No Conflicts with Kitserver):**
   - Kitserver uses `dinput8.dll`.
   - DxWrapper will be chained via `dsound.dll` or `dxgi.dll` in `test_game\`.
   - DxWrapper translates Direct3D 8 API calls to Direct3D 9Ex / Direct3D 11.
2. **Unlocking Advanced Post-Processing:**
   - **True 4x MSAA:** Hardware multisampling eliminates geometric edge staircasing on goalposts and player silhouettes.
   - **Windowed Color Grading Parity:** Translating to D3D9 allows our S-curve tonemapping to apply in windowed and borderless fullscreen modes without DWM desktop interference.
   - **ReShade Compatibility:** Enables optional subtle Depth-of-Field (DoF) on goal celebrations and cinematic replays.

---

## 4. ISOLATED TESTING & ROLLBACK PROTOCOL

Every Stage 15 enhancement is isolated and fully reversible:

```
+--------------------------------------------------------------------------------------------------------------------+
|                                           SAFETY & ROLLBACK PROTOCOL MATRIX                                        |
+--------------------------+-------------------------------------+---------------------------------------------------+
| Component                | Verification Command / Gate         | Rollback Procedure                                |
+--------------------------+-------------------------------------+---------------------------------------------------+
| HD Turf Texture Pack     | test_hires_draw_downsample          | Delete remaster_pack\pitch\ (engine falls back)   |
| 16x AF Enforcement Hook  | test_core.exe (test 26 & 27)        | Set [graphics] preset = classic in remaster.ini  |
| DxWrapper Translation    | test_launch.py --check-dx9          | Remove dsound.dll / dxwrapper.dll                 |
| Face & Kit Pack          | test_hires_texture_crc_disambig     | Remove entries from manifest.json                 |
+--------------------------+-------------------------------------+---------------------------------------------------+
```

---

## 5. STAGE 15 SUCCESS CRITERIA & TIMELINE

Stage 15 visual development concludes when:
1. **Side-by-Side Verification:** Unaltered in-game match captures demonstrate visible grass blade definition and crisp player kits at 1080p/1440p.
2. **Zero Gameplay Regression:** Ball physics, player momentum, and 60 FPS frametime pacing remain 100% stable throughout a full 90-minute match.
3. **Classic Mode Purity:** Setting `preset = classic` continues to reproduce the exact 2006 engine presentation.
