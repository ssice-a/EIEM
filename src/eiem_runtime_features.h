#pragma once

// The authoring formats remain readable while their native runtime adapters
// are validated independently.  Keep these switches compile-time and
// explicit so a production build cannot accidentally start an experimental
// Physics owner or install high-volume observation hooks.
static constexpr bool kEiemEnableExperimentalPhysicsRuntime = false;
// Temporary read-only evidence pass for native Physics ordering.  Hooks are
// installed and recording starts only after the manual F12 request, then an
// independent timer stops and exports the bounded capture.  This does not
// enable the experimental owner or mutate any Physics object.
static constexpr bool kEiemEnableNativePhysicsObservation = false;

// Retired experiment. FAssetProxyHandle/SubMeshInfo return hooks run after
// Unity has already created the source Mesh. Returning an EIEM-created Mesh
// there is post-creation replacement and cannot inherit the game's private
// native Mesh metadata, so it is not a pose fix. Keep this disabled even for
// evidence builds; descriptor hooks may still be used for read-only tracing.
static constexpr bool kEiemEnableUpstreamMeshBoundary = false;

// Read-only proof that the game's native Mesh deserializer handles the source
// Mesh selected by a Render rule. The normal DLL does not install this hook.
#if defined(EIEM_NATIVE_MESH_DESERIALIZE_TRACE_BUILD)
static constexpr bool kEiemEnableNativeMeshDeserializeTrace = true;
#else
static constexpr bool kEiemEnableNativeMeshDeserializeTrace = false;
#endif

