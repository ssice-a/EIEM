"""Source-aware channel regression, in a separate background Blender process.

Run with -- ADDON PROBE_OUTPUT, after tests/EndfieldTangentProbe.
Checks real-parser arrays -> production EIEM file -> Blender -> output.
This is NOT an animation, runtime GPU, or full skeleton round-trip test.
"""
import importlib.util
import json
import struct
import sys
import tempfile
from pathlib import Path

import bpy

addon_path, package = map(Path, sys.argv[sys.argv.index("--") + 1:])
spec = importlib.util.spec_from_file_location("eiem_tangent_test", addon_path)
addon = importlib.util.module_from_spec(spec)
spec.loader.exec_module(addon)
# System.Text.Json writes float32 negative zero as -0. Python's ordinary int
# parser loses its sign; keep it when comparing the original float32 bits.
sources = json.loads((package / "source.json").read_text(encoding="utf-8"),
                     parse_int=lambda value: -0.0 if value == "-0" else int(value))


def float32_bytes(values):
    values = values or []
    return struct.pack("<%df" % len(values), *values)


def same(actual, expected, label):
    if float32_bytes(actual) != float32_bytes(expected):
        mismatches = [(i, a, b, float32_bytes([a]).hex(), float32_bytes([b]).hex())
                      for i, (a, b) in enumerate(zip(actual or [], expected or []))
                      if float32_bytes([a]) != float32_bytes([b])]
        raise AssertionError((label, len(actual or []), len(expected or []), mismatches[:3]))


tested = 0
vertices = 0
with tempfile.TemporaryDirectory(prefix="eiem-source-tangent-") as temp:
    for source in sources:
        payload = addon.read_mesh(package / source["file"])
        for channel in ("normals", "tangents", "colors"):
            same(payload[channel], source[channel], (source["name"], "writer", channel))
        for index, values in enumerate(source["uvs"]):
            same(payload["uvs"][index], values, (source["name"], "writer", index))
        # A missing source tangent is a failure, not an empty-array pass.
        assert len(payload["tangents"]) == 4 * source["count"], source["name"]
        mesh = addon.make_mesh("TangentProbe", payload)
        obj = bpy.data.objects.new("TangentProbe", mesh)
        bpy.context.scene.collection.objects.link(obj)
        assert mesh.has_custom_normals and all(p.use_smooth for p in mesh.polygons)
        target = Path(temp) / source["file"]
        addon.write_mesh(target, obj)
        result = addon.read_mesh(target)
        for channel in ("normals", "tangents", "colors"):
            same(result[channel], payload[channel], (source["name"], "Blender", channel))
        for index, values in enumerate(payload["uvs"]):
            same(result["uvs"][index], values, (source["name"], "Blender", index))
        tested += 1
        vertices += source["count"]
        bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.meshes.remove(mesh)
assert tested > 0
print(f"EIEM_SOURCE_BLENDER_TANGENTS_OK meshes={tested} vertices={vertices} float32_exact=True")
