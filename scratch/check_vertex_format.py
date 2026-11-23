import struct

with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

# Let's inspect vertex blocks starting around 0x4000
# In PES model formats, vertices are typically 32 or 48 or 64 bytes:
# Position: 3 floats (12 bytes) or 3 int16s
# Normal: 3 floats
# UV: 2 int16s or 2 floats
for p in range(0x4000, 0x6000, 32):
    # try reading floats
    x, y, z = struct.unpack('<fff', data[p:p+12])
    if -100.0 < x < 100.0 and -100.0 < y < 100.0 and -100.0 < z < 100.0 and (abs(x) > 0.01 or abs(y) > 0.01):
        print(f'Vertex candidate at 0x{p:04X}: Pos=({x:.2f}, {y:.2f}, {z:.2f})')
        # print remaining 20 bytes as hex and shorts
        print('  Hex:', data[p+12:p+32].hex(' '))
        shorts = struct.unpack('<10h', data[p+12:p+32])
        print('  Shorts:', shorts)
        if p > 0x4100: break
