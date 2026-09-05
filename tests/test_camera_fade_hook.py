"""Real MinHook + simulated CameraMono evaluations; not a gameplay visual test."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <vector>
static int g_guiToggleVK = VK_INSERT, g_modReloadVK = VK_F10;
static std::atomic_bool g_pluginActive{true};
void Log(const char *, ...) {}
#include "eiem_camera_fade.h"
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (false)
using CameraStep = void (*)(void *, void *);
struct Camera {
  bool shouldFade = true, faded = false, skip = true, manualEffect = true;
  bool throwOnPitch = false;
  unsigned pitchCalls = 0, clearCalls = 0;
  void *pitchInfo = nullptr, *clearInfo = nullptr;
  std::vector<int> order;
};
__declspec(noinline) static void NativePitch(void *object, void *info) {
  auto &camera = *(Camera *)object;
  camera.pitchInfo = info;
  ++camera.pitchCalls;
  camera.order.push_back(1);
  if (camera.throwOnPitch) throw std::runtime_error("original evaluation failed");
  camera.faded = camera.shouldFade;
}
__declspec(noinline) static void NativeClear(void *object, void *info) {
  auto &camera = *(Camera *)object;
  camera.clearInfo = info;
  ++camera.clearCalls;
  camera.order.push_back(2);
  camera.faded = false;
}
static MInfo pitchMethod{(void *)NativePitch}, clearMethod{(void *)NativeClear};
static bool missingClass = false, missingPitch = false, missingClear = false, wrongArity = false;
static void *Methods(void *, void **iter) {
  auto index = (uintptr_t)*iter;
  while (index < 2) {
    *iter = (void *)(index + 1);
    if (index++ == 0) { if (!missingPitch) return &pitchMethod; }
    else if (!missingClear) return &clearMethod;
  }
  return nullptr;
}
static void Publish(bool enabled) {
  AcquireSRWLockExclusive(&s_eiemGlobalConfigLock);
  s_eiemGlobalConfig.disableCameraFade = enabled;
  ReleaseSRWLockExclusive(&s_eiemGlobalConfigLock);
}
int main(int argc, char **argv) {
  CHECK(argc == 2);
  std::string scenario = argv[1];
  il2cpp_assembly_get_image = [](void *assembly) -> void * { return assembly; };
  il2cpp_image_get_class_count = [](void *) -> size_t { return missingClass ? 0 : 1; };
  il2cpp_image_get_class = [](void *image, size_t) -> void * { return image; };
  il2cpp_class_get_name = [](void *) -> const char * { return "CameraMono"; };
  il2cpp_class_get_namespace = [](void *) -> const char * { return "Beyond.Gameplay.View"; };
  il2cpp_class_get_methods = Methods;
  il2cpp_method_get_name = [](void *method) -> const char * {
    return method == &pitchMethod ? "_ProcessDitherByPitch" : "ForceClearDither";
  };
  il2cpp_method_get_param_count = [](void *) -> uint32_t { return wrongArity ? 1 : 0; };
  CHECK(MH_Initialize() == MH_OK);
  void *assemblies[] = {(void *)1};
  if (scenario == "one_shot_counterexample") {
    Camera camera;
    NativePitch(&camera, &pitchMethod);
    NativeClear(&camera, &clearMethod);
    CHECK(!camera.faded);
    NativePitch(&camera, &pitchMethod);
    CHECK(camera.faded);
  } else if (scenario == "unavailable") {
    missingClass = true;
    CHECK(!InitEiemCameraFade(assemblies, 1) && !s_eiemCameraFadeInstalled);
    missingClass = false; missingPitch = true;
    CHECK(!InitEiemCameraFade(assemblies, 1) && !s_eiemCameraFadeInstalled);
    missingPitch = false; missingClear = true;
    CHECK(!InitEiemCameraFade(assemblies, 1) && !s_eiemCameraFadeInstalled);
    missingClear = false; wrongArity = true;
    CHECK(!InitEiemCameraFade(assemblies, 1) && !s_eiemCameraFadeInstalled);
    wrongArity = false; clearMethod.mp = nullptr;
    CHECK(!InitEiemCameraFade(assemblies, 1) && !s_eiemCameraFadeInstalled);
  } else {
    CHECK(InitEiemCameraFade(assemblies, 1));
    CHECK(InitEiemCameraFade(assemblies, 1));
    CameraStep volatile entry = NativePitch;
    Camera a, b;
    if (scenario == "per_evaluation") {
      Publish(true);
      for (unsigned frame = 0; frame < 100; ++frame) {
        entry(&a, &pitchMethod);
        entry(&b, &pitchMethod);
        CHECK(!a.faded && !b.faded);
        CHECK(a.pitchCalls == frame + 1 && a.clearCalls == frame + 1);
        CHECK(b.pitchCalls == frame + 1 && b.clearCalls == frame + 1);
        CHECK(a.order[frame * 2] == 1 && a.order[frame * 2 + 1] == 2);
      }
      CHECK(a.pitchInfo == &pitchMethod && a.clearInfo == &clearMethod);
      CHECK(a.skip && a.manualEffect);
      a = Camera{};
      entry(&a, &pitchMethod);
      CHECK(!a.faded && a.clearCalls == 1);
    } else if (scenario == "reload_and_manager") {
      CHECK(LoadEiemConfig());
      entry(&a, &pitchMethod);
      CHECK(a.faded && a.clearCalls == 0);
      for (unsigned cycle = 0; cycle < 5; ++cycle) {
        { std::ofstream file(kEiemGlobalConfigPath); file << "[Graphics]\ndisable_camera_fade=true\n"; }
        CHECK(LoadEiemConfig());
        entry(&a, &pitchMethod);
        CHECK(!a.faded);
        { std::ofstream file(kEiemGlobalConfigPath); file << "[Graphics]\ndisable_camera_fade=typo\n"; }
        CHECK(!LoadEiemConfig());
        entry(&a, &pitchMethod);
        CHECK(!a.faded);
        g_pluginActive = false;
        entry(&a, &pitchMethod);
        CHECK(a.faded);
        g_pluginActive = true;
        entry(&a, &pitchMethod);
        CHECK(!a.faded);
        { std::ofstream file(kEiemGlobalConfigPath); file << "[Graphics]\ndisable_camera_fade=false\n"; }
        CHECK(LoadEiemConfig());
        auto clears = a.clearCalls;
        entry(&a, &pitchMethod);
        CHECK(a.faded && a.clearCalls == clears);
      }
    } else if (scenario == "original_exception") {
      Publish(true);
      a.throwOnPitch = true;
      bool threw = false;
      try { entry(&a, &pitchMethod); } catch (const std::runtime_error &) { threw = true; }
      CHECK(threw && a.pitchCalls == 1 && a.clearCalls == 0);
    } else return 2;
    CHECK(MH_DisableHook(MH_ALL_HOOKS) == MH_OK);
    CHECK(MH_RemoveHook((void *)NativePitch) == MH_OK);
  }
  CHECK(MH_Uninitialize() == MH_OK);
  return 0;
}
'''


class CameraFadeHookTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("cl"):
            raise unittest.SkipTest("Requires MSVC developer environment")
        cls.temp = tempfile.TemporaryDirectory(prefix="eiem-camera-hook-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.folder = Path(cls.temp.name)
        source = cls.folder / "camera_hook.cpp"
        source.write_text(SOURCE, encoding="utf-8")
        cls.exe = cls.folder / "camera_hook.exe"
        result = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8", "/O2",
                                 f"/I{ROOT / 'src'}", f"/I{ROOT / 'deps/minhook_lib/include'}",
                                 str(source), str(ROOT / "deps/minhook_lib/lib/libMinHook.x64.lib"),
                                 f"/Fe{cls.exe}"], cwd=cls.folder,
                                capture_output=True, text=True, encoding="utf-8", errors="replace")
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def run_case(self, name):
        with tempfile.TemporaryDirectory(prefix="eiem-camera-run-") as run_dir:
            result = subprocess.run([str(self.exe), name], cwd=run_dir,
                                    capture_output=True, text=True, timeout=15)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_one_shot_clear_does_not_prevent_later_game_writes(self): self.run_case("one_shot_counterexample")
    def test_actual_hook_clears_after_every_evaluation_on_same_instance(self): self.run_case("per_evaluation")
    def test_actual_hook_obeys_disk_reload_and_manager_enable(self): self.run_case("reload_and_manager")
    def test_failed_original_evaluation_is_not_swallowed_or_cleared(self): self.run_case("original_exception")
    def test_missing_class_or_method_rejects_installation(self): self.run_case("unavailable")


if __name__ == "__main__": unittest.main()
