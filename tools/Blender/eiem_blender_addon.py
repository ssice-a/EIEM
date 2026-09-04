bl_info = {
    "name": "EIEM Resource Package",
    "author": "EIEM",
    "version": (0, 4, 2),
    "blender": (3, 0, 0),
    "location": "File > Import/Export > EIEM package",
    "category": "Import-Export",
}

import configparser
import json
import os
import re
import shutil
import struct
import zlib
from pathlib import Path

import bpy
from bpy.props import StringProperty, BoolProperty
from bpy_extras.io_utils import ImportHelper, ExportHelper
from mathutils import Matrix, Quaternion, Vector


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


def unity_transform_matrix_to_blender(matrix):
    """Move a Unity Y-up Transform matrix into the Z-up mesh editing space.

    Endfield's serialized Mesh buffers are already authored in Z-up model
    space, while prefab Transform TRS uses Unity's Y-up space.  The X flip
    changes handedness and the -Z/Y mapping puts both resources in the same
    Blender basis.  Conjugating the complete matrix preserves rotation and
    non-uniform scale instead of trying to remap quaternion components.
    """
    basis = Matrix((
        (-1.0, 0.0, 0.0, 0.0),
        (0.0, 0.0, -1.0, 0.0),
        (0.0, 1.0, 0.0, 0.0),
        (0.0, 0.0, 0.0, 1.0),
    ))
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
    parser = configparser.ConfigParser(interpolation=None, strict=False)
    parser.optionxform = str
    with open(Path(root) / "mod.ini", "r", encoding="utf-8-sig") as stream:
        parser.read_file(stream)
    return parser


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
    coordinate = reader.string()
    count = reader.i32()
    nodes = []
    for _ in range(max(0, count)):
        nodes.append((reader.string(), reader.i32(),
                      struct.unpack("<3f", reader.take(12)),
                      struct.unpack("<4f", reader.take(16)),
                      struct.unpack("<3f", reader.take(12))))
    bone_count = reader.i32()
    bones = [reader.i32() for _ in range(max(0, bone_count))]
    root_bone = reader.i32()
    return locals()


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
        if key in ("path", "target.path", "target.asset"):
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
        if key not in ("path", "target.path", "target.asset")
    }
    material["eiem_baseline_json"] = json.dumps(
        baseline, sort_keys=True, separators=(",", ":"))
    return material


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
        (path, parent, tuple(position), tuple(rotation), tuple(scale))
        for path, parent, position, rotation, scale in payload["nodes"]
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
    obj.select_set(False)
    return obj


def clear_collection_objects(collection):
    for obj in list(collection.all_objects):
        bpy.data.objects.remove(obj, do_unlink=True)


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


def import_package(root, clean=False):
    parser = parse_ini(root)
    collection = bpy.data.collections.get("EIEM") or bpy.data.collections.new("EIEM")
    if collection.name not in bpy.context.scene.collection.children:
        bpy.context.scene.collection.children.link(collection)
    if clean:
        clear_collection_objects(collection)
        for material in [item for item in bpy.data.materials if item.get("eiem_section")]:
            bpy.data.materials.remove(material, do_unlink=True)
        for image in [item for item in bpy.data.images if item.get("eiem_section")]:
            bpy.data.images.remove(image, do_unlink=True)
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
                armature = make_armature(section, payload, collection)
                armature["eiem_source"] = values.get("source", "")
                armature["eiem_target_path"] = values.get("target.path", "")
                armature["eiem_target_asset"] = values.get("target.asset", "")
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
        mesh_resource_collection(collection, section, payload).objects.link(obj)
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
    return len(meshes)


def parse_json_property(owner, name, default):
    try:
        value = json.loads(owner.get(name, json.dumps(default)))
        return value
    except (TypeError, ValueError, json.JSONDecodeError):
        return default


