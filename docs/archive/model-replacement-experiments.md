# 历史设计与实验记录（截至 v30）

本文保留当时的假设、实现与实验结果，不是当前规范。文内出现的 “Current” 仅表示当时状态；后续结论可能推翻前文。当前职责、格式与风险以 [模型替换设计](../model-replacement-design.md) 为准。

# EIEM Model Replacement Design

Status: extraction/browser, fresh runtime Mesh replacement, and open-world
Material/Texture replacement verified; EIEMESH v3 Blender round-trip
implemented; resource-identity replacement before the game cache commit is
implemented; the world/UI shared-resource regression run is pending

## Current Runtime Backend (2026-09-05)

Existing game resources are matched by their logical resource identity first.
A Mesh or Material declaration with `target.path` and `target.asset` replaces
that Unity resource at the common `Asset._FinishWithAsset` boundary, before the
game commits it to its resource cache. Every PFB, UI presentation hierarchy,
LOD or shadow consumer that later receives the same resource therefore receives
the same replacement without enumerating PFB variants or scanning the scene.

PFB/Render remains a separate assembly layer. It is used only when an operation
belongs to one live instance: disabling the source Renderer, assigning a local
material slot, or creating a partner Renderer. Fresh instances use
`PrefabInstantiateProxy.OnCompleted`; `BaseModelViewPart` and `CharUIModelMono`
remain lifecycle adapters for cached/presentation instances. They all feed one
Render executor. NPC-avatar hooks remain observation-only.

```ini
[MeshBody]
path=meshes/body.mesh
source=assets/.../s_actor_body_lod0.asset
asset=S_actor_body_lod0
target.path=assets/.../s_actor_body_lod0.asset
target.asset=S_actor_body_lod0

[PrefabCharacter]
path=assets/.../chr_0000_character_postmodel.prefab
render.0=RenderBody

[RenderBody]
asset=S_actor_body_lod0
mesh=MeshBody
material.0=MaterialBody
```

The roles are deliberately separate:

- `Mesh`, `Material`, `Texture` and `Skeleton` sections declare package
  resources. They do not discover scene objects. `target.path` plus the
  optional `target.asset` opts an existing game resource into global logical
  replacement.
- `Prefab.path` groups instance-local Render actions. Multiple mods may declare
  the same path; their declarations are applied in load order.
- `Render.asset` selects every Renderer in that PFB using the named Mesh
  asset. Optional `Render.path` narrows it to a relative Transform path.
  Optional `match.*` fields are additional guards, not primary identity.
- `Render.mesh` replaces the Mesh on the existing source Renderer.
  `handling=skip` independently controls the source Renderer enabled state.
  `partner.N` creates an additional Renderer owned by the same PFB instance.

When a target resource completes, or an instance-local Render action executes,
the backend performs the following work on Unity's thread:

1. Reads `EIEMESH` v2/v3 and validates vertices, normals, tangents, colours, eight
   UV channels, triangle submeshes, skin weights and bind poses.
2. Constructs a real `UnityEngine.Mesh` with the game's IL2CPP APIs and assigns
   it to the existing renderer. Reusing the renderer deliberately preserves the
   game-owned transform, Animator binding, bones/rootBone, LOD selection,
   culling and entity lifetime.
3. Reads each requested `EIEMMAT`, loads its `source=` Material through the
   already-running `BundleResourceManager`, clones it with `new Material(source)`,
   and applies exported `float.*`, `int.*`, `value4.*`,
   `texture_scale.*`, and `texture_offset.*` overrides before assigning
   `sharedMaterials`.
4. When the current game build exposes `Texture2D` plus
   `ImageConversion.LoadImage(Texture2D, Byte[], Boolean)`, reads a PNG declared
   by `texture.Property=TextureSection`, recreates the exported linear/mipmap
   and sampler settings, assigns it with `SetTexture`, and verifies the result
   through `GetTexture`.
5. For a `partner.N` record, creates one child GameObject/Renderer, copies the
   source Transform and SkinnedMeshRenderer bone references, and adds the new
   Renderer to every `LODGroup` level that already contains the source Renderer.
   On reload it removes that reference before destroying the partner object.
   If a build does not expose `LODGroup.GetLODs/SetLODs`, partner creation still
   succeeds and the limitation is logged.

Generated Mesh, Material and Texture objects are cached by the mod file,
resource section and file timestamp. Reconciliation therefore reuses unchanged
objects instead of rebuilding them for every Renderer or F10 press. When a
resource file changes, the old generated object for that section is released
after the previous Renderer state has been restored, then the new object is
created and rooted for the next pass.

PFB `Unload/Clear/Dispose`, BaseModelViewPart `ReleaseModel/OnRelease`, and UI
`UnloadModel/_Clear/Dispose` remove only the partners and saved Renderer state
owned by the affected model instance. UI
`Cancel` also releases the pending wrapped callback. F10 first restores saved
source state, reloads the configuration, then reapplies rules only to
registered live model instances; it never searches every Renderer in the
process. These F10 rules apply to instance-local Render state. A pure global
resource replacement is committed at load completion and affects subsequent
consumers of that cached object; changing or removing it after the game has
cached the object requires a later cache-invalidation feature and is not
silently emulated by a scene scan. A cached character model is handled at its native BaseModelViewPart
completion boundary. Later material-controller commits may reapply a rule only
when that Renderer was already bound by a supported lifecycle path. Mesh
setters and diagnostic NPC hooks cannot discover new rules.

AnimeStudio exports one Render action per unique Mesh resource, rather than
one per visible/shadow Renderer instance. Blender retains the PFB and Render
identity on that Mesh resource. A selected-Mesh export therefore produces a
minimal dependency closure plus the required PFB/Render records automatically.
On Blender import, an existing Mesh defaults its global target to its offline
`source`/`asset` identity. A changed existing Material does the same. New PNG
resources referenced by a Material remain dependency payloads and do not become
global Texture replacements unless explicitly targeted.
Blender exposes dedicated EIEM Material, Mesh and Texture panels instead of
requiring authors to edit the raw Custom Properties list. Shader texture paths
are the first Material fields, labels are shortened and left-aligned, and the
baseline/section-mapping JSON remains hidden bookkeeping data.
PFB/Render identity remains required for the authoring round trip and for
instance assembly, but global Mesh/Material delivery no longer depends on
enumerating every PFB that happens to consume the resource.

## Runtime experiment log

All dated implementation descriptions below are preserved as experimental
evidence. They are superseded whenever they conflict with **Current Runtime
Backend** above; in particular, proxy-return, `SubMeshInfo`, character-specific
and scene-wide discovery paths are no longer replacement boundaries.

### 2026-09-03: source-Mesh clone rejected

Test package: `MeshWulfaBodyLod0` points to `body_lod0_half.mesh`, containing
2451 vertices and 12207 indices. The Render rule matched the live Wulfa body
renderer and reported `applied=true`. With the clone-first implementation the
log contained:

```text
[DEBUG-mesh] ... vertices=2451 ...
[DEBUG-mesh] ... cloned=1
[DEBUG-mesh] Unity mesh result=... vertices=4902 ...
[DEBUG-mesh-state] expected boneWeights=2451 bindposes=99
[DEBUG-DRAW-STATE] ... enabled=true visible=false
```

The read-back vertex count remained the original 4902 instead of the payload
2451, and the character disappeared. This proves that the clone path did not
accept the managed channel writes as a replacement in Endfield's native mesh
cache. It is not a match failure: the rule matched and the renderer received a
different object, but that object still exposed the source layout.

Decision: construct a fresh `UnityEngine.Mesh`, write all payload channels,
bind poses and submeshes, recalculate bounds, upload the mesh, and verify the
read-back counts before considering the resource usable. Do not clone a source
Mesh. A future optimization may only be added after it preserves the exact
payload counts in the same live-build read-back check.

### Debugging invariants

Every replacement test must record, in order: rule match, payload counts,
construction mode, read-back vertex/index/submesh counts, skin-array counts,
renderer enabled state, and the final assignment result. `visible=false` is a
camera-time diagnostic and is not by itself proof of a failed assignment; a
count mismatch or a failed assignment is the actionable failure signal.

## Current offline browser implementation (2026-08-28)

The chosen inspection path is now a direct, indexed VFS browser inside the
normal AnimeStudio GUI. It does not load or decrypt the complete 53 GB VFS.

```text
Endfield .blc metadata + encrypted .chk ranges
    -> one-Bundle-at-a-time in-memory decrypt and Unity parse
    -> compact resource/CAB dependency index with checkpoint/resume
    -> unified virtual Container directory tree
    -> native AnimeStudio type filters and search
    -> user selects one resource
    -> read/decrypt its Bundle and transitive CAB dependencies on demand
    -> native Texture/Mesh/GameObject preview and selected export
```

The `.blc` parser and ChaCha20 implementation are local code and were verified
against the previous full extraction byte-for-byte. The completed direct scan
indexed 237,794 Bundles into 3,385,891 resource records and 237,794 CAB mappings
with zero failed Bundles. The compact `endfield_assets.eidx` is 702 MB, loads in
14.44 seconds and retains about 546 MiB of managed data. The old external XML
AssetMap and full plaintext `.ab` directory are no longer part of the Endfield
GUI workflow.

Prefab dependency traversal is included. Two real model Prefabs automatically
resolved closures of 13 and 19 Bundles. The export validation produced an FBX
with four meshes and seven PNG textures. This completes the VFS-to-Blender
extraction slice; Blender-to-runtime Bundle authoring is still a separate,
unfinished slice.

## 1. Objective

Add a runtime model replacement system for the Unity IL2CPP game without
modifying the original game installation. The plugin should redirect selected
game resources to user-provided assets and fall back to the original resource
when a replacement is unavailable or incompatible.

The first supported target is a same-skeleton visual replacement: preserve the
game character's Animator, Avatar, gameplay components, IK, cloth, colliders,
and entity lifecycle while replacing the rendered mesh and materials.

