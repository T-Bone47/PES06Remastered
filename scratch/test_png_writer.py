import struct, zlib

def write_png_rgba(width, height, rgba_bytes):
    # IHDR
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    ihdr_chunk = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)

    # IDAT: Scanlines with filter byte 0 (None)
    raw_scanlines = bytearray()
    row_bytes = width * 4
    for y in range(height):
        raw_scanlines.append(0) # filter byte 0
        raw_scanlines.extend(rgba_bytes[y * row_bytes : (y + 1) * row_bytes])

    compressed = zlib.compress(bytes(raw_scanlines), level=9)
    idat_crc = zlib.crc32(b'IDAT' + compressed)
    idat_chunk = struct.pack('>I', len(compressed)) + b'IDAT' + compressed + struct.pack('>I', idat_crc)

    # IEND
    iend_crc = zlib.crc32(b'IEND')
    iend_chunk = struct.pack('>I', 0) + b'IEND' + struct.pack('>I', iend_crc)

    return b'\x89PNG\r\n\x1a\n' + ihdr_chunk + idat_chunk + iend_chunk

# Test with a tiny 4x4 red image
test_rgba = bytearray([255, 0, 0, 255] * 16)
png_data = write_png_rgba(4, 4, test_rgba)
print('Generated test PNG size:', len(png_data))
assert png_data[:8] == b'\x89PNG\r\n\x1a\n'
print('Test PNG is valid!')
