#!/usr/bin/env python3
"""
Differential analysis for the remaster's match telemetry and memory dumps.

Two things it does:

  1. `scan.py diff a.bin b.bin` compares two memory dumps taken by the
     telemetry module at two labelled moments, and reports which addresses
     changed. This is how a live field is found: the address that changes
     when the player moves, and only then, is the position.

  2. `scan.py entities telemetry.csv` analyses the telemetry CSV's live entity
     table (0x48 bytes per entity) and ranks the byte offsets that change, so
     that a candidate can be picked out.

Usage:
    python scan.py diff <a.bin> <b.bin> [--min 4] [--top 40]
    python scan.py diff <a.bin> <b.bin> --csv <out.csv>
    python scan.py entities <telemetry.csv> [--top 30]
"""
import argparse
import collections
import csv
import struct
import sys
from pathlib import Path


# ---------------------------------------------------------------- memory dumps

def read_dump(path):
    """Reads a dump written by the telemetry module.

    The format is a sequence of records: the text line "R <base> <size>"
    followed by that many raw bytes.
    """
    data = Path(path).read_bytes()
    regions = {}
    pos = 0
    while pos < len(data):
        nl = data.find(b"\n", pos)
        if nl < 0:
            break
        line = data[pos:nl].decode("latin1", "replace")
        pos = nl + 1
        if not line.startswith("R "):
            continue
        parts = line.split()
        if len(parts) != 3:
            continue
        base = int(parts[1], 16)
        size = int(parts[2], 16)
        chunk = data[pos:pos + size]
        pos += size
        regions[base] = chunk
    return regions


def diff_dumps(a_path, b_path, min_run=4, top=40, out_csv=None):
    a = read_dump(a_path)
    b = read_dump(b_path)
    print("dump A: %s  %d regions" % (a_path, len(a)))
    print("dump B: %s  %d regions" % (b_path, len(b)))

    runs = []          # (base, size, bytes changed)
    common = set(a) & set(b)
    print("common regions: %d" % len(common))
    total_changed = 0
    total_bytes = 0
    for base in sorted(common):
        ba, bb = a[base], b[base]
        n = min(len(ba), len(bb))
        total_bytes += n
        i = 0
        while i < n:
            if ba[i] != bb[i]:
                j = i
                while j < n and ba[j] != bb[j]:
                    j += 1
                runs.append((base + i, j - i, sum(1 for k in range(i, j) if ba[k] != bb[k])))
                total_changed += sum(1 for k in range(i, j) if ba[k] != bb[k])
                i = j
            else:
                i += 1
    print("bytes compared: %d, bytes changed: %d (%.2f%%)" %
          (total_bytes, total_changed, 100.0 * total_changed / max(1, total_bytes)))
    print("changed runs of at least %d bytes: %d" % (min_run, len([r for r in runs if r[1] >= min_run])))

    runs.sort(key=lambda r: -r[1])
    print("\nlargest changed runs:")
    for base, size, changed in runs[:top]:
        print("   0x%08X  %6d bytes (%d differ)" % (base, size, changed))

    if out_csv:
        with open(out_csv, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["address", "run_bytes", "bytes_changed"])
            for base, size, changed in runs:
                w.writerow(["0x%08X" % base, size, changed])
        print("\nrun list written to %s" % out_csv)
    return runs


# ---------------------------------------------------------------- telemetry

def read_telemetry(path):
    """Returns (header, rows). Each row is (wall_ms, mode, phase, matchtype, clock, entities, globals)."""
    rows = []
    header = None
    for line in Path(path).read_text(errors="ignore").splitlines():
        if line.startswith("#"):
            if header is None and "columns:" in line:
                header = line
            continue
        if not line or line.startswith("squad"):
            continue
        parts = line.split(",")
        if len(parts) < 5:
            continue
        wall = int(parts[0])
        mode = int(parts[1])
        phase = int(parts[2])
        mtype = int(parts[3])
        clock = float(parts[4])
        entities = [bytes.fromhex(x) for x in parts[5:5 + 64 * 0x48]]
        globals_ = [bytes.fromhex(x) for x in parts[-0x400:]]
        rows.append((wall, mode, phase, mtype, clock, entities, globals_))
    return header, rows


def analyse_entities(path, top=30):
    header, rows = read_telemetry(path)
    if not rows:
        print("no telemetry rows in %s" % path)
        return
    print("samples: %d" % len(rows))
    print("span   : %.1f s" % ((rows[-1][0] - rows[0][0]) / 1000.0))
    for name, idx in (("mode", 1), ("phase", 2), ("matchtype", 3)):
        vals = sorted({r[idx] for r in rows})
        print("%-9s: %s" % (name, vals))
    clocks = [r[4] for r in rows]
    print("clock    : min %.4f max %.4f distinct %d" % (min(clocks), max(clocks), len(set(clocks))))

    print("\nphase over time:")
    last = None
    for r in rows:
        if r[2] != last:
            print("   t+%7.1f s  phase=%d mode=%d matchtype=%d clock=%.4f" %
                  ((r[0] - rows[0][0]) / 1000.0, r[2], r[1], r[3], r[4]))
            last = r[2]

    # per-entity activity
    print("\nper-entity activity (how many of the 72 bytes changed):")
    for e in range(64):
        recs = [r[5][e] for r in rows]
        if all(x == recs[0] for x in recs):
            continue
        changed = sum(1 for i in range(0x48) if len({r[i] for r in recs}) > 1)
        print("   entity %2d : %3d of 72 bytes changed" % (e, changed))

    # byte offset ranking across all entities
    stats = collections.defaultdict(lambda: [0, 0])
    for e in range(64):
        recs = [r[5][e] for r in rows]
        if all(x == recs[0] for x in recs):
            continue
        for i in range(0x48):
            vals = {r[i] for r in recs}
            if len(vals) > 1:
                stats[i][0] += 1
                stats[i][1] = len(vals)
    print("\nbyte offsets that change, ranked by how many entities use them:")
    for off, (ents, distinct) in sorted(stats.items(), key=lambda kv: (-kv[1][0], -kv[1][1]))[:top]:
        print("   0x%02X  used by %2d entities, %2d distinct values" % (off, ents, distinct))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["diff", "entities"])
    ap.add_argument("source")
    ap.add_argument("other", nargs="?")
    ap.add_argument("--min", type=int, default=4)
    ap.add_argument("--top", type=int, default=40)
    ap.add_argument("--csv")
    args = ap.parse_args()

    if args.mode == "diff":
        if not args.other:
            ap.error("diff needs two dumps")
        diff_dumps(args.source, args.other, args.min, args.top, args.csv)
    else:
        analyse_entities(args.source, args.top)


if __name__ == "__main__":
    main()
