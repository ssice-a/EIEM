"""Diff two runs' event sequences with pointers normalised away.

Absolute addresses change every launch, so comparing logs by pointer value is
useless. This replaces every 16-hex-digit pointer with a stable token (its first
occurrence order) and every thread id / tick with a placeholder, then reports
where two runs first diverge and which event kinds differ in count.

Used to find what actually changed between a run where a mesh lay on the ground
and a run where it did not.
"""
from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

HEX16 = re.compile(r"\b[0-9A-F]{16}\b")
TIMING = re.compile(r"\b(?:tick|tid|generation)=\d+")
# Address-shaped hex, with or without a `key=` in front: method-pointer lines
# such as `get_localRotation: 000000007B0AD840` have no key at all.
HEX12 = re.compile(r"\b[0-9A-F]{12,16}\b")


def normalise(line: str, tokens: dict[str, str]) -> str:
    def swap(match: re.Match) -> str:
        value = match.group(0)
        # A null or all-zero pointer is a meaningful constant, not an identity.
        if value.strip("0") == "":
            return "<null>"
        if value not in tokens:
            tokens[value] = f"<p{len(tokens)}>"
        return tokens[value]

    text = HEX12.sub(swap, line)
    return TIMING.sub("", text)


def shape(line: str) -> str:
    """The event kind: values and addresses replaced, so two runs compare."""
    text = re.sub(r"=\S+", "=", line)
    text = HEX12.sub("<addr>", text)
    return text


def load(path: Path):
    tokens: dict[str, str] = {}
    events: list[tuple[str, str]] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        events.append((shape(stripped), normalise(stripped, tokens)))
    return events


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("a")
    parser.add_argument("b")
    parser.add_argument("--context", type=int, default=6)
    args = parser.parse_args()

    events_a = load(Path(args.a))
    events_b = load(Path(args.b))
    print(f"A: {len(events_a)} events   B: {len(events_b)} events")
    print()

    shapes_a = Counter(kind for kind, _ in events_a)
    shapes_b = Counter(kind for kind, _ in events_b)
    keys = sorted(set(shapes_a) | set(shapes_b), key=lambda k: -(shapes_a[k] + shapes_b[k]))
    print(f"{'event shape':<70} {'A':>7} {'B':>7} {'diff':>7}")
    for key in keys[:40]:
        if shapes_a[key] == shapes_b[key]:
            continue
        print(f"{key[:68]:<70} {shapes_a[key]:>7} {shapes_b[key]:>7} "
              f"{shapes_b[key] - shapes_a[key]:>+7}")
    print()

    # First structural divergence: compare only lines that both runs have a
    # recognisable sequence for, skipping pure counts.
    limit = min(len(events_a), len(events_b))
    for index in range(limit):
        if events_a[index][0] != events_b[index][0]:
            print(f"first divergence at event {index}:")
            print("  A line kind:", events_a[index][0][:100])
            print("  B line kind:", events_b[index][0][:100])
            lo = max(0, index - args.context)
            for offset in range(lo, min(index + args.context, limit)):
                mark = ">>" if offset == index else "  "
                print(f"  {mark} [{offset}] A: {events_a[offset][1][:110]}")
                print(f"  {mark} [{offset}] B: {events_b[offset][1][:110]}")
            break
    else:
        shared = min(len(events_a), len(events_b))
        print(f"no divergence in the first {shared} events; "
              f"runs differ only after that")
        for offset in range(max(0, shared - args.context), shared):
            print(f"  [{offset}] A: {events_a[offset][1][:110]}")
            print(f"  [{offset}] B: {events_b[offset][1][:110]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