Endfield Mesh vertex buffers are authored in Z-up model space. Blender flips
their handedness for editing (`x,y,z` becomes `-x,y,z`) and reverses that flip
on export. Prefab Transform TRS is different: it remains in Unity's Y-up
space, so the Blender Armature view maps it as
`(x,y,z) -> (-x,-z,y)`. Source local TRS is retained for an unchanged export;
the runtime assigns the original-basis buffers and bind poses without another
coordinate rotation. The old `model-z-up-left-handed` marker is accepted as a
compatibility alias for packages written by the experimental exporter. The
runtime also verifies the payload's bone-name CRCs against the live
`SkinnedMeshRenderer.bones[]` hierarchy; it remaps only on a complete match
and leaves the source order untouched when the match is incomplete.

Whole-Prefab replacement is a later capability, not the first implementation
target.

## 2. Terms and asset relationships

### Prefab

A Prefab is a serialized Unity object graph or template. It may contain a
root GameObject, child Transforms, Components, Animator references,
SkinnedMeshRenderer components, material references, colliders, scripts, and
other serialized fields.

### Mesh

A Mesh is the geometry asset. For a skinned character it normally contains
vertices, indices, bind poses, bone weights, sub-meshes, and possibly blend
shape data. A Mesh does not contain the complete character behavior.

### Material and texture

Materials and textures are separate assets referenced by Renderer components or
by a Prefab. A material change does not inherently require changing the Prefab:

- Runtime assignment to `SkinnedMeshRenderer.materials` or `sharedMaterials`
  only changes the instantiated object.
- A persistent serialized change to the Prefab does require editing the Prefab
  or replacing the serialized asset that contains the reference.

The first implementation should keep the original Prefab and replace the
Renderer's Mesh/material references at runtime.

## 3. Proposed runtime architecture

```text
Game resource request
    -> game ResourceManager / Addressables / native resource API
    -> EIEM replacement decision
       -> replacement artifact, if present and compatible
       -> original resource on miss or failure
    -> selected replacement backend applies the result
```

AssetBundle is one possible replacement artifact, not a design prerequisite.
The actual artifact format must be selected only after observing the game's
resource loader. A path may identify a Bundle while the actual asset is
selected by an internal asset name, an Addressables key, a GUID/FileID pair,
or a catalog entry.

The plugin's core responsibility is to decide whether a replacement exists. It
must not embed character-specific asset authoring rules. The replacement
decision will eventually use:

- Game build identifier and compatibility rules
- Original resource key/path
- Replacement file or artifact path
- Replacement artifact type and identifier
- Fallback policy and diagnostic reason

The actual replacement mechanism is a separate backend. Possible backends are
file/path redirection, resource API redirection, or patching an already
instantiated Unity object. Backend-specific lifetime and dependency rules must
not leak into the replacement decision layer.

### Responsibility split

```text
Extractor / third-party tools:
  unpack and inspect original resources; export editable reference data

Authoring pipeline:
  modify the exported model; produce a replacement artifact

EIEM replacement decision:
  scan the replacement directory and decide whether a matching artifact exists

Replacement backend:
  perform the selected file, resource, or instantiated-object replacement
```

The term `PFB` must be defined before implementation. It may mean an editor
`.prefab` YAML export, a game's custom `.pfb` binary, or a logical prefab dump.
An editor Prefab export is not automatically loadable by a Unity player. It can
only be directly redirected if the game's runtime loader accepts that exact
format.

## 4. Discovery phase: IL2CPP dump layer

The first engineering task is metadata discovery, not resource replacement.

`src/il2cpp_dump.h` now runs once after the IL2CPP domain and assemblies are
available. It writes:

- `plugin/eiem_il2cpp_classes.txt`: all assemblies and type names
- `plugin/eiem_il2cpp_resource_dump.txt`: detailed candidate classes, fields,
  methods, parameter types, return types, `MethodInfo` addresses, and native
  method pointers

The candidate filter includes Unity and game types containing terms such as:
`AssetBundle`, `Addressable`, `ResourceManager`, `Resource`, `Catalog`,
`GameObject`, `Transform`, `SkinnedMeshRenderer`, `Mesh`, `Material`,
`Animator`, `Avatar`, `Character`, `Entity`, and `Visual`.

The dump is metadata-only. It does not instantiate objects or invoke managed
methods. This makes it suitable for the first run in a live game process.

`src/il2cpp_trace.h` began as the observation layer. When the corresponding
methods are present, it hooks `AssetBundle.LoadAsset`,
`AssetBundle.LoadAssetAsync`, `SkinnedMeshRenderer.set_sharedMesh`, and
`MeshFilter.set_sharedMesh`. Mesh replacement now enters at the logical
resource boundary: a declared Mesh is redirected from the proxy result before
the avatar creates its Renderer and custom skin cache. The
`SubMeshInfo.get_mesh/set_mesh` and avatar-construction hooks remain early
construction coverage for paths that bypass the proxy. Event-bound/F10 scene
reconciliation uses the same resource builder for cached models whose original
construction callback was not observed; it is not a separate fallback format.
The next build also observes
`FAssetProxyHandle.get_pathOrName`, `FAssetProxyHandle.Get`,
`FAssetProxyHandle.GetAssetProxy`, and
`FAssetProxyUntrackedHandle.Get`. Resource declarations with `target.path` can
now substitute the final compatible Unity object at those proxy boundaries;
unmatched requests remain pass-through.

After collecting the dump, the next investigation is to identify:

1. Whether the game uses direct AssetBundle APIs, Addressables, a custom
   resource manager, or a combination.
2. Which methods are synchronous and which are asynchronous.
3. The asset key/path and type passed through the load pipeline.
4. The point at which a loaded Prefab or Mesh becomes attached to a character.
5. Whether the game performs integrity checks or custom decryption before the
   Unity resource API is reached.

## 5. Historical hook strategy (superseded)

This section records the pre-PFB strategy that produced the skinning and
lifecycle evidence below. It is not the current runtime architecture.

For Meshes, use the earliest logical resource boundary that still preserves
the game's own construction pipeline:

```text
FAssetProxyHandle.Get()
    -> EIEM resolves target.path and builds replacement Mesh
    -> game receives replacement Mesh from its normal resource path
    -> SubMeshInfo / CreateSMS / AssignSkin consume that Mesh
    -> original game CreateSMS/AssignSkin pipeline
    -> game initializes bones, materials, LOD and renderer caches normally
```

`SubMeshInfo.get_mesh/set_mesh` and the pre-call avatar construction hooks are
kept as coverage for resources that do not pass through `FAssetProxyHandle`.
They must run before the original skin/cache initialization, never as a late
`sharedMesh` repair.

This is important for cached or prefab-embedded Meshes: they may never issue a
new file-load request. `ModelManager._OnGameObjectAllocate` is therefore the
common instance boundary for both newly instantiated and pooled GameObjects,
with `LoadFromPersistentPool` retaining the logical path when it is available.
The handle-based `BaseModelViewPart._OnLoadUseHandleFinish*` route is covered
separately because it bypasses `_OnLoadModelFinish*`.

F10 restores tracked source state, reloads the declarations, and is allowed to
reapply Mesh resources to already-live renderers. This is not a second Mesh
implementation: construction hooks and reconciliation both call the same
resolver, resource builder and assignment function. A loader hook remains
useful for Materials and Textures, while runtime hide/show remains a Renderer
directive.

Preferred order:

1. Game-specific resource manager methods discovered from metadata.
2. Addressables/ResourceManager load methods.
3. `AssetBundle.LoadAsset` and `LoadAssetAsync`.
4. File I/O redirection only as a last-resort diagnostic or compatibility
   layer.

The initial Hook should capture arguments and return values without changing
them. It should record the resource key, requested type, Bundle identity,
thread, and whether the operation is synchronous or asynchronous.

Only after the call chain is understood should the resolver return a
replacement asset.

### Current validation result (2026-08-27)

The first live observation pass confirmed that Endfield's common resource
entry point is the game-specific overload:

```text
BundleResourceManager._LoadAssetInternal(
    Beyond.Resource.StringPathHash, System.Type, RootCategory, immediate, priority)
    -> FAssetProxyHandle / asset proxy
```

During one normal startup, the hook observed 500 requests representing 267
distinct 64-bit hashes. Repeated hashes are expected cache/proxy reuse and are
not evidence that the underlying Bundle was read repeatedly.

This is evidence for a resource-manager/object-layer backend as the first
replacement candidate. It is not evidence that raw file interception is
impossible; the Bundle and decryption boundary has not yet been observed.

The observation runs confirmed that the runtime installs and executes the VFS
bundle hooks:

```text
VFS.VirtualFileSystem.LoadBundleFromFile(string)
VFS.VirtualFileSystem.LoadBundleFromFileAsync(string)
VFS.VirtualFileSystem.GetAssetStream(string)
```

It also observed repeated `FAssetProxyHandle.Get` and
`FAssetProxyHandle.GetAssetProxy` calls, while
`FAssetProxyHandle.get_pathOrName` and `Asset._FinishWithAsset` did not produce
runtime records in this session. The resource requests still enter through
`BundleResourceManager._LoadAssetInternal(StringPathHash, ...)` and resolve to
proxy objects. The replacement boundary is now explicit: `LoadBundleFromFile`
and its async overload check `plugin\\mods\\override\\<logical path>` for a
plaintext Unity AssetBundle. When present, the plugin calls Unity's
`AssetBundle.LoadFromFile` or `LoadFromFileAsync` on that file; a null result
falls back to the original encrypted VFS request. `GetAssetStream` remains
observation-only because redirecting it to a
filesystem path would not produce a valid VFS stream. The lower
`ReadFileByLowIO` method returns a large value-type handle and is intentionally
not used for replacement.

The run also confirmed a character mesh attachment directly:

```text
SkinnedMeshRenderer.set_sharedMesh
  renderer=S_npc_gentleman_glass_common_b_01_lod3
  mesh=S_npc_gentleman_glass_common_b_01_lod3
```

