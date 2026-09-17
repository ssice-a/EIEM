"""New bones through Blender edit, saved project, Mesh palette and Skeleton v2."""
import configparser
import importlib.util
import json
import sys
import zlib
from pathlib import Path

import bpy
from mathutils import Matrix, Quaternion, Vector

addon_path, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
spec=importlib.util.spec_from_file_location('eiem_skeleton_test',addon_path)
addon=importlib.util.module_from_spec(spec); spec.loader.exec_module(addon); addon.register()

def flat(m): return [m[r][c] for c in range(4) for r in range(4)]
def matrix(record):
    p,q,s=record[2:]
    return Matrix.LocRotScale(Vector(p),Quaternion((q[3],q[0],q[1],q[2])),Vector(s))
def close(a,b): return max(abs(a[r][c]-b[r][c]) for r in range(4) for c in range(4))<1e-5

# Actual offline packages use an empty resource-root path. Source scale must
# survive; Blender bone displays cannot represent it directly.
nodes=[('',-1,(0,0,0),(0,0,0,1),(1,1,1)),
       ('Rig',0,(.3,1,.2),(0,0,.6,.8),(2,2,2)),
       ('Rig/Unused',1,(0,1,0),(0,0,0,1),(1,1,1))]
rig=addon.make_armature('SkeletonShared',{'coordinate':'unity-y-up-left-handed','nodes':nodes},bpy.context.scene.collection)
root_pose=flat(Matrix.Identity(4)); unused_pose=flat((matrix(nodes[1])@matrix(nodes[2])).inverted())
obj_names=[]
def mesh(name):
    data=bpy.data.meshes.new(name); data.from_pydata([(0,0,0),(1,0,0),(0,1,0)],[],[(0,1,2)])
    data['eiem_section']='Mesh'+name; data['eiem_asset']=name; data['eiem_source']='assets/test/'+name+'.asset'
    data['eiem_coordinate_space']='unity-y-up-left-handed'
    obj=bpy.data.objects.new(name,data); bpy.context.scene.collection.objects.link(obj)
    obj.modifiers.new('Skin','ARMATURE').object=rig; obj['eiem_render_section']='Render'+name
    obj['eiem_bone_paths_json']=json.dumps(['','Rig/Unused']); obj['eiem_bone_palette_json']=json.dumps([0,2])
    obj['eiem_bindposes_json']=json.dumps([root_pose,unused_pose]); obj['eiem_bone_hashes_json']=json.dumps([0,zlib.crc32(b'Rig/Unused')])
    for name in ('root','Unused','Extra','Tip'): obj.vertex_groups.new(name=name)
    obj.vertex_groups['Extra'].add([0,1],1,'REPLACE'); obj.vertex_groups['Tip'].add([2],1,'REPLACE')
    obj_names.append(obj.name); return obj

# Create real new bones, not copies of a source bone's identity properties.
bpy.context.view_layer.objects.active=rig; rig.select_set(True); bpy.ops.object.mode_set(mode='EDIT')
parent=rig.data.edit_bones['Rig']
extra=rig.data.edit_bones.new('Extra'); extra.head=(.4,.2,1.5); extra.tail=(.6,.5,1.8); extra.roll=.3; extra.parent=parent
tip=rig.data.edit_bones.new('Tip'); tip.head=extra.tail; tip.tail=(.7,.8,2.1); tip.roll=-.2; tip.parent=extra
bpy.ops.object.mode_set(mode='OBJECT')
first,second=mesh('First'),mesh('Second')
records=addon.skeleton_author_nodes(rig)
assert [source for b,r,source in records]==[True,True,True,False,False]
assert [r[0] for b,r,s in records][-2:]==['Rig/Extra','Rig/Extra/Tip']
native_world=[]; basis=addon.unity_transform_matrix_to_blender_basis()
for bone,r,source in records:
    world=matrix(r) if r[1]<0 else native_world[r[1]]@matrix(r)
    native_world.append(world)
    if not source: assert close(basis@world@basis.inverted(),bone.matrix_local),bone.name

new_file=output/'test.mesh'; addon.write_mesh(new_file,first); result=addon.read_mesh(new_file)
assert result['bone_paths'][:2]==['','Rig/Unused']
assert result['bindposes'][:2]==[root_pose,unused_pose] # original zero-weight slots are not pruned
for vertex,path in enumerate(('Rig/Extra','Rig/Extra','Rig/Extra/Tip')):
    weights,indices=result['skin'][vertex]; assert weights==[1,0,0,0]
    assert result['bone_paths'][indices[0]]==path
    index=next(i for i,(bone,r,source) in enumerate(records) if r[0]==path)
    bind=Matrix([result['bindposes'][indices[0]][r::4] for r in range(4)])
    assert close(native_world[index]@bind,Matrix.Identity(4)) # bind frame is coherent

try:
    addon.export_package(output/'missing-rig',[first],[])
    raise AssertionError('new bone exported without its Skeleton dependency')
except ValueError as ex: assert '同时选择共享骨架' in str(ex),str(ex)
assert not (output/'missing-rig').exists()

