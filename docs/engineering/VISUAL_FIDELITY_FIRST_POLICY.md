# PES 6 Legacy Remaster — Visual Fidelity First Development Policy & Architectural Strategy
**Document Version: 1.0.0 | Status: Official Engineering Directive**
*Target Platform: Modern Windows Gaming PCs & Laptops (Direct3D 9 / Kitserver 6 Architecture)*

---

## 1. Executive Mission & Strategic Realignment

The **PES 6 Legacy Remaster** project officially establishes a **Visual Fidelity First** engineering policy. Pro Evolution Soccer 6 remains widely recognized as having the finest core football simulation mechanics and responsive match engine in video game history. Our mission is to combine that iconic, timeless gameplay experience with the maximum achievable visual fidelity that modern PC hardware can deliver.

We no longer constrain our graphics targets to 2006-era hardware budgets (512 MB system RAM, 128 MB VRAM). Instead, our flagship engineering target is **modern gaming PCs and high-performance laptops** equipped with multi-core processors, modern dedicated GPUs (NVIDIA GeForce RTX, AMD Radeon RX, Intel Arc), high-bandwidth video memory (8 GB to 24 GB VRAM), and 1440p / 4K Ultra-HD displays.

---

## 2. Core Engineering Principles of the Policy

### 2.1 Visual Fidelity First with Classic Simulation Preservation
* **Principle 1:** Target the highest achievable graphical fidelity, presentation quality, and atmospheric realism while preserving 100% of PES 6's original match simulation, ball physics, and player animations.
* **Principle 2:** Modern graphics enhancements and gameplay modes remain **strictly independent**. The user may select our flagship Ultra 4K graphics preset while simultaneously running in 100% bit-exact Classic Gameplay Mode (zero simulation hooks, vanilla physics).

### 2.2 Embracing Modern Asset Scales & Memory Envelopes
* **Principle 3:** We explicitly accept substantially larger asset pack footprints (gigabyte-scale texture packs), higher GPU memory consumption, and increased rendering workload when they produce verifiable, human-visible improvements in visual quality.
* **Principle 4:** Do not reject promising graphical enhancements solely because they increase disk footprint or require modern hardware.

### 2.3 4K Ultra-HD as the Flagship Engineering Target
* **Principle 5:** 4K UHD presentation ($3840 \times 2160$) and 1440p WQHD ($2560 \times 1440$) are designated primary rendering targets.
* **Principle 6:** True internal rendering resolution overrides are enforced via the Kingsley813 pointers (`0x011631F8` and `0x011631FC`), ensuring the legacy engine renders full 3D geometry at native 4K sharpness prior to presentation.

---

## 3. Resolving the 32-Bit Process Architecture vs. Modern GPU VRAM

A fundamental technical challenge in remastering a 2006 Win32 application is the dichotomy between modern GPU capacity and the legacy 32-bit process boundary:

```
+---------------------------------------------------------------------------------+
|                       MEMORY ARCHITECTURE SEPARATION                            |
+---------------------------------------------------------------------------------+
|   32-BIT USER-MODE VIRTUAL ADDRESS SPACE       |    MODERN GPU VIDEO MEMORY     |
|   (Legacy Engine Host: pes6.exe)               |    (Physical Hardware VRAM)    |
+------------------------------------------------+--------------------------------+
| - Total Address Space: 2 GiB (or 4 GiB w/ LAA) | - Dedicated Capacity: 8-24 GiB |
| - Used by: Game heap, EXE code, physics, audio | - Used by: Direct3D 9 textures,|
| - Risk: E_OUTOFMEMORY if textures stored here  |   render targets, framebuffers |
| - Allocation Strategy: D3DPOOL_DEFAULT avoids   | - Isolation: D3DPOOL_DEFAULT    |
|   allocating in 32-bit user-mode system RAM.   |   resides directly in VRAM.    |
+------------------------------------------------+--------------------------------+
```

### 3.1 Direct3D 9 Memory Pools: `D3DPOOL_DEFAULT` vs `D3DPOOL_MANAGED`
* In Direct3D 9, textures allocated under `D3DPOOL_MANAGED` maintain a mirrored backing copy inside the host process's system memory. Allocating dozens of uncompressed 4K textures in `D3DPOOL_MANAGED` would rapidly exhaust the 32-bit address space, causing crashes.
* Under our Visual Fidelity First architecture, high-definition companion textures and internal render targets utilize **`D3DPOOL_DEFAULT`** with hardware staging buffers. Textures stream directly from disk (WIC decoding via asynchronous thread) into dedicated GPU VRAM, bypassing the 32-bit CPU virtual address space.
* On a modern GPU with 8 GB to 16 GB of VRAM, allocating 500 MB to 1.5 GB of ultra-high-definition textures consumes less than 10% of available video memory.

