#!/usr/bin/env python3
"""
Verify PESMod's hardcoded game addresses against a user-supplied pes6.exe.

The PESMod source (github.com/PanosPasch/pesmod) hardcodes ~236 game
addresses discovered with Ghidra against one specific retail build.
This tool parses a PE, resolves each address, and reports whether the
address lands on a plausible function entry in *this* build.

Usage:
    python verify_addresses.py <pes6.exe> [--map addr_map.json]
    python verify_addresses.py --dump <pes6.exe> <addr> [<addr> ...]
"""
import argparse
import json
import pathlib
import struct
import sys

# Documented PESMod hook targets, keyed by VA -> label.
# Source: src/hooks/*.{cpp,h} and docs/INTERNALS.md (2026-09-11, 21021eb).
PESMOD_ADDRESSES = {
    0x00456780: "fnAddr (player_hooks pattern-scan fallback)",
    0x0045BC50: "fn_GameFree",
    0x005C18B0: "fn_GetWeatherCondition",
    0x005EE610: "ADDR_MenuPageFinish",
    0x005FC1E0: "ADDR_CheckTeamUnlocked",
    0x006094B0: "ADDR_MenuPageSetup",
    0x006094F0: "fn_menu_LoadPageTeams",
    0x00800480: "(unlabeled)",
    0x00861640: "fn_GetLeagueString",
    0x00861850: "(unlabeled)",
    0x00861AD0: "orig_SetTeamPlayerAttr",
    0x00861B90: "orig_GetPlayerRecord (worker)",
    0x00861D20: "orig_GetTeamPlayerID (public wrapper)",
    0x00862200: "orig_GetTeamPlayerAttr",
    0x00862710: "orig_GetTeamPlayerID",
    0x00862C10: "orig_GetPlayerRecord",
    0x00863570: "orig_BuildLeaguePanelVisualSlots",
    0x00863660: "(unlabeled)",
    0x008646B0: "fn_ResolveTeamID",
    0x00864EF0: "fn_GetClubTeamKitBase",
    0x00865100: "fn_GetNationalTeamKitBase",
    0x00865240: "fn_GetTeamKitData",
    0x00865380: "orig_GetTeamKitDataB",
    0x00865430: "fn_GetTeamKitDataC",
    0x008654D0: "orig_IsTeamKitEdited",
    0x00866D80: "fn_GetTeamDataType",
    0x00877070: "fn_GameAlloc",
    0x008C0180: "fn_GetModeFlag",
    0x008C6170: "ADDR_GetMasterLeagueLoadedTeam",
    0x0094E380: "fn_ResolveBone",
    0x0094F2C0: "fn_GetStringByHandle",
    0x0094F420: "fn_ReleaseResource",
    0x0094F780: "fn_FinalizeResOwner",
    0x009502B0: "fn_DestroyNodeChild",
    0x00950300: "fn_NodePropA",
    0x00950350: "fn_SetNodePosition",
    0x009503C0: "fn_NodePropC",
    0x009503E0: "fn_NodePropD",
    0x00950490: "fn_GetBoneWorldPosition",
    0x00950580: "fn_GetBoneScale",
    0x009509B0: "fn_SetNodeTexture",
    0x00950A00: "orig_CreateChildNode",
    0x00950B40: "fn_GetChildNode",
    0x00951330: "orig_SetNodePosition",
    0x009519F0: "fn_LabelNodeTransform",
    0x00952180: "fn_CreateRenderNode",
    0x00955320: "fn_TransformNode",
    0x009553F0: "fn_DestroyBoneAttach",
    0x009554A0: "fn_LookupBone",
    0x009557E0: "fn_AttachBone",
    0x00955DB0: "fn_SetBoneVisibility",
    0x00966C70: "fn_HasCustomFormation",
    0x00966F70: "orig_FillKitTeamSlot",
    0x00967170: "orig_FillKitVariantSlot",
    0x00968700: "orig_Match_SetupKitData",
    0x00968960: "orig_Match_SetupKitData2",
    0x00968E70: "orig_ExtractKitColor",
    0x009691D0: "fn_LoadTeamData_EditedKit",
    0x009692D0: "fn_LoadTeamData_Normal",
    0x00969490: "fn_LoadTeamData_MLDefault",
    0x00969530: "fn_LoadTeamData_CustomFormation",
    0x00969610: "orig_LoadTeamKitDataBySlot",
    0x00969790: "orig_LoadBothTeamKitData",
    0x009B0390: "orig_BadgeRenderCdecl",
    0x009B8980: "fn_InputEvent",
    0x009C2720: "fn_SetupNavMapping",
    0x009C2780: "fn_LoadUIResource",
    0x009C39C0: "fn_SetNavContext",
    0x009CC4B0: "fn_IsTeamUnlocked",
    0x009D2880: "fn_GetLeagueSelector",
    0x009D2EC0: "ADDR_SetLeagueTeamCount",
    0x009D3BC0: "fn_GetSelectedCupIdx",
    0x009DAB20: "fn_LoadLeagueLogo",
    0x009DCB00: "fn_PostLoadUpdate",
    0x009E25A0: "orig_LoadClubTeamSlots",
    0x009ED730: "orig_LoadClubTeamSlots",
    0x009EDC40: "fn_InitClubDisplayState",
    0x009EDFD0: "fn_MLClubScreenInit",
    0x009EE940: "orig_SetTeamList",
    0x00A4F0A0: "fn_InitPlayerState",
    0x00A4F120: "fn_SetPlayerActive",
    0x00A4F150: "fn_SetPlayerCursorType",
    0x00A4F3B0: "fn_ResetPlayerSelection",
    0x00A4F520: "fn_SetPlayerInputMode",
    0x00A4F840: "fn_GetPlayerState",
    0x00A65AF0: "fn_AvailSide0",
    0x00A65B50: "fn_AvailSide1",
    0x00AA5D48: "ADDR_MenuFlagsVar",
    0x00AF9E00: "fn_InitDisplaySubObject",
    0x00B00400: "fn_DestroyDisplayItem",
    0x00B004A0: "fn_SetItemScale",
    0x00B00920: "fn_CreateDisplayItem",
    0x00B00CE0: "fn_InitDisplayItem",
    0x00B00D10: "fn_DestroyScrollList",
    0x00B00D70: "fn_SetScrollScale",
    0x00B011C0: "fn_SetScrollFlag",
    0x00B01230: "fn_CreateScrollList",
    0x00B07B00: "fn_SetTeamList",
    0x00B07BC0: "fn_SetLabel",
    0x00B07C40: "fn_SetPage",
    0x00B07EA0: "fn_DisplayReset",
    0x00B07F70: "fn_ShowPanel",
    0x00B081D0: "fn_FinalisePanelLayout",
    0x00B083B0: "orig_BuildLeaguePanelVisualSlots",
    0x00B08B40: "orig_PanelRefresh",
    0x00B09050: "orig_CreateLeagueSelectionPanel",
    0x00B09590: "fn_SetSelectedTeam",
    0x00B09B20: "fn_PreDestroyCleanup",
    0x00B0AC40: "orig_PanelNavUpdate",
    0x00B0AC50: "orig_DestroyPanel",
    0x00B0AD80: "fn_SetNavState",
    0x00B0BB50: "fn_GetPanelRegistrySlot",
    0x00B0BB60: "fn_PreFreeSubObj",
    0x00B0BB80: "fn_FreeSubObj",
    0x00B0BBD0: "fn_CreateDisplaySubObject",
    0x00B0F270: "fn_CreateScrollController",
    0x00B0F400: "fn_DestroyScrollCtrl",
    0x00B0F4B0: "fn_SetScrollBounds",
    0x00B0F540: "fn_SetScrollCtrlScale",
    0x00B0F5A0: "fn_LinkDisplayObjects",
    0x00B0F8B0: "orig_BadgeSet",
    0x00B0F990: "orig_BadgeRender",
    0x00B0FA80: "orig_BadgeRender",
    0x00B0FC60: "fn_SetCellPosition",
    0x00B0FCD0: "orig_GetCellPosition",
    0x00B15700: "fn_IsOnlineMode",
    0x00B3BEB0: "orig_CrestResolve",
    0x00C97340: "RE_GetAliasTeamData",
    0x00D5BF30: "fn_GetLeagueString (table entry)",
    0x00D5BF00: "fn_GetStringByHandle (table entry)",
}

