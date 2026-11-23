#!/usr/bin/env python3
"""
Replace the payload of a single AFS entry, in place.

The archive is not rebuilt: the entry keeps its offset, only its size changes
(and only if the new data still fits in the padding before the next entry).
Everything else in the file stays byte-identical, which is the most
conservative change the game will accept.

Usage:
    python afs_patch_entry.py <in.afs> <out.afs> --entry 49 --tint 255,0,255
    python afs_patch_entry.py <in.afs> <out.afs> --entry 49 --check
"""
import argparse
import struct
import zlib
from pathlib import Path

ALIGN = 0x1000


def entry_bounds(data, num, idx):
    off, size = struct.unpack_from("<II", data, 8 + idx * 8)
    # next entry's offset (entries are in ascending offset order)
    nxt = len(data)
    for i in range(idx + 1, num):
        o2, _ = struct.unpack_from("<II", data, 8 + i * 8)
        if o2 > off:
            nxt = o2
            break
    return off, size, nxt


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--entry", type=int, required=True)
    ap.add_argument("--tint", help="R,G,B to flood the texture with")
    ap.add_argument("--check", action="store_true", help="only report the layout")
    args = ap.parse_args()

    data = bytearray(Path(args.src).read_bytes())
    num = struct.unpack_from("<I", data, 4)[0]
    off, size, nxt = entry_bounds(data, num, args.entry)
    slack = nxt - (off + size)
    print("entry %d: offset=0x%08X size=%-8d next=0x%08X slack=%d"
          % (args.entry, off, size, nxt, slack))

    if args.check:
        print("  header: %s" % data[off:off + 16].hex(" "))
        t, fl, pay, inner = struct.unpack_from("<HHII", data, off)
        print("  type=0x%04X flags=0x%04X payload=%d inner=%d" % (t, fl, pay, inner))
        return

    # locate the zlib stream inside the entry payload
    body = bytes(data[off + 12: off + size])
    k = None
    for p in range(min(len(body), 4096)):
        if body[p] == 0x78 and body[p + 1:p + 2] in (b"\x01", b"\x5e", b"\x9c", b"\xda"):
            try:
                raw = zlib.decompress(body[p:])
                k = p
                break
            except zlib.error:
                pass
    if k is None:
        raise SystemExit("no zlib stream in this entry")

    if args.tint:
        r, g, b = (int(x) for x in args.tint.split(","))
        if len(raw) % 4:
            raise SystemExit("payload is not a multiple of 4 bytes (%d)" % len(raw))
        px = bytearray(raw)
        for i in range(0, len(px), 4):
            px[i] = r
            px[i + 1] = g
            px[i + 2] = b
        raw = bytes(px)
        print("  flooded %d pixels with (%d,%d,%d)" % (len(px) // 4, r, g, b))

    comp = zlib.compress(raw, 9)
    new_body = body[:k] + comp
    new_size = 12 + len(new_body)
    if new_size > size + slack:
        raise SystemExit("replacement does not fit: needs %d bytes, only %d available"
                         % (new_size, size + slack))

    # Entry layout: [12-byte header][20-byte sub-header][zlib stream]
    #   header: u16 type | u16 flags | u32 payloadSize (zlib length) | u32 innerSize
    # payloadSize counts the zlib stream ONLY (entry size = 32 + payloadSize).
    # innerSize is type-dependent, so it is preserved verbatim.
    t, fl, pay, inner = struct.unpack_from("<HHII", data, off)
    comp = zlib.compress(raw, 9)
    new_body = body[:k] + comp
    new_size = 12 + len(new_body)
    print("  old payload=%d new payload=%d  new entry size=%d (was %d)"
          % (pay, len(comp), new_size, size))
    if new_size > size + slack:
        raise SystemExit("replacement does not fit: needs %d bytes, only %d available"
                         % (new_size, size + slack))

    struct.pack_into("<HHII", data, off, t, fl, len(comp), inner)
    data[off + 12: off + 12 + len(new_body)] = new_body
    # zero the leftover gap so stale bytes cannot confuse anything
    for i in range(off + 12 + len(new_body), off + new_size):
        data[i] = 0
    # item table: only this entry's size changes; its offset is untouched
    struct.pack_into("<I", data, 8 + args.entry * 8 + 4, new_size)

    Path(args.dst).write_bytes(bytes(data))
    print("  wrote %s (offset unchanged at 0x%08X)" % (args.dst, off))


if __name__ == "__main__":
    main()
