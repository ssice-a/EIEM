# EIEM DLL source map

EIEM is still compiled as one translation unit from `eiem.cpp`. Several headers contain implementation and depend on declarations introduced earlier in the include order, so file moves require the full native test suite and DLL build.

The repository-level boundaries are documented in [`docs/repository-architecture.md`](../docs/repository-architecture.md); the runtime design is in [`docs/code-architecture.md`](../docs/code-architecture.md).

| Module | Current files | Responsibility |
|---|---|---|
| Host and platform | `eiem.cpp`, `il2cpp_api.h`, `globals.h`, `init.h`, `applepie_mgr.h` | DLL entry, IL2CPP/Unity API resolution, hook installation and Unity-thread dispatch |
| Mod frontend | `eiem_mod_document.h`, `eiem_expression.h`, `eiem_mods.h`, `eiem_keys.h`, `eiem_persistent_state.h`, `eiem_mod_dispatcher.h` | INI parsing, expressions, variables, keys, persistence and update requests |
| Update coordinator | `eiem_mod_update.h` and the reconcile section of `il2cpp_trace.h` | Atomic Reconcile, Reapply and Reload transactions |
| Model lifecycle | `eiem_model_lifecycle.h`, `eiem_npc_model_owner.h` and lifecycle adapters in `il2cpp_trace.h` | Track world, UI and NPC model owners and instances |
| Render replacement | `eiem_resource_backend.h`, `eiem_skin_binding.h`, `eiem_render_state.h` and the executor in `il2cpp_trace.h` | Mesh, Material, Texture, bone palette, submesh visibility, restore and Unity ownership |
| Shape/Skeleton/Physics | `eiem_shape_*`, `eiem_skeleton_*`, `eiem_native_physics_*`, `eiem_physics_*` | Optional authoring features; Skeleton and Physics are not yet fully accepted in game |
| UI | `eiem_ui_host.h`, `eiem_lua_ui.h`, `gui.h` | Mod manager, Lua windows and legacy diagnostic pages |
| Camera fade | `eiem_camera_fade.h` plus its hook adapter | Independent camera transparency override |
| Diagnostics | `eiem_registration_trace.h`, `eiem_skin_probe.h`, `eiem_metadata_probe.h` | Bounded evidence only; never production state decisions |

## Rules

1. World, UI and NPC adapters call the same Render executor.
2. Bone lookup is always local to the current model instance. EIEMESH v5 source Mesh/slot metadata handles renamed bones without character-specific tables.
3. INI parsing cannot call Unity APIs. Unity mutation happens only in a verified lifecycle callback or an update transaction.
4. `partner.N` is rejected. Multiple visible parts are submeshes of one exported Mesh.
5. Diagnostics must have a bounded trigger and a removal condition.
6. Keyboard input is scoped to the selected Mod; manager buttons additionally identify one Key section.
7. Resource caches own generated Unity objects. Renderer override records own restoration responsibility.

The legacy Partner implementation still present in `il2cpp_trace.h` has no parser, executor, hook-installation or lifecycle entry point. It is quarantined compile-only code and must be deleted as part of the Render executor extraction; new work must not call it.