# x86 function-entry prologues worth flagging as "plausible entry".
PROLOGUES = {
    b"\x55\x8b\xec": "push ebp; mov ebp,esp",
    b"\x53": "push ebx",
    b"\x56": "push esi",
    b"\x57": "push edi",
    b"\x83\xec": "sub esp,imm8",
    b"\x81\xec": "sub esp,imm32",
    b"\x6a": "push imm8",
    b"\x68": "push imm32",
    b"\xe8": "call rel32",
    b"\xe9": "jmp rel32",
    b"\x8b\xff": "mov edi,edi",
    b"\xcc": "int3 (padding)",
    b"\x00": "null (data/zero)",
}


class PE:
    def __init__(self, path):
        self.path = pathlib.Path(path)
        self.data = self.path.read_bytes()
        if self.data[:2] != b"MZ":
            raise ValueError("not an MZ executable")
        e_lfanew = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
            raise ValueError("not a PE image")
        coff = e_lfanew + 4
        self.machine, self.n_sections, _, _, _, self.opt_size, self.chars = struct.unpack_from(
            "<HHIIIHH", self.data, coff)
        opt = coff + 20
        self.magic = struct.unpack_from("<H", self.data, opt)[0]
        self.pe32_plus = self.magic == 0x20B
        if self.pe32_plus:
            self.image_base = struct.unpack_from("<Q", self.data, opt + 24)[0]
        else:
            self.image_base = struct.unpack_from("<I", self.data, opt + 28)[0]
        self.entry_rva = struct.unpack_from("<I", self.data, opt + 16)[0]
        self.size_of_image = struct.unpack_from("<I", self.data, opt + 56)[0]
        self.section_align = struct.unpack_from("<I", self.data, opt + 32)[0]
        self.file_align = struct.unpack_from("<I", self.data, opt + 36)[0]

        self.sections = []
        sec_off = opt + self.opt_size
        for i in range(self.n_sections):
            o = sec_off + i * 40
            name = self.data[o:o + 8].rstrip(b"\0").decode("latin-1")
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", self.data, o + 8)
            self.sections.append({
                "name": name, "vsize": vsize, "vaddr": vaddr,
                "rawsize": rawsize, "rawptr": rawptr,
            })

    def sha256(self):
        import hashlib
        return hashlib.sha256(self.data).hexdigest()

    def rva_to_offset(self, rva):
        for s in self.sections:
            if s["vaddr"] <= rva < s["vaddr"] + max(s["vsize"], s["rawsize"]):
                return s["rawptr"] + (rva - s["vaddr"])
        return None

    def section_of(self, rva):
        for s in self.sections:
            if s["vaddr"] <= rva < s["vaddr"] + max(s["vsize"], s["rawsize"]):
                return s["name"]
        return None

    def bytes_at(self, rva, n=16):
        off = self.rva_to_offset(rva)
        if off is None:
            return None
        return self.data[off:off + n]

    def va_to_rva(self, va):
        return va - self.image_base


