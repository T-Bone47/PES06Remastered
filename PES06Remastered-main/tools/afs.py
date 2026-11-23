#!/usr/bin/env python3
"""
Konami AFS archive reader for the PES 6 `dat/*.afs` containers.

Layout (confirmed against kitserver's afsreader.h, and verified against the
shipped pes6.exe archives):

    0x00  char   sig[4]        "AFS\\0"
    0x04  uint32 numFiles
    0x08  AFSITEMINFO[numFiles]   { uint32 offset; uint32 size; }
    ..    AFSNAMEINFO[numFiles]   { char name[32]; byte other[16]; }
    ..    file data

Usage:
    python afs.py <file.afs> [--list] [--limit N] [--extract DIR] [--name X]
    python afs.py <file.afs> --dump-offset 0x13800
"""
import argparse
import struct
import sys
import zlib
from pathlib import Path

AFSSIG = 0x00534641
NAME_SIZE = 32
NAME_REC = 48


class Entry:
    __slots__ = ("index", "name", "offset", "size", "extra")

    def __init__(self, index, name, offset, size, extra):
        self.index = index
        self.name = name
        self.offset = offset
        self.size = size
        self.extra = extra

    def __repr__(self):
        return "<%05d %-40s off=0x%08X size=%d>" % (
            self.index, self.name, self.offset, self.size)


def read_afs(path):
    """Parse one AFS directory. Returns (data, entries, name_table_offset)."""
    data = Path(path).read_bytes()
    if data[:4] != b"AFS\x00":
        raise SystemExit("not an AFS archive (magic %r)" % data[:4])
    num = struct.unpack_from("<I", data, 4)[0]
    item_off = 8
    # After the item table comes a u32 pointing at the name table.
    name_ptr_off = item_off + num * 8
    name_table_off = struct.unpack_from("<I", data, name_ptr_off)[0]
    entries = []
    for i in range(num):
        off, size = struct.unpack_from("<II", data, item_off + i * 8)
        entries.append(Entry(i, "", off, size, b""))
    for i in range(num):
        no = name_table_off + i * NAME_REC
        raw = data[no:no + NAME_SIZE]
        end = raw.find(b"\x00")
        entries[i].name = raw[:end].decode("latin-1") if end >= 0 else ""
        entries[i].extra = data[no + NAME_SIZE:no + NAME_REC]
    return data, entries, name_table_off


def walk(data, entries, base=0, depth=0, out=None, prefix=""):
    """Recurse into nested AFS folders. Returns a list of (path, entry)."""
    if out is None:
        out = []
    for e in entries:
        off = base + e.offset
        nested = None
        if off + 8 <= len(data) and data[off:off + 4] == b"AFS\x00":
            try:
                num = struct.unpack_from("<I", data, off + 4)[0]
                if 0 < num < 100000:
                    sub_items = []
                    for i in range(num):
                        o, s = struct.unpack_from("<II", data, off + 8 + i * 8)
                        sub_items.append(Entry(i, "", o, s, b""))
                    ptr = struct.unpack_from("<I", data, off + 8 + num * 8)[0]
                    for i in range(num):
                        no = off + ptr + i * NAME_REC
                        raw = data[no:no + NAME_SIZE]
                        end = raw.find(b"\x00")
                        sub_items[i].name = raw[:end].decode("latin-1") if end >= 0 else ""
                    nested = sub_items
            except Exception:
                nested = None
        if nested is not None:
            walk(data, nested, base=off, depth=depth + 1, out=out, prefix=prefix + e.name + "/")
        else:
            out.append((prefix + e.name, e, base))
    return out


def entry_bytes(data, e):
    raw = data[e.offset:e.offset + e.size]
    if raw[:2] in (b"\x78\x9c", b"\x78\x01", b"\x78\xda"):
        try:
            return zlib.decompress(raw)
        except zlib.error:
            try:
                return zlib.decompress(raw, -15)
            except zlib.error:
                return raw
    return raw


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--limit", type=int, default=60)
    ap.add_argument("--extract", metavar="DIR")
    ap.add_argument("--name", help="extract only this entry")
    ap.add_argument("--info", help="print info about one entry by name")
    args = ap.parse_args()

    p = Path(args.file)
    data, entries, ntoff = read_afs(p)
    flat = walk(data, entries, 0, 0, [], "")
    print("file   : %s" % p)
    print("size   : %.2f MB" % (len(data) / 1048576))
    print("entries: %d  (name table @ 0x%X)" % (len(entries), ntoff))
    print("leaves : %d" % len(flat))
    print()
    if args.list:
        exts = {}
        for path, e, base in flat:
            ext = Path(path).suffix.lower() or "(none)"
            exts[ext] = exts.get(ext, 0) + 1
        print("extension histogram:")
        for x, c in sorted(exts.items(), key=lambda kv: -kv[1])[:30]:
            print("   %-12s %d" % (x, c))
        print()
        print("first %d leaf entries:" % min(args.limit, len(flat)))
        for path, e, base in flat[:args.limit]:
            print("   %-46s size=%-9d off=0x%08X" % (path[:46], e.size, base + e.offset))
    return

    if args.info:
        for e in entries:
            if e.name.lower() == args.info.lower():
                print("index : %d" % e.index)
                print("name  : %r" % e.name)
                print("offset: 0x%08X" % e.offset)
                print("size  : %d" % e.size)
                b = entry_bytes(data, e)
                print("bytes : %d (decompressed)" % len(b))
                print("head  : %s" % b[:48].hex(" "))
                return
        print("not found")
        return

if __name__ == "__main__":
    main()
