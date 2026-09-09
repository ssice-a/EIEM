"""Real Blender corner seams -> EIEM point streams, without changing author data."""
import importlib.util
import json
import struct
import sys
from pathlib import Path

import bpy

addon_path, output = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_corner_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()


def floats(values):
    return struct.pack("<%df" % len(values), *values)


def make_object(name):
    mesh = bpy.data.meshes.new(name)
    # Two triangles share vertices 0/2; vertex 4 is intentionally loose.
    mesh.from_pydata([(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 2, 2)],
                     [], [(0, 1, 2), (0, 2, 3)])
    mesh["eiem_section"] = "Mesh" + name
    mesh["eiem_asset"] = name
    mesh["eiem_source"] = "assets/test/" + name + ".asset"
    mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
    mesh["eiem_uv_dimensions_json"] = '[2,0,4,0,0,0,0,0]'
    for channel in (0, 2):
        layer = mesh.uv_layers.new(name="UV%d" % channel)
        for loop in mesh.loops:
            layer.data[loop.index].uv = (loop.vertex_index * .125, channel * .125)
    addon.set_point_attribute(mesh, "EIEM_UV2_ZW", "FLOAT2",
                              [(i * .125, -i * .25) for i in range(5)], "vector")
    addon.set_point_attribute(mesh, "EIEM_Tangent", "FLOAT_VECTOR",
                              [(1, i * .125, -0.0) for i in range(5)], "vector")
    addon.set_point_attribute(mesh, "EIEM_TangentSign", "FLOAT", [1, -1, 1, -1, 1], "value")
    mesh.polygons[1].material_index = 2  # Slot 1 must remain empty.
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def snapshot(obj):
    m = obj.data
    return (tuple(tuple(v.co) for v in m.vertices),
            tuple(tuple(p.vertices) for p in m.polygons),
            tuple(tuple(tuple(d.uv) for d in layer.data) for layer in m.uv_layers),
            tuple(tuple(c.vector) for c in m.corner_normals),
            tuple(tuple((g.group, g.weight) for g in v.groups) for v in m.vertices),
            tuple(tuple(tuple(p.co) for p in key.data) for key in m.shape_keys.key_blocks)
            if m.shape_keys else ())


def assert_corners(obj, result):
    m = obj.data
    assert result["vertex_count"] >= len(m.vertices)
    assert result["uvs"][1] == [] and len(m.uv_layers) == 2
    assert result["submeshes"][1][2] == 0
    assert len(result["indices"]) == 6
    for tri in m.loop_triangles:
        slot = m.polygons[tri.polygon_index].material_index
        start = result["submeshes"][slot][1]
        # Blender->Unity conversion reverses each triangle's winding.
        indices = result["indices"][start:start + 3]
        indices = (indices[0], indices[2], indices[1])
        for loop_index, target in zip(tri.loops, indices):
            source = m.loops[loop_index].vertex_index
            assert floats(result["vertices"][target * 3:target * 3 + 3]) == floats(addon.blender_to_unity(m.vertices[source].co))
            for channel, dimension in ((0, 2), (2, 4)):
                expected = tuple(m.uv_layers['UV%d' % channel].data[loop_index].uv)
                if dimension == 4:
                    expected += tuple(m.attributes['EIEM_UV2_ZW'].data[source].vector)
                assert floats(result["uvs"][channel][target * dimension:(target + 1) * dimension]) == floats(expected)
            tangent = m.attributes['EIEM_Tangent'].data[source].vector
            sign = m.attributes['EIEM_TangentSign'].data[source].value
            assert floats(result['tangents'][target * 4:(target + 1) * 4]) == floats((*addon.blender_to_unity(tangent), sign))
            if "Color" in m.color_attributes:
                colors = m.color_attributes['Color']
                expected = colors.data[loop_index if colors.domain == 'CORNER' else source].color
                assert floats(result['colors'][target * 4:(target + 1) * 4]) == floats(expected)
            yield source, target, loop_index


obj = make_object("UVSeam")
m = obj.data
m.uv_layers['UV0'].data[3].uv = (.75, .5)
m.uv_layers['UV2'].data[4].uv = (.625, .75)
addon.write_mesh(output / 'uv-only.mesh', obj)
uv_only = addon.read_mesh(output / 'uv-only.mesh')
assert uv_only['vertex_count'] == 7
list(assert_corners(obj, uv_only))
colors = m.color_attributes.new(name="Color", type="FLOAT_COLOR", domain="CORNER")
for loop in m.loops:
    colors.data[loop.index].color = (loop.index * .125, .5, .25, 1)
rig = addon.make_armature("SkeletonRig", {
    "coordinate": "unity-y-up-left-handed",
    "nodes": [('', -1, (0,0,0), (0,0,0,1), (1,1,1))] +
             [(name, 0, (0,0,-i), (0,0,0,1), (1,1,1))
              for i,name in enumerate(('Root','Tip','Unused'))]}, bpy.context.scene.collection)
for name in ("Root", "Tip", "Unused"):
    obj.vertex_groups.new(name=name)
