import struct, zlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    f.seek(entries[1][0])
    data = f.read(entries[1][1])
    comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
    decomp = zlib.decompress(data[32:32+comp_sz])

print(f'Uncompressed size: {len(decomp)} (0x{len(decomp):X})')

# In PES 6 model files:
# Magic: 0x29857294 (94 72 85 29)
# Let's inspect words and pointers in the header
words = struct.unpack('<16I', decomp[:64])
for i, w in enumerate(words):
    print(f'Word {i:2d}: 0x{w:08X} ({w})')

# Search for texture dimensions (64, 128) or palette markers
# Let's see where the palette is located
# Palette is 256 colors * 4 bytes = 1024 bytes (RGBA or BGRA)
# In PS2 / PES 6 textures, palette often has 256 colors with alpha=0x80 or 0xFF
for p in range(0, len(decomp)-1024, 64):
    # check if 1024 bytes look like a palette
    # e.g., 256 entries where byte 3 is 0x80 or 0xFF
    alphas = [decomp[p + i*4 + 3] for i in range(256)]
    if all(a in (0x80, 0xFF, 0x00) for a in alphas) and alphas.count(0x80) > 200:
        print(f'Potential palette at offset 0x{p:X} ({p})')
