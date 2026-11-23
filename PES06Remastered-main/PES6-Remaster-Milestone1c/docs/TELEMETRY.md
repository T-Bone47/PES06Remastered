# Match telemetry

**Date:** 2026-10-21
**Module:** `remaster/telemetry/telemetry.cpp`
**Output:** `remaster_telemetry.csv` in the game folder
**Switch:** `[telemetry] enabled = 0` in `remaster.ini` — **off by default**

Everything the module does is a **read**. It never writes to the game's
memory and it never calls into the game's code. Every read is wrapped in
`__try`/`__except`, so a bad address can only ever cause a sample to be
skipped.

---

## 1. What was built

A background thread (250 ms by default, `interval_ms` in the config) samples:

| What | Address | How it is read | Evidence |
|---|---|---|---|
| match mode | `0x01311774` | direct read | **FACT** (read directly from global) |
| match state | `0x03BECC60` | direct read | **FACT** (verified runtime) |
| match type | `0x03BE12C9` | direct read, one byte | **HYPOTHESIS** (static analysis) |
| clock | `0x03A6A260` | direct read, as a float | **UNVERIFIED** (reads 0.0000) |
| **live entity table** | `0x01037B58` | direct read, 64 entities of 0x48 bytes | **HYPOTHESIS** (static analysis) |
| side 0 roster | `0x01131FF4` | direct read, 32 records of 0x7C bytes | **FACT** (verified runtime) |
| side 1 roster | `0x01132048` | direct read, 32 records of 0x7C bytes | **FACT** (verified runtime) |
| squad table | `0x03A8AA78` | direct read, 2 × 32 player ids | **HYPOTHESIS** (static analysis) |
| globals window | `0x03A6A000` + `0x400` | direct read | **FACT** (70 changing bytes observed) |

### The row format

```
wall_ms, mode, phase, matchtype, clock,
  <0x48 hex bytes> × 64    live entities
  <0x7C hex bytes> × 32    side 0 roster      (optional, `roster = 1`)
  <0x7C hex bytes> × 32    side 1 roster      (optional)
  <0x400 hex bytes>        globals window
```

Plus `squad,<64 ids>` once per match and `# probe …` comment lines.

### Labelled memory dumps

The thread watches `remaster_scan\label.txt`, **which it creates itself**
(the folder and an empty file are made at startup, so the path always
exists). **When the text in that file changes, every writable, committed
region of the process is dumped** to `remaster_scan\<label>.bin`, along with
the address of each region. This is how a human-played match can be labelled
from outside the game.

VERIFIED end to end: with the telemetry on, writing `alpha` into the label
file produced `alpha.bin` (371 regions, 178.2 MB) and writing `beta`
produced `beta.bin`, and `tools/scan.py diff` compared them. **Each dump is
about 178 MB, so four labels is roughly 700 MB of disk.**

The dump format is a sequence of records: the text line `R <base> <size>`
followed by that many raw bytes. `tools/scan.py diff` compares two dumps.

---

## 2. Where the addresses come from

All from the Ghidra database of `pes6.exe`, sha256
`0c7f804be8a609787713e59aa1cbceb76bb804a473ba3e6149e834492418e30f`.

### The mode — `0x01311774`

`FUN_00425198` is an obfuscated trampoline. Its raw bytes:

```
00425198  9C              PUSHFD
00425199  F8              CLC
0042519A  A1 58 A4 46 04  MOV  EAX, [0x0446A458]
004251A0  13 05 11 1D 4A 00  ADC EAX, [0x004A1D11]
004251A6  8B 00           MOV  EAX, [EAX]
004251A8  9D              POPFD
004251A9  E9 78 D2 23 00  JMP  0x00662425
```

and its tail call target, `FUN_00662425`, is a **clamp**:

```asm
0x662425: TEST EAX, EAX
0x662427: JLE  0x66242e
0x662429: CMP  EAX, 0x2
0x66242c: JLE  0x662430
0x66242e: XOR  EAX, EAX
0x662430: RET
```

**=> the mode is `clamp(*( *(0x0446A458) + *(0x004A1D11) ), 0, 2)`.** The two
constants are embedded in the instruction stream, so their sum is a fixed
address — **`0x01311774`** — in the writable tail of `.data`. It is read
directly here; nothing is called.

**=> The mode selects which entity table the game uses.** That is why
reading the roster table found nothing: it is the wrong table.

### The live entity table — `0x01037B58`

`FUN_0067F490`, the entity accessor, does:

```c
iVar1 = FUN_0068d9b0();            // returns DAT_01037B58
if (iVar1 != 0) {
    if ((uVar3 < 0x7000) || (0x701f < uVar3)) {
        if (uVar3 < 5000) {
            iVar2 = FUN_00666120();
            if (iVar2 != 0) {
                return iVar1 + (param_1 & 0xffff) * 0x48;   // the live entity
            }
        }
        iVar1 = FUN_00862180(param_1);
        return iVar1;
    }
    ...
```

and `FUN_0068D9B0` is `return DAT_01037B58;`.

**=> the live entity is `DAT_01037B58 + index * 0x48`, for an index below
5,000, when `FUN_00666120` is non-zero.** 0x48 = 72 bytes per entity.

There are two more tables, reached through other paths:

| Table | Base | Stride | Source |
|---|---|---|---|
| live entities | `0x01037B58` | `0x48` | `FUN_0068D9B0` |
| static entities | `0x0103E104` | `0x7C` | `FUN_006AEEF0` — `&DAT_0103E104 + (index - 0x6000) * 0x7C` |
| rosters | `0x01131FF4` / `0x01132048` | `0x7C` | `FUN_00866B90` / `FUN_00866CB0` |

### The match type — `0x03BE12C9`

