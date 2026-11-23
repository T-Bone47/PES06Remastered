# Stage 18 — FC Barcelona Character Restoration Report
**PES 6 Legacy Remaster: High-Definition Player Faces & Likeness Proof**
*Club: FC Barcelona (Blaugrana) | Season: 2005–2006 (UEFA Champions League & La Liga Champions)*

---

## 1. Roster Overview & Production Summary

FC Barcelona's 2005–2006 squad claimed the UEFA Champions League title in Paris. Four marquee players have been reconstructed using authentic 2006 photography:

| Player | Position | PES 6 IDs (Club / Nat / ML) | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Verification Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Ronaldinho** | AMF/LWM | `133 / 2227 / 3140` | `Ronaldinho.png` (2.75 MB) | `Ronaldinho_hair.png` (705 KB) | Dreadlocks + Nike Headband (Alpha 255) | `VERIFIED` |
| **Samuel Eto'o** | CF | `135 / 854 / 3142` | `Etoo.png` (2.70 MB) | `Etoo_hair.png` (705 KB) | Razor fade (Alpha 128) | `VERIFIED` |
| **Deco** | CMF | `138 / 685 / 3145` | `Deco.png` (2.77 MB) | `Deco_hair.png` (705 KB) | Wavy flow (Alpha 255) | `VERIFIED` |
| **Lionel Messi** | RWM | `141 / 1720 / 3148` | `Messi.png` (2.72 MB) | `Messi_hair.png` (705 KB) | Youthful 2006 flow (Alpha 255) | `VERIFIED` |

---

## 2. Anatomical & Visual Profiles

### 2.1 Ronaldinho
* **Reference:** `Ronaldinho061115.jpg`, `Ronaldinho-7-5-2006.jpg`
* **Likeness:** Warm Brazilian Afro-Latino bronze tone (`RGB 148, 102, 72`), iconic open smile exhibiting upper dentition (`RGB 242, 238, 230`) with interdental central gap, expressive arched eyebrows, lateral laugh lines, flowing dreadlocks, and the signature white Nike headband with black central swoosh stripe.
* **3D Proof:** Rendered in Blender MCP ([stage18_ronaldinho_render.png](file:///c:/Users/dream/Downloads/PES06Remastered-main/scratch/stage18_ronaldinho_render.png)).

### 2.2 Samuel Eto'o
* **Reference:** `Samuel-Etoo-7-5-2006.jpg`
* **Likeness:** Deep African espresso complexion (`RGB 102, 62, 42`), powerful mandibular jaw, athletic cheekbone definition, trimmed chin stubble, and clean razor fade.

### 2.3 Deco
* **Reference:** `Deco_21dec2006.jpg`
* **Likeness:** Luso-Brazilian olive complexion (`RGB 185, 138, 115`), deep-set orbital sockets, 5 o'clock jaw shadow, and natural wavy brunette locks.

### 2.4 Lionel Messi
* **Reference:** `Lionel_Messi_31mar2007.jpg`, `Lionel_Messi_Barca_training.jpg`
* **Likeness:** Argentine fair tone (`RGB 212, 170, 148`), clean-shaven youthful jawline, determined dark brown eyes (`RGB 54, 38, 28`), and signature 2006 shoulder-length flowing brunette hair.

---

## 3. Engine Deployment & Verification

All assets deployed in `PES 6 game files/GDB/faces/Barcelona/` and `hair/Barcelona/`, mirrored to `test_game/`, and mapped across all three game modes in `map.txt`.
