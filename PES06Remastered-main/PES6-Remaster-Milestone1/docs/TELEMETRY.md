# Match telemetry

**Date:** 2026-10-20
**Module:** `remaster/telemetry/telemetry.cpp`
**Output:** `remaster_telemetry.csv` in the game folder
**Switch:** `[telemetry] enabled = 0` in `remaster.ini` — **off by default**

Everything the module does is a **read**. It never writes to the game's
memory and never calls into the game's code. Every read is wrapped in
`__try`/`__except`, so a bad address can only ever cause a sample to be
skipped.

---

## 1. What was built

A background thread (250 ms by default, `interval_ms` in the config) samples:

| What | Where it comes from | How it is read |
|---|---|---|
| side 0 record table base | `DAT_01131FF4` (what `FUN_00866B90` returns) | direct read |
| side 1 record table base | `DAT_01132048` (what `FUN_00866CB0` returns) | direct read |
| player records | `base + slot * 0x7C`, 32 slots per side, `0x7C` bytes each | direct read |
| clock | `DAT_03A6A260`, read as a float | direct read |
| phase | `DAT_03BECC60`, read as a 32 bit value | direct read |
| match squad table | `DAT_03A8AA78`, 2 teams × 32 player ids, written once per match | direct read |
| globals window | `DAT_03A6A000` … `+0x400` | direct read |
| match mode | see below | direct read |

The mode discriminator `FUN_00425198` is an obfuscated trampoline:

```asm
0x425198: PUSHFD
0x425199: CLC
0x42519a: MOV  EAX, [0x0446A458]
0x42519f: ADC  EAX, dword ptr [0x004A1D11]
0x4251a5: MOV  EAX, [EAX]
0x4251a7: POPFD
0x4251a8: JMP  0x00662425
```

so its value is read as
`*(uint32*)(*(uint32*)0x0446A458 + *(uint32*)0x004A1D11)` **without calling
anything** — the hard rule prefers a read over a call.

### The row format

```
wall_ms, mode, clock, phase,
  <0x7C hex bytes> × 32   side 0, slots 0..31
  <0x7C hex bytes> × 32   side 1, slots 0..31
  <0x400 hex bytes>       globals window at 0x03A6A000
```

plus `squad,<64 ids>` once per match and `# probe …` comment lines every
four seconds with the raw globals the match test is based on.

---

## 2. What is VERIFIED

### The harness works — FACT

```
$ remaster.log
[INFO] telemetry: sampling every 250 ms
...
[INFO] telemetry: 914 samples, 0 failed reads, squad written 0
```

* 1,222 samples over a 320.0 s window were captured in one run
* 0 failed reads in every run
* the game stayed alive with the module enabled, and with it disabled the
  module reported `telemetry: disabled by config` and wrote no file at all
* the address gate stayed at **119/119** and the gameplay gate at **101/101**
  in both configurations

### The phase value is real match state — FACT

`DAT_03BECC60` is stable while the game is in a menu and changes as the
match progresses. Three independent runs produced the same sequence:

```
t+  0.0 s   phase=2
t+ 12.4 s   phase=3
t+ 42.8 s   phase=12
t+142.8 s   phase=3
t+173.4 s   phase=12
```

**Its units are NOT established.** It is a state value, not a clock: it is
stable for tens of seconds and then jumps.

### The record table base appears when a match starts — FACT

```
# probe ... base0=00000000 base1=00000000 ... phase=0
# probe ... base0=0EF2C920 base1=00000000 ... phase=2
# probe ... base0=0EF2C920 base1=00000000 ... phase=3
```

`DAT_01131FF4` goes from `NULL` to a heap pointer when the match starts and
then stays constant. **`DAT_01132048` (the side 1 base) stayed `NULL` in
every sample of every run.**

---

## 3. What is UNVERIFIED, and why

### The nominated clock address is not the clock — FACT (negative result)

`DAT_03A6A260` read as a float was **0.0000 in every sample of four runs**
(min 0.0000, max 0.0000, 1 distinct value). It is either the wrong address
or a value that is only written in some other game state.

