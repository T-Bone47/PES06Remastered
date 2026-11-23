"""
PES 6 Legacy Remaster - Stage 19 Stadium Environment Polish
High-Definition 2006 Stadium Perimeter Adboard Generator

Synthesizes:
  adboard_default.png (1024x128) - Authentic 2006 Champions League & Premier League sponsor board.
"""

import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont

def generate_hd_adboard(width=1024, height=128):
    print(f"Generating HD Stadium Adboard ({width}x{height})...")
    img = Image.new('RGBA', (width, height), (15, 18, 22, 255))
    draw = ImageDraw.Draw(img)

    # 4 distinct sponsor panels across the width (each 256px wide)
    panels = [
        {"bg": (0, 32, 96), "text": "PlayStation 2", "fg": (255, 255, 255)},
        {"bg": (180, 10, 10), "text": "Canon", "fg": (255, 255, 255)},
        {"bg": (0, 70, 30), "text": "Carlsberg", "fg": (255, 255, 255)},
        {"bg": (160, 20, 20), "text": "Fly Emirates", "fg": (255, 240, 200)}
    ]

    panel_w = width // len(panels)
    for i, p in enumerate(panels):
        x0 = i * panel_w
        x1 = x0 + panel_w
        # Beveled border
        draw.rectangle([x0, 0, x1, height], fill=p['bg'])
        draw.line([x0, 0, x1, 0], fill=(220, 220, 230), width=2)
        draw.line([x0, height-1, x1, height-1], fill=(40, 40, 45), width=2)
        draw.line([x1-1, 0, x1-1, height], fill=(60, 60, 70), width=2)

        # High-contrast center text simulation
        # Draw clean stylized banner text
        text = p['text']
        # Text shadow
        draw.text((x0 + 32, 48), text, fill=(10, 10, 15, 180))
        draw.text((x0 + 30, 46), text, fill=p['fg'])

        # Bottom LED tick bar
        for dot_x in range(x0 + 10, x1 - 10, 8):
            draw.point((dot_x, 115), fill=(240, 240, 255, 200))

    return img

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dirs = [
        os.path.join(root, "PES 6 game files", "remaster_pack", "pitch"),
        os.path.join(root, "test_game", "remaster_pack", "pitch")
    ]

    adboard = generate_hd_adboard(1024, 128)
    for d in target_dirs:
        os.makedirs(d, exist_ok=True)
        out_p = os.path.join(d, "adboard_default.png")
        adboard.save(out_p, "PNG", optimize=True)
        print(f"Saved {out_p} ({os.path.getsize(out_p)} bytes)")

if __name__ == '__main__':
    main()
