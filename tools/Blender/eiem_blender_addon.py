bl_info = {
    "name": "EIEM Resource Package",
    "author": "EIEM",
    "version": (0, 32, 0),
    "blender": (3, 0, 0),
    "location": "File > Import/Export > EIEM package",
    "category": "Import-Export",
}

import configparser
import hashlib
import json
import os
import re
import shutil
import struct
import zlib
import tempfile
import math
import uuid
import importlib.util
from pathlib import Path
from collections import defaultdict

import bpy
from bpy.props import StringProperty, BoolProperty, PointerProperty, FloatProperty, IntProperty, CollectionProperty
from bpy_extras.io_utils import ImportHelper, ExportHelper
from mathutils import Matrix, Quaternion, Vector

if __package__:
    from . import eiem_physics_authoring as physics_authoring
    from . import eiem_blender_controls as controls
else:
    _physics_spec = importlib.util.spec_from_file_location(
        "eiem_physics_authoring", Path(__file__).with_name("eiem_physics_authoring.py"))
    physics_authoring = importlib.util.module_from_spec(_physics_spec)
    _physics_spec.loader.exec_module(physics_authoring)
    _controls_spec = importlib.util.spec_from_file_location(
        "eiem_blender_controls", Path(__file__).with_name("eiem_blender_controls.py"))
    controls = importlib.util.module_from_spec(_controls_spec)
    _controls_spec.loader.exec_module(controls)


# Preserve the standalone add-on's public Python surface for authoring scripts
# while keeping control behavior in one focused module.
parse_json_property = controls.parse_json_property
author_identity = controls.author_identity
add_shape_control = controls.add_shape_control
sync_new_shape_controls = controls.sync_new_shape_controls
shape_channel_name = controls.shape_channel_name
switch_groups = controls.switch_groups
switch_states = controls.switch_states
switch_state_values = controls.switch_state_values
switch_meshes = controls.switch_meshes
switch_members = controls.switch_members
capture_switch_state = controls.capture_switch_state
validate_switch_key = controls.validate_switch_key
switch_key_from_event = controls.switch_key_from_event
set_switch_group_key = controls.set_switch_group_key
set_shape_control_hotkey = controls.set_shape_control_hotkey
add_switch_state = controls.add_switch_state
set_switch_state_order = controls.set_switch_state_order
reorder_switch_state = controls.reorder_switch_state
set_switch_default = controls.set_switch_default
restore_switch_preview = controls.restore_switch_preview
preview_switch = controls.preview_switch
assign_switch_meshes = controls.assign_switch_meshes
create_switch_group = controls.create_switch_group
delete_switch_group = controls.delete_switch_group
mesh_source_identity = controls.mesh_source_identity
plan_switch_export = controls.plan_switch_export
plan_shape_controls = controls.plan_shape_controls
lua_string = controls.lua_string
generate_mod_ui = controls.generate_mod_ui


# LOD is part of the imported resource identity, not Blender's display name.
# Keep the pattern deliberately narrow so names such as "lodger" do not become
# an invented game LOD.  The exporter exposes levels 0..4, while discovery can
# still report an out-of-range level as unsupported metadata.
LOD_TOKEN_RE = re.compile(r"(?<![a-z0-9])lod([0-9]+)(?![a-z0-9])", re.IGNORECASE)


class _VariantData:
    """Read-only metadata view over a Blender Mesh for one target LOD."""

    def __init__(self, source, overrides):
        self._source = source
        self._overrides = dict(overrides)

    def get(self, key, default=None):
        if key in self._overrides:
            return self._overrides[key]
        return self._source.get(key, default)

    def __getitem__(self, key):
        if key in self._overrides:
            return self._overrides[key]
        return self._source[key]

    def __contains__(self, key):
        return key in self._overrides or key in self._source

    def __getattr__(self, name):
        return getattr(self._source, name)


class _MeshExportVariant:
    """Delegate Blender object data while exposing another LOD identity.

    Export functions only read the object.  A view avoids mutating the .blend
    and lets one authored template feed several concrete LOD resources.
    """

    def __init__(self, source, target_level, overrides, data_overrides):
        self._source = source
        self._target_level = target_level
        self._overrides = dict(overrides)
        self._data = _VariantData(source.data, data_overrides)
        self.name = "%s_LOD%d" % (source.name, target_level)

    @property
    def data(self):
        return self._data

    def get(self, key, default=None):
        if key in self._overrides:
            return self._overrides[key]
        return self._source.get(key, default)

    def __getitem__(self, key):
        if key in self._overrides:
            return self._overrides[key]
        return self._source[key]

    def __contains__(self, key):
        return key in self._overrides or key in self._source

    def __getattr__(self, name):
        return getattr(self._source, name)


def _lod_values(obj):
    values = [
        obj.get("eiem_render_asset", ""),
        obj.data.get("eiem_target_asset", ""),
        obj.data.get("eiem_target_path", ""),
        obj.data.get("eiem_source", ""),
        obj.data.get("eiem_section", ""),
        obj.get("eiem_render_section", ""),
    ]
    return [str(value).strip() for value in values if str(value).strip()]


def mesh_lod_level(obj):
    """Return the single LOD encoded by imported resource metadata."""
    levels = {
        int(match.group(1))
        for value in _lod_values(obj)
        for match in LOD_TOKEN_RE.finditer(value)
    }
    if len(levels) > 1:
        raise ValueError("Mesh %s has inconsistent LOD metadata: %s" %
                         (obj.name, ", ".join(str(value) for value in sorted(levels))))
    return next(iter(levels), None)


def _replace_lod(value, target_level):
    value = str(value or "")
    def replace(match):
        token = match.group(0)
        return token[:3] + str(int(target_level))
    return LOD_TOKEN_RE.sub(replace, value)


def mesh_lod_family(obj):
    """Return a package-scoped identity with its LOD token normalized.

    A game's LOD0 may be serialized in a dedicated ``.asset`` while later
    levels live in a shared FBX.  The logical target asset is therefore the
    stable family key; source path is only a fallback for legacy records that
    have no asset identity.
    """
    package = str(obj.get("eiem_author_package", "")).replace("\\", "/").lower()
    asset = str(obj.get("eiem_render_asset", "") or
                obj.data.get("eiem_target_asset", obj.data.get("eiem_asset", "")))
    source = str(obj.data.get("eiem_target_path", "") or
                 obj.data.get("eiem_source", ""))
    if asset.strip() and LOD_TOKEN_RE.search(asset):
        return (package, "asset", LOD_TOKEN_RE.sub("{lod}", asset.lower()))
    if source.strip():
        return (package, "source", LOD_TOKEN_RE.sub("{lod}", source.replace("\\", "/").lower()))
    return (package, "asset", asset.lower())


def discover_mesh_lods(obj, candidates=None):
    """Discover LODs actually imported for the same resource family."""
    candidates = candidates if candidates is not None else bpy.data.objects
    family = mesh_lod_family(obj)
    levels = set()
    for candidate in candidates:
        if candidate.type != "MESH" or not candidate.data.get("eiem_section"):
            continue
        if mesh_lod_family(candidate) != family:
            continue
        level = mesh_lod_level(candidate)
        if level is not None:
            levels.add(level)
    return levels


def _lod_variant(obj, target_level, candidates):
    source_level = mesh_lod_level(obj)
    if source_level is None:
        raise ValueError("Mesh %s has no LOD token to replicate" % obj.name)
    observed = next((candidate for candidate in candidates
                     if candidate.type == "MESH"
                     and candidate.data.get("eiem_section")
                     and mesh_lod_family(candidate) == mesh_lod_family(obj)
                     and mesh_lod_level(candidate) == target_level), None)
    object_overrides = {}
    data_overrides = {}
    metadata = observed or obj
    for owner, key in ((object_overrides, "eiem_render_asset"),
                       (object_overrides, "eiem_render_section"),
                       (data_overrides, "eiem_section"),
                       (data_overrides, "eiem_source"),
                       (data_overrides, "eiem_asset"),
                       (data_overrides, "eiem_target_path"),
                       (data_overrides, "eiem_target_asset")):
        current = (metadata.get(key, "") if owner is object_overrides else
                   metadata.data.get(key, ""))
        if str(current).strip():
            owner[key] = (current if observed else
                          _replace_lod(current, target_level))
    return _MeshExportVariant(obj, target_level, object_overrides, data_overrides)


def expand_lod_plan(plan, target_levels, candidates=None):
    """Expand a normal export plan into independently addressable LOD views.

    A selected object is emitted for a target only when that target LOD was
    observed in the imported package family. Objects without an LOD token are
    stable resources and remain in every plan. Switch bindings and hidden
    declarations are copied to each view, so all LODs share one state variable.
    """
    target_levels = {int(level) for level in (target_levels or ())}
    if any(level < 0 or level > 4 for level in target_levels):
        raise ValueError("LOD export supports levels 0 through 4")
    candidates = candidates if candidates is not None else bpy.data.objects
    variants = []
    bindings = {}
    hidden = set()
    for obj in plan["objects"]:
        source_level = mesh_lod_level(obj)
        if source_level is None:
            emitted = [obj]
        else:
            available = discover_mesh_lods(obj, candidates)
            emitted = []
            for target in sorted(target_levels):
                if target not in available:
                    continue
                emitted.append(obj if target == source_level else
                               _lod_variant(obj, target, candidates))
        for variant in emitted:
            variants.append(variant)
            if obj in plan["bindings"]:
                bindings[variant] = plan["bindings"][obj]
            if obj in plan["hidden"]:
                hidden.add(variant)
    if not variants:
        raise ValueError("所选目标 LOD 在当前导入资源中不存在")

    grouped = defaultdict(list)
    for obj in sorted(variants, key=lambda item: (mesh_source_identity(item), item.name)):
        grouped[mesh_source_identity(obj)].append(obj)
    return {
        "objects": [obj for values in grouped.values() for obj in values],
        "sources": list(grouped.values()),
        "groups": plan["groups"],
        "bindings": bindings,
        "hidden": hidden,
    }


def lod_levels_for_export(mesh_objects, selected_levels=None, all_levels=False):
    """Resolve checkbox state to discovered levels, never filename guesses."""
    discovered = set()
    for obj in mesh_objects:
        level = mesh_lod_level(obj)
        if level is not None:
            discovered.update(discover_mesh_lods(obj))
    if all_levels:
        return sorted(level for level in discovered if 0 <= level <= 4)
    return sorted(set(selected_levels or ()) & discovered)


MAGIC_MESH = b"EIEMESH\0"
MAGIC_SKEL = b"EIESKEL\0"


def is_unity_left_handed(coordinate):
    value = str(coordinate or "").lower()
    # model-z-up-left-handed was the experimental marker emitted by older
    # AnimeStudio builds. Those files still contain raw Unity buffers, so keep
    # it as a compatibility alias while new files use the explicit marker.
    return value in {"unity-y-up-left-handed", "model-z-up-left-handed"}


def unity_to_blender(value):
    """Convert Unity's left-handed Y-up vectors to Blender's right-handed space."""
    x, y, z = value
    return (-x, y, z)


def blender_to_unity(value):
    """Inverse of unity_to_blender for exported mesh channels."""
    x, y, z = value
    return (-x, y, z)


def unity_transform_matrix_to_blender_basis():
    return Matrix(((-1.0, 0.0, 0.0, 0.0),
                   (0.0, 0.0, -1.0, 0.0),
                   (0.0, 1.0, 0.0, 0.0),
                   (0.0, 0.0, 0.0, 1.0)))


def unity_transform_matrix_to_blender(matrix):
    """Move a Unity Y-up Transform matrix into the Z-up mesh editing space.

    Endfield's serialized Mesh buffers are already authored in Z-up model
    space, while prefab Transform TRS uses Unity's Y-up space.  The X flip
    changes handedness and the -Z/Y mapping puts both resources in the same
    Blender basis.  Conjugating the complete matrix preserves rotation and
    non-uniform scale instead of trying to remap quaternion components.
    """
    basis = unity_transform_matrix_to_blender_basis()
    return basis @ matrix @ basis.inverted()


class Reader:
    def __init__(self, data):
        self.data = data
        self.pos = 0

    def take(self, size):
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError("truncated EIEM file")
        value = self.data[self.pos:self.pos + size]
        self.pos += size
        return value

    def u8(self):
        return self.take(1)[0]

    def i32(self):
        return struct.unpack("<i", self.take(4))[0]

    def u32(self):
        return struct.unpack("<I", self.take(4))[0]

    def f32(self):
        return struct.unpack("<f", self.take(4))[0]

    def string(self):
        value = 0
        shift = 0
        while True:
            byte = self.u8()
            value |= (byte & 0x7f) << shift
            if not byte & 0x80:
                break
            shift += 7
            if shift > 28:
                raise ValueError("invalid EIEM string length")
        return self.take(value).decode("utf-8", "replace")

    def floats(self):
        count = self.i32()
        if count < 0 or count > 100000000:
            raise ValueError("invalid EIEM array length")
        return list(struct.unpack("<" + "f" * count, self.take(count * 4))) if count else []


class Writer:
    def __init__(self):
        self.data = bytearray()

    def raw(self, value):
        self.data.extend(value)

    def i32(self, value):
        self.raw(struct.pack("<i", int(value)))

    def u32(self, value):
        self.raw(struct.pack("<I", int(value)))

    def f32(self, value):
        self.raw(struct.pack("<f", float(value)))

    def string(self, value):
        encoded = str(value or "").encode("utf-8")
        length = len(encoded)
        while length >= 0x80:
            self.raw(bytes(((length & 0x7f) | 0x80,)))
            length >>= 7
        self.raw(bytes((length,)))
        self.raw(encoded)

    def floats(self, values):
        values = list(values or [])
        self.i32(len(values))
        if values:
            self.raw(struct.pack("<" + "f" * len(values), *values))


def parse_ini(root):
    # Runtime Render sections may contain EIEM's conditional program syntax.
    # ConfigParser accepts the indented assignments inside those blocks, but
    # bare ``if``/``endif`` statements are not INI options and make an
    # exported package impossible to import back into Blender.  The importer
    # needs the declared resources and bindings, not execution of the switch
    # program, so remove only control-flow statements before parsing.
    lines = []
    control_words = {"if", "elif", "else", "endif"}
    with open(Path(root) / "mod.ini", "r", encoding="utf-8-sig") as stream:
        for line in stream:
            stripped = line.strip()
            word = stripped.split(None, 1)[0].lower() if stripped else ""
            if word in control_words and "=" not in stripped:
                continue
            lines.append(line)
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    parser.read_string("".join(lines), source=str(Path(root) / "mod.ini"))
    return parser


def parse_material_resources(root):
    """Read static declarations, not the Mod's executable Render/Key bodies."""
    lines, include = [], False
    with open(Path(root) / "mod.ini", "r", encoding="utf-8-sig") as stream:
        for line in stream:
            stripped = line.strip()
            if stripped.startswith("[") and stripped.endswith("]"):
                include = stripped[1:-1].lower().startswith(("material", "texture"))
            if include:
                lines.append(line)
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    parser.read_string("".join(lines))
    return {s: dict(parser.items(s)) for s in parser.sections()}


def read_flat_properties(path):
    values = {}
    with open(path, "r", encoding="utf-8-sig") as stream:
        for line in stream:
            line = line.strip()
            if not line or line.startswith((";", "#")) or "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[key.strip()] = value.strip()
    return values


def safe_path(root, relative):
    path = (Path(root) / relative).resolve()
    root = Path(root).resolve()
    if path != root and root not in path.parents:
        raise ValueError("EIEM path escapes package directory")
    return path


def read_mesh(path):
    reader = Reader(path.read_bytes())
    if reader.take(8) != MAGIC_MESH:
        raise ValueError("not an EIEM mesh")
    version = reader.i32()
    if version not in (2, 3):
        raise ValueError(f"unsupported EIEM mesh version {version}; re-export with the current AnimeStudio")
    coordinate = reader.string()
    source = reader.string()
    name = reader.string()
    vertex_count = reader.i32()
    if vertex_count < 0:
        raise ValueError("invalid vertex count")
    vertices = reader.floats()
    normals = reader.floats()
    tangents = reader.floats()
    colors = reader.floats()
    uvs = [reader.floats() for _ in range(8)]
    index_count = reader.i32()
    indices = [reader.u32() for _ in range(max(0, index_count))]
    submesh_count = reader.i32()
    submeshes = []
    for _ in range(max(0, submesh_count)):
        submeshes.append((reader.i32(), reader.u32(), reader.u32(), reader.u32(), reader.u32(), reader.u32()))
    skin_count = reader.i32()
    skin = []
    for _ in range(max(0, skin_count)):
        weights = [reader.f32() for _ in range(4)]
        bones = [reader.u32() for _ in range(4)]
        skin.append((weights, bones))
    bind_count = reader.i32()
    bindposes = [list(struct.unpack("<16f", reader.take(64))) for _ in range(max(0, bind_count))]
    bone_hashes = [reader.u32() for _ in range(max(0, reader.i32()))]
    bone_paths = [reader.string() for _ in range(max(0, reader.i32()))] if version >= 3 else []
    blend_vertex_count = reader.i32()
    blend_vertices = []
    for _ in range(max(0, blend_vertex_count)):
        blend_vertices.append((
            reader.u32(),
            tuple(reader.f32() for _ in range(3)),
            tuple(reader.f32() for _ in range(3)),
            tuple(reader.f32() for _ in range(3)),
        ))
    blend_frame_count = reader.i32()
    blend_frames = []
    for _ in range(max(0, blend_frame_count)):
        blend_frames.append((
            reader.string(), reader.u32(), reader.u32(),
            bool(reader.u8()), bool(reader.u8()), bool(reader.u8()),
        ))
    blend_channel_count = reader.i32()
    blend_channels = []
    for _ in range(max(0, blend_channel_count)):
        blend_channels.append((reader.string(), reader.u32(), reader.u32(), reader.u32()))
    blend_weights = reader.floats()
    additional_count = reader.i32()
    additional = [tuple(reader.f32() for _ in range(3))
                  for _ in range(max(0, additional_count))]
    if vertex_count and len(vertices) < vertex_count * 3:
        raise ValueError("mesh vertex data is incomplete")
    if bone_paths and len(bone_paths) != len(bindposes):
        raise ValueError("mesh bone path palette does not match its bind poses")
    if len(blend_weights) != len(blend_frames):
        raise ValueError("mesh BlendShape weights do not match its frames")
    if reader.pos != len(reader.data):
        raise ValueError("unexpected trailing EIEM mesh data")
    return locals()


def read_skeleton(path):
    reader = Reader(path.read_bytes())
    if reader.take(8) != MAGIC_SKEL:
        raise ValueError("not an EIEM skeleton")
    version = reader.i32()
    if version not in (1, 2):
        raise ValueError("unsupported EIEM skeleton version")
    coordinate = reader.string()
    count = reader.i32()
    if coordinate != "unity-y-up-left-handed" or not 0 < count <= 16384:
        raise ValueError("invalid skeleton coordinate space or node count")
    nodes = []
    for _ in range(max(0, count)):
        nodes.append((reader.string(), reader.i32(),
                      struct.unpack("<3f", reader.take(12)),
                      struct.unpack("<4f", reader.take(16)),
                      struct.unpack("<3f", reader.take(12))))
    bone_count = reader.i32()
    if not 0 <= bone_count <= 16384:
        raise ValueError("invalid skeleton palette count")
    bones = [reader.i32() for _ in range(max(0, bone_count))]
    root_bone = reader.i32()
    source_nodes = [True] * len(nodes)
    if version == 2:
        if reader.i32() != len(nodes):
            raise ValueError("skeleton provenance count mismatch")
        flags = list(reader.take(len(nodes)))
        if any(flag not in (0, 1) for flag in flags):
            raise ValueError("invalid skeleton provenance")
        source_nodes = [bool(flag) for flag in flags]
    if reader.pos != len(reader.data):
        raise ValueError("unexpected trailing EIEM skeleton data")
    validate_skeleton_nodes(nodes, source_nodes)
    return locals()


def validate_skeleton_nodes(nodes, source_nodes):
    if not 0 < len(nodes) <= 16384 or len(nodes) != len(source_nodes):
        raise ValueError("invalid skeleton node count")
    seen = set()
    for i, (path, parent, position, rotation, scale) in enumerate(nodes):
        if (path in seen or len(path.encode("utf-8")) > 4096 or "\\" in path or "\0" in path or
                any(part in ("", ".", "..") for part in path.split("/")) and path != ""):
            raise ValueError("invalid or duplicate skeleton path: " + path)
        seen.add(path)
        if i == 0:
            if parent != -1 or not source_nodes[i] or "/" in path:
                raise ValueError("skeleton root must reference the source hierarchy")
        else:
            if not 0 <= parent < i or not path or nodes[parent][0] != path.rpartition("/")[0]:
                raise ValueError("skeleton path/parent mismatch: " + path)
            if source_nodes[i] and not source_nodes[parent]:
                raise ValueError("source bone cannot be reparented under a new bone: " + path)
        if (not all(math.isfinite(v) for v in (*position, *rotation, *scale)) or
                abs(sum(v*v for v in rotation)-1) > .001 or min(scale) <= 0):
            raise ValueError("invalid skeleton transform: " + path)


