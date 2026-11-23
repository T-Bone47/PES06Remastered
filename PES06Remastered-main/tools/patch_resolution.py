#!/usr/bin/env python3
"""
PES 6 resolution patcher.

The game's Direct3D init (FUN_00404c60 @ 0x00404C60) loads the resolution
into ECX/EAX and stores them into four globals:

    00404c83  B9 80 02 00 00       MOV ECX,0x280        ; 640
    00404c88  B8 E0 01 00 00       MOV EAX,0x1e0        ; 480
    00404c99  89 0D D8 3F F8 00    MOV [0x00F83FD8],ECX ; width
    00404c9f  A3 DC 3F F8 00       MOV [0x00F83FDC],EAX ; height
    00404caa  89 0D E0 3F F8 00    MOV [0x00F83FE0],ECX ; width  (copy)
    00404cb0  A3 E4 3F F8 00       MOV [0x00F83FE4],EAX ; height (copy)

Patching the two MOV immediates changes all four globals at once.

Usage:
    python patch_resolution.py <pes6.exe> --width 1280 --height 720 [--out out.exe]
    python patch_resolution.py <pes6.exe> --restore
"""
import argparse
import shutil
import struct
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from verify_addresses import PE

# Four patch sites. The game keeps the backbuffer size and the window size in
# two separate places; both must change for a clean high-resolution window.
#
#   00404c83  B9 80 02 00 00       MOV ECX,0x280     -> backbuffer width
#   00404c88  B8 E0 01 00 00       MOV EAX,0x1e0     -> backbuffer height
#   0041179d  68 80 02 00 00       PUSH 0x280        -> window width
#   00411798  68 E0 01 00 00       PUSH 0x1e0        -> window height
SITES = [
    (0x00404C83, 0x280, "B9", "backbuffer width"),
    (0x00404C88, 0x1E0, "B8", "backbuffer height"),
    (0x0041179D, 0x280, "68", "window width"),
    (0x00411798, 0x1E0, "68", "window height"),
]
WIDTH_SITES = (0x00404C83, 0x0041179D)
HEIGHT_SITES = (0x00404C88, 0x00411798)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe")
    ap.add_argument("--width", type=int)
    ap.add_argument("--height", type=int)
    ap.add_argument("--out")
    ap.add_argument("--restore", action="store_true")
    args = ap.parse_args()

    src = Path(args.exe)
    pe = PE(src)
    data = bytearray(pe.data)

    def off_for(va):
        rva = va - pe.image_base
        o = pe.rva_to_offset(rva)
        if o is None:
            raise SystemExit("VA 0x%08X is not mapped" % va)
        return o

    print("file  : %s" % src)
    print("sha256: %s" % pe.sha256())
    print("image base 0x%08X" % pe.image_base)
    print()

    changed = 0
    for va, original, opcode_hex, desc in SITES:
        o = off_for(va)
        opcode = data[o:o + 1]
        expect = bytes.fromhex(opcode_hex)
        if opcode != expect:
            print("  !! 0x%08X opcode %s != expected %s - ABORT" % (va, opcode.hex(), expect.hex()))
            return 2
        cur = struct.unpack_from("<I", data, o + 1)[0]
        if cur != original:
            print("  !! 0x%08X immediate is 0x%X, expected 0x%X - already patched?" %
                  (va, cur, original))
        if args.restore:
            new = original
        elif va in WIDTH_SITES:
            new = args.width
        else:
            new = args.height
        struct.pack_into("<I", data, o + 1, new)
        changed += 1
        print("  0x%08X (file 0x%08X)  %-18s  0x%03X -> 0x%03X" %
              (va, o, desc, cur, new))

    out = Path(args.out) if args.out else src
    if out != src:
        shutil.copy2(src, out)
    out.write_bytes(bytes(data))
    print()
    print("wrote %s  (%d sites)" % (out, changed))
    return 0


if __name__ == "__main__":
    sys.exit(main())

