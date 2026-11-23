import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    
    ranges = []
    current_magic = None
    start_idx = 0
    count = 0
    for i in range(num_files):
        f.seek(entries[i][0])
        magic = f.read(4)
        if magic != current_magic:
            if current_magic is not None:
                ranges.append((start_idx, i - 1, count, current_magic.hex()))
            current_magic = magic
            start_idx = i
            count = 1
        else:
            count += 1
    ranges.append((start_idx, num_files - 1, count, current_magic.hex()))

print('File ranges by magic in 0_text.afs:')
for start, end, count, m in ranges:
    if count >= 10:
        print(f'Files {start:4d} to {end:4d} (count: {count:4d}) -> magic {m}')
