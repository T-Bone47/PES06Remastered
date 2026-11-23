# Next phase plan

**Date:** 2026-10-20
**Scope decided by:** the research in `docs/research/` — no implementation was
started in this session.

---

## 1. Where we actually stand

| Goal (from the original brief) | State |
|---|---|
| **Modern visuals** — native resolution | ✅ done, 1536×864 |
| **Modern visuals** — high-resolution textures | ✅ done and verified; `texture_scale = 2` ships |
| **Asset replacement** | ✅ works; packs are keyed by CRC32, **not yet authorable by name** |
| **Deeper football** | 🔴 barely started — the upstream layer is a team/kit/league/UI layer and touches no match logic |
| **Faithful restoration** | 🔴 option files (`.opd`) never decoded |

---

## 2. Recommendation

**Build Milestone 1 next: option-file tooling + read-only match telemetry.**

The reasoning, in order of weight:

1. **It is the largest gap against the stated goal.** "Deeper football" is the
   one headline objective with almost nothing behind it. Everything else is
   done and verified.
2. **Both halves are low risk.** The option-file tool needs **no change to
   the ASI at all** — the rolling XOR is already written down in
   kitserver's `filedec.cpp`. The telemetry hooks are **read-only**, so a
   mistake cannot corrupt a match.
3. **It buys the RE foundation for everything after it.** The AI is not
   located. Telemetry is how we find it: once the clock, the phase and the
   player records are readable, the AI's decisions become observable instead
   of guessed at.
4. **It has value even if nothing else ever happens.** A master-league and
   team editor that works on the real file format is worth more to a PES 6
   player than any single AI tweak.

**What I would *not* do first:** the AI. We have the AI's inputs but not its
decision function, so "tuning AI parameters" today means guessing which bytes
are parameters. That is a research task wearing an implementation task's
clothes.

---

## 3. Milestones

### Milestone 1 — Option files + match telemetry

**A. Option-file (`.opd`) tooling** — no ASI change.

Decrypt, edit and re-encrypt the master-league / team / player option files.
The algorithm is already known: rolling XOR
(`param = 0x573; buf[i] ^= param & 0xff; param = param * 0x6C078965 + 1`) with
a 446-byte key table and `DECRYPT_FACTOR2 = 0x7AB3684C`.

**Acceptance tests (runnable):**

1. Take a pristine `.opd` from the game folder, decrypt it with the new tool,
   and confirm the output contains recognisable option-file structures and
   **not** the encrypted bytes.
2. Re-encrypt the untouched file and confirm it is **byte-identical** to the
   original. (This is the test that proves the algorithm, and it must pass
   before anything is edited.)
3. Change one value (a team name's length byte or a formation slot), decrypt
   again, and confirm the change is present.
4. Load the edited file in the game and confirm the game reads it without
   error.

**Risks:** none to the existing build — this is a standalone tool under
`tools/`. The only risk is a wrong key table, which test 2 catches
immediately.

**B. Read-only match telemetry** — ASI change, read-only.

Log the match clock, the phase value, the two squads and a few per-player
record fields to a file.

Hook points (all cross-checked this session — see
`docs/research/deeper_football.md` §2):

| What | Address |
|---|---|
| player record accessor (0x7C-byte records, per side) | `FUN_00861B90` → `0x00861B90` |
| squad table (2×32 ids) | `DAT_03A8AA78` |
| mode/side discriminator | `FUN_00425198` → `0x00425198` |
| clock | `DAT_03A6A260` |
| phase / central state | `DAT_03BECC60` |

**Acceptance tests (runnable):**

1. Start a match and confirm the log shows a clock that advances monotonically
   over at least 5 minutes of match time.
2. Confirm the logged squads contain 11 player ids per side and that they
   match the teams shown on screen.
3. Confirm per-player values change during play (a player who is running
   must not have identical numbers throughout).
4. Confirm the game plays normally with the hook installed: no crash, no
   frame-rate drop beyond noise, and the match is playable to the end.

**Risks:** low. Read-only. The manifest gate skips the layer on a prologue
mismatch. If a global turns out to be written by the game (rather than
read-only), the hook must be a read, not a write.

**Cost:** ~1 week.

---

### Milestone 2 — Packs authorable by name

Turn the working CRC-keyed pack into one people can write.

**A. Build `names.tsv`** from kitserver's `afsmap.txt` (289 named
entries/ranges, already parsed by a working script) plus our own
measurements. Pure data work — **S**.

**B. The RE tasks that make valid replacements possible** — from
`docs/research/mapping_guide.md` §5:

| Task | What | Effort | Risk |
|---|---|---|---|
| R5 | Locate the game's asset-header parser; confirm which fields it reads | M | low |
| R1 | Locate the asset→texture conversion; confirm the format chain | M | low |
| R2 | Classify each entry's native pixel format (8/16/32-bit, header size) | M | low |
| R4 | *(optional)* the exact asset→texture association | L | medium |

