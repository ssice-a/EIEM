"""Blender-background regression for one-Mesh incremental package export."""

import configparser
import importlib.util
import json
import shutil
import sys
import tempfile
from pathlib import Path

import bmesh
import bpy


args = sys.argv[sys.argv.index("--") + 1:]
if len(args) != 4:
    raise SystemExit(
        "usage: blender --background --python test_eiem_incremental_export.py "
        "-- ADDON SOURCE_PACKAGE OUTPUT_PACKAGE REPLACEMENT_PNG"
    )

addon_path, source_root, output_root, replacement_png = map(Path, args)
spec = importlib.util.spec_from_file_location("eiem_blender_addon_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
addon.register()
assert hasattr(bpy.types, "MATERIAL_PT_eiem_properties")
assert hasattr(bpy.types, "DATA_PT_eiem_properties")
assert hasattr(bpy.types, "IMAGE_PT_eiem_properties")

source_root = source_root.resolve()
output_root = output_root.resolve()
replacement_png = replacement_png.resolve()
if output_root.exists():
    shutil.rmtree(output_root)

addon.import_package(source_root, clean=True)
target = next(
    obj for obj in bpy.context.scene.objects
    if obj.type == "MESH" and
    obj.data.get("eiem_section") == "MeshS_actor_typhoea_cloth_01_lod0_2"
)
before_polygons = len(target.data.polygons)
mesh_edit = bmesh.new()
mesh_edit.from_mesh(target.data)
mesh_edit.faces.ensure_lookup_table()
bmesh.ops.delete(
    mesh_edit, geom=list(mesh_edit.faces[:128]), context="FACES",
)
mesh_edit.to_mesh(target.data)
mesh_edit.free()
target.data.update()
assert len(target.data.polygons) < before_polygons

material = target.data.materials[0]
texture_paths, texture_transforms, parameters, resource = \
    addon.material_property_groups(material)
assert texture_paths and all(key.startswith("eiem_texture.")
                             for key in texture_paths), texture_paths
assert not any(key in addon.EIEM_INTERNAL_MATERIAL_PROPERTIES
               for group in (texture_paths, texture_transforms, parameters, resource)
               for key in group)

# Importing the same native texture from another package may allocate a
# suffixed internal section. Equal source bytes still mean that the material
# slot is unchanged and must continue to inherit from the game material.
baseline = json.loads(material["eiem_baseline_json"])
bindings = json.loads(material["eiem_texture_sections_json"])
unchanged_key = next(
    key for key in texture_paths
    if key != "eiem_texture._BaseMap" and key[5:] in baseline
)
unchanged_property = unchanged_key[5:]
original_section = baseline[unchanged_property]
original_image = next(
    image for image in bpy.data.images
    if image.get("eiem_section") == original_section
)
duplicate_root = Path(tempfile.mkdtemp(prefix="eiem_texture_duplicate_"))
duplicate_path = duplicate_root / Path(bpy.path.abspath(original_image.filepath)).name
shutil.copy2(bpy.path.abspath(original_image.filepath), duplicate_path)
duplicate_image = bpy.data.images.load(str(duplicate_path), check_existing=False)
duplicate_section = original_section + "2"
duplicate_image["eiem_section"] = duplicate_section
duplicate_image["eiem_name"] = original_image.get("eiem_name", original_image.name)
duplicate_image["eiem_source"] = original_image.get("eiem_source", "")
duplicate_image["eiem_target_path"] = original_image.get("eiem_target_path", "")
material[unchanged_key] = str(duplicate_path)
bindings[unchanged_property] = duplicate_section
material["eiem_texture_sections_json"] = json.dumps(bindings)

alias_material = bpy.data.materials.new("NativeImportAliasProbe")
alias_material["eiem_source"] = material["eiem_source"]
alias_canonical = addon.texture_section_base(duplicate_path)
alias_material["eiem_baseline_json"] = json.dumps({
    "source": material["eiem_source"],
    "float._UseSpecRampMap": "1",
    unchanged_property: alias_canonical + "2",
})
alias_material["eiem_texture_sections_json"] = json.dumps({
    unchanged_property: alias_canonical,
})
alias_material["eiem_" + unchanged_property] = str(duplicate_path)
alias_values, alias_refs = addon.material_override_payload(
    alias_material, {alias_canonical: duplicate_image}, True)
assert not any(line.startswith(unchanged_property + "=") for line in alias_values)
assert alias_refs == set(), alias_refs
bpy.data.materials.remove(alias_material)

# Extracting the same native Texture from another package can also replace the
# package-local suffix with a resource hash.  The stable game texture name is
# unchanged, so it remains inherited from the cloned game material.
native_alias_path = duplicate_root / "T_actor_test_cloth_N_-123456789.png"
shutil.copy2(duplicate_path, native_alias_path)
native_alias_image = bpy.data.images.load(str(native_alias_path), check_existing=False)
native_alias_section = "TextureT_actor_test_cloth_N__123456789"
native_alias_image["eiem_section"] = native_alias_section
native_alias_material = bpy.data.materials.new("NativeHashAliasProbe")
native_alias_material["eiem_source"] = material["eiem_source"]
native_alias_material["eiem_baseline_json"] = json.dumps({
    "source": material["eiem_source"],
    "float._NormalScale": "1",
    "texture._BumpMap": "TextureT_actor_test_cloth_N_22",
})
native_alias_material["eiem_texture_sections_json"] = json.dumps({
    "texture._BumpMap": native_alias_section,
})
native_alias_material["eiem_texture._BumpMap"] = native_alias_section
native_values, native_refs = addon.material_override_payload(
    native_alias_material, {native_alias_section: native_alias_image}, True)
assert not any(line.startswith("texture._BumpMap=") for line in native_values)
assert native_refs == set(), native_refs
bpy.data.materials.remove(native_alias_material)
bpy.data.images.remove(native_alias_image)

# A legacy scene may already contain TextureTextureBody for body.png while its
# authoring baseline says TextureBody. The file remains an authored override;
# canonicalizing the spelling must not make it disappear.
legacy_image = bpy.data.images.load(str(replacement_png), check_existing=False)
legacy_image["eiem_section"] = "TextureTextureBody"
legacy_image["eiem_name"] = replacement_png.stem
legacy_material = bpy.data.materials.new("LegacyTextureNameProbe")
legacy_material["eiem_source"] = "assets/test/material.mat"
legacy_material["eiem_baseline_json"] = json.dumps({
    "source": "assets/test/material.mat",
    "texture._BaseMap": "TextureBody",
})
legacy_material["eiem_texture_sections_json"] = json.dumps({
    "texture._BaseMap": "TextureTextureBody",
})
legacy_material["eiem_texture._BaseMap"] = str(replacement_png)
legacy_images = {"TextureTextureBody": legacy_image}
legacy_values, legacy_references = addon.material_override_payload(
    legacy_material, legacy_images)
assert "texture._BaseMap=TextureBody" in legacy_values, legacy_values
assert legacy_references == {"TextureBody"}, legacy_references
bpy.data.materials.remove(legacy_material)

# A newly assigned normal map is vector data.  Legacy source packages did not
# record Texture2D.linear, so their inherited false default must not turn the
# replacement into an sRGB texture.
normal_path = duplicate_root / "normal_probe.png"
shutil.copy2(replacement_png, normal_path)
normal_material = bpy.data.materials.new("LinearNormalProbe")
normal_images = {}
normal_section = addon.resolve_material_texture(
    normal_material, "texture._BumpMap", str(normal_path), normal_images, {})
assert normal_images[normal_section]["eiem_linear"] == "true"
bpy.data.images.remove(normal_images[normal_section])
bpy.data.materials.remove(normal_material)

material["eiem_texture._BaseMap"] = str(replacement_png)
bpy.ops.object.select_all(action="DESELECT")
target.select_set(True)
bpy.context.view_layer.objects.active = target
stats = addon.export_package(output_root)
assert stats == {
    "meshes": 1, "skeletons": 0, "physics": 0, "materials": 1, "textures": 1,
    "prefabs": 0,
}, stats

parser = configparser.ConfigParser(interpolation=None, strict=False)
parser.optionxform = str
with (output_root / "mod.ini").open("r", encoding="utf-8-sig") as stream:
    parser.read_file(stream)
sections = parser.sections()
assert sum(name.startswith("Mesh") for name in sections) == 1, sections
assert sum(name.startswith("Prefab") for name in sections) == 0, sections
assert sum(name.startswith("Render") for name in sections) == 1, sections
assert sum(name.startswith("Material") for name in sections) == 1, sections
assert sum(name.startswith("Texture") for name in sections) == 1, sections
assert sum(name.startswith("Skeleton") for name in sections) == 0, sections
mesh_section = next(name for name in sections if name.startswith("Mesh"))
assert parser[mesh_section]["target.path"] == parser[mesh_section]["source"]
assert parser[mesh_section]["target.asset"] == parser[mesh_section]["asset"]
render_section = next(name for name in sections if name.startswith("Render"))
assert parser[render_section]["asset"] == parser[mesh_section]["target.asset"]
assert parser[render_section]["mesh"] == mesh_section

material_section = next(name for name in sections if name.startswith("Material"))
assert parser[material_section]["target.path"] == material["eiem_source"]
assert parser[material_section]["target.asset"] == material["eiem_name"]

material_path = output_root / parser[material["eiem_section"]]["path"]
properties = addon.read_flat_properties(material_path)
expected_texture_section = addon.texture_section_base(replacement_png)
assert properties["texture._BaseMap"] == expected_texture_section, properties
texture_section = next(name for name in sections if name.startswith("Texture"))
assert texture_section == expected_texture_section, texture_section
assert Path(parser[texture_section]["path"]).name == replacement_png.name
assert (output_root / parser[texture_section]["path"]).is_file()
assert not any(
    key.startswith("texture.") and key != "texture._BaseMap"
    for key in properties
), properties
assert not any(path.name == duplicate_path.name
               for path in (output_root / "textures").iterdir()
               if path.name != replacement_png.name)
shutil.rmtree(duplicate_root)
addon.unregister()
print("EIEM_INCREMENTAL_EXPORT_OK", stats)
