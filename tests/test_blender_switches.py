"""Blender-generated mod is evaluated by the actual DLL INI compiler."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include "eiem_mod_document.h"
#include <cstdio>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (false)
static unsigned mask(const EiemModProgram &p) {
  const auto &r = p.rules[p.standaloneRules[0]];
  unsigned bits = 0;
  for (unsigned i = 0; i < r.partnerCount; ++i) if (r.partners[i][0]) bits |= 1u << i;
  return bits;
}
int main(int argc, char **argv) {
  CHECK(argc == 3);
  EiemModProgram p;
  std::string error;
  CHECK(EiemModParseFile(argv[1], p, &error));
  EiemCompileModProgram(p);
  CHECK(p.standaloneRules.size() == 1);
  const auto &r = p.rules[p.standaloneRules[0]];
  CHECK(EiemModEquals(r.handling, "skip") && !r.hasMesh);
  CHECK(mask(p) == 7); // accessory + always + top
  EiemKeyChord top, accessory;
  CHECK(EiemParseKeyChord("F6", &top) && EiemParseKeyChord("F7", &accessory));
  EiemCycleModKey(p, top); CHECK(mask(p) == 3);
  EiemCycleModKey(p, accessory); CHECK(mask(p) == 2);
  EiemCycleModKey(p, top); CHECK(mask(p) == 10); // variant + always
  EiemCycleModKey(p, accessory); CHECK(mask(p) == 11);
  EiemCycleModKey(p, top); CHECK(mask(p) == 7);
  EiemModProgram off;
  CHECK(EiemModParseFile(argv[2], off, &error));
  EiemCompileModProgram(off);
  CHECK(mask(off) == 3);
  EiemCycleModKey(off, top); CHECK(mask(off) == 11);
  puts("EIEM_BLENDER_DLL_SWITCHES_OK");
}
'''


class BlenderSwitchTests(unittest.TestCase):
    def test_real_separate_save_reload_export_and_native_conditions(self):
        blender = os.environ.get("EIEM_BLENDER") or shutil.which("blender")
        if not blender or not shutil.which("cl"):
            self.skipTest("Requires EIEM_BLENDER and MSVC developer environment")
        with tempfile.TemporaryDirectory(prefix="eiem-author-controls-") as directory:
            folder = Path(directory)
            result = subprocess.run([
                blender, "--background", "--factory-startup", "--python-exit-code", "1",
                "--python", str(ROOT / "tools/Blender/test_eiem_switches.py"), "--",
                str(ROOT / "tools/Blender/eiem_blender_addon.py"), str(folder),
            ], capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=90)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("EIEM_SWITCHES_OK", result.stdout)
            source = folder / "native.cpp"
            source.write_text(SOURCE, encoding="utf-8")
            exe = folder / "native.exe"
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}"],
                                   cwd=folder, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(exe), str(folder / "package/mod.ini"),
                                     str(folder / "default-off/mod.ini")],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
