"""Neutral, factory-startup authoring/material test. No user scene or files."""
import importlib.util
import json
import re
import sys
from pathlib import Path
import bpy

addon_path, output = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_author_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()
mesh = bpy.data.meshes.new("TestMesh")
mesh.from_pydata([(0,0,0),(1,0,0),(0,1,0)], [], [(0,1,2)])
mesh["eiem_section"] = "MeshBody"
mesh["eiem_asset"] = "Body"
obj = bpy.data.objects.new("Body", mesh)
bpy.context.scene.collection.objects.link(obj)
bpy.context.view_layer.objects.active = obj
obj.select_set(True)
obj["eiem_render_asset"] = "Body"
obj["eiem_render_section"] = "RenderBody"
obj["eiem_author_package"] = str(output/"offline")
obj.shape_key_add(name="Basis")
blink = obj.shape_key_add(name="Blink")
added = obj.shape_key_add(name="Inflate")
added.data[0].co.z = .1
added.value = .3
mesh["eiem_blend_shapes_json"] = json.dumps([{"name":"Blink","hash":1,"frames":[{"key":"Blink","source_name":"Blink","weight":100}]}])
declarations, bindings, hotkeys = addon.plan_shape_controls([obj])
assert len(declarations)==1 and "shape.Inflate=" in bindings[obj][0]
assert not any("Blink" in x for x in bindings[obj])
variable = declarations[0][0]
addon.export_package(output/"no-ui",[obj],[])
ini = (output/"no-ui/mod.ini").read_text(encoding="utf-8")
assert "persist " + variable in ini and "[UIMod]" not in ini
added.value=.6
assert abs(addon.plan_shape_controls([obj])[0][0][2]-.6)<1e-6
# An unrelated key/resource cannot shift existing variable identities.
extra = obj.shape_key_add(name="Sleeve")
assert addon.plan_shape_controls([obj])[0][0][0] == variable
copy = obj.copy();copy.data = mesh.copy();copy.name="Independent"
bpy.context.scene.collection.objects.link(copy)
decl, bind, hotkeys = addon.plan_shape_controls([copy, obj])
assert bind[obj][0] == "shape.Inflate="+variable and bind[copy][0] != bind[obj][0]
saved = output/"author.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(saved))
bpy.ops.wm.open_mainfile(filepath=str(saved))
obj=bpy.data.objects["Body"];bpy.context.view_layer.objects.active=obj
assert addon.plan_shape_controls([obj])[1][obj][0] == "shape.Inflate="+variable
obj.data.eiem_shape_controls[0].enabled=False
assert not any("Inflate" in x for x in addon.plan_shape_controls([obj])[1][obj])
obj.data.eiem_shape_controls[0].enabled=True
group=addon.create_switch_group("Wardrobe","F6",[obj])
old_states=addon.switch_states(group);old_values=addon.switch_state_values(group)
switch_var=addon.plan_switch_export([obj])["groups"][0][4]
group.name="Renamed Wardrobe"
assert addon.plan_switch_export([obj])["groups"][0][4] == switch_var
first=old_states[0];group.children.unlink(first);group.children.link(first)
assert first["eiem_state_value"] == old_values[0]
new=addon.add_switch_state(group,"Extra")
assert max(addon.switch_state_values(group))>max(old_values)

def package(name):
    root=output/name;(root/"materials").mkdir(parents=True);(root/"textures").mkdir()
    image=bpy.data.images.new("Fixture",width=2,height=2)
    image.filepath_raw=str(root/"textures/shared.png");image.file_format="PNG";image.save()
    bpy.data.images.remove(image)
    (root/"mod.ini").write_text("[MaterialSame]\npath=materials/shared.mat\n[TextureSame]\npath=textures/shared.png\nlinear=true\n",encoding="utf-8")
    (root/"materials/shared.mat").write_text("format=EIEMMAT\nversion=1\nsource=assets/test/"+name+".mat\nshader=TestShader\nfloat._Gloss=.4\ntexture._BaseMap=TextureSame\n",encoding="utf-8")
    return root/"materials/shared.mat"

a=package("role-a");b=package("role-b")
ma=addon.import_material_file(a,obj);old=dict(ma.items())
mb=addon.import_material_file(b,obj)
assert ma!=mb and obj.active_material==mb and dict(ma.items())==old
assert ma["eiem_source"]!=mb["eiem_source"] and ma["eiem_texture._BaseMap"]!=mb["eiem_texture._BaseMap"]
assert mb["eiem_shader"]=="TestShader" and mb["eiem_float._Gloss"]==".4"
# Unknown formats and missing references leave existing slot and data untouched.
before=(len(bpy.data.materials),len(bpy.data.images));broken=output/"broken.mat"
for text in ("%YAML 1.1\n", "format=EIEMMAT\nversion=1\nsource=assets/test/a.mat\ntexture._BaseMap=TextureMissing\n"):
    broken.write_text(text,encoding="utf-8")
    try: addon.import_material_file(broken,obj);raise AssertionError("bad import accepted")
    except ValueError: pass
    assert obj.active_material==mb and before==(len(bpy.data.materials),len(bpy.data.images))
mb["eiem_float._Gloss"]=".7"
addon.export_package(output/"material-mod",[obj],[])
mat_path=next((output/"material-mod/materials").glob("*.mat"))
text=mat_path.read_text(encoding="utf-8")
assert "source=assets/test/role-b.mat" in text and "float._Gloss=.7" in text
assert "texture._BaseMap=" not in text  # unchanged game texture inherited, not redundantly packaged
# Reimport an authored delta: its existing overrides must not become a native baseline.
delta=addon.import_material_file(mat_path,obj)
payload, images=addon.material_override_payload(delta,{},True)
assert "float._Gloss=.7" in payload
# An explicitly changed texture is packaged once and remains an override when
# reimporting the authored .mat, even if its new baseline has no texture key.
mb["eiem_texture._BaseMap"] = ma["eiem_texture._BaseMap"]
obj.active_material=mb
addon.export_package(output/"texture-mod",[obj],[])
mat_path=next((output/"texture-mod/materials").glob("*.mat"))
assert len(list((output/"texture-mod/textures").glob("*.png")))==1
delta=addon.import_material_file(mat_path,obj)
by_section={str(i.get("eiem_section")):i for i in bpy.data.images if i.get("eiem_section")}
payload,images=addon.material_override_payload(delta,by_section,True)
assert any(v.startswith("texture._BaseMap=") for v in payload) and len(images)==1
# A valid reference whose image decoding fails must also roll back allocation.
bad_package=package("bad-image")
(bad_package.parent.parent/"textures/shared.png").write_bytes(b"not a PNG")
before=(len(bpy.data.materials),len(bpy.data.images));previous=obj.active_material
try: addon.import_material_file(bad_package,obj);raise AssertionError("bad image accepted")
except (RuntimeError,ValueError): pass
assert obj.active_material==previous and before==(len(bpy.data.materials),len(bpy.data.images))
addon.unregister()
print("EIEM_AUTHOR_STATE_OK")
