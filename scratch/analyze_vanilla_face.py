# Inspect scratch/extracted_vanilla_face_1.png
# Let's inspect rows to find where eyes, nose, mouth are located in 64x128 space
from test_png_writer import write_png_rgba
import struct

with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

palette_raw = data[0x80 : 0x80 + 1024]
pal = []
for i in range(256):
    r, g, b, a = struct.unpack('<BBBB', palette_raw[i*4 : (i+1)*4])
    pal.append((r, g, b, 255))

pixels_raw = data[0x80 + 1024 : 0x80 + 1024 + 8192]

# Let's compute average brightness and color per row across 128 rows (Y=0..127)
print('Y (row) analysis across 128 rows of face:')
for y in range(0, 128, 8):
    row_pixels = [pal[pixels_raw[y * 64 + x]] for x in range(64)]
    avg_r = sum(p[0] for p in row_pixels) / 64.0
    avg_g = sum(p[1] for p in row_pixels) / 64.0
    avg_b = sum(p[2] for p in row_pixels) / 64.0
    print(f'Row Y={y:3d} (rel {y/128.0:.2f}): RGB=({avg_r:5.1f}, {avg_g:5.1f}, {avg_b:5.1f})')
