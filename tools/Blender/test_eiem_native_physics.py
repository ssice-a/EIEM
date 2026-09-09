"""Real source import/edit/save/closure export and independent Mesh export."""
import copy
import faulthandler
import importlib.util
import json
import sys
from pathlib import Path

import bpy

addon_path, evidence, output = map(Path, sys.argv[sys.argv.index("--")+1:])
faulthandler.dump_traceback_later(45, repeat=True)
spec = importlib.util.spec_from_file_location("eiem_native_test", addon_path)
addon = importlib.util.module_from_spec(spec); spec.loader.exec_module(addon); addon.register()
native = addon.physics_authoring.native
output.mkdir(parents=True, exist_ok=True)
payload = native.source.read_evidence(evidence)
required = native.source.required_transforms(payload)
assert len(required) < len(payload["transforms"])
assert all(item["bone"] != "GrounderIK" for item in required)
print("SOURCE_READ_OK", flush=True)
rig, objects = native.import_source(evidence)
print("SOURCE_SCENE_OK", flush=True)
assert len(objects) == len(payload["components"])
groups = [o for o in objects if o.eiem_physics.kind == "NATIVE_GROUP"]
colliders = [o for o in objects if o.eiem_physics.kind == "NATIVE_COLLIDER"]
assert len(groups) == 11 and len(colliders) == 27
assert len([o for o in bpy.data.objects if o.type == "ARMATURE"]) == 1
assert len({o.eiem_native_physics.source_text for o in objects}) == 1
assert all(o.type == "EMPTY" and o.eiem_physics.rig == rig for o in objects)
assert all(o.hide_render and o.hide_select for o in bpy.data.objects if o.get("eiem_physics_visual"))
assert bpy.context.scene.eiem_physics_preview_style == "SOLID"

# FIXED and MOVE particles use the native chain radius instead of a fixed-size
# marker. Radius is the group base value times the curve sampled at the same
# normalized accumulated root distance used by native chain parameters.
basis = addon.unity_transform_matrix_to_blender_basis()
for group in groups:
    positions, attributes, parents, depths = native.group_local_selection_graph(group)
    expected = [[index, round(depths[index], 7), round(native.node_collision_radius(group, depths[index]), 7)]
                for index, attribute in enumerate(attributes) if attribute & 3]
    visuals = [child for child in group.children
               if child.get("eiem_physics_preview") == native.NODE_RADIUS_PREVIEW_MARKER]
    samples = sorted((sample for visual in visuals
                      for sample in json.loads(visual["eiem_physics_node_radius_samples"])),
                     key=lambda sample: sample[0])
    assert samples == expected
    for visual in visuals:
        visual_samples = json.loads(visual["eiem_physics_node_radius_samples"])
        assert visual.type == "MESH" and len(visual.data.vertices) == 62 * len(visual_samples)
        for offset, (index, depth, radius) in enumerate(visual_samples):
            center = basis @ positions[index]
            top = visual.data.vertices[offset * 62].co
            assert abs((top - center).length - radius) < 1e-5

# A selected Rig bone can be resolved back to its group role and normalized
# curve position without exposing SelectionData array order to the UI.
sample_group = groups[0]
bone_samples = [native.native_bone_sample(sample_group, bone)
                for bone in sample_group.eiem_physics.rig.data.bones]
bone_samples = [sample for sample in bone_samples if sample is not None]
assert bone_samples and native.NODE_SAMPLE_CACHE in sample_group
assert all(sample["role"] in {"FIXED", "MOVE", "IGNORE"} and
           0.0 <= sample["depth"] <= 1.0 for sample in bone_samples)

# The four source groups that actually enable angle limiting receive one
# generated preview object. Each cone belongs to a MOVE edge and samples the
# group curve at the game's normalized accumulated root distance.
angle_groups = [group for group in groups if group.eiem_native_physics.angle_limit_enabled]
assert {native.record(group)["name"] for group in angle_groups} == {
    "MBC_Typhoea_Cloth_Skirt", "MBC_Typhoea_Hair_Back_Ponytail_Long",
    "MBC_Typhoea_Hair_Front_Side_Long", "MBC_Typhoea_Tail"}
