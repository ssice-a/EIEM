"""Real Blender authoring and export test, only inside a caller's temp folder."""
import importlib.util
import json
import sys
from pathlib import Path

import bpy
import bmesh

addon_path, output = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_switch_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()
assert hasattr(bpy.types, "VIEW3D_PT_eiem_switches")
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

mesh = bpy.data.meshes.new("Source")
verts = [(x + dx, dy, 0) for x in (0, 2, 4, 6)
         for dx, dy in ((0, 0), (0.8, 0), (0, 0.8))]
mesh.from_pydata(verts, [], [(i, i + 1, i + 2) for i in range(0, 12, 3)])
mesh["eiem_section"] = "MeshSource"
mesh["eiem_asset"] = "SourceAsset"
mesh["eiem_source"] = "assets/test/source.asset"
mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
obj = bpy.data.objects.new("Always", mesh)
bpy.context.scene.collection.objects.link(obj)
obj["eiem_render_section"] = "RenderSource"
obj["eiem_render_asset"] = "SourceAsset"
obj["eiem_author_package"] = str(output / "offline")
obj["eiem_original_material_sections_json"] = json.dumps({"0": "MaterialBase"})
material = addon.load_material(Path(), "MaterialBase", {
    "source": "assets/test/base.mat", "name": "Base", "float.Value": "0"})
mesh.materials.append(material)
arm = bpy.data.objects.new("SharedRig", bpy.data.armatures.new("SharedRig"))
bpy.context.scene.collection.objects.link(arm)
bpy.context.view_layer.objects.active = arm
arm.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
for i, name in enumerate(("Root", "Tip", "Unused")):
    bone = arm.data.edit_bones.new(name)
    bone.head = (0, i, 0)
    bone.tail = (0, i + .5, 0)
bpy.ops.object.mode_set(mode="OBJECT")
for bone in arm.data.bones:
    bone["eiem_path"] = bone.name
for name in ("Root", "Tip", "Unused"):
    obj.vertex_groups.new(name=name)
obj.vertex_groups[0].add(list(range(6)), 1.0, "REPLACE")
obj.vertex_groups[1].add(list(range(6, 12)), 1.0, "REPLACE")
obj.modifiers.new("Skin", "ARMATURE").object = arm
obj["eiem_bone_palette_json"] = "[0,1,2]"
obj["eiem_bindposes_json"] = json.dumps([[int(r == c) for r in range(4) for c in range(4)]] * 3)
obj["eiem_bone_hashes_json"] = "[10,20,30]"
obj["eiem_bone_paths_json"] = '["Root","Tip","Unused"]'

def separate(source, minimum, maximum, name):
    bpy.ops.object.select_all(action="DESELECT")
    source.select_set(True)
    bpy.context.view_layer.objects.active = source
    before = set(bpy.data.objects)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="DESELECT")
    edit = bmesh.from_edit_mesh(source.data)
    for v in edit.verts:
        v.select_set(minimum <= v.co.x <= maximum)
    edit.select_flush_mode()
    bmesh.update_edit_mesh(source.data)
    bpy.ops.mesh.separate(type="SELECTED")
    bpy.ops.object.mode_set(mode="OBJECT")
    part, = set(bpy.data.objects) - before
    part.name = name
    return part

top = separate(obj, 2, 5, "Top")
accessory = separate(obj, 6, 7, "Accessory")
assert [len(o.data.polygons) for o in (obj, top, accessory)] == [1, 2, 1]
for part in (top, accessory):
    assert part.data["eiem_section"] == "MeshSource"
    assert part["eiem_bone_palette_json"] == "[0,1,2]"
    assert [g.name for g in part.vertex_groups] == ["Root", "Tip", "Unused"]

