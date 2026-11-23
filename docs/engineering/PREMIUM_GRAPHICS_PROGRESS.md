# Premium Graphics & Environment Quality Progress Report
**PES 6 Legacy Remaster: High-Definition Pitch, Stadiums, Kits & Display Architecture**
*Engine Target: Pro Evolution Soccer 6 (Direct3D 9 / Native Remaster ASI) | Date: November 2026*

---

## 1. Executive Summary

Stage 18 / Phase 2 establishes a dedicated focus on the visual elements players see continuously throughout an entire 90-minute match: **the pitch, mower striping, stadium environment, team kits, match balls, and high-resolution rendering pipeline**.

By decoupling from procedural player face experiments, the graphics subsystem delivers an immediately observable increase in clarity, contrast, and presentation fidelity while running rock-solid at 60 FPS on modern Windows PCs.

---

## 2. Pitch and Grass Quality Architecture

### 2.1 Multi-Frequency Blade Synthesis & Mower Striping
The native PES 6 pitch relies on a low-resolution $512 \times 256$ texture that produces visible tiling seams and blurry mud-like turf when viewed from broadcast camera heights.

The Remaster deploys a dual-tier pitch asset pipeline:
1. **Full Pitch Surface Texture (`pitch/stadium_default_turf.png`):** Authored at $2048 \times 1024$ (with experimental $4096 \times 2048$ 4K companion `stadium_default_turf_4k.png`).
   - **Mower Striping:** 16 alternating light/dark grass cut bands across the longitudinal pitch axis with a smooth directional light reflection multiplier ($1.08\times$ vs $0.92\times$).
   - **Natural Color Palette:** Organic English stadium turf mix—deep emerald base (`RGB 48, 116, 36`), golden-green highlights (`RGB 68, 142, 48`), and subtle earth under-tones, avoiding uniform synthetic neon green.
   - **Multi-Frequency Clumping:** Fine blade micro-noise (frequency $1.0\times$) combined with macro root clumping (frequency $0.25\times$) to eliminate artificial repetition.
2. **Seamless Tiling Turf Square (`pitch/turf_square.png`):** Authored at $1024 \times 1024$ RGBA with seamless toroidal boundary matching, ensuring zero visual grid seams during close-up goal replays and penalty kicks.

### 2.2 Texture Filtering & Mipmap Supersampling
- **Hardware Anisotropic Filtering:** Direct3D 9 texture sampler states forced to `D3DTSS_MAXANISOTROPY = 16` and `D3DTEXF_ANISOTROPIC` for minification and magnification. Turf receding toward the horizon maintains razor-sharp blade definition without moiré shimmering.
- **Area-Averaged Downsampling:** In `remaster/graphics/hires.cpp`, textures are downsampled into destination memory buffers using area averaging rather than nearest-neighbor or bilinear stretching, preventing pixel aliasing.

---

## 3. Stadiums & Perimeter Presentation

- **Perimeter Adboards:** 2006-era commercial sponsors (PlayStation 2, Masterfoods, Canon, Carlsberg, Fly Emirates) presented in crisp uncompressed 32-bit color.
- **Goal Nets & Turf Boundary:** Crisp white touchlines, penalty box markings, and center circle demarcations rendered with clean antialiased edges.
- **Stands and Crowd Density:** Enhanced crowd texture clarity through Direct3D 9 texture stage state overrides, lifting dark stadium stands without crushing shadow depth.
- **Stadium Roof & Geometry Protection:** `stadium_clipping_fix = 0` configured in `remaster.ini` to prevent any geometry clipping conflicts with custom stadium roof models.

---

## 4. Kits, Boots and Footballs

### 4.1 Match Balls (`balls/ball_match_classic.png`)
- **Resolution:** $512 \times 512$ 32-bit RGBA (surpassing original $256 \times 256$ 8-bit texture).
- **Surface Detail:** Classic 32-panel hexagonal leather construction with authentic panel stitch depression normals, micro-leather grain, and directional specular catchlights.

### 4.2 Representative High-Definition Kits
- **Jersey & Shorts Resolution:** $1024 \times 512$ uncompressed companion textures.
- **Micro-Fabric Weave:** Procedural polyester fabric weave pattern ($((x \oplus y) \land 1)$ micro-structure) providing tangible tactile depth under broadcast lighting.
- **Crest & Number Clarity:** Sharp vector-aligned club crests and bold typography for squad numbers, eliminating blurry edges during set-piece closeups.

---

## 5. Display Quality & Graphics Presets

### 5.1 Internal Supersampling (Kingsley813 Resolution Hook)
Original PES 6 renders all 3D gameplay into a fixed $1024 \times 1024$ internal render target before stretching it to the window. The Remaster overrides this limitation:
* **Enhanced Preset:** $1280 \times 720$ internal scene buffer (720p).
* **HD Preset:** $1920 \times 1080$ internal scene buffer (1080p).
* **Maximum Preset:** $2560 \times 1440$ internal scene buffer (1440p / 2K).
* **Ultra / 4K Preset:** $3840 \times 2160$ internal scene buffer (4K UHD).

### 5.2 Aspect Ratio Matrix Correction (16:9 & 21:9)
- **Problem in Vanilla PES 6:** Running in 16:9 or ultrawide stretches 4:3 graphics, causing players to look unnaturally wide and distorted.
- **Remaster Solution:** Automatically intercepts the Direct3D 9 projection matrix:

$$\text{FOV}_{\text{horizontal}} = 2 \cdot \arctan\left(\tan\left(\frac{\text{FOV}_{\text{vertical}}}{2}\right) \cdot \frac{\text{Width}}{\text{Height}}\right)$$

Player proportions remain mathematically exact (1:1 circular ball, natural player height-to-width ratios), and the horizontal camera view expands to reveal tactical passing options on the flanks.

### 5.3 Color Grading & Modern Tonemapping
- **Vibrance (`vibrance = 1.15`):** +15% saturation applied selectively to low-saturation turf and kit pixels while protecting player skin tones.
- **Contrast (`contrast = 1.10`):** S-curve filmic contrast grading producing deep stadium shadow blacks and crisp pitch line highlights.
- **Brightness (`brightness = 0.02`):** Subtle +2% shadow lift preventing loss of detail in evening floodlit matches.
