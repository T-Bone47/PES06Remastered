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

print(f'File 485 total bytes: {len(data)}')
num_records = len(data) // 48
print(f'Total 48-byte records: {num_records}')

target_names = ['Adriano', 'Henry', 'Ronaldinho', 'Gerrard', 'Rooney']
results = {}

for i in range(num_records):
    rec = data[i*48 : (i+1)*48]
    val1, val2 = struct.unpack('<II', rec[:8])
    raw_name = rec[8:].split(b'\x00')[0]
    name = raw_name.decode('latin1', 'ignore')
    for target in target_names:
        if name.lower() == target.lower():
            results[target] = (i, val1, val2, name)
            print(f'Found {target}: Index={i}, val1=0x{val1:08X} ({val1}), val2=0x{val2:08X} ({val2}), name="{name}"')

print('\nSummary of Target Players:')
for t in target_names:
    if t in results:
        idx, v1, v2, nm = results[t]
        print(f'{nm}: Index {idx}, val1={v1}, val2={v2}')
    else:
        print(f'{t}: Not found in exact match')
