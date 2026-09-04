"""Blender-background regression for one-Mesh incremental package export."""

import configparser
import importlib.util
import shutil
import sys
from pathlib import Path

import bmesh
import bpy


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) != 4:
    raise SystemExit(
        "usage: blender --background --python test_eiem_incremental_export.py "
        "-- ADDON SOURCE_PACKAGE OUTPUT_PACKAGE REPLACEMENT_PNG"
    )

addon_path, source_root, output_root, replacement_png = map(Path, args)
spec = importlib.util.spec_from_file_location("eiem_blender_addon_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()
assert hasattr(bpy.types, "MATERIAL_PT_eiem_properties")
assert hasattr(bpy.types, "DATA_PT_eiem_properties")
assert hasattr(bpy.types, "IMAGE_PT_eiem_properties")

source_root = source_root.resolve()
output_root = output_root.resolve()
replacement_png = replacement_png.resolve()
if output_root.exists():
    shutil.rmtree(output_root)

addon.import_package(source_root, clean=True)
target = next(
    obj for obj in bpy.context.scene.objects
    if obj.type == "MESH" and
    obj.data.get("eiem_section") == "MeshS_actor_typhoea_cloth_01_lod0_2"
)
before_polygons = len(target.data.polygons)
mesh_edit = bmesh.new()
mesh_edit.from_mesh(target.data)
mesh_edit.faces.ensure_lookup_table()
bmesh.ops.delete(
    mesh_edit, geom=list(mesh_edit.faces[:128]), context="FACES",
)
mesh_edit.to_mesh(target.data)
mesh_edit.free()
target.data.update()
assert len(target.data.polygons) < before_polygons

material = target.data.materials[0]
texture_paths, texture_transforms, parameters, resource = \
    addon.material_property_groups(material)
assert texture_paths and all(key.startswith("eiem_texture.")
                             for key in texture_paths), texture_paths
assert not any(key in addon.EIEM_INTERNAL_MATERIAL_PROPERTIES
               for group in (texture_paths, texture_transforms, parameters, resource)
               for key in group)
material["eiem_texture._BaseMap"] = str(replacement_png)
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True)
bpy.context.view_layer.objects.active = target
stats = addon.export_package(output_root)
assert stats == {
    "meshes": 1, "skeletons": 0, "materials": 1, "textures": 1,
    "prefabs": 0,
}, stats

parser = configparser.ConfigParser(interpolation=None, strict=False)
parser.optionxform = str
with (output_root / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    parser.read_file(stream)
sections = parser.sections()
assert sum(name.startswith("Mesh") for name in sections) == 1, sections
assert sum(name.startswith("Prefab") for name in sections) == 0, sections
assert sum(name.startswith("Render") for name in sections) == 1, sections
assert sum(name.startswith("Material") for name in sections) == 1, sections
assert sum(name.startswith("Texture") for name in sections) == 1, sections
assert sum(name.startswith("Skeleton") for name in sections) == 0, sections
mesh_section = next(name for name in sections if name.startswith("Mesh"))
assert parser[mesh_section]["target.path"] == parser[mesh_section]["source"]
assert parser[mesh_section]["target.asset"] == parser[mesh_section]["asset"]
render_section = next(name for name in sections if name.startswith("Render"))
assert parser[render_section]["asset"] == parser[mesh_section]["target.asset"]
assert parser[render_section]["mesh"] == mesh_section

material_section = next(name for name in sections if name.startswith("Material"))
assert parser[material_section]["target.path"] == material["eiem_source"]
assert parser[material_section]["target.asset"] == material["eiem_name"]

material_path = output_root / parser[material["eiem_section"]]["path"]
properties = addon.read_flat_properties(material_path)
assert properties["texture._BaseMap"].startswith("Texture"), properties
assert not any(
    key.startswith("texture.") and key != "texture._BaseMap"
    for key in properties
), properties
addon.unregister()
print("EIEM_INCREMENTAL_EXPORT_OK", stats)