for group in groups:
    visuals = [child for child in group.children
               if child.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER]
    if group in angle_groups:
        assert len(visuals) == 1 and visuals[0].type == "MESH"
        positions, attributes, parents, depths = native.group_local_selection_graph(group)
        expected_count = sum(bool(attribute & 2 and parents[index] >= 0 and
                                  (positions[index] - positions[parents[index]]).length > 1e-8)
                             for index, attribute in enumerate(attributes))
        assert visuals[0]["eiem_physics_angle_cones"] == expected_count > 0
        samples = json.loads(visuals[0]["eiem_physics_angle_samples"])
        assert len(samples) == expected_count and max(depth for depth, angle in samples) <= 1
        assert all(abs(angle - native.angle_limit_degrees(group, depth)) < 1e-4
                   for depth, angle in samples)
    else:
        assert not visuals and group["eiem_physics_angle_preview"] == "disabled"

tail = next(group for group in angle_groups if native.record(group)["name"] == "MBC_Typhoea_Tail")
assert abs(native.angle_limit_degrees(tail, .5) - 30.0) < 1e-4

# Source physics is organized by role rather than flattened into one scene
# collection. Shared colliders stay separate from groups because one collider
# can be referenced by several groups.
physics_collection = addon.physics_authoring.collection(rig)
categories = {child.get("eiem_physics_category"): child for child in physics_collection.children}
assert set(categories) == {"GROUPS", "COLLIDERS", "VISUALS"}
assert all(group.name in categories["GROUPS"].objects for group in groups)
assert all(collider.name in categories["COLLIDERS"].objects for collider in colliders)
assert all(obj.name in categories["VISUALS"].objects for obj in bpy.data.objects if obj.get("eiem_physics_visual"))

# The retained multi-megabyte source tree is parsed once and treated as an
# immutable baseline. Per-object edits are copied before modification.
assert native.snapshot(groups[0]) is native.snapshot(groups[0])
source_name = native.record(groups[0])["name"]
copy_record = native.current_record(groups[0]); copy_record["name"] = "changed copy"
assert native.record(groups[0])["name"] == source_name

# Default visibility follows the active group and its shared colliders instead
# of drawing every source helper over the character at once.
active = bpy.context.scene.eiem_physics_group
active_refs = set(native.source.group_collider_sources(native.record(active)))
expected_visible = {active.eiem_native_physics.source_key} | active_refs
visible = {o.eiem_native_physics.source_key for o in objects if not o.hide_get()}
assert visible == expected_visible, (visible, expected_visible)
for collider in colliders:
    visuals = [o for o in collider.children if o.get("eiem_physics_visual")]
    assert len(visuals) == 1
    assert visuals[0]["eiem_physics_preview"] == "native-collider-authoring-gizmo"
    kind = native.record(collider)["type"]
    expected = {"BeyondBoneSphereCollider":("SPHERE",3),
                "BeyondBoneCapsuleCollider":("CAPSULE",15),
                "BeyondBonePlaneCollider":("PLANE",11)}[kind]
    assert visuals[0]["eiem_physics_shape"] == expected[0]
    assert visuals[0].type == "MESH" and len(visuals[0].data.polygons) > 0

# Native curve data is presented as nine named group-level controls instead of
# being buried among hundreds of flattened source fields.
assert len(native.CURVE_PARAMETERS) == 9
for group in groups:
    for path, label in native.CURVE_PARAMETERS:
        fields = native.curve_fields(group, path)
        assert {"value", "useCurve"} <= set(fields)
        assert any(key.startswith("curve.m_Curve.") for key in fields)

