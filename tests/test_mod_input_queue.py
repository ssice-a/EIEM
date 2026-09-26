"""Exercise the production queue's ordering and hold-button identity."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import function

ROOT = Path(__file__).resolve().parents[1]

SOURCE = r'''
#include <windows.h>
#include <cstdio>
static void Log(const char *, ...) {}
#include "eiem_mods.h"
#include "eiem_mod_update.h"
static SRWLOCK s_eiemInputLock = SRWLOCK_INIT;
static std::vector<EiemModInputEvent> s_eiemPendingInputs;
static void EiemRequestModUpdate(EiemModUpdate, const char *) {}
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d\n",__LINE__); return 1; } } while(false)
'''

MAIN = r'''
int main() {
  EiemModInputEvent slider{};
  slider.generation=1; slider.modPath="a/mod.ini"; slider.directValues=true;
  slider.values["$shape"]=.2; EiemQueueModInput(slider);
  slider.values["$shape"]=.4; EiemQueueModInput(slider);
  CHECK(s_eiemPendingInputs.size()==1 && s_eiemPendingInputs[0].values.at("$shape")==.4);
  EiemModInputEvent key{}; key.generation=1; key.modPath="a/mod.ini"; key.chord={VK_F6,0};
  EiemQueueModInput(key); slider.values["$shape"]=.8; EiemQueueModInput(slider);
  CHECK(s_eiemPendingInputs.size()==3); // never merge across a key edge
  s_eiemPendingInputs.clear();
  key.holdTick=true; key.keySection="KeyIncrease"; key.holdSeconds=.02;
  EiemQueueModInput(key); EiemQueueModInput(key);
  CHECK(s_eiemPendingInputs.size()==1 && s_eiemPendingInputs[0].holdSeconds==.04);
  key.keySection="KeyDecrease"; EiemQueueModInput(key);
  CHECK(s_eiemPendingInputs.size()==2 && s_eiemPendingInputs[1].keySection=="KeyDecrease");
  return 0;
}
'''


class ModInputQueueTests(unittest.TestCase):
    def test_slider_coalescing_preserves_key_order_and_section_identity(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        queue = function(trace, "static void EiemQueueModInput(")
        with tempfile.TemporaryDirectory(prefix="eiem-input-queue-") as tmp:
            folder = Path(tmp); source = folder / "test.cpp"; exe = folder / "test.exe"
            source.write_text(SOURCE + queue + MAIN, encoding="utf-8")
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}"],
                                   cwd=folder, capture_output=True, text=True, errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True, errors="replace")
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
