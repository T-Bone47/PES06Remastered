import os
import struct
import zlib
import hashlib
import json

def make_png(w, h, color):
    # Minimal uncompressed PNG generator
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xffffffff)
    
    row = b"\x00" + bytes(color) * w
    raw = row * h
    compressed = zlib.compress(raw)
    
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)) # 8-bit RGBA
    png += chunk(b"IDAT", compressed)
    png += chunk(b"IEND", b"")
    return png

pack_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\test_game\remaster_pack"
for sub in ["kits", "faces", "pitch", "balls", "ui"]:
    os.makedirs(os.path.join(pack_dir, sub), exist_ok=True)

assets_def = [
    {"id": "kit_team_alpha_home", "category": "kit", "orig_width": 512, "orig_height": 256, "crc32": "0x1A2B3C4D", "file": "kits/team_alpha_home.png", "size": (1024, 512), "color": [255, 0, 0, 255]},
    {"id": "kit_team_beta_away", "category": "kit", "orig_width": 512, "orig_height": 256, "crc32": "0x2B3C4D5E", "file": "kits/team_beta_away.png", "size": (1024, 512), "color": [0, 0, 255, 255]},
    {"id": "face_player_001", "category": "face", "orig_width": 256, "orig_height": 256, "crc32": "0x3C4D5E6F", "file": "faces/player_001.png", "size": (512, 512), "color": [220, 180, 140, 255]},
    {"id": "pitch_stadium_default_turf", "category": "pitch", "orig_width": 1024, "orig_height": 512, "crc32": "0x5E6F7A8B", "file": "pitch/stadium_default_turf.png", "size": (2048, 1024), "color": [34, 139, 34, 255]},
    {"id": "ball_match_classic", "category": "ball", "orig_width": 256, "orig_height": 256, "crc32": "0x6F7A8B9C", "file": "balls/ball_match_classic.png", "size": (512, 512), "color": [240, 240, 240, 255]},
    {"id": "ui_radar_overlay", "category": "ui", "orig_width": 256, "orig_height": 128, "crc32": "0x7A8B9C0D", "file": "ui/radar_overlay.png", "size": (512, 256), "color": [50, 50, 50, 200]},
]

manifest_assets = []
for a in assets_def:
    filepath = os.path.join(pack_dir, a["file"].replace("/", os.sep))
    data = make_png(a["size"][0], a["size"][1], a["color"])
    with open(filepath, "wb") as f:
        f.write(data)
    sha = hashlib.sha256(data).hexdigest()
    manifest_assets.append({
        "id": a["id"],
        "category": a["category"],
        "crc32": a["crc32"],
        "orig_width": a["orig_width"],
        "orig_height": a["orig_height"],
        "file": a["file"].replace("/", "\\"),
        "sha256": sha
    })

manifest = {
    "name": "PES 6 Legacy Remaster Curated HD Asset Pack",
    "version": "1.0.0",
    "author": "PES 6 Remaster Engineering Team",
    "vram_budget_mb": 1500,
    "description": "Deterministic HD companion textures with SHA-256 integrity and VRAM budget enforcement.",
    "assets": manifest_assets
}

manifest_path = os.path.join(pack_dir, "manifest.json")
with open(manifest_path, "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2)

print(f"Created manifest with {len(manifest_assets)} assets at {manifest_path}")
for a in manifest_assets:
    print(f" - {a['id']} ({a['orig_width']}x{a['orig_height']}) -> {a['file']} [SHA: {a['sha256'][:16]}...]")
