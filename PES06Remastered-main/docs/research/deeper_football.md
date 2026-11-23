# Research B — deeper football: what the game actually exposes

**Date:** 2026-10-20
**Question:** what can we see and change in the match engine, the AI and the
animation system, and where is each thing located?

Labels used throughout: **FACT** = observed in our binary or logs;
**SOURCE-CLAIM** = from a community document; **HYPOTHESIS** = inference.

---

## 1. What the upstream gameplay layer already does

`remaster/src/hooks/club_hooks_register.cpp` installs 33 hooks. Grouped:

| Area | Addresses | What it changes |
|---|---|---|
| Club / team slot loading | `0x009ED730`, `0x009E25A0`, `0x009EE940` | how many teams a league has, which teams appear |
| League selection panel | `0x00B07B00`, `0x00B09050`, `0x00B083B0`, `0x00B0AC40`, `0x00B0AC50` | panel slot count and navigation bounds |
| Custom crests / badges | `0x00B3BEB0`, `0x00B0F990`, `0x00B0FA80`, `0x00B0F8B0`, `0x009B0390` | `teams/<ID>.png` for custom and stock teams |
| Kit data and colour extraction | `0x00968700`, `0x00968960`, `0x00969610`, `0x00968E70`, `0x00865240`, `0x00865380`, `0x00865430`, `0x008654D0` | kit loading, NULL-safety, moddable colours |
| Team / player identity | `0x00862710`, `0x00861B90`, `0x00862C10`, `0x00862200`, `0x00861AD0`, `0x00969790` | squad-slot player id and attribute read/write |
| Panels, screens, bone position | `0x00B08B40`, `0x00B0FCD0`, `0x00950490`, `0x00950580`, `0x00950A00` | panel refresh, crash fixes, bone world position |

**FACT: the upstream layer touches none of the match engine, the AI, the ball
physics, the referee, the crowd, the camera or the animation selection.** It
is a team/kit/league/UI layer. All of those addresses sit in `0x008xxxxx`,
`0x009xxxxx` and `0x00Bxxxxx`; the D3D8 and texture code sits in
`0x004xxxxx`.

**FACT: the upstream layer contains no match-time logic** — there is no
reference to a match clock, a score, a half, or an AI decision in its source.

---

## 2. The match state — located

### 2.1 The player record table — FACT

`FUN_00861B90` (20 callers) is the **per-match player record accessor**:

```c
int FUN_00861b90(void) {
    if (in_CX < 5000) {                       // slot index
        iVar1 = (in_EAX == 1) ? FUN_00866cb0() : FUN_00866b90();   // side
        if (iVar1 != 0) return iVar1 + (uint)in_CX * 0x7C;         // record
    } else if ((in_CX < 6000) || (0x18f6 < in_CX)) {
        if (0x6fff < in_CX && in_CX < 0x7020) return FUN_006b1b30();
        ...
```

**=> A player record is `0x7C` = 124 bytes.**
**=> The table base is per side: `FUN_00866B90` (side 0) / `FUN_00866CB0` (side 1).**
**=> The index is a *slot* number, not a player id.**
**=> Index bands exist: `< 0x1388` (5000) = player slots, `0x7000..0x7020`
= another entity class, `0x7020..0x7040` = another.**

### 2.2 The match entity accessor — FACT

`FUN_00862180` (23 callers) returns a pointer to a match entity:

```c
iVar1 = FUN_0067ce60(param_1);          // entity lookup by index
if (iVar1 != 0) {
    iVar1 = thunk_FUN_00425198();       // mode discriminator: 1 or 2
    if (iVar1 == 2) return FUN_0067f490(param_1);
    if (iVar1 == 1) return FUN_006aeef0(param_1);
}
```

**=> `FUN_00425198()` is a mode/side discriminator returning 1 or 2, and it
selects between two different entity pools.** This is a match-type or
match-state switch.

### 2.3 The squad table — FACT

`FUN_00A4AFA0` (5 callers) writes the match squad:

```c
param_2 &= 0x8000001f;                                    // slot 0..31
(&DAT_03a8aa78)[(param_1 & 1) * 0x20 + param_2] = param_3;   // team, slot, player id
```

**=> `DAT_03A8AA78` is a 2 teams × 32 slots table of player ids** — the match
roster. Written at match setup by `FUN_009A1220`:

```c
uVar3 = FUN_009942c0();
FUN_009a1050();
for (iVar4 = 0; ...) for (iVar6 = 0; iVar6 < 0x20; iVar6++) {
    uVar2 = FUN_00862710(uVar3, *(uint*)(&DAT_03a6a0a0 + iVar6*4), 0);
    FUN_00a4afa0(iVar4, iVar6, uVar2);
}
```

FACT: the table is referenced from only three places in `.text`
(`0x00A4AFC6`, `0x00A4B024`, `0x00A4B132`), all in the writer's
neighbourhood — **so the rest of the game reaches it through a pointer, not
through a direct reference.** (This is why a naive CRC/xref hunt for it is
unfruitful.)

### 2.4 Match phase and clock — FACT

