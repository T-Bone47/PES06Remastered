import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = []
    for _ in range(num_files):
        entries.append(struct.unpack('<II', f.read(8)))
    
    off, sz = entries[485]
    f.seek(off)
    data = f.read(sz)

targets = [b'Adriano', b'Henry', b'Ronaldinho', b'Gerrard', b'Rooney']
for t in targets:
    pos = data.find(t)
    print(f'Target: {t.decode()} at byte {pos} (0x{pos:X})')
    # Print 32 bytes before and 32 bytes after
    print('  Context:', data[pos-16 : pos+32].hex(' '))
    # Check if there are other occurrences
    pos2 = data.find(t, pos + 1)
    if pos2 != -1:
        print(f'  Target {t.decode()} also at byte {pos2} (0x{pos2:X})')