# Native curves use private Float Curve nodes. Only one selected curve is drawn
# in the panel, and the old nine object custom properties are not retained.
curve_group = groups[0]
tree = native.build_curve_mappings(curve_group)
assert len(tree.nodes) == len(native.CURVE_PARAMETERS) == 9
assert curve_group.animation_data is None or curve_group.animation_data.action is None
assert not any(str(key).startswith(native.CURVE_PROPERTY_PREFIX) for key in curve_group.keys())
assert native.friendly_field_label(
    "serializeData.angleLimitConstraint.limitAngle.curve.m_Curve.0.value"
) == "角度限制 · 关键帧 1 · 倍率"
damping_parameter = "serializeData.damping"
damping_fields, damping_original = native.source_curve_keys(curve_group, damping_parameter)
damping_original = copy.deepcopy(damping_original)
damping_original_enabled = damping_fields["useCurve"].integer
damping_fields["useCurve"].integer = "1"
damping_node = native.curve_mapping_node(curve_group, damping_parameter)
damping_points = native.curve_mapping_points(damping_node)
assert native.curve_mapping_endpoints(damping_node) == (damping_points[0], damping_points[-1])
native.set_curve_mapping_interpolation(damping_node, "LINEAR")
assert all(point.handle_type == "VECTOR" for point in damping_points)
native.set_curve_mapping_interpolation(damping_node, "SMOOTH")
assert all(point.handle_type == "AUTO" for point in damping_points)
inserted = native.insert_curve_mapping_point(damping_node, .5)
assert abs(inserted.location.x - .5) < 1e-6 and inserted.select
assert len(native.curve_mapping_points(damping_node)) == len(damping_points) + 1
native.remove_curve_mapping_point(damping_node, inserted)
assert len(native.curve_mapping_points(damping_node)) == len(damping_points)
try:
    native.remove_curve_mapping_point(damping_node, native.curve_mapping_points(damping_node)[0])
except ValueError:
    pass
else:
    raise AssertionError("Curve endpoint deletion should be rejected")
damping_end = max(damping_node.mapping.curves[0].points, key=lambda point: point.location.x)
damping_end.location = (damping_end.location.x, damping_end.location.y + .25)
damping_node.mapping.update()
assert native.poll_curve_previews() == .2
damping_fields, damping_keys = native.source_curve_keys(curve_group, damping_parameter)
assert damping_fields["useCurve"].integer == "1"
assert abs(damping_keys[-1]["value"] - damping_end.location.y) < 1e-6
assert all(key["weightedMode"] == 0 for key in damping_keys)
native.replace_curve_key_fields(curve_group, damping_parameter, damping_original)
damping_fields = native.curve_fields(curve_group, damping_parameter)
damping_fields["useCurve"].integer = damping_original_enabled
native.load_curve_mapping(curve_group, damping_parameter, damping_original)
if native.CURVE_MAPPING_EDITED in curve_group:
    del curve_group[native.CURVE_MAPPING_EDITED]

# useAngleLimit is independent from the angle curve's useCurve flag.
# Inline edits refresh the cone without changing the total enable switch.
rope = next(group for group in groups if native.record(group)["name"] == "MBC_Typhoea_Cloth_Skirt_Rope")
native.build_curve_mappings(rope)
angle_node = native.curve_mapping_node(rope, native.ANGLE_CURVE_PARAMETER)
angle_fields, angle_original = native.source_curve_keys(rope, native.ANGLE_CURVE_PARAMETER)
angle_original = copy.deepcopy(angle_original)
assert int(native.curve_fields(rope, native.ANGLE_CURVE_PARAMETER)["useCurve"].integer) and not rope.eiem_native_physics.angle_limit_enabled
rope.eiem_native_physics.angle_limit_enabled = True
native.flush_group_previews()
before_visual = next(child for child in rope.children
                     if child.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER)
before_pointer = before_visual.as_pointer()
before_samples = before_visual["eiem_physics_angle_samples"]

