# Research A — name-authorable packs

**Date:** 2026-10-20
**Question:** how do we make packs addressable by name rather than by an
opaque CRC?

---

## 1. The community guide

### 1.1 Where it is

| Document | URL | Retrieved |
|---|---|---|
| PES 6 Text and Logo Mapping Guide | `https://www.scribd.com/document/619044499/0-Text-Pes-6-Afs-map` | ❌ client challenge — not retrievable |
| PES 6 E-Text and Logo Map Guide | `https://www.scribd.com/document/484246003/Maps-e-text` | ❌ same |
| PES 6 0_Text AFS Map V2 Overview | (linked from the above) | ❌ same |
| **kitserver `src/misc/afsmap.txt`** | `references/kitserver6/src/misc/afsmap.txt` | ✅ **read in full, 360 lines** |

Scribd's documents are behind a bot challenge and could not be fetched
directly. **The same content exists in machine-readable form** in the
kitserver reference we already have, and that is what this document is based
on. It is the AFS map for `0_TEXT`; there is no equivalent for `e_text`.

### 1.2 The schema

`afsmap.txt` is a flat list of **AFS entry index ranges** and **individual
entry names**. Two line forms:

```
NNNN A NNNN <CATEGORY>      a range of entries that share a category
- NNNN <NAME>               one entry inside the current range
```

Real excerpt:

```
0000 A 0048 BALONES
- 0535 ESCUDOS 32X32
- 0536 ESCUDOS 64X64
1891 A 1916 CARAS ESPECIALES PIEL 4
- 1891 ...
2402 A 2763 CARAS PIEL 1
- 2402 CLAUDIO LOPEZ
- 2403 KILY GONZALEZ
- 2505 OWEN
- 2506 BECKHAM
4448 A 5315 PEINADOS
5322 A 5443 BOTAS
5456 A 5472 TIPOS DE DORSALES Y LETRAS
```

Parsed: **289 named entries/ranges.**

### 1.3 The numbers are AFS entry indices — CONFIRMED against our binary

FACT (cross-check performed this session, `tools/` script over
`0_text_index.tsv`):

| Map line | Category | Our measured inflated size at the range boundaries |
|---|---|---|
| `0000 A 0048 BALONES` | balls | 6,756 .. 66,688 (varying) |
| `1891 A 1916 CARAS ESPECIALES PIEL 4` | faces, skin 4 | **54,176 .. 54,176** (uniform) |
| `1957 A 2157 CARAS ESPECIALES PIEL 1` | faces, skin 1 | 54,160 .. 54,176 |
| `2284 A 2330 CARAS PIEL 4` | faces | **58,048 .. 58,048** (uniform) |
| `2402 A 2763 CARAS PIEL 1` | faces | 58,048 .. 58,048 (uniform) |
| `4448 A 5315 PEINADOS` | hairstyles | 18,752 .. 19,008 |
| `5322 A 5443 BOTAS` | boots | **71,952 .. 71,952** (uniform) |

The ranges land on groups of identically-sized assets, which they would not
if the numbering were arbitrary. Independent corroboration from the community
side: a forum walkthrough says *"Go to file 05322.bin (it's in block 6)"* —
and `5322` is exactly the first index of the `BOTAS` range, while "block 6"
is the sixth range in the file.

**Conclusion (FACT): the guide's numbers are AFS entry indices into
`0_text.afs`.**

### 1.4 What the size annotations reveal about asset formats

The map annotates two entries with explicit dimensions:

```
- 0535 ESCUDOS 32X32
- 0536 ESCUDOS 64X64
```

Measured against our binary:

| Entry | Annotated | Our inflated size | Implied bytes/pixel |
|---|---|---|---|
| 535 | 32×32 | **1,024** | 1024 / 1024 = **1** |
| 536 | 64×64 | **4,096** | 4096 / 4096 = **1** |

**FACT: crest assets are 8 bits per pixel, not 32.** Other assets in the same
archive are 32-bit (e.g. an entry at 16,400 bytes = 64×64×4 + a 16-byte
header), and the 16-bit form is also present (an entry at 8,224 bytes =
64×64×2 + 20).

**This is the direct cause of our current blocker** (§3).

---

## 2. How the community actually names assets

### 2.1 The decisive precedent — kitserver names entries by index

`references/kitserver6/src/afswalk.cpp` reads the AFS name record and then
**overwrites the name with an index-based one**:

```c
fread(&name, sizeof(AFSNAMEINFO), 1, f);
sprintf(name.szFileName, "id_%00005d.bin", i);
```

It does this precisely because the real names are encrypted. Its output is:

```
abs.offset = 00013800, size = 0000093f: id_00000.bin
abs.offset = 00114800, size = 00000760: id_00049.bin
```

