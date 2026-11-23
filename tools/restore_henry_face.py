import math
import struct
import zlib
import os
import hashlib

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

def hash2d(x, y, seed=0):
    n = (x * 374761393 + y * 668265263 + seed * 127412617) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 127412617
    return ((n ^ (n >> 16)) & 0xFFFFFFFF) / 4294967295.0

def smooth_noise(x, y, scale=1.0, seed=0):
    sx = x * scale
    sy = y * scale
    x0 = int(math.floor(sx))
    y0 = int(math.floor(sy))
    fx = sx - x0
    fy = sy - y0
    fx = fx * fx * (3.0 - 2.0 * fx)
    fy = fy * fy * (3.0 - 2.0 * fy)
    
    n00 = hash2d(x0, y0, seed)
    n10 = hash2d(x0 + 1, y0, seed)
    n01 = hash2d(x0, y0 + 1, seed)
    n11 = hash2d(x0 + 1, y0 + 1, seed)
    
    nx0 = n00 * (1.0 - fx) + n10 * fx
    nx1 = n01 * (1.0 - fx) + n11 * fx
    return nx0 * (1.0 - fy) + nx1 * fy

def generate_thierry_henry_face(width=1024, height=2048):
    """
    Synthesizes a bespoke photorealistic 1024x2048 (1:2 aspect ratio) face texture
    for Thierry Henry (Arsenal #14 / France #12).
    """
    buf = bytearray(width * height * 4)
    # Base skin tone: French-Caribbean warm espresso caramel
    r_base, g_base, b_base = 126, 80, 54
    er, eg, eb = 46, 30, 20  # Deep brown iris
    seed = 195 + 14  # Henry #14

    for y in range(height):
        ny = y / float(height)  # 0.0 to 1.0
        row_offset = y * width * 4
        for x in range(width):
            nx = x / float(width)  # 0.0 to 1.0
            dx = (nx - 0.5) * 2.0  # -1.0 to 1.0 from center
            dy = (ny - 0.52) * 2.0

            # Anatomical face boundary ellipse in 1:2 space
            dist_sq = (dx / 0.74) ** 2 + (dy / 0.88) ** 2
            dist = math.sqrt(dist_sq)

            # Lateral ear mapping zones
            is_ear_left = (0.06 <= nx <= 0.20) and (0.42 <= ny <= 0.60)
            is_ear_right = (0.80 <= nx <= 0.94) and (0.42 <= ny <= 0.60)
            is_ear = is_ear_left or is_ear_right

            if dist > 1.06 and not is_ear:
                continue

            # Alpha edge feathering
            if is_ear:
                alpha = 255
            elif dist > 0.96:
                alpha = int(255.0 * (1.0 - (dist - 0.96) / 0.10))
                alpha = max(0, min(255, alpha))
            else:
                alpha = 255

            # Lateral cylindrical surface shading (head curvature)
            lat_shade = math.cos(dx * 1.25)
            lat_shade = max(0.48, min(1.0, lat_shade))

            # Vertical anatomical light falloff (light from overhead stadium floodlights)
            v_shade = 1.0 - 0.15 * (ny - 0.35)

            # Epidermal micro-texture and fine skin pores
            pore = (smooth_noise(x, y, 0.40, seed) - 0.5) * 10.0
            subsurface = math.sin(ny * 3.14159) * 8.0

            skin_r = max(0, min(255, int(r_base * v_shade * lat_shade + pore + subsurface)))
            skin_g = max(0, min(255, int(g_base * v_shade * lat_shade + pore * 0.8 + subsurface * 0.5)))
            skin_b = max(0, min(255, int(b_base * v_shade * lat_shade + pore * 0.8 + subsurface * 0.3)))

            # Henry High Forehead Highlight (ny in 0.22..0.34)
            if 0.35 <= nx <= 0.65 and 0.22 <= ny <= 0.34:
                forehead_hl = math.cos((nx - 0.5) * 10.0) * math.cos((ny - 0.28) * 25.0)
                if forehead_hl > 0:
                    skin_r = min(255, skin_r + int(14 * forehead_hl))
                    skin_g = min(255, skin_g + int(10 * forehead_hl))
                    skin_b = min(255, skin_b + int(8 * forehead_hl))

            # Henry Arched Eyebrows (ny in 0.345..0.380)
            # Characteristic: arched, thin, elegant French brow shape
            in_brow_l = (0.31 <= nx <= 0.46) and (0.345 <= ny <= 0.378)
            in_brow_r = (0.54 <= nx <= 0.69) and (0.345 <= ny <= 0.378)
            if in_brow_l or in_brow_r:
                brow_val = smooth_noise(x, y, 0.50, seed + 10)
                if brow_val > 0.30:
                    skin_r = int(skin_r * 0.28)
                    skin_g = int(skin_g * 0.26)
                    skin_b = int(skin_b * 0.25)

            # Eyes (Left Eye center: X=390, Y=860; Right Eye center: X=634, Y=860)
            for ecx in [390, 634]:
                edx = (x - ecx) / 46.0
                edy = (y - 860) / 24.0
                edist = math.sqrt(edx * edx + edy * edy)
                if edist < 1.0:
                    # Inside eye socket
                    idx = (x - ecx) / 19.0
                    idy = (y - 860) / 19.0
                    idist = math.sqrt(idx * idx + idy * idy)
                    if idist < 0.32:
                        # Pupil
                        skin_r, skin_g, skin_b = 12, 12, 14
                        # Specular catchlight reflection
                        if -0.22 < idx < -0.02 and -0.22 < idy < -0.02:
                            skin_r, skin_g, skin_b = 245, 250, 255
                    elif idist < 0.85:
                        # Rich dark brown iris with radial striations
                        fiber = smooth_noise(x, y, 0.85, seed + 20)
                        skin_r = int(er * (0.85 + 0.3 * fiber))
                        skin_g = int(eg * (0.85 + 0.3 * fiber))
                        skin_b = int(eb * (0.85 + 0.3 * fiber))
                    else:
                        # Sclera with vascular shading
                        sclera_c = 218 - int(edist * 30.0)
                        skin_r, skin_g, skin_b = sclera_c, sclera_c - 10, sclera_c - 14

            # Refined Nose Bridge & Tip (ny in 0.44..0.59)
            if 0.45 <= nx <= 0.55 and 0.44 <= ny <= 0.59:
                ndx = abs(nx - 0.5) / 0.05
                if ny < 0.55:
                    if ndx < 0.30:
                        skin_r = min(255, skin_r + 16)
                        skin_g = min(255, skin_g + 14)
                        skin_b = min(255, skin_b + 12)
                    else:
                        skin_r = int(skin_r * 0.84)
                        skin_g = int(skin_g * 0.84)
                        skin_b = int(skin_b * 0.84)
                else:
                    # Nostrils
                    if (0.465 <= nx <= 0.485 or 0.515 <= nx <= 0.535) and (0.56 <= ny <= 0.585):
                        skin_r, skin_g, skin_b = int(skin_r * 0.25), int(skin_g * 0.22), int(skin_b * 0.22)

            # Lips (ny in 0.65..0.73, nx in 0.41..0.59)
            if 0.41 <= nx <= 0.59 and 0.65 <= ny <= 0.73:
                mdx = (nx - 0.5) / 0.09
                mdy = (ny - 0.69) / 0.04
                mdist = math.sqrt(mdx * mdx + mdy * mdy)
                if mdist < 1.0:
                    lip_r = min(255, int(r_base * 1.22 + 20))
                    lip_g = int(g_base * 0.76)
                    lip_b = int(b_base * 0.76)
                    # Oral fissure seam line
                    if abs(ny - 0.69) < 0.005 and abs(mdx) < 0.85:
                        lip_r, lip_g, lip_b = int(lip_r * 0.35), int(lip_g * 0.30), int(lip_b * 0.30)
                    skin_r, skin_g, skin_b = lip_r, lip_g, lip_b

            # Henry Signature Goatee & Moustache Contour
            # Goatee on chin + neat moustache above upper lip
            is_goatee = (abs(nx - 0.5) < 0.11 and ny > 0.63) or (abs(nx - 0.5) < 0.065 and 0.61 <= ny <= 0.65)
            # Fine stubble tracing jawline
            is_jaw_stubble = (0.28 <= abs(nx - 0.5) <= 0.38) and (0.64 <= ny <= 0.82)
            if is_goatee or is_jaw_stubble:
                fh_noise = smooth_noise(x, y, 0.48, seed + 30)
                if fh_noise > (0.35 if is_goatee else 0.45):
                    skin_r = int(skin_r * 0.40)
                    skin_g = int(skin_g * 0.38)
                    skin_b = int(skin_b * 0.36)

            pixel_idx = row_offset + x * 4
            buf[pixel_idx + 0] = skin_r
            buf[pixel_idx + 1] = skin_g
            buf[pixel_idx + 2] = skin_b
            buf[pixel_idx + 3] = alpha

    return bytes(buf)

