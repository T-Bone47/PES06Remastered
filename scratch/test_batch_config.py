"""
PES 6 Legacy Remaster - Stage 18 Reference-Guided Character Restoration Tool
Batch Face & Hair Texture Generator for Top 2006 Teams

Uses genuine 2006 player reference photographs to reconstruct 1024x2048 (1:2 aspect ratio)
face textures and 1024x512 (2:1 aspect ratio) hair textures for marquee players across:
  - Arsenal (Thierry Henry, Cesc Fàbregas, Gilberto Silva, Fredrik Ljungberg, Robin van Persie)
  - Barcelona (Ronaldinho, Samuel Eto'o, Deco, Lionel Messi)
  - Chelsea (Didier Drogba, Frank Lampard, John Terry)
  - Real Madrid (Zinedine Zidane, Ronaldo Nazário R9, David Beckham, Roberto Carlos)
"""

import os
import sys
import math
import struct
import zlib
import hashlib
import numpy as np
from PIL import Image, ImageFilter, ImageEnhance

REF_DIR = r"C:\Users\dream\Downloads\PES6_2006_Face_Reference_Flat_Fixed\downloaded_references"
GAME_GDB = r"PES 6 game files\GDB"
TEST_GDB = r"test_game\GDB"

PLAYERS = [
    # ARSENAL
    {
        "team": "Arsenal", "id": "thierry_henry", "display_name": "Thierry Henry",
        "club_id": 195, "national_id": 1148, "ml_id": 5012,
        "face_bin": "Henry.bin", "hair_bin": "Henry_hair.bin",
        "ref": os.path.join(REF_DIR, "Arsenal", "thierry_henry", "Thierry_Henry_Charlton.jpg"),
        "skin_base": (126, 80, 54), "skin_undertone": (145, 92, 64),
        "eye_color": (46, 30, 20), "hair_color": (28, 24, 22),
        "hair_alpha": 128, "hair_style": "buzz_fade",
        "beard_type": "goatee_pencil", "features": "high forehead, chiseled jaw, refined arched brow"
    },
    {
        "team": "Arsenal", "id": "cesc_fabregas", "display_name": "Cesc Fàbregas",
        "club_id": 197, "national_id": 487, "ml_id": 5014,
        "face_bin": "Fabregas.bin", "hair_bin": "Fabregas_hair.bin",
        "ref": os.path.join(REF_DIR, "Arsenal", "cesc_fabregas", "Cesc_Fabregas.jpg"),
        "skin_base": (205, 158, 138), "skin_undertone": (218, 168, 146),
        "eye_color": (52, 38, 28), "hair_color": (32, 26, 22),
        "hair_alpha": 255, "hair_style": "medium_shaggy_mullet",
        "beard_type": "clean_shaven", "features": "youthful Spanish features, sharp nose, dark expressive eyes"
    },
    {
        "team": "Arsenal", "id": "gilberto_silva", "display_name": "Gilberto Silva",
        "club_id": 199, "national_id": 1022, "ml_id": 5016,
        "face_bin": "Gilberto.bin", "hair_bin": "Gilberto_hair.bin",
        "ref": os.path.join(REF_DIR, "Arsenal", "gilberto_silva", "Gilberto_Silva.png"),
        "skin_base": (120, 78, 52), "skin_undertone": (138, 88, 60),
        "eye_color": (42, 28, 18), "hair_color": (24, 20, 18),
        "hair_alpha": 200, "hair_style": "close_buzz",
        "beard_type": "subtle_stubble", "features": "calm composure, broad cheekbones, prominent brow"
    },
    {
        "team": "Arsenal", "id": "fredrik_ljungberg", "display_name": "Fredrik Ljungberg",
        "club_id": 201, "national_id": 1533, "ml_id": 5018,
        "face_bin": "Ljungberg.bin", "hair_bin": "Ljungberg_hair.bin",
        "ref": os.path.join(REF_DIR, "Arsenal", "fredrik_ljungberg", "Fredrik_Ljungberg_060602.jpg"),
        "skin_base": (215, 172, 150), "skin_undertone": (225, 180, 158),
        "eye_color": (75, 110, 130), "hair_color": (40, 32, 28),
        "hair_alpha": 128, "hair_style": "shaved_buzz",
        "beard_type": "designer_stubble", "features": "Scandinavian bone structure, piercing blue-gray eyes, chiseled chin"
    },
    {
        "team": "Arsenal", "id": "robin_van_persie", "display_name": "Robin van Persie",
        "club_id": 205, "national_id": 2628, "ml_id": 5022,
        "face_bin": "VanPersie.bin", "hair_bin": "VanPersie_hair.bin",
        "ref": os.path.join(REF_DIR, "Arsenal", "robin_van_persie", "Robin_van_Persie_cropped.jpg"),
        "skin_base": (210, 165, 142), "skin_undertone": (222, 174, 150),
        "eye_color": (48, 36, 26), "hair_color": (34, 28, 24),
        "hair_alpha": 255, "hair_style": "textured_short_crop",
        "beard_type": "clean_shaven", "features": "angular Dutch jawline, intense concentration, straight nasal bridge"
    },

    # BARCELONA
    {
        "team": "Barcelona", "id": "ronaldinho", "display_name": "Ronaldinho",
        "club_id": 133, "national_id": 2227, "ml_id": 3140,
        "face_bin": "Ronaldinho.bin", "hair_bin": "Ronaldinho_hair.bin",
        "ref": os.path.join(REF_DIR, "Barcelona", "ronaldinho", "Ronaldinho061115.jpg"),
        "skin_base": (148, 102, 72), "skin_undertone": (165, 115, 82),
        "eye_color": (52, 36, 24), "hair_color": (22, 20, 22),
        "hair_alpha": 255, "hair_style": "dreadlocks_headband",
        "beard_type": "chin_patch_stubble", "features": "iconic open smile with upper teeth, expressive brows, lateral laugh lines"
    },
    {
        "team": "Barcelona", "id": "samuel_etoo", "display_name": "Samuel Eto'o",
        "club_id": 135, "national_id": 854, "ml_id": 3142,
        "face_bin": "Etoo.bin", "hair_bin": "Etoo_hair.bin",
        "ref": os.path.join(REF_DIR, "Barcelona", "samuel_etoo", "Samuel-Etoo-7-5-2006.jpg"),
        "skin_base": (102, 62, 42), "skin_undertone": (118, 72, 48),
        "eye_color": (40, 26, 18), "hair_color": (22, 18, 18),
        "hair_alpha": 128, "hair_style": "short_fade",
        "beard_type": "trimmed_chin_stubble", "features": "powerful African athletic jaw, wide smile line, chiseled cheekbones"
    },
    {
        "team": "Barcelona", "id": "deco", "display_name": "Deco",
        "club_id": 138, "national_id": 685, "ml_id": 3145,
        "face_bin": "Deco.bin", "hair_bin": "Deco_hair.bin",
        "ref": os.path.join(REF_DIR, "Barcelona", "deco", "Deco_21dec2006.jpg"),
        "skin_base": (185, 138, 115), "skin_undertone": (198, 148, 122),
        "eye_color": (48, 34, 24), "hair_color": (26, 22, 20),
        "hair_alpha": 255, "hair_style": "medium_wavy_flow",
        "beard_type": "five_o_clock_shadow", "features": "Luso-Brazilian olive complexion, deep eye sockets, strategic playmaker intensity"
    },
    {
        "team": "Barcelona", "id": "lionel_messi", "display_name": "Lionel Messi",
        "club_id": 141, "national_id": 1720, "ml_id": 3148,
        "face_bin": "Messi.bin", "hair_bin": "Messi_hair.bin",
        "ref": os.path.join(REF_DIR, "Barcelona", "lionel_messi", "Lionel_Messi_31mar2007.jpg"),
        "skin_base": (212, 170, 148), "skin_undertone": (224, 180, 156),
        "eye_color": (54, 38, 28), "hair_color": (42, 32, 26),
        "hair_alpha": 255, "hair_style": "classic_messi_flow_2006",
        "beard_type": "clean_shaven", "features": "youthful Rosario features, long brunette flowing hair, determined focus"
    },

    # CHELSEA
    {
        "team": "Chelsea", "id": "didier_drogba", "display_name": "Didier Drogba",
        "club_id": 230, "national_id": 784, "ml_id": 5047,
        "face_bin": "Drogba.bin", "hair_bin": "Drogba_hair.bin",
        "ref": os.path.join(REF_DIR, "Chelsea", "didier_drogba", "Didier_Drogba_complains.jpg"),
        "skin_base": (92, 56, 38), "skin_undertone": (110, 68, 46),
        "eye_color": (38, 24, 16), "hair_color": (20, 18, 18),
        "hair_alpha": 255, "hair_style": "slicked_back_ponytail",
        "beard_type": "chin_strap_goatee", "features": "towering mandibular jaw, warrior intensity, prominent brow ridge"
    },
    {
        "team": "Chelsea", "id": "frank_lampard", "display_name": "Frank Lampard",
        "club_id": 234, "national_id": 1461, "ml_id": 5051,
        "face_bin": "Lampard.bin", "hair_bin": "Lampard_hair.bin",
        "ref": os.path.join(REF_DIR, "Chelsea", "frank_lampard", "Frank_Lampard_cropped.jpg"),
        "skin_base": (208, 158, 136), "skin_undertone": (222, 170, 148),
        "eye_color": (65, 85, 105), "hair_color": (48, 38, 30),
        "hair_alpha": 255, "hair_style": "classic_english_crop",
        "beard_type": "clean_shaven", "features": "determined English midfielder countenance, square jaw, intense blue-gray gaze"
    },
    {
        "team": "Chelsea", "id": "john_terry", "display_name": "John Terry",
        "club_id": 236, "national_id": 2515, "ml_id": 5053,
        "face_bin": "Terry.bin", "hair_bin": "Terry_hair.bin",
        "ref": os.path.join(REF_DIR, "Chelsea", "john_terry", "John_Terry.jpg"),
        "skin_base": (206, 155, 134), "skin_undertone": (218, 166, 144),
        "eye_color": (60, 92, 115), "hair_color": (52, 42, 34),
        "hair_alpha": 255, "hair_style": "short_back_and_sides",
        "beard_type": "clean_stubble", "features": "classic leader jawline, prominent supraorbital ridges, warrior captain spirit"
    },

    # REAL MADRID
    {
        "team": "Real_Madrid", "id": "zinedine_zidane", "display_name": "Zinedine Zidane",
        "club_id": 301, "national_id": 2790, "ml_id": 3105,
        "face_bin": "Zidane.bin", "hair_bin": "Zidane_hair.bin",
        "ref": os.path.join(REF_DIR, "Real_Madrid", "zinedine_zidane", "Zinedine_zidane_wcf_2006.jpg"),
        "skin_base": (196, 148, 124), "skin_undertone": (210, 160, 134),
        "eye_color": (68, 88, 72), "hair_color": (32, 28, 26),
        "hair_alpha": 64, "hair_style": "iconic_shaved_head",
        "beard_type": "dense_five_o_clock_shadow", "features": "masterful French-Kabyle bone structure, deep-set hazel-green eyes, stoic authority"
    },
    {
        "team": "Real_Madrid", "id": "ronaldo_nazario", "display_name": "Ronaldo Nazário",
        "club_id": 101, "national_id": 2228, "ml_id": 3108,
        "face_bin": "Ronaldo.bin", "hair_bin": "Ronaldo_hair.bin",
        "ref": os.path.join(REF_DIR, "Real_Madrid", "ronaldo_nazario", "Ronaldo_Real_Madrid.jpg"),
        "skin_base": (172, 126, 98), "skin_undertone": (188, 138, 108),
        "eye_color": (50, 36, 26), "hair_color": (28, 24, 22),
        "hair_alpha": 96, "hair_style": "shaved_buzz",
        "beard_type": "clean_shaven", "features": "legendary Brazilian smile, rounded facial contour, distinctive upper central incisors"
    },
    {
        "team": "Real_Madrid", "id": "david_beckham", "display_name": "David Beckham",
        "club_id": 104, "national_id": 220, "ml_id": 3111,
        "face_bin": "Beckham.bin", "hair_bin": "Beckham_hair.bin",
        "ref": os.path.join(REF_DIR, "Real_Madrid", "david_beckham", "Beckham_warmingup.jpg"),
        "skin_base": (212, 168, 144), "skin_undertone": (224, 178, 154),
        "eye_color": (70, 95, 115), "hair_color": (118, 92, 58),
        "hair_alpha": 255, "hair_style": "blonde_highlighted_crop_2006",
        "beard_type": "designer_goatee_stubble", "features": "chiseled English jaw, golden highlighted locks, signature concentration lines"
    },
    {
        "team": "Real_Madrid", "id": "roberto_carlos", "display_name": "Roberto Carlos",
        "club_id": 106, "national_id": 2202, "ml_id": 3113,
        "face_bin": "RobertoCarlos.bin", "hair_bin": "RobertoCarlos_hair.bin",
        "ref": os.path.join(REF_DIR, "Real_Madrid", "roberto_carlos", "Roberto_Carlos.JPG"),
        "skin_base": (160, 114, 86), "skin_undertone": (176, 126, 96),
        "eye_color": (46, 32, 22), "hair_color": (26, 22, 20),
        "hair_alpha": 64, "hair_style": "smooth_shaved_bald",
        "beard_type": "clean_shaven", "features": "powerful Brazilian muscular jaw, beaming smile, iconic shaved dome"
    }
]

print(f"Loaded configuration for {len(PLAYERS)} marquee players across 4 top teams.")
