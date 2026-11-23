# Advanced Engineering Technical Backlog
**PES 6 Legacy Remaster: Architecture Specifications for Specialist Coding Agents (Codex & Claude Opus)**
*Target Phase: Advanced Reconstruction | Date: November 2026*

---

## 1. Executive Summary

This backlog structures the high-complexity, deep-architectural tasks deferred from Antigravity's immediate scope. These tasks require deep binary reverse-engineering in Ghidra, custom assembly detours, vertex buffer re-encoding, and physical simulation rewrites.

All previous Ghidra decompilation notes, reverse-engineering findings (REA), and Blender 5.3 MCP geometric setups are preserved to serve as the foundation for these future initiatives.

---

## 2. Deferred Technical Epics

```
+----------------------------------------------------------------------------------------------------+
|                                    DEFERRED ADVANCED EPICS                                         |
+-----------------------------------+-----------------------------------+----------------------------+
| Epic Name                         | Primary Domain                    | Recommended Agent          |
+-----------------------------------+-----------------------------------+----------------------------+
| EPIC-1: Bespoke 3D Face Geometry  | PS2 VIF / Vertex Morph Re-encode  | Claude Opus / Codex        |
| EPIC-2: Skeletal Animation Engine | Havok / Bone Matrix Streaming     | Claude Opus                |
| EPIC-3: PBR Shading & Deferred D3D| Direct3D 9 -> D3D11/Vulkan Bridge | Codex                      |
| EPIC-4: Deep Football AI Rewrite  | Tactical Decision Tree Overhaul   | Claude Opus                |
| EPIC-5: Ball & Collision Physics  | Aerodynamic Magnus Force Engine   | Codex / Claude Opus        |
+-----------------------------------+-----------------------------------+----------------------------+
```

---

## 3. Detailed Epic Specifications

### 3.1 EPIC-1: Bespoke 3D Player Mesh & Morph Re-encoding
* **Problem:** Konami's special face model structure uses a fixed-topology 66,688-byte envelope with PlayStation 2 VIF packet vertex formatting (128 B header, 1024 B palette, 8192 B $64\times 128$ texture, 2048 B $32\times 64$ texture, 55,296 B vertex pool). Importing modern high-poly meshes ($> 5,000$ vertices) crashes the fixed streaming buffer.
* **Known Engine Limitations:** The vertex buffer allocation at `0x00967188` expects a fixed stride and hardcoded vertex count.
* **Relevant Files:** `scratch/inspect_face_structure.py`, `scratch/dump_all_special_faces.py`, `remaster/src/hooks/player_hooks.cpp`.
* **Required Reverse-Engineering Evidence:** Ghidra analysis of `CPlayerFace::LoadFromAfs` at `0x0078A120` to hook vertex stream allocation and support dynamic vertex buffers.
* **Smallest Viable Proof of Concept:** Increase vertex count by 25% on a single player (e.g. Thierry Henry) while preserving existing skeletal bone indices.

### 3.2 EPIC-2: Skeletal Animation & Bone Matrix Decoupling
* **Problem:** PES 6 player animations are tied to a 19-bone legacy skeleton with baked keyframes at 60 Hz. Adding secondary motion (jersey flapping, hair physics, individual finger bones) requires runtime bone matrix interpolation.
* **Technical Risks:** Desynchronization between physics collision capsules and visual meshes, leading to ghost tackles or feet sliding.
* **Integration Point:** `CCharacterModel::UpdateBoneMatrices` at `0x0061E450`.
* **Smallest Viable Proof of Concept:** Procedural hair strand bone simulation on players with long hair (e.g. Ronaldinho, Drogba) reacting to head acceleration vectors.

### 3.3 EPIC-3: Physically Based Rendering (PBR) & Deferred Lighting
* **Problem:** PES 6 relies on legacy forward rendering with fixed-function Direct3D 8/9 lighting (ambient + single directional light). Night stadiums lack authentic stadium floodlight shadow casting.
* **Relevant Files:** `remaster/graphics/d3d8.cpp`, `remaster/graphics/postprocess.cpp`.
* **Required Evidence:** Intercepting the Direct3D 9 device creation to inject a custom G-Buffer (Normal, Roughness, Metallic, Depth) using DXVK or custom D3D9 hooks.
* **Technical Risks:** Overrunning 32-bit process memory limits ($< 2\text{ GB}$ address space without Large Address Aware patching).
* **Smallest Viable Proof of Concept:** Screen-space ambient occlusion (SSAO) post-processing pass rendering into the backbuffer.

### 3.4 EPIC-4: Deep Tactical Football AI Rewrite
* **Problem:** AI decision-making (passing options, shooting vs dribbling) is determined by hardcoded probability tables in `pes6.exe`.
* **Technical Risks:** Disrupting the iconic, responsive feel of PES 6 by introducing uncalibrated neural/behavior tree latencies.
* **Integration Point:** `CAITeamDecision::SelectPassTarget` at `0x00523C10`.
* **Smallest Viable Proof of Concept:** Reinforce defensive line coordination during counterattacks to reduce vulnerability to long diagonal through-balls.

### 3.5 EPIC-5: Aerodynamic Magnus Effect & Ball Collision Reconstruction
* **Problem:** Ball trajectory utilizes a simplified 2-pole gravity and drag model without full continuous Magnus lift calculations for dipping knuckleball strikes.
* **Relevant Files:** `remaster/gameplay/enhanced.cpp`, `remaster/analytics/analytics.cpp`.
* **Smallest Viable Proof of Concept:** Dynamic Magnus curve modulation on free kicks based on ball spin angular velocity ($d\vec{\omega}/dt$).

---

## 4. Preservation of Previous Research & Tooling

All preparatory tools, reverse-engineering scripts, and documentation developed in Stages 1–18 remain fully intact in:
- `docs/engineering/` — Complete engineering audit history.
- `archive/face_remaster_stages15_18/` — Full 3D face models, textures, and synthesis scripts.
- `scratch/` — Ghidra memory dumps, entry #485 player database parser, and Blender MCP client code.
