"""
PES 6 Legacy Remaster - Phase 1 Implementation
Programs lodmixer.dat to lock LOD 1 (maximum polycount and 512x256 textures across all cameras)
and enables crowd unblocking across all cameras.
Scaffolds GDB directories for balls, boots, gloves, and stadiums.
"""

import os
import sys

def configure_lodmixer_dat(filepath):
    print(f"Configuring {filepath}...")
    # Exact 41-byte structure decoded from lodcfg.exe and lodmixer.dll:
    # 0x00..0x1F (32 bytes): internal aspect ratio / matrix buffer
    # 0x20      (1 byte) : aspect ratio mode (0x01 = widescreen)
    # 0x21..0x25 (5 bytes): LOD level selectors (0x00 = LOD 1 for all 5 camera ranges)
    # 0x26      (1 byte) : controller stance check (0x00)
    # 0x27      (1 byte) : crowd unblock (0x01 = show crowd on all cameras)
    # 0x28      (1 byte) : crowd stance (0x00)
    
    # Read existing 41 bytes if present, or create standard baseline
    if os.path.exists(filepath):
        with open(filepath, "rb") as f:
            data = bytearray(f.read())
    else:
        data = bytearray(41)
        
    if len(data) < 41:
        data.extend(b'\x00' * (41 - len(data)))
    elif len(data) > 41:
        data = data[:41]
        
    # Set default matrix values in first 32 bytes if all zeroes
    if data[0:4] == b'\x00\x00\x00\x00':
        data[0:32] = b'\xFF' * 32
        
    # Offset 0x20 (index 32): Aspect ratio correction enabled
    data[32] = 0x01
    
    # Offsets 0x21..0x25 (indices 33..37): LOD levels 1..5 set to 0x00 (LOD 1: Very high detail models, 512x256 textures)
    data[33] = 0x00
    data[34] = 0x00
    data[35] = 0x00
    data[36] = 0x00
    data[37] = 0x00
    
    # Offset 0x26 (index 38): 0x00
    data[38] = 0x00
    
    # Offset 0x27 (index 39): Crowd unblock switch = 0x01 (Show crowd on all cameras)
    data[39] = 0x01
    
    # Offset 0x28 (index 40): 0x00
    data[40] = 0x00
    
    with open(filepath, "wb") as f:
        f.write(data)
        
    print(f"Successfully programmed {filepath} (41 bytes, LOD 1 locked, Crowd unblocked).")

def scaffold_gdb(gdb_path):
    print(f"Scaffolding GDB at {gdb_path}...")
    dirs = [
        os.path.join(gdb_path, "balls"),
        os.path.join(gdb_path, "balls", "mdl"),
        os.path.join(gdb_path, "boots"),
        os.path.join(gdb_path, "gloves"),
        os.path.join(gdb_path, "stadiums"),
        os.path.join(gdb_path, "faces"),
        os.path.join(gdb_path, "hair"),
        os.path.join(gdb_path, "uni")
    ]
    for d in dirs:
        os.makedirs(d, exist_ok=True)
        
    # Create clean baseline map.txt files if not present
    maps = {
        os.path.join(gdb_path, "balls", "map.txt"): '# Ballserver Map\n# Format: "<Name in game>","<model.bin>","<texture.png>"\n',
        os.path.join(gdb_path, "boots", "map.txt"): '# Bootserver Map\n# Format: <PlayerID>,"<boot_texture.png>"\n',
        os.path.join(gdb_path, "gloves", "map.txt"): '# Gloveserver Map\n# Format: <PlayerID>,"<glove_texture.png>"\n',
        os.path.join(gdb_path, "stadiums", "map.txt"): '# Stadiumserver Map\n# Format: <TeamID>,"<Stadium_Folder>"\n'
    }
    for map_file, default_content in maps.items():
        if not os.path.exists(map_file):
            with open(map_file, "w") as f:
                f.write(default_content)
            print(f"Created map template: {map_file}")

def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    targets = [
        os.path.join(root, "PES 6 game files"),
        os.path.join(root, "test_game")
    ]
    
    for t in targets:
        lod_dat = os.path.join(t, "kitserver", "lodmixer.dat")
        configure_lodmixer_dat(lod_dat)
        
        gdb_dir = os.path.join(t, "GDB")
        scaffold_gdb(gdb_dir)
        
        # Also ensure kitserver\dat directories for afs2fs exist
        dat_dir = os.path.join(t, "kitserver", "dat")
        for afs in ["0_text.afs", "0_sound.afs", "e_text.afs", "e_sound.afs"]:
            os.makedirs(os.path.join(dat_dir, afs), exist_ok=True)
            
    print("\nPhase 1 LOD & GDB Scaffolding complete!")

if __name__ == '__main__':
    main()