top_group = addon.create_switch_group("TopSwitch", "F6", [top])
acc_group = addon.create_switch_group("AccessorySwitch", "F7", [accessory])
variant = top.copy()
variant.data = top.data.copy()
variant.name = "Variant"
bpy.context.scene.collection.objects.link(variant)
variant_material = material.copy()
variant_material["eiem_source"] = "assets/test/other.mat"
variant.data.materials[0] = variant_material
variant_state = addon.add_switch_state(top_group, "Other style")
addon.assign_switch_meshes(variant_state, [variant])
assert variant not in addon.switch_meshes(addon.switch_states(top_group)[0])

# Default and preview are distinct. Restoring recovers pre-existing eye state.
accessory.hide_set(True)
addon.preview_switch(top_group, addon.switch_states(top_group)[1])
addon.preview_switch(acc_group, addon.switch_states(acc_group)[0])
assert top.hide_get() and variant.hide_get() and not accessory.hide_get()
assert addon.switch_states(top_group)[0]["eiem_default"]
addon.restore_switch_preview()
assert not top.hide_get() and not variant.hide_get() and accessory.hide_get()

plan = addon.plan_switch_export([top])
assert set(plan["objects"]) == {obj, top, accessory, variant}
assert len(plan["sources"]) == 1 and len(plan["groups"]) == 2
assert len(bpy.data.armatures) == 1
saved = output / "author.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(saved))
bpy.ops.wm.open_mainfile(filepath=str(saved))
top = bpy.data.objects["Top"]
obj = bpy.data.objects["Always"]
accessory = bpy.data.objects["Accessory"]
variant = bpy.data.objects["Variant"]
top_group = bpy.data.collections["TopSwitch"]
acc_group = bpy.data.collections["AccessorySwitch"]
assert len(addon.switch_groups()) == 2
plan = addon.plan_switch_export([top])
package = output / "package"
stats = addon.export_package(package, [top], [])
assert stats == {"meshes": 4, "materials": 1, "textures": 0, "skeletons": 0, "prefabs": 0}, stats
text = (package / "mod.ini").read_text(encoding="utf-8")
assert text.count("asset=SourceAsset") >= 1
assert text.count("handling=skip") == 1
assert text.count("[KeySwitch") == 2
assert text.count("[Render") == 5
payloads = [addon.read_mesh(p) for p in (package / "meshes").glob("*.mesh")]
assert sorted(len(p["indices"]) for p in payloads) == [3, 3, 6, 6]
for p in payloads:
    assert len(p["bindposes"]) == 3 and p["bone_hashes"] == [10, 20, 30]
    assert p["bone_paths"] == ["Root", "Tip", "Unused"]
    assert len(p["skin"]) == p["vertex_count"]

# Preview state is never export selection: default off still exports geometry.
addon.set_switch_default(top_group, addon.switch_states(top_group)[1])
addon.preview_switch(top_group, addon.switch_states(top_group)[1])
addon.export_package(output / "default-off", [top], [])
addon.set_switch_default(top_group, addon.switch_states(top_group)[0])

# Validation errors must leave a previously working package untouched.
baseline = {str(p.relative_to(package)): p.read_bytes() for p in package.rglob("*") if p.is_file()}
def unchanged_on_error():
    try:
        addon.export_package(package, [top], [])
    except ValueError:
        pass
    else:
        raise AssertionError("expected export validation error")
    assert baseline == {str(p.relative_to(package)): p.read_bytes() for p in package.rglob("*") if p.is_file()}

acc_group["eiem_key"] = "F6"
unchanged_on_error()
acc_group["eiem_key"] = "F7"
acc_group["eiem_key"] = "Ctrl+Ctrl+F7"
unchanged_on_error()
acc_group["eiem_key"] = "F7"
addon.switch_states(acc_group)[0].objects.link(top)
unchanged_on_error()
addon.switch_states(acc_group)[0].objects.unlink(top)
top["eiem_bone_palette_json"] = "[]"
unchanged_on_error()
top["eiem_bone_palette_json"] = "[0,1,2]"

# Re-registering the add-on does not erase project semantics.
addon.unregister()
addon.register()
assert len(addon.plan_switch_export([top])["groups"]) == 2
addon.unregister()
print("EIEM_SWITCHES_OK", stats)
