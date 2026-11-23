import os

for root in ['PES 6 game files', 'test_game']:
    print(f'Checking GDB paths in {root}...')
    # Check faces
    with open(f'{root}/GDB/faces/map.txt', 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                parts = line.split(',')
                pid = parts[0]
                bin_rel = parts[1].strip('"')
                bin_path = f'{root}/GDB/faces/{bin_rel}'
                png_path = os.path.splitext(bin_path)[0] + '.png'
                assert os.path.exists(bin_path), f'Missing face bin: {bin_path}'
                assert os.path.exists(png_path), f'Missing face png: {png_path}'
                print(f'  [FACE PASS] Player {pid:5s} -> {bin_rel} (BIN: {os.path.getsize(bin_path)} bytes, PNG: {os.path.getsize(png_path)} bytes)')
    # Check hair
    with open(f'{root}/GDB/hair/map.txt', 'r') as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                parts = line.split(',')
                pid = parts[0]
                bin_rel = parts[1].strip('"')
                bin_path = f'{root}/GDB/hair/{bin_rel}'
                png_path = os.path.splitext(bin_path)[0] + '.png'
                assert os.path.exists(bin_path), f'Missing hair bin: {bin_path}'
                assert os.path.exists(png_path), f'Missing hair png: {png_path}'
                print(f'  [HAIR PASS] Player {pid:5s} -> {bin_rel} (BIN: {os.path.getsize(bin_path)} bytes, PNG: {os.path.getsize(png_path)} bytes)')
print('All GDB mapped face and hair assets validated!')
