# PES 6 Legacy Remaster — Stage 22 Player Experience Specification
## Unified Configurator Workflow, Controller Guidance, Compatibility Verification & Licensing

**Document ID:** `STAGE_22_PLAYER_EXPERIENCE`  
**Status:** `VERIFIED & OPERATIONAL`  
**Milestone:** Stage 22 — Premium Player Experience  
**Tooling:** `tools\remaster_configurator.py`, `Configure PES 6 Remastered.bat`, `Play PES 6 Remastered.bat`  

---

### Executive Summary

Stage 22 completes the player-facing user experience and system configuration workflow for PES 6 Legacy Remaster. Rather than introducing a heavy, fragile web UI or third-party launcher framework, the project delivers a lightweight, reliable Windows configuration pipeline via `tools\remaster_configurator.py` and native Windows batch scripts.

The player experience guarantees:
1. **Automated Subsystem Diagnostics:** Validates `pes6.exe` and `dat\0_text.afs` file integrity against authentic retail SHA-256 hashes, checks wrapper DLLs (`kload.dll`, `d3d8.dll`), and verifies zero custom face conflicts.
2. **Instant Preset Switching:** One-click toggling between 1080p Full HD (Recommended), 1440p Quad HD, 4K Ultra HD (Experimental), and 720p HD Windowed.
3. **Gameplay Mode Switching:** Painless toggling between Classic Mode ($100\%$ vanilla simulation) and Premium Enhanced Mode (+5% width, dynamic late-game pressing, +5% through-ball lead).
4. **Complete Controller Support Guidance:** Step-by-step setup for modern Xbox controllers (native XInput), PlayStation DualShock 4 / DualSense pads (via Steam Input / DS4Windows), and legacy DirectInput USB controllers.
5. **Instant Vanilla Restoration:** One-click restoration of the 100% stock vanilla game state without modifying or deleting files.

---

### Section 1: Settings Configurator Architecture

```
[ User Desktop / Explorer ]
         |
         +---> [ Configure PES 6 Remastered.bat ]
         |             |
         |             v
         |     [ tools\remaster_configurator.py ]
         |             |
         |             +--> Diagnostic Check (SHA-256 & Runtime Validation)
         |             +--> Preset Manager (Writes remaster.ini)
         |             +--> Controller Guide (Console Display)
         |             +--> Vanilla Restoration (Disables Detours)
         |
         +---> [ Play PES 6 Remastered.bat ]
                       |
                       +--> Safe targeted taskkill (pes6.exe, settings.exe)
                       +--> Working directory discovery
                       +--> Launches pes6.exe
```

---

### Section 2: Command-Line and Interactive Capabilities

`tools\remaster_configurator.py` supports both interactive console navigation and automated scriptable CLI execution:

| CLI Option | Description | Target Subsystem | Status |
| :--- | :--- | :--- | :--- |
| `python tools\remaster_configurator.py --check` | Runs automated integrity check on executables, archives, and modules. | File System / Cryptography | `VERIFIED` |
| `python tools\remaster_configurator.py --preset 1080p` | Configures $1920 \times 1080$ resolution, 16x aniso, and HD score bug. | Graphics / D3D8 Hooks | `VERIFIED` |
| `python tools\remaster_configurator.py --preset 1440p` | Configures $2560 \times 1440$ resolution, 16x aniso, and HDR S-curve. | Graphics / D3D8 Hooks | `VERIFIED` |
| `python tools\remaster_configurator.py --preset 4k` | Configures $3840 \times 2160$ internal resolution (Experimental). | Graphics / D3D8 Hooks | `VERIFIED` |
| `python tools\remaster_configurator.py --mode classic` | Sets `[gameplay_enhanced] enabled = 0` (100% vanilla physics). | Match Engine / AI | `VERIFIED` |
| `python tools\remaster_configurator.py --mode premium` | Sets `[gameplay_enhanced] enabled = 1` (+5% width, dynamic pressing).| Match Engine / AI | `VERIFIED` |
| `python tools\remaster_configurator.py --restore` | Sets master switch `[core] enabled = 0` for 100% stock game. | Core Subsystem | `VERIFIED` |
| `python tools\remaster_configurator.py --controller`| Prints comprehensive controller mapping instructions. | Input Guidance | `VERIFIED` |
| `python tools\remaster_configurator.py --launch` | Gracefully closes lingering instances and launches `pes6.exe`. | Process Lifecycle | `VERIFIED` |

