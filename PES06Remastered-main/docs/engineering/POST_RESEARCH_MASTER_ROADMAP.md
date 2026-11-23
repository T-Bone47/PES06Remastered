# PES 6 LEGACY REMASTER
# POST-RESEARCH MASTER ROADMAP & LONG-TERM SYSTEM ARCHITECTURE

**Date:** 2026-11-20  
**Status:** Living Engineering Directive & Technical Architecture  
**Target Executable:** `pes6.exe` (Retail PE32 GUI, 3,923,968 bytes)  
**Target SHA-256:** `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`  
**Primary References:**  
- [deep-research-report (4).md](file:///c:/Users/dream/Downloads/deep-research-report%20(4).md)
- [INITIAL_CODEBASE_AUDIT.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/INITIAL_CODEBASE_AUDIT.md)
- [GAME_BASELINE_REPORT.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/GAME_BASELINE_REPORT.md)
- [P0_ENGINEERING_REPORT.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P0_ENGINEERING_REPORT.md)
- [P1_RUNTIME_VALIDATION.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P1_RUNTIME_VALIDATION.md)
- [P2_HIRES_DISAMBIGUATION.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P2_HIRES_DISAMBIGUATION.md)
- [FUNCTION_REGISTER.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/reverse-engineering/FUNCTION_REGISTER.md)

---

## 1. EXECUTIVE SUMMARY & STRATEGIC REALIGNMENT

This engineering roadmap integrates the strategic findings of the PES 6 Deep Research Report ([deep-research-report (4).md](file:///c:/Users/dream/Downloads/deep-research-report%20(4).md)) with the empirical, evidence-backed milestones completed in the **PES06Remastered** repository (P0 Core Hardening, P1 Runtime Validation, and P2 High-Resolution Disambiguation).

### 1.1 The Core Strategic Insight
The deep research confirms that **attempting a full reverse-engineering or decompilation of the PES 6 engine as a prerequisite is an anti-pattern**. 
As demonstrated by the community's PS2 decompilation project (`pes6decomp`), after years of intensive effort, only 2.90% of code bytes have been matched, and that work is specific to the PS2 PAL MIPS architecture rather than the PC x86 target.

Instead, the optimal and technically sound strategy is:
1. **Preserve the original Windows PC executable (`pes6.exe`) as the authoritative simulation engine.**
2. **Build a hardened, fail-closed native x86 C++ runtime (`remaster.asi`) around it.**
3. **Focus reverse engineering strictly on high-leverage engine boundaries:** D3D8 device/viewport pipelines, 2D/3D projection transformation, the 60 Hz simulation tick dispatcher, and read-only player/ball entity state structures.
4. **Decouple high-overhead features (tactical analytics, modern UI, live heatmaps) into an external companion application (Sidecar)** via low-latency local IPC, preserving the game's fragile 32-bit virtual memory space.
5. **Strictly isolate gameplay innovations behind individual, bounded opt-in flags in Enhanced Mode**, while maintaining Classic Mode as an untouched, bit-exact vanilla simulation.

---

## 2. TRI-PARTITE AUDIT: RESEARCH vs. CODEBASE REALITY

To ground the roadmap in verified facts rather than theoretical modding concepts, every recommendation from the research has been audited against our current repository state, categorizing each into:
- **Category A:** Already Implemented & Verified in `PES06Remastered`
- **Category B:** Supported by Public Research (Ready for Staged Architecture)
- **Category C:** Requires Local Executable Verification (Ghidra/REA Reverse Engineering)

```
┌────────────────────────────────────────────────────────────────────────────────┐
│                      TRI-PARTITE AUDIT CLASSIFICATION                          │
├───────────────────────┬────────────────────────┬───────────────────────────────┤
│ CATEGORY A:           │ CATEGORY B:            │ CATEGORY C:                   │
│ ALREADY IMPLEMENTED   │ PUBLIC RESEARCH BACKED │ LOCAL EXE REA / GHIDRA NEEDED │
├───────────────────────┼────────────────────────┼───────────────────────────────┤
│ • Master enable gate  │ • Kitserver DXTOOLS    │ • PES 6 match tick dispatcher │
│ • 119/119 Hook verify │   internal resolution  │ • 22-player entity struct map │
│ • 101/101 Gameplay GT │ • 512x448 PS2 layer    │ • Ball 3D state & spin layout │
│ • D3D8 COM wrappers   │   projection repair    │ • D3D8 depth buffer format    │
│ • Window resize patch │ • PES6 HD Text atlases │ • Match renderer shader use   │
│ • CRC32 texture hash  │ • D3D8->D3D9/11 bridge │ • Replay controller hooks     │
│ • Safe file traversal │ • Companion PNG loader │ • Option file / ML save schema│
│ • COM refcount safety │ • Sidecar external IPC │ • Bounded gameplay variable   │
│ • 10/10 Unit tests    │ • XOR option file dec  │   offsets in retail 649dc...  │
└───────────────────────┴────────────────────────┴───────────────────────────────┘
```

### 2.1 Category A: Already Implemented in PES06Remastered
These features are compiled, unit-tested, and verified running in `test_game/pes6.exe`:
- **Fail-Closed Master Architecture:** `[core] enabled = 0` provides an inert no-op pass-through; `[core] enabled = 1` enforces dual validation gates before installing any hooks ([P0_ENGINEERING_REPORT.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P0_ENGINEERING_REPORT.md), [remaster.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/remaster.cpp)).
- **Manifest Hook Validation Gate:** 119/119 hook addresses (100.0%) verified against original byte signatures prior to attachment ([manifest.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/core/manifest.cpp)).
- **Gameplay Isolation Gate:** 101/101 gameplay hook addresses verified; completely detached in Inert/Classic mode to prevent physics corruption ([gameplay.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/gameplay/gameplay.cpp)).
- **Direct3D 8 COM Lifecycle Wrappers:** Complete hooking for `Direct3DCreate8`, `CreateDevice`, `Reset`, `Present`, and `SetTexture` with explicit reference counting tracking ([d3d8.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/graphics/d3d8.cpp)).
- **Window & Resolution Globals Override:** Verified patching of window dimensions and internal resolution globals at `0x00F83FD8` and `0x00F83FDC` ([P1_RUNTIME_VALIDATION.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P1_RUNTIME_VALIDATION.md)).
- **CRC32 High-Resolution Disambiguation:** Replaced fragile width/height texture replacement with dynamic CRC32 hashing computed across locked surface data, resolving hash collisions across identical dimensions ([P2_HIRES_DISAMBIGUATION.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P2_HIRES_DISAMBIGUATION.md), [hires.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/graphics/hires.cpp)).
- **Transactional & Path-Safe Asset I/O:** Protection against directory traversal (`..`) and atomic asset loading ([assets.cpp](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster/assets/assets.cpp)).
- **Automated Regression Test Suite:** Custom standalone test runner `test_core.exe` executing 10/10 passing tests across config, manifest, safety, and hashing subsystems.

### 2.2 Category B: Supported by Public Research (Architecturally Approved)
These capabilities are proven in third-party projects (Kitserver 6, PES6Patcher, PESMod, PES6 HD Text) and are approved for native adaptation into our codebase:
- **Internal Scene Rendering Resolution:** Decoupling internal scene rasterization (`internal.resolution.width/height`) from final backbuffer window size (proven by Kitserver's `DXTOOLS`), improving 3D edge sharpness rather than stretching low-res buffers.
- **Widescreen & Ultrawide 2D UI Projection Fix:** Correcting the PS2-derived 512×448 fixed aspect projection layer to prevent player "sliding" and stretching on 16:9, 21:9, and 32:9 displays (proven by PES6Patcher).
- **HD Font & Atlas Generation:** Direct3D 8 dynamic font texture creation replacing low-resolution bitmap fonts with sharp vector-rendered text (proven by PES6 HD Text).
- **Direct3D 8 to Direct3D 9/11 Translation Layers:** Leveraging DxWrapper or dgVoodoo2 as an external translation bridge to expose modern post-processing (SMAA, CAS, HDR tone mapping) without rewriting the game's rendering pipeline.
- **Modular Companion PNG Asset Pipeline:** Loading high-res kit, boot, and face companion `.png` files alongside `.bin` archives without packing them into proprietary AFS archives (proven by Kitserver GDB).
- **Option File Cryptography:** Decrypting `.opd` option files using the proven rolling-XOR algorithm (from Kitserver `filedec.cpp`) for career and squad data analysis.

### 2.3 Category C: Requires Local Executable & Runtime Verification
These hypotheses cannot be imported blindly from community patches due to variation across PES 6 executables (e.g., Skidrow, Reloaded, 1.0 NOCD, Russian/Polish localizations) and must be proven against our specific hash (`649dc2fc...`):
- **PES 6 Match Simulation Tick Dispatcher:** The exact RVA and calling convention of the high-frequency match update function executing 60 times/sec.
- **Entity Coordinate Memory Layouts:** Precise struct offsets for the 22 player instances (positions, orientations, velocities, stamina, cards) and the ball physics instance (x, y, z, spin).
- **D3D8 Depth Buffer Availability:** Verifying whether `pes6.exe` allocates an accessible Z-buffer (`D3DFMT_D24S8` or `D3DFMT_D16`) or disables depth clearing during passes, which dictates compatibility with screen-space ambient occlusion (SSAO) and depth-of-field.
- **Match Renderer Shader vs. Fixed-Function Pipeline:** Empirical tracing of `CreateVertexShader` and `CreatePixelShader` to determine if PES 6 utilizes early programmable shaders (VS/PS 1.1) or strictly fixed-function vertex pipelines.
- **Replay State Machine & Camera Hooks:** The exact event dispatcher triggering replay sequences on goals, fouls, and out-of-bounds.
- **Option File & Master League In-Memory Layout:** Struct offsets for club finances, player development curves, and transfer negotiation states.
- **Bounded Gameplay Variable RVAs:** Locating the exact static addresses for team width scalars, pressing activation distances, through-ball lead multipliers, and pass speeds for our target executable.

---

## 3. TECHNICAL EVALUATION OF COMMUNITY REFERENCES

We maintain a strict policy: **evaluate community tools as algorithmic references; never load conflicting DLL proxies or copy memory addresses blindly.**

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                          COMMUNITY TOOLING EVALUATION MATRIX                                │
├─────────────────┬──────────────────────┬──────────────────────────┬─────────────────────────┤
│ TOOL            │ PROVEN STRENGTHS     │ RISKS & LIMITATIONS      │ REMASTER STRATEGY       │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ Kitserver 6     │ • DXTOOLS internal   │ • Hardcoded v1.0 addrs   │ Native C++ re-implemen- │
│ (Juank/Robbie)  │   resolution logic   │ • Hijacks d3d9.dll       │ tation of DXTOOLS & GDB │
│                 │ • GDB companion PNGs │ • Allocator hook crashes │ rules; DO NOT load      │
│                 │ • AFS redirection    │   under modern OS        │ kload.dll concurrently. │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ PES6Patcher     │ • Fixes 512x448 PS2  │ • Direct disk patching   │ Reverse-engineer the 2D │
│ (PES Modders)   │   sliding projection │ • Lacks fail-closed gate │ projection scalar; patch│
│                 │ • Gameplay variables │ • Hardcoded offsets      │ in memory dynamically.  │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ PESMod          │ • Modern ASI model   │ • Focuses on leagues/UI  │ Interoperability testing│
│ (Modern Scene)  │ • MinHook structure  │ • Potential hook collide │ adopt clean ASI habits. │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ PES6 HD Text    │ • Sharp D3D8 text    │ • Font-only scope        │ Integrate atlas gener-  │
│                 │ • Byte validation    │                          │ ator into hires.cpp.    │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ DxWrapper       │ • D3D8 -> D3D9 bridge│ • Hook chaining latency  │ Recommended proxy for   │
│ (elishacloud)   │ • ReShade/SMAA ready │ • Double wrapper risk    │ post-FX; load cleanly.  │
├─────────────────┼──────────────────────┼──────────────────────────┼─────────────────────────┤
│ dgVoodoo2       │ • D3D8 -> D3D11/12   │ • Closed source          │ Optional external       │
│ (Dege)          │ • Superb scaling/AF  │ • Driver overhead        │ alternative for users.  │
└─────────────────┴──────────────────────┴──────────────────────────┴─────────────────────────┘
```

---

## 4. TWO-TIER SYSTEM ARCHITECTURE

To achieve high-fidelity rendering, deep tactical analytics, and modern presentation without triggering out-of-memory crashes in PES 6's 32-bit (2 GB) virtual address space, the project adopts a **Two-Tier Decoupled Architecture**.

```
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│                                PES 6 LEGACY REMASTER ARCHITECTURE                           │
├─────────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────────────────────┐   │
│   │                        PES6.EXE (32-BIT x86 PROCESS)                                │   │
│   │                     Authoritative Simulation & Match Loop                           │   │
│   │                                                                                     │   │
│   │    ┌───────────────────────────────────────────────────────────────────────────┐    │   │
│   │    │               REMASTER.ASI (NATIVE x86 IN-PROCESS RUNTIME)                │    │   │
│   │    │                                                                           │    │   │
│   │    │   ┌─────────────────────┐   ┌───────────────────┐   ┌─────────────────┐   │    │   │
│   │    │   │  CORE LIFECYCLE     │   │  MANIFEST GATE    │   │  GAMEPLAY GATE  │   │    │   │
│   │    │   │  Inert / Active     │   │  119/119 Verified │   │  101/101 Classic│   │    │   │
│   │    │   └──────────┬──────────┘   └─────────┬─────────┘   └────────┬────────┘   │    │   │
│   │    │              │                        │                      │            │    │   │
│   │    │   ┌──────────▼────────────────────────▼──────────────────────▼────────┐   │    │   │
│   │    │   │                     DIRECT3D 8 GRAPHICS CORE                      │   │    │   │
│   │    │   │  • Internal Resolution (DXTOOLS)   • 2D Projection Repair (16:9)  │   │    │   │
│   │    │   │  • CRC32 Texture Disambiguation    • HD Text / UI Atlas Generator │   │    │   │
│   │    │   │  • Companion PNG Streamer          • VRAM Budget Enforcer (<1.5GB)│   │    │   │
│   │    │   └───────────────────────────────────┬───────────────────────────────┘   │    │   │
│   │    │                                       │                                   │    │   │
│   │    │   ┌───────────────────────────────────▼───────────────────────────────┐   │    │   │
│   │    │   │                     MATCH STATE OBSERVER                          │   │    │   │
│   │    │   │  • 10 Hz Non-blocking Engine Tick Hook                            │   │    │   │
│   │    │   │  • Read-only 22-Player & Ball Coordinates Extraction              │   │    │   │
│   │    │   │  • Match Event Interceptor (Goals, Fouls, Cards, Replays)         │   │    │   │
│   │    │   └───────────────────────────────────┬───────────────────────────────┘   │    │   │
│   │    └───────────────────────────────────────┼───────────────────────────────────┘    │   │
│   └────────────────────────────────────────────┼────────────────────────────────────────┘   │
│                                                │                                             │
│                     LOCAL IPC (LOCK-FREE SHARED MEMORY / NAMED PIPES)                       │
│                     Channel: \\.\pipe\PES6Remaster_Telemetry (10-60 Hz)                     │
│                                                │                                             │
│   ┌────────────────────────────────────────────▼────────────────────────────────────────┐   │
│   │                EXTERNAL REMASTER COMPANION (64-BIT SIDECAR APP)                     │   │
│   │                    Rust / C# WPF / Modern Web (Tauri/Electron)                      │   │
│   │                                                                                     │   │
│   │   ┌──────────────────────────┐  ┌──────────────────────────┐  ┌─────────────────┐   │   │
│   │   │ LIVE TACTICAL INTEL      │  │ BROADCAST DIRECTOR       │  │ MASTER LEAGUE   │   │   │
│   │   │ • Real-time Radar & xG   │  │ • Replay Auto-Clipper    │  │ • Scouting Tool │   │   │
│   │   │ • Heatmaps & Passing Net │  │ • OBS Stream Overlay     │  │ • Form Tracker  │   │   │
│   │   │ • Tactical Compactness   │  │ • Multi-angle Director   │  │ • Transfer Sim  │   │   │
│   │   └──────────────────────────┘  └──────────────────────────┘  └─────────────────┘   │   │
│   │                                                                                     │   │
│   │   ┌─────────────────────────────────────────────────────────────────────────────┐   │   │
│   │   │                     ASSET & PROFILE MANAGEMENT                              │   │   │
│   │   │ • Manifest Hash Validation • Texture Pack Manager • INI Configuration GUI   │   │   │
│   │   └─────────────────────────────────────────────────────────────────────────────┘   │   │
│   └─────────────────────────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 4.1 Tier 1: In-Process Native Runtime (`remaster.asi`)
- **Language & Runtime:** C++20 compiled as a 32-bit x86 DLL using MSVC (`/O2 /arch:IA32 /EHsc /MD`).
- **Memory Footprint:** Under 15 MB heap; static allocations for buffers to eliminate runtime heap fragmentation.
- **Execution Constraints:** Zero blocking calls on the main engine thread. All file I/O and telemetry serialization run on dedicated low-priority worker threads.
- **Fail-Closed Policy:** Any address mismatch in `manifest.cpp` aborts hook installation instantly, reverting `pes6.exe` to 100% vanilla operation.

### 4.2 Tier 2: External Companion Application (Sidecar)
- **Architecture:** Standalone 64-bit process isolated from the game's memory space.
- **Safety:** Crashes or hangs in the companion cannot crash `pes6.exe`.
- **IPC Protocol:** Double-buffered memory-mapped file or Windows Named Pipe serializing binary telemetry frames:
  - Header: Frame Number, Timestamp, Match Clock, Match Status.
  - Entities: Ball `(x, y, z, vx, vy, vz)`, 22 Players `(id, team, x, y, vx, vy, stamina, card, state)`.
  - Events: Event ID (Pass, Shot, Tackle, Foul, Goal, Whistle).

---

## 5. FIVE PRIORITIZED FEATURE FOCUS AREAS

Based on research recommendations and our established baseline, development is structured around five strategic priorities:

### Priority 1: Verified Internal Resolution, Widescreen Support, HD UI & CRC32 Textures
- **Internal Rendering Resolution (DXTOOLS Integration):**
  - Implement dynamic render-target recreation allowing scene rasterization at 1080p, 1440p, or 4K regardless of desktop display mode.
  - Ensure backbuffer Present scaling maintains proper aspect ratio without stretching.
- **Widescreen & Ultrawide Projection Repair:**
  - Eliminate the PS2 512×448 logical coordinate bug where players appear to "slide" across the pitch when running on 16:9 / 21:9 aspect ratios.
  - Patch the camera horizontal projection scalar while locking HUD and radar elements to correct proportional anchors.
- **HD UI & Dynamic Font Generation:**
  - Port the PES6 HD Text atlas pipeline to rasterize crisp fonts dynamically into Direct3D 8 textures, replacing low-resolution PS2 bitmap fonts.
- **Accurate Texture Disambiguation:**
  - Expand the CRC32 hashing engine introduced in [P2_HIRES_DISAMBIGUATION.md](file:///c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/docs/engineering/P2_HIRES_DISAMBIGUATION.md) to pair surface CRC32 with AFS Slot and VRAM pointer data, guaranteeing 100% collision-free replacement.

### Priority 2: Complete HD Visual Restoration Pipeline
- **Structured Asset Packs & Curated Manifests:**
  - Implement a JSON-based asset pack manifest format specifying source CRC32, target file path, dimensions, format, and memory cost.
- **Companion Asset Loader (Kits, Faces, Hair, Pitch, Stadiums):**
  - Native file loader replacing Kitserver GDB: reads external `.png` files (e.g. `assets/kits/arsenal_home.png`, `assets/faces/henry.png`) and directly creates replacement Direct3D 8 textures without modifying original AFS archives.
- **VRAM & Virtual Address Budgeting:**
  - Maintain an active counter of allocated replacement VRAM. Clamp total texture memory usage to 1.5 GB to guarantee that `pes6.exe` never breaches the 2.0 GB 32-bit virtual limit.
- **Strict Anti-Artifacting Standards:**
  - Strict rejection of automated blind AI upscaling. All replacement textures must preserve original alpha transparency masks, kit stitch lines, and proper mipmap generation to prevent shimmering.

### Priority 3: Read-Only Match Telemetry & Live Tactical Intelligence
- **High-Frequency Engine Observer:**
  - Safe, read-only hooking of the PES 6 match update loop (running at 60 Hz). Sample telemetry at 10 Hz to minimize CPU overhead (< 0.5% core usage).
- **Comprehensive Match State Extraction:**
  - Extract pitch coordinates `(x, y)` for all 22 players, player velocities, stamina bars, ball 3D coordinates `(x, y, z)`, ball spin, match minute, stoppage time, and current tactical formation.
- **Live Tactical Intelligence in Companion App:**
  - Generate real-time tactical heatmaps, team compactness indicators (distance between defensive line and strikers), pressing intensity meters (PPDA - Passes Per Defensive Action), passing network diagrams, and Expected Goals (xG) calculations.

### Priority 4: Intelligent Broadcast Director & Match Presentation
- **Event-Aware Presentation Logic:**
  - Hook engine match event dispatchers to detect key match incidents: goals, shots on target, near misses, fouls, yellow/red cards, corner kicks, and substitutions.
- **Automated Replay & Camera Switching:**
  - Intelligent camera selection: switch to dynamic broadcast angles on goals or referee focus during heated arguments/cards.
  - Prevent repetitive default replay loops; prioritize dramatic angles.
- **Classic Fallback & Instant Skip:**
  - Instant bypass of custom broadcast logic upon any user button press; bit-exact vanilla replay behavior in Classic Mode.

### Priority 5: Individually Configurable Enhanced-Mode Gameplay (Preserving Classic Mode)
- **Strict Bifurcation Architecture:**
  - **Classic Mode:** 100% stock simulation; all gameplay hooks unhooked or bypassed; identical to vanilla PES 6.
  - **Enhanced Mode:** Strict opt-in feature flags in `remaster.ini`.
- **Targeted Bounded Interventions:**
  - *Tactical Team Width:* Adjust off-ball defensive block width and compactness to counter modern wide formations.
  - *Dynamic Pressing Threshold:* Enhance opponent AI pressing aggression when chasing a deficit in late game minutes.
  - *Pass Lead & Ball Dynamics:* Fine-tune through-ball lead trajectory and natural air drag without altering ball rebound elasticity or player tackle timing.
- **Safety Envelope:**
  - Every gameplay variable is clamped within strict safe bounds (±15% of vanilla values) to guarantee that animation blending and physics calculations never break.

---

## 6. REA & GHIDRA REVERSE-ENGINEERING TARGET REGISTER

To execute these priorities, specific reverse-engineering investigations are mapped to our target executable (`649dc2fc...`).

| Subsystem | Ghidra Investigation / Search Pattern | Expected Evidence | Feature Unlocked | Priority |
|---|---|---|---|---|
| **D3D8 Device & Shaders** | Disassemble `0x007Cxxxx` (Direct3D initialization). Search for calls to `IDirect3DDevice8::CreateVertexShader` and `CreatePixelShader`. | Call log, shader bytecode presence, or confirmation of pure fixed-function pipeline. | Custom post-processing, lighting experiments, wrapper requirements. | P1 |
| **2D/3D Projection** | Search for `float` constants `512.0f`, `448.0f`, and aspect ratio divisors (`1.333333f` / `1.777777f`) in projection setup functions. | Function RVA, assembly pattern, matrix buffer address. | Widescreen & ultrawide projection fix without player sliding. | P1 |
| **Match Tick Dispatcher** | Trace main loop in `0x006xxxx` to locate the function called unconditionally once per frame during active match play. | Function RVA, calling convention, frame delta argument. | 10 Hz read-only telemetry sampling hook. | P3 |
| **Player Entity Array** | Scan for global pointers pointing to 22 contiguous structures of size ~0x400 - 0x800 bytes containing coordinates and jersey numbers. | Base pointer RVA, struct size, offset table for `pos_x`, `pos_y`, `stamina`, `id`. | Real-time player tracking, tactical heatmaps, AI behavior analysis. | P3 |
| **Ball Physics Struct** | Trace coordinates read by camera controller during match play (identifiable by continuous variation). | Global pointer to ball instance, offsets for `x, y, z`, velocities `dx, dy, dz`. | Ball telemetry, 3D broadcast camera tracking, shot physics analysis. | P3 / P4 |
| **Match Event Dispatcher** | Search for functions modifying score, referee card triggers, or whistle audio playback. | Event handler RVAs, event enum values (0=Goal, 1=Foul, etc.). | Broadcast Director automated replays and stream overlays. | P4 |
| **Gameplay Tuning Table** | Cross-reference PES6Patcher variable offsets against local executable disassembly; pattern match surrounding byte sequences. | Verified RVAs for pass lead, pressing distance, team width in `649dc...`. | Bounded Enhanced-mode gameplay tuning. | P5 |

---

## 7. EXPLICIT "REJECT OR POSTPONE" NON-GOALS

To ensure engineering resources remain focused on achievable, high-value outcomes, the following initiatives are explicitly rejected or postponed:

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                      REJECTED OR POSTPONED INITIATIVES                          │
├───────────────────────────────┬────────────┬────────────────────────────────────┤
│ INITIATIVE                    │ STATUS     │ TECHNICAL JUSTIFICATION            │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ Full Engine Decompilation     │ REJECTED   │ Years of effort for <3% code. Real │
│                               │            │ gameplay engine is already stable. │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ Full PBR Lighting Rewrite     │ REJECTED   │ Incompatible with D3D8 fixed func; │
│                               │            │ destroys original aesthetic.       │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ New Skeletal Animation Engine │ REJECTED   │ Destroys responsive PES 6 input    │
│ & Motion Matching             │            │ timing and tactile ball feel.      │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ DLSS / Frame Generation       │ REJECTED   │ Heavy latency; unsupported in D3D8;│
│                               │            │ unnecessary for lightweight 60fps. │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ Blind Batch AI Upscaling      │ REJECTED   │ Corrupts alpha channels, creates   │
│                               │            │ seams, exhausts 32-bit memory.     │
├───────────────────────────────┼────────────┼────────────────────────────────────┤
│ Global AI Rewrite from Zero   │ POSTPONED  │ Massive risk; start with bounded   │
│                               │            │ parameter tuning and telemetry.    │
└───────────────────────────────┴────────────┴────────────────────────────────────┘
```

---

## 8. REALISTIC STAGED DEVELOPMENT ROADMAP

```
===================================================================================
STAGED IMPLEMENTATION ROADMAP
===================================================================================

[CURRENT: STAGE 0] ──────► [STAGE 1] ──────► [STAGE 2] ──────► [STAGE 3]
Core Hardening &           Internal Res &    HD Asset          HD Font & UI
CRC32 Disambiguation       Projection Fix    Pipeline          Text Engine
(COMPLETED)                (Next Target)

     │
     ▼
[STAGE 4] ───────────────► [STAGE 5] ──────► [STAGE 6]
Read-Only Telemetry        Broadcast         Enhanced Gameplay
& Sidecar App              Director          (Opt-In Sliders)
===================================================================================
```

### Stage 1: Internal Resolution, Widescreen Projection & Display Correctness
- **Scope:**
  - Implement internal rendering resolution scaling (`internal_resolution_width`, `internal_resolution_height`).
  - Implement dynamic patch for the 512×448 2D/3D projection matrix scalar to fix player sliding in 16:9 and 21:9.
  - Implement deterministic screenshot benchmarking tool for visual validation.
- **Stop/Go Gate:**
  - Flawless 16:9 / 21:9 rendering verified: zero player sliding across camera movements; 3D scene rasterized at full resolution; HUD elements unskewed.
- **Deliverables:**
  - `remaster/graphics/projection.cpp`, updated `remaster.ini`, `docs/engineering/STAGE_1_DISPLAY_VERIFICATION.md`.

### Stage 2: Deterministic Asset Identification & Curated HD Restoration Pipeline
- **Scope:**
  - Build companion PNG loader replacing Kitserver GDB for external kit, face, hair, and pitch textures.
  - Implement JSON asset manifest parser with SHA-256 verification and memory tracking.
  - Integrate strict 1.5 GB VRAM budget enforcement.
- **Stop/Go Gate:**
  - Full match played with 22 HD faces, 2 HD kits, and HD pitch grass. Zero memory leaks, zero texture collisions, memory usage stable under 1.5 GB.
- **Deliverables:**
  - `remaster/assets/png_loader.cpp`, `assets/manifest.json`, `docs/engineering/STAGE_2_HD_PIPELINE.md`.

### Stage 3: High-Resolution UI & Text Engine
- **Scope:**
  - Port PES6 HD Text atlas generation into `remaster/graphics/hires.cpp`.
  - Replace pixelated low-res bitmap fonts with clean HD vector-rendered fonts across menus, radar, and in-game scoreboard.
- **Stop/Go Gate:**
  - Menus and in-game text perfectly legible and sharp at 1080p, 1440p, and 4K; zero string clipping or character overlap.
- **Deliverables:**
  - `remaster/graphics/font_atlas.cpp`, `docs/engineering/STAGE_3_HD_TEXT.md`.

### Stage 4: Read-Only Match Telemetry & Live Tactical Intelligence Sidecar
- **Scope:**
  - Locate and hook the 60 Hz match tick dispatcher in `pes6.exe`.
  - Extract read-only coordinates for 22 players and ball; export over Windows Named Pipe at 10 Hz.
  - Develop initial standalone 64-bit Sidecar application receiving telemetry and rendering a live 2D radar, player heatmaps, and compactness metrics.
- **Stop/Go Gate:**
  - Full 90-minute match played with telemetry active: zero game-thread frame drops (< 0.5% CPU overhead); sidecar receives ordered, uncorrupted telemetry stream.
- **Deliverables:**
  - `remaster/telemetry/observer.cpp`, Sidecar companion application codebase, `docs/engineering/STAGE_4_TELEMETRY.md`.

### Stage 5: Intelligent Broadcast Director & Event Presentation
- **Scope:**
  - Hook match event triggers (goals, fouls, shots, cards).
  - Implement intelligent camera switching and automated replay selection logic.
  - Support instant user replay skip with zero freeze or audio hitching.
- **Stop/Go Gate:**
  - Contextual replay cameras switch cleanly on goals and fouls; instant skip returns to match immediately; bit-exact vanilla replay verified in Classic Mode.
- **Deliverables:**
  - `remaster/broadcast/director.cpp`, `docs/engineering/STAGE_5_BROADCAST_DIRECTOR.md`.

### Stage 6: Bounded Enhanced Gameplay Systems (with Strict Classic Preservation)
- **Scope:**
  - Locate verified RVAs for team width, pressing distance, and through-ball lead in `649dc...`.
  - Expose individually toggleable sliders in `remaster.ini` under `[gameplay_enhanced]`.
  - Clamp all parameters within safety limits (±15%).
- **Stop/Go Gate:**
  - 100 automated AI vs AI matches completed without physics anomalies or crashes. Classic Mode verified 100% bit-exact to vanilla execution.
- **Deliverables:**
  - `remaster/gameplay/enhanced.cpp`, comprehensive A/B match validation report `docs/engineering/STAGE_6_GAMEPLAY_VALIDATION.md`.

---

## 9. IMMEDIATE IMPLEMENTATION HANDOFF: STAGE 1 EXECUTION

With the strategic roadmap formally established, engineering execution immediately proceeds to **Stage 1 (Internal Resolution & Widescreen Projection)**:

1. **REA / Ghidra Analysis:**
   - Disassemble the projection setup function in `pes6.exe` (searching for `512.0f` and `448.0f` aspect ratios).
   - Locate the Direct3D viewport configuration and render-target creation RVAs.
2. **C++ Implementation:**
   - Expand `remaster/graphics/d3d8.cpp` to intercept backbuffer resizing and calculate aspect ratio scalars.
   - Implement dynamic memory patch to prevent 2D/3D projection entanglement.
3. **Validation:**
   - Execute test match at 1920×1080 and 2560×1440; verify player rendering stability and absence of sliding.
   - Record results and screenshots in `STAGE_1_DISPLAY_VERIFICATION.md`.