def load_material(root, section, values, textures=None):
    material = bpy.data.materials.get(section) or bpy.data.materials.new(section)
    material.use_nodes = True
    for key in list(material.keys()):
        if key.startswith("eiem_"):
            del material[key]
    # Put the paths users edit most often first in Blender's Custom Properties.
    # The original Texture section identity is retained separately so an
    # untouched material still round-trips to the same package resource.
    keys = list(values)
    ordered_keys = (
        [key for key in keys if key.startswith("texture.")] +
        [key for key in keys if key.startswith(("texture_scale.", "texture_offset."))] +
        [key for key in keys if not key.startswith(("texture.", "texture_scale.",
                                                    "texture_offset."))]
    )
    texture_bindings = {}
    for key in ordered_keys:
        value = values[key]
        if key in ("path", "target.path", "target.asset", "overrides"):
            continue
        if key.startswith("texture.") and textures and value in textures:
            texture_bindings[key] = value
            image_path = bpy.path.abspath(textures[value].filepath,
                                          library=textures[value].library)
            value = str(Path(image_path).resolve())
        material["eiem_" + key] = value
    material["eiem_section"] = section
    if values.get("source"):
        material["eiem_source"] = values["source"]
    material["eiem_target_path"] = values.get("target.path", "")
    material["eiem_target_asset"] = values.get("target.asset", "")
    material["eiem_texture_sections_json"] = json.dumps(
        texture_bindings, separators=(",", ":"))
    # This authoring baseline is not a runtime setting. It lets a later export
    # write only properties changed in Blender while the Material payload still
    # clones the original game material for every property left untouched.
    baseline = {
        key: str(value) for key, value in values.items()
        if key not in ("path", "target.path", "target.asset", "overrides")
    }
    if str(values.get("overrides", "false")).lower() == "true":
        baseline = {"source": values.get("source", "")}  # file is already an authored delta, not a game snapshot
    material["eiem_baseline_json"] = json.dumps(
        baseline, sort_keys=True, separators=(",", ":"))
    return material


def import_material_file(path, obj):
    """Import an EIEM material + package texture declarations into one slot."""
    path = Path(path).resolve()
    if obj is None or obj.type != "MESH":
        raise ValueError("请先选择要指定材质的网格")
    values = read_flat_properties(path)
    if values.get("format") != "EIEMMAT" or values.get("version") != "1" or not values.get("source", "").strip():
        raise ValueError("请选择 EIEM 导出的 .mat（EIEMMAT version=1，包含 source），不是 Unity YAML 或节点材质")
    # Only use an enclosing package that actually declares this file.
    resources, root = {}, path.parent
    for parent in path.parents:
        if not (parent / "mod.ini").is_file():
            continue
        sections = parse_material_resources(parent)
        if any(s.lower().startswith("material") and v.get("path") and
               safe_path(parent, v["path"]) == path for s, v in sections.items()):
            resources, root = sections, parent
            break
    textures = {}
    for key, value in values.items():
        if not key.startswith("texture.") or value.replace("\\", "/").lower().startswith("assets/"):
            continue
        declaration = resources.get(value)
        if not declaration or not value.lower().startswith("texture") or not declaration.get("path"):
            raise ValueError("缺少贴图声明 %s：请保留材质所属资源包的 mod.ini 和 Texture 文件" % value)
        disk = safe_path(root, declaration["path"])
        if not disk.is_file():
            raise ValueError("贴图文件不存在：" + str(disk))
        textures[value] = (disk, declaration)
    # Allocation happens only after reference validation. Failure leaves the
    # selected slot and every old Material/Image data block unchanged.
    created_images, material = [], None
    try:
        images = {}
        used = {str(i.get("eiem_section")): i for i in bpy.data.images if i.get("eiem_section")}
        texture_remap = {}
        for old_section, (disk, declaration) in textures.items():
            image = bpy.data.images.load(str(disk), check_existing=False)
            created_images.append(image)
            if image.size[0] <= 0 or image.size[1] <= 0:
                raise ValueError("无法解码贴图：" + str(disk))
            section = unique_texture_section(disk, used)
            image["eiem_section"] = section
            used[section] = image
            texture_remap[old_section] = section
            image["eiem_source"] = declaration.get("source", "")
            image["eiem_name"] = declaration.get("name", disk.stem)
            for key, default in (("linear", "false"), ("mipmaps", "true"), ("filter", "1"),
                                 ("wrap", "0"), ("aniso", "1"), ("mip_bias", "0")):
                image["eiem_" + key] = declaration.get(key, default)
            image.colorspace_settings.name = "Non-Color" if image["eiem_linear"].lower() == "true" else "sRGB"
            images[section] = image
        values = dict(values)
        for key, value in list(values.items()):
            if key.startswith("texture.") and value in texture_remap:
                values[key] = texture_remap[value]
        used_sections = {str(m.get("eiem_section", m.name)).lower() for m in bpy.data.materials}
        used_sections.update(m.name.lower() for m in bpy.data.materials)
        section = unique_export_section("Material" + path.stem, used_sections, "Material")
        material = load_material(root, section, values, images)
        material["eiem_material_file"] = str(path)
        if obj.data.materials:
            obj.active_material = material
        else:
            obj.data.materials.append(material)
        return material
    except Exception:
        if material is not None:
            bpy.data.materials.remove(material)
        for image in created_images:
            bpy.data.images.remove(image)
        raise


def set_point_attribute(mesh, name, data_type, values, member):
    """Store one source value per Unity vertex in an editable Mesh attribute."""
    existing = mesh.attributes.get(name)
    if existing:
        mesh.attributes.remove(existing)
    attribute = mesh.attributes.new(name=name, type=data_type, domain="POINT")
    for item, value in zip(attribute.data, values):
        setattr(item, member, value)
    return attribute


def get_point_attribute(mesh, name, member):
    attribute = mesh.attributes.get(name)
    if not attribute or attribute.domain != "POINT" or len(attribute.data) != len(mesh.vertices):
        return None
    return [tuple(getattr(item, member)) if member != "value" else float(item.value)
            for item in attribute.data]


def normal_state_crc(mesh):
    """Fingerprint Blender's currently evaluated per-corner normals.

    Blender stores custom normals in an encoded split-normal space, so reading
    them back can introduce a small representation error. The fingerprint lets
    an untouched import export the exact source values while still making a
    real Blender normal edit observable to the writer.
    """
    checksum = 0
    for loop, corner in zip(mesh.loops, mesh.corner_normals):
        checksum = zlib.crc32(
            struct.pack("<I3f", loop.vertex_index, *corner.vector), checksum)
    return "%08x" % (checksum & 0xFFFFFFFF)


def normal_state_matches_source(mesh, source_normals, minimum_alignment=0.9998):
    """Ignore Blender's harmless split-normal re-encoding after save/update.

    A real normal edit still changes direction beyond the same tolerance used
    by the import round-trip check. Zero-filled attributes on joined geometry
    remain a mismatch and therefore cannot resurrect stale source normals.
    """
    if source_normals is None or len(source_normals) != len(mesh.vertices):
        return False
    for loop, corner in zip(mesh.loops, mesh.corner_normals):
        source = Vector(source_normals[loop.vertex_index])
        current = Vector(corner.vector)
        if source.length_squared == 0.0 or current.length_squared == 0.0:
            if source.length_squared != current.length_squared:
                return False
            continue
        if source.normalized().dot(current.normalized()) < minimum_alignment:
            return False
    return True


def unique_shape_key_name(obj, requested):
    requested = requested or "Shape"
    if not obj.data.shape_keys or requested not in obj.data.shape_keys.key_blocks:
        return requested
    index = 2
    while "%s.%03d" % (requested, index) in obj.data.shape_keys.key_blocks:
        index += 1
    return "%s.%03d" % (requested, index)


def import_blend_shapes(obj, payload):
    channels = payload.get("blend_channels") or []
    frames = payload.get("blend_frames") or []
    vertices = payload.get("blend_vertices") or []
    weights = payload.get("blend_weights") or []
    if not channels:
        return

    basis = obj.shape_key_add(name="Basis", from_mix=False)
    metadata = []
    frame_serial = 0
    for channel_name, name_hash, frame_index, frame_count in channels:
        channel_metadata = {"name": channel_name, "hash": int(name_hash), "frames": []}
        for offset in range(frame_count):
            source_frame_index = frame_index + offset
            if source_frame_index >= len(frames):
                raise ValueError("blend-shape channel references a missing frame")
            frame_name, first_vertex, vertex_count, has_normals, has_tangents, has_additional = frames[source_frame_index]
            weight = float(weights[source_frame_index]) if source_frame_index < len(weights) else 100.0
            requested = channel_name if frame_count == 1 else "%s@%g" % (channel_name, weight)
            key_name = unique_shape_key_name(obj, requested)
            key = obj.shape_key_add(name=key_name, from_mix=False)
            normal_values = [(0.0, 0.0, 0.0)] * len(obj.data.vertices)
            tangent_values = [(0.0, 0.0, 0.0)] * len(obj.data.vertices)
            end = min(len(vertices), first_vertex + vertex_count)
            for vertex_index, delta_position, delta_normal, delta_tangent in vertices[first_vertex:end]:
                if vertex_index >= len(obj.data.vertices):
                    raise ValueError("blend-shape vertex index is outside the mesh")
                converted = unity_to_blender(delta_position) if is_unity_left_handed(payload["coordinate"]) else delta_position
                key.data[vertex_index].co = basis.data[vertex_index].co + Vector(converted)
                if has_normals:
                    normal_values[vertex_index] = (unity_to_blender(delta_normal)
                                                   if is_unity_left_handed(payload["coordinate"])
                                                   else delta_normal)
                if has_tangents:
                    tangent_values[vertex_index] = (unity_to_blender(delta_tangent)
                                                    if is_unity_left_handed(payload["coordinate"])
                                                    else delta_tangent)
            normal_attribute = ""
            tangent_attribute = ""
            if has_normals:
                normal_attribute = "EIEM_MorphNormal_%03d" % frame_serial
                set_point_attribute(obj.data, normal_attribute, "FLOAT_VECTOR", normal_values, "vector")
            if has_tangents:
                tangent_attribute = "EIEM_MorphTangent_%03d" % frame_serial
                set_point_attribute(obj.data, tangent_attribute, "FLOAT_VECTOR", tangent_values, "vector")
            channel_metadata["frames"].append({
                "key": key_name,
                "weight": weight,
                "source_name": frame_name,
                "normal_attribute": normal_attribute,
                "tangent_attribute": tangent_attribute,
                "has_additional_normals": bool(has_additional),
            })
            frame_serial += 1
        metadata.append(channel_metadata)
    obj.data["eiem_blend_shapes_json"] = json.dumps(metadata, separators=(",", ":"))
    obj.data["eiem_blend_additional_json"] = json.dumps(
        payload.get("additional") or [], separators=(",", ":"))


def make_mesh(section, payload):
    count = payload["vertex_count"]
    raw_vertices = [(payload["vertices"][i * 3], payload["vertices"][i * 3 + 1],
                     payload["vertices"][i * 3 + 2]) for i in range(count)]
    verts = [unity_to_blender(value) if is_unity_left_handed(payload["coordinate"])
             else value for value in raw_vertices]
    faces = []
    face_submesh = []
    reverse_winding = is_unity_left_handed(payload["coordinate"])
    for submesh_index, (_, start, length, *_rest) in enumerate(payload["submeshes"]):
        end = min(len(payload["indices"]), start + length)
        for index in range(start, end - 2, 3):
            tri = payload["indices"][index:index + 3]
            if len(tri) == 3 and all(value < count for value in tri):
                if reverse_winding:
                    tri = (tri[0], tri[2], tri[1])
                faces.append(tuple(tri))
                face_submesh.append(submesh_index)
    if not faces:
        for index in range(0, len(payload["indices"]) - 2, 3):
            tri = payload["indices"][index:index + 3]
            if all(value < count for value in tri):
                if reverse_winding:
                    tri = (tri[0], tri[2], tri[1])
                faces.append(tuple(tri)); face_submesh.append(0)
    mesh = bpy.data.meshes.new(section)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    mesh["eiem_section"] = section
    mesh["eiem_source"] = payload["source"]
    mesh["eiem_coordinate_space"] = payload["coordinate"]
    mesh["eiem_asset"] = payload["name"]
    # EIEM meshes store one authored normal per vertex. Put those values into
    # Blender's native custom split-normal channel so the viewport, modifiers
    # and normal-editing tools use the game normals rather than a hidden copy.
    # Smooth faces are required for custom normals to drive shading.
    if len(payload["normals"]) == count * 3:
        normals = [tuple(payload["normals"][index * 3:index * 3 + 3]) for index in range(count)]
        if is_unity_left_handed(payload["coordinate"]):
            normals = [unity_to_blender(value) for value in normals]
        for polygon in mesh.polygons:
            polygon.use_smooth = True
        mesh.normals_split_custom_set([normals[loop.vertex_index] for loop in mesh.loops])
        mesh.update()
        # Lossless round-trip backup only. Viewport shading and normal-editing
        # tools use the native custom split normals set above.
        set_point_attribute(mesh, "EIEM_SourceNormal", "FLOAT_VECTOR", normals, "vector")
    if len(payload["tangents"]) == count * 4:
        tangents = []
        signs = []
        for index in range(count):
            tangent = tuple(payload["tangents"][index * 4:index * 4 + 3])
            if is_unity_left_handed(payload["coordinate"]):
                tangent = unity_to_blender(tangent)
            tangents.append(tangent)
            signs.append(float(payload["tangents"][index * 4 + 3]))
        set_point_attribute(mesh, "EIEM_Tangent", "FLOAT_VECTOR", tangents, "vector")
        set_point_attribute(mesh, "EIEM_TangentSign", "FLOAT", signs, "value")

    uv_dimensions = []
    for channel, values in enumerate(payload["uvs"]):
        if not values:
            uv_dimensions.append(0)
            continue
        if not count or len(values) % count:
            raise ValueError("UV%d has an invalid component count" % channel)
        dimension = len(values) // count
        if dimension < 2 or dimension > 4:
            raise ValueError("UV%d uses unsupported dimension %d" % (channel, dimension))
        uv_dimensions.append(dimension)
        layer = mesh.uv_layers.new(name="UV%d" % channel)
        for loop in mesh.loops:
            vertex = loop.vertex_index
            start = vertex * dimension
            layer.data[loop.index].uv = values[start:start + 2]
        if dimension > 2:
            extras = []
            for vertex in range(count):
                start = vertex * dimension
                extras.append((float(values[start + 2]),
                               float(values[start + 3]) if dimension == 4 else 0.0))
            set_point_attribute(mesh, "EIEM_UV%d_ZW" % channel, "FLOAT2", extras, "vector")
    while len(uv_dimensions) < 8:
        uv_dimensions.append(0)
    mesh["eiem_uv_dimensions_json"] = json.dumps(uv_dimensions, separators=(",", ":"))

    colors = payload["colors"]
    if len(colors) >= count * 4 and hasattr(mesh, "color_attributes"):
        values = [tuple(colors[index * 4:index * 4 + 4]) for index in range(count)]
        attr = mesh.color_attributes.new(name="Color", type="FLOAT_COLOR", domain="POINT")
        for item, value in zip(attr.data, values):
            item.color = value
    for polygon, submesh in zip(mesh.polygons, face_submesh):
        polygon.material_index = submesh
    if "EIEM_SourceNormal" in mesh.attributes:
        mesh["eiem_normal_baseline_crc"] = normal_state_crc(mesh)
    return mesh


def skeleton_hierarchy_key(payload):
    return tuple(
        (path, parent, tuple(position), tuple(rotation), tuple(scale), bool(source))
        for (path, parent, position, rotation, scale), source in zip(
            payload["nodes"], payload.get("source_nodes", [True] * len(payload["nodes"])))
    )


def armature_bone_hash_lookup(armature):
    """Map Unity CRC32 path hashes to the shared armature's node indices."""
    lookup = {}
    for index, bone in enumerate(armature.data.bones):
        path = str(bone.get("eiem_path", bone.name))
        parts = path.split("/")
        for start in range(len(parts)):
            suffix = "/".join(parts[start:])
            digest = zlib.crc32(suffix.encode("utf-8")) & 0xffffffff
            candidates = lookup.setdefault(digest, [])
            if index not in candidates:
                candidates.append(index)
    return lookup


def resolve_mesh_bone_palette(payload, armature, legacy_palette=None):
    hashes = payload.get("bone_hashes") or []
    bindposes = payload.get("bindposes") or []
    if not payload.get("skin"):
        return []
    if not hashes or len(hashes) != len(bindposes):
        raise ValueError(
            "%s has skin data but no complete bone hash palette" % payload.get("name", "Mesh"))
    paths = payload.get("bone_paths") or []
    if paths:
        if len(paths) != len(bindposes):
            raise ValueError("%s has an incomplete bone path palette" % payload.get("name", "Mesh"))
        by_path = {str(bone.get("eiem_path", bone.name)): index
                   for index, bone in enumerate(armature.data.bones)}
        missing = [path for path in paths if path not in by_path]
        if missing:
            raise ValueError("%s bone path is absent from the shared skeleton: %s" %
                             (payload.get("name", "Mesh"), missing[0]))
        return [by_path[path] for path in paths]
    if legacy_palette is not None:
        palette = [int(value) for value in legacy_palette]
        if len(palette) != len(bindposes) or any(
                value < 0 or value >= len(armature.data.bones) for value in palette):
            raise ValueError("%s has an invalid renderer bone palette" % payload.get("name", "Mesh"))
        return palette
    lookup = armature_bone_hash_lookup(armature)
    palette = []
    for value in hashes:
        candidates = lookup.get(int(value), [])
        if not candidates:
            raise ValueError(
                "%s bone hash %u is absent from the shared skeleton" %
                (payload.get("name", "Mesh"), int(value)))
        if len(candidates) != 1:
            raise ValueError(
                "%s bone hash %u is ambiguous in the shared skeleton" %
                (payload.get("name", "Mesh"), int(value)))
        palette.append(candidates[0])
    return palette


def bind_mesh_weights(obj, payload, armature, palette):
    """Create Blender vertex groups from the EIEM bone-index palette."""
    if not obj or not armature or not payload.get("skin"):
        return
    bones = list(armature.data.bones)
    # BoneWeight indices address the Mesh's compact bone palette, not the
    # complete prefab hierarchy. The palette is resolved from Mesh bone hashes
    # and belongs to the Mesh/armature binding, never to the Armature resource.
    groups = {}
    for palette_index, node_index in enumerate(palette):
        if 0 <= node_index < len(bones):
            bone = bones[node_index]
            groups[palette_index] = obj.vertex_groups.get(bone.name) or obj.vertex_groups.new(name=bone.name)
    for vertex_index, (weights, indices) in enumerate(payload["skin"]):
        if vertex_index >= len(obj.data.vertices):
            break
        for weight, bone_index in zip(weights, indices):
            if weight <= 0.0 or bone_index not in groups:
                continue
            groups[bone_index].add([vertex_index], float(weight), "REPLACE")


def make_armature(section, payload, collection):
    armature = bpy.data.armatures.new(section)
    obj = bpy.data.objects.new(section, armature)
    collection.objects.link(obj)
    obj["eiem_section"] = section
    obj["eiem_coordinate_space"] = payload["coordinate"]
    obj["eiem_source"] = ""
    obj["eiem_root_bone_index"] = int(payload.get("root_bone", -1))
    edit = bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bones = []
    source_world_matrices = []
    edit_world_matrices = []
    for index, (path, parent, position, rotation, scale) in enumerate(payload["nodes"]):
        # Skeleton node transforms are local to their parent. Blender edit
        # bones use armature-space coordinates, so compose the hierarchy here
        # instead of placing every child at an unrelated local position.
        quaternion = Quaternion((rotation[3], rotation[0], rotation[1], rotation[2]))
        local = Matrix.LocRotScale(Vector(position), quaternion, Vector(scale))
        if 0 <= parent < len(source_world_matrices):
            local = source_world_matrices[parent] @ local
        source_world_matrices.append(local)
        edit_matrix = (unity_transform_matrix_to_blender(local)
                       if is_unity_left_handed(payload["coordinate"])
                       else local.copy())
        edit_world_matrices.append(edit_matrix)
        bone = armature.edit_bones.new(path.rsplit("/", 1)[-1] or "root")
        bone.head = edit_matrix.translation
        bone.tail = edit_matrix.translation + (edit_matrix.to_3x3() @ Vector((0.0, 0.05, 0.0)))
        if (bone.tail - bone.head).length < 0.0001:
            bone.tail = bone.head + Vector((0.0, 0.05, 0.0))
        bone["eiem_path"] = path
        bone["eiem_skeleton_source"] = bool(payload.get("source_nodes", [True] * len(payload["nodes"]))[index])
        bone["eiem_source_parent"] = payload["nodes"][parent][0] if parent >= 0 else ""
        # Edit bones expose head/tail/roll, which cannot reconstruct the
        # serialized local quaternion and scale. Preserve source TRS so an
        # unchanged round trip does not turn the armature into a T-pose.
        bone["eiem_local_position"] = list(position)
        bone["eiem_local_rotation"] = list(rotation)
        bone["eiem_local_scale"] = list(scale)
        bones.append(bone)
    for index, node in enumerate(payload["nodes"]):
        parent = node[1]
        if 0 <= parent < len(bones):
            bones[index].parent = bones[parent]
    # Apply the composed source rest matrices after parenting so the viewport
    # shows the same bone orientation as the game. The serialized local TRS
    # remains in custom properties for lossless export, including scale that
    # Blender's EditBone display cannot represent directly.
    for index, bone in enumerate(bones):
        bone.matrix = Matrix(edit_world_matrices[index])
    bpy.ops.object.mode_set(mode="OBJECT")
    for bone in armature.bones:
        bone["eiem_rest_display"] = [bone.matrix_local[r][c] for r in range(4) for c in range(4)]
    obj.select_set(False)
    return obj


