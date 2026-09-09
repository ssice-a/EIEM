"""Real Blender selection/visibility contract, in a caller-owned temp folder."""
import configparser
import importlib.util
import sys
from pathlib import Path

import bpy

addon_path, output = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_selection_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()


def make_object(name, asset=None):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([(0, 0, 0), (1, 0, 0), (0, 1, 0)], [], [(0, 1, 2)])
    mesh["eiem_section"] = "Mesh" + name
    mesh["eiem_asset"] = asset or name
    mesh["eiem_source"] = "assets/test/" + (asset or name) + ".asset"
    mesh["eiem_coordinate_space"] = "unity-y-up-left-handed"
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj["eiem_render_section"] = "Render" + name
    obj["eiem_author_package"] = str(output / "offline")
    return obj


def read_ini(folder):
    ini = configparser.ConfigParser(interpolation=None)
    ini.optionxform = str
    ini.read(folder / "mod.ini", encoding="utf-8")
    return ini


visible = make_object("Visible")
hidden = make_object("Hidden")
unselected = make_object("Unselected")
sibling = make_object("UnselectedSibling", "Visible")
hidden.hide_render = True
# Hidden and unselected objects must not drag unusable resource dependencies in.
invalid_material = bpy.data.materials.new("Not an EIEM material")
hidden.data.materials.append(invalid_material)
unselected.data.materials.append(invalid_material)
sibling.data.materials.append(invalid_material)
hidden.data.eiem_shape_controls.add().shape = "MissingShape"

bpy.ops.object.select_all(action="DESELECT")
visible.select_set(True)
hidden.select_set(True)
bpy.context.view_layer.objects.active = visible
selected, rigs = addon.selected_eiem_resources()
assert set(selected) == {visible, hidden} and not rigs
plan = addon.plan_switch_export(selected)
assert set(plan["objects"]) == {visible, hidden}
assert plan["hidden"] == {hidden}

package = output / "mixed"
stats = addon.export_package(package)
assert stats == {"meshes": 1, "materials": 0, "textures": 0, "skeletons": 0,
                 "physics": 0, "prefabs": 0}, stats
ini = read_ini(package)
assert set(ini.sections()) == {"MeshVisible", "RenderVisible", "RenderHidden"}
assert dict(ini["RenderHidden"]) == {"asset": "Hidden", "handling": "skip"}
assert ini["RenderVisible"]["mesh"] == "MeshVisible"
assert "handling" not in ini["RenderVisible"]
assert not (package / "materials").exists() and not (package / "textures").exists()

# Permanent hide emits only a rule, even with invalid geometry dependencies.
hide_only = output / "hide-only"
stats = addon.export_package(hide_only, [hidden], [])
assert all(value == 0 for value in stats.values()), stats
assert {p.name for p in hide_only.iterdir()} == {"mod.ini"}
assert dict(read_ini(hide_only)["RenderHidden"]) == {"asset": "Hidden", "handling": "skip"}

# Eye/monitor flags are editing state, not the game-hide action.
visible.hide_set(True)
visible.hide_viewport = True
assert addon.export_package(output / "eye-hidden", [visible], [])["meshes"] == 1
assert "handling" not in read_ini(output / "eye-hidden")["RenderVisible"]
visible.hide_set(False)
visible.hide_viewport = False

# Re-export replaces the generated package, not accumulates old object rules.
addon.export_package(package, [hidden], [])
assert set(read_ini(package).sections()) == {"RenderHidden"}
assert not (package / "meshes").exists()

# A subset of a switch group never auto-selects other source resources/states.
group = addon.create_switch_group("SelectionSwitch", "F6", [visible, unselected])
selected_plan = addon.plan_switch_export([visible])
assert selected_plan["objects"] == [visible]
assert set(selected_plan["bindings"]) == {visible}
assert addon.export_package(output / "switch-subset", [visible], [])["meshes"] == 1
switch_text = (output / "switch-subset/mod.ini").read_text(encoding="utf-8")
assert "Unselected" not in switch_text

# Camera-off is unconditional hide and does not export a dead switch group.
addon.assign_switch_meshes(addon.switch_states(group)[0], [hidden])
hidden_plan = addon.plan_switch_export([hidden])
assert hidden_plan["groups"] == [] and hidden_plan["bindings"] == {}

# Split resources share one source rule. Only selected camera-on parts mount.
part_a = make_object("PartA", "SplitSource")
part_b = make_object("PartB", "SplitSource")
part_b.hide_render = True
split = output / "split"
assert addon.export_package(split, [part_a, part_b], [])["meshes"] == 1
ini = read_ini(split)
assert dict(ini["RenderPartA"]) == {
    "asset": "SplitSource", "handling": "skip", "partner.0": "RenderPartAPart0"}
assert ini["RenderPartAPart0"]["mesh"] == "MeshPartA"
assert "MeshPartB" not in ini and "RenderPartB" not in ini
part_a.hide_render = True
assert addon.export_package(output / "split-hidden", [part_a, part_b], [])["meshes"] == 0
assert dict(read_ini(output / "split-hidden")["RenderPartA"]) == {
    "asset": "SplitSource", "handling": "skip"}

# The native camera property persists without inventing another author flag.
bpy.ops.wm.save_as_mainfile(filepath=str(output / "visibility.blend"))
bpy.ops.wm.open_mainfile(filepath=str(output / "visibility.blend"))
assert bpy.data.objects["Hidden"].hide_render
assert not bpy.data.objects["Visible"].hide_render
addon.unregister()
print("EIEM_SELECTION_EXPORT_OK")