No replacement artifact was present, so no `[VFS-OVERRIDE]` record was
expected. End-to-end validation needs one valid plaintext replacement Bundle
at the exact logical path and one normal startup; a failed replacement must
fall back to the original path.

### Diagnostic result (2026-09-04)

The latest run loaded the mod configuration successfully, but its log contains
zero `[RES-REDIRECT]`, `[MOD-LOGICAL-*]`, or mesh-replacement records. The
recorded scene objects are environment assets and
`anm_5123_pigeon_postmodel`; it contains no Wulfa asset name. Therefore that
run proves only that the target Wulfa resource was not observed by the active
process, not that `body_lod0_half.mesh` failed to build. The hash-based loader
also returned `FAssetProxyHandle` values whose origin was recorded on the
heap proxy rather than on the short-lived value-type handle. The current fix
recovers `StringPathHash.get_path` and binds the origin through
`FAssetProxyHandle.GetAssetProxy()` before attempting a global Mesh redirect.

The current build captures the actual low-level read result instead of calling
`GetAssetBytes` after the fact. `FVFSUntrackedLowIOReadHandle.GetData`/
`GetDataThread` and the corresponding tracked-handle methods return a
`NativeArray<byte>` after the VFS has read and decrypted a block. The hooks copy
that native range unchanged into `plugin/captures/lowio/lowio_XXXX.bin` and
write a sidecar containing the physical `.chk` path, `fileNameHash`, offset,
encrypted flag, length, and a first-header classification (`UnityFS`,
`UnityRaw`, `UnityWeb`, or `custom`). F9 only stops the capture and reports
file/byte totals; it does not invoke IL2CPP or Unity APIs and therefore cannot
trigger the previous unknown-thread GC failure. Capture is bounded (4096 files,
64 MiB per read) and deduplicates the two data accessors for the same handle.

Because the startup trace showed Bundle reads through the low-level async path,
the build still observes both
`VFSFileReadStream.Read(byte[],int,int)` and the confirmed
`VFSFileReadStream.Read(Span<byte>)` overload. The Span hook uses the IL2CPP
Windows x64 value-type ABI (a reference to a 16-byte Span containing pointer
and length), validates the memory ranges, and captures the first read buffer
for up to 32 distinct stream instances (maximum 1 MiB each) into
`plugin/captures/span_XXX.bin`. These stream probes remain observation-only and
are not used as the bundle dump source. All probes call the original method
first and leave its return value and caller buffer unchanged.

### Diagnostic result (2026-09-04, v17)

The reported full native Wulfa mesh was consistent with a missing early
replacement, not proof that the half-mesh payload was rendered and ignored.
`NPCAvatarLodMeshAssets.GetSubMeshInfo` exposes Wulfa records with a logical
`meshName` while the `mesh` field is still null. The previous
`NPCAvatarUtils._GetPartCPUMesh` hook discarded exactly those records via
`if (!sourceMesh)`, leaving the later `SkinnedMeshRenderer.set_sharedMesh`
hook as the only remaining point, after the game's skin/GPU state was built.

v17 removes that source-pointer requirement. It resolves the Render rule from
the logical name, builds the replacement with a null template when necessary,
and writes it back through `SubMeshInfo.set_mesh` before the game's renderer
construction. Targeted `[DEBUG-LOGICAL-MESH]` records include the source
pointer and thread IDs so one run distinguishes no target record,
unsafe-thread deferral, resource-build failure, or successful logical
assignment. The change is generic for all Mesh rules; Wulfa is only a bounded
diagnostic log filter.

The previous `GetAssetBytes` batch dump produced 4096 files but none had a
UnityFS/UnityRaw/UnityWeb header; those files were VFS wrapper/table data, not
complete Unity bundles. It has been removed. The lowIO capture is now the
correct validation point because it runs on the successful decrypted read and
records the request metadata needed to correlate blocks with the source VFS
container. A `custom` header is expected until we confirm whether the game
stores complete bundles or compressed/encrypted blocks at this boundary.

### Diagnostic probe (2026-09-04, v18)

The existing runtime trace covered NPC Avatar construction and final Unity
renderer setters, but it had no observation point for the shared character
model path. v18 adds bounded, observation-only hooks for
`Beyond.Gameplay.View.ModelManager.Load/LoadAsync(string)`,
`BaseModelComponent.LoadMainModelSync/Async/OnMainPartLoadFinish`, and
`BaseModelViewPart.OnLoadFinish/PostDealLoadedModel`. The probe reads
`m_modelId`, `m_modelPath`, the part model and `BaseModelViewPartData.modelPath`
when those fields are available, and emits `[TRACE-CHAR-PATH]` and
`[TRACE-CHAR-FLOW]` records. It calls each original method first and does not
alter meshes, materials, lifecycle, or renderer state. One run with the target
character is sufficient to prove whether Wulfa uses this shared path; if no
records appear, the next investigation target is the game's separate ECS or
custom character construction path, not resource-file replacement. Because
`ComplexModelViewPart` overrides `PostDealLoadedModel`, v18 hooks that override
as well as the base implementation; otherwise virtual dispatch could bypass
the base probe entirely.

### Diagnostic result (2026-09-04, v18 runtime)

The v18 run confirms Wulfa's construction path. The log contains
`ModelManager.LoadAsync` for
`Assets/Beyond/DynamicAssets/Gameplay/Actors/PostModels/Characters/chr_0028_wulfa_postmodel.prefab`
followed by `ComplexModelViewPart.PostDealLoadedModel` and
`BaseModelViewPart.OnLoadFinish` for the instantiated Wulfa GameObject. The
same run contains no Wulfa record in the `NPCAvatar` mesh hooks and no
`MOD-LOGICAL-MESH` assignment. Therefore the earlier NPC-only logical mesh
replacement was never a valid Wulfa entry point; its successful `skip` effects
came from the separate late Renderer path. Wulfa's concrete mesh/renderers are
assembled inside the generic model-part completion path (or its custom
`EntityRenderHelper`) after the prefab is returned. The next hook must capture
the loaded GameObject's renderer arrays at that boundary, before the helper's
skin/GPU cache is finalized. Resource-file replacement remains deferred.

### Early replacement probe (2026-09-04, v19)

v19 moves the first real replacement attempt to the two concrete completion
methods exposed by `BaseModelViewPart`:
`_OnLoadModelFinishCallback(int, StringPathHash, GameObject)` and
`_OnLoadModelFinish(int, StringPathHash, GameObject)`. The detours receive the
instantiated prefab object directly, walk its bounded Transform hierarchy, and
inspect every `SkinnedMeshRenderer` before and after the original completion
method. A renderer is eligible only when its source Mesh name resolves to an
explicit `Render` rule; no name guessing or global replacement is performed.
The generated Mesh is assigned through the same Unity setter used by the game,
then the original callback continues so the game's own skin/GPU setup can read
the replacement. This keeps `handling=skip` independent from `mesh=` and avoids
the old NPC-only path.

The deployed v19 build is a runtime probe, not yet a success claim. The next
single game run should provide `[TRACE-CHAR-RENDER]` and
`[TRACE-CHAR-RENDER-SUMMARY]` records for the Wulfa prefab. `matched=0` means
the source Mesh identity does not equal the configured `asset` and the match
key must be corrected; `matched>0` with `safe=0` means the callback runs before
the plugin has recorded the Unity thread and the thread gate must be fixed;
`matched>0` with a mesh-build failure points to the payload or Unity Mesh write
path. Only after these records are present should visual deformation or skin
palette issues be investigated.

The first v19 run showed the callback boundary itself was still too early:
`_OnLoadModelFinish` received Wulfa's root object, but its hierarchy contained
no `SkinnedMeshRenderer` yet (`visited=512, renderers=0`). At the same time,
the later scene reconciliation reported `matched=1, applied=0`; this is
intentional because that pass calls the rule with `allowMeshReplacement=false`.
The follow-up build therefore runs the same inspection after
`PostDealLoadedModel` and `OnLoadFinish`, and uses Unity's
`GameObject.GetComponentsInChildren(Type, true)` before falling back to a
bounded hierarchy walk. This removes the 512-node blind spot and places the
assignment after prefab components exist but before any future late repair pass.

### Cached-model coverage and incremental authoring export (2026-09-04)

The first Typhoea material/Mesh package was present in-game, but the trace had
no Typhoea lifecycle or target-candidate records. This was a monitoring and
entry-point defect, not evidence that the character was absent:

- the single character-flow counter stopped at exactly 320 records (256 flow
  and 64 path records), so later model loads were silent;
- renderer diagnostics were hard-coded to one earlier character or to an
  already-successful full rule match, hiding configured targets that failed
  identity/shape resolution;
- only string-path and fresh `_OnLoadModelFinish*` routes were covered, while
  `ModelManager` exposes a persistent pool and `BaseModelViewPart` exposes a
  distinct handle-reuse completion route;
- reconciliation explicitly passed `allowMeshReplacement=false`, so F10 could
  restore a source Mesh but could not mount the newly reloaded Mesh.

The runtime now applies the same resource rule at
`ModelManager._OnGameObjectAllocate`, after `LoadFromPersistentPool`, and after
both handle-reuse completion methods. Configured Mesh names receive uncapped
candidate/mismatch records without character-name special cases. F10 passes
through the normal Mesh assignment path after the generation restore. Static
contract tests require all of these invariants, and the complete DLL builds
with the exact metadata-selected overloads. Live Typhoea validation remains
required before this coverage change is called end-to-end verified.

Blender export is now an incremental dependency closure rooted at selected
EIEM Mesh objects:

```text
selected Mesh
    -> its Render rule and Mesh payload
    -> only a changed/reassigned Material clone
    -> only Texture resources referenced by changed texture properties
    -> Skeleton only when the Armature is explicitly selected
```

