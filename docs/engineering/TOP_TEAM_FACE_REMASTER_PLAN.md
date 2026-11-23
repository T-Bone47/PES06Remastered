# Top Team Player Face Remaster Plan
**PES 6 Legacy Remaster: Production Roadmap for Top 2006 Clubs**
*Engine Target: Pro Evolution Soccer 6 | Face Format: 1024x2048 PNG (1:2) | Hair Format: 1024x512 PNG (2:1)*

---

## 1. Scope & Objective

This document establishes the official production schedule, asset hierarchy, and quality tiers for remastering player faces across the top four 2006 European powerhouses:
1. **Arsenal FC** (UEFA Champions League Finalists 2006)
2. **FC Barcelona** (UEFA Champions League Winners 2006, La Liga Champions)
3. **Chelsea FC** (Premier League Champions 2005-06)
4. **Real Madrid CF** (The Galácticos Era)

Every player face is reconstructed using **authentic 2005–2007 licensed reference photographs** mapped to native PES 6 66,688-byte mesh envelopes, preserving 100% of original animations and bone rigs.

---

## 2. Priority Team Rosters & Asset Specifications

### 2.1 Team 1: Arsenal FC (North London)

| Player | Position | Club ID | Nat ID | ML ID | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Reference Source File |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Thierry Henry** | CF | 195 | 1148 | 5012 | `Henry.png` | `Henry_hair.png` | Short buzz fade (Alpha 128) | `Thierry_Henry_Charlton.jpg` |
| **Cesc Fàbregas** | CM | 197 | 487 | 5014 | `Fabregas.png` | `Fabregas_hair.png` | Shaggy mullet flow (Alpha 255) | `Cesc_Fabregas.jpg` |
| **Gilberto Silva** | DMF | 199 | 1022 | 5016 | `Gilberto.png` | `Gilberto_hair.png` | Close buzz cut (Alpha 200) | `Gilberto_Silva.png` |
| **Fredrik Ljungberg** | RWM | 201 | 1533 | 5018 | `Ljungberg.png` | `Ljungberg_hair.png` | Shaved buzz / red streak (Alpha 128) | `Fredrik_Ljungberg_060602.jpg` |
| **Robin van Persie** | CF | 205 | 2628 | 5022 | `VanPersie.png` | `VanPersie_hair.png` | Textured crop (Alpha 255) | `Robin_van_Persie_cropped.jpg` |

### 2.2 Team 2: FC Barcelona (Catalonia)

| Player | Position | Club ID | Nat ID | ML ID | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Reference Source File |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Ronaldinho** | AMF/LWM | 133 | 2227 | 3140 | `Ronaldinho.png` | `Ronaldinho_hair.png` | Dreadlocks + Nike Headband (Alpha 255) | `Ronaldinho061115.jpg` |
| **Samuel Eto'o** | CF | 135 | 854 | 3142 | `Etoo.png` | `Etoo_hair.png` | Razor fade (Alpha 128) | `Samuel-Etoo-7-5-2006.jpg` |
| **Deco** | CMF | 138 | 685 | 3145 | `Deco.png` | `Deco_hair.png` | Wavy Portuguese flow (Alpha 255) | `Deco_21dec2006.jpg` |
| **Lionel Messi** | RWM | 141 | 1720 | 3148 | `Messi.png` | `Messi_hair.png` | Youthful 2006 flow (Alpha 255) | `Lionel_Messi_31mar2007.jpg` |

### 2.3 Team 3: Chelsea FC (West London)

| Player | Position | Club ID | Nat ID | ML ID | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Reference Source File |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Didier Drogba** | CF | 230 | 784 | 5047 | `Drogba.png` | `Drogba_hair.png` | Slicked ponytail (Alpha 255) | `Didier_Drogba_complains.jpg` |
| **Frank Lampard** | CMF | 234 | 1461 | 5051 | `Lampard.png` | `Lampard_hair.png` | Classic English crop (Alpha 255) | `Frank_Lampard_cropped.jpg` |
| **John Terry** | CB | 236 | 2515 | 5053 | `Terry.png` | `Terry_hair.png` | Short back and sides (Alpha 255) | `John_Terry.jpg` |

### 2.4 Team 4: Real Madrid CF (Madrid)

| Player | Position | Club ID | Nat ID | ML ID | Face Texture (1024x2048) | Hair Texture (1024x512) | Hair Style & Alpha | Reference Source File |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Zinedine Zidane** | AMF | 301 | 2790 | 3105 | `Zidane.png` | `Zidane_hair.png` | Iconic shaved crown (Alpha 64) | `Zinedine_zidane_wcf_2006.jpg` |
| **Ronaldo Nazário** | CF | 101 | 2228 | 3108 | `Ronaldo.png` | `Ronaldo_hair.png` | Shaved buzz (Alpha 96) | `Ronaldo_Real_Madrid.jpg` |
| **David Beckham** | RMF | 104 | 220 | 3111 | `Beckham.png` | `Beckham_hair.png` | Golden blonde highlights (Alpha 255) | `Beckham_warmingup.jpg` |
| **Roberto Carlos** | LB | 106 | 2202 | 3113 | `RobertoCarlos.png` | `RobertoCarlos_hair.png` | Shaved bald (Alpha 64) | `Roberto_Carlos.JPG` |

---

## 3. Production Quality Criteria

1. **Photographic Likeness Standard:** No generic synthetic faces, no anime/illustration artifacts, no circular synthetic pupils.
2. **Anatomical Accuracy:** Proper eye separation ($X=0.36, 0.64$), nasal bridge alignment ($X=0.50, Y=0.54$), and oral cavity positioning ($Y=0.66$).
3. **Isotropic Mapping:** Strict 1:2 face aspect ratio ($1024 \times 2048$) preventing any vertical stretching or squashing.
4. **Faceserver Integration:** Automatic detection via companion `.png` files mapped in `GDB/faces/map.txt` and `GDB/hair/map.txt`.
