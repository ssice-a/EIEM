"""Run the LOD exporter contract inside Blender when Blender is available."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class BlenderLodExportTests(unittest.TestCase):
    def test_lod_discovery_and_template_replication(self):
        blender = os.environ.get("EIEM_BLENDER") or shutil.which("blender")
        if not blender:
            self.skipTest("Requires EIEM_BLENDER or blender on PATH")
        with tempfile.TemporaryDirectory(prefix="eiem-lod-export-") as directory:
            result = subprocess.run([
                blender, "--background", "--factory-startup", "--python-exit-code", "1",
                "--python", str(ROOT / "tools/Blender/test_eiem_lod_export.py"), "--",
                str(ROOT / "tools/Blender/eiem_blender_addon.py"), directory,
            ], capture_output=True, text=True, encoding="utf-8", errors="replace",
                timeout=120)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("EIEM_LOD_EXPORT_OK", result.stdout)


if __name__ == "__main__":
    unittest.main()
