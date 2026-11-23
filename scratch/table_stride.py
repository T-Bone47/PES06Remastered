import struct

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    off, sz = entries[485]
    f.seek(off)
    data = f.read(sz)

# Let's inspect the first 256 bytes of data
print('First 256 bytes:')
for i in range(0, 256, 32):
    print(f'Offset {i:4d} (0x{i:04X}): {data[i:i+32].hex(" ")}')

# Let's find null-terminated strings in data and their offsets
strings = []
pos = 0
while pos < len(data):
    # find first printable ascii byte
    if 32 <= data[pos] < 127:
        end = pos
        while end < len(data) and 32 <= data[end] < 127:
            end += 1
        if end - pos >= 3 and end < len(data) and data[end] == 0:
            s = data[pos:end].decode('latin1')
            strings.append((pos, s))
            pos = end
    pos += 1

print(f'Total strings found: {len(strings)}')
print('First 15 strings:')
for p, s in strings[:15]:
    print(f'  Offset {p:5d} (0x{p:04X}): "{s}"')

# Print differences between offsets of consecutive strings
diffs = [strings[i+1][0] - strings[i][0] for i in range(min(50, len(strings)-1))]
print('Sample stride between consecutive strings:', diffs[:20])
