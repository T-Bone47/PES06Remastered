import struct

with open(r'PES 6 game files\dat\0_text.afs', 'rb') as f:
    f.seek(8 + 485 * 8)
    offset, size = struct.unpack('<II', f.read(8))
    f.seek(offset)
    data = f.read(size)

record_size = 48
total_records = len(data) // record_size

players = {}
for i in range(total_records):
    rec_off = i * record_size
    rec = data[rec_off : rec_off + record_size]
    # The name is at rec[0:32] or until null byte
    name_bytes = rec.split(b'\x00')[0]
    try:
        name = name_bytes.decode('latin1').strip()
    except:
        name = ""
    if name:
        # Player ID is i - 1 (since record 0 is header at offset 0, first player Aaron Hughes is at offset 48, ID 0)
        pid = i - 1
        players[pid] = (rec_off, name)

print(f"Parsed {len(players)} named player records.")

# Marquee players to find
rosters = {
    "Arsenal": ["Henry", "Fabregas", "Gilberto", "Ljungberg", "Persie"],
    "Barcelona": ["Ronaldinho", "Eto'o", "Deco", "Messi"],
    "Chelsea": ["Drogba", "Lampard", "Terry"],
    "Real Madrid": ["Zidane", "Ronaldo", "Beckham", "Roberto Carlos"]
}

for team, members in rosters.items():
    print(f"\n=== {team} ===")
    for m in members:
        found = [(pid, off, name) for pid, (off, name) in players.items() if m.lower() in name.lower()]
        for pid, off, name in found:
            print(f"  PID {pid:4d} (0x{pid:04X}) [off 0x{off:X}]: {name}")