# Explicit Mesh-only export skips author Skeleton/Physics dependency checks but
# still writes the mesh's existing skin payload for runtime deformation.
mesh_only = output/'mesh-only-explicit'
mesh_only_stats = addon.export_package(
    mesh_only, [first], [], physics_objects=[], mesh_only=True)
assert mesh_only_stats['meshes'] == 1
assert mesh_only_stats['skeletons'] == 0 and mesh_only_stats['physics'] == 0
mesh_only_ini = configparser.ConfigParser(interpolation=None)
mesh_only_ini.read(mesh_only/'mod.ini', encoding='utf-8')
assert 'skeleton' not in mesh_only_ini['RenderFirst']
assert 'physics' not in mesh_only_ini['RenderFirst']

# Other selected Meshes may share an author Rig that contains unrelated new
# bones. They do not own a Skeleton dependency unless positive weights use one.
native_only=mesh('NativeOnly')
obj_names.remove(native_only.name)
native_only.vertex_groups.remove(native_only.vertex_groups['Extra'])
native_only.vertex_groups.remove(native_only.vertex_groups['Tip'])
native_only.vertex_groups['Unused'].add([0,1,2],1,'REPLACE')
native_package=output/'native-only'
native_stats=addon.export_package(native_package,[native_only],[])
assert native_stats['meshes']==1 and native_stats['skeletons']==0,native_stats
native_ini=configparser.ConfigParser(interpolation=None)
native_ini.read(native_package/'mod.ini',encoding='utf-8')
assert 'skeleton' not in native_ini['RenderNativeOnly']

package=output/'package'; stats=addon.export_package(package,[first,second],[rig])
assert stats['meshes']==2 and stats['skeletons']==1 and stats['physics']==0,stats
ini=configparser.ConfigParser(interpolation=None); ini.read(package/'mod.ini',encoding='utf-8')
assert ini['RenderFirst']['skeleton']==ini['RenderSecond']['skeleton']=='SkeletonShared'
skel_file=package/ini['SkeletonShared']['path']; original_skeleton=skel_file.read_bytes()
payload=addon.read_skeleton(skel_file); assert payload['version']==2 and payload['source_nodes']==[True,True,True,False,False]
assert list(payload['nodes'][:3])==nodes or all(close(matrix(a),matrix(b)) for a,b in zip(payload['nodes'][:3],nodes))

# Selecting a v1 group alongside Meshes emits one Physics resource for their
# shared Rig. The Rig dependency is automatic, and every same-Rig Render action
# receives the same physics= binding.
physics=addon.physics_authoring
group=physics.create_group(rig,[rig.data.bones['Extra'],rig.data.bones['Tip']],'新增尾链')
group.eiem_physics.gravity=6.25

# A split source with one shared Rig is merged into one Mesh on the source
# Renderer. Skeleton and Physics remain model-level dependencies on that action.
split_part=second.copy(); split_part.data=second.data.copy()
bpy.context.scene.collection.objects.link(split_part)
split_part.name='SplitPart'
split_part.data['eiem_section']='MeshSplitPart'
split_part.data['eiem_source']=first.data['eiem_source']
split_part.data['eiem_asset']=first.data['eiem_asset']
split_part['eiem_render_section']='RenderSplitPart'
split_package=output/'shared-merged'; stats=addon.export_package(
    split_package,[first,split_part],[rig])
assert stats['meshes']==1 and stats['skeletons']==1 and stats['physics']==1,stats
split_ini=configparser.ConfigParser(interpolation=None)
split_ini.read(split_package/'mod.ini',encoding='utf-8')
split_root=split_ini['RenderFirst']
assert split_root['skeleton']=='SkeletonShared'
split_physics=next(name for name in split_ini.sections()
                   if name.startswith('Physics'))
assert split_root['physics']==split_physics
assert 'mesh' in split_root
assert not any(key.startswith('partner.') for key in split_root)
bpy.data.objects.remove(split_part,do_unlink=True)

# A Mesh that has positive weights on an authored Physics group's bones owns
# that dependency even when the helper Empty was not manually selected. This
# prevents a normal Mesh re-export from silently deleting its working Physics.
inferred=output/'inferred-physics'
stats=addon.export_package(inferred,[first],[rig])
assert stats['physics']==1,stats
inferred_ini=configparser.ConfigParser(interpolation=None)
inferred_ini.read(inferred/'mod.ini',encoding='utf-8')
assert 'physics' in inferred_ini['RenderFirst']

