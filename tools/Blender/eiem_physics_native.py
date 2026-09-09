"""Blender native source import/edit/export. Source snapshots survive .blend save.

Visual helpers do not participate in Mesh selection or hide/skip semantics.
"""
import copy
import hashlib
import importlib.util
import json
import math
import struct
import tempfile
from pathlib import Path

import bpy
from bpy.props import BoolProperty, CollectionProperty, EnumProperty, FloatProperty, IntProperty, PointerProperty, StringProperty
from bpy_extras.io_utils import ImportHelper
from mathutils import Matrix, Quaternion, Vector

if __package__:
    from . import eiem_physics_source as source
else:
    _spec = importlib.util.spec_from_file_location("eiem_physics_source", Path(__file__).with_name("eiem_physics_source.py"))
    source = importlib.util.module_from_spec(_spec); _spec.loader.exec_module(source)

AUTHOR = None
API = {}
KINDS = {"NATIVE_GROUP", "NATIVE_COLLIDER"}
COLORS = {"FIXED": (1.0, .32, .06, 1), "MOVE": (.05, .65, 1, 1),
          "IGNORE": (.45, .45, .5, 1), "LINK": (.15, .8, .45, 1),
          "COLLIDER": (.9, .2, .7, 1), "ANGLE": (1.0, .72, .06, 1)}
PARAMETER_PAGE_SIZE = 32
COMMON_PARAMETERS = (
    ("serializeData.blendWeight", "物理混合权重"),
    ("serializeData.gravity", "重力"),
    ("serializeData.gravityFalloff", "重力衰减"),
    ("serializeData.animationPoseRatio", "动画姿态比例"),
    ("serializeData.stablizationTimeAfterReset", "重置后稳定时间"),
)
CURVE_PARAMETERS = (
    ("serializeData.damping", "阻尼"),
    ("serializeData.radius", "节点半径"),
    ("serializeData.distanceConstraint.stiffness", "距离约束强度"),
    ("serializeData.angleRestorationConstraint.stiffness", "角度恢复强度"),
    ("serializeData.angleLimitConstraint.limitAngle", "角度限制"),
    ("serializeData.motionConstraint.maxDistance", "最大运动距离"),
    ("serializeData.motionConstraint.backstopDistance", "回挡距离"),
    ("serializeData.colliderCollisionConstraint.limitDistance", "碰撞限制距离"),
    ("serializeData.selfCollisionConstraint.surfaceThickness", "自碰撞表面厚度"),
)
CURVE_PARAMETER_INDEX = {path: index for index, (path, label) in enumerate(CURVE_PARAMETERS)}
CURVE_PARAMETER_ITEMS = tuple(
    (str(index), label, "编辑%s沿物理链根部到末端的倍率" % label, index)
    for index, (path, label) in enumerate(CURVE_PARAMETERS)
)
CURVE_CONTEXT_FIELDS = {
    "serializeData.angleRestorationConstraint.stiffness": (
        ("serializeData.angleRestorationConstraint.useAngleRestoration", "启用角度恢复"),
        ("serializeData.angleRestorationConstraint.velocityAttenuation", "速度衰减"),
    ),
    "serializeData.angleLimitConstraint.limitAngle": (
        ("serializeData.angleLimitConstraint.useAngleLimit", "启用角度限制"),
        ("serializeData.angleLimitConstraint.stiffness", "限制刚度"),
    ),
    "serializeData.motionConstraint.maxDistance": (
        ("serializeData.motionConstraint.useMaxDistance", "启用最大距离"),
        ("serializeData.motionConstraint.stiffness", "运动约束强度"),
    ),
    "serializeData.motionConstraint.backstopDistance": (
        ("serializeData.motionConstraint.useBackstop", "启用回挡"),
        ("serializeData.motionConstraint.backstopRadius", "回挡半径"),
        ("serializeData.motionConstraint.stiffness", "运动约束强度"),
    ),
    "serializeData.colliderCollisionConstraint.limitDistance": (
        ("serializeData.colliderCollisionConstraint.mode", "碰撞模式"),
        ("serializeData.colliderCollisionConstraint.friction", "碰撞摩擦"),
    ),
    "serializeData.selfCollisionConstraint.surfaceThickness": (
        ("serializeData.selfCollisionConstraint.selfMode", "自碰撞模式"),
    ),
}
CURVE_FRAME_SCALE = 100.0
CURVE_PROPERTY_PREFIX = "物理曲线 "
CURVE_ACTION_MARKER = "eiem_native_physics_curves"
CURVE_ACTION_SIGNATURE = "eiem_native_physics_curve_signature"
CURVE_GROUP_NAME = "EIEM 物理参数曲线"
COLLIDER_VISUAL_FIELDS = (
    "center.x", "center.y", "center.z", "size.x", "size.y", "size.z",
    "direction", "reverseDirection", "radiusSeparation", "alignedOnCenter",
)
ANGLE_ENABLED_FIELD = "serializeData.angleLimitConstraint.useAngleLimit"
ANGLE_STIFFNESS_FIELD = "serializeData.angleLimitConstraint.stiffness"
ANGLE_CURVE_PARAMETER = "serializeData.angleLimitConstraint.limitAngle"
ANGLE_BASE_FIELD = ANGLE_CURVE_PARAMETER + ".value"
ANGLE_VISUAL_FIELDS = {ANGLE_ENABLED_FIELD, ANGLE_BASE_FIELD,
                       ANGLE_CURVE_PARAMETER + ".useCurve"}
ANGLE_CURVE_FIELD_PREFIX = ANGLE_CURVE_PARAMETER + ".curve."
ANGLE_PREVIEW_MARKER = "native-angle-limit"
NODE_RADIUS_PARAMETER = "serializeData.radius"
NODE_RADIUS_BASE_FIELD = NODE_RADIUS_PARAMETER + ".value"
NODE_RADIUS_VISUAL_FIELDS = {NODE_RADIUS_BASE_FIELD,
                             NODE_RADIUS_PARAMETER + ".useCurve"}
NODE_RADIUS_CURVE_FIELD_PREFIX = NODE_RADIUS_PARAMETER + ".curve."
NODE_RADIUS_PREVIEW_MARKER = "native-node-radius"
NODE_SAMPLE_CACHE = "eiem_physics_node_samples"
CURVE_MAPPING_TREE_MARKER = "eiem_physics_curve_mapping_tree"
CURVE_MAPPING_PARAMETER = "eiem_physics_curve_parameter"
CURVE_MAPPING_SIGNATURE = "eiem_physics_curve_mapping_signature"
CURVE_MAPPING_SOURCE_SIGNATURE = "eiem_physics_curve_mapping_source_signature"
CURVE_MAPPING_EDITED = "eiem_physics_curve_mapping_edited"
_GROUP_PREVIEW_DIRTY = set()
_SOURCE_CACHE = {}


def is_native(obj):
    return obj and obj.eiem_physics.kind in KINDS


def is_parameter_group(obj):
    if not (obj and obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP") and
            hasattr(obj, "eiem_native_physics") and len(obj.eiem_native_physics.fields)):
        return False
    paths = {field.label for field in obj.eiem_native_physics.fields}
    required = {path for path, label in COMMON_PARAMETERS}
    required.update((ANGLE_ENABLED_FIELD, ANGLE_STIFFNESS_FIELD))
    for parameter, label in CURVE_PARAMETERS:
        required.update((parameter + ".value", parameter + ".useCurve",
                         parameter + ".curve.m_Curve.0.time",
                         parameter + ".curve.m_Curve.0.value",
                         parameter + ".curve.m_Curve.0.inSlope",
                         parameter + ".curve.m_Curve.0.outSlope"))
    return required.issubset(paths)


def remember_snapshot(text, payload):
    key = (text.as_pointer(), text.name_full)
    _SOURCE_CACHE[key] = {
        "payload": payload,
        "components": {component["source"]: component for component in payload["components"]},
    }
    return _SOURCE_CACHE[key]


def snapshot_entry(obj):
    text = obj.eiem_native_physics.source_text
    source.require(text is not None, "原物理数据已丢失")
    key = (text.as_pointer(), text.name_full)
    entry = _SOURCE_CACHE.get(key)
    if entry is None:
        entry = remember_snapshot(text, json.loads(text.as_string()))
    return entry


def snapshot(obj):
    return snapshot_entry(obj)["payload"]


def record(obj):
    key = obj.eiem_native_physics.source_key
    return snapshot_entry(obj)["components"][key]


def current_record(obj):
    data = copy.deepcopy(record(obj))
    data["operation"] = "disable" if obj.eiem_native_physics.disabled else data["operation"]
    data["name"] = obj.eiem_physics.label
    edited_curves = curve_mapping_edited_parameters(obj)
    for parameter in edited_curves:
        _, keys = source_curve_keys(obj, parameter)
        parameter_data = source.get_field(data["fields"], tuple(parameter.split(".")))
        parameter_data["curve"]["m_Curve"] = copy.deepcopy(keys)
    for field in obj.eiem_native_physics.fields:
        if any(field.label.startswith(parameter + ".curve.m_Curve.")
               for parameter in edited_curves):
            continue
        value = field.value if field.floating else int(field.integer)
        # RNA floats are float32; preserve the original serialized decimal when
        # the author did not change that field through Blender.
        original = json.loads(field.original)
        baseline = struct.unpack("<f", struct.pack("<f", original))[0] if field.floating else original
        if value != baseline:
            source.set_field(data["fields"], json.loads(field.path), value, field.floating)
    return data


def local_matrix(t):
    q = t["localRotation"]
    return Matrix.LocRotScale(Vector(t["localPosition"]), Quaternion((q[3], q[0], q[1], q[2])), Vector(t["localScale"]))


def source_skeleton(payload):
    transforms = source.required_transforms(payload)
    by_id = {t["identity"]: t for t in transforms}
    nodes, indices = [], {}
    def add(t):
        if t["identity"] in indices:
            return indices[t["identity"]]
        parent = add(by_id[t["parent"]]) if t["parent"] else -1
        indices[t["identity"]] = len(nodes)
        nodes.append((t["bone"], parent, tuple(t["localPosition"]), tuple(t["localRotation"]), tuple(t["localScale"])))
        return indices[t["identity"]]
    for t in transforms: add(t)
    return {"coordinate": payload["coordinate"], "nodes": nodes, "source_nodes": [True]*len(nodes)}


def rig_paths(rig):
    return {record[0]: bone for bone, record, flag in API["skeleton_author_nodes"](rig)}


def validate_rig(payload, rig):
    actual = {record[0]: (record, flag) for bone, record, flag in API["skeleton_author_nodes"](rig)}
    lookup = rig_paths(rig)
    expected = source_skeleton(payload)["nodes"]
    for path, parent, *trs in expected:
        source.require(path in actual and actual[path][1], "物理源节点在 Rig 中缺失：" + path)
        row = actual[path][0]
        parent_path = expected[parent][0] if parent >= 0 else None
        bone = lookup[path]
        source.require((bone.parent.get("eiem_path") if bone.parent else None) == parent_path,
                       "物理源节点父级不一致：" + path)
        source.require(max(abs(a-b) for left, right in zip(row[2:], trs) for a, b in zip(left, right)) < 1e-4,
                       "物理源节点姿态不一致：" + path)


def extend_rig(payload, rig):
    """Merge physics-only source Transforms into an imported shared rig."""
    expected = source_skeleton(payload)["nodes"]
    actual = rig_paths(rig)
    if all(row[0] in actual for row in expected):
        return []
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    edit = rig.data.edit_bones
    lookup = {str(bone.get("eiem_path", bone.name)): bone for bone in edit}
    source_world = []
    created = []
    for path, parent, position, rotation, scale in expected:
        local = Matrix.LocRotScale(
            Vector(position),
            Quaternion((rotation[3], rotation[0], rotation[1], rotation[2])),
            Vector(scale),
        )
        world = source_world[parent] @ local if parent >= 0 else local
        source_world.append(world)
        if path in lookup:
            continue
        parent_path = expected[parent][0] if parent >= 0 else ""
        bone = edit.new(path.rsplit("/", 1)[-1] or "root")
        bone.head = Vector((0.0, 0.0, 0.0))
        bone.tail = Vector((0.0, 0.05, 0.0))
        bone.parent = lookup.get(parent_path) if parent >= 0 else None
        bone["eiem_path"] = path
        bone["eiem_skeleton_source"] = True
        bone["eiem_source_parent"] = parent_path
        bone["eiem_local_position"] = list(position)
        bone["eiem_local_rotation"] = list(rotation)
        bone["eiem_local_scale"] = list(scale)
        bone.matrix = API["unity_transform_matrix_to_blender"](world)
        lookup[path] = bone
        created.append(bone.name)
    bpy.ops.object.mode_set(mode="OBJECT")
    for name in created:
        bone = rig.data.bones[name]
        bone["eiem_rest_display"] = [
            bone.matrix_local[row][column]
            for row in range(4) for column in range(4)
        ]
    rig.select_set(False)
    return created


