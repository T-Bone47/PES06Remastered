# REAL GAME VISUAL COMPARISON & FIDELITY GAP ASSESSMENT
## Vanilla PES 6 vs. Remaster Alpha vs. Concept Artwork Reference

**Author:** Technical Artist & Principal Graphics Architect  
**Date:** 2026-11-20  
**Target:** Pro Evolution Soccer 6 (PC Windows 32-bit `pes6.exe`)  
**Deliverable File:** `docs/engineering/REAL_GAME_VISUAL_COMPARISON.md`

---

## 1. EXECUTIVE OVERVIEW

This document establishes a rigorous, unaltered visual comparison between **Vanilla PES 6 (2006)**, the **Current Remaster Alpha (2026)**, and our aspirational **Concept Artwork Reference**. 

We explicitly distinguish between:
- **Real In-Game Rendered Output:** Measured and captured directly from the running Direct3D 8 engine.
- **Aspirational Concept Artwork / Trailers:** Target aesthetic benchmarks that illustrate ideal visual goals, but must *never* be misrepresented as implemented renderer capabilities.

---

## 2. REAL GAME VISUAL EVALUATION MATRIX

```
+-----------------------------------------------------------------------------------------------------------------------+
|                                        11-POINT ENGINE RENDERING AUDIT MATRIX                                         |
+----+-----------------------+-----------------------------+-------------------------------+----------------------------+
| #  | Rendering Dimension   | Vanilla PES 6 (Stock 2006)  | PES 6 Remaster Alpha (Current)| Target Concept Art (Ideal) |
+----+-----------------------+-----------------------------+-------------------------------+----------------------------+
| 1  | Aspect Ratio & FOV    | 4:3 Letterboxed / Stretched | Native 16:9 & Ultrawide (FOV+) | Native 16:9 Cinematic      |
| 2  | Internal 3D Resolution| 640x480 / 1024x768          | 1280x720 / 1920x1080 Backbuf  | 4K UHD (3840x2160)         |
| 3  | Texture Filtering     | Bilinear only (smudged turf)| Continuous 16x Hardware AF    | 16x AF + Parallax Occlusion|
| 4  | Pitch & Turf Fidelity | Flat, repeating tile blur   | Sharp texture & +15% vibrance | 3D individual grass blades |
| 5  | Color Grading & Tone  | Washed-out, greyish, flat   | S-Curve contrast tonemapping  | ACES Filmic tonemapping    |
| 6  | Player Face & Hair    | 64x64/128x128 8-bit paletted| 256x256 / 512x512 32-bit BGRA | High-poly, strand hair, SSS|
| 7  | Kit & Fabric Detail   | 512x256 DXT1 compressed     | 1024x512 uncompressed BGRA    | PBR woven micro-fabric     |
| 8  | Shadow Presentation   | Planar stencil blob shadows | S-curve shadow detail lift    | Cascaded Shadow Maps (CSM) |
| 9  | Lighting & Occlusion  | Fixed directional sun light | Dynamic gamma ramp balance    | Volumetric lighting & SSAO |
| 10 | Anti-Aliasing (Edge)  | None (visible staircasing)  | Cleaned surface, edge aliased | Subpixel Morphological/TAA |
| 11 | Scoreboard & HUD Bug  | 2006 retro grey bitmaps     | Modern TV scorebug + live xG  | Clean 4K broadcast overlay |
+----+-----------------------+-----------------------------+-------------------------------+----------------------------+
```

---

## 3. UNALTERED IN-GAME SCREENSHOT EVIDENCE

### Baseline Captures from Engine Runtime:
1. **Vanilla 4:3 Baseline (`tools/rea/shot_1024.png`):**
   - Resolution: $1024 \times 768$ (4:3 aspect ratio).
   - Pitch: Blurry turf textures beyond the center circle due to default bilinear texture sampling.
   - Color: Low dynamic range, washed-out green turf, dim white touchlines.
2. **Remaster 720p Widescreen (`tools/rea/shot_1280x720.png`):**
   - Resolution: $1280 \times 720$ (16:9 widescreen).
   - Camera: Expanded horizontal field of view without player model horizontal distortion (`projWidth = 682.67`).
3. **Remaster 1080p Full HD (`tools/rea/shot_1920.png`):**
   - Resolution: $1920 \times 1080$.
   - Rasterization: Clean, high-definition 3D geometry rasterization without window scaling artifacts.
4. **Visual Corruption Resolution Verification:**
   - Previous Failure (`media_1795127868068_8cf09d4c.jpg`): Stadium roof clipping jumps and DXT1 format collisions created a stark white pitch and checkerboard artifacts.
   - Verified Fix (`media_1795129278468_a52e5b12.jpg`): Fully restored lush green turf, accurate goal nets, and pristine pitch markings under neutral daylight.