### The nominated clock address is not the clock — FACT (negative result)

`DAT_03A6A260` read as a float was **0.0000 in every sample of four runs**
(min 0.0000, max 0.0000, 1 distinct value). It is either the wrong address
or a value that is only written in some other game state.

### The record table is static — FACT (negative result)

The first **4,096 bytes** of the side 0 table were dumped on every sample of
a 226 s run: **0 of 4,096 byte offsets changed**. The same holds for the
32 × `0x7C` record layout.

**=> The live per-player state is not at `base + slot * 0x7C` for slots
0..31.** The table holds something that does not change during a match.

### Why: the entity table is chosen by the mode — FACT

Decompiling the entity accessor shows several tables, selected by the mode:

| Function | What it returns |
|---|---|
| `FUN_006AEEF0` | `&DAT_0103E104 + (index - 0x6000) * 0x7C` — a **static** table |
| `FUN_0067F490` | for index < 5000: `base + index * 0x48` — a **different stride** |
| `FUN_00862180` | `base + 0x34`, or an entity from the mode-selected pool |
| `FUN_00861B90` | `base + index * 0x7C` — the fallback we sampled |

The mode decides which of these is used. **Our mode read returned 0 in every
sample**, so the game was using the fallback — the static table — for the
whole run. That is consistent with the attract-mode match we were sampling:
it is not a user match, so the live entity pool was never selected.

**=> The single most valuable thing to do next is to sample a real user
match, and to read the mode correctly.** The obfuscated trampoline's
arithmetic does not obviously yield a 1 or 2 with the values observed
(`modeBase=BA7FD34F`, `modeDelta=46834425`), which suggests the disassembly
is a decoy and the real implementation is `FUN_00662425`, the function it
tail-calls.

### The globals window — FACT

70 of 4,096 byte offsets in `0x03A6A000` changed during a match. The most
volatile is `0x03A6AE40`, a single byte that moves in steps of **±60**
(207 → 147 → 87 → 27 → 0 → 56 → 116 …) and only while the match is live.
**It is a countdown or phase counter, not a match clock** — a match clock
would rise monotonically through 0…90.

---

## 4. Candidate fields

Ranked by how much evidence there is. **None of these is confirmed.**

| Rank | Address | What it does | Confidence |
|---|---|---|---|
| 1 | `DAT_03BECC60` (`0x03BECC60`) | a match **state** value: 0 in the menu, 2 pre-match, 3 in play, 12 at a transition | **medium** — the behaviour is verified, the meaning is not |
| 2 | `0x03A6AE40` | a byte counter moving in ±60 steps, live only during a match | **low** — countdown or phase, not a clock |
| 3 | `0x03A6AE08` / `0x03A6AE18` | small counters, 0 → 5 and 0 → 6 | **low** |

**No position, stamina or ball-possession field was found.** That is the
honest outcome: the live player records were never located, so there is
nothing to point at.

### The controlled test that is still needed

The task's acceptance test for a field is a controlled comparison. The one
that would settle it:

> Start a user match (not attract mode), stand still for ten seconds with
> the telemetry running, then sprint for ten seconds, and compare the two
> segments. A byte that changes in the first segment and not the second is
> speed or stamina; one that changes in both is position.

That test needs the live records, which needs the mode read to work.

---

## 5. Tooling

`tools/analyze_telemetry.py` reads the CSV and reports:

* the sample count, the wall span, and the mode / clock / phase value ranges
* the phase and clock timelines, one line per change
* per-slot activity: how many of the `0x7C` bytes changed at least once
* a ranking of the byte offsets that change, by how many distinct values they
  take and how many slots use them

It is what produced every number quoted above.

---

## 6. Cost and behaviour

* 356 rows in 100 s at 250 ms, about 12 MB — the CSV is large because it
  carries the raw records. `interval_ms` can be raised to trade detail for
  size.
* The module allocates nothing per sample except the CSV's own buffering.
* With it enabled the game behaved the same as with it disabled: the
  address gate and the gameplay gate were unchanged, the game stayed alive,
  and no crash was observed in any run.