bpy.ops.object.select_all(action='DESELECT')
for selected in (first,second,group): selected.select_set(True)
bpy.context.view_layer.objects.active=first
combined=output/'combined'; stats=addon.export_package(combined)
assert stats['meshes']==2 and stats['skeletons']==1 and stats['physics']==1,stats
ini=configparser.ConfigParser(interpolation=None); ini.read(combined/'mod.ini',encoding='utf-8')
physics_sections=[name for name in ini.sections() if name.startswith('Physics')]
assert len(physics_sections)==1,physics_sections
physics_section=physics_sections[0]
assert ini['RenderFirst']['physics']==ini['RenderSecond']['physics']==physics_section
document=physics.document.read(combined/ini[physics_section]['path'])
assert document['colliders']==[] and document['groups'][0]['parameters']['gravity']==6.25
physics_file=combined/ini[physics_section]['path']
physics_skeleton=(physics_file.parent/document['skeleton']).resolve()
render_skeleton=(combined/ini[ini['RenderFirst']['skeleton']]['path']).resolve()
assert physics_skeleton==render_skeleton and physics_skeleton.is_file()

# Authoring colliders are emitted into the combined package. The runtime uses
# one component per author collider and shares that component across group refs.
collider=physics.create_collider(group,rig.data.bones['Rig'],'CAPSULE','测试碰撞体')
with_collider=output/'with-collider'
stats=addon.export_package(with_collider,[first],[rig],[group])
assert stats['physics']==1,stats
collider_ini=configparser.ConfigParser(interpolation=None)
collider_ini.read(with_collider/'mod.ini',encoding='utf-8')
collider_section=collider_ini['RenderFirst']['physics']
collider_document=physics.document.read(with_collider/collider_ini[collider_section]['path'])
assert len(collider_document['colliders'])==1
assert collider_document['colliders'][0]['shape']=='CAPSULE'
assert collider_document['groups'][0]['colliders']==[collider_document['colliders'][0]['id']]
group.eiem_physics.colliders.clear()
visual=collider.eiem_physics.visual
if visual: bpy.data.objects.remove(visual,do_unlink=True)
bpy.data.objects.remove(collider,do_unlink=True)

# Save/reopen authoring state: no runtime-generated identities are needed.
bpy.ops.wm.save_as_mainfile(filepath=str(output/'author.blend'))
bpy.ops.wm.open_mainfile(filepath=str(output/'author.blend'))
rig=bpy.data.objects['SkeletonShared']; first,second=[bpy.data.objects[n] for n in obj_names]
addon.export_package(package,[first,second],[rig])
reloaded_ini=configparser.ConfigParser(interpolation=None)
reloaded_ini.read(package/'mod.ini',encoding='utf-8')
reloaded_skeleton=package/reloaded_ini[reloaded_ini['RenderFirst']['skeleton']]['path']
assert reloaded_skeleton.read_bytes()==original_skeleton
assert (package/'meshes/MeshFirst.mesh').read_bytes()==new_file.read_bytes()

# Two target resources may use author rigs whose copied section names collide.
# Keep their files and Render associations distinct.
copy_rig=rig.copy(); copy_rig.data=rig.data.copy(); bpy.context.scene.collection.objects.link(copy_rig)
copy_mesh=first.copy(); copy_mesh.data=first.data.copy(); bpy.context.scene.collection.objects.link(copy_mesh)
copy_mesh.modifiers[0].object=copy_rig
copy_mesh.name='FirstCopy'; copy_mesh.data['eiem_section']='MeshFirstCopy'
copy_mesh.data['eiem_source']='assets/test/FirstCopy.asset'
copy_mesh.data['eiem_asset']='FirstCopy'
copy_mesh['eiem_render_section']='RenderFirstCopy'
bpy.context.view_layer.objects.active=copy_rig; bpy.ops.object.mode_set(mode='EDIT')
copy_rig.data.edit_bones['Extra'].head.x+=.2
bpy.ops.object.mode_set(mode='OBJECT')
duplicate_package=output/'duplicate-rigs'
stats=addon.export_package(duplicate_package,[first,copy_mesh],[rig,copy_rig])
assert stats['meshes']==2 and stats['skeletons']==2,stats
ini=configparser.ConfigParser(interpolation=None); ini.read(duplicate_package/'mod.ini',encoding='utf-8')
bindings=[ini[s] for s in ini.sections() if 'skeleton' in ini[s]]
assert len(bindings)==2 and bindings[0]['skeleton']!=bindings[1]['skeleton']
assert bindings[0]['mesh']!=bindings[1]['mesh']
mesh_files=[duplicate_package/ini[b['mesh']]['path'] for b in bindings]
assert mesh_files[0].read_bytes()!=mesh_files[1].read_bytes()
bpy.data.objects.remove(copy_mesh,do_unlink=True); bpy.data.objects.remove(copy_rig,do_unlink=True)

# Source edits are not silently ignored, and cannot damage an existing export.
bpy.context.view_layer.objects.active=rig; bpy.ops.object.mode_set(mode='EDIT')
rig.data.edit_bones['Rig'].head.x+=.1
bpy.ops.object.mode_set(mode='OBJECT')
try:
    addon.export_package(package,[first],[rig])
    raise AssertionError('source rest edit was silently ignored')
except ValueError as ex: assert '源骨骼绑定姿态已改变' in str(ex),str(ex)
assert reloaded_skeleton.read_bytes()==original_skeleton
addon.unregister()
print('EIEM_SKELETON_EXPORT_OK')
