"""LOD discovery and template replication through the real Blender exporter."""

import configparser
import importlib.util
import shutil
import sys
from pathlib import Path

import bpy


addon_path, output = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_lod_export_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()

output = output.resolve()
if output.exists():
    shutil.rmtree(output)


def material():
    value = bpy.data.materials.new("LODMaterial")
    value["eiem_section"] = "MaterialLOD"
    value["eiem_source"] = "assets/test/lod.mat"
    return value


def mesh_object(name, level):
    mesh = bpy.data.meshes.new(name + "Data")
    mesh.from_pydata([(0, 0, 0), (1, 0, 0), (0, 1, 0)], [], [(0, 1, 2)])
    mesh.materials.append(material())
    mesh["eiem_section"] = "MeshS_actor_test_lod%d" % level
    mesh["eiem_asset"] = "S_actor_test_lod%d" % level
    mesh["eiem_source"] = (
        "assets/test/s_actor_test_lod0.asset" if level == 0
        else "assets/test/sk_actor_test.fbx")
    mesh["eiem_target_path"] = mesh["eiem_source"]
    mesh["eiem_target_asset"] = mesh["eiem_asset"]
    mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj["eiem_render_section"] = "RenderS_actor_test_lod%d" % level
    obj["eiem_render_asset"] = mesh["eiem_asset"]
    obj["eiem_author_package"] = str(output / "authoring")
    return obj


lod0 = mesh_object("TemplateLOD0", 0)
lod1 = mesh_object("ObservedLOD1", 1)
lod1.hide_render = True

plan = addon.plan_mesh_only_export([lod0])
assert addon.mesh_lod_level(lod0) == 0
assert addon.discover_mesh_lods(lod0, [lod0, lod1]) == {0, 1}
expanded = addon.expand_lod_plan(plan, [0, 1], [lod0, lod1])
assert len(expanded["objects"]) == 2
assert {addon.mesh_source_identity(obj)[2] for obj in expanded["objects"]} == {
    "s_actor_test_lod0", "s_actor_test_lod1"}
ordered = sorted(expanded["objects"], key=addon.mesh_lod_level)
assert ordered[0].data.get("eiem_source", "").endswith("lod0.asset")
assert ordered[1].data.get("eiem_source", "").endswith("sk_actor_test.fbx")

try:
    addon.expand_lod_plan(plan, [4], [lod0, lod1])
    raise AssertionError("an undiscovered LOD must not be emitted")
except ValueError:
    pass

stats = addon.export_package(
    output / "all", mesh_objects=[lod0], armatures=[], physics_objects=[],
    mesh_only=True, lod_levels=[0, 1])
assert stats["meshes"] == 2, stats

parser = configparser.ConfigParser(interpolation=None, strict=False)
parser.optionxform = str
with (output / "all" / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    parser.read_file(stream)
renders = [section for section in parser.sections()
           if section.lower().startswith("render")]
meshes = [section for section in parser.sections()
          if section.lower().startswith("mesh")]
assert len(renders) == 2, renders
assert len(meshes) == 2, meshes
assert {parser[section]["asset"] for section in renders} == {
    "S_actor_test_lod0", "S_actor_test_lod1"}
assert all("lod2" not in section.lower() for section in parser.sections())
assert all(parser[section].get("mesh") for section in renders)

stats = addon.export_package(
    output / "lod1", mesh_objects=[lod0], armatures=[], physics_objects=[],
    mesh_only=True, lod_levels=[1])
assert stats["meshes"] == 1, stats
parser = configparser.ConfigParser(interpolation=None, strict=False)
parser.optionxform = str
with (output / "lod1" / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    parser.read_file(stream)
assert {parser[section]["asset"] for section in parser.sections()
        if section.lower().startswith("render")} == {"S_actor_test_lod1"}

addon.unregister()
print("EIEM_LOD_EXPORT_OK")
