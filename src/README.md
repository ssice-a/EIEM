# EIEM DLL source map

The DLL is currently built as one translation unit from `eiem.cpp`. Most
headers contain implementation, and several depend on declarations introduced
by an earlier include. Moving files or changing include order is therefore a
runtime-sensitive refactor, even when the code itself is unchanged.

The target module layout is documented in
[`docs/code-architecture.md`](../docs/code-architecture.md). Until the physical
split is complete, use the ownership map below when changing code.

| Module | Current implementation | Owns |
|---|---|---|
| Platform and host | `eiem.cpp`, `il2cpp_api.h`, `globals.h`, `init.h`, `applepie_mgr.h` | DLL entry, IL2CPP/Unity API resolution, plugin host, Unity-thread dispatch |
| Mod frontend | `eiem_mod_document.h`, `eiem_expression.h`, `eiem_mods.h`, `eiem_keys.h`, `eiem_persistent_state.h`, `eiem_mod_dispatcher.h` | INI parsing, compilation, variables, keys, persistent values and OS hotkey dispatch |
| Update coordinator | `eiem_mod_update.h` and the update section in `il2cpp_trace.h` | Reconcile, Reapply and Reload transactions |
| Render replacement | `eiem_resource_backend.h`, `eiem_skin_binding.h`, `eiem_render_state.h` and the Render executor in `il2cpp_trace.h` | Mesh, Material, Texture, renderer field ownership, restore and submesh visibility |
| Model lifecycle | Lifecycle adapters and hook installation in `il2cpp_trace.h`, `eiem_npc_model_owner.h` | Observe world, UI and NPC instances; register, reactivate and release them |
| Shape and unfinished authoring | `eiem_shape_*`, `eiem_skeleton_*`, `eiem_native_physics_*`, `eiem_physics_*` | Shape keys, skeleton extension and native Physics work |
| Mod UI | `eiem_ui_host.h`, `eiem_lua_ui.h`, `gui.h` | Minimal Mod manager and Lua windows in their own host; legacy diagnostics UI remains in `gui.h` |
| Camera fade | `eiem_camera_fade.h` plus its small hook adapter | The independent camera transparency override |
| Diagnostics | `eiem_registration_trace.h`, `eiem_skin_probe.h`, `eiem_metadata_probe.h`, dump files | Bounded, opt-in evidence; never production state decisions |
| Legacy runtime | `animation.h`, `trojan.h`, MMD/camera/audio/player headers | Historical MMD and animation features pending quarantine or removal |

## Dependency rules

1. Hook adapters translate game callbacks into model or update events. They do
   not implement a second Mesh/Material replacement path.
2. All world, UI and NPC consumers call the same Render executor. Instance
   adapters may differ, resource semantics may not.
3. INI parsing and condition evaluation do not call Unity APIs. Unity mutation
   occurs only in an update transaction or a verified lifecycle callback.
4. Resource caches own generated Unity objects; Renderer override records own
   restoration responsibility. Neither responsibility belongs to diagnostics.
5. Camera fade, Mod UI, Shape, Skeleton and Physics remain optional feature
   modules. They must not become prerequisites for static resource replacement.
6. New diagnostic code needs a bounded trigger and a removal condition. Do not
   add periodic scene scans or dormant timer state.
7. Keyboard input is scoped by the selected Mod path. Manager button input is
   additionally scoped by Key section; neither path may broadcast a chord to
   every loaded Mod.

## Next physical splits

After F10 and key switching are accepted in game, split `il2cpp_trace.h` in this
order while preserving function order and tests:

1. update coordinator (the key/reload OS dispatcher is now
   `eiem_mod_dispatcher.h`);
2. model instance registry and world/UI/NPC lifecycle adapters;
3. Render override ownership and the Render executor;
4. hook installation and optional diagnostics;
5. legacy Partner and upstream resource experiments after their remaining
   consumers are removed or replaced.