**=> The community's own tooling already exposes AFS entries as
`id_<5-digit index>.bin`.** Our proposed naming is not an invention; it is
the established convention.

Note the entry-49 example: our own analysis independently found entry 49 at
file offset `0x00114800`, size `0x760` = 1,888. **Identical.** That is an
exact cross-validation of our AFS parser against kitserver's.

### 2.2 The per-category runtime map

kitserver's shipped asset tree uses a *different*, higher-level schema, one
folder per category, each with a map file:

```
for_releases/GDB/gloves/map.txt
for_releases/GDB/balls/empty_map.txt
for_releases/GDB/uni/masks/templates.txt
```

`GDB/gloves/map.txt`:

```
# format:
# <player-id>, "<left-glove>", "<right-glove>"
3133, "Barcelona\VictorValdes-left.png", "Barcelona\VictorValdes-right.png"   # V.Valdes
599,  "Real Madrid\bandeeta-left.png", "Real Madrid\bandeeta-right.png"      # Casillas
```

`GDB/balls/empty_map.txt`:

```
# Format: "<name displayed in game>","<model file>","<texture file>"
# Texture paths are relative to GDB\balls\, model paths to GDB\balls\mdl\
"adidas Finale Capitano Grey","ariel_mdl.bin","adidas_finale_capitano_grey.bmp"
```

**=> Two schema families coexist:**
1. **Archive level** — AFS entry index (`id_06920.bin`).
2. **Content level** — a stable content id (player id / ball name) mapped to
   file paths relative to a category folder, via a per-category map file.

The second is what a *user-facing* pack wants; the first is what the archive
actually gives you. **The pack spec below bridges them.**

### 2.3 HD texture mod practice

From the community (SOURCE-CLAIM):

* *"PES6 HD Nets Pack converted by Jmats … he made it with two sizes **256x256
  and 1024x1024**"* — HD packs ship artwork at **two** sizes.
* *"dmitri-f … working on an HD plugin for pes5 and pes6"* — the long-running
  HD-plugin project.
* PES Ultimate Data Explorer workflow: open `0_TEXT.AFS` → the entries appear
  as `06920.bin` → export / edit / *"Import Marked Files"* → save.

**Note (SOURCE-CLAIM → inference):** our largest *managed* texture is 512×512
and `texture_scale = 2` makes it 1024×1024 — which is exactly the second
size Jmats ships. **The community's HD packs are the enlarged-texture
approach, which independently corroborates our M4 finding.**

---

## 3. Our own blocker, characterised

### 3.1 The symptom

The CRC32 of the pixels the game uploads into a texture does **not** match
the CRC32 of any inflated archive entry (0/30 sampled).

### 3.2 The cause — now established

FACT: the archive holds textures in **several native formats** (§1.4):
1-byte-per-pixel (crests), 2-byte (RGB565), and 4-byte (A8R8G8B8), with or
without a header. The game converts all of them into the single
`D3DFMT_A8R8G8B8` the texture uses. **A CRC taken after conversion can never
equal a CRC taken before it.**

This also corrects an earlier claim in `STATUS.md`: the value `0x20050420`
that appears at offset 16 of many inflated assets **is not a magic constant
the game checks**. This session found that exact 4-byte sequence occurs
**once in the entire 20 MB binary**, at `0x008E246B`, inside a
character-index lookup table (`00 20 20 01 02 03 20 20 04 05 20 06 07 …`,
i.e. an index→glyph byte map). No `cmp` against `0x20050420` exists anywhere
(`3D 20 04 05 20` and `81 F8..FF 20 04 05 20` were both searched for).
**The game parses asset headers structurally, not by magic.**

### 3.3 Where the conversion happens

Not yet located. Two leads:

| Lead | Evidence | Status |
|---|---|---|
| The asset loader `FUN_0065B100` (allocate + inflate + hand over the buffer) | decompiled this session; `entry+0x04` compressed size, `entry+0x08` decompressed size, `entry+0x20` compressed data pointer; destination buffer at `param_1+100` | FACT — the pre-conversion point, already hooked by us at `0x008CD490` |
| The texture-creation path | `CreateTexture` is called from the game's per-texture code; the game's art is uploaded through `IDirect3DTexture8::LockRect` | FACT (address of the call site is **not** yet reliably known — see §5) |

**=> The conversion is somewhere between the inflate and the upload.**

---

## 4. Proposed pack-by-name spec

### 4.1 Folder layout

```
remaster_pack/
    index.txt          asset replacements, keyed by archive entry
    hires.txt          texture replacements, keyed by texture size
    names.tsv          the archive index -> human name map (from afsmap.txt)
    art/
        <category>/     human-named artwork, e.g. art/faces/2506_beckham.png
```

### 4.2 Naming rules