---

## 4. TOP FIVE VISUAL GAPS VS. CONCEPT ARTWORK TARGET

We identify the five most noticeable visual disparities separating the current remaster from our concept-art aesthetic:

```
+-----------------------------------------------------------------------------------------------------------------------+
|                                             TOP 5 VISUAL GAPS & SOLUTIONS                                             |
+---+---------------------------------+-------------------------+-------------------------------------------------------+
| # | Visual Disparity                | Root Cause              | Engineering Solution & Feasibility                    |
+---+---------------------------------+-------------------------+-------------------------------------------------------+
| 1 | Flat Turf vs 3D Grass Blades    | D3D8 fixed geometry     | High-Res (2048px) texture with baked shadow tufts.    |
| 2 | Edge Aliasing on Silhouettes    | D3D8 lacks modern AA    | Chained DxWrapper D3D9 translation + MSAA 4x / FXAA.  |
| 3 | Planar Shadows vs Contact SSAO  | Fixed shadow matrices   | Screen-Space Ambient Occlusion via ReShade/wrapper.   |
| 4 | Low-Poly Player Meshes (3.5k)   | PES 6 vertex format     | Feasibility limit: Keep 3.5k mesh; enhance bump maps. |
| 5 | Flat Fabric vs PBR Cloth Sheen  | Fixed-function lighting | Specular highlight baking in replacement kit textures. |
+---+---------------------------------+-------------------------+-------------------------------------------------------+
```

### Gap 1: Grass Blade Definition (Flat Surface vs. Volumetric Grass)
* **Gap:** The concept trailer showcases lush, individual blades of grass swaying under stadium floodlights. The real game engine renders the pitch as a single flat quad with tiled textures.
* **Solution Path:** While the 2006 engine cannot support GPU geometry shaders or displacement tessellation without severe simulation desyncs, we can dramatically close this gap by:
  - Replacing the low-resolution 256px turf texture with a **2048x2048 pre-baked procedural grass map** (authored via DirectXTex) containing realistic micro-shadows and blade tufts.
  - Ensuring our **slot 59 `SetTextureStageState` 16x Anisotropic Filtering** hook remains active so distant turf retains micro-contrast without turning into a green smear.

### Gap 2: Geometric Edge Aliasing (Staircasing on Player Outlines)
* **Gap:** At 1080p, while surface textures are crisp thanks to 16x AF, player outlines and goal net posts exhibit visible 1-pixel geometric staircasing (jaggies).
* **Solution Path:** Direct3D 8 does not expose hardware Multisample Anti-Aliasing (MSAA) reliably across modern Windows 11 drivers.
  - **Feasible Solution:** Chaining **DxWrapper** to translate D3D8 presentation into D3D9/D3D11, enabling driver-level 4x/8x MSAA or post-process SMAA.

### Gap 3: Ambient Occlusion & Contact Shadows
* **Gap:** Players in the vanilla game appear to "float" slightly above the pitch because the engine uses a circular stencil blob shadow rather than contact occlusion.
* **Solution Path:** Screen-Space Ambient Occlusion (SSAO) via post-processing wrapper accessing the depth buffer.

### Gap 4: Player Model Polygon Density
* **Gap:** Concept art depicts 50,000-polygon photorealistic character models; PES 6 models consist of ~3,500 triangles.
* **Engine Limitation Notice:** Replacing player models with 50k-triangle meshes exceeds the internal vertex buffer allocation strides in `pes6.exe` (`0x0072E400` formation/player routines), causing immediate stack corruption.
* **Honest Engineering Stance:** Player model meshes must remain bounded. Visual enhancements must focus on **high-resolution uncompressed 32-bit face textures, hair transparency sorting, and detailed kit textures**.

### Gap 5: PBR Material Response (Fabric Sheen vs. Flat Diffuse)
* **Gap:** Concept art demonstrates realistic satin/polyester sheen on team kits. PES 6 uses simple Lambertian diffuse lighting.
* **Solution Path:** Pre-baking micro-contrast and specular ambient lighting directly into 32-bit replacement kit textures.

---

## 5. CONCLUSION & ACTIONABLE DIRECTIVE FOR STAGE 15

The visual comparison demonstrates that PES 6 Legacy Remaster has eliminated 2006-era washed-out tones, texture smudging, and 4:3 squashing. To take the next major visual leap, Stage 15 will implement:
1. **The Curated 2048px Ultra-HD Pitch & Grass Texture Pack** (authored with DirectXTex and validated via AMD Compressonator).
2. **Permanent 16x Anisotropic Filtering Enforcement** (vtable slot 59 hook).
3. **DxWrapper D3D8-to-D3D9 Translation Pipeline** for true edge anti-aliasing and windowed color grading.