def classify(pe, va):
    rva = pe.va_to_rva(va)
    if rva < 0 or rva >= pe.size_of_image:
        return "OUT_OF_IMAGE", rva, None
    sec = pe.section_of(rva)
    raw = pe.bytes_at(rva, 16)
    if raw is None:
        return "UNMAPPED", rva, None
    for pat, desc in PROLOGUES.items():
        if raw.startswith(pat):
            return ("DATA" if pat in (b"\x00",) else "CODE"), rva, desc
    return "CODE", rva, raw[:6].hex(" ")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("--dump", nargs="+", help="show bytes at these VAs")
    ap.add_argument("--map", help="optional JSON of {va: label} to check")
    args = ap.parse_args()

    pe = PE(args.exe)
    print("=" * 78)
    print("file      : %s" % pe.path)
    print("size      : %.2f MB" % (len(pe.data) / 1048576))
    print("sha256    : %s" % pe.sha256())
    print("machine   : 0x%04X (%s)" % (pe.machine, {0x14C: "x86", 0x8664: "x64"}.get(pe.machine, "?")))
    print("PE32+     : %s" % ("yes (PE32+)" if pe.pe32_plus else "no (PE32)"))
    print("image base: 0x%08X" % pe.image_base)
    print("entry RVA : 0x%08X" % pe.entry_rva)
    print("size image: 0x%08X" % pe.size_of_image)
    print("sections  :")
    for s in pe.sections:
        print("   %-9s vaddr=0x%08X vsize=0x%06X rawsize=0x%06X" %
              (s["name"], s["vaddr"], s["vsize"], s["rawsize"]))

    if args.dump:
        print("=" * 78)
        for va in args.dump:
            va = int(va, 0)
            kind, rva, desc = classify(pe, va)
            raw = pe.bytes_at(rva, 32) if 0 <= rva < pe.size_of_image else None
            print("0x%08X  rva=0x%08X  %-9s %-12s %s" % (
                va, rva, kind, pe.section_of(rva) or "-",
                raw.hex(" ") if raw else "-"))
        return

    targets = dict(PESMOD_ADDRESSES)
    if args.map:
        targets = {int(k, 0): v for k, v in json.loads(pathlib.Path(args.map).read_text()).items()}

    print("=" * 78)
    print("ADDRESS VERIFICATION (%d targets)" % len(targets))
    print("=" * 78)
    stats = {"CODE": 0, "DATA": 0, "OUT_OF_IMAGE": 0, "UNMAPPED": 0}
    mismatches = []
    for va in sorted(targets):
        kind, rva, desc = classify(pe, va)
        stats[kind] = stats.get(kind, 0) + 1
        flag = ""
        if kind in ("OUT_OF_IMAGE", "UNMAPPED"):
            flag = "  <<< MISSING"
            mismatches.append((va, targets[va], kind))
        elif kind == "DATA":
            flag = "  (data?)"
        print("0x%08X  %-13s %-9s %-24s %s%s" % (
            va, kind, pe.section_of(rva) or "-", desc or "", targets[va][:24], flag))

    print("-" * 78)
    print("CODE=%(CODE)d  DATA=%(DATA)d  MISSING=%(OUT_OF_IMAGE)d" % stats)
    if mismatches:
        print("\nMISSING (need re-location via Ghidra):")
        for va, label, kind in mismatches:
            print("  0x%08X  %s  (%s)" % (va, label, kind))
    else:
        print("\nAll target addresses resolve inside the image.")


if __name__ == "__main__":
    main()
