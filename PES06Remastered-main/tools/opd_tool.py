#!/usr/bin/env python3
"""
PES 6 option file (.opd) tooling.

The option file holds the master league, the teams, the created players and
the formations. It is obfuscated in two independent ways:

  1. A rolling XOR over the whole file:

         param = 0x573
         for each byte:  byte ^= (param & 0xFF)
                          param = (param * 0x6C078965 + 1) & 0xFFFFFFFF

     This is its own inverse.

  2. A second pass over the data of nine sections, using a 446 byte key
     table and a constant:

         value  = dword[j+3] ^ 0x7AB3684C
         value += key[j % 446]   (signed)
         dword[j+3] = value

     The section's data offset is taken from a table of section indices
     stored at offset 0x4A of the file.

Both passes are symmetric, so decrypt and encrypt are the same operation.

The algorithm is the one used by kitserver's filedec.cpp, which is the
reference implementation in this project's `references/` tree.

Usage:
    python opd_tool.py decrypt <in.opd> <out.bin>
    python opd_tool.py encrypt <in.bin> <out.opd>
    python opd_tool.py roundtrip <in.opd>          # decrypt then re-encrypt
    python opd_tool.py dump <file>                 # structure dump
"""
import argparse
import struct
import sys
from pathlib import Path

# ---------------------------------------------------------------- constants

DECRYPT_START = 0x573
DECRYPT_FACTOR = 0x6C078965
DECRYPT_FACTOR2 = 0x7AB3684C
KEY_LEN = 0x1BE          # 446

# The 446 byte key table, from kitserver's filedec.cpp (array2[]).
KEY = bytes([
    0x36, 0x52, 0x71, 0x53, 0x58, 0x77, 0x4D, 0x6D, 0x48, 0x69, 0x63, 0x31,
    0x68, 0x73, 0x64, 0x48, 0x69, 0x79, 0x77, 0x4B, 0x44, 0x31, 0x52, 0x59,
    0x42, 0x78, 0x71, 0x31, 0x77, 0x50, 0x35, 0x61, 0x46, 0x37, 0x56, 0x76,
    0x31, 0x61, 0x38, 0x74, 0x4D, 0x43, 0x68, 0x6A, 0x6B, 0x39, 0x59, 0x52,
    0x51, 0x77, 0x50, 0x5A, 0x78, 0x73, 0x57, 0x71, 0x4C, 0x33, 0x55, 0x35,
    0x4E, 0x34, 0x5A, 0x72, 0x66, 0x6A, 0x6D, 0x36, 0x32, 0x5A, 0x63, 0x57,
    0x45, 0x42, 0x33, 0x78, 0x71, 0x68, 0x48, 0x68, 0x71, 0x6C, 0x53, 0x58,
    0x58, 0x57, 0x58, 0x66, 0x64, 0x4A, 0x48, 0x72, 0x62, 0x31, 0x76, 0x56,
    0x78, 0x48, 0x76, 0x65, 0x59, 0x77, 0x4C, 0x48, 0x33, 0x67, 0x50, 0x69,
    0x5A, 0x79, 0x38, 0x71, 0x4A, 0x36, 0x7A, 0x32, 0x71, 0x66, 0x67, 0x44,
    0x34, 0x6B, 0x49, 0x4F, 0x48, 0x59, 0x72, 0x73, 0x76, 0x72, 0x55, 0x58,
    0x51, 0x4D, 0x58, 0x68, 0x6C, 0x43, 0x57, 0x35, 0x45, 0x71, 0x6D, 0x67,
    0x6D, 0x42, 0x49, 0x4A, 0x45, 0x46, 0x33, 0x68, 0x6F, 0x51, 0x42, 0x36,
    0x75, 0x54, 0x45, 0x57, 0x63, 0x73, 0x64, 0x30, 0x78, 0x39, 0x58, 0x51,
    0x7A, 0x68, 0x4C, 0x30, 0x4D, 0x6C, 0x79, 0x4E, 0x49, 0x72, 0x36, 0x31,
    0x31, 0x33, 0x33, 0x6E, 0x44, 0x67, 0x63, 0x43, 0x47, 0x48, 0x35, 0x70,
    0x66, 0x77, 0x67, 0x65, 0x6E, 0x65, 0x78, 0x78, 0x72, 0x7A, 0x67, 0x73,
    0x6E, 0x71, 0x69, 0x61, 0x51, 0x39, 0x41, 0x66, 0x36, 0x5A, 0x6C, 0x66,
    0x75, 0x6A, 0x45, 0x51, 0x56, 0x30, 0x36, 0x59, 0x79, 0x31, 0x79, 0x75,
    0x56, 0x37, 0x72, 0x6B, 0x64, 0x49, 0x38, 0x76, 0x32, 0x30, 0x33, 0x70,
    0x57, 0x4F, 0x51, 0x49, 0x36, 0x69, 0x43, 0x56, 0x38, 0x70, 0x6E, 0x71,
    0x74, 0x42, 0x4B, 0x37, 0x33, 0x65, 0x42, 0x68, 0x34, 0x54, 0x54, 0x51,
    0x33, 0x34, 0x64, 0x68, 0x7A, 0x6D, 0x64, 0x4C, 0x59, 0x58, 0x41, 0x76,
    0x30, 0x35, 0x39, 0x61, 0x44, 0x35, 0x57, 0x49, 0x4D, 0x6C, 0x54, 0x66,
    0x6B, 0x61, 0x35, 0x30, 0x67, 0x62, 0x4A, 0x6B, 0x4A, 0x42, 0x52, 0x6A,
    0x6C, 0x71, 0x48, 0x47, 0x70, 0x70, 0x50, 0x5A, 0x39, 0x6C, 0x61, 0x4A,
    0x6D, 0x72, 0x62, 0x74, 0x5A, 0x35, 0x69, 0x5A, 0x70, 0x4B, 0x63, 0x53,
    0x65, 0x78, 0x73, 0x32, 0x6B, 0x4B, 0x47, 0x43, 0x64, 0x34, 0x53, 0x6B,
    0x48, 0x76, 0x53, 0x4C, 0x6A, 0x77, 0x39, 0x33, 0x58, 0x32, 0x72, 0x30,
    0x70, 0x69, 0x52, 0x4D, 0x5A, 0x35, 0x6F, 0x49, 0x51, 0x5A, 0x53, 0x72,
    0x34, 0x78, 0x69, 0x70, 0x55, 0x61, 0x75, 0x45, 0x31, 0x54, 0x71, 0x4F,
    0x65, 0x4B, 0x35, 0x39, 0x52, 0x36, 0x4F, 0x47, 0x57, 0x41, 0x78, 0x78,
    0x4F, 0x61, 0x78, 0x68, 0x61, 0x38, 0x59, 0x33, 0x63, 0x55, 0x31, 0x30,
    0x69, 0x65, 0x72, 0x77, 0x64, 0x49, 0x39, 0x36, 0x74, 0x59, 0x6C, 0x56,
    0x4D, 0x67, 0x55, 0x31, 0x53, 0x4B, 0x54, 0x33, 0x75, 0x50, 0x56, 0x79,
    0x5A, 0x32, 0x67, 0x48, 0x4E, 0x70, 0x44, 0x51, 0x77, 0x46, 0x4E, 0x6C,
    0x56, 0x77, 0x79, 0x56, 0x79, 0x34, 0x39, 0x6F, 0x53, 0x78, 0x55, 0x5A,
    0x44, 0x00,
])