Material import records an authoring baseline. Exported EIEMMAT files contain
the original `source=` material and only properties changed from that baseline;
the runtime clones the game Material, so omitted properties continue to come
from the game. This avoids copying every original shader parameter and PNG.
The Blender 5.0 background regression imported all 62 Typhoea Mesh resources,
edited one cloth Mesh and its `_BaseMap`, then produced exactly one Mesh, one
Material, one Texture, zero Skeletons, and one Render rule. The full untouched
geometry/channel round trip still validates all 62 Meshes and the single shared
skeleton while correctly omitting unchanged Materials and Textures.

## 6. Packaging and unpacking assumptions

Unpacking the original model is useful for discovering:

- Skeleton and bone names
- Mesh/sub-mesh layout
- Material and shader requirements
- BlendShape names
- Prefab hierarchy and component dependencies
- Bundle and Addressables relationships

Extraction does not automatically produce a Bundle that Unity can load again.
Rebuilding may require matching the game's Unity version, platform, asset
serialization version, compression mode, shader setup, and dependency graph.

Possible obstacles include:

- Encrypted or custom-wrapped Bundle files
- LZ4/LZMA compression and block metadata
- Addressables catalogs and hash files
- GUID/FileID references across Bundles
- Runtime decryption in a game-specific loader
- Version-specific Unity serialization
- Asset integrity checks or signatures

The authoring pipeline is intentionally separate from `eiem.dll`. The target
workflow is a round-trip resource workspace: decrypt and unpack the source
game data, import a versioned dump into a Blender add-on, edit it, then build a
replacement artifact for the runtime loader. Third-party parsers may be used
inside the offline pipeline, but they are not runtime plugin dependencies.

### Better-Endfield comparison

Better-Endfield confirms a useful architecture boundary. Its host dynamically
resolves IL2CPP classes and methods by assembly, namespace, type, signature,
and field descriptors, while a HookBroker owns hook creation, conflict
detection, and per-module release. Its model module consumes a generated
resource manifest containing a logical asset path, a `pathHash`, and the
corresponding `bundleHash`; it then calls the game's
`I18NAssetLoader.Load(hash, type, category)` for a configured login actor.

That is valuable for our host/runtime design, but it is not a generic model
replacement backend. The model module still contains a feature-specific
contract, login-scene discovery, prefab cloning, Animator/PlayableGraph
setup, and explicit character configuration. We should reuse the principles:

- resolve IL2CPP methods by metadata descriptors instead of fixed addresses;
- centralize Hook ownership and release so shutdown does not deadlock;
- generate path/hash/bundle indexes offline and keep them outside the DLL;
- let a missing or incompatible contract disable one feature only.

We should not copy its login actor logic or make the replacement engine depend
on character IDs. Our plugin's generic path is:

```text
VFS extractor -> logical files and manifest/index -> external Unity tools
              -> edited replacement Bundle at the same logical path
              -> EIEM LoadBundleFromFile hook -> original VFS fallback
```

The repository includes `tools/extract_endfield_vfs.ps1` as an orchestration
script. It invokes the current EndfieldUnpacker Python extractor, processes
the base `StreamingAssets` VFS, and writes an extraction report. It does not
put VFS decryption or Unity serialization code into `eiem.dll`.

During validation, the early FkArkEnd `Beyond.VFS` build failed with
`EndOfStreamException` on the installed client's small version-3 BLC files.
The current `endGuaGua/EndfieldUnpacker` Python implementation succeeds on the
same installation: its dry run reports `code_ver=4`, identifies the
`Bundle`/`Audio` layers, and enumerates real paths such as
`Bundles/Windows/initial/<hash>.ab`. This establishes the correct extraction
tool and explains why the earlier C# tool must not be used as the baseline.

The resulting `.ab` files are still Endfield's custom Unity bundle wrapper.
They require the matching AnimeStudio CLI (or an equivalent version-aware
decoder) before Unity objects can be consumed by the authoring pipeline. The
extraction pipeline is therefore:

```text
VFS .blc/.chk --EndfieldUnpacker--> logical .ab files
logical .ab --AnimeStudio--> parsed Unity objects + source metadata
parsed objects --EIEM Blender add-on--> versioned authoring dump
authoring dump --Blender--> edited authoring dump
edited dump --bundle builder--> loadable replacement artifact
replacement artifact --> plugin/mods/override/<same logical path>
```

AssetRipper/UABEA are optional inspection backends in this pipeline. They are
not the authoring format and are not assumed to provide a working Endfield
round trip. The Blender add-on and runtime plugin will consume the same
EIEM-owned resource files. They retain the data needed for round-trip editing:
prefab hierarchy, transforms, mesh buffers and submeshes, original normals and
tangents, bind poses and weights, blend shapes, skeleton metadata, material and
texture references, and physics metadata where the source parser exposes it.

User replacements can be stored in whatever artifact the selected backend can
consume. A custom AssetBundle is one option, but it is not required. A custom
PFB file is only valid if the game or a replacement backend can deserialize it.

## 7. Replacement levels

### Level 1: Renderer patch

Keep the original Prefab and character hierarchy. Replace the existing
`SkinnedMeshRenderer` Mesh and materials after the model is instantiated.

Requirements:

- Compatible bone count and ordering, or an explicit bone remapping table
- Compatible bind poses and weights
- Compatible shader/material properties
- BlendShape name/index mapping where facial animation is required

Expected result: visual model replacement with original gameplay behavior.

### Level 2: Asset load redirection

When the game requests a known Mesh or Material asset, return or apply the
equivalent replacement artifact. The original Prefab remains unchanged.

Expected result: replacement happens at the resource boundary and survives
character recreation, provided the selected backend covers all load paths.

### Level 3: Prefab replacement

Return a replacement Prefab or instantiate a replacement GameObject. This may
require reattaching Animator, gameplay scripts, IK, colliders, cloth, and
entity-specific references.

Expected result: complete visual hierarchy replacement, with substantially
higher compatibility risk.

## 8. Authoring package and configuration (agreed v1)

The package contains one generated `mod.ini` and one EIEM-owned set of resource
files. The offline exporter, Blender add-on, and runtime plugin read and write
this exact structure. FBX is not part of the authoring or runtime format.

```text
mod/
  mod.ini
  meshes/<name>.mesh
  skeletons/<name>.skeleton
  materials/<name>.mat
  textures/<name>.png
```

`mod.ini` uses EFMI-like section/key/value syntax. Blender generates it, while
advanced users may edit it. There are two deliberately separate concerns:

1. `Texture`, `Material`, and `Mesh` declare reusable resources.
2. `Render` finds a scene Renderer and organises an operation using those
   resources.

Resource declarations without a target are inert payloads for `Render` rules.
When a declaration includes `target.path` (and optionally `target.asset`), it
also opts into global logical resource redirection. The declaration still does
not control drawing; `handling=skip` remains a `Render` operation.

### 8.1 Resource files

`*.mesh` is one Unity Mesh-equivalent resource. It must preserve every source
field required for a faithful round trip: positions, original normals and
tangents, every UV channel, vertex colours, index buffers, submeshes, skin
weights, bind poses, blend shapes, and the submesh-to-material-slot mapping.
The writer chooses 16- or 32-bit indices according to the data. It records the
source coordinate marker; Blender applies the documented X-axis handedness
flip to the already Z-up Mesh channels and reverses it on export, so a round
trip does not change axes, scale, winding, normals, or tangents. This Mesh
conversion must not be reused verbatim for prefab Transform TRS, which is
Y-up.

The first binary layout is little-endian and versioned. It begins with
`EIEMESH\0`, version, coordinate-space marker, source logical path and name;
it then writes vertex arrays, eight independent UV arrays, normalized triangle
indices, submesh index ranges, four-weight skinning data, bind poses, bone-name
hashes, and complete blend-shape data. Submesh ranges address the normalized
triangle index list, never Unity's original byte offsets. A reader must reject
an unknown version instead of guessing its layout.

`*.skeleton` is the same file on import and export. It records the complete
prefab Transform hierarchy, relative paths, parent relationships and local
TRS. Renderer-local bone palettes and bind poses belong to each `*.mesh`, not
to this shared hierarchy. An author may elect not to export a skeleton and thereby inherit the
original game's bones unchanged. If a skeleton is exported and modified, the
runtime must really consume it; adding or removing bones is not silently
claimed as supported until Animator, Avatar and physics compatibility are
implemented.

Its first binary layout starts with `EIESKEL\0`, version and coordinate-space
marker, followed by the union of every `SkinnedMeshRenderer.bones[]` Transform
and the ancestors required to preserve that hierarchy. Renderer, LODGroup and
shadow-proxy GameObjects are not bones and are excluded. Renderer-local palette
order remains in each EIEMESH through exact bone paths, including palette
entries that currently have zero vertex weight. This mirrors Unity's actual
binding relation: Mesh weights index `SkinnedMeshRenderer.bones[]`; a Mesh does
not directly refer to a standalone skeleton asset.

`*.mat` is one material authoring file. Its source field names the original
game Material by logical resource path. The current offline package records the
complete serialized saved-property snapshot with its real property names and
types (`Texture`, `Float`, `Range`, `Color`/`Vector`, and integer where exposed).
The serialized `*.mat` values refer to declared texture resources. In Blender,
those texture properties are deliberately placed first in Material Custom
Properties and display editable absolute image paths instead of internal INI
section names. On export the add-on reuses the matching Texture section, or
creates one for a newly entered image path and copies the source slot's colour,
mipmap and sampler metadata. Runtime construction first clones the source
Material, so shader, keywords and properties not represented by the authoring
file remain inherited.

Textures are external PNG files for the first version. AnimeStudio records the
source texture's linear/sRGB choice, mipmap flag, filter mode, wrap mode,
anisotropy and mip bias. The runtime recreates those settings around Unity's
`ImageConversion.LoadImage` result; texture scale and offset remain Material
properties. Users do not need to author BC5, BC7 or DDS files.

### 8.2 INI syntax

