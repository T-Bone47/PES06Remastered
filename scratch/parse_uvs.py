import struct

with open('scratch/sample_face_1.bin', 'rb') as f:
    data = f.read()

# Face file size: 66,688 bytes
# Texture is first 11,264 bytes
# Mesh vertex data starts after texture data (around 0x2C00)
# Let's search for float pairs (u, v) in range [0.0, 1.0]
uv_floats = []
for p in range(11264, len(data) - 8, 4):
    u, v = struct.unpack('<ff', data[p:p+8])
    if 0.0 <= u <= 1.0 and 0.0 <= v <= 1.0:
        uv_floats.append((p, u, v))

print(f'Total valid (u, v) float pairs found: {len(uv_floats)}')
if uv_floats:
    min_u = min(x[1] for x in uv_floats)
    max_u = max(x[1] for x in uv_floats)
    min_v = min(x[2] for x in uv_floats)
    max_v = max(x[2] for x in uv_floats)
    print(f'UV range: U in [{min_u:.4f}, {max_u:.4f}], V in [{min_v:.4f}, {max_v:.4f}]')
    
    # Print sample UV clusters
    print('Sample UV pairs (first 10):')
    for p, u, v in uv_floats[:10]:
        print(f'  Offset 0x{p:04X}: U={u:.4f}, V={v:.4f}')
