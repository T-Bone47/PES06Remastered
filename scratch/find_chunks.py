import struct

with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

# In PES 6 model/texture container format:
# Byte 0..3: magic (e.g. 0x29857294 or similar)
# Let's search for standard texture chunk markers (like 0x00010000 or 0x00030000)
for p in range(0, len(data) - 16, 4):
    tag, = struct.unpack('<I', data[p:p+4])
    # check if tag is a section header
    if tag == 0x00030000 or tag == 0x00010000 or tag == 0x00020000:
        sz, = struct.unpack('<I', data[p+4:p+8])
        if 64 <= sz <= 32768:
            print(f'Chunk at 0x{p:04X}: tag=0x{tag:08X}, sz={sz}')
