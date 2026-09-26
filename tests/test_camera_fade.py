"""Compile and execute the production camera-fade global configuration."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <windows.h>
#include <sstream>
#include <cstdio>
static int g_guiToggleVK = VK_INSERT, g_modReloadVK = VK_F10;
static void Log(const char *, ...) {}
#include "eiem_config.h"
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (false)
int main() {
  EiemGlobalConfig config;
  std::string error;
  CHECK(config.disableCameraFade);
  std::istringstream good("[Graphics]\ndisable_camera_fade=true\n[Hotkeys]\nreload=F8\n");
  CHECK(EiemParseGlobalConfig(good, &config, error));
  CHECK(config.disableCameraFade && config.reload.vk == VK_F8);
  for (const char *bad : {"[Graphics]\ndisable_camera_fade=yes", "[Graphics]\ndisable_camera_fade=",
    "[Graphics]\ndisable_camera_fade=true\ndisable_camera_fade=false", "[Graphics]\nunknown=true",
    "[Graphics]\n[Graphics]", "[Hotkeys]\ndisable_camera_fade=true"}) {
    std::istringstream input(bad);
    CHECK(!EiemParseGlobalConfig(input, &config, error));
    CHECK(config.disableCameraFade && config.reload.vk == VK_F8);
  }
  std::istringstream old("[Hotkeys]\nreload=F10\n");
  CHECK(EiemParseGlobalConfig(old, &config, error) && config.disableCameraFade);
  CHECK(LoadEiemConfig());
  CHECK(EiemGetGlobalConfig().disableCameraFade);
  { std::ofstream f(kEiemGlobalConfigPath); f << "[Graphics]\ndisable_camera_fade=true\n"; }
  CHECK(LoadEiemConfig() && EiemGetGlobalConfig().disableCameraFade);
  auto generation = s_eiemGlobalConfigGeneration;
  { std::ofstream f(kEiemGlobalConfigPath); f << "[Graphics]\ndisable_camera_fade=typo\n"; }
  CHECK(!LoadEiemConfig() && EiemGetGlobalConfig().disableCameraFade);
  CHECK(s_eiemGlobalConfigGeneration == generation);
  { std::ofstream f(kEiemGlobalConfigPath); f << "[Graphics]\ndisable_camera_fade=false\n"; }
  CHECK(LoadEiemConfig() && !EiemGetGlobalConfig().disableCameraFade);
  return 0;
}
'''


class CameraFadeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("cl"):
            raise unittest.SkipTest("Requires MSVC developer environment")
        cls.temp = tempfile.TemporaryDirectory(prefix="eiem-camera-fade-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.folder = Path(cls.temp.name)
        source = cls.folder / "camera_fade.cpp"
        source.write_text(SOURCE, encoding="utf-8")
        cls.exe = cls.folder / "camera_fade.exe"
        result = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                 f"/I{ROOT / 'src'}", str(source), f"/Fe{cls.exe}"],
                                cwd=cls.folder, capture_output=True, text=True, encoding="utf-8", errors="replace")
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def test_real_global_config_publication(self):
        result = subprocess.run([str(self.exe)], cwd=self.folder, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__": unittest.main()