# Section type -> expected payload size, from filedec.cpp.
CS_FILE_SIZES = [
    0x131400, 0x47000, 0x3000, 0x65400, 0x65800, 0xB7800, 0xB4400, 0x9000,
    0x9000, 0x9000, 0x4800, 0x11400, 0x65400,
]

# Section headers start at these offsets.
SECT_STARTS = [0, 0x1400, 0x1800]

# The nine sections that get the second crypto pass, as indices into the
# section table stored at offset 0x4A of the file.
OPT_SECTIONS = [0, 4, 7, 10, 12, 18, 20, 24, 26]

SECTION_NAMES = {
    0: "GENERAL (header)",
    1: "STADIUMNAMES (header)",
    2: "STADIUMNAMES (data)",
    3: "LEAGUENAMES (data)",
    4: "CREATEDPLAYERS (header)",
    5: "CREATEDPLAYERS (data)",
    6: "SHOES",
    7: "STDPLAYERS (header)",
    8: "STDPLAYERS (data)",
    9: "PLAYERNUMBERS (header)",
    10: "NATPLAYERNUMBERS",
    11: "CLUBPLAYERNUMBERS",
    12: "NATTEAMPLAYERS",
    13: "CLUBTEAMPLAYERS",
    14: "TEAMFORMATIONS",
    15: "TEAMINFOS (header)",
    16: "TEAMINFOS (data)",
    17: "TEAMKITINFOS (header)",
    18: "TEAMNATKITINFOS",
    19: "TEAMCLUBKITINFOS",
}

FILE_TYPES = {
    0: "option file", 1: "replay", 2: "formations", 3: "cup", 4: "league",
    5: "master league", 6: "(unknown)", 7: "challenge training",
    8: "dribbling", 9: "(unknown)", 10: "master league (best team)",
    11: "(unknown)", 12: "(unknown)",
}


# ---------------------------------------------------------------- crypto

def rolling_xor(buf):
    """The whole-file rolling XOR. Its own inverse."""
    param = DECRYPT_START
    for i in range(len(buf)):
        buf[i] ^= (param & 0xFF)
        param = (param * DECRYPT_FACTOR + 1) & 0xFFFFFFFF
    return buf