### 3.2 Dynamic Texture Streaming & Mipmap Generation
* Direct3D 9 hardware mipmap generation (`D3DUSAGE_AUTOGENMIPMAP`) is enabled on all companion textures.
* Trilinear and 16x anisotropic texture filtering ensure that 4K and 8K surfaces render without aliasing, moiré patterns, or performance penalties at distance.

---

## 4. Multi-Tier Resolution & Asset Strategy

| Asset Type | Native Resolution | Stage 17 Standard | Stage 18 Ultra 4K Target | Visual Benefit Justification |
| :--- | :--- | :--- | :--- | :--- |
| **Player Faces** | $64 \times 128$ (8-bit) | $1024 \times 2048$ (2K) | **$2048 \times 4096$ (4K)** | $32\times$ supersampling. Captures epidermal micro-pores, corneal highlights, and individual stubble hairs during close-up replays. |
| **Player Hair** | $128 \times 64$ (8-bit) | $1024 \times 512$ (1K) | **$2048 \times 1024$ (2K)** | Resolves fine strand highlights, dreadlock braid textures, and sharp headband edges with zero alpha fringing. |
| **Pitch Turf** | $256 \times 256$ (8-bit) | $2048 \times 1024$ (2K) | **$4096 \times 2048$ (4K) & $8192 \times 4096$ (8K)** | Covers the $105\text{m} \times 68\text{m}$ pitch at $< 1.3\text{ cm/px}$. Completely eliminates repetitive tiling seams from broadcast camera. |
| **Player Kits** | $512 \times 256$ (8-bit) | $1024 \times 512$ (1K) | **$2048 \times 1024$ (2K)** | Sharp sponsor typography, club crest embroidery, and high-frequency fabric weave microtexture. |
| **Match Ball** | $256 \times 256$ (8-bit) | $512 \times 512$ (512p) | **$1024 \times 1024$ (1K)** | Authentic 32-panel truncated icosahedron leather geometry with indented panel seams. |
| **Broadcast HUD** | $256 \times 128$ (8-bit) | $512 \times 256$ (512p) | **$1024 \times 512$ (1K)** | High-density vector fonts, crisp score bug presentation, and real-time match telemetry. |

---

## 5. Flagship Graphics Presets Specification

The remastered engine now provides five distinct, rigorously validated quality presets:

```ini
[graphics]
enabled = 1
; Quality preset: "classic", "enhanced", "hd", "maximum", or "ultra" (default = ultra)
preset = ultra
width = 1280
height = 720
internal_resolution_width = 3840
internal_resolution_height = 2160
aspect_ratio = auto
color_grading = 1
contrast = 1.18
vibrance = 1.25
modern_hud = 1
```

1. **`ultra` / `4k` (Flagship Experience):**
   * Internal Rendering: $3840 \times 2160$ (4K UHD)
   * Anisotropic Filtering: 16x Hardware AF
   * Tonemapping & Postprocessing: Cinematic S-curve tonemapping with +18% contrast and +25% lush pitch vibrance
   * HUD Presentation: Vector Font Atlas & Modern Broadcast Score Bug with real-time xG
2. **`maximum`:** $2560 \times 1440$ (2K WQHD) internal rendering, 16x AF, S-curve tonemapping, Modern HUD.
3. **`hd`:** $1920 \times 1080$ (Full HD) internal rendering, 16x AF, vibrant color grading, Modern HUD.
4. **`enhanced`:** $1280 \times 720$ (720p HD) internal rendering, 8x AF, subtle contrast enhancement.
5. **`classic`:** 100% bit-exact original 2006 engine rendering (no internal scaling, no color grading, no HUD overlay).

---

## 6. Modern Postprocessing & Lighting Roadmap

To further elevate visual fidelity on modern hardware, the graphics pipeline will incorporate advanced postprocessing shaders via Direct3D interception:
1. **Filmic ACES Color Tonemapping:** Dynamic highlight preservation preventing stadium floodlight blowout.
2. **Depth-of-Field (DoF) on Replays:** Context-aware background bokeh blur during cinematic replays and close-up cutscenes, utilizing the engine's Z-buffer.
3. **Screen-Space Ambient Occlusion (SSAO Approximation):** Enhanced contact shadows beneath players' boots and under stadium structural overhangs.
4. **Subsurface Scattering Approximation for Skin:** Realistic light diffusion through epidermal layers, eliminating the plastic appearance of legacy character models.

---

## 7. Performance & Stability Verification Contract

Even with our focus on maximum visual fidelity, the engineering contract remains uncompromising:
* **VSync Locked 60.0 FPS:** Pacing must remain rock-solid with standard deviation $< 0.50\text{ ms}$.
* **Zero Engine Crashes:** Long-session Master League play, instant replay transitions, and multi-match endurance runs must execute with 100% stability.
* **Bit-Exact Preservation:** The physical archives `pes6.exe` and `0_text.afs` will never be modified on disk.
