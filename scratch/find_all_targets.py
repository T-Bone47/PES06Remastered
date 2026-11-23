import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    off, sz = entries[485]
    f.seek(off)
    data = f.read(sz)

header = data[:32]
print('Header (32 bytes):', header.hex(' '))
num_records = (len(data) - 32) // 48
print(f'Total player records: {num_records}')

targets = ['Adriano', 'Henry', 'Ronaldinho', 'Gerrard', 'Rooney']
for i in range(num_records):
    base = 32 + i * 48
    meta = data[base : base + 16]
    name = data[base + 16 : base + 48].split(b'\x00')[0].decode('latin1', 'ignore')
    for t in targets:
        if name.lower() == t.lower():
            u32s = struct.unpack('<4I', meta)
            u16s = struct.unpack('<8H', meta)
            print(f'Record {i:4d}: Name="{name:15s}" | u32s={[hex(x) for x in u32s]} | u16s={[hex(x) for x in u16s]}')
