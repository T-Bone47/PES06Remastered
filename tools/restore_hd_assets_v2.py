import os
import math
import struct
import zlib
import hashlib
import json

def write_png_rgba(width, height, rgba_bytes):
    """Write compliant 32-bit RGBA PNG with zlib level 9 compression."""
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr_crc = zlib.crc32(b'IHDR' + ihdr_data)
    ihdr_chunk = struct.pack('>I', 13) + b'IHDR' + ihdr_data + struct.pack('>I', ihdr_crc)

    raw_scanlines = bytearray()
    row_bytes = width * 4
    for y in range(height):
        raw_scanlines.append(0)  # Filter type 0 (None)
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

# -------------------------------------------------------------------------
# Photorealistic Face Generator in 1:2 Aspect Ratio (1024 x 2048)
# -------------------------------------------------------------------------
def generate_photorealistic_face_1024x2048(player_name, skin_tone, eye_color, width=1024, height=2048):
    """
    Generates a 1024x2048 (1:2 aspect ratio) photorealistic face texture.
    Matches PES 6's 64x128 UV mapping at exactly 16x supersampling in both axes.
    """
    buf = bytearray(width * height * 4)
    r_base, g_base, b_base = skin_tone
    er, eg, eb = eye_color
    seed = sum(ord(c) for c in player_name) + 42

    for y in range(height):
        ny = y / float(height)  # 0.0 to 1.0 (corresponds to V in UV space)
        row_offset = y * width * 4
        for x in range(width):
            nx = x / float(width)  # 0.0 to 1.0 (corresponds to U in UV space)
            dx = (nx - 0.5) * 2.0  # -1.0 to 1.0 from center

            # Head/face mask calculation in 1:2 space
            # Vertical center of face is around ny = 0.52
            dy = (ny - 0.52) * 2.0

            # Elliptical face contour
            dist_sq = (dx / 0.74) ** 2 + (dy / 0.88) ** 2
            dist = math.sqrt(dist_sq)

            # Ear regions
            is_ear_left = (0.06 <= nx <= 0.20) and (0.42 <= ny <= 0.60)
            is_ear_right = (0.80 <= nx <= 0.94) and (0.42 <= ny <= 0.60)
            is_ear = is_ear_left or is_ear_right

            if dist > 1.06 and not is_ear:
                # Transparent boundary
                continue

            # Alpha edge feathering
            if is_ear:
                alpha = 255
            elif dist > 0.96:
                alpha = int(255.0 * (1.0 - (dist - 0.96) / 0.10))
                alpha = max(0, min(255, alpha))
            else:
                alpha = 255

            # Lateral cylindrical surface shading (curving around head)
            lat_shade = math.cos(dx * 1.25)
            lat_shade = max(0.45, min(1.0, lat_shade))

            # Vertical anatomical light falloff (light from upper-front)
            v_shade = 1.0 - 0.16 * (ny - 0.35)

            # High-frequency skin pore / micro-texture
            pore = (smooth_noise(x, y, 0.35, seed) - 0.5) * 12.0

            skin_r = max(0, min(255, int(r_base * v_shade * lat_shade + pore)))
            skin_g = max(0, min(255, int(g_base * v_shade * lat_shade + pore)))
            skin_b = max(0, min(255, int(b_base * v_shade * lat_shade + pore)))

            # Eyebrows (ny in 0.34..0.38)
            in_brow_l = (0.30 <= nx <= 0.46) and (0.345 <= ny <= 0.380)
            in_brow_r = (0.54 <= nx <= 0.70) and (0.345 <= ny <= 0.380)
            if in_brow_l or in_brow_r:
                brow_tex = smooth_noise(x, y, 0.45, seed + 10)
                if brow_tex > 0.32:
                    skin_r = int(skin_r * 0.32)
                    skin_g = int(skin_g * 0.30)
                    skin_b = int(skin_b * 0.28)

            # Eyes (Left Eye center: X=390, Y=860; Right Eye center: X=634, Y=860)
            for ecx in [390, 634]:
                edx = (x - ecx) / 46.0
                edy = (y - 860) / 24.0
                edist = math.sqrt(edx * edx + edy * edy)
                if edist < 1.0:
                    # Inside eye aperture
                    idx = (x - ecx) / 19.0
                    idy = (y - 860) / 19.0
                    idist = math.sqrt(idx * idx + idy * idy)
                    if idist < 0.32:
                        # Pupil
                        skin_r, skin_g, skin_b = 14, 14, 16
                        # Specular catchlight
                        if -0.2 < idx < 0.0 and -0.2 < idy < 0.0:
                            skin_r, skin_g, skin_b = 245, 250, 255
                    elif idist < 0.85:
                        # Iris with radial fibers
                        fiber = smooth_noise(x, y, 0.8, seed + 20)
                        skin_r = int(er * (0.8 + 0.3 * fiber))
                        skin_g = int(eg * (0.8 + 0.3 * fiber))
                        skin_b = int(eb * (0.8 + 0.3 * fiber))
                    else:
                        # Sclera
                        s_shade = 222 - int(edist * 28.0)
                        skin_r, skin_g, skin_b = s_shade, s_shade - 8, s_shade - 12

            # Nose (Bridge: ny in 0.44..0.55, Nostrils: ny in 0.55..0.59)
            if 0.45 <= nx <= 0.55 and 0.44 <= ny <= 0.59:
                ndx = abs(nx - 0.5) / 0.05
                if ny < 0.55:
                    # Bridge highlight in center
                    if ndx < 0.32:
                        skin_r = min(255, skin_r + 18)
                        skin_g = min(255, skin_g + 16)
                        skin_b = min(255, skin_b + 14)
                    else:
                        skin_r = int(skin_r * 0.86)
                        skin_g = int(skin_g * 0.86)
                        skin_b = int(skin_b * 0.86)
                else:
                    # Nostrils
                    if (0.465 <= nx <= 0.485 or 0.515 <= nx <= 0.535) and (0.56 <= ny <= 0.585):
                        skin_r, skin_g, skin_b = int(skin_r * 0.28), int(skin_g * 0.24), int(skin_b * 0.24)

            # Lips & Mouth (ny in 0.65..0.73, nx in 0.41..0.59)
            if 0.41 <= nx <= 0.59 and 0.65 <= ny <= 0.73:
                mdx = (nx - 0.5) / 0.09
                mdy = (ny - 0.69) / 0.04
                mdist = math.sqrt(mdx * mdx + mdy * mdy)
                if mdist < 1.0:
                    if player_name == 'Ronaldinho' and abs(mdx) < 0.68 and abs(mdy) < 0.35:
                        # Ronaldinho wide smile showing teeth
                        skin_r, skin_g, skin_b = 235, 235, 225
                    else:
                        lip_r = min(255, int(r_base * 1.25 + 22))
                        lip_g = int(g_base * 0.78)
                        lip_b = int(b_base * 0.78)
                        # Mouth seam line
                        if abs(ny - 0.69) < 0.005 and abs(mdx) < 0.85:
                            lip_r, lip_g, lip_b = int(lip_r * 0.4), int(lip_g * 0.3), int(lip_b * 0.3)
                        skin_r, skin_g, skin_b = lip_r, lip_g, lip_b

            # Player-specific facial hair & details
            if player_name in ('Adriano', 'Henry'):
                # Goatee & moustache outline
                is_goatee = (abs(nx - 0.5) < 0.12 and ny > 0.63) or (abs(nx - 0.5) < 0.07 and 0.61 <= ny <= 0.65)
                if is_goatee:
                    fh_noise = smooth_noise(x, y, 0.45, seed + 30)
                    if fh_noise > 0.38:
                        skin_r = int(skin_r * 0.45)
                        skin_g = int(skin_g * 0.42)
                        skin_b = int(skin_b * 0.40)
            elif player_name == 'Gerrard':
                # Light 5-o'clock shadow
                if abs(dx) < 0.65 and ny > 0.63:
                    fh_noise = smooth_noise(x, y, 0.5, seed + 30)
                    if fh_noise > 0.42:
                        skin_r = int(skin_r * 0.65)
                        skin_g = int(skin_g * 0.62)
                        skin_b = int(skin_b * 0.58)
            elif player_name == 'Rooney':
                # Light freckles across bridge and cheeks
                if (0.35 <= nx <= 0.65) and (0.46 <= ny <= 0.56):
                    freckle = smooth_noise(x * 2.0, y * 2.0, 0.7, seed + 50)
                    if freckle > 0.55:
                        skin_r = int(skin_r * 0.82)
                        skin_g = int(skin_g * 0.75)
                        skin_b = int(skin_b * 0.70)

            # Cheek flush / blush
            if (0.28 <= nx <= 0.42 or 0.58 <= nx <= 0.72) and (0.48 <= ny <= 0.60):
                skin_r = min(255, skin_r + 14)

            pixel_idx = row_offset + x * 4
            buf[pixel_idx + 0] = skin_r
            buf[pixel_idx + 1] = skin_g
            buf[pixel_idx + 2] = skin_b
            buf[pixel_idx + 3] = alpha

    return bytes(buf)

