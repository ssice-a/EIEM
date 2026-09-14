"""Compare how each Partner was registered into the game's skin arrays across runs.

The reported defect is non-deterministic, so the useful question is not what one
run looks like but what differs between a run where a mesh lies and a run where
it does not. This prints, per Partner section, the arrays and indices it was
seen in, the source/partner flags, and the mesh pointer -- side by side for two
logs.

Read-only.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

FIELD = re.compile(r"(\w+)=(\[[^\]]*\]|[^\s]+)")


def fields(line: str) -> dict[str, str]:
    return {key: value for key, value in FIELD.findall(line)}


def collect(path: Path):
    per_partner: dict[str, list[tuple[str, str, int, str, str]]] = defaultdict(list)
    per_array: dict[str, set[str]] = defaultdict(set)
    gaps = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "event=skin-array-item" in line and "partnerKnown=1" in line:
            values = fields(line)
            name = values.get("name", "")
            match = re.search(r"(Render\S+Part\d+)", name)
            section = match.group(1) if match else name
            per_partner[section].append((
                values.get("boundary", "?"),
                values.get("array", "?"),
                int(values.get("index", "-1")),
                values.get("mesh", "?"),
                values.get("lod", "?"),
            ))
            per_array[values.get("array", "?")].add(section)
        if "MOD-REG-GAP" in line:
            gaps.append(fields(line))
    return per_partner, per_array, gaps


def summarize(label: str, per_partner, per_array, gaps):
    print(f"===== {label} =====")
    print(f"partners seen in arrays : {len(per_partner)}")
    print(f"distinct arrays         : {len(per_array)}")
    for array, sections in sorted(per_array.items(), key=lambda kv: -len(kv[1])):
        print(f"  array {array}  count={len(sections)}")
        families = defaultdict(int)
        for section in sections:
            family = re.sub(r"Part\d+$", "", section)
            families[family] += 1
        for family, count in sorted(families.items()):
            print(f"      {count:>3}  {family}")
    print()
    print("per-partner registration:")
    for section in sorted(per_partner, key=lambda s: (re.sub(r'Part\d+$', '', s), s)):
        entries = per_partner[section]
        arrays = sorted({entry[1] for entry in entries})
        indices = sorted({entry[2] for entry in entries})
        meshes = sorted({entry[3] for entry in entries})
        boundaries = sorted({entry[0] for entry in entries})
        print(f"  {section}")
        print(f"      count={len(entries)} arrays={arrays} indices={indices}")
        print(f"      meshes={meshes} boundaries={boundaries}")
    print()
    print(f"registration gaps: {len(gaps)}")
    stages = defaultdict(int)
    absent = defaultdict(int)
    for gap in gaps:
        stages[gap.get("stage", "?")] += 1
        absent[gap.get("stage", "?")] += int(gap.get("sourceAbsent", "0"))
    for stage in sorted(stages):
        print(f"  {stage}: events={stages[stage]} sourceAbsentTotal={absent[stage]}")
    print()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("clean")
    parser.add_argument("lying")
    args = parser.parse_args()

    clean = collect(Path(args.clean))
    lying = collect(Path(args.lying))
    summarize("CLEAN run", *clean)
    summarize("LYING run", *lying)

    only_clean = set(clean[0]) - set(lying[0])
    only_lying = set(lying[0]) - set(clean[0])
    if only_clean:
        print("partners registered only in the CLEAN run:")
        for section in sorted(only_clean):
            print(f"  {section}")
    if only_lying:
        print("partners registered only in the LYING run:")
        for section in sorted(only_lying):
            print(f"  {section}")
    if not only_clean and not only_lying:
        print("the same Partner sections registered in both runs; the difference "
              "is not which Partners reached the skin arrays")

    print()
    print("arrays present in one run only:")
    clean_arrays = {name for name in clean[1]}
    lying_arrays = {name for name in lying[1]}
    for name in sorted(clean_arrays - lying_arrays):
        print(f"  clean only: {name}")
    for name in sorted(lying_arrays - clean_arrays):
        print(f"  lying only: {name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
