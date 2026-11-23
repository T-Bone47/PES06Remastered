# Option files (`.opd`)

**Date:** 2026-10-20
**Tool:** `tools/opd_tool.py`
**Status:** the crypto is implemented and the file layout is documented; **the
in-game round trip is UNVERIFIED because no real option file could be
obtained in this session.**

---

## 1. What an option file is

The option file holds the master league: the teams, the created players, the
formations, the shirt numbers and the stadium names. It is obfuscated in two
independent ways.

### Pass 1 — a rolling XOR over the whole file

```
param = 0x573
for each byte:
    byte ^= (param & 0xFF)
    param = (param * 0x6C078965 + 1) & 0xFFFFFFFF
```

This pass is its own inverse, so the same code decodes and encodes.

### Pass 2 — a keyed pass over nine sections

```
key[446]   (the table in opd_tool.py)
FACTOR2    = 0x7AB3684C

encrypt:  v = dword ^ FACTOR2 ;  dword = v + key[i % 446]   (signed byte)
decrypt:  v = dword - key[i % 446] ; dword = v ^ FACTOR2   (signed byte)
```

The nine sections are found through a table of section indices stored at
offset `0x4A` of the file:

```
sections  0, 4, 7, 10, 12, 18, 20, 24, 26
```

for each of them, `dword[(section + 0x4A) * 4]` is the offset of that
section's data, and the loop runs `dword[0] >> 2` times starting at
`dword[j + 3]`.

### The order of the two passes matters

The section pass reads its offsets out of the file, and the rolling XOR
changes those bytes. So the two directions are **not** mirror images in
composition — the reference implementation does:

```
decode:  rolling XOR  then  section decrypt
encode:  section encrypt then  rolling XOR
```

`opd_tool.py` implements exactly that, and `transform()` takes the
direction as an argument.

---

## 2. The file layout

The section table below is **SOURCE-CLAIM** — it comes from kitserver's
`filedec.cpp`, which is the reference implementation in
`references/kitserver6/src/filedec.cpp`. It has **not** been confirmed
against a real option file, because none was obtained.

### Header

| Offset | Size | Meaning |
|---|---|---|
| `0x00` | 4 | unknown |
| `0x04` | 4 | unknown |
| `0x08` | 4 | unknown |
| `0x0C` | 1 | file type: 0 option, 1 replay, 2 formations, 3 cup, 4 league, 5 master league, 7 challenge training, 8 dribbling, 10 master league (best team) |
| `0x4A` | 4 × 9 | the section index table used by pass 2 |

### Section headers

There are two or three section headers, at file offsets `0x0000`, `0x1400`
and `0x1800`. Two are used for most file types, three for a league
(type 4) or a master league (type 5).

Each header is:

| Offset | Size | Meaning |
|---|---|---|
| `+0x00` | 4 | the size of the section's payload |
| `+0x04` | 4 | a checksum over the payload |
| `+0x0A` | 1 | the section type |
| `+0x0C` | 4… | the payload, as 32 bit words |

The checksum is the sum of the payload's 32 bit words from `+0x0C` onwards.
The number of words is derived from the section type's expected size,
rounded up to 0x400, minus 0x3C, and then clamped to the size in the
header.

### Section types

| Type | Name | Payload |
|---|---|---|
| 0 | general | header only |
| 1 | stadium names | header only |
| 2 | stadium names | 61 bytes per name |
| 3 | league names | 84 bytes each, the displayed name starts at byte 22 |
| 4 | created players | header only |
| 5 | created players | 124 bytes each, up to 184 players |
| 6 | shoes | 92 bytes for each of 9 shoes |
| 7 | standard players | header only |
| 8 | standard players | 124 bytes each, up to 5000 players |
| 9 | player numbers | header only |
| 10 | national player numbers | 23 bytes per team |
| 11 | club player numbers | 32 bytes per team |
| 12 | national team players | 46 bytes per team, 23 players × 2 bytes |
| 13 | club team players | 64 bytes per club team, 145 teams |
| 14 | team formations | 628 bytes each |
| 15 | team infos | header only |
| 16 | team infos | 140 bytes, club |
| 17 | team kit infos | header only |
| 18 | team national kit infos | 544 bytes per team |
| 19 | team club kit infos | 544 bytes per team |

### Expected payload sizes per section type

```
0x131400 0x47000 0x3000 0x65400 0x65800 0xB7800 0xB4400
0x9000   0x9000  0x9000 0x4800  0x11400 0x65400
```

---

## 3. The tool

```
python tools/opd_tool.py decrypt <in.opd> <out.bin>
python tools/opd_tool.py encrypt <in.bin> <out.opd>
python tools/opd_tool.py roundtrip <in.opd>        # decrypt then re-encrypt
python tools/opd_tool.py dump <file>               # structure
```

`roundtrip` is the acceptance test: it decrypts, re-encrypts and compares
against the original byte for byte.

---

## 4. What is VERIFIED, and what is not

### VERIFIED — the implementation is self consistent on a synthetic file

The two passes are inverses of each other in the matching order, which is
checked by `roundtrip` on any input.

### NOT VERIFIED — against a real option file

**No real option file was obtained in this session.** This is the honest
state, and it has three parts:

1. **The game has never written one on this machine.** The save folder is
   `<Documents>\KONAMI\Pro Evolution Soccer 6\save\` — the strings
   `\KONAMI\` (at `0x0077E1AC`) and `/save` (at `0x0077DFD0`) are in the
   binary — but the folder does not exist, because nothing has been saved
   yet.

2. **Navigation into the game was unreliable.** The menu polls DirectInput,
   so keys have to be injected as real events (`SendInput`, now in
   `tools/rea/press.ps1`; the earlier `SendKeys` approach silently did
   nothing). Even then the arrow keys did not always register, and the
   game's **attract mode** starts a match on its own if the title screen is
   left alone, which is where most of the match samples came from.

3. **The one candidate file in the tree is not an option file.**
   `pesmod/test_files/unzlib_default00901.opd` (16,208 bytes) is a *decoded*
   name/offset table — it contains plain text such as `common_wind.x` — so
   it is not in the on-disk format and the round trip against it is
   meaningless.

**=> The in-game round trip — decrypt a real `.opd`, re-encrypt it, get the
original bytes back, have the game load it — remains UNVERIFIED.** The
missing step is a single saved option file.

### The one edit that was not made

The task asks for one minimal verified edit (a team rename or a player
attribute) proved on screen. **It was not made, because there is no option
file to edit.** Making an edit to a file the game has never written and
cannot be made to write is not something that can be verified.

---

## 5. How to finish this

The shortest remaining path:

1. Get into a real user match from the main menu (MATCH → default teams →
   start). The keyboard injection works; the timing needs a longer settle
   after the title screen.
2. From the match, quit in a way that makes the game write the option file
   (the master league save is the reliable one; the option file is written
   when a league is created or saved).
3. Confirm the file appears under
   `<Documents>\KONAMI\Pro Evolution Soccer 6\save\`.
4. `python tools/opd_tool.py roundtrip <file>` — this must print
   `ROUND TRIP OK`.
5. `python tools/opd_tool.py dump <file>` to see the real section headers and
   replace the SOURCE-CLAIM table above with confirmed offsets.
6. Edit one team name in the decoded form, re-encrypt, and start the game to
   see the change.

Until step 4 passes, the tool should be treated as a faithful port of the
reference algorithm rather than as a verified decoder.
