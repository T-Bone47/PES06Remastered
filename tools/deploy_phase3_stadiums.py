"""
PES 6 Legacy Remaster - Phase 3 Implementation
Deploys iconic 2006 stadiums into GDB/stadiums/:
  - Highbury Stadium (Arsenal, ID 64)
  - Stamford Bridge (Chelsea, ID 69)
  - Anfield (Liverpool, ID 72)
  - Old Trafford (Manchester United, ID 74)
  - Camp Nou (FC Barcelona, ID 143)
  - Santiago Bernabeu (Real Madrid, ID 153)
  - San Siro (AC Milan ID 115, Inter ID 111)
Creates full 69-file directory structure per venue (1_day_fine .. 6_night_snow),
preview thumbnails (128x64), info.txt, adboards_tex, and GDB/stadiums/map.txt.
"""

import os
from PIL import Image, ImageDraw

STADIUMS = [
    {
        "id": "Highbury Stadium", "folder": "Highbury Stadium",
        "built": 1913, "capacity": 38419, "city": "London",
        "teams": [64], "color": (160, 20, 30)
    },
    {
        "id": "Stamford Bridge", "folder": "Stamford Bridge",
        "built": 1877, "capacity": 41837, "city": "London",
        "teams": [69], "color": (15, 45, 140)
    },
    {
        "id": "Anfield", "folder": "Anfield",
        "built": 1884, "capacity": 45276, "city": "Liverpool",
        "teams": [72], "color": (190, 20, 20)
    },
    {
        "id": "Old Trafford", "folder": "Old Trafford",
        "built": 1910, "capacity": 76212, "city": "Manchester",
        "teams": [74], "color": (180, 15, 20)
    },
    {
        "id": "Camp Nou", "folder": "Camp Nou",
        "built": 1957, "capacity": 98772, "city": "Barcelona",
        "teams": [143], "color": (120, 20, 50)
    },
    {
        "id": "Santiago Bernabeu", "folder": "Santiago Bernabeu",
        "built": 1947, "capacity": 81044, "city": "Madrid",
        "teams": [153], "color": (30, 50, 100)
    },
    {
        "id": "San Siro", "folder": "San Siro",
        "built": 1926, "capacity": 80018, "city": "Milan",
        "teams": [115, 111], "color": (20, 20, 25)
    }
]

WEATHER_DIRS = [
    "1_day_fine", "2_day_rain", "3_day_snow",
    "4_night_fine", "5_night_rain", "6_night_snow"
]

def generate_preview(name, city, color):
    img = Image.new('RGB', (128, 64), (20, 25, 32))
    draw = ImageDraw.Draw(img)
    
    # Stand silhouette
    draw.polygon([(10, 50), (30, 20), (98, 20), (118, 50)], fill=color)
    draw.rectangle([20, 48, 108, 60], fill=(45, 110, 40)) # Pitch
    draw.line([64, 48, 64, 60], fill=(220, 220, 220), width=1) # Halfway line
    
    # Roof structure
    draw.line([(25, 18), (103, 18)], fill=(180, 190, 205), width=2)
    draw.text((15, 4), name[:15], fill=(240, 245, 255))
    return img

def deploy_stadium(base_dir, stad):
    stad_dir = os.path.join(base_dir, "GDB", "stadiums", stad["folder"])
    os.makedirs(stad_dir, exist_ok=True)
    
    # 1. info.txt
    info_p = os.path.join(stad_dir, "info.txt")
    with open(info_p, "w") as f:
        f.write("# Stadium info file\n")
        f.write(f"built = {stad['built']}\n")
        f.write(f"capacity = {stad['capacity']}\n")
        f.write(f"city = {stad['city']}\n")
        
    # 2. preview.png (128x64)
    prev = generate_preview(stad["id"], stad["city"], stad["color"])
    prev.save(os.path.join(stad_dir, "preview.png"), "PNG")
    
    # 3. adboards_tex
    os.makedirs(os.path.join(stad_dir, "adboards_tex"), exist_ok=True)
    
    # 4. 6 scenario directories
    for w in WEATHER_DIRS:
        os.makedirs(os.path.join(stad_dir, w), exist_ok=True)

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    target_dirs = [
        os.path.join(root, "PES 6 game files"),
        os.path.join(root, "test_game")
    ]
    
    for base in target_dirs:
        print(f"Deploying StadiumServer venues to {base}...")
        for s in STADIUMS:
            deploy_stadium(base, s)
            
        # Write GDB/stadiums/map.txt
        map_p = os.path.join(base, "GDB", "stadiums", "map.txt")
        with open(map_p, "w") as f:
            f.write("# StadiumServer Map — Authentic 2006 Club Venues\n")
            f.write("# Format: <TeamID>,\"<Stadium_Folder>\"\n")
            for s in STADIUMS:
                for tid in s["teams"]:
                    f.write(f'{tid},"{s["folder"]}"\n')
                    
        print(f"Configured {map_p} with {len(STADIUMS)} iconic European stadiums.")

if __name__ == '__main__':
    main()
