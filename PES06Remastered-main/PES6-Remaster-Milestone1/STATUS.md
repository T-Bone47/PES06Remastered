# PES 6 Legacy Remaster — REA Capability & Project Status

**Date:** 2026-10-20
**Target:** `C:\Users\olive\Downloads\PES-6\PES-6-DownloadComputerGames.net\pes6.exe`
**SHA-256:** `0c7f804be8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f`
**PESMod upstream:** https://github.com/PanosPasch/pesmod @ `21021eb` (2026-09-11, GPL-3.0-or-later)
**Build origin (from the embedded PDB path):** `D:\soccer10\we10ms\vcproj\PC\Release\all.pdb`

## Project layout

All PES work lives under one folder, separate from any other project:

```
C:\Users\olive\Downloads\PES6-Remaster\
    STATUS.md            this file
    docs\                ASSETS.md - the replacement pack format
    remaster\            the ASI: build.bat, core\, graphics\, assets\, src\ (upstream)
    tools\               Python tooling for the archives and the asset index
    tools\rea\           Ghidra scripts, analysis output, capture scripts
    tools\scratch\       one-off experiment leftovers
    pesmod\              upstream clone
    references\          kitserver6, PES-Editor 6, AFS converter (read-only study)
    evidence\            captured proof logs
```

