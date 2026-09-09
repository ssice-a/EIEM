from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class NativePhysicsRuntimeContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.runtime = (ROOT / "src" / "eiem_native_physics_runtime.h").read_text(
            encoding="utf-8"
        )
        cls.trace = (ROOT / "src" / "il2cpp_trace.h").read_text(encoding="utf-8")
        cls.mods = (ROOT / "src" / "eiem_mods.h").read_text(encoding="utf-8")
        cls.diagnostic = (
            ROOT / "src" / "eiem_native_physics_diagnostic.h"
        ).read_text(encoding="utf-8")
        cls.trojan = (ROOT / "src" / "trojan.h").read_text(encoding="utf-8")

    def test_resource_runtime_is_automatic_and_separate_from_dump_ui(self):
        self.assertIn('#include "eiem_native_physics_runtime.h"', self.trace)
        self.assertNotIn('#include "eiem_native_physics_factory_probe.h"', self.trace)
        self.assertIn('EiemPhysicsRuntimePeriodic("window-main-thread")', self.trojan)
        self.assertNotIn("ImGui", self.runtime)
        self.assertNotIn("scene_dump", self.runtime)

    def test_runtime_consumes_renderer_hits_and_selects_one_animator(self):
        self.assertIn("intent.matchedRenderers", self.runtime)
        self.assertIn("EiemPhysicsRuntimeRendererUnderModel", self.runtime)
        self.assertIn("EiemPhysicsRuntimeRendererDepth", self.runtime)
        self.assertIn("ancestors != 1", self.runtime)
        self.assertIn("Physics Render matches span different Animators", self.runtime)

    def test_configuration_precedes_build_on_an_inactive_host(self):
        body = self.runtime[
            self.runtime.index("static bool EiemPhysicsRuntimeBuild") :
            self.runtime.index("static void EiemReconcileModelPhysics")
        ]
        for earlier, later in (
            ("EiemPhysicsRuntimeNewHost", "addComponent"),
            ("addComponent", "disableAutoBuild"),
            ("disableAutoBuild", "setSerializeData"),
            ("setSerializeData", "bool active = true"),
            ("bool active = true", "buildAndRun"),
        ):
            self.assertLess(body.index(earlier), body.rindex(later))
        host = self.runtime[
            self.runtime.index("static bool EiemPhysicsRuntimeNewHost") :
            self.runtime.index("static void EiemPhysicsRuntimeBeginRetire")
        ]
        self.assertIn("bool active = false", host)

    def test_component_keeps_constructor_owned_serialize_data2(self):
        self.assertNotIn("serializeData2Field", self.runtime)
        self.assertIn("componentData2Ref", self.runtime)
        self.assertIn("getSerializeData2", self.runtime)
        self.assertIn("actualData2", self.runtime)
        self.assertIn("EiemUnityRef::Capture(actualData2, false)", self.runtime)

    def test_ready_requires_live_process_and_expected_animator(self):
        poll = self.runtime[
            self.runtime.index("static void EiemPhysicsRuntimePollReady") :
            self.runtime.index("static void EiemPhysicsRuntimePeriodic")
        ]
        for required in ("processValid", "processRunning", "processTeamId"):
            self.assertIn(required, poll)
        self.assertIn("animator != instance->animator", poll)
        self.assertIn("instance->ready = true", poll)

    def test_retirement_destroys_only_the_owned_host_and_waits_for_native_death(self):
        self.assertNotIn("DisposeInternal", self.runtime)
        self.assertNotIn("DestroyClothBindings", self.runtime)
        self.assertIn("g_object_destroy", self.runtime)
        self.assertIn("destroyNeeded", self.runtime)
        self.assertIn("EiemPhysicsRuntimeDead", self.runtime)
        dead = self.runtime[
            self.runtime.index("static bool EiemPhysicsRuntimeDead") :
            self.runtime.index("static void EiemPhysicsRuntimeCollect")
        ]
        self.assertIn("hostRef.Status()", dead)
        self.assertIn("componentRef.Status()", dead)
        release = self.runtime[
            self.runtime.index("static void EiemReleaseModelPhysics") :
            self.runtime.index("static void EiemPhysicsRuntimePollReady")
        ]
        self.assertIn("s_eiemPhysicsPendingReleases.push_back", release)
        self.assertIn("EiemPhysicsRuntimeDrainReleases", self.runtime)
        self.assertIn("EiemPhysicsRuntimeReleaseOnUnityThread", self.runtime)

    def test_loader_publishes_physics_mods_for_the_runtime_adapter(self):
        self.assertNotIn("if (requestsPhysics)", self.mods)
        self.assertNotIn("native Physics execution is not connected (file skipped)", self.mods)
        self.assertIn("EiemAppendModDocument(next,std::move(document))", self.mods)
        self.assertIn("EiemReconcileModelPhysics", self.trace)

    def test_binding_log_reports_mesh_palette_membership(self):
        self.assertIn("g_smr_get_bones", self.runtime)
        self.assertIn("paletteHits", self.runtime)
        self.assertIn("palette=%zu selected=%zu paletteHits=%zu boundaryIgnores=%zu", self.runtime)
        self.assertIn("boundaryIgnores += group.boundaryIgnores.size()", self.runtime)

    def test_shutdown_only_finishes_the_trace(self):
        finish = self.diagnostic[
            self.diagnostic.index("static void EiemFinishPhysicsAutoTraceOnUnityThread") :
        ]
        self.assertNotIn("EiemPhysicsFactoryProbe", finish)
        self.assertLess(finish.index("EiemPhysicsStopTrace"),
                        finish.index("EiemWriteNativePhysicsDiagnostic"))


if __name__ == "__main__":
    unittest.main()