def corner_values_to_points(mesh, values, label, tolerance=1.0e-6):
    """Convert Blender CORNER data to EIEM's per-vertex representation.

    A single EIEM vertex cannot carry two UV/color values.  Exporting the last
    loop silently would corrupt seams, so reject that edit until the vertices
    are split explicitly.
    """
    result = [None] * len(mesh.vertices)
    for loop, value in zip(mesh.loops, values):
        value = tuple(float(component) for component in value)
        previous = result[loop.vertex_index]
        if previous is not None and any(abs(a - b) > tolerance for a, b in zip(previous, value)):
            raise ValueError(
                "%s has a per-corner seam at vertex %d; split that vertex before EIEM export" %
                (label, loop.vertex_index))
        result[loop.vertex_index] = value
    width = len(values[0]) if values else 2
    return [value if value is not None else tuple(0.0 for _ in range(width)) for value in result]


def export_uv_channels(mesh):
    dimensions = parse_json_property(mesh, "eiem_uv_dimensions_json", [])
    output = []
    for channel in range(8):
        layer = mesh.uv_layers.get("UV%d" % channel)
        if layer is None:
            output.append([])
            continue
        dimension = int(dimensions[channel]) if channel < len(dimensions) and dimensions[channel] else 2
        if dimension < 2 or dimension > 4:
            raise ValueError("UV%d uses unsupported dimension %d" % (channel, dimension))
        xy = corner_values_to_points(
            mesh, [tuple(item.uv) for item in layer.data], "UV%d" % channel)
        zw = None
        if dimension > 2:
            zw = get_point_attribute(mesh, "EIEM_UV%d_ZW" % channel, "vector")
            if zw is None:
                raise ValueError("UV%d is %dD but its EIEM_UV%d_ZW attribute is missing" %
                                 (channel, dimension, channel))
        values = []
        for vertex in range(len(mesh.vertices)):
            values.extend(xy[vertex][:2])
            if dimension >= 3:
                values.append(zw[vertex][0])
            if dimension == 4:
                values.append(zw[vertex][1])
        output.append(values)
    return output


def export_colors(mesh):
    if not hasattr(mesh, "color_attributes"):
        return []
    attribute = mesh.color_attributes.get("Color")
    if attribute is None:
        return []
    values = [tuple(item.color) for item in attribute.data]
    if attribute.domain == "CORNER":
        values = corner_values_to_points(mesh, values, "Color")
    elif attribute.domain != "POINT" or len(values) != len(mesh.vertices):
        raise ValueError("Color must use the POINT or CORNER domain")
    return [component for value in values for component in value[:4]]


def export_blend_shapes(obj, to_source):
    mesh = obj.data
    shape_keys = mesh.shape_keys
    if not shape_keys or "Basis" not in shape_keys.key_blocks:
        return [], [], [], [], []
    basis = shape_keys.key_blocks["Basis"]
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
        for index, (basis_point, shape_point) in enumerate(zip(basis.data, key.data)):
            delta_position = shape_point.co - basis_point.co
            delta_normal = Vector(normal_values[index]) if normal_values else Vector((0.0, 0.0, 0.0))
            delta_tangent = Vector(tangent_values[index]) if tangent_values else Vector((0.0, 0.0, 0.0))
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
    if additional and original_count != len(mesh.vertices):
        raise ValueError("blend-shape additional normals cannot survive a topology change")
    converted_additional = [to_source(value) for value in additional]
    return vertices, frames, channels, weights, converted_additional


