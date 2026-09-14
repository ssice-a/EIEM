"""Merged sibling Meshes are produced by the real exporter inside Blender."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BlenderMergedMeshTests(unittest.TestCase):
    def test_sibling_parts_merge_into_one_mesh_with_material_slots(self):
        blender = os.environ.get("EIEM_BLENDER") or shutil.which("blender")
        if not blender:
            self.skipTest("Requires EIEM_BLENDER or blender on PATH")
        with tempfile.TemporaryDirectory(prefix="eiem-merge-export-") as directory:
            folder = Path(directory)
            result = subprocess.run([
                blender, "--background", "--factory-startup", "--python-exit-code", "1",
                "--python", str(ROOT / "tools/Blender/test_eiem_merged_mesh.py"), "--",
                str(ROOT / "tools/Blender/eiem_blender_addon.py"), str(folder),
            ], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("EIEM_MERGE_EXPORT_OK", result.stdout)


if __name__ == "__main__":
    unittest.main()
