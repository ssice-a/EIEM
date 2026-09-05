"""Check real Blender discovery, registration and development reload.

Run: blender --background --factory-startup --python-exit-code 1
             --python THIS_FILE -- ADDON_DIRECTORY
Does not import models, change scene data or save preferences.
"""

import sys
from pathlib import Path

import addon_utils
import bpy


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) != 1:
    raise SystemExit("Expected an add-on package directory")
directory = Path(args[0]).resolve()
module_name = directory.name
entry = directory / "__init__.py"

# This is the same metadata reader used by Blender's add-on discovery.
metadata = addon_utils._fake_module(module_name, str(entry))
assert metadata is not None, "Package entry is not discoverable: missing literal bl_info"
sys.path.insert(0, str(directory.parent))


def callbacks(menu):
    return [callback for callback in getattr(menu.draw, "_draw_funcs", ())
            if callback.__module__ == module_name
            or callback.__module__.startswith(module_name + ".")]


scene_objects = tuple(bpy.data.objects)
previous_module = None
for cycle in range(3):
    module = addon_utils.enable(module_name, default_set=False)
    assert module is not None, "Add-on registration failed"
    assert module is not previous_module, "Reload kept a stale package module"
    assert Path(module.eiem_blender_addon.__file__).resolve() == directory / "eiem_blender_addon.py"
    assert metadata.bl_info == module.bl_info == module.eiem_blender_addon.bl_info
    assert bpy.ops.eiem.import_package.get_rna_type() is not None
    assert bpy.ops.eiem.export_package.get_rna_type() is not None
    assert hasattr(bpy.types.Scene, "eiem_switch_active")
    assert len(callbacks(bpy.types.TOPBAR_MT_file_import)) == 1
    assert len(callbacks(bpy.types.TOPBAR_MT_file_export)) == 1
    assert tuple(bpy.data.objects) == scene_objects

    addon_utils.disable(module_name, default_set=False)
    assert not callbacks(bpy.types.TOPBAR_MT_file_import)
    assert not callbacks(bpy.types.TOPBAR_MT_file_export)
    assert not hasattr(bpy.types.Scene, "eiem_switch_active")
    assert tuple(bpy.data.objects) == scene_objects

    # Blender Development disables, purges the package, then enables it again.
    previous_module = module
    for name in list(sys.modules):
        if name == module_name or name.startswith(module_name + "."):
            del sys.modules[name]

print("EIEM_REGISTRATION_OK: discovery; import/export; 3 reload cycles; scene unchanged")
