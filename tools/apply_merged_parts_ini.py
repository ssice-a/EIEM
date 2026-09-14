"""Swap the nine-Part cloth_01 block in mod.ini for the single merged Render.

The rewrite works on a parsed section list rather than on line offsets, so it
cannot splice a region twice: sections are read into (header, body) blocks,
filtered by name, and re-emitted. Everything that is not the cloth_01 Render
block or the inserted merged Mesh resource is emitted unchanged.

Usage:
    python tools/apply_merged_parts_ini.py mod.ini --backup mod.ini.orig [--dry-run]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

# The Render asset whose nine per-Part children are being collapsed.
MERGED_ASSET = "S_actor_typhoea_cloth_01_lod0"
RENDER_SECTION = "RenderS_actor_typhoea_cloth_01_lod0_2"
PART_PREFIX = "RenderS_actor_typhoea_cloth_01_lod0_2Part"
# The merged Mesh resource is inserted immediately before this resource.
INSERT_BEFORE = "MeshS_actor_typhoea_cloth_02_lod0_3"
MERGED_MESH_SECTION = "MeshS_actor_typhoea_cloth_01_lod0_2_MERGED"

MERGED_MESH = f"""[{MERGED_MESH_SECTION}]
path=meshes/{MERGED_MESH_SECTION}.mesh
source=assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_01_lod0.asset
asset={MERGED_ASSET}
target.path=assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_01_lod0.asset
target.asset={MERGED_ASSET}
"""

# One submesh per source Part, in the order merge_eiem_mesh.py concatenated
# them, each keeping the material its Part declared. Parts 5 and 8 declared no
# material, so those submeshes are left without one exactly as before.
MERGED_RENDER = f"""[{RENDER_SECTION}]
asset={MERGED_ASSET}
mesh={MERGED_MESH_SECTION}
skeleton=Skeletonchr_0034_typhoea_postmodel_0
physics=PhysicsSkeletonchr_0034_typhoea_postmodel_0
submesh.0=0
submesh.1=1
submesh.2=2
submesh.3=3
submesh.4=4
submesh.5=5
submesh.6=6
submesh.7=7
submesh.8=8
material.0=MaterialM_actor_wulfa_cloth_04__2881474875459822074
material.1=MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209
material.2=MaterialM_actor_wulfa_cloth_04__2881474875459822074
material.3=MaterialM_actor_wulfa_cloth_04__2881474875459822074
material.4=MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209
material.6=MaterialM_actor_lod_pelica_cloth_04_7768265222391454716
material.7=MaterialM_actor_wulfa_cloth_04__2881474875459822074
"""


def parse_sections(text: str) -> tuple[list[str], list[tuple[str, list[str]]]]:
    """Split into a leading banner plus (name, body lines) sections."""
    banner: list[str] = []
    sections: list[tuple[str, list[str]]] = []
    current: tuple[str, list[str]] | None = None
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("[") and stripped.endswith("]"):
            if current is not None:
                sections.append(current)
            current = (stripped[1:-1], [])
            continue
        if current is None:
            banner.append(line)
        else:
            current[1].append(line)
    if current is not None:
        sections.append(current)
    return banner, sections


def render(name: str, body: list[str]) -> str:
    lines = [f"[{name}]"]
    lines.extend(body)
    # Trim trailing blanks; the emitter adds exactly one blank separator.
    while lines and not lines[-1].strip():
        lines.pop()
    return "\n".join(lines) + "\n\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ini", help="mod.ini to rewrite")
    parser.add_argument("--backup", help="copy the original here before rewriting")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    path = Path(args.ini)
    if args.backup and Path(args.backup).resolve() == path.resolve():
        print("error: --backup must not be the input file", file=sys.stderr)
        return 1
    original = path.read_text(encoding="utf-8")
    banner, sections = parse_sections(original)
    names = [name for name, _ in sections]

    if RENDER_SECTION not in names:
        print(f"error: [{RENDER_SECTION}] not found", file=sys.stderr)
        return 1
    if INSERT_BEFORE not in names:
        print(f"error: [{INSERT_BEFORE}] not found", file=sys.stderr)
        return 1
    # The merged block is identifiable by its own Mesh section, not by the
    # absence of Part sections (the untouched original has those). Refusing here
    # is what keeps a second run from collapsing an already-merged file.
    if MERGED_MESH_SECTION in names:
        print(
            "error: this mod.ini already carries a merged cloth_01 block; "
            "restore the backup before applying again",
            file=sys.stderr,
        )
        return 1

    dropped = [name for name in names if name.startswith(PART_PREFIX) or name == RENDER_SECTION]
    print(f"replacing {len(dropped)} Render section(s): {', '.join(dropped)}")

    emitted: list[str] = []
    for name, body in sections:
        if name.startswith(PART_PREFIX):
            continue
        if name == RENDER_SECTION:
            emitted.append(MERGED_RENDER)
            continue
        if name == INSERT_BEFORE:
            emitted.append(MERGED_MESH)
        emitted.append(render(name, body))

    rewritten = "\n".join(banner).rstrip() + "\n\n" + "".join(emitted)

    # Self-check: the result must parse to exactly the expected section list, or
    # we refuse to write. This is what catches a spliced or duplicated region.
    expected: list[str] = []
    for name in names:
        if name.startswith(PART_PREFIX):
            continue
        if name == INSERT_BEFORE:
            expected.append(MERGED_MESH_SECTION)
        expected.append(name)
    _, check = parse_sections(rewritten)
    check_names = [name for name, _ in check]
    if check_names != expected:
        print("error: rewrite self-check failed", file=sys.stderr)
        print(f"  expected {len(expected)} sections, got {len(check_names)}", file=sys.stderr)
        for index, name in enumerate(check_names):
            want = expected[index] if index < len(expected) else "<none>"
            if name != want:
                print(f"  first mismatch at {index}: got [{name}], want [{want}]", file=sys.stderr)
                break
        return 2
    if len(check_names) != len(set(check_names)):
        duplicates = sorted({name for name in check_names if check_names.count(name) > 1})
        print(f"error: duplicate sections after rewrite: {duplicates}", file=sys.stderr)
        return 2

    if args.dry_run:
        print(f"\n-- dry run: {len(original)} -> {len(rewritten)} bytes, "
              f"{len(names)} -> {len(check_names)} sections --")
        return 0

    if args.backup:
        Path(args.backup).write_text(original, encoding="utf-8")
        print(f"backup: {args.backup}")
    path.write_text(rewritten, encoding="utf-8")
    print(
        f"wrote {path}: {len(original)} -> {len(rewritten)} bytes, "
        f"{len(names)} -> {len(check_names)} sections"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