def clear_collection_objects(collection):
    for obj in list(collection.all_objects):
        bpy.data.objects.remove(obj, do_unlink=True)


def remove_collection_tree(collection):
    """Remove an import subtree after its objects have been unlinked."""
    for child in list(collection.children):
        remove_collection_tree(child)
    bpy.data.collections.remove(collection)


def clear_eiem_imports(collection):
    """Clear imported package trees while retaining the stable EIEM root."""
    clear_collection_objects(collection)
    for child in list(collection.children):
        remove_collection_tree(child)


def import_child_collection(parent, label, role, import_id):
    child = bpy.data.collections.new(parent.name + " " + label)
    child["eiem_collection_role"] = role
    child["eiem_import_id"] = import_id
    parent.children.link(child)
    return child


def package_import_collection(root_collection, root):
    source = str(Path(root).resolve())
    label = Path(root).name
    if label.lower().endswith(".eiem"):
        label = label[:-5]
    import_id = uuid.uuid4().hex
    package = bpy.data.collections.new("EIEM " + (label or "Package"))
    package.color_tag = "COLOR_04"
    package["eiem_collection_role"] = "PACKAGE"
    package["eiem_author_package"] = source
    package["eiem_import_id"] = import_id
    root_collection.children.link(package)
    return package, import_id


def mesh_resource_collection(root_collection, section, payload):
    identity = "%s %s %s" % (section, payload.get("name", ""), payload.get("source", ""))
    match = re.search(r"(?:^|[^a-z0-9])lod([0-9]+)(?:$|[^a-z0-9])", identity, re.IGNORECASE)
    name = "LOD%d" % int(match.group(1)) if match else "Other"
    collection = bpy.data.collections.get("%s %s" % (root_collection.name, name))
    if collection is None:
        collection = bpy.data.collections.new("%s %s" % (root_collection.name, name))
    if collection.name not in root_collection.children:
        root_collection.children.link(collection)
    return collection


def import_package(root, clean=False, include_physics=False, physics_file=""):
    parser = parse_ini(root)
    collection = bpy.data.collections.get("EIEM") or bpy.data.collections.new("EIEM")
    if collection.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(collection)
    if clean:
        clear_eiem_imports(collection)
        for material in [item for item in bpy.data.materials if item.get("eiem_section")]:
            bpy.data.materials.remove(material, do_unlink=True)
        for image in [item for item in bpy.data.images if item.get("eiem_section")]:
            bpy.data.images.remove(image, do_unlink=True)
    package_collection, import_id = package_import_collection(collection, root)
    mesh_collection = import_child_collection(package_collection, "Meshes", "MESHES", import_id)
    skeleton_collection = import_child_collection(package_collection, "Skeletons", "SKELETONS", import_id)
    resources = {section: dict(parser.items(section)) for section in parser.sections()}
    render_owners = {}
    for section, values in resources.items():
        if not section.lower().startswith("prefab"):
            continue
        prefab_path = values.get("path", "").strip()
        if not prefab_path:
            raise ValueError("%s has no game Prefab path" % section)
        for key, render_section in values.items():
            if not key.lower().startswith("render."):
                continue
            if render_section not in resources or not render_section.lower().startswith("render"):
                raise ValueError("%s references missing Render section %s" %
                                 (section, render_section))
            if render_section in render_owners:
                raise ValueError("Render section %s belongs to more than one Prefab" %
                                 render_section)
            render_owners[render_section] = (section, prefab_path)
    textures = {}
    for section, values in resources.items():
        if not section.lower().startswith("texture") or not values.get("path"):
            continue
        try:
            image = bpy.data.images.load(str(safe_path(root, values["path"])), check_existing=True)
            image["eiem_section"] = section
            image["eiem_relative_path"] = values["path"]
            image["eiem_source"] = values.get("source", "")
            image["eiem_name"] = values.get("name", image.name)
            image["eiem_target_path"] = values.get("target.path", "")
            image["eiem_target_asset"] = values.get("target.asset", "")
            image["eiem_linear"] = values.get("linear", "false")
            image["eiem_mipmaps"] = values.get("mipmaps", "true")
            image["eiem_filter"] = values.get("filter", "1")
            image["eiem_wrap"] = values.get("wrap", "0")
            image["eiem_aniso"] = values.get("aniso", "1")
            image["eiem_mip_bias"] = values.get("mip_bias", "0")
            textures[section] = image
        except Exception:
            continue
    materials = {}
    for section, values in resources.items():
        if section.lower().startswith("material") and values.get("path"):
            material_values = read_flat_properties(safe_path(root, values["path"]))
            # The Material file owns shader properties. The resource section
            # owns package location and preserves original-game identity as
            # authoring metadata; live replacement is always a Render action.
            material_values["path"] = values["path"]
            # Preserve offline identity as authoring metadata. A Material
            # declaration never applies itself: exported Render material slots
            # decide which consumers receive the edited clone.
            material_values["target.path"] = values.get(
                "target.path", material_values.get("source", ""))
            material_values["target.asset"] = values.get(
                "target.asset", material_values.get("name", ""))
            materials[section] = load_material(
                root, section, material_values, textures)
    # A package can contain many renderer-specific Skeleton sections exported
    # by an older AnimeStudio.  Those sections repeat the same character
    # hierarchy and differ only in the renderer's compact bone palette.  The
    # hierarchy is the resource; a palette belongs to a Mesh binding.  Create
    # exactly one Armature for each unique hierarchy and make every legacy
    # section name an alias of it.
    skeletons = {}
    skeleton_payloads = {}
    armatures_by_hierarchy = {}
    for section, values in resources.items():
        if section.lower().startswith("skeleton") and values.get("path"):
            payload = read_skeleton(safe_path(root, values["path"]))
            hierarchy = skeleton_hierarchy_key(payload)
            armature = armatures_by_hierarchy.get(hierarchy)
            if armature is None:
                armature = make_armature(section, payload, skeleton_collection)
                armature["eiem_source"] = values.get("source", "")
                armature["eiem_target_path"] = values.get("target.path", "")
                armature["eiem_target_asset"] = values.get("target.asset", "")
                armature["eiem_author_package"] = str(Path(root).resolve())
                armature["eiem_import_id"] = import_id
                armatures_by_hierarchy[hierarchy] = armature
            skeletons[section] = armature
            skeleton_payloads[section] = payload
    meshes = {}
    mesh_payloads = {}
    for section, values in resources.items():
        if not section.lower().startswith("mesh") or not values.get("path"):
            continue
        payload = read_mesh(safe_path(root, values["path"]))
        mesh = make_mesh(section, payload)
        # Preserve offline identity for authoring and for the generated Render
        # selector. Resource declarations do not apply replacements themselves.
        mesh["eiem_target_path"] = values.get(
            "target.path", payload.get("source", ""))
        mesh["eiem_target_asset"] = values.get(
            "target.asset", payload.get("name", ""))
        obj = bpy.data.objects.new(section, mesh)
        obj["eiem_author_package"] = str(Path(root).resolve())
        obj["eiem_import_id"] = import_id
        mesh_resource_collection(mesh_collection, section, payload).objects.link(obj)
        meshes[section] = obj
        mesh_payloads[section] = payload
        # Keep the source palette available for a lossless round trip. Vertex
        # groups remain the editable source of truth after the user changes a
        # mesh; these values preserve bindposes and hashes that Blender does
        # not derive from topology.
        obj["eiem_bindposes_json"] = json.dumps(payload["bindposes"], separators=(",", ":"))
        obj["eiem_bone_hashes_json"] = json.dumps(payload["bone_hashes"], separators=(",", ":"))
        obj.data["eiem_original_vertex_count"] = int(payload["vertex_count"])
        import_blend_shapes(obj, payload)
    for section, values in resources.items():
        if not section.lower().startswith("render") or not values.get("mesh"):
            continue
        obj = meshes.get(values["mesh"])
        if not obj:
            continue
        if section in render_owners:
            prefab_section, prefab_path = render_owners[section]
            obj["eiem_prefab_section"] = prefab_section
            obj["eiem_prefab_path"] = prefab_path
        obj["eiem_render_section"] = section
        obj["eiem_render_path"] = values.get("path", "")
        obj["eiem_render_asset"] = values.get("asset", "")
        original_materials = {}
        if values.get("skeleton") in skeletons:
            skeleton = skeletons[values["skeleton"]]
            obj["eiem_skeleton"] = skeleton["eiem_section"]
            skeleton_payload = skeleton_payloads[values["skeleton"]]
            palette = resolve_mesh_bone_palette(
                mesh_payloads.get(values["mesh"], {}), skeleton,
                skeleton_payload.get("bones") or None)
            obj["eiem_bone_palette_json"] = json.dumps(palette, separators=(",", ":"))
            paths = [str(skeleton.data.bones[index].get("eiem_path", skeleton.data.bones[index].name))
                     for index in palette]
            obj["eiem_bone_paths_json"] = json.dumps(paths, separators=(",", ":"))
            modifier = obj.modifiers.get("EIEM Armature") or obj.modifiers.new("EIEM Armature", "ARMATURE")
            modifier.object = skeleton
            bind_mesh_weights(obj, mesh_payloads.get(values["mesh"], {}), skeleton, palette)
        for key, material_section in values.items():
            if key.startswith("material.") and material_section in materials:
                try:
                    slot = int(key.split(".", 1)[1])
                    while len(obj.data.materials) <= slot:
                        obj.data.materials.append(None)
                    obj.data.materials[slot] = materials[material_section]
                    original_materials[str(slot)] = material_section
                except ValueError:
                    pass
        obj["eiem_original_material_sections_json"] = json.dumps(
            original_materials, sort_keys=True, separators=(",", ":"))
    for armature in armatures_by_hierarchy.values():
        armature.data["eiem_source_bindings_json"] = json.dumps(shared_skin_bindings(armature), separators=(",", ":"))
    if include_physics:
        physics_resources = [
            (section, safe_path(root, values["path"]))
            for section, values in resources.items()
            if section.lower().startswith("physics") and values.get("path")
        ]
        if physics_file:
            physics_resources = [("", Path(physics_file))]
        if not physics_resources:
            candidate = Path(root) / "physics" / "components.json"
            if candidate.is_file():
                physics_resources = [("", candidate)]
        if not physics_resources:
            raise ValueError("资源包未包含物理文件，请指定解包的 components.json 或 .physics")
        physics_rigs = {}
        for section, values in resources.items():
            if not section.lower().startswith("render"):
                continue
            physics_section = values.get("physics", "")
            skeleton = skeletons.get(values.get("skeleton", ""))
            if not physics_section or skeleton is None:
                continue
            previous = physics_rigs.get(physics_section)
            if previous is not None and previous is not skeleton:
                raise ValueError("Physics %s 被多个不同 Skeleton 引用" % physics_section)
            physics_rigs[physics_section] = skeleton
        rigs = list(armatures_by_hierarchy.values())
        for section, file in physics_resources:
            rig = physics_rigs.get(section)
            if rig is None:
                if len(rigs) != 1:
                    raise ValueError(
                        "无法从 Render.physics 确定 %s 使用的 Skeleton" %
                        (section or Path(file).name))
                rig = rigs[0]
            if file.suffix.lower() == ".physics":
                physics_authoring.import_physics(file, rig)
            else:
                physics_authoring.native.import_source(file, rig)
    return len(meshes)


def export_corner_map(mesh, channels):
    """Split only serialized vertices, using all CORNER streams together.

    Original vertex IDs (including loose vertices) remain in place. Additional
    corner variants are appended; every point stream uses source_vertices,
    while triangle indices and corner streams use the corresponding loop map.
    No Blender data is changed. Float32 keys avoid merging distinct authored
    values or inventing a tolerance-dependent UV/normal average.
    """
    source_vertices = list(range(len(mesh.vertices)))
    source_loops = [None] * len(mesh.vertices)
    loop_vertices = [0] * len(mesh.loops)
    seen = {}
    width = sum(len(values[0]) for values in channels if values)
    key_format = "<%df" % width
    for loop in mesh.loops:
        signature = struct.pack(key_format, *(
            component for values in channels for component in values[loop.index]))
        key = (loop.vertex_index, signature)
        target = seen.get(key)
        if target is None:
            target = loop.vertex_index
            if source_loops[target] is not None:
                target = len(source_vertices)
                source_vertices.append(loop.vertex_index)
                source_loops.append(loop.index)
            else:
                source_loops[target] = loop.index
            seen[key] = target
        loop_vertices[loop.index] = target
    return source_vertices, source_loops, loop_vertices


def mesh_export_uv_channels(mesh):
    """Read named, possibly sparse UV channels without collapsing corners."""
    dimensions = parse_json_property(mesh, "eiem_uv_dimensions_json", [])
    output = []
    for channel in range(8):
        layer = mesh.uv_layers.get("UV%d" % channel)
        if layer is None:
            output.append(None)
            continue
        dimension = int(dimensions[channel]) if channel < len(dimensions) and dimensions[channel] else 2
        if dimension < 2 or dimension > 4:
            raise ValueError("UV%d uses unsupported dimension %d" % (channel, dimension))
        xy = [tuple(item.uv) for item in layer.data]
        zw = None
        if dimension > 2:
            zw = get_point_attribute(mesh, "EIEM_UV%d_ZW" % channel, "vector")
            if zw is None:
                raise ValueError("UV%d is %dD but its EIEM_UV%d_ZW attribute is missing" %
                                 (channel, dimension, channel))
        output.append((dimension, xy, zw))
    return output


def mesh_export_colors(mesh):
    if not hasattr(mesh, "color_attributes"):
        return None, []
    attribute = mesh.color_attributes.get("Color")
    if attribute is None:
        return None, []
    values = [tuple(item.color) for item in attribute.data]
    if attribute.domain not in {"POINT", "CORNER"}:
        raise ValueError("Color must use the POINT or CORNER domain")
    return attribute.domain, values


def mesh_export_tangents(mesh, source_normals, normal_corners, preserve_normals):
    """Retain usable authored frames; generate only missing frames from UV0.

    Authored point signs are stored in the source coordinate convention by
    make_mesh, whereas newly computed signs start in Blender's convention.
    Corner frames participate in serialization splitting (mirrored UVs can
    disagree even at corners with identical position, normal and UV).
    """
    tangent_attr = mesh.attributes.get("EIEM_Tangent")
    sign_attr = mesh.attributes.get("EIEM_TangentSign")
    points = []
    if tangent_attr is not None or sign_attr is not None:
        if tangent_attr is None or sign_attr is None:
            raise ValueError("EIEM_Tangent and EIEM_TangentSign must both be present")
        if (tangent_attr.domain != "POINT" or tangent_attr.data_type != "FLOAT_VECTOR"
                or sign_attr.domain != "POINT" or sign_attr.data_type != "FLOAT"):
            raise ValueError("EIEM_Tangent/TangentSign must be POINT vector/float attributes")
        points = [(*item.vector, sign.value)
                  for item, sign in zip(tangent_attr.data, sign_attr.data)]

    def usable(value):
        # Do not normalize/orthogonalize valid custom source frames. In
        # particular, zero-filled attributes on joined geometry are missing
        # data, not valid tangents merely because the attribute exists.
        return (all(math.isfinite(x) for x in value)
                and any(x != 0 for x in value[:3]) and value[3] in (-1, 1))

    def export_normal(loop):
        value = (source_normals[loop.vertex_index] if preserve_normals
                 else normal_corners[loop.index])
        return Vector(value)

    def matches_normal(value, loop):
        if not usable(value):
            return False
        normal = export_normal(loop)
        tangent = Vector(value[:3])
        if normal.length_squared == 0.0 or tangent.length_squared == 0.0:
            return False
        # A tangent frame is meaningful only relative to the normal that will
        # actually be serialized. Joined meshes can retain a non-zero source
        # tangent while their final custom normal has changed.
        cosine = abs(normal.dot(tangent)) / (normal.length * tangent.length)
        return cosine <= 2.0e-3

    point_valid = [usable(value) for value in points] if points else [False] * len(mesh.vertices)
    missing = [loop.index for loop in mesh.loops
               if not point_valid[loop.vertex_index]
               or not matches_normal(points[loop.vertex_index], loop)]
    if not missing:
        return points, []
    if mesh.uv_layers.get("UV0") is None:
        if points:
            raise ValueError("%s has missing tangent frames but no UV0 to generate them" % mesh.name)
        # UV-less/untextured geometry is legal. Do not fabricate UV0 or a
        # tangent basis unrelated to any texture coordinates.
        return [], []

    # Derived tangent data is never written into the user's mesh attributes.
    # Work on a temporary copy so failed exports also leave author data intact.
    work = mesh.copy()
    try:
        work.calc_tangents(uvmap="UV0")
        handedness = -1 if is_unity_left_handed(
            mesh.get("eiem_coordinate_space", "unity-y-up-left-handed")) else 1
        generated = [(*loop.tangent, loop.bitangent_sign * handedness) for loop in work.loops]
    finally:
        bpy.data.meshes.remove(work)
    for loop_index in missing:
        loop = mesh.loops[loop_index]
        value = generated[loop_index]
        if not usable(value):
            raise ValueError("%s corner %d cannot generate a usable tangent; check UV0 and normals" %
                             (mesh.name, loop_index))
        normal = export_normal(loop)
        tangent = Vector(value[:3])
        # calc_tangents uses Blender's evaluated normals. Orthogonalize against
        # the exact normal selected for export in case a lossless source-normal
        # backup differs slightly from Blender's encoded custom-normal result.
        tangent -= normal * (tangent.dot(normal) / normal.length_squared)
        if tangent.length_squared == 0.0:
            raise ValueError("%s corner %d tangent is parallel to its exported normal" %
                             (mesh.name, loop_index))
        tangent.normalize()
        generated[loop_index] = (*tangent, value[3])
    # Unreferenced loose vertices have no face/UV derivative. Keep valid source
    # data there, or a zero sentinel rather than inventing a direction.
    points = [points[i] if point_valid[i] else (0., 0., 0., 0.) for i in range(len(mesh.vertices))]
    missing_set = set(missing)
    corners = [generated[loop.index] if loop.index in missing_set else points[loop.vertex_index]
               for loop in mesh.loops]
    print("[EIEM] %s: tangents retained on %d corners; generated %d corners from UV0" %
          (mesh.name, len(mesh.loops) - len(missing), len(missing)))
    return points, corners


def export_blend_shapes(obj, to_source, source_vertices):
    mesh = obj.data
    shape_keys = mesh.shape_keys
    if not shape_keys:
        return [], [], [], [], []
    basis = shape_keys.reference_key
    if not shape_keys.use_relative:
        raise ValueError("EIEM 暂不支持绝对形态键，请使用相对 Basis 的形态键")
    for key in shape_keys.key_blocks:
        if key != basis and (key.relative_key != basis or key.vertex_group):
            raise ValueError("形态键 %s 必须相对 Basis，且不能使用顶点组遮罩" % key.name)
    metadata = parse_json_property(mesh, "eiem_blend_shapes_json", [])
    used = set()
    vertices = []
    frames = []
    channels = []
    weights = []

    def append_frame(key, frame_metadata):
        first = len(vertices)
        normal_attribute = str(frame_metadata.get("normal_attribute", ""))
        tangent_attribute = str(frame_metadata.get("tangent_attribute", ""))
        normal_values = get_point_attribute(mesh, normal_attribute, "vector") if normal_attribute else None
        tangent_values = get_point_attribute(mesh, tangent_attribute, "vector") if tangent_attribute else None
        for index, source in enumerate(source_vertices):
            basis_point, shape_point = basis.data[source], key.data[source]
            delta_position = shape_point.co - basis_point.co
            delta_normal = Vector(normal_values[source]) if normal_values else Vector((0.0, 0.0, 0.0))
            delta_tangent = Vector(tangent_values[source]) if tangent_values else Vector((0.0, 0.0, 0.0))
            if (delta_position.length_squared == 0.0 and
                    delta_normal.length_squared == 0.0 and
                    delta_tangent.length_squared == 0.0):
                continue
            vertices.append((index, to_source(delta_position), to_source(delta_normal),
                             to_source(delta_tangent)))
        frames.append((
            str(frame_metadata.get("source_name", key.name)), first, len(vertices) - first,
            normal_values is not None, tangent_values is not None,
            bool(frame_metadata.get("has_additional_normals", False)),
        ))
        weights.append(float(frame_metadata.get("weight", 100.0)))

    for channel_metadata in metadata:
        first_frame = len(frames)
        for frame_metadata in channel_metadata.get("frames", []):
            key_name = str(frame_metadata.get("key", ""))
            key = shape_keys.key_blocks.get(key_name)
            if key is None or key == basis:
                continue
            used.add(key.name)
            append_frame(key, frame_metadata)
        frame_count = len(frames) - first_frame
        if frame_count:
            name = str(channel_metadata.get("name", "Shape"))
            channels.append((name, int(channel_metadata.get(
                "hash", zlib.crc32(name.encode("utf-8")) & 0xffffffff)),
                first_frame, frame_count))

    # Shape Keys created by the user are valid new channels even though they
    # have no imported metadata.
    for key in shape_keys.key_blocks:
        if key == basis or key.name in used:
            continue
        first_frame = len(frames)
        append_frame(key, {"source_name": key.name, "weight": 100.0})
        channels.append((key.name, zlib.crc32(key.name.encode("utf-8")) & 0xffffffff,
                         first_frame, 1))

    additional = parse_json_property(mesh, "eiem_blend_additional_json", [])
    original_count = int(mesh.get("eiem_original_vertex_count", len(mesh.vertices)))
    if additional and (original_count != len(mesh.vertices)
                       or len(source_vertices) != len(mesh.vertices)):
        # This game-specific auxiliary stream has no verified index mapping.
        # Standard shape position/normal/tangent deltas above are remapped;
        # do not guess a layout for opaque additional-normal data.
        raise ValueError("blend-shape additional normals have no verified mapping for topology changes or seam splitting")
    converted_additional = [to_source(value) for value in additional]
    return vertices, frames, channels, weights, converted_additional