**C. Name resolution in the pack loader** — the pack may reference an entry
by index **or** by name; names resolve through `names.tsv`; **an unknown name
falls back to the index form, and an unusable pack degrades to "no
replacement"**, never to a crash.

**Acceptance tests (runnable):**

1. `make_pack.py` resolves `"512x512"` and `"id_05318.bin"` to the same
   entry, and a name from `names.tsv` resolves to the documented index.
2. A pack naming a texture that does not exist is skipped and logged, and
   the game plays normally.
3. A replacement with the wrong byte length is rejected and logged, and the
   game plays normally.
4. A pack with a valid replacement still changes the picture on screen
   (re-run the baseline substitution from `docs/BASELINE.md`).

**Risks:** low, provided the fallback rules hold. The dangerous failure mode
is a replacement that corrupts a header the game parses — which is exactly
what R5 exists to prevent, and what acceptance test 3 guards.

**Cost:** ~1 week.

---

### Milestone 3 — Animation override, then the AI

**3a. Animation selection override.** Hook the actor API
(`FUN_00950B40` acquire / `FUN_009509B0` configure / `FUN_009502B0` release)
or the id dispatcher `FUN_00801C00`. Whitelist of known-good ids only;
disabled by default.

*Acceptance:* in a match, force one animation id for the ball and confirm
(a) no crash, (b) a screenshot shows the expected pose, (c) the same
configuration is a no-op when disabled.

**3b. AI parameters.** Blocked until the decision routine is located. The
lead is the `0x006xxxxx`–`0x007xxxxx` band (505 bone-accessor callers — the
per-frame simulation).

*Acceptance:* change one parameter; observe the AI's behaviour change in a
fixed situation; the change is reverted by restoring the default.

**Risks:** medium for animation (wrong ids → broken poses), high for the AI
(deep coupling). Both ship behind config switches that default to off, and
both must have a "no-op when disabled" test.

**Cost:** 3a ~3–5 days; 3b ~1 week of RE before any implementation, plus
unknown implementation cost.

---

## 4. Known risks across the plan

| Risk | Where | Mitigation |
|---|---|---|
| A hook target differs in another game build | all ASI work | the manifest gate already skips the layer on a prologue mismatch — never crashes |
| A "read-only" global turns out to be written by us | telemetry | the hook must never write; assert in review |
| A replacement corrupts a header the game parses | packs | R5 first; wrong-length replacements rejected; the game's own header is preserved byte-for-byte by the pack writer |
| The AI work expands without bound | Milestone 3b | time-box the RE to one week; if the decision function is not found, report that and re-plan rather than continue |
| The upstream gameplay layer's assumptions break | anything touching squads | the upstream layer is verified by the 101-hook gate on every boot |

---

## 5. Open questions for you

1. **Priority.** Do you want the next stretch spent on *deeper football*
   (option files + telemetry + animation + AI) or on *finishing the asset
   side for end users* (Milestone 2)? They are independent and could run in
   parallel if you prefer breadth.
2. **Scope of "deeper football".** Which matters most to you?
   (a) managing teams/leagues/players through the real option files,
   (b) seeing and understanding the match (telemetry),
   (c) changing how the game plays (AI), (d) changing how it looks while
   playing (animation)?
3. **The AI is not located.** Do you want me to time-box a dedicated
   research pass to find it, or to treat it as a later milestone and spend
   the effort on 3a (animation) instead?
4. **Pack naming.** Should the pack keep supporting raw CRC keys, or should
   name/index be the only supported form? Keeping both is safer for existing
   packs; name-only is cleaner.
5. **Risk appetite.** Are read-only hooks in the match engine acceptable at
   all? If not, the option-file route is the only low-risk way into
   "deeper football".
6. **Fullscreen.** Still unimplemented (the adapter refuses modes above
   800×600). Is fullscreen worth a milestone, given windowed at 1536×864
   already looks right?

---

## 6. What this session changed in the existing work

Two corrections to earlier conclusions, both established by direct
measurement:

1. **The `0x20050420` "Rich magic" is not a magic.** The game never compares
   against it; the 4-byte sequence occurs once in the whole binary, inside a
   character lookup table. The game parses asset headers *structurally*.
   `STATUS.md` and `docs/ASSETS.md` need updating.
2. **The `caller=` field in the D3D8 log is not valid attribution.**
   `_ReturnAddress()` under `/O2` returned `0x0040C84B`, which lands
   mid-instruction inside `mov dword ptr [edi+0x144C], 0x0D`. The two
   "caller" values quoted in `STATUS.md` should not be relied on.

Neither breaks anything that is shipped; both affect how the texture path is
understood.