**Rule 1 — every addressable thing has an archive entry index.**
The archive entry index is the only identifier the game exposes. It is stable
for a given game build, it is what the community already uses
(`id_%00005d.bin`), and it is what `afsmap.txt` documents.

**Rule 2 — human names come from a side table, never from the pack.**
`names.tsv` maps `entry-index → name → category`, generated from
`afsmap.txt` plus our own measurements. The pack may reference a name; the
runtime resolves it through `names.tsv`. **If the name is unknown, the
pack entry is still usable by index.** The pack format therefore never
breaks when the game updates its name table.

**Rule 3 — artwork is addressed by texture size, not by asset.**
`hires.txt` maps `WxH → file`. The texture size is exactly what the game
asks for at creation time, it is unambiguous, and it does not require the
asset→texture association to be solved first. This is the key simplification.

**Rule 4 — a replacement preserves the asset's own structure.**
An archive replacement (`index.txt`) must have the **same byte length** as
the original entry: the game parses the asset's header structurally and
expects the declared sizes to match. Only the pixel region may differ.

### 4.3 Resolution rules

* Artwork may be **any size**; it is filtered into the texture with an area
  average (the correct filter when shrinking).
* For an archive replacement the artwork must be in the **asset's own
  native format** (8-bit indexed / 16-bit / 32-bit, with the asset's own
  header). This is the one place where the asset format has to be known per
  entry — and it is known, from the size annotations and from measurement.
* `texture_scale` decides the texture's real size. At `2`, artwork should be
  authored at 4× the texture size to be supersampled properly.

### 4.4 Fallback behaviour

| Situation | Behaviour |
|---|---|
| Pack names a texture size with no artwork | the game's own art is used (filtered up into the enlarged texture) |
| Pack names an asset index that does not exist | the entry is skipped and logged |
| Pack names an asset by name that `names.tsv` does not know | fall back to the index form; if that also fails, skip and log |
| Replacement has the wrong byte length | rejected and logged, never copied |
| Pack folder absent | every layer is a no-op; the game is unaffected |

**Every fallback is non-destructive: an unusable pack degrades to "no
replacement", never to a crash or a corrupt texture.**

### 4.5 Worked example

```hires.txt
512x512   menu_background.png
256x256   badge_generic.png
256x64    ribbon.png
```

```
index.txt
C9CA48F1 id_05318.bin     # CRC32 of the inflated original
```

```names.tsv
535    ESCUDOS 32X32   crests
536    ESCUDOS 64X64   crests
5322   BOTAS           boots
2506   BECKHAM         faces
```

---

## 5. RE tasks required to implement it

| # | Task | Effort | Risk | Notes |
|---|---|---|---|---|
| R1 | Locate the asset→texture conversion function and prove the format chain | **M** | low | read-only; decompile the path between the inflate and `LockRect` |
| R2 | Determine each entry's native pixel format and header layout | **M** | low | we already know 8/16/32-bit variants exist and two exact examples; the header is 0/16/20 bytes depending on kind |
| R3 | Build `names.tsv` from `afsmap.txt` + our measured sizes | **S** | none | data conversion only; `afsmap.txt` is already parsed by a working script |
| R4 | Build the asset→texture association (which entry feeds which texture) | **L** | medium | needs R1; the fallback is to key art by texture size only, which avoids R4 entirely |
| R5 | Locate the game's asset-header parser to confirm the fields the game actually reads | **M** | low | needed to write *valid* replacements; must not guess |
| R6 | Verify the community's block→category map covers the whole archive | **S** | none | `afsmap.txt` names 289 ranges; the other ~9,500 entries stay unnamed |

**R4 is optional.** Rules 1–4 of the spec work without it, because artwork is
addressed by texture size. R4 only matters for naming *individual* assets
rather than texture sizes.

**Recommendation: implement Rules 1–3 + R1/R2/R3/R5 and skip R4.**

---

## 6. Sources

| Source | Type | Where |
|---|---|---|
| `references/kitserver6/src/misc/afsmap.txt` | community RE, 360 lines | local |
| `references/kitserver6/src/afswalk.cpp` (`id_%00005d.bin`) | community tool source | local |
| `references/kitserver6/src/afsreplace.cpp` | community hook set | local |
| `references/kitserver6/for_releases/GDB/*/map.txt` | community asset schema | local |
| `https://www.scribd.com/document/619044499/0-Text-Pes-6-Afs-map` | community guide | ❌ blocked |
| `https://www.scribd.com/document/484246003/Maps-e-text` | community guide | ❌ blocked |
| forum walkthrough: *"Open your PES6 0_TEXT.AFS … file 06920.bin"* | community | web |
| *"PES6 HD Nets Pack … two sizes 256x256 and 1024x1024"* | community | web |
| our own measurements of `0_text.afs` | FACT | this project |
