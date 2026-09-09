"""Package entry for Blender and the Blender Development extension."""

# Blender reads this literal without importing the implementation. Keep it in
# sync with the standalone entry; test_eiem_registration.py checks both.
bl_info = {
    "name": "EIEM Resource Package",
    "author": "EIEM",
    "version": (0, 25, 0),
    "blender": (3, 0, 0),
    "location": "File > Import/Export > EIEM package",
    "category": "Import-Export",
}

from . import eiem_blender_addon


def register():
    eiem_blender_addon.register()


def unregister():
    eiem_blender_addon.unregister()