| Global | References | What it is |
|---|---|---|
| `DAT_03BECC60` | **174**, spread across `0x004xxxxx`–`0x007xxxxx` | a heavily used state value; `FUN_00801C00` tests `== 6` |
| `DAT_03A6A260` | 17, **all** in `0x009A1B37`–`0x009A2851` | a float clamped to `0..0x23` — a clock-like value |
| `DAT_03A6A128` | 21, same `0x009Axxxx` cluster | an array read alongside the clock |

**=> The `0x009A1xxx`–`0x009A2xxx` region is the match tick / clock
subsystem.** `FUN_009A2550` reads the clock, calls `FUN_009ACC90`,
`FUN_009A6AB0(&DAT_03A6A19C)` and `FUN_0041ACF0()`.

**=> `DAT_03BECC60` with 174 references across every subsystem is the game's
central "what am I doing" state value.** That is the best single candidate
for "which phase of the match are we in".

### 2.5 The AI — NOT LOCATED

**UNVERIFIED.** We have the *inputs* an AI would read:

* `FUN_00862C10` (`GetTeamPlayerAttr`) — 19 callers, of which a cluster sits
  in `0x009Axxxx` (`0x00985750`, `0x009A1220`, `0x009A1630`, `0x009A2550`,
  `0x009A2850`).
* `FUN_00861B90` — the 0x7C-byte per-match player records (speed, accuracy,
  stamina and the rest would live in there).

**But no function has been positively identified as a decision routine.**
None of the candidates examined (the squad initialiser `0x009A1220`, the tick
`0x009A2550`, the id dispatcher `0x00801C00`) is recognisably AI. The
strings table is useless here — the binary is stripped and 236,988 "strings"
are mostly noise (`cccccccc` repeats).

**HYPOTHESIS:** the AI lives in the `0x006xxxxx`–`0x007xxxxx` band, the
same band as the 505 `GetBoneWorldPosition` callers — i.e. the per-frame
simulation. **This is untested.**

---

## 3. The animation / rendering layer — located

### 3.1 The bone accessor — FACT

`FUN_00950490` (505 callers):

```c
float * FUN_00950490(int param_1, float *param_2, short param_3) {
    iVar2 = *(int *)(param_1 + 0x1c);   // actor -> skeleton
    FUN_00955320(iVar2 + 4);
    iVar3 = *(int *)(iVar2 + 0x14);
    ...
```

**=> 505 call sites** — this is the busiest single function in the match
rendering path, and the upstream gameplay layer already hooks it
(`hook_GetBoneWorldPosition`, `0x00950490`) to fix a NULL crash.

### 3.2 The actor bone query API — FACT

`FUN_006BEA30` (80 callers) is a thin wrapper around the bone accessor:

```c
iVar1 = FUN_00950b40(param_1);          // acquire actor
if (!iVar1) return 0;
FUN_009509b0(iVar1, param_2);            // set something on the actor
iVar2 = FUN_00950490(iVar1, param_4, param_3);
if (!iVar2) { FUN_009502b0(iVar1); return 0; }   // release actor
FUN_009502b0(iVar1);
return param_4;
```

**=> `FUN_00950B40` / `FUN_009509B0` / `FUN_009502B0` are an
acquire / configure / release actor API, and `FUN_006BEA30` is the public
"give me this bone's world position" entry point.**

### 3.3 An id dispatcher — HYPOTHESIS

`FUN_00801C00` (13 callers) branches on an id and then calls
`FUN_00862C10`:

```c
if ((((param_1 < 0x40) || (0xcb < param_1)) && (param_1 != 0x126)) &&
    ((param_1 < 0xdd) || (0xfd < param_1))) { FUN_00862c10(); return; }
if (DAT_03becc60 == 6) { FUN_00862c10(); return; }
FUN_00862c10();
```

The id bands (`0x40`–`0xCB`, `0xDD`–`0xFD`, `0x126`) look like an **animation
or state id space**, and the `DAT_03BECC60 == 6` test looks like a
**"which phase are we in"** test. **HYPOTHESIS, not established.**

---

## 4. Summary table

| Thing | Address | Confidence | Notes |
|---|---|---|---|
| Player record table | `FUN_00861B90` | **HIGH** | 0x7C-byte records, per-side base |
| Player record base, side 0 / side 1 | `FUN_00866B90` / `FUN_00866CB0` | **HIGH** | |
| Match entity accessor | `FUN_00862180` | **HIGH** | mode dispatch via `FUN_00425198` |
| Mode / side discriminator | `FUN_00425198` | **HIGH** | returns 1 or 2 |
| Entity lookup | `FUN_0067CE60` | MEDIUM | |
| Match squad table (2×32 ids) | `DAT_03A8AA78` | **HIGH** | writer `FUN_00A4AFA0` |
| Squad initialiser | `FUN_009A1220` | **HIGH** | fills the squad table at match setup |
| Match phase / central state | `DAT_03BECC60` | MEDIUM | 174 refs, `== 6` tested |
| Match clock | `DAT_03A6A260` | MEDIUM | float 0..0x23, cluster at `0x009A1xxx` |
| Match tick region | `0x009A1B37`–`0x009A2851` | MEDIUM | |
| Bone world position | `FUN_00950490` | **HIGH** | 505 callers |
| Actor bone query API | `FUN_006BEA30` | **HIGH** | 80 callers |
| Actor acquire / configure / release | `FUN_00950B40` / `FUN_009509B0` / `FUN_009502B0` | **HIGH** | |
| Player attribute accessor | `FUN_00862C10` | **HIGH** | AI would read through this |
| Animation / state id dispatcher | `FUN_00801C00` | LOW | hypothesis only |
| **AI decision routine** | — | **NONE** | **not located** |