```ini
[TextureWulfaBody]
path=textures/wulfa_body.png

[MaterialWulfaBody]
path=materials/wulfa_body.mat

[MeshWulfaBodyLod0]
path=meshes/wulfa_body_lod0.mesh
source=assets/beyond/arts/entity/actor/loli/wulfa/models/s_actor_wulfa_body_01_lod0.asset
asset=S_actor_wulfa_body_01_lod0

[PrefabWulfa]
path=assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0028_wulfa_postmodel.prefab
render.0=RenderWulfaBodyLod0

[RenderWulfaBodyLod0]
asset=S_actor_wulfa_body_01_lod0
mesh=MeshWulfaBodyLod0
material.0=MaterialWulfaBody
partner.0=RenderWulfaShoe
```

`source` and `asset` on a resource section are offline provenance. They help
Blender display what was exported but do not cause replacement. An explicitly
authored `target.path` opts that one resource into global loader-level
redirection. AnimeStudio and Blender never infer it from `source`.

`Prefab.path` is the exact logical PFB identity returned by the game. Its
`render.N` entries define the actions scoped to each live instance. One Mesh
resource shared by visible, shadow or other consumers produces one Render
action; the runtime applies an asset-only selector to every matching Renderer
under the PFB root. Add `Render.path` only when the operation must be narrowed
to one relative Transform path. If both `path` and `asset` are present, both
must match. `match.vertices`, `match.indices` and `match.submeshes` are optional
additional guards and are not exported by default.

The section name inside `[]` uses plain identifiers without dots. Dots are
reserved for indexed keys such as `render.0`, `material.0` and `partner.0`.

The `.mat` file referenced above can be represented as simple typed text while
the final parser is implemented, for example:

```ini
source=assets/beyond/.../M_actor_other_body
texture._BaseMap=TextureWulfaBody
color._Color=1,1,1,1
float._Metallic=0.2
```

The source Material is always loaded by its game logical path and then copied.
There is no `from=original` branch and no material source chosen from only the
currently visible scene. This permits using a material from an unloaded
character.

Submesh/material assembly is declared by `Render`, while Mesh, Material and
Texture sections remain resource declarations. A Render is evaluated only
inside the PFB declarations that reference it. A separate Render rule is used
when LODs or relative hierarchy paths need different edits.

The operation set is intentionally small:

```text
mesh present                     assign the named Mesh to the matched source Renderer
handling=skip                    disable the matched source Renderer
partner.N                        create an additional Renderer from that Render record
neither                          leave the matched source Renderer unchanged
```

`mesh` and `handling=skip` are independent. With `mesh` and no
`handling=skip`, the named Mesh is assigned to the matched source Renderer.
With both present, the named Mesh is assigned first and the same source
Renderer is then disabled. No implicit replacement or partner Renderer is
created; use an explicit `partner.N` entry when an additional draw is wanted.
`handling=skip` without `mesh` only disables the matched source Renderer.
`partner.N` always means an additional Renderer; it never replaces the source
Renderer and it never performs another match. There is no `clone`, `override`,
implicit duplication, polling reload, or condition language in v1. Resource
declarations remain separate from the Render record that organizes them.

When `mesh` is present on the matching Render, the plugin assigns one cached
Mesh resource to the existing source Renderer. The existing `bones[]`,
`rootBone`, Animator, LOD and physics remain owned by the game. Omitted
material slots retain the source Renderer values. For each `partner.N`, the
plugin creates a separate Renderer, copies the source Transform's local
position, rotation and scale, plus bones,
`rootBone`, relevant Renderer settings and LOD membership, then applies the
partner's Mesh and material slots. Partner objects are tracked and destroyed
or recreated together with their owning PFB instance.

### 8.3 LOD and Blender contract

Each distinct source LOD Mesh remains an independent resource and Render rule.
Repeated Renderer instances that reference the same Mesh do not create extra
Blender objects or package records. Users may intentionally reuse one edited
Mesh across several LOD rules, but the exporter never does so implicitly.

The Blender add-on presents the original Mesh resource path, LOD,
material slots, material source paths, shader parameter names/types and texture
bindings as an explicit editing workflow. It must not require a user to inspect
Bundle hashes or maintain hidden metadata. The exact Blender UI may use
dedicated panels, collections or helper objects, but it must remain visible and
low-friction; that UI is designed separately from this file format.

The runtime reload is manual: a configured hotkey schedules reload on the game
thread, removes objects/resources created by the previous application, reads
the changed package, and reapplies its rules. It does not poll the filesystem.

## 9. Milestones and acceptance criteria

### Dump UI contract (agreed)

The in-game Ins panel exposes exactly two dump actions:

- `Dump Current`: dump the selected Mesh and its complete dependency closure.
- `Full Dump`: dump every Mesh/Renderer resource reachable from the current
  scene, including inactive LOD objects and their dependency closures.

Both actions include all resolvable LOD levels. `Full Dump` means the current
scene and its serialized/resource dependencies; it does not scan or decrypt the
entire VFS. The user chooses the output directory. The dump writes a readable
directory tree plus a machine-readable `manifest.json` containing the game
build, logical paths, type, PathID, source Bundle, renderer path, LOD,
SubMesh/material-slot mapping, and unresolved references.

The Mesh list has an independent `Highlight` checkbox. Highlighting is
temporary and applies only to Mesh users in the scene; it must be removed on
uncheck, scene unload, object destruction, or plugin shutdown. It must not
mutate the original Material or Shader.

The first implementation must preserve the distinction between an asset file
and the objects serialized inside it. A Prefab is one file in the dump tree;
its contained Meshes and references are represented in `manifest.json` and are
not emitted as duplicate virtual files.

Known implementation boundaries:

- inactive LODs that are present in the instantiated hierarchy can be dumped;
  LODs held only in an unresolved/unloaded external asset must be reported as
  unresolved rather than silently omitted;
- true 3D outline rendering is a separate renderer task and must not block the
  first metadata/export pass;
- Unity/IL2CPP object enumeration and object creation run on the game main
  thread; disk serialization runs off-thread after data has been copied;
- a failed dependency or unsupported object type falls back to the original
  game object and is recorded in the manifest/report.

### M0: Metadata discovery

- Dump is generated on game startup.
- Resource-related classes and methods are present in the dump.
- No managed calls or model changes occur during discovery.

### M1: Resource observation

- Log one complete original model load path.
- Identify sync/async behavior and requested asset types.
- Confirm whether the game uses encryption or a custom resource layer.

### M2: Same-skeleton renderer replacement

- Replace one known character Mesh and material.
- Original Animator, IK, cloth, and gameplay remain active.
- Stop/reload/character-switch restores or reapplies the correct state.

Earlier experiments modified NPC-specific `SubMeshInfo` and CreateSMS stages.
Those hooks proved useful for understanding the custom skinning pipeline, but
they are no longer replacement boundaries. The verified fresh Mesh builder is
now invoked from the common PFB-scoped Render action. NPC-specific hooks may
remain diagnostic only and cannot mutate the replacement state.

### M3: Resource or object redirection

- Redirect or patch one known asset using the backend selected from M1.
- Keep dependencies and replacement artifact lifetime valid where applicable.
- Fall back cleanly on load or type mismatch.

Explicit resource declarations with `target.path` may perform object-level
redirect in `FAssetProxyHandle.Get` and `FAssetProxyUntrackedHandle.Get`.
Normal model packages do not generate this field: their Mesh, Material and
Texture resources are assembled by PFB-scoped Render actions. The resource
manager and VFS remain pass-through for identity and decryption.

### M4: Multi-profile and versioning

- Separate replacement profiles from code.
- Detect unsupported game builds.
- Disable only the incompatible profile instead of installing unsafe hooks.

## 10. Current limitations and risks

- Shutdown handling now sets a process-wide exit flag, restores the game
  window procedure before forwarding close messages, wakes the GUI thread, and
  disables MinHook asynchronously after the original close callback returns.
  The new build still needs one normal in-game exit verification before any
  replacement work is considered stable.
- IL2CPP metadata and generated method layouts are not a stable public ABI.
- Unity object creation and resource APIs generally need the Unity main thread.
- Existing EIEM hooks and face/IK logic may overwrite replacement transforms.
- The game may use custom native resource code that is invisible to managed
  type metadata.
- Game Terms of Service and account risk remain outside the technical design.

## 11. Implemented model dump v1

The in-game Dump tab now keeps the two agreed actions. `Dump Current` writes a
reference manifest for the checked, deduplicated Mesh objects; `Full Dump`
writes every Mesh observed in the current scene. These actions do not read
vertex buffers or instantiate assets. They serialize runtime names, hierarchy,
LOD and identity metadata plus the observed Bundle list; the offline
AnimeStudio index resolves the actual files.

The runtime reference dump contains:

```text
plugin/dumps/
  scene_dump_current.json
  scene_dump_full.json
```

Schema 2 records include `lookupType`, `lookupName`, `hierarchy`, `lod`, and a
process-independent `identityHash`. They also snapshot the selected
Renderer's material slots, shader names, texture-property-to-texture bindings,
and, for `SkinnedMeshRenderer`, the bone names and root bone. `lookupName` and
the dependency names are selectors for the offline index; native object
pointers remain diagnostic only and are never used as persistent identity.

The resource-load hooks connect `_LoadAssetInternal` proxy handles and
`Asset._FinishWithAsset` completions to the resulting Unity object. When this
evidence is available, Mesh, Material and Texture records also carry
`runtimePathHash`, `runtimePath`, and a logical `container`. The mapping uses a
fixed-size four-way pointer table: it allocates nothing in resource hooks and
does not grow with play time. A resource that cannot be tied to a logical path
is explicitly marked `resolution: name-only`.

AnimeStudio's `Export EIEM from JSON...` action resolves the Mesh, Material and
Texture selectors from the compact Endfield index, loads each required source
Bundle only once, and exports the actual resources. Resolution performs one
pass over the compact index rather than one full scan per dependency. The
exported Material JSON supplies serialized shader/material parameters; the
runtime plugin does not guess or duplicate those values. If a name-only
selector matches multiple indexed resources, AnimeStudio reports it as
ambiguous and skips it instead of silently exporting an arbitrary same-named
asset.

