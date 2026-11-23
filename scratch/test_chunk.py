import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    off, sz = entries[485]
    f.seek(off)
    data = f.read(sz)

print('Adriano at byte 1008:')
chunk = data[960:1056]
for i in range(0, len(chunk), 16):
    print(f'{960+i}: {chunk[i:i+16].hex(" ")}  {repr(chunk[i:i+16])}')
