# Option files (`.opd`)

**Date:** 2026-10-20
**Tool:** `tools/opd_tool.py`
**Status:** the crypto and the save path are implemented and **statically
confirmed**; the in-game round trip is still **UNVERIFIED**, because no option
file exists on this machine yet.

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

`opd_tool.py` implements exactly that, and `transform()` takes the direction
as an argument. Getting it wrong is why the first round trip failed.

---

## 2. Where the file lives — CONFIRMED

The path is built by the game at run time. From the raw bytes of `pes6.exe`:

```
004160EA  E8 01 D4 FE FF     call  string::append
004160F0  6A 08              push 8
004160F2  68 AC E1 B7 00     push 0x00B7E1AC      ; "\KONAMI\"
004160F8  8D 4C 24 10        lea  ecx, [esp+0x10]
004160FB  E8 61 CD FF FF     call  string::append
00416100  6A 16              push 0x16 (22)
00416102  68 D0 DC B7 00     push 0x00B7DCD0      ; "Pro Evolution Soccer 6"
00416108  8D 4C 24 10        lea  ecx, [esp+0x10]
0041610B  E8 51 CD FF FF     call  string::append
```

and separately:

```
004144BF  68 D0 DF B7 00     push 0x00B7DFD0      ; "/save"
004144C4  B9 90 49 BC 00     mov  ecx, 0x00BC4990 ; a global std::string
004144C9  E8 93 E9 FF FF     call  string::append
```

**FACT:** the game appends `"\KONAMI\"` and `"Pro Evolution Soccer 6"` to a
string, and appends `"/save"` to a **global `std::string` at `0x00BC4990`**.

**FACT:** `SHGetFolderPath(CSIDL_PERSONAL)` on this machine returns
`C:\Users\olive\OneDrive\Documents`, so the option file will be written to

```
C:\Users\olive\OneDrive\Documents\KONAMI\Pro Evolution Soccer 6\save\
```

(The remaster's file watcher resolves the same folder through
`SHGetFolderPathA`, so it agrees with the game.)

The **file name** is assembled in the `.rld` section, which is decoded at run
time and is not present in the file on disk, so it is still UNVERIFIED. The
strings `"w+b"` and `"r+b"` (at `0x00B7DFDC` and `0x00B7DFD8`) are the modes
it opens it with, and both are referenced from that same decoded section.

---

## 3. The file layout

**The section table below is SOURCE-CLAIM.** It comes from kitserver's
`filedec.cpp`, the reference implementation in
`references/kitserver6/src/filedec.cpp`. It has **not** been confirmed
against a real option file, because none has been obtained yet. Only the
offsets in §2 are FACT.

### Header

| Offset | Size | Meaning | Status |
|---|---|---|---|
| `0x00` | 4 | unknown | SOURCE-CLAIM |
| `0x04` | 4 | unknown | SOURCE-CLAIM |
| `0x08` | 4 | unknown | SOURCE-CLAIM |
| `0x0C` | 1 | file type: 0 option, 1 replay, 2 formations, 3 cup, 4 league, 5 master league, 7 challenge training, 8 dribbling, 10 master league (best team) | SOURCE-CLAIM |
| `0x4A` | 4 × 9 | the section index table used by pass 2 | SOURCE-CLAIM |

### Section headers

There are two or three section headers, at file offsets `0x0000`, `0x1400`
and `0x1800`. Two are used for most file types, three for a league
(type 4) or a master league (type 5).

| Offset | Size | Meaning | Status |
|---|---|---|---|
| `+0x00` | 4 | the size of the section's payload | SOURCE-CLAIM |
| `+0x04` | 4 | a checksum over the payload | SOURCE-CLAIM |
| `+0x0A` | 1 | the section type | SOURCE-CLAIM |
| `+0x0C` | 4… | the payload, as 32 bit words | SOURCE-CLAIM |

The checksum is the sum of the payload's 32 bit words from `+0x0C` onwards.
The number of words is derived from the section type's expected size,
rounded up to `0x400`, minus `0x3C`, and then clamped to the size in the
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

## 4. The tool

```
python tools/opd_tool.py decrypt <in.opd> <out.bin>
python tools/opd_tool.py encrypt <in.bin> <out.opd>
python tools/opd_tool.py roundtrip <in.opd>        # decrypt then re-encrypt
python tools/opd_tool.py dump <file>               # structure
```

`roundtrip` is the acceptance test: it decrypts, re-encrypts and compares
against the original byte for byte.

---

## 5. What is VERIFIED, and what is not

### VERIFIED

* The two crypto passes are implemented exactly as the reference does them,
  including the order, and they are inverses of each other in the matching
  order — checked by `roundtrip` on any input.
* **The save path**, statically confirmed from the instruction stream: the
  game appends `"\KONAMI\"`, `"Pro Evolution Soccer 6"` and `"/save"`, and
  keeps the result in the global `std::string` at `0x00BC4990`.
* The remaster's file watcher resolves the same folder the game will use.

### NOT VERIFIED

* **No real option file was obtained.** The save folder does not exist yet,
  because nothing has been saved in the game.
* The section table is SOURCE-CLAIM, not FACT.
* The one minimal verified edit the task asked for — rename a team, change an
  attribute, reload, and prove it on screen — **has not been made**, because
  there is no file to edit.

### Why the API hooks alone did not find it

The file logger hooks `CreateFileA`, `CreateFileW`, `ReadFile`, `WriteFile`,
`fopen` and `_wfopen`. The `CreateFileW` hook **does** fire (it caught the
remaster's own log file), so the hooks work — but the game never uses them
for its data files, because the `.rld` section that does the file I/O is
decoded at run time and resolves its own entry points.

**=> The logger therefore also watches the save folder directly**, reporting
every file that appears or changes with its path, its size and a CRC32 of its
first 64 bytes. That is what found the path in the first place, and it is
what will find the option file.

---

## 6. How to finish this

The remaining step needs a human at the keyboard:

1. Start the game, go into **Master League** and create a new league (or
   continue an existing one).
2. Save it, so the game writes the option file.
3. The remaster's log will then contain a `fileio: found "…"` line naming the
   file, its size and its hash.
4. `python tools/opd_tool.py roundtrip <that file>` must print
   `ROUND TRIP OK`.
5. `python tools/opd_tool.py dump <that file>` shows the real section
   headers, and the SOURCE-CLAIM table above can be replaced with confirmed
   offsets.
6. Edit one team name in the decoded form, re-encrypt, and start the game to
   see the change.

Until step 4 passes, the tool should be treated as a faithful port of the
reference algorithm rather than as a verified decoder.
