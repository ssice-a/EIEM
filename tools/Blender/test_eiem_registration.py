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
    assert bpy.ops.eiem.import_material.get_rna_type() is not None
    assert bpy.ops.eiem.import_physics.get_rna_type() is not None
    assert bpy.ops.eiem.export_physics.get_rna_type() is not None
    assert bpy.ops.eiem.organize_physics.get_rna_type() is not None
    assert bpy.ops.eiem.physics_parameters.get_rna_type() is not None
    assert bpy.ops.eiem.switch_key_record.get_rna_type() is not None
    assert hasattr(bpy.types, "OBJECT_PT_eiem_physics")
    assert hasattr(bpy.types, "EIEM_MT_physics_create")
    assert not hasattr(bpy.types, "EIEM_MT_physics_copy")
    assert hasattr(bpy.types.Object, "eiem_physics")
    assert hasattr(bpy.types.Object, "eiem_native_physics")
    assert hasattr(bpy.types.Scene, "eiem_physics_visibility")
    assert hasattr(bpy.types.Scene, "eiem_physics_preview_style")
    assert hasattr(bpy.types.Scene, "eiem_physics_xray")
    handlers = [handler for handler in bpy.app.handlers.depsgraph_update_post
                if handler.__module__ == module_name + ".eiem_physics_authoring"]
    assert len(handlers) == 1
    native_handlers = [handler for handler in bpy.app.handlers.depsgraph_update_post
                       if handler.__module__ == module_name + ".eiem_physics_native"]
    assert not native_handlers
    probe = bpy.data.objects.new("Physics Current Group Probe", None)
    bpy.context.scene.collection.objects.link(probe)
    probe.eiem_physics.kind = "GROUP"
    probe.select_set(True)
    bpy.context.view_layer.objects.active = probe
    # Background Blender does not dispatch the UI notifier queue. Exercise the
    # real message-bus callback directly; repeated register/unregister cycles
    # above still validate subscription cleanup.
    module.eiem_blender_addon.physics_authoring.active_object_updated()
    assert bpy.context.scene.eiem_physics_group == probe
    bpy.data.objects.remove(probe, do_unlink=True)
    assert hasattr(bpy.types.Scene, "eiem_switch_active")
    assert len(callbacks(bpy.types.TOPBAR_MT_file_import)) == 1
    assert len(callbacks(bpy.types.TOPBAR_MT_file_export)) == 1
    assert tuple(bpy.data.objects) == scene_objects

    addon_utils.disable(module_name, default_set=False)
    assert not callbacks(bpy.types.TOPBAR_MT_file_import)
    assert not callbacks(bpy.types.TOPBAR_MT_file_export)
    assert not hasattr(bpy.types.Scene, "eiem_switch_active")
    assert not hasattr(bpy.types.Object, "eiem_physics")
    assert not hasattr(bpy.types.Scene, "eiem_physics_group")
    assert not hasattr(bpy.types.Scene, "eiem_physics_visibility")
    assert not hasattr(bpy.types.Scene, "eiem_physics_preview_style")
    assert not hasattr(bpy.types.Scene, "eiem_physics_xray")
    assert not hasattr(bpy.types, "OBJECT_PT_eiem_physics")
    assert not hasattr(bpy.types, "EIEM_MT_physics_create")
    assert not hasattr(bpy.types, "EIEM_MT_physics_copy")
    assert not [handler for handler in bpy.app.handlers.depsgraph_update_post
                if handler.__module__ == module_name + ".eiem_physics_authoring"]
    assert not [handler for handler in bpy.app.handlers.depsgraph_update_post
                if handler.__module__ == module_name + ".eiem_physics_native"]
    assert tuple(bpy.data.objects) == scene_objects

    # Blender Development disables, purges the package, then enables it again.
    previous_module = module
    for name in list(sys.modules):
        if name == module_name or name.startswith(module_name + "."):
            del sys.modules[name]

print("EIEM_REGISTRATION_OK: discovery; import/export; 3 reload cycles; scene unchanged")
