"""Real Blender scene, helper transforms, saved authoring and binary round trip."""
import importlib.util
import copy as pycopy
import sys
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector

addon_path, output = map(Path, sys.argv[sys.argv.index("--")+1:])
spec = importlib.util.spec_from_file_location("eiem_physics_test", addon_path)
addon = importlib.util.module_from_spec(spec); spec.loader.exec_module(addon); addon.register()
physics = addon.physics_authoring
nodes = [("",-1,(0,0,0),(0,0,0,1),(1,1,1)),
         ("Rig",0,(0.3,1,0.2),(0,0,0.6,0.8),(2,2,2))]
rig = addon.make_armature("SkeletonShared", {"coordinate": "unity-y-up-left-handed", "nodes": nodes}, bpy.context.scene.collection)
bpy.context.view_layer.objects.active = rig; rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
extra = rig.data.edit_bones.new("Extra"); extra.parent = rig.data.edit_bones["Rig"]
extra.head, extra.tail = (0,0,1), (0,0,1.2)
tip = rig.data.edit_bones.new("Tip"); tip.parent = extra
tip.head, tip.tail = extra.tail, (0,0,1.4)
bpy.ops.object.mode_set(mode="OBJECT")
group = physics.create_group(rig, [rig.data.bones["Extra"],rig.data.bones["Tip"]], "尾链")
group.eiem_physics.gravity = 5
extra_sample = physics.author_bone_sample(group, rig.data.bones["Extra"])
tip_sample = physics.author_bone_sample(group, rig.data.bones["Tip"])
assert extra_sample["role"] == "FIXED" and abs(extra_sample["depth"]) < 1e-8
assert tip_sample["role"] == "MOVE" and abs(tip_sample["depth"] - 1.0) < 1e-8
assert abs(group.eiem_physics.node_radius - .006) < 1e-8
radius_node = physics.native.curve_mapping_node(group, physics.native.NODE_RADIUS_PARAMETER)
assert radius_node is not None and len(radius_node.mapping.curves[0].points) == 2
group.eiem_physics.node_radius = .02
radius_end = max(radius_node.mapping.curves[0].points, key=lambda point: point.location.x)
radius_end.location = (radius_end.location.x, .25)
radius_node.mapping.update(); physics.native.poll_curve_previews(); physics.rebuild_group(group)
samples = []
for child in group.children:
    if child.get("eiem_physics_preview") == physics.native.NODE_RADIUS_PREVIEW_MARKER:
        import json
        samples.extend(json.loads(child["eiem_physics_node_radius_samples"]))
assert sorted(round(item[2], 6) for item in samples) == [.005, .02], samples

# Membership is explicit. New bones do not silently become physical; the
# author adds/removes selected nodes and each operation rebuilds the view.
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
follow = rig.data.edit_bones.new("Follow"); follow.parent = rig.data.edit_bones["Tip"]
follow.head, follow.tail = (0,0,1.4), (0,0,1.6)
detached = rig.data.edit_bones.new("Detached")
detached.head, detached.tail = (1,0,0), (1,0,.2)
detached.parent = rig.data.edit_bones["Rig"]
bpy.ops.object.mode_set(mode="OBJECT")
before_visuals = {child.as_pointer() for child in group.children if child.get("eiem_physics_visual")}
assert physics.add_selected_nodes(group, [rig.data.bones["Follow"]]) == 1
assert len(group.eiem_physics.nodes) == 3
after_visuals = {child.as_pointer() for child in group.children if child.get("eiem_physics_visual")}
assert after_visuals and after_visuals != before_visuals
assert physics.add_selected_nodes(group, [rig.data.bones["Detached"]]) == 1
assert len(group.eiem_physics.nodes) == 4
detached_id = physics.bone_id(rig.data.bones["Detached"])
assert next(node.role for node in group.eiem_physics.nodes if node.bone_id == detached_id) == "FIXED"
assert physics.remove_selected_nodes(group, [rig.data.bones["Follow"]]) == 1
assert len(group.eiem_physics.nodes) == 3
assert physics.remove_selected_nodes(group, [rig.data.bones["Detached"]]) == 1
assert len(group.eiem_physics.nodes) == 2

