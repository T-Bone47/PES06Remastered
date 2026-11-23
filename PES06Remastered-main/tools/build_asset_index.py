#!/usr/bin/env python3
"""
Build the asset index for the PES 6 AFS archives.

For every entry: decompress it, record the decompressed size and a CRC32 of the
decompressed bytes. That signature is how the runtime identifies an asset
without needing the (encrypted) AFS file names - the same approach kitserver's
Sign() uses.

Usage:
    python build_asset_index.py <file.afs> --out index.tsv
"""
import argparse
import binascii
import struct
import zlib
from pathlib import Path


def inflate(raw, limit=8192):
    best = None
    for k in range(min(len(raw), limit)):
        if raw[k] != 0x78:
            continue
        if raw[k + 1:k + 2] not in (b"\x01", b"\x5e", b"\x9c", b"\xda"):
            continue
        try:
            out = zlib.decompress(raw[k:])
            if best is None or len(out) > len(best[1]):
                best = (k, out)
        except zlib.error:
            pass
    return best if best else (None, None)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--out", required=True)
    ap.add_argument("--dump", help="write decompressed assets here")
    args = ap.parse_args()

    data = Path(args.file).read_bytes()
    num = struct.unpack_from("<I", data, 4)[0]
    print("file   : %s" % args.file)
    print("entries: %d" % num)

    rows = []
    types = {}
    if args.dump:
        dumper = Path(args.dump)
        dumper.mkdir(parents=True, exist_ok=True)

    for i in range(num):
        off, size = struct.unpack_from("<II", data, 8 + i * 8)
        if off + 12 > len(data):
            continue
        t, fl = struct.unpack_from("<HH", data, off)
        pay, inner = struct.unpack_from("<II", data, off + 8)
        body = data[off + 12: off + size]
        k, out = inflate(body)
        types[t] = types.get(t, 0) + 1
        if out is None:
            rows.append((i, t, size, 0, 0, 0))
            continue
        crc = binascii.crc32(out) & 0xFFFFFFFF
        rows.append((i, t, size, len(out), crc, k))
        if args.dump:
            (dumper / ("id_%05d.bin" % i)).write_bytes(out)

    print("type histogram:", {hex(k): v for k, v in sorted(types.items(), key=lambda kv: -kv[1])[:8]})
    with Path(args.out).open("w", encoding="utf-8") as f:
        f.write("afs_id\tentry_type\tsize\tout_size\tcrc32\tzlib_at\n")
        for r in rows:
            f.write("%d\t0x%04X\t%d\t%d\t0x%08X\t%d\n" % (r[0], r[1], r[2], r[3], r[4], r[5]))
    print("wrote %s (%d rows)" % (args.out, len(rows)))

    # duplicate signature report: a signature that appears twice is ambiguous
    seen = {}
    dupes = 0
    for r in rows:
        if not r[4]:
            continue
        if r[4] in seen:
            dupes += 1
        seen[r[4]] = r[0]
    print("distinct signatures: %d   duplicated: %d" % (len(seen), dupes))


if __name__ == "__main__":
    main()
