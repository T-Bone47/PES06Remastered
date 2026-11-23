import struct
import re

with open(r'PES 6 game files\dat\0_text.afs', 'rb') as f:
    f.seek(8 + 485 * 8)
    offset, size = struct.unpack('<II', f.read(8))
    f.seek(offset)
    data = f.read(size)

print(f"Total entry 485 size: {len(data)}")

# Search for our target names
targets = [
    "Henry", "Fabregas", "Gilberto", "Ljungberg", "van Persie",
    "Ronaldinho", "Eto'o", "Deco", "Messi",
    "Drogba", "Lampard", "Terry",
    "Zidane", "Ronaldo", "Beckham", "Roberto Carlos"
]

for t in targets:
    pattern = re.compile(re.escape(t.encode('latin1')), re.IGNORECASE)
    for m in pattern.finditer(data):
        pos = m.start()
        # Find start of string by looking backward for null bytes
        # Record usually has a name field of 32 or 48 or 64 bytes
        # Let's inspect surrounding bytes
        prefix = data[max(0, pos-24):pos]
        suffix = data[pos:min(len(data), pos+40)]
        name_str = suffix.split(b'\x00')[0].decode('latin1', errors='ignore')
        print(f"Target '{t}': found '{name_str}' at offset 0x{pos:X}, prefix: {prefix.hex()}")
