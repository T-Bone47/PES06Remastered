"""
PES 6 Legacy Remaster - Stage 19 Premium Environment Graphics
Next-Generation Photorealistic Pitch Turf Generator

Synthesizes:
  1. stadium_default_turf.png (2048x1024) - Full pitch surface with authentic 16-band mower striping,
     organic blade clumping, subtle earth undertones, and zero synthetic grain.
  2. turf_square.png (1024x1024) - Seamless toroidal turf tile for close-ups, penalty box, and set-pieces.
"""

import os
import math
import numpy as np
from PIL import Image, ImageFilter

def generate_organic_pitch(width=2048, height=1024):
    print(f"Generating Premium Organic Pitch Turf ({width}x{height})...")
    np.random.seed(20060517)  # Iconic UCL Paris Final seed

    y_indices, x_indices = np.indices((height, width))
    nx = x_indices / float(width)
    ny = y_indices / float(height)

    # 1. 16-Band Alternating Mower Striping
    # Professional grass cut creates directional light reflection
    stripe_count = 16
    stripe_phase = (x_indices / (width / float(stripe_count)))
    # Smooth sinusoidal transition between mower stripes (not harsh blocky step)
    stripe_factor = 1.0 + 0.075 * np.sin(stripe_phase * 2.0 * np.pi)

    # 2. Organic Multi-Octave Turf Variation (Macro & Micro Clumping)
    # Low frequency turf health / moisture variation
    macro_variation = 0.5 + 0.5 * np.sin(nx * 6.0 * np.pi + np.cos(ny * 4.0 * np.pi))
    
    # Mid-frequency blade clusters
    np_noise1 = np.random.normal(0.0, 1.0, (height // 8, width // 8))
    img_n1 = Image.fromarray(np_noise1).resize((width, height), Image.Resampling.BICUBIC)
    mid_noise = np.array(img_n1, dtype=np.float32)

    # High-frequency blade micro-texture
    np_noise2 = np.random.normal(0.0, 1.0, (height // 2, width // 2))
    img_n2 = Image.fromarray(np_noise2).resize((width, height), Image.Resampling.BILINEAR)
    fine_noise = np.array(img_n2, dtype=np.float32)

    total_blade = (mid_noise * 14.0 + fine_noise * 8.0)

    # 3. Authentic European Stadium Turf Color Palette
    # Deep Emerald Base: RGB(38, 96, 36)
    # Sunlit Blade Tip:  RGB(58, 128, 46)
    # Earth Undertone:   RGB(32, 74, 28)
    base_r = 38.0 + (macro_variation - 0.5) * 6.0 + total_blade * 0.40
    base_g = 98.0 + (macro_variation - 0.5) * 12.0 + total_blade * 0.85
    base_b = 36.0 + (macro_variation - 0.5) * 5.0 + total_blade * 0.35

    # Apply mower striping reflection
    r_chan = np.clip(base_r * stripe_factor, 18.0, 95.0)
    g_chan = np.clip(base_g * stripe_factor, 55.0, 165.0)
    b_chan = np.clip(base_b * stripe_factor, 15.0, 80.0)

    # 4. Subtle Radial Stadium Vignette (natural stadium shadow falloff)
    cx, cy = 0.50, 0.50
    dist_sq = ((nx - cx) / 0.55)**2 + ((ny - cy) / 0.55)**2
    vignette = np.clip(1.0 - dist_sq * 0.08, 0.90, 1.0)[:, :, np.newaxis]

    rgb = np.dstack([r_chan, g_chan, b_chan]) * vignette
    alpha = np.full((height, width, 1), 255.0, dtype=np.float32)
    rgba = np.dstack([rgb, alpha])

    return np.clip(rgba, 0, 255).astype(np.uint8)

def generate_seamless_turf_square(size=1024):
    print(f"Generating Seamless Toroidal Turf Square ({size}x{size})...")
    np.random.seed(42069)

    y_indices, x_indices = np.indices((size, size))
    nx_rad = (x_indices / float(size)) * 2.0 * np.pi
    ny_rad = (y_indices / float(size)) * 2.0 * np.pi

    # Toroidal 4D seamless coordinates
    cx = np.cos(nx_rad)
    sx = np.sin(nx_rad)
    cy = np.cos(ny_rad)
    sy = np.sin(ny_rad)

    # Deterministic toroidal blade noise
    torus_noise = (np.sin(cx * 3.0 + cy * 3.0) * np.cos(sx * 2.0 - sy * 2.0)) * 12.0
    fine_noise = np.random.normal(0.0, 6.0, (size, size))

    blade = torus_noise + fine_noise

    # Mower stripe (subtle)
    stripe = 1.0 + 0.04 * np.sin(nx_rad * 4.0)

    r_chan = np.clip((40.0 + blade * 0.40) * stripe, 20.0, 90.0)
    g_chan = np.clip((102.0 + blade * 0.85) * stripe, 60.0, 160.0)
    b_chan = np.clip((38.0 + blade * 0.35) * stripe, 18.0, 75.0)

    rgb = np.dstack([r_chan, g_chan, b_chan])
    alpha = np.full((size, size, 1), 255.0, dtype=np.float32)
    rgba = np.dstack([rgb, alpha])

    return np.clip(rgba, 0, 255).astype(np.uint8)

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dirs = [
        os.path.join(root, "PES 6 game files", "remaster_pack", "pitch"),
        os.path.join(root, "test_game", "remaster_pack", "pitch")
    ]

    pitch_img = Image.fromarray(generate_organic_pitch(2048, 1024), 'RGBA')
    turf_sq_img = Image.fromarray(generate_seamless_turf_square(1024), 'RGBA')

    for d in target_dirs:
        os.makedirs(d, exist_ok=True)
        pitch_path = os.path.join(d, "stadium_default_turf.png")
        turf_sq_path = os.path.join(d, "turf_square.png")

        pitch_img.save(pitch_path, "PNG", optimize=True)
        turf_sq_img.save(turf_sq_path, "PNG", optimize=True)
        print(f"Saved {pitch_path} ({os.path.getsize(pitch_path)} bytes)")
        print(f"Saved {turf_sq_path} ({os.path.getsize(turf_sq_path)} bytes)")

if __name__ == '__main__':
    main()