def write_mesh(path, obj):
    mesh = obj.data
    mesh.calc_loop_triangles()
    coordinate = mesh.get("eiem_coordinate_space", "unity-y-up-left-handed")
    to_source = blender_to_unity if is_unity_left_handed(coordinate) else (lambda value: tuple(value))
    vertices = [coord for vertex in mesh.vertices for coord in to_source(vertex.co)]

    # EIEM stores one normal per vertex, whereas Blender evaluates custom
    # normals per face corner. Keep the original float32 values for a lossless
    # untouched round trip; after a native Blender normal edit, export the
    # evaluated state instead of silently falling back to that backup.
    source_normals = get_point_attribute(mesh, "EIEM_SourceNormal", "vector")
    baseline_crc = str(mesh.get("eiem_normal_baseline_crc", ""))
    if (source_normals is not None and len(source_normals) == len(mesh.vertices)
            and baseline_crc and normal_state_crc(mesh) == baseline_crc):
        normal_values = source_normals
    else:
        normal_values = [None] * len(mesh.vertices)
        normal_counts = [0] * len(mesh.vertices)
        first_values = [None] * len(mesh.vertices)
        for loop, corner in zip(mesh.loops, mesh.corner_normals):
            vertex_index = loop.vertex_index
            value = Vector(corner.vector)
            first = first_values[vertex_index]
            # Blender's split-normal encoding adds small per-corner numerical
            # differences. A real split larger than this does not fit EIEM's
            # per-vertex channel and must be represented by split vertices.
            if first is not None and (first - value).length > 1.0e-3:
                raise ValueError(
                    "Mesh %s has multiple corner normals for vertex %d; split the vertex before EIEM export"
                    % (obj.name, vertex_index)
                )
            if first is None:
                first_values[vertex_index] = value.copy()
                normal_values[vertex_index] = Vector((0.0, 0.0, 0.0))
            normal_values[vertex_index] += value
            normal_counts[vertex_index] += 1
        for index, value in enumerate(normal_values):
            if value is None:
                value = Vector(mesh.vertices[index].normal)
            elif normal_counts[index] > 1:
                value /= normal_counts[index]
            if value.length_squared:
                value.normalize()
            normal_values[index] = tuple(value)
    normals = [coord for value in normal_values for coord in to_source(value)]

    tangent_values = get_point_attribute(mesh, "EIEM_Tangent", "vector")
    tangent_signs = get_point_attribute(mesh, "EIEM_TangentSign", "value")
    tangents = []
    if tangent_values is not None or tangent_signs is not None:
        if tangent_values is None or tangent_signs is None:
            raise ValueError("EIEM_Tangent and EIEM_TangentSign must both be present")
        for value, sign in zip(tangent_values, tangent_signs):
            tangents.extend(to_source(value))
            tangents.append(sign)
    colors = export_colors(mesh)
    uv_layers = export_uv_channels(mesh)

    indices = []
    submesh_indices = {}
    for tri in mesh.loop_triangles:
        submesh = int(mesh.polygons[tri.polygon_index].material_index)
        triangle = tuple(tri.vertices)
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
    palette = [int(value) for value in parse_json_property(obj, "eiem_bone_palette_json", [])]
    bone_index = {}
    if armature:
        if not palette:
            raise ValueError("skinned mesh %s has no EIEM bone palette" % obj.name)
        bones = list(armature.data.bones)
        for palette_index, node_index in enumerate(palette):
            if not 0 <= node_index < len(bones):
                raise ValueError("mesh bone palette references an absent armature bone")
            bone_index[bones[node_index].name] = palette_index
    skin = []
    if armature:
        for vertex in mesh.vertices:
            influences = []
            for group in vertex.groups:
                if group.group >= len(obj.vertex_groups):
                    continue
                name = obj.vertex_groups[group.group].name
                if name in bone_index and group.weight > 0.0:
                    influences.append((float(group.weight), bone_index[name]))
            influences.sort(key=lambda item: item[0], reverse=True)
            influences = influences[:4]
            while len(influences) < 4:
                influences.append((0.0, 0))
            skin.append((tuple(item[0] for item in influences),
                         tuple(item[1] for item in influences)))
    bindposes = parse_json_property(obj, "eiem_bindposes_json", [])
    bone_hashes = [int(value) for value in parse_json_property(obj, "eiem_bone_hashes_json", [])]
    if armature and (len(bindposes) != len(palette) or len(bone_hashes) != len(palette)):
        raise ValueError("mesh bindposes, bone hashes and local palette must have equal lengths")
    blend_vertices, blend_frames, blend_channels, blend_weights, additional = export_blend_shapes(obj, to_source)

    writer = Writer(); writer.raw(MAGIC_MESH); writer.i32(3)
    writer.string(coordinate)
    writer.string(obj.data.get("eiem_source", "")); writer.string(obj.data.get("eiem_asset", obj.name))
    writer.i32(len(mesh.vertices)); writer.floats(vertices); writer.floats(normals)
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
    bone_paths = parse_json_property(obj, "eiem_bone_paths_json", [])
    if armature:
        bones = list(armature.data.bones)
        bone_paths = [str(bones[index].get("eiem_path", bones[index].name)) for index in palette]
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