def remove_extended_bones(rig, names):
    if not names:
        return
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    if bpy.context.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.object.mode_set(mode="EDIT")
    for name in reversed(names):
        bone = rig.data.edit_bones.get(name)
        if bone:
            rig.data.edit_bones.remove(bone)
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.select_set(False)


def finish_visual(owner, obj, color, alpha=1.0):
    """Link and mark generated preview objects through one ownership path."""
    AUTHOR.collection(owner.eiem_physics.rig, "VISUALS").objects.link(obj)
    obj.parent = owner
    obj.hide_render = True
    obj.hide_select = True
    obj.show_in_front = getattr(bpy.context.scene, "eiem_physics_xray", False)
    obj.color = (color[0], color[1], color[2], alpha)
    obj["eiem_physics_visual"] = True
    return obj


def make_visual(owner, paths, color, label, width=.0008, native_coordinates=True):
    curve = bpy.data.curves.new(label, "CURVE")
    curve.dimensions = "3D"; curve.bevel_depth = width; curve.bevel_resolution = 1
    basis = API["unity_transform_matrix_to_blender_basis"]() if native_coordinates else Matrix.Identity(3)
    for points, cyclic in paths:
        if len(points) < 2: continue
        spline = curve.splines.new("POLY"); spline.points.add(len(points)-1)
        for point, xyz in zip(spline.points, points): point.co = (*(basis @ Vector(xyz)), 1)
        spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(label, curve)
    finish_visual(owner, obj, color)
    mat_name = "EIEM Physics " + str(color)
    mat = bpy.data.materials.get(mat_name) or bpy.data.materials.new(mat_name)
    mat.diffuse_color = color; curve.materials.append(mat)
    return obj


def remove_visual(obj):
    """Remove one generated preview and its private geometry datablock."""
    if not obj:
        return
    data, kind = obj.data, obj.type
    bpy.data.objects.remove(obj, do_unlink=True)
    if data and data.users == 0:
        if kind == "CURVE":
            bpy.data.curves.remove(data)
        elif kind == "MESH":
            bpy.data.meshes.remove(data)


def preview_style():
    return getattr(bpy.context.scene, "eiem_physics_preview_style", "SOLID")


def preview_material(color, alpha, label):
    rgba = (color[0], color[1], color[2], alpha)
    name = "EIEM Physics %s %.3f %.3f %.3f %.3f" % ((label,) + rgba)
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    material.diffuse_color = rgba
    if hasattr(material, "surface_render_method"):
        try:
            material.surface_render_method = "DITHERED"
        except (TypeError, ValueError):
            pass
    return material


def append_sphere(vertices, faces, center, radius, segments=12, rings=6):
    """Append a compact UV sphere in native author coordinates."""
    if radius <= 0:
        return
    center = Vector(center)
    top = len(vertices); vertices.append(tuple(center + Vector((0, 0, radius))))
    rows = []
    for row in range(1, rings):
        angle = math.pi * row / rings
        ring_vertices = []
        for step in range(segments):
            around = math.tau * step / segments
            offset = Vector((radius * math.sin(angle) * math.cos(around),
                             radius * math.sin(angle) * math.sin(around),
                             radius * math.cos(angle)))
            ring_vertices.append(len(vertices)); vertices.append(tuple(center + offset))
        rows.append(ring_vertices)
    bottom = len(vertices); vertices.append(tuple(center + Vector((0, 0, -radius))))
    for step in range(segments):
        following = (step + 1) % segments
        faces.append((top, rows[0][step], rows[0][following]))
        for row in range(len(rows) - 1):
            faces.append((rows[row][step], rows[row + 1][step],
                          rows[row + 1][following], rows[row][following]))
        faces.append((rows[-1][following], rows[-1][step], bottom))


def append_tube(vertices, faces, start, end, start_radius, end_radius, segments=10, caps=True):
    """Append a solid tube/frustum between two native-space points."""
    start, end = Vector(start), Vector(end)
    axis = end - start
    if axis.length < 1e-9 or max(start_radius, end_radius) <= 0:
        return
    direction = axis.normalized()
    helper = Vector((0, 0, 1)) if abs(direction.z) < .9 else Vector((0, 1, 0))
    side = direction.cross(helper).normalized()
    other = direction.cross(side).normalized()
    first, second = [], []
    for step in range(segments):
        angle = math.tau * step / segments
        radial = side * math.cos(angle) + other * math.sin(angle)
        first.append(len(vertices)); vertices.append(tuple(start + radial * start_radius))
        second.append(len(vertices)); vertices.append(tuple(end + radial * end_radius))
    for step in range(segments):
        following = (step + 1) % segments
        faces.append((first[step], second[step], second[following], first[following]))
    if caps:
        faces.append(tuple(reversed(first)))
        faces.append(tuple(second))


def make_mesh_visual(owner, vertices, faces, color, label, alpha=.75, native_coordinates=True):
    if not vertices or not faces:
        return None
    basis = API["unity_transform_matrix_to_blender_basis"]() if native_coordinates else Matrix.Identity(3)
    mesh = bpy.data.meshes.new(label)
    mesh.from_pydata([basis @ Vector(point) for point in vertices], [], faces)
    mesh.update()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj = bpy.data.objects.new(label, mesh)
    finish_visual(owner, obj, color, alpha)
    obj.show_transparent = True
    obj["eiem_physics_preview_style"] = "SOLID"
    mesh.materials.append(preview_material(color, alpha, label.rsplit(" ", 1)[-1]))
    return obj


def make_node_visual(owner, centers, color, label, native_coordinates=True,
                     wire_radius=.005, solid_radius=.006):
    if preview_style() == "WIREFRAME":
        paths = [ring(center, wire_radius, axis) for center in centers for axis in range(3)]
        return make_visual(owner, paths, color, label, native_coordinates=native_coordinates)
    vertices, faces = [], []
    for center in centers:
        append_sphere(vertices, faces, center, solid_radius)
    return make_mesh_visual(owner, vertices, faces, color, label, .9,
                            native_coordinates=native_coordinates)


def make_node_radius_visual(owner, samples, color, label, native_coordinates=True):
    """Draw each source simulation point at its evaluated collision radius."""
    samples = [(index, Vector(center), max(0.0, float(radius)), float(depth))
               for index, center, radius, depth in samples if float(radius) > 0]
    if not samples:
        return None
    if preview_style() == "WIREFRAME":
        paths = [ring(center, radius, axis)
                 for index, center, radius, depth in samples for axis in range(3)]
        visual = make_visual(owner, paths, color, label, native_coordinates=native_coordinates)
    else:
        vertices, faces = [], []
        for index, center, radius, depth in samples:
            append_sphere(vertices, faces, center, radius)
        visual = make_mesh_visual(owner, vertices, faces, color, label, .36,
                                  native_coordinates=native_coordinates)
    if visual:
        visual["eiem_physics_preview"] = NODE_RADIUS_PREVIEW_MARKER
        visual["eiem_physics_node_radius_samples"] = json.dumps(
            [[index, round(depth, 7), round(radius, 7)]
             for index, center, radius, depth in samples],
            separators=(",", ":"))
    return visual


def make_link_visual(owner, segments, color, label, native_coordinates=True,
                     radius=.0015, alpha=.75):
    if preview_style() == "WIREFRAME":
        return make_visual(owner, segments, color, label, native_coordinates=native_coordinates)
    vertices, faces = [], []
    for points, cyclic in segments:
        if len(points) == 2:
            append_tube(vertices, faces, points[0], points[1], radius, radius, 8)
    return make_mesh_visual(owner, vertices, faces, color, label, alpha,
                            native_coordinates=native_coordinates)


def ring(center, radius, axis=1):
    a, b = [i for i in range(3) if i != axis]
    points = []
    for step in range(40):
        point = list(center); angle = step * math.tau / 40
        point[a] += radius * math.cos(angle); point[b] += radius * math.sin(angle); points.append(point)
    return points, True


def sphere_paths(center, radius):
    return [ring(center, radius, axis) for axis in range(3)]


def collider_wire_paths(geometry):
    if geometry["shape"] == "SPHERE":
        return sphere_paths(geometry["center"], geometry["radius"])
    if geometry["shape"] == "PLANE":
        center = Vector(geometry["center"]); extent = .12
        paths = []
        for step in range(-2, 3):
            offset = step * extent / 2
            paths.append(([tuple(center + Vector((-extent, 0, offset))),
                           tuple(center + Vector((extent, 0, offset)))], False))
            paths.append(([tuple(center + Vector((offset, 0, -extent))),
                           tuple(center + Vector((offset, 0, extent)))], False))
        paths.append(([tuple(center), tuple(center + Vector((0, .04, 0)))], False))
        return paths
    start, end = Vector(geometry["start"]), Vector(geometry["end"])
    paths = sphere_paths(start, geometry["startRadius"]) + sphere_paths(end, geometry["endRadius"])
    direction = end - start
    axis = max(range(3), key=lambda index: abs(direction[index])) if direction.length else 1
    cross = [value for value in range(3) if value != axis]
    for step in range(8):
        angle = step * math.tau / 8
        a, b = Vector(start), Vector(end)
        for point, radius in ((a, geometry["startRadius"]), (b, geometry["endRadius"])):
            point[cross[0]] += math.cos(angle) * radius
            point[cross[1]] += math.sin(angle) * radius
        paths.append(([tuple(a), tuple(b)], False))
    paths.append(([tuple(start), tuple(end)], False))
    return paths


def make_collider_visual(owner, geometry, label):
    if preview_style() == "WIREFRAME":
        visual = make_visual(owner, collider_wire_paths(geometry), COLORS["COLLIDER"], label)
    else:
        vertices, faces = [], []
        if geometry["shape"] == "SPHERE":
            append_sphere(vertices, faces, geometry["center"], geometry["radius"], 16, 8)
        elif geometry["shape"] == "PLANE":
            center, extent = Vector(geometry["center"]), .12
            base = len(vertices)
            vertices.extend([tuple(center + Vector((-extent, 0, -extent))),
                             tuple(center + Vector((extent, 0, -extent))),
                             tuple(center + Vector((extent, 0, extent))),
                             tuple(center + Vector((-extent, 0, extent)))])
            faces.extend([(base, base + 1, base + 2, base + 3),
                          (base + 3, base + 2, base + 1, base)])
            append_tube(vertices, faces, center, center + Vector((0, .04, 0)), .0025, .0025, 8)
        else:
            start, end = geometry["start"], geometry["end"]
            append_sphere(vertices, faces, start, geometry["startRadius"], 16, 8)
            if Vector(end) != Vector(start):
                append_sphere(vertices, faces, end, geometry["endRadius"], 16, 8)
                append_tube(vertices, faces, start, end, geometry["startRadius"], geometry["endRadius"], 16)
        visual = make_mesh_visual(owner, vertices, faces, COLORS["COLLIDER"], label, .32)
    if visual:
        visual["eiem_physics_preview"] = "native-collider-authoring-gizmo"
        visual["eiem_physics_shape"] = geometry["shape"]
    return visual


def curve_fields(obj, parameter):
    prefix = parameter + "."
    return {field.label[len(prefix):]: field for field in obj.eiem_native_physics.fields
            if field.label.startswith(prefix)}


def curve_property(index, label):
    return "%s%02d %s倍率" % (CURVE_PROPERTY_PREFIX, index + 1, label)


def property_data_path(name):
    return '["%s"]' % bpy.utils.escape_identifier(name)


def action_fcurves(obj):
    """Return the action curve collection on legacy and Blender 4.4+ actions."""
    animation = obj.animation_data
    action = animation.action if animation else None
    if action is None:
        return None
    if hasattr(action, "fcurves"):
        return action.fcurves
    slot = getattr(animation, "action_slot", None)
    if slot is None:
        return None
    for layer in action.layers:
        for strip in layer.strips:
            if hasattr(strip, "channelbag"):
                bag = strip.channelbag(slot)
                if bag is not None:
                    return bag.fcurves
    return None


def find_action_curve(obj, data_path):
    curves = action_fcurves(obj)
    return next((curve for curve in curves or () if curve.data_path == data_path), None)


def source_curve_keys(obj, parameter):
    fields = curve_fields(obj, parameter)
    indices = sorted({int(key.split(".")[2]) for key in fields
                      if key.startswith("curve.m_Curve.") and key.split(".")[2].isdigit()})
    keys = []
    for index in indices:
        values = {}
        for suffix in ("time", "value", "inSlope", "outSlope"):
            field = fields.get("curve.m_Curve.%d.%s" % (index, suffix))
            source.require(field is not None and field.floating,
                           "源曲线关键帧字段不完整：%s %d" % (parameter, index + 1))
            values[suffix] = float(field.value)
        for suffix, default in (("weightedMode", 0), ("inWeight", 1 / 3), ("outWeight", 1 / 3)):
            field = fields.get("curve.m_Curve.%d.%s" % (index, suffix))
            if field is None:
                values[suffix] = default
            else:
                values[suffix] = float(field.value) if field.floating else int(field.integer)
        keys.append(values)
    source.require(keys, "源曲线没有关键帧：" + parameter)
    return fields, keys