# -------------------------------------------------------------------------
# Photorealistic Hair Generator (1024 x 512, 2:1 Aspect Ratio)
# -------------------------------------------------------------------------
def generate_photorealistic_hair_1024x512(player_name, hair_color, style='crop', width=1024, height=512):
    """
    Generates a 1024x512 (2:1 aspect ratio) hair texture with transparent alpha.
    """
    buf = bytearray(width * height * 4)
    hr, hg, hb = hair_color
    seed = sum(ord(c) for c in player_name) + 999

    for y in range(height):
        ny = y / float(height)
        row_offset = y * width * 4
        for x in range(width):
            nx = x / float(width)
            dx = (nx - 0.5) * 2.0

            dist_sq = (dx / 0.85) ** 2 + ((ny - 0.5) / 0.46) ** 2
            dist = math.sqrt(dist_sq)

            if dist > 1.08:
                continue

            # Alpha falloff at outer perimeter
            if dist > 0.85:
                alpha = int(255.0 * (1.0 - (dist - 0.85) / 0.23))
                alpha = max(0, min(255, alpha))
            else:
                alpha = 255

            strand_freq = 0.55 if style == 'crop' else 0.32
            strand = smooth_noise(x * 3.0, y * 0.5, strand_freq, seed)
            sheen = math.sin(ny * 3.14159) * 0.22

            hair_r = max(0, min(255, int(hr * (0.75 + 0.35 * strand + sheen))))
            hair_g = max(0, min(255, int(hg * (0.75 + 0.35 * strand + sheen))))
            hair_b = max(0, min(255, int(hb * (0.75 + 0.35 * strand + sheen))))

            # Ronaldinho signature headband across forehead
            if player_name == 'Ronaldinho' and 0.28 <= ny <= 0.36 and dist < 0.95:
                if abs(ny - 0.32) < 0.008:
                    hair_r, hair_g, hair_b = 30, 30, 30
                else:
                    hair_r, hair_g, hair_b = 240, 240, 245
                alpha = 255

            pixel_idx = row_offset + x * 4
            buf[pixel_idx + 0] = hair_r
            buf[pixel_idx + 1] = hair_g
            buf[pixel_idx + 2] = hair_b
            buf[pixel_idx + 3] = alpha

    return bytes(buf)

