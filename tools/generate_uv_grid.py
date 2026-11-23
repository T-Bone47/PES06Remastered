import math
import struct
import zlib
import os

def write_png_rgba(width, height, rgba_bytes):
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    ihdr_chunk = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)

    raw_scanlines = bytearray()
    row_bytes = width * 4
    for y in range(height):
        raw_scanlines.append(0)
        raw_scanlines.extend(rgba_bytes[y * row_bytes : (y + 1) * row_bytes])

    compressed = zlib.compress(bytes(raw_scanlines), level=9)
    idat_crc = zlib.crc32(b'IDAT' + compressed)
    idat_chunk = struct.pack('>I', len(compressed)) + b'IDAT' + compressed + struct.pack('>I', idat_crc)

    iend_crc = zlib.crc32(b'IEND')
    iend_chunk = struct.pack('>I', 0) + b'IEND' + struct.pack('>I', iend_crc)

    return b'\x89PNG\r\n\x1a\n' + ihdr_chunk + idat_chunk + iend_chunk

# Minimal 5x7 bitmap font for rendering numbers and letters on the UV grid
FONT_5X7 = {
    '0': ["11111", "10001", "10011", "10101", "11001", "10001", "11111"],
    '1': ["00100", "01100", "00100", "00100", "00100", "00100", "01110"],
    '2': ["11111", "00001", "00001", "11111", "10000", "10000", "11111"],
    '3': ["11111", "00001", "00001", "01111", "00001", "00001", "11111"],
    '4': ["10001", "10001", "10001", "11111", "00001", "00001", "00001"],
    '5': ["11111", "10000", "10000", "11111", "00001", "00001", "11111"],
    '6': ["11111", "10000", "10000", "11111", "10001", "10001", "11111"],
    '7': ["11111", "00001", "00010", "00100", "01000", "01000", "01000"],
    '8': ["11111", "10001", "10001", "11111", "10001", "10001", "11111"],
    '9': ["11111", "10001", "10001", "11111", "00001", "00001", "11111"],
    'U': ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    'V': ["10001", "10001", "10001", "10001", "10001", "01010", "00100"],
    '_': ["00000", "00000", "00000", "00000", "00000", "00000", "11111"],
    ' ': ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
}

def draw_text(buf, width, height, text, start_x, start_y, color_rgb, scale=2):
    cx = start_x
    for char in text.upper():
        glyph = FONT_5X7.get(char, FONT_5X7[' '])
        for row_i, row_str in enumerate(glyph):
            for col_i, bit in enumerate(row_str):
                if bit == '1':
                    for sy in range(scale):
                        for sx in range(scale):
                            px = cx + col_i * scale + sx
                            py = start_y + row_i * scale + sy
                            if 0 <= px < width and 0 <= py < height:
                                idx = (py * width + px) * 4
                                buf[idx+0] = color_rgb[0]
                                buf[idx+1] = color_rgb[1]
                                buf[idx+2] = color_rgb[2]
                                buf[idx+3] = 255
        cx += (5 + 1) * scale