angle_end = max(angle_node.mapping.curves[0].points, key=lambda point: point.location.x)
original_y = float(angle_end.location.y)
angle_end.location = (angle_end.location.x, original_y + .25)
angle_node.mapping.update()
assert native.poll_curve_previews() == .2
assert rope.name_full in native._GROUP_PREVIEW_DIRTY
native.flush_group_previews()
after_visual = next(child for child in rope.children
                    if child.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER)
assert after_visual.as_pointer() != before_pointer
assert after_visual["eiem_physics_angle_samples"] != before_samples
native.replace_curve_key_fields(rope, native.ANGLE_CURVE_PARAMETER, angle_original)
native.load_curve_mapping(rope, native.ANGLE_CURVE_PARAMETER, angle_original)
if native.CURVE_MAPPING_EDITED in rope:
    del rope[native.CURVE_MAPPING_EDITED]
rope.eiem_native_physics.angle_limit_enabled = False
native.flush_group_previews()
assert not any(child.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER for child in rope.children)

# Radius Float Curve edits refresh physical-size node spheres even when the
# angle-limit preview is disabled.
radius_group = next(group for group in groups
                    if int(native.curve_fields(group, native.NODE_RADIUS_PARAMETER)["useCurve"].integer))
native.build_curve_mappings(radius_group)
radius_fields, radius_original = native.source_curve_keys(radius_group, native.NODE_RADIUS_PARAMETER)
radius_original = copy.deepcopy(radius_original)
native.flush_group_previews()
radius_visual = next(child for child in radius_group.children
                     if child.get("eiem_physics_preview") == native.NODE_RADIUS_PREVIEW_MARKER and
                     any(sample[1] > .99 for sample in json.loads(child["eiem_physics_node_radius_samples"])))
before_radius_samples = radius_visual["eiem_physics_node_radius_samples"]
radius_node = native.curve_mapping_node(radius_group, native.NODE_RADIUS_PARAMETER)
radius_end = max(radius_node.mapping.curves[0].points, key=lambda point: point.location.x)
radius_original_y = float(radius_end.location.y)
radius_end.location = (radius_end.location.x, radius_original_y + .25)
radius_node.mapping.update()
assert native.poll_curve_previews() == .2
assert radius_group.name_full in native._GROUP_PREVIEW_DIRTY
native.flush_group_previews()
radius_visual = next(child for child in radius_group.children
                     if child.get("eiem_physics_preview") == native.NODE_RADIUS_PREVIEW_MARKER and
                     any(sample[1] > .99 for sample in json.loads(child["eiem_physics_node_radius_samples"])))
assert radius_visual["eiem_physics_node_radius_samples"] != before_radius_samples
native.replace_curve_key_fields(radius_group, native.NODE_RADIUS_PARAMETER, radius_original)
native.load_curve_mapping(radius_group, native.NODE_RADIUS_PARAMETER, radius_original)
if native.CURVE_MAPPING_EDITED in radius_group:
    del radius_group[native.CURVE_MAPPING_EDITED]
native.schedule_group_preview(radius_group)
native.flush_group_previews()

# Each group Empty owns explicit references to the shared Collider Empties.
by_source = {o.eiem_native_physics.source_key:o for o in colliders}
for group in groups:
    expected = [by_source[key] for key in native.source.group_collider_sources(native.record(group))]
    assert [ref.object for ref in group.eiem_physics.colliders] == expected

# Wire mode remains available and recreates the old diagnostic outlines.
bpy.context.scene.eiem_physics_preview_style = "WIREFRAME"
for collider in colliders:
    visual = next(o for o in collider.children if o.get("eiem_physics_visual"))
    kind = native.record(collider)["type"]
    expected_splines = {"BeyondBoneSphereCollider":3,
                        "BeyondBoneCapsuleCollider":15,
                        "BeyondBonePlaneCollider":11}[kind]
    assert visual.type == "CURVE" and len(visual.data.splines) == expected_splines
