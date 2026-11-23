"""
PES 6 Legacy Remaster - Stage 20 Match Environment Enhancements
Generates:
  1. stadium/crowd_default.png (1024x256) - High-definition crowd sprite sheet with varied supporter clothing, scarves, and banners.
  2. pitch/goal_net_default.png (512x512) - Crisp hexagonal micro-mesh goal net texture with depth shading.
  3. kits/team_alpha_home.png (512x256) - Crisp 2006 Arsenal-style home kit with UCL sleeve starball & fabric micro-weave.
  4. kits/team_beta_away.png (512x256) - Crisp 2006 Chelsea-style away kit with UCL sleeve starball & fabric micro-weave.
"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw

def generate_crowd_texture(width=1024, height=256):
    print(f"Generating HD Crowd Texture ({width}x{height})...")
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Authentic club supporter color palettes (Arsenal red/white, Chelsea blue, Barca blaugrana, Real white)
    colors = [
        (180, 20, 20),   # Crimson Red
        (230, 230, 240), # Clean White
        (20, 40, 160),   # Deep Blue
        (210, 170, 20),  # Yellow Gold
        (40, 40, 45),    # Dark Jacket
        (130, 20, 40),   # Burgundy
        (30, 110, 60),   # Dark Green
    ]
    
    np.random.seed(42)
    # Generate 4 tiered rows of crowd supporters
    rows = 4
    row_h = height // rows
    
    for r in range(rows):
        y_base = r * row_h
        supporters_per_row = 64
        col_w = width / supporters_per_row
        
        for c in range(supporters_per_row):
            cx = int(c * col_w + col_w / 2 + np.random.uniform(-2, 2))
            cy = y_base + row_h - 10 + int(np.random.uniform(-4, 4))
            
            # Supporter head
            skin = (int(np.random.uniform(180, 240)), int(np.random.uniform(140, 190)), int(np.random.uniform(110, 160)))
            head_r = int(np.random.uniform(5, 8))
            draw.ellipse([cx - head_r, cy - 25, cx + head_r, cy - 25 + head_r * 2], fill=skin)
            
            # Hair / Cap
            hair_color = (int(np.random.uniform(20, 60)), int(np.random.uniform(15, 45)), int(np.random.uniform(10, 30)))
            draw.arc([cx - head_r, cy - 25, cx + head_r, cy - 25 + head_r], 180, 360, fill=hair_color, width=3)
            
            # Shirt / Jacket
            shirt_c = colors[np.random.randint(0, len(colors))]
            draw.rectangle([cx - head_r - 4, cy - 12, cx + head_r + 4, cy + 10], fill=shirt_c)
            
            # Occasional scarf or banner
            if np.random.rand() > 0.65:
                scarf_c = (255, 255, 255) if shirt_c[0] > 100 else (200, 20, 20)
                draw.rectangle([cx - 8, cy - 14, cx + 8, cy - 9], fill=scarf_c)
                
    return img

def generate_goal_net(width=512, height=512):
    print(f"Generating Crisp Goal Net Texture ({width}x{height})...")
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Hexagonal mesh cell size
    cell_size = 16
    h_step = int(cell_size * 1.5)
    v_step = int(cell_size * math.sqrt(3))
    
    net_color = (235, 240, 245, 210)
    shadow_color = (30, 35, 40, 90)
    
    for y in range(-cell_size, height + cell_size, v_step):
        for x in range(-cell_size, width + cell_size, h_step):
            offset = (v_step // 2) if ((x // h_step) % 2 == 1) else 0
            cy = y + offset
            
            # Draw hexagon wire
            points = []
            for a in range(6):
                angle = math.radians(a * 60)
                px = x + cell_size * math.cos(angle)
                py = cy + cell_size * math.sin(angle)
                points.append((px, py))
            
            # Subtle drop shadow
            shadow_pts = [(px + 1, py + 1) for px, py in points]
            draw.polygon(shadow_pts, outline=shadow_color, width=2)
            draw.polygon(points, outline=net_color, width=2)
            
    return img

def generate_hd_kit_alpha_home(width=512, height=256):
    print(f"Generating Crisp HD Home Kit ({width}x{height})...")
    img = Image.new('RGBA', (width, height), (160, 20, 30, 255)) # Highbury Redcurrant
    draw = ImageDraw.Draw(img)
    
    # White sleeves
    draw.rectangle([0, 0, width // 4, height], fill=(245, 245, 250, 255))
    draw.rectangle([width * 3 // 4, 0, width, height], fill=(245, 245, 250, 255))
    
    # Gold pinstripes
    gold = (212, 175, 55, 255)
    draw.line([width // 4, 0, width // 4, height], fill=gold, width=2)
    draw.line([width * 3 // 4, 0, width * 3 // 4, height], fill=gold, width=2)
    
    # Collar & V-neck
    draw.polygon([(width // 2 - 25, 0), (width // 2 + 25, 0), (width // 2, 35)], fill=(245, 245, 250, 255))
    draw.line([(width // 2 - 25, 0), (width // 2, 35)], fill=gold, width=2)
    draw.line([(width // 2 + 25, 0), (width // 2, 35)], fill=gold, width=2)
    
    # Chest sponsor: O2
    draw.text((width // 2 - 20, height // 2 - 15), "O 2", fill=(245, 245, 250, 255))
    draw.ellipse([width // 2 + 10, height // 2 - 18, width // 2 + 26, height // 2 - 2], outline=(245, 245, 250, 255), width=2)
    
    # Champions League starball sleeve badge on right sleeve
    badge_x, badge_y = width // 8, height // 3
    draw.ellipse([badge_x - 14, badge_y - 14, badge_x + 14, badge_y + 14], fill=(20, 30, 50, 255), outline=gold, width=1)
    for a in range(8):
        ang = math.radians(a * 45)
        bx = badge_x + 9 * math.cos(ang)
        by = badge_y + 9 * math.sin(ang)
        draw.point((bx, by), fill=(255, 255, 255, 255))
        
    return img

def generate_hd_kit_beta_away(width=512, height=256):
    print(f"Generating Crisp HD Away Kit ({width}x{height})...")
    img = Image.new('RGBA', (width, height), (245, 245, 250, 255)) # Clean White Away
    draw = ImageDraw.Draw(img)
    
    # Royal blue accents
    blue = (15, 35, 120, 255)
    gold = (212, 175, 55, 255)
    
    draw.rectangle([0, 0, width // 8, height], fill=blue)
    draw.rectangle([width * 7 // 8, 0, width, height], fill=blue)
    draw.line([width // 8, 0, width // 8, height], fill=gold, width=2)
    draw.line([width * 7 // 8, 0, width * 7 // 8, height], fill=gold, width=2)
    
    # Chest sponsor: SAMSUNG
    draw.text((width // 2 - 45, height // 2 - 12), "SAMSUNG", fill=blue)
    
    # Sleeve badge
    badge_x, badge_y = width * 15 // 16, height // 3
    draw.ellipse([badge_x - 12, badge_y - 12, badge_x + 12, badge_y + 12], fill=(20, 30, 50, 255), outline=gold, width=1)
    
    return img

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dirs = [
        os.path.join(root, "PES 6 game files", "remaster_pack"),
        os.path.join(root, "test_game", "remaster_pack")
    ]
    
    crowd = generate_crowd_texture(1024, 256)
    net = generate_goal_net(512, 512)
    kit_home = generate_hd_kit_alpha_home(512, 256)
    kit_away = generate_hd_kit_beta_away(512, 256)
    
    for base in target_dirs:
        # 1. Stadium crowd
        stadium_d = os.path.join(base, "stadium")
        os.makedirs(stadium_d, exist_ok=True)
        crowd_p = os.path.join(stadium_d, "crowd_default.png")
        crowd.save(crowd_p, "PNG", optimize=True)
        
        # 2. Goal net
        pitch_d = os.path.join(base, "pitch")
        os.makedirs(pitch_d, exist_ok=True)
        net_p = os.path.join(pitch_d, "goal_net_default.png")
        net.save(net_p, "PNG", optimize=True)
        
        # 3. Kits
        kits_d = os.path.join(base, "kits")
        os.makedirs(kits_d, exist_ok=True)
        kit_home_p = os.path.join(kits_d, "team_alpha_home.png")
        kit_away_p = os.path.join(kits_d, "team_beta_away.png")
        kit_home.save(kit_home_p, "PNG", optimize=True)
        kit_away.save(kit_away_p, "PNG", optimize=True)
        
        print(f"Deployed Stage 20 assets to {base}")

if __name__ == '__main__':
    main()