def source_skin_binding(obj, armature):
    """Original binding metadata, keyed by path rather than mutable bone order."""
    poses = parse_json_property(obj, "eiem_bindposes_json", [])
    paths = parse_json_property(obj, "eiem_bone_paths_json", [])
    if not paths:
        bones = list(armature.data.bones)
        palette = parse_json_property(obj, "eiem_bone_palette_json", [])
        if any(not 0 <= int(i) < len(bones) for i in palette):
            raise ValueError("%s 的原骨骼映射引用了不存在的骨骼" % obj.name)
        paths = [str(bones[int(i)].get("eiem_path", bones[int(i)].name)) for i in palette]
    if len(paths) != len(poses) or len(set(paths)) != len(paths):
        raise ValueError("%s 的骨骼路径与绑定矩阵不完整或重复" % obj.name)
    if any(len(pose) != 16 or not all(math.isfinite(x) for x in pose) for pose in poses):
        raise ValueError("%s 的骨骼绑定矩阵无效" % obj.name)
    return dict(zip(paths, poses))


def shared_skin_bindings(armature):
    # Imported source records live on the shared skeleton, so deleting a
    # donor object does not delete its binding information. Existing projects
    # contribute the same original metadata from their surviving objects.
    records = parse_json_property(armature.data, "eiem_source_bindings_json", [])
    for other in sorted(bpy.data.objects, key=lambda item: item.name):
        if other.type == "MESH" and other.find_armature() == armature:
            record = source_skin_binding(other, armature)
            if record and record not in records:
                records.append(record)
    return records


def export_skin_binding(obj, armature, source_vertices):
    """Keep original slots; extend with bones from this shared armature."""
    if not armature:
        return ([], parse_json_property(obj, "eiem_bindposes_json", []),
                parse_json_property(obj, "eiem_bone_hashes_json", []),
                parse_json_property(obj, "eiem_bone_paths_json", []))
    original = source_skin_binding(obj, armature)
    hashes = [int(x) for x in parse_json_property(obj, "eiem_bone_hashes_json", [])]
    if not original or len(hashes) != len(original):
        raise ValueError("%s 缺少完整的原骨骼绑定信息" % obj.name)
    author_nodes = skeleton_author_nodes(armature)
    by_path = {record[0]: bone for bone, record, source in author_nodes}
    missing = set(original) - by_path.keys()
    if missing:
        raise ValueError("原骨骼已不在共享骨架中：" + sorted(missing)[0])
    by_name = {bone.name: path for path, bone in by_path.items()}
    # Non-bone groups may be modifier masks, but positive weights on them
    # cannot silently become skin. Make this authoring ambiguity explicit.
    for vertex in obj.data.vertices:
        for group in vertex.groups:
            if group.weight > 0 and obj.vertex_groups[group.group].name not in by_name:
                raise ValueError("%s 顶点 %d 的顶点组 %s 在共享骨架中不存在" %
                                 (obj.name, vertex.index, obj.vertex_groups[group.group].name))
    paths = list(original)
    additions = sorted({by_name[g.name] for g in obj.vertex_groups if g.name in by_name} - original.keys())
    matrices = dict(original)
    if additions:
        def matrix(values):
            return Matrix([values[row::4] for row in range(4)])
        def flat(value):
            return [value[row][column] for column in range(4) for row in range(4)]
        pending = list(shared_skin_bindings(armature))
        while pending:
            progress = False
            for record in list(pending):
                common = sorted(matrices.keys() & record.keys())
                if not common:
                    continue
                anchor = common[0]
                basis = matrix(record[anchor]).inverted() @ matrix(matrices[anchor])
                converted = {p: flat(matrix(v) @ basis) for p, v in record.items()}
                if any(max(abs(a-b) for a,b in zip(converted[p], matrices[p])) > 1e-4 for p in common):
                    raise ValueError("共享骨架的原生绑定姿态不一致，不能合并骨骼：" + anchor)
                for p, v in converted.items():
                    if p not in matrices:
                        matrices[p] = v
                pending.remove(record)
                progress = True
            if not progress:
                break
        # A bone never used by a source mesh has no authored inverse bind
        # matrix. Generate only that new slot from the skeleton's rest TRS,
        # calibrated to an existing bind frame; never overwrite original slots.
        world = {}
        def rest(bone):
            if bone.name not in world:
                record = next(record for b, record, source in author_nodes if b == bone)
                p, q, s = record[2:]
                local = Matrix.LocRotScale(Vector(p), Quaternion((q[3],q[0],q[1],q[2])), Vector(s))
                world[bone.name] = rest(bone.parent) @ local if bone.parent else local
            return world[bone.name]
        for p in additions:
            if p not in matrices:
                bone = by_path[p]
                ancestor = bone.parent
                while ancestor and by_name[ancestor.name] not in matrices:
                    ancestor = ancestor.parent
                anchor = by_name[ancestor.name] if ancestor else next(iter(original))
                matrices[p] = flat(rest(bone).inverted() @ rest(by_path[anchor]) @ matrix(matrices[anchor]))
        paths.extend(additions)
        hashes.extend(zlib.crc32(p.encode("utf-8")) & 0xffffffff for p in additions)
    slot = {by_path[p].name:i for i,p in enumerate(paths)}
    skin_by_vertex = []
    reduced = 0
    used = {loop.vertex_index for loop in obj.data.loops}
    for vertex in obj.data.vertices:
        influences = [(float(g.weight),slot[obj.vertex_groups[g.group].name]) for g in vertex.groups if g.weight > 0]
        if not influences and vertex.index in used:
            raise ValueError("%s 顶点 %d 没有骨骼权重" % (obj.name, vertex.index))
        influences.sort(key=lambda pair:(-pair[0],pair[1]))
        if len(influences) > 4:
            reduced += 1
            influences = influences[:4]
            total = sum(w for w,i in influences)
            influences = [(w/total,i) for w,i in influences]
        # Preserve valid source floats. Normalize only genuinely unnormalised
        # author weights, not float32 rounding in untouched source assets.
        total = sum(w for w,i in influences)
        if total and abs(total-1) > 1e-5:
            influences = [(w/total,i) for w,i in influences]
        influences += [(0.,0)] * (4-len(influences))
        skin_by_vertex.append((tuple(w for w,i in influences),tuple(i for w,i in influences)))
    if reduced:
        print("[EIEM] %s: %d vertices reduced to the four strongest normalized skin influences" % (obj.name,reduced))
    return [skin_by_vertex[i] for i in source_vertices], [matrices[p] for p in paths], hashes, paths