# Rest-pose edits mark only authored groups on this Armature for a delayed
# rebuild after Edit Mode ends.
class Update:
    id = rig.data
class Graph:
    updates = [Update()]
physics.depsgraph_physics_updated(bpy.context.scene, Graph())
assert rig.data.as_pointer() in physics._DIRTY_ARMATURES
assert physics.flush_armature_visuals() is None
assert rig.data.as_pointer() not in physics._DIRTY_ARMATURES
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
rig.data.edit_bones.remove(rig.data.edit_bones["Follow"])
rig.data.edit_bones.remove(rig.data.edit_bones["Detached"])
bpy.ops.object.mode_set(mode="OBJECT")
collider = physics.create_collider(group, rig.data.bones["Rig"], "CAPSULE", "身体胶囊")
collider.location = (0.01,0.02,0.03)
collider.rotation_quaternion = Quaternion(Vector((1,0,0)), 0.3)
collider.eiem_physics.radius, collider.eiem_physics.span = 0.04, 0.2
copy = physics.duplicate_group(group)
assert copy.eiem_physics.identity != group.eiem_physics.identity
assert copy.eiem_physics.gravity == 5 and copy.eiem_physics.colliders[0].object == collider
assert abs(copy.eiem_physics.node_radius - .02) < 1e-7
assert abs(physics.author_radius_record(copy)["keys"][-1]["value"] - .25) < 1e-7
copy.eiem_physics.gravity = 7
assert group.eiem_physics.gravity == 5

# The two-button clipboard copies every author parameter, the complete editable
# radius Float Curve, and the group's collider set rather than retaining a preset
# pointer to the source.
bpy.context.view_layer.objects.active = group
assert bpy.ops.eiem.physics_parameters(action="COPY") == {"FINISHED"}
clipboard = __import__("json").loads(bpy.context.scene.eiem_physics_parameter_clipboard)
assert clipboard["version"] == physics.PARAMETER_CLIPBOARD_VERSION == 2
assert clipboard["colliderRefs"] == [{"kind": "AUTHOR", "key": collider.eiem_physics.identity}]
source_radius = physics.author_radius_record(group)
copy.eiem_physics.gravity = 9
physics.load_author_radius(copy, physics.document.default_radius())
copy.eiem_physics.colliders.clear()
bpy.context.view_layer.objects.active = copy
assert bpy.ops.eiem.physics_parameters(action="PASTE") == {"FINISHED"}
assert copy.eiem_physics.gravity == group.eiem_physics.gravity
assert physics.author_radius_record(copy) == source_radius
assert [ref.object for ref in copy.eiem_physics.colliders] == [collider]
assert bpy.ops.eiem.physics_edit(action="UNLINK", index=0) == {"FINISHED"}
assert len(copy.eiem_physics.colliders) == 0
copy.eiem_physics.collider_candidate = collider
assert bpy.ops.eiem.physics_edit(action="LINK_CANDIDATE") == {"FINISHED"}
assert [ref.object for ref in copy.eiem_physics.colliders] == [collider]

# Shape dimensions are measured independently of the encoder. Solid preview uses
# cap-centre span plus one radius on each end, in the documented author space.
assert bpy.context.scene.eiem_physics_preview_style == "SOLID"
assert collider.eiem_physics.visual.type == "MESH"
points = [v.co for v in collider.eiem_physics.visual.data.vertices]
assert abs(max(p.z for p in points)-min(p.z for p in points)-0.28) < 1e-5
assert abs(max(p.x for p in points)-min(p.x for p in points)-0.08) < 1e-5
bpy.context.scene.eiem_physics_preview_style = "WIREFRAME"
assert collider.eiem_physics.visual.type == "CURVE"
bpy.context.scene.eiem_physics_preview_style = "SOLID"
bpy.context.view_layer.update()
depsgraph = bpy.context.evaluated_depsgraph_get()
actual = collider.evaluated_get(depsgraph).matrix_world
expected = rig.matrix_world @ physics.native_world(rig, rig.data.bones["Rig"]) @ collider.matrix_basis
assert max(abs(actual[r][c]-expected[r][c]) for r in range(4) for c in range(4)) < 1e-5, (actual, expected)

