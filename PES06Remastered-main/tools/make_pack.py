#!/usr/bin/env python3
"""
Build a replacement pack for the remaster's asset subsystem.

The pack is a folder next to the game executable:

    remaster_pack/index.txt     one "<crc32 hex> " line per asset
    remaster_pack/<file>        the replacement, already inflated

The runtime hook identifies an asset by the CRC32 of its inflated bytes, so
the index key is the CRC32 of the ORIGINAL asset, not of the replacement.

Usage:
    python make_pack.py --archive <file.afs> --id <n> --out <packdir> \
        [--transform invert|solid=R,G,B|byte=XX] [--name <file>]

    python make_pack.py --archive <file.afs> --list
"""
import argparse
import binascii
import struct
import zlib
from pathlib import Path


def read_afs(path):
    data = Path(path).read_bytes()
    if data[:4] != b"AFS\x00":
        raise SystemExit("not an AFS archive")
    num = struct.unpack_from("<I", data, 4)[0]
    items = [struct.unpack_from("<II", data, 8 + i * 8) for i in range(num)]
    return data, num, items


def inflate_entry(data, idx, items):
    off, size = items[idx]
    t, fl = struct.unpack_from("<HH", data, off)
    pay, inner = struct.unpack_from("<II", data, off + 8)
    body = data[off + 12: off + size]
    for k in range(min(len(body), 4096)):
        if body[k] == 0x78 and body[k + 1:k + 2] in (b"\x01", b"\x5e", b"\x9c", b"\xda"):
            try:
                out = zlib.decompress(body[k:])
                if out:
                    return t, fl, pay, inner, k, out
            except zlib.error:
                pass
    return t, fl, pay, inner, None, None


def apply_transform(payload, spec):
    """Transform the pixel region (after the sub-header)."""
    if spec == "invert":
        return bytes(b ^ 0xFF for b in payload)
    if spec.startswith("byte="):
        v = int(spec.split("=", 1)[1], 0) & 0xFF
        return bytes(v for _ in payload)
    if spec.startswith("word="):
        # Fill with a repeated 16-bit value, for RGB565 format probes.
        v = int(spec.split("=", 1)[1], 0) & 0xFFFF
        out = bytearray(payload)
        for i in range(0, len(out) - 1, 2):
            out[i] = v & 0xFF
            out[i + 1] = (v >> 8) & 0xFF
        return bytes(out)
    if spec.startswith("solid="):
        r, g, b = (int(x) for x in spec.split("=", 1)[1].split(","))
        out = bytearray(payload)
        for i in range(0, len(out) - 3, 4):
            out[i] = b
            out[i + 1] = g
            out[i + 2] = r
            out[i + 3] = 0xFF
        return bytes(out)
    raise SystemExit("unknown transform: %s" % spec)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--archive", required=True)
    ap.add_argument("--id", type=int)
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--out")
    ap.add_argument("--transform", default="invert")
    ap.add_argument("--name")
    args = ap.parse_args()

    data, num, items = read_afs(args.archive)

    if args.list:
        print("%-6s %-8s %-10s %-10s %-8s %s" % ("id", "type", "inflated", "crc32", "hdr", "name"))
        for i in range(num):
            t, fl, pay, inner, k, out = inflate_entry(data, i, items)
            if out is None:
                continue
            crc = binascii.crc32(out) & 0xFFFFFFFF
            print("%-6d 0x%04X %-10d 0x%08X %-8d" % (i, t, len(out), crc, k or 0))
        return

    if args.id is None or not args.out:
        ap.error("--id and --out are required unless --list is given")

    t, fl, pay, inner, k, out = inflate_entry(data, args.id, items)
    if out is None:
        raise SystemExit("entry %d did not inflate" % args.id)

    crc = binascii.crc32(out) & 0xFFFFFFFF
    head = out[:k]
    body = apply_transform(out[k:], args.transform)
    replaced = head + body

    pack = Path(args.out)
    pack.mkdir(parents=True, exist_ok=True)
    name = args.name or ("id_%05d.bin" % args.id)
    (pack / name).write_bytes(replaced)

    index = pack / "index.txt"
    lines = []
    if index.exists():
        lines = [l for l in index.read_text(encoding="utf-8").splitlines()
                 if l.strip() and not l.strip().startswith("#")]
    lines = [l for l in lines if not l.split()[1:2] == [name]]
    lines.append("%08X %s" % (crc, name))
    index.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print("entry %d  type=0x%04X  inflated=%d  hdr=%d  crc=0x%08X" % (args.id, t, len(out), k, crc))
    print("  transform : %s" % args.transform)
    print("  replacement: %s (%d bytes)" % (pack / name, len(replaced)))
    print("  index     : %s (%d entries)" % (index, len(lines)))


if __name__ == "__main__":
    main()
