# PES 6 Legacy Remaster — Stage 20 Art Polish & Specialist Agent Backlog
## Prioritized Visual Enhancements, Stadium Atmosphere & Advanced Agent Handoff

**Document ID:** `STAGE_20_ART_POLISH_BACKLOG`  
**Status:** `READY FOR PLANNING`  
**Target Milestone:** Stage 20 Art Polish & Match Presentation  
**Scope Boundary:** Easy-to-Medium Asset Enhancements + Specialist Agent Decompilation Backlog  

---

### Executive Summary

With the successful certification of Stage 19 (100% vanilla face restoration, 2048x1024 organic pitch turf, 1024x128 2006 UEFA adboards, 16x anisotropic filtering, anamorphic widescreen scaling, and certified 60 Hz gameplay), Stage 20 focuses on prioritized visual polish and sets up the technical architecture for deeper decompilation tasks delegated to specialist coding agents (Claude Opus and Codex).

---

### Section 1: Stage 20 High-Priority Art Polish Items

The following items are scheduled for production in Stage 20, adhering strictly to easy-to-medium asset pipeline complexity without destabilizing core engine code:

| Priority | Feature Item | Target Asset / Module | Engineering Complexity | Expected Visual Impact |
| :--- | :--- | :--- | :--- | :--- |
| **P1** | **High-Resolution Stadium Crowd Sprites** | `remaster_pack\stadium\crowd_default.png` ($2048 \times 512$) | Easy (Asset streaming via manifest) | Replaces flat 2006 cardboard cutouts with varied, authentic supporters wearing club colors. |
| **P2** | **Authentic Kit Sleeve Patches & Numbers** | `remaster_pack\kits\fonts\` & `kserv.dll` | Medium (Palette-indexed font atlas) | Adds UEFA Champions League starballs, 2006 Premier League lion sleeve badges, and vector kit lettering. |
| **P3** | **Rain & Wet Pitch Specular Map** | `remaster_pack\pitch\turf_wet_specular.png` | Medium (D3D8 specular stage hook) | Adds subtle sheen and light reflections on the pitch during rainy and evening floodlight matches. |
| **P4** | **Authentic 2006 Corner Flags & Goal Posts** | `remaster_pack\pitch\goal_props.png` | Easy (Texture replacement) | High-contrast sponsor flags and realistic hexagonal goal net textures with micro-mesh detail. |
| **P5** | **Expanded Scoreboard Theme Packs** | `remaster\graphics\modern_hud.cpp` | Medium (INI theme switcher) | Optional vintage 2006 Sky Sports and UEFA Champions League television broadcast scoreboard graphics. |

---

### Section 2: Advanced Decompilation & AI Engine Backlog (Specialist Agents)

Tasks requiring deep x86 disassembly, Ghidra decompilation, and reverse engineering of Konami's proprietary engine data structures are preserved in this backlog for execution by advanced coding agents (Claude Opus / Codex):

```
+--------------------------------------------------------------------------+
|            STAGE 20+ SPECIALIST AGENT ARCHITECTURAL BACKLOG              |
+--------------------------------------------------------------------------+
|  Agent: Claude Opus / Codex                                              |
|  Scope: Deep x86 Assembly, Ghidra Decompilation, Protocol Reversal        |
+--------------------------------------------------------------------------+
|  1. Commentary Audio Stream Detour:                                      |
|     - Memory target: 0x00612A00 (Audio voice bank index)                 |
|     - Objective: Intercept commentary trigger IDs to play high-fidelity   |
|       FLAC/WAV audio assets instead of low-bitrate ADX streams.          |
+--------------------------------------------------------------------------+
|  2. Animation Transition State Machine Decoupling:                       |
|     - Memory target: 0x005E3280 (Player skeletal animation controller)   |
|     - Objective: Smooth blend transitions between sprinting, ball touch,  |
|       and shooting animations to eliminate legacy 2-frame snap.          |
+--------------------------------------------------------------------------+
|  3. Advanced Referee Collision & Advantage Rule AI:                      |
|     - Memory target: 0x00A18040 (Foul arbitration dispatcher)           |
|     - Objective: Restore authentic advantage play logic without blowing  |
|       premature whistles on through-ball opportunities.                  |
+--------------------------------------------------------------------------+
```

---

### Section 3: Engineering Guardrails for Stage 20

To ensure the project remains stable and playable at all times, the following rules apply to Stage 20 development:

1. **Rule 1 — Zero Unverified Claims:**  
   Every asset and code hook must be verified by automated unit tests (`test_core.exe`) or runtime memory profiling before documentation.
2. **Rule 2 — Preserved Vanilla Fallback:**  
   The game must always boot and play 100% identically to vanilla PES 6 when `remaster.ini` has `[core] enabled = 0`.
3. **Rule 3 — Strict Memory Budgeting:**  
   Total process memory must never exceed $1.2\text{ GB}$ (leaving $\ge 800\text{ MB}$ headroom below the 2 GB 32-bit ceiling).
4. **Rule 4 — No Uncanny Valley Facial Experiments:**  
   Do not introduce procedural or AI facial reconstruction unless backed by verified, photorealistic vertex-matched geometry and community review.