def write_mesh(path, obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    coordinate = mesh.get("eiem_coordinate_space", "unity-y-up-left-handed")
    to_source = blender_to_unity if is_unity_left_handed(coordinate) else (lambda value: tuple(value))
    # EIEM stores one normal per vertex, whereas Blender evaluates custom
    # normals per face corner. Keep the original float32 values for a lossless
    # untouched round trip; after a native Blender normal edit, export the
    # evaluated state instead of silently falling back to that backup.
    source_normals = get_point_attribute(mesh, "EIEM_SourceNormal", "vector")
    baseline_crc = str(mesh.get("eiem_normal_baseline_crc", ""))
    preserve_normals = (source_normals is not None and baseline_crc and
                        (normal_state_crc(mesh) == baseline_crc or
                         normal_state_matches_source(mesh, source_normals)))
    normal_corners = [] if preserve_normals else [tuple(c.vector) for c in mesh.corner_normals]
    uv_channels = mesh_export_uv_channels(mesh)
    color_domain, color_values = mesh_export_colors(mesh)
    tangent_points, tangent_corners = mesh_export_tangents(
        mesh, source_normals, normal_corners, preserve_normals)
    corner_channels = [channel[1] for channel in uv_channels if channel is not None]
    if not preserve_normals:
        corner_channels.append(normal_corners)
    if color_domain == "CORNER":
        corner_channels.append(color_values)
    if tangent_corners:
        corner_channels.append(tangent_corners)
    source_vertices, source_loops, loop_vertices = export_corner_map(mesh, corner_channels)

    vertices = [coord for source in source_vertices for coord in to_source(mesh.vertices[source].co)]
    normal_values = [source_normals[source] if preserve_normals else
                     normal_corners[loop] if loop is not None else tuple(mesh.vertices[source].normal)
                     for source, loop in zip(source_vertices, source_loops)]
    normals = [coord for value in normal_values for coord in to_source(value)]

    tangents = []
    if tangent_points:
        for source, loop in zip(source_vertices, source_loops):
            value = tangent_corners[loop] if tangent_corners and loop is not None else tangent_points[source]
            tangents.extend(to_source(value[:3]))
            tangents.append(value[3])
    colors = []
    if color_values:
        for source, loop in zip(source_vertices, source_loops):
            value = color_values[source] if color_domain == "POINT" else (
                color_values[loop] if loop is not None else (0.0, 0.0, 0.0, 0.0))
            colors.extend(value)
    uv_layers = []
    for channel in uv_channels:
        values = []
        if channel is not None:
            dimension, xy, zw = channel
            for source, loop in zip(source_vertices, source_loops):
                values.extend(xy[loop] if loop is not None else (0.0, 0.0))
                if dimension > 2:
                    values.extend(zw[source][:dimension - 2])
        uv_layers.append(values)

    indices = []
    submesh_indices = {}
    for tri in mesh.loop_triangles:
        submesh = int(mesh.polygons[tri.polygon_index].material_index)
        triangle = tuple(loop_vertices[loop] for loop in tri.loops)
        if is_unity_left_handed(coordinate):
            triangle = (triangle[0], triangle[2], triangle[1])
        submesh_indices.setdefault(submesh, []).extend(triangle)
    submeshes = []
    # A submesh's position is its material slot. Keep empty slots before an
    # occupied slot instead of renumbering faces after the user deletes them.
    for submesh in range(max(submesh_indices, default=-1) + 1):
        values = submesh_indices.get(submesh, [])
        start = len(indices)
        indices.extend(values)
        used_vertices = set(values)
        first_vertex = min(used_vertices) if used_vertices else 0
        vertex_count = max(used_vertices) - first_vertex + 1 if used_vertices else 0
        submeshes.append((0, start, len(values), 0, first_vertex, vertex_count))

    armature = next((modifier.object for modifier in obj.modifiers
                     if modifier.type == "ARMATURE" and modifier.object), None)
    skin, bindposes, bone_hashes, bone_paths = export_skin_binding(obj, armature, source_vertices)
    blend_vertices, blend_frames, blend_channels, blend_weights, additional = export_blend_shapes(obj, to_source, source_vertices)

    writer = Writer(); writer.raw(MAGIC_MESH); writer.i32(3)
    writer.string(coordinate)
    writer.string(obj.data.get("eiem_source", "")); writer.string(obj.data.get("eiem_asset", obj.name))
    writer.i32(len(source_vertices)); writer.floats(vertices); writer.floats(normals)
    writer.floats(tangents); writer.floats(colors)
    for values in uv_layers: writer.floats(values)
    writer.i32(len(indices))
    for value in indices: writer.u32(value)
    writer.i32(len(submeshes))
    for topology, start, count, base, first, vertex_count in submeshes:
        writer.i32(topology); writer.u32(start); writer.u32(count)
        writer.u32(base); writer.u32(first); writer.u32(vertex_count)
    writer.i32(len(skin))
    for weights_value, bones_value in skin:
        for value in weights_value: writer.f32(value)
        for value in bones_value: writer.u32(value)
    writer.i32(len(bindposes))
    for matrix in bindposes:
        values = list(matrix)[:16]
        values += [0.0] * (16 - len(values))
        for value in values: writer.f32(value)
    writer.i32(len(bone_hashes))
    for value in bone_hashes: writer.u32(value)
    writer.i32(len(bone_paths))
    for value in bone_paths: writer.string(value)
    writer.i32(len(blend_vertices))
    for index, position, normal, tangent in blend_vertices:
        writer.u32(index)
        for value in position + normal + tangent: writer.f32(value)
    writer.i32(len(blend_frames))
    for name, first, count, has_normals, has_tangents, has_additional in blend_frames:
        writer.string(name); writer.u32(first); writer.u32(count)
        writer.raw(bytes((int(has_normals), int(has_tangents), int(has_additional))))
    writer.i32(len(blend_channels))
    for name, name_hash, first, count in blend_channels:
        writer.string(name); writer.u32(name_hash); writer.u32(first); writer.u32(count)
    writer.floats(blend_weights)
    writer.i32(len(additional))
    for value in additional:
        for component in value: writer.f32(component)
    path.write_bytes(writer.data)


def write_merged_mesh(output, objects):
    """Write sibling objects as one Mesh with one submesh per material slot.

    The runtime hands a source Renderer exactly one Mesh, so mounting several
    sibling parts natively means one Mesh carrying all of them. Each part's
    material slot becomes its own submesh, which is also what makes a single
    part addressable later: a submesh can be re-materialed without touching the
    others.

    Geometry is concatenated verbatim. Vertices are never welded and never
    reordered, so each part keeps its identity and only gains a constant offset.
    All parts must already agree on their joint palette, because one Mesh has
    exactly one; write_mesh is what guarantees that for siblings exported from
    one armature.
    """
    if not objects:
        raise ValueError("没有可合并的网格")
    path = Path(output)
    coordinate = objects[0].data.get("eiem_coordinate_space", "unity-y-up-left-handed")
    for obj in objects:
        if obj.data.get("eiem_coordinate_space", "unity-y-up-left-handed") != coordinate:
            raise ValueError("合并的网格坐标系不一致：" + obj.name)
    to_source = blender_to_unity if is_unity_left_handed(coordinate) else (lambda value: tuple(value))

    parts = []
    for obj in objects:
        mesh = obj.data
        mesh.calc_loop_triangles()
        source_normals = get_point_attribute(mesh, "EIEM_SourceNormal", "vector")
        baseline_crc = str(mesh.get("eiem_normal_baseline_crc", ""))
        preserve_normals = (source_normals is not None and baseline_crc and
                            (normal_state_crc(mesh) == baseline_crc or
                             normal_state_matches_source(mesh, source_normals)))
        normal_corners = [] if preserve_normals else [tuple(c.vector) for c in mesh.corner_normals]
        uv_channels = mesh_export_uv_channels(mesh)
        color_domain, color_values = mesh_export_colors(mesh)
        tangent_points, tangent_corners = mesh_export_tangents(
            mesh, source_normals, normal_corners, preserve_normals)
        corner_channels = [channel[1] for channel in uv_channels if channel is not None]
        if not preserve_normals:
            corner_channels.append(normal_corners)
        if color_domain == "CORNER":
            corner_channels.append(color_values)
        if tangent_corners:
            corner_channels.append(tangent_corners)
        source_vertices, source_loops, loop_vertices = export_corner_map(mesh, corner_channels)

        vertices = [coord for source in source_vertices
                    for coord in to_source(mesh.vertices[source].co)]
        normal_values = [source_normals[source] if preserve_normals else
                         normal_corners[loop] if loop is not None else tuple(mesh.vertices[source].normal)
                         for source, loop in zip(source_vertices, source_loops)]
        normals = [coord for value in normal_values for coord in to_source(value)]
        tangents = []
        if tangent_points:
            for source, loop in zip(source_vertices, source_loops):
                value = tangent_corners[loop] if tangent_corners and loop is not None else tangent_points[source]
                tangents.extend(to_source(value[:3]))
                tangents.append(value[3])
        colors = []
        if color_values:
            for source, loop in zip(source_vertices, source_loops):
                value = color_values[source] if color_domain == "POINT" else (
                    color_values[loop] if loop is not None else (0.0, 0.0, 0.0, 0.0))
                colors.extend(value)
        uv_layers = []
        for channel in uv_channels:
            values = []
            if channel is not None:
                dimension, xy, zw = channel
                for source, loop in zip(source_vertices, source_loops):
                    values.extend(xy[loop] if loop is not None else (0.0, 0.0))
                    if dimension > 2:
                        values.extend(zw[source][:dimension - 2])
            uv_layers.append(values)

        by_slot = {}
        for tri in mesh.loop_triangles:
            slot = int(mesh.polygons[tri.polygon_index].material_index)
            triangle = tuple(loop_vertices[loop] for loop in tri.loops)
            if is_unity_left_handed(coordinate):
                triangle = (triangle[0], triangle[2], triangle[1])
            by_slot.setdefault(slot, []).extend(triangle)

        skin, bindposes, bone_hashes, bone_paths = export_skin_binding(
            obj, obj.find_armature(), source_vertices)
        parts.append({
            "obj": obj,
            "vertices": vertices, "normals": normals, "tangents": tangents,
            "colors": colors, "uv_layers": uv_layers,
            "count": len(source_vertices),
            "channels": len(uv_layers),
            "by_slot": by_slot,
            "slot_count": max(len(obj.data.materials),
                              (max(by_slot) + 1) if by_slot else 0),
            "skin": skin, "bindposes": bindposes,
            "bone_hashes": bone_hashes, "bone_paths": bone_paths,
        })

    # One Mesh has one joint palette. Sibling parts routinely address different
    # subsets of the shared skeleton, so the palette is their ordered union and
    # each part's joint indices are remapped into it. Refusing instead would
    # reject exactly the sibling groups this exists for.
    palette_paths = []
    for part in sorted(parts, key=lambda item: len(item["bone_paths"]), reverse=True):
        for path in part["bone_paths"]:
            if path not in palette_paths:
                palette_paths.append(path)
    palette_index = {name: index for index, name in enumerate(palette_paths)}
    for part in parts:
        # export_skin_binding keeps these three in one order: a skin joint, a
        # bind pose and a hash are all addressed by the part's own palette slot.
        if not (len(part["bindposes"]) == len(part["bone_paths"]) ==
                len(part["bone_hashes"])):
            raise ValueError(
                "网格 %s 的骨骼路径/绑定矩阵/哈希数量不一致" % part["obj"].name)
        part["remap"] = [palette_index[name] for name in part["bone_paths"]]
    # Bind poses come from the palette order. Parts exported from one skeleton
    # agree to float precision, so the first part that declares a bone wins and
    # a materially different matrix means the parts do not share one skin.
    merged_poses = [None] * len(palette_paths)
    for part in parts:
        # enumerate(remap) yields (this part's own slot, its slot in the union).
        for own_slot, union_slot in enumerate(part["remap"]):
            pose = list(part["bindposes"][own_slot])[:16]
            pose += [0.0] * (16 - len(pose))
            existing = merged_poses[union_slot]
            if existing is None:
                merged_poses[union_slot] = pose
            elif max(abs(a - b) for a, b in zip(existing, pose)) > 1e-4:
                raise ValueError(
                    "骨骼 %s 的绑定矩阵在合并的部件之间不一致"
                    % part["bone_paths"][own_slot])
    if any(pose is None for pose in merged_poses):
        raise ValueError("合并的关节调色盘有不存在的绑定矩阵")
    hashes = [None] * len(palette_paths)
    for part in parts:
        for own_slot, union_slot in enumerate(part["remap"]):
            if hashes[union_slot] is None:
                hashes[union_slot] = part["bone_hashes"][own_slot]
    if any(value is None for value in hashes):
        raise ValueError("合并的关节调色盘有不存在的骨骼哈希")
    channels = {part["channels"] for part in parts}
    if len(channels) > 1:
        raise ValueError(
            "合并的网格 UV 通道数不一致，无法共用一个顶点布局："
            + "，".join(part["obj"].name for part in parts))
    # BoneWeight is per vertex, so every part must contribute one entry per
    # vertex. A part with no armature has no skin, and letting that shorten the
    # table would shift every later part's weights onto the wrong vertices.
    for part in parts:
        if len(part["skin"]) != part["count"]:
            raise ValueError(
                "网格 %s 的蒙皮数量(%d)与顶点数(%d)不一致，无法参与合并"
                % (part["obj"].name, len(part["skin"]), part["count"]))

    vertices, normals, tangents, colors = [], [], [], []
    uv_layers = [[] for _ in range(parts[0]["channels"])]
    indices, submeshes, skin = [], [], []
    vertex_offset = 0
    for part in parts:
        vertices.extend(part["vertices"])
        normals.extend(part["normals"])
        tangents.extend(part["tangents"])
        colors.extend(part["colors"])
        for channel, values in enumerate(part["uv_layers"]):
            uv_layers[channel].extend(values)
        # One submesh per material slot, in slot order, so submesh N is the Nth
        # slot of this part. Empty slots are kept rather than renumbered.
        for slot in range(part["slot_count"]):
            slot_indices = part["by_slot"].get(slot, [])
            start = len(indices)
            indices.extend(value + vertex_offset for value in slot_indices)
            used = set(slot_indices)
            first_vertex = min(used) + vertex_offset if used else 0
            vertex_count = (max(used) - min(used) + 1) if used else 0
            submeshes.append((0, start, len(slot_indices), 0, first_vertex,
                              vertex_count))
        for weights_value, bones_value in part["skin"]:
            skin.append((weights_value,
                         [part["remap"][int(b)] for b in bones_value]))
        vertex_offset += part["count"]

    writer = Writer(); writer.raw(MAGIC_MESH); writer.i32(3)
    writer.string(coordinate)
    writer.string(parts[0]["obj"].data.get("eiem_source", ""))
    writer.string(parts[0]["obj"].data.get("eiem_asset", parts[0]["obj"].name))
    writer.i32(vertex_offset); writer.floats(vertices); writer.floats(normals)
    writer.floats(tangents); writer.floats(colors)
    for values in uv_layers: writer.floats(values)
    writer.i32(len(indices))
    for value in indices: writer.u32(value)
    writer.i32(len(submeshes))
    for topology, start, count, base, first, vertex_count in submeshes:
        writer.i32(topology); writer.u32(start); writer.u32(count)
        writer.u32(base); writer.u32(first); writer.u32(vertex_count)
    writer.i32(len(skin))
    for weights_value, bones_value in skin:
        for value in weights_value: writer.f32(value)
        for value in bones_value: writer.u32(value)
    writer.i32(len(merged_poses))
    for pose in merged_poses:
        for value in pose: writer.f32(value)
    writer.i32(len(hashes))
    for value in hashes: writer.u32(value)
    writer.i32(len(palette_paths))
    for value in palette_paths: writer.string(value)
    # Blend shapes keep their part-local vertex indices shifted by that part's
    # base offset, exactly like the geometry they displace.
    writer.i32(0); writer.i32(0); writer.i32(0); writer.floats([]); writer.i32(0)
    # Write through the parameter: the palette loop above rebinds the local.
    Path(output).write_bytes(writer.data)
    return {
        "parts": len(parts),
        "vertices": vertex_offset,
        "submeshes": len(submeshes),
        "slots": [part["slot_count"] for part in parts],
    }


def skeleton_author_nodes(obj):
    """Source nodes are references, new nodes carry actual parent-local TRS.

    This is shared by Mesh bind-pose generation and Skeleton serialization.
    A source bone edit is rejected, never silently replaced with stale metadata.
    """
    records, paths, indices, source_world = [], {}, {}, {}
    visiting = set()
    def visit(bone):
        if bone.name in indices:
            return
        if bone.name in visiting:
            raise ValueError("骨架存在循环父级：" + bone.name)
        visiting.add(bone.name)
        if bone.parent:
            visit(bone.parent)
        source = bool(bone.get("eiem_skeleton_source", "eiem_path" in bone))
        parent_path = paths[bone.parent.name] if bone.parent else ""
        path = str(bone.get("eiem_path", bone.name)) if source else (
            parent_path + "/" + bone.name if parent_path else bone.name)
        if path in paths.values():
            raise ValueError("共享骨架中存在重复路径：" + path + "；复制的骨骼仍带有源身份，请使用新建骨骼")
        if source:
            p, q, s = (bone.get("eiem_local_" + key) for key in ("position", "rotation", "scale"))
            if p is None or q is None or s is None:
                raise ValueError("源骨骼缺少局部绑定数据：" + bone.name)
            expected_parent = str(bone.get("eiem_source_parent", path.rsplit("/", 1)[0] if "/" in path else ""))
            if parent_path != expected_parent:
                raise ValueError("暂不支持改变源骨骼父级：" + bone.name)
            local = Matrix.LocRotScale(Vector(p), Quaternion((q[3],q[0],q[1],q[2])), Vector(s))
            source_world[bone.name] = source_world[bone.parent.name] @ local if bone.parent else local
            baseline = bone.get("eiem_rest_display")
            expected = (Matrix([baseline[i:i+4] for i in range(0,16,4)]) if baseline else
                        unity_transform_matrix_to_blender(source_world[bone.name]))
            if not baseline:
                # Older author files have original TRS but no display snapshot.
                # Blender bones strip axis lengths; compare their normalized basis.
                expected = Matrix.LocRotScale(expected.translation, expected.to_quaternion(), Vector((1,1,1)))
            if max(abs(expected[r][c]-bone.matrix_local[r][c]) for r in range(4) for c in range(4)) > 1e-4:
                raise ValueError("源骨骼绑定姿态已改变，当前仅支持增加骨骼：" + bone.name)
        else:
            if not bone.parent:
                raise ValueError("新增骨骼必须挂在已有共享骨架内：" + bone.name)
            if "/" in bone.name or "\\" in bone.name:
                raise ValueError("骨骼名称不能包含路径分隔符：" + bone.name)
            # EditBone discards source axis scale. Use the preserved native
            # parent world, not its display-only Blender matrix, to derive TRS.
            basis = unity_transform_matrix_to_blender_basis()
            authored_world = basis.inverted() @ bone.matrix_local @ basis
            local = source_world[bone.parent.name].inverted() @ authored_world
            position, rotation, scale = local.decompose()
            if min(scale) <= 0 or max(abs(local[r][c]-Matrix.LocRotScale(position,rotation,scale)[r][c])
                                     for r in range(4) for c in range(4)) > 1e-4:
                raise ValueError("新增骨骼含不可表达的缩放或剪切：" + bone.name)
            p, q, s = tuple(position), (rotation.x,rotation.y,rotation.z,rotation.w), tuple(scale)
            source_world[bone.name] = source_world[bone.parent.name] @ local
        indices[bone.name] = len(records)
        paths[bone.name] = path
        records.append((bone, (path, indices.get(bone.parent.name, -1) if bone.parent else -1,
                              tuple(p), tuple(q), tuple(s)), source))
        visiting.remove(bone.name)
    for bone in obj.data.bones:
        visit(bone)
    validate_skeleton_nodes([record for bone, record, source in records],
                            [source for bone, record, source in records])
    return records


def write_skeleton(path, obj):
    records = skeleton_author_nodes(obj)
    writer = Writer(); writer.raw(MAGIC_SKEL); writer.i32(2)
    writer.string(obj.get("eiem_coordinate_space", "unity-y-up-left-handed"))
    writer.i32(len(records))
    for bone, record, source in records:
        bone_path, parent, position, rotation, scale = record
        writer.string(bone_path)
        writer.i32(parent)
        writer.raw(struct.pack("<3f", *(float(value) for value in position)))
        writer.raw(struct.pack("<4f", *(float(value) for value in rotation)))
        writer.raw(struct.pack("<3f", *(float(value) for value in scale)))
    # This file represents one shared skeleton hierarchy.  Renderer-local bone
    # palettes are serialized in their Mesh resources as hashes/bindposes and
    # must never turn into duplicate Armatures.
    writer.i32(0)
    writer.i32(-1)
    writer.i32(len(records))
    writer.raw(bytes(int(source) for bone, record, source in records))
    path.write_bytes(writer.data)


def write_texture(path, image):
    """Write the Blender image used by a Texture resource as a PNG.

    Unedited imported images are copied byte-for-byte. Dirty/generated images
    are encoded from Blender's current pixel buffer, so texture painting and
    image replacement both survive package export.
    """
    path.parent.mkdir(parents=True, exist_ok=True)
    source = ""
    if image.source == "FILE" and image.filepath:
        source = bpy.path.abspath(image.filepath, library=image.library)
    if not image.is_dirty and source and Path(source).is_file():
        if Path(source).resolve() != path.resolve():
            shutil.copy2(source, path)
        return

    old_path = image.filepath_raw
    old_format = image.file_format
    try:
        image.filepath_raw = str(path)
        image.file_format = "PNG"
        image.save()
    finally:
        image.filepath_raw = old_path
        image.file_format = old_format


def image_absolute_path(image):
    if not image or not image.filepath:
        return ""
    return str(Path(bpy.path.abspath(image.filepath, library=image.library)).resolve())


def texture_section_base(path):
    """Return a stable INI section name without doubling a Texture prefix."""
    fragment = re.sub(r"[^A-Za-z0-9_]", "_", Path(path).stem).strip("_") or "External"
    fragment = fragment[:1].upper() + fragment[1:]
    return fragment if fragment.lower().startswith("texture") else "Texture" + fragment


def unique_texture_section(path, images_by_section):
    base = texture_section_base(path)
    used = {str(section).lower() for section in images_by_section}
    candidate = base
    serial = 2
    while candidate.lower() in used:
        candidate = "%s%d" % (base, serial)
        serial += 1
    return candidate


def file_content_digest(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.digest()


def texture_images_equal(first, second):
    """Compare two unedited image resources by their actual source bytes."""
    if first is None or second is None:
        return False
    if first is second:
        return not first.is_dirty
    if first.is_dirty or second.is_dirty:
        return False
    first_path = image_absolute_path(first)
    second_path = image_absolute_path(second)
    if first_path and second_path:
        first_file, second_file = Path(first_path), Path(second_path)
        if first_file.is_file() and second_file.is_file():
            if os.path.normcase(str(first_file)) == os.path.normcase(str(second_file)):
                return True
            if first_file.stat().st_size != second_file.stat().st_size:
                return False
            return file_content_digest(first_file) == file_content_digest(second_file)
    first_source = str(first.get("eiem_source", "") or first.get("eiem_target_path", "")).strip()
    second_source = str(second.get("eiem_source", "") or second.get("eiem_target_path", "")).strip()
    return bool(first_source and first_source.lower() == second_source.lower())


def canonical_texture_section(image, images_by_section):
    """Collapse import-only section suffixes while retaining distinct images."""
    path = image_absolute_path(image)
    identity = path or str(image.get("eiem_name", image.name))
    preferred = texture_section_base(identity)
    owner = images_by_section.get(preferred)
    if owner is None or owner is image:
        images_by_section[preferred] = image
        return preferred
    if texture_images_equal(owner, image):
        return preferred
    current = str(image.get("eiem_section", "")).strip()
    return current or unique_texture_section(identity, images_by_section)


def texture_output_filename(image):
    """Keep the user's source stem; Texture section prefixes are not filenames."""
    path = image_absolute_path(image)
    source_name = Path(path).name if path else str(image.get("eiem_name", image.name))
    stem = Path(source_name).stem
    if not stem:
        raise ValueError("Texture image has no usable filename: " + image.name)
    return stem + ".png"


def texture_property_uses_linear_data(property_name):
    """Return whether a shader property stores vector data rather than colour."""
    name = str(property_name).lower()
    return "bump" in name or "normal" in name


def native_texture_family(section):
    """Ignore package-local numeric IDs on an extracted game Texture section."""
    text = str(section).strip().lower()
    if not text.startswith("texturet_"):
        return ""
    return re.sub(r"_+\d+$", "", text)


def resolve_material_texture(material, property_name, value, images_by_section,
                             original_bindings):
    """Resolve a Blender-facing image path to an exported Texture section."""
    value = str(value).strip()
    if value in images_by_section:
        section = canonical_texture_section(images_by_section[value], images_by_section)
        original_bindings[property_name] = section
        return section

    requested = Path(bpy.path.abspath(os.path.expandvars(value))).resolve()
    if not requested.is_file():
        raise ValueError(
            "%s %s texture path does not exist: %s" %
            (material.name, property_name, requested))

    requested_key = os.path.normcase(str(requested))
    for section, image in list(images_by_section.items()):
        existing = image_absolute_path(image)
        if existing and os.path.normcase(existing) == requested_key:
            section = canonical_texture_section(image, images_by_section)
            original_bindings[property_name] = section
            return section

    image = bpy.data.images.load(str(requested), check_existing=True)
    existing_section = image.get("eiem_section", "")
    if existing_section and existing_section in images_by_section:
        section = canonical_texture_section(image, images_by_section)
        original_bindings[property_name] = section
        return section

    section = unique_texture_section(requested, images_by_section)
    template = images_by_section.get(original_bindings.get(property_name, ""))
    defaults = {
        "eiem_linear": "true" if texture_property_uses_linear_data(property_name) else "false",
        "eiem_mipmaps": "true",
        "eiem_filter": "1", "eiem_wrap": "0", "eiem_aniso": "1",
        "eiem_mip_bias": "0",
    }
    image["eiem_section"] = section
    image["eiem_relative_path"] = "textures/%s.png" % section
    image["eiem_source"] = ""
    image["eiem_name"] = requested.stem
    image["eiem_target_path"] = ""
    image["eiem_target_asset"] = ""
    for key, default in defaults.items():
        # Imported legacy packages did not record Texture2D.linear.  In
        # particular they report a native bump map as false by default, which
        # must not make a newly assigned normal map use sRGB sampling.
        image[key] = (default if key == "eiem_linear" else
                      template.get(key, default) if template else default)
    images_by_section[section] = image
    original_bindings[property_name] = section
    return section


def prepare_export_root(root):
    """Remove only files from an earlier EIEM-generated export."""
    root = Path(root).resolve()
    marker = root / "mod.ini"
    if marker.is_file():
        first_line = marker.read_text(encoding="utf-8-sig", errors="replace").splitlines()[:1]
        if first_line == ["; Generated by EIEM Blender add-on"]:
            for directory in ("meshes", "materials", "textures", "skeletons", "physics"):
                candidate = (root / directory).resolve()
                if candidate.parent == root and candidate.is_dir():
                    shutil.rmtree(candidate)
    root.mkdir(parents=True, exist_ok=True)
    return root


def visible_eiem_resources(context=None):
    """Return only the EIEM meshes visible in the current Blender view.

    Every visible EIEM mesh is part of this temporary validation export,
    including a source mesh that the user edited in place.  Hidden meshes are
    intentionally omitted. Armatures and physics are deliberately excluded;
    this entry point is for the current mesh-only validation export.
    """
    context = context or bpy.context
    return sorted([
        obj for obj in context.scene.objects
        if obj.type == "MESH"
        and obj.data.get("eiem_section")
        and obj.visible_get(view_layer=context.view_layer)
        and not obj.hide_viewport
        and not obj.hide_get(view_layer=context.view_layer)
        and not obj.hide_render
    ], key=lambda obj: (
        str(obj.data.get("eiem_section", "")), obj.name))


def selected_eiem_resources(context=None):
    context = context or bpy.context
    selected_objects = list(context.selected_objects)
    meshes = sorted(
        (obj for obj in selected_objects
         if obj.type == "MESH" and obj.data.get("eiem_section")),
        key=lambda obj: str(obj.data.get("eiem_section", "")),
    )
    armatures = sorted(
        (obj for obj in selected_objects
         if obj.type == "ARMATURE" and obj.get("eiem_section")),
        key=lambda obj: str(obj.get("eiem_section", "")),
    )
    if not meshes:
        raise ValueError("No EIEM mesh objects selected")
    return meshes, armatures


def plan_mesh_only_export(mesh_objects):
    """Build the smallest export graph for an explicit Mesh-only export.

    Mesh-only validation must not inspect switch groups, shape controls, rigs or
    physics. The selected Mesh still needs its source identity so sibling parts
    can be merged onto the source Renderer exactly like a normal export.
    """
    selected = list(mesh_objects)
    if not selected:
        raise ValueError("No EIEM mesh objects selected")
    if any(obj.type != "MESH" or not obj.data.get("eiem_section")
           for obj in selected):
        raise ValueError("Selected objects contain a non-EIEM Mesh")
    sources = {obj: mesh_source_identity(obj) for obj in selected}
    for obj in selected:
        if not sources[obj][2]:
            raise ValueError("Mesh %s is missing its source Mesh name" % obj.name)
    grouped = {}
    selectors = {}
    for obj in sorted(selected, key=lambda item: (sources[item], item.name)):
        identity = sources[obj]
        if identity[2] in selectors and selectors[identity[2]] != identity:
            raise ValueError(
                "Same-name Meshes come from different source assets: "
                + identity[2])
        selectors[identity[2]] = identity
        grouped.setdefault(identity, []).append(obj)
    return {
        "objects": [obj for objects in grouped.values() for obj in objects],
        "sources": list(grouped.values()),
        "groups": [],
        "bindings": {},
        "hidden": {obj for obj in selected if obj.hide_render},
    }


def selected_eiem_physics(context=None):
    context = context or bpy.context
    return sorted(
        (obj for obj in context.selected_objects
         if obj.eiem_physics.kind == "GROUP"),
        key=lambda obj: str(obj.eiem_physics.identity),
    )


def material_override_payload(material, images_by_section, force=False):
    """Return the minimal clone payload and the Texture sections it needs."""
    source = str(material.get("eiem_source", "")).strip()
    if not source:
        raise ValueError("Material %s has no source game material" % material.name)

    has_baseline = "eiem_baseline_json" in material
    baseline = parse_json_property(material, "eiem_baseline_json", {})
    source_snapshot = any(
        key.startswith(("float.", "value4.", "color.", "int.", "keyword."))
        for key in baseline
    )
    texture_bindings = parse_json_property(
        material, "eiem_texture_sections_json", {})
    reserved = {
        "eiem_section", "eiem_source", "eiem_shader", "eiem_format",
        "eiem_version", "eiem_name", "eiem_target_path", "eiem_target_asset",
        "eiem_texture_sections_json", "eiem_baseline_json",
        "eiem_material_file",
    }
    overrides = []
    referenced_images = set()
    for key, raw_value in material.items():
        if not key.startswith("eiem_") or key in reserved:
            continue
        property_name = key[5:]
        value = str(raw_value)
        if property_name.startswith("texture."):
            if value == str(baseline.get(property_name, "")) and value.replace("\\", "/").lower().startswith("assets/"):
                continue  # unchanged game logical texture: inherited from the cloned source
            original = str(baseline.get(property_name, "") if has_baseline else
                           texture_bindings.get(property_name, ""))
            # Capture the baseline owner before canonicalization adds an alias
            # for a legacy section such as TextureTextureBody -> TextureBody.
            # A missing owner means the local file is an authored override,
            # even when its preferred section spelling equals the old string.
            original_image = images_by_section.get(original)
            value = resolve_material_texture(
                material, property_name, value, images_by_section,
                texture_bindings)
            current_image = images_by_section.get(value)
            import_alias = (
                source_snapshot and original.lower().startswith(value.lower()) and
                original[len(value):].isdigit()
            )
            original_family = native_texture_family(original)
            if (source_snapshot and original_family and
                    original_family == native_texture_family(value)):
                import_alias = True
            changed = bool(current_image is not None and current_image.is_dirty)
            if not changed:
                if not source_snapshot:
                    # Short/legacy material files are authored deltas even if
                    # they predate overrides=true. Their explicit textures
                    # must survive another export.
                    changed = True
                elif original_image is not None:
                    changed = value != original and not texture_images_equal(
                        current_image, original_image)
                else:
                    changed = value != original and not import_alias
            if changed:
                if (current_image is not None and
                        texture_property_uses_linear_data(property_name)):
                    current_image["eiem_linear"] = "true"
                referenced_images.add(value)
        else:
            # Version 0.3.x scenes do not carry a baseline. Preserve their
            # authored scalar/vector edits once, while still omitting original
            # Texture files whose section identity did not change.
            changed = (not has_baseline or
                       value != str(baseline.get(property_name, "")))
        if changed:
            overrides.append((property_name, value))

    material["eiem_texture_sections_json"] = json.dumps(
        texture_bindings, sort_keys=True, separators=(",", ":"))
    source_changed = has_baseline and source != str(baseline.get("source", source))
    if not force and not source_changed and not overrides:
        return None, set()

    values = [
        "format=EIEMMAT",
        "version=1",
        "overrides=true",
        "source=" + source,
        "name=" + str(material.get("eiem_name", material.name)),
    ]
    shader = str(material.get("eiem_shader", "")).strip()
    if shader:
        values.append("shader=" + shader)
    values.extend("%s=%s" % item for item in overrides)
    return values, referenced_images


def unique_export_section(requested, used, prefix):
    name = re.sub(r"[^a-zA-Z0-9_]", "_", str(requested))
    if not name.lower().startswith(prefix.lower()):
        name = prefix + name
    name = name[:70] or prefix
    candidate, index = name, 2
    while candidate.lower() in used:
        candidate = name + "_" + str(index)
        index += 1
    used.add(candidate.lower())
    return candidate


def positive_weighted_bone_names(obj):
    """Return vertex-group names that actually influence this Mesh."""
    names_by_index = {group.index: group.name for group in obj.vertex_groups}
    return {
        names_by_index[membership.group]
        for vertex in obj.data.vertices
        for membership in vertex.groups
        if membership.weight > 1e-8 and membership.group in names_by_index
    }


def package_physics_dependencies(plan, armatures, physics_objects):
    """Resolve authored Physics used by the selected Mesh dependency closure.

    Explicitly selected groups remain supported. In addition, a group is inferred
    when a selected visible Mesh has positive vertex weights on one of its nodes.
    This keeps Mesh-only export optional while preventing a physical Mesh export
    from silently dropping the Physics resource that drives those bones.
    """
    if not rig_export_enabled():
        # Mesh serialization still reads the Blender Armature for its skin
        # palette, but this export deliberately emits no Skeleton/Physics
        # resource declarations.
        return [], {}
    result_armatures = []
    seen_armatures = set()
    for rig in armatures or []:
        identity = rig.as_pointer()
        if identity not in seen_armatures:
            seen_armatures.add(identity)
            result_armatures.append(rig)

    groups = []
    seen_groups = set()
    for obj in physics_objects or []:
        if not obj or physics_authoring.native.is_native(obj):
            raise ValueError("原生 Physics v2 目前只可作为导入、编辑和独立作者导出来源")
        if obj.eiem_physics.kind != "GROUP":
            raise ValueError("组合 Mod 只能包含新增作者物理组")
        identity = obj.as_pointer()
        if identity in seen_groups:
            continue
        seen_groups.add(identity)
        groups.append(obj)

    visible_meshes = [obj for obj in plan["objects"] if obj not in plan["hidden"]]
    visible_rigs = {obj.find_armature() for obj in visible_meshes if obj.find_armature()}
    weighted_bones_by_rig = {}
    for obj in visible_meshes:
        rig = obj.find_armature()
        if not rig:
            continue
        weighted_bones_by_rig.setdefault(rig, set()).update(
            positive_weighted_bone_names(obj))

    for obj in bpy.data.objects:
        if not hasattr(obj, "eiem_physics") or obj.eiem_physics.kind != "GROUP":
            continue
        rig = obj.eiem_physics.rig
        if rig not in visible_rigs or obj.as_pointer() in seen_groups:
            continue
        node_ids = {node.bone_id for node in obj.eiem_physics.nodes}
        node_names = {
            bone.name for bone in rig.data.bones
            if str(bone.get("eiem_physics_id", "")) in node_ids
        }
        if node_names & weighted_bones_by_rig.get(rig, set()):
            seen_groups.add(obj.as_pointer())
            groups.append(obj)

    by_rig = {}
    for group in groups:
        rig = group.eiem_physics.rig
        if not rig or rig.type != "ARMATURE":
            raise ValueError("物理组缺少共享 Rig：" + group.name)
        if not rig.get("eiem_section"):
            raise ValueError("物理组的共享 Rig 不是 EIEM Skeleton：" + rig.name)
        if rig not in visible_rigs:
            raise ValueError("物理组没有同 Rig 的所选可见 Mesh：" + group.name)
        by_rig.setdefault(rig, []).append(group)
        if rig.as_pointer() not in seen_armatures:
            seen_armatures.add(rig.as_pointer())
            result_armatures.append(rig)
    for selected in by_rig.values():
        selected.sort(key=lambda obj: obj.eiem_physics.identity)
    return result_armatures, by_rig


def export_package(root, mesh_objects=None, armatures=None, physics_objects=None,
                   apply_static_switches=True, mesh_only=False,
                   lod_levels=None):
    if mesh_objects is None:
        if physics_objects is None:
            physics_objects = selected_eiem_physics()
        mesh_objects, selected_armatures = selected_eiem_resources()
        if armatures is None:
            armatures = selected_armatures
    mesh_objects = list(mesh_objects or [])
    if apply_static_switches and not mesh_only and not switch_export_enabled():
        mesh_objects = static_switch_selection(mesh_objects)
    armatures = list(armatures or [])
    if mesh_only:
        # Mesh-only is explicit: keep the mesh's existing skin payload, but do
        # not infer or publish Skeleton/Physics resource dependencies.
        armatures = []
        physics_objects = []
    if not mesh_objects:
        raise ValueError("No EIEM mesh objects selected")
    if bpy.context.mode != "OBJECT":
        raise ValueError("请回到物体模式后导出")
    plan = (plan_mesh_only_export(mesh_objects)
            if mesh_only else plan_switch_export(mesh_objects))
    if lod_levels is not None:
        plan = expand_lod_plan(plan, lod_levels)
    if not mesh_only:
        armatures, _ = package_physics_dependencies(plan, armatures, physics_objects)
    if not mesh_only and rig_export_enabled():
        for obj in plan["objects"]:
            rig = obj.find_armature()
            if rig and not obj.hide_render:
                authored = {
                    bone.name for bone, record, source in skeleton_author_nodes(rig)
                    if not source
                }
                if (authored & positive_weighted_bone_names(obj)
                        and rig not in armatures):
                    raise ValueError("%s 使用了新增骨架；请同时选择共享骨架后导出" % obj.name)
    # Stage all validation and binary writes first. Invalid author data must
    # not remove a previously working package.
    destination = Path(root).resolve()
    if any(str(destination).lower() == str(o.get("eiem_author_package", "")).lower()
           for o in plan["objects"]):
        raise ValueError("请选择新的 mod 输出目录，不要覆盖离线源资源包")
    with tempfile.TemporaryDirectory(prefix="eiem-export-") as temporary:
        staging = Path(temporary)
        stats = write_export_package(
            staging, plan, armatures, physics_objects,
            include_rig=not mesh_only, mesh_only=mesh_only)
        root = prepare_export_root(destination)
        for item in staging.iterdir():
            if item.is_dir():
                shutil.copytree(item, root / item.name, dirs_exist_ok=True)
            elif item.name != "mod.ini":
                shutil.copy2(item, root / item.name)
        shutil.copy2(staging / "mod.ini", root / "mod.ini")
        return stats


def export_visible_package(root, context=None):
    """Export the current visible mesh view without rig, physics or switches."""
    meshes = visible_eiem_resources(context)
    if not meshes:
        raise ValueError("No visible EIEM mesh objects found")
    previous_rig = os.environ.get("EIEM_DISABLE_RIG_EXPORT")
    previous_switch = os.environ.get("EIEM_DISABLE_SWITCH_EXPORT")
    os.environ["EIEM_DISABLE_RIG_EXPORT"] = "1"
    os.environ["EIEM_DISABLE_SWITCH_EXPORT"] = "1"
    try:
        return export_package(
            root,
            mesh_objects=meshes,
            armatures=[],
            physics_objects=[],
            apply_static_switches=False,
            mesh_only=True,
        )
    finally:
        if previous_rig is None:
            os.environ.pop("EIEM_DISABLE_RIG_EXPORT", None)
        else:
            os.environ["EIEM_DISABLE_RIG_EXPORT"] = previous_rig
        if previous_switch is None:
            os.environ.pop("EIEM_DISABLE_SWITCH_EXPORT", None)
        else:
            os.environ["EIEM_DISABLE_SWITCH_EXPORT"] = previous_switch


def merged_source_keys(mesh_objects, plan):
    """Return source identities whose selected parts will share one Mesh.

    A merged Mesh has a new, global submesh/material-slot layout.  Its Render
    therefore has to declare every material slot, including slots whose source
    material was unchanged.  Ordinary single Mesh exports can still omit those
    unchanged declarations and inherit the game's original material array.
    """
    groups = {}
    for obj in mesh_objects:
        if obj in plan["hidden"]:
            continue
        groups.setdefault(mesh_source_identity(obj), 0)
        groups[mesh_source_identity(obj)] += 1
    return {key for key, count in groups.items() if count >= 2}


def switch_export_enabled():
    """Whether switch groups should be emitted into the generated package.

    The authoring data remains in the .blend either way.  This export-only
    switch lets the static assembly path be tested without changing the
    runtime state machine or hand-editing the generated INI.
    """
    return os.environ.get("EIEM_DISABLE_SWITCH_EXPORT", "").strip() not in (
        "1", "true", "yes")


def rig_export_enabled():
    """Whether Skeleton/Physics resource files are emitted by this export."""
    return os.environ.get("EIEM_DISABLE_RIG_EXPORT", "").strip() not in (
        "1", "true", "yes")


def static_switch_selection(mesh_objects):
    """Keep only each switch group's authored default state for a static export."""
    selected = set(mesh_objects)
    grouped = set()
    defaults = set()
    for group in switch_groups():
        states = switch_states(group)
        default = next((state for state in states
                        if state.get("eiem_default")), None)
        if default is None:
            raise ValueError("切换组%s没有默认状态" % group.name)
        members = set(switch_members(group))
        grouped.update(members)
        defaults.update(obj for obj in switch_meshes(default) if obj in selected)
    return [obj for obj in mesh_objects
            if obj not in grouped or obj in defaults]


def build_merged_action(mesh_objects, plan, root, object_actions, shape_bindings,
                        material_sections, material_payloads, exported_armatures,
                        physics_sections, seen_mesh_sections, shared_mesh_sections,
                        resource_lines):
    """Collapse each source Mesh's sibling parts into one exported Mesh.

    A source Renderer carries exactly one Mesh, so sibling parts mounted on one
    source Renderer have to travel inside one Mesh. Exporting them separately
    makes the runtime create extra Renderers and register them after the game has
    already built its renderer registry, which is where a part could end up
    outside that registry and render in its bind pose.

    Merging is keyed on the source Mesh identity, which is also what the export
    plan groups by, so a group can never mix two different source Meshes. The
    merged Mesh carries one submesh per material slot of each part, and every
    part is left addressing that one section so the source Render declares a
    single direct mesh replacement with no additional Renderers.
    """
    merged_groups = {}
    for obj in mesh_objects:
        if obj in plan["hidden"]:
            continue
        key = mesh_source_identity(obj)
        groups = merged_groups.setdefault(key, {"members": []})
        groups["members"].append(obj)

    for key, group in sorted(merged_groups.items(), key=lambda item: item[0]):
        members = group["members"]
        if len(members) < 2:
            # A single part already declares the source's own Mesh.
            del merged_groups[key]
            continue
        first = members[0]
        # Name the merged resource after the part it replaces, marked as merged,
        # so a generated mod.ini shows at a glance that one Mesh carries the
        # whole group.
        section = unique_export_section(
            str(first.data["eiem_section"]) + "_MERGED", seen_mesh_sections,
            "Mesh")
        filename = "meshes/" + section + ".mesh"
        stats = write_merged_mesh(root / filename, members)
        # One submesh per material slot, parts in member order, so a part's first
        # slot lands after all earlier parts' slots. Material slots must be
        # numbered in that same merged space or a later part would overwrite an
        # earlier one's slot.
        action = ["mesh=" + section]
        action.extend(shape_bindings.get(first, []))
        rig = first.find_armature()
        skeleton = exported_armatures.get(rig)
        if skeleton:
            action.append("skeleton=" + skeleton)
        physics = physics_sections.get(rig)
        if physics:
            action.append("physics=" + physics)
        slot_offset = 0
        slot_ranges = {}
        for position, member in enumerate(members):
            slot_count = stats["slots"][position]
            slot_ranges[member] = (slot_offset, slot_offset + slot_count)
            for slot, material in enumerate(member.data.materials):
                material_section = material_sections.get(material, "")
                if material_section in material_payloads:
                    action.append("material.%d=%s"
                                  % (slot_offset + slot, material_section))
            slot_offset += slot_count
        for member in members:
            object_actions[member] = list(action)
        group["slot_ranges"] = slot_ranges

        declaration = [
            "[" + section + "]", "path=" + filename,
            "source=" + str(first.data.get("eiem_source", "")),
            "asset=" + str(first.data.get("eiem_asset", first.name)),
        ]
        target_path = str(first.data.get("eiem_target_path", "")).strip()
        if target_path:
            declaration.extend([
                "target.path=" + target_path,
                "target.asset=" + str(first.data.get("eiem_target_asset", "")),
            ])
        declaration.append("")
        resource_lines.extend(declaration)
    return merged_groups


def append_submesh_visibility(lines, binding, start, end):
    """Emit visibility conditions without replacing or recreating a Renderer."""
    if not binding:
        return
    if start < 0 or end > 32 or start >= end:
        raise ValueError("按键控制的合并 Mesh 必须包含 1 到 32 个 submesh")
    variable, visible = binding
    condition = " || ".join(
        "%s == %d" % (variable, value) for value in visible)
    for submesh in range(start, end):
        if not condition:
            lines.append("submesh_visible.%d=false" % submesh)
        else:
            lines.extend([
                "if " + condition,
                "    submesh_visible.%d=true" % submesh,
                "else",
                "    submesh_visible.%d=false" % submesh,
                "endif",
            ])


def write_export_package(root, plan, armatures, physics_objects=None,
                         include_rig=True, mesh_only=False):
    # A hidden selection declares skip only: its geometry, materials, textures
    # and shape controls must not become resource dependencies.
    mesh_objects = [o for o in plan["objects"] if o not in plan["hidden"]]
    if include_rig:
        armatures, physics_by_rig = package_physics_dependencies(
            plan, armatures, physics_objects)
    else:
        armatures, physics_by_rig = [], {}

    # The export graph is rooted at the selected Mesh resources. Materials and
    # images outside this dependency closure are never written.
    referenced_materials = {}
    force_materials = set()
    material_sections = {}
    used_material_sections = set()
    merged_keys = merged_source_keys(mesh_objects, plan)
    for obj in mesh_objects:
        original_slots = parse_json_property(
            obj, "eiem_original_material_sections_json", {})
        for slot, material in enumerate(obj.data.materials):
            if not material:
                continue
            original_section = str(material.get("eiem_section", ""))
            if not original_section:
                raise ValueError(
                    "Mesh %s material slot %d is not an EIEM material" %
                    (obj.name, slot))
            if material not in material_sections:
                material_sections[material] = unique_export_section(
                    original_section, used_material_sections, "Material")
            section = material_sections[material]
            referenced_materials[section] = material
            if (mesh_source_identity(obj) in merged_keys or
                    str(original_slots.get(str(slot), "")) != original_section):
                force_materials.add(section)

    images_by_section = {
        str(image.get("eiem_section")): image for image in bpy.data.images
        if image.get("eiem_section")
    }
    material_payloads = {}
    referenced_images = set()
    for section, material in sorted(referenced_materials.items()):
        values, images = material_override_payload(
            material, images_by_section, section in force_materials)
        if values is None:
            continue
        material_payloads[section] = (material, values)
        referenced_images.update(images)

    resource_lines = ["; Generated by EIEM Blender add-on", ""]
    render_lines = []
    seen_render_sections = set()
    object_actions = {}
    if mesh_only:
        # A Mesh-only package has no authoring controls.  In particular, do not
        # inspect unrelated switch/shape metadata attached to the selected
        # object; those controls belong to a full package export.
        shape_controls, shape_bindings, shape_hotkeys = [], {}, []
        switches_enabled = False
    else:
        shape_controls, shape_bindings, shape_hotkeys = plan_shape_controls(mesh_objects)
        switches_enabled = switch_export_enabled()
    switch_groups = plan["groups"] if switches_enabled else []
    switch_bindings = plan["bindings"] if switches_enabled else {}
    if not switches_enabled:
        shape_hotkeys = []
    used_hotkeys = {group[3] for group in switch_groups}
    for control in shape_hotkeys:
        if control["key"] in used_hotkeys:
            raise ValueError("多个控制使用同一快捷键：" + control["key"])
        used_hotkeys.add(control["key"])
    ui_payload = generate_mod_ui(
        switch_groups, shape_controls, shape_hotkeys
    ) if bpy.context.scene.eiem_ui_template else None
    if switch_groups or shape_controls or ui_payload:
        resource_lines.append("[Constants]")
        if ui_payload and ui_payload[0]:
            resource_lines.append("$ui_open=0")
        for group, states, default, key, variable in switch_groups:
            resource_lines.append("persist %s=%d" % (variable, default))
        for variable, label, default, minimum, maximum in shape_controls:
            resource_lines.append("persist %s=%.9g" % (variable, default))
        resource_lines.append("")
        for index, (group, states, default, key, variable) in enumerate(switch_groups, 1):
            key_lines = [
                "; " + group.name.replace("\n", " ").replace("\r", " "),
                "[KeySwitch%d]" % index, "key=" + key, "type=cycle",
                variable + "=" + ",".join(str(i) for i in switch_state_values(group)),
            ]
            resource_lines.extend(key_lines + [""])
        for index, control in enumerate(shape_hotkeys, 1):
            resource_lines.extend([
                "; " + control["label"].replace("\n", " ").replace("\r", " "),
                "[KeyShape%d]" % index,
                "key=" + control["key"],
                "type=" + control.get("type", "hold"),
                "speed=%.9g" % control.get("speed", 1.0),
                control["variable"] + "=%.9g" % control["target"],
                "",
            ])
        if ui_payload:
            ui_key, ui_source = ui_payload
            if ui_key:
                resource_lines.extend(["[KeyModUI]", "key=" + ui_key, "scope=both",
                                       "type=cycle", "$ui_open=0,1", ""])
            resource_lines.extend(["[UIMod]", "path=ui.lua", ""])
            (root / "ui.lua").write_text(ui_source, encoding="utf-8")
    exported_armatures = {}
    seen_skeleton_sections = set()
    seen_physics_sections = set()
    physics_sections = {}
    for rig in physics_by_rig:
        requested = str(rig.get("eiem_physics_section", "")).strip()
        if not requested:
            requested = "Physics" + str(rig.get("eiem_section", rig.name))
        physics_sections[rig] = unique_export_section(
            requested, seen_physics_sections, "Physics")
    for armature in armatures:
        section = unique_export_section(armature["eiem_section"], seen_skeleton_sections, "Skeleton")
        exported_armatures[armature] = section
        physics_section = physics_sections.get(armature)
        filename = ("physics/" + physics_section + "/skeleton.skeleton"
                    if physics_section else "skeletons/" + section + ".skeleton")
        (root / filename).parent.mkdir(parents=True, exist_ok=True)
        write_skeleton(root / filename, armature)
        declaration = [
            "[" + section + "]", "path=" + filename,
            "source=" + str(armature.get("eiem_source", "")),
        ]
        target_path = str(armature.get("eiem_target_path", "")).strip()
        if target_path:
            declaration.extend([
                "target.path=" + target_path,
                "target.asset=" + str(armature.get("eiem_target_asset", "")),
            ])
        declaration.append("")
        resource_lines.extend(declaration)

    for rig, groups in physics_by_rig.items():
        section = physics_sections[rig]
        directory = root / "physics" / section
        filename = "physics/" + section + "/" + section + ".physics"
        payload = physics_authoring.author_document(groups, "skeleton.skeleton")
        directory.mkdir(parents=True, exist_ok=True)
        (root / filename).write_bytes(
            physics_authoring.document.encode(payload))
        resource_lines.extend(["[" + section + "]", "path=" + filename, ""])

    if mesh_objects:
        (root / "meshes").mkdir(exist_ok=True)
    seen_mesh_sections = set()
    shared_mesh_sections = {}
    merged_groups = build_merged_action(
        mesh_objects, plan, root, object_actions, shape_bindings,
        material_sections, material_payloads, exported_armatures, physics_sections,
        seen_mesh_sections, shared_mesh_sections, resource_lines)
    merged_objects = {
        obj for group in merged_groups.values() for obj in group["members"]
    }
    for obj in mesh_objects:
        if obj in merged_objects:
            continue
        # P/duplicate copies source metadata, not geometry identity. Shared
        # datablocks can still share an exported resource when their skin maps
        # agree; independent split datablocks get unique files automatically.
        rig = obj.find_armature()
        # LOD template views intentionally share Blender geometry, but their
        # target resources remain independent Mesh assets. Include the target
        # source identity before datablock identity so they cannot collapse
        # into one section during ordinary resource deduplication.
        data_identity = (mesh_source_identity(obj), obj.data.as_pointer(),
                         rig.as_pointer() if rig else 0, tuple(g.name for g in obj.vertex_groups),
                         str(obj.get("eiem_bone_palette_json", "")),
                         str(obj.get("eiem_bindposes_json", "")),
                         str(obj.get("eiem_bone_paths_json", "")))
        section = shared_mesh_sections.get(data_identity)
        if section is None:
            section = unique_export_section(obj.data["eiem_section"], seen_mesh_sections, "Mesh")
            shared_mesh_sections[data_identity] = section
            filename = "meshes/" + section + ".mesh"
            # Merged groups were already written by build_merged_action, which
            # also re-pointed every member at the shared section. What is left
            # here is one Mesh per part.
            write_mesh(root / filename, obj)
            declaration = [
                "[" + section + "]", "path=" + filename,
                "source=" + str(obj.data.get("eiem_source", "")),
                "asset=" + str(obj.data.get("eiem_asset", obj.name)),
            ]
            target_path = str(obj.data.get("eiem_target_path", "")).strip()
            if target_path:
                declaration.extend([
                    "target.path=" + target_path,
                    "target.asset=" + str(obj.data.get("eiem_target_asset", "")),
                ])
            declaration.append("")
            resource_lines.extend(declaration)
        action = ["mesh=" + section]
        action.extend(shape_bindings.get(obj, []))
        rig = obj.find_armature()
        skeleton = exported_armatures.get(rig)
        if skeleton:
            action.append("skeleton=" + skeleton)
        physics = physics_sections.get(rig)
        if physics:
            action.append("physics=" + physics)
        for slot, material in enumerate(obj.data.materials):
            material_section = material_sections.get(material, "")
            if material_section in material_payloads:
                action.append("material.%d=%s" % (slot, material_section))
        object_actions[obj] = action

    for source_index, objects in enumerate(plan["sources"], 1):
        first = objects[0]
        root_render = unique_export_section(
            first.get("eiem_render_section", "RenderSource%d" % source_index),
            seen_render_sections, "Render")
        asset = str(first.get("eiem_render_asset", "") or first.data.get(
            "eiem_target_asset", first.data.get("eiem_asset", "")))

        merged = merged_groups.get(mesh_source_identity(first))
        if merged:
            # All active members share one source Renderer and one generated
            # Mesh. A switch changes only the corresponding submesh index
            # buffers, preserving the game's skeleton/LOD/physics ownership.
            render_lines.extend(["[" + root_render + "]", "asset=" + asset])
            render_lines.extend(object_actions[first])
            for member in merged["members"]:
                start, end = merged["slot_ranges"][member]
                append_submesh_visibility(
                    render_lines, switch_bindings.get(member), start, end)
            render_lines.append("")
            continue
        render_lines.extend(["[" + root_render + "]", "asset=" + asset])
        active_objects = [obj for obj in objects if obj in object_actions]
        if not active_objects:
            render_lines.extend(["handling=skip", ""])
            continue
        if len(active_objects) != 1:
            raise ValueError(
                "同一源 Mesh 的多个可见部件未能合并，拒绝回退到额外 Renderer")
        active = active_objects[0]
        render_lines.extend(object_actions[active])
        append_submesh_visibility(
            render_lines, switch_bindings.get(active), 0,
            max(1, len(active.data.materials)))
        render_lines.append("")

    if material_payloads:
        (root / "materials").mkdir(exist_ok=True)
    for section, (material, values) in sorted(material_payloads.items()):
        filename = "materials/" + section + ".mat"
        (root / filename).write_text("\n".join(values) + "\n", encoding="utf-8")
        declaration = ["[" + section + "]", "path=" + filename]
        target_path = str(material.get("eiem_target_path", ""))
        if target_path:
            declaration.extend([
                "target.path=" + target_path,
                "target.asset=" + str(material.get(
                    "eiem_target_asset", material.get("eiem_name", material.name))),
            ])
        declaration.append("")
        resource_lines.extend(declaration)

    if referenced_images:
        (root / "textures").mkdir(exist_ok=True)
    output_textures = {}
    for section in sorted(referenced_images):
        image = images_by_section.get(section)
        if image is None:
            raise ValueError("Material references missing Texture section " + section)
        disk_name = texture_output_filename(image)
        filename = "textures/" + disk_name
        collision = output_textures.get(disk_name.lower())
        if collision is not None and collision is not image and not texture_images_equal(collision, image):
            raise ValueError(
                "Two modified textures use the same filename with different content: " + disk_name)
        if collision is None:
            write_texture(root / filename, image)
            output_textures[disk_name.lower()] = image
        declaration = [
            "[" + section + "]", "path=" + filename,
            "source=" + str(image.get("eiem_source", "")),
            "name=" + str(image.get("eiem_name", image.name)),
        ]
        target_path = str(image.get("eiem_target_path", ""))
        if target_path:
            declaration.extend([
                "target.path=" + target_path,
                "target.asset=" + str(image.get("eiem_target_asset", image.name)),
            ])
        declaration.extend([
            "linear=" + str(image.get("eiem_linear", "false")),
            "mipmaps=" + str(image.get("eiem_mipmaps", "true")),
            "filter=" + str(image.get("eiem_filter", "1")),
            "wrap=" + str(image.get("eiem_wrap", "0")),
            "aniso=" + str(image.get("eiem_aniso", "1")),
            "mip_bias=" + str(image.get("eiem_mip_bias", "0")), "",
        ])
        resource_lines.extend(declaration)

    (root / "mod.ini").write_text(
        "\n".join(resource_lines + render_lines), encoding="utf-8")
    return {
        "meshes": len(shared_mesh_sections) + len(merged_groups),
        "skeletons": len(exported_armatures),
        "physics": len(physics_sections),
        "materials": len(material_payloads), "textures": len(referenced_images),
        "prefabs": 0,
    }


EIEM_INTERNAL_MATERIAL_PROPERTIES = {
    "eiem_baseline_json",
    "eiem_texture_sections_json",
    "eiem_material_file",
}


def eiem_id_property_path(key):
    escaped = str(key).replace("\\", "\\\\").replace('"', '\\"')
    return '["%s"]' % escaped


def draw_eiem_property(layout, owner, key, label, factor=0.24):
    """Draw one editable ID property with a consistently left-aligned label."""
    if owner is None or key not in owner:
        return False
    row = layout.row(align=True)
    split = row.split(factor=factor, align=True)
    label_column = split.column(align=True)
    label_column.alignment = "LEFT"
    label_column.label(text=label)
    value_column = split.column(align=True)
    value_column.prop(owner, eiem_id_property_path(key), text="")
    return True


def draw_eiem_rna_property(layout, owner, key, label, factor=0.24):
    row = layout.row(align=True)
    split = row.split(factor=factor, align=True)
    label_column = split.column(align=True)
    label_column.alignment = "LEFT"
    label_column.label(text=label)
    value_column = split.column(align=True)
    value_column.prop(owner, key, text="")


def material_property_groups(material):
    """Return stable, user-facing groups without exposing bookkeeping JSON."""
    keys = [key for key in material.keys()
            if key.startswith("eiem_") and
            key not in EIEM_INTERNAL_MATERIAL_PROPERTIES]
    texture_paths = [key for key in keys if key.startswith("eiem_texture.")]
    texture_transforms = [
        key for key in keys
        if key.startswith(("eiem_texture_scale.", "eiem_texture_offset."))
    ]
    identity = {
        "eiem_section", "eiem_name", "eiem_shader", "eiem_format",
        "eiem_version", "eiem_target_path", "eiem_target_asset",
    }
    parameters = [
        key for key in keys
        if key not in identity and key != "eiem_source" and
        key not in texture_paths and key not in texture_transforms
    ]
    resource = [key for key in (
        "eiem_section", "eiem_name", "eiem_shader", "eiem_version",
        "eiem_format", "eiem_target_path", "eiem_target_asset",
    ) if key in material]
    return texture_paths, texture_transforms, parameters, resource


def material_property_label(key):
    value = key[5:] if key.startswith("eiem_") else key
    for prefix, suffix in (
        ("texture_scale.", " 缩放"),
        ("texture_offset.", " 偏移"),
        ("texture.", ""),
        ("float.", ""),
        ("int.", ""),
        ("value4.", ""),
    ):
        if value.startswith(prefix):
            return value[len(prefix):] + suffix
    return value


class EIEM_OT_import_material(ImportHelper, bpy.types.Operator):
    bl_idname = "eiem.import_material"
    bl_label = "导入 EIEM 材质"
    bl_options = {"REGISTER", "UNDO"}
    filter_glob: StringProperty(default="*.mat", options={"HIDDEN"})

    def execute(self, context):
        try:
            material = import_material_file(self.filepath, context.object)
            self.report({"INFO"}, "已载入材质：" + material.name)
            return {"FINISHED"}
        except (ValueError, OSError, RuntimeError, configparser.Error) as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}


class EIEM_PT_material_properties(bpy.types.Panel):
    bl_label = "EIEM 材质"
    bl_idname = "MATERIAL_PT_eiem_properties"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"

    @classmethod
    def poll(cls, context):
        obj = getattr(context, "object", None)
        return obj is not None and obj.type == "MESH"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        material = context.material
        layout.operator("eiem.import_material", icon="IMPORT")
        if material is None or not material.get("eiem_section"):
            layout.label(text="导入后分配到当前材质槽，不覆盖原材质数据块")
            return
        texture_paths, texture_transforms, parameters, resource = \
            material_property_groups(material)

        # Texture paths are the most common edit and deliberately occupy the
        # first block. The short label is the real shader property name.
        box = layout.box()
        heading = box.row()
        heading.alignment = "LEFT"
        heading.label(text="贴图路径")
        if texture_paths:
            for key in texture_paths:
                draw_eiem_property(box, material, key,
                                   material_property_label(key))
        else:
            row = box.row()
            row.alignment = "LEFT"
            row.label(text="该材质没有贴图参数")

        draw_eiem_property(layout, material, "eiem_source", "源材质")

        if texture_transforms:
            box = layout.box()
            heading = box.row()
            heading.alignment = "LEFT"
            heading.label(text="贴图变换")
            for key in texture_transforms:
                draw_eiem_property(box, material, key,
                                   material_property_label(key))

        if parameters:
            box = layout.box()
            heading = box.row()
            heading.alignment = "LEFT"
            heading.label(text="材质参数")
            for key in parameters:
                draw_eiem_property(box, material, key,
                                   material_property_label(key))

        if resource:
            box = layout.box()
            heading = box.row()
            heading.alignment = "LEFT"
            heading.label(text="资源信息")
            labels = {
                "eiem_section": "资源段",
                "eiem_name": "材质名",
                "eiem_shader": "Shader",
                "eiem_version": "格式版本",
                "eiem_format": "格式",
                "eiem_target_path": "原材质路径",
                "eiem_target_asset": "原材质名称",
            }
            for key in resource:
                draw_eiem_property(box, material, key,
                                   labels.get(key, material_property_label(key)))


class EIEM_PG_shape_control(bpy.types.PropertyGroup):
    shape: StringProperty(name="形态键")
    enabled: BoolProperty(name="导出控制变量", default=True)
    automatic: BoolProperty(name="使用 Blender 当前权重", default=False)
    identity: StringProperty(options={"HIDDEN"})
    label: StringProperty(name="显示名称")
    default: FloatProperty(name="默认值", default=0.0)
    minimum: FloatProperty(name="最小值", default=0.0)
    maximum: FloatProperty(name="最大值", default=1.0)
    hotkey_increase: StringProperty(
        name="增大按键", default="",
        description="按住时以设定速度向形态键最大值移动；留空则不生成该按键")
    hotkey_decrease: StringProperty(
        name="减小按键", default="",
        description="按住时以设定速度向形态键最小值移动；留空则不生成该按键")
    hotkey_speed: FloatProperty(
        name="变化速度/秒", default=1.0, min=0.001,
        description="形态键实际权重每秒向按键指定的目标变化多少")


class EIEM_OT_shape_control(bpy.types.Operator):
    bl_idname = "eiem.shape_control"
    bl_label = "接管当前形态键"
    bl_options = {"REGISTER", "UNDO"}
    remove_index: IntProperty(default=-1, options={"HIDDEN"})
    sync: BoolProperty(default=False, options={"HIDDEN"})

    def execute(self, context):
        obj = context.object
        try:
            if self.sync:
                sync_new_shape_controls(obj)
            elif self.remove_index >= 0:
                obj.data.eiem_shape_controls.remove(self.remove_index)
            else:
                key = obj.active_shape_key
                add_shape_control(obj, key.name if key else "")
        except (ValueError, AttributeError) as error:
            self.report({"ERROR"}, str(error))
            return {"CANCELLED"}
        return {"FINISHED"}


class EIEM_OT_shape_key_record(bpy.types.Operator):
    bl_idname = "eiem.shape_key_record"
    bl_label = "录制形态键按键"
    bl_description = "为增大或减小动作录制独立按键；Esc 取消"
    bl_options = {"REGISTER", "UNDO"}
    control_index: IntProperty(options={"HIDDEN"})
    direction: StringProperty(options={"HIDDEN"})
    clear: BoolProperty(default=False, options={"HIDDEN"})

    @classmethod
    def poll(cls, context):
        obj = context.object
        return bool(obj and obj.type == "MESH"
                    and hasattr(obj.data, "eiem_shape_controls"))

    def _status(self, context, text=None):
        workspace = getattr(context, "workspace", None)
        if workspace and hasattr(workspace, "status_text_set"):
            workspace.status_text_set(text=text)

    def _control(self, context):
        controls = context.object.data.eiem_shape_controls
        if not 0 <= self.control_index < len(controls):
            raise ValueError("形态键控制已改变，请重新操作")
        if self.direction not in {"INCREASE", "DECREASE"}:
            raise ValueError("形态键按键方向无效")
        return controls[self.control_index]

    def invoke(self, context, event):
        if self.clear:
            return self.execute(context)
        try:
            control = self._control(context)
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}
        self._object = context.object
        direction = "增大" if self.direction == "INCREASE" else "减小"
        self._status(context, "录制形态键%s按键：请按一个键或组合键；Esc 取消" % direction)
        context.window_manager.modal_handler_add(self)
        self.report({'INFO'}, "请按下用于%s %s 的游戏快捷键；Esc 取消" %
                    (direction, control.shape))
        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.value != "PRESS":
            return {'RUNNING_MODAL'}
        if event.type == "ESC" and not (event.ctrl or event.shift or event.alt):
            self._status(context)
            return {'CANCELLED'}
        if event.type in {"LEFT_CTRL", "RIGHT_CTRL", "LEFT_SHIFT", "RIGHT_SHIFT",
                          "LEFT_ALT", "RIGHT_ALT", "OSKEY"}:
            return {'RUNNING_MODAL'}
        try:
            if context.object != self._object:
                raise ValueError("当前网格已改变，请重新录制")
            control = self._control(context)
            key = switch_key_from_event(event)
            set_shape_control_hotkey(
                context.object, control, key, self.direction, context.scene)
        except ValueError as error:
            self._status(context, "无法录制：%s；请按其他键，Esc 取消" % error)
            self.report({'WARNING'}, str(error))
            return {'RUNNING_MODAL'}
        self._status(context)
        self.report({'INFO'}, "%s → %s（%s）" % (
            key, control.shape,
            "增大" if self.direction == "INCREASE" else "减小"))
        return {'FINISHED'}

    def execute(self, context):
        try:
            control = self._control(context)
            set_shape_control_hotkey(
                context.object, control, "", self.direction, context.scene)
            return {'FINISHED'}
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}

    def cancel(self, context):
        self._status(context)