# -------------------------------------------------------------------------
# Seamless Ultra-HD Turf (2048 x 1024)
# -------------------------------------------------------------------------
def generate_seamless_turf_2048x1024(width=2048, height=1024):
    buf = bytearray(width * height * 4)
    stripe_width = 128

    for y in range(height):
        ny_rad = (y / float(height)) * 2.0 * math.pi
        row_offset = y * width * 4
        for x in range(width):
            nx_rad = (x / float(width)) * 2.0 * math.pi

            stripe_idx = (x // stripe_width) % 2
            stripe_factor = 1.08 if stripe_idx == 0 else 0.92

            cx, sx = math.cos(nx_rad), math.sin(nx_rad)
            cy, sy = math.cos(ny_rad), math.sin(ny_rad)

            b1 = hash2d(int((cx + 1.0) * 1024), int((cy + 1.0) * 1024), 42)
            b2 = hash2d(int((sx + 1.0) * 1024), int((sy + 1.0) * 1024), 84)
            blade = (b1 * 0.6 + b2 * 0.4 - 0.5) * 30.0

            r = max(20, min(140, int((48.0 + blade * 0.5) * stripe_factor)))
            g = max(60, min(180, int((112.0 + blade) * stripe_factor)))
            b = max(15, min(100, int((38.0 + blade * 0.4) * stripe_factor)))

            pixel_idx = row_offset + x * 4
            buf[pixel_idx + 0] = r
            buf[pixel_idx + 1] = g
            buf[pixel_idx + 2] = b
            buf[pixel_idx + 3] = 255

    return bytes(buf)

# -------------------------------------------------------------------------
# Execution & Deployment
# -------------------------------------------------------------------------
def main():
    print('=== Stage 16: Synthesizing Photorealistic 1:2 Faces & HD Assets ===')

    players = [
        ('Adriano',    (146, 96, 70),   (45, 30, 20),   (25, 25, 28),   'crop'),
        ('Henry',      (124, 78, 52),   (48, 32, 22),   (30, 25, 25),   'crop'),
        ('Ronaldinho', (168, 116, 78),  (60, 42, 26),   (20, 20, 22),   'dreads'),
        ('Gerrard',    (218, 174, 148), (62, 92, 122),  (85, 62, 42),   'crop'),
        ('Rooney',     (226, 172, 156), (70, 105, 135), (118, 88, 58),  'crop'),
    ]

    assets = {}

    for name, skin, eye, hcol, hstyle in players:
        print(f'Generating photorealistic 1:2 face for {name} (1024x2048)...')
        face_rgba = generate_photorealistic_face_1024x2048(name, skin, eye, 1024, 2048)
        assets[f'face_{name}'] = write_png_rgba(1024, 2048, face_rgba)

        print(f'Generating hair for {name} (1024x512)...')
        hair_rgba = generate_photorealistic_hair_1024x512(name, hcol, hstyle, 1024, 512)
        assets[f'hair_{name}'] = write_png_rgba(1024, 512, hair_rgba)

    print('Generating seamless turf (2048x1024)...')
    turf_rgba = generate_seamless_turf_2048x1024(2048, 1024)
    assets['turf_2k'] = write_png_rgba(2048, 1024, turf_rgba)

    print('Generating turf fallback (1024x1024)...')
    turf_sq_rgba = generate_seamless_turf_2048x1024(1024, 1024)
    assets['turf_sq'] = write_png_rgba(1024, 1024, turf_sq_rgba)

    # Deploy to both directories
    deploy_roots = ['PES 6 game files', 'test_game']
    club_map = {
        'Adriano': ('Inter', 'Adriano'),
        'Henry': ('Arsenal', 'Henry'),
        'Ronaldinho': ('Barcelona', 'Ronaldinho'),
        'Gerrard': ('Liverpool', 'Gerrard'),
        'Rooney': ('Man_Utd', 'Rooney'),
    }

    for root in deploy_roots:
        for p_name, (club, base_f) in club_map.items():
            face_p = f'{root}/GDB/faces/{club}/{base_f}.png'
            hair_p = f'{root}/GDB/hair/{club}/{base_f}_hair.png'
            with open(face_p, 'wb') as f:
                f.write(assets[f'face_{p_name}'])
            with open(hair_p, 'wb') as f:
                f.write(assets[f'hair_{p_name}'])
            print(f'Deployed {face_p} (1024x2048, {len(assets[f"face_{p_name}"])} bytes)')
            print(f'Deployed {hair_p} (1024x512, {len(assets[f"hair_{p_name}"])} bytes)')

        # Turf
        with open(f'{root}/remaster_pack/pitch/stadium_default_turf.png', 'wb') as f:
            f.write(assets['turf_2k'])
        with open(f'{root}/remaster_pack/pitch/turf_square.png', 'wb') as f:
            f.write(assets['turf_sq'])

        # player_001.png in remaster_pack
        with open(f'{root}/remaster_pack/faces/player_001.png', 'wb') as f:
            f.write(assets['face_Adriano'])

    # Update manifest.json
    for root in deploy_roots:
        man_path = f'{root}/remaster_pack/manifest.json'
        with open(man_path, 'r', encoding='utf-8') as f:
            man = json.load(f)
        for item in man['assets']:
            item_path = f'{root}/remaster_pack/{item["file"]}'
            with open(item_path, 'rb') as fi:
                item['sha256'] = hashlib.sha256(fi.read()).hexdigest()
        with open(man_path, 'w', encoding='utf-8') as f:
            json.dump(man, f, indent=2)
        print(f'Updated {man_path}')

    print('=== Stage 16 Asset Synthesis & Deployment Complete ===')

if __name__ == '__main__':
    main()
