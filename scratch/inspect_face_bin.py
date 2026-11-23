import struct, zlib

afs_path = 'PES 6 game files/dat/0_text.afs'
with open(afs_path, 'rb') as f:
    f.seek(4)
    num_files, = struct.unpack('<I', f.read(4))
    entries = [struct.unpack('<II', f.read(8)) for _ in range(num_files)]
    f.seek(entries[1][0])
    data = f.read(entries[1][1])

comp_sz, uncomp_sz = struct.unpack('<II', data[4:12])
decomp = zlib.decompress(data[32:32+comp_sz])
print('File 1 decompressed size:', len(decomp))
print('Header (64 bytes):', decomp[:64].hex(' '))

# Save decompressed File 1 as a sample face bin
with open('scratch/sample_face_1.bin', 'wb') as out:
    out.write(decomp)

print('Saved scratch/sample_face_1.bin successfully')