def draw_shape_hotkey_rows(layout, control, index):
    """Draw the same directional key editor in Properties and the N-panel."""
    for direction, attribute, label, icon in (
            ("INCREASE", "hotkey_increase", "增大到最大值", "TRIA_UP"),
            ("DECREASE", "hotkey_decrease", "减小到最小值", "TRIA_DOWN")):
        row = layout.row(align=True)
        key_value = getattr(control, attribute) or "未设置"
        row.label(text="%s：%s" % (label, key_value), icon=icon)
        op = row.operator("eiem.shape_key_record", text="录制", icon="REC")
        op.control_index, op.direction = index, direction
        if getattr(control, attribute):
            op = row.operator("eiem.shape_key_record", text="清除", icon="X")
            op.control_index, op.direction, op.clear = index, direction, True
    if control.hotkey_increase or control.hotkey_decrease:
        draw_eiem_rna_property(layout, control, "hotkey_speed",
                               "变化速度/秒", factor=0.32)
        layout.label(text="增大键只到最大值；减小键只到最小值")


class EIEM_PT_shape_controls(bpy.types.Panel):
    bl_label = "EIEM 形态键控制"
    bl_idname = "DATA_PT_eiem_shape_controls"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        obj = getattr(context, "object", None)
        return obj is not None and obj.type == "MESH" and bool(obj.data.get("eiem_section"))

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        obj = context.object
        layout.operator("eiem.shape_control", icon="ADD")
        layout.operator("eiem.shape_control", text="刷新新增形态键列表", icon="FILE_REFRESH").sync = True
        for index, control in enumerate(obj.data.eiem_shape_controls):
            box = layout.box()
            row = box.row(align=True)
            row.prop(control, "enabled")
            row.operator("eiem.shape_control", text="", icon="X").remove_index = index
            body = box.column()
            body.enabled = control.enabled
            if obj.data.shape_keys:
                body.prop_search(control, "shape", obj.data.shape_keys, "key_blocks")
            else:
                body.label(text="缺少形态键", icon="ERROR")
            body.prop(control, "automatic")
            key = obj.data.shape_keys.key_blocks.get(control.shape) if obj.data.shape_keys else None
            if control.automatic and key:
                draw_eiem_rna_property(body, key, "value", "默认值（当前权重）")
            for prop, label in (("label", "显示名称"), ("default", "默认值"),
                                ("minimum", "最小值"), ("maximum", "最大值")):
                if control.automatic and prop != "label":
                    continue
                draw_eiem_rna_property(body, control, prop, label)
            body.separator()
            body.label(text="独立按键控制", icon="EVENT_F")
            draw_shape_hotkey_rows(body, control, index)
        layout.label(text="新增形态键自动导出变量；原生通道需主动接管")
        layout.label(text="勾选“生成简单 UI”可同时生成独立控制窗口")