def section_crypto(buf, decrypt=True):
    """The second pass over nine sections.

    The two directions are inverses of each other:
        encrypt:  v = dword ^ FACTOR2 ; dword = v + key
        decrypt:  v = dword - key    ; dword = v ^ FACTOR2
    """
    if len(buf) < 0x4A * 4 + 4:
        return buf
    for sect in OPT_SECTIONS:
        idx = (sect + 0x4A) * 4
        if idx + 4 > len(buf):
            continue
        sect_start = struct.unpack_from("<I", buf, idx)[0]
        if sect_start + 16 > len(buf):
            continue
        count = struct.unpack_from("<I", buf, sect_start)[0]
        dwords = count >> 2
        for j in range(dwords):
            at = sect_start + (j + 3) * 4
            if at + 4 > len(buf):
                break
            if decrypt:
                key = KEY[j % KEY_LEN]
                if key & 0x80:
                    key -= 0x100
                val = (struct.unpack_from("<I", buf, at)[0] - key) & 0xFFFFFFFF
                struct.pack_into("<I", buf, at, val ^ DECRYPT_FACTOR2)
            else:
                val = struct.unpack_from("<I", buf, at)[0] ^ DECRYPT_FACTOR2
                key = KEY[j % KEY_LEN]
                if key & 0x80:
                    key -= 0x100
                struct.pack_into("<I", buf, at, (val + key) & 0xFFFFFFFF)
    return buf


def transform(data, decrypt=True):
    """Applies both passes.

    The order matters, because the section pass reads the section data offsets
    out of the file, and the rolling XOR changes those bytes. The reference
    implementation (kitserver filedec.cpp) therefore does:

        decode:  DeEncCode  then  DecryptOptionfile
        encode:  EncryptOptionfile  then  DeEncCode

    so the two directions are not mirror images of each other in composition -
    they are inverses only when applied in matching pairs.
    """
    buf = bytearray(data)
    if decrypt:
        rolling_xor(buf)          # DeEncCode: its own inverse
        section_crypto(buf, True)  # DecryptOptionfile
    else:
        section_crypto(buf, False) # EncryptOptionfile
        rolling_xor(buf)           # DeEncCode
    return bytes(buf)


# ---------------------------------------------------------------- structure

def dump(path):
    """Prints what can be read out of an option file."""
    raw = Path(path).read_bytes()
    print("file      : %s" % path)
    print("size      : %d bytes (0x%X)" % (len(raw), len(raw)))
    if len(raw) < 0x50:
        print("too short to hold a header")
        return

    dec = bytearray(raw)
    rolling_xor(dec)
    print("file type : %d (%s)   [byte 12 of the rolled-XOR view]"
          % (dec[12], FILE_TYPES.get(dec[12], "?")))

    num_sects = 3 if dec[12] in (4, 5) else 2
    print("sections  : %d headers at %s" % (num_sects, [hex(x) for x in SECT_STARTS]))
    for i in range(num_sects):
        base = SECT_STARTS[i]
        if base + 16 > len(dec):
            continue
        sect_type = dec[base + 10]
        size = struct.unpack_from("<I", dec, base)[0]
        checksum = struct.unpack_from("<I", dec, base + 4)[0]
        print("  section %d @ 0x%04X  type=%-2d %-26s size=0x%X  checksum=0x%08X"
              % (i, base, sect_type,
                 SECTION_NAMES.get(sect_type, "(unknown)")[:26],
                 size, checksum))

    print()
    print("section data offsets (the table at 0x4A):")
    for sect in OPT_SECTIONS:
        idx = (sect + 0x4A) * 4
        if idx + 4 > len(dec):
            continue
        off = struct.unpack_from("<I", dec, idx)[0]
        print("   section %-2d -> data at 0x%08X" % (sect, off))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["decrypt", "encrypt", "roundtrip", "dump"])
    ap.add_argument("source")
    ap.add_argument("target", nargs="?")
    args = ap.parse_args()

    src = Path(args.source)
    if not src.exists():
        raise SystemExit("no such file: %s" % src)
    raw = src.read_bytes()

    if args.mode == "dump":
        dump(src)
        return

    out = transform(raw, decrypt=(args.mode == "decrypt"))

    if args.mode == "roundtrip":
        # the transformation is symmetric, so a second pass must be identical
        again = transform(out, decrypt=False)
        if again == raw:
            print("ROUND TRIP OK: decrypt then re-encrypt is byte identical")
            print("  %d bytes" % len(raw))
        else:
            diff = sum(1 for a, b in zip(raw, again) if a != b)
            print("ROUND TRIP FAILED: %d of %d bytes differ" % (diff, len(raw)))
            sys.exit(1)
        if args.target:
            Path(args.target).write_bytes(out)
            print("  decoded copy written to %s" % args.target)
        return

    if not args.target:
        raise SystemExit("%s needs a target file" % args.mode)
    Path(args.target).write_bytes(out)
    print("%s -> %s (%d bytes)" % (args.mode, args.target, len(out)))


if __name__ == "__main__":
    main()

