from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class RuntimeHookContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace = (ROOT / "src" / "il2cpp_trace.h").read_text(encoding="utf-8")
        cls.api = (ROOT / "src" / "il2cpp_api.h").read_text(encoding="utf-8")
        cls.backend = (ROOT / "src" / "eiem_resource_backend.h").read_text(
            encoding="utf-8"
        )

    def test_cached_model_allocation_is_a_replacement_entry(self):
        self.assertIn("TraceModelManagerGameObjectAllocate", self.trace)
        self.assertIn('"ModelManager._OnGameObjectAllocate"', self.trace)

    def test_use_handle_completion_is_a_replacement_entry(self):
        self.assertIn("TraceBasePartLoadUseHandleFinish", self.trace)
        self.assertIn('"BaseModelViewPart._OnLoadUseHandleFinish"', self.trace)

    def test_generic_prefab_completion_is_a_replacement_entry(self):
        self.assertIn("TracePrefabInstantiateCompleted", self.trace)
        self.assertIn('"PrefabInstantiateProxy.OnCompleted"', self.trace)

    def test_game_material_controller_commits_reapply_render_material_rules(self):
        self.assertIn("TraceRendererInfoTrySetSharedMaterial", self.trace)
        self.assertIn("TraceRendererInfoTrySetSharedMaterials", self.trace)
        self.assertIn("TraceRendererInfoTryReplaceSharedMaterials", self.trace)
        self.assertIn('"RendererInfo.TrySetSharedMaterial"', self.trace)
        self.assertIn('"RendererInfo.TrySetSharedMaterials"', self.trace)
        self.assertIn('"RendererInfo.TryReplaceSharedMaterials"', self.trace)
        self.assertNotIn('"Renderer.set_sharedMaterial"', self.trace)
        self.assertNotIn('"Renderer.set_sharedMaterials"', self.trace)
        for function in (
            "static bool TraceRendererInfoTrySetSharedMaterial",
            "static bool TraceRendererInfoTrySetSharedMaterials",
            "static bool TraceRendererInfoTryReplaceSharedMaterials",
        ):
            start = self.trace.index(function)
            body = self.trace[start : start + 1200]
            original = body.index("original(")
            reapply = body.index("EiemReapplyRendererMaterialsAfterCommit")
            self.assertLess(original, reapply)

    def test_renderer_info_is_found_by_shape_and_renderer_field_offset(self):
        self.assertIn("FindMaterialRendererInfoClass", self.trace)
        self.assertIn('"m_renderer"', self.trace)
        self.assertIn("FindFieldInHierarchy", self.trace)

    def test_reconcile_includes_inactive_and_persistent_renderers(self):
        start = self.trace.index("static void TraceVisitRendererType")
        body = self.trace[start : start + 1800]
        resources = body.index("void *enumerator = g_resources_find_objects_of_type_all")
        scene_only = body.index(": g_object_find_objects_of_type", resources)
        self.assertLess(resources, scene_only)

    def test_value_type_asset_handle_is_unboxed_before_instance_methods(self):
        self.assertIn("il2cpp_object_unbox", self.api)
        start = self.backend.index("static void *EiemLoadOriginalAsset")
        body = self.backend[start : start + 1800]
        self.assertIn("il2cpp_object_unbox(handle)", body)
        self.assertIn("Invoke(s_eiemProxyLoadImmediate, unboxedHandle)", body)
        self.assertIn("Invoke(s_eiemProxyGet, unboxedHandle)", body)

    def test_f10_reconcile_can_reapply_meshes(self):
        marker = "reconcile->matches[(size_t)resolvedIndex],"
        start = self.trace.index(marker)
        call_tail = self.trace[start:start + 180]
        self.assertIn("true", call_tail)
        self.assertNotIn("false", call_tail)

    def test_configured_target_diagnostics_are_not_character_hardcoded(self):
        self.assertNotIn('strstr(pathText, "wulfa")', self.trace)
        self.assertNotIn('_strnicmp(assetName, "S_actor_wulfa"', self.trace)


class BlenderExportContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.addon = (ROOT / "tools" / "Blender" / "eiem_blender_addon.py").read_text(
            encoding="utf-8"
        )

    def test_export_is_rooted_at_selected_eiem_meshes(self):
        self.assertIn("selected_objects", self.addon)
        self.assertIn("No EIEM mesh objects selected", self.addon)

    def test_export_materials_and_textures_are_dependency_closure(self):
        self.assertIn("referenced_materials", self.addon)
        self.assertIn("referenced_images", self.addon)


if __name__ == "__main__":
    unittest.main()
