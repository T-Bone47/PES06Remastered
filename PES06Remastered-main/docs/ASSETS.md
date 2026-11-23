# Asset replacement pack format

The remaster can replace any asset the game loads from its `dat\<lang>_text.afs`
archive. This document describes the pack format, how to build one, and what is
known about the underlying data.

## Why replacement happens after inflation

The AFS archive is **integrity checked**. Changing a single bit in a single
entry header is enough to make the game crash on startup, and rebuilding the
archive with new entry offsets crashes it as well. The only part of the
pipeline that can be changed safely is the *inflated* buffer, after the game
has decompressed it and before it hands it to the renderer.

That is exactly what the `assets` subsystem does: it hooks the inflate routine
at `0x008CD490`, lets the game inflate normally, then looks at the result.

## How an asset is identified

Assets are identified by the **CRC32 of their inflated bytes**, not by name.
The AFS name table is encrypted and the names are not present in the inflated
payloads, so a name-based scheme would need a lookup table that does not
exist. A CRC32 of the inflated data is stable, cheap, and unique enough:

* `0_text.afs` has 9,806 entries and 6,788 distinct signatures
  (1,626 entries are shared between two or more names).
* A signature collision means two different assets with identical content.
  Replacing one replaces both. This is harmless in practice, but it is why the
  pack is keyed by content and not by identity.

## Pack layout

A pack is a folder next to `pes6.exe`:

```
remaster_pack/
    index.txt          one "<crc32 hex> " line per asset
    <files>            the replacement, already inflated
```

`index.txt` is a plain text file. Blank lines and lines starting with `#` are
ignored. The CRC is written as up to 8 hexadecimal digits and is matched
case-insensitively:

```
AF3C2875 id_01056.bin
C9CA48F1 id_05318.bin
```

The referenced file must hold the **whole inflated asset**, header included,
with the same byte count as the original. The runtime copies it over the
game's buffer, so a replacement that is larger than the original is rejected
and logged rather than allowed to corrupt memory.

## Building a pack

`tools/make_pack.py` extracts an asset from an archive and writes a
replacement into a pack folder:

```sh
python tools/make_pack.py --archive "C:\...\dat\0_text.afs" --id 1056 \
    --out "C:\...\remaster_pack" --transform invert
```

Useful transforms:

| transform        | effect                                              |
|------------------|-----------------------------------------------------|
| `invert`         | invert every byte of the pixel region               |
| `byte=0x00`      | fill the pixel region with one byte value           |
| `word=0xF81F`    | fill with a repeated 16-bit value (RGB565 probes)   |
| `solid=255,0,255`| fill with an opaque colour in BGRA order             |

The transform is applied to the pixel region only, after the asset's
sub-header, so the header stays intact.

To list every asset in an archive with its signature:

```sh
python tools/build_asset_index.py "C:\...\dat\0_text.afs" --out index.tsv
```

## What is known about the asset layout

```
AFS entry
  +0x00  u16 type          0x600 for textures, 0xE00 for UI atlases, ...
  +0x02  u16 flags
  +0x04  u32 payloadSize   length of the zlib stream
  +0x08  u32 innerSize     type dependent; 0 for 0x600
  +0x0C  20-byte sub-header (type dependent)
  +0x20  zlib stream
```

After inflation a texture asset is either:

* **raw pixels** - a 16-byte sub-header
  (`u32 flags`, `u32 8`, `u32 0x10`, `u32 0`) followed by `width * height * 4`
  bytes of `D3DFMT_A8R8G8B8` data; or
* **a "Rich" container** - a 20-byte sub-header whose last word is the build
  stamp `0x20050420` (20 April 2005), followed by a table of offsets into the
  inflated buffer. The pixel data lives in one of the sections the table
  points at.

The 16-byte sub-header of the Rich form is `u32 flags`, `u32 8`,
`u32 0x10`, `u32 0`.

## Textures the game creates

The `d3d8` subsystem logs every texture the game creates. Turn it on with
`log_textures = 1` in the `[graphics]` section of `remaster.ini`.

Every texture observed so far is:

* `D3DFMT_A8R8G8B8` (format 21)
* one mip level
* either `D3DPOOL_DEFAULT` with `D3DUSAGE_DYNAMIC`, or `D3DPOOL_MANAGED`

Sizes seen range from 2x2 up to 1024x512.

## Higher resolution artwork

The game asks for textures of a fixed size and uploads its own artwork into
them. The remaster can replace the contents of a texture with artwork supplied
at a higher resolution, filtered down into the texture with an area average.
The result is a properly supersampled image where the game previously had a
low resolution source that it stretched itself.

This is a second part to the pack, in the same folder:

```
remaster_pack/
    hires.txt        one "<width>x<height> <file>" line per texture size
    <file>           the artwork
```

The size in the index is the size of the **texture** the artwork replaces,
not the size of the artwork. The artwork may be any size; it is filtered down
to fit.

### The artwork format

**PNG** is read through WIC and may be any size.

**Raw** files use a small header so the artwork can be larger than the
texture it replaces:

```
"RAW1"          four bytes
uint32 width    little endian
uint32 height   little endian
byte   pixels   width * height * 4, in B, G, R, A order
```

`B, G, R, A` is the memory order of `D3DFMT_A8R8G8B8`, which is what the
game's textures use.

### Making a pack

`tools/make_hires.py` turns images into pack entries:

```sh
# one image for one texture size
python tools/make_hires.py --image art.png --size 512x512 --out C:\...\remaster_pack

# every image in a folder, named "<width>x<height>.png"
python tools/make_hires.py --scan C:\art --out C:\...\remaster_pack

# what texture sizes the game actually uses
python tools/make_hires.py --sizes C:\...\remaster.log
```

The size list comes from the game's own log. Run the game once with
`observe_textures = 1` (or `log_textures = 1`) in `[graphics]` and the sizes
will be in `remaster.log`.

### Making the textures bigger

`texture_scale` in `[graphics]` makes the game allocate the artwork textures
that many times larger, and the pack's artwork is filtered into the larger
texture. With `texture_scale = 2` and artwork authored at four times the
texture size, the texture ends up with twice the detail of the original.

Enlarging only works if the whole texture is filled. When the game uploads
its own artwork into a texture that was made larger, that artwork lands in
the top-left corner and the rest of the texture is empty, which looks like
the picture being drawn at the wrong size. The remaster therefore replaces the
texture's contents after the upload, with the pack's artwork if there is any
and with a filtered version of the game's own artwork if there is not.

Only `D3DPOOL_MANAGED` textures are enlarged. The `D3DPOOL_DEFAULT` ones with
`D3DUSAGE_DYNAMIC` are the game's own render targets and working buffers:
the 3D scene is rendered into them at a fixed size, so making them larger
leaves the scene black.

## Reference: the game's texture inventory

Every texture the game creates is `D3DFMT_A8R8G8B8`, one mip level, and
either `D3DPOOL_DEFAULT` + `D3DUSAGE_DYNAMIC` (the 3D working buffers and
render targets) or `D3DPOOL_MANAGED` (the artwork). Managed sizes observed:

```
512x512   512x256   256x256   256x128   128x128   256x64
128x64     64x128    64x64      32x32      8x8        2x2
```

The working buffers are `1024x512`, `512x256`, `256x128` and so on, and must
not be enlarged: the 3D scene is rendered into them at a fixed size.