---

## 5. Three scoped goals, ranked by value and risk

### Goal 1 — Read-only match-state telemetry  *(value: high, risk: low)*

**What:** log the match clock, the phase, the two squads and a few per-player
fields to a file, so we can *see* what the game thinks is happening.

**Hook points:** `FUN_00861B90` (read a player record), `FUN_00425198`
(mode), `FUN_00A4AFA0` (squad table writes) and a periodic read of
`DAT_03A6A260` / `DAT_03BECC60`.

**Why it is first:** it is **read-only** — nothing is written, so a mistake
cannot corrupt the game; it is the cheapest way to learn the layout of the
remaining structures; and it is the prerequisite for every other goal
because it turns "I think this byte is the stamina" into "the stamina goes
from 100 to 63 between minute 60 and 61".

**Verification (runnable):** start a match, let it run 5 minutes, and check
that the log contains a clock that advances, two squads of 11 player ids
that match the teams on screen, and per-player values that change plausibly.

**Risk:** low. Read-only hooks at addresses that are already cross-checked;
the manifest gate skips the layer on a mismatch.

**Cost:** ~1 day.

---

### Goal 2 — Animation selection override  *(value: medium-high, risk: medium)*

**What:** change which animation a player/ball entity plays.

**Hook points:** the id dispatcher `FUN_00801C00` (if the hypothesis holds)
or the actor API `FUN_00950B40`/`FUN_009509B0`, which is where an actor's
animation state is configured.

**Why second:** it is the most *visible* gameplay-adjacent change available,
and the actor API is already mapped with three clean function addresses. But
picking the wrong animation id produces broken poses, and the id space is
not yet understood.

**Verification (runnable):** in a match, force one animation id for the
ball and check (a) the game does not crash, (b) a screenshot shows the
expected pose.

**Risk:** medium — a wrong id may crash or corrupt state. Mitigation: a
whitelist of known-good ids, and a config switch that disables it by default.

**Cost:** ~3–5 days including the id-space work.

---

### Goal 3 — AI parameter tuning  *(value: high, risk: high, currently blocked)*

**What:** change how the AI plays — aggression, reaction time, offside
tolerance, difficulty scaling.

**Hook points:** **not yet known.** The candidates are the consumers of
`FUN_00862C10` and `FUN_00861B90`.

**Why blocked:** we have the AI's *inputs* but not its *decision function*.
Without that, "tuning AI parameters" means guessing which bytes are
parameters.

**What must happen first:** locate the decision routine. **The most
promising route is the `0x006xxxxx`–`0x007xxxxx` band**, where the 505
bone-accessor callers live — that band is clearly the per-frame simulation.
A targeted sweep of that band for functions that read the 0x7C-byte player
records and then write to entity state should find it.

**Verification (runnable):** change one parameter, play a scripted 1-vs-1
scenario (or observe the AI's behaviour in a fixed situation) and confirm the
behaviour changed.

**Risk:** high — the AI is deeply coupled to the simulation.

**Cost:** unknown until the routine is found; budget ~1 week of RE first.

---

## 6. Alternatives that need no hooks at all

Worth putting on the table because they are far cheaper than any of the
above:

| Option | What it needs | Value |
|---|---|---|
| **Option file (`.opd`) editing** | kitserver's `filedec.cpp` already has the rolling XOR (`param=0x573; buf[i]^=param&0xff; param=param*0x6C078965+1`) plus a 446-byte key table for `DECRYPT_FACTOR2=0x7AB3684C` | master league / team / player management in a *file*, no game hooks — big value, low risk |
| **Stadium / weather / ball selection** | the game already loads these from the AFS; our asset pack can swap them by entry index | visual variety, no new code |
| **More upstream gameplay** | the PESMod layer is a known-good base for leagues/kits/squads | incremental |

**The `.opd` route is the single best value-per-risk item in this document.**
It was noted as "not done" in STATUS.md; the algorithm is already written
down in a reference we hold.

---

## 7. Open questions

1. What exactly is `FUN_00425198()`'s 1-vs-2? Match mode (1v1 / 11v11), or
   side?
2. Are the 0x7C-byte player records the same for both sides, and what is at
   each offset?
3. What are the `DAT_03BECC60` values? (174 refs suggests an enum; `== 6` is
   one case.)
4. Is `DAT_03A6A260` seconds, minutes, or a 0..35 match-minute counter?
5. Where is the AI, really?
6. Does `FUN_00801C00` dispatch animations or player states?
