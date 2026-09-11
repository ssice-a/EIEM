"""Exercise actual partner retirement/ownership code without a game process."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(text, name):
    start = text.index("static ", text.index(name) - 20)
    opening = text.index("{", text.index(name, start))
    depth = 1
    end = opening + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


class PartnerControlsTests(unittest.TestCase):
    def test_lod_membership_reconciles_stale_partner_levels(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        membership = function(trace, "EiemSetPartnerLodMembership")
        self.assertIn("sourceIndex != SIZE_MAX", membership)
        self.assertIn("else if (partnerIndex != SIZE_MAX)", membership)
        self.assertIn("rendererCount - 1", membership)

    def test_control_reconcile_has_selective_partner_retirement_path(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        function(trace, "EiemPartnerDesired")
        function(trace, "EiemDestroyUndesiredPartnerObjects")
        reconcile = function(trace, "EiemRunModReconcile")
        self.assertIn("partnerLinksOnly", reconcile)
        self.assertIn("EiemDestroyUndesiredPartnerObjects", reconcile)

    def test_partner_is_retired_only_when_its_source_rule_drops_the_link(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC developer environment")
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        desired = function(trace, "EiemPartnerDesired")
        code = r'''
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "eiem_mod_document.h"
struct EiemPartnerState {
  char sourceSection[96] = {};
  char section[96] = {};
  char modPath[MAX_PATH] = {};
};
''' + desired + r'''
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while(false)
int main() {
  EiemModProgram program;
  EiemModRule source = {};
  EiemModInitRule(&source);
  strcpy_s(source.modPath, "a/mod.ini");
  strcpy_s(source.section, "RenderMain");
  strcpy_s(source.partners[0], "RenderPart");
  source.partnerCount = 1;
  program.rules.push_back(source);
  EiemPartnerState state;
  strcpy_s(state.modPath, "a/mod.ini");
  strcpy_s(state.sourceSection, "RenderMain");
  strcpy_s(state.section, "RenderPart");
  CHECK(EiemPartnerDesired(state, program));
  program.rules[0].partnerCount = 0;
  CHECK(!EiemPartnerDesired(state, program));
  strcpy_s(program.rules[0].modPath, "b/mod.ini");
  program.rules[0].partnerCount = 1;
  CHECK(!EiemPartnerDesired(state, program));
}
'''
        with tempfile.TemporaryDirectory(prefix="eiem-partner-link-test-") as directory:
            folder = Path(directory)
            source, exe = folder / "test.cpp", folder / "test.exe"
            source.write_text(code, encoding="utf-8")
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}"],
                                   cwd=folder, capture_output=True, text=True,
                                   encoding="utf-8", errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_partner_stays_inactive_until_skinning_is_fully_configured(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        create = function(trace, "EiemCreatePartnerRenderer")

        deactivate = create.index("bool inactive = false;")
        add_renderer = create.index("partnerMeshOwner = addComponent")
        copy_skinning = create.index("EiemCopySkinnedRendererState")
        reactivate = create.index("void *activateParams[] = {&sourceActive};")

        self.assertLess(deactivate, add_renderer)
        self.assertLess(add_renderer, copy_skinning)
        self.assertLess(copy_skinning, reactivate)
        self.assertIn("InvokeChecked(g_gameObject_set_active, partnerGo, inactiveParams", create)
        self.assertIn("InvokeChecked(g_gameObject_set_active, partnerGo, activateParams", create)

    def test_retirement_detaches_before_destroy_and_ownership_blocks_rematch(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC developer environment")
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        shared = r'''
#include "eiem_mod_document.h"
#include <cstdio>
static std::string calls;
static bool valid = true, prepared = false;
static void Log(const char *, ...) {}
static void *g_gameObject_get_transform = (void*)10, *g_transform_set_parent = (void*)11, *g_object_destroy = (void*)12,
            *g_gameObject_set_active = (void*)13;
static void *Invoke(void *method, void *self, void **args = nullptr) {
  if (method == g_gameObject_set_active) { valid &= self == (void*)1 && !*(bool*)args[0]; calls += 'A'; }
  if (method == g_gameObject_get_transform) { valid &= self == (void*)1; return (void*)3; }
  if (method == g_transform_set_parent) {
    valid &= self == (void*)3 && args[0] == nullptr && !*(bool*)args[1]; calls += 'D';
  }
  if (method == g_object_destroy) { valid &= args[0] == (void*)1; calls += 'X'; }
  return nullptr;
}
static void EiemSetRendererEnabled(void *r, bool enabled) { valid &= r == (void*)2 && !enabled; calls += 'H'; }
static void EiemSetPartnerLodMembership(void *s, void *r, bool add) { valid &= s == (void*)4 && r == (void*)2 && !add; calls += 'L'; }
struct ShapeState { int binding=17; };
struct EiemPartnerState { void *partnerObject; void *partnerRenderer; void *sourceDrawRenderer; ShapeState shapes; };
static void EiemRetireShapeBinding(int binding) { valid &= binding==17; calls += 'S'; }
static SRWLOCK s_eiemPartnerLock = SRWLOCK_INIT;
static std::vector<EiemPartnerState> s_eiemPartners;
static bool s_eiemCreatingPartner = false;
struct EiemResolvedRenderRule { EiemModRule rule; char source[768]; char asset[192]; };
struct EiemPhysicsIntent {};
static bool EiemCollectPhysicsIntent(const EiemModRule &, std::vector<EiemPhysicsIntent> *, void *) { return true; }
static void EiemPrepareRenderInput(void*, void*, const char*, void**) { prepared = true; }
static bool EiemReadLiveMeshIdentity(void*, char*, size_t, char*, size_t) { return false; }
static bool EiemBuildRelativeRendererPath(void*, void*, char*, size_t) { return false; }
static bool EiemRenderRuleMatches(const EiemModRule&, const char*, void*, const char*) { return false; }
static bool EiemModAffected(const char*, const std::vector<std::string>*) { return true; }
static bool EiemApplyResolvedRenderRule(void*, void*, void*, const char*, void*, const EiemResolvedRenderRule&, bool) { return true; }
'''
        code = shared + "\n" + "\n".join(function(trace, name) for name in (
            "EiemIsPartnerRenderer", "EiemRetirePartner", "EiemApplyRenderRuleSetToRenderer")) + r'''
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d\n", __LINE__); return 1; } } while(false)
int main() {
  EiemPartnerState s{(void*)1, (void*)2, (void*)4};
  s_eiemPartners.push_back(s);
  std::vector<EiemModRule> rules;
  CHECK(EiemIsPartnerRenderer((void*)2));
  CHECK(!EiemIsPartnerRenderer((void*)4));
  CHECK(!EiemApplyRenderRuleSetToRenderer(nullptr, (void*)2, (void*)2, (void*)5, "SkinnedMeshRenderer", nullptr, rules, "test"));
  CHECK(!prepared);
  CHECK(!EiemApplyRenderRuleSetToRenderer(nullptr, (void*)4, (void*)4, (void*)5, "SkinnedMeshRenderer", nullptr, rules, "test"));
  CHECK(prepared);
  prepared = false; s_eiemCreatingPartner = true;
  CHECK(!EiemApplyRenderRuleSetToRenderer(nullptr, (void*)6, (void*)6, (void*)5, "SkinnedMeshRenderer", nullptr, rules, "test"));
  CHECK(!prepared);
  EiemRetirePartner(s);
  CHECK(valid && calls == "SAHLDX");
}
'''
        with tempfile.TemporaryDirectory(prefix="eiem-partner-test-") as directory:
            folder = Path(directory)
            source, exe = folder / "test.cpp", folder / "test.exe"
            source.write_text(code, encoding="utf-8")
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}"],
                                   cwd=folder, capture_output=True, text=True, encoding="utf-8", errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