`FUN_00662D70` reads it as a byte: **4 for a league and 5 for a master
league** (`return *(uint *)(PTR_DAT_00BD7C80 + 0x20) & 0x20000` in the second
case).

---

## 3. What is VERIFIED

### The harness works — FACT

* 223 samples over a 56.7 s window; 0 failed reads
* the game stays alive with the module enabled and disabled
* the address gate stays at **119/119** and the gameplay gate at **101/101**
  in both configurations

### The state value is real match state — FACT

`DAT_03BECC60` is stable in a menu and moves as a match progresses. The same
sequence in three independent runs:

```
t+  0.0 s   phase=0
t+  4.8 s   phase=2
t+ 18.9 s   phase=3
t+ 49.8 s   phase=12
```

**Its units are NOT established.** It is a state value, not a clock.

### The record table base appears when a match starts — FACT

`DAT_01131FF4` goes from `NULL` to a heap pointer when the match starts and
then stays constant. `DAT_01132048` (side 1) stayed `NULL` in every sample.

### The roster table is static — FACT (negative result)

The first **4,096 bytes** of the side 0 roster were dumped on every sample of
a 226 s run: **0 of 4,096 byte offsets changed**. The same holds for the
32 × `0x7C` record layout.

**=> the roster holds the squad's static data, not the live match state.**
This is why the first attempt found no position, stamina or possession field:
it was reading the wrong table. The live table is the 0x48-stride one above.

### The mode stayed 0 — FACT (negative result)

`0x01311774` read **0** in every sample of every run. The mode selects the
entity table, and with the mode at 0 the game used the static fallback.
The matches that were sampled were **attract mode** — the game starts one on
its own if the title screen is left alone — not a user match.

---

## 4. What is UNVERIFIED

* **The live entity table was never populated in a sample**, because the mode
  was 0 throughout. The table is read and dumped every sample; it is simply
  empty. **A user-played match is needed to fill it.**
* `DAT_03A6A260`, the address nominated as the clock, read as a float was
  **0.0000 in every sample of four runs**. It is either the wrong address or
  a value only written in some other game state.
* The on-screen clock has **not** been correlated against any telemetry
  value, because no frame with a visible clock and a matching sample has
  been captured.
* No position, stamina or possession field has been identified.

---

## 5. The controlled test that is still needed

The protocol, for a human to run:

1. Turn the telemetry on (`[telemetry] enabled = 1`).
2. Start a **user** match from the main menu (not attract mode). Confirm it
   is a user match by checking that `mode` is 1 or 2 in `remaster.log`.
3. During the match, write a label into `remaster_scan\label.txt` at each
   controlled moment — the file's contents are the label, and changing it
   takes a memory dump:

   | label | when |
   |---|---|
   | `stand` | holding still, 10 s |
   | `sprint` | sprinting, 10 s |
   | `goal` | just after a goal is scored |
   | `minute` | one minute later than `goal` |

4. Let the game run to the end of the match.
5. The telemetry CSV and the dumps are then diffed by `tools/scan.py`:

   ```
   python tools/scan.py diff remaster_scan\stand.bin remaster_scan\sprint.bin
   python tools/scan.py entities remaster_telemetry.csv
   ```

**The acceptance rule:** a byte offset is only called a field if two
independent controlled tests agree — for example, if the bytes that differ
between `stand` and `sprint` also differ between `goal` and `minute` in the
way the field would be expected to.

---

## 6. Tooling

`tools/scan.py` has two modes:

* `scan.py diff a.bin b.bin` — compares two memory dumps and reports which
  addresses changed, ranked by the size of each changed run. Optional
  `--csv out.csv` and `--stride N`.
* `scan.py entities telemetry.csv` — analyses the live entity table: per
  entity activity, the phase timeline, and a ranking of the byte offsets
  that change.

`--stride N` reports how the changed bytes fall over a record stride, which
is how a live table is told apart from a busy buffer: a table concentrates
the changes on a few residues, a buffer spreads them flatly.

Measured on a real pair of dumps: 186,892,288 bytes compared, 207,580
changed (0.11%), 19,810 changed runs. At stride `0x48` the peak residue
concentration was **1.24x** the even share — flat, so that region is **not**
an entity table. That is the tool working: it eliminated a candidate.

---

## 7. Cost and behaviour

* 223 rows in 57 s at 250 ms, about 8.7 MB — the CSV is large because it
  carries the raw tables. `interval_ms` and `roster` trade detail for size.
* The module allocates nothing per sample beyond the CSV's own buffering.
* With it enabled the game behaved exactly as with it disabled: the gates
  were unchanged, the game stayed alive, and no crash was observed.


---

## 8. Session 1b — the instrumentation is proven

Everything above the field mapping is now tested against real runs.

| Check | Result |
|---|---|
| the telemetry samples | 223 samples over a 56.7 s window, 0 failed reads |
| the label file is created by the module | `telemetry: label file is readable` |
| a label produces a dump | `scan dump 'alpha' -> 371 regions, 178.2 MB` |
| two labels produce two dumps | `alpha.bin` and `beta.bin` |
| `scan.py diff` on two real dumps | 186,892,288 bytes compared, 207,580 changed (0.11%), 19,810 runs, ranked |
| `scan.py entities` on a real CSV | parses it and reports the phase timeline |
| `scan.py diff --stride 0x48` | peak concentration 1.24x — flat, so the region is not an entity table |

Two bugs in the new code were found by testing it and fixed:

* `readLabel` opened the **folder** instead of the label file inside it.
* the label file did not exist until the user created it, and the module's
  own `CreateDirectory` ran before the label path was built. The module now
  creates the file itself at startup.

**=> The only thing still missing is the data that only a person can
produce: a user match with the four labels, and a saved master league.**
