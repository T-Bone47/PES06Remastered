#!/usr/bin/env python3
"""
Write PES 6 AFS archives, replacing entries by index.

Layout (from the reader):
    0x00  char   sig[4]        "AFS\\0"
    0x04  uint32 numFiles
    0x08  AFSITEMINFO[numFiles]      { uint32 offset; uint32 size; }
    ..    uint32 nameTableOffset      <-- pointer to the name table
    ..    file data (0x1000-aligned)
    ..    name table (copied verbatim)

The name table is encrypted in the shipped archives, so it is copied byte for
byte and the pointer is updated. Entries keep their exact bytes unless a
replacement is supplied.

Usage:
    python afs_write.py <in.afs> <out.afs> [--replace id:file] [--replace-raw id:file]
    python afs_write.py <in.afs> <out.afs> --verify
"""
import argparse
import struct
import zlib
from pathlib import Path

ALIGN = 0x1000


def align_up(n):
    return (n + ALIGN - 1) & ~(ALIGN - 1)


def read_afs(path):
    data = Path(path).read_bytes()
    if data[:4] != b"AFS\x00":
        raise SystemExit("not an AFS archive")
    num = struct.unpack_from("<I", data, 4)[0]
    item_off = 8
    name_ptr_off = item_off + num * 8
    name_table_off = struct.unpack_from("<I", data, name_ptr_off)[0]
    items = []
    for i in range(num):
        off, size = struct.unpack_from("<II", data, item_off + i * 8)
        items.append((off, size))
    return data, num, items, name_table_off


def build(src_path, out_path, replacements, verify=False):
    data, num, items, name_off = read_afs(src_path)

    # The name table is exactly numFiles * 48 bytes. It sits in the middle of
    # the data region in the original archive, so it must NOT be copied to EOF.
    name_bytes = data[name_off:name_off + num * 48]
    # data region of the original (everything between the header block and the
    # name table) is rebuilt from the item list
    payloads = []
    for i, (off, size) in enumerate(items):
        payloads.append(data[off:off + size])

    replaced = 0
    for idx, blob in replacements.items():
        if idx < 0 or idx >= num:
            raise SystemExit("entry %d out of range" % idx)
        payloads[idx] = blob
        replaced += 1

    # rebuild
    header = bytearray()
    header += b"AFS\x00"
    header += struct.pack("<I", num)
    table = bytearray()
    for _ in range(num):
        table += b"\0\0\0\0\0\0\0\0"

    body_start = len(header) + len(table) + 4
    # layout: header, item table, name-table pointer, data, name table
    cursor = align_up(body_start)
    new_items = []
    for p in payloads:
        new_items.append((cursor, len(p)))
        cursor = align_up(cursor + len(p))
    data_region = bytearray()
    for p in payloads:
        pass
    # rebuild data region with alignment padding
    data_region = bytearray()
    for (off, size), p in zip(new_items, payloads):
        pad = off - (body_start + len(data_region))
        if pad < 0:
            raise SystemExit("internal layout error")
        data_region += b"\0" * pad
        data_region += p
    # pad to the end of the last entry
    tail_pad = cursor - (body_start + len(data_region))
    data_region += b"\0" * max(0, tail_pad)

    name_table_offset = body_start + len(data_region)

    out = bytearray()
    out += header
    for (off, size) in new_items:
        out += struct.pack("<II", off, size)
    out += struct.pack("<I", name_table_offset)
    out += data_region
    out += name_bytes

    Path(out_path).write_bytes(bytes(out))
    return num, replaced, len(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--replace", action="append", default=[], metavar="ID:FILE")
    ap.add_argument("--verify", action="store_true")
    args = ap.parse_args()

    replacements = {}
    for spec in args.replace:
        idx, _, path = spec.partition(":")
        replacements[int(idx)] = Path(path).read_bytes()

    num, replaced, size = build(args.src, args.dst, replacements)
    print("wrote %s" % args.dst)
    print("  entries  : %d" % num)
    print("  replaced : %d" % replaced)
    print("  size     : %.2f MB" % (size / 1048576))

    if args.verify:
        # re-read and confirm every non-replaced entry is byte-identical
        orig, n1, it1, no1 = read_afs(args.src)
        new, n2, it2, no2 = read_afs(args.dst)
        bad = 0
        for i in range(n1):
            if i in replacements:
                continue
            o = orig[it1[i][0]:it1[i][0] + it1[i][1]]
            nn = new[it2[i][0]:it2[i][0] + it2[i][1]]
            if o != nn:
                bad += 1
                if bad <= 5:
                    print("  MISMATCH entry %d" % i)
        print("  verify   : %d entries differ (expected %d)" % (bad, len(replacements)))
        print("  name table preserved: %s" % (orig[no1:] == new[no2:]))


if __name__ == "__main__":
    main()