for group in angle_groups:
    visual = next(o for o in group.children
                  if o.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER)
    assert visual.type == "CURVE" and len(visual.data.splines) > visual["eiem_physics_angle_cones"]
for group in groups:
    for visual in (child for child in group.children
                   if child.get("eiem_physics_preview") == native.NODE_RADIUS_PREVIEW_MARKER):
        samples = json.loads(visual["eiem_physics_node_radius_samples"])
        assert visual.type == "CURVE" and len(visual.data.splines) == 3 * len(samples)
bpy.context.scene.eiem_physics_preview_style = "SOLID"
assert all(next(o for o in group.children if o.get("eiem_physics_preview") == native.ANGLE_PREVIEW_MARKER).type == "MESH"
           for group in angle_groups)

# The source left-thigh capsule is start-aligned.  Its native total length
# includes both end radii, and its second sphere center lies on local -X.
thigh = next(o for o in colliders if native.record(o)["name"] ==
             "Magica Capsule Collider (Bip001_L_Thigh)")
thigh_geometry = native.source.collider_geometry(native.record(thigh))
assert thigh_geometry["start"] == (-.02, 0.0, 0.0)
assert abs(thigh_geometry["end"][0] - (-.312)) < 1e-6
assert abs(thigh_geometry["segmentLength"] - .292) < 1e-6

# Root/ignore references define the imported Transform graph. Selection points
# remain a separate source array; the preview matches positions rather than
# treating its array index as a bone identity.
for group in groups:
    paths = native.source.group_transform_paths(payload, native.record(group))
    assert paths and all(path in native.rig_paths(rig) for path in paths)
    assert len(native.source.group_collider_sources(native.record(group))) >= 0

file = output / "all.physics"
native.export_source(file, objects)
print("SOURCE_EXPORT_OK", flush=True)
baseline = native.source.read(file)
assert baseline["components"] == payload["components"]
assert baseline["transforms"] == payload["transforms"]

# Every imported parameter starts at the actual source value; only edits alter
# fields. This includes curve tangents and all unedited prebuild bytes.
group = groups[0]
gravity = next(f for f in group.eiem_native_physics.fields if f.label == "serializeData.gravity")
group_visuals = tuple(group.children)
if gravity.floating: gravity.value += 1
else: gravity.integer = str(int(gravity.integer)+1)
# Ordinary solver parameters do not change preview geometry and must not
# destroy/recreate thousands of curve points on every slider event.
assert tuple(group.children) == group_visuals
curve = next(f for f in group.eiem_native_physics.fields if "damping.curve.m_Curve.0.value" in f.label)
if curve.floating: curve.value += 1
else: curve.integer = str(int(curve.integer)+1)
native.export_source(output / "edited.physics", [group])
edited = native.source.read(output / "edited.physics")
expected = native.source.closure(payload, [group.eiem_native_physics.source_key])
expected["components"] = [native.current_record(o) for c in expected["components"] for o in objects
                          if c["source"] == o.eiem_native_physics.source_key]
assert edited["components"] == expected["components"]
assert len(edited["components"]) < len(objects)
saved = json.loads(gravity.original)
if gravity.floating: gravity.value = saved
else: gravity.integer = str(saved)
saved = json.loads(curve.original)
if curve.floating: curve.value = saved
else: curve.integer = str(saved)

# A referenced collider is one object even when several groups use it.
usage = {}
for c in payload["components"]:
    for r in c["references"]:
        if r["type"] == "MonoBehaviour" and not r["isNull"]: usage[r["identity"]] = usage.get(r["identity"],0)+1
shared = next(key for key,count in usage.items() if count > 1)
assert sum(o.eiem_native_physics.source_key == shared for o in colliders) == 1

