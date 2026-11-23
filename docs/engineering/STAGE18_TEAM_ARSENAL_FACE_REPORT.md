# Stage 18 — Arsenal FC Character Restoration Report
**PES 6 Legacy Remaster: High-Definition Player Faces & Likeness Proof**
*Club: Arsenal FC (The Gunners) | Season: 2005–2006 (UEFA Champions League Finalists)*

---

## 1. Roster Overview & Production Summary

Arsenal's 2005–2006 squad represents the peak of Arsène Wenger's high-tempo European campaign. Five marquee players have been fully restored from authentic 2006 reference photography:

| Player | Position | PES 6 IDs (Club / Nat / ML) | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Verification Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Thierry Henry** | CF | `195 / 1148 / 5012` | `Henry.png` (2.69 MB) | `Henry_hair.png` (705 KB) | Buzz fade (Alpha 128) | `VERIFIED` |
| **Cesc Fàbregas** | CM | `197 / 487 / 5014` | `Fabregas.png` (2.69 MB) | `Fabregas_hair.png` (705 KB) | Shaggy mullet flow (Alpha 255) | `VERIFIED` |
| **Gilberto Silva** | DMF | `199 / 1022 / 5016` | `Gilberto.png` (2.65 MB) | `Gilberto_hair.png` (705 KB) | Close buzz cut (Alpha 200) | `VERIFIED` |
| **Fredrik Ljungberg** | RWM | `201 / 1533 / 5018` | `Ljungberg.png` (2.67 MB) | `Ljungberg_hair.png` (705 KB) | Shaved buzz (Alpha 128) | `VERIFIED` |
| **Robin van Persie** | CF | `205 / 2628 / 5022` | `VanPersie.png` (2.66 MB) | `VanPersie_hair.png` (705 KB) | Textured crop (Alpha 255) | `VERIFIED` |

---

## 2. Anatomical & Visual Profiles

### 2.1 Thierry Henry
* **Reference:** `Thierry_Henry_Charlton.jpg`
* **Likeness:** French-Caribbean caramel espresso tone (`RGB 126, 80, 54`), almond-shaped dark brown eyes with corneal specular catchlights, manicured pencil moustache, and chin goatee.
* **3D Proof:** Rendered in Blender MCP ([stage18_henry_restored_front.png](file:///c:/Users/dream/Downloads/PES06Remastered-main/scratch/stage18_henry_restored_front.png)).

### 2.2 Cesc Fàbregas
* **Reference:** `Cesc_Fabregas.jpg`
* **Likeness:** Youthful Spanish complexion (`RGB 205, 158, 138`), sharp nasal ridge, dark espresso eyes (`RGB 52, 38, 28`), clean-shaven aesthetic, and characteristic 2006 shaggy brunette flow.

### 2.3 Gilberto Silva
* **Reference:** `Gilberto_Silva.png`, `Gilberto_Silva_NorthLondon.jpg`
* **Likeness:** Brazilian Afro-Latino bronze tone (`RGB 120, 78, 52`), wide jawline, prominent brow ridge, subtle chin stubble, and razor-cropped fade.

### 2.4 Fredrik Ljungberg
* **Reference:** `Fredrik_Ljungberg_060602.jpg`
* **Likeness:** Scandinavian fair tone (`RGB 215, 172, 150`), piercing blue-gray eyes (`RGB 75, 110, 130`), chiseled square jaw, and trademark trimmed stubble.

### 2.5 Robin van Persie
* **Reference:** `Robin_van_Persie_cropped.jpg`
* **Likeness:** Dutch athletic tone (`RGB 210, 165, 142`), straight nasal profile, dark brown intense gaze (`RGB 48, 36, 26`), clean-shaven, and textured forward crop.

---

## 3. Engine Deployment & Verification

All five players are deployed in `PES 6 game files/GDB/faces/Arsenal/` and `hair/Arsenal/`, mirrored to `test_game/`, and mapped across all three game modes in `map.txt`.