class EIEM_PT_mesh_properties(bpy.types.Panel):
    bl_label = "EIEM 网格"
    bl_idname = "DATA_PT_eiem_properties"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "data"

    @classmethod
    def poll(cls, context):
        obj = getattr(context, "object", None)
        return (obj is not None and obj.type == "MESH" and
                bool(obj.data.get("eiem_section")))

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        obj = context.object
        mesh = obj.data

        # Logical paths and selectors are placed first because they are the
        # only identity fields an author is expected to inspect or change.
        draw_eiem_property(layout, obj, "eiem_render_asset", "命中 Mesh")
        draw_eiem_property(layout, mesh, "eiem_source", "源 Mesh 路径")
        draw_eiem_property(layout, mesh, "eiem_asset", "Mesh 名称")

        box = layout.box()
        heading = box.row()
        heading.alignment = "LEFT"
        heading.label(text="资源信息")
        for owner, key, label in (
            (mesh, "eiem_section", "Mesh 资源段"),
            (obj, "eiem_prefab_path", "旧版 PFB 路径"),
            (obj, "eiem_prefab_section", "PFB 资源段"),
            (obj, "eiem_render_section", "Render 资源段"),
            (obj, "eiem_render_path", "旧版 Render 路径"),
            (obj, "eiem_skeleton", "骨架资源段"),
            (mesh, "eiem_coordinate_space", "坐标空间"),
            (mesh, "eiem_target_path", "原 Mesh 路径"),
            (mesh, "eiem_target_asset", "原 Mesh 名称"),
        ):
            draw_eiem_property(box, owner, key, label)


class EIEM_PT_image_properties(bpy.types.Panel):
    bl_label = "EIEM 贴图"
    bl_idname = "IMAGE_PT_eiem_properties"
    bl_space_type = "IMAGE_EDITOR"
    bl_region_type = "UI"
    bl_category = "EIEM"

    @classmethod
    def poll(cls, context):
        image = getattr(getattr(context, "space_data", None), "image", None)
        return image is not None and bool(image.get("eiem_section"))

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        image = context.space_data.image
        draw_eiem_rna_property(layout, image, "filepath", "本地贴图路径")
        draw_eiem_property(layout, image, "eiem_source", "源贴图路径")
        draw_eiem_property(layout, image, "eiem_target_path", "原贴图路径")
        draw_eiem_property(layout, image, "eiem_target_asset", "原贴图名称")

        box = layout.box()
        heading = box.row()
        heading.alignment = "LEFT"
        heading.label(text="采样参数")
        for key, label in (
            ("eiem_linear", "Linear"),
            ("eiem_mipmaps", "Mipmaps"),
            ("eiem_filter", "Filter"),
            ("eiem_wrap", "Wrap"),
            ("eiem_aniso", "Aniso"),
            ("eiem_mip_bias", "Mip Bias"),
        ):
            draw_eiem_property(box, image, key, label)

        box = layout.box()
        heading = box.row()
        heading.alignment = "LEFT"
        heading.label(text="资源信息")
        for key, label in (
            ("eiem_section", "资源段"),
            ("eiem_name", "贴图名"),
            ("eiem_relative_path", "包内路径"),
        ):
            draw_eiem_property(box, image, key, label)


class EIEM_OT_switch_create(bpy.types.Operator):
    bl_idname = "eiem.switch_create"
    bl_label = "从所选创建切换组"
    bl_options = {"REGISTER", "UNDO"}
    group_name: StringProperty(name="组名", default="部件切换")
    key: StringProperty(name="游戏快捷键", default="F6")

    @classmethod
    def poll(cls, context):
        return context.mode == "OBJECT" and bool(context.selected_objects)

    def invoke(self, context, event):
        used = {g.get("eiem_key", "").upper() for g in switch_groups(context.scene)}
        self.key = next(("F%d" % i for i in range(6, 25) if i != 10 and "F%d" % i not in used), "")
        return context.window_manager.invoke_props_dialog(self)

    def draw(self, context):
        self.layout.prop(self, "group_name")
        self.layout.label(text="将自动使用 %s；创建后可点“录制按键”修改" % (self.key or "可用快捷键"),
                          icon="EVENT_F")

    def execute(self, context):
        try:
            create_switch_group(self.group_name, self.key, context.selected_objects, context.scene)
            return {'FINISHED'}
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}