names = [o.name for o in objects]
rig_name = rig.name
radius_group_name = radius_group.name
bpy.ops.wm.save_as_mainfile(filepath=str(output / "native-author.blend"))
bpy.ops.wm.open_mainfile(filepath=str(output / "native-author.blend"))
objects = [bpy.data.objects[name] for name in names]; rig = bpy.data.objects[rig_name]
radius_group = bpy.data.objects[radius_group_name]
assert radius_group.eiem_native_physics.curve_mapping_tree is not None
assert len(radius_group.eiem_native_physics.curve_mapping_tree.nodes) == 9
native.export_source(output / "saved.physics", objects)
assert native.source.read(output / "saved.physics") == baseline
before = set(bpy.data.objects)
try:
    native.import_source(evidence, rig)
    raise AssertionError("duplicate source imported")
except ValueError as error: assert "已有" in str(error)
assert set(bpy.data.objects) == before

# Native helpers cannot leak into selected mesh resources or produce skip.
mesh = bpy.data.meshes.new("MeshOnly")
mesh.from_pydata([(0,0,0), (1,0,0), (0,1,0)], [], [(0,1,2)])
mesh["eiem_section"] = "MeshOnly"; mesh["eiem_asset"] = "Only"
mesh["eiem_source"] = "assets/test/only.asset"
mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
obj = bpy.data.objects.new("Only", mesh); bpy.context.scene.collection.objects.link(obj)
obj["eiem_render_section"] = "RenderOnly"
bpy.ops.object.select_all(action="DESELECT"); obj.select_set(True)
for helper in objects:
    helper.select_set(True); helper.hide_render = True
bpy.context.view_layer.objects.active = obj
meshes, armatures = addon.selected_eiem_resources()
assert meshes == [obj] and armatures == []
stats = addon.export_package(output / "mesh-only")
assert stats["meshes"] == 1 and stats["skeletons"] == 0
ini = (output / "mesh-only/mod.ini").read_text(encoding="utf-8")
assert "physics=" not in ini.lower() and "[Physics" not in ini and "skip" not in ini
assert not list((output / "mesh-only").rglob("*.physics"))

# Native v2 remains an authoring/reference graph and cannot enter a runtime Mod.
# Reject it before changing the destination; standalone author export remains.
modifier = obj.modifiers.new("EIEM Armature", "ARMATURE"); modifier.object = rig
root_bone = native.rig_paths(rig)[""]
obj.vertex_groups.new(name=root_bone.name).add([0,1,2],1.0,"REPLACE")
obj["eiem_bone_paths_json"] = json.dumps([""])
obj["eiem_bindposes_json"] = json.dumps([[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]])
obj["eiem_bone_hashes_json"] = json.dumps([0])
destination = output / "rejected-mesh-physics"
destination.mkdir(exist_ok=True)
(destination / "mod.ini").write_text("existing author work", encoding="utf-8")
try:
    addon.export_package(destination, [obj], [], physics_objects=[objects[0]])
    raise AssertionError("unsupported runtime Physics export succeeded")
except ValueError as error:
    assert "v2" in str(error)
assert (destination / "mod.ini").read_text(encoding="utf-8") == "existing author work"
assert not list(destination.rglob("*.physics"))

# The .NET source's JSON integer-looking float remains a fractional float in
# the editor according to its TypeTree, including zero-valued collider center.
collider = next(o for o in objects if o.eiem_physics.kind == "NATIVE_COLLIDER")
center = next(f for f in collider.eiem_native_physics.fields if f.label == "center.x")
assert center.floating
center.value += .025
collider.location.x = .01
native.export_source(output / "moved.physics", [collider])
result = native.source.read(output / "moved.physics")["components"][0]
assert abs(result["fields"]["center"]["x"] - (center.value-.01)) < 1e-6
center.value = json.loads(center.original); collider.location.x = 0

# Reimport a portable v2 package onto its own shared rig; no evidence directory
# dependency is retained in the file.
new_rig, new_objects = native.import_source(file)
native.export_source(output / "portable.physics", new_objects)
portable = native.source.read(output / "portable.physics")
assert portable["components"] == baseline["components"]
assert portable["transforms"] == baseline["transforms"]

