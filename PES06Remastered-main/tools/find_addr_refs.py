#!/usr/bin/env python3
"""
Find code references to a data address by scanning for the address as an
immediate operand, then decode the instruction around each hit.

Ghidra's reference manager finds nothing for strings inside regions it could
not analyse (this build has protected .text islands), so a raw operand scan
is the reliable fallback.

Usage: python find_addr_refs.py <pes6.exe> <0xADDR> [0xADDR ...]
"""
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from verify_addresses import PE

# ModRM byte -> (reg field, rm field) for [reg+disp32] and [disp32] forms
def decode_at(pe, va):
    """Return a short human description of the instruction at va (best effort)."""
    d = pe.data
    off = None
    for s in pe.sections:
        if s["vaddr"] <= (va - pe.image_base) < s["vaddr"] + max(s["vsize"], s["rawsize"]):
            off = s["rawptr"] + (va - pe.image_base - s["vaddr"])
            break
    if off is None:
        return None, None
    b = d[off]
    if b == 0x68:  # push imm32
        return "push imm32", d[off:off+5].hex(" ")
    if b == 0xA1:  # mov eax, [disp32]
        return "mov eax,[disp32]", d[off:off+5].hex(" ")
    if b == 0xA3:  # mov [disp32], eax
        return "mov [disp32],eax", d[off:off+5].hex(" ")
    if b == 0x8B:  # mov reg, [disp32]
        return "mov reg,[disp32]", d[off:off+6].hex(" ")
    if b == 0x89:  # mov [disp32], reg
        return "mov [disp32],reg", d[off:off+6].hex(" ")
    if b == 0x05:  # add eax, imm32
        return "add eax,imm32", d[off:off+5].hex(" ")
    if b == 0x3D:  # cmp eax, imm32
        return "cmp eax,imm32", d[off:off+5].hex(" ")
    if b == 0x35:  # xor eax, imm32
        return "xor eax,imm32", d[off:off+5].hex(" ")
    if b == 0xE8:  # call rel32
        return "call rel32", d[off:off+5].hex(" ")
    if b == 0xE9:  # jmp rel32
        return "jmp rel32", d[off:off+5].hex(" ")
    if 0xB8 <= b <= 0xBF:  # mov reg, imm32
        return "mov reg,imm32", d[off:off+5].hex(" ")
    if b in (0x0F,):
        return "0F prefix", d[off:off+6].hex(" ")
    if 0x80 <= b <= 0x8F:
        return "arith r/m32,imm32", d[off:off+6].hex(" ")
    return "op %02x" % b, d[off:off+8].hex(" ")


def main():
    exe = sys.argv[1]
    pe = PE(exe)
    d = pe.data
    for arg in sys.argv[2:]:
        target = int(arg, 0)
        pat = struct.pack("<I", target)
        print("\n=== references to 0x%08X ===" % target)
        pos = 0
        n = 0
        while True:
            i = d.find(pat, pos)
            if i < 0:
                break
            pos = i + 1
            # only report hits that sit in the first byte of an instruction:
            # look back a few bytes for a plausible opcode
            for back in range(0, 7):
                va_guess = None
                for s in pe.sections:
                    if s["rawptr"] <= (i - back) < s["rawptr"] + s["rawsize"]:
                        va_guess = pe.image_base + s["vaddr"] + (i - back - s["rawptr"])
                        break
                if va_guess is None:
                    continue
                kind, hexb = decode_at(pe, va_guess)
                if kind and kind != "op %02x" % 0:
                    # confirm the immediate really is at +1 for the common forms
                    o = i - back
                    if d[o] in (0x68, 0xA1, 0xA3, 0x05, 0x3D, 0x35) and d[o+1:o+5] == pat:
                        print("  VA 0x%08X  %-22s %s" % (va_guess, kind, hexb))
                        n += 1
                        break
                    if d[o] in (0x8B, 0x89) and d[o+2:o+6] == pat:
                        print("  VA 0x%08X  %-22s %s" % (va_guess, kind, hexb))
                        n += 1
                        break
                    if 0xB8 <= d[o] <= 0xBF and d[o+1:o+5] == pat:
                        print("  VA 0x%08X  %-22s %s" % (va_guess, kind, hexb))
                        n += 1
                        break
        print("  total: %d" % n)


if __name__ == "__main__":
    main()

