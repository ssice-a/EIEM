"""Collapse a source asset's per-Part Partner Renders into one merged Render.

This is the second stage of the merged-part design: after merge_eiem_mesh.py has
produced one Mesh for a source asset's sibling parts, this rewrites mod.ini so
the source Render swaps that Mesh in directly instead of declaring one Partner
per part.

The rewrite works on a parsed section list, refuses to write unless the result
parses to exactly the expected section list, and refuses to run twice. Sections
that are not part of the named group are emitted unchanged.

Usage:
    python tools/apply_merged_group_ini.py mod.ini \\
        --source-render RenderS_actor_typhoea_cloth_02_lod0_3 \\
        --merged-mesh MeshS_actor_typhoea_cloth_02_lod0_3_MERGED \\
        --mesh-path meshes/MeshS_actor_typhoea_cloth_02_lod0_3_MERGED.mesh \\
        --asset S_actor_typhoea_cloth_02_lod0 \\
        --source-logical assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_02_lod0.asset \\
        --insert-before MeshS_actor_typhoea_cloth_02_lod0_3_2 \\
        [--material SLOT:SECTION ...] [--dry-run]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

SKELETON = "Skeletonchr_0034_typhoea_postmodel_0"
PHYSICS = "PhysicsSkeletonchr_0034_typhoea_postmodel_0"


def parse_sections(text: str) -> tuple[list[str], list[tuple[str, list[str]]]]:
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
    while lines and not lines[-1].strip():
        lines.pop()
    return "\n".join(lines) + "\n\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ini")
    parser.add_argument("--source-render", required=True)
    parser.add_argument("--merged-mesh", required=True)
    parser.add_argument("--mesh-path", required=True)
    parser.add_argument("--asset", required=True)
    parser.add_argument("--source-logical", required=True)
    parser.add_argument("--insert-before", required=True)
    parser.add_argument(
        "--material",
        action="append",
        default=[],
        metavar="SLOT:SECTION",
        help="material section for a submesh slot; slots without an entry inherit",
    )
    parser.add_argument("--backup")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    path = Path(args.ini)
    if args.backup and Path(args.backup).resolve() == path.resolve():
        print("error: --backup must not be the input file", file=sys.stderr)
        return 1
    original = path.read_text(encoding="utf-8")
    banner, sections = parse_sections(original)
    names = [name for name, _ in sections]

    if args.source_render not in names:
        print(f"error: [{args.source_render}] not found", file=sys.stderr)
        return 1
    if args.merged_mesh in names:
        print(
            f"error: [{args.merged_mesh}] already present; restore the backup "
            "before applying again",
            file=sys.stderr,
        )
        return 1
    if args.insert_before not in names:
        print(f"error: [{args.insert_before}] not found", file=sys.stderr)
        return 1

    # The group is the source Render plus every Partner it declares. Partner
    # names are read from the Render body rather than guessed from a prefix, so a
    # differently named part is still collapsed correctly.
    body = dict(sections)[args.source_render]
    partners: list[str] = []
    for line in body:
        stripped = line.strip()
        if not stripped.startswith("partner."):
            continue
        _, _, value = stripped.partition("=")
        name = value.strip()
        if name and name not in partners:
            partners.append(name)
    missing = [name for name in partners if name not in names]
    if missing:
        print(f"error: declared Part section(s) not found: {missing}", file=sys.stderr)
        return 1

    materials: dict[int, str] = {}
    for entry in args.material:
        slot_text, _, section = entry.partition(":")
        if not section:
            print(f"error: --material needs SLOT:SECTION, got {entry!r}", file=sys.stderr)
            return 1
        materials[int(slot_text)] = section

    merged_mesh = f"""[{args.merged_mesh}]
path={args.mesh_path}
source={args.source_logical}
asset={args.asset}
target.path={args.source_logical}
target.asset={args.asset}
"""

    merged_render = [f"[{args.source_render}]", f"asset={args.asset}"]
    merged_render.append(f"mesh={args.merged_mesh}")
    merged_render.append(f"skeleton={SKELETON}")
    merged_render.append(f"physics={PHYSICS}")
    slot_count = len(partners)
    for slot in range(slot_count):
        merged_render.append(f"submesh.{slot}={slot}")
    if materials:
        for slot in sorted(materials):
            merged_render.append(f"material.{slot}={materials[slot]}")
    # A Render with neither `handling=skip` nor a Partner is a direct mesh
    # replacement: the game's own Renderer keeps carrying the model, which is the
    # whole point of merging.
    merged_render_text = "\n".join(merged_render) + "\n\n"

    dropped = set(partners) | {args.source_render}
    emitted: list[str] = []
    for name, section_body in sections:
        if name in partners:
            continue
        if name == args.source_render:
            emitted.append(merged_render_text)
            continue
        if name == args.insert_before:
            emitted.append(merged_mesh)
        emitted.append(render(name, section_body))

    rewritten = "\n".join(banner).rstrip() + "\n\n" + "".join(emitted)

    expected: list[str] = []
    for name in names:
        if name in partners:
            continue
        if name == args.insert_before:
            expected.append(args.merged_mesh)
        expected.append(name)
    _, check = parse_sections(rewritten)
    check_names = [name for name, _ in check]
    if check_names != expected:
        print("error: rewrite self-check failed", file=sys.stderr)
        for index, name in enumerate(check_names):
            want = expected[index] if index < len(expected) else "<none>"
            if name != want:
                print(f"  first mismatch at {index}: got [{name}], want [{want}]", file=sys.stderr)
                break
        return 2
    if len(check_names) != len(set(check_names)):
        duplicates = sorted({n for n in check_names if check_names.count(n) > 1})
        print(f"error: duplicate sections after rewrite: {duplicates}", file=sys.stderr)
        return 2

    print(f"group: [{args.source_render}] + {len(partners)} Part(s)")
    for index, name in enumerate(partners):
        print(f"  submesh {index} <- [{name}]")
    for slot in sorted(materials):
        print(f"  material.{slot} = {materials[slot]}")

    if args.dry_run:
        print(
            f"\n-- dry run: {len(original)} -> {len(rewritten)} bytes, "
            f"{len(names)} -> {len(check_names)} sections --"
        )
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
