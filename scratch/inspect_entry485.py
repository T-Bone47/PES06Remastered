import struct

with open(r'PES 6 game files\dat\0_text.afs', 'rb') as f:
    f.seek(8 + 485 * 8)
    offset, size = struct.unpack('<II', f.read(8))
    f.seek(offset)
    data = f.read(size)

print(f"Entry #485 size: {len(data)} bytes")

# Let's inspect where the names are and if there is a fixed record length or index table
# Look for strings
names = [b'Henry', b'Ronaldinho', b'Zidane', b'Lampard', b'Terry', b'Drogba', b'Messi', b'Beckham', b"Eto'o", b'Deco', b'van Persie', b'Ljungberg', b'Gilberto', b'Fabregas', b'Ronaldo']

for name in names:
    pos = data.find(name)
    if pos != -1:
        # Show surrounding 32 bytes
        surround = data[max(0, pos-16) : min(len(data), pos+32)]
        print(f"{name.decode('latin1', errors='ignore')} @ 0x{pos:X}: {surround.hex()}")
