#!/usr/bin/env python3
"""
Analyse the remaster's match telemetry CSV.

The CSV has one row per sample:

    wall_ms, mode, clock, phase, <124 hex bytes> x 32 (side 0),
                                  <124 hex bytes> x 32 (side 1)

plus "squad" rows and "# probe" comment lines.

This tool reports what changes and what does not, so that the interesting
fields inside the 0x7C byte player records can be picked out. Fields that
never change are identity-ish (name, shirt number, position). Fields that
change every sample are live state (position, stamina, possession). Fields
that change slowly and monotonically are likely stamina or a timer.

Nothing here claims to know what a byte means. The output is a ranked list of
candidates; a field is only identified once it has been checked against
something observable in the game.

Usage:
    python analyze_telemetry.py <telemetry.csv> [--top 20]
"""
import argparse
import collections
from pathlib import Path

RECORD_SIZE = 0x7C
SLOTS = 32
HEADER = 4          # wall_ms, mode, clock, phase


def load(path):
    rows = []
    probes = []
    squad = None
    for line in Path(path).read_text(errors="ignore").splitlines():
        if not line or line.startswith("#"):
            if line.startswith("# probe"):
                probes.append(line)
            continue
        if line.startswith("squad"):
            squad = [int(x) for x in line.split(",")[1:]]
            continue
        parts = line.split(",")
        if len(parts) < HEADER + 2:
            continue
        wall = int(parts[0])
        mode = int(parts[1])
        clock = float(parts[2])
        phase = int(parts[3])
        recs = []
        for s in range(2):
            base = HEADER + s * SLOTS * RECORD_SIZE
            chunk = parts[base:base + SLOTS * RECORD_SIZE]
            if len(chunk) != SLOTS * RECORD_SIZE:
                continue
            recs.append([bytes.fromhex(x) for x in chunk])
        if len(recs) == 2:
            rows.append((wall, mode, clock, phase, recs))
    return rows, probes, squad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv")
    ap.add_argument("--top", type=int, default=20)
    args = ap.parse_args()

    rows, probes, squad = load(args.csv)
    if not rows:
        print("no data rows in %s" % args.csv)
        return
    print("file        : %s" % args.csv)
    print("samples     : %d" % len(rows))
    span = (rows[-1][0] - rows[0][0]) / 1000.0
    print("wall span   : %.1f s" % span)
    print("mode values : %s" % sorted({r[1] for r in rows}))
    clocks = [r[2] for r in rows]
    phases = [r[3] for r in rows]
    print("clock float : min %.4f max %.4f distinct %d" % (min(clocks), max(clocks), len(set(clocks))))
    print("phase value : min %d max %d distinct %d" % (min(phases), max(phases), len(set(phases))))
    if squad:
        print("squad ids   : %s" % " ".join(str(x) for x in squad))

    print("\nphase over time (one line per phase change):")
    last = None
    t0 = rows[0][0]
    for wall, mode, clock, phase, _ in rows:
        if phase != last:
            print("   t+%7.1f s   phase=%-4d clock=%.4f" % ((wall - t0) / 1000.0, phase, clock))
            last = phase

    print("\nclock float over time (first 12 changes):")
    last = None
    shown = 0
    for wall, mode, clock, phase, _ in rows:
        if clock != last and shown < 12:
            print("   t+%7.1f s   clock=%.4f phase=%d" % ((wall - t0) / 1000.0, clock, phase))
            last = clock
            shown += 1

    # ---- per-slot change analysis -------------------------------------
    print("\nper-slot activity (how many of the %d bytes changed at least once):" % RECORD_SIZE)
    for side in (0, 1):
        for slot in range(SLOTS):
            recs = [r[4][side][slot] for r in rows]
            if all(x == recs[0] for x in recs):
                continue
            changed = sum(1 for i in range(RECORD_SIZE) if len({r[i] for r in recs}) > 1)
            print("   side %d slot %2d : %3d of %d bytes changed" % (side, slot, changed, RECORD_SIZE))

    # ---- byte-level ranking -------------------------------------------
    print("\nbyte offsets that change, ranked by how often:")
    stats = collections.defaultdict(lambda: [0, set(), 0])
    for side in (0, 1):
        for slot in range(SLOTS):
            recs = [r[4][side][slot] for r in rows]
            if len({r for r in recs}) < 2:
                continue
            for i in range(RECORD_SIZE):
                vals = {r[i] for r in recs}
                if len(vals) < 2:
                    continue
                key = (side, i)
                stats[key][0] += 1
                stats[key][1].update(vals)
                stats[key][2] += 1
    ranked = sorted(stats.items(), key=lambda kv: (-kv[1][0], -kv[1][2]))
    print("   %-6s %-6s %-10s %s" % ("side", "offset", "slots", "distinct values"))
    for (side, off), (slots, vals, _) in ranked[:args.top]:
        print("   %-6d 0x%02X   %-10d %d" % (side, off, slots, len(vals)))

    print("\nthe same, ignoring which slot: byte offset -> number of slot-instances that change")
    byoff = collections.defaultdict(set)
    totals = collections.Counter()
    for (side, off), (slots, vals, _) in stats.items():
        byoff[off].add(side)
        totals[off] += slots
    for off in sorted(byoff, key=lambda o: -totals[o])[:args.top]:
        print("   0x%02X : %d slot-instances change" % (off, totals[off]))


if __name__ == "__main__":
    main()
