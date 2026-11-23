import struct

with open(r'PES 6 game files\dat\0_text.afs', 'rb') as f:
    data = f.read()

# Let's inspect AFS header to find which file contains the names
magic = data[:4]
num_files = struct.unpack('<I', data[4:8])[0]
print(f"AFS Num files: {num_files}")

entries = []
for i in range(num_files):
    offset, size = struct.unpack('<II', data[8 + i*8 : 8 + (i+1)*8])
    entries.append((i, offset, size))

names = [b'Henry', b'Ronaldinho', b'Zidane', b'Lampard', b'Terry', b'Drogba', b'Messi', b'Beckham', b"Eto'o", b'Deco', b'van Persie', b'Ljungberg', b'Gilberto']

for name in names:
    for idx, off, sz in entries:
        chunk = data[off : off+sz]
        if name in chunk:
            print(f"{name.decode('latin1')} found in entry #{idx} (offset 0x{off:X}, size {sz})")
            break