// Expensive evidence collectors are opt-in.  The production resource path can
// rebuild the Mesh observation list on demand, so it must not maintain that
// list from every global sharedMesh setter call.  Lifecycle traces likewise
// must not scan their de-duplication table for every Prefab in the game.
static constexpr bool kEiemEnableContinuousMeshObservation = false;
// The owner routes have been identified.  Disable the scene-wide lifecycle
// log/dedup table for the focused Typhoea evidence run.
static constexpr bool kEiemEnableLifecycleDiagnostics = false;
// Diagnostic-only native caller chains at the low-frequency skin/registration
// boundaries. This is independent of the broad lifecycle census: enabling it
// must not turn on scene-wide identity logging or any mutation. A disposable
// evidence DLL can define EIEM_NATIVE_BOUNDARY_STACKS_BUILD at compile time;
// normal builds remain off.
#if defined(EIEM_NATIVE_BOUNDARY_STACKS_BUILD)
static constexpr bool kEiemEnableNativeBoundaryStacks = true;
#else
static constexpr bool kEiemEnableNativeBoundaryStacks = false;
#endif
// Disposable read-only trace for the game's native 0x40-byte per-draw source
// records. It is kept separate from the boundary census because DF9CA0 is a
// hot append helper; the diagnostic build caps output to a small fixed count.
#if defined(EIEM_NATIVE_FLAG_SOURCE_TRACE_BUILD)
static constexpr bool kEiemEnableNativeFlagSourceTrace = true;
#else
static constexpr bool kEiemEnableNativeFlagSourceTrace = false;
#endif
// Safer follow-up trace: observe only the DF1A60 consumer and reconstruct a
// bounded list of source records.  This deliberately does not hook the hot
// DF9CA0 append helper or capture a stack from a render thread.
#if defined(EIEM_NATIVE_FLAG_DRAW_TRACE_BUILD)
static constexpr bool kEiemEnableNativeFlagDrawTrace = false;
#else
static constexpr bool kEiemEnableNativeFlagDrawTrace = false;
#endif
// Evidence build: run the bounded CPU skin calculation at the existing
// post-window sample points. It reads Mesh weights/bindposes and Transform
// matrices only; it never changes a Renderer or animation object.
// Full vertex/weight skin math is reserved for the second post-F10 sample of
// the selected target transaction.  Binding fingerprints remain enabled for
// the cold-start window, so startup never walks every replacement mesh.
static constexpr bool kEiemEnableSkinDiagnostics = false;
// Required for EIEM-created skinned Meshes. EIEMESH BoneWeight records store
// four slots, including zero weights. The Endfield fixed-width skin path also
// reads native m_BonesPerVertex; validate the supported Unity layout before
// publishing a replacement. Static Meshes and source Meshes are untouched.
static constexpr bool kEiemEnableNativeMeshBoneSlots = true;
// Extra snapshots remain opt-in independently of the production correction.
#if defined(EIEM_NATIVE_MESH_BONE_SLOTS_PROBE_BUILD)
static constexpr bool kEiemEnableSkinBindingDiagnostics = true;
static constexpr bool kEiemEnableNativeMeshFlagProbe = true;
#else
static constexpr bool kEiemEnableSkinBindingDiagnostics = false;
static constexpr bool kEiemEnableNativeMeshFlagProbe = false;
#endif
// Disposable causal probe for the native Mesh metadata normalizer identified
// in UnityPlayer at RVA 0x4A90D0. This mutates native Mesh state, so it is
// validated on a diagnostic DLL before it can enter the normal path.
#if defined(EIEM_NATIVE_MESH_1C8_NORMALIZE_PROBE_BUILD)
static constexpr bool kEiemEnableNativeMesh1c8NormalizeProbe = true;
#else
static constexpr bool kEiemEnableNativeMesh1c8NormalizeProbe = false;
#endif
// Disposable hook for the same native routine. It observes the real game
// calls and their object type without invoking the function from EIEM. This
// distinguishes a Mesh method from a table callback on another native type.
#if defined(EIEM_NATIVE_MESH_1C8_CALL_TRACE_BUILD)
static constexpr bool kEiemEnableNativeMesh1c8CallTrace = true;
#else
static constexpr bool kEiemEnableNativeMesh1c8CallTrace = false;
#endif
// Disposable hardware data-breakpoint trace for the actual replacement Mesh
// objects. It watches only native Mesh +0x1C8 after EIEM has published a
// replacement and records the instruction pointer that performed a write.
// This is observation-only: the handler resumes the original instruction
// stream and restores every saved thread debug context when the window ends.
#if defined(EIEM_NATIVE_MESH_1C8_WRITE_TRACE_BUILD)
static constexpr bool kEiemEnableNativeMesh1c8WriteTrace = true;
#else
static constexpr bool kEiemEnableNativeMesh1c8WriteTrace = false;
#endif
// Disposable hardware watch for the native acceptance chain immediately
// upstream of UnityPlayer!0xEF230. It watches only the pointer fields that
// feed EF230 (Mesh+0x38, metadata+0x190, aux+0x50), records the writer RIP
// and a bounded unwind, then restores every thread's debug registers. This
// is observation-only and never changes the accepted pointer or flag.
#if defined(EIEM_NATIVE_SKIN_EF230_WRITE_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinEf230WriteTrace = true;
#else
static constexpr bool kEiemEnableNativeSkinEf230WriteTrace = false;
#endif
// Diagnostic compatibility check for Endfield's Unity fork.  The fork may
// attach native skin-registration metadata in the legacy BoneWeight[] setter
// even when the variable BoneWeight1 entry point reports successful managed
// readback.  Keep both calls bounded to Mesh construction, and log the native
// state after each call; this is not a GPU/Vulkan write or a guessed flag.
// The 2026-09-25 v2 run showed that neither setter changes native Mesh
// +0x1C8, so leave the compatibility experiment disabled in normal builds.
static constexpr bool kEiemMeshBoneWeightDualPathProbe = false;
// Temporary low-frequency read-only probe for UnityPlayer's native skin
// capability gate (RVA 0x41FD40).  This gate controls the 0x20 bit that
// distinguishes the observed cloth flags 0x30/0x34.  The probe never changes
// its return value or any Unity state; it records only state transitions and
// is capped in the detour.
// Disabled in the normal DLL after the 2026-09-24 crash investigation.  This
// detour runs on every native skin record and is unnecessary for the repair
// path; keep the source available for a disposable evidence build only.
#if defined(EIEM_NATIVE_SKIN_RECORD_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinSupportProbe = true;
#else
static constexpr bool kEiemEnableNativeSkinSupportProbe = false;
#endif
// Temporary bounded read-only trace at UnityPlayer!0xC7B750.  This is the
// native function that assembles the per-draw skin record and writes the
// 0x30/0x34 flag.  Keep it independent from the old broad boundary build so
// the trace can be enabled without turning on unrelated registration logs.
// Disabled in the normal DLL.  C7B750 is a hot native render path (the last
// run exceeded 100k calls); leaving a diagnostic detour installed can add
// timing/race risk while the game is rebuilding its skin buffers.
#if defined(EIEM_NATIVE_SKIN_RECORD_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinRecordProbe = true;
#else
static constexpr bool kEiemEnableNativeSkinRecordProbe = false;
#endif
// Disposable short-window trace for C7B750's metadata-derived influence
// count. Unlike the older record trace, this mode records only metadata
// candidates with a count of four or fewer and captures no stacks.
#if defined(EIEM_NATIVE_SKIN_C7_TARGET_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinC7TargetProbe = true;
#else
static constexpr bool kEiemEnableNativeSkinC7TargetProbe = false;
#endif
// Disposable read-only trace for UnityPlayer!0x19CE90, the low-frequency
// native skin-validation boundary.  It is filtered to native Mesh pointers
// captured from EIEM replacement commits, so it does not scan unrelated
// renderers or alter the game's result.
#if defined(EIEM_NATIVE_SKIN_VALIDATION_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinValidationProbe = true;
#else
static constexpr bool kEiemEnableNativeSkinValidationProbe = false;
#endif
// Disposable trace for the actual managed Mesh construction setters.  The
// detours are installed only in the dedicated evidence build and filter on
// Mesh objects created by EIEM, so no scene-wide setter census is performed.
#if defined(EIEM_NATIVE_MESH_SETTER_TRACE_BUILD)
static constexpr bool kEiemEnableNativeMeshSetterTrace = true;
#else
static constexpr bool kEiemEnableNativeMeshSetterTrace = false;
#endif
// Disposable read-only trace at the native Mesh constructor entry. This is
// deliberately separate from the post-constructor hardware watch: a writer
// that runs inside the constructor would otherwise happen before the watch
// can arm. The hook only records before/after values and delegates unchanged.
#if defined(EIEM_NATIVE_MESH_CTOR_TRACE_BUILD)
static constexpr bool kEiemEnableNativeMeshCtorTrace = true;
#else
static constexpr bool kEiemEnableNativeMeshCtorTrace = false;
#endif
// Disposable constructor experiment: clone the game's source Mesh through
// UnityEngine.Object.Instantiate so Unity copies native Mesh metadata, then
// overwrite the replacement payload through the existing setters.  This is
// an evidence build only; production remains on the fresh-constructor path
// until the result is validated.
#if defined(EIEM_NATIVE_MESH_CLONE_BUILD)
static constexpr bool kEiemEnableNativeMeshClone = true;
#else
static constexpr bool kEiemEnableNativeMeshClone = false;
#endif
// Follow-up clone experiment: keep the source clone's native layout and
// metadata intact, then let the ordinary Mesh setters resize/replace the
// payload.  The earlier clone probe called Clear(false), which preserved the
// private metadata but left the fork's writable payload at zero vertices.
#if defined(EIEM_NATIVE_MESH_CLONE_NO_CLEAR_BUILD)
static constexpr bool kEiemEnableNativeMeshCloneNoClear = true;
#else
static constexpr bool kEiemEnableNativeMeshCloneNoClear = false;
#endif
// Disposable in-place experiment: write the replacement payload into the
// already-loaded source Mesh object.  This models the user's intended global
// replacement semantics and avoids the fresh native-constructor path.  It is
// never enabled in the production DLL by default.
#if defined(EIEM_NATIVE_MESH_INPLACE_BUILD)
static constexpr bool kEiemEnableNativeMeshInplace = true;
#else
static constexpr bool kEiemEnableNativeMeshInplace = false;
#endif
// Temporary bounded read-only trace for UnityPlayer!0xEF200, the mode query
// immediately before C7B750 chooses the 0x10/0x30 versus 0x14/0x34 base flag.
// Disabled in the normal DLL.  EF200 is queried from the same hot path and
// the bounded anomaly evidence has already been preserved on disk.
#if defined(EIEM_NATIVE_SKIN_RECORD_TRACE_BUILD)
static constexpr bool kEiemEnableNativeSkinModeProbe = true;
#else
static constexpr bool kEiemEnableNativeSkinModeProbe = false;
#endif
// Use the game's completed EntityRenderHelper/MaterialController assembly as
// the resource commit boundary.  The replacement is published before the
// game's own RendererInfo, LOD and native skin registration walk, so every
// pipeline (world/NPC/UI) consumes the same Mesh through its normal path.
// This is deliberately independent of PFB/SubMeshInfo, whose ownership and
// call chain differ between those pipelines.
// GPU/skin-submission timing probe is disabled. Keep the code available for a
// later evidence build, but do not arm its timer or install its submission
// hooks in the normal DLL.
static constexpr bool kEiemEnableSkinTimingProbe = false;
// Targeted one-shot observation for the game's SubMeshInfo/partSubMeshGPU
// descriptor cache. It is read-only and filtered to assets selected by the
// active mod rules; it does not mutate the game's resource table. A disposable
// PFB-boundary build enables this without turning on the broad identity census.
#if defined(EIEM_PFB_MESH_BOUNDARY_TRACE_BUILD)
static constexpr bool kEiemEnableDescriptorDiagnostics = true;
#else
static constexpr bool kEiemEnableDescriptorDiagnostics = false;
#endif
// One-run metadata census for the game-owned skin/render path.  This only
// enumerates the exact classes that can separate the ordinary SMR input from
// Endfield's HG/partSubMeshGPU submission.  It installs no hooks and mutates no
// object; disable it after the signatures have been collected.
static constexpr bool kEiemEnableCustomSkinPipelineMetadata = false;
// Disabled: the custom-pipeline observation path installs GpuClothManager and
// GPUDrivenRenderer method hooks. The current evidence pass is CPU-only;
// Vulkan remains a pure forwarding layer and no GPU functions are observed.
static constexpr bool kEiemEnableCustomSkinPipelineObservation = false;