The game itself lives outside the project, in
`C:\Users\olive\Downloads\PES-6\PES-6-DownloadComputerGames.net\`. The
remaster is deployed into it as `remaster.asi`, `remaster.ini` and
`remaster_manifest.txt`; the archives there are never modified.

One piece of state stays outside the project on purpose: the Ghidra analysis
project, `C:\Users\olive\tools\ghidra-projects\PES6` (437 MB). It is
managed by the REA Ghidra provider and moving it would force a full re-import
of the 20 MB executable.

---

## 2026-10-20 — research session: baseline re-proof + two research tracks

**No feature work was done in this session.** Output is three documents:

| Document | What |
|---|---|
| `docs/BASELINE.md` | the re-proof, with commands and outputs |
| `docs/research/mapping_guide.md` | how packs can be addressed by name |
| `docs/research/deeper_football.md` | what the game exposes for match/AI/animation |
| `docs/NEXT_PHASE_PLAN.md` | the recommendation and the milestones |

**Baseline re-proof — all PASS.** Clean rebuild → 281,600 bytes; address gate
119/119; gameplay gate 101/101; 5 hooks; client area **1536 × 864**; one
pack substitution visible on screen; all four `.afs` sha256 unchanged.

### Two corrections to earlier conclusions

Both established by direct measurement this session.

**1. `0x20050420` is not a magic constant.** Earlier documents described the
per-asset "Rich container magic". It is not one:

* the exact 4-byte sequence occurs **once in the whole 20 MB binary**, at
  `0x008E246B`, inside a character-index lookup table
  (`00 20 20 01 02 03 20 20 04 05 20 06 07 …`);
* **no comparison against it exists anywhere** — `3D 20 04 05 20` (`cmp eax`)
  and `81 F8..FF 20 04 05 20` (`cmp r32`) were both searched for and are
  absent.

**The game parses asset headers structurally, not by magic.** The value in the
asset is a build-stamp field the community tools happen to use.

**2. The `caller=` field in the D3D8 log is not valid attribution.**
`_ReturnAddress()` under `/O2` returned `0x0040C84B`, which lands
**mid-instruction** inside `mov dword ptr [edi+0x144C], 0x0D`. The two
"caller" values quoted in the M4 section should not be relied on; a different
method (stack walk, or hooking the asset read path) is needed.

### Research A — name-authorable packs

The Scribd guides could not be fetched (bot challenge), but **the same
content exists machine-readably** in kitserver's `src/misc/afsmap.txt`
(360 lines, 289 named entries/ranges), which was read in full.

* The map's numbers **are AFS entry indices** — confirmed against our binary:
  the ranges land on groups of identically-sized assets (e.g. `5322 A 5443
  BOTAS` → every entry in it is 71,952 bytes inflated).
* The community's own tooling already names entries by index:
  `afswalk.cpp` does `sprintf(name.szFileName, "id_%00005d.bin", i)` —
  and its entry-49 output (`offset 0x00114800, size 0x760`) matches our
  parser **exactly**.
* The map's size annotations reveal the asset formats: entry 535
  "ESCUDOS 32X32" inflates to exactly **1,024 bytes** = 32×32×**1 byte per
  pixel**, and 536 "64X64" to 4,096. So the archive mixes 8-bit, 16-bit and
  32-bit textures — which is exactly why hashing the *uploaded* pixels can
  never match the *source* asset.

A full pack-by-name spec, with rules, fallbacks and the RE task list, is in
`docs/research/mapping_guide.md`.

### Research B — deeper football

The upstream layer is a team/kit/league/UI layer; it installs 33 hooks and
touches **no** match engine, AI, ball physics, camera or animation.

Located this session (all cross-checked in the Ghidra DB):

| Thing | Address | Confidence |
|---|---|---|
| player record table — **0x7C-byte records**, per-side base | `0x00861B90`, `0x00866B90`, `0x00866CB0` | high |
| match entity accessor + mode discriminator | `0x00862180`, `0x00425198` | high |
| match squad table — **2 teams × 32 player ids** | `DAT_03A8AA78`, writer `0x00A4AFA0` | high |
| match clock (float 0..0x23) and the tick region | `DAT_03A6A260`, `0x009A1B37`–`0x009A2851` | medium |
| central phase/state value — **174 references** | `DAT_03BECC60` | medium |
| bone world position — **505 callers** | `0x00950490` | high |
| actor acquire/configure/release API | `0x00950B40`, `0x009509B0`, `0x009502B0` | high |
| **AI decision routine** | — | **not located** |

**The AI is not located.** We have its inputs (the player records and the
attribute accessor) but not its decision function. It is not identified in
any of the functions examined, and the strings table is useless — the binary
is stripped and 236,988 "strings" are mostly noise.

Three goals are defined in `docs/research/deeper_football.md`: read-only
match telemetry (low risk), animation override (medium), AI parameters
(high, blocked). The recommendation in `docs/NEXT_PHASE_PLAN.md` is to start
with **option-file tooling plus read-only telemetry**, and to treat the AI as
a research task rather than an implementation task.

---


| Check | Result |
|---|---|
| Registry install key | `HKLM\SOFTWARE\Wow6432Node\KONAMIPES6\PES6` set (`installdir`, `lang_e=1`, `code` from the bundled 32-bit `.REG`) |
| Game launches | ✅ window `Pro Evolution Soccer 6`, 640×480 client, 207 MB |
| Title screen renders | ✅ PES 6 logo, "PRESS ANY BUTTON", ©2006 Konami |
| Main menu reached | ✅ MATCH / MASTER LEAGUE / LEAGUE / CUP / TRAINING / EDIT / OPTIONS / INTERNATIONAL CHALLENGE / NETWORK / EXIT |
| ASI chain | ✅ `dinput8.dll` (Ultimate ASI Loader v9.7.4) → `PESMod.asi` → `DllMain` |
| MinHook | ✅ initialised |
| Hooks installed | ✅ **35 hooks, zero errors, zero warnings** (log archived at `evidence/PESMod-phase0.log`) |

The log ends with `[PESMod] Initialisation complete.` — every hardcoded address in
PESMod resolved and hooked cleanly against this exact executable.

---


---

## 2026-10-20 - Milestone 1: option-file tooling + read-only match telemetry

**Baseline re-proof at the start of the session:** clean rebuild -> 285,184
bytes; address gate **119/119**; gameplay gate **101/101**; client area
**1536 x 864**; all four `.afs` sha256 unchanged.

### Task 1 - the log attribution is fixed

`_ReturnAddress()` under `/O2` was returning a value that landed **inside**
an instruction (`0x0040C84B` is the middle of
`mov dword ptr [edi+0x144C], 0x0D`), so every `caller=` in the D3D8 log was
meaningless.

Replaced with `_AddressOfReturnAddress()`, which yields the address of the
return-address slot and is therefore something the compiler cannot fold.

**VERIFIED.** The new capture returns `0x0040E7BA` for every texture
creation, and the five bytes ending there are

```
52 50 FF 51 50      push edx / push eax / call dword ptr [ecx+0x50]
```

so `0x0040E7BA` is exactly the instruction after the game's
`IDirect3DDevice8::CreateTexture` call. **This also corrects an earlier
conclusion:** the two "caller" values quoted in the M4 section
(`0x0040C84B` and `0x0040E7BA`) were one real and one garbage. The real
one is `0x0040E7BA`, inside `FUN_0040E700` - the texture creator the
research section had already identified.

### Task 2 - option-file tooling: implemented, in-game round trip UNVERIFIED

`tools/opd_tool.py` implements both obfuscation passes from kitserver's
`filedec.cpp`:

* a whole-file rolling XOR (`param = 0x573`, `param = param * 0x6C078965 + 1`)
* a keyed pass over nine sections using the 446 byte table and `0x7AB3684C`

**The order of the two passes matters** - the section pass reads its offsets
out of the file and the rolling XOR changes them, so decode is
`rolling XOR -> section decrypt` and encode is `section encrypt -> rolling
XOR`. Getting this wrong is why the first round trip failed.

**UNVERIFIED: no real option file was obtained.** The game has never
written one on this machine
(`<Documents>\KONAMI\Pro Evolution Soccer 6\save\` does not exist), menu
navigation is unreliable, and the only `.opd` in the tree -
`pesmod/test_files/unzlib_default00901.opd` - is a *decoded* file containing
plain text such as `common_wind.x`, not an option file. The layout in
`docs/OPD.md` is marked SOURCE-CLAIM throughout, and the one minimal
verified edit the task asked for was **not made**, because there was nothing
to edit.

### Task 3 - read-only match telemetry: harness VERIFIED, field mapping NOT

`remaster/telemetry/telemetry.cpp`, off by default, writes
`remaster_telemetry.csv` into the game folder. It only ever **reads**, and
every read is `__try`/`__except` guarded.

**VERIFIED**

* 1,222 samples over a 320.0 s window; 0 failed reads; the game stayed
  alive with it on and off; the address gate stayed at 119/119 and the
  gameplay gate at 101/101 in both configurations
* `DAT_03BECC60` is a genuine match **state** value: 0 in the menu, 2
  pre-match, 3 in play, 12 at a transition - the same sequence in three
  independent runs
* `DAT_01131FF4` (the side 0 record table base) goes from `NULL` to a heap
  pointer when a match starts, and then stays constant
* the globals window at `0x03A6A000` has 70 changing byte offsets

**NOT verified**

* `DAT_03A6A260`, the address nominated as the clock, read as a float was
  **0.0000 in every sample of four runs**. It is not the clock.
* The first 4,096 bytes of the side 0 record table **never changed** in a
  226 s run - the live player state is not at `base + slot * 0x7C` for slots
  0..31.
* **No position, stamina or possession field was found.** The decompiled
  entity accessor shows the game chooses between several tables by the mode:
  a static table at `DAT_0103E104` with a `0x7C` stride, another with a
  `0x48` stride, and `base + 0x34`. Our mode read returned 0 throughout, so
  the static fallback was in use - consistent with the attract-mode match
  that was being sampled rather than a user match.

`tools/analyze_telemetry.py` does the analysis: per-slot activity, a ranking
of the byte offsets that change, and the phase and clock timelines.

**=> The honest conclusion is that the harness works and the field mapping
is not done.** The blocker is reading the mode correctly (the obfuscated
trampoline's arithmetic does not yield 1 or 2 with the values observed, so
the disassembly is probably a decoy and the real implementation is
`FUN_00662425`, the function it tail-calls) and sampling a real user match
rather than the attract mode.

### Top candidate fields

| Rank | Address | Behaviour | Confidence |
|---|---|---|---|
| 1 | `DAT_03BECC60` | match state: 0 / 2 / 3 / 12 | medium (behaviour verified, meaning not) |
| 2 | `0x03A6AE40` | byte counter moving in +/-60 steps, live only during a match | low - a countdown or phase, not a clock |
| 3 | `0x03A6AE08`, `0x03A6AE18` | small counters, 0 -> 5 and 0 -> 6 | low |

---

## Phase 0 - Runtime gate: **PASSED**

## Phase 1 — Static analysis: **COMPLETE**

Ghidra 12.1.4 headless, `x86:LE:32:default:windows`, ~1320 s auto-analysis,
**34,578 functions** identified. Project retained at `C:\Users\olive\tools\ghidra-projects\PES6`.

### Address map: 130 documented targets

| Outcome | Count | Detail |
|---|---|---|
| Resolved **exactly** at a function entry | **118** | byte-level prologue check also agreed |
| Resolved but **inside** a function (entry moved) | 5 | see below |
| Not a function (data table) | 3 | `0x00C97340`, `0x00D5BF00`, `0x00D5BF30` — expected, these are table entries |
| Not code at all | 3 | `0x00456780` (pattern-scan fallback), `0x006094B0`, `0x006094F0` |
| Not code at all | 1 | `0x00B08B40` (`orig_PanelRefresh`) |

**The 5 needing re-location** (all four of the maintainer's `// REPLACE`-flagged
constants fall in this group, plus one unlabelled):

| VA | Label | Containing function | Offset into it |
|---|---|---|---|
| `0x005EE610` | `ADDR_MenuPageFinish` | `FUN_005ee080` | +1424 |
| `0x005FC1E0` | `ADDR_CheckTeamUnlocked` | `FUN_005fc127` | +185 |
| `0x008C6170` | `ADDR_GetMasterLeagueLoadedTeam` | `FUN_008c5f50` | +544 |
| `0x00AA5D48` | `ADDR_MenuFlagsVar` | `FUN_00aa5c90` | +184 |
| `0x00800480` | (unlabelled) | `FUN_00800460` | +32 |

`0x006094B0` / `0x006094F0` (menu page setup / load-teams) and `0x00B08B40`
(panel refresh) sit in islands Ghidra found no function in — the nearest
functions are 1936–3216 bytes away. These are the strongest candidates for
runtime-decrypted or packed regions.

### Semantics verified against the source's own documentation

`fn_GetTeamKitData` (`0x00865240`, 30 callers, 3 callees):
```c
puVar2 = (undefined *)FUN_00864ef0(param_1);      // GetClubTeamKitBase
...
puVar3 = puVar2 + param_2 * 0x3e;                 // stride 0x3E  ✓ matches docs
```