file = output / "chain.physics"
result = physics.export_physics(file, [group, copy])
assert result == {"groups":2,"colliders":1,"skeletons":1}, result
original = file.read_bytes()
payload = physics.document.decode(original)
assert payload["version"] == physics.document.VERSION
assert len(payload["groups"][0]["nativeParameters"]) >= 24
assert abs(payload["groups"][0]["radius"]["value"] - .02) < 1e-7
assert payload["groups"][0]["radius"]["useCurve"]
assert abs(payload["groups"][0]["radius"]["keys"][-1]["value"] - .25) < 1e-7
assert len(payload["colliders"]) == 1
assert all(g["colliders"] == [collider.eiem_physics.identity] for g in payload["groups"])
skel = addon.read_skeleton(output / payload["skeleton"])
assert [n[0] for n in skel["nodes"]][-2:] == ["Rig/Extra", "Rig/Extra/Tip"]
assert not (output/"mod.ini").exists() # authoring export cannot imply runtime assembly

# Visibility of helpers has no effect on resources or Mesh skip actions.
group.hide_render = True; collider.hide_render = True
group.hide_set(True); collider.hide_set(True)
physics.export_physics(file, [group,copy]); assert file.read_bytes() == original
group.hide_set(False); collider.hide_set(False)

names = [rig.name, group.name, copy.name, collider.name]
bpy.ops.wm.save_as_mainfile(filepath=str(output/"author.blend"))
bpy.ops.wm.open_mainfile(filepath=str(output/"author.blend"))
rig,group,copy,collider = [bpy.data.objects[n] for n in names]
physics.export_physics(file, [group,copy]); assert file.read_bytes() == original

# Bone identity survives display-name edits, including new unweighted nodes.
rig.data.bones["Tip"].name = "TipRenamed"
renamed = output / "renamed.physics"; physics.export_physics(renamed,[group,copy])
assert any(n["bone"].endswith("TipRenamed") for n in physics.document.read(renamed)["groups"][0]["nodes"])
rig.data.bones["TipRenamed"].name = "Tip"

# Invalid author state leaves the previously published resource untouched.
collider.scale.x = 2
try:
    physics.export_physics(file,[group,copy]); raise AssertionError("scaled collider accepted")
except ValueError as error: assert "缩放" in str(error)
assert file.read_bytes() == original
collider.scale.x = 1
bad = group.eiem_physics.colliders.add()
try:
    physics.export_physics(file,[group,copy]); raise AssertionError("missing reference accepted")
except ValueError as error: assert "缺失碰撞体" in str(error)
group.eiem_physics.colliders.remove(len(group.eiem_physics.colliders)-1)
assert file.read_bytes() == original

# A new import owns a separate Rig but shares one collider across its groups.
other_rig, imported = physics.import_physics(file)
assert other_rig != rig and len(imported) == 2
assert imported[0].eiem_physics.colliders[0].object == imported[1].eiem_physics.colliders[0].object
assert imported[0].eiem_physics.colliders[0].object != collider
again = output / "roundtrip.physics"; physics.export_physics(again,imported)
reencoded = physics.document.read(again)
assert reencoded["groups"] == payload["groups"]
for left,right in zip(reencoded["colliders"], payload["colliders"]):
    assert left.keys() == right.keys()
    for k in left:
        if isinstance(left[k],list): assert max(abs(a-b) for a,b in zip(left[k],right[k])) < 1e-6
        elif isinstance(left[k],float): assert abs(left[k]-right[k]) < 1e-6
        else: assert left[k] == right[k]
before = tuple(bpy.data.objects)
try:
    physics.import_physics(file,other_rig); raise AssertionError("duplicate source identities accepted")
except ValueError as error: assert "同身份" in str(error)
assert tuple(bpy.data.objects) == before
try:
    physics.export_physics(file, imported); raise AssertionError("source overwritten")
except ValueError as error: assert "不覆盖" in str(error)
assert file.read_bytes() == original