# A complete editable native parameter tree can be copied as a preset without
# copying roots, component identity or SelectionData. Native-to-native paste
# retains the target source graph; author paste below also copies collider use.
groups = [o for o in objects if o.eiem_physics.kind == "NATIVE_GROUP"]
template, target = groups[:2]
template_gravity = next(f for f in template.eiem_native_physics.fields if f.label == "serializeData.gravity")
target_gravity = next(f for f in target.eiem_native_physics.fields if f.label == "serializeData.gravity")
template_damping = next(f for f in template.eiem_native_physics.fields
                        if f.label == "serializeData.damping.curve.m_Curve.0.value")
target_damping = next(f for f in target.eiem_native_physics.fields
                      if f.label == "serializeData.damping.curve.m_Curve.0.value")
bpy.context.view_layer.objects.active = template
assert bpy.ops.eiem.physics_parameters(action="COPY") == {"FINISHED"}
template_clipboard = json.loads(bpy.context.scene.eiem_physics_parameter_clipboard)
assert len(template_clipboard["colliderRefs"]) == len(template.eiem_physics.colliders)
target_gravity.value = template_gravity.value + 3
target_damping.value = template_damping.value + 3
bpy.context.view_layer.objects.active = target
assert bpy.ops.eiem.physics_parameters(action="PASTE") == {"FINISHED"}
assert target_gravity.value == template_gravity.value
assert target_damping.value == template_damping.value
assert set(native.mapped_parameters(template)) == {"gravity", "stablizationTimeAfterReset",
                                                   "gravityFalloff", "blendWeight", "animationPoseRatio"}
mapped_radius = native.mapped_radius(radius_group)
assert mapped_radius["value"] > 0 and len(mapped_radius["keys"]) >= 2
assert all(0 <= key["time"] <= 1 for key in mapped_radius["keys"])
assert all(a["time"] < b["time"] for a,b in zip(mapped_radius["keys"], mapped_radius["keys"][1:]))

# Clone two connected source bones into new Skeleton nodes. The clone keeps the
# rest hierarchy but deliberately has no source identity/path.
paths = native.source.group_transform_paths(payload, native.record(template))
path_set = set(paths); path_lookup = native.rig_paths(rig)
pair = next((path_lookup[path].parent, path_lookup[path]) for path in paths
            if path_lookup[path].parent and path_lookup[path].parent.get("eiem_path") in path_set)
