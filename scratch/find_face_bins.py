import struct, zlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]

    print('Listing files with uncompressed size 66688:')
    count = 0
    for i in range(1, 500):
        off, sz = entries[i]
        f.seek(off)
        data = f.read(16)
        if data[:4] == b'\x00\x00\x01\x00':
            comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
            if uncomp_sz == 66688:
                print(f'File {i:4d}: comp_sz={comp_sz:5d}, uncomp_sz={uncomp_sz}')
                count += 1
                if count >= 10:
                    break
