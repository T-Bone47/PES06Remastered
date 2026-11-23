#!/usr/bin/env python3
"""
Inventory the PES 6 AFS archives.

Walks the nested AFS tree, and for every leaf entry records:
  * its index path
  * the 12-byte entry header (flags, payload size, inner size)
  * a content probe: DDS / PNG / TGA / TIM / unknown

The AFS name table is encrypted in the shipped archives, so entries are
identified by index, exactly as kitserver's afswalk.cpp does.

Usage:
    python afs_inventory.py <file.afs> [--out index.tsv] [--probe N]
"""
import argparse
import struct
import sys
from pathlib import Path

AFSSIG = b"AFS\x00"
NAME_REC = 48


def read_dir(data, base):
    """Return (entries, name_table_offset) for the AFS directory at `base`."""
    if data[base:base + 4] != AFSSIG:
        return None, 0
    num = struct.unpack_from("<I", data, base + 4)[0]
    if not (0 < num < 100000):
        return None, 0
    items = []
    for i in range(num):
        off, size = struct.unpack_from("<II", data, base + 8 + i * 8)
        items.append([off, size, ""])
    try:
        ntoff = struct.unpack_from("<I", data, base + 8 + num * 8)[0]
        for i in range(num):
            no = base + ntoff + i * NAME_REC
            raw = data[no:no + 32]
            end = raw.find(b"\x00")
            if end > 0:
                try:
                    items[i][2] = raw[:end].decode("ascii")
                except UnicodeDecodeError:
                    items[i][2] = ""
    except Exception:
        pass
    return items, num


def walk(data, base, path, out, depth=0):
    items, num = read_dir(data, base)
    if items is None:
        return
    for i, (off, size, name) in enumerate(items):
        abs_off = base + off
        if abs_off + 8 <= len(data) and data[abs_off:abs_off + 4] == AFSSIG:
            walk(data, abs_off, path + [name or ("dir%03d" % i)], out, depth + 1)
        else:
            out.append((path + [name or ("id_%05d" % i)], abs_off, size))


def probe(data, off, size):
    """Classify an entry by its header and content magic."""
    flags = inner = 0
    payload = size
    head = b""
    if size >= 12 and off + 12 <= len(data):
        flags, payload, inner = struct.unpack_from("<III", data, off)
        head = data[off + 12:off + 12 + 16]
    else:
        head = data[off:off + 16] if off < len(data) else b""

    kind = "unknown"
    if head[:4] == b"DDS ":
        kind = "dds"
    elif head[:8] == b"\x89PNG\r\n\x1a\n":
        kind = "png"
    elif head[:2] in (b"BM",):
        kind = "bmp"
    elif len(head) >= 18 and head[0:2] == b"\x10\x00" and head[2:4] == b"\x00\x00":
        kind = "psx-tim"
    elif head[:4] == b"RIFF":
        kind = "riff"
    elif head[:4] == b"\x00\x00\x01\x00":
        kind = "pes-rich"      # the "Rich" container PESMod's parser handles
    return flags, payload, inner, kind, head


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--out")
    ap.add_argument("--probe", type=int, default=0, help="dump N entry headers")
    args = ap.parse_args()

    p = Path(args.file)
    data = p.read_bytes()
    leaves = []
    walk(data, 0, [], leaves)

    print("file   : %s" % p)
    print("size   : %.2f MB" % (len(data) / 1048576))
    print("leaves : %d" % len(leaves))

    hist = {}
    rows = []
    for path, off, size in leaves:
        flags, payload, inner, kind, head = probe(data, off, size)
        hist[kind] = hist.get(kind, 0) + 1
        rows.append(("/".join(path), off, size, flags, payload, inner, kind))

    print("\ncontent histogram:")
    for k, v in sorted(hist.items(), key=lambda kv: -kv[1]):
        print("   %-12s %d" % (k, v))

    biggest = sorted(rows, key=lambda r: -r[2])[:12]
    print("\nlargest entries:")
    for r in biggest:
        print("   %-28s size=%-10d flags=0x%08X payload=%-10d inner=%-10d %s"
              % (r[0][:28], r[2], r[3], r[4], r[5], r[6]))

    if args.out:
        out = Path(args.out)
        with out.open("w", encoding="utf-8") as f:
            f.write("path\toffset\tsize\tflags\tpayload\tinner\tkind\n")
            for r in rows:
                f.write("%s\t0x%08X\t%d\t0x%08X\t%d\t%d\t%s\n" % (r[0], r[1], r[2], r[3], r[4], r[5], r[6]))
        print("\nwrote %s (%d rows)" % (out, len(rows)))

    for r in rows[:args.probe]:
        print("\n[%s] size=%d" % (r[0], r[2]))
        print("   header: flags=0x%08X payload=%d inner=%d" % (r[3], r[4], r[5]))
        print("   head  : %s" % data[r[1]:r[1] + 32].hex(" "))


if __name__ == "__main__":
    main()