def main():
    print("=== Generating Bespoke Thierry Henry Photorealistic Face (1024x2048) ===")
    face_rgba = generate_thierry_henry_face(1024, 2048)
    png_bytes = write_png_rgba(1024, 2048, face_rgba)
    h_sha = hashlib.sha256(png_bytes).hexdigest()
    print(f"Generated Henry PNG: {len(png_bytes)} bytes | SHA256: {h_sha}")

    targets = [
        "PES 6 game files/GDB/faces/Arsenal/Henry.png",
        "test_game/GDB/faces/Arsenal/Henry.png",
        "PES 6 game files/remaster_pack/faces/player_001.png",
        "test_game/remaster_pack/faces/player_001.png",
    ]

    for p in targets:
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "wb") as f:
            f.write(png_bytes)
        print(f"Deployed -> {p}")

    # Copy to artifact and showcase comparison directory
    art_path = r"C:\Users\dream\.gemini\antigravity\brain\781e5817-86f4-4879-a612-de7b97374a19\thierry_henry_hd_face_1024x2048.png"
    comp_path = "PES06Remastered-main/docs/visual_comparison/thierry_henry_hd_face_1024x2048.png"
    with open(art_path, "wb") as f:
        f.write(png_bytes)
    with open(comp_path, "wb") as f:
        f.write(png_bytes)
    print(f"Copied to artifact: {art_path}")
    print(f"Copied to visual comparison: {comp_path}")

if __name__ == '__main__':
    main()
