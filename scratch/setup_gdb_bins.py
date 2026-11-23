import os, struct, zlib

def ensure_dirs(base):
    dirs = [
        f'{base}/GDB/faces/Inter',
        f'{base}/GDB/faces/Arsenal',
        f'{base}/GDB/faces/Barcelona',
        f'{base}/GDB/faces/Liverpool',
        f'{base}/GDB/faces/Man_Utd',
        f'{base}/GDB/hair/Inter',
        f'{base}/GDB/hair/Arsenal',
        f'{base}/GDB/hair/Barcelona',
        f'{base}/GDB/hair/Liverpool',
        f'{base}/GDB/hair/Man_Utd',
        f'{base}/GDB/uni',
        f'{base}/GDB/stadiums',
    ]
    for d in dirs:
        os.makedirs(d, exist_ok=True)
        print(f'Created: {d}')

# Extract face and hair BINs from 0_text.afs
afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    
    # We extract 5 sample face bins (File 1, 3, 5, 7, 9) and 5 hair bins (File 0, 2, 4, 6, 8)
    face_bins = {}
    hair_bins = {}
    for idx, name in [(1, 'Adriano'), (3, 'Henry'), (5, 'Ronaldinho'), (7, 'Gerrard'), (9, 'Rooney')]:
        off, sz = entries[idx]
        f.seek(off)
        data = f.read(sz)
        comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
        face_bins[name] = zlib.decompress(data[32:32+comp_sz])
        
    for idx, name in [(0, 'Adriano'), (2, 'Henry'), (4, 'Ronaldinho'), (6, 'Gerrard'), (8, 'Rooney')]:
        off, sz = entries[idx]
        f.seek(off)
        data = f.read(sz)
        comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
        hair_bins[name] = zlib.decompress(data[32:32+comp_sz])

# Deploy to both PES 6 game files and test_game
player_paths = {
    'Adriano': ('Inter/Adriano', 'Inter/Adriano_hair'),
    'Henry': ('Arsenal/Henry', 'Arsenal/Henry_hair'),
    'Ronaldinho': ('Barcelona/Ronaldinho', 'Barcelona/Ronaldinho_hair'),
    'Gerrard': ('Liverpool/Gerrard', 'Liverpool/Gerrard_hair'),
    'Rooney': ('Man_Utd/Rooney', 'Man_Utd/Rooney_hair'),
}

for root in ['PES 6 game files', 'test_game']:
    ensure_dirs(root)
    for player, (face_rel, hair_rel) in player_paths.items():
        face_bin_path = f'{root}/GDB/faces/{face_rel}.bin'
        hair_bin_path = f'{root}/GDB/hair/{hair_rel}.bin'
        with open(face_bin_path, 'wb') as out_f:
            out_f.write(face_bins[player])
        with open(hair_bin_path, 'wb') as out_h:
            out_h.write(hair_bins[player])
        print(f'Wrote {face_bin_path} ({len(face_bins[player])} bytes)')
        print(f'Wrote {hair_bin_path} ({len(hair_bins[player])} bytes)')

print('GDB directories and base BIN models established successfully.')
