"""Tally skin-probe measurements, grouped by verdict and by renderer side."""
from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

FIELD = re.compile(r"(\w+)=(\[[^\]]*\]|[^\s]+)")


def fields(line: str) -> dict[str, str]:
    return {key: value for key, value in FIELD.findall(line)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log")
    parser.add_argument("--show-non-ok", type=int, default=0)
    args = parser.parse_args()

    lines = Path(args.log).read_text(encoding="utf-8", errors="replace").splitlines()
    verdicts: Counter = Counter()
    by_phase: Counter = Counter()
    reasons: Counter = Counter()
    rows: list[tuple[str, str, str]] = []

    for line in lines:
        if "[SKIN-PROBE]" not in line:
            continue
        values = fields(line)
        measured = values.get("measured", "?")
        if measured != "1":
            reasons[values.get("reason", "<none>")] += 1
            phase = "unmeasured"
            verdict = "unmeasured"
        else:
            verdict = values.get("verdict", "<missing>")
            # `phase=create source=1` names the side in a trailing field.
            phase = "setBones" if "phase=setBones" in line else "create"
            if "source=1" in line:
                phase += "/source"
            elif "partner=1" in line:
                phase += "/partner"
            else:
                phase += "/" + ("partner" if values.get("partner") == "1" else "source")
        verdicts[verdict] += 1
        by_phase[phase] += 1
        rows.append((phase, verdict, values.get("name", "<unnamed>")))

    print(f"total measurements: {sum(verdicts.values())}")
    print()
    print("verdicts:")
    for verdict, count in verdicts.most_common():
        print(f"  {count:>5}  {verdict}")
    print()
    print("by phase:")
    for phase, count in by_phase.most_common():
        print(f"  {count:>5}  {phase}")
    if reasons:
        print()
        print("unmeasured reasons:")
        for reason, count in reasons.most_common():
            print(f"  {count:>5}  {reason}")

    if args.show_non_ok:
        print()
        print("non-ok measurements:")
        shown = 0
        for phase, verdict, name in rows:
            if verdict == "ok":
                continue
            print(f"  {phase:<18} {verdict:<16} {name}")
            shown += 1
            if shown >= args.show_non_ok:
                break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
