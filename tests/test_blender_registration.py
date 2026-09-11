"""Verify the shipped development package, not only its standalone module."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class BlenderRegistrationTests(unittest.TestCase):
    def test_discovery_and_development_reload(self):
        blender = os.environ.get("EIEM_BLENDER") or shutil.which("blender")
        if not blender:
            self.skipTest("Requires EIEM_BLENDER")
        with tempfile.TemporaryDirectory(prefix="eiem-registration-") as directory:
            package = Path(directory) / "EIEM_Blender"
            package.mkdir()
            for name in ("__init__.py", "eiem_blender_addon.py",
                         "eiem_blender_controls.py", "eiem_physics_authoring.py",
                         "eiem_physics_document.py", "eiem_physics_native.py",
                         "eiem_physics_source.py"):
                shutil.copy2(ROOT / "tools/Blender" / name, package / name)
            result = subprocess.run([
                blender, "--background", "--factory-startup", "--python-exit-code", "1",
                "--python", str(ROOT / "tools/Blender/test_eiem_registration.py"),
                "--", str(package),
            ], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("EIEM_REGISTRATION_OK", result.stdout)


if __name__ == "__main__":
    unittest.main()