def write_skeleton(path, obj):
    bones = list(obj.data.bones)
    index = {bone: i for i, bone in enumerate(bones)}
    writer = Writer(); writer.raw(MAGIC_SKEL); writer.i32(1)
    writer.string(obj.get("eiem_coordinate_space", "unity-y-up-left-handed"))
    writer.i32(len(bones))
    for bone in bones:
        writer.string(bone.get("eiem_path", bone.name))
        writer.i32(index.get(bone.parent, -1))
        position = bone.get("eiem_local_position")
        rotation = bone.get("eiem_local_rotation")
        scale = bone.get("eiem_local_scale")
        if not (position and len(position) == 3):
            position = tuple(bone.head_local)
        if not (rotation and len(rotation) == 4):
            rotation = (0.0, 0.0, 0.0, 1.0)
        if not (scale and len(scale) == 3):
            scale = (1.0, 1.0, 1.0)
        writer.raw(struct.pack("<3f", *(float(value) for value in position)))
        writer.raw(struct.pack("<4f", *(float(value) for value in rotation)))
        writer.raw(struct.pack("<3f", *(float(value) for value in scale)))
    # This file represents one shared skeleton hierarchy.  Renderer-local bone
    # palettes are serialized in their Mesh resources as hashes/bindposes and
    # must never turn into duplicate Armatures.
    writer.i32(0)
    writer.i32(-1)
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


def unique_texture_section(path, images_by_section):
    fragment = re.sub(r"[^A-Za-z0-9_]", "_", Path(path).stem).strip("_") or "External"
    fragment = fragment[:1].upper() + fragment[1:]
    base = "Texture" + fragment
    used = {str(section).lower() for section in images_by_section}
    candidate = base
    serial = 2
    while candidate.lower() in used:
        candidate = "%s%d" % (base, serial)
        serial += 1
    return candidate


