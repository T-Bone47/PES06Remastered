import struct, zlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]

    face_like_files = []
    hair_like_files = []

    for i in range(min(num_files, 3000)):
        off, sz = entries[i]
        f.seek(off)
        hdr = f.read(16)
        if hdr[:4] in (b'\x00\x00\x01\x00', b'\x00\x06\x01\x00'):
            comp_sz, uncomp_sz = struct.unpack('<II', hdr[4:12])
            if 50000 <= uncomp_sz <= 80000:
                face_like_files.append((i, comp_sz, uncomp_sz, hdr[:4].hex()))
            elif 3000 <= uncomp_sz <= 20000:
                hair_like_files.append((i, comp_sz, uncomp_sz, hdr[:4].hex()))

print(f'Total files in 0_text.afs: {num_files}')
print(f'Files with face-like uncompressed sizes (50k-80k) in first 3000: {len(face_like_files)}')
print(f'Files with hair-like uncompressed sizes (3k-20k) in first 3000: {len(hair_like_files)}')
print('Sample face-like files:', face_like_files[:15])
print('Sample hair-like files:', hair_like_files[:15])
