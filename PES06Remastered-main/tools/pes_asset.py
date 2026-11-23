#!/usr/bin/env python3
"""
Extract and classify PES 6 assets from the AFS archives.

Pipeline (all confirmed against dat/0_text.afs):

    AFS entry
      -> 12-byte header:  u16 type | u16 flags | u32 payloadSize | u32 innerSize
      -> zlib stream (starts at +20, or further in for some types)
      -> decompressed payload

The decompressed payload is a PES "Rich" container. Its magic is the build
stamp 0x20050420 (20 April 2005) and the fields after it are offsets into the
decompressed buffer. Some entries (type 0x600) decompress straight to raw
pixel data instead.

Usage:
    python pes_asset.py <file.afs> [--list] [--dump N] [--out DIR] [--index index.tsv]
"""
import argparse
import struct
import sys
import zlib
from pathlib import Path

RICH_MAGIC = 0x20050420
TYPE_NAMES = {
    0x0000: "rich-asset",
    0x0100: "type-0100",
    0x0200: "type-0200",
    0x0400: "type-0400",
    0x0800: "type-0800",
    0x5061: "texture-apdt",
    0x6000: "type-6000",
    0xE000: "type-e000",
}


def inflate_at(buf, limit=8192):
    """Find and inflate a zlib stream in buf. Returns (offset, data)."""
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
            try:
                d = zlib.decompressobj()
                out = d.decompress(buf[k:])
                if len(out) > 64 and (best is None or len(out) > len(best[1])):
                    best = (k, out)
            except zlib.error:
                pass
    return best if best else (None, None)


def read_afs(path):
    data = Path(path).read_bytes()
    if data[:4] != b"AFS\x00":
        raise SystemExit("not an AFS archive")
    num = struct.unpack_from("<I", data, 4)[0]
    items = []
    for i in range(num):
        off, size = struct.unpack_from("<II", data, 8 + i * 8)
        items.append((off, size))
    return data, items


def classify(payload, out):
    """Describe a decompressed payload."""
    if len(out) < 16:
        return "tiny", {}
    if struct.unpack_from("<I", out, 0)[0] == RICH_MAGIC:
        # Rich container: magic then a table of offsets.
        offs = struct.unpack_from("<8I", out, 4)
        return "rich", {"offsets": offs}
    # Raw pixels: check for a repeating alpha of 0xFF (RGBA) or 0x80.
    a0 = out[3]
    if len(out) % 4 == 0:
        alpha = {out[i * 4 + 3] for i in range(min(64, len(out) // 4))}
        if len(alpha) == 1:
            return "rgba-tex", {"alpha": alpha.pop(), "pixels": len(out) // 4}
    return "binary", {}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--limit", type=int, default=20)
    ap.add_argument("--dump", type=int, default=0, help="decompress and show N entries")
    ap.add_argument("--out")
    ap.add_argument("--index")
    args = ap.parse_args()

    p = Path(args.file)
    data, items = read_afs(p)
    print("file   : %s" % p)
    print("entries: %d" % len(items))

    rows = []
    hist = {}
    for i, (off, size) in enumerate(items):
        if off + 16 > len(data):
            continue
        t, fl = struct.unpack_from("<HH", data, off)
        pay, inner = struct.unpack_from("<II", data, off + 8)
        raw = data[off + 12: off + 12 + (pay if pay else max(0, size - 12))]
        k, out = inflate_at(raw)
        kind = "undecoded"
        info = {}
        if out:
            kind, info = classify(raw[k:], out)
        hist[kind] = hist.get(kind, 0) + 1
        rows.append((i, off, size, t, fl, pay, inner, k, len(out) if out else 0, kind, info))

    print("\ndecompressed classification:")
    for k, v in sorted(hist.items(), key=lambda kv: -kv[1]):
        print("   %-14s %d" % (k, v))

    if args.list:
        print("\nfirst %d entries:" % min(args.limit, len(rows)))
        for r in rows[:args.limit]:
            print("   id_%-6d type=0x%04X size=%-9d zlib@%-5s out=%-9d %s"
                  % (r[0], r[3], r[2], r[7], r[8], r[9]))

    if args.dump:
        print("\ndumped entries:")
        for r in rows[:args.dump]:
            i, off, size, t, fl, pay, inner, k, olen, kind, info = r
            print("   id_%05d type=0x%04X size=%-9d -> %-14s out=%d %s"
                  % (i, t, size, kind, olen, info))

    if args.out:
        outdir = Path(args.out)
        outdir.mkdir(parents=True, exist_ok=True)
        for r in rows:
            i, off, size, t, fl, pay, inner, k, olen, kind, info = r
            if not olen:
                continue
            raw = data[off + 12: off + 12 + (pay if pay else max(0, size - 12))]
            _, out = inflate_at(raw)
            (outdir / ("id_%05d.bin" % i)).write_bytes(out)
        print("\nwrote %d decompressed entries to %s" % (len(rows), outdir))

    if args.index:
        with Path(args.index).open("w", encoding="utf-8") as f:
            f.write("id\ttype\tflags\tsize\tpayload\tinner\tzlib_at\tout_size\tkind\n")
            for r in rows:
                f.write("%d\t0x%04X\t0x%04X\t%d\t%d\t%d\t%s\t%d\t%s\n" %
                        (r[0], r[3], r[4], r[2], r[5], r[6], r[7], r[8], r[9]))
        print("wrote %s" % args.index)


if __name__ == "__main__":
    main()
