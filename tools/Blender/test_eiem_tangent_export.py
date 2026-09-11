"""Headless tangent export regressions; does not touch the user's Blender."""
import importlib.util
import struct
import sys
from pathlib import Path

import bpy
from mathutils import Vector

addon_path, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
spec = importlib.util.spec_from_file_location('eiem_tangent_export_test', addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)


def bits(values):
    return struct.pack('<%df' % len(values), *values)


def make_object(name, uv=True):
    mesh = bpy.data.meshes.new(name)
    # Mirrored UV charts meet at vertex 0 with equal UV/normal but opposite
    # tangent direction/sign. Only the tangent seam forces this vertex split.
    mesh.from_pydata([(0,0,0),(1,0,0),(0,1,0),(-1,0,0),(0,-1,0), (9,9,9)],
                     [], [(0,1,2),(0,3,4)])
    mesh['eiem_coordinate_space'] = 'unity-y-up-left-handed'
    if uv:
        for layer_name in ('UV0', 'UV2'):
            layer = mesh.uv_layers.new(name=layer_name)
            for i, value in enumerate(((0,0),(1,0),(0,1),(0,0),(1,0),(0,-1))):
                layer.data[i].uv = value
        mesh.uv_layers.active_index = 1  # Generation must explicitly use UV0.
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def export(obj, name):
    mesh = obj.data
    before = (len(bpy.data.meshes),
              tuple((a.name, a.domain, a.data_type) for a in mesh.attributes),
              tuple(tuple(c.vector) for c in mesh.corner_normals),
              tuple(tuple(tuple(v.uv) for v in layer.data) for layer in mesh.uv_layers))
    file = output / (name + '.mesh')
    addon.write_mesh(file, obj)
    first = file.read_bytes()
    addon.write_mesh(file, obj)
    assert first == file.read_bytes(), 'Non-deterministic tangent generation'
    after = (len(bpy.data.meshes),
             tuple((a.name, a.domain, a.data_type) for a in mesh.attributes),
             tuple(tuple(c.vector) for c in mesh.corner_normals),
             tuple(tuple(tuple(v.uv) for v in layer.data) for layer in mesh.uv_layers))
    assert before == after, 'Export changed author data or leaked a temporary Mesh'
    return addon.read_mesh(file)


def corner_indices(payload):
    result = []
    for start in range(0, len(payload['indices']), 3):
        a,b,c = payload['indices'][start:start+3]
        result.extend((a,c,b))  # Back to Blender winding.
    return result


# Blender may re-encode an imported split normal by a tiny amount after an
# update/save. That is still the same author normal and must not trigger a full
# current-normal export. A visible direction edit must trigger it.
normal_probe = make_object('NormalEncodingProbe')
source_normals = [(0, 0, 1)] * len(normal_probe.data.vertices)
addon.set_point_attribute(normal_probe.data, 'EIEM_SourceNormal', 'FLOAT_VECTOR',
                          source_normals, 'vector')
assert addon.normal_state_matches_source(normal_probe.data, source_normals)
for polygon in normal_probe.data.polygons:
    polygon.use_smooth = True
normal_probe.data.normals_split_custom_set([(0, .01, .99995)] * len(normal_probe.data.loops))
normal_probe.data.update()
assert addon.normal_state_matches_source(normal_probe.data, source_normals)
normal_probe.data.normals_split_custom_set([(0, .1, .994987)] * len(normal_probe.data.loops))
normal_probe.data.update()
assert not addon.normal_state_matches_source(normal_probe.data, source_normals)


# Missing tangents must produce actual UV0-derived data, not an empty array.
obj = make_object('NewGeometry')
result = export(obj, 'new')
assert len(result['tangents']) == result['vertex_count'] * 4, 'Missing generated tangents'
assert result['vertex_count'] == 7, 'Mirrored tangent-only seam was not split'
assert result['uvs'][1] == [] and len(result['uvs'][2]) == result['vertex_count'] * 2
indices = corner_indices(result)
assert indices[0] != indices[3]
for corner, index in enumerate(indices):
    normal = Vector(result['normals'][index*3:index*3+3])
    tangent = Vector(result['tangents'][index*4:index*4+3])
    sign = result['tangents'][index*4+3]
    # In source coordinates dP/du is -X on the first chart, +X on the second;
    # dP/dv is +Y on both. Independently validate the reflected tangent frame.
    assert (tangent - Vector((-1 if corner < 3 else 1, 0, 0))).length < 1e-6
    assert (sign * normal.cross(tangent) - Vector((0,1,0))).length < 1e-6
assert result['tangents'][5*4:6*4] == [0,0,0,0], 'Invented tangent on a loose vertex'