def curve_mapping_tree(obj, create=False):
    """Return the private node tree used only by the inline Float Curve UI."""
    state = obj.eiem_native_physics
    tree = state.curve_mapping_tree
    if tree is not None:
        return tree
    if not create:
        return None
    name = ".EIEM 物理曲线 " + (obj.eiem_physics.identity or obj.name_full)
    tree = bpy.data.node_groups.new(name=name, type="ShaderNodeTree")
    tree[CURVE_MAPPING_TREE_MARKER] = obj.eiem_physics.identity or obj.name_full
    state.curve_mapping_tree = tree
    return tree


def curve_mapping_node(obj, parameter, create=False):
    tree = curve_mapping_tree(obj, create)
    if tree is None:
        return None
    node = next((item for item in tree.nodes
                 if item.get(CURVE_MAPPING_PARAMETER) == parameter), None)
    if node is None and create:
        index = CURVE_PARAMETER_INDEX.get(parameter, 0)
        label = CURVE_PARAMETERS[index][1] if parameter in CURVE_PARAMETER_INDEX else parameter
        node = tree.nodes.new("ShaderNodeFloatCurve")
        node.name = "EIEM_CURVE_%02d" % index
        node.label = label
        node[CURVE_MAPPING_PARAMETER] = parameter
    return node


def curve_mapping_points(node):
    """Return curve keys in root-to-tip order."""
    if node is None:
        return []
    return sorted(node.mapping.curves[0].points, key=lambda point: point.location.x)


def curve_mapping_endpoints(node):
    points = curve_mapping_points(node)
    source.require(len(points) >= 2, "浮点曲线至少需要两个关键点")
    return points[0], points[-1]


def curve_mapping_is_linear(node):
    points = curve_mapping_points(node)
    return bool(points) and all(point.handle_type == "VECTOR" for point in points)


def set_curve_mapping_interpolation(node, interpolation):
    """Apply one predictable interpolation style to the editable projection."""
    source.require(interpolation in ("LINEAR", "SMOOTH"), "未知的曲线插值方式")
    handle = "VECTOR" if interpolation == "LINEAR" else "AUTO"
    for point in curve_mapping_points(node):
        point.handle_type = handle
    node.mapping.update()


def insert_curve_mapping_point(node, position):
    """Insert a key without changing the currently displayed curve shape."""
    source.require(node is not None, "物理曲线尚未建立")
    position = min(1.0, max(0.0, float(position)))
    points = curve_mapping_points(node)
    linear = curve_mapping_is_linear(node)
    source.require(not any(abs(point.location.x - position) < 1e-5 for point in points),
                   "此链位置已有关键点")
    curve = node.mapping.curves[0]
    node.mapping.update()
    value = float(node.mapping.evaluate(curve, position))
    for point in points:
        point.select = False
    point = curve.points.new(position, value)
    point.handle_type = "VECTOR" if linear else "AUTO"
    point.select = True
    node.mapping.update()
    return point


def remove_curve_mapping_point(node, point):
    points = curve_mapping_points(node)
    source.require(point in points, "曲线关键点已改变")
    index = points.index(point)
    source.require(0 < index < len(points) - 1, "根端和末端关键点不能删除")
    source.require(len(points) > 2, "物理曲线至少保留两个关键点")
    node.mapping.curves[0].points.remove(point)
    node.mapping.update()


def curve_mapping_signature(node):
    if node is None:
        return ""
    mapping = node.mapping
    values = {
        "clip": [float(mapping.clip_min_x), float(mapping.clip_max_x),
                 float(mapping.clip_min_y), float(mapping.clip_max_y)],
        "points": [[float(point.location.x), float(point.location.y), point.handle_type]
                   for point in sorted(mapping.curves[0].points,
                                       key=lambda item: item.location.x)],
    }
    return json.dumps(values, separators=(",", ":"), allow_nan=False)


def source_curve_signature(keys):
    return json.dumps(keys, sort_keys=True, separators=(",", ":"), allow_nan=False)


def source_curve_is_linear(keys):
    if any(int(key.get("weightedMode", 0)) != 0 for key in keys):
        return False
    for first, second in zip(keys, keys[1:]):
        duration = float(second["time"]) - float(first["time"])
        if duration <= 1e-9:
            return False
        slope = (float(second["value"]) - float(first["value"])) / duration
        if (abs(float(first["outSlope"]) - slope) > 1e-5 or
                abs(float(second["inSlope"]) - slope) > 1e-5):
            return False
    return True


def load_curve_mapping(obj, parameter, keys):
    """Project source keys into Blender's compact Float Curve widget."""
    source.require(2 <= len(keys) <= 64, "浮点曲线需要 2..64 个控制点")
    node = curve_mapping_node(obj, parameter, True)
    mapping = node.mapping
    curve = mapping.curves[0]
    points = curve.points
    while len(points) > 2:
        points.remove(points[-1])
    ordered = sorted(keys, key=lambda item: item["time"])
    points[0].location = (float(ordered[0]["time"]), float(ordered[0]["value"]))
    points[1].location = (float(ordered[-1]["time"]), float(ordered[-1]["value"]))
    handle = "VECTOR" if source_curve_is_linear(ordered) else "AUTO"
    points[0].handle_type = points[1].handle_type = handle
    for key in ordered[1:-1]:
        point = points.new(float(key["time"]), float(key["value"]))
        point.handle_type = handle
    values = [float(key["value"]) for key in ordered]
    low, high = min(values), max(values)
    padding = max(.1, (high - low) * .2)
    mapping.use_clip = True
    mapping.clip_min_x = 0.0
    mapping.clip_max_x = 1.0
    mapping.clip_min_y = min(0.0, low - padding)
    mapping.clip_max_y = max(1.0, high + padding)
    mapping.extend = "HORIZONTAL"
    mapping.update()
    node[CURVE_MAPPING_SOURCE_SIGNATURE] = source_curve_signature(ordered)
    node[CURVE_MAPPING_SIGNATURE] = curve_mapping_signature(node)
    return node


def curve_mapping_keys(node):
    """Convert one simple Float Curve to Unity-style keys with unweighted slopes."""
    source.require(node is not None, "物理浮点曲线已丢失")
    mapping = node.mapping
    curve = mapping.curves[0]
    points = sorted(curve.points, key=lambda item: item.location.x)
    source.require(2 <= len(points) <= 64, "浮点曲线需要 2..64 个控制点")
    times = [float(point.location.x) for point in points]
    source.require(all(0.0 <= value <= 1.0 for value in times) and
                   all(a < b for a, b in zip(times, times[1:])),
                   "浮点曲线横轴必须在 0..1 内严格递增")
    result = []
    for index, (point, time) in enumerate(zip(points, times)):
        left_span = time - times[index - 1] if index else (
            times[1] - time if len(times) > 1 else 1.0)
        right_span = times[index + 1] - time if index + 1 < len(times) else left_span
        epsilon = max(1e-5, min(left_span, right_span) * .001)
        left = max(0.0, time - epsilon)
        right = min(1.0, time + epsilon)
        value = float(point.location.y)
        in_slope = ((value - float(mapping.evaluate(curve, left))) / (time - left)
                    if time > left else 0.0)
        out_slope = ((float(mapping.evaluate(curve, right)) - value) / (right - time)
                     if right > time else 0.0)
        result.append({"time": time, "value": value,
                       "inSlope": in_slope, "outSlope": out_slope,
                       "weightedMode": 0, "inWeight": 1 / 3,
                       "outWeight": 1 / 3})
    return result


def curve_mapping_edited_parameters(obj):
    try:
        values = json.loads(str(obj.get(CURVE_MAPPING_EDITED, "[]")))
    except (TypeError, ValueError, json.JSONDecodeError):
        values = []
    return {value for value in values if value in CURVE_PARAMETER_INDEX}


def mark_curve_mapping_edited(obj, parameter):
    values = curve_mapping_edited_parameters(obj)
    values.add(parameter)
    obj[CURVE_MAPPING_EDITED] = json.dumps(sorted(values), separators=(",", ":"))


def replace_curve_key_fields(obj, parameter, keys):
    """Replace one curve array while keeping every unrelated source field intact."""
    state = obj.eiem_native_physics
    prefix = parameter + ".curve.m_Curve."
    records = [{"path": field.path, "label": field.label,
                "original": field.original, "floating": bool(field.floating),
                "value": float(field.value) if field.floating else str(field.integer)}
               for field in state.fields if not field.label.startswith(prefix)]
    path_prefix = tuple(parameter.split(".")) + ("curve", "m_Curve")
    for index, key in enumerate(keys):
        for suffix in ("time", "value", "inSlope", "outSlope",
                       "weightedMode", "inWeight", "outWeight"):
            floating = suffix != "weightedMode"
            value = float(key[suffix]) if floating else int(key[suffix])
            path = path_prefix + (index, suffix)
            records.append({"path": source.dumps(path),
                            "label": ".".join(str(part) for part in path),
                            "original": source.dumps(value),
                            "floating": floating,
                            "value": value if floating else str(value)})
    previous_ready = bool(state.ready)
    state.ready = False
    try:
        state.fields.clear()
        for record in sorted(records, key=lambda item: natural_path_key(item["label"])):
            field = state.fields.add()
            field.path, field.label, field.original = record["path"], record["label"], record["original"]
            field.floating = record["floating"]
            if field.floating:
                field.value = float(record["value"])
            else:
                field.integer = str(record["value"])
    finally:
        state.ready = previous_ready


def apply_curve_mapping(obj, parameter):
    """Commit one edited inline curve to the canonical parameter fields."""
    source.require(is_parameter_group(obj), "当前物理组没有完整参数模板")
    node = curve_mapping_node(obj, parameter)
    keys = curve_mapping_keys(node)
    replace_curve_key_fields(obj, parameter, keys)
    mark_curve_mapping_edited(obj, parameter)
    node[CURVE_MAPPING_SOURCE_SIGNATURE] = source_curve_signature(keys)
    node[CURVE_MAPPING_SIGNATURE] = curve_mapping_signature(node)
    if obj.eiem_physics.kind == "GROUP" and AUTHOR is not None:
        AUTHOR.sync_author_controls_from_native_fields(obj)
    else:
        schedule_group_preview(obj)
    return len(keys)


def retire_legacy_curve_projection(obj):
    """Remove old Graph Editor channels after their values have been migrated."""
    action = obj.animation_data.action if obj.animation_data else None
    if action is None or action.get(CURVE_ACTION_MARKER) != obj.eiem_native_physics.source_key:
        return
    if action.get(CURVE_ACTION_SIGNATURE, "") != curve_projection_signature(obj):
        apply_curve_projection(obj)
    remove_curve_projection(obj)
    obj.animation_data_clear()
    if action.users == 0:
        bpy.data.actions.remove(action)