def resolve_material_texture(material, property_name, value, images_by_section,
                             original_bindings):
    """Resolve a Blender-facing image path to an exported Texture section."""
    value = str(value).strip()
    if value in images_by_section:
        original_bindings[property_name] = value
        return value

    requested = Path(bpy.path.abspath(os.path.expandvars(value))).resolve()
    if not requested.is_file():
        raise ValueError(
            "%s %s texture path does not exist: %s" %
            (material.name, property_name, requested))

    requested_key = os.path.normcase(str(requested))
    for section, image in images_by_section.items():
        existing = image_absolute_path(image)
        if existing and os.path.normcase(existing) == requested_key:
            original_bindings[property_name] = section
            return section

    image = bpy.data.images.load(str(requested), check_existing=True)
    existing_section = image.get("eiem_section", "")
    if existing_section and existing_section in images_by_section:
        original_bindings[property_name] = existing_section
        return existing_section

    section = unique_texture_section(requested, images_by_section)
    template = images_by_section.get(original_bindings.get(property_name, ""))
    defaults = {
        "eiem_linear": "false", "eiem_mipmaps": "true",
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
        image[key] = template.get(key, default) if template else default
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
            for directory in ("meshes", "materials", "textures", "skeletons"):
                candidate = (root / directory).resolve()
                if candidate.parent == root and candidate.is_dir():
                    shutil.rmtree(candidate)
    root.mkdir(parents=True, exist_ok=True)
    return root


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


def material_override_payload(material, images_by_section, force=False):
    """Return the minimal clone payload and the Texture sections it needs."""
    source = str(material.get("eiem_source", "")).strip()
    if not source:
        raise ValueError("Material %s has no source game material" % material.name)

    has_baseline = "eiem_baseline_json" in material
    baseline = parse_json_property(material, "eiem_baseline_json", {})
    texture_bindings = parse_json_property(
        material, "eiem_texture_sections_json", {})
    reserved = {
        "eiem_section", "eiem_source", "eiem_shader", "eiem_format",
        "eiem_version", "eiem_name", "eiem_target_path", "eiem_target_asset",
        "eiem_texture_sections_json", "eiem_baseline_json",
    }
    overrides = []
    referenced_images = set()
    for key, raw_value in material.items():
        if not key.startswith("eiem_") or key in reserved:
            continue
        property_name = key[5:]
        value = str(raw_value)
        if property_name.startswith("texture."):
            value = resolve_material_texture(
                material, property_name, value, images_by_section,
                texture_bindings)
            original = str(
                baseline.get(property_name,
                             texture_bindings.get(property_name, "")))
            changed = value != original
            if changed:
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
        "source=" + source,
        "name=" + str(material.get("eiem_name", material.name)),
    ]
    shader = str(material.get("eiem_shader", "")).strip()
    if shader:
        values.append("shader=" + shader)
    values.extend("%s=%s" % item for item in overrides)
    return values, referenced_images


def export_package(root, mesh_objects=None, armatures=None):
    root = prepare_export_root(root)
    if mesh_objects is None:
        mesh_objects, selected_armatures = selected_eiem_resources()
        if armatures is None:
            armatures = selected_armatures
    mesh_objects = list(mesh_objects or [])
    armatures = list(armatures or [])
    if not mesh_objects:
        raise ValueError("No EIEM mesh objects selected")

    # The export graph is rooted at the selected Mesh resources. Materials and
    # images outside this dependency closure are never written.
    referenced_materials = {}
    force_materials = set()
    for obj in mesh_objects:
        original_slots = parse_json_property(
            obj, "eiem_original_material_sections_json", {})
        for slot, material in enumerate(obj.data.materials):
            if not material:
                continue
            section = str(material.get("eiem_section", ""))
            if not section:
                raise ValueError(
                    "Mesh %s material slot %d is not an EIEM material" %
                    (obj.name, slot))
            referenced_materials[section] = material
            if str(original_slots.get(str(slot), "")) != section:
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
    exported_armatures = {}
    if armatures:
        (root / "skeletons").mkdir(exist_ok=True)
    for armature in armatures:
        section = str(armature["eiem_section"])
        exported_armatures[section] = armature
        filename = "skeletons/" + section + ".skeleton"
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

    (root / "meshes").mkdir(exist_ok=True)
    seen_mesh_sections = set()
    for obj in mesh_objects:
        section = str(obj.data["eiem_section"])
        if section in seen_mesh_sections:
            raise ValueError("EIEM Mesh section selected more than once: " + section)
        seen_mesh_sections.add(section)
        filename = "meshes/" + section + ".mesh"
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

        render = str(obj.get("eiem_render_section", "")).strip()
        if not render:
            render = ("Render" + section[4:]) if section.lower().startswith("mesh") else ("Render" + section)
        # Standalone Render rules are shared-Mesh identity actions. Legacy
        # packages used `path` for both a logical asset path and a hierarchy
        # path, which made matching context-dependent. Export only the stable
        # Mesh sub-asset name; an explicitly scoped authoring mode can add a
        # hierarchy selector later without overloading this field again.
        render_asset = str(obj.get("eiem_render_asset", "")).strip()
        if not render_asset:
            render_asset = str(obj.data.get(
                "eiem_target_asset", obj.data.get("eiem_asset", ""))).strip()
        if not render_asset:
            raise ValueError("Render %s has no Mesh asset selector" % render)
        if render in seen_render_sections:
            raise ValueError("Render section selected more than once: " + render)
        seen_render_sections.add(render)
        render_lines.append("[" + render + "]")
        render_lines.append("asset=" + render_asset)
        render_lines.append("mesh=" + section)
        skeleton = str(obj.get("eiem_skeleton", ""))
        if skeleton in exported_armatures:
            render_lines.append("skeleton=" + skeleton)
        for slot, material in enumerate(obj.data.materials):
            material_section = str(material.get("eiem_section", "")) if material else ""
            if material_section in material_payloads:
                render_lines.append("material.%d=%s" % (slot, material_section))
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
    for section in sorted(referenced_images):
        image = images_by_section.get(section)
        if image is None:
            raise ValueError("Material references missing Texture section " + section)
        filename = "textures/" + section + ".png"
        write_texture(root / filename, image)
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
        "meshes": len(mesh_objects), "skeletons": len(exported_armatures),
        "materials": len(material_payloads), "textures": len(referenced_images),
        "prefabs": 0,
    }


