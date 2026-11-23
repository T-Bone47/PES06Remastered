#!/usr/bin/env python3
"""
Locate the game's Direct3DCreate8 thunk, find every call to it, and read the
IDirect3D8 vtable offset the game uses right after the call.

The vtable offset the game loads IS the authoritative CreateDevice slot index
for the system d3d8.dll, because the game was compiled against Microsoft's
real d3d8.h.

Usage: python find_d3d8_callsite.py <pes6.exe> [symbol]
"""
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from verify_addresses import PE

REGS = ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"]


def imports(pe):
    """Return {dll_lower: [(name, iat_va), ...]}."""
    out = {}
    dd = pe.data
    e_lfanew = struct.unpack_from("<I", dd, 0x3C)[0]
    coff = e_lfanew + 4
    opt_off = coff + 20
    magic = struct.unpack_from("<H", dd, opt_off)[0]
    # Data directories begin after the optional header; index 1 is the import table.
    dd_off = opt_off + (96 if magic == 0x10B else 112) + 8
    imp_rva, _ = struct.unpack_from("<II", dd, dd_off)
    i = pe.rva_to_offset(imp_rva)
    if i is None:
        return out
    while True:
        olt, ts, fchain, name_rva, iat_rva = struct.unpack_from("<IIIII", dd, i)
        if name_rva == 0 and olt == 0:
            break
        no = pe.rva_to_offset(name_rva)
        dll = dd[no:no + 64].split(b"\0")[0].decode("latin-1")
        thunk = iat_rva
        while True:
            val = struct.unpack_from("<I", dd, pe.rva_to_offset(thunk))[0]
            if val == 0:
                break
            if val & 0x80000000:
                nm = "#%d" % (val & 0xFFFF)
            else:
                po = pe.rva_to_offset(val)
                nm = dd[po + 2:po + 66].split(b"\0")[0].decode("latin-1")
            out.setdefault(dll.lower(), []).append((nm, pe.image_base + thunk))
            thunk += 4
        i += 20
    return out


def main():
    path = sys.argv[1]
    want = (sys.argv[2] if len(sys.argv) > 2 else "Direct3DCreate8").lower()
    pe = PE(path)
    imps = imports(pe)

    iat_va = None
    for dll, entries in imps.items():
        for nm, va in entries:
            if nm.lower() == want:
                iat_va = va
    if iat_va is None:
        print("import not found")
        return 1

    data = pe.data
    text = next(s for s in pe.sections if s["vaddr"] == 0x1000)
    tstart, tend = text["rawptr"], text["rawptr"] + text["rawsize"]
    text_base = pe.image_base + text["vaddr"]

    # 1) the import thunk:  FF 25 <iat_va>   (jmp dword ptr [IAT])
    thunk_pat = b"\xFF\x25" + struct.pack("<I", iat_va)
    thunk_va = None
    for off in range(tstart, tend):
        if data[off:off + 6] == thunk_pat:
            thunk_va = text_base + (off - tstart)
            break
    print("import %s  IAT=0x%08X" % (want, iat_va))
    print("thunk: %s" % ("VA=0x%08X" % thunk_va if thunk_va else "not found"))
    if thunk_va is None:
        return 1

    # 2) every  call rel32  targeting that thunk
    print("\ncalls to the thunk:")
    for off in range(tstart, tend):
        if data[off] != 0xE8:
            continue
        rel = struct.unpack_from("<i", data, off + 1)[0]
        if text_base + (off - tstart) + 5 + rel != thunk_va:
            continue
        call_va = text_base + (off - tstart)
        print("\n  call at VA=0x%08X" % call_va)
        after = data[off + 5:off + 5 + 96]
        print("    after: " + after.hex(" "))
        for k in range(0, len(after) - 5):
            if after[k] != 0x8B:
                continue
            modrm = after[k + 1]
            if (modrm & 0xC0) != 0x40:
                continue
            reg = (modrm >> 3) & 7
            rm = modrm & 7
            disp = struct.unpack_from("<b", after, k + 2)[0]
            if rm == reg:
                print("      +%-2d mov %s,[%s+0x%02X]  -> vtable slot %d"
                      % (k, REGS[reg], REGS[rm], disp & 0xFF, disp // 4))
    return 0


if __name__ == "__main__":
    sys.exit(main())

