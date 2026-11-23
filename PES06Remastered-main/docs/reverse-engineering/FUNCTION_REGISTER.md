# Reverse Engineering Evidence Register: PES 6 Function & Subsystem Map

**Target Executable:** `pes6.exe`  
**SHA-256:** `649dc2fc95dfa8502a24c13bb746a0a330aa0236cb9c6ce5b5d828ccae448144`  
**PE Machine:** x86 32-bit (Image Base: `0x00400000`)  
**Analysis Providers:** Ghidra 12.2 Headless Analyzer & REA (Reverse Engineer Anything)  
**Total Registered Targets:** 140  
**Integrity Verification:** 129 CODE entries, 1 DATA entry verified against binary image  

---

## 1. Classification & Methodology

Each function entry is classified based on evidence rigor:
- **VERIFIED:** Resolved by both Ghidra decompiler and runtime binary scanner; hook or call sites observed.
- **PROBABLE:** Function entry and signature verified by decompiler, caller/callee graph established, pending runtime hook test.
- **HYPOTHESIS:** Address lands within code boundaries with plausible instructions; exact runtime role under active investigation.
- **UNKNOWN:** Data pointer, stub, or unresolved thunk.

---

## 2. Subsystem Overview

| Subsystem | Function Count | Primary Source Files | Remaster Hook / Role |
|---|---|---|---|
| Graphics & Direct3D | 6 | `graphics.cpp`, `d3d8.cpp`, `hires.cpp` | Resolution globals, window creation, D3D8 proxy |
| AFS & Asset I/O | 3 | `assets.cpp`, `fileio.cpp` | Transactional AFS asset replacement, unzlib detour |
| Kit & Team Data | 47 | `club_hooks_kit_data.cpp`, `custom_team_loader.cpp` | 2D/3D Kit structures, custom team registration |
| Player & Formation Management | 16 | `player_hooks.cpp`, `club_hooks_squad_data.cpp` | Player attributes, formation loading, weather |
| UI, Menus & Presentation | 34 | `club_hooks_menu.cpp`, `club_hooks_panel.cpp` | Navigation, layout scaling, badges, menu panels |
| Scene Graph & Skeletal Bones | 17 | Upstream gameplay layer | 3D player skeleton, bone hierarchy, node rendering |
| Engine Memory & Utility | 9 | `core.cpp`, `patcher.cpp` | Custom allocator/deallocator hooks, string tables |
| Other / Unclassified | 8 | Various | Auxiliary engine subroutines |

---

## 3. Detailed Function Registry

### 3.1 Subsystem: Graphics & Direct3D

#### `0x00404C60` — `d3d.init`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `d3d.init`
- **Signature:** `manifest: d3d.init`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** D3D initialization and resolution globals setter
```c
// Manifest entry d3d.init at 0x00404C60
// See remaster_manifest.txt
```

#### `0x00405300` — `d3d.createdevice`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `d3d.createdevice`
- **Signature:** `manifest: d3d.createdevice`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** D3D device creation caller
```c
// Manifest entry d3d.createdevice at 0x00405300
// See remaster_manifest.txt
```

#### `0x00410C20` — `d3d.createwindow`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `d3d.createwindow`
- **Signature:** `manifest: d3d.createwindow`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** CreateWindowExA wrapper with client width/height
```c
// Manifest entry d3d.createwindow at 0x00410C20
// See remaster_manifest.txt
```

#### `0x00874A20` — `tex.set_size`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `tex.set_size`
- **Signature:** `manifest: tex.set_size`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** Sets texture width and height globals
```c
// Manifest entry tex.set_size at 0x00874A20
// See remaster_manifest.txt
```

#### `0x009509B0` — `fn_SetNodeTexture`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `FUN_009509b0`
- **Signature:** `undefined FUN_009509b0(void)`
- **Binary Size:** 72 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `85` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009509b0(int param_1,undefined4 param_2)

{
  int iVar1;
  
  if ((param_1 != 0) && (iVar1 = FUN_00952770(*(undefined4 *)(param_1 + 0x2c),param_2), iVar1 != 0))
  {
    *(undefined4 *)(param_1 + 0x44) = param_2;
    *(int *)(param_1 + 0x48) = iVar1;
    FUN_00950260();
    *(undefined1 *)(param_1 + 0xa4) = 0;
    *(undefined1 *)(param_1 + 0xa3) = 0;
    *(undefined1 *)(param_1 + 0xa2) = 0xff;
  }
  return;
}
```

#### `0x009C39C0` — `fn_SetNavContext`
- **Subsystem:** Graphics & Direct3D
- **Function Symbol:** `FUN_009c39c0`
- **Signature:** `undefined FUN_009c39c0(void)`
- **Binary Size:** 1208 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `32` | Callees: `19`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009c39c0(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined *local_2c;
  undefined2 local_28;
  undefined2 local_26;
  short local_24;
  short local_22;
  float local_20;
  float local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  float local_8;
  float local_4;
  
  local_20 = (float)-((int)DAT_03becc84 / 2);
  local_2c = (undefined *)-((int)DAT_03becc86 / 2);
  local_1c = (float)(int)local_2c;
  local_18 = 0;
  local_14 = 0;
  // ... [truncated for register brevity] ...
```

### 3.2 Subsystem: AFS & Asset I/O

#### `0x0065B176` — `afs.unpack_cs`
- **Subsystem:** AFS & Asset I/O
- **Function Symbol:** `afs.unpack_cs`
- **Signature:** `manifest: afs.unpack_cs`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** AFS unpack call site invoking inflate
```c
// Manifest entry afs.unpack_cs at 0x0065B176
// See remaster_manifest.txt
```

#### `0x0065B63F` — `afs.newfilefromafs`
- **Subsystem:** AFS & Asset I/O
- **Function Symbol:** `afs.newfilefromafs`
- **Signature:** `manifest: afs.newfilefromafs`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** Core AFS container archive stream loader
```c
// Manifest entry afs.newfilefromafs at 0x0065B63F
// See remaster_manifest.txt
```

#### `0x008CD490` — `afs.unpack`
- **Subsystem:** AFS & Asset I/O
- **Function Symbol:** `afs.unpack`
- **Signature:** `manifest: afs.unpack`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** unzlib inflate routine - asset replacement hook
```c
// Manifest entry afs.unpack at 0x008CD490
// See remaster_manifest.txt
```

### 3.3 Subsystem: Kit & Team Data

#### `0x005FC1E0` — `ADDR_CheckTeamUnlocked`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_005fc127`
- **Signature:** `undefined FUN_005fc127(void)`
- **Binary Size:** 890 bytes (Exact Entry: `False`)
- **Call Relationships:** Callers: `1` | Callees: `9`
- **Confidence Level:** `PROBABLE`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_005fc127(void)