The older geometry exporter is retained only as an internal validation tool;
it is not used by the Dump-tab buttons because runtime vertex reads were
already shown to be unreliable for this client. Texture pixels and serialized
material parameter values are resolved from the offline Bundle index instead
 of guessed from native memory. Complete prefab, animation and cloth/physics
dependency traversal remains a later layer; it must not be presented as part
of this Mesh/Material/Texture milestone.

## 12. Runtime skinning diagnosis (2026-09-04)

The fresh EIEM Mesh constructor produced the expected payload sizes
(`2451` vertices, `1` submesh, `12207` indices, `2451` weights and `99`
bindposes). Static comparison of the source and half-mesh payloads found that
the retained vertices preserve their original weights, bindposes and bone hash
order. The exported skeleton has 523 transform nodes and a compact 99-bone
renderer palette; all 99 payload hashes match the complete transform paths in
that palette in the same order. Therefore the half-mesh file is not currently
known to contain a bad weight or bindpose.

The live log showed `DEBUG-SKIN-PALETTE status=identity exact=0 suffix=99`.
The runtime check now resolves complete paths first, then uses suffixes only
when they are unique and explicitly records the first twelve
payload-index-to-live-bone mappings. A zero exact-match count is expected when
the runtime hierarchy has a different instance root; it is not a defect by
itself. The decisive checks are unique suffixes, the expected palette count,
and an unchanged index mapping. An ambiguous suffix or a non-identity mapping
must be treated as a real skinning error rather than guessed through.

The runtime now also observes `SkinnedMeshRenderer.set_bones` only for tracked
replacement instances. The hook forwards the setter unchanged and records the
final palette count and first entries as `DEBUG-SKIN-BONES`; it is diagnostic
only and does not rewrite the game's bone array.

The same log showed every `CreateSMSGO`/`AssignSkin` boundary with
`prepared=0`; the old implementation therefore attached the replacement only
from the late `SkinnedMeshRenderer.sharedMesh` reconciliation path, after
Endfield's own skin/GPU state had been initialized. That path has now been
removed for Meshes. The replacement is first attempted at the proxy resource
return, with the logical SubMeshInfo/construction hooks covering direct cached
asset paths. No weight or coordinate conversion should be changed until the
early resource boundary is observed and its input Mesh is confirmed.

### Skin palette preservation rule

Reducing a mesh's vertex count must never reduce its skin palette. The export
must preserve the complete `bindposes` and bone-identity table from the source
mesh, including bones currently referenced only by zero-weight vertices (or not
referenced by the reduced vertex subset). Vertex weights may be filtered with
the retained vertices, but palette entries remain in their original order so
the game's per-part indices still address the same transforms. Blender cleanup
must therefore not remove apparently unused vertex groups or reorder them.

This is especially important for Endfield's custom path: the normal Unity
`SkinnedMeshRenderer.bones[]` array is only one input. The game also has
per-part `VirtualMeshBoneWeight`, `skinBoneTransformIndices`, and
`skinBoneBindPoses` buffers, populated before the Burst skinning kernels run.
Replacing the public Mesh after those buffers are built can leave the source
geometry's metadata paired with the replacement vertices, producing the
observed twist. The replacement must enter before that custom import/cache
step, or explicitly rebuild the same per-part buffers; CPU-side pre-skinned
geometry is not equivalent because it bypasses the game's animation and
rendering path.

### v20 single-run discriminator

The current visual result is animated but twisted. That proves the replacement
is reaching a skinning path; it does not prove that the payload palette matches
the live source Mesh. The next probe compares the retained payload directly
against the runtime source Mesh before assignment and against the generated
Mesh after construction. It reports positions, each vertex's four floating
weights, each vertex's four integer bone indices, and all bind-pose matrices
under the unique prefix `DEBUG-SKIN-DIFF`. It also fingerprints the 99 live
Transform references and their `localToWorldMatrix` values before construction
and after assignment.

Only two outcomes are actionable:

1. Any source comparison differs: the offline extraction or exchange-format
   conversion is wrong, and the first differing channel identifies the format
   defect.
2. Source and replacement comparisons are exact while Transform references
   stay unchanged: the public Unity Mesh data is correct, so the distortion is
   caused by Endfield's already-built per-part GPU skin cache. The next change
   must move replacement before that cache import or rebuild the single
   confirmed cache owner; changing weights, bind poses, coordinates, or bone
   names would be unjustified.

There is deliberately no fallback behavior in this probe. One target run must
decide between payload corruption and replacement timing.

### v20 observed result

The target run proved that the reduced Mesh is constructed and remains bound
to the intended renderer. Its 2,451 weights and 99 bind poses read back exactly
after construction. The renderer's 99 `bones[]` references and their animated
`localToWorldMatrix` fingerprint also remain unchanged across assignment, and
the payload bone-hash palette resolves to the same live indices 0 through 98.

The only mismatch is between the offline payload and the original runtime
Mesh's public `boneWeights` and `bindposes`: all compared entries differ. The
source vertex array is not readable, so it cannot be used as evidence either
way. This rules out setter failure, replacement loss, and mutation of the live
Transform array, but does not yet distinguish a C++ value-type layout error
from a load-time skin-table transformation performed by the game.

The v21 probe therefore logs the actual IL2CPP field offsets and value sizes
for `UnityEngine.BoneWeight` and `UnityEngine.Matrix4x4`, plus the first three
weights and first bind pose from both the offline payload and runtime Mesh. It
also tests whether the bind-pose difference is merely a matrix transpose. No
runtime behavior or fallback is changed by this probe.

### v21 result and v22 correction

The runtime metadata reports `BoneWeight` as 32 bytes with four floats followed
by four 32-bit indices, and `Matrix4x4` as 64 bytes in Unity's managed field
order. These layouts exactly match the native EIEM structs. The live renderer
bone palette is also identity-mapped, so neither struct packing nor bone-index
remapping caused the distortion.

The original Mesh's first bind pose is the exact transpose of the offline
payload, and the same relation holds for all 99 matrices with zero error. The
AnimeStudio export boundary was therefore writing Unity's serialized asset
matrix order directly into the managed `Mesh.bindposes` order. Version 2 of
the mesh exchange format transposes bind poses once in AnimeStudio and stores
the canonical managed/runtime order. Blender preserves that canonical array
unchanged. The runtime accepts v2 and v3; both use the corrected matrix order,
while v3 adds authoring-only shared-skeleton palette paths.

The original Mesh's legacy `boneWeights` getter returns 4,902 zero-filled
entries in this title, while the raw vertex stream and replacement readback
contain the expected non-zero weights. This is evidence that Endfield's source
asset uses a newer/compressed weight path; it is not a reason to copy those
zero values into the replacement. The v22 correction changes only bind-pose
ordering and retains the payload weights and local bone indices.

### EIEMESH v3 Blender resource model

The authoring pipeline imports resources, not instantiated renderer copies.
One unique Prefab transform hierarchy becomes one Blender Armature. A Mesh's
compact bone indices do not identify separate skeletons: they address a local
palette whose entries point into that shared hierarchy. EIEMESH v3 therefore
stores those palette entries as transform paths alongside the existing bone
hashes and bind poses. This is required because some exported game bone hashes
cannot be reconstructed reliably from Prefab node names. EIEMESH v2 remains
readable by migrating its exact renderer palette during import; all new
AnimeStudio and Blender exports use v3.

Blender import maps source channels as follows:

- UV0 through UV7 become Blender UV layers. Three- and four-component UVs keep
  XY in the UV layer and Z/W in a named point-domain Mesh Attribute.
- Vertex colours become a point-domain Color Attribute named `Color`.
- BlendShape channels and frames become editable Shape Keys; source normal and
  tangent deltas use named point-domain Mesh Attributes because Blender has no
  native editable container for those deltas.
- Source normals are assigned to Blender's native per-corner custom-normal
  channel and every imported face uses smooth shading. Because the X-axis
  handedness reflection also reverses face orientation, import reverses each
  triangle's winding and export reverses it back; omitting this step makes
  Blender clamp otherwise valid authored normals against backwards faces.
- Blender encodes custom normals in a face-fan-relative representation, whose
  decoded values may be slightly quantized. `EIEM_SourceNormal` is therefore a
  lossless point-domain backup used only while a checksum proves that the
  native normal state is untouched. Once the user edits Blender's native
  normals, export writes the evaluated edited normals instead. EIEMESH has one
  normal per vertex, so a deliberate per-corner normal split must first split
  that vertex or export fails explicitly.
- Tangents remain named point-domain Mesh Attributes because Blender does not
  provide an equivalent authorable tangent channel; they are not used as a
  substitute for viewport normal shading.

Export reads those same locations and rejects ambiguous per-corner UV or colour
seams instead of silently choosing one value for an EIEM per-vertex channel.
Meshes are grouped into LOD collections by their resource identity, while the
single shared Armature remains at the package root. A Typhoea round-trip test
currently covers 62 Mesh resources, 14 multi-UV Meshes, three tangent-bearing
Meshes, two BlendShape-bearing Meshes and one unique skeleton hierarchy.

### Blender skeleton-space diagnosis (2026-09-04)

The first shared-Armature importer composed raw prefab Transform TRS directly
as Blender edit-bone matrices. That put the biped along Blender Y while the
Mesh was already upright on Blender Z, so the Armature visibly lay on its
back. This was a real resource-space mismatch, not an octahedral-display or
bone-roll artifact.

The deterministic check compares named biped bones against the skinned Mesh
space. Before the fix, the head was approximately `(0, 1.269, -0.011)`; after
the Unity-Transform conversion it is `(0, 0.011, 1.269)`. Left and right thigh
bones also align with their weighted vertex sides. The importer now conjugates
the complete composed Transform matrix by the Unity-Y-up to Blender-Z-up basis
instead of remapping quaternion components independently.