def generate_uv_calibration_grid(width=1024, height=2048):
    """
    Generates a 1024x2048 UV calibration grid:
    16 columns x 32 rows of 64x64 checker cells with sub-pixel crosshairs,
    border coordinates, and landmark reference lines.
    """
    buf = bytearray(width * height * 4)
    cell_w = 64
    cell_h = 64
    cols = width // cell_w   # 16
    rows = height // cell_h  # 32

    # Color palette for 4-quadrant chromatic identification
    palette = [
        ((40, 160, 220), (20, 60, 100)),   # Cyan / Deep Blue (Quadrant I)
        ((220, 50, 150), (100, 20, 60)),   # Magenta / Deep Plum (Quadrant II)
        ((230, 200, 40), (100, 90, 15)),   # Yellow / Dark Olive (Quadrant III)
        ((50, 210, 80),  (20, 90, 35)),    # Green / Dark Forest (Quadrant IV)
    ]

    for y in range(height):
        cell_y = y // cell_h
        in_cell_y = y % cell_h
        quad_y = 0 if cell_y < 16 else 1
        row_offset = y * width * 4

        for x in range(width):
            cell_x = x // cell_w
            in_cell_x = x % cell_w
            quad_x = 0 if cell_x < 8 else 1
            quad_idx = quad_y * 2 + quad_x

            # Checkerboard within cell
            is_check = ((cell_x + cell_y) % 2 == 0)
            c_light, c_dark = palette[quad_idx]
            base_col = c_light if is_check else c_dark

            r, g, b = base_col

            # 1px border around every 64x64 cell
            is_cell_border = (in_cell_x == 0 or in_cell_x == cell_w - 1 or in_cell_y == 0 or in_cell_y == cell_h - 1)
            if is_cell_border:
                r, g, b = (255, 255, 255) if is_check else (0, 0, 0)

            # Center crosshairs inside cell
            is_crosshair = (in_cell_x == cell_w // 2 or in_cell_y == cell_h // 2)
            if is_crosshair and (in_cell_x % 4 == 0 or in_cell_y % 4 == 0):
                r, g, b = (255, 255, 0)

            # 8-pixel tick marks
            if (in_cell_x % 8 == 0 or in_cell_y % 8 == 0) and (in_cell_x in (0, 1, cell_w-2, cell_w-1) or in_cell_y in (0, 1, cell_h-2, cell_h-1)):
                r, g, b = (255, 255, 255)

            # Prominent Anatomical Horizon Lines:
            # Forehead: row 8 (Y=512)
            # Eyes: row 13 (Y=832)
            # Nose Tip: row 18 (Y=1152)
            # Mouth: row 22 (Y=1408)
            # Chin: row 26 (Y=1664)
            # Central Vertical Symmetry Axis: X=512
            if abs(x - 512) <= 1:
                r, g, b = 255, 0, 0  # Bright Red Vertical Center Axis
            elif cell_y in (8, 13, 18, 22, 26) and in_cell_y in (0, 1):
                r, g, b = 255, 255, 0  # Yellow Landmark Horizons

            pixel_idx = row_offset + x * 4
            buf[pixel_idx + 0] = r
            buf[pixel_idx + 1] = g
            buf[pixel_idx + 2] = b
            buf[pixel_idx + 3] = 255

    # Draw coordinate text into each cell (e.g. "U05_V12")
    for cy in range(rows):
        for cx in range(cols):
            text = f"U{cx:02d}V{cy:02d}"
            tx = cx * cell_w + 4
            ty = cy * cell_h + 24
            # Dark drop shadow
            draw_text(buf, width, height, text, tx + 1, ty + 1, (0, 0, 0), scale=1)
            # Bright text
            draw_text(buf, width, height, text, tx, ty, (255, 255, 255), scale=1)

    return bytes(buf)

def main():
    print("Generating High-Precision 1024x2048 UV Calibration Grid...")
    grid_rgba = generate_uv_calibration_grid(1024, 2048)
    png_bytes = write_png_rgba(1024, 2048, grid_rgba)

    out_paths = [
        "scratch/uv_calibration_grid_1024x2048.png",
        "PES 6 game files/remaster_pack/faces/player_001.png",
        "PES 6 game files/GDB/faces/Arsenal/Henry.png",
        "test_game/GDB/faces/Arsenal/Henry.png",
    ]

    for p in out_paths:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "wb") as f:
            f.write(png_bytes)
        print(f"Saved UV grid to {p} ({len(png_bytes)} bytes)")

    # Also copy to artifact directory
    art_path = r"C:\Users\dream\.gemini\antigravity\brain\781e5817-86f4-4879-a612-de7b97374a19\uv_calibration_grid_1024x2048.png"
    with open(art_path, "wb") as f:
        f.write(png_bytes)
    print(f"Copied to artifact directory: {art_path}")

if __name__ == '__main__':
    main()