`fn_GetClubTeamKitBase` (`0x00864EF0`, 16 callers):
```c
iVar5 = (param_1 & 0xffff) - 0x40;
iVar2 = FUN_00866c00();
return (undefined *)(iVar2 + iVar5 * 0x220);        // base + (teamID-0x40)*0x220  ✓
```

Both match `docs/INTERNALS.md` exactly. The upstream reverse engineering is sound
and the supplied `pes6.exe` is the matching build.

### Graphics: the Direct3D 8 path, fully mapped

`pes6.exe` imports exactly one D3D symbol: `d3d8.dll!Direct3DCreate8`
(IAT `0x00B7D3A8`, import thunk `0x0042C050`). No anti-debug APIs
(no `IsDebuggerPresent` / `NtQueryInformationProcess`).

`FUN_00404c60` — the game's D3D init — decompiled:
```c
DAT_00f83fd8 = 0x280;    // 640   <-- width
DAT_00f83fdc = 0x1e0;    // 480   <-- height
DAT_00f83fe0 = 0x280;    // 640   <-- width (second copy)
DAT_00f83fe4 = 0x1e0;    // 480   <-- height (second copy)
DAT_00f83fe8 = 0x3c;     // 60
DAT_00f83ff0 = 1;        // D3DDEVTYPE_HAL
DAT_00f83fec = 2;        // D3DDEVTYPE_REF (fallback)
...
DAT_00f83eb8 = (int *)Direct3DCreate8(0xdc);       // 220 = D3D8_SDK_VERSION
iVar1 = (**(code **)(*DAT_00f83eb8 + 0x34))(DAT_00f83eb8, 0, 1, &DAT_00f83ef0);
if (iVar1 < 0) {   // HAL failed -> retry with REF
    iVar1 = (**(code **)(*DAT_00f83eb8 + 0x34))(DAT_00f83eb8, DAT_00f83fc4, 2, &DAT_00f83ef0);
}
```

Key facts:
- **vtable slot `0x34` (= index 13) is `CreateDevice`**, confirmed from the game itself.
- The presentation parameters live at `DAT_00f83ef0`; a 13-dword (52-byte)
  struct at `DAT_00f83ebc` is zeroed immediately before the call.
- The device is created with `BehaviorFlags = 0x00400000` and the pp filled in
  **afterwards** (at `0x00404D51` the pp is still zero), i.e. the game creates a
  **suspended** device and then calls `Reset` with the real backbuffer.
- The game already runs **windowed** (observed 640×480 window), and contains
  `"You cannot run the game in Windowed Mode with the current display settings."`

**⇒ The remaster hook point is the four globals** `0x00F83FD8/0x00F83FDC` and
`0x00F83FE0/0x00F83FE4` (width/height), written at the top of `FUN_00404c60`.

---

## Phase 2 — Graphics: resolution work **deferred by request**

The reverse engineering of the graphics path was completed even though the
resolution change itself was parked. What was established:

### The D3D8 path, fully mapped

`pes6.exe` imports exactly one D3D symbol: `d3d8.dll!Direct3DCreate8`
(IAT `0x00B7D3A8`, import thunk `0x0042C050`). No anti-debug APIs.

| Function | Address | Role |
|---|---|---|
| `FUN_00404C60` | `0x00404C60` | D3D init: sets the resolution globals, calls `Direct3DCreate8(220)` |
| `FUN_00405300` | `0x00405300` | builds the presentation parameters and calls `CreateDevice` |
| `FUN_00404DB0` | `0x00404DB0` | display-mode picker; walks a mode list, `CheckDeviceType` + `CheckDeviceFormat` |
| `FUN_004051D0` | `0x004051D0` | calls `SetDisplayMode` (vtable `0x24`), falls back HAL→REF |
| `FUN_00410C20` | `0x00410C20` | `CreateWindowExA` — window class `{20418978-752D-491c-AF1E-A2EDE086E000}` |
| `FUN_00411730` | `0x00411730` | window creation caller; owns the windowed/fullscreen flag |

**`IDirect3D8` vtable indices** (derived from the game's own calls, not the
DX 8.0 header, which disagrees): `0x20`=8, `0x24`=9 `SetDisplayMode`,
`0x28`=10 `CheckDeviceType`, `0x30`=12 `CheckDeviceFormat`,
`0x34`=13 caps query, `0x3C`=15 `CreateDevice`.

