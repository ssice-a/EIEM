from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class RuntimeHookContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trace = (ROOT / "src" / "il2cpp_trace.h").read_text(encoding="utf-8")
        cls.mods = (ROOT / "src" / "eiem_mod_document.h").read_text(encoding="utf-8") + (ROOT / "src" / "eiem_mods.h").read_text(encoding="utf-8")
        cls.api = (ROOT / "src" / "il2cpp_api.h").read_text(encoding="utf-8")
        cls.backend = (ROOT / "src" / "eiem_resource_backend.h").read_text(
            encoding="utf-8"
        )
        cls.init = (ROOT / "src" / "init.h").read_text(encoding="utf-8")

    def test_model_instances_have_prefab_and_ui_lifecycle_adapters(self):
        self.assertIn("TracePrefabInstantiateCompleted", self.trace)
        self.assertIn('"PrefabInstantiateProxy.OnCompleted"', self.trace)
        self.assertIn("TraceUIModelLoaderLoadModel", self.trace)
        self.assertIn("TraceUIModelLoaderLoadModelAsync", self.trace)
        self.assertNotIn("EiemUIModelLoadedCallback", self.trace)
        self.assertIn("EiemRegisterAndApplyModelInstance", self.trace)
        self.assertIn("EiemApplyStandaloneRenderRules", self.trace)
        self.assertNotIn("UnityEngine.Object.Instantiate", self.trace)
        self.assertNotIn("TraceApplyLoadedModelRenderers", self.trace)
        self.assertNotIn("EiemResolveRenderRuleForAsset", self.trace)

    def test_partner_renderers_use_inherited_bounds_and_real_lod_methods(self):
        self.assertIn(
            'FindMethodInHierarchy(g_skinnedMeshRendererClass, "get_localBounds", 0)',
            self.init,
        )
        self.assertIn(
            'FindMethodInHierarchy(g_skinnedMeshRendererClass, "set_localBounds", 1)',
            self.init,
        )
        self.assertIn('FindMethod(g_lodGroupClass, "GetLODs", 1)', self.init)
        self.assertIn('FindMethod(g_lodGroupClass, "SetLODs", 1)', self.init)
        self.assertNotIn('FindMethod(g_lodGroupClass, "get_lods"', self.init)
        self.assertIn("bool getPlatformLODs = false", self.trace)
        self.assertIn("EiemCopySkinnedRendererState(sourceMeshOwner, partnerMeshOwner)", self.trace)
        for property_name in (
            "skinningRoot",
            "quality",
            "updateWhenOffscreen",
            "forceMatrixRecalculationPerRender",
            "skinnedMotionVectors",
        ):
            self.assertIn(f'"get_{property_name}"', self.init)
            self.assertIn(f'"set_{property_name}"', self.init)

    def test_prefab_lifecycle_releases_instance_state(self):
        for method in ("Unload", "Clear", "Dispose"):
            self.assertIn("TracePrefabInstantiate" + method, self.trace)
        self.assertIn("EiemDestroyPartnerObjects(modelOwner)", self.trace)
        self.assertIn("EiemForgetRenderOverrides(modelOwner)", self.trace)

    def test_ui_async_leaves_managed_callback_and_completion_to_game(self):
        start = self.trace.rindex("static int32_t TraceUIModelLoaderLoadModelAsync")
        end = self.trace.index("static void TraceUIModelLoaderUnloadModel", start)
        body = self.trace[start:end]
        self.assertNotIn("EiemCreateUIModelCallback", self.trace)
        self.assertNotIn("TraceActionGameObjectCtorFn", self.trace)
        self.assertIn("original(self, path, parent, callback, methodInfo)", body)
        self.assertNotIn("EiemRegisterAndApplyModelInstance", body)

    def test_ui_lifecycle_releases_instances_without_native_pending_callbacks(self):
        self.assertIn("TraceUIModelLoaderUnloadModel", self.trace)
        self.assertNotIn("TraceUIModelLoaderCancel", self.trace)
        self.assertIn("TraceUIModelLoaderClear", self.trace)
        self.assertIn("TraceUIModelLoaderDispose", self.trace)
        self.assertIn("EiemForgetModelInstance(model", self.trace)
        self.assertNotIn("EiemPendingUIModelCallback", self.trace)
        self.assertNotIn("EiemCancelPendingUIModelCallbacks", self.trace)

    def test_cached_model_allocation_only_reapplies_known_instances(self):
        start = self.trace.rindex("static void TraceModelManagerGameObjectAllocate")
        body = self.trace[start : start + 700]
        self.assertIn("EiemReapplyRegisteredModelInstance", body)
        self.assertNotIn("EiemRegisterAndApplyModelInstance", body)

    def test_f10_reconcile_only_walks_registered_model_instances(self):
        start = self.trace.index("static void EiemRunModReconcile")
        end = self.trace.index("\n}\n", start)
        body = self.trace[start:end]
        self.assertIn("s_eiemModelInstances", body)
        self.assertIn("EiemApplyStandaloneRenderRules", body)
        self.assertNotIn("find_objects_of_type", body.lower())

    def test_submesh_hooks_do_not_replace_resources(self):
        self.assertNotIn("EiemSubMeshOverrideState", self.trace)
        start = self.trace.index("static void TraceSubMeshInfoSetMesh")
        body = self.trace[start : start + 2200]
        self.assertNotIn("EiemApplyResolvedRenderRule", body)
        self.assertNotIn("EiemRememberSubMeshReplacement", body)

    def test_vfs_observation_cannot_bypass_render_rules_with_loose_bundles(self):
        for obsolete in ("TraceFindVfsOverride", "TraceTryBundleOverride",
                         "TraceLoadPlaintextBundle", "TraceCreateManagedPath",
                         "s_assetBundleLoadFromFile", "s_vfsThreadAttached"):
            self.assertNotIn(obsolete, self.trace)
        for method in ("TraceVfsLoadBundleFromFile", "TraceVfsLoadBundleFromFileAsync",
                       "TraceVfsLoadBundleFromFilePos", "TraceVfsLoadBundleFromFileAsyncPos"):
            start = self.trace.index("static void *" + method + "(")
            end = self.trace.index("\n}\n", start)
            body = self.trace[start:end]
            self.assertIn("original ? original(self, path,", body)
            self.assertIn("TraceRememberBundlePath(path)", body)
            self.assertNotIn("GetFileAttributes", body)

    def test_base_model_view_part_completion_registers_exact_model_path(self):
        self.assertIn("EiemModelOwnerKind::BaseModelPart", self.trace)
        self.assertIn("EiemRegisterBaseModelViewPartInstance", self.trace)

        finish = self.trace[
            self.trace.rindex("static void TraceBasePartFinish") :
        ][:1200]
        self.assertIn("EiemRegisterBaseModelViewPartInstance", finish)

        callback = self.trace[
            self.trace.rindex("static void TraceBasePartLoadUseHandleFinishCallback") :
        ][:1200]
        self.assertIn("EiemRegisterBaseModelViewPartInstance", callback)

        result = self.trace[
            self.trace.rindex("static bool TraceBasePartLoadUseHandleFinish") :
        ][:1200]
        self.assertIn("EiemRegisterBaseModelViewPartInstance", result)
        self.assertIn("TraceBasePartReleaseModel", self.trace)
        self.assertIn("TraceBasePartOnRelease", self.trace)
        self.assertIn(
            "EiemForgetModelOwner(EiemModelOwnerKind::BaseModelPart",
            self.trace,
        )

    def test_character_ui_owner_can_apply_mesh_rules_without_prefab_identity(self):
        self.assertIn("EiemModelOwnerKind::CharUIModel", self.trace)
        self.assertIn("TraceCharUIModelOnAwake", self.trace)
        self.assertIn("TraceCharUIModelSetVisible", self.trace)
        self.assertIn("TraceCharUIModelOnRelease", self.trace)
        start = self.trace.rindex("static bool EiemRegisterCharUIModelInstance")
        body = self.trace[start : start + 3200]
        self.assertIn("g_component_get_gameObject", body)
        self.assertIn("EiemRegisterAndApplyModelInstance", body)
        self.assertIn("nullptr", body)
        self.assertNotIn("FindObjectsOfType", body)
        self.assertIn(
            "EiemForgetModelOwner(EiemModelOwnerKind::CharUIModel",
            self.trace,
        )
        self.assertNotIn("EiemFindModPrefabPathByModelName", body)

    def test_npc_owner_registers_completed_model_and_releases_by_component(self):
        owner = (ROOT / "src" / "eiem_npc_model_owner.h").read_text(
            encoding="utf-8"
        )
        self.assertIn("EiemModelOwnerKind::NpcAvatar", owner)
        start = owner[
            owner.index("static void EiemPhysicsOwnerTraceStartNpc") :
            owner.index("static bool EiemPhysicsOwnerReleaseInfo")
        ]
        self.assertIn("EiemRegisterAndApplyModelInstance", start)
        release = owner[
            owner.index("static void EiemPhysicsOwnerTraceReleaseAvatar") :
            owner.index("static bool EiemPhysicsOwnerHookExact")
        ]
        self.assertEqual(
            release.count("EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar"),
            2,
        )

    def test_physics_intents_follow_render_hits_but_are_owned_by_the_model(self):
        match_start = self.trace.index("static bool EiemApplyRenderRuleSetToRenderer")
        match_body = self.trace[match_start : match_start + 5200]
        self.assertIn("EiemCollectPhysicsIntent(rule, physicsIntents, drawRenderer)", match_body)
        self.assertLess(
            match_body.index("EiemCollectPhysicsIntent(rule, physicsIntents, drawRenderer)"),
            match_body.index("EiemModAffected(rule.modPath, affected)"),
        )
        setter_start = self.trace.rindex("static bool EiemApplyStandaloneRenderRulesToRenderer")
        setter_body = self.trace[setter_start : setter_start + 800]
        self.assertNotIn("EiemCollectPhysicsIntent", setter_body)
        self.assertIn("std::vector<EiemPhysicsIntent> physicsIntents", self.trace)
        self.assertIn("EiemStoreModelPhysicsIntents", self.trace)

    def test_character_ui_visibility_keeps_plan_and_tracks_activity(self):
        start = self.trace.rindex("static void TraceCharUIModelSetVisible")
        end = self.trace.rindex("static void TraceCharUIModelOnRelease")
        body = self.trace[start:end]
        self.assertIn("EiemRegisterCharUIModelInstance", body)
        self.assertIn("EiemSetModelOwnerActive", body)
        self.assertNotIn("EiemForgetModelOwner", body)
        release_start = self.trace.rindex("static void TraceCharUIModelOnRelease")
        release_body = self.trace[release_start : release_start + 500]
        self.assertIn("EiemForgetModelOwner", release_body)

    def test_npc_final_bone_boundary_remains_observation_only(self):
        start = self.trace.rindex("static void TraceSetSmrRootBone")
        body = self.trace[start : start + 900]
        self.assertIn(
            "original(animator, renderers, rootBoneInfos, methodInfo)", body
        )
        self.assertNotIn("EiemApplyStandaloneRenderRules", body)
        self.assertNotIn("EiemFindPrefabRenderRuleByAsset", self.mods)

    def test_prefab_parser_supports_multiple_mods_for_one_path(self):
        self.assertIn("struct EiemModPrefab", self.mods)
        self.assertIn("EiemFindModPrefabs", self.mods)
        body = self.mods[self.mods.index("static void EiemFindModPrefabs") :]
        self.assertIn("out->push_back(prefab)", body[:800])

    def test_asset_only_render_selector_does_not_require_transform_path(self):
        start = self.trace.index("static bool EiemApplyRenderRuleSetToRenderer")
        body = self.trace[start : start + 4200]
        self.assertNotIn(
            "!model || !EiemOnUnityThread() || !g_gameObject_get_transform",
            body,
        )
        self.assertIn("if (rule.path[0])", body)
        self.assertIn("relativePathAttempted", body)

    def test_game_material_controller_commits_reapply_bound_rules(self):
        self.assertIn("TraceRendererInfoTrySetSharedMaterial", self.trace)
        self.assertIn("TraceRendererInfoTrySetSharedMaterials", self.trace)
        self.assertIn("TraceRendererInfoTryReplaceSharedMaterials", self.trace)
        self.assertIn("EiemFindBoundRenderRule", self.trace)

    def test_renderer_info_init_covers_npc_direct_mesh_construction(self):
        start = self.trace.index("static void TraceMaterialInfoInit")
        end = self.trace.index("static bool TraceRendererInfoTrySetSharedMaterial", start)
        body = self.trace[start:end]
        self.assertLess(
            body.index("original(self, renderer, configs, methodInfo)"),
            body.index("EiemApplyStandaloneRenderRulesToRenderer"),
        )
        self.assertIn('"RendererInfo._Init"', body)
        self.assertIn("EiemReapplyRendererMaterialsAfterCommit", body)
        self.assertNotIn("EiemApplyPrefabRules", body)

    def test_value_type_asset_handle_is_unboxed_before_instance_methods(self):
        self.assertIn("il2cpp_object_unbox", self.api)
        start = self.backend.index("static void *EiemLoadOriginalAsset")
        body = self.backend[start : start + 1800]
        self.assertIn("il2cpp_object_unbox(handle)", body)
        self.assertIn("Invoke(s_eiemProxyLoadImmediate, unboxedHandle)", body)
        self.assertIn("Invoke(s_eiemProxyGet, unboxedHandle)", body)

    def test_configured_target_diagnostics_are_not_character_hardcoded(self):
        self.assertNotIn('strstr(pathText, "wulfa")', self.trace)
        self.assertNotIn('_strnicmp(assetName, "S_actor_wulfa"', self.trace)

    def test_asset_completion_is_observation_only(self):
        start = self.trace.index("static void TraceAssetFinishWithAsset")
        end = self.trace.index("static void TraceAssetOnComplete", start)
        body = self.trace[start:end]
        self.assertNotIn("TraceTryGlobalResourceRedirect", body)
        self.assertIn("original(self, asset, methodInfo)", body)

    def test_standalone_render_rules_are_mesh_identity_actions(self):
        self.assertIn("EiemFindStandaloneRenderRules", self.mods)
        self.assertIn("EiemApplyStandaloneRenderRules", self.trace)
        start = self.trace.rindex("static bool EiemApplyStandaloneRenderRules")
        body = self.trace[start : start + 1000]
        self.assertIn("EiemFindStandaloneRenderRules", body)
        self.assertIn("EiemApplyRenderRuleSet", body)

    def test_prefab_completion_always_offers_completed_model_to_mesh_rules(self):
        start = self.trace.rindex("static void TracePrefabInstantiateCompleted")
        body = self.trace[start : start + 2200]
        self.assertIn("EiemRegisterAndApplyModelInstance", body)
        self.assertNotIn("configured &&\n      EiemRegisterAndApplyModelInstance", body)

    def test_f10_replays_standalone_mesh_rules_for_registered_instances(self):
        start = self.trace.index("static void EiemRunModReconcile")
        end = self.trace.index("static void *s_origAssetBundleLoadAsset1", start)
        body = self.trace[start:end]
        self.assertIn("EiemApplyStandaloneRenderRules", body)

    def test_mesh_setter_matches_even_when_resource_origin_was_not_observed(self):
        start = self.trace.rindex("static void TraceSkinnedMeshSetSharedMesh")
        body = self.trace[start : start + 1800]
        self.assertIn("EiemApplyStandaloneRenderRulesToRenderer", body)
        self.assertNotIn("TraceTryGlobalResourceRedirect", body)

    def test_material_clones_its_declared_source_asset(self):
        start = self.backend.index("static bool EiemBuildMaterialResource")
        end = self.backend.index("static void EiemApplySubmeshMaterialMap", start)
        material_body = self.backend[start:end]
        self.assertNotIn("templateMaterial", material_body)
        self.assertIn("void *sourceMaterial = EiemLoadOriginalAsset", material_body)
        self.assertNotIn("EiemBuildGlobalResource", self.backend)

    def test_initial_mod_rules_load_before_resource_hooks_are_enabled(self):
        start = self.init.index("static DWORD WINAPI InitThread")
        body = self.init[start:]
        self.assertLess(body.index("EiemReloadMods();"),
                        body.index("InitIl2CppResourceTrace(asms, ac);"))

        hotkey_start = self.init.index("static DWORD WINAPI HotkeyThread")
        hotkey_end = self.init.index("static DWORD WINAPI InitThread")
        hotkey_body = self.init[hotkey_start:hotkey_end]
        hotkey_setup = hotkey_body[:hotkey_body.index("enum class KeyAction")]
        self.assertNotIn("EiemReloadMods();", hotkey_setup)
        self.assertNotIn("EiemReloadMods();", hotkey_body)
        self.assertIn("EiemRequestModUpdate(EiemModUpdate::Reload", hotkey_body)

    def test_prefab_reference_does_not_scope_a_mesh_identity_rule(self):
        start = self.mods.index("static void EiemCompileModProgram")
        end = self.mods.index("static bool EiemValidShapeExpression", start)
        body = self.mods[start:end]
        self.assertNotIn("for (const auto &prefab", body)
        self.assertIn('statement.key.compare(0, 8, "partner.")', body)
        self.assertIn("program.standaloneRules.push_back(i)", body)
        self.assertNotIn("EiemApplyPrefabRules", self.trace)

    def test_reload_is_dispatched_only_on_unity_thread(self):
        body = self.trace[self.trace.index("static void EiemRunModReconcile()") :][:2700]
        self.assertIn("EiemDispatchModUpdate(requests", body)
        self.assertIn("EiemRestoreRenderOverrides(affected)", body)
        self.assertIn("EiemReloadMods()", body)
        entry = (ROOT / "src" / "eiem.cpp").read_text(encoding="utf-8")
        self.assertNotIn("EiemReloadMods();", entry)


class BlenderExportContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.addon = (ROOT / "tools" / "Blender" / "eiem_blender_addon.py").read_text(
            encoding="utf-8"
        )
        cls.controls = (ROOT / "tools" / "Blender" / "eiem_blender_controls.py").read_text(
            encoding="utf-8"
        )
        writer_path = (
            ROOT / "tools" / "AnimeStudio" / "AnimeStudio.GUI" /
            "EiemPackageWriter.cs"
        )
        cls.writer = writer_path.read_text(encoding="utf-8") if writer_path.exists() else None
        physics_writer_path = writer_path.with_name("EiemPhysicsSourceWriter.cs")
        cls.physics_writer = (physics_writer_path.read_text(encoding="utf-8")
                              if physics_writer_path.exists() else None)

    def test_export_is_rooted_at_selected_eiem_meshes(self):
        self.assertIn("selected_objects", self.addon)
        self.assertIn("No EIEM mesh objects selected", self.controls)

    def test_export_materials_and_textures_are_dependency_closure(self):
        self.assertIn("referenced_materials", self.addon)
        self.assertIn("referenced_images", self.addon)

    def test_blender_emits_standalone_render_identity_without_prefab_scope(self):
        import ast
        tree = ast.parse(self.addon)
        body = "\n".join(ast.get_source_segment(self.addon, node) for node in tree.body
                         if isinstance(node, ast.FunctionDef) and
                         node.name in ("export_package", "write_export_package"))
        self.assertIn('asset = str(first.get("eiem_render_asset"', body)
        self.assertIn('"eiem_target_asset", first.data.get("eiem_asset"', body)
        self.assertNotIn("has no Prefab/Render identity", body)
        self.assertNotIn('"render.%d=%s"', body)

    def test_existing_resources_default_to_their_offline_identity(self):
        self.assertIn('mesh["eiem_target_path"] = values.get(', self.addon)
        self.assertIn('"target.path", payload.get("source", ""))', self.addon)
        self.assertIn('material_values["target.path"] = values.get(', self.addon)
        self.assertIn(
            '"target.path", material_values.get("source", ""))', self.addon
        )

    def test_animestudio_emits_one_render_action_per_mesh_resource(self):
        if self.writer is None:
            self.skipTest("AnimeStudio is a separately managed checkout")
        self.assertIn("renderedMeshes", self.writer)
        self.assertIn("if (!renderedMeshes.Add(mesh))", self.writer)
        self.assertIn('ini.Add($"render.{index}={renderSections[index]}")', self.writer)

    def test_animestudio_normal_package_embeds_optional_native_source(self):
        if self.writer is None or self.physics_writer is None:
            self.skipTest("AnimeStudio is a separately managed checkout")
        self.assertIn("EiemPhysicsSourceWriter.Write", self.writer)
        self.assertIn('Path.Combine(rootDirectory, "physics")', self.writer)
        for component in ("BeyondBoneCloth", "BeyondBoneSphereCollider",
                          "BeyondBoneCapsuleCollider", "BeyondBonePlaneCollider"):
            self.assertIn(component, self.physics_writer)
        self.assertIn('purpose = "raw-component-evidence-not-a-physics-package"',
                      self.physics_writer)

    def test_blender_has_ordered_left_aligned_eiem_property_panels(self):
        self.assertIn("class EIEM_PT_material_properties", self.addon)
        self.assertIn("class EIEM_PT_mesh_properties", self.addon)
        self.assertIn("class EIEM_PT_image_properties", self.addon)
        self.assertIn('label_column.alignment = "LEFT"', self.addon)
        start = self.addon.index("def material_property_groups")
        body = self.addon[start : start + 1800]
        self.assertLess(body.index("texture_paths"), body.index("parameters"))
        self.assertIn("EIEM_INTERNAL_MATERIAL_PROPERTIES", body)


if __name__ == "__main__":
    unittest.main()
