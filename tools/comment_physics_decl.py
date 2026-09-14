"""Comment out every Physics resource DECLARATION in a mod.ini, in place.

Why a script and not a text edit: `EiemPrepareModPhysics` loads every declared
Physics resource unconditionally, so a stale `[Physics...]` section aborts the
whole file even when no Render rule says `physics=`. Commenting only the
`physics=` lines of the rules is therefore not enough, and commenting only the
section header is actively wrong - the parser keys off `[section]` lines, so the
leftover `path=` would be attached to the previous section.

Detection is structural rather than name-based for the same reason: a resource
is Physics iff its `path=` ends in `.physics`, whatever the rules currently say.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


SECTION_RE = re.compile(r"^\s*\[([^\]]+)\]\s*$")
TAG = "; PHYSICS-OFF "


def EiemUncomment(line: str) -> str:
    """Strip this tool's own tag so a second run still recognises the section."""
    stripped = line.lstrip()
    if stripped.startswith(TAG):
        return stripped[len(TAG) :]
    return stripped


def EiemFindPhysicsSections(lines: list[str]) -> list[tuple[int, int, str]]:
    """Return `(start, end, name)` for every section that declares Physics."""
    headers = [
        (index, match.group(1).strip())
        for index, line in enumerate(lines)
        if (match := SECTION_RE.match(EiemUncomment(line)))
    ]
    found: list[tuple[int, int, str]] = []
    for position, (start, name) in enumerate(headers):
        end = headers[position + 1][0] if position + 1 < len(headers) else len(lines)
        for line in lines[start:end]:
            # Strip this tool's own tag *before* splitting, or a second run sees
            # a key of "; PHYSICS-OFF path" and reports nothing to do.
            bare = EiemUncomment(line)
            if "=" not in bare or bare.startswith((";", "#")):
                continue
            key, _, value = bare.partition("=")
            if key.strip() == "path" and value.strip().lower().endswith(".physics"):
                found.append((start, end, name))
                break
    return found


def EiemCommentPhysics(lines: list[str]) -> tuple[list[str], list[str], list[int]]:
    """Comment every non-blank line of every Physics section.

    Returns the new line list, the section names touched, and the 1-based line
    numbers rewritten. The input list is not modified.
    """
    result = list(lines)
    sections = EiemFindPhysicsSections(lines)
    edits: list[int] = []
    for start, end, _name in sections:
        for index in range(start, end):
            text = result[index]
            stripped = text.lstrip()
            if not text.strip() or stripped.startswith((";", "#")):
                continue
            indent = text[: len(text) - len(stripped)]
            result[index] = f"{indent}{TAG}{stripped}"
            edits.append(index + 1)
    return result, [name for _s, _e, name in sections], edits


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ini")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--out")
    args = parser.parse_args()

    source = Path(args.ini)
    lines = source.read_text(encoding="utf-8").splitlines(keepends=True)
    result, found, edits = EiemCommentPhysics(lines)

    if not found:
        print("no Physics declaration found - nothing to do", file=sys.stderr)
        return 1
    if not edits:
        print("Physics declarations already commented:", ", ".join(found))
        return 0

    text = "".join(result)
    print("commented sections:", ", ".join(found))
    print("lines rewritten:", len(edits))
    if args.dry_run:
        flat = text.splitlines()
        for number in edits:
            print(f"  {number}: {flat[number - 1]}")
        print("dry run - not written")
        return 0
    target = Path(args.out) if args.out else source
    target.write_text(text, encoding="utf-8")
    print(f"wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
