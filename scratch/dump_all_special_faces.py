import struct, zlib, os

afs_path = 'PES 6 game files/dat/0_text.afs'
special_face_indices = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 46, 48, 448]

os.makedirs('scratch/special_faces', exist_ok=True)

with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    
    for idx in special_face_indices:
        off, sz = entries[idx]
        f.seek(off)
        data = f.read(sz)
        comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
        decomp = zlib.decompress(data[32:32+comp_sz])
        
        # Extract 256-color palette at 0x80 (1024 bytes)
        palette_raw = decomp[0x80 : 0x80 + 1024]
        pal = []
        for c in range(256):
            r, g, b, a = struct.unpack('<BBBB', palette_raw[c*4 : (c+1)*4])
            pal.append((r, g, b, 255))
            
        # Extract 64x128 pixels (8192 bytes)
        pixels_raw = decomp[0x80 + 1024 : 0x80 + 1024 + 8192]
        
        # Calculate skin tone average
        skin_r = sum(pal[p][0] for p in pixels_raw) / 8192.0
        skin_g = sum(pal[p][1] for p in pixels_raw) / 8192.0
        skin_b = sum(pal[p][2] for p in pixels_raw) / 8192.0
        
        # Save raw bin
        bin_path = f'scratch/special_faces/face_{idx:03d}.bin'
        with open(bin_path, 'wb') as fb:
            fb.write(decomp)
            
        print(f'Special Face File {idx:3d}: Skin Tone Avg RGB=({skin_r:.1f}, {skin_g:.1f}, {skin_b:.1f})')
