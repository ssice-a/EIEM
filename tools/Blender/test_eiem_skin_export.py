"""Exercise the real exporter with weights beyond the source mesh palette."""
import importlib.util
import json
import sys
import zlib
from pathlib import Path

import bpy
from mathutils import Matrix

addon_path, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
spec = importlib.util.spec_from_file_location('eiem_skin_test', addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)

def flat(matrix):
    return [matrix[r][c] for c in range(4) for r in range(4)]

nodes = [('Root', -1, (0, 0, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Unused', 0, (0, 1, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Pelvis', 0, (0, 2, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Pelvis/Foot', 2, (0, -1, 0), (0, 0, 0, 1), (1, 1, 1)),
         ('Root/Accessory', 0, (1, 0, 0), (0, 0, 0, 1), (1, 1, 1))]
rig = addon.make_armature('SkeletonTest', {'coordinate':'unity-y-up-left-handed', 'nodes':nodes}, bpy.context.scene.collection)
by_path = {b['eiem_path']: i for i,b in enumerate(rig.data.bones)}
poses = {n[0]:flat(Matrix.Translation((0,-y,0))) for n,y in zip(nodes, (0,1,2,1,0))}

def mesh(name, paths):
    data=bpy.data.meshes.new(name)
    data.from_pydata([(0,0,0),(1,0,0),(0,1,0)],[],[(0,1,2)])
    data['eiem_section']='Mesh'+name; data['eiem_asset']=name
    obj=bpy.data.objects.new(name,data); bpy.context.scene.collection.objects.link(obj)
    obj.modifiers.new('Skin','ARMATURE').object=rig
    obj['eiem_bone_palette_json']=json.dumps([by_path[p] for p in paths])
    obj['eiem_bone_paths_json']=json.dumps(paths)
    obj['eiem_bone_hashes_json']=json.dumps([zlib.crc32(p.encode()) for p in paths])
    obj['eiem_bindposes_json']=json.dumps([poses[p] for p in paths])
    return obj

obj=mesh('Body',['Root','Root/Unused'])
donor=mesh('OtherPart',['Root','Root/Pelvis','Root/Pelvis/Foot'])
for name in ('Root','Unused','Pelvis','Foot','Accessory'):
    obj.vertex_groups.new(name=name)
obj.vertex_groups['Pelvis'].add([0],1,'REPLACE')
obj.vertex_groups['Foot'].add([1],1,'REPLACE')
obj.vertex_groups['Accessory'].add([2],1,'REPLACE')
before=[[(g.group,g.weight) for g in v.groups] for v in obj.data.vertices]
file=output/'expanded.mesh'
addon.write_mesh(file,obj)
result=addon.read_mesh(file)
assert result['bind_count']==5, result['bind_count']
assert result['bone_paths'][:2]==['Root','Root/Unused']
assert result['bindposes'][:2]==[poses['Root'],poses['Root/Unused']]
for vertex,name in enumerate(('Root/Pelvis','Root/Pelvis/Foot','Root/Accessory')):
    weights,indices=result['skin'][vertex]
    assert weights==[1,0,0,0], (vertex,weights)
    assert result['bone_paths'][indices[0]]==name
assert result['bindposes'][result['bone_paths'].index('Root/Pelvis')]==poses['Root/Pelvis']
assert before==[[(g.group,g.weight) for g in v.groups] for v in obj.data.vertices]
assert json.loads(obj['eiem_bone_palette_json'])==[by_path['Root'],by_path['Root/Unused']]
first=file.read_bytes(); addon.write_mesh(file,obj); assert file.read_bytes()==first

# Donor meshes may have another bind frame. Convert using common original
# slots, rather than recomputing every inverse bind from prefab transforms.
donor_basis=Matrix.Translation((3,4,5)) @ Matrix.Rotation(.3,4,'Z')
donor_paths=json.loads(donor['eiem_bone_paths_json'])
donor['eiem_bindposes_json']=json.dumps([
    flat(Matrix([poses[p][r::4] for r in range(4)]) @ donor_basis) for p in donor_paths])
addon.write_mesh(file,obj)
converted=addon.read_mesh(file)
for p in donor_paths:
    actual=converted['bindposes'][converted['bone_paths'].index(p)]
    assert max(abs(a-b) for a,b in zip(actual,poses[p])) < 1e-5, (p,actual)

# A saved source library survives donor object removal; full paths, not
# mutable armature indices, remain the authoring identity.
rig.data['eiem_source_bindings_json']=json.dumps(addon.shared_skin_bindings(rig))
bpy.data.objects.remove(donor,do_unlink=True)
obj['eiem_bone_palette_json']=json.dumps([999,998])
addon.write_mesh(file,obj)
assert addon.read_mesh(file)['bone_paths']==result['bone_paths']

# The current binary skin record holds four influences. Keep the strongest
# four deterministically, normalise, and never edit the author weights.
for name,weight in zip(('Root','Unused','Pelvis','Foot','Accessory'),(.1,.2,.3,.4,.5)):
    obj.vertex_groups[name].add([0],weight,'REPLACE')
weights_before=[(g.group,g.weight) for g in obj.data.vertices[0].groups]
addon.write_mesh(file,obj)
reduced=addon.read_mesh(file)
weights,indices=reduced['skin'][0]
assert [reduced['bone_paths'][i].split('/')[-1] for i in indices]==['Accessory','Foot','Pelvis','Unused']
assert abs(sum(weights)-1)<1e-6
assert max(abs(a-b) for a,b in zip(weights,[.5/1.4,.4/1.4,.3/1.4,.2/1.4]))<1e-6
assert weights_before==[(g.group,g.weight) for g in obj.data.vertices[0].groups]
first=file.read_bytes()

# A positive weight with no matching bone must fail, not turn into zero skin.
obj.vertex_groups.new(name='MissingBone').add([0],.2,'REPLACE')
try:
    addon.write_mesh(file,obj)
    raise AssertionError('unknown weighted group was silently discarded')
except ValueError as ex:
    assert 'MissingBone' in str(ex), str(ex)
assert file.read_bytes()==first
print('EIEM_SKIN_EXPORT_OK')
