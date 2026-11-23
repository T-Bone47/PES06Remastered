import struct, zlib

# Read scratch/sample_face_1.bin
with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

# Let's locate the 256-color palette and the 64x128 index buffer
# In Word 8: 0x1000 (4096)
# Word 9: 64
# Word 10: 128
# Let's inspect bytes starting at offset 0x40 or 0x80
# In PES 6 textures, header is 32 or 64 or 128 bytes, then palette, then pixels or pixels then palette
# Let's search for the 8192 bytes of pixels and 1024 bytes of palette
print('Searching for texture data in sample_face_1.bin...')

# Let's check offsets
# Palette in PS2 PES format: 256 entries * 4 bytes = 1024 bytes (RGBA)
# Let's inspect offset 0x80 to 0x480 (1024 bytes)
palette_raw = data[0x80 : 0x80 + 1024]
# Check if palette entries look like valid colors
# In PS2 textures, alpha is 0x80 (half) which maps to 255 in Direct3D
pal = []
for i in range(256):
    r, g, b, a = struct.unpack('<BBBB', palette_raw[i*4 : (i+1)*4])
    # D3D / PS2 conversion
    a_d3d = min(255, a * 2) if a <= 128 else 255
    pal.append((r, g, b, a_d3d))

print('First 8 palette entries:', pal[:8])

# Pixels: 64 * 128 = 8192 bytes
# Offset right after palette or at offset 0x480:
pixels_raw = data[0x80 + 1024 : 0x80 + 1024 + 8192]
print(f'Pixels raw length: {len(pixels_raw)}')

# Convert 8-bit index to 32-bit RGBA
rgba_bytes = bytearray(64 * 128 * 4)
for i, idx in enumerate(pixels_raw):
    r, g, b, a = pal[idx]
    rgba_bytes[i*4 : (i+1)*4] = struct.pack('BBBB', r, g, b, a)

from test_png_writer import write_png_rgba
png = write_png_rgba(64, 128, rgba_bytes)
with open('scratch/extracted_vanilla_face_1.png', 'wb') as out:
    out.write(png)

print('Saved scratch/extracted_vanilla_face_1.png (64x128)!')