# Legacy author v1 remains importable; it acquires the documented flat default
# radius in Blender and exports as the current author v4 format.
legacy_payload = pycopy.deepcopy(payload); legacy_payload["version"] = physics.document.LEGACY_VERSION
for record in legacy_payload["groups"]: record.pop("radius"); record.pop("nativeParameters")
legacy_file = output / "legacy.physics"; legacy_file.write_bytes(physics.document.encode(legacy_payload))
legacy_rig, legacy_groups = physics.import_physics(legacy_file)
assert all(abs(item.eiem_physics.node_radius - .006) < 1e-8 for item in legacy_groups)
assert all(physics.native.curve_mapping_node(item, physics.native.NODE_RADIUS_PARAMETER) is not None
           for item in legacy_groups)
legacy_out = output / "legacy-current.physics"; physics.export_physics(legacy_out, legacy_groups)
assert physics.document.read(legacy_out)["version"] == physics.document.VERSION

# Exercise the actual edit-mode UI path: new identity properties must survive
# Blender's EditBone -> Bone synchronization when the operator exits Edit Mode.
bpy.context.view_layer.objects.active = rig
for item in bpy.context.selected_objects: item.select_set(False)
rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
for bone in rig.data.edit_bones:
    bone.select = bone.name in {"Extra", "Tip"}
    bone.select_head = bone.select_tail = bone.select
assert bpy.ops.eiem.physics_edit(action="CREATE") == {"FINISHED"}
ui_group = bpy.context.object
assert ui_group.eiem_physics.kind == "GROUP" and bpy.context.mode == "OBJECT"
ui_document = physics.author_document([ui_group], "test.skeleton")
assert len(ui_document["groups"][0]["nodes"]) == 2
rig.data.bones.active = rig.data.bones["Extra"]
assert bpy.ops.eiem.physics_edit(action="SPHERE") == {"FINISHED"}
ui_collider = bpy.context.object
assert ui_collider.eiem_physics.shape == "SPHERE"
visual_name = ui_collider.eiem_physics.visual.name
assert bpy.ops.eiem.physics_edit(action="DELETE_COLLIDER") == {"FINISHED"}
assert bpy.data.objects.get(visual_name) is None and len(ui_group.eiem_physics.colliders) == 0
bpy.data.objects.remove(ui_group, do_unlink=True)

# Multiple selected branches are one author group with shared parameters.  Each
# selected branch root remains FIXED, matching native BoneCloth rootBones; their
# common skeleton parent is not inserted into the physical node set.
bpy.context.view_layer.objects.active = rig
rig.select_set(True)
bpy.ops.object.mode_set(mode="EDIT")
extra_edit = rig.data.edit_bones["Extra"]
branch_names = []
for side, x in (("A", -.1), ("B", .1)):
    root = rig.data.edit_bones.new("Branch" + side); root.parent = extra_edit
    root.head, root.tail = (x, 0, 1.2), (x, 0, 1.35)
    tip = rig.data.edit_bones.new("Branch" + side + "Tip"); tip.parent = root
    tip.head, tip.tail = root.tail, (x, 0, 1.5)
    branch_names.extend((root.name, tip.name))
bpy.ops.object.mode_set(mode="OBJECT")
branch_group = physics.create_group(rig, [rig.data.bones[name] for name in branch_names], "分支链")
branch_lookup = {item.bone_id:item.role for item in branch_group.eiem_physics.nodes}
assert len(branch_lookup) == 4
assert physics.bone_id(rig.data.bones["Extra"]) not in branch_lookup
assert all(branch_lookup[physics.bone_id(rig.data.bones[name])] ==
           ("FIXED" if name in {"BranchA", "BranchB"} else "MOVE") for name in branch_names)
for child in list(branch_group.children):
    if child.get("eiem_physics_visual"): physics.native.remove_visual(child)
bpy.data.objects.remove(branch_group, do_unlink=True)
bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="EDIT")
for name in reversed(branch_names): rig.data.edit_bones.remove(rig.data.edit_bones[name])
bpy.ops.object.mode_set(mode="OBJECT")

# No native API calls, preferences, installed add-ons or game files are used.
addon.unregister()
print("EIEM_PHYSICS_AUTHORING_OK")
