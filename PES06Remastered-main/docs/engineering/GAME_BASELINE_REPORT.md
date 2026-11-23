# PES 6 Legacy Remaster — Game Baseline Report

**Date:** 2026-11-20  
**Environment:** Windows 11 x64  
**Workspace Root:** `C:\Users\dream\Downloads\PES06Remastered-main`  
**Report Status:** COMPLETE & VERIFIED

---

## 1. Environment Architecture & Isolation

Three isolated environments have been formally established to ensure the source game installation is protected against accidental modification:

```
[ORIGINAL - Pristine Game Files]
  Path: C:\Users\dream\Downloads\PES06Remastered-main\PES 6 game files\
  Policy: READ-ONLY. Untouched. Never patched or written to.

[DEVELOPMENT - Remaster Source & Tools]
  Path: C:\Users\dream\Downloads\PES06Remastered-main\PES06Remastered-main\
  Policy: Version-controlled development repository (remaster C++ source, tools, docs).

[TEST GAME - Isolated Working Copy]
  Path: C:\Users\dream\Downloads\PES06Remastered-main\test_game\
  Policy: Runtime validation, ASI loading, texture testing, and integration sandbox.
```

---

## 2. Integrity Baseline & Binary Profile

### Primary Executable (`pes6.exe`)
- **Path:** `C:\Users\dream\Downloads\PES06Remastered-main\PES 6 game files\pes6.exe`
- **File Size:** 36,003,840 bytes (34.34 MB)
- **SHA-256:** `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`
- **Machine Architecture:** x86 (0x014C, 32-bit PE32)
- **Subsystem:** Windows GUI (subsystem 2)
- **Image Base:** `0x00400000`
- **Entry Point RVA:** `0x0077CEE0`
- **Size of Image:** `0x0511A000` (85,041,152 bytes)
- **PE Sections:**
  - `¡·V t`: Virtual Address `0x00001000`, Virtual Size `0x0077C000`, Raw Size `0x0077BF00`
  - `.rdata`: Virtual Address `0x0077D000`, Virtual Size `0x00047000`, Raw Size `0x00046280`
  - `.data`: Virtual Address `0x007C4000`, Virtual Size `0x03195000`, Raw Size `0x002E212B`
  - `.data1`: Virtual Address `0x03959000`, Virtual Size `0x00001000`, Raw Size `0x000008D0`
  - `.rsrc`: Virtual Address `0x0395A000`, Virtual Size `0x00003000`, Raw Size `0x00002144`
  - `age`: Virtual Address `0x0395D000`, Virtual Size `0x004B1000`, Raw Size `0x004B0030`
  - `agis`: Virtual Address `0x03E0E000`, Virtual Size `0x0000C000`, Raw Size `0x000006F3`
  - `quod`: Virtual Address `0x03E1A000`, Virtual Size `0x00219000`, Raw Size `0x00218729`
  - `.rld`: Virtual Address `0x04033000`, Virtual Size `0x010E7000`, Raw Size `0x010E0000`
- **Build Identity:** SecuROM/Fixed retail executable with unpacked code and `.rld` payload. All 130 documented PESMod and Remaster hook targets were verified to resolve into valid code space via `verify_addresses.py`.

### Configuration Tool (`settings.exe`)
- **Path:** `C:\Users\dream\Downloads\PES06Remastered-main\PES 6 game files\settings.exe`
- **File Size:** 513,931 bytes
- **SHA-256:** `d7aad358beb4049228ea3129f3d37a895454273dd8be6d4694d694dd87b2208b`
- **DirectX Symbol:** Imports `d3d8.dll!Direct3DCreate8`.
- **Target Settings Path:** Writes binary settings to `%USERPROFILE%\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\settings.dat` (420 bytes).

### Core Game Data Archives (`dat/`)
All primary game archives in `PES 6 game files\dat\` were hashed to establish the permanent non-modified baseline:

| Archive Name | File Size | SHA-256 Hash |
|---|---|---|
| `0_text.afs` | 444,692,480 bytes | `f611a065e47596e0a31c477a9e34b09e2ccf019a728ee6c223b2d6bc357df985` |
| `0_sound.afs` | 24,797,184 bytes | `d90c9d8a6712a5227b901bcf1e622da592539961e78b02222a40922ccf75be7b` |
| `e_text.afs` | 24,797,184 bytes | `d90c9d8a6712a5227b901bcf1e622da592539961e78b02222a40922ccf75be7b` |
| `e_sound.afs` | 434,155,520 bytes | `c1b6c21a894a36f22093e0886e648bfa5552ce83ced4f370b93a433d220d50c5` |

---

## 3. Observed Vanilla & Mod Runtime Evidence

From runtime logs captured during actual execution of `pes6.exe`:

1. **Rendering API:** Direct3D 8 (`d3d8.dll`), initialized via `Direct3DCreate8(D3D_SDK_VERSION = 220)`.
2. **Device Creation:** Virtual table index 13 / slot `0x34` is queried; device is created with backbuffer `640x480` and reset during presentation mode transitions (`800x600` observed in windowed/fullscreen tests).
3. **Kitserver Integration:**
   - The game installation contains `kitserver` 6.8.2 (`kload.dll`) configured with 16 modules (`zlib1`, `libpng13`, `afs2fs`, `kserv`, `bserv`, `fserv`, `lodmixer`, `dxtools`, `stadium`, `bootserv`, `speeder`, `gloves`, `skins`, `camerazoomer`).
   - Verified clean hook execution and clean detachment on process termination (`{KLOAD} Closing log.`).
4. **Match Engine & Gameplay:**
   - Successfully loaded into "Blautraum Stadion" (night/fine lighting).
   - AFS replacement and file streaming verified via `stad1_main.bin`, `crowd_h3.str`, `crowd_a3.str`, `adboards_tex\default.bin`, and ball files.
5. **Option Files & Saves:**
   - User profile storage confirmed at `C:\Users\dream\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\save\folder1\KONAMI-WIN32PES6OPT` (1,191,936 bytes).
