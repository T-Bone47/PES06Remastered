import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    off, sz = entries[485]
    f.seek(off)
    data = f.read(sz)

num_records = len(data) // 48
targets = ['Adriano', 'Henry', 'Ronaldinho', 'Gerrard', 'Rooney']

print(f'Total player records: {num_records}')
found = {}
for i in range(num_records):
    rec = data[i*48 : (i+1)*48]
    meta = rec[:16]
    name_bytes = rec[16:48].split(b'\x00')[0]
    name = name_bytes.decode('latin1', 'ignore')
    for t in targets:
        if name.lower() == t.lower():
            found[t] = (i, name, meta)
            print(f'Player ID / Index: {i:4d} | Name: "{name:15s}" | Meta: {meta.hex(" ")}')

print('\nDetailed metadata:')
for t, (idx, name, meta) in found.items():
    u32s = struct.unpack('<4I', meta)
    u16s = struct.unpack('<8H', meta)
    print(f'{name:15s} (ID={idx:4d}):')
    print(f'  uint32s: {[hex(x) for x in u32s]}')
    print(f'  uint16s: {[hex(x) for x in u16s]}')