### Skeleton membership and material/texture audit (2026-09-04)

The earlier skeleton writer serialized all 556 Transform nodes in the Prefab,
which incorrectly turned LOD containers, renderer GameObjects and shadow-proxy
nodes into Blender bones. The writer now derives membership only from every
SkinnedMeshRenderer bone palette and rootBone, then includes the required
ancestors in original hierarchy order. The Typhoea result contains one
316-node Armature, zero names matching LOD/shadowProxy, and zero unresolved
palette paths across all 62 Mesh resources.

The same audit found two blocking material round-trip defects:

- The runtime selected `Material` construction by argument count. Endfield's
  metadata shows that address was `Material(Shader)`, while the required
  `Material(Material)` constructor is a different overload. Resolution now
  uses the exact `UnityEngine.Material` parameter type.
- Blender previously created materials from only their `mod.ini` declaration;
  it never read the referenced `.mat`, and export emitted neither Texture
  declarations nor PNG files. Import now reads every EIEMMAT file, and export
  writes each unique Material and Texture resource exactly once.
- Blender also previously inferred a Material `target.path` from its `source`.
  That silently changed a renderer-local material clone into a global resource
  redirect after one round trip. Import now preserves a global target only
  when the resource declaration explicitly contains one.

Texture declarations additionally carry source color-space/mipmap/sampler
metadata. Runtime PNG decoding checks the returned Boolean, applies those
settings, and verifies every Material texture assignment with `GetTexture`.
Renderer material-array assignment is also read back element-by-element, so a
failed IL2CPP setter is reported instead of being mistaken for success.
The Blender regression package now verifies 62 Meshes, one skeleton, 31
Materials and 65 Textures; all references resolve and all 65 untouched PNGs
round-trip byte-for-byte. A separate edit probe changes a saved float and a
`texture.*` reference in Blender, then verifies both values in the exported
EIEMMAT while the Render material-slot references remain intact.

Material and Texture `target.path` remain explicit opt-in global redirects.
AnimeStudio does not enable them automatically for a model package: Render
records assign cloned Materials to model renderers, and those Materials refer
to Texture resources. This makes F10 restoration deterministic and avoids
globally replacing every source texture merely because it was present in an
offline authoring package. A live game test is still required before the
Material/Texture runtime path is called end-to-end verified.

### UI prefab coverage and material-handle correction (2026-09-04)

The first successful Typhoea Mesh test applied in the open world but not in
the character UI. Runtime metadata shows that the UI owns a separate
`Beyond.UI.UIModelLoader`, while the successful lifecycle hooks were attached
to gameplay `ModelManager` and `BaseModelViewPart`. This was a hook-coverage
defect, not evidence that UI Meshes require a separate replacement model.

This result first motivated using
`Beyond.Resource.Runtime.PrefabInstantiateProxy.OnCompleted` as the sole
Render discovery boundary. That reduced the design to PFB identity plus one
common Render executor and removed `Resources.FindObjectsOfTypeAll`, but the
2026-09-05 trace below showed that the character presentation loader can
bypass this callback. The PFB identity and common executor remain; only the
instance-lifecycle adapter set was expanded. F10 still walks registered
instances only.

The missing texture had an independent, earlier failure. Logs showed
`BundleResourceManager._LoadAssetInternal` returning a proxy, followed by
`Game resource loader could not resolve source path`; therefore PNG decoding
and `Material.SetTexture` were never reached. `BundleResourceManager.Load`
returns the value type `FAssetProxyHandle`, and `il2cpp_runtime_invoke` returns
that value boxed. EIEM was incorrectly calling `LoadImmediate` and `Get` with
the boxed object header as `this`. The backend now resolves and uses the
exported `il2cpp_object_unbox` API before invoking either instance method. The
game's `RootCategory` is confirmed by runtime metadata to use `System.Byte`,
so the existing one-byte `Main` argument remains correct.

Static regression contracts cover the common Render executor,
registered-instance F10 reconciliation, and mandatory value-type unboxing. The current
`GameAssembly.dll` export table contains `il2cpp_object_unbox`, and the native
DLL builds successfully. Visual confirmation of the UI Mesh and `_BaseMap`
override remains the single required live-game check.

### Material-controller lifecycle enforcement (2026-09-04, historical trace)

That live test separated Mesh coverage from Material lifetime: the edited Mesh
appeared in both paths and `_BaseMap` changed in the open world, while the
character presentation view later showed the source texture. The build then in
use reported a matched UI Renderer and the cloned Material array passed
immediate element-by-element read-back. The later registered-instance trace
below proved that this did not establish universal PFB lifecycle coverage; the
UI loader still needs its own adapter. The material result nevertheless rules
out PNG decoding and the `_BaseMap` property name for that matched instance.

The first attempted lifetime fix hooked Unity
`Renderer.set_sharedMaterial`/`set_sharedMaterials`. A subsequent map-switch
trace disproved that boundary: both hooks installed, but neither received a
single game commit. In the same trace, one matched Renderer changed from three
materials to two and then one, and a new UI Renderer was instantiated and
successfully matched. Therefore the failure is neither a missing UI instance
nor a lost Mesh identity; Endfield changes the material array through its own
controller path without entering those public Unity wrappers.

Runtime metadata identifies the actual owner as the nested
`EntityRenderHelperMaterialController.RendererInfo`. It holds `m_renderer`,
`sourceMaterials`, `replacingMaterials` and `materialReplacing`, and exposes
`TrySetSharedMaterial`, `TrySetSharedMaterials` and
`TryReplaceSharedMaterials`. EIEM now locates this class by that method shape,
resolves `m_renderer` from IL2CPP field metadata, and hooks the three commit
forms. Each detour calls the game first and then reapplies only the matched
Render rule's Material slots to that Renderer. Mesh replacement,
`handling=skip`, partners and scene-wide reconciliation are not run here.
The ineffective Unity setter detours and their re-entry guard were removed.

This remains common resource-rule behavior rather than a UI special case and
adds no polling. Temporary `[DEBUG-matlifecycle]` records distinguish a
controller array replacement (`reapplied`) from an unchanged material-array
identity (`retained`). The latter result together with a visually reverted
texture would prove a second mutation path inside the Material or a property
block; that case must be fixed at its demonstrated boundary instead of adding
a speculative fallback. One live `open presentation -> change map -> reopen
presentation` trace is required to close the runtime regression loop.

### Historical UIModelLoader hypothesis (2026-09-05)

The next trace corrected the earlier assumption that every presentation model
would eventually pass through the configured PFB completion hook. The Typhoea
world instance produced a configured `PrefabInstantiateProxy.OnCompleted`
record and applied 72 renderers. Opening the character presentation page did
not produce a second configured completion record. `Beyond.UI.UIModelLoader`
was then selected from static metadata because it owns synchronous/asynchronous
model loads plus `Cancel`, `UnloadModel`, `_Clear` and `Dispose`; at that point
there was no runtime call record proving that this character page used it.

The runtime now treats PFB and UI loading as two lifecycle adapters around one
model-instance registry. The registry is keyed by the resulting `GameObject`;
if both adapters report the same object, it stores both owner references but
applies only through the common `EiemApplyPrefabRules` path. Removing one owner
does not destroy scoped state while the other owner remains. `UnloadModel`
removes the exact UI instance, while loader clear/dispose removes all instances
owned by that loader.

Synchronous UI loading registers only after the game's method returns a
`GameObject`. Asynchronous UI loading wraps the supplied
`Action<GameObject>` with another managed delegate. The wrapper registers the
actual completed object and then invokes the original callback. A rooted
handle keeps the original callback alive; completion, cancellation, clear and
dispose release it exactly once. The request-return path never guesses an
instance and never applies a Render rule. This adds no polling,
`Resources.FindObjectsOfTypeAll`, or `UnityEngine.Object.Instantiate` hook.

Static contracts verify the adapter's callback ownership and cleanup, but they
cannot prove that a caller exists. The v26 runtime result below supersedes the
earlier attribution for the character presentation page.

### v25 NPC hypothesis disproved; BaseModelViewPart reuse restored (2026-09-05)

The v24 trace showed NPC-avatar activity during the same interval in which the
character presentation was opened, but it did not associate that activity
with the configured Typhoea PFB or Mesh. Treating temporal proximity as object
identity was an error. The v25 live run entered `SetSMRRootBone`, but every
observed renderer array contained only two or three entries, every application
count was zero, and no Typhoea `[MOD-AVATAR]` record existed. The NPC boundary
is therefore not the character-presentation replacement owner and has returned
to observation-only status.

Earlier confirmed traces remain authoritative: character postmodels load via
`ModelManager.LoadAsync`, then complete through
`ComplexModelViewPart.PostDealLoadedModel` and
`BaseModelViewPart.OnLoadFinish`. `BaseModelViewPart` also has a distinct
`_OnLoadUseHandleFinish*` route for an already loaded model. During the PFB
lifecycle refactor those two handle hooks stayed installed but their bodies no
longer registered or applied the completed model. That is a direct cached-UI
coverage regression: reusing a model does not create another
`PrefabInstantiateProxy`, so waiting for a new PFB callback cannot work.

v26 treats BaseModelViewPart as a third lifecycle adapter around the same PFB
registry and Render executor. `OnLoadFinish` and both handle-completion forms
read the part's exact `m_model` and embedded `m_cfg.modelPath`; if the path was
cleared, they use the model-to-path association captured by the original load
completion. Only an explicitly configured PFB path is registered. A part owns
one live result, and `ReleaseModel`/`OnRelease` release that ownership. This is
neither a scene scan nor a UI-specific Mesh implementation. The next live run
must produce `[MOD-MODEL-PART] ... path=...chr_0034_typhoea... applied=1` for
the presentation instance; its absence identifies a different concrete owner
without reviving the disproved NPC assumption.

### v26 result and CharUIModelMono owner adapter (2026-09-05, v27)

