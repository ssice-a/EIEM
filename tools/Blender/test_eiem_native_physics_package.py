"""Import a normal AnimeStudio package with its embedded native source graph."""
import importlib.util
import sys
from pathlib import Path

import bpy


addon_path, package = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_normal_package_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()
try:
    mesh_count = addon.import_package(package, clean=True, include_physics=True)
    native = addon.physics_authoring.native
    groups = [
        obj for obj in bpy.data.objects
        if native.is_native(obj) and obj.eiem_physics.kind == "NATIVE_GROUP"
    ]
    colliders = [
        obj for obj in bpy.data.objects
        if native.is_native(obj) and obj.eiem_physics.kind == "NATIVE_COLLIDER"
    ]
    rigs = [obj for obj in bpy.data.objects if obj.type == "ARMATURE"]
    meshes = [
        obj for obj in bpy.data.objects
        if obj.type == "MESH" and obj.data.get("eiem_section")
    ]
    payload = native.source.read_evidence(package / "physics" / "components.json")
    required = native.source.required_transforms(payload)
    assert len(required) == 192, len(required)
    paths = native.rig_paths(rigs[0])
    assert mesh_count == len(meshes) and mesh_count > 0, (mesh_count, len(meshes))
    assert len(rigs) == 1, len(rigs)
    assert len(groups) == 11, len(groups)
    assert len(colliders) == 27, len(colliders)
    assert all(item["bone"] in paths for item in required)
    assert "GrounderIK" not in paths
    assert all(obj.eiem_physics.rig == rigs[0] for obj in groups + colliders)

    # Every package import owns one navigable collection tree. Importing a
    # second package must not mix its Mesh/LOD objects into the first tree.
    packages = [c for c in bpy.data.collections if c.get("eiem_collection_role") == "PACKAGE"]
    assert len(packages) == 1, [c.name for c in packages]
    first = packages[0]
    roles = {c.get("eiem_collection_role") for c in first.children}
    assert {"MESHES", "SKELETONS", "PHYSICS"} <= roles, roles
    assert all(mesh.name in first.all_objects for mesh in meshes)
    first_objects = set(first.all_objects)
    first_id = first["eiem_import_id"]

    second_count = addon.import_package(package, clean=False, include_physics=False)
    packages = [c for c in bpy.data.collections if c.get("eiem_collection_role") == "PACKAGE"]
    assert len(packages) == 2, [c.name for c in packages]
    second = next(c for c in packages if c.get("eiem_import_id") != first_id)
    assert second_count == len([o for o in second.all_objects if o.type == "MESH" and o.data.get("eiem_section")])
    assert first_objects.isdisjoint(set(second.all_objects))
    assert {c.get("eiem_collection_role") for c in second.children} >= {"MESHES", "SKELETONS"}
    print(
        "EIEM_NORMAL_PACKAGE_PHYSICS_OK "
        f"meshes={mesh_count} rigs={len(rigs)} groups={len(groups)} "
        f"colliders={len(colliders)} required_transforms={len(required)}",
        flush=True,
    )
finally:
    addon.unregister()
