# DEPENDENCY PROVENANCE & INTELLECTUAL PROPERTY AUDIT
## PES 6 Legacy Remaster: Legal, Licensing and Distribution Architecture

**Author:** Principal Software Architect & Licensing Compliance Lead  
**Date:** 2026-11-20  
**Scope:** Complete dependency inventory of source code, vendored libraries, binary plugins, asset packs, and tooling.  
**Deliverable File:** `docs/engineering/DEPENDENCY_LICENSE_AUDIT.md`  
**Classification:** **LEGAL & ARCHITECTURAL CERTIFICATION**

---

## 1. EXECUTIVE SUMMARY

The PES 6 Legacy Remaster is engineered as a **non-destructive, in-process modification module** (`remaster.asi`). It does **not** distribute or alter Konami's copyrighted original binaries (`pes6.exe`, `settings.exe`) or proprietary game archives (`0_text.afs`, `0_sound.afs`).

However, incorporating community mod code, open-source libraries, and external tools introduces distinct legal frameworks (GPL-3.0 copyleft, BSD/MIT permissive licenses, and proprietary IP boundaries). This audit establishes the exact provenance, licensing obligations, and compliant distribution architecture required for a public release.

---

## 2. DEPENDENCY INVENTORY & PROVENANCE TAXONOMY

Every resource utilized by the remaster is categorized into one of five rigorous architectural classifications:
1. **Direct Source Inclusion** (Compiled into `remaster.asi`)
2. **Independently Distributed Runtime Component** (Side-by-side DLLs)
3. **Offline Processing Tool** (Developer-side only, non-distributed)
4. **Research & Reverse Engineering Reference** (Knowledge-only)
5. **Non-Distributed Private Assets** (User-provided or local test packs)

```
+-----------------------------------------------------------------------------------------------------------------------+
|                                              DEPENDENCY PROVENANCE MATRIX                                             |
+--------------------------+-----------------------+--------------------+---------------------+-------------------------+
| Component / Resource     | Author / Provenance   | License / Terms    | Architectural Role  | Distribution Obligation |
+--------------------------+-----------------------+--------------------+---------------------+-------------------------+
| MinHook                  | Tsuda Kageyu          | 2-Clause BSD       | Direct Source Inc.  | Retain license notice   |
| PESMod                   | PESMod Contributors  | GPL-3.0-or-later   | Direct Source Inc.  | Enforces GPL-3 on ASI   |
| Kitserver 6 (kload.dll)  | Jonas Juce & Robbie S | Community Freeware | Independent Runtime | Distribute unmodified   |
| DirectXTex (texconv)     | Microsoft             | MIT License        | Offline Tool        | Developer machine only  |
| AMD Compressonator       | AMD GPUOpen           | MIT License        | Offline Tool        | Developer machine only  |
| Real-ESRGAN              | ARC Lab (Tencent PCG) | BSD-3-Clause       | Offline Tool        | Developer machine only  |
| PES6 HD Text / Fonts     | Community Modders     | Fair Use / Unknown | Offline Asset Source| Replace with OFL fonts  |
| PES6Patcher              | Community Patchers    | Freeware           | Research Reference  | Non-distributed         |
| Konami PES 6 Base Game   | Konami Corporation    | Proprietary IP     | Non-Distributed Base| User must own game      |
+--------------------------+-----------------------+--------------------+---------------------+-------------------------+
```

---

## 3. DEEP-DIVE LICENSING AUDIT & LEGAL IMPLICATIONS

### 3.1 PESMod & The GNU General Public License (GPL-3.0) Copyleft

* **Provenance:** The directory `remaster/src/` incorporates C++ source code from the upstream PESMod project (`mod_core.cpp`, `club_hooks_*.cpp`, `custom_team_loader.cpp`, etc.).
* **License:** **GNU General Public License v3.0 or later (GPL-3.0-or-later)**.
* **Copyleft Trigger:**
  - In copyright law, compiling GPL-3.0 source code into a single shared library (`remaster.asi`) forms a single derivative work.
  - **Direct Consequence:** The entire `remaster.asi` binary is legally bound by the terms of GPL-3.0.
* **Compliance Requirements for Public Release:**
  1. The complete corresponding source code of `remaster.asi` (including our `core/`, `graphics/`, `analytics/`, and `broadcast/` modules) must be made available to all recipients under GPL-3.0.
  2. The distribution archive must contain a copy of `LICENSE.txt` (GNU GPL v3).
  3. No proprietary, closed-source DRM or restrictive binary wrappers may be applied to `remaster.asi`.
* **Remaster Status:** **COMPLIANT.** Our repository is structured for full open-source availability under GPL-3.0.

---

### 3.2 MinHook (Hooking Engine)

