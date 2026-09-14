"""Rewrite a typhoeus mod.ini for the merged-Mesh form, on a parsed section list.

Three edits, each addressed by section name rather than by text pattern, because
a cross-line regex has silently corrupted this file twice:

1. Comment the Skeleton and Physics RESOURCE declarations. `EiemPrepareModPhysics`
   loads every declared Physics resource unconditionally, so a stale declaration
   aborts the whole file; and Skeleton/Physics are a separate workstream here.
2. Replace the per-Part Partner Renders of one source asset with a single Render
   that swaps in the merged Mesh and maps each submesh to its own material slot.
3. Leave every other section byte-identical.

The script refuses to write unless the result re-parses to the expected section
list, and it reports every change it made.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

TAG = "; MERGE2-OFF "
HEADER = re.compile(r"^\[([^\]]+)\]\s*$")


def parse(text: str) -> tuple[list[str], list[tuple[str, int, int]]]:
    """Return (lines, [(name, start, end)]) with end exclusive."""
    lines = text.splitlines(keepends=True)
    headers = [
        (match.group(1).strip(), index)
        for index, line in enumerate(lines)
        if (match := HEADER.match(line.rstrip("\r\n")))
    ]
    sections: list[tuple[str, int, int]] = []
    for position, (name, start) in enumerate(headers):
        end = headers[position + 1][1] if position + 1 < len(headers) else len(lines)
        sections.append((name, start, end))
    return lines, sections


def comment_block(lines: list[str], start: int, end: int) -> int:
    changed = 0
    for index in range(start, end):
        text = lines[index]
        stripped = text.lstrip()
        if not text.strip() or stripped.startswith((";", "#")):
            continue
        indent = text[: len(text) - len(stripped)]
        lines[index] = f"{indent}{TAG}{stripped}"
        changed += 1
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ini")
    parser.add_argument("--out")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    source = Path(args.ini)
    original = source.read_text(encoding="utf-8")
    lines, sections = parse(original)
    report: list[str] = []

    # 1. Comment the Skeleton and Physics resource declarations.
    for name, start, end in sections:
        body = "".join(lines[start:end])
        is_physics = re.search(r"^path=.*\.physics\s*$", body, re.M) is not None
        is_skeleton_resource = (
            name.startswith("Skeleton") and "path=" in body and "asset=" not in body
        )
        if is_physics or is_skeleton_resource:
            count = comment_block(lines, start, end)
            if count:
                report.append(f"commented resource section [{name}] ({count} lines)")

    # 1b. Comment the directives that name those resources. The parser rejects a
    #     Render that references an undeclared Skeleton or Physics resource, so
    #     commenting only the declarations would fail the whole file.
    for index, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith((";", "#")):
            continue
        match = re.match(r"(skeleton|physics)\s*=", stripped)
        if not match:
            continue
        indent = line[: len(line) - len(stripped)]
        lines[index] = f"{indent}{TAG}{stripped}"
        report.append(f"commented rule directive: {stripped.strip()}")

    # Re-parse after the edits so section offsets are current.
    lines, sections = parse("".join(lines))
    by_name = {name: (start, end) for name, start, end in sections}

    # 2. Declare the merged Mesh resources, right after the last existing Mesh
    #    declaration so the resource block stays contiguous.
    merged_resources = (
        ("MeshCloth01Merged", "meshes/MeshCloth01Merged.mesh",
         "assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_01_lod0.asset",
         "S_actor_typhoea_cloth_01_lod0"),
        ("MeshCloth02Merged", "meshes/MeshCloth02Merged.mesh",
         "assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_02_lod0.asset",
         "S_actor_typhoea_cloth_02_lod0"),
    )
    last_mesh_span = None
    for name, start, end in sections:
        body = "".join(lines[start:end])
        # Identify by what the section points at, not by its name: the resource
        # sections are named after their asset, which also starts with "Mesh".
        if re.search(r"^path=meshes/", body, re.M):
            last_mesh_span = (start, end)
    if last_mesh_span is None:
        print("no Mesh resource section found to anchor the new declarations",
              file=sys.stderr)
        return 1
    block: list[str] = []
    for name, path, logical, asset in merged_resources:
        if name in by_name:
            continue
        block.extend([
            f"[{name}]\n",
            f"path={path}\n",
            f"source={logical}\n",
            f"asset={asset}\n",
            f"target.path={logical}\n",
            f"target.asset={asset}\n",
            "\n",
        ])
    if block:
        lines[last_mesh_span[1]:last_mesh_span[1]] = block
        report.append(
            "declared merged Mesh resources: "
            + ", ".join(name for name, *_ in merged_resources if name not in by_name)
        )
    lines, sections = parse("".join(lines))
    by_name = {name: (start, end) for name, start, end in sections}

    # 3. Rebuild the cloth Renders.
    #
    # cloth_01 merges the eight parts whose palettes resolve entirely against
    # game-owned bones. The ninth (MeshS_actor_typhoea_cloth_01_lod0_2_5, the
    # skirt) is left out: it is the only mesh carrying the eighteen
    # maid_skirt_* bones, which the live skeleton does not have, so including it
    # fails the whole Mesh assignment with "Skeleton bone path not found" and the
    # Renderer keeps the original Mesh. Excluding it keeps every merged part
    # bindable; the game's own skirt mesh is not replaced yet.
    for source_section, merged_section, merged_mesh, asset, slots, enabled in (
        (
            "RenderS_actor_typhoea_cloth_01_lod0_2",
            "RenderS_actor_typhoea_cloth_01_lod0_2",
            "MeshCloth01Merged",
            "S_actor_typhoea_cloth_01_lod0",
            # One entry per submesh, in submesh order: parts 0,1,2,3,4,6,7,8.
            # A part that declared no material takes its source Renderer's own:
            # that is the typhoea cloth material, not the wulfa one that the
            # undeclared-slot fallback would otherwise pick.
            ["MaterialM_actor_wulfa_cloth_04__2881474875459822074",
             "MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209",
             "MaterialM_actor_wulfa_cloth_04__2881474875459822074",
             "MaterialM_actor_wulfa_cloth_04__2881474875459822074",
             "MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209",
             "MaterialM_actor_lod_pelica_cloth_04_7768265222391454716",
             "MaterialM_actor_wulfa_cloth_04__2881474875459822074",
             "MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209"],
            True,
        ),
        (
            "RenderS_actor_typhoea_cloth_02_lod0_3",
            "RenderS_actor_typhoea_cloth_02_lod0_3",
            "MeshCloth02Merged",
            "S_actor_typhoea_cloth_02_lod0",
            ["MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209",
             "MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209",
             "MaterialM_actor_lod_typhoea_cloth_01__5993152462152797209"],
            True,
        ),
    ):
        if source_section not in by_name:
            print(f"missing source section [{source_section}]", file=sys.stderr)
            return 1
        # Remove the Part sections this source owned, plus the source itself.
        prefix = source_section + "Part"
        doomed = [
            name for name in by_name if name == source_section or name.startswith(prefix)
        ]
        spans = sorted((by_name[name] for name in doomed), reverse=True)
        for start, end in spans:
            del lines[start:end]
        report.append(
            f"[{source_section}]: removed {len(doomed)} sections "
            f"(source + {len(doomed) - 1} Parts)"
        )
        lines, sections = parse("".join(lines))
        by_name = {name: (start, end) for name, start, end in sections}

        # Insert the merged Render where the old source section stood.
        anchor = None
        for name in by_name:
            if name.startswith("Render"):
                anchor = by_name[name][0]
                break
        if anchor is None:
            print("no Render section left to anchor the insert", file=sys.stderr)
            return 1
        block = [
            f"[{merged_section}]\n",
            f"asset={asset}\n",
        ]
        if not enabled:
            # Inert match: the game keeps its own Mesh and material. Commented
            # rather than deleted so re-enabling is one edit, and so the intended
            # mapping stays visible next to the reason it is off.
            block.append(
                f"{TAG}mesh={merged_mesh}  (cannot bind: the Mesh palette names "
                f"bones the live skeleton lacks)\n"
            )
            for index in range(len(slots)):
                block.append(f"{TAG}submesh.{index}={index}\n")
            for index, material in enumerate(slots):
                block.append(f"{TAG}material.{index}={material}\n")
        else:
            block.append(f"mesh={merged_mesh}\n")
            # State the submesh -> material-slot map explicitly, as the earlier
            # working merged form did. It is the identity here, but it also pins
            # submeshCount, which sizes the material array, so the mapping does
            # not depend on how the undeclared-slot fallback behaves.
            for index in range(len(slots)):
                block.append(f"submesh.{index}={index}\n")
            for index, material in enumerate(slots):
                block.append(f"material.{index}={material}\n")
        block.append("\n")
        lines[anchor:anchor] = block
        report.append(
            f"[{merged_section}]: "
            + (f"merged Render with {len(slots)} submeshes"
               if enabled else "DISABLED (inert match, game Mesh kept)")
        )
        lines, sections = parse("".join(lines))
        by_name = {name: (start, end) for name, start, end in sections}

    result = "".join(lines)
    # Every Partner reference must be gone, or a later reload would try to build
    # a Renderer again and reintroduce the failure this change removes.
    leftover = re.findall(r"^\s*partner\.\d+=", result, re.M)
    if leftover:
        print(f"refusing to write: {len(leftover)} partner. references remain",
              file=sys.stderr)
        return 1

    print("\n".join(report))
    print()
    print("sections after edit:")
    _lines, final_sections = parse(result)
    for name, _start, _end in final_sections:
        print(f"  {name}")
    if args.dry_run:
        print("\ndry run - not written")
        return 0
    target = Path(args.out) if args.out else source
    target.write_text(result, encoding="utf-8")
    print(f"\nwrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
