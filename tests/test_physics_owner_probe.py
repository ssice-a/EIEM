from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class PhysicsOwnerProbeContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.probe = (ROOT / "src" / "eiem_npc_model_owner.h").read_text(
            encoding="utf-8"
        )
        cls.trace = (ROOT / "src" / "il2cpp_trace.h").read_text(
            encoding="utf-8"
        )
        cls.init = (ROOT / "src" / "init.h").read_text(encoding="utf-8")

    def test_probe_is_automatic_and_has_no_manual_control(self):
        self.assertIn(
            "EiemInstallNpcModelOwner(assemblies, assemblyCount)", self.trace
        )
        self.assertIn('#include "eiem_npc_model_owner.h"', self.trace)
        self.assertNotIn("ImGui::", self.probe)
        self.assertNotIn("GetAsyncKeyState", self.probe)
        self.assertIn("resource-runtime-v70-physics-npc-owner", self.init)

    def test_probe_hooks_exact_npc_owner_boundaries(self):
        for method in ("StartNPC", "_BuildBeyondCloth", "ReleaseAvatar", "OnRelease"):
            self.assertIn(f'"{method}"', self.probe)
        self.assertIn("FindMethodWithParamTypesAndReturnType", self.probe)
        self.assertIn("Beyond.NPC.Avatar.FNPCAvatarGOReference&", self.probe)

    def test_value_type_members_use_metadata_offsets(self):
        for field in ("avatarGoRef", "animator", "go", "boneCloths"):
            self.assertIn(f'{{"{field}"}}', self.probe)
        self.assertIn("boxedOffset - objectHeader", self.probe)
        self.assertNotIn("+ 0x120", self.probe)
        self.assertNotIn("+ 0x48", self.probe)

    def test_adapter_does_not_call_native_physics_apis_directly(self):
        for forbidden in (
            "g_gameObject_AddComponent",
            "BuildAndRun",
            "DisposeInternal",
            "DestroyClothBindings",
            "EiemCollectPhysicsIntent",
        ):
            self.assertNotIn(forbidden, self.probe)
        build = self.probe[
            self.probe.index("static void EiemPhysicsOwnerTraceBuildCloth") :
            self.probe.index("static void EiemPhysicsOwnerTraceStartNpc")
        ]
        self.assertIn(
            "original(meshConfig, animator, goRef, model, methodInfo)", build
        )
        self.assertNotIn("original(self", build)
        self.assertLess(build.index("original(meshConfig"), build.index("refAnimator"))

    def test_start_npc_can_correlate_without_build_cloth(self):
        start = self.probe[
            self.probe.index("static void EiemPhysicsOwnerTraceStartNpc") :
            self.probe.index("static bool EiemPhysicsOwnerReleaseInfo")
        ]
        self.assertIn("if (!entry && s_eiemPhysicsOwnerNpcCount", start)
        self.assertIn("entry->animator = embeddedAnimator", start)
        self.assertIn("entry->sawStart = true", start)
        self.assertIn("EiemPhysicsOwnerCorrelate(*entry)", start)

    def test_exact_npc_owner_boundary_drives_shared_model_runtime(self):
        start = self.probe[
            self.probe.index("static void EiemPhysicsOwnerTraceStartNpc") :
            self.probe.index("static bool EiemPhysicsOwnerReleaseInfo")
        ]
        self.assertIn("EiemModelOwnerKind::NpcAvatar", start)
        self.assertIn("EiemRegisterAndApplyModelInstance", start)
        self.assertLess(start.index("original(avatar, component, methodInfo)"),
                        start.index("EiemRegisterAndApplyModelInstance"))

        release = self.probe[
            self.probe.index("static void EiemPhysicsOwnerTraceReleaseAvatar") :
            self.probe.index("static bool EiemPhysicsOwnerHookExact")
        ]
        self.assertIn("EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar", release)
        self.assertLess(release.index("EiemForgetModelOwner"),
                        release.index("original(manager, component, methodInfo)"))

    def test_renderer_match_selects_nearest_animator_ancestor(self):
        self.assertIn("model-render-match", self.probe)
        self.assertIn("animatorAncestors=%zu", self.probe)
        self.assertIn("nearestAnimator=%p", self.probe)
        self.assertIn("nearestDepth=%d", self.probe)
        self.assertIn("EiemPhysicsOwnerRendererDepthUnderComponent", self.probe)

    def test_owner_release_does_not_match_model_when_owner_is_present(self):
        self.assertIn("const bool sameOwner = owner && entry.owner == owner", self.probe)
        self.assertIn("const bool sameModelWithoutOwner = !owner && model", self.probe)

    def test_owner_adapter_uses_shared_model_executor_instead_of_factory(self):
        self.assertNotIn("EiemPhysicsFactoryProbe", self.probe)
        self.assertNotIn("EiemPhysicsRuntime", self.probe)
        self.assertIn("EiemRegisterAndApplyModelInstance", self.probe)

    def test_mesh_hit_feeds_renderer_and_model_observation(self):
        renderer_init = self.trace[
            self.trace.index("static void TraceMaterialInfoInit") :
            self.trace.index("static bool TraceRendererInfoTrySetSharedMaterial")
        ]
        self.assertIn("EiemPhysicsOwnerProbeObserveRenderer", renderer_init)
        start = self.trace.rindex("static bool EiemRegisterAndApplyModelInstance")
        end = self.trace.index("static bool EiemRegisterBaseModelViewPartInstance", start)
        register = self.trace[start:end]
        self.assertIn("if (applied)", register)
        self.assertIn("EiemPhysicsOwnerProbeObserveModel", register)

    def test_release_return_is_not_used_as_a_completion_fence(self):
        self.assertNotIn("completion fence", self.probe)
        self.assertNotIn("EiemDestroyPartnerObjects", self.probe)


if __name__ == "__main__":
    unittest.main()
