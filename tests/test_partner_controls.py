"""Exercise Partner visibility and final retirement without a game process."""
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
        reconcile_lods = function(trace, "EiemReconcilePartnerLodMemberships")
        root_sync = function(trace, "EiemSyncPartnerRootBonesFromArray")
        self.assertIn("sourceIndex != SIZE_MAX", membership)
        self.assertIn("else if (partnerIndex != SIZE_MAX)", membership)
        self.assertIn("rendererCount - 1", membership)
        # LOD membership is structural; hidden key states must also be
        # reconciled so a later show does not leave the Partner outside LOD.
        self.assertNotIn("state.controlVisible", reconcile_lods)
        self.assertIn("EiemReconcilePartnerLodMemberships", root_sync)

    def test_control_reconcile_changes_visibility_without_retirement(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        function(trace, "EiemPartnerDesired")
        visibility = function(trace, "EiemApplyPartnerControlVisibility")
        visibility_only = function(trace, "EiemSetPartnerRendererVisibilityOnly")
        reconcile = function(trace, "EiemRunModReconcile")
        self.assertIn("EiemSetPartnerRendererVisibilityOnly", visibility)
        self.assertIn("EiemSetRendererEnabled", visibility_only)
        self.assertNotIn("EiemSetPartnerLodMembership", visibility_only)
        self.assertNotIn("EiemRetirePartner", visibility)
        self.assertNotIn("s_eiemPartners.erase", visibility)
        self.assertIn("partnerLinksOnly", reconcile)
        self.assertIn("EiemApplyPartnerControlVisibility", reconcile)
        self.assertIn("if (partnerLinksOnly)", reconcile)
        self.assertIn("skipped model resource/Physics replay", reconcile)
        self.assertIn("if (!reload) EiemDestroyPartnerObjects(affected)", reconcile)

    def test_key_visibility_does_not_remove_lod_membership(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        visibility = function(trace, "EiemSetPartnerDrawVisibility")
        self.assertIn("EiemSetPartnerLodMembership(change.sourceDrawRenderer,",
                      visibility)
        self.assertIn("change.partnerRenderer, true", visibility)
        self.assertIn("change.visible && change.enabledWhenVisible", visibility)
        self.assertNotIn("change.partnerRenderer, false", visibility)

    def test_all_conditional_partners_are_prebuilt_once(self):
        mods = (ROOT / "src/eiem_mods.h").read_text(encoding="utf-8")
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        potential = function(mods, "EiemFindPotentialPartnerRules")
        apply_partners = function(trace, "EiemApplyPartners")
        self.assertIn("s_eiemModProgram.definitions", potential)
        self.assertIn("EiemVisitStatements", potential)
        self.assertIn('statement.key.compare(0, 8, "partner.")', potential)
        self.assertIn("EiemFindPotentialPartnerRules", apply_partners)
        self.assertIn("EiemSetPartnerDrawVisibility", apply_partners)
        self.assertIn("sourceMesh, desired, error", apply_partners)
        self.assertNotIn("!sourceRule.partnerCount", apply_partners)

    def test_f10_reuses_partner_components_across_generation_reload(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        apply_partners = function(trace, "EiemApplyPartners")
        reconcile = function(trace, "EiemRunModReconcile")
        self.assertIn("EiemFindPartnerAnyGenerationLocked", apply_partners)
        self.assertIn("EiemRefreshPartnerRenderer", apply_partners)
        self.assertIn("s_eiemPartners[commitIndex] = std::move(refreshState)",
                      apply_partners)
        self.assertIn("if (!reload) EiemDestroyPartnerObjects(affected)",
                      reconcile)

    def test_model_replay_binds_early_partner_to_owner(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        apply_partners = function(trace, "EiemApplyPartners")
        self.assertIn("if (s_eiemActivePrefabInstance)", apply_partners)
        self.assertIn("state.ownerPrefabInstance = s_eiemActivePrefabInstance",
                      apply_partners)

    def test_refresh_rebinds_resources_without_allocating_a_gameobject(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        refresh = function(trace, "EiemRefreshPartnerRenderer")
        self.assertIn("EiemBuildMeshResource", refresh)
        self.assertIn("EiemSkeletonMeshBones", refresh)
        self.assertIn("EiemAssignRendererMaterials", refresh)
        self.assertIn("EiemSetSharedMesh(state.partnerRenderer", refresh)
        self.assertNotIn("il2cpp_object_new", refresh)
        self.assertNotIn("g_gameObject_AddComponent", refresh)

    def test_game_skin_writes_replace_the_f10_source_baseline(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        remember_bones = function(trace, "EiemRememberGameSourceBones")
        remember_root = function(trace, "EiemRememberGameSourceRootBone")
        set_bones_start = trace.rindex("static void TraceSkinnedMeshSetBones")
        assign_skin_start = trace.rindex("static void TraceAssignSkinPost")
        set_root_start = trace.rindex("static void TraceSetSmrRootBone")
        set_bones_source = trace[set_bones_start - 20 :]
        assign_skin_source = trace[assign_skin_start - 20 :]
        set_root_source = trace[set_root_start - 20 :]
        set_bones = function(set_bones_source, "TraceSkinnedMeshSetBones")
        assign_skin = function(assign_skin_source, "TraceAssignSkinPost")
        set_root = function(set_root_source, "TraceSetSmrRootBone")
        restore = function(trace, "EiemRestoreRenderOverrides")

        self.assertIn("state.originalBonesHandle = handle", remember_bones)
        self.assertIn("!replacementBinding", remember_bones)
        self.assertIn("state.originalRootBoneHandle = handle", remember_root)
        self.assertLess(set_bones.index("EiemRememberGameSourceBones"),
                        set_bones.index("EiemPreserveSourceSkinning"))
        self.assertIn("EiemRememberGameSourceSkinningFromArray", assign_skin)
        self.assertIn("EiemRememberGameSourceSkinningFromArray", set_root)
        self.assertIn("state.originalBonesHandle", restore)
        self.assertIn("state.originalRootBoneHandle", restore)

    def test_partner_bone_setter_probe_is_observational_and_deduplicated(self):
        trace = (ROOT / "src/il2cpp_trace.h").read_text(encoding="utf-8")
        snapshot = function(trace, "EiemSnapshotPartnerBones")
        setter_start = trace.rindex("static void TraceSkinnedMeshSetBones")
        setter = function(trace[setter_start - 20:], "TraceSkinnedMeshSetBones")
        self.assertIn("state.partnerRenderer != renderer", snapshot)
        self.assertIn("expectedBoneCount", snapshot)
        self.assertIn('"partner-set-bones"', setter)
        self.assertIn('"partner-set-bones-mismatch"', setter)
        self.assertIn("s_eiemApplyingModMeshAssignment", setter)
        self.assertIn("afterRead", setter)
        self.assertNotIn("EiemPreserveSourceSkinning", setter[:setter.index("bool tracked")])

    def test_partner_visibility_follows_its_source_rule_link(self):
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
static bool EiemRendererEligibleForRule(void *, void *) { return true; }
static void EiemPrepareRenderInput(void*, void*, const char*, void**) { prepared = true; }
static bool EiemReadLiveMeshIdentity(void*, char*, size_t, char*, size_t) { return false; }
static bool EiemBuildRelativeRendererPath(void*, void*, char*, size_t) { return false; }
static bool EiemRenderRuleMatches(const EiemModRule&, const char*, void*, const char*) { return false; }
static bool EiemModAffected(const char*, const std::vector<std::string>*) { return true; }
static bool EiemApplyResolvedRenderRule(void*, void*, void*, const char*, void*, const EiemResolvedRenderRule&, bool, bool) { return true; }
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
