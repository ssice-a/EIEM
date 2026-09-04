"""Blender-side regression checks for an EIEM package round trip.

Run inside Blender with ``EIEM_TEST_PACKAGE`` and ``EIEM_TEST_OUTPUT`` passed
through ``runpy.run_path(..., init_globals=...)``.  The add-on module must be
loaded as ``eiem_blender_addon`` first.
"""

from pathlib import Path
import configparser
import shutil
import sys

import bpy
from mathutils import Vector


package = Path(globals()["EIEM_TEST_PACKAGE"])
output = Path(globals()["EIEM_TEST_OUTPUT"])
addon = sys.modules["eiem_blender_addon"]

if output.name != "_eiem_roundtrip_test" or output.parent != Path(r"E:\EIEM_Workspace"):
    raise RuntimeError(f"refusing to clear unexpected test output path: {output}")


def hierarchy_key(payload):
    return tuple(
        (path, parent, tuple(position), tuple(rotation), tuple(scale))
        for path, parent, position, rotation, scale in payload["nodes"]
    )


parser = configparser.ConfigParser(interpolation=None, strict=False)
parser.optionxform = str
with (package / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    parser.read_file(stream)

mesh_payloads = {}
skeleton_keys = set()
material_values = {}
texture_values = {}
prefab_values = {}
for section in parser.sections():
    values = parser[section]
    if section.lower().startswith("mesh") and values.get("path"):
        mesh_payloads[section] = addon.read_mesh(addon.safe_path(package, values["path"]))
    elif section.lower().startswith("skeleton") and values.get("path"):
        payload = addon.read_skeleton(addon.safe_path(package, values["path"]))
        skeleton_keys.add(hierarchy_key(payload))
    elif section.lower().startswith("material") and values.get("path"):
        material_values[section] = addon.read_flat_properties(
            addon.safe_path(package, values["path"]))
    elif section.lower().startswith("texture") and values.get("path"):
        texture_values[section] = dict(values)
    elif section.lower().startswith("prefab") and values.get("path"):
        prefab_values[section] = dict(values)

addon.import_package(package, clean=True)
collection = bpy.data.collections["EIEM"]
objects = list(collection.all_objects)
armatures = [obj for obj in objects if obj.type == "ARMATURE"]
assert len(armatures) == len(skeleton_keys), (
    f"resource import created {len(armatures)} armatures for "
    f"{len(skeleton_keys)} unique skeleton hierarchies"
)
assert len([item for item in bpy.data.materials if item.get("eiem_section")]) == len(material_values)
assert len([item for item in bpy.data.images if item.get("eiem_section")]) == len(texture_values)
for section, values in material_values.items():
    material = next(item for item in bpy.data.materials if item.get("eiem_section") == section)
    assert material.get("eiem_source") == values.get("source", ""), section
    for key, value in values.items():
        if key.startswith("texture."):
            image = next(
                item for item in bpy.data.images
                if item.get("eiem_section") == value
            )
            assert material.get("eiem_" + key) == addon.image_absolute_path(image), (
                section, key
            )

for section, payload in mesh_payloads.items():
    obj = next(
        obj for obj in objects
        if obj.type == "MESH" and obj.data.get("eiem_section") == section
    )
    dimensions = [
        len(values) // payload["vertex_count"] if values else 0
        for values in payload["uvs"]
    ]
    assert len(obj.data.uv_layers) == sum(value >= 2 for value in dimensions)
    for channel, dimension in enumerate(dimensions):
        if dimension > 2:
            assert f"EIEM_UV{channel}_ZW" in obj.data.attributes
    if payload["normals"]:
        assert all(polygon.use_smooth for polygon in obj.data.polygons), (
            section, "source normals require smooth faces"
        )
        assert "EIEM_SourceNormal" in obj.data.attributes, (section, "normal backup")
        assert "EIEM_Normal" not in obj.data.attributes, (section, "legacy fake normal")
        assert obj.data.get("eiem_normal_baseline_crc") == addon.normal_state_crc(obj.data), (
            section, "normal baseline"
        )
        source_normals = [
            payload["normals"][index:index + 3]
            for index in range(0, len(payload["normals"]), 3)
        ]
        if addon.is_unity_left_handed(payload["coordinate"]):
            source_normals = [addon.unity_to_blender(value) for value in source_normals]
        for loop, corner in zip(obj.data.loops, obj.data.corner_normals):
            expected = Vector(source_normals[loop.vertex_index])
            if expected.length_squared:
                expected.normalize()
                # Blender encodes split normals in a face-fan-relative space.
                # The decoded native normal may be slightly quantized, but it
                # must still point in the authored direction.
                alignment = Vector(corner.vector).dot(expected)
                assert alignment >= 0.9998, (
                    section, "custom normal", loop.index, alignment
                )
    if payload["tangents"]:
        assert "EIEM_Tangent" in obj.data.attributes
        assert "EIEM_TangentSign" in obj.data.attributes
    if payload["colors"]:
        assert "Color" in obj.data.color_attributes
    expected_shapes = sum(channel[3] for channel in payload["blend_channels"])
    actual_shapes = len(obj.data.shape_keys.key_blocks) - 1 if obj.data.shape_keys else 0
    assert actual_shapes == expected_shapes, (section, actual_shapes, expected_shapes)

if output.exists():
    shutil.rmtree(output)
addon.export_package(
    output,
    mesh_objects=[obj for obj in bpy.context.scene.objects
                  if obj.type == "MESH" and obj.data.get("eiem_section")],
    armatures=[obj for obj in bpy.context.scene.objects
               if obj.type == "ARMATURE" and obj.get("eiem_section")],
)

roundtrip = configparser.ConfigParser(interpolation=None, strict=False)
roundtrip.optionxform = str
with (output / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    roundtrip.read_file(stream)

assert sum(name.lower().startswith("skeleton") for name in roundtrip.sections()) == len(skeleton_keys)
assert sum(name.lower().startswith("prefab") for name in roundtrip.sections()) == 0
# An untouched imported material inherits the game's original resource. The
# incremental package therefore contains no redundant .mat or Texture copies.
assert sum(name.lower().startswith("material") for name in roundtrip.sections()) == 0
assert sum(name.lower().startswith("texture") for name in roundtrip.sections()) == 0


def assert_close(actual, expected, section, channel, tolerance=2.0e-5):
    assert len(actual) == len(expected), (section, channel, len(actual), len(expected))
    if actual:
        error = max(abs(float(a) - float(b)) for a, b in zip(actual, expected))
        assert error <= tolerance, (section, channel, error)


for section, source in mesh_payloads.items():
    target = addon.read_mesh(addon.safe_path(output, roundtrip[section]["path"]))
    assert_close(target["vertices"], source["vertices"], section, "vertices")
    assert_close(target["normals"], source["normals"], section, "normals")
    assert_close(target["tangents"], source["tangents"], section, "tangents")
    assert_close(target["colors"], source["colors"], section, "colors")
    assert target["indices"] == source["indices"], (section, "triangle winding")
    for channel, (actual, expected) in enumerate(zip(target["uvs"], source["uvs"])):
        assert_close(actual, expected, section, f"UV{channel}")
    assert target["bone_hashes"] == source["bone_hashes"], section
    assert len(target["bone_paths"]) == len(source["bindposes"]), section
    assert [value[0] for value in target["blend_channels"]] == [
        value[0] for value in source["blend_channels"]
    ], section
    assert [value[3] for value in target["blend_channels"]] == [
        value[3] for value in source["blend_channels"]
    ], section
    for index, (actual, expected) in enumerate(zip(target["blend_vertices"], source["blend_vertices"])):
        assert actual[0] == expected[0], (section, "blend vertex index", index)
        assert_close(actual[1], expected[1], section, f"blend position {index}")
        assert_close(actual[2], expected[2], section, f"blend normal {index}")
        assert_close(actual[3], expected[3], section, f"blend tangent {index}")

# Native normal edits must be exported. The source backup is allowed only for
# an untouched native normal state, never as a permanent hidden override.
normal_probe_source = None
for section, source in mesh_payloads.items():
    if not source["normals"]:
        continue
    source_object = next(
        obj for obj in objects
        if obj.type == "MESH" and obj.data.get("eiem_section") == section
    )
    loop_counts = [0] * len(source_object.data.vertices)
    for loop in source_object.data.loops:
        loop_counts[loop.vertex_index] += 1
    one_corner_vertex = next((index for index, count in enumerate(loop_counts) if count == 1), None)
    if one_corner_vertex is not None:
        normal_probe_source = (section, source, source_object, one_corner_vertex)
        break
assert normal_probe_source is not None, "normal edit probe needs a one-corner vertex"
probe_section, probe_payload, source_object, probe_vertex = normal_probe_source
probe_object = source_object.copy()
probe_object.data = source_object.data.copy()
collection.objects.link(probe_object)
edited_corner_normals = [Vector(item.vector) for item in probe_object.data.corner_normals]
for loop in probe_object.data.loops:
    if loop.vertex_index == probe_vertex:
        edited_corner_normals[loop.index] = -edited_corner_normals[loop.index]
probe_object.data.normals_split_custom_set(edited_corner_normals)
probe_object.data.update()
assert addon.normal_state_crc(probe_object.data) != probe_object.data["eiem_normal_baseline_crc"]
expected_blender = next(
    Vector(corner.vector)
    for loop, corner in zip(probe_object.data.loops, probe_object.data.corner_normals)
    if loop.vertex_index == probe_vertex
)
if expected_blender.length_squared:
    expected_blender.normalize()
expected_source = (addon.blender_to_unity(expected_blender)
                   if addon.is_unity_left_handed(probe_payload["coordinate"])
                   else tuple(expected_blender))
normal_probe_path = output / "meshes" / "normal_edit_probe.mesh"
addon.write_mesh(normal_probe_path, probe_object)
normal_probe_target = addon.read_mesh(normal_probe_path)
normal_start = probe_vertex * 3
assert_close(
    normal_probe_target["normals"][normal_start:normal_start + 3],
    expected_source,
    probe_section,
    "edited native normal",
)
assert max(
    abs(float(a) - float(b))
    for a, b in zip(
        normal_probe_target["normals"][normal_start:normal_start + 3],
        probe_payload["normals"][normal_start:normal_start + 3],
    )
) > 0.1, (probe_section, "native normal edit was ignored")
probe_mesh = probe_object.data
bpy.data.objects.remove(probe_object, do_unlink=True)
bpy.data.meshes.remove(probe_mesh)

# The current Typhoea source contains no standard Unity colour stream. Exercise
# Blender's native Color Attribute path synthetically so absence in this one
# character cannot leave colour round-tripping untested.
probe_source = dict(next(iter(mesh_payloads.values())))
probe_source["colors"] = [
    component
    for index in range(probe_source["vertex_count"])
    for component in ((index % 7) / 6.0, (index % 5) / 4.0, (index % 3) / 2.0, 1.0)
]
probe_source["skin"] = []
probe_source["bindposes"] = []
probe_source["bone_hashes"] = []
probe_source["bone_paths"] = []
probe_source["blend_vertices"] = []
probe_source["blend_frames"] = []
probe_source["blend_channels"] = []
probe_source["blend_weights"] = []
probe_source["additional"] = []
probe_mesh = addon.make_mesh("EIEMColorProbe", probe_source)
probe_object = bpy.data.objects.new("EIEMColorProbe", probe_mesh)
collection.objects.link(probe_object)
probe_path = output / "meshes" / "color_probe.mesh"
addon.write_mesh(probe_path, probe_object)
probe_target = addon.read_mesh(probe_path)
assert_close(probe_target["colors"], probe_source["colors"], "probe", "colors")
bpy.data.objects.remove(probe_object, do_unlink=True)

print(
    f"EIEM round trip OK: meshes={len(mesh_payloads)} "
    f"skeleton_resources={len(skeleton_keys)} materials={len(material_values)} "
    f"textures={len(texture_values)}"
)