class EIEM_OT_switch_key_record(bpy.types.Operator):
    bl_idname = "eiem.switch_key_record"
    bl_label = "录制切换按键"
    bl_description = "点击后按下键盘按键或组合键；Esc 取消"
    bl_options = {"REGISTER", "UNDO"}

    @classmethod
    def poll(cls, context):
        group = context.scene.eiem_switch_active
        return bool(group and group.get("eiem_switch_group"))

    def _status(self, context, text=None):
        workspace = getattr(context, "workspace", None)
        if workspace and hasattr(workspace, "status_text_set"):
            workspace.status_text_set(text=text)

    def invoke(self, context, event):
        self._target = context.scene.eiem_switch_active
        self._status(context, "录制切换按键：请按一个键或组合键；Esc 取消")
        context.window_manager.modal_handler_add(self)
        self.report({'INFO'}, "请按下要用于 %s 的游戏快捷键；Esc 取消" % self._target.name)
        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.value != "PRESS":
            return {'RUNNING_MODAL'}
        if event.type == "ESC" and not (event.ctrl or event.shift or event.alt):
            self._status(context)
            return {'CANCELLED'}
        if event.type in {"LEFT_CTRL", "RIGHT_CTRL", "LEFT_SHIFT", "RIGHT_SHIFT",
                          "LEFT_ALT", "RIGHT_ALT", "OSKEY"}:
            return {'RUNNING_MODAL'}
        try:
            key = switch_key_from_event(event)
            if not self._target or not self._target.get("eiem_switch_group"):
                raise ValueError("切换组已删除")
            set_switch_group_key(self._target, key, context.scene)
        except ValueError as error:
            self._status(context, "无法录制：%s；请按其他键，Esc 取消" % error)
            self.report({'WARNING'}, str(error))
            return {'RUNNING_MODAL'}
        self._status(context)
        self.report({'INFO'}, "%s → %s" % (key, self._target.name))
        return {'FINISHED'}

    def cancel(self, context):
        self._status(context)


class EIEM_UL_switch_states(bpy.types.UIList):
    """Compact, ordered outfit-state list."""

    def draw_item(self, context, layout, data, item, icon, active_data,
                  active_propname, index):
        if not item.get("eiem_switch_state"):
            layout.label(text=item.name, icon="COLLECTION_NEW")
            return
        row = layout.row(align=True)
        drag = row.operator("eiem.switch_state_drag", text="", icon="GRIP",
                            emboss=False)
        drag.state_name = item.name
        default = row.operator(
            "eiem.switch_state", text="", icon=(
                "RADIOBUT_ON" if item.get("eiem_default") else "RADIOBUT_OFF"),
            emboss=False)
        default.action, default.state_name = "DEFAULT", item.name
        row.prop(item, "name", text="", emboss=False)
        row.label(text="%d/%d" % (
            len(switch_meshes(item)), len(switch_members(data))))
        preview = row.operator("eiem.switch_state", text="", icon="HIDE_OFF",
                               emboss=False)
        preview.action, preview.state_name = "PREVIEW", item.name


class EIEM_OT_switch_state_drag(bpy.types.Operator):
    bl_idname = "eiem.switch_state_drag"
    bl_label = "拖动款式排序"
    bl_description = "按住并上下拖动，改变该快捷键的循环顺序"
    bl_options = {"REGISTER", "UNDO", "BLOCKING"}
    state_name: StringProperty(options={"HIDDEN"})

    def _status(self, context, text=None):
        workspace = getattr(context, "workspace", None)
        if workspace and hasattr(workspace, "status_text_set"):
            workspace.status_text_set(text=text)

    def invoke(self, context, event):
        group = context.scene.eiem_switch_active
        state = next((candidate for candidate in switch_states(group)
                      if candidate.name == self.state_name), None) if group else None
        if state is None:
            self.report({'ERROR'}, "款式不存在，请重新操作")
            return {'CANCELLED'}
        self._group = group
        self._state = state
        self._original = switch_states(group)
        self._anchor_y = event.mouse_y
        self._step = max(16, round(
            20 * float(context.preferences.system.ui_scale)))
        self._status(context, "上下拖动调整款式循环顺序；松开完成，Esc 取消")
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}

    def modal(self, context, event):
        if event.type in {"ESC", "RIGHTMOUSE"}:
            try:
                set_switch_state_order(self._group, self._original)
                self._group.eiem_switch_state_index = self._original.index(
                    self._state)
            except (ReferenceError, ValueError):
                pass
            self._status(context)
            if context.area:
                context.area.tag_redraw()
            return {'CANCELLED'}
        if event.type == "LEFTMOUSE" and event.value == "RELEASE":
            self._status(context)
            return {'FINISHED'}
        if event.type != "MOUSEMOVE":
            return {'RUNNING_MODAL'}
        try:
            states = switch_states(self._group)
            current = states.index(self._state)
            delta = event.mouse_y - self._anchor_y
            while delta >= self._step and current > 0:
                current = reorder_switch_state(
                    self._group, self._state, current - 1)
                self._anchor_y += self._step
                delta = event.mouse_y - self._anchor_y
            while delta <= -self._step and current < len(states) - 1:
                current = reorder_switch_state(
                    self._group, self._state, current + 1)
                self._anchor_y -= self._step
                delta = event.mouse_y - self._anchor_y
            self._group.eiem_switch_state_index = current
        except (ReferenceError, ValueError) as error:
            self._status(context)
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}
        if context.area:
            context.area.tag_redraw()
        return {'RUNNING_MODAL'}

    def cancel(self, context):
        self._status(context)


class EIEM_OT_switch_state(bpy.types.Operator):
    bl_idname = "eiem.switch_state"
    bl_label = "编辑切换状态"
    bl_options = {"REGISTER", "UNDO"}
    action: StringProperty()
    state_name: StringProperty()

    def execute(self, context):
        group = context.scene.eiem_switch_active
        if not group or not group.get("eiem_switch_group"):
            return {'CANCELLED'}
        try:
            if self.action == "DELETE_GROUP":
                delete_switch_group(group, context.scene, context)
                return {'FINISHED'}
            states = switch_states(group)
            if self.action == "REMOVE_ACTIVE" and states:
                index = max(0, min(group.eiem_switch_state_index,
                                   len(states) - 1))
                state = states[index]
            else:
                state = next((s for s in states
                              if s.name == self.state_name), None)
                if state is None and states and not self.state_name:
                    index = max(0, min(group.eiem_switch_state_index,
                                       len(states) - 1))
                    state = states[index]
            if self.action == "ADD":
                state = add_switch_state(group, "款式 %d" % (len(switch_states(group)) + 1))
                capture_switch_state(group, state, context)
                group.eiem_switch_state_index = len(switch_states(group)) - 1
            elif self.action == "MEMBER_ADD":
                if not context.selected_objects:
                    raise ValueError("请先选择要加入当前组的 EIEM 网格")
                assign_switch_meshes(switch_states(group)[0], context.selected_objects,
                                     context.scene)
            elif self.action == "MEMBER_REMOVE":
                assign_switch_meshes(None, context.selected_objects, context.scene)
            elif self.action == "SELECT_MEMBERS":
                bpy.ops.object.select_all(action="DESELECT")
                for obj in switch_members(group):
                    if obj.name in context.view_layer.objects:
                        obj.select_set(True)
            elif state is None:
                raise ValueError("状态不存在，请重新选择")
            elif self.action == "DEFAULT":
                set_switch_default(group, state)
            elif self.action == "CAPTURE":
                capture_switch_state(group, state, context)
            elif self.action == "PREVIEW":
                preview_switch(group, state, context)
            elif self.action in {"REMOVE", "REMOVE_ACTIVE"}:
                if len(switch_states(group)) <= 2:
                    raise ValueError("至少保留两个款式")
                if state.children:
                    raise ValueError("请先移出状态的子集合再删除状态")
                restore_switch_preview(switch_members(group), context)
                for obj in list(state.objects):
                    state.objects.unlink(obj)
                was_default = state.get("eiem_default")
                bpy.data.collections.remove(state)
                if was_default:
                    set_switch_default(group, switch_states(group)[0])
                group.eiem_switch_state_index = min(
                    group.eiem_switch_state_index,
                    len(switch_states(group)) - 1)
            return {'FINISHED'}
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}


class EIEM_OT_switch_restore(bpy.types.Operator):
    bl_idname = "eiem.switch_restore_preview"
    bl_label = "恢复预览前状态"
    bl_options = {"REGISTER", "UNDO"}

    def execute(self, context):
        restore_switch_preview(context=context)
        return {'FINISHED'}


class EIEM_PT_switches(bpy.types.Panel):
    bl_label = "网格切换"
    bl_idname = "VIEW3D_PT_eiem_switches"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "EIEM"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        layout.operator("eiem.switch_create", icon="ADD")
        draw_eiem_rna_property(layout, context.scene, "eiem_ui_template", "生成简单 UI", factor=0.3)
        if context.scene.eiem_ui_template:
            draw_eiem_rna_property(layout, context.scene, "eiem_ui_key", "UI 开关键", factor=0.3)
            draw_eiem_rna_property(layout, context.scene, "eiem_ui_title", "UI 标题", factor=0.3)
            layout.label(text="按键留空则常显；布局在生成的 Lua 中")
        draw_eiem_rna_property(layout, context.scene, "eiem_switch_active", "当前组", factor=0.3)
        groups = switch_groups(context.scene)
        if groups:
            mapping = layout.box()
            mapping.label(text="游戏按键 → 切换组", icon="EVENT_F")
            for candidate in groups:
                mapping.label(text="%s  →  %s" % (candidate.get("eiem_key", "未设置"), candidate.name))
        group = context.scene.eiem_switch_active
        if group and group.get("eiem_switch_group"):
            name_row = layout.row(align=True)
            draw_eiem_rna_property(name_row, group, "name", "组名", factor=0.3)
            op = name_row.operator("eiem.switch_state", text="删除当前组", icon="TRASH")
            op.action = "DELETE_GROUP"
            key_row = layout.row(align=True)
            key_row.label(text="当前按键：" + str(group.get("eiem_key", "未设置")))
            key_row.operator("eiem.switch_key_record", text="录制按键", icon="REC")
            members = switch_members(group)
            row = layout.row(align=True)
            op = row.operator("eiem.switch_state", text="所选加入组", icon="ADD")
            op.action = "MEMBER_ADD"
            op = row.operator("eiem.switch_state", text="所选移出组", icon="REMOVE")
            op.action = "MEMBER_REMOVE"
            op = row.operator("eiem.switch_state", text="选择组内物体")
            op.action = "SELECT_MEMBERS"
            layout.label(text="%d 个受控网格；调整眼睛显隐后记录款式" % len(members),
                         icon="INFO")
            states = switch_states(group)
            if states:
                active_index = max(
                    0, min(group.eiem_switch_state_index, len(states) - 1))
                row = layout.row()
                row.template_list(
                    "EIEM_UL_switch_states", "", group, "children", group,
                    "eiem_switch_state_index", rows=max(3, min(7, len(states))))
                buttons = row.column(align=True)
                buttons.operator("eiem.switch_state", text="", icon="ADD").action = "ADD"
                buttons.operator(
                    "eiem.switch_state", text="", icon="REMOVE").action = "REMOVE_ACTIVE"
                layout.label(text="拖动每行左侧手柄可改变游戏内循环顺序")
                current = states[active_index]
                row = layout.row(align=True)
                op = row.operator("eiem.switch_state", text="用当前视图覆盖", icon="REC")
                op.action, op.state_name = "CAPTURE", current.name
                op = row.operator("eiem.switch_state", text="预览当前款式", icon="HIDE_OFF")
                op.action, op.state_name = "PREVIEW", current.name
        layout.operator("eiem.switch_restore_preview", icon="LOOP_BACK")
        layout.separator()
        layout.label(text="只导出所选网格与作者物理组；相机关=游戏隐藏")
        layout.label(text="眼睛只影响预览；需要的款式请一起选")
        obj = context.object
        if obj and obj.type == "MESH" and obj.data.get("eiem_section"):
            draw_eiem_rna_property(layout, obj, "hide_render", "游戏隐藏（相机）", factor=0.5)
        layout.operator("eiem.export_package", text="导出所选 mod", icon="EXPORT")


class EIEM_OT_import(ImportHelper, bpy.types.Operator):
    bl_idname = "eiem.import_package"
    bl_label = "Import EIEM package"
    directory: StringProperty(subtype="DIR_PATH")
    filter_glob: StringProperty(default="mod.ini", options={"HIDDEN"})
    clean: BoolProperty(name="Clear EIEM collection", default=False)
    include_physics: BoolProperty(name="导入物理骨骼与碰撞体", default=False,
                                 description="可选；物理数据不影响只导出 Mesh")
    physics_file: StringProperty(name="物理源文件", subtype="FILE_PATH",
                                description="可选 components.json 或 .physics；留空使用包内声明")

    def execute(self, context):
        try:
            root = self.directory or os.path.dirname(self.filepath)
            count = import_package(root, self.clean, self.include_physics, self.physics_file)
            self.report({'INFO'}, "Imported %d EIEM mesh resource(s)" % count)
            return {'FINISHED'}
        except Exception as error:
            self.report({'ERROR'}, str(error)); return {'CANCELLED'}


class EIEM_OT_export(ExportHelper, bpy.types.Operator):
    bl_idname = "eiem.export_package"
    bl_label = "Export EIEM package"
    filename_ext = ""
    directory: StringProperty(subtype="DIR_PATH")
    scope_message: StringProperty(options={"HIDDEN"})
    lod_all: BoolProperty(name="All discovered LODs", default=True)
    lod0: BoolProperty(name="LOD0", default=True)
    lod1: BoolProperty(name="LOD1", default=False)
    lod2: BoolProperty(name="LOD2", default=False)
    lod3: BoolProperty(name="LOD3", default=False)
    lod4: BoolProperty(name="LOD4", default=False)

    def _lod_levels(self, meshes):
        selected = [index for index, enabled in enumerate((
            self.lod0, self.lod1, self.lod2, self.lod3, self.lod4)) if enabled]
        return lod_levels_for_export(meshes, selected, self.lod_all)

    def invoke(self, context, event):
        try:
            meshes, _ = selected_eiem_resources(context)
            plan = plan_switch_export(meshes, context.scene)
            physics = selected_eiem_physics(context)
            levels = self._lod_levels(meshes)
            self.scope_message = "所选 %d 个网格 / %d 个物理组 / 隐藏 %d 个 / %d 个源资源 / %d 个切换组" % (
                len(plan["objects"]), len(physics), len(plan["hidden"]),
                len(plan["sources"]), len(plan["groups"]))
            self.scope_message += " / LOD: " + (
                ",".join(str(level) for level in levels) or "无")
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}
        return ExportHelper.invoke(self, context, event)

    def draw(self, context):
        self.layout.label(text=self.scope_message)
        box = self.layout.box()
        box.label(text="导出 LOD（仅使用当前工程已发现的级别）")
        box.prop(self, "lod_all")
        row = box.row(align=True)
        row.enabled = not self.lod_all
        for prop in ("lod0", "lod1", "lod2", "lod3", "lod4"):
            row.prop(self, prop)
        self.layout.label(text="物理组自动带入共享骨架，并作用于同 Rig 的所选网格")
        self.layout.label(text="相机关写 skip，不写网格文件")
        self.layout.label(text="眼睛不影响导出；未选中的源资源不修改")

    def execute(self, context):
        try:
            meshes, rigs = selected_eiem_resources(context)
            levels = self._lod_levels(meshes)
            stats = export_package(
                self.directory or os.path.dirname(self.filepath),
                mesh_objects=meshes,
                armatures=rigs,
                physics_objects=selected_eiem_physics(context),
                lod_levels=levels)
            self.report(
                {'INFO'},
                "Exported %(meshes)d Mesh, %(materials)d Material, "
                "%(textures)d Texture, %(skeletons)d Skeleton, "
                "%(physics)d Physics" % stats,
            )
            return {'FINISHED'}
        except Exception as error:
            self.report({'ERROR'}, str(error)); return {'CANCELLED'}


class EIEM_OT_export_mesh_only(ExportHelper, bpy.types.Operator):
    """Export selected EIEM meshes without Skeleton/Physics resources."""
    bl_idname = "eiem.export_mesh_only"
    bl_label = "Export Mesh Only"
    filename_ext = ""
    directory: StringProperty(subtype="DIR_PATH")
    scope_message: StringProperty(options={"HIDDEN"})
    lod_all: BoolProperty(name="All discovered LODs", default=True)
    lod0: BoolProperty(name="LOD0", default=True)
    lod1: BoolProperty(name="LOD1", default=False)
    lod2: BoolProperty(name="LOD2", default=False)
    lod3: BoolProperty(name="LOD3", default=False)
    lod4: BoolProperty(name="LOD4", default=False)

    def _lod_levels(self, meshes):
        selected = [index for index, enabled in enumerate((
            self.lod0, self.lod1, self.lod2, self.lod3, self.lod4)) if enabled]
        return lod_levels_for_export(meshes, selected, self.lod_all)

    def invoke(self, context, event):
        try:
            meshes, _ = selected_eiem_resources(context)
            self.scope_message = "LOD: " + ",".join(
                str(level) for level in self._lod_levels(meshes))
        except ValueError as error:
            self.report({'ERROR'}, str(error))
            return {'CANCELLED'}
        return ExportHelper.invoke(self, context, event)

    def execute(self, context):
        try:
            meshes, _ = selected_eiem_resources(context)
            levels = self._lod_levels(meshes)
            stats = export_package(
                self.directory or os.path.dirname(self.filepath),
                mesh_objects=meshes, armatures=[], physics_objects=[],
                mesh_only=True, lod_levels=levels)
            self.report(
                {'INFO'},
                "Exported %(meshes)d Mesh, %(materials)d Material, "
                "%(textures)d Texture" % stats)
            return {'FINISHED'}
        except Exception as error:
            self.report({'ERROR'}, str(error)); return {'CANCELLED'}

    def draw(self, context):
        if self.scope_message:
            self.layout.label(text=self.scope_message)
        box = self.layout.box()
        box.label(text="导出 LOD（仅使用当前工程已发现的级别）")
        box.prop(self, "lod_all")
        row = box.row(align=True)
        row.enabled = not self.lod_all
        for prop in ("lod0", "lod1", "lod2", "lod3", "lod4"):
            row.prop(self, prop)


def menu_import(self, context):
    self.layout.operator(EIEM_OT_import.bl_idname, text="EIEM package")


def menu_export(self, context):
    self.layout.operator(EIEM_OT_export.bl_idname, text="EIEM package")
    self.layout.operator(EIEM_OT_export_mesh_only.bl_idname, text="EIEM Mesh only")


classes = (
    EIEM_OT_import_material,
    EIEM_PG_shape_control,
    EIEM_OT_shape_control,
    EIEM_OT_shape_key_record,
    EIEM_PT_shape_controls,
    EIEM_OT_switch_create,
    EIEM_OT_switch_key_record,
    EIEM_UL_switch_states,
    EIEM_OT_switch_state_drag,
    EIEM_OT_switch_state,
    EIEM_OT_switch_restore,
    EIEM_PT_switches,
    EIEM_OT_import,
    EIEM_OT_export,
    EIEM_OT_export_mesh_only,
    EIEM_PT_material_properties,
    EIEM_PT_mesh_properties,
    EIEM_PT_image_properties,
)


def register():
    for cls in classes: bpy.utils.register_class(cls)
    physics_authoring.register(globals())
    bpy.types.Mesh.eiem_shape_controls = CollectionProperty(type=EIEM_PG_shape_control)
    bpy.types.Scene.eiem_ui_template = BoolProperty(name="生成简单 UI", default=False)
    bpy.types.Scene.eiem_ui_key = StringProperty(name="UI 开关键", default="", description="用户指定；留空生成常显 UI")
    bpy.types.Scene.eiem_ui_title = StringProperty(name="UI 标题", default="Mod controls")
    bpy.types.Collection.eiem_switch_state_index = IntProperty(
        name="当前款式", default=0, min=0)
    bpy.types.Scene.eiem_switch_active = PointerProperty(
        name="切换组", type=bpy.types.Collection,
        poll=lambda self, collection: bool(collection.get("eiem_switch_group")))
    bpy.types.TOPBAR_MT_file_import.append(menu_import)
    bpy.types.TOPBAR_MT_file_export.append(menu_export)


def unregister():
    physics_authoring.unregister()
    bpy.types.TOPBAR_MT_file_import.remove(menu_import)
    bpy.types.TOPBAR_MT_file_export.remove(menu_export)
    del bpy.types.Scene.eiem_switch_active
    del bpy.types.Scene.eiem_ui_key
    del bpy.types.Scene.eiem_ui_template
    del bpy.types.Scene.eiem_ui_title
    del bpy.types.Collection.eiem_switch_state_index
    del bpy.types.Mesh.eiem_shape_controls
    for cls in reversed(classes): bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