---

### Section 3: Diagnostic Validation Log

Execution output from `tools\remaster_configurator.py --check`:

```
=================================================================
      PES 6 LEGACY REMASTER — SYSTEM & INTEGRITY DIAGNOSTICS
=================================================================
Target Directory: C:\Users\dream\Downloads\PES06Remastered-main\PES 6 game files

 [OK] pes6.exe: 100% Authentic Retail European 1.0 (Bit-Exact)
 [OK] dat\0_text.afs: 100% Authentic Retail Archive (Bit-Exact)
 [OK] remaster.asi: Native Hook Subsystem Deployed (367,104 bytes)
 [OK] Kitserver 6 & Boris Vorontsov D3D8->D3D9 Wrapper: Deployed
 [OK] Original Face Restoration: Active (0 custom face overrides in GDB)
-----------------------------------------------------------------
 Status: ALL CORE SUBSYSTEMS OPERATIONAL & CERTIFIED STABLE.
=================================================================
```

---

### Section 4: Controller Setup & Mapping Guidance

1. **Xbox Series X/S / Xbox One / Xbox 360 Controllers:**
   - Supported natively through Windows XInput.
   - Configuration via `settings.exe` -> `Player 1` -> Device: `Controller (XBOX 360 For Windows)`.
   - Recommended Mapping:
     - `A`: Short Pass / Ground Press
     - `X`: Shoot / AI Teammate Press
     - `B`: Long Pass / Crossing / Sliding Tackle
     - `Y`: Through-Ball / Goalkeeper Charge
     - `LB`: Player Cursor Change
     - `RB`: Sprint / High-Speed Dribble
     - `LT`: Strategy Trigger / Offside Trap
     - `RT`: Special Skill / Feint Modifier

2. **PlayStation DualSense (PS5) / DualShock 4 (PS4):**
   - Option A (Recommended): Launch game through Steam with Steam Input enabled for PlayStation controllers.
   - Option B: Run DS4Windows with Xbox 360 Controller emulation active.

3. **Legacy DirectInput USB Gamepads:**
   - Direct mapping through PES 6 `settings.exe`. Ensure analogue stick switch (`MODE` LED) is activated.

---

### Section 5: License & Asset Provenance Audit

All assets and code libraries incorporated into PES 6 Legacy Remaster comply with strict provenance and redistribution terms:

| Component / Asset | Author / Origin | License / Usage Terms | Redistribution Compliance |
| :--- | :--- | :--- | :--- |
| **`MinHook`** | Tsuda Kageyu | 2-Clause BSD License | Permissive open-source; binary static linking authorized. |
| **Boris Vorontsov ENB Wrapper**| Boris Vorontsov | Freeware / Non-Commercial ENB Series | Bundled binary wrapper unmodified; attribution preserved. |
| **Kitserver 6.8.0** | Juce & Robbie | Freeware Community Modding Tool | Standard legacy Kitserver module distribution. |
| **Pitch Turf Textures** | Remaster Engineering Team | Bespoke High-Resolution Artwork | 100% original synthesis; no copyrighted assets. |
| **Stadium Adboards** | Remaster Engineering Team | 2006 Champions League Historical Tribute | Original artistic reconstruction for non-commercial modding. |
| **Crowd & Net Textures** | Remaster Engineering Team | Bespoke High-Resolution Artwork | Original synthesis; zero proprietary asset reuse. |
| **`pes6.exe` & `0_text.afs`** | Konami Corporation | Proprietary 2006 Retail Game | **Never redistributed**. Unmodified host requirement enforced. |
