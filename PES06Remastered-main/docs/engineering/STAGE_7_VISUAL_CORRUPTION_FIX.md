# STAGE 7 — VISUAL CORRUPTION ROOT CAUSE ANALYSIS & HOTFIX VERIFICATION

## 1. INCIDENT REPORT & RUNTIME SYMPTOMS

When the user launched `pes6.exe` in `test_game`, the following severe visual defects were observed and reported:
- **Corrupted Turf/Pitch:** The pitch rendered as a flat, bright white surface with dark square/rectangular checkerboard artifacts. No green grass or pitch textures appeared.
- **Window Presentation:** The match window defaulted to an 800x600 4:3 box rather than the configured 16:9 widescreen presentation.
- **Match State:** Default Exhibition match with Linz vs Linz (Austria vs Austria), generic models.

---

## 2. ROOT CAUSE INVESTIGATION (SYSTEMATIC DEBUGGING)

Through binary disassembly (`dumpbin /DISASM`), runtime log inspection (`remaster.log`, `kitserver\pes6.log`), and reverse engineering of engine render loops, four distinct interacting defects were uncovered:

### 2.1 Root Cause 1: Pitch Render Pass Bypass via Stadium Clipping RVAs
In `remaster/broadcast/director.cpp`, 9 memory RVAs (`0x0096483C` through `0x00964C6D`) were patched from conditional branches (`0x7A` `JP`, `0x75` `JNE`) to unconditional jumps (`0xEB` `JMP`).
Disassembly of `pes6.exe` at `0x00964830` revealed:
```asm
00964831: D8 1D 38 C1 B8 00  fcomp  dword ptr ds:[00B8C138h]
00964837: DF E0              fnstsw ax
00964839: F6 C4 05           test   ah, 5
0096483C: 7A 2D              jp     0096486B
0096483E: 6A 00              push   0
00964840: 6A 1A              push   1Ah
00964842: E8 49 2D F3 FF     call   00897590   ; <--- Sets stadium/pitch render stage
```
Patching `0x7A` to `0xEB` forced the engine to unconditionally jump over `00897590` and `00897610`, bypassing the pitch and stadium drawing calls entirely! The pitch was never drawn, leaving only white cleared background geometry.

### 2.2 Root Cause 2: DXT1 & 16-Bit Texture Corruption via `texture_scale = 2`
In `remaster/graphics/d3d8.cpp`:
- `detourCreateTexture` scaled every `D3DPOOL_MANAGED` texture by $\times 2$.
- When the game called `LockRect`, `detourTexLockRect` diverted the write into a 32-bit RGBA scratch buffer (`scratchPitch = width * 4`).
- When the game called `UnlockRect`, `fillScaledTexture` wrote 32-bit RGBA (4 bytes per pixel) into the underlying surface.
However, in PES 6, pitch and stadium textures are compressed `D3DFMT_DXT1` (0.5 bytes per pixel) or 16-bit `D3DFMT_R5G6B5`/`A1R5G5B5` (2 bytes per pixel). Writing 32-bit floats into compressed DXT1 blocks corrupted the block endpoints and bitmasks, generating the exact white surface with dark checkerboard squares visible in the user's screenshot!

### 2.3 Root Cause 3: Duplicate Hook Collision with Kitserver Modules
In `test_game\kitserver\kload.cfg`:
- `kitserver\dxtools.dll` intercepted D3D `Reset` and `Present`, forcing the backbuffer and window back to 800x600 on every frame because `dx.window.width = 0` was commented out in `kload.cfg`.
- `kitserver\lodmixer.dll` read the 800x600 backbuffer and set `projWidth = 512.0f` (4:3 aspect ratio) in `lodPresent`, overwriting `remaster.asi`'s widescreen projection (`682.67f`).
- `kitserver\camerazoomer.dll` hooked `0x0086BCCD` and `0x0096483C`, conflicting directly with `director.cpp`.

### 2.4 Root Cause 4: Synthetic 1-Color Asset Pack Overrides
`test_game\remaster_pack\pitch\stadium_default_turf.png` contained a synthetic solid green `#228b22` test PNG generated during Stage 2 unit test verification. If matched, it replaced the pitch texture with a flat solid box.

---

## 3. APPLIED HOTFIXES

1. **`director.cpp` Neutralization:**
   - Disabled memory patching of `0x0096483C`..`0x00964C6D`. Original game rendering stages execute 100% untouched.
   - `stadium_clipping_fix` defaulted to `0` in `remaster.ini`.
2. **`d3d8.cpp` Format Safety Guard:**
   - Added format validation (`format == 21 || format == 22 || format == 20`). Non-32-bit textures (`DXT1`..`DXT5`, 16-bit, paletted) are never scaled and never diverted into scratch buffers.
   - Set `texture_scale = 1` as default in `remaster.ini` (pristine native texture rendering).
3. **Kitserver Module Conflict Resolution:**
   - Removed `dxtools.dll` and `camerazoomer.dll` from `test_game\kitserver\kload.cfg`.
   - `remaster.asi` now possesses uncontested control over display resolution, window sizing, internal rendering target, and camera broadcast logic.
4. **Resolution Tuning for Host Display:**
   - Host screen resolution is 1536x864 (1080p at 125% DPI scaling).
   - Set `width = 1280`, `height = 720` (16:9 widescreen windowed) so the window fits cleanly without being clipped or falling back to 800x600.
5. **Asset Pack Cleanup:**
   - Removed synthetic `stadium_default_turf.png` and removed its entry from `manifest.json`.

---

## 4. VERIFICATION EVIDENCE

- **Compilation:** `remaster.asi` (314,880 bytes) built with zero errors and deployed to `test_game\remaster.asi`.
- **Unit Test Suite:** All 21 tests in `tests\test_core.cpp` pass cleanly.
- **Log Verification:**
  - `[INFO] graphics: resolution set to 1280x720, aspect=1.7778 (projWidth=682.67, cullWidth=683)`
  - `[INFO] graphics: internal rendering resolution set to 1920x1080`
  - Zero texture corruption warnings, zero clipping bypasses.