bpy.ops.object.mode_set(mode="OBJECT") if bpy.context.mode != "OBJECT" else None
bpy.ops.object.select_all(action="DESELECT"); rig.select_set(True); bpy.context.view_layer.objects.active = rig
bpy.ops.object.mode_set(mode="POSE")
bpy.ops.pose.select_all(action="DESELECT")
for bone in pair: rig.pose.bones[bone.name].select = True
rig.data.bones.active = pair[-1]
before_bones = set(rig.data.bones.keys())
assert bpy.ops.eiem.physics_edit(action="CLONE_CHAIN") == {"FINISHED"}
created_group = bpy.context.object
new_bones = set(rig.data.bones.keys()) - before_bones
assert created_group.eiem_physics.kind == "GROUP" and len(created_group.eiem_physics.nodes) == 2
assert len(new_bones) == 2 and all(not rig.data.bones[name].get("eiem_skeleton_source", True) for name in new_bones)
# The same clipboard maps every field supported by an author group, including
# the full node collision-radius curve with Unity tangent/weight metadata.
bpy.context.view_layer.objects.active = template
assert bpy.ops.eiem.physics_parameters(action="COPY") == {"FINISHED"}
bpy.context.view_layer.objects.active = created_group
assert bpy.ops.eiem.physics_parameters(action="PASTE") == {"FINISHED"}
assert created_group.eiem_physics.gravity == template_gravity.value
assert addon.physics_authoring.author_radius_record(created_group) == native.mapped_radius(template)
saved_parameters = json.loads(created_group.eiem_physics.native_parameter_snapshot)
assert saved_parameters["nativeType"] == native.record(template)["type"]
assert len(saved_parameters["nativeFields"]) == len(template.eiem_native_physics.fields) == 249
assert len(created_group.eiem_native_physics.fields) == 249
assert native.is_parameter_group(created_group)
author_tree = created_group.eiem_native_physics.curve_mapping_tree
assert author_tree is not None and len(author_tree.nodes) == len(native.CURVE_PARAMETERS) == 9
assert created_group.animation_data is None or created_group.animation_data.action is None
assert not any(str(key).startswith(native.CURVE_PROPERTY_PREFIX) for key in created_group.keys())
copied_colliders = [ref.object for ref in template.eiem_physics.colliders]
assert [ref.object for ref in created_group.eiem_physics.colliders] == copied_colliders
# The old 0.23 .blend representation stored only the hidden snapshot. Reload
# migration makes those values visible on the author Empty without guessing.
created_group.eiem_native_physics.fields.clear()
assert len(created_group.eiem_native_physics.fields) == 0
assert addon.physics_authoring.rebuild_existing_author_groups() is None
assert len(created_group.eiem_native_physics.fields) == 249
# Existing same-Rig source colliders can be removed and added explicitly.
assert bpy.ops.eiem.physics_edit(action="UNLINK", index=0) == {"FINISHED"}
assert len(created_group.eiem_physics.colliders) == 0
created_group.eiem_physics.collider_candidate = copied_colliders[0]
assert bpy.ops.eiem.physics_edit(action="LINK_CANDIDATE") == {"FINISHED"}
assert [ref.object for ref in created_group.eiem_physics.colliders] == copied_colliders
# Main controls feed the author snapshot; the complete field table remains an
# internal/read-only export contract.
author_damping = next(field for field in created_group.eiem_native_physics.fields
                      if field.label == "serializeData.damping.value")
author_damping_path = author_damping.label
author_damping.value += .125
edited_damping = author_damping.value
saved_parameters = json.loads(created_group.eiem_physics.native_parameter_snapshot)
assert next(item["value"] for item in saved_parameters["nativeFields"]
            if item["path"] == author_damping.label) == edited_damping
damping_node = native.curve_mapping_node(created_group, "serializeData.damping")
damping_point = min(damping_node.mapping.curves[0].points, key=lambda point: point.location.x)
damping_point.location = (damping_point.location.x, damping_point.location.y + .125)
damping_node.mapping.update()
native.poll_curve_previews()
author_damping_key = next(field for field in created_group.eiem_native_physics.fields
                          if field.label == "serializeData.damping.curve.m_Curve.0.value")
assert abs(author_damping_key.value - damping_point.location.y) < 1e-6
# Source collider associations are now preserved for editing, selection and
# visibility. Conversion to author collider records remains a separate DLL task.
try:
    addon.physics_authoring.author_document([created_group], "test.skeleton")
    raise AssertionError("source collider reference exported as author collider")
except ValueError as error:
    assert "游戏源碰撞体" in str(error)
created_group.eiem_physics.colliders.clear()
author_payload = addon.physics_authoring.author_document([created_group], "test.skeleton")
assert author_payload["version"] == addon.physics_authoring.document.VERSION == 4
assert len(author_payload["groups"][0]["nativeParameters"]) == 249
assert next(item["value"] for item in author_payload["groups"][0]["nativeParameters"]
            if item["path"] == author_damping_path) == edited_damping
author_roundtrip = addon.physics_authoring.document.decode(
    addon.physics_authoring.document.encode(author_payload))
assert len(author_roundtrip["groups"][0]["nativeParameters"]) == 249
addon.unregister()
faulthandler.cancel_dump_traceback_later()
print("EIEM_NATIVE_PHYSICS_OK: 11 groups; 27 shaped colliders; parameters; physical Transform selection; safe new-chain clone; incremental source export; Mesh only")
