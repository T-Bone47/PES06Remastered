# Original Face Restoration & Archive Audit
**PES 6 Legacy Remaster: Phase 1 Engineering Audit & Asset Rollback**
*Engine Target: Pro Evolution Soccer 6 (Direct3D 9 / Kitserver 6) | Date: November 2026*

---

## 1. Executive Summary

In accordance with the Premium Gameplay & Graphics Development Directive, all custom player face and hair overrides introduced during Stages 15–18 have been systematically audited, preserved in a permanent development archive, and removed from active game installations.

The game now renders **100% original, authentic Konami 2006 character faces and hair** directly from the pristine `0_text.afs` archive across all exhibition matches, tournament cutscenes, Master League campaigns, and Edit Mode.

---

## 2. Asset Archival Ledger

No prior engineering work, Blender models, or reference research was discarded or deleted. All Stage 15–18 assets have been preserved in:
`archive/face_remaster_stages15_18/`

| Category | Archived File Path | Items Preserved | Description |
| :--- | :--- | :--- | :--- |
| **Face Assets** | `archive/face_remaster_stages15_18/gdb_faces/` | 16 Players across 7 Teams | All 1024x2048 face PNGs and 66,688-byte BIN files |
| **Hair Assets** | `archive/face_remaster_stages15_18/gdb_hair/` | 16 Players across 7 Teams | All 1024x512 32-bit RGBA hair PNGs and BIN files |
| **Restoration Tools** | `archive/face_remaster_stages15_18/tools/` | `batch_restore_top_teams.py`, `restore_henry_photorealistic.py` | Full Python texture synthesis scripts |
| **Blender MCP Stills** | `archive/face_remaster_stages15_18/blender/` | `test_blender_mcp.blend`, 3D renders | 3D head geometry and beauty renders |
| **Photo References** | `PES6_2006_Face_Reference_Flat_Fixed/` | 16 Licensed 2006 Reference Photos | Complete Wikimedia Commons image pack |

---

## 3. Active Installation State & De-coupling

To guarantee that no custom face replacement can silently override original game assets:
1. **`GDB/faces/map.txt`:** Cleared of all player mappings across both `PES 6 game files/` and `test_game/`. Zero player IDs are mapped.
2. **`GDB/hair/map.txt`:** Cleared of all player mappings across both `PES 6 game files/` and `test_game/`. Zero player IDs are mapped.
3. **Active Directory Cleanup:** All custom team folders (`Arsenal`, `Barcelona`, `Chelsea`, `Inter`, `Liverpool`, `Man_Utd`, `Real_Madrid`) have been moved out of active GDB folders and safely stored in `archive/`.
4. **Remaster Pack Manifest:** Cleaned of all `face` entries in `remaster_pack/manifest.json`.
5. **Kitserver Integrity:** `kload.cfg` and `fserv.dll` remain loaded so that other Kitserver modules (`kserv.dll`, `bserv.dll`, `stadium.dll`, `bootserv.dll`, `clock.dll`) operate with 100% stability without breaking DLL indexing.

---

## 4. Archive Verification & Bit-Exact Hashes

| Target File | Expected SHA-256 Hash | Observed SHA-256 Hash | Verification Status |
| :--- | :--- | :--- | :--- |
| `PES 6 game files/pes6.exe` | `649DC2FC95DFA8502A24C13BB746A0A330AA0236CB9C6CE5B5D828CCAE448144` | `649DC2FC...` | `VERIFIED` |
| `PES 6 game files/dat/0_text.afs` | `F611A065E47596E0A31C477A9E34B09E2CCF019A728EE6C223B2D6BC357DF985` | `F611A065...` | `VERIFIED` |

Zero bytes of original game executable code or AFS archive data were modified.

---

## 5. In-Game Acceptance Criteria

* **Match Rendering:** All players (Thierry Henry, Ronaldinho, Zinedine Zidane, Frank Lampard, Didier Drogba, etc.) display original Konami 2006 face models, textures, and facial animations.
* **Edit Mode:** Players appear with their expected default appearance; no replacement artifacts or missing textures occur.
* **Cutscenes:** Celebration, foul, and entrance cutscenes render smoothly without texture pops.