def build_curve_mappings(obj):
    """Ensure all nine compact widgets reflect the canonical source fields."""
    source.require(is_parameter_group(obj), "请选择含完整参数的物理组")
    action = obj.animation_data.action if obj.animation_data else None
    if action and action.get(CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key and \
            action.get(CURVE_ACTION_SIGNATURE, "") != curve_projection_signature(obj):
        apply_curve_projection(obj)
    for parameter, label in CURVE_PARAMETERS:
        fields, keys = source_curve_keys(obj, parameter)
        node = curve_mapping_node(obj, parameter)
        wanted = source_curve_signature(keys)
        if node is not None:
            current = curve_mapping_signature(node)
            baseline = str(node.get(CURVE_MAPPING_SIGNATURE, ""))
            if baseline and baseline != current:
                node.mapping.update()
                apply_curve_mapping(obj, parameter)
                fields, keys = source_curve_keys(obj, parameter)
                wanted = source_curve_signature(keys)
        if node is None or str(node.get(CURVE_MAPPING_SOURCE_SIGNATURE, "")) != wanted:
            load_curve_mapping(obj, parameter, keys)
    retire_legacy_curve_projection(obj)
    ensure_curve_preview_timer()
    return curve_mapping_tree(obj)


def remove_curve_projection(obj):
    curves = action_fcurves(obj)
    if curves is not None:
        wanted = {property_data_path(curve_property(index, label))
                  for index, (_, label) in enumerate(CURVE_PARAMETERS)}
        for curve in list(curves):
            if curve.data_path in wanted:
                curves.remove(curve)
    for key in list(obj.keys()):
        if str(key).startswith(CURVE_PROPERTY_PREFIX):
            del obj[key]


def curve_projection_signature(obj, parameters=None):
    curves = action_fcurves(obj)
    if curves is None:
        return ""
    wanted = None if parameters is None else {
        property_data_path(curve_property(CURVE_PARAMETER_INDEX[parameter],
                                          CURVE_PARAMETERS[CURVE_PARAMETER_INDEX[parameter]][1]))
        for parameter in parameters}
    values = []
    for curve in sorted(curves, key=lambda item: item.data_path):
        if CURVE_PROPERTY_PREFIX not in curve.data_path or (wanted is not None and curve.data_path not in wanted):
            continue
        values.append({"path": curve.data_path, "mute": bool(curve.mute), "keys": [
            [float(point.co.x), float(point.co.y),
             float(point.handle_left.x), float(point.handle_left.y),
             float(point.handle_right.x), float(point.handle_right.y),
             point.interpolation, point.handle_left_type, point.handle_right_type]
            for point in sorted(curve.keyframe_points, key=lambda item: item.co.x)]})
    return json.dumps(values, ensure_ascii=False, separators=(",", ":"), allow_nan=False)


def build_curve_projection(obj):
    """Project a source-compatible parameter table into Blender's Graph Editor."""
    source.require(is_parameter_group(obj), "请选择含完整参数的物理组")
    action = obj.animation_data.action if obj.animation_data else None
    source.require(action is None or action.get(CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key,
                   "该物理组已有非 EIEM 动画，请先移除或转移该 Action")
    remove_curve_projection(obj)
    for index, (parameter, label) in enumerate(CURVE_PARAMETERS):
        fields, keys = source_curve_keys(obj, parameter)
        prop = curve_property(index, label)
        path = property_data_path(prop)
        obj[prop] = keys[0]["value"]
        try:
            obj.id_properties_ui(prop).update(
                description="%s；横轴 0–100 对应链位置 0–1；纵轴为倍率" % label)
        except (AttributeError, TypeError):
            pass
        for key in keys:
            obj[prop] = key["value"]
            obj.keyframe_insert(data_path=path, frame=key["time"] * CURVE_FRAME_SCALE,
                                group=CURVE_GROUP_NAME)
            obj.animation_data.action[CURVE_ACTION_MARKER] = obj.eiem_native_physics.source_key
        curve = find_action_curve(obj, path)
        source.require(curve is not None, "Blender 未建立曲线：" + label)
        curve.mute = not bool(int(fields["useCurve"].integer))
        curve.color_mode = "CUSTOM"
        curve.color = COLORS[("FIXED", "MOVE", "LINK")[index % 3]][:3]
        points = sorted(curve.keyframe_points, key=lambda point: point.co.x)
        for key_index, (point, key) in enumerate(zip(points, keys)):
            left_interval = (keys[key_index]["time"] - keys[key_index - 1]["time"]
                             if key_index else
                             keys[1]["time"] - keys[0]["time"] if len(keys) > 1 else 1)
            right_interval = (keys[key_index + 1]["time"] - keys[key_index]["time"]
                              if key_index + 1 < len(keys) else left_interval)
            left_span = left_interval * (keys[key_index]["inWeight"]
                                         if int(keys[key_index]["weightedMode"]) & 1 else 1 / 3)
            right_span = right_interval * (keys[key_index]["outWeight"]
                                           if int(keys[key_index]["weightedMode"]) & 2 else 1 / 3)
            point.interpolation = "BEZIER"
            point.handle_left_type = "FREE"; point.handle_right_type = "FREE"
            point.handle_left = ((key["time"] - left_span) * CURVE_FRAME_SCALE,
                                 key["value"] - key["inSlope"] * left_span)
            point.handle_right = ((key["time"] + right_span) * CURVE_FRAME_SCALE,
                                  key["value"] + key["outSlope"] * right_span)
        curve.update()
    action = obj.animation_data.action
    action[CURVE_ACTION_MARKER] = obj.eiem_native_physics.source_key
    action.name = "EIEM 物理曲线 " + obj.eiem_physics.label
    action[CURVE_ACTION_SIGNATURE] = curve_projection_signature(obj)
    schedule_group_preview(obj)
    return action


def apply_curve_projection(obj):
    """Write Graph Editor key positions and slopes back to parameter fields."""
    source.require(is_parameter_group(obj), "请选择含完整参数的物理组")
    action = obj.animation_data.action if obj.animation_data else None
    source.require(action and action.get(CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key,
                   "请先生成 Blender 曲线")
    applied = 0
    parameter_state = obj.eiem_native_physics
    previous_ready = bool(parameter_state.ready)
    parameter_state.ready = False
    try:
        for index, (parameter, label) in enumerate(CURVE_PARAMETERS):
            fields, source_keys = source_curve_keys(obj, parameter)
            curve = find_action_curve(obj, property_data_path(curve_property(index, label)))
            source.require(curve is not None, "Blender 曲线已丢失：" + label)
            points = sorted(curve.keyframe_points, key=lambda point: point.co.x)
            source.require(len(points) == len(source_keys),
                           "%s 的关键帧数量已改变；当前版本请保持 %d 个关键帧" % (label, len(source_keys)))
            times = [float(point.co.x) / CURVE_FRAME_SCALE for point in points]
            source.require(all(0 <= value <= 1 for value in times), label + " 的关键帧必须位于横轴 0–100")
            source.require(all(a < b for a, b in zip(times, times[1:])), label + " 的关键帧位置必须递增")
            fields["useCurve"].integer = "0" if curve.mute else "1"
            for key_index, (point, time) in enumerate(zip(points, times)):
                values = {"time": time, "value": float(point.co.y)}
                left_dx = (float(point.co.x) - float(point.handle_left.x)) / CURVE_FRAME_SCALE
                right_dx = (float(point.handle_right.x) - float(point.co.x)) / CURVE_FRAME_SCALE
                values["inSlope"] = ((float(point.co.y) - float(point.handle_left.y)) / left_dx
                                     if left_dx > 1e-8 else 0.0)
                values["outSlope"] = ((float(point.handle_right.y) - float(point.co.y)) / right_dx
                                      if right_dx > 1e-8 else 0.0)
                for suffix, value in values.items():
                    fields["curve.m_Curve.%d.%s" % (key_index, suffix)].value = value
                mode_field = fields.get("curve.m_Curve.%d.weightedMode" % key_index)
                in_weight_field = fields.get("curve.m_Curve.%d.inWeight" % key_index)
                out_weight_field = fields.get("curve.m_Curve.%d.outWeight" % key_index)
                if mode_field and in_weight_field and out_weight_field:
                    old_mode = int(mode_field.integer)
                    left_interval = time - times[key_index - 1] if key_index else (
                        times[1] - times[0] if len(times) > 1 else 1)
                    right_interval = times[key_index + 1] - time if key_index + 1 < len(times) else left_interval
                    in_weight = left_dx / left_interval if left_interval > 1e-8 else 1 / 3
                    out_weight = right_dx / right_interval if right_interval > 1e-8 else 1 / 3
                    mode = ((1 if old_mode & 1 or abs(in_weight - 1 / 3) > 1e-5 else 0) |
                            (2 if old_mode & 2 or abs(out_weight - 1 / 3) > 1e-5 else 0))
                    mode_field.integer = str(mode)
                    if mode & 1:
                        in_weight_field.value = in_weight
                    if mode & 2:
                        out_weight_field.value = out_weight
                applied += 1
    finally:
        parameter_state.ready = previous_ready
    action[CURVE_ACTION_SIGNATURE] = curve_projection_signature(obj)
    if obj.eiem_physics.kind == "GROUP" and AUTHOR is not None:
        AUTHOR.sync_author_controls_from_native_fields(obj)
    else:
        schedule_group_preview(obj)
    return applied


def friendly_field_label(path):
    """Compact source paths for UI while retaining the exact path for search/export."""
    for parameter, label in CURVE_PARAMETERS:
        prefix = parameter + "."
        if path.startswith(prefix):
            suffix = path[len(prefix):]
            if suffix == "value": return label + " · 基础值"
            if suffix == "useCurve": return label + " · 使用曲线"
            parts = suffix.split(".")
            if len(parts) == 4 and parts[:2] == ["curve", "m_Curve"] and parts[2].isdigit():
                leaf = {"time":"链位置", "value":"倍率", "inSlope":"进入切线",
                        "outSlope":"离开切线", "weightedMode":"权重模式",
                        "inWeight":"进入权重", "outWeight":"离开权重"}.get(parts[3], parts[3])
                return "%s · 关键帧 %d · %s" % (label, int(parts[2]) + 1, leaf)
            return label + " · " + suffix.rsplit(".", 1)[-1]
    exact = dict(COMMON_PARAMETERS)
    exact.update({ANGLE_ENABLED_FIELD: "启用角度限制",
                  ANGLE_STIFFNESS_FIELD: "角度限制 · 刚度"})
    if path in exact:
        return exact[path]
    translations = {
        "center":"中心", "size":"尺寸", "direction":"方向", "reverseDirection":"反向",
        "radiusSeparation":"端半径分离", "alignedOnCenter":"中心对齐",
        "distanceConstraint":"距离约束", "angleRestorationConstraint":"角度恢复",
        "angleLimitConstraint":"角度限制", "motionConstraint":"运动约束",
        "colliderCollisionConstraint":"碰撞约束", "selfCollisionConstraint":"自碰撞",
        "stiffness":"强度", "limitAngle":"限制角度", "maxDistance":"最大距离",
        "backstopDistance":"回挡距离", "limitDistance":"限制距离",
        "surfaceThickness":"表面厚度", "x":"X", "y":"Y", "z":"Z",
    }
    parts = [part for part in path.split(".") if part not in ("serializeData", "serializeData2")]
    compact = parts[-2:] if len(parts) > 1 else parts
    return " · ".join(translations.get(part, part) for part in compact)


def natural_path_key(path):
    """Give imported and authored source fields the same deterministic order."""
    parts = []
    for part in path.split("."):
        parts.append((0, int(part)) if part.isdigit() else (1, part.casefold()))
    return tuple(parts)


def native_field(obj, path):
    return next((field for field in obj.eiem_native_physics.fields if field.label == path), None)


def native_number(obj, path, default=0.0):
    field = native_field(obj, path)
    if field is None:
        return default
    return float(field.value) if field.floating else int(field.integer)


def set_native_number(obj, path, value):
    field = native_field(obj, path)
    source.require(field is not None, "源物理字段已丢失：" + path)
    if field.floating:
        field.value = float(value)
    else:
        field.integer = str(int(value))


def evaluate_source_curve(keys, time):
    """Evaluate Unity keyframes, including weighted Bezier tangents."""
    if time <= keys[0]["time"]:
        return keys[0]["value"]
    if time >= keys[-1]["time"]:
        return keys[-1]["value"]
    right = next(index for index in range(1, len(keys)) if time <= keys[index]["time"])
    first, second = keys[right - 1], keys[right]
    duration = second["time"] - first["time"]
    if duration <= 1e-9:
        return second["value"]
    first_mode, second_mode = int(first["weightedMode"]), int(second["weightedMode"])
    out_weight = first["outWeight"] if first_mode & 2 else 1 / 3
    in_weight = second["inWeight"] if second_mode & 1 else 1 / 3
    x0, x3 = first["time"], second["time"]
    x1, x2 = x0 + duration * out_weight, x3 - duration * in_weight
    y0, y3 = first["value"], second["value"]
    y1 = y0 + first["outSlope"] * (x1 - x0)
    y2 = y3 - second["inSlope"] * (x3 - x2)

    def cubic(a, b, c, d, value):
        inverse = 1 - value
        return inverse ** 3 * a + 3 * inverse ** 2 * value * b + 3 * inverse * value ** 2 * c + value ** 3 * d

    low, high = 0.0, 1.0
    for _ in range(28):
        middle = (low + high) * .5
        if cubic(x0, x1, x2, x3, middle) < time:
            low = middle
        else:
            high = middle
    return cubic(y0, y1, y2, y3, (low + high) * .5)


def curve_parameter_multiplier(obj, parameter, depth):
    """Evaluate the optional root-to-tip multiplier for one native parameter."""
    fields, keys = source_curve_keys(obj, parameter)
    enabled = bool(int(fields["useCurve"].integer))
    node = curve_mapping_node(obj, parameter)
    pending_mapping_edit = (node is not None and str(node.get(CURVE_MAPPING_SIGNATURE, "")) !=
                            curve_mapping_signature(node))
    if enabled and pending_mapping_edit:
        return float(node.mapping.evaluate(node.mapping.curves[0], float(depth)))
    return evaluate_source_curve(keys, float(depth)) if enabled else 1.0


def curve_parameter_value(obj, parameter, depth):
    """Evaluate one native base value and its optional chain-depth multiplier."""
    base = float(native_number(obj, parameter + ".value"))
    return base * curve_parameter_multiplier(obj, parameter, depth)


def node_collision_radius(obj, depth):
    return max(0.0, curve_parameter_value(obj, NODE_RADIUS_PARAMETER, depth))


def angle_limit_degrees(obj, depth):
    return curve_parameter_value(obj, ANGLE_CURVE_PARAMETER, depth)


def component_local_graph(payload, component):
    """Return one component's Transform graph and component-local origins."""
    by_id = {item["identity"]: item for item in payload["transforms"]}
    order = {item["identity"]: index for index, item in enumerate(payload["transforms"])}
    children = {}
    for item in payload["transforms"]:
        children.setdefault(item["parent"], []).append(item["identity"])
    world = {}

    def matrix(identity):
        if identity not in world:
            item = by_id[identity]
            world[identity] = (matrix(item["parent"]) if item["parent"] else Matrix.Identity(4)) @ local_matrix(item)
        return world[identity]

    anchor = next(item for item in by_id.values() if item["owner"] == component["owner"])
    inverse = matrix(anchor["identity"]).inverted()
    origins = {identity: (inverse @ matrix(identity)).translation for identity in by_id}
    return by_id, order, children, origins


def group_local_selection_graph(obj, include_mapping=False):
    """Resolve a preview graph by matching serialized points to source Transform origins.

    The match is geometric and one-to-one. It does not make SelectionData array
    order an export/runtime identity contract.
    """
    payload, component = snapshot(obj), current_record(obj)
    selection = component["fields"]["serializeData2"]["selectionData"]
    positions = [Vector(tuple(point[axis] for axis in "xyz")) for point in selection["positions"]]
    attributes = [int(attribute["Value"]) for attribute in selection["attributes"]]
    by_id, order, children, origins = component_local_graph(payload, component)
    roots = source.referenced_sources(component, source.ROOT_REFERENCE, "Transform")
    transform_ids, seen, pending = [], set(), list(roots)
    while pending:
        identity = pending.pop(0)
        if identity in seen:
            continue
        seen.add(identity); transform_ids.append(identity)
        pending.extend(children.get(identity, ()))

    candidates = {index: sorted(((positions[index] - point).length, order[identity], identity)
                                for identity in transform_ids for point in (origins[identity],)
                                if (positions[index] - point).length <= 1e-4)
                  for index in range(len(positions))}
    source.require(all(candidates.values()), "Selection 点无法匹配到本组 Transform")
    mapping, available = {}, set(transform_ids)
    for index in sorted(range(len(positions)), key=lambda value: (len(candidates[value]), value)):
        choices = [item for item in candidates[index] if item[2] in available]
        source.require(bool(choices), "Selection 点与 Transform 不能一一匹配")
        identity = choices[0][2]
        mapping[identity] = index; available.remove(identity)

    parents = [-1] * len(positions)
    for identity, index in mapping.items():
        parent = by_id[identity]["parent"]
        visited = set()
        while parent is not None and parent not in mapping:
            source.require(parent not in visited and parent in by_id, "物理 Transform 父链无效")
            visited.add(parent); parent = by_id[parent]["parent"]
        if parent in mapping:
            parents[index] = mapping[parent]

    depths = [0.0] * len(positions)
    for index, attribute in enumerate(attributes):
        if not attribute & 2:
            continue
        current, parent = index, parents[index]
        while parent >= 0:
            depths[index] += (positions[current] - positions[parent]).length
            if not attributes[parent] & 2:
                break
            current, parent = parent, parents[parent]
    maximum = max(depths, default=0.0)
    if maximum > 1e-8:
        depths = [min(1.0, max(0.0, value / maximum)) for value in depths]
    result = (positions, attributes, parents, depths)
    return result + (mapping,) if include_mapping else result


def cache_native_bone_samples(obj, graph=None):
    """Build and cache source node roles and normalized chain positions."""
    positions, attributes, parents, depths, mapping = (
        graph or group_local_selection_graph(obj, include_mapping=True))
    payload = snapshot(obj)
    by_id = {item["identity"]: item for item in payload["transforms"]}
    samples = {}
    for identity, index in mapping.items():
        attribute = attributes[index]
        role = "FIXED" if attribute & 1 else "MOVE" if attribute & 2 else "IGNORE"
        samples[by_id[identity]["bone"]] = {
            "depth": float(depths[index]), "role": role, "index": int(index)}
    obj[NODE_SAMPLE_CACHE] = json.dumps(samples, separators=(",", ":"))
    return samples


def native_bone_sample(obj, bone):
    """Return the source node role and curve position for one Rig bone."""
    source.require(is_native(obj) and obj.eiem_physics.kind == "NATIVE_GROUP",
                   "所选物体不是游戏源物理组")
    try:
        samples = json.loads(str(obj.get(NODE_SAMPLE_CACHE, "{}")))
    except (TypeError, ValueError, json.JSONDecodeError):
        samples = {}
    path = str(bone.get("eiem_path", ""))
    if path not in samples:
        samples = cache_native_bone_samples(obj)
    sample = samples.get(path)
    return dict(sample, bone=bone, path=path) if sample is not None else None


def angle_cone_ring(apex, edge, angle, segments=20):
    length = edge.length
    if length <= 1e-8:
        return []
    direction = edge.normalized()
    helper = Vector((0, 0, 1)) if abs(direction.z) < .9 else Vector((0, 1, 0))
    side = direction.cross(helper).normalized()
    other = direction.cross(side).normalized()
    radians = math.radians(min(179.5, max(0.0, angle)))
    center = apex + direction * (length * math.cos(radians))
    radius = length * math.sin(radians)
    return [center + radius * (side * math.cos(math.tau * step / segments) +
                               other * math.sin(math.tau * step / segments))
            for step in range(segments)]


def make_angle_visual(owner, cones):
    if not cones:
        return None
    label = owner.name + " 角度限制锥"
    if preview_style() == "WIREFRAME":
        paths = []
        for apex, ring_points, angle, depth in cones:
            paths.append(([tuple(point) for point in ring_points], True))
            stride = max(1, len(ring_points) // 4)
            paths.extend(([tuple(apex), tuple(ring_points[index])], False)
                         for index in range(0, len(ring_points), stride))
        visual = make_visual(owner, paths, COLORS["ANGLE"], label, .00065)
    else:
        vertices, faces = [], []
        for apex, ring_points, angle, depth in cones:
            start = len(vertices); vertices.append(tuple(apex))
            ring_indices = []
            for point in ring_points:
                ring_indices.append(len(vertices)); vertices.append(tuple(point))
            for index in range(len(ring_indices)):
                faces.append((start, ring_indices[index], ring_indices[(index + 1) % len(ring_indices)]))
        visual = make_mesh_visual(owner, vertices, faces, COLORS["ANGLE"], label, .22)
    if visual:
        visual["eiem_physics_preview"] = ANGLE_PREVIEW_MARKER
        visual["eiem_physics_angle_cones"] = len(cones)
        visual["eiem_physics_angle_samples"] = json.dumps(
            [[round(depth, 7), round(angle, 7)] for apex, ring, angle, depth in cones],
            separators=(",", ":"))
    return visual


def rebuild_angle_preview(obj, graph=None):
    if not is_native(obj) or obj.eiem_physics.kind != "NATIVE_GROUP":
        return None
    for child in list(obj.children):
        if child.get("eiem_physics_preview") == ANGLE_PREVIEW_MARKER:
            remove_visual(child)
    action = obj.animation_data.action if obj.animation_data else None
    if action and action.get(CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key:
        ensure_curve_preview_timer()
    if not bool(native_number(obj, ANGLE_ENABLED_FIELD)):
        obj["eiem_physics_angle_preview"] = "disabled"
        return None
    try:
        positions, attributes, parents, depths = graph or group_local_selection_graph(obj)
        cones = []
        for index, attribute in enumerate(attributes):
            parent = parents[index]
            if not attribute & 2 or parent < 0:
                continue
            edge = positions[index] - positions[parent]
            angle = angle_limit_degrees(obj, depths[index])
            ring_points = angle_cone_ring(positions[parent], edge, angle)
            if ring_points:
                cones.append((positions[parent], ring_points, angle, depths[index]))
        visual = make_angle_visual(obj, cones)
        obj["eiem_physics_angle_preview"] = "ok:%d" % len(cones)
        return visual
    except (ValueError, KeyError, ReferenceError) as error:
        obj["eiem_physics_angle_preview"] = "error:" + str(error)
        return None


def rebuild(obj):
    if not is_native(obj): return
    for child in list(obj.children):
        if child.get("eiem_physics_visual"):
            remove_visual(child)
    c = current_record(obj)
    if c["type"] == "BeyondBoneCloth":
        positions, attributes, parents, depths, mapping = group_local_selection_graph(
            obj, include_mapping=True)
        cache_native_bone_samples(obj, (positions, attributes, parents, depths, mapping))
        samples = {key: [] for key in ("FIXED", "MOVE")}
        ignored = []
        for index, (position, attribute, depth) in enumerate(zip(positions, attributes, depths)):
            if attribute & 1:
                samples["FIXED"].append((index, position, node_collision_radius(obj, depth), depth))
            elif attribute & 2:
                samples["MOVE"].append((index, position, node_collision_radius(obj, depth), depth))
            else:
                ignored.append(position)
        for role, role_samples in samples.items():
            make_node_radius_visual(obj, role_samples, COLORS[role], obj.name + " " + role + " 碰撞半径")
        # IGNORE points are topology context rather than simulated particles;
        # retain a small neutral marker instead of assigning them a physical radius.
        make_node_visual(obj, ignored, COLORS["IGNORE"], obj.name + " IGNORE")
        # Source selection positions and attributes stay paired as serialized.
        # Hierarchy lines are visual context, not a guessed selection->bone map.
        payload = snapshot(obj)
        by_id, _, _, origins = component_local_graph(payload, c)
        physical_paths = set(source.group_transform_paths(payload, c))
        descendants = {t["identity"] for t in by_id.values() if t["bone"] in physical_paths}
        segments = []
        for key in descendants:
            t = by_id[key]
            if t["parent"] in descendants:
                segments.append(([list(origins[t["parent"]]), list(origins[key])], False))
        make_link_visual(obj, segments, COLORS["LINK"], obj.name + " 层级连线")
        rebuild_angle_preview(obj, (positions, attributes, parents, depths))
    else:
        geometry = source.collider_geometry(c)
        label = {"SPHERE":"球体", "CAPSULE":"胶囊", "PLANE":"无限平面"}[geometry["shape"]]
        make_collider_visual(obj, geometry, obj.name + " " + label)


def flush_group_previews():
    names = tuple(_GROUP_PREVIEW_DIRTY); _GROUP_PREVIEW_DIRTY.clear()
    for name in names:
        obj = bpy.data.objects.get(name)
        if obj and is_parameter_group(obj) and obj.eiem_native_physics.ready:
            try:
                if obj.eiem_physics.kind == "GROUP" and AUTHOR is not None:
                    AUTHOR.rebuild_group(obj)
                else:
                    rebuild(obj)
            except (ValueError, KeyError, ReferenceError): pass
    if getattr(bpy.context, "scene", None):
        apply_visibility(bpy.context.scene)
    return None


def schedule_group_preview(obj):
    if not is_parameter_group(obj):
        return
    _GROUP_PREVIEW_DIRTY.add(obj.name_full)
    if not bpy.app.timers.is_registered(flush_group_previews):
        bpy.app.timers.register(flush_group_previews, first_interval=.06)
    ensure_curve_preview_timer()


def poll_curve_previews():
    """Commit inline Float Curve edits and refresh only affected previews."""
    if AUTHOR is None or not hasattr(bpy.data, "objects"):
        return None
    tracked = False
    groups = [obj for obj in bpy.data.objects
              if hasattr(obj, "eiem_native_physics") and obj.eiem_native_physics.curve_mapping_tree]
    for obj in groups:
        tree = curve_mapping_tree(obj)
        if tree is None:
            continue
        tracked = True
        for node in list(tree.nodes):
            parameter = str(node.get(CURVE_MAPPING_PARAMETER, ""))
            if not parameter:
                continue
            signature = curve_mapping_signature(node)
            previous = str(node.get(CURVE_MAPPING_SIGNATURE, ""))
            if previous and previous != signature:
                try:
                    node.mapping.update()
                    if is_parameter_group(obj):
                        apply_curve_mapping(obj, parameter)
                    elif obj.eiem_physics.kind == "GROUP" and AUTHOR is not None:
                        AUTHOR.author_curve_mapping_edited(obj, parameter, curve_mapping_keys(node))
                        node[CURVE_MAPPING_SIGNATURE] = curve_mapping_signature(node)
                except (ValueError, KeyError, ReferenceError):
                    continue
            elif not previous:
                node[CURVE_MAPPING_SIGNATURE] = signature
    return .2 if tracked else None


def ensure_curve_preview_timer():
    if not bpy.app.timers.is_registered(poll_curve_previews):
        bpy.app.timers.register(poll_curve_previews, first_interval=.2)


def rebuild_existing_native():
    """Migrate/rebuild saved helpers once Blender leaves registration context."""
    if AUTHOR is None or not hasattr(bpy.data, "objects"):
        return .05
    sync_native_references()
    owners = [obj for obj in bpy.data.objects if is_native(obj)]
    for obj in owners:
        try:
            if obj.eiem_physics.kind == "NATIVE_GROUP" and is_parameter_group(obj):
                build_curve_mappings(obj)
            rebuild(obj)
        except (ValueError, KeyError, ReferenceError):
            continue
    if getattr(bpy.context, "scene", None):
        apply_visibility(bpy.context.scene)
    return None


def edited(self, context):
    obj = self.id_data
    if isinstance(obj, bpy.types.Object) and obj.eiem_physics.kind == "GROUP" and AUTHOR is not None:
        AUTHOR.author_native_field_edited(obj, self)
        return
    changes_geometry = (isinstance(obj, bpy.types.Object) and
                         obj.eiem_physics.kind == "NATIVE_COLLIDER" and
                         self.label in COLLIDER_VISUAL_FIELDS)
    if changes_geometry and obj.eiem_native_physics.ready:
        try: rebuild(obj)
        except (ValueError, KeyError, ReferenceError): pass
    changes_group_preview = (isinstance(obj, bpy.types.Object) and
                             obj.eiem_physics.kind == "NATIVE_GROUP" and
                             (self.label in ANGLE_VISUAL_FIELDS or
                              self.label.startswith(ANGLE_CURVE_FIELD_PREFIX) or
                              self.label in NODE_RADIUS_VISUAL_FIELDS or
                              self.label.startswith(NODE_RADIUS_CURVE_FIELD_PREFIX)))
    if changes_group_preview and obj.eiem_native_physics.ready:
        schedule_group_preview(obj)


def angle_enabled_get(self):
    return bool(native_number(self.id_data, ANGLE_ENABLED_FIELD))


def angle_enabled_set(self, value):
    set_native_number(self.id_data, ANGLE_ENABLED_FIELD, bool(value))


def angle_base_get(self):
    return float(native_number(self.id_data, ANGLE_BASE_FIELD))


def angle_base_set(self, value):
    set_native_number(self.id_data, ANGLE_BASE_FIELD, value)


def angle_stiffness_get(self):
    return float(native_number(self.id_data, ANGLE_STIFFNESS_FIELD))


def angle_stiffness_set(self, value):
    set_native_number(self.id_data, ANGLE_STIFFNESS_FIELD, value)


def active_curve_parameter(state):
    try:
        index = min(max(int(state.active_curve), 0), len(CURVE_PARAMETERS) - 1)
    except (TypeError, ValueError):
        index = 0
    return CURVE_PARAMETERS[index][0]


def curve_enabled_get(self):
    return bool(native_number(self.id_data, active_curve_parameter(self) + ".useCurve"))


def curve_enabled_set(self, value):
    set_native_number(self.id_data, active_curve_parameter(self) + ".useCurve", bool(value))


def context_enabled_path(state):
    return next((path for path, label in CURVE_CONTEXT_FIELDS.get(active_curve_parameter(state), ())
                 if path.rsplit(".", 1)[-1].startswith("use")), "")


def context_enabled_get(self):
    path = context_enabled_path(self)
    return bool(native_number(self.id_data, path)) if path else False


def context_enabled_set(self, value):
    path = context_enabled_path(self)
    if path:
        set_native_number(self.id_data, path, bool(value))


class EIEM_PG_native_field(bpy.types.PropertyGroup):
    path: StringProperty()
    label: StringProperty()
    original: StringProperty()
    floating: BoolProperty()
    value: FloatProperty(update=edited)
    integer: StringProperty(update=edited)


class EIEM_PG_native_physics(bpy.types.PropertyGroup):
    source_text: PointerProperty(type=bpy.types.Text)
    source_key: StringProperty()
    fields: CollectionProperty(type=EIEM_PG_native_field)
    curve_mapping_tree: PointerProperty(type=bpy.types.NodeTree, options={"HIDDEN"})
    active_curve: EnumProperty(name="变化参数", items=CURVE_PARAMETER_ITEMS, default="0")
    show_curve_keys: BoolProperty(
        name="高级关键点", description="显示曲线中间关键点的链位置和倍率", default=False)
    new_curve_key_position: FloatProperty(
        name="关键点位置", description="新关键点在骨链根部 0 到末端 1 之间的位置",
        default=0.5, min=0.0, max=1.0)
    search: StringProperty(name="筛选参数", description="参数名称，例如 gravity、damping、curve")
    show_parameters: BoolProperty(name="源数据检查（只读）", default=False)
    parameter_page: IntProperty(name="页", default=0, min=0)
    ready: BoolProperty(default=False, options={"HIDDEN"})
    disabled: BoolProperty(name="显式禁用此源组件", default=False)
    angle_limit_enabled: BoolProperty(
        name="启用角度限制", description="使用本组的角度限制曲线约束每一根可动骨段",
        get=angle_enabled_get, set=angle_enabled_set)
    angle_limit_base: FloatProperty(
        name="基础角度（°）", description="角度限制曲线乘以此基础角度后得到各骨段的限制角",
        min=0.0, max=180.0, get=angle_base_get, set=angle_base_set)
    angle_limit_stiffness: FloatProperty(
        name="限制刚度", description="原生角度限制求解的刚度",
        min=0.0, max=1.0, get=angle_stiffness_get, set=angle_stiffness_set)
    curve_enabled: BoolProperty(
        name="使用位置曲线", description="让当前参数按根部到末端的曲线倍率变化",
        get=curve_enabled_get, set=curve_enabled_set)
    context_enabled: BoolProperty(
        name="启用约束", description="启用当前曲线所属的原生约束",
        get=context_enabled_get, set=context_enabled_set)


def mapped_parameters(obj):
    data = current_record(obj)["fields"]["serializeData"]
    names = ("gravity", "stablizationTimeAfterReset", "gravityFalloff",
             "blendWeight", "animationPoseRatio")
    source.require(all(name in data and type(data[name]) in (int, float) for name in names),
                   "源物理组缺少可映射的基础参数")
    return {name: float(data[name]) for name in names}


def mapped_radius(obj):
    """Copy the native BoneCloth node-radius value and curve into author v4."""
    fields, keys = source_curve_keys(obj, NODE_RADIUS_PARAMETER)

    def integer(name, default):
        field = fields.get("curve." + name)
        return int(field.integer) if field is not None else default

    return {"value": float(native_number(obj, NODE_RADIUS_BASE_FIELD)),
            "useCurve": bool(int(fields["useCurve"].integer)), "keys": keys,
            "preInfinity": integer("m_PreInfinity", 2),
            "postInfinity": integer("m_PostInfinity", 2),
            "rotationOrder": integer("m_RotationOrder", 4)}


def native_group_objects(obj):
    return [item for item in bpy.data.objects if item.eiem_physics.kind == "NATIVE_GROUP" and
            item.eiem_native_physics.source_text == obj.eiem_native_physics.source_text and
            item.eiem_physics.rig == obj.eiem_physics.rig]


def group_root_paths(value, component, root_source):
    """Resolve one root branch while respecting the group's ignored branches."""
    by_id = {item["identity"]: item for item in value["transforms"]}
    children = {}
    for item in value["transforms"]:
        children.setdefault(item["parent"], []).append(item["identity"])
    roots = source.referenced_sources(component, source.ROOT_REFERENCE, "Transform")
    ignores = set(source.referenced_sources(component, source.IGNORE_REFERENCE, "Transform"))
    source.require(root_source in roots and root_source in by_id, "物理根节点已变化")
    result, pending = [], [root_source]
    while pending:
        identity = pending.pop()
        if identity in ignores:
            continue
        result.append(by_id[identity]["bone"])
        pending.extend(reversed(children.get(identity, [])))
    return result


def sync_native_references():
    """Keep each native Group Empty's collider list explicit after load/reload."""
    for group in [o for o in bpy.data.objects if is_native(o) and o.eiem_physics.kind == "NATIVE_GROUP"]:
        candidates = {o.eiem_native_physics.source_key: o for o in bpy.data.objects
                      if is_native(o) and o.eiem_physics.kind == "NATIVE_COLLIDER" and
                      o.eiem_physics.rig == group.eiem_physics.rig and
                      o.eiem_native_physics.source_text == group.eiem_native_physics.source_text}
        wanted = source.group_collider_sources(record(group))
        current = [ref.object.eiem_native_physics.source_key for ref in group.eiem_physics.colliders
                   if ref.object and is_native(ref.object)]
        if current == wanted:
            continue
        group.eiem_physics.colliders.clear()
        for source_key in wanted:
            if source_key in candidates:
                group.eiem_physics.colliders.add().object = candidates[source_key]


def current_group(scene):
    group = getattr(scene, "eiem_physics_group", None)
    return group if group and group.eiem_physics.kind in ("GROUP", "NATIVE_GROUP") else None


def related_objects(group):
    if group is None:
        return set()
    result = {group}
    if group.eiem_physics.kind == "GROUP":
        result.update(ref.object for ref in group.eiem_physics.colliders if ref.object)
        return result
    references = set(source.group_collider_sources(record(group)))
    result.update(obj for obj in bpy.data.objects if is_native(obj) and
                  obj.eiem_physics.rig == group.eiem_physics.rig and
                  obj.eiem_native_physics.source_text == group.eiem_native_physics.source_text and
                  obj.eiem_native_physics.source_key in references)
    return result


def apply_visibility(scene):
    """Apply an editor-only filter without changing exported Physics state."""
    mode = getattr(scene, "eiem_physics_visibility", "CURRENT")
    active = current_group(scene)
    current = related_objects(active) if mode == "CURRENT" else set()
    xray = getattr(scene, "eiem_physics_xray", False)
    for obj in scene.objects:
        if not hasattr(obj, "eiem_physics") or obj.eiem_physics.kind == "NONE":
            continue
        kind = obj.eiem_physics.kind
        show = (mode == "ALL" or
                mode == "GROUPS" and kind in ("GROUP", "NATIVE_GROUP") or
                mode == "COLLIDERS" and kind in ("COLLIDER", "NATIVE_COLLIDER") or
                mode == "CURRENT" and obj in current)
        obj.hide_set(not show)
        obj.show_in_front = xray
        for child in obj.children:
            if child.get("eiem_physics_visual"):
                child.hide_set(not show)
                child.show_in_front = xray


def visibility_updated(self, context):
    if context and context.scene:
        apply_visibility(context.scene)


def preview_style_updated(self, context):
    if not context or not context.scene or AUTHOR is None:
        return
    for obj in list(context.scene.objects):
        try:
            if is_native(obj):
                rebuild(obj)
            elif hasattr(obj, "eiem_physics") and obj.eiem_physics.kind == "GROUP":
                AUTHOR.rebuild_group(obj)
            elif hasattr(obj, "eiem_physics") and obj.eiem_physics.kind == "COLLIDER":
                AUTHOR.rebuild_collider(obj.eiem_physics, context)
        except (ValueError, KeyError, ReferenceError):
            continue
    apply_visibility(context.scene)


class EIEM_OT_native_parameter_page(bpy.types.Operator):
    bl_idname = "eiem.native_physics_parameter_page"
    bl_label = "切换原生参数页"
    delta: IntProperty(default=0)
    def execute(self, context):
        obj = context.object if context.object and context.object.eiem_physics.kind in ("GROUP", "NATIVE_GROUP") \
            else AUTHOR.group_of(context)
        if not obj or obj.eiem_physics.kind not in ("GROUP", "NATIVE_GROUP") or not len(obj.eiem_native_physics.fields):
            return {"CANCELLED"}
        p = obj.eiem_native_physics
        search = p.search.casefold()
        count = sum(search in field.label.casefold() or search in friendly_field_label(field.label).casefold()
                    for field in p.fields)
        last = max(0, (count + PARAMETER_PAGE_SIZE - 1) // PARAMETER_PAGE_SIZE - 1)
        current = min(p.parameter_page, last)
        p.parameter_page = min(last, max(0, current + self.delta))
        return {"FINISHED"}


class EIEM_OT_native_focus(bpy.types.Operator):
    bl_idname = "eiem.focus_native_physics"
    bl_label = "仅显示当前物理组"
    bl_options = {"REGISTER", "UNDO"}
    def execute(self, context):
        obj = context.object
        group = obj if obj and obj.eiem_physics.kind == "NATIVE_GROUP" else None
        if group is None and obj and obj.eiem_physics.kind == "NATIVE_COLLIDER":
            group = next((candidate for candidate in native_group_objects(obj)
                          if obj.eiem_native_physics.source_key in
                          source.group_collider_sources(record(candidate))), None)
        if group is None:
            group = current_group(context.scene)
        if group is None:
            return {"CANCELLED"}
        context.scene.eiem_physics_group = group
        context.scene.eiem_physics_visibility = "CURRENT"
        apply_visibility(context.scene)
        return {"FINISHED"}


class EIEM_OT_native_bones(bpy.types.Operator):
    bl_idname = "eiem.select_native_physics_bones"
    bl_label = "选择源物理骨骼"
    bl_options = {"REGISTER", "UNDO"}
    root_source: StringProperty()
    def execute(self, context):
        try:
            obj = context.object if is_native(context.object) else AUTHOR.group_of(context)
            source.require(obj and obj.eiem_physics.kind == "NATIVE_GROUP", "请选择源物理组")
            value, component = snapshot(obj), current_record(obj)
            paths = set(group_root_paths(value, component, self.root_source) if self.root_source else
                        source.group_transform_paths(value, component))
            rig = obj.eiem_physics.rig
            lookup = rig_paths(rig)
            source.require(paths <= set(lookup), "共享 Rig 缺少源物理骨骼")
            if context.mode != "OBJECT": bpy.ops.object.mode_set(mode="OBJECT")
            for selected in context.selected_objects: selected.select_set(False)
            rig.hide_set(False); rig.select_set(True); context.view_layer.objects.active = rig
            bpy.ops.object.mode_set(mode="POSE")
            bpy.ops.pose.select_all(action="DESELECT")
            for path in paths: rig.pose.bones[lookup[path].name].select = True
            if paths: rig.data.bones.active = lookup[next(iter(paths))]
            self.report({"INFO"}, "已选择 %d 根源物理 Transform" % len(paths))
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error)); return {"CANCELLED"}


def commit_curve_mapping(obj, parameter):
    """Commit the editable Blender projection through the canonical authoring path."""
    node = curve_mapping_node(obj, parameter)
    source.require(node is not None, "物理曲线尚未建立")
    if is_parameter_group(obj):
        apply_curve_mapping(obj, parameter)
    else:
        source.require(obj.eiem_physics.kind == "GROUP" and AUTHOR is not None,
                       "所选物理组无法编辑这条曲线")
        AUTHOR.author_curve_mapping_edited(obj, parameter, curve_mapping_keys(node))
        node[CURVE_MAPPING_SIGNATURE] = curve_mapping_signature(node)


class EIEM_OT_native_curve_key(bpy.types.Operator):
    bl_idname = "eiem.physics_curve_key"
    bl_label = "编辑物理曲线"
    bl_options = {"REGISTER", "UNDO"}

    action: StringProperty()
    parameter: StringProperty()
    index: IntProperty(default=-1)
    position: FloatProperty(default=0.5, min=0.0, max=1.0)

    def execute(self, context):
        try:
            obj = context.object
            if not obj or obj.eiem_physics.kind not in ("GROUP", "NATIVE_GROUP"):
                obj = AUTHOR.group_of(context)
            source.require(obj and obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"),
                           "请选择物理组")
            parameter = self.parameter or (active_curve_parameter(obj.eiem_native_physics)
                                             if is_parameter_group(obj) else NODE_RADIUS_PARAMETER)
            node = curve_mapping_node(obj, parameter)
            source.require(node is not None, "物理曲线尚未建立")
            if self.action in ("LINEAR", "SMOOTH"):
                set_curve_mapping_interpolation(node, self.action)
            elif self.action == "ADD":
                insert_curve_mapping_point(node, self.position)
            elif self.action == "DELETE":
                points = curve_mapping_points(node)
                source.require(0 < self.index < len(points) - 1, "只能删除中间关键点")
                remove_curve_mapping_point(node, points[self.index])
            else:
                source.require(False, "未知的曲线编辑操作")
            commit_curve_mapping(obj, parameter)
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


def import_source(filename, rig=None):
    source.require(bpy.context.mode == "OBJECT", "请回到物体模式后导入")
    filename = Path(filename).resolve()
    payload = source.read(filename) if filename.suffix == ".physics" else source.read_evidence(filename)
    source.validate(payload, filename.suffix == ".physics")
    skeleton = source_skeleton(payload)
    if filename.suffix == ".physics":
        skeleton = API["read_skeleton"](API["safe_path"](filename.parent, payload["skeleton"]))
        expected = {t["bone"] for t in source.required_transforms(payload)}
        source.require(expected <= {n[0] for n in skeleton["nodes"]}, "Physics 的 Skeleton 依赖缺少源节点")
    existing = {o.eiem_native_physics.source_key for o in bpy.data.objects if is_native(o) and o.eiem_physics.rig == rig}
    source.require(not existing.intersection(c["source"] for c in payload["components"]), "当前 Rig 已有这些物理组件")
    kinds = ("objects", "curves", "meshes", "armatures", "collections", "texts", "materials", "node_groups")
    before = {kind: set(getattr(bpy.data, kind)) for kind in kinds}
    old_group = bpy.context.scene.eiem_physics_group
    bone_ids = {b.name: b.get("eiem_physics_id") for b in rig.data.bones} if rig else {}
    existing_rig = rig
    extended_bones = []
    try:
        if rig is None:
            rig = API["make_armature"]("SkeletonPhysics", skeleton, bpy.context.scene.collection)
        else:
            extended_bones = extend_rig(payload, rig)
        validate_rig(payload, rig)
        if existing_rig and filename.suffix == ".physics":
            source.require(AUTHOR.skeleton_equal(API["skeleton_author_nodes"](rig), skeleton), "当前 Rig 与 Physics 骨架依赖不一致")
        lookup = rig_paths(rig)
        text = bpy.data.texts.new("EIEM Physics Source " + payload["id"])
        # Blender Text insertion has quadratic cost for a multi-megabyte line.
        # Keep the retained tree formatted into ordinary-sized source lines.
        text.from_string(json.dumps(payload, ensure_ascii=False, allow_nan=False, indent=1))
        text.use_fake_user = True
        remember_snapshot(text, payload)
        made = []
        for c in payload["components"]:
            kind = "NATIVE_GROUP" if c["type"] == "BeyondBoneCloth" else "NATIVE_COLLIDER"
            obj = AUTHOR.new_helper(c["name"], rig, kind)
            obj.eiem_physics.identity = c["id"]
            obj["eiem_physics_source_file"] = str(filename)
            p = obj.eiem_native_physics
            p.source_text, p.source_key = text, c["source"]
            p.disabled = c["operation"] == "disable"
            AUTHOR.bind_collider(obj, lookup[c["bone"]])
            obj.show_in_front = True; obj.hide_render = True
            editable = c["fields"]["serializeData"] if kind == "NATIVE_GROUP" else {
                k:v for k,v in c["fields"].items() if k not in ("m_GameObject", "m_Script", "m_Name")}
            field_types = source.schema_types(c["schema"])
            for path, value in source.numeric_fields(editable):
                path = (("serializeData",)+path) if kind == "NATIVE_GROUP" else path
                item = p.fields.add(); item.path = source.dumps(path)
                item.label = ".".join(str(x) for x in path)
                primitive = field_types.get(tuple("*" if isinstance(x,int) else x for x in path))
                item.original = source.dumps(value); item.floating = primitive in ("float", "double") or type(value) is float
                if item.floating: item.value = value
                else: item.integer = str(int(value))
            p.ready = True
            if kind == "NATIVE_GROUP":
                build_curve_mappings(obj)
            rebuild(obj); made.append(obj)
        groups = [o for o in made if o.eiem_physics.kind == "NATIVE_GROUP"]
        colliders = {o.eiem_native_physics.source_key: o for o in made
                     if o.eiem_physics.kind == "NATIVE_COLLIDER"}
        for group in groups:
            group.eiem_physics.colliders.clear()
            for source_key in source.group_collider_sources(record(group)):
                source.require(source_key in colliders, "源物理组引用的碰撞体未导入")
                group.eiem_physics.colliders.add().object = colliders[source_key]
        if groups: bpy.context.scene.eiem_physics_group = groups[0]
        apply_visibility(bpy.context.scene)
        bpy.context.view_layer.update()
        return rig, made
    except Exception:
        bpy.context.scene.eiem_physics_group = old_group
        for kind in kinds:
            for item in set(getattr(bpy.data, kind)) - before[kind]: getattr(bpy.data, kind).remove(item, do_unlink=True)
        if existing_rig:
            remove_extended_bones(existing_rig, extended_bones)
            for b in existing_rig.data.bones:
                if bone_ids[b.name] is None:
                    if "eiem_physics_id" in b: del b["eiem_physics_id"]
                else: b["eiem_physics_id"] = bone_ids[b.name]
        raise


def export_source(filename, objects):
    objects = list(objects)
    source.require(bool(objects) and all(is_native(o) for o in objects), "请选择源物理组或碰撞体")
    rig = objects[0].eiem_physics.rig
    text = objects[0].eiem_native_physics.source_text
    source.require(all(o.eiem_physics.rig == rig and o.eiem_native_physics.source_text == text for o in objects),
                   "一次导出同一来源和同一 Rig 的物理组件")
    filename = Path(filename).resolve()
    source.require(filename.suffix == ".physics", "输出文件必须为 .physics")
    source.require(all(Path(o["eiem_physics_source_file"]).resolve() != filename for o in objects), "请选择新文件，不覆盖源包")
    for obj in objects:
        if obj.eiem_physics.kind == "NATIVE_GROUP":
            build_curve_mappings(obj)
        action = obj.animation_data.action if obj.animation_data else None
        if obj.eiem_physics.kind == "NATIVE_GROUP" and action and \
                action.get(CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key and \
                action.get(CURVE_ACTION_SIGNATURE, "") != curve_projection_signature(obj):
            apply_curve_projection(obj)
    value = source.closure(snapshot(objects[0]), [o.eiem_native_physics.source_key for o in objects])
    by_source = {o.eiem_native_physics.source_key:o for o in bpy.data.objects if is_native(o) and
                 o.eiem_physics.rig == rig and o.eiem_native_physics.source_text == text}
    validate_rig(value, rig)
    for i, c in enumerate(value["components"]):
        source.require(c["source"] in by_source, "共享碰撞体已删除，请恢复引用组件")
        obj = by_source[c["source"]]
        edited = current_record(obj)
        basis = API["unity_transform_matrix_to_blender_basis"]()
        offset = basis.inverted() @ obj.matrix_basis @ basis
        source.require(max(abs(offset[r][col] - Matrix.Identity(4)[r][col]) for r in range(3) for col in range(3)) < 1e-6,
                       "源碰撞体方向使用 direction 参数编辑；尺寸使用 size 参数编辑")
        if edited["type"] != "BeyondBoneCloth":
            for axis, amount in zip("xyz", offset.translation):
                if abs(amount) > 1e-7: edited["fields"]["center"][axis] += float(amount)
        else:
            source.require(offset.translation.length < 1e-6, "源物理组的锚点不能整体平移")
        value["components"][i] = edited
    with tempfile.TemporaryDirectory(prefix="eiem-native-physics-") as temporary:
        skeleton = Path(temporary) / "shared.skeleton"
        API["write_skeleton"](skeleton, rig); data = skeleton.read_bytes()
        value["skeleton"] = "skeletons/" + hashlib.sha256(data).hexdigest() + ".skeleton"
        encoded = source.encode(value)
        filename.parent.mkdir(parents=True, exist_ok=True)
        dependency = API["safe_path"](filename.parent, value["skeleton"])
        dependency.parent.mkdir(exist_ok=True)
        AUTHOR.atomic_write(dependency, data); AUTHOR.atomic_write(filename, encoded)
    return {"groups": sum(c["type"] == "BeyondBoneCloth" for c in value["components"]),
            "colliders": sum(c["type"] != "BeyondBoneCloth" for c in value["components"]), "skeletons": 1}


class EIEM_OT_native_import(ImportHelper, bpy.types.Operator):
    bl_idname = "eiem.import_native_physics"
    bl_label = "导入源物理"
    bl_options = {"REGISTER", "UNDO"}
    filter_glob: StringProperty(default="components.json;*.physics", options={"HIDDEN"})
    def execute(self, context):
        try:
            rig = AUTHOR.rig_of(context.object)
            _, objects = import_source(self.filepath, rig)
            self.report({"INFO"}, "已导入 %d 个源物理组件" % len(objects)); return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error)); return {"CANCELLED"}


class EIEM_OT_native_select(bpy.types.Operator):
    bl_idname = "eiem.select_native_physics"
    bl_label = "选择关联物理"
    bl_options = {"REGISTER", "UNDO"}
    target: StringProperty()
    def execute(self, context):
        group = AUTHOR.group_of(context)
        if not is_native(group): return {"CANCELLED"}
        c = record(group)
        keys = set(source.group_collider_sources(c)) if not self.target else {self.target}
        for obj in context.selected_objects: obj.select_set(False)
        for obj in context.scene.objects:
            if is_native(obj) and obj.eiem_physics.rig == group.eiem_physics.rig and obj.eiem_native_physics.source_key in keys:
                obj.hide_set(False); obj.select_set(True); context.view_layer.objects.active = obj
        return {"FINISHED"}


def draw_curve_distribution(layout, obj, parameter, enabled=True):
    """Draw the common endpoints and an optional numeric key editor."""
    node = curve_mapping_node(obj, parameter)
    if node is None:
        layout.label(text="曲线数据尚未建立", icon="ERROR")
        return
    state = obj.eiem_native_physics
    points = curve_mapping_points(node)
    root, tip = curve_mapping_endpoints(node)
    controls = layout.column()
    values = controls.column()
    values.enabled = bool(enabled)
    values.prop(root, "location", index=1, text="根部倍率")
    values.prop(tip, "location", index=1, text="末端倍率")
    style = values.row(align=True)
    style.label(text="变化方式")
    linear = curve_mapping_is_linear(node)
    op = style.operator("eiem.physics_curve_key", text="线性", depress=linear)
    op.action, op.parameter = "LINEAR", parameter
    op = style.operator("eiem.physics_curve_key", text="平滑", depress=not linear)
    op.action, op.parameter = "SMOOTH", parameter
    if len(points) > 2:
        controls.label(text="另有 %d 个中间关键点" % (len(points) - 2), icon="KEY_HLT")
    controls.prop(
        state, "show_curve_keys",
        text="高级关键点", toggle=True,
        icon="TRIA_DOWN" if state.show_curve_keys else "TRIA_RIGHT")
    if state.show_curve_keys:
        editor = controls.column()
        editor.enabled = bool(enabled)
        keys = editor.box()
        keys.label(text="链位置 0 为根端，1 为末端")
        for index, point in enumerate(points):
            point_box = keys.box()
            header = point_box.row(align=True)
            kind = "根端" if index == 0 else "末端" if index == len(points) - 1 else "中间"
            header.label(text="%s关键点 %d" % (kind, index + 1), icon="KEY_HLT")
            if 0 < index < len(points) - 1:
                op = header.operator("eiem.physics_curve_key", text="", icon="X")
                op.action, op.parameter, op.index = "DELETE", parameter, index
            values = point_box.row(align=True)
            if 0 < index < len(points) - 1:
                values.prop(point, "location", index=0, text="链位置")
            else:
                values.label(text="链位置 %.4g" % point.location.x)
            values.prop(point, "location", index=1, text="倍率")
        add = editor.row(align=True)
        add.prop(state, "new_curve_key_position", text="新关键点位置")
        op = add.operator("eiem.physics_curve_key", text="添加", icon="ADD")
        op.action, op.parameter, op.position = "ADD", parameter, state.new_curve_key_position


def draw_group_parameter_controls(layout, obj):
    """Draw the one supported day-to-day editor in a fixed order."""
    p = obj.eiem_native_physics
    common = {field.label: field for field in p.fields}
    box = layout.box(); box.label(text="物理组参数", icon="PREFERENCES")
    for key, label in COMMON_PARAMETERS:
        field = common.get(key)
        if field:
            API["draw_eiem_rna_property"](
                box, field, "value" if field.floating else "integer", label, factor=.42)

    curve_box = layout.box(); curve_box.label(text="沿骨链位置变化", icon="FCURVE")
    curve_box.prop(p, "active_curve", text="参数")
    index = min(max(int(p.active_curve), 0), len(CURVE_PARAMETERS) - 1)
    parameter, label = CURVE_PARAMETERS[index]
    base = common.get(parameter + ".value")
    enabled = common.get(parameter + ".useCurve")
    if base:
        API["draw_eiem_rna_property"](
            curve_box, base, "value" if base.floating else "integer", "基础值", factor=.42)
    if enabled:
        curve_box.prop(p, "curve_enabled")
    for path, extra_label in CURVE_CONTEXT_FIELDS.get(parameter, ()):
        field = common.get(path)
        if field:
            if path.rsplit(".", 1)[-1].startswith("use"):
                curve_box.prop(p, "context_enabled", text=extra_label)
            else:
                API["draw_eiem_rna_property"](
                    curve_box, field, "value" if field.floating else "integer", extra_label, factor=.42)
    node = curve_mapping_node(obj, parameter)
    if node is not None:
        draw_curve_distribution(
            curve_box, obj, parameter,
            bool(p.curve_enabled) if enabled else True)
    else:
        curve_box.label(text="曲线数据尚未建立，请重新选择此物理组", icon="ERROR")
    if parameter == ANGLE_CURVE_PARAMETER and bool(native_number(obj, ANGLE_ENABLED_FIELD)):
        curve_box.label(text="黄色锥体显示每段当前允许角度", icon="MESH_CONE")
        preview = str(obj.get("eiem_physics_angle_preview", ""))
        if preview.startswith("error:"):
            curve_box.label(text=preview[6:], icon="ERROR")


def draw_parameter_fields(layout, obj, source_type, allow_refresh=False, include_disabled=False):
    p = obj.eiem_native_physics
    layout.prop(p, "show_parameters", icon="TRIA_DOWN" if p.show_parameters else "TRIA_RIGHT", emboss=False)
    if not p.show_parameters:
        return
    layout.label(text="源类型：" + source_type)
    layout.label(text="复制/粘贴会一次填充这些保留值；日常无需逐项输入", icon="INFO")
    layout.prop(p, "search", icon="VIEWZOOM")
    search = p.search.casefold()
    fields = sorted((field for field in p.fields if search in field.label.casefold() or
                     search in friendly_field_label(field.label).casefold()),
                    key=lambda field: natural_path_key(field.label))
    pages = max(1, (len(fields) + PARAMETER_PAGE_SIZE - 1) // PARAMETER_PAGE_SIZE)
    page = min(p.parameter_page, pages - 1)
    row = layout.row(align=True)
    previous = row.operator("eiem.native_physics_parameter_page", text="", icon="TRIA_LEFT")
    previous.delta = -1
    row.label(text="第 %d/%d 页 · %d 项" % (page + 1, pages, len(fields)))
    following = row.operator("eiem.native_physics_parameter_page", text="", icon="TRIA_RIGHT")
    following.delta = 1
    for field in fields[page * PARAMETER_PAGE_SIZE:(page + 1) * PARAMETER_PAGE_SIZE]:
        row = layout.row(align=True)
        row.label(text=friendly_field_label(field.label))
        value = ("%.7g" % float(field.value)) if field.floating else str(field.integer)
        trailing = row.row()
        trailing.alignment = "RIGHT"
        trailing.label(text=value)


def draw(layout, context, obj):
    c = record(obj); p = obj.eiem_native_physics
    layout.prop(obj.eiem_physics, "label")
    layout.label(text=("物理组（可包含多条骨链）" if c["type"] == "BeyondBoneCloth" else "共享碰撞体"), icon="OUTLINER_OB_EMPTY")
    layout.label(text="绑定骨骼：" + (c.get("bone") or "Prefab 根"))
    layout.prop(p, "disabled")
    if c["type"] == "BeyondBoneCloth":
        sel = c["fields"]["serializeData2"]["selectionData"]
        value = snapshot(obj)
        bones = source.group_transform_paths(value, c)
        roots = source.referenced_sources(c, source.ROOT_REFERENCE, "Transform")
        collider_ids = source.group_collider_sources(c)
        layout.label(text="%d 个根节点 · %d 根物理 Transform · %d 个选择点" %
                     (len(roots), len(bones), len(sel["positions"])))
        layout.label(text="锥体按静止位置匹配 Transform；不把数组序号当作骨骼编号", icon="INFO")
        layout.label(text="绿色层级连线是物理链方向；bone tail 是源 Transform 局部 +Y 轴", icon="INFO")
        layout.label(text="%d 个关联碰撞体 · 橙色固定 / 蓝色运动 / 灰色忽略" % len(collider_ids))
        draw_group_parameter_controls(layout, obj)
        by_id = {item["identity"]: item for item in value["transforms"]}
        chain_box = layout.box(); chain_box.label(text="组内根链（共享本组参数）", icon="BONE_DATA")
        for index, root_source in enumerate(roots):
            row = chain_box.row(align=True)
            path = by_id[root_source]["bone"] if root_source in by_id else "根节点已丢失"
            row.label(text="%d. %s" % (index + 1, path.rsplit("/", 1)[-1] or "Prefab 根"))
            operator = row.operator("eiem.select_native_physics_bones", text="选择此链", icon="RESTRICT_SELECT_OFF")
            operator.root_source = root_source
        collider_box = layout.box(); collider_box.label(text="本组使用的碰撞体", icon="MESH_UVSPHERE")
        referenced = {ref.object.eiem_native_physics.source_key: ref.object for ref in obj.eiem_physics.colliders
                      if ref.object and is_native(ref.object)}
        all_colliders = {item.eiem_native_physics.source_key: item for item in bpy.data.objects
                         if is_native(item) and item.eiem_physics.kind == "NATIVE_COLLIDER" and
                         item.eiem_physics.rig == obj.eiem_physics.rig and
                         item.eiem_native_physics.source_text == obj.eiem_native_physics.source_text}
        for source_key in collider_ids:
            collider = referenced.get(source_key) or all_colliders.get(source_key)
            row = collider_box.row(align=True)
            if collider:
                geometry = source.collider_geometry(current_record(collider))
                shape = {"SPHERE":"球", "CAPSULE":"胶囊", "PLANE":"平面"}[geometry["shape"]]
                row.label(text="%s · %s" % (shape, collider.name))
                operator = row.operator("eiem.select_native_physics", text="选择", icon="RESTRICT_SELECT_OFF")
                operator.target = source_key
            else:
                row.label(text="碰撞体引用已丢失", icon="ERROR")
    else:
        geometry = source.collider_geometry(current_record(obj))
        users = []
        for group in native_group_objects(obj):
            if c["source"] in source.group_collider_sources(record(group)): users.append(group.name)
        shape = {"SPHERE":"球体", "CAPSULE":"胶囊", "PLANE":"无限平面"}[geometry["shape"]]
        layout.label(text="原生外形：%s · 被 %d 个物理组引用" % (shape, len(users)), icon="INFO")
        if geometry["shape"] == "CAPSULE":
            layout.label(text="外部长 %.4g · 两端球心距 %.4g" %
                         (geometry["length"], geometry["segmentLength"]))
            axis = "XYZ"[geometry["axis"]] + ("（反向）" if geometry["reverse"] else "")
            layout.label(text="局部 %s：Start 在正端，End 在负端" % axis, icon="ORIENTATION_LOCAL")
        if users: layout.label(text="、".join(users[:4]) + ("…" if len(users) > 4 else ""))
        layout.prop(obj, "location", text="中心偏移")
        for field in p.fields:
            if field.label.startswith(("center.", "size.")) or field.label in ("direction", "reverseDirection", "radiusSeparation", "alignedOnCenter"):
                API["draw_eiem_rna_property"](layout, field, "value" if field.floating else "integer", field.label)
    draw_parameter_fields(layout, obj, c["type"],
                          allow_refresh=c["type"] == "BeyondBoneCloth", include_disabled=True)


CLASSES = (EIEM_PG_native_field, EIEM_PG_native_physics, EIEM_OT_native_import,
           EIEM_OT_native_select,
           EIEM_OT_native_parameter_page,
           EIEM_OT_native_focus, EIEM_OT_native_bones, EIEM_OT_native_curve_key)


def register(author, api):
    global AUTHOR
    AUTHOR = author; API.update(api)
    for cls in CLASSES: bpy.utils.register_class(cls)
    bpy.types.Object.eiem_native_physics = PointerProperty(type=EIEM_PG_native_physics)
    bpy.types.Scene.eiem_physics_visibility = EnumProperty(
        name="视图范围",
        items=(("CURRENT", "当前组", "显示当前物理组及它引用的碰撞体"),
               ("GROUPS", "全部物理组", "只显示所有物理组"),
               ("COLLIDERS", "全部碰撞体", "只显示所有碰撞体"),
               ("ALL", "全部", "显示所有物理辅助对象"),
               ("NONE", "隐藏", "隐藏所有物理辅助对象")),
        default="CURRENT", update=visibility_updated)
    bpy.types.Scene.eiem_physics_preview_style = EnumProperty(
        name="预览样式",
        items=(("SOLID", "实体", "实体节点、连接体和半透明碰撞壳"),
               ("WIREFRAME", "线框", "诊断用圆环和轮廓线")),
        default="SOLID", update=preview_style_updated)
    bpy.types.Scene.eiem_physics_xray = BoolProperty(
        name="穿透模型显示", default=False,
        description="让物理骨链和碰撞体显示在 Mesh 前方", update=visibility_updated)
    # During add-on registration Blender may expose _RestrictData, which has no
    # objects collection. A short timer performs saved-file migration once
    # normal scene data becomes available.
    if hasattr(bpy.data, "objects"):
        rebuild_existing_native()
    elif not bpy.app.timers.is_registered(rebuild_existing_native):
        bpy.app.timers.register(rebuild_existing_native, first_interval=.01)


def unregister():
    if bpy.app.timers.is_registered(flush_group_previews):
        bpy.app.timers.unregister(flush_group_previews)
    if bpy.app.timers.is_registered(rebuild_existing_native):
        bpy.app.timers.unregister(rebuild_existing_native)
    if bpy.app.timers.is_registered(poll_curve_previews):
        bpy.app.timers.unregister(poll_curve_previews)
    _GROUP_PREVIEW_DIRTY.clear()
    _SOURCE_CACHE.clear()
    del bpy.types.Scene.eiem_physics_preview_style
    del bpy.types.Scene.eiem_physics_xray
    del bpy.types.Scene.eiem_physics_visibility
    del bpy.types.Object.eiem_native_physics
    for cls in reversed(CLASSES): bpy.utils.unregister_class(cls)
    API.clear()