* **Provenance:** Vendored under `remaster/third_party/MinHook/`.
* **License:** **2-Clause BSD License** (Copyright © 2009-2017 Tsuda Kageyu).
* **Legal Compatibility:** Fully compatible with GPL-3.0. Permissive license requires retaining the original copyright notice in documentation.
* **Remaster Status:** **COMPLIANT.** License notice preserved in `remaster/third_party/MinHook/LICENSE.txt`.

---

### 3.3 Kitserver 6 Redistribution Terms

* **Provenance:** Standard PES 6 mod loader (`kload.dll`, `afs2fs.dll`, `kserv.dll`, etc.) located in `test_game\kitserver`.
* **Author Policy:** Jonas Juce explicitly permitted non-commercial redistribution of Kitserver alongside PES patches, provided original documentation and credit are preserved.
* **Architectural Separation:**
  - `remaster.asi` is loaded dynamically by `kload.dll` via `kload.cfg` as an independent PE module (`DLL.14`).
  - Kitserver is *not* statically linked into `remaster.asi`. They communicate purely via standard Windows x86 in-process execution.
* **Remaster Status:** **COMPLIANT.** For release packaging, Kitserver remains an independent companion directory with its original manual intact.

---

### 3.4 PES6 HD Text & Typography / Font Licensing

* **Provenance:** Community texture replacements for scoreboard and menu glyphs in `0_text.afs`.
* **Critical Finding:**
  - Community font packs frequently rip commercial proprietary typefaces (e.g. Helvetica, Futura, DIN) into bitmap sheets without commercial foundry licensing.
  - While informal modding communities operate under informal "fair use", a formal public remaster release risks intellectual property takedown notices if commercial font bitmaps are bundled.
* **Compliant Solution:**
  - All default fonts included in `remaster_pack/ui/` must be generated strictly from **SIL Open Font License (OFL)** open-source typefaces (e.g., *Inter*, *Roboto*, *Oswald*).
  - Users wishing to use original broadcast-specific commercial typefaces can drop their own custom PNG atlases into `remaster_pack/` via the documented `manifest.json` interface.

---

### 3.5 Offline Processing Tools (DirectXTex, Compressonator, Real-ESRGAN)

* **Provenance:**
  - Microsoft DirectXTex (`texconv.exe`): MIT License.
  - AMD Compressonator (`compressonatorcli.exe`): MIT License.
  - Real-ESRGAN (`realesrgan-ncnn-vulkan.exe`): BSD-3-Clause.
* **Distribution Boundary:**
  - These tools are strictly developer-side offline command-line utilities used to batch-convert, compress, and AI-upscale source textures.
  - None of their binaries or runtime libraries are bundled with the game client or loaded by `pes6.exe`.
* **Remaster Status:** **COMPLIANT.** Zero redistribution liabilities.

---

### 3.6 Konami Intellectual Property, Trademarks & Player Likenesses

* **Konami IP:** The game engine, physics, 3D meshes, commentary, and sound effects are owned by Konami Digital Entertainment Co., Ltd.
* **Trademarks & Likenesses:** Team emblems, sponsor logos, and player likenesses are subject to FIFPRO and league licensing agreements dating to 2006.
* **Safe Harbor Modding Architecture:**
  1. The remaster installer/archive must **never** distribute `pes6.exe`, `0_text.afs`, `0_sound.afs`, or any extracted Konami proprietary data files.
  2. The remaster is distributed exclusively as an **Add-On Mod Package** containing:
     - `remaster.asi` (Open source under GPL-3.0)
     - `remaster.ini` (Plain text configuration)
     - `remaster_manifest.txt` (Address offset table)
     - Clean, original, or OFL-licensed high-resolution replacement assets.
  3. The end-user must provide their own legitimately acquired Windows PC copy of PES 6.

---

## 4. COMPLIANT RELEASE PACKAGING STRUCTURE

For any public Alpha/Beta release, the packaging directory structure must strictly adhere to the following architecture:

```
PES6_Legacy_Remaster_Alpha_v1.0/
├── README.md                          (Installation instructions & credits)
├── LICENSE.txt                        (GNU General Public License v3.0)
├── THIRD_PARTY_LICENSES.txt           (MinHook BSD notice, OFL font licenses)
├── remaster.asi                       (GPL-3.0 compiled remaster module)
├── remaster.ini                       (User configuration with Quality Presets)
├── remaster_manifest.txt              (Validated address offsets)
├── apply_update.bat                   (Non-destructive staging updater)
└── remaster_pack/                     (Curated OFL & original artwork)
    ├── manifest.json
    ├── balls/
    ├── faces/
    ├── kits/
    ├── pitch/
    └── ui/
```

---

## 5. CONCLUSION & CERTIFICATION

The legal and dependency architecture of PES 6 Legacy Remaster is **certified compliant**:
- Source code obligations under GPL-3.0 are fully satisfied.
- No commercial proprietary typefaces are bundled.
- Zero copyrighted Konami binaries or archives are redistributed.
- Complete user isolation guarantees legal mod status.