**`D3DPRESENT_PARAMETERS` layout** (Microsoft 56-byte, from the game's own stores):
`+0x00` BackBufferWidth, `+0x04` BackBufferHeight, `+0x0C` BackBufferCount (=2).

**Resolution globals** (written at `0x00404C83`/`0x00404C88`):
`0x00F83FD8`/`0x00F83FDC` = width/height (backbuffer),
`0x00F83FE0`/`0x00F83FE4` = width/height (copy),
`0x00F83FE8` = 60 (refresh), `0x00F83FF0` = 1 (HAL), `0x00F83FEC` = 2 (REF).

**Window size** is hardcoded separately at `0x00411798`/`0x0041179D`
(`PUSH 0x1E0` / `PUSH 0x280`). Both sites are patchable — `tools/patch_resolution.py`
does it, and the game *does* render at a higher internal resolution when patched.

### The supported way to change resolution (found, working)

The game ships `settings.exe` (Konami's own tool) with a **Display tab**:
Screen Mode (Window / Full Screen), **Resolution** (11 options, `(640,480)` …
`(1920,1080)`), Quality, Brightness, plus Online / keyboard / Player1-8 tabs.

Settings persist to:
```
C:\Users\olive\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\settings.dat
```
(420 bytes, binary; width at `+0x04`, height at `+0x06` as uint16).
**This is the supported lever — no binary patching required.**

Observed: the game falls back to **800x600** for resolutions the adapter refuses
(`SetDisplayMode` fails → the game's own fallback). 800x600 is retained per
request.

### Dead end: the `d3d8.dll` proxy — root cause

A `d3d8.dll` proxy was built and **abandoned**. Bisected with three build modes:
- mode 1 (forward `Direct3DCreate8` only, vtable untouched) — **still crashes**
- mode 2 (patch vtable, no-op trampoline) — crashes
- mode 3 (patch vtable + inspect) — crashes

So the crash is caused by the *presence of a second module named `d3d8.dll`*
in the process, not by the hook logic. Bisected away: loader-lock (deferred
`LoadLibrary`), the vtable copy overrun (fixed by patching 4 bytes in place),
the arity/stack imbalance (fixed with a tail-call trampoline). The patch route
(`patch_resolution.py`) is simpler and works, so the proxy is not needed.

---

## Phase 3 — Gameplay, AI, animation: **started**

The string table (236,988 entries) is a semantic map. Anchors resolved to
functions via `MapAnchors.java`:

| Anchor | Function | Subsystem |
|---|---|---|
| `OnSectMatchEnvInit` | `FUN_00AB6320` (514 B) | **match environment bootstrap** — initialises ~15 subsystems in sequence (`FUN_00AD31E0`, `FUN_00AD2B10`, `FUN_00A61470`…`FUN_00A616B0`) |
| `OnExecMatchEnvStepInit1` | `FUN_00A98C40` | match-environment step init; registers via `FUN_00A50550(FUN_00AED890, FUN_00AED890, name)` |
| `OnExecMatchPsetStepInit1` | `FUN_00A98E90` | match-player-set step init (same shape) |
| `formation` | `FUN_0072E400` (639 B) | **formation loader** — table at `+0x29A54`, per-slot via `FUN_0069C060(index, name)` + `FUN_006BE990` |
| `TEAM_A` / `TEAM_H` | `FUN_00752AC0` | home/away team slot; calls `FUN_00950490` = `fn_GetBoneWorldPosition` |
| `ResTeamSelectInit` | `FUN_00921DC`→ | team selection screen init |

Global state blocks identified: `0x03BE6E02/04/08` (match step state),
`0x03B2B4F0` (match env object).

Asset-format anchors in the string table: `formation_opd_00/01` (the `.opd`
format PESMod also ships a fixture for), `ball_10/12/16`, `ball_random`,
`BALL DISP`, `player_side[%s]`, `open_ml_pitch`, `bg_exhibition_player`,
`can't support texture format: %.8x` (texture loader), and the game's own
**shader** engine (`Assembly Error ... shader body`).

Many strings in `0x00BAxxxx`–`0x00BBxxxx` have **no referencing function** —
the same protected regions seen at `0x006094B0` / `0x00B08B40`. Those parts of
`.text` are not statically analysable; they need runtime capture.

---

## M1 + M2 — IMPLEMENTED AND VERIFIED

The remaster is now a working ASI. Built, deployed, and running in the game.

### `remaster.asi` (x86, 139 KB, static CRT, MinHook linked in)

```
PES6/remaster/
  remaster.cpp              DllMain: logger -> manifest -> config -> subsystems
  remaster.ini              user config
  remaster_manifest.txt     Ghidra-proven addresses + prologue signatures
  core/core.cpp             logger, config, manifest, hook registry
  graphics/graphics.cpp     resolution + window hooks
  third_party/minhook/      vendored MinHook (BSD), built statically
  build.bat
```

`remaster.log` from a live run:
```
[INFO] remaster.asi attaching (pid 9628)
[INFO] manifest loaded from ...\remaster_manifest.txt
[INFO] config loaded from ...\remaster.ini
[INFO] hook 'graphics.d3d_init' -> 0x00404C60
[INFO] hook 'graphics.create_window' -> 0x00410C20
[INFO] hooks installed: 2
[INFO] graphics: window 640x480 -> 1280x720
[INFO] graphics: resolution globals set to 1280x720
```

### Result

The game went from a blurry **800×600** upscale to the **native 1536×864 panel** —
logo, menu text, crowd and player model all render crisply. Verified by
screenshot. PESMod's 35 hooks continue to work alongside it.

### How the graphics hooks work

| Hook | Target | Effect |
|---|---|---|
| `graphics.d3d_init` | `FUN_00404C60` | runs the original, then overwrites the resolution globals `0x00F83FD8/DC` (and the `E0/E4` copies) so the later device creation picks up our size |
| `graphics.create_window` | `FUN_00410C20` | passes our client width/height to `CreateWindowExA` |

Both are in-process MinHook hooks on the game's own functions — no `d3d8.dll`
proxy, no import rewriting, no second module in the process. Signature
validation compares the target's prologue against the manifest before patching,
so a mismatched build skips the hook instead of crashing.

---

## Research findings — the prior art (2026-10-20)

Web + GitHub research paid off. `PES6/references/` now holds clones of the three
most relevant open-source projects.

### `kitserver/kitserver6` — the reference implementation

Kitserver is the standard PES modding toolkit and has supported PES6 for
years. Three findings from it:

**1. Our `pes6.exe` is the same retail build kitserver targets.** All eleven
PES6 addresses in their `afsreplace.cpp` are valid code in our executable:

| Their symbol | Address | Bytes in our build |
|---|---|---|
| `C_NEWFILEFROMAFS_CS` | `0x65B63F` | `e8 9c fe ff ff 85 c0 75 1f…` ✓ |
| `C_NEWFILEFROMAFS2_CS` | `0x8A4C36` | `8b 89 60 68 24 01 52 51 50 e8…` ✓ |
| `C_ALLOCATEBUFFERS_CS` | `0x45A607` | `e8 64 f4 ff ff 83 c4 14 85 f6…` ✓ |
| `C_AFTERFILEFROMAFS_CS` | `0x65B668` | `c3 cc cc cc … ff 25 54 8e 46 04` ✓ (import thunk) |
| `C_FILEFROMAFS_JUMPHACK` | `0x8B1983` | `e8 78 bb 01 00 89 44 24 14…` ✓ |
| `C_UNIDECODE_CS` | `0x8CD490` | `8b 44 24 10 8b 4c 24 0c 56…` ✓ |
| `C_UNPACK` | `0x65B176` | `e8 15 23 27 00 83 c4 20 6a 00…` ✓ |
| `C_UNPACK_CS` | `0x8CD469` | `e8 d2 fe ff ff 8b 4c 24 1c 03 cb…` ✓ |
| `C_MULTI_UNPACK_CS` | `0x65B152` | `e8 d9 f9 ff ff 8b 4f 08 8b 47 04…` ✓ |
| `C_READFILE_CS` | `0x44C014` | `ff 15 30 d2 b7 00 c3 90 90 90…` ✓ (import thunk) |
| `C_CLOSEHANDLE_CS` | `0x44BC6C` | `c7 46 30 00 00 00 00 e8 08 78…` ✓ |

**2. The proven asset-replacement mechanism** (their `afsreplace.cpp`): hook the
game's **own** AFS loader — `NewFileFromAFS`, `ReadFile`, `CloseHandle` — and
substitute the file stream from disk. Not a D3D-level hook. `GETFILEINFO`
carries `replaceBuf`/`replaceSize`; replacement data is swapped into the stream
before the game sees it.

**3. `apihijack.cpp` — the technique for intercepting D3D.** Kitserver
redirects the **IAT entry** for a DLL's imports (e.g. `d3d8.dll!Direct3DCreate8`)
to its own function, with the real DLL still loaded normally by the loader. This
is the proven answer to our `d3d8.dll` proxy crash: there is never a second
module named `d3d8.dll`. Implementation notes from theirs:
- walk `IMAGE_IMPORT_DESCRIPTOR`s, match on the DLL name;
- use `OriginalFirstThunk` (INT) for the import *name* and `FirstThunk` (IAT)
  for the *slot* to write;
- `VirtualProtect` the IAT (usually read-only `.rdata`) to writable;
- keep the original pointer so the detour can call through.

### AFS container format (from `afsreader.h` + `afsreader.cpp`)

```
0x00  char   sig[4]              "AFS\0"
0x04  uint32 numFiles
0x08  AFSITEMINFO[numFiles]      { uint32 offset; uint32 size; }     (8 bytes each)
..    uint32 nameTableOffset      <-- pointer to the name table
      AFSNAMEINFO[numFiles]       { char name[32]; byte other[16]; } (48 bytes each)
      file data
```
- **AFS archives nest**: an item whose data starts with `"AFS\0"` is a child
  folder; the search recurses. `dat/*.afs` is a tree.
- **The name table is encrypted** in the shipped archives, but kitserver
  **ignores it** — `afswalk.cpp` exports by index (`id_%00005d.bin`) and
  identifies files by **content signature** (`Sign()` = CRC32 of the buffer).
  So we work by index + signature, not by name.
- Entry payloads are **not** XOR-encrypted; they start with a 12-byte header
  (`u32 flags, u32 payloadSize, u32 innerSize`) — e.g. entry 100 is 414 KB
  (flags `0x00010600`), entry 0 is 2.4 KB.
- The XOR scheme in kitserver's `filedec.cpp` applies to the **`.opd` / option
  files**, not the AFS container:
  ```c
  param = 0x573;
  for (i = 0; i < fs; i++) { buf[i] ^= (param & 0xff); param = param * 0x6C078965 + 1; }
  ```
  plus an option-file variant using `DECRYPT_FACTOR2 = 0x7AB3684C` and a
  446-byte key table.

### Asset format groundwork already done upstream

`PESMod/tools/ui-bin-viewer/src/parser.js` (30 KB) is a complete parser for
PES6's "Rich" asset format: section directory, texture banks, meshes, draw
records, Uvs, colours, name tables, pointer-list descriptors. The `.opd`
fixture header is `u32 decompressedSize, u32 a, u32 b, u32 c, …`.

### Other references
- `lazanet/PES-Editor-6`, `lazanet/PES6-EXE-modifier` — PES6 data/EXE tools.
- `moth1995/AFS_PES_PSP_Converter` — independent AFS reference.
- `juce/fiveserver` — the PES5/6 **network protocol** was reverse engineered
  in Python; relevant to Phase 3.
- `dmitri-f` — is building an HD-texture plugin for PES5/PES6: direct precedent
  for M3.
- `ThirteenAG/d3d9-wrapper` — the standard widescreen wrapper (the L4 option).

---

## Stage 1 + build gate — IMPLEMENTED AND VERIFIED

The remaster is now a **single ASI** containing the upstream PESMod gameplay
layer plus the remaster layers, gated by a self-validating manifest.

### Decision: fork PESMod, don't run two ASIs

MinHook allows one hook per target. The graphics layer owns `0x00404C60` and
`0x00410C20`; the gameplay layer wants `0x00865240`, `0x00864EF0`,
`0x00862710` — and two ASIs hooking the same target means the second
`MH_CreateHook` fails silently. **One ASI must own every hook.** PESMod also
has no plugin point (its `Patcher` writes bytes, it cannot add hooks), so
"layer on top without forking" was not available.

### Layout

```
PES6/remaster/
  remaster.cpp              our DllMain: logger -> manifest -> gate -> layers
  remaster.ini              our config
  remaster_manifest.txt     GENERATED - every address + prologue signature
  core/core.cpp             logger, config, manifest, hook registry, gate
  graphics/graphics.cpp     resolution + window hooks
  src/                      upstream PESMod gameplay sources (34 files, untouched)
  third_party/MinHook/      upstream MinHook, built statically
  tools/gen_manifest.py     regenerates the manifest from the sources + exe
  build.bat
```

`remaster.asi` = 254 KB, x86, static CRT, MinHook linked in statically
(**no external DLL dependencies** — it no longer needs `MinHook.x86.dll`).

### The build gate

`tools/gen_manifest.py` scans the adopted sources for every hardcoded game
address, pairs each with the first bytes of the target's instruction stream
in *this* executable, and emits `remaster_manifest.txt`. The ASI verifies
those bytes before installing anything.

Live output from the game:
```
[INFO] gate: 118 addresses checked, 118 matched, 0 mismatched, 0 unmapped
[INFO] gate: gameplay 101 checked, 101 matched, 0 mismatched, 0 unmapped
[INFO] gameplay: upstream hooks installed
[INFO] hook 'graphics.d3d_init' -> 0x00404C60
[INFO] hook 'graphics.create_window' -> 0x00410C20
[INFO] graphics: window 640x480 -> 1280x720
[INFO] graphics: resolution globals set to 1280x720
```
And from the upstream layer's own log:
```
[Hooks] All hooks installed.
[PESMod] Initialisation complete.
  hooks installed: 35    errors: 0
```

**The game now runs at the native 1536x864 panel** instead of a blurry 800x600
upscale, with the full gameplay layer active alongside.

**What this buys:** the upstream hooks are hardcoded literals with no
validation, so on any other build they would crash. The gate turns that into
"the gameplay layer is skipped and the game still runs". That single change
makes every later phase safe to build on.

### Caveats
- The gate currently reads *code* bytes only; data addresses are recorded but
  not verified (their contents are runtime values).
- `PESMod.ini` and `remaster.ini` both exist and both are read. Unifying them
  is Stage 3 work.
- The `d3d8.dll` proxy remains parked — the IAT-redirect technique from
  kitserver's `apihijack.cpp` is the replacement when device-level hooks are
  needed.

---

## Asset pipeline — format decoded

`tools/afs_inventory.py` walks the archive tree; `tools/afs.py` is the reader.

### AFS container (confirmed against `dat/0_text.afs`, 424 MB, 9,806 entries)

```
0x00  char   sig[4]        "AFS\0"
0x04  uint32 numFiles
0x08  AFSITEMINFO[numFiles]      { uint32 offset; uint32 size; }   (8 bytes each)
..    uint32 nameTableOffset      <-- pointer to the name table
      AFSNAMEINFO[numFiles]       { char name[32]; byte other[16]; } (48 bytes each)
      file data
```
- **Archives nest**: an item whose data begins with `"AFS\0"` is a child folder.
- The **name table is encrypted** in the shipped archives — but it is not
  needed: kitserver's `afswalk.cpp` identifies files by **index**
  (`id_%05d.bin`) and by **content signature** (CRC32). `tools/afs_inventory.py`
  does the same.
- The game loads 15 language-named archives plus a standalone **`over.afs`**
  (string at `0x00B93918`, in a data table beside the float constants). Whether
  `over.afs` is an *override* the game merges is the open question — see below.

### Per-entry header

```
u16 type | u16 flags | u32 payloadSize | u32 innerSize
```
- `innerSize == 0` → the payload is stored as-is; otherwise it is the
  **decompressed** size (entry 0: payload 2,335 → inner 6,756).
- Type histogram over 9,806 entries: **`0x600` → 8,858**, `0xE00` → 423,
  `0x000` → 373, `0x080` → 34, `0x100` → 33, `0x400` → 27,
  **`0x5061` → 22** (magic `"aPDT"`).
- The 22 `aPDT` entries are a distinct container:
  `magic "aPDT", u32 flags, u32 0x800, u32 0x40, u32 dataSize, u32 1`.
  `dataSize` equals the entry size exactly, and `0x800` / `0x40` are constant
  across all 22 — a fixed-cell container.

### The full decode chain (confirmed, 10/10 sampled entries)

```
dat/0_text.afs
  -> AFS entry:  [12-byte header] [sub-header] [zlib stream]
  -> inflate
  -> PES "Rich" container, magic 0x20050420  (the 20 April 2005 build stamp)
       -> section directory of offsets
       -> texture banks, meshes, draw records, UVs, name tables
```
- The zlib stream starts at **+20** for most entries, **+84/+100** for others
  (the difference is a per-type sub-header).
- The decompressor is Konami's **unzlib 1.1.4** — a minizip build. The game
  calls it through `FUN_008CD490` → `FUN_008CD340` (`unzOpen` / `unzRead`).
- `tools/pes_asset.py` performs the whole chain and classifies each entry.

**Textures are individual AFS entries, not one packed atlas** — at least 27
entries decompress straight to raw RGBA/pixel payloads with per-entry headers.
That means one-for-one replacement is structurally possible.

### The per-texture header (partially decoded)

A type-`0x600` entry's sub-header (between the 12-byte entry header and the
zlib stream) runs to 84 or 100 bytes and ends with fields that look like
dimensions and format, e.g. for one entry:
```
06 00 00 00   08 00 00 00   20 00 00 00   f0 01 00 00   00 03 00 00 ...
```
with `0x300` (768) and `0x1F0` (496) in the tail — plausible width/height.

**Still open:** the exact field semantics, and whether the width/height live
in the sub-header or in the decompressed payload.

### The naming question — the AFS name table is a dead end

The encrypted AFS name table holds the file names, and they are **not** present
inside the decompressed payload either: a decompressed entry contains no
readable strings at all (a scan of a 6,756-byte entry found 56 "strings",
every one of them coincidental byte runs, and the structures at the header
offsets are u16/u32 fields and float32 vertex data, not text).

Note also that the `.opd` UI-bin files that PESMod's `parser.js` parses are a
**different format** from the AFS game-asset container — `parser.js` expects a
root *pointer* at word 0, whereas AFS entries start with the magic
`0x20050420` and a table of offsets. The magic is the 20 April 2005 build
stamp, and the offset table points at sections containing u16/u32 fields and
float32 vertex data.

**So the name → id mapping must come from outside the file.** Two routes:

1. **The community "PES 6 Text and Logo Mapping Guide"**, which documents PES6
   asset *file names and block numbers* (balls, referees, faces, kits,
   backgrounds). This is the authorable source of truth.
2. **Our own classifier** — identify each texture entry by dimensions plus a
   CRC32 content signature (kitserver's `Sign()`), and ship a pack that
   targets entry ids. Less friendly to hand-authoring, entirely under our
   control, and it doubles as a regression check.

Route 2 is implementable immediately; route 1 makes packs authorable by name.
Doing both gives a pack format that is human-friendly *and* verifiable.

### Game-side AFS entry points (all verified as code in this build)

| Address | Role |
|---|---|
| `0x0065B4E0` | `NewFileFromAFS` — the AFS request entry (an indirect thunk) |
| `0x00459A70` | `AllocateBuffers` |
| `0x008CD500` | `FileFromAFS` |
| `0x008CD490` | `unpack` wrapper |
| `0x008CD340` | the unzlib decompressor |
| `0x008CD469` → `0x008CD340` | multi-unpack call site |
| `0x0044C014` | `ReadFile` import thunk |
| `0x0044BC6C` | `CloseHandle` call site |
| `0x00415310` | AFS existence check (builds `<baseDir>%s` + name) |
| `0x00410E50` | game pre-flight: CD-ROM → install → AFS → single-instance mutex |

### Open question that decides the texture strategy

**Resolved: there is no game-folder override.** `FUN_00415310` builds each
archive path as `"%s%s"` (`DAT_00B7E0E8`) — base directory (from the registry
`installdir`) + archive name — then `GetFileAttributesA`. The language table
is:

| lang | text | sound |
|---|---|---|
| -1 | `0_text.afs` | `0_sound.afs` |
| 0 | `e_text.afs` | `e_sound.afs` |
| 1 | `f_text.afs` | `f_sound.afs` |
| 2 | `g_text.afs` | `g_sound.afs` |
| 3 | `i_text.afs` | `i_sound.afs` |
| 4 | `s_text.afs` | `s_sound.afs` |
| 9 | `p_text.afs` | `p_sound.afs` |

So the archives are only ever read from `<installdir>\dat\`, and **asset
replacement must go through a hook** — kitserver's mechanism, at addresses
verified in this build.

**RESOLVED.** The per-texture header layout is now known (see *M3 — Asset
pipeline* below), and so is the mechanism the game uses to turn an asset into
a D3D texture. The short version: the archive itself is integrity checked and
cannot be edited; the substitution has to happen after inflation, and the
game's texture sizes are derived from the asset by code at a known address.

### Prior art for the asset side
- **"PES 6 Text and Logo Mapping Guide"** — a community document listing
  PES6 asset **file names and block numbers** (balls, referees, faces, kits,
  backgrounds). This is the name→index map the encrypted table denies us.
- **"PES Ultimate Data Explorer 2.9.7"** — exports textures from `.bin` files.
- **"Big PES 6 PC TOOLS PACK"** (soccergaming, 2006) — the classic PES6
  editing tool pack: graphics, kitserver, import/export.
- **ENB-based HD packs** for PES6 exist (virtual-file override of the .afs).
- `dmitri-f` — building an HD-texture plugin for PES5/PES6.

---

## M3 — Asset pipeline: IMPLEMENTED AND VERIFIED

The remaster can now replace the game's textures and UI art. The mechanism
was chosen from evidence, not assumption, and the result is verified on screen.

### The finding that shaped the design

The archive is **integrity checked**. Three separate experiments, each with the
original restored in between:

| Experiment | Result |
|---|---|
| Rebuild the whole archive with new entry offsets and the name table moved to the end | game crashes on startup (`0xC0000005`) |
| Replace one entry's payload in place, same offset, sizes updated | game crashes on startup |
| Flip **one bit** in **one entry's** 12-byte header (`flags` word at `0x00114800`) | game crashes on startup |

Relocating only the name table, changing nothing else, is fine — so the
container is sound, but the per-entry headers are checked. **Editing
`dat/*.afs` is not an option.** Replacement therefore has to happen after the
game inflates an entry and before it hands the data to the renderer, which is
precisely where kitserver hooks.

### The hook

`FUN_008CD490` (`0x008CD490`) is the inflate routine. Its signature was read
off the game's own call site at `0x0065B176`, which pushes 8 dwords and the
caller cleans all 32 bytes, so it is `__cdecl` with five arguments:

```c
int unpack(void* unused, void* dest, void* src, void* arg4, int* sizeOut);
```

After the original returns, `dest` holds the inflated asset and `*sizeOut` its
length. The remaster computes a CRC32 over those bytes and, if the signature
is in the pack, copies the replacement over the buffer.

The hook is **self-validating**: if the CRC does not match, nothing is
touched. A wrong guess cannot corrupt the game, it just does nothing.

### Proof that the identification is right

With `log_all = 1` the runtime log records every inflation, and those CRCs were
matched against an offline index built from the archives
(`tools/build_asset_index.py`):

| Archive | Entries | Distinct signatures |
|---|---|---|
| `0_text.afs` | 9,806 | 6,788 (1,626 shared) |
| `0_sound.afs` | 421 | 360 |
| `e_text.afs` | 421 | 360 |
| `e_sound.afs` | 13,127 | 0 (different layout — see below) |

124 of the first 172 runtime inflations resolved to an exact AFS entry id,
**every one of them with the size matching byte for byte**. The remaining 48
come from `e_sound.afs`, whose entries use a layout this tooling does not yet
decode (all 13,127 entries are type `0x0080`, 72 bytes, with no inflation).

### Proof that the replacement reaches the screen

92 textures that the game loads during boot were replaced with solid magenta.
The main menu, which is normally a full-screen golden streak animation, came
up **solid black** instead — the substituted pixels were being rendered.

A control run is what makes this conclusive: the *same* build without a pack
differs from itself by 2.48% of pixels in exactly one region (the blinking
"PRESS ANY BUTTON" text), and the packed build differed by the same 2.48% in
the same region. Without that control the difference would have been
attributed to the replacement. It was not.

### The D3D8 device proxy

`graphics/d3d8.cpp` gives the device its own copy of the method table with
`CreateTexture` replaced, so the remaster can see every texture the game
creates.

Two details cost real time and are worth recording:

* **The game keeps its own copy of the D3D8 vtable in its own heap**
  (observed at `0x0DD40760`, right after the device object at `0x0DD40600`).
  Substituting a private copy of the table makes the game call through a
  table the device was never built with and it crashes on the next call. The
  working approach is to **patch the game's own table in place**.
* The first attempt to validate the load with a separate `LoadLibrary` test
  harness reported error 1114. That was the harness, not the ASI: an upstream
  static initialiser reads a game global at `0x00B84970`, which is mapped in
  `pes6.exe` but not in the test program. **An ASI that touches game memory
  cannot be validated outside the game.**

### The game's texture path, fully mapped

| Address | What it is |
|---|---|
| `0x00F83EB8` | the `IDirect3D8` object |
| `0x00F83E68` | the `IDirect3DDevice8` pointer |
| `0x00F83EBC` | `D3DPRESENT_PARAMETERS` (+0x00 width, +0x04 height, +0x0C back buffer format) |
| `0x00405300` | the game's device-creation routine |
| `0x00410C20` | the `CreateWindowExA` wrapper (window size) |
| `0x011631F8` | texture **width** |
| `0x011631FC` | texture **height** |
| `0x00874A20` | `setTextureSize(width, height)` — writes both globals |
| `0x00875020` | creates the texture(s) for the current size |
| `0x0040E850` | fills the texture descriptor and calls the creator |
| `0x0040E700` | `__thiscall`; calls `IDirect3DDevice8::CreateTexture` |

The texture descriptor object holds `+0x14` width, `+0x18` height, `+0x1C`
levels, `+0x20` usage, `+0x24` format.

`FUN_00875270` sets the default size to **1024 × 512**, format `0x15` =
`D3DFMT_A8R8G8B8`. `FUN_00874FA0` **halves the width or the height and
retries** when texture creation fails with `D3DERR_OUTOFVIDEOMEMORY` or
`D3DERR_INVALIDCALL`, down to a floor of 256. That is the game's own
resolution ladder, and it is where a higher resolution would be raised.

### The vtable indices, cross-checked against the game's own calls

Taken from the DirectX 8.1 SDK header in `references/kitserver6/src/soft/dxsdk81/include/d3d8.h`
and each confirmed against a call site in the game:

| Interface | Index | Offset | Method | Evidence |
|---|---|---|---|---|
| `IDirect3D8` | 9 | `0x24` | `CheckDeviceType` | 5 arguments at `0x00403C10` |
| `IDirect3D8` | 13 | `0x34` | `GetDeviceCaps` | 3 arguments at `0x00404D5D` |
| `IDirect3D8` | 15 | `0x3C` | `CreateDevice` | 6 arguments at `0x00405589` |
| `IDirect3DDevice8` | 20 | `0x50` | `CreateTexture` | — |
| `IDirect3DDevice8` | 25 | `0x64` | `CreateRenderTarget` | 6 arguments at `0x0040E700` |
| `IDirect3DDevice8` | 36 | `0x90` | `Clear` | 6 arguments at `0x004056A4` |

### Every texture the game creates

All of them are `D3DFMT_A8R8G8B8`, one mip level, and either
`D3DPOOL_DEFAULT` + `D3DUSAGE_DYNAMIC` or `D3DPOOL_MANAGED`:

```
1024x512   512x512   512x256   256x256   256x128   128x128
256x64     128x64     64x128     64x64      32x32      8x8
2x2        1x256
```

### The asset layout

```
AFS entry
  +0x00  u16 type          0x600 textures, 0xE00 UI atlases, 0x080/0x100/0x400 misc
  +0x02  u16 flags
  +0x04  u32 payloadSize   length of the zlib stream
  +0x08  u32 innerSize     type dependent; 0 for 0x600
  +0x0C  20-byte sub-header
  +0x20  zlib stream
```

After inflation a texture is either:

* **raw pixels** — a 16-byte sub-header (`u32 flags`, `u32 8`, `u32 0x10`,
  `u32 0`) then `width * height * 4` bytes of `A8R8G8B8`; or
* **a "Rich" container** — a 20-byte sub-header whose last word is the build
  stamp `0x20050420` (20 April 2005), then a table of offsets into the inflated
  buffer. The pixels live in the section the table points at.

### What the pack format is, and why

See `docs/ASSETS.md`. In short: a folder `remaster_pack/` with an
`index.txt` of `CRC32 → filename` and the inflated replacements beside it.
The CRC32 is of the **original** inflated bytes, so the key is stable and does
not depend on the encrypted AFS name table.

### Remaining limitation, stated plainly

A replacement must be the **same size** as the original, because the game
derives the texture size from the asset and the archive cannot be edited.
Higher resolution therefore needs the D3D8 texture to be created larger
*and* the pixels re-uploaded through the surface. The addresses for both are
now known; the work left is to proxy `IDirect3DSurface8::Lock` and resample.
That is the next milestone, not a finished feature.

---


| Path | Purpose |
|---|---|
| `PES6-Remaster/pesmod/` | upstream clone (builds, deploys) |
| `PES6-Remaster/docs/ASSETS.md` | the replacement pack format and how to build one |
| `PES6-Remaster/tools/build_asset_index.py` | builds the CRC32 signature index for an AFS |
| `PES6-Remaster/tools/make_pack.py` | builds a replacement pack from an AFS entry |
| `PES6-Remaster/tools/afs_write.py` | rebuilds an AFS archive (reference only: the game rejects the result) |
| `PES6-Remaster/tools/afs_patch_entry.py` | patches one AFS entry in place (reference only: the game rejects the result) |
| `PES6-Remaster/tools/pes_asset.py` | AFS → zlib → Rich extraction and classification |
| `PES6-Remaster/tools/afs_inventory.py`, `tools/afs.py`, `tools/pes_uibin.py`, `tools/find_addr_refs.py` | asset enumeration and byte-pattern xref finding |
| `PES6-Remaster/tools/index_*.tsv` | generated signature indexes |
| `PES6-Remaster/tools/verify_addresses.py` | PE parser + address verifier; `--dump VA...` |
| `PES6-Remaster/tools/patch_resolution.py` | resolution patcher (4 immediates, both backbuffer + window) |
| `PES6-Remaster/tools/find_d3d8_callsite.py` | finds the D3D import thunk and its callers |
| `PES6-Remaster/tools/targets.txt` | 130 labelled target addresses |
| `PES6-Remaster/evidence/PESMod-phase0.log` | Phase 0 proof (35 hooks installed) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\pes6_targets.json` | 130 targets + decompilation |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\pes6_functions.tsv` | 34,578 functions |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\pes6_strings.tsv` | 236,988 strings |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\phase3_anchors_out.txt` | gameplay anchors → functions |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\pass2_out.txt` | 20 hand-picked functions decompiled |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\DumpTargetFunctions.java` | Ghidra post-script (target dump) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\DecompileAddresses.java` | Ghidra post-script (address list) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\DumpFunctionListing.java` | Ghidra post-script (raw listing) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\FindGlobalRefs.java` | Ghidra post-script (data xrefs) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\FindImportCallers.java` | Ghidra post-script (import callers) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\MapAnchors.java` | Ghidra post-script (string → function) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\dx8\` | d3d8 proxy sources + ABI probes (parked) |
| `C:\Users\olive\Downloads\PES6-Remaster\tools\rea\shot.ps1`, `press.ps1` | window capture and key input for verification runs |

---

## M4 — Higher resolution: what works, what does not, and why

The texture pipeline was taken to a working implementation and then measured,
because the first plausible approach turned out to be wrong. The result is
worth recording in full.

### The pipeline that was built

`graphics/d3d8.cpp` now watches every texture the game creates:

* the device vtable is patched in place, so `CreateTexture` is seen
  (slot 20, `IDirect3DDevice8`);
* every `D3DPOOL_MANAGED` texture is recorded, and the texture vtable is
  patched so `GetLevelDesc`, `GetSurfaceLevel`, `LockRect` and `UnlockRect`
  are seen (slots 14, 15, 16, 17);
* the game's upload can be diverted into a scratch buffer, so the real
  surface can be rewritten afterwards;
* the uploaded pixels can be read back and reported with a CRC32.

### What the measurements said

Three experiments were run before the working combination was found:

| Experiment | Result |
|---|---|
| Enlarge the texture, change nothing | the artwork appears in the top-left of the screen at **half** the size it should be |
| Enlarge the texture and fill it with a bilinear upscale of the game's own art | the artwork appears at **a quarter** of the size |
| Enlarge the texture and report the original size from `GetLevelDesc` | the game **never calls `GetLevelDesc`** |

The first two results looked like "the game's rendering is tied to the size
of the texture it asked for". That reading was wrong, and the third
experiment is what showed it.

**What is actually happening:** the game's artwork is uploaded into the
*top-left corner* of the enlarged texture, and the rest of the texture is
empty. The picture is then stretched over the screen, so only the corner of
the screen has any artwork in it — which reads as "the artwork is drawn at
the wrong size". The fix is not to change the geometry but to **fill the
whole texture**: the pack's artwork, or the game's own artwork filtered up
into the larger texture. With that done the enlarged texture renders
correctly, and with the pack's artwork at four times the texture size the
texture ends up with twice the detail of the original.

`GetLevelDesc` is never called by the game, so it is not the source of its
geometry, and reporting the original size through it does nothing.

**So `texture_scale = 2` ships as the default**, and the pack supplies the
artwork for the enlarged textures.

### The route that works

Higher resolution art goes through the pack:

```
remaster_pack/
    hires.txt        one "<width>x<height> <file>" line per texture size
    <file>           the artwork, PNG or raw
```

The size in the index is the size of the *texture* the artwork replaces, not
the size of the artwork. The artwork may be any size and is filtered down
into the texture with an area average, which is the right filter when
shrinking: every source pixel contributes to the destination pixel it falls
in. The raw format carries its own size so the artwork can be larger than the
texture — `"RAW1"`, width, height, then B,G,R,A pixels.

`tools/make_hires.py` builds these entries from PNGs, and lists the texture
sizes the game actually uses.

### Verified

With the pack's artwork in place and `texture_scale = 2`, the title screen
shows the supplied artwork across the whole screen, with the texture twice
the size the game asked for. Without a pack the same setting still renders
correctly and filters the game's own artwork up into the larger texture.

### The texture inventory

Every texture the game creates is `D3DFMT_A8R8G8B8`, one mip level, and
either `D3DPOOL_DEFAULT` + `D3DUSAGE_DYNAMIC` (the 3D working buffers and
render targets) or `D3DPOOL_MANAGED` (the artwork). The managed sizes seen
so far:

```
512x512   512x256   256x256   256x128   128x128   256x64
128x64     64x128    64x64      32x32      8x8        2x2
```

The working buffers are `1024x512`, `512x256`, `256x128` and so on.

### Two bugs worth remembering

* The texture vtable is **shared** by every texture. Capturing the "original"
  function pointers on each creation captures this file's own detours the
  second time round, and the game dies with a stack overflow. The originals
  are captured once.
* D3D8 surfaces can be **bottom-up**, in which case `LockRect` reports a
  negative pitch and the rows run backwards from `pBits`. The row offset has
  to be computed as a signed value; mixing `size_t` into it turns the
  pointer into nonsense and the crash lands inside the remaster, not the
  game.

### The sound archive

`e_sound.afs` turned out to have a **plaintext** name table
(`unknow_00000.adx`, `unknow_00001.adx`, ...), unlike `0_text.afs` whose
names are encrypted. Its entries are not zlib, though: every entry is type
`0x0080` with a constant eight byte tag and no inflation, so its payload
format still needs its own decode. It supplies sound, not artwork.

---

## Environment (verified)

| Component | Version | Location |
|---|---|---|
| Node.js | 24.18.0 | — |
| REA `rea-agents` | 6.7.0 (latest) | global npm |
| Ghidra | 12.1.4 PUBLIC | `C:\Users\olive\tools\ghidra_12.1.4_PUBLIC` |
| Temurin JDK | 21.0.12.1 LTS 64-bit | `C:\Users\olive\tools\jdk-21` |
| MSVC | 19.44 / 14.44.35207 | `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools` |
| CMake / Ninja | 3.31.6 / bundled | VS `CommonExtensions\Microsoft\CMake` |
| Windows SDK | 10.0.26100.0 | via VS workload |

`rea doctor --json` → `healthy: true`, `environment_healthy: true`.
REA MCP registered for 9 clients, all `aligned`; skill v42 / 207 tools.

> `rea setup` strips OpenCode's `mcp.rea.timeout` (default 5000 ms). Current
> value `1800000`. Re-add after any future `rea setup`.

---

## Remaining phases

| Phase | Status |
|---|---|
| 0 — runtime gate | ✅ passed |
| 1 — static analysis | ✅ complete |
| 2 — graphics | ✅ running at the native 1536×864 panel (windowed); the supported resolution lever and the D3D8 path are both mapped |
| 3 — gameplay / AI / animation | 🔄 upstream PESMod layer running (101 hook targets verified against the build) |
| 4 — assets | ✅ **replacement pipeline working and verified on screen**; 9,806-entry signature index built; the archive itself is integrity checked and cannot be edited |
| 5 — packaging | 🔄 single `remaster.asi` (257 KB, static CRT, MinHook linked in) + `remaster.ini` + `remaster_manifest.txt` |
| 6 — high-resolution textures | ✅ **working** — the pack supplies artwork at any resolution, filtered into the texture; `texture_scale = 2` ships as the default (M4) |
| 7 — `e_sound.afs` layout (13,127 entries, none inflate) | 🔄 its name table is plaintext, but the payload is not zlib; needs its own decode |