for i in range(5):
    obj.vertex_groups[0].add([i], .75, "REPLACE")
    obj.vertex_groups[1].add([i], .25, "REPLACE")
obj.modifiers.new("Skin", "ARMATURE").object = rig
obj['eiem_bone_palette_json'] = '[1,2,3]'
obj['eiem_bone_hashes_json'] = '[10,20,30]'
obj['eiem_bindposes_json'] = json.dumps([[int(r == c) for r in range(4) for c in range(4)]] * 3)
basis = obj.shape_key_add(name="Basis")
key = obj.shape_key_add(name="Inflate")
for i in (0, 2):
    key.data[i].co.z += .25
addon.set_point_attribute(m, 'MorphNormal', 'FLOAT_VECTOR', [(0, .125, 0)] * 5, 'vector')
addon.set_point_attribute(m, 'MorphTangent', 'FLOAT_VECTOR', [(.125, 0, 0)] * 5, 'vector')
m['eiem_blend_shapes_json'] = json.dumps([{'name':'Inflate', 'hash':123, 'frames':[
    {'key':'Inflate', 'source_name':'Inflate', 'weight':100,
     'normal_attribute':'MorphNormal', 'tangent_attribute':'MorphTangent'}]}])
before = snapshot(obj)
stats = addon.export_package(output / 'package', [obj], [])
assert stats['meshes'] == 1
result = addon.read_mesh(next((output / 'package/meshes').glob('*.mesh')))
assert result['vertex_count'] == 7, result['vertex_count']
assert result['bone_hashes'] == [10,20,30]
assert result['bone_paths'] == ['Root','Tip','Unused']
assert len(result['bindposes']) == 3
morph = {entry[0]:entry for entry in result['blend_vertices']}
assert len(morph) == result['vertex_count']
assert result['blend_frames'][0][2] == result['vertex_count']
for source, target, _ in assert_corners(obj, result):
    assert result['skin'][target] == ([.75,.25,0,0], [0,1,0,0])
    delta = key.data[source].co - basis.data[source].co
    assert morph[target][1:] == (addon.blender_to_unity(delta), (0,.125,0), (-.125,0,0))
assert before == snapshot(obj), 'Export modified the authoring mesh'
# A second export is deterministic, including duplicate IDs and sparse shapes.
again = output / 'again.mesh'
addon.write_mesh(again, obj)
assert again.read_bytes() == next((output / 'package/meshes').glob('*.mesh')).read_bytes()

# Opaque game-specific shape streams must not be silently assigned a made-up
# seam mapping, even when deletion + duplication happen to cancel in count.
package_before = {p.relative_to(output / 'package'): p.read_bytes()
                  for p in (output / 'package').rglob('*') if p.is_file()}
m['eiem_blend_additional_json'] = '[[0,1,0]]'
m['eiem_original_vertex_count'] = 7
try:
    addon.export_package(output / 'package', [obj], [])
    raise AssertionError('Unknown additional-normal mapping was accepted')
except ValueError as error:
    assert 'additional normals' in str(error)
assert package_before == {p.relative_to(output / 'package'): p.read_bytes()
                          for p in (output / 'package').rglob('*') if p.is_file()}
del m['eiem_blend_additional_json']
del m['eiem_original_vertex_count']

# Normal seams alone must split instead of averaging/shading across the edge.
normal_obj = make_object('NormalSeam')
normal_mesh = normal_obj.data
for p in normal_mesh.polygons:
    p.use_smooth = True
normal_mesh.normals_split_custom_set([(0,0,1)] * 3 + [(0,1,0)] * 3)
normal_mesh.update()
before = snapshot(normal_obj)
addon.write_mesh(output / 'normals.mesh', normal_obj)
result = addon.read_mesh(output / 'normals.mesh')
assert result['vertex_count'] == 7
for source, target, corner in assert_corners(normal_obj, result):
    assert floats(result['normals'][target*3:target*3+3]) == floats(addon.blender_to_unity(normal_mesh.corner_normals[corner].vector))
assert snapshot(normal_obj) == before

# Untouched authored normals/tangents keep float32 bits and vertex IDs.
raw = addon.read_mesh(output / 'normals.mesh')
raw['colors'] = [component for i in range(raw['vertex_count']) for component in (.25,.5,.75,1)]
round_mesh = addon.make_mesh('RoundTrip', raw)
round_obj = bpy.data.objects.new('RoundTrip', round_mesh)
bpy.context.scene.collection.objects.link(round_obj)
addon.write_mesh(output / 'roundtrip.mesh', round_obj)
roundtrip = addon.read_mesh(output / 'roundtrip.mesh')
assert roundtrip['vertex_count'] == raw['vertex_count']
assert roundtrip['indices'] == raw['indices']
for channel in ('vertices','normals','tangents','colors'):
    assert floats(roundtrip[channel]) == floats(raw[channel]), channel
for channel in range(8):
    assert floats(roundtrip['uvs'][channel]) == floats(raw['uvs'][channel]), channel
addon.unregister()
print('EIEM_CORNER_EXPORT_OK')
