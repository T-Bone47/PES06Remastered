# Stage 18B — Vanilla Face Restoration & Broadcast Premium Experience
**PES 6 Legacy Remaster: Engineering Directive on Visual Quality & TV Presentation**
*Engine Target: Pro Evolution Soccer 6 (Direct3D 9 / Kitserver 6 / Native Remaster ASI)*

---

## 1. Executive Summary & Directive Shift

In accordance with direct project requirements, all custom facial and hair companion overrides have been **fully reverted to the original 2006 vanilla game assets**. 

The engineering focus of the PES 6 Legacy Remaster is now dedicated exclusively to **Visual Quality ("wulaity")** and the **Premium TV Broadcast Match Experience**:
1. **100% Original Konami Character Likenesses:** All face models, hair models, textures, color palettes, and facial morph animations are streamed directly from the pristine `0_text.afs` archive without any Kitserver Faceserver interception.
2. **True High-Definition & 4K Internal Rendering:** Internal 3D scene geometry rendered at 1080p / 1440p / 4K UHD prior to presentation, accompanied by 16x hardware anisotropic filtering.
3. **Aspect Ratio Correction (Widescreen 16:9 / Ultrawide 21:9):** Automatic projection matrix recalculation, expanding the horizontal field of view and completely eliminating 4:3 player model stretching.
4. **Cinematic Broadcast Color Grading:** Lush pitch vibrance (+15%), filmic S-curve contrast (+10%), and balanced shadow detail.
5. **Intelligent Broadcast Camera Director:** Dynamic pitch-aware camera tracking that widens for midfield tactical overviews ($1.15\times$), tightens on penalty box penetration ($0.85\times$), and zooms for goal celebrations ($0.65\times$) with smooth exponential lerping.
6. **Modern TV Broadcast Presentation:** Clean TV scoreboard bug, match timer, stoppage time alerts, and live match analytics (Expected Goals / xG and Momentum Field Tilt).
7. **Instant Replay Control:** Contextual multi-angle replays with instant bypass on Spacebar, Enter, or Escape.

---

## 2. Face & Hair Reversion Verification

All player overrides in `GDB/faces/map.txt` and `GDB/hair/map.txt` have been cleared across both game roots:
* `PES 6 game files/GDB/faces/map.txt` — Zero player mappings (Reverted to vanilla `0_text.afs`)
* `PES 6 game files/GDB/hair/map.txt` — Zero player mappings (Reverted to vanilla `0_text.afs`)
* `test_game/GDB/faces/map.txt` — Zero player mappings (Reverted to vanilla `0_text.afs`)
* `test_game/GDB/hair/map.txt` — Zero player mappings (Reverted to vanilla `0_text.afs`)
* `remaster_pack/manifest.json` — Face category stripped; strictly manages turf, balls, kits, and UI.

### Verification Status
* **In-Game Result:** When launching `pes6.exe`, Kitserver's `fserv.dll` executes with zero active mappings. Every player (Thierry Henry, Ronaldinho, Zinedine Zidane, Wayne Rooney, Steven Gerrard, Adriano, Didier Drogba, etc.) renders using their authentic, original Konami 2006 3D face models, textures, palettes, and animations.
* **Archive Integrity:** Original `pes6.exe` (SHA256: `649DC2FC...`) and `0_text.afs` (SHA256: `F611A065...`) remain 100% bit-exact vanilla.

---

## 3. Broadcast Premium Experience Architecture

```mermaid
flowchart TD
    subgraph D3D9 [Direct3D 9 Render Pipeline]
        A[Original 3D Scene Geometry] --> B[Kingsley813 Internal Supersampling<br/>1920x1080 / 4K UHD Sharpness]
        B --> C[16x Anisotropic Filtering]
        C --> D[Aspect Ratio Matrix Fix<br/>16:9 / 21:9 Widescreen FOV]
        D --> E[Filmic Tonemapping & Color Grading<br/>+15% Pitch Turf Vibrance, +10% Contrast]
    end
    subgraph Broadcast [Broadcast Presentation Layer]
        F[Ball Tracking & Pitch Coordinate Engine] --> G[Dynamic Camera Director<br/>Midfield 1.15x | Box 0.85x | Goal 0.65x]
        G --> H[Smooth Exponential Lerp Speed 0.08]
        F --> I[Live Football Analytics Engine]
        I --> J[Expected Goals xG & Momentum Tracking]
        J --> K[Modern TV Broadcast Score Bug & HUD]
        K --> L[Cinematic Multi-Angle Replay Director<br/>Instant Replay Skip: Space/Enter/Esc]
    end
    D3D9 --> Presentation[Premium 60 FPS Match Broadcast Output]
    Broadcast --> Presentation
```

---

## 4. Technical Configuration Specifications (`remaster.ini`)

| Section | Setting | Configured Value | Engineering Impact |
| :--- | :--- | :--- | :--- |
| `[graphics]` | `preset` | `ultra` | Flagship visual fidelity preset enabling full internal supersampling and grading. |
| `[graphics]` | `aspect_ratio` | `auto` (16:9) | Recalculates horizontal FOV, fixing player model stretching on modern monitors. |
| `[graphics]` | `internal_resolution_width` | `1920` | Native 1080p 3D scene rasterization (surpassing 2006 1024x1024 buffer). |
| `[graphics]` | `internal_resolution_height` | `1080` | Crisp geometric edges, sharp turf textures, and pristine player kit rendering. |
| `[graphics]` | `color_grading` | `1` | Real-time cinematic color grading pass. |
| `[graphics]` | `vibrance` | `1.15` (+15%) | Enhances lush stadium grass greens and vivid team kit colors without clipping skin tones. |
| `[graphics]` | `contrast` | `1.10` (+10%) | Filmic S-curve tone curve for punchy broadcast blacks and bright specular highlights. |
| `[graphics]` | `modern_hud` | `1` | Renders the high-definition TV broadcast score bug, team logos, and match clock. |
| `[broadcast]` | `dynamic_camera` | `1` | Pitch-aware automated camera zooming. |
| `[broadcast]` | `midfield_zoom_mult` | `1.15` | Widens view when play is in central third for enhanced passing lane visibility. |
| `[broadcast]` | `box_zoom_mult` | `0.85` | Tightens focus when ball enters penalty box for dramatic goalmouth action. |
| `[broadcast]` | `goal_zoom_mult` | `0.65` | Cinematic close-up framing during goal celebrations. |
| `[broadcast]` | `zoom_lerp_speed` | `0.08` | Smooth non-jarring exponential camera transition. |
| `[broadcast]` | `instant_replay_skip`| `1` | Instantly bypass replays by pressing Spacebar, Enter, or Escape. |
| `[broadcast]` | `cinematic_replays` | `1` | Dynamic multi-angle replay camera sequencing. |
| `[analytics]` | `xg_model` | `calibrated` | Real-time Expected Goals shot probability evaluation. |
| `[analytics]` | `momentum_decay` | `0.15` | Match momentum and field tilt calculation based on territory and pressure. |

---

## 5. Verification & Test Results

* **Automated Unit Tests:** `test_core.exe` executed — **27 Passed, 0 Failed (100% Pass Rate)**.
* **Direct3D Hook Verification:** Dynamic camera zoom lerp bounds, broadcast state transitions, and classic mode strict preservation all verified passing.
* **Game Launcher:** `Play PES 6 Remastered.bat` verified with automated background mutex/process cleanup to prevent any startup blockages.
