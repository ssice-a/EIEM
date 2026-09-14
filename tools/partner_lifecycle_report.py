"""Per Partner Renderer: how many times each lifecycle step actually ran.

One Partner Renderer is named by several log lines, and the same Renderer is
re-reported whenever a later boundary touches it. Counting lines therefore
over-counts renderers while under-counting steps. This keys on the Partner
pointer itself and tallies each step, so a Renderer that exists but was never
posed, or was posed only once before creation finished, stands out.

Read-only.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path


FIELD = re.compile(r"(\w+)=([^\s]+)")


def fields(line: str) -> dict[str, str]:
    return {key: value for key, value in FIELD.findall(line)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log")
    parser.add_argument("--top", type=int, default=0,
                        help="list this many renderers by pose count ascending")
    args = parser.parse_args()

    lines = Path(args.log).read_text(encoding="utf-8", errors="replace").splitlines()

    created: set[tuple[str, str]] = set()
    steps: dict[tuple[str, str], Counter] = defaultdict(Counter)
    lod_groups: dict[tuple[str, str], set[str]] = defaultdict(set)
    sources: Counter = Counter()

    for line in lines:
        if "partner=" not in line:
            continue
        values = fields(line)
        source = values.get("source", "")
        partner = values.get("partner", "")
        if not source or not partner:
            continue
        if source.strip("0") == "" or partner.strip("0") == "":
            continue
        key = (source, partner)
        sources[source] += 1

        if "boundary=partner-created" in line:
            created.add(key)
            steps[key]["created"] += 1
        event = values.get("event", "")
        if event:
            steps[key][event] += 1
        if event == "lod-group-member":
            lod_groups[key].add(values.get("group", ""))

    print(f"distinct Partner Renderers : {len(created)}")
    print(f"distinct sources           : {len(sources)}")
    print()

    step_names = ["created", "partner", "partner-pose", "partner-skin-map",
                  "partner-bone-diff", "partner-skin-private-detail"]
    print(f"{'source':>12} {'partner':>10} " + " ".join(f"{n[:11]:>11}" for n in step_names)
          + f" {'lodGroups':>10}")
    for key in sorted(created, key=lambda k: (k[0], k[1])):
        row = " ".join(f"{steps[key][n]:>11}" for n in step_names)
        groups = ",".join(sorted(lod_groups.get(key, set())))
        real = sum(1 for g in lod_groups.get(key, set()) if g.strip("0"))
        print(f"{key[0][-6:]:>12} {key[1][-6:]:>10} {row} {real:>10}")

    print()
    never_posed = [k for k in created if not steps[k]["partner-pose"]]
    print(f"created but never posed  : {len(never_posed)}")
    for key in never_posed[:15]:
        print(f"  {key[0][-8:]}/{key[1][-8:]}")
    no_real_group = [k for k in created
                     if not any(g.strip("0") for g in lod_groups.get(k, set()))]
    print(f"never in a real LODGroup : {len(no_real_group)}")
    for key in no_real_group[:15]:
        groups = ",".join(sorted(lod_groups.get(key, set())))
        print(f"  {key[0][-8:]}/{key[1][-8:]}  groups={groups or '<none>'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
