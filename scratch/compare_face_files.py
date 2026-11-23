import struct, zlib, hashlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    
    print('Comparing face files 1, 3, 5, 7, 9, 11, 13, 15:')
    for idx in [1, 3, 5, 7, 9, 11, 13, 15]:
        off, sz = entries[idx]
        f.seek(off)
        data = f.read(sz)
        comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
        decomp = zlib.decompress(data[32:32+comp_sz])
        h_full = hashlib.sha256(decomp).hexdigest()[:12]
        h_mesh = hashlib.sha256(decomp[11264:]).hexdigest()[:12]
        h_tex  = hashlib.sha256(decomp[:11264]).hexdigest()[:12]
        print(f'File {idx:2d}: size={len(decomp)} bytes | full_sha={h_full} | tex_sha={h_tex} | mesh_sha={h_mesh}')