EIEM_INTERNAL_MATERIAL_PROPERTIES = {
    "eiem_baseline_json",
    "eiem_texture_sections_json",
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


class EIEM_PT_material_properties(bpy.types.Panel):
    bl_label = "EIEM 材质"
    bl_idname = "MATERIAL_PT_eiem_properties"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"

    @classmethod
    def poll(cls, context):
        material = getattr(context, "material", None)
        return material is not None and bool(material.get("eiem_section"))

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = False
        layout.use_property_decorate = False
        material = context.material
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


class EIEM_OT_import(ImportHelper, bpy.types.Operator):
    bl_idname = "eiem.import_package"
    bl_label = "Import EIEM package"
    directory: StringProperty(subtype="DIR_PATH")
    filter_glob: StringProperty(default="mod.ini", options={"HIDDEN"})
    clean: BoolProperty(name="Clear EIEM collection", default=False)

    def execute(self, context):
        try:
            root = self.directory or os.path.dirname(self.filepath)
            count = import_package(root, self.clean)
            self.report({'INFO'}, "Imported %d EIEM mesh resource(s)" % count)
            return {'FINISHED'}
        except Exception as error:
            self.report({'ERROR'}, str(error)); return {'CANCELLED'}


class EIEM_OT_export(ExportHelper, bpy.types.Operator):
    bl_idname = "eiem.export_package"
    bl_label = "Export EIEM package"
    filename_ext = ""
    directory: StringProperty(subtype="DIR_PATH")

    def execute(self, context):
        try:
            stats = export_package(self.directory or os.path.dirname(self.filepath))
            self.report(
                {'INFO'},
                "Exported %(meshes)d Mesh, %(materials)d Material, "
                "%(textures)d Texture, %(skeletons)d Skeleton" % stats,
            )
            return {'FINISHED'}
        except Exception as error:
            self.report({'ERROR'}, str(error)); return {'CANCELLED'}


def menu_import(self, context):
    self.layout.operator(EIEM_OT_import.bl_idname, text="EIEM package")


def menu_export(self, context):
    self.layout.operator(EIEM_OT_export.bl_idname, text="EIEM package")


classes = (
    EIEM_OT_import,
    EIEM_OT_export,
    EIEM_PT_material_properties,
    EIEM_PT_mesh_properties,
    EIEM_PT_image_properties,
)


def register():
    for cls in classes: bpy.utils.register_class(cls)
    bpy.types.TOPBAR_MT_file_import.append(menu_import)
    bpy.types.TOPBAR_MT_file_export.append(menu_export)


def unregister():
    bpy.types.TOPBAR_MT_file_import.remove(menu_import)
    bpy.types.TOPBAR_MT_file_export.remove(menu_export)
    for cls in reversed(classes): bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
