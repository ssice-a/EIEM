import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
class BlenderAuthorStateTests(unittest.TestCase):
    def test_auto_controls_persistence_identity_and_material_import(self):
        blender=os.environ.get("EIEM_BLENDER") or shutil.which("blender")
        if not blender:self.skipTest("Requires Blender")
        with tempfile.TemporaryDirectory(prefix="eiem-author-state-") as temp:
            result=subprocess.run([blender,"--background","--factory-startup","--python-exit-code","1","--python",str(ROOT/"tools/Blender/test_eiem_author_state.py"),"--",str(ROOT/"tools/Blender/eiem_blender_addon.py"),temp],capture_output=True,text=True,encoding="utf-8",errors="replace",timeout=90)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertIn("EIEM_AUTHOR_STATE_OK",result.stdout)
