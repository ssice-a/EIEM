"""Merge sibling parts into one Mesh with one submesh per material slot.

Runs inside real Blender. Writer and reader are the exporter's own, so this
checks the artifact the runtime will actually load rather than a reimplementation
of it.

The case that matters is the one that motivated the merge: parts that address
different subsets of one shared skeleton, each with its own material, which must
end up as one Mesh whose submesh N is part N's material slot.
"""
import importlib.util
import json
import sys
import zlib
from pathlib import Path

import bpy
from mathutils import Matrix

addon_path, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
spec = importlib.util.spec_from_file_location('eiem_merge_test', addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)


def flat(matrix):
    return [matrix[r][c] for c in range(4) for r in range(4)]


nodes = [('Root', -1, (0, 0, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Pelvis', 0, (0, 1, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Pelvis/Foot', 1, (0, 1, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Cloth', 0, (1, 0, 0), (0, 0, 0, 1), (1, 1, 1))]
rig = addon.make_armature('SkeletonMerge', {'coordinate': 'unity-y-up-left-handed',
                                            'nodes': nodes},
                          bpy.context.scene.collection)
by_path = {b['eiem_path']: i for i, b in enumerate(rig.data.bones)}
poses = {n[0]: flat(Matrix.Translation((0, -y, 0))) for n, y in zip(nodes, (0, 1, 2, 0))}


def material(name):
    mat = bpy.data.materials.new(name)
    mat['eiem_section'] = name
    mat['eiem_source'] = 'assets/test/%s.mat' % name
    return mat


def part(name, paths, slot_materials, vertices, triangles):
    data = bpy.data.meshes.new(name)
    data.from_pydata(vertices, [], triangles)
    data['eiem_section'] = 'Mesh' + name
    data['eiem_asset'] = 'SharedAsset'
    data['eiem_source'] = 'assets/test/shared.asset'
    data['eiem_target_path'] = 'assets/test/shared.asset'
    data['eiem_target_asset'] = 'SharedAsset'
    data['eiem_coordinate_space'] = 'unity-y-up-left-handed'
    for mat in slot_materials:
        data.materials.append(mat)
    obj = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(obj)
    obj['eiem_render_section'] = 'RenderShared'
    obj['eiem_render_asset'] = 'SharedAsset'
    obj.modifiers.new('Skin', 'ARMATURE').object = rig
    obj['eiem_bone_palette_json'] = json.dumps([by_path[p] for p in paths])
    obj['eiem_bone_paths_json'] = json.dumps(paths)
    obj['eiem_bone_hashes_json'] = json.dumps([zlib.crc32(p.encode()) for p in paths])
    obj['eiem_bindposes_json'] = json.dumps([poses[p] for p in paths])
    for path in paths:
        if path.split('/')[-1] not in obj.vertex_groups:
            obj.vertex_groups.new(name=path.split('/')[-1])
    # Every vertex needs a positive weight on a real bone: the exporter refuses
    # unweighted vertices rather than letting them become zero skin.
    for index in range(len(vertices)):
        path = paths[index % len(paths)]
        obj.vertex_groups[path.split('/')[-1]].add([index], 1, 'REPLACE')
    return obj


mat_a = material('MaterialA')
mat_b = material('MaterialB')
mat_c = material('MaterialC')
# Part A: two bones, two material slots, and a face in each slot so both
# submeshes carry geometry.
first = part('PartA', ['Root', 'Root/Pelvis'], [mat_a, mat_b],
             [(0, 0, 0), (1, 0, 0), (0, 1, 0), (1, 1, 0)],
             [(0, 1, 2), (1, 3, 2)])
first.data.polygons[1].material_index = 1
# Part B: a different subset of the same skeleton, one slot.
second = part('PartB', ['Root', 'Root/Cloth'], [mat_c],
             [(2, 0, 0), (3, 0, 0), (2, 1, 0)], [(0, 1, 2)])

# These slots are unchanged from the source Render.  They still need explicit
# material resources once the parts are folded into one new global slot layout.
first['eiem_original_material_sections_json'] = json.dumps(
    {'0': 'MaterialA', '1': 'MaterialB'})
second['eiem_original_material_sections_json'] = json.dumps({'0': 'MaterialC'})
identity = addon.mesh_source_identity(first)
assert addon.merged_source_keys([first, second], {'hidden': set()}) == {identity}
assert addon.material_override_payload(mat_a, {}, False)[0] is None
assert addon.material_override_payload(mat_a, {}, True)[0] is not None

file = output / 'merged.mesh'
stats = addon.write_merged_mesh(file, [first, second])
result = addon.read_mesh(file)

assert stats['parts'] == 2, stats
assert stats['slots'] == [2, 1], stats
assert result['name'] == 'SharedAsset', result['name']
# Vertices are concatenated, never welded.
assert result['vertex_count'] == 7, result['vertex_count']
# One submesh per material slot, parts in member order: A0, A1, B0.
assert result['submesh_count'] == 3, result['submesh_count']
assert len(result['submeshes']) == 3, result['submeshes']
assert result['index_count'] == 9, result['index_count']

# The palette is the ordered union, seeded by the widest part.
assert result['bone_paths'] == ['Root', 'Root/Pelvis', 'Root/Cloth'], result['bone_paths']
assert result['bind_count'] == 3, result['bind_count']
assert result['bindposes'][0] == poses['Root']
assert result['bindposes'][1] == poses['Root/Pelvis']
assert result['bindposes'][2] == poses['Root/Cloth']

# Part-local vertex indices are shifted, never renumbered, and each part's
# joints are remapped into the union palette.
second_base = 4
assert len(result['skin']) == 7, len(result['skin'])
for vertex in range(second_base):
    weights, indices = result['skin'][vertex]
    assert weights == [1, 0, 0, 0], (vertex, weights)
    assert result['bone_paths'][indices[0]] in ('Root', 'Root/Pelvis'), (vertex, indices)
for vertex in range(second_base, 7):
    weights, indices = result['skin'][vertex]
    assert weights == [1, 0, 0, 0], (vertex, weights)
    assert result['bone_paths'][indices[0]] in ('Root', 'Root/Cloth'), (vertex, indices)

# Slots must be merged-space: submesh 2 belongs to part B, and its indices must
# address part B's own vertices, not part A's.
expected_bases = [0, 0, second_base]
expected_counts = [3, 3, 3]
for slot, submesh in enumerate(result['submeshes']):
    topology, start, count, base, first_vertex, vertex_count = submesh
    assert topology == 0, submesh
    assert count == expected_counts[slot], (slot, submesh)
    part_base = expected_bases[slot]
    span = 4 if slot < 2 else 3
    assert first_vertex >= part_base, (slot, submesh)
    assert first_vertex + vertex_count <= part_base + span, (slot, submesh)
    for index in result['indices'][start:start + count]:
        assert part_base <= index < part_base + span, (slot, index)

# Writing again must be byte-identical, so an unchanged export is a no-op.
before = file.read_bytes()
addon.write_merged_mesh(file, [first, second])
assert file.read_bytes() == before

# A part authored in another coordinate space cannot share one vertex buffer,
# and must fail loudly instead of being silently reinterpreted.
foreign = part('PartForeign', ['Root'], [mat_c],
               [(4, 0, 0), (5, 0, 0), (4, 1, 0)], [(0, 1, 2)])
foreign.data['eiem_coordinate_space'] = 'z-up-right-handed'
try:
    addon.write_merged_mesh(output / 'foreign.mesh', [first, foreign])
    raise AssertionError('mixed coordinate spaces must be rejected')
except ValueError as ex:
    assert '坐标系' in str(ex), str(ex)

print('EIEM_MERGE_EXPORT_OK')
