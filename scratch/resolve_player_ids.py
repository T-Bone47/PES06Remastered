import struct

with open(r'PES 6 game files\dat\0_text.afs', 'rb') as f:
    f.seek(8 + 485 * 8)
    offset, size = struct.unpack('<II', f.read(8))
    f.seek(offset)
    data = f.read(size)

base = 0x30
record_size = 48

num_records = (len(data) - base) // record_size
print(f"Total player records in database: {num_records}")

all_players = {}
for i in range(num_records):
    rec_off = base + i * record_size
    rec = data[rec_off : rec_off + record_size]
    # Name is null-terminated string at offset 8 in record
    name_bytes = rec[8:].split(b'\x00')[0]
    name = name_bytes.decode('latin1', errors='ignore').strip()
    if name:
        all_players[i] = name

targets = [
    # Arsenal
    ("Henry", "Arsenal"),
    ("Fabregas", "Arsenal"),
    ("Gilberto", "Arsenal"),
    ("Ljungberg", "Arsenal"),
    ("Van Persie", "Arsenal"),
    # Barcelona
    ("Ronaldinho", "Barcelona"),
    ("Eto'o", "Barcelona"),
    ("Deco", "Barcelona"),
    ("Messi", "Barcelona"),
    # Chelsea
    ("Drogba", "Chelsea"),
    ("Lampard", "Chelsea"),
    ("Terry", "Chelsea"),
    # Real Madrid
    ("Zidane", "Real Madrid"),
    ("Ronaldo", "Real Madrid"),
    ("Beckham", "Real Madrid"),
    ("Roberto Carlos", "Real Madrid")
]

print("\n--- MATCHED MARQUEE PLAYERS ---")
for t_name, team in targets:
    matches = [(pid, name) for pid, name in all_players.items() if t_name.lower() in name.lower()]
    print(f"\n{team} - Search: {t_name}")
    for pid, name in matches:
        print(f"  Player ID {pid:4d} (0x{pid:04X}): '{name}'")
