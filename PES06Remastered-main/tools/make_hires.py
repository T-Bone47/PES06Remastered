#!/usr/bin/env python3
"""
Build the higher resolution artwork part of a remaster pack.

The game asks for textures of a fixed size and uploads its own artwork into
them. The remaster can replace the contents of a texture with artwork supplied
at a higher resolution, filtered down into the texture with an area average.
That gives a properly supersampled image where the game previously had a low
resolution source that it stretched itself.

The pack is a folder next to the game executable:

    remaster_pack/
        hires.txt       one "<width>x<height> <file>" line per texture size
        <file>          the artwork

The size in the index is the size of the *texture* the artwork replaces, not
the size of the artwork. The artwork may be any size.

Usage:
    # one image for one texture size
    python make_hires.py --image art.png --size 512x512 --out C:\\...\\remaster_pack

    # every image in a folder, named "<width>x<height>.png"
    python make_hires.py --scan C:\\art --out C:\\...\\remaster_pack

    # what texture sizes the game actually uses
    python make_hires.py --sizes C:\\...\\remaster.log
"""
import argparse
import re
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    Image = None


def write_raw1(path, width, height, bgra):
    """Writes the raw format the runtime reads: "RAW1", w, h, then BGRA."""
    with open(path, "wb") as f:
        f.write(b"RAW1")
        f.write(struct.pack("<II", width, height))
        f.write(bgra)


def image_to_bgra(path):
    """Loads an image and returns (width, height, BGRA bytes)."""
    if Image is None:
        raise SystemExit("Pillow is needed to read images: pip install pillow")
    with Image.open(path) as im:
        im = im.convert("RGBA")
        w, h = im.size
        data = im.tobytes()          # R, G, B, A per pixel
    bgra = bytearray(w * h * 4)
    bgra[0::4] = data[2::4]          # B
    bgra[1::4] = data[1::4]          # G
    bgra[2::4] = data[0::4]          # R
    bgra[3::4] = data[3::4]          # A
    return w, h, bytes(bgra)


def read_index(path):
    lines = []
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if line and not line.startswith("#"):
                lines.append(line)
    return lines


def write_index(path, entries):
    path.write_text("\n".join("%s %s" % e for e in entries) + "\n", encoding="utf-8")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--image")
    ap.add_argument("--size", help="the texture size the artwork replaces, WxH")
    ap.add_argument("--scan", help="a folder of images named WxH.png")
    ap.add_argument("--out", help="the pack folder")
    ap.add_argument("--sizes", help="a remaster.log to read the texture sizes from")
    ap.add_argument("--name", help="the file name to use in the pack")
    args = ap.parse_args()

    if args.sizes:
        log = Path(args.sizes).read_text(errors="ignore")
        sizes = {}
        # what the game asked for, for the textures it uploads into
        for m in re.finditer(r"d3d8: uploaded (\d+)x(\d+)", log):
            key = (int(m.group(1)), int(m.group(2)))
            sizes[key] = sizes.get(key, 0) + 1
        # every texture it creates, with the pool it was created in
        for m in re.finditer(r"d3d8: texture (\d+)x(\d+) -> (\d+)x(\d+) .*pool=(\d+)", log):
            key = (int(m.group(1)), int(m.group(2)))
            sizes.setdefault(key, 0)
        # the enlarged ones
        for m in re.finditer(r"d3d8: scaled texture (\d+)x(\d+) -> (\d+)x(\d+)", log):
            key = (int(m.group(1)), int(m.group(2)))
            sizes.setdefault(key, 0)
        if not sizes:
            print("no texture sizes found in %s" % args.sizes)
            print("run the game once with observe_textures = 1 or log_textures = 1")
            return
        print("%-12s %s" % ("size", "count"))
        for (w, h), c in sorted(sizes.items(), key=lambda kv: -(kv[0][0] * kv[0][1])):
            print("%-12s %d" % ("%dx%d" % (w, h), c))
        return

    if not args.out:
        ap.error("--out is required")
    pack = Path(args.out)
    pack.mkdir(parents=True, exist_ok=True)
    index = pack / "hires.txt"
    entries = []
    for line in read_index(index):
        parts = line.split(None, 1)
        if len(parts) == 2:
            entries.append((parts[0], parts[1].strip()))

    added = []

    if args.image:
        if not args.size:
            ap.error("--size is required with --image")
        w, h = (int(x) for x in args.size.lower().split("x"))
        name = args.name or (Path(args.image).stem + ".raw")
        pw, ph, bgra = image_to_bgra(args.image)
        write_raw1(pack / name, pw, ph, bgra)
        added.append(("%dx%d" % (w, h), name))
        print("%s -> %dx%d artwork for a %dx%d texture" % (args.image, pw, ph, w, h))

    if args.scan:
        folder = Path(args.scan)
        for path in sorted(folder.glob("*.png")):
            m = re.match(r"^(\d+)x(\d+)$", path.stem)
            if not m:
                print("  skipped %s (name is not WxH)" % path.name)
                continue
            w, h = int(m.group(1)), int(m.group(2))
            pw, ph, bgra = image_to_bgra(path)
            name = path.stem + ".raw"
            write_raw1(pack / name, pw, ph, bgra)
            added.append(("%dx%d" % (w, h), name))
            print("%s -> %dx%d artwork for a %dx%d texture" % (path.name, pw, ph, w, h))

    if not added:
        ap.error("nothing to do: pass --image, --scan or --sizes")

    merged = []
    for size, name in added:
        merged = [(s, n) for (s, n) in merged if s != size]
        merged.append((size, name))
    for size, name in entries:
        merged = [(s, n) for (s, n) in merged if s != size]
        merged.append((size, name))
    write_index(index, merged)
    print("\n%s now has %d entries" % (index, len(merged)))
    for size, name in merged:
        print("  %-12s %s" % (size, name))


if __name__ == "__main__":
    main()
