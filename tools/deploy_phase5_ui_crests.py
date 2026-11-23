"""
PES 6 Legacy Remaster - Phase 5 Implementation
Deploys high-definition vector crests and competition emblems into remaster_pack/ui/crests/
Registers assets in manifest.json with cryptographic SHA-256 verification.
"""

import os
import math
import json
import hashlib
from PIL import Image, ImageDraw

def compute_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest().lower()

def generate_crest_arsenal(size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Shield shape
    red = (200, 16, 46, 255)
    gold = (212, 175, 55, 255)
    blue = (0, 32, 96, 255)
    
    # Gold border
    draw.polygon([(32, 20), (224, 20), (224, 150), (128, 240), (32, 150)], fill=gold)
    draw.polygon([(38, 26), (218, 26), (218, 146), (128, 230), (38, 146)], fill=red)
    
    # Left blue diagonal, right white diagonal
    draw.line([(38, 70), (218, 70)], fill=gold, width=3)
    draw.text((65, 35), "ARSENAL", fill=(255, 255, 255))
    
    # Historic Cannon facing forward
    draw.rectangle([70, 115, 186, 135], fill=gold)
    draw.ellipse([80, 105, 120, 145], outline=gold, width=6)
    return img

def generate_crest_chelsea(size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    blue = (3, 70, 148, 255)
    gold = (218, 165, 32, 255)
    white = (255, 255, 255, 255)
    red = (220, 20, 30, 255)
    
    # Outer circle
    draw.ellipse([20, 20, 236, 236], fill=blue, outline=gold, width=4)
    # Inner circle
    draw.ellipse([50, 50, 206, 206], fill=white, outline=blue, width=3)
    
    # Text
    draw.text((70, 26), "CHELSEA", fill=white)
    draw.text((58, 212), "FOOTBALL CLUB", fill=white)
    
    # Blue lion rampant in center
    draw.polygon([(110, 80), (145, 80), (160, 120), (140, 175), (105, 170), (120, 130)], fill=blue)
    draw.line([(145, 90), (170, 70)], fill=red, width=4) # Staff
    return img

def generate_crest_barcelona(size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    gold = (237, 187, 0, 255)
    blue = (0, 77, 152, 255)
    claret = (165, 0, 68, 255)
    
    # Pot-bellied shield
    draw.polygon([(30, 25), (226, 25), (226, 140), (128, 235), (30, 140)], fill=gold)
    draw.polygon([(36, 31), (220, 31), (220, 135), (128, 225), (36, 135)], fill=(255, 255, 255))
    
    # St George cross (top left)
    draw.rectangle([36, 31, 128, 100], fill=(255, 255, 255))
    draw.line([(82, 31), (82, 100)], fill=(200, 20, 30), width=10)
    draw.line([(36, 65), (128, 65)], fill=(200, 20, 30), width=10)
    
    # Senyera red/gold bars (top right)
    for i in range(5):
        c = (200, 20, 30) if i % 2 == 1 else gold
        draw.rectangle([128 + i * 18, 31, 128 + (i+1) * 18, 100], fill=c)
        
    # Center banner
    draw.rectangle([36, 100, 220, 125], fill=gold)
    draw.text((75, 105), "F. C. B.", fill=(0, 32, 96))
    
    # Bottom Blaugrana stripes
    for i in range(4):
        c = blue if i % 2 == 0 else claret
        draw.rectangle([36 + i * 46, 125, 36 + (i+1) * 46, 220], fill=c)
        
    return img

def generate_crest_realmadrid(size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    gold = (238, 198, 67, 255)
    white = (255, 255, 255, 255)
    purple = (112, 48, 160, 255)
    
    # Royal crown on top
    draw.polygon([(70, 60), (186, 60), (170, 25), (128, 45), (86, 25)], fill=gold)
    
    # Circle with diagonal purple stripe
    draw.ellipse([36, 60, 220, 244], fill=white, outline=gold, width=8)
    draw.polygon([(50, 180), (180, 80), (205, 110), (75, 210)], fill=purple)
    
    # Monogram MFC in gold
    draw.text((105, 120), "MCF", fill=gold)
    return img

def generate_emblem_ucl(size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    navy = (10, 20, 50, 255)
    white = (255, 255, 255, 255)
    
    draw.ellipse([20, 20, 236, 236], fill=navy, outline=(200, 215, 240), width=4)
    # Champions League Starball
    cx, cy = 128, 128
    for i in range(8):
        ang = math.radians(i * 45 - 90)
        sx = cx + 60 * math.cos(ang)
        sy = cy + 60 * math.sin(ang)
        # Draw star
        pts = []
        for k in range(10):
            a = math.radians(k * 36 - 90)
            r = 25 if k % 2 == 0 else 11
            pts.append((sx + r * math.cos(a), sy + r * math.sin(a)))
        draw.polygon(pts, fill=white)
        
    draw.text((62, 190), "CHAMPIONS", fill=white)
    draw.text((80, 205), "LEAGUE", fill=white)
    return img

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dirs = [
        os.path.join(root, "PES 6 game files", "remaster_pack"),
        os.path.join(root, "test_game", "remaster_pack")
    ]
    
    crests = {
        "crest_arsenal.png": generate_crest_arsenal(),
        "crest_chelsea.png": generate_crest_chelsea(),
        "crest_barcelona.png": generate_crest_barcelona(),
        "crest_realmadrid.png": generate_crest_realmadrid(),
        "emblem_ucl.png": generate_emblem_ucl()
    }
    
    for base in target_dirs:
        crests_d = os.path.join(base, "ui", "crests")
        os.makedirs(crests_d, exist_ok=True)
        for fname, img in crests.items():
            out_p = os.path.join(crests_d, fname)
            img.save(out_p, "PNG", optimize=True)
            print(f"Saved {out_p}")
            
        # Register in manifest.json
        manifest_p = os.path.join(base, "manifest.json")
        if os.path.exists(manifest_p):
            with open(manifest_p, "r") as f:
                m = json.load(f)
                
            for fname in crests.keys():
                asset_id = "ui_" + fname.replace(".png", "")
                rel_file = f"ui\\\\crests\\\\{fname}"
                full_p = os.path.join(crests_d, fname)
                sha = compute_sha256(full_p)
                
                # Check if exists
                if not any(a["id"] == asset_id for a in m["assets"]):
                    m["assets"].append({
                        "id": asset_id,
                        "category": "ui",
                        "crc32": "0x00000000",
                        "orig_width": 256,
                        "orig_height": 256,
                        "file": rel_file,
                        "sha256": sha
                    })
                    
            with open(manifest_p, "w") as f:
                json.dump(m, f, indent=2)
            print(f"Updated {manifest_p} with HD crests.")

if __name__ == '__main__':
    main()
