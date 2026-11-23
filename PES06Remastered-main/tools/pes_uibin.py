#!/usr/bin/env python3
"""
Parse PES 6 "Rich" / UI-BIN assets and produce a named asset index.

Ported from PESMod's tools/ui-bin-viewer/src/parser.js, which already
understands this format.

Layout (from parseUiBin):
    +0x00  u32  rawTexturePtr     -> texture descriptor
    +0x04  u32  extraRoot[1]      -> subresource bank
    +0x08  u32  extraRoot[2]      -> asset bank
    +0x0C  u32  extraRoot[3]      -> name table
    texture descriptor:
        +0x00 u32 count
        +0x04 u32 nameTablePtr   -> per-texture name pointers -> C strings

Pointers may be absolute or relocated by a base; the base is inferred.

Usage:
    python pes_uibin.py <file.afs> [--index out.tsv] [--limit N]
"""
import argparse
import struct
import sys
import zlib
from pathlib import Path

U32 = 4
MAX_COUNT = 4096


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def can_read(b, o, n):
    return 0 <= o and o + n <= len(b)


def resolve_pointer(ptr, length, base):
    if ptr == 0:
        return None
    if 0 < ptr < length:
        return ptr
    rel = ptr - base
    if 0 < rel < length:
        return rel
    return None


def looks_like_texture_desc(b, off, base):
    if not can_read(b, off, U32 * 2):
        return False
    count = u32(b, off)
    if count == 0 or count > MAX_COUNT:
        return False
    nameTable = resolve_pointer(u32(b, off + U32), len(b), base)
    if nameTable is None:
        return False
    if not can_read(b, nameTable, U32):
        return False
    p = resolve_pointer(u32(b, nameTable), len(b), base)
    if p is None or not can_read(b, p, 1):
        return False
    # first texture name should be printable
    for i in range(24):
        if p + i >= len(b):
            return False
        c = b[p + i]
        if c == 0:
            return True
        if not (32 <= c < 127):
            return False
    return False


def infer_pointer_base(b):
    length = len(b)
    root = u32(b, 0)
    if root < length:
        return 0
    max_off = min(length - U32, 0x20000)
    for off in range(U32, max_off + 1, U32):
        base = root - off
        if looks_like_texture_desc(b, off, base):
            return base
    best = (0, -1)
    for off in range(0, max_off + 1, U32):
        base = root - off
        score = 0
        for pos in range(0, min(length, 0x400), U32):
            p = u32(b, pos)
            r = p - base
            if 0 <= r < length and r % U32 == 0:
                score += 1
        if score > best[1]:
            best = (base, score)
    return best[0]


def read_cstring(b, off):
    if not can_read(b, off, 1):
        return ""
    end = b.find(b"\x00", off, min(len(b), off + 512))
    if end < 0:
        end = min(len(b), off + 512)
    return b[off:end].decode("cp1252", errors="replace")


def parse_uibin(b):
    """Returns dict with root info and a list of texture names."""
    if len(b) < U32:
        return None
    base = infer_pointer_base(b)
    resolve = lambda p: resolve_pointer(p, len(b), base)
    rootPtr = u32(b, 0)
    texDesc = resolve(rootPtr)
    if texDesc is None:
        return None
    if not can_read(b, texDesc, U32 * 2):
        return None
    count = u32(b, texDesc)
    if count == 0 or count > MAX_COUNT:
        return None
    nameTable = resolve(u32(b, texDesc + U32))
    if nameTable is None or not can_read(b, nameTable, U32):
        return None
    names = []
    for i in range(count):
        po = nameTable + i * U32
        if not can_read(b, po, U32):
            break
        raw = u32(b, po)
        no = resolve(raw)
        nm = read_cstring(b, no) if no is not None else ""
        names.append((i, nm, raw))
    return {
        "pointerBase": base,
        "rootPtr": rootPtr,
        "texDesc": texDesc,
        "count": count,
        "textures": names,
    }


def inflate_at(buf, limit=8192):
    best = None
    for k in range(min(len(buf), limit)):
        if buf[k] != 0x78:
            continue
        if buf[k + 1:k + 2] not in (b"\x01", b"\x5e", b"\x9c", b"\xda"):
            continue
        try:
            out = zlib.decompress(buf[k:])
            if best is None or len(out) > len(best[1]):
                best = (k, out)
        except zlib.error:
            pass
    return best if best else (None, None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--index")
    ap.add_argument("--limit", type=int, default=20)
    ap.add_argument("--dump", type=int, default=0)
    args = ap.parse_args()

    data = Path(args.file).read_bytes()
    num = u32(data, 4)
    print("file   : %s" % args.file)
    print("entries: %d" % num)

    parsed = 0
    named = 0
    rows = []
    allnames = []
    for i in range(num):
        off, size = struct.unpack_from("<II", data, 8 + i * 8)
        if off + 16 > len(data):
            continue
        t, fl = struct.unpack_from("<HH", data, off)
        pay, inner = struct.unpack_from("<II", data, off + 8)
        raw = data[off + 12: off + 12 + (pay if pay else max(0, size - 12))]
        k, out = inflate_at(raw)
        if not out:
            continue
        parsed += 1
        r = parse_uibin(out)
        if r:
            named += 1
            for (ti, nm, rawptr) in r["textures"]:
                if nm:
                    allnames.append((i, ti, nm))
                    rows.append((i, t, ti, nm, r["pointerBase"], r["count"]))

    print("decompressed      : %d / %d" % (parsed, num))
    print("parsed as UI-BIN  : %d" % named)
    print("named textures    : %d" % len(allnames))

    if allnames:
        print("\nsample texture names:")
        for i, ti, nm in allnames[:args.limit]:
            print("   afs_id_%-6d tex[%d]  %r" % (i, ti, nm))

    if args.index:
        with Path(args.index).open("w", encoding="utf-8") as f:
            f.write("afs_id\tentry_type\ttexture_index\ttexture_name\tpointer_base\ttex_count\n")
            for r in rows:
                f.write("%d\t0x%04X\t%d\t%s\t%d\t%d\n" % (r[0], r[1], r[2], r[3], r[4], r[5]))
        print("\nwrote %s (%d rows)" % (args.index, len(rows)))


if __name__ == "__main__":
    main()