# Import/re-export of generated data also remains byte-exact.
round_mesh = addon.make_mesh('GeneratedRoundTrip', result)
round_obj = bpy.data.objects.new('GeneratedRoundTrip', round_mesh)
bpy.context.scene.collection.objects.link(round_obj)
roundtrip = export(round_obj, 'roundtrip')
assert bits(result['tangents']) == bits(roundtrip['tangents'])
assert result['indices'] == roundtrip['indices']

# Native custom bases are retained, including sign and signed-zero bits,
# even if they differ from the frame MikkTSpace would choose for the UVs.
native = make_object('Native')
values = [(1, .125*i, -0.0) for i in range(6)]
signs = [1,-1,1,-1,1,-1]
addon.set_point_attribute(native.data, 'EIEM_Tangent', 'FLOAT_VECTOR', values, 'vector')
addon.set_point_attribute(native.data, 'EIEM_TangentSign', 'FLOAT', signs, 'value')
native_result = export(native, 'native')
assert native_result['vertex_count'] == 6
expected = [v for tangent,sign in zip(values, signs) for v in (*addon.blender_to_unity(tangent),sign)]
assert bits(native_result['tangents']) == bits(expected)

# Joined external geometry often inherits zero-filled point attributes.
# Preserve usable source points; generate only those without a valid frame.
mixed = make_object('Mixed')
mixed_values = [values[i] if i in (1,2) else (0,0,0) for i in range(6)]
mixed_signs = [signs[i] if i in (1,2) else 0 for i in range(6)]
addon.set_point_attribute(mixed.data, 'EIEM_Tangent', 'FLOAT_VECTOR', mixed_values, 'vector')
addon.set_point_attribute(mixed.data, 'EIEM_TangentSign', 'FLOAT', mixed_signs, 'value')
mixed_result = export(mixed, 'mixed')
for corner, index in enumerate(corner_indices(mixed_result)):
    source = mixed.data.loops[corner].vertex_index
    actual = mixed_result['tangents'][index*4:index*4+4]
    if source in (1,2):
        assert bits(actual) == bits((*addon.blender_to_unity(values[source]), signs[source]))
    else:
        new_index = indices[corner]
        assert bits(actual) == bits(result['tangents'][new_index*4:new_index*4+4])

# A non-zero imported tangent can still be unusable after a join or a custom
# normal edit. It must be regenerated against the normal that is serialized.
parallel = make_object('ParallelTangent')
addon.set_point_attribute(parallel.data, 'EIEM_Tangent', 'FLOAT_VECTOR',
                          [(0,0,1)] * 6, 'vector')
addon.set_point_attribute(parallel.data, 'EIEM_TangentSign', 'FLOAT', [1] * 6, 'value')
parallel_result = export(parallel, 'parallel')
for index in corner_indices(parallel_result):
    normal = Vector(parallel_result['normals'][index*3:index*3+3])
    tangent = Vector(parallel_result['tangents'][index*4:index*4+3])
    assert abs(normal.dot(tangent)) < 1e-5 and abs(tangent.length-1) < 1e-5

# Generated data uses the final native Blender custom normals, not a backup.
custom = make_object('CustomNormals')
for polygon in custom.data.polygons:
    polygon.use_smooth = True
custom.data.normals_split_custom_set([(0,.6,.8)] * len(custom.data.loops))
custom.data.update()
custom_result = export(custom, 'custom-normal')
for index in corner_indices(custom_result):
    n = Vector(custom_result['normals'][index*3:index*3+3])
    t = Vector(custom_result['tangents'][index*4:index*4+3])
    assert abs(n.dot(t)) < 1e-5 and abs(t.length-1) < 1e-5

# UV-less meshes are legal (untextured); don't invent UV0 or an arbitrary basis.
no_uv = make_object('NoUV', uv=False)
assert export(no_uv, 'no-uv')['tangents'] == []

# Partially missing source data cannot be repaired without the derivative UV.
for layer in list(mixed.data.uv_layers):
    mixed.data.uv_layers.remove(layer)
try:
    addon.write_mesh(output / 'missing-uv.mesh', mixed)
    raise AssertionError('Invented a frame for missing data without UV0')
except ValueError as error:
    assert 'no UV0' in str(error)

# A malformed authored attribute is an error, not an excuse to discard it.
native.data.attributes.remove(native.data.attributes['EIEM_TangentSign'])
try:
    addon.write_mesh(output / 'invalid.mesh', native)
    raise AssertionError('Incomplete source attributes were silently replaced')
except ValueError as error:
    assert 'both be present' in str(error)

print('EIEM_TANGENT_EXPORT_OK')
