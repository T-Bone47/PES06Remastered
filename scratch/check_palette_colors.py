with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

import struct

# Print first 64 bytes of palette_raw at offset 0x80
print('Bytes at 0x40:')
print(data[0x40:0x80].hex(' '))

print('Bytes at 0x80:')
print(data[0x80:0xC0].hex(' '))

# Let's search for skin tone colors in the entire file
# Skin tone colors typically have R > G > B, e.g. R in 120..220, G in 80..180, B in 50..150
skin_count = 0
for p in range(0, 4096, 4):
    r, g, b, a = struct.unpack('<BBBB', data[p:p+4])
    if r > g > b and r > 80 and (r - b) > 20:
        skin_count += 1
print(f'Skin tone matches in first 4096 bytes: {skin_count}')

# Check next 4096 bytes
skin_count2 = 0
for p in range(4096, 8192, 4):
    r, g, b, a = struct.unpack('<BBBB', data[p:p+4])
    if r > g > b and r > 80 and (r - b) > 20:
        skin_count2 += 1
print(f'Skin tone matches in 4096..8192 bytes: {skin_count2}')
