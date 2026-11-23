import struct, zlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    
    face_bins = []
    print(f'Scanning {num_files} files in 0_text.afs for face models...')
    for i in range(num_files):
        off, sz = entries[i]
        f.seek(off)
        data = f.read(16)
        if data[:4] in (b'\x00\x00\x01\x00', b'\x00\x06\x01\x00'):
            comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
            if uncomp_sz == 66688:
                face_bins.append(i)

print(f'Total files with uncompressed size 66688: {len(face_bins)}')
print('List of all 66688 files:', face_bins)
