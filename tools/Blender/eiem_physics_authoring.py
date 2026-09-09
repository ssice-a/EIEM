"""Blender authoring for new Physics groups. Native application is separate."""
import hashlib
import importlib.util
import json
import os
import tempfile
import uuid
from pathlib import Path

import bpy
from bpy.props import CollectionProperty, EnumProperty, FloatProperty, PointerProperty, StringProperty
from bpy_extras.io_utils import ExportHelper, ImportHelper
from mathutils import Matrix, Quaternion, Vector

if __package__:
    from . import eiem_physics_document as document
    from . import eiem_physics_native as native
else:
    _spec = importlib.util.spec_from_file_location("eiem_physics_document", Path(__file__).with_name("eiem_physics_document.py"))
    document = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(document)
    _spec = importlib.util.spec_from_file_location("eiem_physics_native", Path(__file__).with_name("eiem_physics_native.py"))
    native = importlib.util.module_from_spec(_spec)
    _spec.loader.exec_module(native)

API = {}
_DIRTY_ARMATURES = set()
_REBUILDING_GROUPS = False
_MSGBUS_OWNER = object()
AUTHOR_RADIUS_PROPERTY = "物理曲线 节点半径"
AUTHOR_CURVE_ACTION_MARKER = "eiem_author_physics_radius"
AUTHOR_CURVE_SIGNATURE = "eiem_author_physics_radius_signature"
AUTHOR_CURVE_METADATA = "eiem_author_physics_radius_metadata"
PARAMETER_CLIPBOARD_VERSION = 2


def rig_of(obj):
    if obj and obj.type == "ARMATURE" and obj.get("eiem_section"):
        return obj
    return obj.eiem_physics.rig if obj and hasattr(obj, "eiem_physics") else None


def group_of(context):
    obj = context.object
    if obj and obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"):
        return obj
    return context.scene.eiem_physics_group


def physics_group_updated(self, context):
    if context and context.scene:
        native.apply_visibility(context.scene)


def active_object_updated():
    """Make a selected group Empty become the current group automatically."""
    scene = getattr(bpy.context, "scene", None)
    view_layer = getattr(bpy.context, "view_layer", None)
    obj = view_layer.objects.active if view_layer else None
    if scene and obj and hasattr(obj, "eiem_physics") and \
            obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP") and \
            scene.eiem_physics_group != obj:
        scene.eiem_physics_group = obj


def node_role_updated(self, context):
    obj = self.id_data
    if isinstance(obj, bpy.types.Object) and obj.eiem_physics.kind == "GROUP":
        rebuild_group(obj)


def author_shared_parameter_updated(self, context):
    obj = self.id_data
    if isinstance(obj, bpy.types.Object) and obj.eiem_physics.kind == "GROUP" and \
            hasattr(obj, "eiem_native_physics") and obj.eiem_native_physics.ready and \
            len(obj.eiem_native_physics.fields):
        sync_author_direct_fields(obj)