{
  short sVar1;
  float fVar2;
  char cVar3;
  ushort uVar4;
  uint uVar5;
  int iVar6;
  undefined4 uVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  float10 fVar11;
  float local_10;
  int local_c;
  
  cVar3 = FUN_0048fee0();
  if (cVar3 == -1) {
    uVar5 = 1;
  }
  else {
    uVar5 = FUN_0048fee0(DAT_03be09cd);
    uVar5 = (uVar5 & 0xff) % 0x17;
  }
  // ... [truncated for register brevity] ...
```

#### `0x006094F0` — `fn_menu_LoadPageTeams`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `fn_menu_LoadPageTeams`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

#### `0x00861640` — `fn_GetLeagueString`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00861640`
- **Signature:** `undefined FUN_00861640(void)`
- **Binary Size:** 79 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `36` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00861640(int param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_00866b80();
  if (param_1 < 0x1d) {
    iVar1 = iVar1 + param_1 * 0x54;
  }
  iVar2 = FUN_00866b80();
  if (*(char *)(iVar2 + param_1 * 0x54 + 0x53) == '\0') {
    if (param_1 < 0x1d) {
      iVar1 = FUN_0094f2c0(*(undefined4 *)(&DAT_00c96dec + param_1 * 8));
      return iVar1;
    }
    return 0;
  }
  return iVar1 + 0x15;
}
```

#### `0x00861AD0` — `orig_SetTeamPlayerAttr`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00861ad0`
- **Signature:** `undefined FUN_00861ad0(void)`
- **Binary Size:** 31 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `17` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00861ad0(undefined4 param_1,undefined4 param_2,undefined2 param_3)

{
  undefined2 *puVar1;
  
  puVar1 = (undefined2 *)FUN_00861970(param_1,param_2);
  if (puVar1 != (undefined2 *)0x0) {
    *puVar1 = param_3;
  }
  return;
}
```

#### `0x00861D20` — `orig_GetTeamPlayerID (public wrapper)`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00861d20`
- **Signature:** `undefined FUN_00861d20(void)`
- **Binary Size:** 23 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `8` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00861d20(void)

{
  int iVar1;
  
  iVar1 = FUN_00861b90();
  if (iVar1 == 0) {
    FUN_00866b90();
    return;
  }
  return;
}
```

#### `0x00862200` — `orig_GetTeamPlayerAttr`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00862200`
- **Signature:** `undefined FUN_00862200(void)`
- **Binary Size:** 212 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `6`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00862200(uint param_1,uint param_2,int param_3,undefined4 param_4)

{
  int iVar1;
  ushort uVar2;
  
  if ((param_3 != 0) && (param_3 != 1)) {
    param_3 = 0;
  }
  uVar2 = (ushort)param_1;
  if ((uVar2 < 0x40) || (0xcb < uVar2)) {
    if ((uVar2 == 0x126) || (uVar2 == 0x127)) {
      iVar1 = FUN_00866d50(param_1);
      *(undefined1 *)(iVar1 + 0x1174 + (param_2 & 0xff)) = (undefined1)param_4;
    }
    else {
      FUN_00861850();
      if ((uVar2 < 0x49) && ((byte)param_2 < 0x17)) {
        iVar1 = FUN_00866bb0();
        *(undefined1 *)(iVar1 + (param_1 & 0xffff) * 0x17 + (param_2 & 0xff)) = (undefined1)param_4;
        return;
      }
    }
  }
  else if ((byte)param_2 < 0x20) {
  // ... [truncated for register brevity] ...
```

#### `0x00862710` — `orig_GetTeamPlayerID`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00862710`
- **Signature:** `undefined FUN_00862710(void)`
- **Binary Size:** 536 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `50` | Callees: `10`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
uint FUN_00862710(int param_1,uint param_2,int param_3)

{
  ushort uVar1;
  ushort *puVar2;
  int iVar3;
  uint uVar4;
  byte bVar5;
  ushort uVar6;
  
  if (((param_3 != 0) && (param_3 != 1)) && (param_3 != 2)) {
    param_3 = 0;
  }
  iVar3 = thunk_FUN_00425198();
  uVar6 = (ushort)param_1;
  if (iVar3 == 2) {
    if ((0xfd < uVar6) && (uVar6 < 0x10e)) {
      uVar4 = FUN_0067cea0(param_1,param_2,param_3);
      return uVar4;
    }
  }
  else {
    iVar3 = thunk_FUN_00425198();
    if (iVar3 == 1) {
      if ((0xcb < uVar6) && (uVar6 < 0xdd)) {
  // ... [truncated for register brevity] ...
```

#### `0x00863570` — `orig_BuildLeaguePanelVisualSlots`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00863570`
- **Signature:** `undefined FUN_00863570(void)`
- **Binary Size:** 234 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00863570(uint param_1,int param_2,int param_3)

{
  undefined *puVar1;
  int *piVar2;
  uint uVar3;
  ushort uVar4;
  
  while( true ) {
    while (((uVar4 = (ushort)param_1, param_3 == 0 && (0x3f < uVar4)) && (uVar4 < 0xcc))) {
      puVar1 = (undefined *)FUN_00862fc0(param_1);
      if (puVar1[0x38] != '\0') goto LAB_0086364d;
      param_3 = 1;
    }
    if ((uVar4 != 0x126) && (uVar4 != 0x127)) {
      if (0x127 < uVar4) {
        return &DAT_00b9885c;
      }
      if (param_2 == 0) {
        uVar3 = 0;
        do {
          if (uVar4 == (&DAT_00c96f30)[uVar3 * 4]) {
            puVar1 = (undefined *)FUN_0094f2c0((&DAT_00c96f34)[uVar3 * 2]);
            return puVar1;
          }
  // ... [truncated for register brevity] ...
```

#### `0x008646B0` — `fn_ResolveTeamID`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_008646b0`
- **Signature:** `undefined FUN_008646b0(void)`
- **Binary Size:** 46 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `63` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
short FUN_008646b0(undefined4 param_1)

{
  int iVar1;
  short sVar2;
  
  iVar1 = FUN_00866d50(param_1);
  sVar2 = (short)param_1;
  if (((sVar2 == 0x126) || (sVar2 == 0x127)) && (iVar1 != 0)) {
    return *(short *)(iVar1 + 0x170);
  }
  return sVar2;
}
```

#### `0x00864EF0` — `fn_GetClubTeamKitBase`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00864ef0`
- **Signature:** `undefined FUN_00864ef0(void)`
- **Binary Size:** 179 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `16` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00864ef0(uint param_1)

{
  int *piVar1;
  int iVar2;
  undefined *puVar3;
  ushort uVar4;
  int iVar5;
  
  uVar4 = (ushort)param_1;
  if (((uVar4 == 0x126) || (uVar4 == 0x127)) &&
     (piVar1 = (int *)FUN_00866d50(param_1), piVar1 != (int *)0x0)) {
    FUN_00866d80(param_1);
    if ((undefined *)(*piVar1 + 0x58) != (undefined *)0x0) {
      return (undefined *)(*piVar1 + 0x58);
    }
  }
  if (((0x3f < uVar4) && (uVar4 < 0xcc)) && (iVar5 = (param_1 & 0xffff) - 0x40, iVar5 != -1)) {
    iVar2 = FUN_00866c00();
    return (undefined *)(iVar2 + iVar5 * 0x220);
  }
  if (((0xdc < uVar4) && (uVar4 < 0xfe)) && (iVar5 = (param_1 & 0xffff) - 0xdd, iVar5 != -1)) {
    iVar2 = FUN_00866ed0();
    return (undefined *)(iVar2 + iVar5 * 0x220);
  }
  // ... [truncated for register brevity] ...
```

#### `0x00865100` — `fn_GetNationalTeamKitBase`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00865100`
- **Signature:** `undefined FUN_00865100(void)`
- **Binary Size:** 160 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `12` | Callees: `5`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00865100(uint param_1)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  undefined *puVar4;
  ushort uVar5;
  
  uVar5 = (ushort)param_1;
  if (((uVar5 == 0x126) || (uVar5 == 0x127)) &&
     (piVar1 = (int *)FUN_00866d50(param_1), piVar1 != (int *)0x0)) {
    FUN_00866d80(param_1);
    if ((undefined *)(*piVar1 + 0x58) != (undefined *)0x0) {
      return (undefined *)(*piVar1 + 0x58);
    }
  }
  if (((uVar5 < 0x39) || ((0x38 < uVar5 && (uVar5 < 0x40)))) && ((param_1 & 0xffff) != 0xffffffff))
  {
    iVar2 = FUN_00866bf0();
    return (undefined *)(iVar2 + (param_1 & 0xffff) * 0x160);
  }
  if (((0xcb < uVar5) && (uVar5 < 0xdd)) && (iVar2 = (param_1 & 0xffff) - 0xcc, iVar2 != -1)) {
    iVar3 = FUN_00866ec0();
    return (undefined *)(iVar3 + iVar2 * 0x160);
  // ... [truncated for register brevity] ...
```

#### `0x00865240` — `fn_GetTeamKitData`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00865240`
- **Signature:** `undefined FUN_00865240(void)`
- **Binary Size:** 173 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `30` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00865240(uint param_1,int param_2)

{
  int iVar1;
  undefined *puVar2;
  undefined *puVar3;
  ushort uVar4;
  
  puVar3 = (undefined *)0x0;
  if (param_2 < 0) {
    return (undefined *)0x0;
  }
  iVar1 = FUN_00866d80(param_1);
  uVar4 = (ushort)param_1;
  if (((uVar4 < 0xfe) || (0x10d < uVar4)) && (iVar1 == 3)) {
    param_1 = param_1 & 0xffff;
    puVar2 = (undefined *)FUN_00864ef0(param_1);
    if ((puVar2 != (undefined *)0x0) ||
       (((0x10d < uVar4 && (uVar4 < 0x110)) &&
        (puVar2 = &DAT_01132098 + (param_1 - 0x10e) * 0x220, puVar2 != (undefined *)0x0)))) {
LAB_008652da:
      if (param_2 < 4) {
        puVar3 = puVar2 + param_2 * 0x3e;
      }
      return puVar3;
  // ... [truncated for register brevity] ...
```

#### `0x00865380` — `orig_GetTeamKitDataB`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00865380`
- **Signature:** `undefined FUN_00865380(void)`
- **Binary Size:** 164 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `17` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00865380(uint param_1,int param_2)

{
  int iVar1;
  undefined *puVar2;
  undefined *puVar3;
  ushort uVar4;
  
  puVar3 = (undefined *)0x0;
  if (param_2 < 0) {
    return (undefined *)0x0;
  }
  iVar1 = FUN_00866d80(param_1);
  uVar4 = (ushort)param_1;
  if (((uVar4 < 0xfe) || (0x10d < uVar4)) && (iVar1 == 3)) {
    puVar2 = (undefined *)FUN_00864ef0(param_1 & 0xffff);
    if ((puVar2 == (undefined *)0x0) &&
       (((uVar4 < 0x10e || (0x10f < uVar4)) ||
        (puVar2 = &DAT_01132098 + ((param_1 & 0xffff) - 0x10e) * 0x220, puVar2 == (undefined *)0x0))
       )) {
      return (undefined *)0x0;
    }
    if (param_2 < 4) {
      return puVar2 + param_2 * 0x18 + 0x100;
    }
  // ... [truncated for register brevity] ...
```

#### `0x00865430` — `fn_GetTeamKitDataC`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00865430`
- **Signature:** `undefined FUN_00865430(void)`
- **Binary Size:** 145 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `6` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * FUN_00865430(uint param_1,int param_2)

{
  int iVar1;
  undefined *puVar2;
  undefined *puVar3;
  ushort uVar4;
  
  puVar3 = (undefined *)0x0;
  if (param_2 < 0) {
    return (undefined *)0x0;
  }
  iVar1 = FUN_00866d80(param_1);
  uVar4 = (ushort)param_1;
  if (((uVar4 < 0xfe) || (0x10d < uVar4)) && (iVar1 == 3)) {
    puVar2 = (undefined *)FUN_00864ef0(param_1 & 0xffff);
    if ((puVar2 == (undefined *)0x0) &&
       (((uVar4 < 0x10e || (0x10f < uVar4)) ||
        (puVar2 = &DAT_01132098 + ((param_1 & 0xffff) - 0x10e) * 0x220, puVar2 == (undefined *)0x0))
       )) {
      return (undefined *)0x0;
    }
    if (param_2 < 4) {
      puVar3 = puVar2 + param_2 * 0x30 + 0x160;
    }
  // ... [truncated for register brevity] ...
```

#### `0x008654D0` — `orig_IsTeamKitEdited`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_008654d0`
- **Signature:** `undefined FUN_008654d0(void)`
- **Binary Size:** 161 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
bool FUN_008654d0(uint param_1)

{
  int iVar1;
  undefined *puVar2;
  ushort uVar3;
  
  iVar1 = FUN_00866d80(param_1);
  uVar3 = (ushort)param_1;
  if (((uVar3 < 0xfe) || (0x10d < uVar3)) && (iVar1 == 3)) {
    param_1 = param_1 & 0xffff;
    puVar2 = (undefined *)FUN_00864ef0(param_1);
    if (puVar2 != (undefined *)0x0) goto LAB_00865561;
    if (((uVar3 < 0x10e) || (0x10f < uVar3)) ||
       (puVar2 = &DAT_01132098 + (param_1 - 0x10e) * 0x220, puVar2 == (undefined *)0x0)) {
      if ((uVar3 < 0x110) || (0x111 < uVar3)) {
        puVar2 = (undefined *)0x0;
      }
      else {
        puVar2 = &DAT_011324d8 + (param_1 - 0x110) * 0xf8;
      }
    }
  }
  else {
    puVar2 = (undefined *)FUN_00865100(param_1);
  // ... [truncated for register brevity] ...
```

#### `0x00866D80` — `fn_GetTeamDataType`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00866d80`
- **Signature:** `undefined FUN_00866d80(void)`
- **Binary Size:** 309 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `15` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00866d80(uint param_1)

{
  ushort uVar1;
  int iVar2;
  
  while( true ) {
    uVar1 = (ushort)param_1;
    if ((uVar1 < 0x39) || ((0xcb < uVar1 && (uVar1 < 0xdd)))) {
      return 0;
    }
    if ((0x38 < uVar1) && (uVar1 < 0x40)) {
      return 2;
    }
    if (((((((0x3f < uVar1) && (uVar1 < 0xcc)) || ((0xb7 < uVar1 && (uVar1 < 0xcc)))) ||
          (((0xdc < uVar1 && (uVar1 < 0xfe)) || ((0xfd < uVar1 && (uVar1 < 0x10e)))))) ||
         (((((0x10d < uVar1 && (uVar1 < 0x110)) || ((0x10f < uVar1 && (uVar1 < 0x112)))) ||
           ((((uVar1 == 0xdd || (uVar1 == 0x122)) || (uVar1 == 0x112)) ||
            ((uVar1 == 0x113 || (uVar1 == 0x114)))))) || (uVar1 == 0x115)))) ||
        (((uVar1 == 0x116 || (uVar1 == 0x117)) ||
         (((uVar1 == 0x118 || (((uVar1 == 0x11e || (uVar1 == 0x11f)) || (uVar1 == 0x120)))) ||
          (((uVar1 == 0x121 || (uVar1 == 0x123)) || (uVar1 == 0x124)))))))) || (uVar1 == 0x125))
    break;
    if ((uVar1 != 0x126) && (uVar1 != 0x127)) {
      return 0;
  // ... [truncated for register brevity] ...
```

#### `0x008C6170` — `ADDR_GetMasterLeagueLoadedTeam`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_008c5f50`
- **Signature:** `undefined FUN_008c5f50(void)`
- **Binary Size:** 628 bytes (Exact Entry: `False`)
- **Call Relationships:** Callers: `1` | Callees: `1`
- **Confidence Level:** `PROBABLE`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_008c5f50(byte *param_1,char *param_2,uint *param_3,int param_4,uint *param_5)

{
  byte *pbVar1;
  byte bVar2;
  byte *pbVar3;
  byte *pbVar4;
  byte *pbVar5;
  uint *puVar6;
  int iVar7;
  byte *pbVar8;
  int iVar9;
  int local_1c;
  int local_18;
  uint local_10;
  uint local_c;
  uint local_8;
  uint local_4;
  
  pbVar4 = param_1;
  if (((param_1 == (byte *)0x0) || (param_2 == (char *)0x0)) || (*param_2 == '\x01')) {
    return -5;
  }
  if ((param_3 == (uint *)0x0) || (param_4 < 1)) {
    return 0;
  // ... [truncated for register brevity] ...
```

#### `0x00966F70` — `orig_FillKitTeamSlot`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00966f70`
- **Signature:** `undefined FUN_00966f70(void)`
- **Binary Size:** 501 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `7`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_00966f70(int param_1,int param_2)

{
  char cVar1;
  char *pcVar2;
  undefined4 uVar3;
  int in_ECX;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 local_1410;
  undefined4 local_140c;
  undefined4 local_1408 [256];
  undefined4 local_1008 [1023];
  undefined4 uStack_c;
  
  uStack_c = 0x966f80;
  iVar4 = *(int *)(param_2 + 0x38 + in_ECX * 4);
  pcVar2 = *(char **)(param_2 + in_ECX * 4);
  local_1410 = 0x3f000000;
  local_140c = 0x3f000000;
  *(char *)(in_ECX + 200 + param_1) = pcVar2[2];
  if ((((iVar4 < 0x124) || (0x1b9 < iVar4)) && (iVar4 != 0x1ba)) && (iVar4 != 0x1bb)) {
  // ... [truncated for register brevity] ...
```

#### `0x00967170` — `orig_FillKitVariantSlot`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00967170`
- **Signature:** `undefined FUN_00967170(void)`
- **Binary Size:** 336 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00967170(int param_1,int param_2,int param_3,int param_4)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  undefined4 local_248 [16];
  undefined4 local_208 [129];
  
  iVar5 = param_1 + 2;
  iVar2 = FUN_008650a0(*(undefined1 *)(*(int *)(param_3 + 8 + param_1 * 4) + 3));
  *(undefined1 *)(param_1 + 0xca + param_2) = *(undefined1 *)(*(int *)(param_3 + iVar5 * 4) + 2);
  puVar6 = (undefined4 *)(iVar2 + 0x60);
  puVar7 = local_208;
  for (iVar4 = 0x80; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  puVar6 = (undefined4 *)(iVar2 + 0x20);
  puVar7 = local_248;
  // ... [truncated for register brevity] ...
```

#### `0x00968700` — `orig_Match_SetupKitData`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00968700`
- **Signature:** `undefined FUN_00968700(void)`
- **Binary Size:** 71 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `4` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00968700(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  iVar1 = FUN_00865240(param_2,param_3);
  if (*(char *)(iVar1 + 0x3a) != '\0') {
    FUN_009683c0(param_1,param_2);
    return;
  }
  FUN_00968690(param_1,iVar1,param_3,param_4);
  return;
}
```

#### `0x00968960` — `orig_Match_SetupKitData2`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00968960`
- **Signature:** `undefined FUN_00968960(void)`
- **Binary Size:** 69 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `4` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00968960(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  
  iVar1 = FUN_00865240(param_2,param_3);
  if (*(char *)(iVar1 + 0x3a) != '\0') {
    FUN_00968410(param_2);
    return;
  }
  FUN_009688e0(param_1,iVar1,param_4);
  return;
}
```

#### `0x00968E70` — `orig_ExtractKitColor`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00968e70`
- **Signature:** `undefined FUN_00968e70(void)`
- **Binary Size:** 851 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00968e70(char *param_1,undefined4 param_2,undefined4 param_3,int param_4,int param_5)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  uint *puVar4;
  undefined4 *puVar5;
  undefined4 local_138;
  uint local_134 [77];
  
  local_134[0x1a] = 0x6f;
  local_134[0x1e] = 0x6f;
  local_134[0x2c] = 0x6f;
  local_134[0x2f] = 0;
  local_134[0x30] = 0;
  local_134[0x31] = 0;
  local_134[0x32] = 0;
  local_134[0x33] = 0;
  local_134[0x18] = 0x6c;
  local_134[0x2a] = 0x6c;
  local_134[0x19] = 0x6d;
  local_134[0x1d] = 0x6e;
  local_134[0x2b] = 0x6d;
  local_134[0x34] = 0;
  // ... [truncated for register brevity] ...
```

#### `0x009691D0` — `fn_LoadTeamData_EditedKit`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009691d0`
- **Signature:** `undefined FUN_009691d0(void)`
- **Binary Size:** 251 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009691d0(undefined1 *param_1,undefined4 param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  uVar1 = FUN_008646b0(param_2);
  param_1[0x10] = 0;
  *(undefined4 *)(param_1 + 0x14) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x18) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x1c) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x20) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x24) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x28) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x2c) = 0xffffffff;
  *(undefined4 *)(param_1 + 0x30) = 0xffffffff;
  iVar3 = (param_3 + (uVar1 & 0xffff) * 2) * 6;
  iVar2 = FUN_00865240(param_2,((&DAT_03be6078)[(byte)param_1[1]] & 1) * '\x02' + '\x01');
  *param_1 = *(undefined1 *)(iVar2 + 0x3c);
  iVar2 = FUN_00865240(param_2,param_3 + (uint)(byte)param_1[0x34] * 2);
  *(uint *)(param_1 + 0x14) = iVar3 + (uint)(byte)param_1[0x34] * 3;
  *(undefined4 *)(param_1 + 0x30) = 0;
  param_1[4] = *(undefined1 *)(iVar2 + 0x36);
  param_1[0xb] = *(undefined1 *)(iVar2 + 0x3d);
  // ... [truncated for register brevity] ...
```

#### `0x009692D0` — `fn_LoadTeamData_Normal`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009692d0`
- **Signature:** `undefined FUN_009692d0(void)`
- **Binary Size:** 445 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009692d0(char *param_1,undefined4 param_2,int param_3,int param_4)

{
  undefined2 *puVar1;
  int iVar2;
  int iVar3;
  uint local_8 [2];
  
  FUN_008646b0(param_2);
  param_1[0x10] = '\x01';
  puVar1 = (undefined2 *)FUN_00865240(param_2,param_3 + (uint)(byte)param_1[0x34] * 2);
  param_1[0xb] = *(char *)((int)puVar1 + 0x3d);
  param_1[10] = *(char *)(puVar1 + 0x15);
  *(uint *)(param_1 + 0x14) = (uint)*(byte *)(puVar1 + 0x15);
  *(uint *)(param_1 + 0x18) = (uint)*(byte *)(puVar1 + 0x17);
  *(uint *)(param_1 + 0x1c) = (uint)*(byte *)((int)puVar1 + 0x2d);
  *(uint *)(param_1 + 0x20) = (uint)*(byte *)(puVar1 + 0x16);
  *(uint *)(param_1 + 0x24) = (uint)*(byte *)((int)puVar1 + 0x2b);
  *(uint *)(param_1 + 0x30) = (uint)*(byte *)((int)puVar1 + 0x31);
  param_1[4] = *(char *)(puVar1 + 0x1b);
  param_1[0xc] = *(char *)(puVar1 + 0x1c);
  *(undefined2 *)(param_1 + 0x38) = *puVar1;
  *(undefined2 *)(param_1 + 0x3a) = puVar1[4];
  *(undefined2 *)(param_1 + 0x3c) = puVar1[3];
  *(undefined2 *)(param_1 + 0x3e) = puVar1[2];
  // ... [truncated for register brevity] ...
```

#### `0x00969490` — `fn_LoadTeamData_MLDefault`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00969490`
- **Signature:** `undefined FUN_00969490(void)`
- **Binary Size:** 158 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00969490(undefined1 *param_1)

{
  byte bVar1;
  ushort uVar2;
  ushort uVar3;
  ushort uVar4;
  byte *pbVar5;
  
  pbVar5 = &DAT_03be14a1;
  if (param_1[0x34] != '\0') {
    pbVar5 = &DAT_03be14a5;
  }
  uVar4 = (ushort)(*pbVar5 >> 3);
  uVar2 = *(ushort *)(param_1 + 0x38);
  *(ushort *)(param_1 + 0x38) = uVar4 | uVar2 & 0xffe0;
  uVar3 = (ushort)(pbVar5[1] >> 3) << 5;
  *(ushort *)(param_1 + 0x38) = uVar3 | uVar4 | uVar2 & 0xfc00;
  bVar1 = pbVar5[2];
  *(undefined2 *)(param_1 + 0xe) = 0;
  param_1[0xb] = 0;
  param_1[2] = 0;
  param_1[4] = 0;
  param_1[6] = 0;
  param_1[8] = 0;
  // ... [truncated for register brevity] ...
```

#### `0x00969530` — `fn_LoadTeamData_CustomFormation`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00969530`
- **Signature:** `undefined FUN_00969530(void)`
- **Binary Size:** 212 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00969530(char *param_1,undefined4 param_2,int param_3,int param_4)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  
  FUN_008646b0(param_2);
  param_1[0x10] = '\x03';
  param_1[0x14] = -1;
  param_1[0x15] = -1;
  param_1[0x16] = -1;
  param_1[0x17] = -1;
  param_1[0x18] = -1;
  param_1[0x19] = -1;
  param_1[0x1a] = -1;
  param_1[0x1b] = -1;
  param_1[0x1c] = -1;
  param_1[0x1d] = -1;
  param_1[0x1e] = -1;
  param_1[0x1f] = -1;
  param_1[0x20] = -1;
  param_1[0x21] = -1;
  param_1[0x22] = -1;
  param_1[0x23] = -1;
  // ... [truncated for register brevity] ...
```

#### `0x00969610` — `orig_LoadTeamKitDataBySlot`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00969610`
- **Signature:** `undefined FUN_00969610(void)`
- **Binary Size:** 382 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `9`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00969610(int param_1,int param_2)

{
  byte bVar1;
  undefined2 uVar2;
  undefined1 uVar3;
  char cVar4;
  short sVar5;
  int in_EAX;
  uint uVar6;
  sbyte sVar7;
  int iVar8;
  byte bVar9;
  int iVar10;
  undefined *puVar11;
  undefined4 *puVar12;
  
  uVar2 = (&DAT_03be0940)[param_1];
  iVar10 = (in_EAX + param_1 * 2) * 100;
  puVar11 = &DAT_03b8eea0 + iVar10;
  sVar5 = FUN_008646b0(uVar2);
  puVar12 = (undefined4 *)(&DAT_03b8eed8 + iVar10);
  for (iVar8 = 10; iVar8 != 0; iVar8 = iVar8 + -1) {
    *puVar12 = 0;
    puVar12 = puVar12 + 1;
  // ... [truncated for register brevity] ...
```

#### `0x00969790` — `orig_LoadBothTeamKitData`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00969790`
- **Signature:** `undefined FUN_00969790(void)`
- **Binary Size:** 33 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `6` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00969790(undefined4 param_1)

{
  FUN_00969610(param_1,0);
  FUN_00969610(param_1,1);
  return;
}
```

#### `0x009B0390` — `orig_BadgeRenderCdecl`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009b0390`
- **Signature:** `undefined FUN_009b0390(void)`
- **Binary Size:** 46 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009b0390(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  
  iVar1 = FUN_00b0c1a0(param_1);
  if (iVar1 == 1) {
    FUN_00863ff0(param_2);
    return;
  }
  FUN_008639f0(param_2);
  return;
}
```

#### `0x009CC4B0` — `fn_IsTeamUnlocked`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009cc4b0`
- **Signature:** `undefined FUN_009cc4b0(void)`
- **Binary Size:** 84 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `12` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_009cc4b0(ushort param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = thunk_FUN_00866b40();
  iVar2 = FUN_008c0180();
  if ((((iVar2 == 0) && (0x38 < param_1)) && (param_1 < 0x40)) &&
     ((*(uint *)(iVar1 + ((int)(param_1 + 0x8f) >> 5) * 4) & 1 << ((byte)(param_1 + 0x8f) & 0x1f))
      != 0)) {
    return 1;
  }
  return 0;
}
```

#### `0x009D2880` — `fn_GetLeagueSelector`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009d2880`
- **Signature:** `undefined FUN_009d2880(void)`
- **Binary Size:** 18 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_009d2880(int param_1)

{
  return (int)(short)(&DAT_03a74054)[param_1 * 0xd4];
}
```

#### `0x009D2EC0` — `ADDR_SetLeagueTeamCount`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009d2ec0`
- **Signature:** `undefined FUN_009d2ec0(void)`
- **Binary Size:** 393 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_009d2ec0(void)

{
  short sVar1;
  int iVar2;
  bool bVar3;
  
  sVar1 = DAT_03a74a46;
  iVar2 = DAT_03a74050 * 0xd + (int)DAT_03a74048;
  if (iVar2 != 1) goto LAB_009d2f75;
  switch(DAT_03a741fe) {
  case 0:
  case 2:
  case 3:
  case 4:
    DAT_03a748a6 = 0x14;
    DAT_03a7489e = 0x14;
    goto LAB_009d2f27;
  case 1:
    DAT_03a748a6 = 0x12;
    DAT_03a7489e = 0x12;
LAB_009d2f27:
    DAT_03a74a46 = 1;
  // ... [truncated for register brevity] ...
```

#### `0x009D3BC0` — `fn_GetSelectedCupIdx`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009d3bc0`
- **Signature:** `undefined FUN_009d3bc0(void)`
- **Binary Size:** 18 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_009d3bc0(int param_1)

{
  return (int)(short)(&DAT_03a76d2c)[param_1 * 0xd4];
}
```

#### `0x009DAB20` — `fn_LoadLeagueLogo`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009dab20`
- **Signature:** `undefined FUN_009dab20(void)`
- **Binary Size:** 98 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `10` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009dab20(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  bool bVar5;
  
  uVar3 = FUN_00af9f30(param_1);
  uVar3 = FUN_0094e9c0(uVar3);
  FUN_0094e9e0(uVar3,param_2,param_3);
  puVar4 = (undefined4 *)thunk_FUN_00877070(1,0xc);
  iVar2 = (int)DAT_03a79d74;
  bVar5 = DAT_03a79d74 != (undefined4 *)0x0;
  *puVar4 = param_1;
  puVar4[1] = param_2;
  if (bVar5) {
    iVar1 = *(int *)(iVar2 + 8);
    while (iVar1 != 0) {
      iVar2 = *(int *)(iVar2 + 8);
      iVar1 = *(int *)(iVar2 + 8);
    }
    *(undefined4 **)(iVar2 + 8) = puVar4;
    return;
  // ... [truncated for register brevity] ...
```

#### `0x009E25A0` — `orig_LoadClubTeamSlots`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009e25a0`
- **Signature:** `undefined FUN_009e25a0(void)`
- **Binary Size:** 2341 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `10`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009e25a0(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  char *pcVar5;
  int iVar6;
  int *piVar7;
  char *pcVar8;
  int local_4b0 [40];
  char local_410 [1024];
  undefined4 local_10;
  
  piVar7 = local_4b0;
  local_4b0[0] = 0;
  for (iVar6 = 0x128; piVar7 = piVar7 + 1, iVar6 != 0; iVar6 = iVar6 + -1) {
    *piVar7 = 0;
  }
  if (DAT_03be12c9 != '\x02') {
    uVar2 = FUN_009d2880(1);
    switch(uVar2) {
    case 0:
      local_4b0[0] = 0xffff;
  // ... [truncated for register brevity] ...
```

#### `0x009ED730` — `orig_LoadClubTeamSlots`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009ed730`
- **Signature:** `undefined FUN_009ed730(void)`
- **Binary Size:** 1166 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `9`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009ed730(int param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  undefined4 uVar5;
  int iVar6;
  int *piVar7;
  char *pcVar8;
  int iVar9;
  int iVar10;
  short sVar11;
  undefined4 *puVar12;
  int local_4c8;
  undefined4 *local_4c4;
  int local_4c0;
  int local_4a4 [40];
  char local_404 [1024];
  undefined4 local_4;
  
  iVar10 = 0;
  piVar7 = local_4a4;
  for (iVar6 = 0x128; piVar7 = piVar7 + 1, iVar6 != 0; iVar6 = iVar6 + -1) {
  // ... [truncated for register brevity] ...
```

#### `0x009EE940` — `orig_SetTeamList`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_009ee940`
- **Signature:** `undefined FUN_009ee940(void)`
- **Binary Size:** 744 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `20`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_009ee940(void)

{
  int iVar1;
  undefined4 uVar2;
  ushort uVar3;
  ushort uVar4;
  float10 fVar5;
  undefined4 local_20;
  undefined4 local_1c;
  float local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  
  if (DAT_03becc60 == 4) {
    FUN_009dab20(0x158,0x4e9c,5);
    FUN_009dab20(0x158,0x4e9b,5);
    FUN_009dab20(0x158,0x4e98,5);
    FUN_009dab20(0x158,0x4e99,5);
    FUN_009dab20(0x158,0x4e9a,5);
    FUN_009dab20(0x158,0x4e9d,5);
    FUN_009dab20(0x158,0x4e9e,5);
  // ... [truncated for register brevity] ...
```

#### `0x00B07B00` — `fn_SetTeamList`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b07b00`
- **Signature:** `undefined FUN_00b07b00(void)`
- **Binary Size:** 89 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `10` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b07b00(int param_1,int param_2,int param_3)

{
  char cVar1;
  int iVar2;
  char *pcVar3;
  undefined4 *puVar4;
  
  iVar2 = 0;
  param_1 = param_2 * 0x4a4 + param_1;
  puVar4 = (undefined4 *)(param_1 + 0x1944);
  do {
    *puVar4 = *(undefined4 *)(param_3 + iVar2 * 4);
    iVar2 = iVar2 + 1;
    puVar4 = puVar4 + 1;
  } while (iVar2 < 0x14);
  pcVar3 = (char *)(param_3 + 0xa0);
  iVar2 = (param_1 + 0x19e4) - (int)pcVar3;
  do {
    cVar1 = *pcVar3;
    pcVar3[iVar2] = cVar1;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  *(undefined4 *)(param_1 + 0x1de4) = *(undefined4 *)(param_3 + 0x4a0);
  return;
  // ... [truncated for register brevity] ...
```

#### `0x00B083B0` — `orig_BuildLeaguePanelVisualSlots`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b083b0`
- **Signature:** `undefined FUN_00b083b0(void)`
- **Binary Size:** 1927 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `0` | Callees: `24`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b083b0(void)

{
  float fVar1;
  char cVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int *piVar6;
  undefined4 uVar7;
  uint uVar8;
  int *piVar9;
  undefined4 *puVar10;
  int iVar11;
  undefined4 *puVar12;
  int unaff_ESI;
  int iVar13;
  bool bVar14;
  float10 fVar15;
  undefined2 uVar16;
  int local_98;
  int local_94;
  int local_90;
  float local_8c;
  float local_88;
  // ... [truncated for register brevity] ...
```

#### `0x00B09050` — `orig_CreateLeagueSelectionPanel`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b09050`
- **Signature:** `undefined FUN_00b09050(void)`
- **Binary Size:** 1237 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `11` | Callees: `28`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 * FUN_00b09050(int *param_1,undefined4 *param_2,undefined4 *param_3)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 uVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  puVar10 = param_3;
  local_20 = 0;
  local_1c = 0;
  local_18 = 0;
  // ... [truncated for register brevity] ...
```

#### `0x00B09590` — `fn_SetSelectedTeam`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b09590`
- **Signature:** `undefined FUN_00b09590(void)`
- **Binary Size:** 273 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `25` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00b09590(int param_1,int param_2,ushort param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint *puVar5;
  int iVar6;
  
  iVar6 = 0;
  puVar5 = (uint *)(param_1 + 0x1944);
  do {
    iVar2 = 0;
    puVar4 = puVar5;
    do {
      if ((uint)param_3 == *puVar4) {
        iVar1 = param_1 + param_2 * 0x18;
        *(int *)(iVar1 + 0x1910) = iVar2;
        *(int *)(iVar1 + 0x1914) = iVar6;
        iVar6 = *(int *)(param_1 + 0x1914 + *(int *)(param_1 + 0x1940) * 0x18);
        puVar5 = (uint *)(param_1 + 0x1914 + *(int *)(param_1 + 0x1940) * 0x18);
        if (iVar6 < 8) {
          iVar6 = 0;
        }
  // ... [truncated for register brevity] ...
```

#### `0x00B0F8B0` — `orig_BadgeSet`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b0f8b0`
- **Signature:** `undefined FUN_00b0f8b0(void)`
- **Binary Size:** 210 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `6`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00b0f8b0(undefined4 param_1,int param_2)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  int *piVar4;
  int *unaff_EDI;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar1 = *unaff_EDI;
  FUN_009b0280(*(undefined4 *)(iVar1 + 4),param_1,param_2);
  iVar1 = FUN_009b02f0(*(undefined4 *)(iVar1 + 4),param_1,param_2,&local_18,0);
  if (iVar1 == 0) {
    return 0;
  }
  local_10 = 0;
  local_c = 0;
  local_8 = *(undefined4 *)(*unaff_EDI + 0x14);
  local_4 = *(undefined4 *)(*unaff_EDI + 0x18);
  // ... [truncated for register brevity] ...
```

#### `0x00B0F990` — `orig_BadgeRender`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b0f990`
- **Signature:** `undefined FUN_00b0f990(void)`
- **Binary Size:** 168 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `5`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0f990(int param_1)

{
  undefined4 uVar1;
  int *in_EAX;
  int iVar2;
  int in_ECX;
  
  if (in_EAX != (int *)0x0) {
    if (in_ECX == 0xffff) {
      *(undefined1 *)(in_EAX + 4) = 0;
      FUN_00951310(in_EAX[1],0xff00);
      FUN_00951310(in_EAX[2],0);
      return;
    }
    *(undefined1 *)(in_EAX + 4) = 1;
    uVar1 = *(undefined4 *)(*in_EAX + 8);
    iVar2 = FUN_009b0390(*(undefined4 *)(*in_EAX + 4));
    if ((param_1 == 0) && (iVar2 == in_EAX[3])) {
      FUN_00b0f870();
      return;
    }
    FUN_009b0540(*(undefined4 *)(*in_EAX + 4),*(undefined4 *)(*in_EAX + 8),in_EAX[3]);
    iVar2 = FUN_00b0f8b0(uVar1,iVar2);
    if (iVar2 == 0) {
  // ... [truncated for register brevity] ...
```

#### `0x00B0FA80` — `orig_BadgeRender`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b0fa80`
- **Signature:** `undefined FUN_00b0fa80(void)`
- **Binary Size:** 160 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `5`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0fa80(int *param_1,int param_2)

{
  undefined4 uVar1;
  int *piVar2;
  int iVar3;
  
  piVar2 = param_1;
  if (param_1 != (int *)0x0) {
    if (param_2 == 0xffff) {
      iVar3 = param_1[1];
      param_1 = (int *)((uint)param_1 & 0xffffff00);
      *(undefined1 *)(piVar2 + 4) = 0;
      FUN_00951310(iVar3,param_1);
      FUN_00951310(piVar2[2],0);
      return;
    }
    *(undefined1 *)(param_1 + 4) = 1;
    uVar1 = *(undefined4 *)(*param_1 + 8);
    iVar3 = FUN_009b0390(*(undefined4 *)(*param_1 + 4),param_2);
    if (iVar3 == param_1[3]) {
      FUN_00b0f870();
      return;
    }
    FUN_009b0540(*(undefined4 *)(*param_1 + 4),*(undefined4 *)(*param_1 + 8),param_1[3]);
  // ... [truncated for register brevity] ...
```

#### `0x00B3BEB0` — `orig_CrestResolve`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `FUN_00b3beb0`
- **Signature:** `undefined FUN_00b3beb0(void)`
- **Binary Size:** 134 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `7` | Callees: `6`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00b3beb0(undefined4 param_1,undefined2 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_00953630(param_1);
  if (iVar1 == 0) {
    FUN_009b0370(DAT_03b51284,param_3);
    FUN_009b0460(DAT_03b51284,param_4,param_2);
    FUN_009b0370(DAT_03b51284,param_3);
    uVar2 = FUN_009b06e0(DAT_03b51284,param_4,param_2,0);
    iVar1 = FUN_00954690(uVar2,param_1);
    FUN_009b0370(DAT_03b51284,param_3);
    FUN_009b0780(DAT_03b51284,param_4,param_2);
  }
  return iVar1;
}
```

#### `0x00C97340` — `RE_GetAliasTeamData`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `RE_GetAliasTeamData`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

#### `0x00D5BF30` — `fn_GetLeagueString (table entry)`
- **Subsystem:** Kit & Team Data
- **Function Symbol:** `fn_GetLeagueString (table entry)`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

### 3.4 Subsystem: Player & Formation Management

#### `0x00456780` — `fnAddr (player_hooks pattern-scan fallback)`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `fnAddr (player_hooks pattern-scan fallback)`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

#### `0x005C18B0` — `fn_GetWeatherCondition`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_005c18b0`
- **Signature:** `undefined FUN_005c18b0(void)`
- **Binary Size:** 72 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `6` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_005c18b0(void)

{
  if (_DAT_01017508 < 5.0 != (_DAT_01017508 == 5.0)) {
    return CONCAT31((uint3)(byte)(_DAT_01017508 < 5.0 |
                                  (byte)((ushort)((ushort)NAN(_DAT_01017508) << 10) >> 8) |
                                 (byte)((ushort)((ushort)(_DAT_01017508 == 5.0) << 0xe) >> 8)),1);
  }
  if (_DAT_01017508 >= 20.0) {
    return (uint)(ushort)((ushort)(_DAT_01017508 < 20.0) << 8 | (ushort)NAN(_DAT_01017508) << 10 |
                         (ushort)(_DAT_01017508 == 20.0) << 0xe);
  }
  if (_DAT_01017508 < 15.0 != (_DAT_01017508 == 15.0)) {
    return 1;
  }
  return 0;
}
```

#### `0x0072E400` — `gameplay.formation`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `gameplay.formation`
- **Signature:** `manifest: gameplay.formation`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** Formation data loader and strategy setup
```c
// Manifest entry gameplay.formation at 0x0072E400
// See remaster_manifest.txt
```

#### `0x00752AC0` — `gameplay.team_a`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `gameplay.team_a`
- **Signature:** `manifest: gameplay.team_a`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** Match home/away team assignment
```c
// Manifest entry gameplay.team_a at 0x00752AC0
// See remaster_manifest.txt
```

#### `0x00861B90` — `orig_GetPlayerRecord (worker)`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00861b90`
- **Signature:** `undefined FUN_00861b90(void)`
- **Binary Size:** 390 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `20` | Callees: `8`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00861b90(void)

{
  int in_EAX;
  int iVar1;
  ushort in_CX;
  int iVar2;
  
  iVar2 = 0;
  if (in_CX < 5000) {
    if (in_EAX == 1) {
      iVar1 = FUN_00866cb0();
    }
    else {
      iVar1 = FUN_00866b90();
    }
    iVar2 = 0;
    if (iVar1 != 0) {
      return iVar1 + (uint)in_CX * 0x7c;
    }
  }
  else if ((in_CX < 6000) || (0x18f6 < in_CX)) {
    if ((0x6fff < in_CX) && (in_CX < 0x7020)) {
      iVar2 = FUN_006b1b30();
      return iVar2;
  // ... [truncated for register brevity] ...
```

#### `0x00862C10` — `orig_GetPlayerRecord`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00862c10`
- **Signature:** `undefined FUN_00862c10(void)`
- **Binary Size:** 604 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `19` | Callees: `11`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined1 FUN_00862c10(uint param_1,uint param_2,uint param_3)

{
  undefined1 uVar1;
  short sVar2;
  short sVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  byte bVar7;
  short sVar8;
  uint uVar9;
  uint uVar10;
  byte local_5;
  
  if (((param_3 != 0) && (param_3 != 1)) && (param_3 != 2)) {
    param_3 = 0;
  }
  iVar6 = param_3;
  uVar4 = FUN_00862710(param_1,param_2,param_3);
  iVar5 = FUN_008625b0(uVar4);
  if (iVar5 == 1) {
    return 0xff;
  }
  bVar7 = (byte)param_2;
  // ... [truncated for register brevity] ...
```

#### `0x008C0180` — `fn_GetModeFlag`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_008c0180`
- **Signature:** `undefined FUN_008c0180(void)`
- **Binary Size:** 6 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `173` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_008c0180(void)

{
  return DAT_03a62a40;
}
```

#### `0x00966C70` — `fn_HasCustomFormation`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00966c70`
- **Signature:** `undefined FUN_00966c70(void)`
- **Binary Size:** 12 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `8` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined1 FUN_00966c70(byte param_1)

{
  return *(undefined1 *)((int)&DAT_03be6076 + (uint)param_1);
}
```

#### `0x00A4F0A0` — `fn_InitPlayerState`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f0a0`
- **Signature:** `undefined FUN_00a4f0a0(void)`
- **Binary Size:** 38 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `13` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f0a0(undefined4 param_1)

{
  if (DAT_03a8af20 == 0) {
    return 0;
  }
  FUN_00a73510(DAT_03a8af08,param_1);
  return 1;
}
```

#### `0x00A4F120` — `fn_SetPlayerActive`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f120`
- **Signature:** `undefined FUN_00a4f120(void)`
- **Binary Size:** 33 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `23` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f120(undefined4 param_1)

{
  undefined4 uVar1;
  
  if (DAT_03a8af20 == 0) {
    return 0;
  }
  uVar1 = FUN_00a734d0(DAT_03a8af08,param_1);
  return uVar1;
}
```

#### `0x00A4F150` — `fn_SetPlayerCursorType`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f150`
- **Signature:** `undefined FUN_00a4f150(void)`
- **Binary Size:** 33 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `16` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f150(undefined4 param_1)

{
  undefined4 uVar1;
  
  if (DAT_03a8af20 == 0) {
    return 0;
  }
  uVar1 = FUN_00a743f0(DAT_03a8af0c,param_1);
  return uVar1;
}
```

#### `0x00A4F3B0` — `fn_ResetPlayerSelection`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f3b0`
- **Signature:** `undefined FUN_00a4f3b0(void)`
- **Binary Size:** 33 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f3b0(undefined4 param_1)

{
  undefined4 uVar1;
  
  if (DAT_03a8af20 == 0) {
    return 0;
  }
  uVar1 = FUN_00a73590(DAT_03a8af08,param_1);
  return uVar1;
}
```

#### `0x00A4F520` — `fn_SetPlayerInputMode`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f520`
- **Signature:** `undefined FUN_00a4f520(void)`
- **Binary Size:** 269 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `6` | Callees: `13`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f520(undefined4 param_1)

{
  char cVar1;
  char cVar2;
  undefined1 uVar3;
  int iVar4;
  int iVar5;
  undefined4 uVar6;
  undefined1 uVar7;
  bool bVar8;
  
  if (DAT_03a8af20 == 0) {
    return 0;
  }
  iVar4 = FUN_00a608c0();
  FUN_00a737e0(DAT_03a8af0c,iVar4);
  FUN_00a73830(DAT_03a8af0c,param_1);
  iVar5 = 0;
  do {
    uVar6 = 0;
    uVar7 = 3;
    bVar8 = false;
    uVar3 = 2;
    FUN_00a73b70(DAT_03a8af0c,iVar5,0);
  // ... [truncated for register brevity] ...
```

#### `0x00A4F840` — `fn_GetPlayerState`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00a4f840`
- **Signature:** `undefined FUN_00a4f840(void)`
- **Binary Size:** 98 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00a4f840(int param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  
  cVar1 = FUN_00a68260();
  iVar3 = (int)cVar1;
  if ((((param_1 < 0) || (0x11 < param_1)) || (iVar3 < 0)) || (2 < iVar3)) {
    return 0;
  }
  iVar2 = FUN_00a521b0();
  if (iVar2 != 4) {
    if (iVar2 == 5) {
      return *(undefined4 *)(&DAT_00e836d8 + iVar3 * 4 + param_1 * 0xc);
    }
    if (iVar2 == 6) {
      return *(undefined4 *)(&DAT_00e837b0 + iVar3 * 4 + param_1 * 0xc);
    }
  }
  return *(undefined4 *)(&DAT_00e83600 + iVar3 * 4 + param_1 * 0xc);
}
```

#### `0x00AB6320` — `gameplay.match_init`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `gameplay.match_init`
- **Signature:** `manifest: gameplay.match_init`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Multiple` | Callees: `Multiple`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `VERIFIED (Hooked / Validated in Manifest)`
- **Description:** Match simulation bootstrap and stadium environment
```c
// Manifest entry gameplay.match_init at 0x00AB6320
// See remaster_manifest.txt
```

#### `0x00B15700` — `fn_IsOnlineMode`
- **Subsystem:** Player & Formation Management
- **Function Symbol:** `FUN_00b15700`
- **Signature:** `undefined FUN_00b15700(void)`
- **Binary Size:** 27 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00b15700(void)

{
  undefined4 uVar1;
  
  if ((DAT_03be12c9 != '\n') || (uVar1 = 1, DAT_03becc44 != '\x01')) {
    uVar1 = 0;
  }
  return uVar1;
}
```

### 3.5 Subsystem: UI, Menus & Presentation

#### `0x005EE610` — `ADDR_MenuPageFinish`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_005ee080`
- **Signature:** `undefined FUN_005ee080(void)`
- **Binary Size:** 2458 bytes (Exact Entry: `False`)
- **Call Relationships:** Callers: `1` | Callees: `20`
- **Confidence Level:** `PROBABLE`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_005ee080(char *param_1,int *param_2)

{
  byte bVar1;
  byte bVar2;
  char cVar3;
  byte bVar4;
  ushort uVar5;
  ushort uVar6;
  undefined4 uVar7;
  int iVar8;
  uint uVar9;
  undefined4 uVar10;
  int iVar11;
  int iVar12;
  char *pcVar13;
  bool bVar14;
  
  uVar7 = FUN_00496fd0(param_1,*(undefined4 *)(param_1 + 0xe0),0);
  *param_2 = 0;
  switch(*(undefined2 *)(PTR_DAT_00bcce94 + 0x84)) {
  case 0:
  case 1:
  case 2:
  case 5:
  // ... [truncated for register brevity] ...
```

#### `0x006094B0` — `ADDR_MenuPageSetup`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `ADDR_MenuPageSetup`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

#### `0x00800480` — `(unlabeled)`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00800460`
- **Signature:** `undefined FUN_00800460(void)`
- **Binary Size:** 112 bytes (Exact Entry: `False`)
- **Call Relationships:** Callers: `1` | Callees: `4`
- **Confidence Level:** `PROBABLE`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00800460(void)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  
  iVar1 = FUN_00778000(*(undefined4 *)(&DAT_011043f8)[DAT_011043f4]);
  iVar3 = 0;
  if (0 < iVar1) {
    do {
      uVar2 = FUN_00777f90(*(undefined4 *)(&DAT_011043f8)[DAT_011043f4],iVar3);
      FUN_009502b0(uVar2);
      iVar3 = iVar3 + 1;
    } while (iVar3 < iVar1);
  }
  FUN_00777f20(*(undefined4 *)(&DAT_011043f8)[DAT_011043f4]);
  *(undefined4 *)(&DAT_011043f8)[DAT_011043f4] = 0;
  return;
}
```

#### `0x00861850` — `(unlabeled)`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00861850`
- **Signature:** `undefined FUN_00861850(void)`
- **Binary Size:** 284 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `4` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00861850(void)

{
  ushort uVar1;
  byte *in_EAX;
  undefined4 uVar2;
  ushort *in_ECX;
  
  uVar1 = *in_ECX;
  if ((uVar1 < 0x39) || ((0x38 < uVar1 && (uVar1 < 0x40)))) {
    return 0;
  }
  if (uVar1 != 0x11c) {
    if (uVar1 != 0x11d) {
      if ((0x3f < uVar1) && (uVar1 < 0xcc)) {
        *in_ECX = uVar1 - 0x40;
        return 1;
      }
      if ((0x10f < uVar1) && (uVar1 < 0x112)) {
        *in_ECX = uVar1 - 0x7e;
        return 1;
      }
      if (uVar1 == 0x112) {
        *in_ECX = 0x8c;
        return 1;
  // ... [truncated for register brevity] ...
```

#### `0x00863660` — `(unlabeled)`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `thunk_FUN_00863570`
- **Signature:** `undefined thunk_FUN_00863570(void)`
- **Binary Size:** 5 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `57` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined * thunk_FUN_00863570(uint param_1,int param_2,int param_3)

{
  undefined *puVar1;
  int *piVar2;
  uint uVar3;
  ushort uVar4;
  
  while( true ) {
    while (((uVar4 = (ushort)param_1, param_3 == 0 && (0x3f < uVar4)) && (uVar4 < 0xcc))) {
      puVar1 = (undefined *)FUN_00862fc0(param_1);
      if (puVar1[0x38] != '\0') goto LAB_0086364d;
      param_3 = 1;
    }
    if ((uVar4 != 0x126) && (uVar4 != 0x127)) {
      if (0x127 < uVar4) {
        return &DAT_00b9885c;
      }
      if (param_2 == 0) {
        uVar3 = 0;
        do {
          if (uVar4 == (&DAT_00c96f30)[uVar3 * 4]) {
            puVar1 = (undefined *)FUN_0094f2c0((&DAT_00c96f34)[uVar3 * 2]);
            return puVar1;
          }
  // ... [truncated for register brevity] ...
```

#### `0x009519F0` — `fn_LabelNodeTransform`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_009519f0`
- **Signature:** `undefined FUN_009519f0(void)`
- **Binary Size:** 42 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `50` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009519f0(int param_1,undefined4 *param_2)

{
  if ((param_1 != 0) && (param_2 != (undefined4 *)0x0)) {
    *param_2 = *(undefined4 *)(param_1 + 0xc);
    param_2[1] = *(undefined4 *)(param_1 + 0x10);
    param_2[2] = *(undefined4 *)(param_1 + 0x14);
    param_2[3] = *(undefined4 *)(param_1 + 0x18);
  }
  return;
}
```

#### `0x009C2720` — `fn_SetupNavMapping`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_009c2720`
- **Signature:** `undefined FUN_009c2720(void)`
- **Binary Size:** 25 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `78` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009c2720(uint *param_1,uint param_2)

{
  if ((*param_1 & param_2) != param_2) {
    *param_1 = *param_1 + param_2;
  }
  return;
}
```

#### `0x009EDC40` — `fn_InitClubDisplayState`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_009edc40`
- **Signature:** `undefined FUN_009edc40(void)`
- **Binary Size:** 900 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `22`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_009edc40(void)

{
  int iVar1;
  undefined4 uVar2;
  bool bVar3;
  float10 fVar4;
  float local_30 [2];
  float local_28;
  float local_20 [2];
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [16];
  
  iVar1 = FUN_008c0180();
  bVar3 = iVar1 != 0;
  iVar1 = FUN_008c0180();
  if (iVar1 == 0) {
    iVar1 = FUN_0065c510(1);
    DAT_03be6304 = FUN_009da940(*(undefined4 *)(&DAT_00d5af08 + iVar1 * 4));
  }
  else {
    DAT_03be6304 = FUN_009da940(0x1038f);
  // ... [truncated for register brevity] ...
```

#### `0x00AA5D48` — `ADDR_MenuFlagsVar`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00aa5c90`
- **Signature:** `undefined FUN_00aa5c90(void)`
- **Binary Size:** 228 bytes (Exact Entry: `False`)
- **Call Relationships:** Callers: `2` | Callees: `7`
- **Confidence Level:** `PROBABLE`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00aa5c90(int param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  if (param_1 == 0) {
    return 1;
  }
  if (*(int *)(param_1 + 0xd40) != 0) {
    FUN_00b0caa0(*(int *)(param_1 + 0xd40));
    *(undefined4 *)(param_1 + 0xd40) = 0;
  }
  iVar1 = 0x19;
  iVar2 = param_1;
  do {
    FUN_00acade0(iVar2);
    iVar2 = iVar2 + 0x40;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  FUN_00acc300(param_1 + 0x808);
  piVar3 = (int *)(param_1 + 0x7e0);
  iVar2 = 5;
  do {
  // ... [truncated for register brevity] ...
```

#### `0x00AF9E00` — `fn_InitDisplaySubObject`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00af9e00`
- **Signature:** `undefined FUN_00af9e00(void)`
- **Binary Size:** 21 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00af9e00(undefined4 param_1)

{
  FUN_00b12910(DAT_03b47204,param_1);
  return;
}
```

#### `0x00B00400` — `fn_DestroyDisplayItem`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b00400`
- **Signature:** `undefined FUN_00b00400(void)`
- **Binary Size:** 63 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `84` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b00400(int param_1)

{
  int iVar1;
  int iVar2;
  
  if (param_1 != 0) {
    iVar1 = param_1 + 0x28;
    iVar2 = 2;
    do {
      FUN_00b12c50(iVar1);
      iVar1 = iVar1 + 0x24;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    FUN_00b0bb60(*(undefined4 *)(param_1 + 0x70));
    FUN_00b0bb80();
    return;
  }
  return;
}
```

#### `0x00B00920` — `fn_CreateDisplayItem`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b00920`
- **Signature:** `undefined FUN_00b00920(void)`
- **Binary Size:** 139 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `65` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
uint * FUN_00b00920(void)

{
  uint *puVar1;
  uint uVar2;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  puVar1 = (uint *)FUN_00b00390();
  uVar2 = *puVar1;
  if (uVar2 == 2) {
    *puVar1 = 1;
    uVar2 = *puVar1;
  }
  else if (9 < uVar2) {
    local_10 = 0;
    local_c = 0;
    local_8 = 0;
    local_4 = 0;
    FUN_00b00510(puVar1,&local_10);
    FUN_00b00720(puVar1,0xff);
    FUN_00b12cd0(puVar1 + 10,0);
    FUN_00b12cd0(puVar1 + 0x13,0xbf800000);
  // ... [truncated for register brevity] ...
```

#### `0x00B00CE0` — `fn_InitDisplayItem`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b00ce0`
- **Signature:** `undefined FUN_00b00ce0(void)`
- **Binary Size:** 48 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `143` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b00ce0(int param_1,undefined4 param_2)

{
  int iVar1;
  int iVar2;
  
  if (param_1 != 0) {
    iVar1 = param_1 + 0x28;
    iVar2 = 2;
    do {
      FUN_00b12ca0(iVar1,param_2);
      iVar1 = iVar1 + 0x24;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    *(char *)(param_1 + 0xb) = (char)param_2;
  }
  return;
}
```

#### `0x00B00D10` — `fn_DestroyScrollList`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b00d10`
- **Signature:** `undefined FUN_00b00d10(void)`
- **Binary Size:** 29 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `28` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b00d10(int param_1)

{
  if (param_1 != 0) {
    FUN_0094fce0(*(undefined4 *)(param_1 + 0x14));
    FUN_0045bc50(param_1);
  }
  return;
}
```

#### `0x00B00D70` — `fn_SetScrollScale`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b00d70`
- **Signature:** `undefined FUN_00b00d70(void)`
- **Binary Size:** 21 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `19` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b00d70(int param_1)

{
  if (param_1 != 0) {
    FUN_0094fa90();
    return;
  }
  return;
}
```

#### `0x00B011C0` — `fn_SetScrollFlag`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b011c0`
- **Signature:** `undefined FUN_00b011c0(void)`
- **Binary Size:** 44 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `48` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b011c0(int param_1,undefined4 param_2)

{
  if (param_1 != 0) {
    FUN_0094fa30(*(undefined4 *)(param_1 + 0x18),param_2);
    if ((char)param_2 == '\x01') {
      FUN_00b00ef0(param_1);
    }
  }
  return;
}
```

#### `0x00B01230` — `fn_CreateScrollList`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b01230`
- **Signature:** `undefined FUN_00b01230(void)`
- **Binary Size:** 660 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `26` | Callees: `10`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
uint * FUN_00b01230(uint param_1,int param_2)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint *puVar4;
  undefined4 local_8;
  undefined4 local_4;
  
  local_8 = 0;
  local_4 = 0;
  if (param_2 == 0) {
    return (uint *)0x0;
  }
  puVar1 = (uint *)thunk_FUN_00877070(1,0x2c);
  if ((int)param_1 < 0x14) {
    param_1 = param_1 & ((int)param_1 < 0) - 1;
  }
  else {
    param_1 = 0x13;
  }
  *puVar1 = param_1;
  uVar2 = FUN_0094fd20();
  puVar1[5] = uVar2;
  // ... [truncated for register brevity] ...
```

#### `0x00B07BC0` — `fn_SetLabel`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b07bc0`
- **Signature:** `undefined FUN_00b07bc0(void)`
- **Binary Size:** 62 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `7` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b07bc0(int param_1,int param_2,char *param_3)

{
  param_1 = param_2 * 0x400 + param_1;
  _strncpy((char *)(param_1 + 0xfc),param_3,0x400);
  _strncpy((char *)(param_1 + 0xcfc),param_3,0x400);
  return;
}
```

#### `0x00B07EA0` — `fn_DisplayReset`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b07ea0`
- **Signature:** `undefined FUN_00b07ea0(void)`
- **Binary Size:** 36 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `8` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00b07ea0(int param_1,int param_2)

{
  if ((param_2 == 0) && (*(int *)(param_1 + 0x18fc) == 1)) {
    return 0;
  }
  *(int *)(param_1 + 0x190c) = param_2;
  return 1;
}
```

#### `0x00B07F70` — `fn_ShowPanel`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b07f70`
- **Signature:** `undefined FUN_00b07f70(void)`
- **Binary Size:** 15 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b07f70(int param_1,undefined4 param_2)

{
  *(undefined4 *)(param_1 + 0xe8) = param_2;
  return;
}
```

#### `0x00B081D0` — `fn_FinalisePanelLayout`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b081d0`
- **Signature:** `undefined FUN_00b081d0(void)`
- **Binary Size:** 285 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b081d0(undefined4 *param_1)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 local_a4;
  undefined1 *local_a0;
  undefined4 local_9c;
  undefined4 local_98;
  undefined1 *local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined1 *local_88;
  undefined4 local_84;
  undefined4 local_80;
  undefined1 *local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined1 *local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined1 *local_64;
  undefined4 local_60;
  // ... [truncated for register brevity] ...
```

#### `0x00B08B40` — `orig_PanelRefresh`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `orig_PanelRefresh`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

#### `0x00B0AC40` — `orig_PanelNavUpdate`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0ac40`
- **Signature:** `undefined FUN_00b0ac40(void)`
- **Binary Size:** 12 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `8` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0ac40(void)

{
  FUN_00b09b80();
  return;
}
```

#### `0x00B0AC50` — `orig_DestroyPanel`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0ac50`
- **Signature:** `undefined FUN_00b0ac50(void)`
- **Binary Size:** 298 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `14` | Callees: `11`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0ac50(int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  int *piVar3;
  
  FUN_00b09b20(param_1);
  FUN_00b0f400(*(undefined4 *)(param_1 + 0x54));
  FUN_00b00d10(*(undefined4 *)(param_1 + 100));
  puVar2 = (undefined4 *)(param_1 + 0x58);
  iVar1 = 2;
  do {
    FUN_00b00400(*puVar2);
    puVar2 = puVar2 + 1;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  FUN_00b00400(*(undefined4 *)(param_1 + 0x60));
  piVar3 = (int *)(param_1 + 0x6c);
  iVar1 = 3;
  do {
    if (piVar3[3] != 0) {
      FUN_009553f0(piVar3[3]);
    }
    if (*piVar3 != 0) {
  // ... [truncated for register brevity] ...
```

#### `0x00B0AD80` — `fn_SetNavState`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0ad80`
- **Signature:** `undefined FUN_00b0ad80(void)`
- **Binary Size:** 1834 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0ad80(int param_1,int param_2,int param_3,int param_4)

{
  char cVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  int iVar5;
  char *pcVar6;
  int *piVar7;
  int iVar8;
  int iVar9;
  undefined4 uVar10;
  char *local_1a8;
  int local_1a4;
  int local_1a0;
  undefined4 *local_19c;
  int local_198;
  undefined4 local_194 [5];
  int local_180 [96];
  
  local_180[0x38] = 0;
  local_180[0x39] = 0x13;
  local_180[0x3a] = 0x14;
  local_180[0x3b] = 0x1f;
  // ... [truncated for register brevity] ...
```

#### `0x00B0BB50` — `fn_GetPanelRegistrySlot`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0bb50`
- **Signature:** `undefined FUN_00b0bb50(void)`
- **Binary Size:** 14 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `17` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_00b0bb50(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_1 != 0) {
    uVar1 = *(undefined4 *)(param_1 + 0x10);
  }
  return uVar1;
}
```

#### `0x00B0BBD0` — `fn_CreateDisplaySubObject`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0bbd0`
- **Signature:** `undefined FUN_00b0bbd0(void)`
- **Binary Size:** 147 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `30` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined2 *
FUN_00b0bbd0(undefined4 param_1,undefined4 param_2,undefined4 param_3,int param_4,undefined4 param_5
            )

{
  undefined2 *puVar1;
  int iVar2;
  
  puVar1 = (undefined2 *)thunk_FUN_00876fc0(0x28);
  if (param_4 == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = thunk_FUN_00877070(1,param_4);
  }
  if (puVar1 != (undefined2 *)0x0) {
    if ((param_4 == 0) || (iVar2 != 0)) {
      *(undefined4 *)(puVar1 + 4) = param_2;
      *(undefined4 *)(puVar1 + 2) = param_1;
      puVar1[1] = 0;
      *puVar1 = 0;
      *(undefined4 *)(puVar1 + 6) = param_3;
      *(int *)(puVar1 + 8) = iVar2;
      *(undefined4 *)(puVar1 + 10) = param_5;
      FUN_00b0c230(puVar1 + 0xc,puVar1);
  // ... [truncated for register brevity] ...
```

#### `0x00B0F270` — `fn_CreateScrollController`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0f270`
- **Signature:** `undefined FUN_00b0f270(void)`
- **Binary Size:** 381 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `15` | Callees: `8`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 * FUN_00b0f270(int param_1,float *param_2,int param_3,int param_4)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  float local_10;
  float local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_2 == (float *)0x0) {
    return (undefined4 *)0x0;
  }
  iVar3 = param_3 * param_4;
  if (0 < iVar3) {
    puVar1 = (undefined4 *)thunk_FUN_00877070(1,0x28);
    puVar1[8] = iVar3;
    uVar2 = thunk_FUN_009b08a0(iVar3 + 1);
    puVar1[1] = uVar2;
    uVar2 = FUN_0094f750();
    *puVar1 = uVar2;
    local_10 = *param_2;
    local_c = param_2[1];
  // ... [truncated for register brevity] ...
```

#### `0x00B0F400` — `fn_DestroyScrollCtrl`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0f400`
- **Signature:** `undefined FUN_00b0f400(void)`
- **Binary Size:** 86 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `14` | Callees: `4`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0f400(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  
  if (param_1 != (undefined4 *)0x0) {
    iVar2 = 0;
    if (0 < (int)param_1[8]) {
      iVar1 = 0;
      do {
        FUN_00b0fa40(param_1[9] + iVar1);
        iVar2 = iVar2 + 1;
        iVar1 = iVar1 + 0x14;
      } while (iVar2 < (int)param_1[8]);
    }
    FUN_0045bc50(param_1[9]);
    FUN_0094f780(*param_1);
    thunk_FUN_009b0d40(param_1[1]);
    FUN_0045bc50(param_1);
  }
  return;
}
```

#### `0x00B0F4B0` — `fn_SetScrollBounds`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0f4b0`
- **Signature:** `undefined FUN_00b0f4b0(void)`
- **Binary Size:** 135 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `11` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0f4b0(int param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  if (param_1 != 0) {
    local_8 = param_2;
    iVar1 = 0;
    local_10 = 0;
    local_c = 0;
    local_4 = param_3;
    if (0 < *(int *)(param_1 + 0x20)) {
      iVar2 = 0;
      do {
        FUN_00951430(*(undefined4 *)(*(int *)(param_1 + 0x24) + 4 + iVar2),&local_10);
        FUN_00951430(*(undefined4 *)(*(int *)(param_1 + 0x24) + 8 + iVar2),&local_10);
        iVar1 = iVar1 + 1;
        iVar2 = iVar2 + 0x14;
      } while (iVar1 < *(int *)(param_1 + 0x20));
    }
  // ... [truncated for register brevity] ...
```

#### `0x00B0F540` — `fn_SetScrollCtrlScale`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0f540`
- **Signature:** `undefined FUN_00b0f540(void)`
- **Binary Size:** 20 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `12` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0f540(int param_1)

{
  if (param_1 != 0) {
    FUN_0094f6a0();
    return;
  }
  return;
}
```

#### `0x00B0F5A0` — `fn_LinkDisplayObjects`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0f5a0`
- **Signature:** `undefined FUN_00b0f5a0(void)`
- **Binary Size:** 84 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `9` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0f5a0(int param_1,int param_2,uint param_3)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  
  if ((param_1 != 0) && (param_2 != 0)) {
    uVar1 = *(uint *)(param_1 + 0x20);
    if ((int)param_3 <= (int)uVar1) {
      uVar1 = ((int)param_3 < 0) - 1 & param_3;
    }
    iVar2 = 0;
    if (0 < (int)uVar1) {
      iVar3 = 0;
      do {
        FUN_00b0fa80(*(int *)(param_1 + 0x24) + iVar3,*(undefined4 *)(param_2 + iVar2 * 4));
        iVar2 = iVar2 + 1;
        iVar3 = iVar3 + 0x14;
      } while (iVar2 < (int)uVar1);
    }
  }
  return;
}
```

#### `0x00B0FC60` — `fn_SetCellPosition`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0fc60`
- **Signature:** `undefined FUN_00b0fc60(void)`
- **Binary Size:** 102 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0fc60(int *param_1,float *param_2)

{
  float local_10;
  float local_c;
  float local_8;
  undefined4 local_4;
  
  if ((param_1 != (int *)0x0) && (param_2 != (float *)0x0)) {
    FUN_00951330(param_1[1],param_2);
    local_8 = param_2[2];
    local_10 = *(float *)(*param_1 + 0x14) * 0.046875 + *param_2;
    local_4 = 0;
    local_c = *(float *)(*param_1 + 0x18) * 0.046875 + param_2[1];
    FUN_00951330(param_1[2],&local_10);
  }
  return;
}
```

#### `0x00B0FCD0` — `orig_GetCellPosition`
- **Subsystem:** UI, Menus & Presentation
- **Function Symbol:** `FUN_00b0fcd0`
- **Signature:** `undefined FUN_00b0fcd0(void)`
- **Binary Size:** 83 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 * FUN_00b0fcd0(int *param_1,undefined4 *param_2)

{
  undefined4 local_10;
  undefined4 local_c;
  
  if ((param_1 != (int *)0x0) && (param_2 != (undefined4 *)0x0)) {
    FUN_009519f0(param_1[1],&local_10);
    param_2[1] = local_c;
    *param_2 = local_10;
    param_2[2] = *(undefined4 *)(*param_1 + 0x14);
    param_2[3] = *(undefined4 *)(*param_1 + 0x18);
    return param_2;
  }
  return (undefined4 *)0x0;
}
```

### 3.6 Subsystem: Scene Graph & Skeletal Bones

#### `0x0094E380` — `fn_ResolveBone`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_0094e380`
- **Signature:** `undefined FUN_0094e380(void)`
- **Binary Size:** 63 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `32` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_0094e380(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar1 = DAT_03a67b94;
  if (DAT_03a67b94 == 0) {
    return;
  }
  do {
    iVar2 = 0;
    if (0 < *(int *)(iVar1 + 4)) {
      piVar3 = *(int **)(iVar1 + 8);
      do {
        if (*(short *)(*(int *)(*piVar3 + 4) + 4) == (short)((uint)param_1 >> 0x10)) {
          return;
        }
        iVar2 = iVar2 + 1;
        piVar3 = piVar3 + 1;
      } while (iVar2 < *(int *)(iVar1 + 4));
    }
    piVar3 = (int *)(iVar1 + 0x10);
    iVar1 = *piVar3;
  // ... [truncated for register brevity] ...
```

#### `0x009502B0` — `fn_DestroyNodeChild`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009502b0`
- **Signature:** `undefined FUN_009502b0(void)`
- **Binary Size:** 66 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `271` | Callees: `7`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009502b0(int param_1)

{
  if (param_1 != 0) {
    FUN_0095c8d0(param_1);
    FUN_009528a0(*(undefined4 *)(param_1 + 0x2c),param_1);
    FUN_0095c7f0(param_1);
    FUN_0094efd0(*(undefined4 *)(param_1 + 0x34));
    FUN_0094f280(*(undefined4 *)(param_1 + 0x30));
    FUN_009500a0(param_1);
    FUN_0045bc50(param_1);
  }
  return;
}
```

#### `0x00950300` — `fn_NodePropA`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950300`
- **Signature:** `undefined FUN_00950300(void)`
- **Binary Size:** 45 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `454` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00950300(int param_1,short param_2)

{
  if ((param_1 != 0) && (*(undefined2 *)(param_1 + 0x4c) = 0, *(short *)(param_1 + 0x4e) != param_2)
     ) {
    FUN_00950260();
    FUN_0095c990(param_1);
  }
  return;
}
```

#### `0x00950350` — `fn_SetNodePosition`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950350`
- **Signature:** `undefined FUN_00950350(void)`
- **Binary Size:** 98 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `150` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00950350(int param_1,float *param_2)

{
  if (((param_1 != 0) && (param_2 != (float *)0x0)) &&
     ((*(float *)(param_1 + 0x6c) != *param_2 ||
      ((*(float *)(param_1 + 0x70) != param_2[1] || (*(float *)(param_1 + 0x74) != param_2[2]))))))
  {
    *(float *)(param_1 + 0x6c) = *param_2;
    *(float *)(param_1 + 0x70) = param_2[1];
    *(float *)(param_1 + 0x74) = param_2[2];
    *(float *)(param_1 + 0x78) = param_2[3];
    *(byte *)(param_1 + 0xa2) = *(byte *)(param_1 + 0xa2) | 1;
  }
  return;
}
```

#### `0x009503C0` — `fn_NodePropC`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009503c0`
- **Signature:** `undefined FUN_009503c0(void)`
- **Binary Size:** 30 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `110` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009503c0(int param_1,int param_2)

{
  if ((param_1 != 0) && (*(char *)(param_1 + 0xa4) = (char)param_2, param_2 == 2)) {
    *(undefined2 *)(param_1 + 0x4c) = 0;
  }
  return;
}
```

#### `0x009503E0` — `fn_NodePropD`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009503e0`
- **Signature:** `undefined FUN_009503e0(void)`
- **Binary Size:** 19 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `20` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009503e0(int param_1,undefined1 param_2)

{
  if (param_1 != 0) {
    *(undefined1 *)(param_1 + 0xa3) = param_2;
  }
  return;
}
```

#### `0x00950490` — `fn_GetBoneWorldPosition`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950490`
- **Signature:** `undefined FUN_00950490(void)`
- **Binary Size:** 225 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `505` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
float * FUN_00950490(int param_1,float *param_2,short param_3)

{
  float fVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined2 local_28;
  undefined2 local_26;
  undefined2 local_24;
  undefined2 local_22;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  
  if ((param_1 != 0) && (param_2 != (float *)0x0)) {
    iVar2 = *(int *)(param_1 + 0x1c);
    FUN_00955320(iVar2 + 4);
    iVar3 = *(int *)(iVar2 + 0x14);
    iVar4 = 0;
    if (*(byte *)(iVar2 + 2) != 0) {
      do {
  // ... [truncated for register brevity] ...
```

#### `0x00950580` — `fn_GetBoneScale`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950580`
- **Signature:** `undefined FUN_00950580(void)`
- **Binary Size:** 89 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `300` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
float10 FUN_00950580(int param_1,short param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 == 0) {
    return (float10)0.0;
  }
  iVar1 = *(int *)(param_1 + 0x1c);
  iVar2 = *(int *)(iVar1 + 0x14);
  iVar3 = 0;
  if (*(byte *)(iVar1 + 2) != 0) {
    do {
      if (param_2 == *(short *)(iVar2 + 2)) {
        return (float10)(int)((int)*(short *)(iVar2 + 8) + (uint)*(ushort *)(iVar1 + 8)) +
               (float10)*(float *)(param_1 + 0x74);
      }
      iVar3 = iVar3 + 1;
      iVar2 = iVar2 + 0xe;
    } while (iVar3 < (int)(uint)*(byte *)(iVar1 + 2));
  }
  return (float10)0.0;
}
```

#### `0x00950A00` — `orig_CreateChildNode`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950a00`
- **Signature:** `undefined FUN_00950a00(void)`
- **Binary Size:** 317 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `14`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00950a00(int param_1,char param_2)

{
  int iVar1;
  undefined1 uVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar1 = param_1;
  if (param_1 == 0) {
    return 0;
  }
  iVar3 = thunk_FUN_00877070(1,0xa8);
  *(int *)(iVar3 + 0x2c) = iVar1;
  iVar4 = FUN_00952770(iVar1,0);
  if (iVar4 != 0) {
    *(int *)(iVar3 + 0x48) = iVar4;
    *(undefined4 *)(iVar3 + 0x44) = 0;
    FUN_00950260();
    *(undefined1 *)(iVar3 + 0xa4) = 0;
  // ... [truncated for register brevity] ...
```

#### `0x00950B40` — `fn_GetChildNode`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00950b40`
- **Signature:** `undefined FUN_00950b40(void)`
- **Binary Size:** 16 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `77` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00950b40(undefined4 param_1)

{
  FUN_00950a00(param_1,1);
  return;
}
```

#### `0x00951330` — `orig_SetNodePosition`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00951330`
- **Signature:** `undefined FUN_00951330(void)`
- **Binary Size:** 113 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `97` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00951330(int param_1,float *param_2)

{
  if (((param_1 != 0) && (param_2 != (float *)0x0)) &&
     ((*(float *)(param_1 + 0xc) != *param_2 ||
      (((*(float *)(param_1 + 0x10) != param_2[1] || (*(float *)(param_1 + 0x14) != param_2[2])) ||
       (*(float *)(param_1 + 0x18) != param_2[3])))))) {
    *(float *)(param_1 + 0xc) = *param_2;
    *(float *)(param_1 + 0x10) = param_2[1];
    *(float *)(param_1 + 0x14) = param_2[2];
    *(float *)(param_1 + 0x18) = param_2[3];
    *(byte *)(param_1 + 0x11a) = *(byte *)(param_1 + 0x11a) | 1;
  }
  return;
}
```

#### `0x00952180` — `fn_CreateRenderNode`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00952180`
- **Signature:** `undefined FUN_00952180(void)`
- **Binary Size:** 31 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `190` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00952180(int param_1)

{
  int iVar1;
  
  iVar1 = DAT_03a67ca0;
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if (*(int *)(iVar1 + 8) == param_1) break;
    iVar1 = *(int *)(iVar1 + 4);
  }
  return iVar1;
}
```

#### `0x00955320` — `fn_TransformNode`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00955320`
- **Signature:** `undefined FUN_00955320(void)`
- **Binary Size:** 198 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00955320(int param_1)

{
  int iVar1;
  short *psVar2;
  float *pfVar3;
  int unaff_EBX;
  int iVar4;
  int unaff_EDI;
  
  iVar1 = 0;
  if (3 < unaff_EDI) {
    iVar4 = (unaff_EDI - 4U >> 2) + 1;
    iVar1 = iVar4 * 4;
    psVar2 = (short *)(param_1 + 4);
    pfVar3 = (float *)(unaff_EBX + 8);
    do {
      iVar4 = iVar4 + -1;
      pfVar3[-2] = (float)(int)psVar2[-2] * 0.0625;
      pfVar3[-1] = (float)(int)psVar2[-1] * 0.0625;
      *pfVar3 = (float)(int)*psVar2 * 0.0625;
      pfVar3[1] = (float)(int)psVar2[1] * 0.0625;
      psVar2 = psVar2 + 4;
      pfVar3 = pfVar3 + 4;
    } while (iVar4 != 0);
  // ... [truncated for register brevity] ...
```

#### `0x009553F0` — `fn_DestroyBoneAttach`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009553f0`
- **Signature:** `undefined FUN_009553f0(void)`
- **Binary Size:** 25 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `22` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009553f0(int param_1)

{
  FUN_00955c20(*(undefined4 *)(param_1 + 0xc));
  FUN_0095c880(param_1);
  return;
}
```

#### `0x009554A0` — `fn_LookupBone`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009554a0`
- **Signature:** `undefined FUN_009554a0(void)`
- **Binary Size:** 36 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `35` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_009554a0(int param_1,undefined4 param_2)

{
  int iVar1;
  
  if (param_1 != 0) {
    iVar1 = FUN_0095c900(param_1,1,param_2);
    if (iVar1 != 0) {
      return *(undefined4 *)(iVar1 + 0x14);
    }
  }
  return 0xffffffff;
}
```

#### `0x009557E0` — `fn_AttachBone`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_009557e0`
- **Signature:** `undefined FUN_009557e0(void)`
- **Binary Size:** 602 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `19` | Callees: `13`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 * FUN_009557e0(int param_1,int param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 uVar5;
  bool bVar6;
  float10 fVar7;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10 [8];
  float local_8;
  undefined4 local_4;
  
  if ((param_1 != 0) && (param_2 != 0)) {
    iVar2 = FUN_0095c900(param_1,1,param_3);
    if (iVar2 != 0) {
      iVar3 = FUN_0094e3c0(param_2,*(undefined4 *)(iVar2 + 0x14));
      if (iVar3 != 0) {
        puVar4 = (undefined4 *)FUN_0095c820(param_1);
        puVar4[1] = param_3;
        *puVar4 = 0;
        puVar4[2] = iVar2;
  // ... [truncated for register brevity] ...
```

#### `0x00955DB0` — `fn_SetBoneVisibility`
- **Subsystem:** Scene Graph & Skeletal Bones
- **Function Symbol:** `FUN_00955db0`
- **Signature:** `undefined FUN_00955db0(void)`
- **Binary Size:** 16 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `99` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00955db0(int param_1,undefined1 param_2)

{
  if (param_1 != 0) {
    *(undefined1 *)(param_1 + 0x78) = param_2;
  }
  return;
}
```

### 3.7 Subsystem: Engine Memory & Utility

#### `0x0045BC50` — `fn_GameFree`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_0045bc50`
- **Signature:** `undefined FUN_0045bc50(void)`
- **Binary Size:** 18 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `362` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_0045bc50(int param_1)

{
  if (param_1 != 0) {
    FUN_00876f20();
    return;
  }
  return;
}
```

#### `0x00877070` — `fn_GameAlloc`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_00877070`
- **Signature:** `undefined FUN_00877070(void)`
- **Binary Size:** 211 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `0` | Callees: `5`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
int FUN_00877070(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  iVar1 = FUN_008839a0();
  iVar3 = 1;
  if (1 < DAT_012436d4) {
    do {
      if (((iVar3 != iVar1) && (iVar2 = FUN_00883a30(iVar3,param_1 * param_2), iVar2 != 0)) &&
         (iVar2 = FUN_00884620(iVar3,param_1,param_2), iVar2 != 0)) {
        FUN_00883a50(iVar3,iVar2);
        return iVar2;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < DAT_012436d4);
  }
  iVar3 = FUN_00883b20(iVar1);
  if (iVar3 == 0) {
    iVar1 = FUN_008839a0();
  }
  iVar2 = FUN_00884620(iVar1,param_1,param_2);
  iVar3 = iVar1;
  // ... [truncated for register brevity] ...
```

#### `0x0094F2C0` — `fn_GetStringByHandle`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_0094f2c0`
- **Signature:** `undefined FUN_0094f2c0(void)`
- **Binary Size:** 41 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `551` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_0094f2c0(undefined4 param_1)

{
  int iVar1;
  
  iVar1 = FUN_0094e380(param_1);
  if (iVar1 != 0) {
    iVar1 = FUN_0094e3c0(iVar1,param_1);
    if (iVar1 != 0) {
      return *(undefined4 *)(iVar1 + 8);
    }
  }
  return 0;
}
```

#### `0x0094F420` — `fn_ReleaseResource`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_0094f420`
- **Signature:** `undefined FUN_0094f420(void)`
- **Binary Size:** 72 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `22` | Callees: `2`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
undefined4 FUN_0094f420(int *param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  
  if ((param_1 != (int *)0x0) && (param_2 != 0)) {
    iVar2 = *param_1;
    do {
      if (iVar2 == param_2) {
        uVar1 = *(undefined4 *)(param_2 + 8);
        FUN_0094f2f0();
        FUN_0094efd0(param_2);
        *(byte *)((int)param_1 + 0x2e) = *(byte *)((int)param_1 + 0x2e) | 2;
        return uVar1;
      }
      iVar2 = *(int *)(iVar2 + 8);
    } while (iVar2 != 0);
  }
  return 0;
}
```

#### `0x0094F780` — `fn_FinalizeResOwner`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_0094f780`
- **Signature:** `undefined FUN_0094f780(void)`
- **Binary Size:** 83 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `82` | Callees: `5`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_0094f780(int *param_1)

{
  int iVar1;
  
  if (param_1 != (int *)0x0) {
    if ((short)param_1[10] != 0) {
      iVar1 = *param_1;
      if (param_1[4] == 4) {
        do {
          iVar1 = FUN_0094f4d0(param_1,iVar1);
        } while (iVar1 != 0);
      }
      else {
        do {
          iVar1 = FUN_0094f420(param_1,iVar1);
        } while (iVar1 != 0);
      }
    }
    FUN_0094f280(param_1[2]);
    FUN_0094efd0(param_1[3]);
    FUN_0045bc50(param_1);
  }
  return;
}
```

#### `0x009C2780` — `fn_LoadUIResource`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_009c2780`
- **Signature:** `undefined FUN_009c2780(void)`
- **Binary Size:** 324 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `13` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009c2780(undefined4 param_1)

{
  int iVar1;
  
  FUN_00afe5e0(1);
  iVar1 = FUN_008c0180();
  if (iVar1 != 0) {
    FUN_00afe680(5,param_1,1);
    return;
  }
  switch(DAT_03be12c9) {
  case 0:
  case 7:
    if (DAT_03becc60 != 4) {
      FUN_00afe680(0,param_1,1);
      return;
    }
    goto LAB_009c28a2;
  case 1:
    if (DAT_03becc60 == 7) {
      FUN_00afe680(4,param_1,1);
      return;
    }
    goto LAB_009c2865;
  // ... [truncated for register brevity] ...
```

#### `0x00B0BB60` — `fn_PreFreeSubObj`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_00b0bb60`
- **Signature:** `undefined FUN_00b0bb60(void)`
- **Binary Size:** 21 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `11` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0bb60(int param_1)

{
  if (param_1 != 0) {
    FUN_00b0c250();
    return;
  }
  return;
}
```

#### `0x00B0BB80` — `fn_FreeSubObj`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `FUN_00b0bb80`
- **Signature:** `undefined FUN_00b0bb80(void)`
- **Binary Size:** 66 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `12` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b0bb80(int param_1)

{
  int iVar1;
  
  if (param_1 != 0) {
    iVar1 = FUN_00b0c1a0(param_1 + 0x18);
    if (iVar1 != 0) {
      FUN_00970490();
      return;
    }
    if (*(int *)(param_1 + 0x10) != 0) {
      FUN_0045bc50(*(int *)(param_1 + 0x10));
    }
    FUN_0045bc50(param_1);
  }
  return;
}
```

#### `0x00D5BF00` — `fn_GetStringByHandle (table entry)`
- **Subsystem:** Engine Memory & Utility
- **Function Symbol:** `fn_GetStringByHandle (table entry)`
- **Signature:** `undefined`
- **Binary Size:** Unknown bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `Unknown` | Callees: `Unknown`
- **Confidence Level:** `HYPOTHESIS`
- **Runtime Verification:** `Verified (Code Boundary Match)`

### 3.8 Subsystem: Other / Unclassified

#### `0x009B8980` — `fn_InputEvent`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_009b8980`
- **Signature:** `undefined FUN_009b8980(void)`
- **Binary Size:** 194 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `718` | Callees: `6`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_009b8980(undefined4 param_1)

{
  uint uVar1;
  int iVar2;
  undefined4 in_ECX;
  uint uVar3;
  
  FUN_00401000(DAT_03a6b904,"SD_call",param_1);
  uVar1 = FUN_009b9850(param_1);
  iVar2 = FUN_009b85d0();
  if (iVar2 == 1) {
    _DAT_03a6b894 = 0x7f0040;
    _DAT_03a6b890 = 1;
    FUN_008aa4e0(uVar1,0x7f0040,1,0);
  }
  else {
    if (iVar2 == 2) {
      FUN_009ba200(uVar1 & 0x3ff);
      uVar3 = uVar1 >> 0x19 & 0xf;
      if ((uVar3 == 10) || (uVar3 == 0xb)) {
        in_ECX = 0;
      }
  // ... [truncated for register brevity] ...
```

#### `0x009DCB00` — `fn_PostLoadUpdate`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_009dcb00`
- **Signature:** `undefined FUN_009dcb00(void)`
- **Binary Size:** 63 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `2` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_009dcb00(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = 0;
  iVar3 = 0;
  do {
    uVar1 = DAT_03be6318;
    (&DAT_03a79e34)[iVar3] = 0xffff;
    iVar2 = FUN_00b08050(uVar1,iVar3);
    if (iVar2 == 1) {
      (&DAT_03a79e34)[iVar4] = (short)iVar3;
      iVar4 = iVar4 + 1;
    }
    iVar3 = iVar3 + 1;
  } while (iVar3 < 0xcc);
  _DAT_03a79fcc = iVar4;
  return;
}
```

#### `0x009EDFD0` — `fn_MLClubScreenInit`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_009edfd0`
- **Signature:** `undefined FUN_009edfd0(void)`
- **Binary Size:** 99 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `5` | Callees: `6`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_009edfd0(undefined4 param_1)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = FUN_00b08030(DAT_03be6314);
  if (iVar1 == 1) {
    uVar2 = FUN_00b0f460(DAT_03be6300,1,param_1);
    FUN_00b0fd60(uVar2);
    FUN_009d1590(DAT_03be69c4,1,param_1);
  }
  thunk_FUN_00950330(*(undefined4 *)(DAT_03be6308 + 8),param_1);
  FUN_00b07e80(DAT_03be6314,param_1);
  return;
}
```

#### `0x00A65AF0` — `fn_AvailSide0`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_00a65af0`
- **Signature:** `undefined FUN_00a65af0(void)`
- **Binary Size:** 89 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `0` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00a65af0(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_00a63550();
  iVar2 = FUN_00a772e0();
  if ((((iVar2 != 0) && (iVar2 + 0xf99c != 0)) && (-1 < iVar1)) &&
     ((iVar1 < 4 && (iVar1 = (iVar1 + 0x7c) * 0x10 + iVar2 + 0xf99c, iVar1 != 0)))) {
    FUN_00a65a00((int)*(char *)(iVar1 + 8),param_1);
    return;
  }
  FUN_00a65a00(2,param_1);
  return;
}
```

#### `0x00A65B50` — `fn_AvailSide1`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_00a65b50`
- **Signature:** `undefined FUN_00a65b50(void)`
- **Binary Size:** 92 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `0` | Callees: `3`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00a65b50(undefined4 param_1)

{
  int iVar1;
  int iVar2;
  
  iVar1 = FUN_00a63550();
  iVar2 = FUN_00a772e0();
  if ((((iVar2 != 0) && (iVar2 + 0xf99c != 0)) && (-1 < iVar1)) &&
     (((iVar1 < 4 && (iVar1 = (iVar1 + 0x7c) * 0x10 + iVar2 + 0xf99c, iVar1 != 0)) &&
      (*(char *)(iVar1 + 8) == '\0')))) {
    FUN_00a65a00(1,param_1);
    return;
  }
  FUN_00a65a00(0,param_1);
  return;
}
```

#### `0x00B004A0` — `fn_SetItemScale`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_00b004a0`
- **Signature:** `undefined FUN_00b004a0(void)`
- **Binary Size:** 53 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `76` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b004a0(int param_1,float param_2)

{
  if (param_1 != 0) {
    FUN_00b12cd0(param_1 + 0x28,param_2);
    FUN_00b12cd0(param_1 + 0x4c,param_2 - 1.0);
  }
  return;
}
```

#### `0x00B07C40` — `fn_SetPage`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_00b07c40`
- **Signature:** `undefined FUN_00b07c40(void)`
- **Binary Size:** 30 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `20` | Callees: `0`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b07c40(int param_1,int param_2,undefined4 param_3)

{
  *(undefined4 *)(param_1 + 0x1918 + param_2 * 0x18) = param_3;
  *(undefined4 *)(param_1 + 0x48) = 1;
  return;
}
```

#### `0x00B09B20` — `fn_PreDestroyCleanup`
- **Subsystem:** Other / Unclassified
- **Function Symbol:** `FUN_00b09b20`
- **Signature:** `undefined FUN_00b09b20(void)`
- **Binary Size:** 96 bytes (Exact Entry: `True`)
- **Call Relationships:** Callers: `1` | Callees: `1`
- **Confidence Level:** `VERIFIED`
- **Runtime Verification:** `Verified (Code Boundary Match)`
```c
void FUN_00b09b20(int *param_1)

{
  int iVar1;
  int *piVar2;
  
  iVar1 = 0xe;
  piVar2 = param_1;
  do {
    piVar2 = piVar2 + 1;
    thunk_FUN_00b141f0(*piVar2);
    *piVar2 = 0;
    iVar1 = iVar1 + -1;
  } while (iVar1 != 0);
  iVar1 = *param_1 * 4;
  *(undefined4 *)(&DAT_03b49140 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b49160 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b49180 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b491a0 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b491c0 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b491e0 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b49200 + iVar1) = 0;
  *(undefined4 *)(&DAT_03b49220 + iVar1) = 0;
  return;
}
```
