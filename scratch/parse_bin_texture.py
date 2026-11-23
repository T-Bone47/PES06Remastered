import struct

with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

print('Face bin size:', len(data))
# Look at the texture header
# Header had: 40 00 (64), 80 00 (128) at offset 36
w, h = struct.unpack('<HH', data[36:40])
print(f'Texture dimensions in header: {w}x{h}')

# In PES 6 textures:
# 256 colors * 4 bytes (RGBA) = 1024 bytes palette
# 64 * 128 = 8192 bytes index pixels
# Let's search for 1024-byte palette or pixel offsets in the header
u32s = struct.unpack('<32I', data[:128])
for i, v in enumerate(u32s):
    if v < len(data):
        print(f'Header u32[{i:2d}]: 0x{v:06X} ({v})')