def author_group_parameter_updated(self, context):
    obj = self.id_data
    if isinstance(obj, bpy.types.Object) and obj.eiem_physics.kind == "GROUP":
        if hasattr(obj, "eiem_native_physics") and len(obj.eiem_native_physics.fields):
            if not obj.eiem_native_physics.ready:
                return
            sync_author_radius_controls_to_native_fields(obj)
            rebuild_group(obj)
            return
        try:
            radius = json.loads(obj.eiem_physics.radius_curve_snapshot)
        except (TypeError, ValueError, json.JSONDecodeError):
            radius = document.default_radius()
        radius["value"] = float(obj.eiem_physics.node_radius)
        radius["useCurve"] = bool(obj.eiem_physics.radius_use_curve)
        obj.eiem_physics.radius_curve_snapshot = json.dumps(
            radius, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
        rebuild_group(obj)


def collider_candidate_poll(self, obj):
    owner = self.id_data
    return bool(isinstance(owner, bpy.types.Object) and self.kind == "GROUP" and obj and
                obj != owner and
                obj.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER") and
                obj.eiem_physics.rig == self.rig and
                not any(ref.object == obj for ref in self.colliders))


def author_radius_curve(obj):
    action = obj.animation_data.action if obj and obj.animation_data else None
    if action and hasattr(obj, "eiem_native_physics") and len(obj.eiem_native_physics.fields) and \
            action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key:
        index = native.CURVE_PARAMETER_INDEX[native.NODE_RADIUS_PARAMETER]
        return native.find_action_curve(
            obj, native.property_data_path(native.curve_property(index, native.CURVE_PARAMETERS[index][1])))
    if not action or action.get(AUTHOR_CURVE_ACTION_MARKER) != obj.eiem_physics.identity:
        return None
    return native.find_action_curve(obj, native.property_data_path(AUTHOR_RADIUS_PROPERTY))


def author_curve_signature(obj):
    curve = author_radius_curve(obj)
    if curve is None:
        return ""
    return json.dumps({"mute": bool(curve.mute), "keys": json.loads(author_curve_geometry_signature(curve))},
        separators=(",", ":"), allow_nan=False)


def author_curve_geometry_signature(curve):
    return json.dumps([
        [float(point.co.x), float(point.co.y),
         float(point.handle_left.x), float(point.handle_left.y),
         float(point.handle_right.x), float(point.handle_right.y),
         point.interpolation, point.handle_left_type, point.handle_right_type]
        for point in sorted(curve.keyframe_points, key=lambda item: item.co.x)],
        separators=(",", ":"), allow_nan=False)


def author_curve_keys(obj):
    curve = author_radius_curve(obj)
    document.require(curve is not None, "节点半径 Blender 曲线已丢失")
    points = sorted(curve.keyframe_points, key=lambda item: item.co.x)
    document.require(2 <= len(points) <= document.MAX_CURVE_KEYS,
                     "节点半径曲线需要 2..%d 个关键帧" % document.MAX_CURVE_KEYS)
    times = [float(point.co.x) / native.CURVE_FRAME_SCALE for point in points]
    document.require(all(0.0 <= value <= 1.0 for value in times) and
                     all(a < b for a, b in zip(times, times[1:])),
                     "节点半径关键帧必须位于横轴 0～100，位置严格递增")
    action = obj.animation_data.action
    try:
        metadata = json.loads(str(action.get(AUTHOR_CURVE_METADATA, "{}")))
    except (TypeError, ValueError):
        metadata = {}
    if metadata.get("geometrySignature") == author_curve_geometry_signature(curve) and \
            isinstance(metadata.get("keys"), list) and len(metadata["keys"]) == len(points):
        return [{name: value for name, value in key.items()} for key in metadata["keys"]]
    original_modes = metadata.get("weightedModes", [])
    result = []
    for index, (point, time) in enumerate(zip(points, times)):
        left_dx = (float(point.co.x) - float(point.handle_left.x)) / native.CURVE_FRAME_SCALE
        right_dx = (float(point.handle_right.x) - float(point.co.x)) / native.CURVE_FRAME_SCALE
        left_interval = time - times[index - 1] if index else times[1] - times[0]
        right_interval = times[index + 1] - time if index + 1 < len(times) else left_interval
        in_weight = left_dx / left_interval if left_interval > 1e-8 else 1 / 3
        out_weight = right_dx / right_interval if right_interval > 1e-8 else 1 / 3
        old_mode = int(original_modes[index]) if index < len(original_modes) else 0
        mode = ((1 if old_mode & 1 or abs(in_weight - 1 / 3) > 1e-5 else 0) |
                (2 if old_mode & 2 or abs(out_weight - 1 / 3) > 1e-5 else 0))
        result.append({"time": time, "value": float(point.co.y),
            "inSlope": ((float(point.co.y) - float(point.handle_left.y)) / left_dx
                        if left_dx > 1e-8 else 0.0),
            "outSlope": ((float(point.handle_right.y) - float(point.co.y)) / right_dx
                         if right_dx > 1e-8 else 0.0),
            "weightedMode": mode, "inWeight": max(0.0, min(1.0, in_weight)),
            "outWeight": max(0.0, min(1.0, out_weight))})
    return result


def author_radius_record(obj):
    node = native.curve_mapping_node(obj, native.NODE_RADIUS_PARAMETER)
    if node is not None and str(node.get(native.CURVE_MAPPING_SIGNATURE, "")) != \
            native.curve_mapping_signature(node):
        if native.is_parameter_group(obj):
            native.apply_curve_mapping(obj, native.NODE_RADIUS_PARAMETER)
        else:
            author_curve_mapping_edited(obj, native.NODE_RADIUS_PARAMETER,
                                        native.curve_mapping_keys(node))
            node[native.CURVE_MAPPING_SIGNATURE] = native.curve_mapping_signature(node)
    if native.is_parameter_group(obj):
        action = obj.animation_data.action if obj and obj.animation_data else None
        if action and action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key and \
                action.get(native.CURVE_ACTION_SIGNATURE, "") != native.curve_projection_signature(obj):
            native.apply_curve_projection(obj)
        return native.mapped_radius(obj)

    saved = obj.eiem_physics.radius_curve_snapshot
    if saved:
        try:
            result = json.loads(saved)
        except (TypeError, ValueError, json.JSONDecodeError):
            raise ValueError("Physics: 节点半径曲线数据损坏")
        result["value"] = float(obj.eiem_physics.node_radius)
        result["useCurve"] = bool(obj.eiem_physics.radius_use_curve)
        return result

    # Compatibility path for .blend files authored before the inline curve UI.
    action = obj.animation_data.action if obj and obj.animation_data else None
    curve = author_radius_curve(obj)
    result = document.default_radius()
    result["value"] = float(obj.eiem_physics.node_radius)
    result["useCurve"] = not curve.mute if curve is not None else bool(obj.eiem_physics.radius_use_curve)
    if curve is not None:
        result["keys"] = author_curve_keys(obj)
        try:
            metadata = json.loads(str(obj.animation_data.action.get(AUTHOR_CURVE_METADATA, "{}")))
        except (TypeError, ValueError):
            metadata = {}
        for key in ("preInfinity", "postInfinity", "rotationOrder"):
            if key in metadata:
                result[key] = int(metadata[key])
    return result


def load_author_radius(obj, radius):
    """Load the author radius into the private inline Float Curve."""
    action = obj.animation_data.action if obj.animation_data else None
    if action is not None:
        owned = (action.get(AUTHOR_CURVE_ACTION_MARKER) == obj.eiem_physics.identity or
                 hasattr(obj, "eiem_native_physics") and
                 action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key)
        document.require(owned, "该物理组已有非 EIEM 动画，请先移除或转移该 Action")
        native.remove_curve_projection(obj)
        obj.animation_data_clear()
    if AUTHOR_RADIUS_PROPERTY in obj:
        del obj[AUTHOR_RADIUS_PROPERTY]
    state = obj.eiem_native_physics if hasattr(obj, "eiem_native_physics") else None
    previous_ready = bool(state.ready) if state else False
    if state:
        state.ready = False
    try:
        obj.eiem_physics.node_radius = float(radius["value"])
        obj.eiem_physics.radius_use_curve = bool(radius["useCurve"])
        obj.eiem_physics.radius_curve_snapshot = json.dumps(
            radius, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
        node = native.load_curve_mapping(obj, native.NODE_RADIUS_PARAMETER, radius["keys"])
    finally:
        if state:
            state.ready = previous_ready
    native.ensure_curve_preview_timer()
    return node


def author_curve_mapping_edited(obj, parameter, keys):
    """Commit an inline curve for a minimal author group."""
    document.require(parameter == native.NODE_RADIUS_PARAMETER and
                     obj.eiem_physics.kind == "GROUP", "作者组不支持此曲线")
    try:
        radius = json.loads(obj.eiem_physics.radius_curve_snapshot)
    except (TypeError, ValueError, json.JSONDecodeError):
        radius = document.default_radius()
    radius["value"] = float(obj.eiem_physics.node_radius)
    radius["useCurve"] = bool(obj.eiem_physics.radius_use_curve)
    radius["keys"] = keys
    obj.eiem_physics.radius_curve_snapshot = json.dumps(
        radius, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
    rebuild_group(obj)


def rebuild_existing_author_groups():
    """Upgrade saved pre-v3 helpers after add-on registration completes."""
    if not hasattr(bpy.data, "objects"):
        return .05
    found = False
    # Rebuilding a group replaces its preview Objects. Capture only stable
    # owner Empties before mutating Blender's live object collection; a plain
    # snapshot would still contain preview wrappers removed by an earlier group.
    groups = [obj for obj in bpy.data.objects
              if hasattr(obj, "eiem_physics") and obj.eiem_physics.kind == "GROUP"]
    for obj in groups:
        found = True
        if native.curve_mapping_node(obj, native.NODE_RADIUS_PARAMETER) is None:
            radius = author_radius_record(obj) if author_radius_curve(obj) is not None or \
                native.is_parameter_group(obj) else document.default_radius()
            load_author_radius(obj, radius)
        ensure_author_native_fields(obj)
        if native.is_parameter_group(obj):
            activate_author_full_projection(obj)
        rebuild_group(obj)
    if found:
        native.ensure_curve_preview_timer()
    return None


def collection(rig, category=None):
    """Return the Physics root or one of its role collections for a Rig.

    A package may contain many shared or referenced objects, so collection
    hierarchy is reserved for navigation. Object parenting and explicit source
    references remain the data model.
    """
    key = rig.get("eiem_physics_collection_id")
    if not key:
        key = rig["eiem_physics_collection_id"] = uuid.uuid4().hex
    found = next((c for c in bpy.data.collections
                  if c.get("eiem_physics_collection_id") == key and
                  c.get("eiem_collection_role") == "PHYSICS"), None)
    # Upgrade an older .blend collection in place when possible.
    if found is None:
        found = next((c for c in bpy.data.collections
                      if c.get("eiem_physics_rig") == rig.name_full), None)
    if found is None:
        found = bpy.data.collections.new(rig.name + " Physics")
        found["eiem_physics_rig"] = rig.name_full
        found["eiem_physics_collection_id"] = key
        found["eiem_collection_role"] = "PHYSICS"
        import_id = rig.get("eiem_import_id")
        package = next((c for c in bpy.data.collections
                        if import_id and c.get("eiem_import_id") == import_id and
                        c.get("eiem_collection_role") == "PACKAGE"), None)
        (package or bpy.context.scene.collection).children.link(found)
    else:
        found["eiem_physics_collection_id"] = key
        found["eiem_collection_role"] = "PHYSICS"
    if category is None:
        return found
    child = next((c for c in found.children if c.get("eiem_physics_category") == category), None)
    if child is None:
        labels = {"GROUPS": "Groups", "COLLIDERS": "Colliders", "VISUALS": "Visuals"}
        child = bpy.data.collections.new(found.name + " " + labels[category])
        child["eiem_collection_role"] = "PHYSICS_" + category
        child["eiem_physics_category"] = category
        child["eiem_physics_collection_id"] = key
        found.children.link(child)
    return child


def organize_rig(rig):
    """Move saved helpers from the legacy flat collection into role folders."""
    root = collection(rig)
    key = root["eiem_physics_collection_id"]
    groups = collection(rig, "GROUPS")
    colliders = collection(rig, "COLLIDERS")
    visuals = collection(rig, "VISUALS")
    for obj in list(bpy.data.objects):
        target = None
        if obj.get("eiem_physics_visual") and obj.parent and obj.parent.eiem_physics.rig == rig:
            target = visuals
        elif hasattr(obj, "eiem_physics") and obj.eiem_physics.rig == rig:
            if obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"):
                target = groups
            elif obj.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER"):
                target = colliders
        if target is None:
            continue
        if obj.name not in target.objects:
            target.objects.link(obj)
        for linked in list(obj.users_collection):
            if linked == target:
                continue
            if linked == root or linked.get("eiem_physics_collection_id") == key:
                linked.objects.unlink(obj)
    return root


def bone_id(bone):
    if not bone.get("eiem_physics_id"):
        bone["eiem_physics_id"] = uuid.uuid4().hex
    return str(bone["eiem_physics_id"])


def bones(rig):
    document.require(rig and rig.type == "ARMATURE" and rig.get("eiem_section"), "请选择 EIEM 共享骨架")
    result = {}
    for bone, record, source in API["skeleton_author_nodes"](rig):
        key = bone.get("eiem_physics_id")
        if key:
            document.require(key not in result, "骨骼身份重复，请使用新建骨骼：" + bone.name)
            result[key] = (bone, record)
    return result


def selected_bones(rig):
    if rig.mode == "EDIT":
        return list(bpy.context.selected_editable_bones or [])
    if rig.mode == "POSE":
        return [pose.bone for pose in bpy.context.selected_pose_bones or [] if pose.id_data == rig]
    raise ValueError("Physics: 请进入 Rig 的编辑模式或姿态模式选择骨骼")


def new_helper(name, rig, kind):
    obj = bpy.data.objects.new(name, None)
    category = "COLLIDERS" if kind in ("COLLIDER", "NATIVE_COLLIDER") else "GROUPS"
    collection(rig, category).objects.link(obj)
    obj.empty_display_size = 0.04
    obj.eiem_physics.kind = kind
    obj.eiem_physics.label = name
    obj.eiem_physics.identity = uuid.uuid4().hex
    obj.eiem_physics.rig = rig
    return obj


def remove_generated_visuals(owner):
    for child in list(owner.children):
        if child.get("eiem_physics_visual"):
            native.remove_visual(child)


def author_bone_sample(group, bone):
    """Return one authored node's role and normalized root distance."""
    if not group or group.eiem_physics.kind != "GROUP" or not group.eiem_physics.rig:
        return None
    lookup = {str(item.get("eiem_physics_id")): item for item in group.eiem_physics.rig.data.bones
              if item.get("eiem_physics_id")}
    roles = {lookup[node.bone_id].name: node.role for node in group.eiem_physics.nodes
             if node.bone_id in lookup}
    if bone.name not in roles:
        return None
    distances = {}

    def distance(item):
        if item.name in distances:
            return distances[item.name]
        parent = item.parent
        if roles[item.name] != "MOVE" or not parent or parent.name not in roles:
            value = 0.0
        else:
            value = float((item.head_local - parent.head_local).length)
            if roles[parent.name] == "MOVE":
                value += distance(parent)
        distances[item.name] = value
        return value

    maximum = max((distance(item) for item in lookup.values()
                   if item.name in roles and roles[item.name] == "MOVE"), default=0.0)
    depth = distance(bone) / maximum if maximum > 1e-9 else 0.0
    return {"bone": bone, "path": str(bone.get("eiem_path", bone.name)),
            "depth": depth, "role": roles[bone.name]}


def rebuild_group(obj):
    """Draw an authored group from its explicit bone references."""
    global _REBUILDING_GROUPS
    if not obj or obj.eiem_physics.kind != "GROUP" or not obj.eiem_physics.rig:
        return
    _REBUILDING_GROUPS = True
    try:
        remove_generated_visuals(obj)
        rig = obj.eiem_physics.rig
        lookup = {str(b.get("eiem_physics_id")): b for b in rig.data.bones if b.get("eiem_physics_id")}
        nodes = [(lookup.get(item.bone_id), item.role) for item in obj.eiem_physics.nodes]
        nodes = [(bone, role) for bone, role in nodes if bone]
        selected = {bone.name for bone, role in nodes}
        roles = {bone.name: role for bone, role in nodes}
        distances = {}
        def distance(bone):
            if bone.name in distances:
                return distances[bone.name]
            role, parent = roles[bone.name], bone.parent
            if role != "MOVE" or not parent or parent.name not in roles:
                value = 0.0
            else:
                value = float((bone.head_local - parent.head_local).length)
                if roles[parent.name] == "MOVE":
                    value += distance(parent)
            distances[bone.name] = value
            return value
        maximum = max((distance(bone) for bone, role in nodes if role == "MOVE"), default=0.0)
        curve_node = native.curve_mapping_node(obj, native.NODE_RADIUS_PARAMETER)
        use_curve = bool(obj.eiem_physics.radius_use_curve) and curve_node is not None
        samples = {role: [] for role in ("FIXED", "MOVE")}
        ignored = []
        for index, (bone, role) in enumerate(nodes):
            depth = distance(bone) / maximum if maximum > 1e-9 else 0.0
            if role in samples:
                multiplier = (float(curve_node.mapping.evaluate(curve_node.mapping.curves[0], depth))
                              if use_curve else 1.0)
                samples[role].append((index, tuple(bone.head_local),
                                      max(0.0, obj.eiem_physics.node_radius * multiplier), depth))
            else:
                ignored.append(tuple(bone.head_local))
        for role, role_samples in samples.items():
            native.make_node_radius_visual(obj, role_samples, native.COLORS[role],
                                           obj.name + " " + role + " 节点半径",
                                           native_coordinates=False)
        native.make_node_visual(obj, ignored, native.COLORS["IGNORE"], obj.name + " IGNORE",
                                native_coordinates=False)
        segments = [([tuple(bone.parent.head_local), tuple(bone.head_local)], False)
                    for bone, role in nodes if bone.parent and bone.parent.name in selected]
        native.make_link_visual(obj, segments, native.COLORS["LINK"], obj.name + " 骨链",
                                native_coordinates=False, radius=.0018, alpha=.8)
        if native.is_parameter_group(obj):
            action = obj.animation_data.action if obj.animation_data else None
            if action and action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key:
                native.ensure_curve_preview_timer()
            if bool(native.native_number(obj, native.ANGLE_ENABLED_FIELD)):
                cones = []
                for bone, role in nodes:
                    parent = bone.parent
                    if role != "MOVE" or not parent or parent.name not in selected:
                        continue
                    depth = distance(bone) / maximum if maximum > 1e-9 else 0.0
                    edge = bone.head_local - parent.head_local
                    angle = native.angle_limit_degrees(obj, depth)
                    ring = native.angle_cone_ring(parent.head_local, edge, angle)
                    if ring:
                        cones.append((parent.head_local, ring, angle, depth))
                native.make_angle_visual(obj, cones, native_coordinates=False)
                obj["eiem_physics_angle_preview"] = "ok:%d" % len(cones)
            else:
                obj["eiem_physics_angle_preview"] = "disabled"
    finally:
        _REBUILDING_GROUPS = False


def validate_group_nodes(rig, pairs):
    records = {bone.name: record for bone, record, source in API["skeleton_author_nodes"](rig)}
    document.require(all(bone.name in records for bone, role in pairs), "物理骨骼已从共享 Rig 删除")
    nodes = [{"bone": records[bone.name][0], "role": role} for bone, role in pairs]
    probe = {"version": document.VERSION, "purpose": "authoring", "coordinate": document.COORDINATE,
             "backend": "BeyondDynamicBone", "id": "0"*32, "skeleton": "test.skeleton",
             "colliders": [], "groups": [{"id": "1"*32, "name": "物理链", "nodes": nodes,
                  "parameters": dict(document.DEFAULTS), "radius": document.default_radius(),
                  "nativeParameters": [], "colliders": []}]}
    document.validate(probe)
    return nodes


def add_selected_nodes(group, selection):
    """Explicitly extend one author chain; a second root remains invalid."""
    rig = group.eiem_physics.rig
    lookup = bones(rig)
    pairs = []
    present = set()
    for item in group.eiem_physics.nodes:
        document.require(item.bone_id in lookup, "物理组引用的骨骼已删除")
        bone = lookup[item.bone_id][0]
        pairs.append((bone, item.role)); present.add(bone.name)
    combined = present | {bone.name for bone in selection}
    additions = []
    for bone in selection:
        if bone.name in present:
            continue
        role = "MOVE" if bone.parent and bone.parent.name in combined else "FIXED"
        pairs.append((bone, role)); additions.append((bone, role))
    document.require(additions, "所选骨骼已经在当前物理链中")
    validate_group_nodes(rig, pairs)
    for bone, role in additions:
        item = group.eiem_physics.nodes.add(); item.bone_id, item.role = bone_id(bone), role
    rebuild_group(group)
    return len(additions)


def remove_selected_nodes(group, selection):
    rig = group.eiem_physics.rig
    lookup = bones(rig)
    removed = {bone_id(bone) for bone in selection}
    present = {item.bone_id for item in group.eiem_physics.nodes}
    removed &= present
    pairs = [(lookup[item.bone_id][0], item.role) for item in group.eiem_physics.nodes
             if item.bone_id not in removed and item.bone_id in lookup]
    document.require(len(pairs) < len(group.eiem_physics.nodes), "所选骨骼不在当前物理链中")
    validate_group_nodes(rig, pairs)
    for index in reversed(range(len(group.eiem_physics.nodes))):
        if group.eiem_physics.nodes[index].bone_id in removed:
            group.eiem_physics.nodes.remove(index)
    rebuild_group(group)
    return len(removed)


def flush_armature_visuals():
    if not _DIRTY_ARMATURES:
        return None
    rigs = [obj for obj in bpy.data.objects if obj.type == "ARMATURE" and obj.data.as_pointer() in _DIRTY_ARMATURES]
    if any(rig.mode == "EDIT" for rig in rigs):
        return .25
    pointers = set(_DIRTY_ARMATURES); _DIRTY_ARMATURES.clear()
    for group in [obj for obj in bpy.data.objects if hasattr(obj, "eiem_physics") and
                  obj.eiem_physics.kind == "GROUP" and obj.eiem_physics.rig and
                  obj.eiem_physics.rig.data.as_pointer() in pointers]:
        rebuild_group(group)
    return None


def depsgraph_physics_updated(scene, depsgraph):
    if _REBUILDING_GROUPS:
        return
    for update in depsgraph.updates:
        if isinstance(update.id, bpy.types.Armature):
            _DIRTY_ARMATURES.add(update.id.as_pointer())
    if _DIRTY_ARMATURES and not bpy.app.timers.is_registered(flush_armature_visuals):
        bpy.app.timers.register(flush_armature_visuals, first_interval=.15)


def create_group(rig, selection, name="物理链", template=None):
    document.require(len(selection) >= 2, "至少选择一根固定根骨和一根运动骨骼")
    document.require(all(b.id_data == rig.data for b in selection), "骨骼必须属于同一个共享 Rig")
    selected = {bone.name for bone in selection}
    pairs = [(bone, "MOVE" if bone.parent and bone.parent.name in selected else "FIXED")
             for bone in selection]
    node_records = validate_group_nodes(rig, pairs)
    obj = new_helper(name, rig, "GROUP")
    obj.parent = rig
    values = obj.eiem_physics
    for b, node in zip(selection, node_records):
        item = values.nodes.add()
        item.bone_id, item.role = bone_id(b), node["role"]
    if template:
        for key in document.PARAMETERS:
            setattr(values, key, getattr(template.eiem_physics, key))
        values.native_parameter_snapshot = template.eiem_physics.native_parameter_snapshot
    load_author_radius(obj, author_radius_record(template) if template else document.default_radius())
    if template and hasattr(obj, "eiem_native_physics"):
        load_author_native_fields(obj, author_native_field_records(template))
        activate_author_full_projection(obj)
    rebuild_group(obj)
    bpy.context.scene.eiem_physics_group = obj
    return obj


def duplicate_group(source):
    data = source.eiem_physics
    lookup = bones(data.rig)
    selection = []
    for item in data.nodes:
        document.require(item.bone_id in lookup, "物理组引用的骨骼已删除")
        selection.append(lookup[item.bone_id][0])
    obj = create_group(data.rig, selection, source.name + " 副本", source)
    for dst, src in zip(obj.eiem_physics.nodes, data.nodes):
        dst.role = src.role
    for ref in data.colliders:
        obj.eiem_physics.colliders.add().object = ref.object
    return obj


def collider_reference_payload(group):
    """Store stable references to the collider set used by one group."""
    result = []
    for ref in group.eiem_physics.colliders:
        obj = ref.object
        document.require(obj and obj.eiem_physics.rig == group.eiem_physics.rig,
                         "物理组含失效或其他 Rig 的碰撞体引用")
        if obj.eiem_physics.kind == "COLLIDER":
            token = {"kind": "AUTHOR", "key": obj.eiem_physics.identity}
        elif obj.eiem_physics.kind == "NATIVE_COLLIDER":
            token = {"kind": "NATIVE", "key": obj.eiem_native_physics.source_key}
        else:
            document.require(False, "物理组含不受支持的碰撞体引用")
        document.require(token not in result, "物理组含重复碰撞体引用")
        result.append(token)
    return result


def resolve_collider_references(group, payload):
    """Resolve clipboard collider identities against the target group's Rig."""
    records = payload.get("colliderRefs")
    if records is None:  # Compatibility with the short-lived clipboard v1.
        return None
    document.require(isinstance(records, list), "参数剪贴板碰撞集合损坏")
    wanted = []
    for record in records:
        document.require(isinstance(record, dict) and set(record) == {"kind", "key"} and
                         record["kind"] in ("AUTHOR", "NATIVE") and
                         isinstance(record["key"], str) and bool(record["key"]),
                         "参数剪贴板碰撞引用损坏")
        matches = []
        for obj in bpy.data.objects:
            if not hasattr(obj, "eiem_physics") or obj.eiem_physics.rig != group.eiem_physics.rig:
                continue
            if record["kind"] == "AUTHOR" and obj.eiem_physics.kind == "COLLIDER" and \
                    obj.eiem_physics.identity == record["key"]:
                matches.append(obj)
            if record["kind"] == "NATIVE" and obj.eiem_physics.kind == "NATIVE_COLLIDER" and \
                    obj.eiem_native_physics.source_key == record["key"]:
                matches.append(obj)
        document.require(len(matches) == 1,
                         "目标 Rig 找不到参数模板使用的碰撞体；请先把源物理导入同一 Rig")
        document.require(matches[0] not in wanted, "参数剪贴板含重复碰撞体")
        wanted.append(matches[0])
    return wanted


def author_native_field_records(obj):
    """Return editable author fields, upgrading an older hidden snapshot."""
    fields = getattr(obj, "eiem_native_physics", None)
    if fields and len(fields.fields):
        return [{"path": field.label, "floating": bool(field.floating),
                 "value": float(field.value) if field.floating else int(field.integer)}
                for field in fields.fields]
    snapshot = obj.eiem_physics.native_parameter_snapshot
    if not snapshot:
        return []
    try:
        saved = json.loads(snapshot)
    except (TypeError, ValueError, json.JSONDecodeError):
        raise ValueError("Physics: 当前作者组的原生参数快照损坏，请重新粘贴参数")
    document.require(isinstance(saved, dict) and saved.get("nativeType") == "BeyondBoneCloth" and
                     isinstance(saved.get("nativeFields"), list), "作者组原生参数快照不完整")
    return saved["nativeFields"]


def persist_author_native_fields(obj):
    p = obj.eiem_native_physics
    if not len(p.fields):
        obj.eiem_physics.native_parameter_snapshot = ""
        return
    records = [{"path": field.label, "floating": bool(field.floating),
                "value": float(field.value) if field.floating else int(field.integer)}
               for field in p.fields]
    for record in records:
        document.native_parameter(record)
    obj.eiem_physics.native_parameter_snapshot = json.dumps({
        "nativeType": "BeyondBoneCloth", "nativeFields": records}, ensure_ascii=False,
        separators=(",", ":"), allow_nan=False)


def load_author_native_fields(obj, records):
    """Expose a copied native template as editable RNA fields on the author Empty."""
    p = obj.eiem_native_physics
    p.ready = False
    p.fields.clear()
    for record in records:
        document.native_parameter({"path": record["path"], "floating": bool(record["floating"]),
                                   "value": float(record["value"]) if record["floating"] else int(record["value"])})
        field = p.fields.add()
        field.path = json.dumps(record["path"], ensure_ascii=False)
        field.label = record["path"]
        field.original = json.dumps(record["value"], ensure_ascii=False, allow_nan=False)
        field.floating = bool(record["floating"])
        if field.floating:
            field.value = float(record["value"])
        else:
            field.integer = str(int(record["value"]))
    p.source_key = "author:" + obj.eiem_physics.identity
    p.ready = True
    persist_author_native_fields(obj)


def ensure_author_native_fields(obj):
    if obj and obj.eiem_physics.kind == "GROUP" and not len(obj.eiem_native_physics.fields) and \
            obj.eiem_physics.native_parameter_snapshot:
        load_author_native_fields(obj, author_native_field_records(obj))


def clear_author_parameter_action(obj):
    """Remove only an EIEM-owned legacy/full curve projection from an author Empty."""
    action = obj.animation_data.action if obj and obj.animation_data else None
    if action is not None:
        owned = (action.get(AUTHOR_CURVE_ACTION_MARKER) == obj.eiem_physics.identity or
                 action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key)
        document.require(owned, "该物理组已有非 EIEM 动画，请先移除或转移该 Action")
        native.remove_curve_projection(obj)
        obj.animation_data_clear()
    if AUTHOR_RADIUS_PROPERTY in obj:
        del obj[AUTHOR_RADIUS_PROPERTY]


def activate_author_full_projection(obj):
    """Give a pasted author group the same nine inline curves as its source group."""
    if not native.is_parameter_group(obj):
        return None
    sync_author_direct_fields(obj, include_radius=True)
    clear_author_parameter_action(obj)
    tree = native.build_curve_mappings(obj)
    native.ensure_curve_preview_timer()
    return tree


def sync_author_controls_from_native_fields(obj):
    """Update the compact author controls after fields or projected curves change."""
    if not native.is_parameter_group(obj):
        return
    p = obj.eiem_native_physics
    p.ready = False
    try:
        for name in document.PARAMETERS:
            setattr(obj.eiem_physics, name, float(native.native_number(obj, "serializeData." + name)))
        radius = native.mapped_radius(obj)
        obj.eiem_physics.node_radius = float(radius["value"])
        obj.eiem_physics.radius_use_curve = bool(radius["useCurve"])
        persist_author_native_fields(obj)
    finally:
        p.ready = True
    rebuild_group(obj)


def sync_author_radius_controls_to_native_fields(obj):
    """Write the two compact radius controls into the shared field/curve model."""
    if not native.is_parameter_group(obj):
        return
    p = obj.eiem_native_physics
    p.ready = False
    try:
        base = native.native_field(obj, native.NODE_RADIUS_BASE_FIELD)
        enabled = native.native_field(obj, native.NODE_RADIUS_PARAMETER + ".useCurve")
        document.require(base is not None and base.floating and enabled is not None and not enabled.floating,
                         "完整参数缺少节点半径字段")
        base.value = float(obj.eiem_physics.node_radius)
        enabled.integer = "1" if obj.eiem_physics.radius_use_curve else "0"
        persist_author_native_fields(obj)
    finally:
        p.ready = True


def author_native_field_edited(obj, field):
    """Keep compact author controls and the complete field table coherent."""
    if not obj or obj.eiem_physics.kind != "GROUP" or not obj.eiem_native_physics.ready:
        return
    p = obj.eiem_native_physics
    direct = {"serializeData." + name: name for name in document.PARAMETERS}
    curve_parameter = next((parameter for parameter, label in native.CURVE_PARAMETERS
                            if field.label == parameter + ".value" or
                            field.label == parameter + ".useCurve" or
                            field.label.startswith(parameter + ".curve.")), None)
    changes_radius = field.label in native.NODE_RADIUS_VISUAL_FIELDS or \
                     field.label.startswith(native.NODE_RADIUS_CURVE_FIELD_PREFIX)
    changes_angle = field.label in native.ANGLE_VISUAL_FIELDS or \
                    field.label.startswith(native.ANGLE_CURVE_FIELD_PREFIX)
    p.ready = False
    try:
        if field.label in direct and field.floating:
            setattr(obj.eiem_physics, direct[field.label], float(field.value))
        if curve_parameter is not None:
            native.build_curve_mappings(obj)
        if changes_radius:
            radius = native.mapped_radius(obj)
            obj.eiem_physics.node_radius = float(radius["value"])
            obj.eiem_physics.radius_use_curve = bool(radius["useCurve"])
        persist_author_native_fields(obj)
    finally:
        p.ready = True
    if changes_radius or changes_angle:
        rebuild_group(obj)


def sync_author_direct_fields(obj, include_radius=False):
    """Write compact controls/F-Curve edits back into the visible full table."""
    if not obj or obj.eiem_physics.kind != "GROUP" or not len(obj.eiem_native_physics.fields):
        return
    p = obj.eiem_native_physics
    replacements = _author_values_as_native_fields({
        "parameters": {name: float(getattr(obj.eiem_physics, name)) for name in document.PARAMETERS},
        "radius": author_radius_record(obj)})
    if not include_radius:
        replacements = {path: value for path, value in replacements.items()
                        if not path.startswith(native.NODE_RADIUS_PARAMETER + ".")}
    p.ready = False
    try:
        available = {field.label: field for field in p.fields}
        if include_radius:
            kept = [{"path": field.label, "floating": bool(field.floating),
                     "value": float(field.value) if field.floating else int(field.integer)}
                    for field in p.fields
                    if not field.label.startswith(native.NODE_RADIUS_PARAMETER + ".")]
            kept.extend({"path": path, "floating": floating,
                         "value": float(value) if floating else int(value)}
                        for path, (floating, value) in replacements.items()
                        if path.startswith(native.NODE_RADIUS_PARAMETER + "."))
            load_author_native_fields(obj, sorted(kept, key=lambda item: item["path"]))
            return
        for path, (floating, value) in replacements.items():
            field = available.get(path)
            if field is None:
                continue
            document.require(bool(field.floating) == bool(floating), "作者参数字段类型已变化：" + path)
            if floating:
                field.value = float(value)
            else:
                field.integer = str(int(value))
        persist_author_native_fields(obj)
    finally:
        p.ready = True


def _parameter_clipboard_payload(obj):
    """Snapshot group parameters and its collider set without copying bones."""
    document.require(obj and obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"),
                     "请选择要复制参数的物理组")
    if native.is_native(obj):
        native.build_curve_mappings(obj)
        action = obj.animation_data.action if obj.animation_data else None
        if action and action.get(native.CURVE_ACTION_MARKER) == obj.eiem_native_physics.source_key:
            native.apply_curve_projection(obj)
        fields = [{"path": field.label, "floating": bool(field.floating),
                   "value": float(field.value) if field.floating else str(field.integer)}
                  for field in obj.eiem_native_physics.fields]
        document.require(bool(fields), "源物理组没有可复制参数")
        author = {"parameters": native.mapped_parameters(obj),
                  "radius": native.mapped_radius(obj)}
        return {"version": PARAMETER_CLIPBOARD_VERSION, "sourceKind": "NATIVE_GROUP",
                "sourceName": obj.eiem_physics.label, "nativeType": native.record(obj)["type"],
                "nativeFields": fields, "author": author,
                "colliderRefs": collider_reference_payload(obj)}
    ensure_author_native_fields(obj)
    sync_author_direct_fields(obj, include_radius=True)
    result = {"version": PARAMETER_CLIPBOARD_VERSION, "sourceKind": "GROUP",
              "sourceName": obj.eiem_physics.label,
              "author": {"parameters": {key: float(getattr(obj.eiem_physics, key))
                                          for key in document.PARAMETERS},
                         "radius": author_radius_record(obj)},
              "colliderRefs": collider_reference_payload(obj)}
    fields = author_native_field_records(obj)
    if fields:
        result.update(nativeType="BeyondBoneCloth", nativeFields=fields)
    return result


def copy_group_parameters(scene, obj):
    payload = _parameter_clipboard_payload(obj)
    scene.eiem_physics_parameter_clipboard = json.dumps(
        payload, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
    return len(payload.get("nativeFields", ())) or len(document.PARAMETERS) + 1


def _read_parameter_clipboard(scene):
    try:
        payload = json.loads(scene.eiem_physics_parameter_clipboard)
    except (TypeError, ValueError, json.JSONDecodeError):
        raise ValueError("Physics: 参数剪贴板为空，请先选择一个物理组并点击复制参数")
    document.require(isinstance(payload, dict) and payload.get("version") in (1, PARAMETER_CLIPBOARD_VERSION),
                     "参数剪贴板版本不受支持，请重新复制")
    author = payload.get("author")
    document.require(isinstance(author, dict) and set(author) == {"parameters", "radius"},
                     "参数剪贴板不完整，请重新复制")
    parameters = author["parameters"]
    document.require(isinstance(parameters, dict) and set(parameters) == set(document.PARAMETERS),
                     "参数剪贴板缺少作者组参数")
    for key, value in parameters.items():
        document.number(value, maximum=1.0 if key in document.PARAMETERS[2:] else 3.4028234663852886e38)
    radius = author["radius"]
    document.keys(radius, "value useCurve keys preInfinity postInfinity rotationOrder", "node radius")
    document.number(radius["value"], 1.401298464324817e-45)
    document.require(type(radius["useCurve"]) is bool, "节点半径曲线开关无效")
    document.require(isinstance(radius["keys"], list) and
                     2 <= len(radius["keys"]) <= document.MAX_CURVE_KEYS,
                     "节点半径曲线关键帧数量无效")
    previous = -1.0
    for key in radius["keys"]:
        document.keys(key, "time value inSlope outSlope weightedMode inWeight outWeight", "node radius key")
        document.number(key["time"], 0.0, 1.0)
        document.require(key["time"] > previous, "节点半径曲线位置必须递增")
        previous = key["time"]
        document.number(key["value"])
        document.number(key["inSlope"], -3.4028234663852886e38)
        document.number(key["outSlope"], -3.4028234663852886e38)
        document.require(type(key["weightedMode"]) is int and 0 <= key["weightedMode"] <= 3,
                         "节点半径曲线权重模式无效")
        document.number(key["inWeight"], 0.0, 1.0)
        document.number(key["outWeight"], 0.0, 1.0)
    if payload.get("colliderRefs") is not None:
        document.require(isinstance(payload["colliderRefs"], list), "参数剪贴板碰撞集合损坏")
        for ref in payload["colliderRefs"]:
            document.require(isinstance(ref, dict) and set(ref) == {"kind", "key"} and
                             ref["kind"] in ("AUTHOR", "NATIVE") and
                             isinstance(ref["key"], str) and bool(ref["key"]),
                             "参数剪贴板碰撞引用损坏")
    return payload


def _author_values_as_native_fields(author):
    values = {"serializeData." + key: (True, float(value))
              for key, value in author["parameters"].items()}
    radius = author["radius"]
    prefix = native.NODE_RADIUS_PARAMETER
    values[prefix + ".value"] = (True, float(radius["value"]))
    values[prefix + ".useCurve"] = (False, str(int(radius["useCurve"])))
    for index, key in enumerate(radius["keys"]):
        for suffix in ("time", "value", "inSlope", "outSlope", "inWeight", "outWeight"):
            values["%s.curve.m_Curve.%d.%s" % (prefix, index, suffix)] = (True, float(key[suffix]))
        values["%s.curve.m_Curve.%d.weightedMode" % (prefix, index)] = (
            False, str(int(key["weightedMode"])))
    for source_name, native_name in (("preInfinity", "m_PreInfinity"),
                                     ("postInfinity", "m_PostInfinity"),
                                     ("rotationOrder", "m_RotationOrder")):
        values[prefix + ".curve." + native_name] = (False, str(int(radius[source_name])))
    return values


def author_native_parameters(obj, author):
    """Merge a copied native template with values edited on this author group."""
    merged = {}
    ensure_author_native_fields(obj)
    sync_author_direct_fields(obj, include_radius=True)
    saved_fields = author_native_field_records(obj)
    if saved_fields:
        for item in saved_fields:
            document.require(isinstance(item, dict) and set(item) == {"path", "floating", "value"},
                             "author native parameter field is incomplete")
            path = item["path"]
            document.require(path not in merged, "author native parameter snapshot has duplicate paths")
            floating = item["floating"]
            value = float(item["value"]) if floating else int(item["value"])
            merged[path] = {"path": path, "floating": bool(floating), "value": value}
    # Radius is also stored as a directly editable author curve. Drop every
    # copied radius key first so deleting/reordering keys cannot leave stale
    # template entries behind; the current author record replaces it whole.
    radius_prefix = native.NODE_RADIUS_PARAMETER + "."
    merged = {path: item for path, item in merged.items()
              if not path.startswith(radius_prefix)}
    for path, (floating, value) in _author_values_as_native_fields(author).items():
        merged[path] = {"path": path, "floating": bool(floating),
                        "value": float(value) if floating else int(value)}
    result = [merged[path] for path in sorted(merged)]
    for parameter in result:
        document.native_parameter(parameter)
    return result


def paste_group_parameters(scene, obj):
    document.require(obj and obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"),
                     "请选择要粘贴参数的物理组")
    payload = _read_parameter_clipboard(scene)
    author = payload["author"]
    if obj.eiem_physics.kind == "GROUP":
        previous_parameters = {key: float(getattr(obj.eiem_physics, key)) for key in document.PARAMETERS}
        previous_radius = author_radius_record(obj)
        previous_snapshot = obj.eiem_physics.native_parameter_snapshot
        previous_fields = author_native_field_records(obj)
        previous_colliders = [ref.object for ref in obj.eiem_physics.colliders]
        wanted_colliders = resolve_collider_references(obj, payload)
        try:
            obj.eiem_native_physics.ready = False
            for key, value in author["parameters"].items():
                setattr(obj.eiem_physics, key, value)
            if payload.get("nativeFields") is not None:
                obj.eiem_physics.native_parameter_snapshot = json.dumps({
                    "nativeType": payload.get("nativeType", "BeyondBoneCloth"),
                    "nativeFields": payload["nativeFields"]}, ensure_ascii=False,
                    separators=(",", ":"), allow_nan=False)
            else:
                obj.eiem_physics.native_parameter_snapshot = ""
            load_author_radius(obj, author["radius"])
            load_author_native_fields(obj, payload.get("nativeFields", ()))
            if native.is_parameter_group(obj):
                activate_author_full_projection(obj)
            if wanted_colliders is not None:
                obj.eiem_physics.colliders.clear()
                for collider in wanted_colliders:
                    obj.eiem_physics.colliders.add().object = collider
            native.apply_visibility(scene)
            rebuild_group(obj)
        except Exception:
            obj.eiem_native_physics.ready = False
            for key, value in previous_parameters.items():
                setattr(obj.eiem_physics, key, value)
            obj.eiem_physics.native_parameter_snapshot = previous_snapshot
            load_author_radius(obj, previous_radius)
            load_author_native_fields(obj, previous_fields)
            if native.is_parameter_group(obj):
                activate_author_full_projection(obj)
            obj.eiem_physics.colliders.clear()
            for collider in previous_colliders:
                obj.eiem_physics.colliders.add().object = collider
            rebuild_group(obj)
            raise
        return len(payload.get("nativeFields", ())) or len(document.PARAMETERS) + 1

    source_fields = payload.get("nativeFields")
    if source_fields is not None:
        document.require(payload.get("nativeType") == native.record(obj)["type"],
                         "复制源与目标原生组件类型不同")
        document.require(isinstance(source_fields, list) and bool(source_fields), "原生参数剪贴板为空")
        wanted = {(item.get("path"), item.get("floating")): item.get("value")
                  for item in source_fields if isinstance(item, dict)}
        document.require(len(wanted) == len(source_fields), "原生参数剪贴板含重复或无效字段")
    else:
        wanted = {(path, floating): value for path, (floating, value)
                  in _author_values_as_native_fields(author).items()}
    target = obj.eiem_native_physics
    available = {(field.label, bool(field.floating)): field for field in target.fields}
    missing = set(wanted) - set(available)
    document.require(not missing,
                     "目标组参数结构不同，缺少：" + "、".join(sorted(path for path, floating in missing)[:3]))
    backup = {key: (float(field.value) if field.floating else str(field.integer))
              for key, field in available.items() if key in wanted}
    target.ready = False
    try:
        for key, value in wanted.items():
            field = available[key]
            if field.floating:
                field.value = float(value)
            else:
                field.integer = str(value)
    except Exception:
        for key, value in backup.items():
            field = available[key]
            if field.floating: field.value = value
            else: field.integer = value
        raise
    finally:
        target.ready = True
    native.build_curve_mappings(obj)
    native.rebuild(obj)
    return len(wanted)


def clone_selected_chain(rig):
    """Copy rest geometry as new Skeleton nodes and create an additive group."""
    selection = list(selected_bones(rig))
    document.require(len(selection) >= 2, "至少选择两根连续骨骼")
    names = {bone.name for bone in selection}
    roots = [bone for bone in selection if not bone.parent or bone.parent.name not in names]
    document.require(len(roots) == 1, "所选骨骼必须组成一棵连续骨链")
    source_names = [bone.name for bone in selection]; root_name = roots[0].name
    if rig.mode != "EDIT": bpy.ops.object.mode_set(mode="EDIT")
    edit = rig.data.edit_bones
    pending = set(source_names); clones = {}; created_names = []
    try:
        while pending:
            progressed = False
            for name in list(pending):
                original = edit.get(name)
                document.require(original is not None, "所选骨骼已变化：" + name)
                if original.parent and original.parent.name in pending: continue
                clone = edit.new(name + "_physics")
                clone.head, clone.tail, clone.roll = original.head.copy(), original.tail.copy(), original.roll
                clone.head_radius, clone.tail_radius = original.head_radius, original.tail_radius
                clone.envelope_distance, clone.envelope_weight = original.envelope_distance, original.envelope_weight
                clone.parent = clones.get(original.parent.name) if original.parent and original.parent.name in clones else original.parent
                clone.use_connect = bool(original.use_connect and clone.parent in clones.values())
                clone["eiem_skeleton_source"] = False
                clones[name] = clone; created_names.append(clone.name)
                pending.remove(name); progressed = True
            document.require(progressed, "无法复制循环或断开的骨链")
        bpy.ops.object.mode_set(mode="OBJECT")
        copied = [rig.data.bones[name] for name in created_names]
        group = create_group(rig, copied, root_name + " 新增物理链")
        group["eiem_physics_cloned_from"] = ",".join(source_names)
        return group
    except Exception:
        if rig.mode != "EDIT": bpy.ops.object.mode_set(mode="EDIT")
        for name in reversed(created_names):
            bone = edit.get(name)
            if bone: edit.remove(bone)
        bpy.ops.object.mode_set(mode="OBJECT")
        raise


def native_world(rig, wanted):
    world = []
    for bone, record, source in API["skeleton_author_nodes"](rig):
        path, parent, p, q, s = record
        matrix = Matrix.LocRotScale(Vector(p), Quaternion((q[3], q[0], q[1], q[2])), Vector(s))
        world.append(world[parent] @ matrix if parent >= 0 else matrix)
        if bone == wanted:
            return API["unity_transform_matrix_to_blender"](world[-1])
    raise ValueError("Physics: 找不到碰撞体绑定骨骼")


def bind_collider(obj, bone):
    rig = obj.eiem_physics.rig
    obj.eiem_physics.bone_id = bone_id(bone)
    constraint = obj.constraints.get("EIEM Physics bone") or obj.constraints.new("CHILD_OF")
    constraint.name = "EIEM Physics bone"
    constraint.target, constraint.subtarget = rig, bone.name
    constraint.inverse_matrix = bone.matrix_local.inverted() @ native_world(rig, bone)


def rebuild_collider(self, context):
    obj = self.id_data
    if not isinstance(obj, bpy.types.Object) or self.kind != "COLLIDER":
        return
    display = self.visual
    if display:
        self.visual = None
        native.remove_visual(display)
    radius = self.radius
    if self.shape == "SPHERE":
        geometry = {"shape": "SPHERE", "center": (0, 0, 0), "radius": radius}
    else:
        span = self.span
        geometry = {"shape": "CAPSULE", "center": (0, 0, 0),
                    "start": (0, span / 2, 0), "end": (0, -span / 2, 0),
                    "startRadius": radius, "endRadius": radius}
    self.visual = native.make_collider_visual(obj, geometry, obj.name + " " +
        ("球体" if self.shape == "SPHERE" else "胶囊"))


def create_collider(group, bone, shape="CAPSULE", name="碰撞体"):
    rig = group.eiem_physics.rig
    document.require(shape in document.SHAPES and bone.id_data == rig.data, "碰撞体骨骼必须属于当前物理组的 Rig")
    obj = new_helper(name, rig, "COLLIDER")
    obj.rotation_mode = "QUATERNION"
    obj.eiem_physics.shape = shape
    bind_collider(obj, bone)
    rebuild_collider(obj.eiem_physics, bpy.context)
    group.eiem_physics.colliders.add().object = obj
    return obj


def author_document(groups, skeleton_path):
    document.require(bool(groups), "请选择物理组")
    document.require(all(o.eiem_physics.kind == "GROUP" for o in groups), "导出对象必须是物理组")
    rig = groups[0].eiem_physics.rig
    document.require(all(o.eiem_physics.rig == rig for o in groups), "一份 Physics 只能引用一个共享 Rig")
    lookup = bones(rig)
    group_records, collider_objects = [], {}

    def bone_path(key):
        document.require(key in lookup, "物理组/碰撞体引用的骨骼已删除")
        return lookup[key][1][0]

    for obj in sorted(groups, key=lambda o: o.eiem_physics.identity):
        p = obj.eiem_physics
        refs = []
        for ref in p.colliders:
            c = ref.object
            document.require(c and c.eiem_physics.rig == rig,
                             "缺失碰撞体或引用了其他 Rig；请移除失效引用")
            document.require(c.eiem_physics.kind == "COLLIDER",
                             "当前组引用了游戏源碰撞体；关联已保存在 .blend，DLL 转换完成前不能导出作者 Physics")
            cid = c.eiem_physics.identity
            document.require(cid not in collider_objects or collider_objects[cid] == c,
                             "复制碰撞体后身份冲突，请通过添加碰撞体创建")
            collider_objects[cid] = c
            refs.append(cid)
        author = {"parameters": {k: getattr(p, k) for k in document.PARAMETERS},
                  "radius": author_radius_record(obj)}
        group_records.append({"id": p.identity, "name": p.label,
            "nodes": [{"bone": bone_path(n.bone_id), "role": n.role} for n in p.nodes],
            "parameters": author["parameters"], "radius": author["radius"],
            "nativeParameters": author_native_parameters(obj, author),
            "colliders": sorted(refs)})
    colliders = []
    for cid, obj in sorted(collider_objects.items()):
        p = obj.eiem_physics
        document.require(obj.parent is None and len(obj.constraints) == 1 and
                         obj.constraints[0].type == "CHILD_OF" and not obj.constraints[0].mute and
                         abs(obj.constraints[0].influence-1) < 1e-6 and obj.constraints[0].target == rig,
                         "碰撞体绑定已改变，请重新绑定")
        document.require(max(abs(x-1) for x in obj.scale) < 1e-5 and
                         max(abs(x) for x in obj.delta_location) < 1e-6 and
                         max(abs(x-1) for x in obj.delta_scale) < 1e-6 and
                         max(abs(x) for x in obj.delta_rotation_euler) < 1e-6 and
                         abs(obj.delta_rotation_quaternion.w-1) < 1e-6,
                         "请用半径和端点间距编辑尺寸；不支持对象缩放或增量变换")
        bone_path(p.bone_id)
        bone = lookup[p.bone_id][0]
        document.require(obj.constraints[0].subtarget == bone.name, "碰撞体绑定名称已变化，请点击重新绑定")
        constraint = obj.constraints[0]
        inverse = bone.matrix_local.inverted() @ native_world(rig, bone)
        document.require(all(getattr(constraint, "use_"+channel+"_"+axis)
                             for channel in ("location", "rotation", "scale") for axis in "xyz") and
                         max(abs(constraint.inverse_matrix[r][c]-inverse[r][c]) for r in range(4) for c in range(4)) < 1e-5,
                         "碰撞体约束或骨架姿态已变化，请刷新骨骼绑定")
        basis = API["unity_transform_matrix_to_blender_basis"]()
        matrix = basis.inverted() @ obj.matrix_basis @ basis
        position, rotation, scale = matrix.decompose()
        colliders.append({"id": cid, "name": p.label, "bone": bone_path(p.bone_id),
            "shape": p.shape, "position": list(position), "rotation": [rotation.x, rotation.y, rotation.z, rotation.w],
            "radius": p.radius, "span": p.span if p.shape == "CAPSULE" else 0.0})
    identity = rig.get("eiem_physics_resource_id")
    if not identity:
        identity = rig["eiem_physics_resource_id"] = uuid.uuid4().hex
    result = {"version": document.VERSION, "purpose": "authoring", "coordinate": document.COORDINATE,
              "backend": "BeyondDynamicBone", "id": str(identity), "skeleton": skeleton_path,
              "groups": group_records, "colliders": colliders}
    paths = {record[0] for bone, record, source in API["skeleton_author_nodes"](rig)}
    return document.validate(result, paths)


def export_physics(filename, groups):
    document.require(bpy.context.mode == "OBJECT", "请回到物体模式后导出")
    filename = Path(filename).resolve()
    document.require(filename.suffix.lower() == ".physics", "输出文件必须为 .physics")
    groups = list(groups)
    document.require(bool(groups), "请选择物理组")
    if any(native.is_native(o) for o in groups):
        return native.export_source(filename, groups)
    for group in groups:
        source = group.get("eiem_physics_source_file")
        document.require(not source or Path(source).resolve() != filename, "请选择新文件，不覆盖导入的 Physics 源包")
    # Serialize/validate first. Immutable content-named Skeleton is published
    # before the Physics file, so a failed update keeps the old dependency valid.
    with tempfile.TemporaryDirectory(prefix="eiem-physics-") as temporary:
        skeleton = Path(temporary) / "author.skeleton"
        rig = groups[0].eiem_physics.rig
        API["write_skeleton"](skeleton, rig)
        skeleton_data = skeleton.read_bytes()
        relative = "skeletons/" + hashlib.sha256(skeleton_data).hexdigest() + ".skeleton"
        payload = author_document(groups, relative)
        encoded = document.encode(payload)
        filename.parent.mkdir(parents=True, exist_ok=True)
        skeleton_out = API["safe_path"](filename.parent, relative)
        skeleton_out.parent.mkdir(exist_ok=True)
        atomic_write(skeleton_out, skeleton_data)
        atomic_write(filename, encoded)
    return {"groups": len(payload["groups"]), "colliders": len(payload["colliders"]), "skeletons": 1}


def atomic_write(path, data):
    descriptor, temporary = tempfile.mkstemp(prefix=".eiem-physics-", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def import_physics(filename, rig=None):
    filename = Path(filename).resolve()
    with filename.open("rb") as stream:
        if stream.read(12) == native.source.MAGIC + b"\x02\x00\x00\x00":
            return native.import_source(filename, rig)
    payload = document.read(filename)
    skeleton = API["read_skeleton"](API["safe_path"](filename.parent, payload["skeleton"]))
    document.validate(payload, {n[0] for n in skeleton["nodes"]})
    document.require(bpy.context.mode == "OBJECT", "请回到物体模式后导入")
    if rig:
        records = API["skeleton_author_nodes"](rig)
        document.require([r for b, r, s in records] == skeleton["nodes"] or
                         skeleton_equal(records, skeleton), "当前 Rig 与 Physics 的共享骨架不一致")
    # Parse all dependencies before creating anything; unexpected Blender API
    # failures roll back only the objects/data blocks created by this import.
    kinds = ("objects", "curves", "meshes", "materials", "armatures", "collections")
    before = {kind: set(getattr(bpy.data, kind)) for kind in kinds}
    previous_bone_ids = {b.name: b.get("eiem_physics_id") for b in rig.data.bones} if rig else {}
    previous_resource_id = rig.get("eiem_physics_resource_id") if rig else None
    existing_rig = rig
    previous_group = bpy.context.scene.eiem_physics_group
    try:
        if rig is None:
            rig = API["make_armature"]("SkeletonPhysics", skeleton, bpy.context.scene.collection)
        records = API["skeleton_author_nodes"](rig)
        by_path = {record[0]: bone for bone, record, source in records}
        existing = {o.eiem_physics.identity for o in bpy.data.objects if o.eiem_physics.rig == rig}
        document.require(not existing.intersection(c["id"] for c in payload["colliders"] + payload["groups"]),
                         "当前 Rig 已包含同身份 Physics，请导入到新的 Rig")
        made_groups = []
        for g in payload["groups"]:
            obj = create_group(rig, [by_path[n["bone"]] for n in g["nodes"]], g["name"])
            obj.eiem_physics.identity = g["id"]
            if obj.animation_data and obj.animation_data.action:
                obj.animation_data.action[AUTHOR_CURVE_ACTION_MARKER] = g["id"]
            obj["eiem_physics_source_file"] = str(filename)
            for n, record in zip(obj.eiem_physics.nodes, g["nodes"]):
                n.role = record["role"]
            for key, value in g["parameters"].items():
                setattr(obj.eiem_physics, key, value)
            load_author_radius(obj, g.get("radius", document.default_radius()))
            if g.get("nativeParameters") is not None:
                load_author_native_fields(obj, g["nativeParameters"])
                activate_author_full_projection(obj)
            rebuild_group(obj)
            made_groups.append(obj)
        by_id = {}
        for c in payload["colliders"]:
            obj = create_collider(made_groups[0], by_path[c["bone"]], c["shape"], c["name"])
            p = obj.eiem_physics
            p.identity, p.radius, p.span = c["id"], c["radius"], c["span"]
            q = c["rotation"]
            native_matrix = Matrix.LocRotScale(Vector(c["position"]), Quaternion((q[3],q[0],q[1],q[2])), Vector((1,1,1)))
            obj.matrix_basis = API["unity_transform_matrix_to_blender"](native_matrix)
            by_id[c["id"]] = obj
        made_groups[0].eiem_physics.colliders.clear()
        for obj, g in zip(made_groups, payload["groups"]):
            for cid in g["colliders"]:
                obj.eiem_physics.colliders.add().object = by_id[cid]
        rig["eiem_physics_resource_id"] = payload["id"]
        bpy.context.view_layer.update()
        return rig, made_groups
    except Exception:
        bpy.context.scene.eiem_physics_group = previous_group
        for kind in kinds:
            for item in set(getattr(bpy.data, kind)) - before[kind]:
                getattr(bpy.data, kind).remove(item, do_unlink=True)
        if existing_rig:
            for bone in existing_rig.data.bones:
                value = previous_bone_ids[bone.name]
                if value is None:
                    if "eiem_physics_id" in bone:
                        del bone["eiem_physics_id"]
                else:
                    bone["eiem_physics_id"] = value
            if previous_resource_id is None:
                if "eiem_physics_resource_id" in existing_rig:
                    del existing_rig["eiem_physics_resource_id"]
            else:
                existing_rig["eiem_physics_resource_id"] = previous_resource_id
        raise


def skeleton_equal(records, payload):
    if len(records) != len(payload["nodes"]):
        return False
    for (bone, left, source), right, flag in zip(records, payload["nodes"], payload["source_nodes"]):
        if left[:2] != right[:2] or source != flag:
            return False
        if any(abs(a-b) > 1e-5 for av, bv in zip(left[2:], right[2:]) for a, b in zip(av, bv)):
            return False
    return True


class EIEM_PG_physics_node(bpy.types.PropertyGroup):
    bone_id: StringProperty()
    role: EnumProperty(name="节点角色", items=[("FIXED", "固定", ""), ("MOVE", "运动", ""), ("IGNORE", "忽略", "")],
                       update=node_role_updated)


class EIEM_PG_physics_reference(bpy.types.PropertyGroup):
    object: PointerProperty(type=bpy.types.Object,
        poll=lambda self, obj: obj.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER"))


class EIEM_PG_physics(bpy.types.PropertyGroup):
    kind: EnumProperty(items=[("NONE", "无", ""), ("GROUP", "物理组", ""), ("COLLIDER", "碰撞体", ""),
                              ("NATIVE_GROUP", "源物理组", ""), ("NATIVE_COLLIDER", "源碰撞体", "")])
    identity: StringProperty()
    label: StringProperty(name="名称")
    rig: PointerProperty(type=bpy.types.Object, poll=lambda self, obj: obj.type == "ARMATURE")
    bone_id: StringProperty()
    nodes: CollectionProperty(type=EIEM_PG_physics_node)
    colliders: CollectionProperty(type=EIEM_PG_physics_reference)
    collider_candidate: PointerProperty(
        name="添加已有碰撞体", type=bpy.types.Object, poll=collider_candidate_poll)
    visual: PointerProperty(type=bpy.types.Object)
    shape: EnumProperty(name="类型", items=[("SPHERE", "球体", ""), ("CAPSULE", "胶囊", "")], default="CAPSULE", update=rebuild_collider)
    radius: FloatProperty(name="半径", default=0.03, min=0.000001, update=rebuild_collider)
    span: FloatProperty(name="两端球心间距", default=0.1, min=0, update=rebuild_collider)
    gravity: FloatProperty(name="重力", default=9.8, min=0, update=author_shared_parameter_updated)
    stablizationTimeAfterReset: FloatProperty(
        name="重置后稳定时间", default=0.1, min=0, update=author_shared_parameter_updated)
    gravityFalloff: FloatProperty(
        name="重力衰减", default=0, min=0, max=1, update=author_shared_parameter_updated)
    blendWeight: FloatProperty(
        name="物理混合权重", default=1, min=0, max=1, update=author_shared_parameter_updated)
    animationPoseRatio: FloatProperty(
        name="动画姿态比例", default=0, min=0, max=1, update=author_shared_parameter_updated)
    node_radius: FloatProperty(
        name="节点基础半径", description="整组模拟点的基础碰撞半径；各点再乘以链位置曲线",
        default=0.006, min=0.000001, update=author_group_parameter_updated)
    radius_use_curve: bpy.props.BoolProperty(
        name="使用半径曲线", description="按根部到末端的归一化链位置改变每个模拟点半径",
        default=True, update=author_group_parameter_updated)
    radius_curve_snapshot: StringProperty(
        name="节点半径曲线数据", options={"HIDDEN"},
        description="内嵌浮点曲线的无损作者数据；界面只显示当前曲线")
    native_parameter_snapshot: StringProperty(
        name="完整原生参数快照", options={"HIDDEN"},
        description="由复制/粘贴保存的完整原生数值与曲线字段；不包含根骨、碰撞体引用或 SelectionData")
    show_structure: bpy.props.BoolProperty(name="结构编辑", default=False)


class EIEM_OT_physics_edit(bpy.types.Operator):
    bl_idname = "eiem.physics_edit"
    bl_label = "编辑物理"
    bl_options = {"REGISTER", "UNDO"}
    action: StringProperty()
    index: bpy.props.IntProperty(default=-1)
    target: StringProperty()

    def execute(self, context):
        try:
            obj, group = context.object, group_of(context)
            if group:
                context.scene.eiem_physics_group = group
            if self.action == "SELECT_BONES":
                document.require(group is not None, "请选择物理组")
                rig = group.eiem_physics.rig
                lookup = bones(rig)
                selected = []
                root = lookup.get(self.target, (None,))[0] if self.target else None
                for node in group.eiem_physics.nodes:
                    bone = lookup.get(node.bone_id, (None,))[0]
                    if not bone:
                        continue
                    if root:
                        cursor = bone
                        while cursor and cursor != root:
                            cursor = cursor.parent
                        if cursor != root:
                            continue
                    selected.append(bone.name)
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                for item in context.selected_objects:
                    item.select_set(False)
                rig.hide_set(False); rig.select_set(True); context.view_layer.objects.active = rig
                bpy.ops.object.mode_set(mode="POSE"); bpy.ops.pose.select_all(action="DESELECT")
                for name in selected:
                    rig.pose.bones[name].select = True
                if selected:
                    rig.data.bones.active = rig.data.bones[selected[0]]
                return {"FINISHED"}
            elif self.action == "FOCUS_GROUP":
                document.require(group is not None, "请选择当前物理组")
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                for item in context.selected_objects:
                    item.select_set(False)
                group.hide_set(False); group.select_set(True); context.view_layer.objects.active = group
                context.scene.eiem_physics_visibility = "CURRENT"
                native.apply_visibility(context.scene)
                return {"FINISHED"}
            elif self.action == "CREATE":
                rig = rig_of(obj)
                document.require(rig is not None, "请选择共享 Rig 上的骨骼")
                selection = [bone.name for bone in selected_bones(rig)]
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                created = create_group(rig, [rig.data.bones[name] for name in selection])
            elif self.action in ("ADD_NODES", "REMOVE_NODES"):
                document.require(group and group.eiem_physics.kind == "GROUP", "请选择新增物理组")
                rig = group.eiem_physics.rig
                selection = [bone.name for bone in selected_bones(rig)]
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                selected = [rig.data.bones[name] for name in selection]
                count = (add_selected_nodes(group, selected) if self.action == "ADD_NODES"
                         else remove_selected_nodes(group, selected))
                self.report({"INFO"}, ("已加入" if self.action == "ADD_NODES" else "已移除") +
                            " %d 根骨骼，物理链显示已刷新" % count)
                return {"FINISHED"}
            elif self.action == "CLONE_CHAIN":
                rig = rig_of(obj)
                document.require(rig is not None, "请选择共享 Rig 上的连续骨链")
                created = clone_selected_chain(rig)
            elif self.action == "COPY":
                document.require(group is not None, "请选择物理组")
                created = duplicate_group(group)
            elif self.action in document.SHAPES:
                document.require(group is not None, "请选择当前物理组")
                rig = group.eiem_physics.rig
                bone = rig.data.bones.active
                document.require(bone is not None, "请先在 Rig 上选中碰撞体绑定骨骼")
                name = bone.name
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                bone = rig.data.bones[name]
                created = create_collider(group, bone, self.action)
            elif self.action == "LINK":
                document.require(group and group.eiem_physics.kind == "GROUP" and obj and
                                 obj.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER") and
                                 obj.eiem_physics.rig == group.eiem_physics.rig, "请选择同 Rig 的碰撞体和当前物理组")
                if not any(ref.object == obj for ref in group.eiem_physics.colliders):
                    group.eiem_physics.colliders.add().object = obj
                native.apply_visibility(context.scene)
                return {"FINISHED"}
            elif self.action == "LINK_CANDIDATE":
                document.require(group and group.eiem_physics.kind == "GROUP", "请选择新增物理组")
                collider = group.eiem_physics.collider_candidate
                document.require(collider and collider.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER") and
                                 collider.eiem_physics.rig == group.eiem_physics.rig,
                                 "请选择同 Rig 的已有碰撞体")
                if not any(ref.object == collider for ref in group.eiem_physics.colliders):
                    group.eiem_physics.colliders.add().object = collider
                group.eiem_physics.collider_candidate = None
                native.apply_visibility(context.scene)
                self.report({"INFO"}, "已加入碰撞集合：" + collider.eiem_physics.label)
                return {"FINISHED"}
            elif self.action == "SELECT_COLLIDER":
                collider = bpy.data.objects.get(self.target)
                document.require(collider and collider.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER"),
                                 "碰撞体已删除")
                if context.mode != "OBJECT":
                    bpy.ops.object.mode_set(mode="OBJECT")
                for selected in context.selected_objects:
                    selected.select_set(False)
                collider.hide_set(False); collider.select_set(True); context.view_layer.objects.active = collider
                return {"FINISHED"}
            elif self.action == "UNLINK":
                document.require(group and 0 <= self.index < len(group.eiem_physics.colliders), "引用已变化")
                group.eiem_physics.colliders.remove(self.index)
                native.apply_visibility(context.scene)
                return {"FINISHED"}
            elif self.action == "REBIND":
                document.require(obj and obj.eiem_physics.kind == "COLLIDER", "请选择碰撞体")
                lookup = bones(obj.eiem_physics.rig)
                document.require(obj.eiem_physics.bone_id in lookup, "绑定骨骼已删除")
                bind_collider(obj, lookup[obj.eiem_physics.bone_id][0])
                return {"FINISHED"}
            elif self.action == "REBIND_ACTIVE":
                document.require(obj and obj.eiem_physics.kind == "COLLIDER", "请选择碰撞体")
                rig = obj.eiem_physics.rig
                document.require(rig and rig.data.bones.active, "请先在 Rig 上选择目标骨骼")
                bind_collider(obj, rig.data.bones.active)
                return {"FINISHED"}
            elif self.action == "DELETE_COLLIDER":
                document.require(obj and obj.eiem_physics.kind == "COLLIDER", "请选择碰撞体")
                for owner in bpy.data.objects:
                    if owner.eiem_physics.kind == "GROUP":
                        for i in reversed(range(len(owner.eiem_physics.colliders))):
                            if owner.eiem_physics.colliders[i].object == obj:
                                owner.eiem_physics.colliders.remove(i)
                display = obj.eiem_physics.visual
                if display and display.parent == obj and display.type in ("CURVE", "MESH"):
                    obj.eiem_physics.visual = None
                    native.remove_visual(display)
                bpy.data.objects.remove(obj, do_unlink=True)
                return {"FINISHED"}
            else:
                raise ValueError("未知 Physics 操作")
            if context.mode != "OBJECT":
                bpy.ops.object.mode_set(mode="OBJECT")
            for item in context.selected_objects:
                item.select_set(False)
            created.select_set(True)
            context.view_layer.objects.active = created
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


class EIEM_OT_physics_export(ExportHelper, bpy.types.Operator):
    bl_idname = "eiem.export_physics"
    bl_label = "导出 Physics 作者包"
    filename_ext = ".physics"
    filter_glob: StringProperty(default="*.physics", options={"HIDDEN"})

    def draw(self, context):
        self.layout.label(text="包含所选物理组及其共享碰撞体、完整骨架")
        self.layout.label(text="当前为作者资源；游戏物理装配仍待接入")

    def execute(self, context):
        try:
            groups = [o for o in context.selected_objects if o.eiem_physics.kind in ("GROUP", "NATIVE_GROUP", "NATIVE_COLLIDER")]
            result = export_physics(self.filepath, groups)
            self.report({"INFO"}, "已导出 %(groups)d 组、%(colliders)d 碰撞体、1 份骨架" % result)
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


class EIEM_OT_physics_import(ImportHelper, bpy.types.Operator):
    bl_idname = "eiem.import_physics"
    bl_label = "导入 Physics 作者包"
    filename_ext = ".physics"
    filter_glob: StringProperty(default="*.physics", options={"HIDDEN"})

    def execute(self, context):
        try:
            rig = context.object if context.object and context.object.type == "ARMATURE" else None
            import_physics(self.filepath, rig)
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


class EIEM_OT_physics_parameters(bpy.types.Operator):
    bl_idname = "eiem.physics_parameters"
    bl_label = "复制或粘贴物理参数"
    bl_options = {"REGISTER", "UNDO"}
    action: EnumProperty(items=(("COPY", "复制参数", ""), ("PASTE", "粘贴参数", "")))

    @classmethod
    def poll(cls, context):
        return group_of(context) is not None

    def execute(self, context):
        try:
            group = group_of(context)
            if self.action == "COPY":
                count = copy_group_parameters(context.scene, group)
                payload = _read_parameter_clipboard(context.scene)
                self.report({"INFO"}, "已复制 %s：%d 个参数字段、%d 个碰撞体引用" %
                            (group.eiem_physics.label, count, len(payload.get("colliderRefs", ()))))
            elif self.action == "PASTE":
                count = paste_group_parameters(context.scene, group)
                payload = _read_parameter_clipboard(context.scene)
                source_name = payload.get("sourceName", "参数模板")
                if group.eiem_physics.kind == "GROUP" and payload.get("nativeFields") is not None:
                    message = "已将 %s 的 %d 项完整参数和 %d 个碰撞体引用粘贴到 %s" % (
                        source_name, count, len(payload.get("colliderRefs", ())), group.eiem_physics.label)
                else:
                    message = "已将 %s 的 %d 个参数字段粘贴到 %s" % (
                        source_name, count, group.eiem_physics.label)
                self.report({"INFO"}, message)
            else:
                document.require(False, "未知参数操作")
            return {"FINISHED"}
        except Exception as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


class EIEM_MT_physics_create(bpy.types.Menu):
    bl_label = "创建 / 导入"
    bl_idname = "EIEM_MT_physics_create"

    def draw(self, context):
        layout = self.layout
        layout.operator("eiem.physics_edit", text="从所选骨骼新建物理链", icon="BONE_DATA").action = "CREATE"
        layout.operator("eiem.physics_edit", text="复制所选骨骼并新建物理链", icon="DUPLICATE").action = "CLONE_CHAIN"
        group = group_of(context)
        if group and group.eiem_physics.kind == "GROUP":
            layout.separator()
            layout.operator("eiem.physics_edit", text="在活动骨骼添加球体", icon="MESH_UVSPHERE").action = "SPHERE"
            layout.operator("eiem.physics_edit", text="在活动骨骼添加胶囊", icon="MESH_CAPSULE").action = "CAPSULE"
        layout.separator()
        layout.operator("eiem.import_native_physics", text="导入游戏源物理", icon="IMPORT")
        layout.operator("eiem.import_physics", text="导入 Physics 作者包", icon="IMPORT")


def active_group_node_sample(context, group):
    rig = group.eiem_physics.rig if group else None
    if not rig or context.object != rig:
        return None
    bone = rig.data.bones.active
    if not bone or not bone.select:
        return None
    if native.is_native(group):
        return native.native_bone_sample(group, bone)
    return author_bone_sample(group, bone)


def draw_active_group_node(layout, context, group):
    sample = active_group_node_sample(context, group)
    if sample is None:
        return
    box = layout.box()
    role = {"FIXED": "固定", "MOVE": "运动", "IGNORE": "忽略"}.get(
        sample["role"], sample["role"])
    box.label(text="当前节点：%s · %s" % (sample["bone"].name, role),
              icon="BONE_DATA")
    depth = float(sample["depth"])
    if native.is_parameter_group(group):
        parameter = native.active_curve_parameter(group.eiem_native_physics)
        label = dict(native.CURVE_PARAMETERS)[parameter]
        base = float(native.native_number(group, parameter + ".value"))
        enabled = bool(native.native_number(group, parameter + ".useCurve"))
        multiplier = native.curve_parameter_multiplier(group, parameter, depth)
    else:
        parameter = native.NODE_RADIUS_PARAMETER
        label = "节点半径"
        base = float(group.eiem_physics.node_radius)
        enabled = bool(group.eiem_physics.radius_use_curve)
        curve_node = native.curve_mapping_node(group, parameter)
        multiplier = (float(curve_node.mapping.evaluate(curve_node.mapping.curves[0], depth))
                      if enabled and curve_node else 1.0)
    box.label(text="链位置 %.3f · %s倍率 %.4g" % (depth, label, multiplier))
    box.label(text="基础值 %.5g · 当前实际值 %.5g" % (base, base * multiplier))
    curve_node = native.curve_mapping_node(group, parameter)
    exists = curve_node is not None and any(
        abs(point.location.x - depth) < 1e-5 for point in native.curve_mapping_points(curve_node))
    row = box.row()
    row.enabled = bool(enabled and curve_node is not None and not exists)
    op = row.operator("eiem.physics_curve_key", text="在此位置增加关键点", icon="ADD")
    op.action, op.parameter, op.position = "ADD", parameter, depth
    if exists:
        box.label(text="此链位置已有关键点")


class EIEM_PT_physics(bpy.types.Panel):
    bl_label = "物理骨骼"
    bl_idname = "EIEM_PT_physics"
    bl_space_type, bl_region_type, bl_category = "VIEW_3D", "UI", "EIEM"

    def draw(self, context):
        layout = self.layout
        layout.prop(context.scene, "eiem_physics_group", text="当前组")
        row = layout.row(align=True)
        row.menu("EIEM_MT_physics_create", text="新建", icon="ADD")
        group = group_of(context)
        if group:
            op = row.operator("eiem.physics_parameters", text="复制参数", icon="COPYDOWN"); op.action = "COPY"
            op = row.operator("eiem.physics_parameters", text="粘贴参数", icon="PASTEDOWN"); op.action = "PASTE"
            row.operator("eiem.physics_edit", text="查看", icon="RESTRICT_VIEW_OFF").action = "FOCUS_GROUP"
            layout.label(text="参数：选中 Empty → 对象属性 → EIEM 物理参数", icon="INFO")
            draw_active_group_node(layout, context, group)
        structure_header, structure = layout.panel("eiem_physics_structure_tools", default_closed=True)
        structure_header.label(text="链与碰撞体", icon="BONE_DATA")
        if structure:
            if group and group.eiem_physics.kind == "GROUP":
                structure.operator("eiem.physics_edit", text="选择当前骨链").action = "SELECT_BONES"
                structure.operator("eiem.physics_edit", text="将所选骨骼加入当前链").action = "ADD_NODES"
                structure.operator("eiem.physics_edit", text="从当前链移除所选骨骼").action = "REMOVE_NODES"
            elif native.is_native(group):
                structure.operator("eiem.select_native_physics_bones", text="选择物理骨骼")
                structure.operator("eiem.select_native_physics", text="选择引用碰撞体")
            obj = context.object
            if obj and obj.eiem_physics.kind in ("COLLIDER", "NATIVE_COLLIDER"):
                structure.operator("eiem.physics_edit", text="加入当前物理组").action = "LINK"
                if obj.eiem_physics.kind == "COLLIDER":
                    structure.operator("eiem.physics_edit", text="绑定到 Rig 活动骨骼").action = "REBIND_ACTIVE"
                    structure.operator("eiem.physics_edit", text="刷新骨骼绑定").action = "REBIND"
                    structure.operator("eiem.physics_edit", text="删除碰撞体及引用").action = "DELETE_COLLIDER"
        view_header, view = layout.panel("eiem_physics_view_settings", default_closed=True)
        view_header.label(text="显示", icon="HIDE_OFF")
        if view:
            view.prop(context.scene, "eiem_physics_visibility", text="范围")
            view.prop(context.scene, "eiem_physics_preview_style", text="样式")
            view.prop(context.scene, "eiem_physics_xray", text="穿透模型")
        file_header, files = layout.panel("eiem_physics_file_tools", default_closed=True)
        file_header.label(text="文件与整理", icon="FILE_FOLDER")
        if files:
            files.operator("eiem.organize_physics", text="整理当前 Rig 的物理集合")
            files.operator("eiem.export_physics", text="导出所选物理组")


class EIEM_PT_physics_object(bpy.types.Panel):
    bl_label = "EIEM 物理参数"
    bl_idname = "OBJECT_PT_eiem_physics"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    @classmethod
    def poll(cls, context):
        return bool(context.object and hasattr(context.object, "eiem_physics") and
                    context.object.eiem_physics.kind != "NONE")

    def draw(self, context):
        layout, obj, data = self.layout, context.object, context.object.eiem_physics
        if native.is_native(obj):
            native.draw(layout, context, obj)
            return
        layout.prop(data, "label", text="名称")
        layout.label(text=("作者 v4 物理组" if data.kind == "GROUP" else "作者碰撞体") +
                     " · 数据保存在此 Empty", icon="OUTLINER_OB_EMPTY")
        if data.kind == "GROUP":
            if native.is_parameter_group(obj):
                native.draw_group_parameter_controls(layout, obj)
            else:
                parameter_box = layout.box(); parameter_box.label(text="整组共享参数", icon="PREFERENCES")
                for path, label in native.COMMON_PARAMETERS:
                    parameter_box.prop(data, path.rsplit(".", 1)[-1], text=label)
                radius_box = layout.box(); radius_box.label(text="模拟点碰撞半径", icon="MESH_UVSPHERE")
                radius_box.prop(data, "node_radius")
                radius_box.prop(data, "radius_use_curve")
                node = native.curve_mapping_node(obj, native.NODE_RADIUS_PARAMETER)
                if node is not None:
                    native.draw_curve_distribution(
                        radius_box, obj, native.NODE_RADIUS_PARAMETER,
                        bool(data.radius_use_curve))
            lookup = {str(b.get("eiem_physics_id")): b for b in data.rig.data.bones
                      if b.get("eiem_physics_id")} if data.rig else {}
            names = [lookup[node.bone_id].name if node.bone_id in lookup else "骨骼已删除"
                     for node in data.nodes]
            layout.label(text="骨链：" + " → ".join(names[:5]) + ("…" if len(names) > 5 else ""), icon="BONE_DATA")
            collider_box = layout.box(); collider_box.label(
                text="碰撞集合：%d 个（本物理组使用）" % len(data.colliders), icon="MESH_CAPSULE")
            for index, ref in enumerate(data.colliders):
                row = collider_box.row(align=True)
                row.label(text=ref.object.eiem_physics.label if ref.object else "碰撞体已删除")
                if ref.object:
                    op = row.operator("eiem.physics_edit", text="", icon="RESTRICT_SELECT_OFF")
                    op.action, op.target = "SELECT_COLLIDER", ref.object.name_full
                op = row.operator("eiem.physics_edit", text="", icon="X")
                op.action, op.index = "UNLINK", index
            candidate = collider_box.row(align=True)
            candidate.prop(data, "collider_candidate", text="")
            candidate.operator("eiem.physics_edit", text="加入", icon="ADD").action = "LINK_CANDIDATE"
            if any(ref.object and ref.object.eiem_physics.kind == "NATIVE_COLLIDER" for ref in data.colliders):
                collider_box.label(text="源碰撞体关联已保存在 .blend；DLL 转换留待碰撞阶段", icon="INFO")
            layout.prop(data, "show_structure", icon="TRIA_DOWN" if data.show_structure else "TRIA_RIGHT",
                        emboss=False)
            if data.show_structure:
                for node in data.nodes:
                    node_box = layout.row(); node_box.prop(node, "role", text=lookup[node.bone_id].name if node.bone_id in lookup else "已删除")
            if native.is_parameter_group(obj):
                native.draw_parameter_fields(layout, obj, "BeyondBoneCloth",
                                             allow_refresh=True, include_disabled=False)
        elif data.kind == "COLLIDER":
            lookup = {str(b.get("eiem_physics_id")): b.name for b in data.rig.data.bones
                      if b.get("eiem_physics_id")} if data.rig else {}
            layout.label(text="绑定骨骼：" + lookup.get(data.bone_id, "已删除"), icon="CONSTRAINT_BONE")
            layout.prop(data, "shape", text="形状")
            layout.prop(data, "radius")
            if data.shape == "CAPSULE":
                layout.prop(data, "span")
            layout.prop(obj, "location", text="骨骼局部位移")
            layout.prop(obj, "rotation_quaternion", text="局部旋转")
            users = [owner.name for owner in bpy.data.objects if owner.eiem_physics.kind == "GROUP" and
                     any(ref.object == obj for ref in owner.eiem_physics.colliders)]
            layout.label(text="引用组：" + ("、".join(users) if users else "无"))


class EIEM_OT_physics_organize(bpy.types.Operator):
    bl_idname = "eiem.organize_physics"
    bl_label = "整理当前 Rig 的物理集合"
    bl_options = {"REGISTER", "UNDO"}
    def execute(self, context):
        rig = rig_of(context.object)
        if rig is None:
            group = group_of(context)
            rig = group.eiem_physics.rig if group else None
        if rig is None:
            self.report({"ERROR"}, "请选择 EIEM Rig、物理组或碰撞体")
            return {"CANCELLED"}
        root = organize_rig(rig)
        native.apply_visibility(context.scene)
        self.report({"INFO"}, "已整理到 " + root.name + " / Groups / Colliders / Visuals")
        return {"FINISHED"}


CLASSES = (EIEM_PG_physics_node, EIEM_PG_physics_reference, EIEM_PG_physics,
           EIEM_OT_physics_edit, EIEM_OT_physics_parameters,
           EIEM_OT_physics_export, EIEM_OT_physics_import,
           EIEM_OT_physics_organize, EIEM_MT_physics_create,
           EIEM_PT_physics, EIEM_PT_physics_object)


def register(api):
    API.update(api)
    for cls in CLASSES:
        bpy.utils.register_class(cls)
    bpy.types.Object.eiem_physics = PointerProperty(type=EIEM_PG_physics)
    bpy.types.Scene.eiem_physics_group = PointerProperty(type=bpy.types.Object,
        poll=lambda self, obj: obj.eiem_physics.kind in ("GROUP", "NATIVE_GROUP"),
        update=physics_group_updated)
    bpy.types.Scene.eiem_physics_parameter_clipboard = StringProperty(options={"HIDDEN"})
    native.register(api["physics_authoring"], api)
    if not bpy.app.timers.is_registered(rebuild_existing_author_groups):
        bpy.app.timers.register(rebuild_existing_author_groups, first_interval=.05)
    if depsgraph_physics_updated not in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.append(depsgraph_physics_updated)
    bpy.msgbus.clear_by_owner(_MSGBUS_OWNER)
    bpy.msgbus.subscribe_rna(key=(bpy.types.LayerObjects, "active"), owner=_MSGBUS_OWNER,
                             args=(), notify=active_object_updated)
    active_object_updated()


def unregister():
    bpy.msgbus.clear_by_owner(_MSGBUS_OWNER)
    if depsgraph_physics_updated in bpy.app.handlers.depsgraph_update_post:
        bpy.app.handlers.depsgraph_update_post.remove(depsgraph_physics_updated)
    if bpy.app.timers.is_registered(flush_armature_visuals):
        bpy.app.timers.unregister(flush_armature_visuals)
    if bpy.app.timers.is_registered(rebuild_existing_author_groups):
        bpy.app.timers.unregister(rebuild_existing_author_groups)
    _DIRTY_ARMATURES.clear()
    native.unregister()
    del bpy.types.Scene.eiem_physics_parameter_clipboard
    del bpy.types.Scene.eiem_physics_group
    del bpy.types.Object.eiem_physics
    for cls in reversed(CLASSES):
        bpy.utils.unregister_class(cls)
    API.clear()