The v26 live log contains exactly one Typhoea `ModelManager.LoadAsync`, one
configured PFB completion and one Typhoea `BaseModelViewPart.OnLoadFinish`, all
for the same `chr_0034_typhoea_postmodel(Clone)#53` GameObject. The Render rule
successfully installs the replacement Mesh on that instance. Opening the
character page produces no second Typhoea load or model-part completion.
Although all six `UIModelLoader` hooks installed, none was invoked anywhere in
the run. This disproves both the UIModelLoader attribution and the later NPC
hypothesis for this page; the failure occurs before Render execution because
the displayed UI hierarchy never enters the model-instance registry.

Runtime metadata exposes the actual UI-specific component
`Beyond.Gameplay.View.CharUIModelMono`, derived from `TickableUIMono`, with
`OnAwake`, `SetVisible` and `OnRelease` plus an `EntityRenderHelper`. v27 uses
those methods only as lifecycle ownership boundaries. On awake or transition
to visible, it walks that component's ancestor chain, accepts only a root name
whose prefab stem uniquely matches a configured `[Prefab]` path, and then calls
the existing `EiemRegisterAndApplyModelInstance`/`EiemApplyPrefabRules` path.
On release it restores and removes only that owned instance. The matching code
understands Unity's `(Clone)#<instance>` suffix, rejects ambiguous stems and
never enumerates unrelated scene objects. `[MOD-CHAR-UI]` proves a successful
registration; `[MOD-CHAR-UI] unresolved` captures the bounded ancestry when no
configured identity can be resolved. Live visual verification remains pending.

### Shared Mesh/Material identity across world and character UI (2026-09-05)

The exact character-UI PFB was exported offline and compared with the existing
postmodel export:

- world: `assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0034_typhoea_postmodel.prefab`
- UI: `assets/beyond/dynamicassets/gameplay/prefabs/uimodels/chr_0034_typhoea_uimodel.prefab`

The two target Renderers are different serialized instances. Their Renderer
PathIDs and `rootBone` references differ, which means renderer-local bone
binding and lifecycle must never be copied globally from one PFB to the other.
Both nevertheless reference the exact same resources:

- Mesh `S_actor_typhoea_cloth_01_lod0`, PathID `4411933801208500242`
- Material `M_actor_typhoea_cloth_01`, PathID `510500371492053005`

This disproves the assumption that UI coverage requires discovering every PFB
variant. The stable edit identity is the shared Mesh/Material resource; PFB is
only the assembly scope for operations that truly modify a Renderer instance.

The previous `_FinishWithAsset` hook called the game's original completion
method first and only remembered the returned object's origin afterwards. At
that point asynchronous consumers could already retain the source object. The
new order resolves the logical path, builds the configured replacement, and
passes that replacement to the original completion method. A regression test
requires `TraceTryGlobalResourceRedirect` to occur before
`original(self, deliveredAsset, methodInfo)`.

Initial rule loading also moved out of the window/hotkey thread. `mod.ini` is
now parsed before the resource hooks are enabled, so a resource observed during
early startup cannot pass through merely because the game window did not yet
exist. F10 remains the only subsequent explicit reload path.

The Typhoea probe is intentionally reduced to global Mesh and Material target
declarations, with no PFB or Render block. This isolates the resource path in
the next live run: the edited Mesh and `_BaseMap` must appear in both the world
and character UI after one process restart. Blender 5.0.1 background testing
also confirmed that a selected imported Mesh plus one edited Material exports
exactly one Mesh, one Material, one PNG dependency, one PFB metadata record and
one Render metadata record, with automatic Mesh/Material `target.*` identity.

### v28 result: resource-completion replacement rejected (2026-09-05)

The v28 live trace invalidated the global-resource hypothesis above. The DLL
and Typhoea declarations loaded and the relevant PFB completed, but the target
flow produced no `Asset._FinishWithAsset` invocation and no
`[RES-REDIRECT]` record. That hook is therefore not a universal resource
boundary for this game. Keeping it would create a second replacement engine
whose coverage depends on an internal loader path, while also competing with
F10 restoration and Renderer lifetime. It has been removed rather than kept as
a fallback.

The trace and offline packages also exposed a format defect: historical
`Render.path` values were logical `.asset` paths, while the runtime matcher
interpreted `Render.path` as a Transform path relative to a model root. A rule
containing both `path` and `asset` consequently failed even when the Mesh asset
name was correct. Standalone Blender exports no longer emit this ambiguous
field. `asset` is the stable Mesh sub-asset identity; optional hierarchy
scoping must use a separately defined field in a future format instead of
overloading `path` again.

### Renderer/Mesh identity execution model (v29)

The replacement engine now has one direction of control:

1. `[Mesh]`, `[Material]`, `[Texture]`, and `[Skeleton]` declare payloads.
   Their `target.*` values preserve original-game identity for authoring and
   do not execute replacement.
2. A top-level `[Render]` with `asset=` is an action for every live Renderer
   consuming that Mesh asset. `mesh=`, `material.N=`, `handling=skip`, and
   `partner.N=` remain independent operations on the matched Renderer.
3. Completed PFB, BaseModelViewPart, UIModelLoader, and CharUIModelMono objects
   only offer exact model roots to the same executor. PFB names are not used as
   Mesh identity. CharUIModelMono submits its own GameObject directly instead
   of inferring a PFB from ancestor names.
4. `SkinnedMeshRenderer` is handled directly. Static content uses its
   `MeshFilter` as Mesh owner and sibling `MeshRenderer` as material/visibility
   owner. The same rule therefore covers characters, NPCs, buildings, world
   instances, UI instances, and repeated instances without type-specific INI.
5. Shared-Mesh setter hooks are lifecycle events, not a second rule system.
   They retain an existing binding when the game rewrites the original Mesh,
   or evaluate the same standalone rules for a newly assigned Mesh. F10 first
   restores recorded source state and then replays rules only on registered
   model instances; there is no per-frame or scene-wide polling.

The minimal generated form is now:

```ini
[MeshS_actor_typhoea_cloth_01_lod0_2]
path=meshes/MeshS_actor_typhoea_cloth_01_lod0_2.mesh
target.asset=S_actor_typhoea_cloth_01_lod0

[MaterialM_actor_typhoea_cloth_01_3]
path=materials/MaterialM_actor_typhoea_cloth_01_3.mat

[RenderS_actor_typhoea_cloth_01_lod0_2]
asset=S_actor_typhoea_cloth_01_lod0
mesh=MeshS_actor_typhoea_cloth_01_lod0_2
material.0=MaterialM_actor_typhoea_cloth_01_3
```

Blender 5.0.1 background regressions pass for the complete 62-Mesh round trip
and for the one-Mesh incremental export. The latter contains exactly one Mesh,
one changed Material, one PNG dependency, and one standalone Render action;
it contains no PFB block. Native build and static runtime contracts pass.
World/UI visual confirmation remains the required live regression for v29.

### v29 UI crash and v30 callback boundary correction (2026-09-05)

The user confirmed working world replacement but a crash on entering character
UI. The crash dump locates the fault at `GameAssembly+0x440D5D8`, invoking EIEM's
manufactured `Action<GameObject>` delegate. Its method field contains the native
completion function's code address, not valid managed method metadata. The
original constructor call incorrectly mixed those two representations.

v30 removes the delegate wrapper and pending-callback lifetime system outright.
The asynchronous UI loader passes the game's callback unchanged, preserving
completion timing, cancellation, and request IDs. Completed UI roots still reach
the same Render executor through the existing PFB completion and CharUIModelMono
lifecycle; both were observed in this very crash run. No mesh/skin/INI change or
exception-swallowing fallback is used. The earlier claim that character UI never
calls UIModelLoader is superseded by the actual trace.

The production async hook now has an executable passthrough regression, including
inline/deferred completion and cancellation. It failed before the fix and passes
afterwards. This proves the corrected native boundary, not in-game UI rendering;
live crash/visibility acceptance is still pending. Full evidence and experiment
limits: [UI callback crash record](ui-callback-crash-v29.md).

### v58 NPC miss and v59 concrete Renderer boundary (2026-09-07)

The v58 live run confirmed that PFB references no longer scoped Render rules:
the program published four source rules, and ordinary model and character-UI
Typhoea Renderers received the replacement. It also disproved
`SetSMRRootBone` as useful fallback coverage in that run. All 52 recorded calls
reported zero matching Renderers.

Two distinct Typhoea Renderers exposed the actual miss. Renderer
`00000011E4D09F40` appeared at log lines 6430-6435 and renderer
`0000001022369BA0` at lines 7729-7734/8183-8188. Both reached
`EntityRenderHelperMaterialController.RendererInfo._Init` with the original
917-vertex Mesh, original material, and enabled cloth Renderers. Neither object
entered the public `SkinnedMeshRenderer.set_sharedMesh` hook, a registered
model-root executor, or a successful final-bone replay. This is direct evidence
of an NPC/direct-construction path omitted by v58; it is not an INI, PFB, or
Mesh-name mismatch.

v59 keeps `SetSMRRootBone` observation-only. After `RendererInfo._Init` has let
the game capture clean source materials, the concrete SkinnedMeshRenderer is
offered to the same global Mesh-identity executor used at the other lifecycle
boundaries. Existing bound Renderers still use the material-commit reapply
path when a new global match is unavailable. The relevant 63 MSVC/host checks
and the full DLL build pass. The DLL with SHA-256
`E5E11A9A51FDDBC1CD2BF8EBF7A0D9A39392F0268E1608367CFBFFA4813DA890`
was deployed after the game stopped. The next v59 run showed
`[MOD-RENDERER-INIT]` on multiple Typhoea body and cloth Renderers, with body
replacement reporting `applied=true` and both cloth rules applying `skip`.
The user also confirmed the NPC result visually. This accepts the v59 NPC
Mesh/skip entry point for the declared LOD0 assets; it does not establish
coverage for undeclared LOD assets or native Physics.
