"""Background-Blender regression: deleting slot 0 faces must not renumber slot 1.

Run with ``blender --background --factory-startup --python THIS_FILE -- ADDON``.
The synthetic package is exported only into a new temporary directory.
"""

import importlib.util
import json
import sys
import tempfile
from pathlib import Path

import bmesh
import bpy


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) != 1:
    raise SystemExit("usage: test_eiem_material_slot_gaps.py -- ADDON")

spec = importlib.util.spec_from_file_location("eiem_slot_gap_addon", args[0])
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)

mesh = bpy.data.meshes.new("SlotGapGeometry")
mesh.from_pydata(
    [(0, 0, 0), (1, 0, 0), (0, 1, 0),
     (2, 0, 0), (3, 0, 0), (2, 1, 0)],
    [], [(0, 1, 2), (3, 4, 5)],
)
mesh["eiem_section"] = "MeshSlotGap"
mesh["eiem_asset"] = "SlotGapSource"
mesh["eiem_source"] = "assets/test/slot_gap.asset"
mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
obj = bpy.data.objects.new("SlotGapObject", mesh)
bpy.context.scene.collection.objects.link(obj)
obj["eiem_render_section"] = "RenderSlotGap"
obj["eiem_render_asset"] = "SlotGapSource"

for section in ("MaterialEmpty", "MaterialVisible"):
    material = addon.load_material(Path(), section, {
        "source": "assets/test/" + section + ".mat",
        "name": section,
        "float.TestParameter": "0",
    })
    mesh.materials.append(material)
mesh.polygons[0].material_index = 0
mesh.polygons[1].material_index = 1
obj["eiem_original_material_sections_json"] = json.dumps({
    "0": "MaterialEmpty", "1": "MaterialVisible",
})
mesh.materials[1]["eiem_float.TestParameter"] = "1"

edit = bmesh.new()
edit.from_mesh(mesh)
bmesh.ops.delete(
    edit, geom=[face for face in edit.faces if face.material_index == 0],
    context="FACES",
)
edit.to_mesh(mesh)
edit.free()
mesh.update()
assert len(mesh.polygons) == 1 and mesh.polygons[0].material_index == 1
assert len(mesh.materials) == 2

with tempfile.TemporaryDirectory(prefix="eiem-slot-gap-") as temp:
    output = Path(temp) / "package"
    stats = addon.export_package(output, mesh_objects=[obj], armatures=[])
    assert stats["meshes"] == 1 and stats["materials"] == 1, stats
    ini = addon.parse_ini(output)
    render = ini["RenderSlotGap"]
    assert "material.0" not in render, dict(render)
    assert render["material.1"] == "MaterialVisible", dict(render)
    payload = addon.read_mesh(output / ini["MeshSlotGap"]["path"])
    assert len(payload["submeshes"]) == 2, payload["submeshes"]
    empty, visible = payload["submeshes"]
    assert empty[1:3] == (0, 0), empty
    assert visible[1:3] == (0, 3), visible
    assert len(payload["indices"]) == 3
    # Re-importing geometry must still address the same INI material slot.
    imported = addon.make_mesh("SlotGapRoundtrip", payload)
    assert len(imported.polygons) == 1
    assert imported.polygons[0].material_index == 1

print("EIEM_MATERIAL_SLOT_GAPS_OK")
