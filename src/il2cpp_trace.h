#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

#include "eiem_mods.h"
#include "eiem_mod_update.h"
static bool EiemOnUnityThread();
#include "eiem_unity_lifetime.h"
#include "eiem_native_physics_parameters.h"
#include "eiem_native_physics_config.h"
#include "eiem_native_physics_events.h"
#include "eiem_native_physics_order_probe.h"
#include "eiem_model_lifecycle.h"
#include "eiem_render_replay.h"
#include "eiem_resource_backend.h"
#include "eiem_performance.h"
static size_t EiemManagedArrayLength(void *array);
#include "eiem_skeleton_runtime.h"
#include "eiem_registration_trace.h"
#include "eiem_skin_probe.h"

static void EiemReportNativeMeshDeserializeSource(void *mesh,
                                                  const char *source,
                                                  const char *asset,
                                                  const char *section);

// Resource-loading hooks preserve the game's VFS/decryption pipeline and
// observe logical identities. Only the Render executor applies declared
// replacement resources to the matching game instances.

static volatile LONG s_traceLoadAssetCount = 0;
static volatile LONG s_traceLoadAssetAsyncCount = 0;
static volatile LONG s_traceSharedMeshCount = 0;
static volatile LONG s_traceMeshFilterCount = 0;
static volatile LONG s_traceBundleLoadCount = 0;
static volatile LONG s_traceHashLoadCount = 0;
static volatile LONG s_traceHashSubAssetCount = 0;
static volatile LONG s_traceAssetLoaderTryLoadCount = 0;
static volatile LONG s_traceCachedLoaderCount = 0;
static volatile LONG s_tracePathHashCount = 0;
static volatile LONG s_tracePreloadCount = 0;
static volatile LONG s_traceAssetNameCount = 0;
static volatile LONG s_traceAssetCompleteCount = 0;
static volatile LONG s_traceProxyPathCount = 0;
static volatile LONG s_traceProxyGetCount = 0;
static volatile LONG s_traceProxyAssetCount = 0;
static volatile LONG s_traceVfsPathCount = 0;
static volatile LONG s_traceStreamCaptureCount = 0;
static thread_local bool s_traceReentrant = false;
static volatile LONG s_traceSetterThreadLogged = 0;
static volatile LONG s_tracePrefabIdentityCount = 0;
static volatile LONG s_traceBonesSetterCount = 0;
// One bounded timeline for the intermittent ground-pose investigation. This
// records only tracked replacement Renderers; it does not mutate Unity state.
static volatile LONG s_eiemSkinTimelineCount = 0;
static volatile LONG64 s_eiemLastSkinCommitTick = 0;
// F10 skin timing probe. This is observation-only; it never rebinds a
// Renderer. A transaction id lets a post-window sample be matched to the
// replay that scheduled it.
static volatile LONG s_eiemSkinTimingProbeSequence = 0;
static volatile LONG s_eiemSkinTimingProbePending = 0;
static volatile LONG s_eiemSkinTimingProbeStage = 0;
static volatile LONG s_eiemSkinTimingProbeTicks = 0;
// Bounded calls from Unity's internal skin submission API during one probe
// window. The callbacks below are observation-only and deliberately stop
// after a small number of tracked/untracked calls.
static volatile LONG s_eiemSkinNativeTrackedCalls = 0;
static volatile LONG s_eiemSkinNativeUntrackedCalls = 0;
static volatile LONG s_eiemSkinCaptureRequestCalls = 0;
// One cold-start observation window.  It is armed only after the first
// complete model replay has produced the clothing targets, so the startup
// sample can be compared with the later F10 transactions without changing
// their behavior.
static volatile LONG s_eiemSkinColdProbeArmed = 0;
// Bounded descriptor-call census. This is enabled only for the temporary
// descriptor investigation and records the first calls even when the
// descriptor's name field is not the one expected by our metadata guess.
static volatile LONG s_eiemDescriptorInfoCallCount = 0;
static volatile LONG s_eiemDescriptorAssetsCallCount = 0;
// Legacy Partner diagnostics still use this counter inside quarantined code.
static volatile LONG s_traceSkinProbeCount = 0;
// Bounded one-shot census of the native bones available at the resource
// submission boundary. This is separate from legacy skin diagnostics so a
// noisy unrelated probe cannot hide the assembly evidence.
static volatile LONG s_eiemSkinCaptureProbeCount = 0;
static volatile LONG s_eiemMaterialBoundarySkinProbeCount = 0;
// Re-entrancy guard and bounded audit counter for the upstream Mesh return
// boundary. The guard matters because building a replacement itself invokes
// Unity Mesh APIs and may consult the same resource proxy.
static thread_local bool s_eiemUpstreamMeshBuildActive = false;
static volatile LONG s_eiemUpstreamMeshRedirectCount = 0;
// The CPU evidence window follows one owner containing body, cloth_01 and
// cloth_02.  Keeping the owner stable across A/B/C/D samples prevents a
// scene-wide override table from turning the probe into a cold-start scan.
static volatile LONG s_eiemSkinTargetTransaction = 0;
static void *s_eiemSkinTargetOwner = nullptr;
// Short diagnostic pass for resource replacements. It compares the game's
// source skin with the generated Mesh only for the first few cloth instances,
// so a bad bind/weight result is visible without reintroducing periodic work.
// Bounded evidence for EIEMESH v4 hierarchy-index rejection. This records the
// exact divergent slot without changing the binding decision.
static volatile LONG s_traceHierarchyIndexFailureCount = 0;
static volatile LONG s_traceMaterialCommitCount = 0;

// The manager often returns the same cached proxy repeatedly. Keep the first
// sighting of each hash so completion records retain their own log budget.
static SRWLOCK s_traceSeenHashLock = SRWLOCK_INIT;
static int64_t s_traceSeenHashes[1024] = {};
static size_t s_traceSeenHashCount = 0;

// Runtime bundle manifest. Unlike diagnostic trace counters, this collection
// is intentionally not capped: it is the input manifest for the offline VFS
// extractor. Paths are kept as logical game paths and written periodically by
// the hotkey worker, so resource hooks never perform disk I/O.
struct TraceBundleManifestEntry {
  std::string path;
  ULONGLONG firstSeenMs;
};
struct TraceActiveBundleEntry {
  void *bundle;
  std::string path;
};
struct TracePendingBundleRequestEntry {
  void *request;
  std::string path;
};
static SRWLOCK s_bundleManifestLock = SRWLOCK_INIT;
static std::vector<TraceBundleManifestEntry> s_bundleManifestEntries;
static std::vector<TraceActiveBundleEntry> s_activeBundleEntries;
static std::vector<TracePendingBundleRequestEntry> s_pendingBundleRequests;

// Connect the game's asynchronous proxy/load lifecycle to the final Unity
// object. This is process-local evidence only, but it lets a scene Dump carry
// the original logical asset path and hash instead of guessing between
// same-named objects in different serialized files.
struct TraceProxyOriginEntry {
  void *proxy;
  int64_t pathHash;
  uint64_t stamp;
  char path[768];
};
struct TraceAssetOriginEntry {
  void *asset;
  int64_t pathHash;
  uint64_t stamp;
  char path[768];
};
static SRWLOCK s_assetOriginLock = SRWLOCK_INIT;
static TraceProxyOriginEntry s_proxyOrigins[8192] = {};
static TraceAssetOriginEntry s_assetOrigins[16384] = {};
static volatile LONG64 s_assetOriginStamp = 0;

static void TraceRememberBundlePathText(const char *pathText);
static void TraceRememberBundlePath(void *path);
static void TraceRememberActiveBundle(void *bundle, void *path);
static void TraceForgetActiveBundle(void *bundle);
static void TraceRememberPendingBundleRequest(void *request, void *path);
static void TraceResolvePendingBundleRequest(void *request, void *bundle);
static void TraceDescribeObject(void *object, char *out, int outSize);
static void TraceDescribeString(void *stringObject, char *out, int outSize);
static bool TraceIdentityTextMatchesConfiguredRule(const char *text);
static bool TraceTakeTargetBudget(volatile LONG *counter, LONG limit,
                                  const char *primary,
                                  const char *secondary = nullptr);
static void TraceRememberAssetOrigin(void *asset, int64_t pathHash,
                                     const char *path);
static bool TraceTakeBudget(volatile LONG *counter, LONG limit);
static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize);
static void TraceReadUnityObjectName(void *object, char *out, int outSize);
static void *EiemMaybeUpstreamReplaceMesh(void *sourceMesh,
                                          const char *logicalPath,
                                          const char *descriptorName);
static void EiemReconcileModelPhysics(
    void *model, const std::vector<EiemPhysicsIntent> &intents, bool active,
    const char *stage);
static void EiemPhysicsRuntimeBoundary(const char *stage);
static size_t EiemPhysicsRuntimeRetireChangedAssets(const char *stage);
// Nested model callbacks during one F10 replay share the outer transaction;
// they must not each repeat Physics collection/readiness work.
static thread_local bool s_eiemPhysicsLifecycleTransaction = false;
static void EiemReleaseModelPhysics(void *model, const char *stage);
static void TraceLookupHashPath(int64_t hash, char *out, int outSize);
static bool TraceResolveStringPathHashPath(int64_t hash, char *out,
                                           size_t outSize);
static bool TraceLookupAssetOrigin(void *asset, int64_t *pathHash, char *path,
                                   size_t pathSize);
static void EiemExtractObjectName(const char *description, char *out,
                                   size_t outSize);
typedef void (__fastcall *TraceSetSharedMeshFn)(void *self, void *mesh,
                                                 void *methodInfo);
static void *s_origSkinnedMeshSetSharedMesh = nullptr;
static void *s_origMeshFilterSetSharedMesh = nullptr;
typedef bool (__fastcall *TraceRendererInfoMaterialCommitFn)(
    void *self, void *materialOrMaterials, void *methodInfo);
static void *s_origRendererInfoTrySetSharedMaterial = nullptr;
static void *s_origRendererInfoTrySetSharedMaterials = nullptr;
static void *s_origRendererInfoTryReplaceSharedMaterials = nullptr;
static int s_materialRendererInfoRendererOffset = -1;
static void *s_origMaterialInfoInit = nullptr;
typedef void (__fastcall *TraceSetBonesFn)(void *self, void *bones,
                                           void *methodInfo);
static void *s_origSkinnedMeshSetBones = nullptr;
typedef bool (__fastcall *TraceRequestCurrentFrameSkinMatricesFn)(
    void *self, void *skinMatrices, int32_t count, void *methodInfo);
typedef bool (__fastcall *TraceSkinMatricesRequestFinishedFn)(
    void *self, void *methodInfo);
typedef void *(__fastcall *TraceSkinGraphicsBufferFn)(void *self,
                                                       void *methodInfo);
static void *s_origSkinnedMeshRequestCurrentFrameSkinMatrices = nullptr;
static void *s_origSkinnedMeshSkinMatricesRequestFinished = nullptr;
static void *s_origSkinnedMeshGetVertexBuffer = nullptr;
static void *s_origSkinnedMeshGetPreviousVertexBuffer = nullptr;
// HG's custom skin-capture path is a candidate boundary between the managed
// Renderer state and the game's GPU-side skin buffer. The probe below is
// observation-only and is enabled only while the bounded cold/F10 window is
// active; it never changes the capture request or its property block.
typedef void (__fastcall *TraceSkinCaptureRequestFn)(
    void *self, void *meshRenderer, void *skinnedMeshRenderer,
    void *propertyBlock, void *methodInfo);
static void *s_origSkinnedMeshCaptureRequest = nullptr;
// MaterialPropertyBlock buffer bindings are the next read-only boundary after
// the renderer/bones state.  The custom pipeline may bind its skin palette
// through SetBuffer/SetConstantBuffer without entering Unity's public skin
// request APIs.  Keep this probe bounded to the existing cold/F10 timing
// window; it records the binding descriptor only and never changes it.
typedef void (__fastcall *TraceMaterialPropertyBlockSetBufferFn)(
    void *self, int32_t propertyId, void *buffer, int32_t offset,
    int32_t size, void *methodInfo);
static void *s_origMaterialPropertyBlockSetBuffer = nullptr;
static void *s_origMaterialPropertyBlockSetConstantBuffer = nullptr;
static void *s_origMaterialSetConstantBuffer = nullptr;
typedef void *(__fastcall *TraceRenderGraphGetComputeBufferFn)(
    void *self, void *handle, void *methodInfo);
static void *s_origRenderGraphGetComputeBuffer = nullptr;
static volatile LONG s_eiemSkinBufferBindingCalls = 0;
static volatile LONG s_eiemGpuDrivenCalls = 0;
// Read-only observation of the game's custom GPU-cloth path.  These counters
// are reset for each cold/F10 skin timing window and bound the amount of log
// data produced by high-frequency Tick/SetPerDrawData calls.
static volatile LONG s_eiemGpuClothObservationCalls = 0;
static volatile LONG s_eiemGpuClothEventSequence = 0;
static volatile LONG s_eiemGpuClothStartupCalls = 0;
typedef void (__fastcall *TraceGpuClothTickFn)(void *self, float deltaTime,
                                               void *methodInfo);
typedef void (__fastcall *TraceGpuClothPipelineUpdateV2Fn)(
    void *self, void *transform, void *methodInfo);
typedef void (__fastcall *TraceGpuClothPipelineUpdateV2StaticFn)(
    void *transform, void *methodInfo);
typedef void (__fastcall *TraceGpuClothRegisterGroupFn)(
    void *self, void *clothGroupData, void *methodInfo);
typedef void (__fastcall *TraceGpuClothSetCharacterProxyMeshFn)(
    void *self, void *mesh, void *methodInfo);
typedef void *(__fastcall *TraceGpuClothGetSkeletonBufferFn)(
    void *self, void *methodInfo);
typedef bool (__fastcall *TraceGpuClothBoolFn)(void *self, void *methodInfo);
static void *s_origGpuClothTick = nullptr;
static void *s_origGpuClothSetPerDrawData = nullptr;
static void *s_origGpuClothPipelineUpdateV2 = nullptr;
static void *s_origGpuClothPipelineUpdateV2Static = nullptr;
static void *s_origGpuClothRegisterGroup = nullptr;
static void *s_origGpuClothSetCharacterProxyMesh = nullptr;
static void *s_origGpuClothGetSkeletonBuffer = nullptr;
static void *s_origGpuClothIsSkeletonValid = nullptr;
static void *s_origGpuClothIsSkeletonFlipped = nullptr;
static void *s_origGpuClothFlipSkeletonFlag = nullptr;
// The custom HG renderer records skin resources directly on CommandBuffer.
// These hooks observe the command descriptor (native buffer id, property id,
// offset and size) without touching the command or draw state.
typedef void (__fastcall *TraceCommandBufferSetGlobalConstantBuffer0Fn)(
    void *self, uint32_t bufferId, int32_t propertyId, int32_t offset,
    int32_t size, void *methodInfo);
typedef void (__fastcall *TraceCommandBufferSetGlobalBufferIdFn)(
    void *self, int32_t propertyId, uint32_t bufferId, void *methodInfo);
typedef void (__fastcall *TraceCommandBufferSetGlobalConstantBufferFn)(
    void *self, void *buffer, int32_t propertyId, int32_t offset,
    int32_t size, void *methodInfo);
typedef void (__fastcall *TraceCommandBufferSetGlobalBufferFn)(
    void *self, int32_t propertyId, void *buffer, void *methodInfo);
static void *s_origCommandBufferSetGlobalConstantBuffer0 = nullptr;
static void *s_origCommandBufferSetGlobalBufferId = nullptr;
static void *s_origCommandBufferSetGlobalConstantBuffer = nullptr;
static void *s_origCommandBufferSetGlobalBuffer = nullptr;
// HG GPU-driven renderer is the game's custom render submission path.  These
// hooks only identify the command buffer/list submitted during a bounded
// skin transaction; they do not alter renderer state or GPU resources.
typedef void (__fastcall *TraceGpuDrivenBindBuffersForRenderingFn)(
    void *self, void *commandBuffer, void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenPopulatePerFrameDataFn)(
    void *self, void *commandBuffer, uint32_t frameDataId,
    uint32_t rendererDataId, bool flag, void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenDrawRendererListFn)(
    void *self, void *commandBuffer, uint32_t rendererListId, bool flag,
    void *methodInfo);
static void *s_origGpuDrivenV2BindBuffersForCulling = nullptr;
static void *s_origGpuDrivenV2BindBuffersForRendering = nullptr;
static void *s_origGpuDrivenV2PopulatePerFrameData = nullptr;
static void *s_origGpuDrivenV2DrawRendererList = nullptr;
static void *s_origGpuDrivenV1PopulatePerFrameData = nullptr;
static void *s_origGpuDrivenV1DrawRendererList = nullptr;
static void *s_origGpuDrivenV1BindBuffersForCulling = nullptr;
typedef void (__fastcall *TraceGpuDrivenBindBuffersForCullingFn)(
    void *self, void *commandBuffer, void *computeShader, uint32_t bufferId,
    void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenDispatchComputeFn)(
    void *self, void *commandBuffer, void *computeShader, uint32_t dispatchId,
    void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenBindFrameConstantsFn)(
    void *self, void *commandBuffer, void *computeShader, uint32_t bufferId,
    void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenBindFrameConstantsGlobalFn)(
    void *self, void *commandBuffer, void *methodInfo);
typedef void (__fastcall *TraceGpuDrivenAdvanceFrameFn)(
    void *self, void *methodInfo);
static void *s_origGpuDrivenV1BindFrameConstants = nullptr;
static void *s_origGpuDrivenV1BindFrameConstantsGlobal = nullptr;
static void *s_origGpuDrivenV1DispatchMeshletInstanceCount = nullptr;
static void *s_origGpuDrivenV1DispatchDrawBucketCount = nullptr;
static void *s_origGpuDrivenV1BindBuffersForRendering = nullptr;
static void *s_origGpuDrivenV1AdvanceFrame = nullptr;
static void *s_origGpuDrivenV2BindFrameConstants = nullptr;
static void *s_origGpuDrivenV2BindFrameConstantsGlobal = nullptr;
static void *s_origGpuDrivenV2DispatchMeshletInstanceCount = nullptr;
static void *s_origGpuDrivenV2DispatchDrawBucketCount = nullptr;
static void *s_origGpuDrivenV2AdvanceFrame = nullptr;
typedef void(__fastcall *TraceCreateSmsGoFn)(
    void *assetLoader, void *meshAssets, int32_t lod, void *goPool,
    void *parent, void *stringList, void *intList, void **renderers,
    void **rootBones, bool flag, void *handleMap, bool deferred,
    void *methodInfo);
typedef void(__fastcall *TraceCreateSmsPostFn)(
    void *meshAssets, int32_t lod, void *goPool, void *parent,
    void *stringList, void *intList, void **renderers, void **rootBones,
    bool flag, void *methodInfo);
typedef void(__fastcall *TraceLodGroupSetLodsFn)(
    void *self, void *lods, void *methodInfo);
// The post-model helper is a compiler-generated static local function. Its
// explicit parameters are (lod, renderer array, root-bone array, closure).
// It is the last point before the game's AssignSkin code consumes the array.
typedef void(__fastcall *TraceAssignSkinPostFn)(
    int32_t lod, void *renderers, void *rootBones, void *closure,
    void *methodInfo);
static void *s_origCreateSmsGo = nullptr;
static void *s_origCreateSmsPost = nullptr;
static void *s_origLodGroupSetLODs = nullptr;
static void *s_origAssignSkinGo = nullptr;
static void *s_origAssignSkinPost = nullptr;
static thread_local bool s_eiemApplyingPartnerLod = false;
// NPCAvatarCreatorUtils assigns the final Animator bone palette after the
// renderer array has been created. This remains an ordering observation; the
// concrete NPC Renderer is handled at RendererInfo._Init.
typedef void(__fastcall *TraceSetSmrRootBoneFn)(
    void *animator, void *renderers, void *rootBoneInfos, void *methodInfo);
static void *s_origSetSmrRootBone = nullptr;
typedef void (__fastcall *TraceModelManagerGameObjectFn)(void *self,
                                                          void *model,
                                                          void *methodInfo);
typedef void *(__fastcall *TraceModelManagerLoadHashFn)(void *self,
                                                         int64_t pathHash,
                                                         void *methodInfo);
typedef void (__fastcall *TracePrefabInstantiateCompletedFn)(void *self,
                                                              void *methodInfo);
typedef void (__fastcall *TracePrefabInstantiateLifecycleFn)(void *self,
                                                              void *methodInfo);
typedef void *(__fastcall *TraceUIModelLoaderLoadModelFn)(
    void *self, void *path, void *parent, void *methodInfo);
typedef int32_t (__fastcall *TraceUIModelLoaderLoadModelAsyncFn)(
    void *self, void *path, void *parent, void *callback, void *methodInfo);
typedef void (__fastcall *TraceUIModelLoaderUnloadModelFn)(void *self,
                                                            void *model,
                                                            void *methodInfo);
typedef void (__fastcall *TraceUIModelLoaderLifecycleFn)(void *self,
                                                          void *methodInfo);
typedef void (__fastcall *TraceCharUIModelLifecycleFn)(void *self,
                                                        void *methodInfo);
typedef void (__fastcall *TraceCharUIModelSetVisibleFn)(void *self,
                                                         bool visible,
                                                         void *methodInfo);
typedef void (__fastcall *TraceBasePartFinishFn)(void *self, bool success,
                                                  void *methodInfo);
typedef void (__fastcall *TraceBasePartPostDealFn)(void *self,
                                                    void *methodInfo);
// EntityRenderHelper builds the game's internal RendererInfo/material and
// visibility registries from the hierarchy. Partners must exist before this
// method scans the hierarchy; otherwise assigning public bones later does not
// make the new Renderer part of the Animator/render-control path.
typedef void (__fastcall *TraceEntityRenderHelperInitFn)(
    void *self, void *methodInfo);
// Read-only observation of the game's material/renderer registry commit.  The
// controller receives the complete Renderer list and builds RendererInfo
// entries used by the custom visibility/material path.
typedef void (__fastcall *TraceEntityRenderHelperMaterialControllerInitFn)(
    void *self, void *renderers, void *rendererTypeConfigs,
    void *customizeRendererPropertyConfig, bool calculateBoundsWithTransform,
    void *methodInfo);
typedef void (__fastcall *TraceBasePartLoadFinishCallbackFn)(
    void *self, int32_t requestId, int64_t pathHash, void *model,
    void *methodInfo);
typedef bool (__fastcall *TraceBasePartLoadFinishResultFn)(
    void *self, int32_t requestId, int64_t pathHash, void *model,
    void *methodInfo);
typedef void (__fastcall *TraceBasePartLoadUseHandleFinishCallbackFn)(
    void *self, bool success, void *handle, void *methodInfo);
typedef bool (__fastcall *TraceBasePartLoadUseHandleFinishResultFn)(
    void *self, bool success, void *handle, void *methodInfo);
static void *s_origModelManagerGameObjectAllocate = nullptr;
static void *s_origModelManagerLoadFromPersistentPool = nullptr;
static void *s_origPrefabInstantiateCompleted = nullptr;
static void *s_origPrefabInstantiateUnload = nullptr;
static void *s_origPrefabInstantiateClear = nullptr;
static void *s_origPrefabInstantiateDispose = nullptr;
static void *s_prefabInstantiateGetGameObject = nullptr;
static void *s_prefabInstantiateGetLogName = nullptr;
static void *s_prefabInstantiateGetInstanceUid = nullptr;
static void *s_origUIModelLoaderLoadModel = nullptr;
static void *s_origUIModelLoaderLoadModelAsync = nullptr;
static void *s_origUIModelLoaderUnloadModel = nullptr;
static void *s_origUIModelLoaderClear = nullptr;
static void *s_origUIModelLoaderDispose = nullptr;
static void *s_origCharUIModelOnAwake = nullptr;
static void *s_origCharUIModelSetVisible = nullptr;
static void *s_origCharUIModelOnRelease = nullptr;
static void *s_origBasePartFinish = nullptr;
static void *s_origBasePartLoadFinishCallback = nullptr;
static void *s_origBasePartLoadFinishResult = nullptr;
static void *s_origBasePartLoadUseHandleFinishCallback = nullptr;
static void *s_origBasePartLoadUseHandleFinishResult = nullptr;
static void *s_origBasePartReleaseModel = nullptr;
static void *s_origBasePartOnRelease = nullptr;
static void *s_origBasePartPostDeal = nullptr;
static void *s_origComplexPartPostDeal = nullptr;
static void *s_origEntityRenderHelperInitRenderAndMaterial = nullptr;
static void *s_origEntityRenderHelperMaterialControllerInit = nullptr;
static void *s_entityRenderHelperClass = nullptr;
static thread_local bool s_eiemEntityRenderHelperInitGuard = false;
static thread_local bool s_eiemEntityRenderHelperMaterialInitGuard = false;
// The outer helper owns the model transaction, while MaterialController.Init
// is the first native boundary at which the game has populated each
// SkinnedMeshRenderer's local bones[] palette.  Carry the concrete model into
// that nested call so the replacement is committed against the same instance
// (never by a scene-wide search or a cross-instance bone lookup).
static thread_local void *s_eiemEntityRenderHelperActiveModel = nullptr;
static thread_local bool s_eiemEntityRenderHelperMaterialApplied = false;
// RendererInfo._Init samples original materials during an enclosing helper.
// The helper must finish that sampling before EIEM restores its owned slots.
// RendererInfo callbacks may occur anywhere inside the outer helper, not only
// inside MaterialController.Init. Drain this queue after the complete helper
// returns so no raw Renderer pointer survives into a later assembly pass.
static thread_local std::vector<void *> s_eiemMaterialsToReapplyAfterHelper;
static thread_local EiemRenderReplayLedger *s_eiemActiveRenderReplay = nullptr;
static SRWLOCK s_eiemNativeSkinRefreshLock = SRWLOCK_INIT;
static std::vector<uintptr_t> s_eiemNativeSkinRefreshModels;
// Character assembly hooks are nested. Inner hooks only observe native state;
// the outermost completed boundary commits one replacement generation.
static thread_local uint32_t s_eiemEnclosingModelAssemblyDepth = 0;
static int s_basePartModelOffset = -1;
static int s_basePartConfigOffset = -1;
static int s_basePartConfigPathOffset = -1;
static int s_basePartRenderersOffset = -1;
static int s_basePartRenderersInitStateOffset = -1;
static int s_basePartHgRenderersOffset = -1;
static int s_basePartHgRenderersInitStateOffset = -1;
static int s_basePartMeshesOffset = -1;
static int s_basePartMeshesInitStateOffset = -1;
static int s_basePartBoneClothsOffset = -1;
static int s_basePartLodGroupsOffset = -1;
struct TraceLoadedModelPathEntry {
  void *model = nullptr;
  int64_t pathHash = 0;
  char path[768] = {};
};
static SRWLOCK s_traceLoadedModelPathLock = SRWLOCK_INIT;
static TraceLoadedModelPathEntry s_traceLoadedModelPaths[256] = {};
static size_t s_traceLoadedModelPathCount = 0;
// Set while the reconciliation pass calls Unity's managed setter. The setter
// hooks then forward to their original trampoline instead of recursively
// resolving the replacement that is already being assigned.
static thread_local bool s_eiemApplyingModMeshAssignment = false;
static thread_local bool s_eiemCreatingPartner = false;
// A model pass snapshots the game's completed source palettes before any
// Renderer is mutated. EIEMESH v5 slots can then reuse the exact Transform
// chosen by this concrete world/NPC/UI instance even when another prefab
// spells that bone differently.
static thread_local const std::vector<EiemLiveSkinSource>
    *s_eiemLiveSkinSources = nullptr;
static void TraceSkinnedMeshSetSharedMesh(void *self, void *mesh,
                                           void *methodInfo);
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo);
static void EiemLogSkinSetterTimeline(const char *event, void *renderer,
                                      void *requestedMesh, void *appliedMesh,
                                      void *incomingBones, void *afterBones);
static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                          void *methodInfo);
static size_t EiemApplyStandaloneRenderRulesToSkinArray(
    void *renderers, const char *stage);
static void *EiemReadSharedMesh(void *renderer, const char *rendererType);
static void TraceModelManagerGameObjectAllocate(void *self, void *model,
                                                 void *methodInfo);
static void *TraceModelManagerLoadFromPersistentPool(void *self,
                                                      int64_t pathHash,
                                                      void *methodInfo);
static void TracePrefabInstantiateCompleted(void *self, void *methodInfo);
static void TracePrefabInstantiateUnload(void *self, void *methodInfo);
static void TracePrefabInstantiateClear(void *self, void *methodInfo);
static void TracePrefabInstantiateDispose(void *self, void *methodInfo);
static void *TraceUIModelLoaderLoadModel(void *self, void *path,
                                         void *parent, void *methodInfo);
static int32_t TraceUIModelLoaderLoadModelAsync(void *self, void *path,
                                                void *parent, void *callback,
                                                void *methodInfo);
static void TraceUIModelLoaderUnloadModel(void *self, void *model,
                                          void *methodInfo);
static void TraceUIModelLoaderClear(void *self, void *methodInfo);
static void TraceUIModelLoaderDispose(void *self, void *methodInfo);
static void TraceCharUIModelOnAwake(void *self, void *methodInfo);
static void TraceCharUIModelSetVisible(void *self, bool visible,
                                       void *methodInfo);
static void TraceCharUIModelOnRelease(void *self, void *methodInfo);
static void TraceBasePartFinish(void *self, bool success, void *methodInfo);
static void TraceBasePartPostDeal(void *self, void *methodInfo);
static void TraceComplexPartPostDeal(void *self, void *methodInfo);
static void TraceEntityRenderHelperInitRenderAndMaterial(void *self,
                                                           void *methodInfo);
static void TraceEntityRenderHelperMaterialControllerInit(
    void *self, void *renderers, void *rendererTypeConfigs,
    void *customizeRendererPropertyConfig, bool calculateBoundsWithTransform,
    void *methodInfo);
static int TraceManagedListCount(void *list);
static void EiemLogMaterialControllerRegistry(void *controller,
                                               void *rendererList,
                                               const char *phase);
static void TraceBasePartLoadFinishCallback(void *self, int32_t requestId,
                                            int64_t pathHash, void *model,
                                            void *methodInfo);
static bool TraceBasePartLoadFinishResult(void *self, int32_t requestId,
                                          int64_t pathHash, void *model,
                                          void *methodInfo);
static void TraceBasePartLoadUseHandleFinishCallback(void *self, bool success,
                                                      void *handle,
                                                      void *methodInfo);
static bool TraceBasePartLoadUseHandleFinish(void *self, bool success,
                                             void *handle,
                                             void *methodInfo);
static void TraceBasePartReleaseModel(void *self, void *methodInfo);
static void TraceBasePartOnRelease(void *self, void *methodInfo);
static bool EiemRegisterAndApplyModelInstance(
    EiemModelOwnerKind ownerKind, void *owner, void *model,
    const char *prefabPath, uint32_t instanceUid, const char *stage,
    bool applyResources);
static bool EiemRegisterBaseModelViewPartInstance(void *part,
                                                   const char *stage,
                                                   bool applyResources);
static void *EiemFindBaseModelPartForModel(void *model);
static void EiemLogBaseModelCacheProbe(void *part, LONG transaction,
                                       const char *phase);
static bool EiemRegisterCharUIModelInstance(void *component,
                                             const char *stage,
                                             bool applyResources);
static bool EiemReapplyRegisteredModelInstance(void *model,
                                                const char *stage);

static EiemPerfCounter s_eiemPerfPrefabCompletion;
static EiemPerfCounter s_eiemPerfModelRegistration;
static EiemPerfCounter s_eiemPerfRuleApplication;
static EiemPerfCounter s_eiemPerfComponentSnapshot;
static EiemPerfCounter s_eiemPerfRendererVisit;

static void EiemLogPerformanceSummary(LONG64 prefabCalls) {
  const LONG64 registrationCalls = EiemPerfRead(s_eiemPerfModelRegistration.calls);
  const LONG64 applicationCalls = EiemPerfRead(s_eiemPerfRuleApplication.calls);
  const LONG64 snapshotCalls = EiemPerfRead(s_eiemPerfComponentSnapshot.calls);
  const LONG64 visitCalls = EiemPerfRead(s_eiemPerfRendererVisit.calls);
  Log("[PERF-STARTUP-v1] prefabs=%lld prefabMs=%.2f registerCalls=%lld "
      "registerMs=%.2f applyCalls=%lld applyMs=%.2f snapshotCalls=%lld "
      "snapshotMs=%.2f visitCalls=%lld visitMs=%.2f maxApplyMs=%.2f",
      prefabCalls,
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfPrefabCompletion.ticks)),
      registrationCalls,
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfModelRegistration.ticks)),
      applicationCalls,
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfRuleApplication.ticks)),
      snapshotCalls,
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfComponentSnapshot.ticks)),
      visitCalls,
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfRendererVisit.ticks)),
      EiemPerfMilliseconds(EiemPerfRead(s_eiemPerfRuleApplication.maximum)));
}
static bool EiemApplyStandaloneRenderRules(void *model, const char *stage,
                                           bool *matched = nullptr,
                                           const std::vector<std::string> *affected = nullptr,
                                           std::vector<EiemPhysicsIntent> *physicsIntents = nullptr);
static bool EiemApplyStandaloneRenderRulesToRenderer(
    void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo, const char *stage);
static bool EiemReapplyRendererMaterialsAfterCommit(void *renderer,
                                                     const char *stage);
static bool EiemResolveMeshBonesFromAssembly(
    const EiemSkinIdentity &identity, void *renderer, void **out,
    char *error, size_t errorSize);
static bool EiemBuildRelativeRendererPath(void *rootTransform, void *renderer,
                                          char *out, size_t outSize);
static void *EiemReadLodRendererMesh(void *renderer,
                                     const char **rendererTypeOut);
static bool EiemRenderRuleMatches(const EiemModRule &rule,
                                  const char *relativePath, void *mesh,
                                  const char *asset);
static void EiemForgetModelOwner(EiemModelOwnerKind ownerKind, void *owner,
                                 const char *stage);
static void EiemForgetModelInstance(void *model, const char *stage);
static void EiemSetModelOwnerActive(EiemModelOwnerKind ownerKind, void *owner,
                                    bool active, const char *stage);
static void EiemQueueModReconcile(const char *reason);
static void EiemRequestModUpdate(EiemModUpdate request, const char *reason);
static void EiemQueueNativeSkinRefresh(uintptr_t ownerModel);


static bool EiemOnUnityThread() {
  const DWORD current = GetCurrentThreadId();
  if (s_eiemUnityThreadId) return current == s_eiemUnityThreadId;
  if (!g_gameHwnd) return false;
  DWORD windowProcess = 0;
  const DWORD windowThread = GetWindowThreadProcessId(g_gameHwnd, &windowProcess);
  if (!windowThread || windowProcess != GetCurrentProcessId() ||
      windowThread != current)
    return false;
  InterlockedCompareExchange((volatile LONG *)&s_eiemUnityThreadId,
                             (LONG)current, 0);
  return true;
}

// These assembly callbacks execute on the game's Unity thread before the
// window-procedure path necessarily observes its first message.  Pin the
// thread from the callback that is about to invoke Unity setters; otherwise
// the startup resource pass is rejected as `unityThread=0` and no replacement
// can be built until a later F10 reconcile.
static void EiemAdoptUnityThreadFromAssemblyHook(const char *hook) {
  const DWORD current = GetCurrentThreadId();
  const LONG previous = InterlockedCompareExchange(
      (volatile LONG *)&s_eiemUnityThreadId, (LONG)current, 0);
  if (!previous)
    Log("[MOD-THREAD] Unity thread adopted from %s: %lu",
        hook ? hook : "assembly hook", (unsigned long)current);
}

static int32_t EiemTraceUnboxInt(void *boxed) {
  __try {
    return boxed ? *(int32_t *)((char *)boxed + 16) : -1;
  } __except (1) {
    return -1;
  }
}


static void EiemSetOriginalSharedMesh(void *renderer, void *mesh,
                                      const char *rendererType,
                                      void *methodInfo) {
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    auto original = (TraceSetSharedMeshFn)s_origSkinnedMeshSetSharedMesh;
    if (original) original(renderer, mesh, methodInfo);
  } else {
    auto original = (TraceSetSharedMeshFn)s_origMeshFilterSetSharedMesh;
    if (original) original(renderer, mesh, methodInfo);
  }
}

static bool EiemSetSharedMesh(void *renderer, void *mesh,
                              const char *rendererType, void *methodInfo) {
  if (EiemReadSharedMesh(renderer, rendererType) == mesh) return true;
  if (methodInfo) {
    EiemSetOriginalSharedMesh(renderer, mesh, rendererType, methodInfo);
    return EiemReadSharedMesh(renderer, rendererType) == mesh;
  }
  void *setter = EiemModEquals(rendererType, "SkinnedMeshRenderer")
                     ? g_smr_set_sharedMesh
                     : g_meshFilter_set_sharedMesh;
  if (!setter) {
    Log("[MOD] %s setter is unavailable; reconciliation cannot assign mesh",
        rendererType ? rendererType : "Renderer");
    return false;
  }
  void *params[] = {mesh};
  s_eiemApplyingModMeshAssignment = true;
  Invoke(setter, renderer, params);
  s_eiemApplyingModMeshAssignment = false;
  void *actual = EiemReadSharedMesh(renderer, rendererType);
  if (actual != mesh)
    Log("[MOD] %s sharedMesh verification failed: renderer=%p expected=%p actual=%p",
        rendererType ? rendererType : "Renderer", renderer, mesh, actual);
  return actual == mesh;
}

static void *EiemReadSharedMesh(void *renderer, const char *rendererType) {
  if (!renderer) return nullptr;
  void *getter = EiemModEquals(rendererType, "SkinnedMeshRenderer")
                     ? g_smr_get_sharedMesh
                     : g_meshFilter_get_sharedMesh;
  return getter ? Invoke(getter, renderer) : nullptr;
}

static bool EiemReadLiveMeshIdentity(void *mesh, char *source, size_t sourceSize,
                                     char *asset, size_t assetSize) {
  if (!mesh || !asset || assetSize == 0) return false;
  char description[512] = {};
  TraceDescribeObject(mesh, description, sizeof(description));
  EiemExtractObjectName(description, asset, assetSize);
  if (!asset[0]) return false;
  if (source && sourceSize) {
    int64_t hash = 0;
    TraceLookupAssetOrigin(mesh, &hash, source, sourceSize);
  }
  return true;
}

static void EiemReadLiveMeshShape(void *mesh, int32_t *vertices,
                                  int32_t *indices, int32_t *subMeshes) {
  if (vertices) *vertices = -1;
  if (indices) *indices = -1;
  if (subMeshes) *subMeshes = -1;
  if (!mesh) return;
  if (vertices)
    *vertices = g_mesh_get_vertexCount ? EiemTraceUnboxInt(Invoke(g_mesh_get_vertexCount, mesh)) : -1;
  if (subMeshes)
    *subMeshes = g_mesh_get_subMeshCount ? EiemTraceUnboxInt(Invoke(g_mesh_get_subMeshCount, mesh)) : -1;
  if (indices) {
    *indices = -1;
    if (g_mesh_GetIndexCount && *subMeshes >= 0 && *subMeshes <= 64) {
      int32_t total = 0;
      bool valid = true;
      for (int32_t index = 0; index < *subMeshes; ++index) {
        void *params[] = {&index};
        const int32_t count = EiemTraceUnboxInt(Invoke(g_mesh_GetIndexCount, mesh, params));
        if (count < 0 || total > INT32_MAX - count) { valid = false; break; }
        total += count;
      }
      if (valid) *indices = total;
    }
  }
}

// Set only while a matched Prefab declaration is applying its Render actions.
// It associates every mutation with one concrete Prefab instance so unload
// cleanup and hot reload never need a scene-wide identity guess.
static thread_local uintptr_t s_eiemActivePrefabInstance = 0;

#include "eiem_render_state.h"
static bool EiemManagedObjectArraySame(void *left, void *right);
static size_t EiemManagedArrayLength(void *array);
static void EiemProbePartnerBoneBindings(void *sourceRenderer = nullptr,
                                         const char *boundary = "unknown");
static void EiemTraceRendererOwnerCorrelation(void *renderer,
                                              uintptr_t modelKey,
                                              const char *stage,
                                              LONG generation);

#include "eiem_shape_state.h"
#include "eiem_shape_guard.h"

// Runtime shape ownership is per Renderer, never per shared Mesh. No Unity
// calls on ImGui's thread; mutation failures are reported through the adapter.
struct EiemUnityShapes {
  bool Names(void *mesh,std::vector<std::string> &names) {
    void *boxed=nullptr;
    if(!mesh || !InvokeChecked(g_mesh_get_blendShapeCount,mesh,nullptr,&boxed) || !boxed)return false;
    const int count=*(int *)((char *)boxed+16);if(count<0)return false;
    std::vector<std::string> result;
    for(int i=0;i<count;++i){void *text=nullptr;void *args[]={&i};char name[192]={};
      if(!InvokeChecked(g_mesh_GetBlendShapeName,mesh,args,&text) || !text)return false;
      ReadStrUtf8(text,name,sizeof(name));if(strlen(name)>=sizeof(name)-1)return false;
      result.emplace_back(name);
    }
    names=std::move(result);return true;
  }
  bool Snapshot(void *renderer, void *mesh, std::vector<EiemShapeBaseline> &weights) {
    std::vector<std::string> names;
    if (!Names(mesh,names)) return false;
    std::vector<EiemShapeBaseline> values;
    for (int i = 0; i < (int)names.size(); ++i) {
      float value = 0;
      if (!Read(renderer, i, value)) return false;
      values.push_back({names[i], value});
    }
    weights = std::move(values);
    return true;
  }
  int Index(void *mesh, const std::string &name) {
    void *boxed = nullptr;
    if (!InvokeChecked(g_mesh_get_blendShapeCount, mesh, nullptr, &boxed) || !boxed) return -1;
    int count = *(int *)((char *)boxed + 16);
    for (int i = 0; i < count; ++i) {
      void *text = nullptr; void *args[] = {&i};
      if (!InvokeChecked(g_mesh_GetBlendShapeName, mesh, args, &text) || !text) return -1;
      char actual[192] = {}; ReadStrUtf8(text, actual, sizeof(actual));
      if (name == actual) return i;
    }
    return -1;
  }
  bool Read(void *renderer, int index, float &value) {
    EiemShapeAuthorScope origin;
    void *boxed = nullptr; void *args[] = {&index};
    if (!InvokeChecked(g_smr_GetBlendShapeWeight, renderer, args, &boxed) || !boxed) return false;
    value = *(float *)((char *)boxed + 16);
    return std::isfinite(value);
  }
  bool Write(void *renderer, int index, float value) {
    EiemShapeAuthorScope origin;
    void *result = nullptr; void *args[] = {&index, &value};
    return InvokeChecked(g_smr_SetBlendShapeWeight, renderer, args, &result);
  }
};
static SRWLOCK s_eiemShapeMessageLock = SRWLOCK_INIT;
static std::string s_eiemShapeMessage;
static void EiemShapeMessage(const EiemModRule &rule, const std::string &error) {
  const std::string text = std::string(rule.modPath) + " / " + rule.section + ": " + error;
  AcquireSRWLockExclusive(&s_eiemShapeMessageLock);
  if (text != s_eiemShapeMessage) Log("[SHAPE] %s", text.c_str());
  s_eiemShapeMessage = text;
  ReleaseSRWLockExclusive(&s_eiemShapeMessageLock);
}
static bool EiemUpdateRendererShapes(void *renderer, const char *type,
                                      const EiemModRule &rule, EiemShapeState &state,
                                      float elapsedSeconds = -1.0f) {
  if (!rule.shapeCount && state.owned.empty()) return true;
  if (!EiemOnUnityThread()) { EiemShapeMessage(rule, "Shape update requires Unity thread"); return false; }
  if (!EiemModEquals(type, "SkinnedMeshRenderer")) {
    EiemShapeMessage(rule, "Shape weights require SkinnedMeshRenderer"); return false;
  }
  EiemUnityShapes backend; std::string error;
  EiemShapeAuthorScope author;
  void *mesh=EiemReadSharedMesh(renderer,type);
  if(rule.shapeCount && state.binding && !EiemShapeBindingMatches(state,renderer,mesh)) {
    EiemRetireShapeBinding(state.binding);state={};
  }
  if(!state.binding && rule.shapeCount) {
    std::vector<EiemShapeBaseline> baseline;
    if(!backend.Snapshot(renderer,mesh,baseline) || !EiemPrepareShapeBinding(state,renderer,mesh,baseline,backend,error)) {
      EiemShapeMessage(rule,error.empty()?"Cannot capture native shape state":error);return false;
    }
    if(state.binding)state.binding->initialized=true; // No mesh assignment, keep current weights.
  }
  EiemSyncShapeClaims(state,rule);
  const bool applied = EiemApplyShapeWeights(renderer, mesh, rule, state, backend, error, elapsedSeconds);
  if (!applied)
    EiemShapeMessage(rule, error);
  EiemModRule none={};EiemSyncShapeClaims(state,none);
  return applied;
}

#include "eiem_render_override.h"

struct EiemResolvedRenderRule {
  EiemModRule rule = {};
  char source[768] = {};
  char asset[192] = {};
};

struct EiemPartnerState {
  std::shared_ptr<EiemSkeletonInstance> skeleton;
  std::shared_ptr<const EiemSkinIdentity> skin;
  void *sourceRenderer = nullptr;
  void *sourceDrawRenderer = nullptr;
  void *partnerObject = nullptr;
  void *partnerRenderer = nullptr;
  LONG generation = -1;
  uintptr_t ownerPrefabInstance = 0;
  char sourceSection[96] = {};
  char section[96] = {};
  char modPath[MAX_PATH] = {};
  EiemShapeState shapes;
  char rendererType[32] = {};
  // Bone palette length committed to this Partner at its last resource
  // publish.  This is an observation baseline for diagnostics; it is not an
  // acceptance claim about the game's internal Animator registry.
  size_t expectedBoneCount = 0;
  // Set only after a concrete game-owned Renderer array contains this
  // Partner.  Public SMR bones/rootBone readback alone does not establish
  // that registration, so keep this as an observation for the boundary log.
  bool skinArrayObserved = false;
  // A key-controlled partner remains constructed for the lifetime of this
  // model generation. Controls only change draw/LOD visibility.
  bool controlVisible = true;
  bool enabledWhenVisible = true;
};
static SRWLOCK s_eiemPartnerLock = SRWLOCK_INIT;
static std::vector<EiemPartnerState> s_eiemPartners;

static bool EiemIsPartnerRenderer(void *renderer) {
  AcquireSRWLockShared(&s_eiemPartnerLock);
  bool owned = false;
  for (const auto &state : s_eiemPartners) {
    if (state.partnerRenderer == renderer) { owned = true; break; }
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  return owned;
}

struct EiemPartnerBonesSnapshot {
  bool known = false;
  void *sourceRenderer = nullptr;
  void *sourceDrawRenderer = nullptr;
  LONG generation = -1;
  size_t expectedBoneCount = 0;
  bool skinArrayObserved = false;
  bool controlVisible = false;
  char section[96] = {};
};

// Read only the native bookkeeping for one Partner.  The set_bones hook can
// run while the game is rebuilding its skin arrays, so no Unity call is made
// while holding the Partner lock.  This is deliberately a snapshot rather
// than a repair path: it lets a log distinguish a game-owned palette write
// from our own F10 assignment without changing either path.
static bool EiemSnapshotPartnerBones(void *renderer,
                                     EiemPartnerBonesSnapshot *out) {
  if (!renderer || !out) return false;
  *out = {};
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners) {
    if (state.partnerRenderer != renderer) continue;
    out->known = true;
    out->sourceRenderer = state.sourceRenderer;
    out->sourceDrawRenderer = state.sourceDrawRenderer;
    out->generation = state.generation;
    out->expectedBoneCount = state.expectedBoneCount;
    out->skinArrayObserved = state.skinArrayObserved;
    out->controlVisible = state.controlVisible && state.enabledWhenVisible;
    strncpy_s(out->section, sizeof(out->section), state.section, _TRUNCATE);
    break;
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  return out->known;
}

// Snapshot the known source/Partner relationships before a game-owned
// registration callback is logged.  The trace header only sees raw managed
// arrays; this adapter supplies the concrete Renderer pointers and the current
// key visibility state without invoking Unity while holding the Partner lock.
static void EiemTraceKnownPartnerArrayMembers(const char *boundary,
                                              void *owner, void *array,
                                              size_t count, LONG generation) {
  if (!array || !count) return;
  std::vector<EiemRegistrationTracePartnerEntry> entries;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  entries.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer && !state.partnerRenderer) continue;
    EiemRegistrationTracePartnerEntry entry = {};
    entry.source = state.sourceRenderer;
    entry.partner = state.partnerRenderer;
    entry.ownerModel = (void *)state.ownerPrefabInstance;
    entry.visible = state.controlVisible && state.enabledWhenVisible;
    strncpy_s(entry.section, sizeof(entry.section), state.section, _TRUNCATE);
    entries.push_back(entry);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  for (const auto &entry : entries)
    EiemTraceRendererOwnerCorrelation(entry.source, (uintptr_t)entry.ownerModel,
                                      boundary, generation);
  if (!entries.empty())
    EiemRegistrationTraceArrayMembers(boundary, owner, array, count,
                                       generation, entries.data(),
                                       entries.size());
}

static void EiemManagedObjectTypeName(void *object, char *out,
                                      size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = 0;
  if (!object || !il2cpp_object_get_class || !il2cpp_class_get_name) return;
  __try {
    void *klass = il2cpp_object_get_class(object);
    const char *name = klass ? il2cpp_class_get_name(klass) : nullptr;
    const char *nameSpace =
        klass && il2cpp_class_get_namespace
            ? il2cpp_class_get_namespace(klass)
            : nullptr;
    if (nameSpace && nameSpace[0])
      _snprintf_s(out, outSize, _TRUNCATE, "%s.%s", nameSpace,
                  name ? name : "?");
    else
      strncpy_s(out, outSize, name ? name : "?", _TRUNCATE);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    strncpy_s(out, outSize, "<exception>", _TRUNCATE);
  }
}

static bool EiemManagedArrayFindPointer(void *array, size_t count,
                                        void *needle, size_t *indexOut) {
  if (indexOut) *indexOut = SIZE_MAX;
  if (!array || !needle || !indexOut || count > 8192) return false;
  __try {
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < count; ++index) {
      if (items[index] == needle) {
        *indexOut = index;
        return true;
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return false;
}

// Only describe arrays that contain a concrete source Renderer with known
// Partners. This keeps the registration trace bounded while proving the
// relationship between the Renderer array and its parallel skinning input.
static void EiemTraceKnownPartnerParallelArrays(const char *boundary,
                                                void *primary,
                                                void *parallel,
                                                LONG generation) {
  const size_t primaryCount = EiemManagedArrayLength(primary);
  if (!primary || primaryCount == 0 || primaryCount > 8192) return;

  std::vector<EiemRegistrationTracePartnerEntry> entries;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  entries.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer || !state.partnerRenderer) continue;
    EiemRegistrationTracePartnerEntry entry = {};
    entry.source = state.sourceRenderer;
    entry.partner = state.partnerRenderer;
    strncpy_s(entry.section, sizeof(entry.section), state.section, _TRUNCATE);
    entries.push_back(entry);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  if (entries.empty()) return;

  size_t sourceIndex = SIZE_MAX;
  const char *sourceSection = nullptr;
  for (const auto &entry : entries) {
    size_t index = SIZE_MAX;
    if (EiemManagedArrayFindPointer(primary, primaryCount, entry.source,
                                    &index) &&
        index < sourceIndex) {
      sourceIndex = index;
      sourceSection = entry.section;
    }
  }
  if (sourceIndex == SIZE_MAX) return;

  char primaryType[192] = {}, parallelType[192] = {};
  EiemManagedObjectTypeName(primary, primaryType, sizeof(primaryType));
  EiemManagedObjectTypeName(parallel, parallelType, sizeof(parallelType));
  EiemRegistrationTraceParallelArrays(
      boundary, primary, primaryType, primaryCount, parallel, parallelType,
      EiemManagedArrayLength(parallel), sourceIndex, sourceSection, generation);
}

struct EiemPartnerArrayAppend {
  void *partner = nullptr;
  size_t sourceIndex = SIZE_MAX;
  char section[96] = {};
};

static bool EiemCopyExpandedRendererPointers(
    void *source, void *destination, size_t oldCount,
    const EiemPartnerArrayAppend *append, size_t appendCount) {
  if (!source || !destination || (appendCount && !append)) return false;
  __try {
    void **sourceItems = (void **)((char *)source + IL2CPP_ARRAY_DATA);
    void **destinationItems =
        (void **)((char *)destination + IL2CPP_ARRAY_DATA);
    memcpy(destinationItems, sourceItems, oldCount * sizeof(void *));
    for (size_t index = 0; index < appendCount; ++index)
      destinationItems[oldCount + index] = append[index].partner;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return true;
}

static size_t EiemManagedArrayValueElementSize(void *array,
                                               bool *valueTypeOut) {
  if (valueTypeOut) *valueTypeOut = false;
  if (!array || !il2cpp_object_get_class || !il2cpp_class_get_element_class ||
      !il2cpp_class_get_type || !il2cpp_type_get_type ||
      !il2cpp_class_value_size)
    return 0;
  __try {
    void *arrayClass = il2cpp_object_get_class(array);
    void *elementClass = arrayClass
                             ? il2cpp_class_get_element_class(arrayClass)
                             : nullptr;
    void *elementType = elementClass ? il2cpp_class_get_type(elementClass)
                                     : nullptr;
    const int typeCode = elementType ? il2cpp_type_get_type(elementType) : -1;
    // IL2CPP_TYPE_VALUETYPE is 0x11. Reference arrays store one object
    // pointer per element and need no class-size query.
    if (typeCode != 0x11) return sizeof(void *);
    uint32_t alignment = 0;
    const int32_t size = il2cpp_class_value_size(elementClass, &alignment);
    if (size <= 0 || size > 4096) return 0;
    if (valueTypeOut) *valueTypeOut = true;
    return (size_t)size;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}

static bool EiemCopyExpandedManagedArray(void *source, void *destination,
                                         size_t oldCount, size_t elementSize,
                                         const size_t *sourceIndices,
                                         size_t appendCount) {
  if (!source || !destination || !elementSize ||
      (appendCount && !sourceIndices))
    return false;
  __try {
    const char *sourceData = (const char *)source + IL2CPP_ARRAY_DATA;
    char *destinationData = (char *)destination + IL2CPP_ARRAY_DATA;
    memcpy(destinationData, sourceData, oldCount * elementSize);
    for (size_t index = 0; index < appendCount; ++index) {
      const size_t sourceIndex = sourceIndices[index];
      if (sourceIndex >= oldCount) return false;
      memcpy(destinationData + (oldCount + index) * elementSize,
             sourceData + sourceIndex * elementSize, elementSize);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return true;
}

// Register every already-created Partner in the game's own skinning input.
// This runs only at the CreateSMSInfo output boundary, before AssignSkin
// consumes the arrays. Keys still affect only Renderer/LOD visibility; they
// never add or remove array entries after this point.
// Read one RootBoneInfo value slot.
//
// Beyond.NPC.Avatar.RootBoneInfo is a 16-byte value type:
//   String rootBoneName; Int32 rootBoneID; Boolean deferEnableRestore; Boolean rendererEnabled
// The game resolves `rootBoneID` through the avatar templet's global bone list
// (NPCAvatarCreatorUtils._FindTransformByBoneID(Animator, int, ...)) to obtain
// the SMR rootBone. The plugin has always memcpy'd the *source* slot's value
// into every appended Partner slot and never read a single field, so a Partner
// whose own Mesh bone list differs from its source inherits a root-bone identity
// that was never checked against it. Record the value before touching anything.
struct EiemRootBoneInfoValue {
  bool readable = false;
  char name[192] = {};
  int32_t id = -1;
  int deferEnableRestore = -1;
  int rendererEnabled = -1;
};

static bool EiemReadRootBoneInfoAt(void *rootBones, size_t index,
                                   size_t elementSize, size_t count,
                                   EiemRootBoneInfoValue *out) {
  if (!out) return false;
  *out = {};
  // RootBoneInfo is a 16-byte value type on the current IL2CPP build:
  // pointer (8) + bone id (4) + two boolean flags (1 each) + padding.
  if (!rootBones || index >= count || elementSize < 16) return false;
  __try {
    const char *slot = (const char *)rootBones + IL2CPP_ARRAY_DATA +
                       index * elementSize;
    void *name = *(void **)slot;
    out->id = *(int32_t *)(slot + 8);
    out->deferEnableRestore = *(uint8_t *)(slot + 12) ? 1 : 0;
    out->rendererEnabled = *(uint8_t *)(slot + 13) ? 1 : 0;
    if (name) ReadStrUtf8(name, out->name, sizeof(out->name));
    out->readable = true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out->readable = false;
  }
  return out->readable;
}

static bool EiemRegisterPartnersInSkinArrays(void **renderersOut,
                                             void **rootBonesOut,
                                             LONG generation,
                                             const char *stage) {
  if (!renderersOut || !rootBonesOut || !*renderersOut || !*rootBonesOut ||
      !il2cpp_array_new_specific || !il2cpp_object_get_class)
    return false;
  void *renderers = *renderersOut;
  void *rootBones = *rootBonesOut;
  const size_t rendererCount = EiemManagedArrayLength(renderers);
  const size_t rootBoneCount = EiemManagedArrayLength(rootBones);
  if (!rendererCount || rendererCount != rootBoneCount ||
      rendererCount > 8192)
    return false;

  std::vector<EiemPartnerArrayAppend> append;
  size_t knownPartners = 0, sourceAbsent = 0, alreadyInArray = 0;
  void *firstAbsentSource = nullptr;
  const char *firstAbsentSection = "<none>";
  AcquireSRWLockShared(&s_eiemPartnerLock);
  append.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer || !state.partnerRenderer) continue;
    ++knownPartners;
    size_t sourceIndex = SIZE_MAX;
    if (!EiemManagedArrayFindPointer(renderers, rendererCount,
                                     state.sourceRenderer, &sourceIndex)) {
      // The game builds one renderer array per model instance and LOD level.
      // A Partner whose source Renderer is not a member of THIS array cannot be
      // paired with a RootBoneInfo slot here and is therefore never handed to
      // the game's skin assembly for this pass. That used to return silently,
      // which is why a Partner could stay unregistered while every log line
      // still looked healthy. Record it instead of dropping it.
      ++sourceAbsent;
      if (!firstAbsentSource) {
        firstAbsentSource = state.sourceRenderer;
        firstAbsentSection = state.section;
      }
      continue;
    }
    size_t existingIndex = SIZE_MAX;
    if (EiemManagedArrayFindPointer(renderers, rendererCount,
                                    state.partnerRenderer, &existingIndex)) {
      ++alreadyInArray;
      continue;
    }
    bool duplicate = false;
    for (const auto &candidate : append)
      if (candidate.partner == state.partnerRenderer) {
        duplicate = true;
        break;
      }
    if (duplicate) continue;
    EiemPartnerArrayAppend candidate = {};
    candidate.partner = state.partnerRenderer;
    candidate.sourceIndex = sourceIndex;
    strncpy_s(candidate.section, sizeof(candidate.section), state.section,
              _TRUNCATE);
    append.push_back(candidate);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  if (sourceAbsent &&
      EiemRegistrationTraceFirst("partner-source-absent", stage, renderers,
                                 rootBones, firstAbsentSource, generation))
    Log("[MOD-REG-GAP] stage=%s arrayCount=%zu sourceHits=%zu knownPartners=%zu "
        "alreadyInArray=%zu sourceAbsent=%zu firstAbsentSource=%p "
        "firstAbsentSection=%s appended=%zu",
        stage ? stage : "unknown", rendererCount,
        rendererCount >= sourceAbsent ? rendererCount - sourceAbsent : 0,
        knownPartners, alreadyInArray, sourceAbsent, firstAbsentSource,
        firstAbsentSection, append.size());
  if (append.empty()) return false;
  if (append.size() > 8192 - rendererCount) return false;

  const size_t newCount = rendererCount + append.size();
  void *rendererClass = il2cpp_object_get_class(renderers);
  void *rootBoneClass = il2cpp_object_get_class(rootBones);
  if (!rendererClass || !rootBoneClass) return false;
  bool rootBonesAreValueType = false;
  const size_t rootBoneElementSize = EiemManagedArrayValueElementSize(
      rootBones, &rootBonesAreValueType);
  if (!rootBoneElementSize) return false;
  void *expandedRenderers =
      il2cpp_array_new_specific(rendererClass, newCount);
  void *expandedRootBones =
      il2cpp_array_new_specific(rootBoneClass, newCount);
  if (!expandedRenderers || !expandedRootBones) return false;

  std::vector<size_t> sourceIndices;
  sourceIndices.reserve(append.size());
  for (const auto &candidate : append)
    sourceIndices.push_back(candidate.sourceIndex);
  if (!EiemCopyExpandedRendererPointers(
          renderers, expandedRenderers, rendererCount, append.data(),
          append.size()) ||
      !EiemCopyExpandedManagedArray(rootBones, expandedRootBones,
                                    rootBoneCount, rootBoneElementSize,
                                    sourceIndices.data(),
                                    sourceIndices.size())) {
    // A failed expansion used to look exactly like "no Partner needed appending".
    Log("[MOD-REG-FAIL] stage=%s arrayCount=%zu append=%zu elementSize=%zu "
        "reason=expansion-copy",
        stage ? stage : "unknown", rendererCount, append.size(),
        rootBoneElementSize);
    return false;
  }

  *renderersOut = expandedRenderers;
  *rootBonesOut = expandedRootBones;
  // Record the root-bone identity the game will resolve for each appended
  // Partner versus the source slot it was copied from. A mismatch here means
  // the Partner's SMR.rootBone is resolved to a bone that is not its own root.
  for (const auto &candidate : append) {
    if (!EiemRegistrationTraceFirst("partner-rootbone-info", candidate.section,
                                    candidate.partner, renderers, nullptr,
                                    generation))
      continue;
    EiemRootBoneInfoValue sourceValue, partnerValue;
    const bool sourceRead = EiemReadRootBoneInfoAt(
        rootBones, candidate.sourceIndex, rootBoneElementSize, rootBoneCount,
        &sourceValue);
    const bool partnerRead = EiemReadRootBoneInfoAt(
        expandedRootBones, rendererCount + (&candidate - append.data()),
        rootBoneElementSize, newCount, &partnerValue);
    void *partnerRoot = g_smr_get_rootBone
                            ? Invoke(g_smr_get_rootBone, candidate.partner)
                            : nullptr;
    char partnerRootName[160] = {};
    TraceReadUnityObjectName(partnerRoot, partnerRootName,
                             sizeof(partnerRootName));
    Log("[MOD-ROOTBONE-INFO] stage=%s section=%s sourceIndex=%zu "
        "sourceRead=%d sourceName=%s sourceID=%d sourceDefer=%d sourceEnabled=%d "
        "partnerRead=%d partnerName=%s partnerID=%d partnerDefer=%d "
        "partnerEnabled=%d smrRootBone=%p smrRootBoneName=%s",
        stage ? stage : "unknown", candidate.section, candidate.sourceIndex,
        sourceRead ? 1 : 0,
        sourceValue.name[0] ? sourceValue.name : "<none>", sourceValue.id,
        sourceValue.deferEnableRestore, sourceValue.rendererEnabled,
        partnerRead ? 1 : 0,
        partnerValue.name[0] ? partnerValue.name : "<none>", partnerValue.id,
        partnerValue.deferEnableRestore, partnerValue.rendererEnabled,
        partnerRoot, partnerRootName[0] ? partnerRootName : "<unknown>");
  }
  // Record the concrete post-expansion membership while both managed arrays
  // are still available.  This is observation only; it does not retain the
  // array or alter any Renderer after the game's callback returns.
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  void **publishedItems =
      (void **)((char *)expandedRenderers + IL2CPP_ARRAY_DATA);
  for (auto &state : s_eiemPartners) {
    if (!state.partnerRenderer) continue;
    for (size_t index = 0; index < newCount; ++index) {
      if (publishedItems[index] == state.partnerRenderer) {
        state.skinArrayObserved = true;
        break;
      }
    }
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  if (EiemRegistrationTraceFirst("partner-array-register", stage, renderers,
                                 rootBones, nullptr, generation)) {
    Log("[MOD-REG] stage=%s sourceCount=%zu appended=%zu newCount=%zu "
        "rootBoneElementSize=%zu rootBonesValueType=%d",
        stage ? stage : "unknown", rendererCount, append.size(), newCount,
        rootBoneElementSize, rootBonesAreValueType ? 1 : 0);
  }
  return true;
}

static size_t EiemFindPartnerLocked(void *sourceRenderer, const char *modPath, const char *section,
                                    LONG generation) {
  for (size_t i = 0; i < s_eiemPartners.size(); ++i) {
    const auto &state = s_eiemPartners[i];
    if (state.sourceRenderer == sourceRenderer &&
        EiemModEquals(state.modPath, modPath) &&
        state.generation == generation &&
        _stricmp(state.section, section ? section : "") == 0)
      return i;
  }
  return SIZE_MAX;
}

static bool EiemReadRendererForceRenderingOff(void *renderer, bool *off) {
  if (off) *off = false;
  if (!renderer || !off || !g_renderer_get_forceRenderingOff) return false;
  __try {
    void *boxed = Invoke(g_renderer_get_forceRenderingOff, renderer);
    if (!boxed) return false;
    *off = *(bool *)((char *)boxed + 16);
    return true;
  } __except (1) {
    return false;
  }
}

// F10 advances the Mod configuration generation, but it does not replace the
// game's model/skin instance. Reuse the existing Partner Renderer across that
// boundary so the game's internal skin registration remains attached to the
// same component. The caller refreshes its resource payload and generation.
static size_t EiemFindPartnerAnyGenerationLocked(
    void *sourceRenderer, const char *modPath, const char *section) {
  for (size_t i = 0; i < s_eiemPartners.size(); ++i) {
    const auto &state = s_eiemPartners[i];
    if (state.sourceRenderer == sourceRenderer &&
        EiemModEquals(state.modPath, modPath) &&
        _stricmp(state.section, section ? section : "") == 0)
      return i;
  }
  return SIZE_MAX;
}

// Unity stores LODGroup.lods as a managed array of the value type
// UnityEngine.LOD.  We only touch that array when a partner is created or
// removed; ordinary resource replacement never walks LOD metadata.
struct EiemNativeLod {
  float screenRelativeTransitionHeight;
  float fadeTransitionWidth;
  void *renderers;
};

static size_t EiemManagedArrayLength(void *array) {
  if (!array) return 0;
  __try {
    const uintptr_t length = *(uintptr_t *)((char *)array + 24);
    return length > 100000 ? 0 : (size_t)length;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}

static bool EiemManagedObjectArraySame(void *left, void *right) {
  if (left == right) return true;
  const size_t leftCount = EiemManagedArrayLength(left);
  const size_t rightCount = EiemManagedArrayLength(right);
  if (leftCount != rightCount) return false;
  if (!left || !right) return left == right;
  __try {
    void **leftItems = (void **)((char *)left + 32);
    void **rightItems = (void **)((char *)right + 32);
    for (size_t index = 0; index < leftCount; ++index)
      if (leftItems[index] != rightItems[index]) return false;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static bool EiemAssignRendererMaterials(void *renderer, void *materials,
                                        char *error, size_t errorSize) {
  if (!renderer || !materials || !s_eiemRendererSetSharedMaterials ||
      !g_renderer_get_sharedMaterials) {
    if (error)
      strncpy_s(error, errorSize,
                "Unity Renderer material assignment APIs are unavailable",
                _TRUNCATE);
    return false;
  }
  void *current = Invoke(g_renderer_get_sharedMaterials, renderer);
  if (EiemManagedObjectArraySame(materials, current)) return true;
  void *params[] = {materials};
  Invoke(s_eiemRendererSetSharedMaterials, renderer, params);
  void *actual = Invoke(g_renderer_get_sharedMaterials, renderer);
  if (!EiemManagedObjectArraySame(materials, actual)) {
    if (error)
      strncpy_s(error, errorSize,
                "Unity Renderer material assignment read-back failed",
                _TRUNCATE);
    return false;
  }
  return true;
}

// Expose the existing per-instance baseline while RendererInfo._Init reads
// Renderer.sharedMaterials into its sourceMaterials table. No controller field
// offsets are written and no baseline/ownership record is released here.
static bool EiemExposeSourceMaterialsForInit(void *renderer) {
  if (!renderer || !EiemOnUnityThread()) return false;
  EiemUnityRef originalRef;
  std::vector<size_t> owned;
  bool hasBaseline = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  for (const auto &state : s_eiemOverrides) {
    if (state.drawRenderer != renderer || state.drawRendererRef.Target() != renderer ||
        !state.hasMaterials) continue;
    hasBaseline = true;
    void *original = state.originalMaterialsHandle && il2cpp_gchandle_get_target
                         ? il2cpp_gchandle_get_target(state.originalMaterialsHandle) : nullptr;
    originalRef = EiemUnityRef::Capture(original, false);
    owned = state.materialSlots;
    break;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (!hasBaseline) return true; // not modified by EIEM
  if (!originalRef || EiemNativeObjectStatus(renderer) != 1 ||
      !g_renderer_get_sharedMaterials || !s_eiemMaterialClass) {
    Log("[MOD-MATERIAL-SOURCE] cannot expose source slots before init renderer=%p", renderer);
    return false;
  }
  void *original = originalRef.Target();
  void *current = Invoke(g_renderer_get_sharedMaterials, renderer);
  if (!original || !current) {
    Log("[MOD-MATERIAL-SOURCE] source/current slots unavailable renderer=%p", renderer);
    return false;
  }
  const size_t originalCount = EiemManagedArrayLength(original);
  const size_t currentCount = EiemManagedArrayLength(current);
  void **originalItems = (void **)((char *)original + IL2CPP_ARRAY_DATA);
  void **currentItems = (void **)((char *)current + IL2CPP_ARRAY_DATA);
  std::vector<void *> baseline(originalItems, originalItems + originalCount);
  std::vector<void *> live(currentItems, currentItems + currentCount);
  auto restored = EiemRestoreOwnedSlots(live, baseline, owned);
  for (size_t slot : owned) {
    if (slot < restored.size() && restored[slot] && EiemNativeObjectStatus(restored[slot]) != 1) {
      Log("[MOD-MATERIAL-SOURCE] source material unavailable renderer=%p slot=%zu", renderer, slot);
      return false;
    }
  }
  if (restored == live) return true;
  void *array = il2cpp_array_new(s_eiemMaterialClass, restored.size());
  char error[256] = {};
  if (!array) {
    Log("[MOD-MATERIAL-SOURCE] cannot allocate source slots renderer=%p", renderer);
    return false;
  }
  if (!restored.empty())
    memcpy((char *)array + IL2CPP_ARRAY_DATA, restored.data(), restored.size() * sizeof(void *));
  if (!EiemAssignRendererMaterials(renderer, array, error, sizeof(error))) {
    Log("[MOD-MATERIAL-SOURCE] source exposure failed renderer=%p error=%s", renderer, error);
    return false;
  }
  Log("[MOD-MATERIAL-SOURCE] exposed source before controller init renderer=%p ownedSlots=%zu", renderer, owned.size());
  return true;
}

// Resolve resource-local slots against the current instance's shared skeleton.
// This does not change any Renderer; callers prepare everything before writing.
static bool EiemResolveMeshBones(const EiemSkinIdentity &identity, void *renderer,
                                   void **out, char *error, size_t errorSize) {
  if (out) *out = nullptr;
  auto reject = [&](const std::string &reason) {
    if (error) strncpy_s(error, errorSize, reason.c_str(), _TRUNCATE);
    return false;
  };
  if (!EiemOnUnityThread() || !g_smr_get_bones)
    return reject("Shared skeleton APIs are unavailable");
  void *current = EiemBackendInvokeNoThrow(g_smr_get_bones, renderer);
  const size_t count = EiemManagedArrayLength(current);
  if (!current || !count || count > 4096) return reject("Source renderer has no valid bone palette");
  void **items = (void **)((char *)current + IL2CPP_ARRAY_DATA);
  // Blender preserves the source palette as an ordered prefix and appends
  // genuinely new slots.  The source Renderer already owns the correct
  // Transform references for that prefix.  Do not resolve those entries by
  // name: NPC/UI/world prefabs can rename one Transform while retaining the
  // same slot and bind-pose semantics.
  const size_t pathSlots = identity.paths.size();
  const size_t hashSlots = identity.hashes.size();
  const size_t indexSlots = identity.indexPaths.size();
  if (pathSlots && hashSlots && pathSlots != hashSlots)
    return reject("Replacement bone path/hash counts differ");
  const size_t payloadSlots = pathSlots ? pathSlots : hashSlots;
  if (!payloadSlots) return reject("Replacement mesh has no bone palette identity");
  // v5 records carry the source renderer/slot identity. They are authoritative
  // even when the exporter also left the legacy hierarchy-index array behind;
  // that array can be shorter or meaningless for another LOD/Prefab instance.
  if (!identity.sources.empty() && identity.sources.size() == payloadSlots) {
    if (EiemResolveMeshBonesFromAssembly(identity, renderer, out, error, errorSize))
      return true;
    Log("[MOD-SKIN-V5] binding=assembly-source failed renderer=%p error=%s",
        renderer, error ? error : "unknown");
  }
  if (indexSlots && indexSlots != payloadSlots)
    return reject("Replacement bone hierarchy-index count differs");
  if (!indexSlots && payloadSlots < count)
    return reject("Replacement bone palette removes source slots");
  if (!indexSlots && payloadSlots == count) {
    Log("[MOD-SKIN] renderer=%p binding=source-index slots=%zu reason=source-prefix", renderer, count);
    if (out) *out = current;
    return true;
  }
  if (identity.paths.empty())
    return reject("Added bone slots require authored bone paths");
  // v5 source records are authoritative only when a complete resolver is
  // available. Until the instance registry is connected, keep the legacy
  // path-based resolver active for diagnostic compatibility.
  if (!g_transform_get_parent || !g_object_get_name || !il2cpp_array_new ||
      !g_transformClass)
    return reject("Shared skeleton traversal APIs are unavailable");

  std::vector<void *> resolved;
  if (!indexSlots) resolved.assign(items, items + count);
  {
    if (!g_transform_get_childCount || !g_transform_GetChild)
      return reject("Shared skeleton traversal APIs are unavailable");
    std::vector<std::string> sourcePaths;
    std::unordered_map<std::string, void *> ancestors;
    for (size_t i=0; i<count; ++i) {
      std::vector<std::pair<std::string,void *>> chain;
      for (void *bone=items[i]; bone; bone=EiemBackendInvokeNoThrow(g_transform_get_parent,bone)) {
        if (chain.size() >= 128 || EiemNativeObjectStatus(bone) != 1)
          return reject("Source skeleton ancestor is invalid");
        char name[256]={};
        ReadStrUtf8(EiemBackendInvokeNoThrow(g_object_get_name,bone),name,sizeof(name));
        if (!name[0]) return reject("Source skeleton node has no name");
        chain.emplace_back(name,bone);
      }
      std::string full;
      for (auto it=chain.rbegin(); it!=chain.rend(); ++it) {
        if (!full.empty()) full+='/';
        full+=it->first;
        auto inserted=ancestors.emplace(full,it->second);
        if (!inserted.second && inserted.first->second!=it->second)
          return reject("Ambiguous skeleton ancestor path");
      }
      sourcePaths.push_back(full);
    }
    std::string root, why;
    // Legacy v3 payloads preserve the source palette as an ordered prefix.
    // V4 hierarchy-index payloads can be authored from another LOD, so any
    // common source slot may anchor the same live skeleton instance.
    std::vector<std::string> sourcePrefixPaths;
    if (indexSlots) {
      sourcePrefixPaths = identity.paths;
    } else {
      sourcePrefixPaths.reserve((std::min)(count, identity.paths.size()));
      for (size_t index = 0;
           index < count && index < identity.paths.size(); ++index)
        sourcePrefixPaths.push_back(identity.paths[index]);
    }
    if (!EiemSkinRootPath(sourcePaths,sourcePrefixPaths,root,why))
      return reject(why.c_str());
    auto ancestor=ancestors.find(root);
    if (ancestor==ancestors.end())
      return reject("Shared skeleton root is absent");
    if (indexSlots) {
      for (size_t slot = 0; slot < identity.indexPaths.size(); ++slot) {
        const std::string &indexPath = identity.indexPaths[slot];
        void *node = ancestor->second;
        size_t offset = 0;
        size_t depth = 0;
        while (offset < indexPath.size()) {
          const size_t slash = indexPath.find('/', offset);
          const size_t end = slash == std::string::npos
                                 ? indexPath.size() : slash;
          if (end == offset)
            return reject("Invalid empty bone hierarchy-index component");
          uint64_t childIndex = 0;
          for (size_t cursor = offset; cursor < end; ++cursor) {
            const char digit = indexPath[cursor];
            if (digit < '0' || digit > '9')
              return reject("Invalid bone hierarchy-index component");
            childIndex = childIndex * 10 + (uint64_t)(digit - '0');
            if (childIndex > 16384)
              return reject("Bone hierarchy-index exceeds traversal limit");
          }
          void *boxed = EiemBackendInvokeNoThrow(g_transform_get_childCount,
                                                  node);
          if (!boxed)
            return reject("Cannot read shared skeleton children");
          const int children = *(int *)((char *)boxed + 16);
          if (children < 0 || childIndex >= (uint64_t)children) {
            const LONG sample = InterlockedIncrement(
                &s_traceHierarchyIndexFailureCount);
            if (sample <= 24) {
              char nodeName[256] = {};
              ReadStrUtf8(EiemBackendInvokeNoThrow(g_object_get_name, node),
                          nodeName, sizeof(nodeName));
              Log("[DEBUG-v4-index] renderer=%p root=%s slot=%zu bonePath=%s "
                  "indexPath=%s depth=%zu node=%s requested=%llu children=%d "
                  "sourceSlots=%zu payloadSlots=%zu",
                  renderer, root.c_str(), slot, identity.paths[slot].c_str(),
                  indexPath.c_str(), depth,
                  nodeName[0] ? nodeName : "<unnamed>",
                  (unsigned long long)childIndex, children, count,
                  payloadSlots);
            }
            return reject("Bone hierarchy-index is absent from live skeleton");
          }
          int child = (int)childIndex;
          void *params[] = {&child};
          node = Invoke(g_transform_GetChild, node, params);
          if (!node || EiemNativeObjectStatus(node) != 1)
            return reject("Shared skeleton child is unavailable");
          offset = slash == std::string::npos ? indexPath.size() : slash + 1;
          ++depth;
        }
        resolved.push_back(node);
      }
      Log("[MOD-SKIN] renderer=%p binding=hierarchy-index slots=%zu "
          "sourceSlots=%zu", renderer, resolved.size(), count);
    } else {
    std::vector<void *> nodes{ancestor->second};
    std::vector<std::string> paths{root.substr(root.find_last_of('/')+1)};
    for (size_t index=0; index<nodes.size(); ++index) {
      if (nodes.size()>16384) return reject("Skeleton hierarchy exceeds traversal limit");
      void *boxed=EiemBackendInvokeNoThrow(g_transform_get_childCount,nodes[index]);
      if (!boxed) return reject("Cannot read shared skeleton children");
      const int children=*(int *)((char *)boxed+16);
      if (children<0 || children>16384) return reject("Invalid skeleton child count");
      for (int child=0; child<children; ++child) {
        void *params[]={&child};
        void *node=Invoke(g_transform_GetChild,nodes[index],params);
        if (!node || EiemNativeObjectStatus(node)!=1) return reject("Shared skeleton child is unavailable");
        char name[256]={};
        ReadStrUtf8(EiemBackendInvokeNoThrow(g_object_get_name,node),name,sizeof(name));
        if (!name[0]) return reject("Shared skeleton child has no name");
        paths.push_back(paths[index]+"/"+name); nodes.push_back(node);
      }
    }
    std::vector<std::string> addedPaths;
    addedPaths.reserve(identity.paths.size() - count);
    for (size_t index = count; index < identity.paths.size(); ++index)
      addedPaths.push_back(identity.paths[index]);
    std::vector<size_t> slots;
    if (!EiemResolveSkinPathIndices(addedPaths,paths,slots,why))
      return reject(why.c_str());
    for (size_t index:slots) resolved.push_back(nodes[index]);
    }
  }
  if (resolved.size() != payloadSlots)
    return reject("Replacement bone palette size is inconsistent");
  void *array=il2cpp_array_new(g_transformClass,resolved.size());
  if (!array) return reject("Cannot allocate expanded bone palette");
  memcpy((char *)array+IL2CPP_ARRAY_DATA,resolved.data(),resolved.size()*sizeof(void *));
  if (out) *out=array;
  return true;
}

// Resolve a replacement Mesh against the concrete native skeleton instance.
// EIEMESH v5/v6 source records preserve the original Mesh-local slot selected
// by the game.  The resolver runs only after native assembly has populated the
// current model instance and maps every donor slot into that instance's one
// skinningRoot.  Names, authored hierarchy paths and LOD-local slot order are
// never used as a fallback.
static bool EiemResolveMeshBonesFromNativeInstance(
    const EiemSkinIdentity &identity, void *renderer, void **out,
    char *error, size_t errorSize) {
  if (out) *out = nullptr;
  auto reject = [&](const std::string &message) {
    if (error) strncpy_s(error, errorSize, message.c_str(), _TRUNCATE);
    return false;
  };
  if (!renderer || identity.paths.empty() || !g_smr_get_bones ||
      !il2cpp_array_new || !g_transformClass)
    return reject("Native instance skeleton APIs are unavailable");

  // A slot number is local to one Mesh sub-asset's bones[] palette.  Several
  // sub-assets may live in the same FBX container, so an equal container path
  // cannot override a differing sub-asset identity.  Path matching is only a
  // compatibility fallback for payloads that did not record a sub-asset.
  auto sourceMatchesMeshIdentity =
      [](const EiemSkinIdentity::Source &source, const char *meshPath,
         const char *meshAsset) {
        if (!source.meshAsset.empty())
          return meshAsset && meshAsset[0] &&
                 EiemModEquals(source.meshAsset.c_str(), meshAsset);
        return !source.meshPath.empty() && meshPath && meshPath[0] &&
               EiemModSameLogicalPath(source.meshPath.c_str(), meshPath);
      };

  // All LOD Renderers in one PFB instance consume one game skeleton, while
  // each Renderer keeps only a local bones[] palette.  Do not compare local
  // slot numbers or rootBone pointers across LODs: rootBone is a Renderer
  // local anchor and may differ for body, cloth, shadow, and physical parts.
  // The instance boundary is skinningRoot.  A donor slot is therefore allowed
  // to come from any Renderer under the same skinningRoot, with same-root
  // donors preferred when both branches expose the slot.  This uses the
  // completed native hierarchy and never reads a Transform name or falls back
  // to an authored path.
  void *targetRootBone = g_smr_get_rootBone
                             ? EiemBackendInvokeNoThrow(g_smr_get_rootBone,
                                                        renderer)
                             : nullptr;
  void *targetSkinningRoot = g_smr_get_skinningRoot
                                 ? EiemBackendInvokeNoThrow(
                                       g_smr_get_skinningRoot, renderer)
                                 : nullptr;
  if (!targetSkinningRoot || !g_transform_get_parent ||
      !g_transform_get_childCount || !g_transform_GetChild)
    return reject("Native instance has no completed unified skeleton root");

  // The authoring Armature describes one logical skeleton, but a model can
  // expose several native Transform branches for its main, LOD and shadow
  // renderers.  Bone names cannot identify those branches because different
  // PFBs may rename the same logical bone.  Anchor the target branch with the
  // exact original bones[] captured before this model transaction mutates any
  // Renderer, then grow the branch through shared Transform references.
  void *targetNativeBones = EiemBackendInvokeNoThrow(g_smr_get_bones, renderer);
  if (s_eiemLiveSkinSources) {
    for (const auto &source : *s_eiemLiveSkinSources) {
      if (source.renderer == renderer && source.bones) {
        targetNativeBones = source.bones;
        break;
      }
    }
  }
  auto paletteOverlap = [](void *left, void *right) -> size_t {
    const size_t leftCount = EiemManagedArrayLength(left);
    const size_t rightCount = EiemManagedArrayLength(right);
    if (!left || !right || !leftCount || !rightCount) return 0;
    void **leftItems = (void **)((char *)left + IL2CPP_ARRAY_DATA);
    void **rightItems = (void **)((char *)right + IL2CPP_ARRAY_DATA);
    size_t overlap = 0;
    for (size_t leftIndex = 0; leftIndex < leftCount; ++leftIndex) {
      void *bone = leftItems[leftIndex];
      if (!bone) continue;
      for (size_t rightIndex = 0; rightIndex < rightCount; ++rightIndex) {
        if (rightItems[rightIndex] != bone) continue;
        ++overlap;
        break;
      }
    }
    return overlap;
  };
  auto sameSkeletonContext = [&](void *candidateRenderer) -> bool {
    if (!candidateRenderer) return false;
    // rootBone is deliberately not an instance boundary.  The game assigns
    // different local rootBone values to body/cloth/shadow Renderers while
    // their Transform objects still belong to the same skinningRoot.
    if (targetSkinningRoot && g_smr_get_skinningRoot) {
      void *candidateRoot = EiemBackendInvokeNoThrow(
          g_smr_get_skinningRoot, candidateRenderer);
      if (!candidateRoot || candidateRoot != targetSkinningRoot) return false;
    }
    return true;
  };
  std::vector<const EiemLiveSkinSource *> targetBranchSources;
  if (s_eiemLiveSkinSources && targetNativeBones) {
    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &candidate : *s_eiemLiveSkinSources) {
        if (!candidate.bones) continue;
        if (std::find(targetBranchSources.begin(), targetBranchSources.end(),
                      &candidate) != targetBranchSources.end())
          continue;
        bool connected = candidate.renderer == renderer ||
                         (sameSkeletonContext(candidate.renderer) &&
                          paletteOverlap(targetNativeBones, candidate.bones) != 0);
        if (!connected) {
          for (const auto *member : targetBranchSources) {
            if (member && sameSkeletonContext(candidate.renderer) &&
                paletteOverlap(member->bones, candidate.bones) != 0) {
              connected = true;
              break;
            }
          }
        }
        // A lower LOD may omit every bone owned by a donor Renderer.  It is
        // still a valid donor when the game's completed root context proves
        // that both palettes belong to this same model instance.
        if (!connected && sameSkeletonContext(candidate.renderer))
          connected = true;
        if (!connected) continue;
        targetBranchSources.push_back(&candidate);
        changed = true;
      }
    }
  }
  auto sourceBranchScore = [&](const EiemLiveSkinSource &candidate) -> size_t {
    if (!candidate.bones || !targetNativeBones) return 0;
    if (candidate.renderer == renderer)
      return (size_t)1 << (sizeof(size_t) * 8 - 2);
    if (std::find(targetBranchSources.begin(), targetBranchSources.end(),
                  &candidate) == targetBranchSources.end())
      return 0;
    // Direct overlap selects the closest native palette inside the connected
    // branch.  The +1 keeps a transitively connected donor usable when the
    // target LOD omits every bone owned by that specialised source Mesh.
    size_t score = paletteOverlap(targetNativeBones, candidate.bones) + 1;
    if (targetRootBone && g_smr_get_rootBone) {
      void *candidateRoot = EiemBackendInvokeNoThrow(
          g_smr_get_rootBone, candidate.renderer);
      // Prefer a donor from the target Renderer branch, but keep a lower
      // scoring cross-branch donor available for bones only exposed by cloth,
      // physics, or another specialised Mesh.
      if (candidateRoot == targetRootBone)
        score += (size_t)1 << (sizeof(size_t) * 8 - 3);
    }
    return score;
  };

  auto childIndexPath = [&](void *root, void *node,
                            std::vector<uint32_t> *path) -> bool {
    if (!root || !node || !path) return false;
    path->clear();
    void *current = node;
    for (size_t depth = 0; current && current != root && depth < 256;
         ++depth) {
      void *parent = EiemBackendInvokeNoThrow(g_transform_get_parent, current);
      if (!parent || parent == current) return false;
      void *boxed = EiemBackendInvokeNoThrow(g_transform_get_childCount, parent);
      if (!boxed) return false;
      const int childCount = *(int *)((char *)boxed + 16);
      if (childCount < 0 || childCount > 16384) return false;
      size_t found = SIZE_MAX;
      for (int index = 0; index < childCount; ++index) {
        int childIndex = index;
        void *params[] = {&childIndex};
        void *child = Invoke(g_transform_GetChild, parent, params);
        if (child == current) {
          found = (size_t)index;
          break;
        }
      }
      if (found == SIZE_MAX || found > UINT32_MAX) return false;
      path->push_back((uint32_t)found);
      current = parent;
    }
    if (current != root) return false;
    std::reverse(path->begin(), path->end());
    return true;
  };
  auto resolveChildIndexPath = [&](void *root,
                                   const std::vector<uint32_t> &path) -> void * {
    if (!root) return nullptr;
    void *current = root;
    for (uint32_t index : path) {
      void *boxed = EiemBackendInvokeNoThrow(g_transform_get_childCount,
                                             current);
      if (!boxed) return nullptr;
      const int childCount = *(int *)((char *)boxed + 16);
      if (childCount < 0 || index >= (uint32_t)childCount) return nullptr;
      int childIndex = (int)index;
      void *params[] = {&childIndex};
      current = Invoke(g_transform_GetChild, current, params);
      if (!current || EiemNativeObjectStatus(current) != 1) return nullptr;
    }
    return current;
  };
  auto mapDonorToTargetSkeleton = [&](void *candidateRenderer,
                                      void *candidateBone) -> void * {
    if (!candidateRenderer || !candidateBone) return nullptr;
    // The target Renderer is already part of the completed native table; its
    // own slot is authoritative and needs no cross-palette conversion.
    if (candidateRenderer == renderer) return candidateBone;
    void *candidateRoot = g_smr_get_skinningRoot
                              ? EiemBackendInvokeNoThrow(
                                    g_smr_get_skinningRoot,
                                    candidateRenderer)
                              : nullptr;
    if (!candidateRoot) return nullptr;
    if (candidateRoot == targetSkinningRoot) return candidateBone;
    std::vector<uint32_t> path;
    if (!childIndexPath(candidateRoot, candidateBone, &path)) return nullptr;
    return resolveChildIndexPath(targetSkinningRoot, path);
  };

  auto resolveSourceSlot = [&](const EiemSkinIdentity::Source &source,
                               bool *ambiguous,
                               const EiemLiveSkinSource **donorOut,
                               size_t *scoreOut) -> void * {
    if (ambiguous) *ambiguous = false;
    if (donorOut) *donorOut = nullptr;
    if (scoreOut) *scoreOut = 0;
    if (!s_eiemLiveSkinSources ||
        (source.meshAsset.empty() && source.meshPath.empty()))
      return nullptr;
    static volatile LONG s_candidateConflictLogCount = 0;
    void *firstRenderer = nullptr;
    void *firstBone = nullptr;
    void *firstRootBone = nullptr;
    void *firstSkinningRoot = nullptr;
    void *resolved = nullptr;
    size_t resolvedScore = 0;
    for (const auto &candidate : *s_eiemLiveSkinSources) {
      if (!sourceMatchesMeshIdentity(source, candidate.source.c_str(),
                                     candidate.asset.c_str()))
        continue;
      const size_t branchScore = sourceBranchScore(candidate);
      if (!branchScore) continue;
      const size_t boneCount = EiemManagedArrayLength(candidate.bones);
      if (!candidate.bones || source.slot >= boneCount) continue;
      void **sourceBones =
          (void **)((char *)candidate.bones + IL2CPP_ARRAY_DATA);
      void *bone = sourceBones[source.slot];
      if (!bone || EiemNativeObjectStatus(bone) != 1) continue;
      void *candidateSkinningRoot =
          g_smr_get_skinningRoot
              ? EiemBackendInvokeNoThrow(g_smr_get_skinningRoot,
                                         candidate.renderer)
              : nullptr;
      void *candidateRootBone =
          g_smr_get_rootBone
              ? EiemBackendInvokeNoThrow(g_smr_get_rootBone,
                                         candidate.renderer)
              : nullptr;
      void *mappedBone = mapDonorToTargetSkeleton(candidate.renderer, bone);
      if (!mappedBone) continue;
      if (!resolved || branchScore > resolvedScore) {
        firstRenderer = candidate.renderer;
        firstBone = bone;
        firstRootBone = candidateRootBone;
        firstSkinningRoot = candidateSkinningRoot;
        resolved = mappedBone;
        resolvedScore = branchScore;
        if (donorOut) *donorOut = &candidate;
        continue;
      }
      if (branchScore == resolvedScore && resolved != mappedBone) {
        const LONG sample = InterlockedIncrement(&s_candidateConflictLogCount);
        if (sample <= 48) {
          Log("[MOD-SKIN-CANDIDATE-v1] model=%p target=%p targetRoot=%p "
              "targetSkinningRoot=%p targetBranch=%p targetBranchKey=%s "
              "sourceAsset=%s "
              "sourcePath=%s slot=%u "
              "firstRenderer=%p firstBone=%p firstRoot=%p firstSkinningRoot=%p "
              "conflictRenderer=%p conflictBone=%p conflictRoot=%p "
              "conflictSkinningRoot=%p conflictBranch=%p "
              "conflictBranchKey=%s",
              (void *)s_eiemActivePrefabInstance, renderer, targetRootBone,
              targetSkinningRoot, nullptr, "native-root-context",
              source.meshAsset.c_str(), source.meshPath.c_str(), source.slot,
              firstRenderer, firstBone, firstRootBone, firstSkinningRoot,
              candidate.renderer, bone, candidateRootBone, candidateSkinningRoot,
              nullptr, "unified-skeleton");
        }
        if (ambiguous) *ambiguous = true;
        return nullptr;
      }
    }
    if (scoreOut) *scoreOut = resolvedScore;
    return resolved;
  };

  auto resolveSourceCandidates =
      [&](const std::vector<EiemSkinIdentity::Source> &candidates,
          bool *ambiguous) -> void * {
    if (ambiguous) *ambiguous = false;
    static volatile LONG s_candidateDonorMergeLogCount = 0;
    void *resolved = nullptr;
    const EiemLiveSkinSource *resolvedDonor = nullptr;
    size_t resolvedScore = 0;
    for (const auto &candidate : candidates) {
      bool candidateAmbiguous = false;
      const EiemLiveSkinSource *donor = nullptr;
      size_t candidateScore = 0;
      void *bone = resolveSourceSlot(candidate, &candidateAmbiguous, &donor,
                                     &candidateScore);
      if (candidateAmbiguous) {
        if (ambiguous) *ambiguous = true;
        return nullptr;
      }
      if (!bone) continue;
      if (!resolved || candidateScore > resolvedScore) {
        resolved = bone;
        resolvedDonor = donor;
        resolvedScore = candidateScore;
        continue;
      }
      if (candidateScore == resolvedScore && resolved != bone) {
        const LONG sample =
            InterlockedIncrement(&s_candidateDonorMergeLogCount);
        if (sample <= 48) {
          void *conflictRootBone =
              donor && g_smr_get_rootBone
                  ? EiemBackendInvokeNoThrow(g_smr_get_rootBone,
                                             donor->renderer)
                  : nullptr;
          void *conflictSkinningRoot =
              donor && g_smr_get_skinningRoot
                  ? EiemBackendInvokeNoThrow(g_smr_get_skinningRoot,
                                             donor->renderer)
                  : nullptr;
          Log("[MOD-SKIN-CANDIDATE-v1] model=%p target=%p "
              "targetRoot=%p targetSkinningRoot=%p targetBranch=%p "
              "targetBranchKey=%s slotPath=%s "
              "resolvedRenderer=%p resolvedBone=%p resolvedRoot=%p "
              "resolvedSkinningRoot=%p conflictRenderer=%p conflictBone=%p "
              "conflictRoot=%p conflictSkinningRoot=%p conflictBranch=%p "
              "conflictBranchKey=%s",
              (void *)s_eiemActivePrefabInstance, renderer, targetRootBone,
              targetSkinningRoot, nullptr, "native-root-context",
              candidate.meshPath.c_str(),
              resolvedDonor ? resolvedDonor->renderer : nullptr, resolved,
              targetRootBone, targetSkinningRoot,
              donor ? donor->renderer : nullptr, bone, conflictRootBone,
              conflictSkinningRoot, nullptr, "native-root-context");
        }
        if (ambiguous) *ambiguous = true;
        return nullptr;
      }
    }
    return resolved;
  };

  // Prefer the palette that belongs to this exact Renderer.  A model can
  // contain several LOD and shadow renderers for the same logical Mesh, and
  // their local palettes may be different subsets of the same native
  // skeleton.  Comparing all of those donors before the renderer has a
  // completed assembly snapshot can reject a valid replacement.  The source
  // Mesh/slot record is authoritative for the current renderer, so use its
  // own bones[] whenever it contains every replacement slot we need.
  void *currentBones = targetNativeBones;
  const size_t currentBoneCount = EiemManagedArrayLength(currentBones);
  void **currentBoneItems =
      currentBones && currentBoneCount
          ? (void **)((char *)currentBones + IL2CPP_ARRAY_DATA)
          : nullptr;
  char currentSource[768] = {}, currentAsset[192] = {};
  void *currentMesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
  void *currentIdentityMesh = currentMesh;
  if (currentMesh)
    EiemPrepareRenderInput(renderer, currentMesh, "SkinnedMeshRenderer",
                           &currentIdentityMesh);
  const bool currentIdentityKnown =
      currentIdentityMesh &&
      EiemReadLiveMeshIdentity(currentIdentityMesh, currentSource,
                               sizeof(currentSource), currentAsset,
                               sizeof(currentAsset));
  auto sourceMatchesCurrentRenderer =
      [&](const EiemSkinIdentity::Source &source) {
        return currentIdentityKnown && sourceMatchesMeshIdentity(
                                           source, currentSource, currentAsset);
      };
  auto allocateCurrentPalette = [&](const std::vector<void *> &resolved,
                                    const char *binding) -> bool {
    if (resolved.empty() || !il2cpp_array_new || !g_transformClass)
      return false;
    void *array = il2cpp_array_new(g_transformClass, resolved.size());
    if (!array) return false;
    memcpy((char *)array + IL2CPP_ARRAY_DATA, resolved.data(),
           resolved.size() * sizeof(void *));
    if (out) *out = array;
    Log("[MOD-SKIN-NATIVE] renderer=%p binding=%s slots=%zu", renderer,
        binding, resolved.size());
    return true;
  };

  if (currentBoneItems && currentIdentityKnown &&
      identity.sourceCandidates.size() == identity.paths.size() &&
      !identity.sourceCandidates.empty()) {
    std::vector<void *> resolved;
    resolved.reserve(identity.sourceCandidates.size());
    bool complete = true;
    for (const auto &candidates : identity.sourceCandidates) {
      void *selected = nullptr;
      for (const auto &source : candidates) {
        if (!sourceMatchesCurrentRenderer(source) ||
            source.slot >= currentBoneCount)
          continue;
        void *bone = currentBoneItems[source.slot];
        if (bone && EiemNativeObjectStatus(bone) == 1)
          selected = bone;
      }
      if (!selected) {
        complete = false;
        break;
      }
      resolved.push_back(selected);
    }
    if (complete && resolved.size() == identity.paths.size() &&
        allocateCurrentPalette(resolved, "renderer-source-slots"))
      return true;
  }

  if (currentBoneItems && currentIdentityKnown &&
      identity.sources.size() == identity.paths.size() &&
      !identity.sources.empty()) {
    std::vector<void *> resolved;
    resolved.reserve(identity.sources.size());
    bool complete = true;
    for (const auto &source : identity.sources) {
      if (!sourceMatchesCurrentRenderer(source) ||
          source.slot >= currentBoneCount) {
        complete = false;
        break;
      }
      void *bone = currentBoneItems[source.slot];
      if (!bone || EiemNativeObjectStatus(bone) != 1) {
        complete = false;
        break;
      }
      resolved.push_back(bone);
    }
    if (complete && resolved.size() == identity.paths.size() &&
        allocateCurrentPalette(resolved, "renderer-source-slots"))
      return true;
  }

  // EIEMESH v6 is strict by design.  Every replacement slot is backed by one
  // or more original Mesh/slot donors.  Resolve only donors present in this
  // model instance; if none exists, or existing donors disagree, fail instead
  // of guessing with a renamed hierarchy or a local LOD array index.
  if (identity.sourceCandidates.size() == identity.paths.size() &&
      !identity.sourceCandidates.empty()) {
    std::vector<void *> sourceResolved;
    sourceResolved.reserve(identity.sourceCandidates.size());
    for (size_t slot = 0; slot < identity.sourceCandidates.size(); ++slot) {
      bool ambiguous = false;
      void *bone = resolveSourceCandidates(identity.sourceCandidates[slot],
                                            &ambiguous);
      if (ambiguous)
        return reject("Replacement bone source candidates disagree in model instance: " +
                      identity.paths[slot]);
      if (!bone)
        return reject("Replacement bone has no native Mesh donor in model instance: " +
                      identity.paths[slot]);
      sourceResolved.push_back(bone);
    }
    void *array = il2cpp_array_new(g_transformClass, sourceResolved.size());
    if (!array) return reject("Unable to allocate donor-slot bone palette");
    memcpy((char *)array + IL2CPP_ARRAY_DATA, sourceResolved.data(),
           sourceResolved.size() * sizeof(void *));
    if (out) *out = array;
    Log("[MOD-SKIN-NATIVE] renderer=%p binding=instance-donor-candidates "
        "slots=%zu sourceResolved=%zu",
        renderer, sourceResolved.size(), sourceResolved.size());
    return true;
  }

  // A complete v5 compatibility palette does not need a name anchor at all. Resolve every
  // slot before touching the target Renderer's local palette so a different
  // prefab may rename any number of bones without changing the result.
  if (identity.sources.size() == identity.paths.size()) {
    std::vector<void *> sourceResolved;
    sourceResolved.reserve(identity.sources.size());
    bool complete = true;
    for (size_t slot = 0; slot < identity.sources.size(); ++slot) {
      bool ambiguous = false;
      void *bone = resolveSourceSlot(identity.sources[slot], &ambiguous,
                                     nullptr, nullptr);
      if (ambiguous)
        return reject("Replacement bone source is ambiguous in model instance: " +
                      identity.paths[slot]);
      if (!bone) {
        complete = false;
        break;
      }
      sourceResolved.push_back(bone);
    }
    if (complete) {
      void *array = il2cpp_array_new(g_transformClass, sourceResolved.size());
      if (!array)
        return reject("Unable to allocate source-slot bone palette");
      memcpy((char *)array + IL2CPP_ARRAY_DATA, sourceResolved.data(),
             sourceResolved.size() * sizeof(void *));
      if (out) *out = array;
      Log("[MOD-SKIN-NATIVE] renderer=%p binding=instance-source slots=%zu "
          "sourceResolved=%zu",
          renderer, sourceResolved.size(), sourceResolved.size());
      return true;
    }
  }

  return reject("EIEMESH has no native source-Mesh slot records");
}

// Mesh bindings are instance state. Store the exact replacement array for
// later game setter refreshes; the original array remains separate for F10.
static bool EiemPreserveSourceSkinning(void *renderer, void *bones,
                                        char *error, size_t errorSize) {
  if (!renderer || !bones || !g_smr_get_bones || !g_smr_set_bones ||
      !il2cpp_gchandle_new || !il2cpp_gchandle_free) return false;
  const size_t boneCount=EiemManagedArrayLength(bones);
  void **boneItems=(void **)((char *)bones+IL2CPP_ARRAY_DATA);
  for (size_t i=0; i<boneCount; ++i) if (EiemNativeObjectStatus(boneItems[i])!=1) {
    if (error) strncpy_s(error,errorSize,"Replacement skeleton instance is no longer alive",_TRUNCATE);
    return false;
  }
  const uint32_t handle=il2cpp_gchandle_new(bones,false);
  if (!handle) return false;
  bool same=EiemManagedObjectArraySame(bones,Invoke(g_smr_get_bones,renderer));
  bool ok=true;
  if (!same) {
    void *params[]={bones};
    const bool previous=s_eiemApplyingModMeshAssignment;
    s_eiemApplyingModMeshAssignment=true;
    void *result=nullptr;
    ok=InvokeChecked(g_smr_set_bones,renderer,params,&result);
    s_eiemApplyingModMeshAssignment=previous;
    ok=ok && EiemManagedObjectArraySame(bones,Invoke(g_smr_get_bones,renderer));
  }
  uint32_t old=0;
  if (ok) {
    AcquireSRWLockExclusive(&s_eiemOverrideLock);
    const size_t index=EiemFindOverrideLocked(renderer);
    if (index!=SIZE_MAX) {
      old=s_eiemOverrides[index].replacementBonesHandle;
      s_eiemOverrides[index].replacementBonesHandle=handle;
    } else ok=false;
    ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  }
  if (!ok) {
    il2cpp_gchandle_free(handle);
    if (error) strncpy_s(error,errorSize,"Replacement bone palette assignment failed",_TRUNCATE);
  }
  if (old) il2cpp_gchandle_free(old);
  Log("[MOD-SKIN] renderer=%p slots=%zu changed=%d applied=%d",renderer,EiemManagedArrayLength(bones),!same,ok);
  return ok;
}

// The game can finish or rebuild a Renderer skin after EIEM first replaced
// its Mesh. The incoming game-owned palette/root are the baseline that F10
// must restore. Keep that baseline separate from EIEM's replacement palette;
// otherwise a reload restores the early construction snapshot and the affected
// Renderer can fall back to an unanimated/T-pose binding.
static void EiemRememberGameSourceBones(void *renderer, void *bones,
                                        const char *stage) {
  if (!renderer || !bones || !il2cpp_gchandle_new ||
      !il2cpp_gchandle_get_target || !il2cpp_gchandle_free)
    return;

  bool tracked = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t observed = EiemFindOverrideLocked(renderer);
  if (observed != SIZE_MAX) {
    const auto &state = s_eiemOverrides[observed];
    void *replacement = state.replacementBonesHandle
                            ? il2cpp_gchandle_get_target(
                                  state.replacementBonesHandle)
                            : nullptr;
    void *baseline = state.originalBonesHandle
                         ? il2cpp_gchandle_get_target(
                               state.originalBonesHandle)
                         : nullptr;
    const bool replacementBinding =
        replacement && EiemManagedObjectArraySame(bones, replacement);
    const bool currentBaseline =
        baseline && EiemManagedObjectArraySame(bones, baseline);
    tracked = !state.restorePending && state.ownsMesh &&
              state.replacementMesh && !replacementBinding &&
              !currentBaseline;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (!tracked) return;

  uint32_t handle = il2cpp_gchandle_new(bones, false);
  if (!handle) return;
  uint32_t previous = 0;
  bool committed = false;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    auto &state = s_eiemOverrides[index];
    void *replacement = state.replacementBonesHandle
                            ? il2cpp_gchandle_get_target(
                                  state.replacementBonesHandle)
                            : nullptr;
    const bool replacementBinding =
        replacement && EiemManagedObjectArraySame(bones, replacement);
    if (!state.restorePending && state.ownsMesh && state.replacementMesh &&
        !replacementBinding) {
      previous = state.originalBonesHandle;
      state.originalBonesHandle = handle;
      state.hasSkinning = true;
      committed = true;
    }
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  if (!committed) il2cpp_gchandle_free(handle);
  if (previous) il2cpp_gchandle_free(previous);
  if (committed)
    Log("[MOD-SKIN-BASELINE] bones renderer=%p array=%p count=%zu stage=%s",
        renderer, bones, EiemManagedArrayLength(bones),
        stage ? stage : "unknown");
}

static void EiemRememberGameSourceRootBone(void *renderer,
                                           const char *stage) {
  if (!renderer || !g_smr_get_rootBone || !il2cpp_gchandle_new ||
      !il2cpp_gchandle_get_target || !il2cpp_gchandle_free)
    return;
  void *rootBone = Invoke(g_smr_get_rootBone, renderer);
  if (!rootBone) return;

  bool tracked = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t observed = EiemFindOverrideLocked(renderer);
  if (observed != SIZE_MAX) {
    const auto &state = s_eiemOverrides[observed];
    void *baseline = state.originalRootBoneHandle
                         ? il2cpp_gchandle_get_target(
                               state.originalRootBoneHandle)
                         : nullptr;
    tracked = !state.restorePending && state.ownsMesh &&
              state.replacementMesh && rootBone != baseline;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (!tracked) return;

  uint32_t handle = il2cpp_gchandle_new(rootBone, false);
  if (!handle) return;
  uint32_t previous = 0;
  bool committed = false;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    auto &state = s_eiemOverrides[index];
    if (!state.restorePending && state.ownsMesh && state.replacementMesh) {
      previous = state.originalRootBoneHandle;
      state.originalRootBoneHandle = handle;
      state.hasSkinning = true;
      committed = true;
    }
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  if (!committed) il2cpp_gchandle_free(handle);
  if (previous) il2cpp_gchandle_free(previous);
  if (committed)
    Log("[MOD-SKIN-BASELINE] root renderer=%p rootBone=%p stage=%s",
        renderer, rootBone, stage ? stage : "unknown");
}

static void EiemRememberGameSourceSkinningFromArray(void *renderers,
                                                     bool rememberBones,
                                                     bool rememberRoot,
                                                     const char *stage) {
  const size_t count = EiemManagedArrayLength(renderers);
  if (!renderers || count > 8192) return;
  void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  for (size_t index = 0; index < count; ++index) {
    void *renderer = items[index];
    if (!renderer) continue;
    if (rememberBones && g_smr_get_bones) {
      void *bones = Invoke(g_smr_get_bones, renderer);
      if (bones) EiemRememberGameSourceBones(renderer, bones, stage);
    }
    if (rememberRoot)
      EiemRememberGameSourceRootBone(renderer, stage);
  }
}

// A later game skin refresh must not shrink an extended palette back to the
// source slots. Reassert only this instance's owned replacement binding.
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo) {
  EiemRegistrationTraceNativeStackContext(
      "SkinnedMeshRenderer.set_bones.entry", self, bones, nullptr,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  auto original = (TraceSetBonesFn)s_origSkinnedMeshSetBones;
  if (original) original(self, bones, methodInfo);
  if (!self || s_eiemApplyingModMeshAssignment) {
    return;
  }
  void *gameAfterBones = g_smr_get_bones ? Invoke(g_smr_get_bones, self) : nullptr;
  EiemLogSkinSetterTimeline("bones-game", self, nullptr, nullptr, bones,
                            gameAfterBones);

  bool tracked = false;
  uint32_t binding = 0;
  uintptr_t ownerModel = 0;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(self);
  if (index != SIZE_MAX && !s_eiemOverrides[index].restorePending && s_eiemOverrides[index].replacementMesh) {
    tracked = true;
    binding = s_eiemOverrides[index].replacementBonesHandle;
    ownerModel = s_eiemOverrides[index].ownerPrefabInstance;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (tracked && bones)
    EiemRememberGameSourceBones(self, bones, "SkinnedMeshRenderer.set_bones");
  if (tracked && binding && EiemOnUnityThread() && il2cpp_gchandle_get_target) {
    void *expected = il2cpp_gchandle_get_target(binding);
    if (expected && !EiemManagedObjectArraySame(bones, expected)) {
      // The native caller may still be constructing cloth/physics state.
      // Never recurse into set_bones here; rebind after this call stack exits.
      if (ownerModel)
        EiemQueueNativeSkinRefresh(ownerModel);
      else
        EiemQueueModReconcile("native skin refresh without model owner");
    }
  }
  void *finalBones = g_smr_get_bones ? Invoke(g_smr_get_bones, self) : nullptr;
  EiemLogSkinSetterTimeline("bones-final", self, nullptr, nullptr, bones,
                            finalBones);
  if (!tracked || !TraceTakeBudget(&s_traceBonesSetterCount, 80)) return;

  const size_t count = EiemManagedArrayLength(bones);
  Log("[DEBUG-SKIN-BONES] renderer=%p bones=%p count=%zu tid=%lu", self,
      bones, count, (unsigned long)GetCurrentThreadId());
  if (!bones || count > 128) return;
  void **items = (void **)((char *)bones + IL2CPP_ARRAY_DATA);
  for (size_t i = 0; i < count && i < 12; ++i) {
    char name[192] = {};
    if (items[i] && g_object_get_name)
      ReadStrUtf8(EiemBackendInvokeNoThrow(g_object_get_name, items[i]),
                  name, sizeof(name));
    Log("[DEBUG-SKIN-BONES] renderer=%p index=%zu bone=%p name=%s", self, i,
        items[i], name[0] ? name : "<unnamed>");
  }
}

static void *EiemFindSourceLodGroup(void *sourceRenderer) {
  if (!sourceRenderer || !g_lodGroupClass || !g_gameObject_GetComponent ||
      !g_component_get_transform || !g_component_get_gameObject ||
      !il2cpp_class_get_type || !il2cpp_type_get_object)
    return nullptr;
  void *type = il2cpp_class_get_type(g_lodGroupClass);
  void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
  if (!typeObject) return nullptr;
  void *transform = Invoke(g_component_get_transform, sourceRenderer);
  for (int depth = 0; transform && depth < 128; ++depth) {
    void *gameObject = Invoke(g_component_get_gameObject, transform);
    if (gameObject) {
      void *params[] = {typeObject};
      void *group = Invoke(g_gameObject_GetComponent, gameObject, params);
      if (group) return group;
    }
    transform = g_transform_get_parent
                    ? Invoke(g_transform_get_parent, transform)
                    : nullptr;
  }
  return nullptr;
}

static bool EiemSetPartnerLodMembershipInternal(void *sourceRenderer,
                                                void *partnerRenderer,
                                                bool add,
                                                size_t *lodCountOut = nullptr,
                                                void **groupOut = nullptr,
                                                uint64_t *sourceLevelsOut = nullptr,
                                                uint64_t *partnerLevelsOut = nullptr,
                                                void *groupOverride = nullptr) {
  if (groupOut) *groupOut = nullptr;
  if (sourceLevelsOut) *sourceLevelsOut = 0;
  if (partnerLevelsOut) *partnerLevelsOut = 0;
  if (!sourceRenderer || !partnerRenderer || !g_lodGroup_get_lods ||
      !g_lodGroup_set_lods || !il2cpp_array_new_specific ||
      !il2cpp_object_get_class)
    return false;
  __try {
    void *group = groupOverride ? groupOverride
                                : EiemFindSourceLodGroup(sourceRenderer);
    if (groupOut) *groupOut = group;
    if (!group) return false;
    bool getPlatformLODs = false;
    void *getParams[] = {&getPlatformLODs};
    void *lods = Invoke(g_lodGroup_get_lods, group, getParams);
    const size_t lodCount = EiemManagedArrayLength(lods);
    if (lodCountOut) *lodCountOut = lodCount;
    if (!lods || lodCount == 0 || lodCount > 64) return false;
    bool changed = false;
    uint64_t sourceLevels = 0;
    uint64_t partnerLevels = 0;
    char *lodData = (char *)lods + 32;
    for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex) {
      EiemNativeLod *lod = (EiemNativeLod *)(lodData + lodIndex * sizeof(EiemNativeLod));
      void *renderers = lod->renderers;
      const size_t rendererCount = EiemManagedArrayLength(renderers);
      if (!renderers || rendererCount > 4096) continue;
      void **items = (void **)((char *)renderers + 32);
      size_t sourceIndex = SIZE_MAX;
      size_t partnerIndex = SIZE_MAX;
      for (size_t index = 0; index < rendererCount; ++index) {
        if (items[index] == sourceRenderer) sourceIndex = index;
        if (items[index] == partnerRenderer) partnerIndex = index;
      }
      if (sourceIndex != SIZE_MAX) sourceLevels |= (uint64_t(1) << lodIndex);
      if (partnerIndex != SIZE_MAX) partnerLevels |= (uint64_t(1) << lodIndex);
      if (add) {
        if (sourceIndex != SIZE_MAX) {
          if (partnerIndex != SIZE_MAX) continue;
          void *arrayClass = il2cpp_object_get_class(renderers);
          if (!arrayClass) continue;
          void *replacement = il2cpp_array_new_specific(arrayClass,
                                                         rendererCount + 1);
          if (!replacement) continue;
          void **outItems = (void **)((char *)replacement + 32);
          memcpy(outItems, items, rendererCount * sizeof(void *));
          outItems[rendererCount] = partnerRenderer;
          lod->renderers = replacement;
          changed = true;
        } else if (partnerIndex != SIZE_MAX) {
          // The game can rebuild LOD arrays during a scene transition. Do not
          // leave a partner in a level where its source Renderer is absent.
          void *arrayClass = il2cpp_object_get_class(renderers);
          if (!arrayClass) continue;
          void *replacement = il2cpp_array_new_specific(arrayClass,
                                                        rendererCount - 1);
          if (!replacement) continue;
          void **outItems = (void **)((char *)replacement + 32);
          for (size_t index = 0, outIndex = 0; index < rendererCount; ++index)
            if (index != partnerIndex) outItems[outIndex++] = items[index];
          lod->renderers = replacement;
          changed = true;
        }
      } else {
        if (partnerIndex == SIZE_MAX) continue;
        void *arrayClass = il2cpp_object_get_class(renderers);
        if (!arrayClass) continue;
        void *replacement = il2cpp_array_new_specific(arrayClass,
                                                       rendererCount - 1);
        if (!replacement) continue;
        void **outItems = (void **)((char *)replacement + 32);
        for (size_t index = 0, outIndex = 0; index < rendererCount; ++index) {
          if (index != partnerIndex) outItems[outIndex++] = items[index];
        }
        lod->renderers = replacement;
        changed = true;
      }
    }
    if (sourceLevelsOut) *sourceLevelsOut = sourceLevels;
    if (partnerLevelsOut) *partnerLevelsOut = partnerLevels;
    if (!changed) return false;
    void *params[] = {lods};
    const bool previousApplying = s_eiemApplyingPartnerLod;
    s_eiemApplyingPartnerLod = true;
    Invoke(g_lodGroup_set_lods, group, params);
    s_eiemApplyingPartnerLod = previousApplying;
    Log("[MOD] %s partner Renderer %p in LODGroup %p (%zu LOD level(s))",
        add ? "Added" : "Removed", partnerRenderer, group, lodCount);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[MOD] LODGroup %s failed with exception=0x%08lX",
        add ? "insert" : "remove", GetExceptionCode());
    return false;
  }
}

static bool EiemSetPartnerLodMembershipForGroup(void *sourceRenderer,
                                                void *partnerRenderer, bool add,
                                                void *groupOverride) {
  size_t lodCount = 0;
  void *group = nullptr;
  uint64_t sourceLevels = 0;
  uint64_t partnerLevels = 0;
  const bool changed = EiemSetPartnerLodMembershipInternal(
      sourceRenderer, partnerRenderer, add, &lodCount, &group, &sourceLevels,
      &partnerLevels, groupOverride);
  bool sourceEnabled = true, partnerEnabled = true;
  bool sourceForceOff = false, partnerForceOff = false;
  EiemReadRendererEnabled(sourceRenderer, &sourceEnabled);
  EiemReadRendererEnabled(partnerRenderer, &partnerEnabled);
  EiemReadRendererForceRenderingOff(sourceRenderer, &sourceForceOff);
  EiemReadRendererForceRenderingOff(partnerRenderer, &partnerForceOff);
  EiemRegistrationTraceLod(
      sourceRenderer, partnerRenderer, add, changed, lodCount,
      "partner-visibility", InterlockedCompareExchange(&s_eiemModGeneration, 0,
                                                        0),
      group, sourceLevels, partnerLevels, sourceEnabled, partnerEnabled,
      sourceForceOff, partnerForceOff);
  return changed;
}

static bool EiemSetPartnerLodMembership(void *sourceRenderer,
                                        void *partnerRenderer, bool add) {
  return EiemSetPartnerLodMembershipForGroup(sourceRenderer, partnerRenderer,
                                              add, nullptr);
}

static bool EiemLodArrayContainsRenderer(void *lods, size_t lodCount,
                                         void *renderer) {
  if (!lods || !renderer || lodCount == 0 || lodCount > 64) return false;
  __try {
    char *lodData = (char *)lods + IL2CPP_ARRAY_DATA;
    for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex) {
      EiemNativeLod *lod =
          (EiemNativeLod *)(lodData + lodIndex * sizeof(EiemNativeLod));
      void *renderers = lod->renderers;
      const size_t rendererCount = EiemManagedArrayLength(renderers);
      if (!renderers || rendererCount > 4096) continue;
      void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
      for (size_t index = 0; index < rendererCount; ++index)
        if (items[index] == renderer) return true;
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
  return false;
}

// Partner creation can precede the game's LODGroup assembly. Once the game
// publishes the final Renderer arrays, use that concrete membership to attach
// existing Partners to the same levels. This runs only at SetLODs boundaries;
// it is skipped for the recursive setter call made by our own array update.
static size_t EiemReconcilePartnerLodGroup(void *group, void *lods,
                                           size_t lodCount) {
  if (!group || !lods || s_eiemApplyingPartnerLod) return 0;
  struct Pair {
    void *source = nullptr;
    void *partner = nullptr;
  };
  std::vector<Pair> pairs;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners) {
    // LOD membership is structural, while a key-controlled Partner's
    // visibility is dynamic.  Hidden Partners must still be present when the
    // game publishes its LOD arrays; otherwise showing one later has no
    // reliable group lookup to attach it to and it survives a LOD switch.
    if (!state.partnerRenderer) continue;
    void *source = state.sourceDrawRenderer
                       ? state.sourceDrawRenderer
                       : state.sourceRenderer;
    if (!source || !EiemLodArrayContainsRenderer(lods, lodCount, source))
      continue;
    pairs.push_back({source, state.partnerRenderer});
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);

  size_t changed = 0;
  for (const auto &pair : pairs) {
    if (EiemNativeObjectStatus(pair.source) != 1 ||
        EiemNativeObjectStatus(pair.partner) != 1)
      continue;
    if (EiemSetPartnerLodMembershipForGroup(pair.source, pair.partner, true,
                                             group))
      ++changed;
  }
  if (changed)
    Log("[MOD-LOD] reconciled %zu existing Partner Renderer(s) from game "
        "LODGroup=%p",
        changed, group);
  return changed;
}

static bool EiemReadBoxedBool(void *getter, void *object, bool *value) {
  if (!getter || !object || !value) return false;
  void *boxed = Invoke(getter, object);
  if (!boxed) return false;
  __try {
    *value = *(bool *)((char *)boxed + 16);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static bool EiemReadBoxedInt(void *getter, void *object, int32_t *value) {
  if (!getter || !object || !value) return false;
  void *boxed = Invoke(getter, object);
  if (!boxed) return false;
  __try {
    *value = *(int32_t *)((char *)boxed + 16);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static uint64_t EiemSkinTimelineBoneRefs(void *bones) {
  const size_t count = EiemManagedArrayLength(bones);
  if (!bones || count > 512) return 0;
  void **items = (void **)((char *)bones + IL2CPP_ARRAY_DATA);
  uint64_t hash = 1469598103934665603ull;
  for (size_t index = 0; index < count; ++index) {
    hash ^= (uint64_t)(uintptr_t)items[index];
    hash *= 1099511628211ull;
  }
  return hash;
}

// This probe answers the timing question directly: a later game-owned setter
// is evidence of a post-commit overwrite; no later setter shifts suspicion to
// Unity/game skin-cache invalidation. Keep it small enough for a real run.
static void EiemLogSkinSetterTimeline(const char *event, void *renderer,
                                      void *requestedMesh, void *appliedMesh,
                                      void *incomingBones, void *afterBones) {
  if (!renderer || InterlockedIncrement(&s_eiemSkinTimelineCount) > 240)
    return;
  bool tracked = false;
  void *replacement = nullptr;
  char section[96] = {};
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    const auto &state = s_eiemOverrides[index];
    tracked = !state.restorePending && state.ownsMesh && state.replacementMesh;
    replacement = state.replacementMesh;
    strncpy_s(section, sizeof(section), state.renderSection, _TRUNCATE);
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (!tracked) return;

  const size_t incomingCount = EiemManagedArrayLength(incomingBones);
  const size_t afterCount = EiemManagedArrayLength(afterBones);
  const ULONGLONG now = GetTickCount64();
  const LONG64 commitTick = InterlockedCompareExchange64(
      &s_eiemLastSkinCommitTick, 0, 0);
  const ULONGLONG sinceCommit =
      commitTick > 0 && now >= (ULONGLONG)commitTick
          ? now - (ULONGLONG)commitTick
          : 0;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceNativeStackContext(
      event ? event : "skin-setter", renderer, requestedMesh, incomingBones,
      generation);
  Log("[DEBUG-SKIN-TIMELINE-v1] event=%s tick=%llu gen=%ld tid=%lu "
      "renderer=%p section=%s requestedMesh=%p appliedMesh=%p currentMesh=%p "
      "replacement=%p incomingBones=%p incomingCount=%zu incomingRefs=%016llX "
      "afterBones=%p afterCount=%zu afterRefs=%016llX sinceCommitMs=%llu",
      event ? event : "unknown", (unsigned long long)now, generation,
      (unsigned long)GetCurrentThreadId(), renderer,
      section[0] ? section : "<unknown>", requestedMesh, appliedMesh,
      EiemReadSharedMesh(renderer, "SkinnedMeshRenderer"), replacement,
      incomingBones, incomingCount,
      (unsigned long long)EiemSkinTimelineBoneRefs(incomingBones), afterBones,
      afterCount, (unsigned long long)EiemSkinTimelineBoneRefs(afterBones),
      (unsigned long long)sinceCommit);
}

static uint64_t EiemSkinTimingBoneMatrixHash(void *bones) {
  if (!bones || !g_transform_get_localToWorldMatrix) return 0;
  const size_t count = EiemManagedArrayLength(bones);
  if (count > 512) return 0;
  void **items = (void **)((char *)bones + IL2CPP_ARRAY_DATA);
  uint64_t hash = 1469598103934665603ULL;
  for (size_t index = 0; index < count; ++index) {
    const uintptr_t identity = (uintptr_t)items[index];
    hash ^= (uint64_t)identity;
    hash *= 1099511628211ULL;
    if (!items[index]) continue;
    __try {
      void *boxed = Invoke(g_transform_get_localToWorldMatrix, items[index]);
      if (!boxed) continue;
      const unsigned char *bytes = (const unsigned char *)boxed + 16;
      for (size_t byte = 0; byte < sizeof(float) * 16; ++byte) {
        hash ^= bytes[byte];
        hash *= 1099511628211ULL;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      hash ^= 0xA5;
      hash *= 1099511628211ULL;
    }
  }
  return hash;
}

static uint64_t EiemSkinTimingTransformMatrixHash(void *transform) {
  if (!transform || !g_transform_get_localToWorldMatrix) return 0;
  __try {
    void *boxed = Invoke(g_transform_get_localToWorldMatrix, transform);
    if (!boxed) return 0;
    const unsigned char *bytes = (const unsigned char *)boxed + 16;
    uint64_t hash = 1469598103934665603ULL;
    for (size_t byte = 0; byte < sizeof(float) * 16; ++byte) {
      hash ^= bytes[byte];
      hash *= 1099511628211ULL;
    }
    return hash;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
}

// Compare only Transform objects shared by two Renderer palettes.  A clothing
// Mesh may contain slots supplied by several source Meshes, so comparing the
// whole palette hash would mix its extra/physical slots with the torso slots.
// This observation uses Transform identity, never names or target-Lod indices.
static uint64_t EiemSkinTimingSharedBoneMatrixHash(void *left, void *right,
                                                   size_t *sharedCount) {
  if (sharedCount) *sharedCount = 0;
  if (!left || !right || !g_transform_get_localToWorldMatrix) return 0;
  const size_t leftCount = EiemManagedArrayLength(left);
  const size_t rightCount = EiemManagedArrayLength(right);
  if (!leftCount || !rightCount || leftCount > 512 || rightCount > 512)
    return 0;
  void **leftItems = (void **)((char *)left + IL2CPP_ARRAY_DATA);
  void **rightItems = (void **)((char *)right + IL2CPP_ARRAY_DATA);
  uint64_t hash = 1469598103934665603ULL;
  size_t matches = 0;
  for (size_t rightIndex = 0; rightIndex < rightCount; ++rightIndex) {
    void *transform = rightItems[rightIndex];
    if (!transform) continue;
    bool found = false;
    for (size_t leftIndex = 0; leftIndex < leftCount; ++leftIndex) {
      if (leftItems[leftIndex] == transform) {
        found = true;
        break;
      }
    }
    if (!found) continue;
    ++matches;
    const uintptr_t identity = (uintptr_t)transform;
    hash ^= (uint64_t)identity;
    hash *= 1099511628211ULL;
    __try {
      void *boxed = Invoke(g_transform_get_localToWorldMatrix, transform);
      if (!boxed) continue;
      const unsigned char *bytes = (const unsigned char *)boxed + 16;
      for (size_t byte = 0; byte < sizeof(float) * 16; ++byte) {
        hash ^= bytes[byte];
        hash *= 1099511628211ULL;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      hash ^= 0xA5;
      hash *= 1099511628211ULL;
    }
  }
  if (sharedCount) *sharedCount = matches;
  return matches ? hash : 0;
}

// Hash only the Transform objects present in the clothing palette but absent
// from the body's palette. These are the cloth/skirt/other clothing-specific
// slots that the shared-body probe intentionally excludes. This observation
// uses Transform identity, never names or target-LOD indices, and never writes
// to Unity state.
static uint64_t EiemSkinTimingClothOnlyBoneMatrixHash(void *body,
                                                      void *cloth,
                                                      size_t *clothOnlyCount) {
  if (clothOnlyCount) *clothOnlyCount = 0;
  if (!body || !cloth || !g_transform_get_localToWorldMatrix) return 0;
  const size_t bodyCount = EiemManagedArrayLength(body);
  const size_t clothCount = EiemManagedArrayLength(cloth);
  if (!bodyCount || !clothCount || bodyCount > 512 || clothCount > 512)
    return 0;
  void **bodyItems = (void **)((char *)body + IL2CPP_ARRAY_DATA);
  void **clothItems = (void **)((char *)cloth + IL2CPP_ARRAY_DATA);
  uint64_t hash = 1469598103934665603ULL;
  size_t matches = 0;
  for (size_t clothIndex = 0; clothIndex < clothCount; ++clothIndex) {
    void *transform = clothItems[clothIndex];
    if (!transform) continue;
    bool shared = false;
    for (size_t bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex) {
      if (bodyItems[bodyIndex] == transform) {
        shared = true;
        break;
      }
    }
    if (shared) continue;
    ++matches;
    const uintptr_t identity = (uintptr_t)transform;
    hash ^= (uint64_t)identity;
    hash *= 1099511628211ULL;
    __try {
      void *boxed = Invoke(g_transform_get_localToWorldMatrix, transform);
      if (!boxed) {
        hash ^= 0xD1;
        hash *= 1099511628211ULL;
        continue;
      }
      const unsigned char *bytes = (const unsigned char *)boxed + 16;
      for (size_t byte = 0; byte < sizeof(float) * 16; ++byte) {
        hash ^= bytes[byte];
        hash *= 1099511628211ULL;
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      hash ^= 0xA7;
      hash *= 1099511628211ULL;
    }
  }
  if (clothOnlyCount) *clothOnlyCount = matches;
  return matches ? hash : 0;
}

// Correlate Endfield's parallel HG component path with the ordinary
// SkinnedMeshRenderer path on the exact same GameObject. This is limited to
// the existing cold/F10 probe windows. HGMeshRendererData has a native-facing
// value-type ABI, so this pass intentionally avoids get_data until its layout
// has been collected by the metadata probe.
static void EiemLogHgRendererCensus(void *model, void *root, LONG transaction,
                                    const char *phase) {
  if (!kEiemEnableCustomSkinPipelineObservation || !model || !root ||
      !g_hgMeshRendererClass || !g_skinnedMeshRendererClass ||
      !g_gameObject_GetComponentsInChildren || !g_gameObject_GetComponent ||
      !g_component_get_gameObject || !il2cpp_class_get_type ||
      !il2cpp_type_get_object)
    return;

  void *hgType = il2cpp_class_get_type(g_hgMeshRendererClass);
  void *hgTypeObject = hgType ? il2cpp_type_get_object(hgType) : nullptr;
  void *smrType = il2cpp_class_get_type(g_skinnedMeshRendererClass);
  void *smrTypeObject = smrType ? il2cpp_type_get_object(smrType) : nullptr;
  if (!hgTypeObject || !smrTypeObject) return;

  bool includeInactive = true;
  void *childrenParams[] = {hgTypeObject, &includeInactive};
  void *array = Invoke(g_gameObject_GetComponentsInChildren, model,
                       childrenParams);
  const size_t count = EiemManagedArrayLength(array);
  if (!array || count > 4096) {
    Log("[HG-CENSUS-v1] transaction=%ld phase=%s owner=%p count=invalid",
        transaction, phase ? phase : "unknown", model);
    return;
  }

  void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
  size_t paired = 0;
  size_t targets = 0;
  for (size_t index = 0; index < count && index < 256; ++index) {
    void *hg = items[index];
    if (!hg) continue;
    char path[768] = {};
    EiemBuildRelativeRendererPath(root, hg, path, sizeof(path));
    void *gameObject = Invoke(g_component_get_gameObject, hg);
    void *pairedSmr = nullptr;
    if (gameObject) {
      void *componentParams[] = {smrTypeObject};
      pairedSmr = Invoke(g_gameObject_GetComponent, gameObject,
                         componentParams);
    }
    if (pairedSmr) ++paired;
    const bool target =
        path[0] && (strstr(path, "body_01") || strstr(path, "cloth_01") ||
                    strstr(path, "cloth_02"));
    if (!target) continue;
    ++targets;
    void *mesh = pairedSmr
                     ? EiemReadSharedMesh(pairedSmr, "SkinnedMeshRenderer")
                     : nullptr;
    void *bones = pairedSmr && g_smr_get_bones
                      ? Invoke(g_smr_get_bones, pairedSmr)
                      : nullptr;
    bool tracked = false;
    if (pairedSmr) {
      AcquireSRWLockShared(&s_eiemOverrideLock);
      tracked = EiemFindOverrideLocked(pairedSmr) != SIZE_MAX;
      ReleaseSRWLockShared(&s_eiemOverrideLock);
    }
    Log("[HG-CENSUS-v1] transaction=%ld phase=%s owner=%p hg=%p "
        "path=%s gameObject=%p pairedSmr=%p tracked=%d mesh=%p bones=%zu",
        transaction, phase ? phase : "unknown", model, hg, path, gameObject,
        pairedSmr, tracked ? 1 : 0, mesh, EiemManagedArrayLength(bones));
  }
  Log("[HG-CENSUS-v1] transaction=%ld phase=%s owner=%p total=%zu "
      "paired=%zu targetPaths=%zu",
      transaction, phase ? phase : "unknown", model, count, paired, targets);
}

// Enumerate the complete Unity Renderer hierarchy for the model owner.  The
// existing POSE-CENSUS only asks for SkinnedMeshRenderer, so it cannot rule out
// a parallel MeshRenderer/custom Renderer being the object actually submitted
// for a visible clothing draw.  This pass is read-only and bounded; it does
// not register, replace, enable, disable, or otherwise touch any component.
static void EiemLogAllRendererCensus(void *model, void *root, LONG transaction,
                                     const char *phase) {
  if (!kEiemEnableCustomSkinPipelineObservation || !model || !root ||
      !g_rendererClass || !g_gameObject_GetComponentsInChildren ||
      !il2cpp_class_get_type || !il2cpp_type_get_object)
    return;
  __try {
    void *type = il2cpp_class_get_type(g_rendererClass);
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!typeObject) return;
    bool includeInactive = true;
    void *params[] = {typeObject, &includeInactive};
    void *array = Invoke(g_gameObject_GetComponentsInChildren, model, params);
    const size_t count = EiemManagedArrayLength(array);
    if (!array || count > 8192) {
      Log("[POSE-RENDERER-CENSUS-v1] transaction=%ld phase=%s owner=%p "
          "root=%p count=invalid",
          transaction, phase ? phase : "unknown", model, root);
      return;
    }
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    size_t logged = 0;
    size_t targetCount = 0;
    size_t visibleCount = 0;
    for (size_t index = 0; index < count && logged < 512; ++index) {
      void *renderer = items[index];
      if (!renderer) continue;
      char path[768] = {};
      EiemBuildRelativeRendererPath(root, renderer, path, sizeof(path));
      const char *rendererType = "Renderer";
      void *mesh = EiemReadLodRendererMesh(renderer, &rendererType);
      char rendererClass[128] = {};
      if (il2cpp_object_get_class && il2cpp_class_get_name) {
        void *klass = il2cpp_object_get_class(renderer);
        const char *name = klass ? il2cpp_class_get_name(klass) : nullptr;
        if (name) strncpy_s(rendererClass, sizeof(rendererClass), name,
                            _TRUNCATE);
      }
      bool enabled = false;
      bool visible = false;
      const bool enabledRead = EiemReadRendererEnabled(renderer, &enabled);
      const bool visibleRead = EiemReadRendererVisible(renderer, &visible);
      if (visibleRead && visible) ++visibleCount;
      const bool target = path[0] &&
                          (strstr(path, "body_01") ||
                           strstr(path, "cloth_01") ||
                           strstr(path, "cloth_02"));
      if (target) ++targetCount;
      // Keep the output focused on the model's clothing/body path and any
      // Renderer that is currently visible.  Inactive unrelated effects are
      // still represented by the summary count above.
      if (!target && !(visibleRead && visible)) continue;
      bool tracked = false;
      AcquireSRWLockShared(&s_eiemOverrideLock);
      tracked = EiemFindOverrideLocked(renderer) != SIZE_MAX;
      ReleaseSRWLockShared(&s_eiemOverrideLock);
      char meshDescription[384] = {};
      TraceDescribeObject(mesh, meshDescription, sizeof(meshDescription));
      Log("[POSE-RENDERER-CENSUS-v1] transaction=%ld phase=%s owner=%p "
          "index=%zu renderer=%p class=%s type=%s tracked=%d path=%s "
          "mesh=%p meshDesc=%s enabled=%s visible=%s",
          transaction, phase ? phase : "unknown", model, index, renderer,
          rendererClass[0] ? rendererClass : "<unknown>", rendererType,
          tracked ? 1 : 0, path[0] ? path : "<root>", mesh,
          meshDescription[0] ? meshDescription : "<unknown>",
          enabledRead ? (enabled ? "1" : "0") : "?",
          visibleRead ? (visible ? "1" : "0") : "?");
      ++logged;
    }
    Log("[POSE-RENDERER-CENSUS-v1] transaction=%ld phase=%s owner=%p "
        "root=%p count=%zu targetCount=%zu visibleCount=%zu logged=%zu",
        transaction, phase ? phase : "unknown", model, root, count,
        targetCount, visibleCount, logged);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[POSE-RENDERER-CENSUS-v1] transaction=%ld phase=%s owner=%p "
        "read=exception=0x%08lX",
        transaction, phase ? phase : "unknown", model, GetExceptionCode());
  }
}

static bool EiemSkinTargetFamily(const char *section, const char *family) {
  return section && family && strstr(section, family) != nullptr;
}

static int EiemSkinTargetRank(const char *section) {
  if (!section) return 99;
  if (strstr(section, "_lod0") != nullptr) return 0;
  if (strstr(section, "_lod1") != nullptr) return 1;
  if (strstr(section, "_lod2") != nullptr) return 2;
  if (strstr(section, "_lod3") != nullptr) return 3;
  return 10;
}

// Select one model owner and one authored family entry per body/cloth pair.
// Replacement state can contain many NPCs and four LOD sections; observing
// all of them was the source of the previous startup/frame spikes.  The
// selected pointers are evidence-only and never used for binding decisions.
static bool EiemSelectSkinTargetStates(
    LONG transaction, std::vector<EiemRenderOverrideState> *out,
    void **selectedOwner) {
  if (out) out->clear();
  if (selectedOwner) *selectedOwner = nullptr;
  if (!out) return false;
  struct Candidate {
    uintptr_t owner = 0;
    int families = 0;
    int bestRank = 99;
    bool body = false, cloth01 = false, cloth02 = false;
  };
  std::vector<Candidate> candidates;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  for (const auto &state : s_eiemOverrides) {
    if (state.restorePending || !state.renderer ||
        !EiemModEquals(state.rendererType, "SkinnedMeshRenderer") ||
        !state.ownerPrefabInstance)
      continue;
    const bool body = EiemSkinTargetFamily(state.renderSection, "body_01");
    const bool cloth01 = EiemSkinTargetFamily(state.renderSection, "cloth_01");
    const bool cloth02 = EiemSkinTargetFamily(state.renderSection, "cloth_02");
    if (!body && !cloth01 && !cloth02) continue;
    auto it = std::find_if(candidates.begin(), candidates.end(),
                           [&](const Candidate &c) {
                             return c.owner == state.ownerPrefabInstance;
                           });
    if (it == candidates.end()) {
      candidates.push_back({state.ownerPrefabInstance, 0, 99, body, cloth01,
                            cloth02});
    } else {
      it->body = it->body || body;
      it->cloth01 = it->cloth01 || cloth01;
      it->cloth02 = it->cloth02 || cloth02;
    }
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (candidates.empty()) return false;
  for (auto &candidate : candidates) {
    candidate.families = (candidate.body ? 1 : 0) +
                         (candidate.cloth01 ? 1 : 0) +
                         (candidate.cloth02 ? 1 : 0);
  }
  void *preferred = s_eiemSkinTargetOwner;
  auto chosen = std::find_if(candidates.begin(), candidates.end(),
                             [&](const Candidate &c) {
                               return preferred &&
                                      c.owner == (uintptr_t)preferred &&
                                      c.families == 3;
                             });
  if (chosen == candidates.end()) {
    chosen = std::max_element(
        candidates.begin(), candidates.end(),
        [](const Candidate &left, const Candidate &right) {
          return left.families < right.families;
        });
  }
  if (chosen == candidates.end() || chosen->families < 3) {
    Log("[CPU-SKIN-TARGET-v1] transaction=%ld selected=0 owners=%zu "
        "reason=no-owner-with-body-cloth01-cloth02",
        transaction, candidates.size());
    return false;
  }
  s_eiemSkinTargetOwner = (void *)chosen->owner;
  if (selectedOwner) *selectedOwner = s_eiemSkinTargetOwner;

  // Choose the lowest authored LOD for each family.  This is a stable
  // observation key, not a LOD policy and does not affect the game.
  const char *families[] = {"body_01", "cloth_01", "cloth_02"};
  for (const char *family : families) {
    EiemRenderOverrideState best = {};
    int bestRank = 99;
    bool found = false;
    AcquireSRWLockShared(&s_eiemOverrideLock);
    for (const auto &state : s_eiemOverrides) {
      if (state.restorePending || !state.renderer ||
          state.ownerPrefabInstance != chosen->owner ||
          !EiemModEquals(state.rendererType, "SkinnedMeshRenderer") ||
          !EiemSkinTargetFamily(state.renderSection, family))
        continue;
      const int rank = EiemSkinTargetRank(state.renderSection);
      if (!found || rank < bestRank) {
        best = state;
        bestRank = rank;
        found = true;
      }
    }
    ReleaseSRWLockShared(&s_eiemOverrideLock);
    if (found) out->push_back(best);
  }
  Log("[CPU-SKIN-TARGET-v1] transaction=%ld selected=1 owner=%p "
      "families=%zu body=%d cloth01=%d cloth02=%d",
      transaction, s_eiemSkinTargetOwner, out->size(),
      chosen->body ? 1 : 0, chosen->cloth01 ? 1 : 0,
      chosen->cloth02 ? 1 : 0);
  return out->size() == 3;
}

// Compare the exact same cloth_02 Renderers immediately after replay and on
// the next Unity window cycle. If the first sample is healthy and the second
// one is ground-bound, a later animation/physics/LOD write is the cause. If
// both samples are already wrong, the commit boundary itself is too early or
// its source transform state is not complete. This function only reads Unity
// state and is intentionally capped to avoid turning diagnostics into a
// frame-time spike.
static void EiemLogSkinTimingProbe(const char *phase, LONG transaction) {
  // The full probe reads vertices, weights and bindposes. The binding-only
  // mode below reads only palette/root identities and matrix fingerprints.
  // Both modes are evidence-only and opt in independently.
  if (!kEiemEnableSkinDiagnostics && !kEiemEnableSkinBindingDiagnostics)
    return;
  if (!EiemOnUnityThread()) {
    Log("[SKIN-TIMING] transaction=%ld phase=%s skipped=not-unity-thread "
        "tid=%lu unityTid=%lu",
        transaction, phase ? phase : "unknown",
        (unsigned long)GetCurrentThreadId(),
        (unsigned long)s_eiemUnityThreadId);
    return;
  }
  std::vector<EiemRenderOverrideState> targets;
  void *targetOwner = nullptr;
  EiemSelectSkinTargetStates(transaction, &targets, &targetOwner);

  if (targets.empty()) {
    Log("[SKIN-TIMING] transaction=%ld phase=%s samples=0 tid=%lu",
        transaction, phase ? phase : "unknown",
        (unsigned long)GetCurrentThreadId());
    return;
  }
  if (kEiemEnableNativePhysicsObservation)
    EiemPhysicsOrderProbeLogSinceRender(transaction, phase);
  if (!kEiemEnableSkinDiagnostics && kEiemEnableSkinBindingDiagnostics) {
    size_t sampled = 0;
    for (const auto &state : targets) {
      bool enabled = false;
      bool visible = false;
      const bool enabledRead = EiemReadRendererEnabled(state.drawRenderer,
                                                        &enabled);
      const bool visibleRead = EiemReadRendererVisible(state.drawRenderer,
                                                        &visible);
      void *mesh = EiemReadSharedMesh(state.renderer, "SkinnedMeshRenderer");
      void *bones = g_smr_get_bones ? Invoke(g_smr_get_bones, state.renderer)
                                    : nullptr;
      void *rootBone = g_smr_get_rootBone
                           ? Invoke(g_smr_get_rootBone, state.renderer)
                           : nullptr;
      void *skinningRoot = g_smr_get_skinningRoot
                               ? Invoke(g_smr_get_skinningRoot, state.renderer)
                               : nullptr;
      bool updateWhenOffscreen = false;
      bool forceMatrixPerRender = false;
      bool skinnedMotionVectors = false;
      const bool updateWhenOffscreenRead = EiemReadBoxedBool(
          g_smr_get_updateWhenOffscreen, state.renderer,
          &updateWhenOffscreen);
      const bool forceMatrixPerRenderRead = EiemReadBoxedBool(
          g_smr_get_forceMatrixRecalculationPerRender, state.renderer,
          &forceMatrixPerRender);
      const bool skinnedMotionVectorsRead = EiemReadBoxedBool(
          g_smr_get_skinnedMotionVectors, state.renderer,
          &skinnedMotionVectors);
      void *bodyBones = nullptr;
      if (strstr(state.renderSection, "cloth_") != nullptr) {
        for (const auto &body : targets) {
          if (body.ownerPrefabInstance != state.ownerPrefabInstance ||
              strstr(body.renderSection, "body_01") == nullptr)
            continue;
          bodyBones = g_smr_get_bones
                          ? Invoke(g_smr_get_bones, body.renderer)
                          : nullptr;
          if (bodyBones) break;
        }
      }
      size_t sharedCount = 0;
      size_t clothOnlyCount = 0;
      const uint64_t sharedMatrix =
          bodyBones ? EiemSkinTimingSharedBoneMatrixHash(
                          bodyBones, bones, &sharedCount)
                    : 0;
      const uint64_t clothOnlyMatrix =
          bodyBones ? EiemSkinTimingClothOnlyBoneMatrixHash(
                          bodyBones, bones, &clothOnlyCount)
                    : 0;
      const EiemSkinProbe::WorldBounds bounds =
          EiemSkinProbe::ReadRendererBounds(state.renderer);
      EiemPhysicsTargetAssociation physicsAssociation;
      if (kEiemEnableNativePhysicsObservation && bones) {
        const size_t count = EiemManagedArrayLength(bones);
        if (count && count <= 16384) {
          void **items = (void **)((char *)bones + IL2CPP_ARRAY_DATA);
          physicsAssociation =
              EiemPhysicsAssociateTargetBones(items, count);
        }
      }
      Log("[POSE-BIND-WINDOW-v1] transaction=%ld phase=%s owner=%p "
          "renderer=%p section=%s enabled=%s visible=%s currentMesh=%p "
          "expectedMesh=%p bones=%p count=%zu refs=%016llX matrixRefs=%016llX "
          "rootBone=%p rootMatrix=%016llX sharedCount=%zu "
          "sharedMatrix=%016llX clothOnlyCount=%zu clothOnlyMatrix=%016llX "
          "skinningRoot=%p updateWhenOffscreen=%s "
          "forceMatrixPerRender=%s skinnedMotionVectors=%s "
          "boundsRead=%d boundsCenterY=%.3f boundsMaxY=%.3f",
          transaction, phase ? phase : "unknown",
          (void *)state.ownerPrefabInstance, state.renderer,
          state.renderSection, enabledRead ? (enabled ? "1" : "0") : "?",
          visibleRead ? (visible ? "1" : "0") : "?", mesh,
          state.replacementMesh, bones, EiemManagedArrayLength(bones),
          (unsigned long long)EiemSkinTimelineBoneRefs(bones),
          (unsigned long long)EiemSkinTimingBoneMatrixHash(bones), rootBone,
          (unsigned long long)EiemSkinTimingTransformMatrixHash(rootBone),
          sharedCount, (unsigned long long)sharedMatrix, clothOnlyCount,
          (unsigned long long)clothOnlyMatrix, skinningRoot,
          updateWhenOffscreenRead
              ? (updateWhenOffscreen ? "1" : "0")
              : "?",
          forceMatrixPerRenderRead
              ? (forceMatrixPerRender ? "1" : "0")
              : "?",
          skinnedMotionVectorsRead
              ? (skinnedMotionVectors ? "1" : "0")
              : "?",
          bounds.read ? 1 : 0,
          bounds.read ? bounds.CenterY() : 0.0f,
          bounds.read ? bounds.maxY : 0.0f);
      Log("[CPU-SKIN-PHYSICS-v1] transaction=%ld phase=%s owner=%p "
          "section=%s bones=%p matchedTransforms=%zu matchedTeams=%zu "
          "latestPhysicsSeq=%llu team0=%d clothProcess0=%p",
          transaction, phase ? phase : "unknown", targetOwner,
          state.renderSection, bones, physicsAssociation.matchedTransforms,
          physicsAssociation.matchedTeams,
          (unsigned long long)physicsAssociation.latestSequence,
          physicsAssociation.matchedTeams ? physicsAssociation.teamIds[0] : -1,
          physicsAssociation.matchedTeams
              ? (void *)physicsAssociation.clothProcesses[0]
              : nullptr);
      // Only the second post-F10 sample performs the expensive vertex/weight
      // calculation.  Cold start and the first window retain the cheap
      // palette/root fingerprints above, so entering the game stays bounded.
      if (transaction > 0 && phase &&
          strcmp(phase, "post-window-2") == 0) {
        const LONG64 started = EiemPerfNow();
        const EiemSkinProbe::Result result =
            EiemSkinProbe::Measure(state.renderer);
        EiemSkinProbe::LogResult("[CPU-SKIN-C-v1]", result);
        Log("[CPU-SKIN-C-v1] transaction=%ld phase=%s owner=%p section=%s "
            "measureMs=%.2f physicsCompletionSeq=%llu",
            transaction, phase, targetOwner, state.renderSection,
            EiemPerfMilliseconds(EiemPerfNow() - started),
            (unsigned long long)EiemPhysicsOrderCompletionSequence());
      }
      ++sampled;
    }
      Log("[POSE-BIND-WINDOW-v1] transaction=%ld phase=%s samples=%zu",
        transaction, phase ? phase : "unknown", sampled);

    // Target selection above already binds body/cloth01/cloth02 to one owner.
    // Do not census the complete hierarchy here: that was the major startup
    // stall and it did not improve the CPU ownership evidence.
    Log("[CPU-SKIN-D-v1] transaction=%ld phase=%s owner=%p targets=%zu "
        "boundary=last-proven-public-renderer-state",
        transaction, phase ? phase : "unknown", targetOwner, targets.size());
    return;
  }
  size_t measured = 0;
  for (const auto &state : targets) {
    bool enabled = false;
    bool visible = false;
    const bool enabledRead = EiemReadRendererEnabled(state.drawRenderer,
                                                      &enabled);
    const bool visibleRead = EiemReadRendererVisible(state.drawRenderer,
                                                      &visible);
    void *mesh = EiemReadSharedMesh(state.renderer, "SkinnedMeshRenderer");
    void *bones = g_smr_get_bones ? Invoke(g_smr_get_bones, state.renderer)
                                  : nullptr;
    void *rootBone = g_smr_get_rootBone
                         ? Invoke(g_smr_get_rootBone, state.renderer)
                         : nullptr;
    void *skinningRoot = g_smr_get_skinningRoot
                             ? Invoke(g_smr_get_skinningRoot, state.renderer)
                             : nullptr;
    bool updateWhenOffscreen = false;
    bool forceMatrixPerRender = false;
    bool skinnedMotionVectors = false;
    const bool updateWhenOffscreenRead = EiemReadBoxedBool(
        g_smr_get_updateWhenOffscreen, state.renderer, &updateWhenOffscreen);
    const bool forceMatrixPerRenderRead = EiemReadBoxedBool(
        g_smr_get_forceMatrixRecalculationPerRender, state.renderer,
        &forceMatrixPerRender);
    const bool skinnedMotionVectorsRead = EiemReadBoxedBool(
        g_smr_get_skinnedMotionVectors, state.renderer,
        &skinnedMotionVectors);
    void *rendererTransform = g_component_get_transform
                                  ? Invoke(g_component_get_transform,
                                           state.renderer)
                                  : nullptr;
    const size_t boneCount = EiemManagedArrayLength(bones);
    const uint64_t matrixHash = EiemSkinTimingBoneMatrixHash(bones);
    const uint64_t rootMatrixHash =
        EiemSkinTimingTransformMatrixHash(rootBone);
    const uint64_t rendererMatrixHash =
        EiemSkinTimingTransformMatrixHash(rendererTransform);
    void *bodyBones = nullptr;
    if (strstr(state.renderSection, "cloth_") != nullptr) {
      for (const auto &body : targets) {
        if (body.ownerPrefabInstance != state.ownerPrefabInstance ||
            strstr(body.renderSection, "body_01") == nullptr)
          continue;
        bodyBones = g_smr_get_bones
                        ? Invoke(g_smr_get_bones, body.renderer)
                        : nullptr;
        if (bodyBones) break;
      }
    }
    size_t sharedCount = 0;
    const uint64_t sharedMatrixHash =
        bodyBones ? EiemSkinTimingSharedBoneMatrixHash(bodyBones, bones,
                                                        &sharedCount)
                  : 0;
    size_t clothOnlyCount = 0;
    const uint64_t clothOnlyMatrixHash =
        bodyBones ? EiemSkinTimingClothOnlyBoneMatrixHash(
                        bodyBones, bones, &clothOnlyCount)
                  : 0;
    char tag[768] = {};
    snprintf(tag, sizeof(tag),
             "[SKIN-TIMING] transaction=%ld phase=%s tid=%lu renderer=%p "
             "owner=%p section=%s enabled=%s visible=%s currentMesh=%p "
             "expectedMesh=%p bones=%p count=%zu refs=%016llX "
             "matrixRefs=%016llX rootBone=%p rootMatrix=%016llX "
             "rendererMatrix=%016llX skinningRoot=%p "
             "updateWhenOffscreen=%s forceMatrixPerRender=%s "
             "skinnedMotionVectors=%s",
             transaction, phase ? phase : "unknown",
             (unsigned long)GetCurrentThreadId(), state.renderer,
             (void *)state.ownerPrefabInstance, state.renderSection,
             enabledRead ? (enabled ? "1" : "0") : "?",
             visibleRead ? (visible ? "1" : "0") : "?", mesh,
             state.replacementMesh, bones, boneCount,
             (unsigned long long)EiemSkinTimelineBoneRefs(bones),
             (unsigned long long)matrixHash, rootBone,
             (unsigned long long)rootMatrixHash,
             (unsigned long long)rendererMatrixHash, skinningRoot,
             updateWhenOffscreenRead
                 ? (updateWhenOffscreen ? "1" : "0")
                 : "?",
             forceMatrixPerRenderRead
                 ? (forceMatrixPerRender ? "1" : "0")
                 : "?",
             skinnedMotionVectorsRead
                 ? (skinnedMotionVectors ? "1" : "0")
                 : "?");
    if (bodyBones) {
      const size_t used = strnlen(tag, sizeof(tag));
      if (used < sizeof(tag))
        snprintf(tag + used, sizeof(tag) - used,
                 " sharedBodyCount=%zu sharedBodyMatrix=%016llX"
                 " clothOnlyCount=%zu clothOnlyMatrix=%016llX",
                 sharedCount, (unsigned long long)sharedMatrixHash,
                 clothOnlyCount, (unsigned long long)clothOnlyMatrixHash);
    }
    const EiemSkinProbe::Result result =
        EiemSkinProbe::Measure(state.renderer);
    EiemSkinProbe::LogResult(tag, result);
    ++measured;
  }
  Log("[SKIN-TIMING] transaction=%ld phase=%s samples=%zu tid=%lu",
      transaction, phase ? phase : "unknown", measured,
      (unsigned long)GetCurrentThreadId());
}

static void EiemArmSkinTimingProbe(LONG transaction) {
  if (!kEiemEnableSkinTimingProbe && !kEiemEnableSkinDiagnostics &&
      !kEiemEnableSkinBindingDiagnostics)
    return;
  InterlockedExchange(&s_eiemSkinTimingProbePending, transaction);
  s_eiemSkinTargetOwner = nullptr;
  InterlockedExchange(&s_eiemSkinTargetTransaction, transaction);
  InterlockedExchange(&s_eiemSkinTimingProbeStage, 0);
  InterlockedExchange(&s_eiemSkinNativeTrackedCalls, 0);
  InterlockedExchange(&s_eiemSkinNativeUntrackedCalls, 0);
  InterlockedExchange(&s_eiemSkinCaptureRequestCalls, 0);
  InterlockedExchange(&s_eiemGpuClothObservationCalls, 0);
  InterlockedExchange(&s_eiemGpuClothEventSequence, 0);
  InterlockedExchange(&s_eiemSkinBufferBindingCalls, 0);
  // Both cold and F10 evidence need two windows. F10 performs its one full
  // CPU skin calculation in window 2; cold start remains fingerprints-only.
  // GPU submission hooks remain disabled.
  InterlockedExchange(&s_eiemSkinTimingProbeTicks, 2);
  if (g_gameHwnd && IsWindow(g_gameHwnd))
    SetTimer(g_gameHwnd, kEiemSkinTimingProbeTimer,
             transaction < 0 ? 250 : 32, nullptr);
}

static void EiemMaybeArmColdSkinTimingProbe() {
  if (!kEiemEnableSkinDiagnostics && !kEiemEnableSkinBindingDiagnostics) return;
  if (InterlockedCompareExchange(&s_eiemSkinTimingProbeSequence, 0, 0) != 0)
    return;
  if (InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0) != 0)
    return;
  std::vector<EiemRenderOverrideState> targets;
  void *owner = nullptr;
  if (!EiemSelectSkinTargetStates(-1, &targets, &owner) || !owner) return;
  if (InterlockedCompareExchange(&s_eiemSkinColdProbeArmed, 1, 0) != 0)
    return;
  // A negative transaction is reserved for the one cold-start window.  It
  // shares the same bounded timer path as F10, but is labelled separately.
  EiemArmSkinTimingProbe(-1);
  Log("[SKIN-TIMING] cold-start probe armed");
}

static void EiemRunSkinTimingProbe() {
  if (!kEiemEnableSkinTimingProbe && !kEiemEnableSkinDiagnostics &&
      !kEiemEnableSkinBindingDiagnostics)
    return;
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction) return;
  const LONG stage = InterlockedIncrement(&s_eiemSkinTimingProbeStage);
  char phase[64] = {};
  if (transaction < 0)
    snprintf(phase, sizeof(phase), "cold-start-post-window-%ld", stage);
  else
    snprintf(phase, sizeof(phase), "post-window-%ld", stage);
  EiemLogSkinTimingProbe(phase, transaction);
  const LONG remaining = InterlockedDecrement(&s_eiemSkinTimingProbeTicks);
  if (remaining > 0 && g_gameHwnd && IsWindow(g_gameHwnd)) {
    SetTimer(g_gameHwnd, kEiemSkinTimingProbeTimer,
             transaction < 0 ? 500 : 32, nullptr);
  } else {
    InterlockedExchange(&s_eiemSkinTimingProbePending, 0);
  }
}

// A partner is another view of the same source skin. Unity does not clone the
// source SkinnedMeshRenderer settings when AddComponent creates it, so copy the
// state that controls bone-matrix space and update scheduling explicitly.
static void EiemCopySkinnedRendererState(void *source, void *partner) {
  if (!source || !partner) return;
  auto copyBool = [&](void *getter, void *setter) {
    bool value = false;
    if (!setter || !EiemReadBoxedBool(getter, source, &value)) return;
    void *params[] = {&value};
    Invoke(setter, partner, params);
  };
  auto copyInt = [&](void *getter, void *setter) {
    int32_t value = 0;
    if (!setter || !EiemReadBoxedInt(getter, source, &value)) return;
    void *params[] = {&value};
    Invoke(setter, partner, params);
  };
  auto copyReference = [&](void *getter, void *setter) {
    if (!getter || !setter) return;
    void *value = Invoke(getter, source);
    void *params[] = {value};
    Invoke(setter, partner, params);
  };

  copyInt(g_smr_get_quality, g_smr_set_quality);
  copyBool(g_smr_get_updateWhenOffscreen, g_smr_set_updateWhenOffscreen);
  copyBool(g_smr_get_forceMatrixRecalculationPerRender,
           g_smr_set_forceMatrixRecalculationPerRender);
  copyBool(g_smr_get_skinnedMotionVectors,
           g_smr_set_skinnedMotionVectors);
  copyReference(g_smr_get_skinningRoot, g_smr_set_skinningRoot);

  int32_t sourceQuality = -1, partnerQuality = -1;
  bool sourceOffscreen = false, partnerOffscreen = false;
  bool sourceForceMatrix = false, partnerForceMatrix = false;
  bool sourceMotion = false, partnerMotion = false;
  EiemReadBoxedInt(g_smr_get_quality, source, &sourceQuality);
  EiemReadBoxedInt(g_smr_get_quality, partner, &partnerQuality);
  EiemReadBoxedBool(g_smr_get_updateWhenOffscreen, source, &sourceOffscreen);
  EiemReadBoxedBool(g_smr_get_updateWhenOffscreen, partner, &partnerOffscreen);
  EiemReadBoxedBool(g_smr_get_forceMatrixRecalculationPerRender, source,
                    &sourceForceMatrix);
  EiemReadBoxedBool(g_smr_get_forceMatrixRecalculationPerRender, partner,
                    &partnerForceMatrix);
  EiemReadBoxedBool(g_smr_get_skinnedMotionVectors, source, &sourceMotion);
  EiemReadBoxedBool(g_smr_get_skinnedMotionVectors, partner, &partnerMotion);
  void *sourceSkinningRoot = g_smr_get_skinningRoot
                                 ? Invoke(g_smr_get_skinningRoot, source)
                                 : nullptr;
  void *partnerSkinningRoot = g_smr_get_skinningRoot
                                  ? Invoke(g_smr_get_skinningRoot, partner)
                                  : nullptr;
  void *sourceRootBone = g_smr_get_rootBone
                           ? Invoke(g_smr_get_rootBone, source)
                           : nullptr;
  void *partnerRootBone = g_smr_get_rootBone
                            ? Invoke(g_smr_get_rootBone, partner)
                            : nullptr;
  Log("[MOD-PARTNER-SKIN-v76] source=%p partner=%p quality=%d/%d "
      "offscreen=%d/%d forceMatrix=%d/%d motion=%d/%d "
      "rootBone=%p/%p skinningRoot=%p/%p",
      source, partner, sourceQuality, partnerQuality,
      sourceOffscreen ? 1 : 0, partnerOffscreen ? 1 : 0,
      sourceForceMatrix ? 1 : 0, partnerForceMatrix ? 1 : 0,
      sourceMotion ? 1 : 0, partnerMotion ? 1 : 0,
      sourceRootBone, partnerRootBone,
      sourceSkinningRoot, partnerSkinningRoot);
}

// Root bones are assigned by the game's Animator/skin assembly after some
// Renderers have already been observed. A Partner copies the source value at
// creation time, so a late source assignment must be replayed to every live
// Partner that belongs to that source Renderer.
static size_t EiemSyncPartnerRootBones(void *sourceRenderer = nullptr) {
  if (!g_smr_get_rootBone || !g_smr_set_rootBone) return 0;
  std::vector<std::pair<void *, void *>> partners;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners) {
    if (sourceRenderer && state.sourceRenderer != sourceRenderer) continue;
    if (state.sourceRenderer && state.partnerRenderer)
      partners.emplace_back(state.sourceRenderer, state.partnerRenderer);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  size_t changed = 0;
  for (const auto &pair : partners) {
    if (EiemNativeObjectStatus(pair.first) != 1 ||
        EiemNativeObjectStatus(pair.second) != 1)
      continue;
    void *rootBone = Invoke(g_smr_get_rootBone, pair.first);
    if (!rootBone || EiemNativeObjectStatus(rootBone) != 1) continue;
    void *current = Invoke(g_smr_get_rootBone, pair.second);
    if (current == rootBone) continue;
    void *params[] = {rootBone};
    void *result = nullptr;
    if (!InvokeChecked(g_smr_set_rootBone, pair.second, params, &result) ||
        Invoke(g_smr_get_rootBone, pair.second) != rootBone)
      continue;
    ++changed;
    Log("[MOD-PARTNER-ROOT] synchronized source=%p partner=%p rootBone=%p",
        pair.first, pair.second, rootBone);
  }
  return changed;
}

static size_t EiemManagedArrayFirstDiff(void *left, void *right) {
  if (left == right) return SIZE_MAX;
  const size_t leftCount = EiemManagedArrayLength(left);
  const size_t rightCount = EiemManagedArrayLength(right);
  const size_t compared = leftCount < rightCount ? leftCount : rightCount;
  if (!left || !right) return 0;
  __try {
    void **leftItems = (void **)((char *)left + IL2CPP_ARRAY_DATA);
    void **rightItems = (void **)((char *)right + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < compared; ++index)
      if (leftItems[index] != rightItems[index]) return index;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
  return leftCount == rightCount ? SIZE_MAX : compared;
}

// Stable only within one process. This is an evidence fingerprint for the
// order of a Renderer bone palette; it is never used as an identity or as a
// binding decision.
static uint64_t EiemManagedObjectArrayHash(void *array) {
  const size_t count = EiemManagedArrayLength(array);
  if (!array || count > 16384) return 0;
  uint64_t hash = 1469598103934665603ULL;
  hash ^= (uint64_t)count;
  hash *= 1099511628211ULL;
  void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
  for (size_t index = 0; index < count; ++index) {
    const uintptr_t value = (uintptr_t)items[index];
    for (size_t byte = 0; byte < sizeof(value); ++byte) {
      hash ^= (uint8_t)(value >> (byte * 8));
      hash *= 1099511628211ULL;
    }
  }
  return hash;
}

// Mesh.bindposes is a managed value-type array, so pointer identity is not
// useful. Hash its bytes only for the one-time partner evidence record; the
// result is never used as a binding decision.
static uint64_t EiemManagedValueArrayHash(void *array, size_t elementSize) {
  const size_t count = EiemManagedArrayLength(array);
  if (!array || !elementSize || count > 4096 || count > SIZE_MAX / elementSize)
    return 0;
  const size_t byteCount = count * elementSize;
  const unsigned char *bytes =
      (const unsigned char *)array + IL2CPP_ARRAY_DATA;
  uint64_t hash = 1469598103934665603ULL;
  hash ^= (uint64_t)count;
  hash *= 1099511628211ULL;
  __try {
    for (size_t index = 0; index < byteCount; ++index) {
      hash ^= bytes[index];
      hash *= 1099511628211ULL;
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return 0;
  }
  return hash;
}

static bool EiemTraceReadVector3Value(void *getter, void *object,
                                      Vector3 *value) {
  if (value) *value = {};
  if (!getter || !object || !value) return false;
  void *boxed = EiemBackendInvokeNoThrow(getter, object);
  if (!boxed) return false;
  __try {
    memcpy(value, (char *)boxed + 16, sizeof(Vector3));
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    *value = {};
    return false;
  }
}

// Refresh diagnostics only. A bone-array pointer/index mismatch is useful
// evidence when a Partner falls into a bind/T-pose after F10, but dumping the
// whole palette would flood the game log. Keep one safe object-name sample at
// the first divergent slot.
static void EiemTraceReadObjectNameSafe(void *object, char *out,
                                        size_t outSize) {
  if (!out || !outSize) return;
  out[0] = '\0';
  if (!object || !g_object_get_name) return;
  __try {
    void *name = EiemBackendInvokeNoThrow(g_object_get_name, object);
    if (name) ReadStrUtf8(name, out, outSize);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out[0] = '\0';
  }
}

// Registration evidence only. Partner bones are assigned once during creation,
// while later skin/Animator boundaries may replace the source palette. The
// unified trace records the relationship without changing either renderer.
static void EiemProbePartnerBoneBindings(void *sourceRenderer,
                                         const char *boundary) {
  if (!g_smr_get_bones)
    return;
  struct ProbePair {
    void *source = nullptr;
    void *partner = nullptr;
    LONG generation = -1;
    uintptr_t ownerModel = 0;
    char section[96] = {};
    char rendererType[32] = {};
    void *sourceMesh = nullptr;
    void *partnerMesh = nullptr;
    void *skeletonAnchor = nullptr;
    std::shared_ptr<EiemSkeletonInstance> skeleton;
    std::shared_ptr<const EiemSkinIdentity> skin;
    size_t skeletonNodes = 0;
    size_t skeletonAdded = 0;
    bool skinArrayObserved = false;
  };
  std::vector<ProbePair> pairs;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners) {
    if (sourceRenderer && state.sourceRenderer != sourceRenderer) continue;
    ProbePair pair = {};
    pair.source = state.sourceRenderer;
    pair.partner = state.partnerRenderer;
    pair.generation = state.generation;
    pair.ownerModel = state.ownerPrefabInstance;
    strncpy_s(pair.section, sizeof(pair.section), state.section, _TRUNCATE);
    strncpy_s(pair.rendererType, sizeof(pair.rendererType), state.rendererType,
              _TRUNCATE);
    pair.skeletonAnchor = state.skeleton ? state.skeleton->anchor.Target() : nullptr;
    pair.skeleton = state.skeleton;
    pair.skin = state.skin;
    pair.skeletonNodes = state.skeleton ? state.skeleton->nodes.size() : 0;
    pair.skeletonAdded =
        state.skeleton ? state.skeleton->createdObjects.size() : 0;
    pair.skinArrayObserved = state.skinArrayObserved;
    pairs.push_back(pair);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  for (auto &pair : pairs) {
    EiemTraceRendererOwnerCorrelation(
        pair.source, pair.ownerModel, boundary, pair.generation);
    void *sourceBones = Invoke(g_smr_get_bones, pair.source);
    void *partnerBones = Invoke(g_smr_get_bones, pair.partner);
    pair.sourceMesh = EiemReadSharedMesh(
        pair.source, pair.rendererType[0] ? pair.rendererType
                                         : "SkinnedMeshRenderer");
    pair.partnerMesh = EiemReadSharedMesh(
        pair.partner, pair.rendererType[0] ? pair.rendererType
                                           : "SkinnedMeshRenderer");
    const size_t sourceCount = EiemManagedArrayLength(sourceBones);
    const size_t partnerCount = EiemManagedArrayLength(partnerBones);
    const size_t firstDiff = EiemManagedArrayFirstDiff(sourceBones, partnerBones);
    const uint64_t sourceBoneHash = EiemManagedObjectArrayHash(sourceBones);
    const uint64_t partnerBoneHash = EiemManagedObjectArrayHash(partnerBones);
    if (pair.skin && pair.skeleton && partnerBones &&
        EiemRegistrationTraceFirst("partner-skin-map", boundary,
                                   pair.source, pair.partner, nullptr,
                                   pair.generation)) {
      std::vector<std::string> skeletonPaths;
      skeletonPaths.reserve(pair.skeleton->document.nodes.size());
      for (const auto &node : pair.skeleton->document.nodes)
        skeletonPaths.push_back(node.path);
      std::vector<size_t> skinIndices;
      std::string skinError;
      size_t privateCount = 0, unresolvedCount = 0, identityMismatch = 0;
      size_t firstPrivateSkinIndex = SIZE_MAX;
      size_t firstPrivateSkeletonIndex = SIZE_MAX;
      int firstPrivateSourceFlag = -1;
      std::string firstPrivate, firstPrivateNodePath;
      std::string firstPrivateSourceBone, firstPrivatePartnerBone;
      std::string firstUnresolved, firstMismatch;
      if (!EiemResolveSkinPathIndices(pair.skin->paths, skeletonPaths,
                                      skinIndices, skinError)) {
        unresolvedCount = pair.skin->paths.size();
        if (!pair.skin->paths.empty()) firstUnresolved = pair.skin->paths.front();
      } else {
        void **partnerItems = (void **)((char *)partnerBones + IL2CPP_ARRAY_DATA);
        const size_t count = (std::min)(skinIndices.size(),
                                        EiemManagedArrayLength(partnerBones));
        for (size_t index = 0; index < count; ++index) {
          const size_t skeletonIndex = skinIndices[index];
          if (skeletonIndex >= pair.skeleton->nodes.size()) {
            ++unresolvedCount;
            if (firstUnresolved.empty()) firstUnresolved = pair.skin->paths[index];
            continue;
          }
          char name[160] = {};
          EiemTraceReadObjectNameSafe(partnerItems[index], name, sizeof(name));
          if (strncmp(name, "EIEM_Bone_", 10) == 0) {
            ++privateCount;
            if (firstPrivate.empty()) {
              firstPrivate = pair.skin->paths[index];
              firstPrivateSkinIndex = index;
              firstPrivateSkeletonIndex = skeletonIndex;
              firstPrivateSourceFlag =
                  pair.skeleton->document.nodes[skeletonIndex].source ? 1 : 0;
              firstPrivateNodePath =
                  pair.skeleton->document.nodes[skeletonIndex].path;
              firstPrivatePartnerBone = name;
              const size_t sourceCount = EiemManagedArrayLength(sourceBones);
              if (sourceBones && index < sourceCount) {
                void **sourceItems =
                    (void **)((char *)sourceBones + IL2CPP_ARRAY_DATA);
                char sourceName[160] = {};
                EiemTraceReadObjectNameSafe(sourceItems[index], sourceName,
                                            sizeof(sourceName));
                firstPrivateSourceBone = sourceName;
              }
            }
          }
          if (partnerItems[index] != pair.skeleton->nodes[skeletonIndex].Target()) {
            ++identityMismatch;
            if (firstMismatch.empty()) firstMismatch = pair.skin->paths[index];
          }
        }
      }
      Log("%s event=partner-skin-map boundary=%s generation=%ld source=%p "
          "partner=%p skeleton=%p skeletonAnchor=%p skeletonNodes=%zu "
          "skeletonAdded=%zu pathCount=%zu private=%zu unresolved=%zu "
          "identityMismatch=%zu firstPrivate=%s firstUnresolved=%s firstMismatch=%s",
          EiemRegistrationTraceTag, boundary ? boundary : "unknown",
          pair.generation, pair.source, pair.partner, pair.skeleton.get(),
          pair.skeletonAnchor, pair.skeletonNodes, pair.skeletonAdded,
          pair.skin->paths.size(), privateCount, unresolvedCount,
          identityMismatch,
          firstPrivate.empty() ? "<none>" : firstPrivate.c_str(),
          firstUnresolved.empty() ? "<none>" : firstUnresolved.c_str(),
          firstMismatch.empty() ? "<none>" : firstMismatch.c_str());
      if (firstPrivateSkeletonIndex != SIZE_MAX)
        Log("%s event=partner-skin-private-detail boundary=%s generation=%ld "
            "source=%p partner=%p skinIndex=%zu skeletonIndex=%zu "
            "sourceFlag=%d skinPath=%s skeletonPath=%s sourceBone=%s "
            "partnerBone=%s",
            EiemRegistrationTraceTag, boundary ? boundary : "unknown",
            pair.generation, pair.source, pair.partner,
            firstPrivateSkinIndex, firstPrivateSkeletonIndex,
            firstPrivateSourceFlag,
            firstPrivate.empty() ? "<none>" : firstPrivate.c_str(),
            firstPrivateNodePath.empty() ? "<none>"
                                         : firstPrivateNodePath.c_str(),
            firstPrivateSourceBone.empty() ? "<none>"
                                           : firstPrivateSourceBone.c_str(),
            firstPrivatePartnerBone.empty() ? "<none>"
                                            : firstPrivatePartnerBone.c_str());
    }
    void *sourceBindposes =
        pair.sourceMesh && g_mesh_get_bindposes
            ? EiemBackendInvokeNoThrow(g_mesh_get_bindposes, pair.sourceMesh)
            : nullptr;
    void *partnerBindposes =
        pair.partnerMesh && g_mesh_get_bindposes
            ? EiemBackendInvokeNoThrow(g_mesh_get_bindposes, pair.partnerMesh)
            : nullptr;
    const size_t sourceBindposeCount = EiemManagedArrayLength(sourceBindposes);
    const size_t partnerBindposeCount = EiemManagedArrayLength(partnerBindposes);
    const uint64_t sourceBindposeHash =
        EiemManagedValueArrayHash(sourceBindposes, sizeof(Matrix4x4));
    const uint64_t partnerBindposeHash =
        EiemManagedValueArrayHash(partnerBindposes, sizeof(Matrix4x4));
    if (firstDiff != SIZE_MAX &&
        (boundary && (strstr(boundary, "refresh") ||
                      strcmp(boundary, "partner-created") == 0))) {
      void *sourceBone = nullptr;
      void *partnerBone = nullptr;
      const size_t sourceCountForDiff = EiemManagedArrayLength(sourceBones);
      const size_t partnerCountForDiff = EiemManagedArrayLength(partnerBones);
      if (sourceBones && firstDiff < sourceCountForDiff)
        sourceBone = ((void **)((char *)sourceBones + IL2CPP_ARRAY_DATA))[firstDiff];
      if (partnerBones && firstDiff < partnerCountForDiff)
        partnerBone = ((void **)((char *)partnerBones + IL2CPP_ARRAY_DATA))[firstDiff];
      char sourceName[160] = {}, partnerName[160] = {};
      EiemTraceReadObjectNameSafe(sourceBone, sourceName, sizeof(sourceName));
      EiemTraceReadObjectNameSafe(partnerBone, partnerName, sizeof(partnerName));
      Vector3 sourceBonePosition = {}, partnerBonePosition = {};
      const bool sourceBonePositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, sourceBone, &sourceBonePosition);
      const bool partnerBonePositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, partnerBone, &partnerBonePosition);
      Log("%s event=partner-bone-diff boundary=%s generation=%ld source=%p "
          "partner=%p index=%zu sourceBone=%p sourceName=%s sourcePosRead=%d "
          "sourcePos=%.6g,%.6g,%.6g partnerBone=%p partnerName=%s "
          "partnerPosRead=%d partnerPos=%.6g,%.6g,%.6g",
          EiemRegistrationTraceTag, boundary ? boundary : "unknown",
          InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
          pair.source, pair.partner, firstDiff, sourceBone,
          sourceName[0] ? sourceName : "<unnamed>",
          sourceBonePositionRead ? 1 : 0, sourceBonePosition.x,
          sourceBonePosition.y, sourceBonePosition.z, partnerBone,
          partnerName[0] ? partnerName : "<unnamed>",
          partnerBonePositionRead ? 1 : 0, partnerBonePosition.x,
          partnerBonePosition.y, partnerBonePosition.z);
    }
    void *sourceTransform = g_component_get_transform
                                ? Invoke(g_component_get_transform, pair.source)
                                : nullptr;
    void *partnerTransform = g_component_get_transform
                                 ? Invoke(g_component_get_transform, pair.partner)
                                 : nullptr;
    void *sourceParent = sourceTransform && g_transform_get_parent
                             ? Invoke(g_transform_get_parent, sourceTransform)
                             : nullptr;
    void *partnerParent = partnerTransform && g_transform_get_parent
                              ? Invoke(g_transform_get_parent, partnerTransform)
                              : nullptr;
    void *sourceRootBone = g_smr_get_rootBone
                               ? Invoke(g_smr_get_rootBone, pair.source)
                               : nullptr;
    void *partnerRootBone = g_smr_get_rootBone
                                ? Invoke(g_smr_get_rootBone, pair.partner)
                                : nullptr;
    Vector3 sourcePosition = {}, partnerPosition = {};
    Vector3 sourceLocalPosition = {}, partnerLocalPosition = {};
    Vector3 sourceRootPosition = {}, partnerRootPosition = {};
    const bool sourcePositionRead = EiemTraceReadVector3Value(
        g_transform_get_position, sourceTransform, &sourcePosition);
    const bool partnerPositionRead = EiemTraceReadVector3Value(
        g_transform_get_position, partnerTransform, &partnerPosition);
    const bool sourceLocalPositionRead = EiemTraceReadVector3Value(
        g_transform_get_localPosition, sourceTransform, &sourceLocalPosition);
    const bool partnerLocalPositionRead = EiemTraceReadVector3Value(
        g_transform_get_localPosition, partnerTransform, &partnerLocalPosition);
    const bool sourceRootPositionRead = EiemTraceReadVector3Value(
        g_transform_get_position, sourceRootBone, &sourceRootPosition);
    const bool partnerRootPositionRead = EiemTraceReadVector3Value(
        g_transform_get_position, partnerRootBone, &partnerRootPosition);
    // This is the single bounded record that connects the public Renderer
    // assignment to the game's own skin assembly.  A direct RendererInfo
    // path normally reports gameArray=0; the PostModel path can report 1
    // after EiemRegisterPartnersInSkinArrays.  It is intentionally emitted
    // only at creation/refresh boundaries, never from a frame callback.
    if (boundary &&
        (strcmp(boundary, "partner-created") == 0 ||
         strstr(boundary, "refresh")) &&
        EiemRegistrationTraceFirst("partner-commit-v102", boundary,
                                   pair.source, pair.partner, nullptr,
                                   pair.generation)) {
      bool sourceEnabled = true, partnerEnabled = true;
      EiemReadRendererEnabled(pair.source, &sourceEnabled);
      EiemReadRendererEnabled(pair.partner, &partnerEnabled);
      Log("[PARTNER-COMMIT-v102] boundary=%s generation=%ld source=%p "
          "partner=%p section=%s gameArray=%d sourceBones=%zu "
          "partnerBones=%zu firstDiff=%lld sourceMesh=%p partnerMesh=%p "
          "sourceRoot=%p partnerRoot=%p sourceEnabled=%d partnerEnabled=%d",
          boundary, pair.generation, pair.source, pair.partner,
          pair.section[0] ? pair.section : "<unknown>",
          pair.skinArrayObserved ? 1 : 0, sourceCount, partnerCount,
          firstDiff == SIZE_MAX ? -1LL : (long long)firstDiff,
          pair.sourceMesh, pair.partnerMesh, sourceRootBone, partnerRootBone,
          sourceEnabled ? 1 : 0, partnerEnabled ? 1 : 0);
    }
    EiemRegistrationTracePartner(
        boundary, pair.source, pair.partner,
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
        pair.section, sourceCount, partnerCount,
        firstDiff == SIZE_MAX ? -1LL : (long long)firstDiff,
        sourceParent, partnerParent, sourceRootBone, partnerRootBone,
        pair.sourceMesh, pair.partnerMesh, pair.skeletonAnchor,
        pair.skeletonNodes, pair.skeletonAdded, sourceBoneHash,
        partnerBoneHash, sourceBindposeCount, partnerBindposeCount,
        sourceBindposeHash, partnerBindposeHash);
    EiemRegistrationTracePartnerPose(
        boundary, pair.source, pair.partner,
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
        sourceTransform, partnerTransform, sourceRootBone, partnerRootBone,
        sourcePositionRead, sourcePosition.x, sourcePosition.y,
        sourcePosition.z, partnerPositionRead, partnerPosition.x,
        partnerPosition.y, partnerPosition.z, sourceLocalPositionRead,
        sourceLocalPosition.x, sourceLocalPosition.y, sourceLocalPosition.z,
        partnerLocalPositionRead, partnerLocalPosition.x,
        partnerLocalPosition.y, partnerLocalPosition.z, sourceRootPositionRead,
        sourceRootPosition.x, sourceRootPosition.y, sourceRootPosition.z,
        partnerRootPositionRead, partnerRootPosition.x,
        partnerRootPosition.y, partnerRootPosition.z);
  }
}

// A Mesh identity can be observed while Unity is preparing another LOD.  The
// resource setter is still called for those renderers, but they are not a
// current match and must not create a partner.  A source with handling=skip is
// the one exception: its own Renderer is intentionally disabled by EIEM after
// the rule has matched, so key-state replay must still be allowed to revisit
// that already-owned source.
static bool EiemRendererEligibleForRule(void *renderer, void *drawRenderer,
                                        bool includeGameHidden) {
  if (!drawRenderer) return false;

  bool active = true;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  const bool activeRead =
      g_component_get_gameObject && g_gameObject_get_activeInHierarchy &&
      EiemReadBoxedBool(
           g_gameObject_get_activeInHierarchy,
           Invoke(g_component_get_gameObject, drawRenderer), &active);
  if (activeRead && !active) {
    if (includeGameHidden) {
      EiemRegistrationTraceEligibility(
          renderer, drawRenderer, generation, active, true, false, false,
          false, true, "inactive-authored-lod");
      return true;
    }
    EiemRegistrationTraceEligibility(
        renderer, drawRenderer, generation, active, true, false, false, false,
        false, "inactive");
    return false;
  }

  bool enabled = true;
  const bool enabledRead = EiemReadRendererEnabled(drawRenderer, &enabled);
  bool forceRenderingOff = false;
  const bool forceRenderingOffRead =
      EiemReadRendererForceRenderingOff(drawRenderer, &forceRenderingOff);

  // Unity's LODGroup may leave Renderer.enabled true and set only
  // forceRenderingOff while preparing or selecting a different LOD. Such a
  // Renderer is never a current source hit and must not acquire a Mesh rule or
  // create a partner. This state is owned by the game, so the old
  // handling=skip exception for an EIEM-disabled source cannot override it.
  if (forceRenderingOffRead && forceRenderingOff) {
    if (includeGameHidden) {
      EiemRegistrationTraceEligibility(
          renderer, drawRenderer, generation, active, enabled, enabledRead,
          forceRenderingOff, forceRenderingOffRead, true,
          "force-off-authored-lod");
      return true;
    }
    EiemRegistrationTraceEligibility(
        renderer, drawRenderer, generation, active, enabled, enabledRead,
        forceRenderingOff, forceRenderingOffRead, false, "force-off");
    return false;
  }
  if (!enabledRead || enabled) {
    EiemRegistrationTraceEligibility(
        renderer, drawRenderer, generation, active, enabled, enabledRead,
        forceRenderingOff, forceRenderingOffRead, true, "enabled-or-unread");
    return true;
  }

  if (includeGameHidden) {
    EiemRegistrationTraceEligibility(
        renderer, drawRenderer, generation, active, enabled, enabledRead,
        forceRenderingOff, forceRenderingOffRead, true,
        "disabled-authored-lod");
    return true;
  }

  bool ownedSkip = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    const EiemRenderOverrideState &state = s_eiemOverrides[index];
    ownedSkip = !state.restorePending && state.hasEnabled;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (ownedSkip) {
    EiemRegistrationTraceEligibility(
        renderer, drawRenderer, generation, active, enabled, enabledRead,
        forceRenderingOff, forceRenderingOffRead, true, "owned-disabled");
    return true;
  }

  EiemRegistrationTraceEligibility(
      renderer, drawRenderer, generation, active, enabled, enabledRead,
      forceRenderingOff, forceRenderingOffRead, false, "disabled-unowned");
  return false;
}

static size_t EiemReconcilePartnerLodMemberships(void *sourceRenderer = nullptr) {
  std::vector<std::pair<void *, void *>> partners;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners) {
    if (sourceRenderer && state.sourceRenderer != sourceRenderer) continue;
    if (state.sourceDrawRenderer && state.partnerRenderer)
      partners.emplace_back(state.sourceDrawRenderer, state.partnerRenderer);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  size_t changed = 0;
  for (const auto &pair : partners) {
    if (EiemNativeObjectStatus(pair.first) != 1 ||
        EiemNativeObjectStatus(pair.second) != 1)
      continue;
    if (EiemSetPartnerLodMembership(pair.first, pair.second, true)) ++changed;
  }
  return changed;
}

static size_t EiemSyncPartnerRootBonesFromArray(void *renderers) {
  const size_t count = EiemManagedArrayLength(renderers);
  if (!renderers || count > 8192) {
    EiemReconcilePartnerLodMemberships();
    return 0;
  }
  void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  size_t changed = 0;
  for (size_t index = 0; index < count; ++index)
    changed += EiemSyncPartnerRootBones(items[index]);
  EiemReconcilePartnerLodMemberships();
  return changed;
}

static void EiemRetirePartner(const EiemPartnerState &state) {
  // Capture the last live pose before disabling/detaching the Partner. This
  // is diagnostic only and helps distinguish a bad skin pose from a pose that
  // changes during the teardown boundary.
  if (state.sourceRenderer && state.partnerRenderer &&
      g_component_get_transform) {
    __try {
      void *sourceTransform = Invoke(g_component_get_transform,
                                     state.sourceRenderer);
      void *partnerTransform = Invoke(g_component_get_transform,
                                      state.partnerRenderer);
      void *sourceRoot = g_smr_get_rootBone
                             ? Invoke(g_smr_get_rootBone, state.sourceRenderer)
                             : nullptr;
      void *partnerRoot = g_smr_get_rootBone
                              ? Invoke(g_smr_get_rootBone, state.partnerRenderer)
                              : nullptr;
      Vector3 sourcePosition = {}, partnerPosition = {};
      Vector3 sourceLocalPosition = {}, partnerLocalPosition = {};
      Vector3 sourceRootPosition = {}, partnerRootPosition = {};
      const bool sourcePositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, sourceTransform, &sourcePosition);
      const bool partnerPositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, partnerTransform, &partnerPosition);
      const bool sourceLocalPositionRead = EiemTraceReadVector3Value(
          g_transform_get_localPosition, sourceTransform, &sourceLocalPosition);
      const bool partnerLocalPositionRead = EiemTraceReadVector3Value(
          g_transform_get_localPosition, partnerTransform, &partnerLocalPosition);
      const bool sourceRootPositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, sourceRoot, &sourceRootPosition);
      const bool partnerRootPositionRead = EiemTraceReadVector3Value(
          g_transform_get_position, partnerRoot, &partnerRootPosition);
      EiemRegistrationTracePartnerPose(
          "retire", state.sourceRenderer, state.partnerRenderer,
          InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
          sourceTransform, partnerTransform, sourceRoot, partnerRoot,
          sourcePositionRead, sourcePosition.x, sourcePosition.y,
          sourcePosition.z, partnerPositionRead, partnerPosition.x,
          partnerPosition.y, partnerPosition.z, sourceLocalPositionRead,
          sourceLocalPosition.x, sourceLocalPosition.y, sourceLocalPosition.z,
          partnerLocalPositionRead, partnerLocalPosition.x,
          partnerLocalPosition.y, partnerLocalPosition.z, sourceRootPositionRead,
          sourceRootPosition.x, sourceRootPosition.y, sourceRootPosition.z,
          partnerRootPositionRead, partnerRootPosition.x,
          partnerRootPosition.y, partnerRootPosition.z);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      Log("[INSTANCE-REG-v3] event=partner-pose-failed boundary=retire "
          "source=%p partner=%p exception=0x%08lX",
          state.sourceRenderer, state.partnerRenderer, GetExceptionCode());
    }
  }
  EiemRetireShapeBinding(state.shapes.binding);
  // Object.Destroy is deferred until Unity's end-of-frame processing. Make
  // the whole partner inactive first so no LOD/controller callback can make a
  // detached, already-retired SkinnedMeshRenderer visible at the scene root.
  if (state.partnerObject && g_gameObject_set_active) {
    bool active = false;
    void *params[] = {&active};
    Invoke(g_gameObject_set_active, state.partnerObject, params);
  }
  if (state.partnerRenderer) {
    EiemSetRendererEnabled(state.partnerRenderer, false);
    EiemSetPartnerLodMembership(state.sourceDrawRenderer, state.partnerRenderer, false);
  }
  // Destroy is deferred. Detach now so this same reconcile's hierarchy walk
  // cannot treat a retired partner as an original game renderer.
  if (state.partnerObject && g_gameObject_get_transform && g_transform_set_parent) {
    void *transform = Invoke(g_gameObject_get_transform, state.partnerObject);
    if (transform) {
      bool worldPositionStays = false;
      void *params[] = {nullptr, &worldPositionStays};
      Invoke(g_transform_set_parent, transform, params);
    }
  }
  if (state.partnerObject && g_object_destroy) {
    void *params[] = {state.partnerObject};
    Invoke(g_object_destroy, nullptr, params);
  }
}

static void EiemDestroyPartnerObjects(const std::vector<std::string> *affected = nullptr) {
  std::vector<EiemPartnerState> states;
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (size_t i = 0; i < s_eiemPartners.size();) {
    if (!EiemModAffected(s_eiemPartners[i].modPath, affected)) { ++i; continue; }
    states.push_back(s_eiemPartners[i]);
    s_eiemPartners.erase(s_eiemPartners.begin() + i);
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  for (const auto &state : states) EiemRetirePartner(state);
  if (!states.empty())
    Log("[MOD] Destroyed %zu partner Renderer(s) before reload", states.size());
}

static bool EiemPartnerDesired(const EiemPartnerState &state,
                               const EiemModProgram &program) {
  for (const auto &rule : program.rules) {
    if (!EiemModEquals(rule.modPath, state.modPath) ||
        _stricmp(rule.section, state.sourceSection) != 0)
      continue;
    for (uint32_t index = 0; index < rule.partnerCount; ++index)
      if (_stricmp(rule.partners[index], state.section) == 0) return true;
    return false;
  }
  return false;
}

struct EiemPartnerVisibilityChange {
  void *sourceDrawRenderer = nullptr;
  void *partnerRenderer = nullptr;
  bool visible = false;
  bool enabledWhenVisible = true;
};

static void EiemSetPartnerDrawVisibility(
    const EiemPartnerVisibilityChange &change) {
  if (!change.sourceDrawRenderer || !change.partnerRenderer) return;
  // Membership follows the source Renderer for the lifetime of this Partner;
  // key controls only its draw state. Removing it on every hide loses the
  // game's LOD relationship and re-adding later depends on a hierarchy lookup
  // that is not valid for all model presentation paths.
  EiemSetPartnerLodMembership(change.sourceDrawRenderer,
                              change.partnerRenderer, true);
  EiemSetRendererEnabled(change.partnerRenderer,
                         change.visible && change.enabledWhenVisible);
}

// A key transition is a draw-state transaction only.  The Partner was
// attached to the source LOD during model assembly; touching LOD arrays here
// would race the game's own LOD rebuild and make a visibility change look like
// a structural re-registration.  Keep this path limited to Renderer.enabled.
static void EiemSetPartnerRendererVisibilityOnly(
    const EiemPartnerVisibilityChange &change) {
  if (!change.partnerRenderer) return;
  EiemSetRendererEnabled(change.partnerRenderer,
                         change.visible && change.enabledWhenVisible);
}

static void EiemApplyPartnerControlVisibility(
    const std::vector<std::string> *affected,
    const EiemModProgram &program) {
  std::vector<EiemPartnerVisibilityChange> changes;
  size_t visible = 0, hidden = 0, unchanged = 0;
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (auto &state : s_eiemPartners) {
    if (!EiemModAffected(state.modPath, affected)) {
      continue;
    }
    const bool desired = EiemPartnerDesired(state, program);
    if (state.controlVisible == desired) ++unchanged;
    else if (desired) ++visible;
    else ++hidden;
    state.controlVisible = desired;
    changes.push_back({state.sourceDrawRenderer, state.partnerRenderer,
                       desired, state.enabledWhenVisible});
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  for (const auto &change : changes)
    EiemSetPartnerRendererVisibilityOnly(change);
  Log("[MOD] Partner control visibility: shown=%zu hidden=%zu unchanged=%zu",
      visible, hidden, unchanged);
}

static void EiemDestroyPartnerObjects(uintptr_t ownerPrefabInstance) {
  if (!ownerPrefabInstance) return;
  std::vector<EiemPartnerState> states;
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (size_t index = 0; index < s_eiemPartners.size();) {
    if (s_eiemPartners[index].ownerPrefabInstance != ownerPrefabInstance) {
      ++index;
      continue;
    }
    states.push_back(s_eiemPartners[index]);
    s_eiemPartners.erase(s_eiemPartners.begin() + index);
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  for (const auto &state : states) EiemRetirePartner(state);
}

static void *EiemCreatePartnerRenderer(void *sourceMeshOwner,
                                       void *sourceDrawRenderer,
                                       const char *rendererType,
                                       const char *sourceSection,
                                       const EiemModRule &partnerRule,
                                       void *sourceMesh, bool controlVisible,
                                       char *error,
                                       size_t errorSize) {
  if (!sourceMeshOwner || !sourceDrawRenderer || !rendererType ||
      !g_gameObjectClass ||
      (!g_gameObject_ctor && !g_gameObject_ctorDefault) ||
      !g_gameObject_AddComponent || !g_component_get_gameObject ||
      !g_component_get_transform || !g_transform_set_parent ||
      !g_gameObject_get_activeSelf || !g_gameObject_set_active ||
      !il2cpp_class_get_type || !il2cpp_type_get_object) {
    if (error) strncpy_s(error, errorSize,
                         "Unity GameObject/Transform creation APIs are unavailable",
                         _TRUNCATE);
    return nullptr;
  }
  void *sourceGo = Invoke(g_component_get_gameObject, sourceDrawRenderer);
  if (!sourceGo) {
    if (error) strncpy_s(error, errorSize, "Source Renderer has no GameObject", _TRUNCATE);
    return nullptr;
  }
  void *sourceTransform = Invoke(g_component_get_transform,
                                 sourceDrawRenderer);
  if (!sourceTransform) {
    if (error) strncpy_s(error, errorSize, "Source Renderer has no Transform", _TRUNCATE);
    return nullptr;
  }
  void *partnerGo = il2cpp_object_new(g_gameObjectClass);
  if (!partnerGo) {
    if (error) strncpy_s(error, errorSize, "Unable to allocate partner GameObject", _TRUNCATE);
    return nullptr;
  }
  void *partnerMeshOwner = nullptr;
  void *partnerDrawRenderer = nullptr;
  EiemShapeState partnerShapes;
  auto cleanupPartner = [&]() {
    EiemRetireShapeBinding(partnerShapes.binding);
    if (partnerGo && g_object_destroy) {
      void *params[] = {partnerGo};
      Invoke(g_object_destroy, nullptr, params);
    }
  };
  char name[192] = {};
  snprintf(name, sizeof(name), "EIEM Partner %s", partnerRule.section);
  if (g_gameObject_ctor) {
    void *nameString = il2cpp_string_new(name);
    void *ctorParams[] = {nameString};
    Invoke(g_gameObject_ctor, partnerGo, ctorParams);
  } else {
    Invoke(g_gameObject_ctorDefault, partnerGo);
    if (g_gameObject_set_name) {
      void *nameString = il2cpp_string_new(name);
      void *nameParams[] = {nameString};
      Invoke(g_gameObject_set_name, partnerGo, nameParams);
    }
  }
  if (g_gameObject_get_layer && g_gameObject_set_layer) {
    void *boxedLayer = Invoke(g_gameObject_get_layer, sourceGo);
    if (boxedLayer) {
      int32_t layer = *(int32_t *)((char *)boxedLayer + 16);
      void *layerParams[] = {&layer};
      Invoke(g_gameObject_set_layer, partnerGo, layerParams);
    }
  }
  // AddComponent on an active GameObject invokes the renderer's enable path
  // before its Mesh, bones, bind poses, rootBone and skinningRoot exist. The
  // game's skin registry does not necessarily rebuild that first empty
  // registration when those fields are assigned later. Construct the entire
  // partner offline, then expose it once with a complete skin binding.
  void *boxedActive = Invoke(g_gameObject_get_activeSelf, sourceGo);
  if (!boxedActive) {
    if (error) strncpy_s(error, errorSize,
                         "Cannot read source GameObject activation state",
                         _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  bool sourceActive = *(bool *)((char *)boxedActive + 16);
  bool inactive = false;
  void *inactiveParams[] = {&inactive};
  void *activationResult = nullptr;
  if (!InvokeChecked(g_gameObject_set_active, partnerGo, inactiveParams,
                     &activationResult)) {
    if (error) strncpy_s(error, errorSize,
                         "Cannot deactivate partner during construction",
                         _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  void *partnerTransform = g_gameObject_get_transform
                               ? Invoke(g_gameObject_get_transform, partnerGo)
                               : nullptr;
  if (!partnerTransform) {
    if (error) strncpy_s(error, errorSize, "Partner GameObject has no Transform", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  // A newly constructed GameObject has identity local TRS. Parenting it under
  // the source with worldPositionStays=false gives the exact source transform,
  // including later animation/nonuniform scale, without a per-frame copier.
  bool worldPositionStays = false;
  void *parentParams[] = {sourceTransform, &worldPositionStays};
  Invoke(g_transform_set_parent, partnerTransform, parentParams);

  auto addComponent = [&](void *klass) -> void * {
    void *type = klass ? il2cpp_class_get_type(klass) : nullptr;
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!typeObject) return nullptr;
    void *componentParams[] = {typeObject};
    return Invoke(g_gameObject_AddComponent, partnerGo, componentParams);
  };
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    partnerMeshOwner = addComponent(g_skinnedMeshRendererClass);
    partnerDrawRenderer = partnerMeshOwner;
  } else {
    partnerMeshOwner = addComponent(g_meshFilterClass);
    partnerDrawRenderer = addComponent(g_meshRendererClass);
  }
  if (!partnerMeshOwner || !partnerDrawRenderer) {
    if (error)
      strncpy_s(error, errorSize,
                "Partner Mesh owner/Renderer components are unavailable",
                _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }

  void *assignedMesh = sourceMesh;
  std::shared_ptr<EiemSkeletonInstance> partnerSkeleton;
  if (partnerRule.hasSkeleton &&
      (!EiemModEquals(rendererType,"SkinnedMeshRenderer") ||
       !EiemAcquireSkeleton(partnerRule,sourceMeshOwner,partnerSkeleton,error,errorSize))) {
    cleanupPartner(); return nullptr;
  }
  if (partnerSkeleton && !EiemWatchSkeletonPartner(*partnerSkeleton,partnerMeshOwner)) {
    if (error) strncpy_s(error,errorSize,"Cannot retain Skeleton partner lifetime",_TRUNCATE);
    cleanupPartner(); return nullptr;
  }
  std::shared_ptr<const EiemSkinIdentity> partnerSkin;
  char buildError[256] = {};
  if (partnerRule.hasMesh &&
      !EiemBuildMeshResource(partnerRule, &assignedMesh, buildError,
                             sizeof(buildError), sourceMesh, &partnerSkin)) {
    if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  void *partnerBones = nullptr;
  EiemUnityRef partnerBonesRoot;
  if (partnerSkin && !EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    if (error) strncpy_s(error, errorSize, "Skinned partner mesh requires a source skeleton", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  if (partnerSkin && !(partnerSkeleton
      ? EiemSkeletonMeshBones(*partnerSkin,*partnerSkeleton,&partnerBones,buildError,sizeof(buildError))
      : EiemResolveMeshBones(*partnerSkin, sourceMeshOwner, &partnerBones, buildError, sizeof(buildError)))) {
    if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  if (partnerBones) {
    partnerBonesRoot=EiemUnityRef::Capture(partnerBones,false);
    if (!partnerBonesRoot) { cleanupPartner(); return nullptr; }
  }
  if(EiemModEquals(rendererType,"SkinnedMeshRenderer")) {
    std::vector<EiemShapeBaseline> baseline;bool captured=false;
    AcquireSRWLockShared(&s_eiemOverrideLock);
    size_t index=EiemFindOverrideLocked(sourceMeshOwner);
    if(index!=SIZE_MAX && s_eiemOverrides[index].hasSourceShapeWeights) {
      const auto &original=s_eiemOverrides[index];baseline=EiemShapeSourceWeights(original.shapes,original.sourceShapeWeights);captured=true;
    }
    ReleaseSRWLockShared(&s_eiemOverrideLock);
    EiemUnityShapes backend;std::string reason;
    if(!captured && EiemReadSharedMesh(sourceMeshOwner,rendererType)==sourceMesh)
      captured=backend.Snapshot(sourceMeshOwner,sourceMesh,baseline);
    if(!captured || !EiemPrepareShapeBinding(partnerShapes,partnerMeshOwner,assignedMesh,baseline,backend,reason)) {
      if(error)strncpy_s(error,errorSize,reason.empty()?"Partner source shape baseline unavailable":reason.c_str(),_TRUNCATE);
      cleanupPartner();return nullptr;
    }
  }
  if (assignedMesh && !EiemSetSharedMesh(partnerMeshOwner, assignedMesh,
                                          rendererType, nullptr)) {
    if (error) strncpy_s(error, errorSize, "Partner mesh assignment failed", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    EiemUnityShapes shapeBackend;std::string shapeError;
    if(!EiemInitializeBoundShapes(partnerShapes,partnerMeshOwner,shapeBackend,shapeError)) {
      if(error)strncpy_s(error,errorSize,shapeError.c_str(),_TRUNCATE);cleanupPartner();return nullptr;
    }
    if (g_smr_get_bones && g_smr_set_bones) {
      void *bones = partnerBones ? partnerBones : Invoke(g_smr_get_bones, sourceMeshOwner);
      if (bones) {
        void *params[] = {bones};
        void *result = nullptr;
        if (!InvokeChecked(g_smr_set_bones, partnerMeshOwner, params, &result) ||
            !EiemManagedObjectArraySame(bones, Invoke(g_smr_get_bones, partnerMeshOwner))) {
          if (error) strncpy_s(error, errorSize, "Partner bone palette assignment failed", _TRUNCATE);
          cleanupPartner();
          return nullptr;
        }
      }
    } else if (partnerSkin) {
      if (error) strncpy_s(error, errorSize, "Partner skin binding APIs are unavailable", _TRUNCATE);
      cleanupPartner();
      return nullptr;
    }
    if (g_smr_get_rootBone && g_smr_set_rootBone) {
      void *rootBone = Invoke(g_smr_get_rootBone, sourceMeshOwner);
      if (rootBone) { void *params[] = {rootBone}; Invoke(g_smr_set_rootBone, partnerMeshOwner, params); }
    }
    EiemCopySkinnedRendererState(sourceMeshOwner, partnerMeshOwner);
    EiemBounds sourceBounds = {};
    const bool hasSourceBounds = EiemReadBounds(
        g_smr_get_localBounds, sourceMeshOwner, &sourceBounds);
    EiemSetReplacementDrawBounds(
        partnerMeshOwner, rendererType, assignedMesh,
        hasSourceBounds ? &sourceBounds : nullptr);
  }
  if (partnerRule.materialCount || partnerRule.submeshCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(partnerRule, sourceDrawRenderer,
                                             &materials, buildError,
                                             sizeof(buildError)) ||
        !materials ||
        !EiemAssignRendererMaterials(partnerDrawRenderer, materials,
                                     buildError,
                                     sizeof(buildError))) {
      if (error) strncpy_s(error, errorSize, buildError[0] ? buildError : "Partner materials failed", _TRUNCATE);
      cleanupPartner();
      return nullptr;
    }
  } else if (g_renderer_get_sharedMaterials && s_eiemRendererSetSharedMaterials) {
    void *materials = Invoke(g_renderer_get_sharedMaterials,
                             sourceDrawRenderer);
    if (materials &&
        !EiemAssignRendererMaterials(partnerDrawRenderer, materials,
                                     buildError,
                                     sizeof(buildError))) {
      if (error)
        strncpy_s(error, errorSize,
                  buildError[0] ? buildError
                                : "Partner source materials failed",
                  _TRUNCATE);
      cleanupPartner();
      return nullptr;
    }
  }
  const bool enabledWhenVisible =
      !EiemModEquals(partnerRule.handling, "skip");
  bool enabled = controlVisible && enabledWhenVisible;
  if (g_renderer_set_enabled) {
    void *params[] = {&enabled};
    Invoke(g_renderer_set_enabled, partnerDrawRenderer, params);
  }
  // Publish the structural LOD relationship even when the key starts this
  // Partner hidden.  Visibility is carried by Renderer.enabled above.
  EiemSetPartnerLodMembership(sourceDrawRenderer, partnerDrawRenderer, true);
  void *activateParams[] = {&sourceActive};
  activationResult = nullptr;
  if (!InvokeChecked(g_gameObject_set_active, partnerGo, activateParams,
                     &activationResult)) {
    EiemSetPartnerLodMembership(sourceDrawRenderer, partnerDrawRenderer,
                                false);
    if (error) strncpy_s(error, errorSize,
                         "Cannot activate configured partner Renderer",
                         _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }

  bool sourceEnabled = true;
  bool partnerEnabled = true;
  EiemReadRendererEnabled(sourceDrawRenderer, &sourceEnabled);
  EiemReadRendererEnabled(partnerDrawRenderer, &partnerEnabled);
  int sourceMaterials = -1;
  int partnerMaterials = -1;
  if (g_renderer_get_sharedMaterials) {
    void *sourceArray = Invoke(g_renderer_get_sharedMaterials,
                               sourceDrawRenderer);
    void *partnerArray = Invoke(g_renderer_get_sharedMaterials,
                                partnerDrawRenderer);
    if (sourceArray) sourceMaterials = *(int *)((char *)sourceArray + 24);
    if (partnerArray) partnerMaterials = *(int *)((char *)partnerArray + 24);
  }
  Log("[DEBUG-partner] source=%p enabled=%d mesh=%p materials=%d partner=%p "
      "enabled=%d mesh=%p materials=%d",
      sourceMeshOwner, sourceEnabled ? 1 : 0,
      EiemReadSharedMesh(sourceMeshOwner, rendererType), sourceMaterials,
      partnerDrawRenderer, partnerEnabled ? 1 : 0,
      EiemReadSharedMesh(partnerMeshOwner, rendererType), partnerMaterials);

  // Measure both sides of the handover at the moment the Partner is complete.
  // The source Renderer is the control: its skinning is the game's own, so a
  // Partner whose skinned extent disagrees with this one is the defect.
  //
  // No TraceTakeBudget here: it is a stub returning constant false, and pairing
  // it with && made MSVC delete this block as dead code.
  if (kEiemEnableSkinDiagnostics &&
      EiemModEquals(rendererType, "SkinnedMeshRenderer") &&
      InterlockedIncrement(&s_traceSkinProbeCount) <= 200) {
    const EiemSkinProbe::Result sourceSide =
        EiemSkinProbe::Measure(sourceMeshOwner);
    EiemSkinProbe::LogResult("[SKIN-PROBE] phase=create source=1", sourceSide);
    const EiemSkinProbe::Result partnerSide =
        EiemSkinProbe::Measure(partnerDrawRenderer);
    EiemSkinProbe::LogResult("[SKIN-PROBE] phase=create partner=1", partnerSide);
    // Ask for one sweep of the settled state. Only the first creation arms it,
    // so the measurement lands after the whole model is assembled.
  }

  size_t expectedBoneCount = 0;
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    expectedBoneCount = EiemManagedArrayLength(partnerBones);
    if (!expectedBoneCount && g_smr_get_bones)
      expectedBoneCount = EiemManagedArrayLength(
          Invoke(g_smr_get_bones, partnerMeshOwner));
  }
  EiemPartnerState state = {};
  state.skeleton=std::move(partnerSkeleton);
  state.skin=std::move(partnerSkin);
  state.sourceRenderer = sourceMeshOwner;
  state.sourceDrawRenderer = sourceDrawRenderer;
  state.partnerObject = partnerGo;
  state.partnerRenderer = partnerDrawRenderer;
  state.expectedBoneCount = expectedBoneCount;
  state.shapes=std::move(partnerShapes);
  strncpy_s(state.rendererType, sizeof(state.rendererType), rendererType, _TRUNCATE);
  EiemUpdateRendererShapes(partnerDrawRenderer, rendererType, partnerRule, state.shapes);
  state.generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  state.ownerPrefabInstance = s_eiemActivePrefabInstance;
  state.controlVisible = controlVisible;
  state.enabledWhenVisible = enabledWhenVisible;
  strncpy_s(state.sourceSection, sizeof(state.sourceSection),
            sourceSection ? sourceSection : "", _TRUNCATE);
  strncpy_s(state.section, sizeof(state.section), partnerRule.section, _TRUNCATE);
  strncpy_s(state.modPath, sizeof(state.modPath), partnerRule.modPath, _TRUNCATE);
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  s_eiemPartners.push_back(state);
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  Log("[MOD] partner Renderer created: source=%p section=%s renderer=%p mesh=%s",
      sourceMeshOwner, partnerRule.section, partnerDrawRenderer,
      partnerRule.hasMesh ? partnerRule.mesh : "<source>");
  EiemProbePartnerBoneBindings(sourceMeshOwner, "partner-created");
  return partnerDrawRenderer;
}

// Refresh a still-registered Partner in place after F10. Rebuilding the
// GameObject would put it after the game's skin/LOD assembly and can leave the
// component outside the internal Animator registry. Updating the existing
// component keeps that registration while allowing changed Mesh/Skeleton/
// Material resources to be published.
static bool EiemRefreshPartnerRenderer(
    EiemPartnerState &state, void *sourceMeshOwner, void *sourceDrawRenderer,
    const char *rendererType, const EiemModRule &partnerRule,
    void *sourceMesh, bool controlVisible, char *error, size_t errorSize) {
  if (!state.partnerObject || !state.partnerRenderer ||
      EiemNativeObjectStatus(state.partnerObject) != 1 ||
      EiemNativeObjectStatus(state.partnerRenderer) != 1) {
    if (error) strncpy_s(error, errorSize, "Existing Partner Renderer is no longer alive", _TRUNCATE);
    return false;
  }

  void *assignedMesh = sourceMesh;
  std::shared_ptr<const EiemSkinIdentity> partnerSkin;
  std::shared_ptr<EiemSkeletonInstance> refreshedSkeleton = state.skeleton;
  char buildError[256] = {};
  if (partnerRule.hasSkeleton &&
      (!EiemModEquals(rendererType, "SkinnedMeshRenderer") ||
       !EiemAcquireSkeleton(partnerRule, sourceMeshOwner, refreshedSkeleton,
                            error, errorSize)))
    return false;
  if (partnerRule.hasMesh &&
      !EiemBuildMeshResource(partnerRule, &assignedMesh, buildError,
                             sizeof(buildError), sourceMesh, &partnerSkin)) {
    if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
    return false;
  }

  void *partnerBones = nullptr;
  if (partnerSkin && !EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    if (error) strncpy_s(error, errorSize, "Skinned Partner Mesh requires a SkinnedMeshRenderer", _TRUNCATE);
    return false;
  }
  if (partnerSkin &&
      !(refreshedSkeleton
            ? EiemSkeletonMeshBones(*partnerSkin, *refreshedSkeleton,
                                    &partnerBones, buildError,
                                    sizeof(buildError))
            : EiemResolveMeshBones(*partnerSkin, sourceMeshOwner,
                                   &partnerBones, buildError,
                                   sizeof(buildError)))) {
    if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
    return false;
  }
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer") && !partnerBones &&
      g_smr_get_bones)
    partnerBones = Invoke(g_smr_get_bones, sourceMeshOwner);

  const bool skeletonChanged = refreshedSkeleton != state.skeleton;
  if (skeletonChanged && refreshedSkeleton &&
      !EiemWatchSkeletonPartner(*refreshedSkeleton, state.partnerRenderer)) {
    if (error) strncpy_s(error, errorSize, "Cannot retain refreshed Skeleton Partner lease", _TRUNCATE);
    return false;
  }

  if (assignedMesh && !EiemSetSharedMesh(state.partnerRenderer, assignedMesh,
                                          rendererType, nullptr)) {
    if (skeletonChanged && refreshedSkeleton)
      EiemReleaseSkeletonConsumer(*refreshedSkeleton, state.partnerRenderer);
    if (error) strncpy_s(error, errorSize, "Existing Partner Mesh assignment failed", _TRUNCATE);
    return false;
  }
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    if (partnerBones && g_smr_set_bones && g_smr_get_bones) {
      void *params[] = {partnerBones};
      void *result = nullptr;
      const bool previous = s_eiemApplyingModMeshAssignment;
      s_eiemApplyingModMeshAssignment = true;
      const bool assigned = InvokeChecked(g_smr_set_bones, state.partnerRenderer,
                                          params, &result);
      s_eiemApplyingModMeshAssignment = previous;
      if (!assigned || !EiemManagedObjectArraySame(
                          partnerBones,
                          Invoke(g_smr_get_bones, state.partnerRenderer))) {
        if (skeletonChanged && refreshedSkeleton)
          EiemReleaseSkeletonConsumer(*refreshedSkeleton, state.partnerRenderer);
        if (error) strncpy_s(error, errorSize, "Existing Partner bone palette assignment failed", _TRUNCATE);
        return false;
      }
    }
    if (g_smr_get_rootBone && g_smr_set_rootBone) {
      void *rootBone = Invoke(g_smr_get_rootBone, sourceMeshOwner);
      if (rootBone) {
        void *params[] = {rootBone};
        Invoke(g_smr_set_rootBone, state.partnerRenderer, params);
      }
    }
    EiemCopySkinnedRendererState(sourceDrawRenderer, state.partnerRenderer);
  }

  if (partnerRule.materialCount || partnerRule.submeshCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(partnerRule, sourceDrawRenderer,
                                             &materials, buildError,
                                             sizeof(buildError)) ||
        !materials ||
        !EiemAssignRendererMaterials(state.partnerRenderer, materials,
                                     buildError, sizeof(buildError))) {
      if (skeletonChanged && refreshedSkeleton)
        EiemReleaseSkeletonConsumer(*refreshedSkeleton, state.partnerRenderer);
      if (error) strncpy_s(error, errorSize, buildError[0] ? buildError : "Existing Partner materials failed", _TRUNCATE);
      return false;
    }
  } else if (g_renderer_get_sharedMaterials && s_eiemRendererSetSharedMaterials) {
    void *materials = Invoke(g_renderer_get_sharedMaterials, sourceDrawRenderer);
    if (materials &&
        !EiemAssignRendererMaterials(state.partnerRenderer, materials,
                                     buildError, sizeof(buildError))) {
      if (skeletonChanged && refreshedSkeleton)
        EiemReleaseSkeletonConsumer(*refreshedSkeleton, state.partnerRenderer);
      if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
      return false;
    }
  }

  if (skeletonChanged && state.skeleton)
    EiemReleaseSkeletonConsumer(*state.skeleton, state.partnerRenderer);
  state.skeleton = std::move(refreshedSkeleton);
  state.skin = std::move(partnerSkin);
  state.expectedBoneCount = 0;
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    state.expectedBoneCount = EiemManagedArrayLength(partnerBones);
    if (!state.expectedBoneCount && g_smr_get_bones)
      state.expectedBoneCount = EiemManagedArrayLength(
          Invoke(g_smr_get_bones, state.partnerRenderer));
  }
  state.sourceRenderer = sourceMeshOwner;
  state.sourceDrawRenderer = sourceDrawRenderer;
  state.generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  state.controlVisible = controlVisible;
  state.enabledWhenVisible = !EiemModEquals(partnerRule.handling, "skip");
  EiemUpdateRendererShapes(state.partnerRenderer, rendererType, partnerRule,
                           state.shapes);
  EiemSetPartnerDrawVisibility({state.sourceDrawRenderer,
                                state.partnerRenderer, state.controlVisible,
                                state.enabledWhenVisible});
  Log("[MOD] partner Renderer refreshed: source=%p section=%s renderer=%p mesh=%s generation=%ld",
      sourceMeshOwner, partnerRule.section, state.partnerRenderer,
      partnerRule.hasMesh ? partnerRule.mesh : "<source>", state.generation);
  return true;
}

static void EiemApplyPartners(void *sourceMeshOwner,
                              void *sourceDrawRenderer, void *sourceMesh,
                              const char *rendererType,
                              const EiemModRule &sourceRule) {
  if (!sourceMeshOwner || !sourceDrawRenderer || s_eiemCreatingPartner)
    return;
  std::vector<EiemModRule> potentialPartners;
  EiemFindPotentialPartnerRules(sourceRule.modPath, sourceRule.section,
                                &potentialPartners);
  if (potentialPartners.empty()) return;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  std::vector<std::string> seenSections;
  seenSections.reserve(potentialPartners.size());
  for (const auto &partner : potentialPartners) {
    seenSections.emplace_back(partner.section);
    bool desired = false;
    for (uint32_t index = 0; index < sourceRule.partnerCount; ++index) {
      if (_stricmp(sourceRule.partners[index], partner.section) == 0) {
        desired = true;
        break;
      }
    }

    EiemPartnerVisibilityChange existing;
    bool found = false;
    AcquireSRWLockExclusive(&s_eiemPartnerLock);
    size_t existingIndex = EiemFindPartnerLocked(
        sourceMeshOwner, sourceRule.modPath, partner.section, generation);
    if (existingIndex == SIZE_MAX)
      existingIndex = EiemFindPartnerAnyGenerationLocked(
          sourceMeshOwner, sourceRule.modPath, partner.section);
    if (existingIndex != SIZE_MAX) {
      auto &state = s_eiemPartners[existingIndex];
      // RendererInfo._Init can create a Partner before the model lifecycle
      // adapter (BaseModelViewPart/NPCAvatar/CharUI) publishes its owner.
      // Bind the already-created component when the model-scoped replay sees
      // it.  Without this, ownerPrefabInstance stays zero and teardown cannot
      // retire that Partner with its source model, leaving a stale renderer
      // that can retain the wrong visibility or skin generation after F10.
      if (s_eiemActivePrefabInstance)
        state.ownerPrefabInstance = s_eiemActivePrefabInstance;
      existing = {state.sourceDrawRenderer, state.partnerRenderer, desired,
                  state.enabledWhenVisible};
      found = true;
    }
    ReleaseSRWLockExclusive(&s_eiemPartnerLock);
    if (found) {
      // A generation change is a resource refresh, not a reason to allocate a
      // second Renderer. Keep the component address stable so game skin/LOD
      // registration survives F10. Ordinary key changes only take this fast
      // visibility path because the generation already matches.
      bool refreshed = true;
      EiemPartnerState refreshState = {};
      bool needsRefresh = false;
      AcquireSRWLockShared(&s_eiemPartnerLock);
      size_t refreshIndex = EiemFindPartnerAnyGenerationLocked(
          sourceMeshOwner, sourceRule.modPath, partner.section);
      if (refreshIndex != SIZE_MAX &&
          s_eiemPartners[refreshIndex].generation != generation) {
        refreshState = s_eiemPartners[refreshIndex];
        needsRefresh = true;
      }
      ReleaseSRWLockShared(&s_eiemPartnerLock);
      if (needsRefresh) {
        EiemProbePartnerBoneBindings(sourceMeshOwner,
                                     "partner-refresh-before");
        Log("[MOD-RELOAD-TRACE] partner-refresh begin source=%p partner=%p "
            "section=%s oldGeneration=%ld newGeneration=%ld",
            sourceMeshOwner, refreshState.partnerRenderer, partner.section,
            refreshState.generation, generation);
        s_eiemCreatingPartner = true;
        char refreshError[256] = {};
        refreshed = EiemRefreshPartnerRenderer(
            refreshState, sourceMeshOwner, sourceDrawRenderer, rendererType,
            partner, sourceMesh, desired, refreshError, sizeof(refreshError));
        s_eiemCreatingPartner = false;
        if (refreshed) {
          AcquireSRWLockExclusive(&s_eiemPartnerLock);
          const size_t commitIndex = EiemFindPartnerAnyGenerationLocked(
              sourceMeshOwner, sourceRule.modPath, partner.section);
          if (commitIndex != SIZE_MAX)
            s_eiemPartners[commitIndex] = std::move(refreshState);
           else
             refreshed = false;
           ReleaseSRWLockExclusive(&s_eiemPartnerLock);
          if (refreshed) {
            EiemProbePartnerBoneBindings(sourceMeshOwner,
                                         "partner-refresh-after");
            Log("[MOD-RELOAD-TRACE] partner-refresh end source=%p partner=%p "
                "section=%s generation=%ld result=success",
                sourceMeshOwner, refreshState.partnerRenderer, partner.section,
                generation);
          }
        }
        if (!refreshed)
          Log("[MOD] partner refresh failed: source=%s partner=%s error=%s",
              sourceRule.section, partner.section,
              refreshError[0] ? refreshError : "unknown");
      } else if (refreshIndex != SIZE_MAX) {
        AcquireSRWLockExclusive(&s_eiemPartnerLock);
        s_eiemPartners[refreshIndex].controlVisible = desired;
        existing = {s_eiemPartners[refreshIndex].sourceDrawRenderer,
                    s_eiemPartners[refreshIndex].partnerRenderer, desired,
                    s_eiemPartners[refreshIndex].enabledWhenVisible};
        ReleaseSRWLockExclusive(&s_eiemPartnerLock);
      }
      if (refreshed) {
        AcquireSRWLockShared(&s_eiemPartnerLock);
        const size_t visibleIndex = EiemFindPartnerAnyGenerationLocked(
            sourceMeshOwner, sourceRule.modPath, partner.section);
        if (visibleIndex != SIZE_MAX) {
          const auto &state = s_eiemPartners[visibleIndex];
          existing = {state.sourceDrawRenderer, state.partnerRenderer,
                      state.controlVisible, state.enabledWhenVisible};
        }
        ReleaseSRWLockShared(&s_eiemPartnerLock);
        EiemSetPartnerDrawVisibility(existing);
      }
      continue;
    }

    char error[256] = {};
    s_eiemCreatingPartner = true;
    EiemCreatePartnerRenderer(sourceMeshOwner, sourceDrawRenderer,
                              rendererType, sourceRule.section, partner,
                              sourceMesh, desired, error,
                              sizeof(error));
    s_eiemCreatingPartner = false;
    if (error[0])
      Log("[MOD] partner creation failed: source=%s partner=%s error=%s",
          sourceRule.section, partner.section, error);
  }
  // Remove Partner declarations that disappeared from the reloaded config.
  // Failed resource refreshes deliberately keep their previous generation so
  // a transient file write cannot turn a working component into a blank one.
  std::vector<EiemPartnerState> removed;
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (size_t index = 0; index < s_eiemPartners.size();) {
    const auto &state = s_eiemPartners[index];
    const bool sameSource =
        state.sourceRenderer == sourceMeshOwner &&
        EiemModEquals(state.modPath, sourceRule.modPath) &&
        _stricmp(state.sourceSection, sourceRule.section) == 0;
    const bool stillDeclared =
        std::find(seenSections.begin(), seenSections.end(), state.section) !=
        seenSections.end();
    if (!sameSource || stillDeclared || state.generation == generation) {
      ++index;
      continue;
    }
    removed.push_back(state);
    s_eiemPartners.erase(s_eiemPartners.begin() + index);
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  for (const auto &state : removed) EiemRetirePartner(state);
  // The source Animator may assign rootBone after the initial partner pass.
  // Re-read it here so a newly created Partner never keeps a stale/null value.
  EiemSyncPartnerRootBones(sourceMeshOwner);
}

static void TraceReadStringField(void *object, int offset, char *out,
                                 size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!object || offset < 0) return;
  __try {
    void *value = *(void **)((char *)object + offset);
    if (value) ReadStrUtf8(value, out, (int)outSize);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out[0] = '\0';
  }
}

static void *TraceReadObjectField(void *object, int offset) {
  if (!object || offset < 0) return nullptr;
  __try { return *(void **)((char *)object + offset); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

static int EiemReadManagedBoolArrayValue(void *array, size_t count,
                                         size_t index) {
  if (!array || index >= count || count > 8192) return -1;
  __try {
    return *((uint8_t *)array + IL2CPP_ARRAY_DATA + index) ? 1 : 0;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return -1;
  }
}

struct EiemBaseModelPartnerArrayProbe {
  void *source = nullptr;
  void *partner = nullptr;
  LONG generation = -1;
  char section[96] = {};
};

// BaseModelViewPart keeps its own Renderer/SkinnedMeshRenderer caches and
// parallel initial-enabled arrays. NPC assembly has a separate, confirmed
// registration path; the world-model path has not yet been proved. Observe
// only known Partner pairs at the exact load-completion boundary so a missing
// cache entry can be distinguished from a bad public bones/root assignment.
// This probe never changes a game-owned array.
static void EiemTraceBaseModelPartnerArrays(void *part,
                                            const char *boundary) {
  if (!kEiemValidationIdentityProbe) return;
  if (!part) return;
  void *renderers =
      TraceReadObjectField(part, s_basePartRenderersOffset);
  void *rendererStates =
      TraceReadObjectField(part, s_basePartRenderersInitStateOffset);
  void *meshes = TraceReadObjectField(part, s_basePartMeshesOffset);
  void *meshStates =
      TraceReadObjectField(part, s_basePartMeshesInitStateOffset);
  void *lodGroups = TraceReadObjectField(part, s_basePartLodGroupsOffset);
  const size_t rendererCount = EiemManagedArrayLength(renderers);
  const size_t rendererStateCount = EiemManagedArrayLength(rendererStates);
  const size_t meshCount = EiemManagedArrayLength(meshes);
  const size_t meshStateCount = EiemManagedArrayLength(meshStates);
  const size_t lodCount = EiemManagedArrayLength(lodGroups);

  std::vector<EiemBaseModelPartnerArrayProbe> candidates;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  candidates.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer || !state.partnerRenderer) continue;
    size_t sourceMeshIndex = SIZE_MAX, sourceRendererIndex = SIZE_MAX;
    const bool belongs =
        EiemManagedArrayFindPointer(meshes, meshCount, state.sourceRenderer,
                                    &sourceMeshIndex) ||
        EiemManagedArrayFindPointer(renderers, rendererCount,
                                    state.sourceDrawRenderer
                                        ? state.sourceDrawRenderer
                                        : state.sourceRenderer,
                                    &sourceRendererIndex);
    if (!belongs) continue;
    EiemBaseModelPartnerArrayProbe probe = {};
    probe.source = state.sourceRenderer;
    probe.partner = state.partnerRenderer;
    probe.generation = state.generation;
    strncpy_s(probe.section, sizeof(probe.section), state.section,
              _TRUNCATE);
    candidates.push_back(probe);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  if (candidates.empty()) return;

  Log("[BASEMODEL-SKIN-v108] boundary=%s part=%p model=%p candidates=%zu "
      "renderers=%zu rendererStates=%zu meshes=%zu meshStates=%zu lods=%zu",
      boundary ? boundary : "unknown", part,
      TraceReadObjectField(part, s_basePartModelOffset), candidates.size(),
      rendererCount, rendererStateCount, meshCount, meshStateCount, lodCount);
  for (const auto &probe : candidates) {
    size_t sourceRendererIndex = SIZE_MAX, partnerRendererIndex = SIZE_MAX;
    size_t sourceMeshIndex = SIZE_MAX, partnerMeshIndex = SIZE_MAX;
    EiemManagedArrayFindPointer(renderers, rendererCount, probe.source,
                                &sourceRendererIndex);
    EiemManagedArrayFindPointer(renderers, rendererCount, probe.partner,
                                &partnerRendererIndex);
    EiemManagedArrayFindPointer(meshes, meshCount, probe.source,
                                &sourceMeshIndex);
    EiemManagedArrayFindPointer(meshes, meshCount, probe.partner,
                                &partnerMeshIndex);
    const int sourceRendererInit = EiemReadManagedBoolArrayValue(
        rendererStates, rendererStateCount, sourceRendererIndex);
    const int partnerRendererInit = EiemReadManagedBoolArrayValue(
        rendererStates, rendererStateCount, partnerRendererIndex);
    const int sourceMeshInit = EiemReadManagedBoolArrayValue(
        meshStates, meshStateCount, sourceMeshIndex);
    const int partnerMeshInit = EiemReadManagedBoolArrayValue(
        meshStates, meshStateCount, partnerMeshIndex);
    Log("[BASEMODEL-SKIN-v108] boundary=%s generation=%ld section=%s "
        "source=%p partner=%p rendererIndex=%lld/%lld rendererInit=%d/%d "
        "meshIndex=%lld/%lld meshInit=%d/%d",
        boundary ? boundary : "unknown", probe.generation,
        probe.section[0] ? probe.section : "<unknown>", probe.source,
        probe.partner,
        sourceRendererIndex == SIZE_MAX ? -1LL
                                        : (long long)sourceRendererIndex,
        partnerRendererIndex == SIZE_MAX ? -1LL
                                         : (long long)partnerRendererIndex,
        sourceRendererInit, partnerRendererInit,
        sourceMeshIndex == SIZE_MAX ? -1LL : (long long)sourceMeshIndex,
        partnerMeshIndex == SIZE_MAX ? -1LL : (long long)partnerMeshIndex,
        sourceMeshInit, partnerMeshInit);
  }
}

// BaseModelViewPart owns the world-model Renderer/SkinnedMeshRenderer caches
// that are used after model loading for visibility, LOD and skin setup.  A
// Partner created as a child GameObject is not automatically inserted into
// those managed arrays.  Add every already-created Partner whose source is in
// this part's arrays at the model-completion boundary, before any later game
// visibility or skin pass can consume the cache.  This is deliberately a
// one-time structural operation; key controls never call it.
static bool EiemRegisterPartnersInBaseModelArrays(void *part,
                                                  const char *stage) {
  // This path is the world/PostModel sibling of
  // EiemRegisterPartnersInSkinArrays. It used to fail silently at eight
  // different early exits, so a PostModel whose Partners never reached the
  // game's skin assembly was indistinguishable from "nothing to append".
  // Every rejection now names its reason once per stage.
  const char *const label = stage ? stage : "unknown";
  // Only genuinely actionable rejections are logged. OnLoadFinish runs before
  // the game has published its caches, so "cache not ready yet" and "nothing to
  // append" are the normal case and used to flood the log with one line per
  // part per boundary.
  auto reject = [&](const char *why) {
    const bool interesting =
        strcmp(why, "nothing-to-append") != 0 &&
        strcmp(why, "renderer-or-mesh-cache-unusable") != 0 &&
        strcmp(why, "null-field-address") != 0;
    if (interesting &&
        EiemRegistrationTraceFirst("base-array-reject", why, part, nullptr,
                                   (void *)stage, -1))
      Log("[MOD-BASE-ARRAY-FAIL] stage=%s part=%p reason=%s", label, part, why);
    return false;
  };
  if (!part || !il2cpp_array_new_specific || !il2cpp_object_get_class ||
      !il2cpp_class_get_element_class)
    return reject("array-apis-unavailable");
  if (s_basePartRenderersOffset < 0 || s_basePartMeshesOffset < 0)
    return reject("field-offsets-unresolved");
  void **renderersField =
      (void **)((char *)part + s_basePartRenderersOffset);
  void **rendererStatesField =
      (void **)((char *)part + s_basePartRenderersInitStateOffset);
  void **meshesField = (void **)((char *)part + s_basePartMeshesOffset);
  void **meshStatesField =
      (void **)((char *)part + s_basePartMeshesInitStateOffset);
  if (!renderersField || !meshesField) return reject("null-field-address");
  void *renderers = *renderersField;
  void *meshes = *meshesField;
  const size_t rendererCount = EiemManagedArrayLength(renderers);
  const size_t meshCount = EiemManagedArrayLength(meshes);
  if (!renderers || !meshes || !rendererCount || rendererCount > 8192 ||
      meshCount > 8192)
    return reject("renderer-or-mesh-cache-unusable");

  struct Append {
    void *renderer = nullptr;
    size_t rendererSourceIndex = SIZE_MAX;
    size_t meshSourceIndex = SIZE_MAX;
    char section[96] = {};
  };
  std::vector<Append> append;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  append.reserve(s_eiemPartners.size());
  size_t considered = 0, dead = 0, noSourceRenderer = 0, noSourceMesh = 0;
  size_t alreadyPresent = 0;
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer || !state.partnerRenderer) continue;
    ++considered;
    if (EiemNativeObjectStatus(state.partnerRenderer) != 1) {
      ++dead;
      continue;
    }
    size_t rendererSourceIndex = SIZE_MAX;
    size_t meshSourceIndex = SIZE_MAX;
    if (!EiemManagedArrayFindPointer(renderers, rendererCount,
                                     state.sourceDrawRenderer
                                         ? state.sourceDrawRenderer
                                         : state.sourceRenderer,
                                     &rendererSourceIndex)) {
      ++noSourceRenderer;
      continue;
    }
    if (!EiemManagedArrayFindPointer(meshes, meshCount, state.sourceRenderer,
                                     &meshSourceIndex)) {
      ++noSourceMesh;
      continue;
    }
    size_t existing = SIZE_MAX;
    if (EiemManagedArrayFindPointer(renderers, rendererCount,
                                    state.partnerRenderer, &existing) ||
        EiemManagedArrayFindPointer(meshes, meshCount, state.partnerRenderer,
                                    &existing)) {
      ++alreadyPresent;
      continue;
    }
    bool duplicate = false;
    for (const auto &candidate : append)
      if (candidate.renderer == state.partnerRenderer) {
        duplicate = true;
        break;
      }
    if (duplicate) continue;
    Append candidate = {};
    candidate.renderer = state.partnerRenderer;
    candidate.rendererSourceIndex = rendererSourceIndex;
    candidate.meshSourceIndex = meshSourceIndex;
    strncpy_s(candidate.section, sizeof(candidate.section), state.section,
              _TRUNCATE);
    append.push_back(candidate);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  // Only report once a real candidate set existed; an empty `considered` means
  // this boundary ran before the Partners for this model were created at all.
  if (considered &&
      EiemRegistrationTraceFirst("base-array-reject", "candidates", part,
                                 renderers, (void *)stage, -1))
    Log("[MOD-BASE-ARRAY-PROBE] stage=%s part=%p renderers=%zu meshes=%zu "
        "considered=%zu dead=%zu noSourceRenderer=%zu noSourceMesh=%zu "
        "alreadyPresent=%zu append=%zu rendererStatesField=%p meshStatesField=%p",
        label, part, rendererCount, meshCount, considered, dead,
        noSourceRenderer, noSourceMesh, alreadyPresent, append.size(),
        rendererStatesField ? *rendererStatesField : nullptr,
        meshStatesField ? *meshStatesField : nullptr);
  if (append.empty())
    return reject("nothing-to-append");
  if (append.size() > 8192 - rendererCount ||
      append.size() > 8192 - meshCount)
    return reject("array-capacity-limit");

  auto expandReferenceArray = [&](void *source, size_t oldCount,
                                  const std::vector<void *> &values) -> void * {
    if (!source || values.empty()) return nullptr;
    void *arrayClass = il2cpp_object_get_class(source);
    if (!arrayClass || !il2cpp_class_get_element_class(arrayClass))
      return nullptr;
    void *expanded = il2cpp_array_new_specific(arrayClass,
                                               oldCount + values.size());
    if (!expanded) return nullptr;
    __try {
      void **sourceItems = (void **)((char *)source + IL2CPP_ARRAY_DATA);
      void **expandedItems =
          (void **)((char *)expanded + IL2CPP_ARRAY_DATA);
      memcpy(expandedItems, sourceItems, oldCount * sizeof(void *));
      for (size_t index = 0; index < values.size(); ++index)
        expandedItems[oldCount + index] = values[index];
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return nullptr;
    }
    return expanded;
  };

  std::vector<void *> partnerRenderers;
  partnerRenderers.reserve(append.size());
  for (const auto &candidate : append)
    partnerRenderers.push_back(candidate.renderer);
  // Build every replacement array before publishing any field.  A failed
  // allocation or copy must leave the game's parallel caches untouched; a
  // half-expanded Renderer/Mesh pair is indistinguishable from a bad skin
  // registration and can leave a model in bind/T-pose.
  void *expandedRenderers =
      expandReferenceArray(renderers, rendererCount, partnerRenderers);
  void *expandedMeshes =
      expandReferenceArray(meshes, meshCount, partnerRenderers);
  if (!expandedRenderers || !expandedMeshes)
    return reject("reference-array-expansion-failed");

  auto expandBoolArray = [&](void *source, size_t oldCount,
                             const std::vector<size_t> &sourceIndices) -> void * {
    if (!source || sourceIndices.empty()) return nullptr;
    void *arrayClass = il2cpp_object_get_class(source);
    if (!arrayClass || !il2cpp_class_get_element_class(arrayClass))
      return nullptr;
    void *expanded = il2cpp_array_new_specific(
        arrayClass, oldCount + sourceIndices.size());
    if (!expanded) return nullptr;
    __try {
      const uint8_t *sourceData =
          (const uint8_t *)source + IL2CPP_ARRAY_DATA;
      uint8_t *expandedData =
          (uint8_t *)expanded + IL2CPP_ARRAY_DATA;
      memcpy(expandedData, sourceData, oldCount);
      for (size_t index = 0; index < sourceIndices.size(); ++index) {
        const size_t sourceIndex = sourceIndices[index];
        if (sourceIndex >= oldCount) return nullptr;
        expandedData[oldCount + index] = sourceData[sourceIndex];
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      return nullptr;
    }
    return expanded;
  };

  // The game's initial-enabled values are per cached Renderer/Mesh.  Copy the
  // source slot for each appended Partner so later SetRendererVisible and
  // ResetMeshVisible calls see the same initial state as the source.
  std::vector<size_t> rendererIndices, meshIndices;
  rendererIndices.reserve(append.size());
  meshIndices.reserve(append.size());
  for (const auto &candidate : append) {
    rendererIndices.push_back(candidate.rendererSourceIndex);
    meshIndices.push_back(candidate.meshSourceIndex);
  }
  void *expandedRendererStates = nullptr;
  void *expandedMeshStates = nullptr;
  if (rendererStatesField && *rendererStatesField) {
    if (EiemManagedArrayLength(*rendererStatesField) != rendererCount)
      return reject("renderer-state-length-mismatch");
    expandedRendererStates =
        expandBoolArray(*rendererStatesField, rendererCount, rendererIndices);
    if (!expandedRendererStates)
      return reject("renderer-state-expansion-failed");
  }
  if (meshStatesField && *meshStatesField) {
    if (EiemManagedArrayLength(*meshStatesField) != meshCount)
      return reject("mesh-state-length-mismatch");
    expandedMeshStates =
        expandBoolArray(*meshStatesField, meshCount, meshIndices);
    if (!expandedMeshStates) return reject("mesh-state-expansion-failed");
  }

  // Commit all four parallel fields together.  The game can observe these
  // caches from later callbacks; keeping their lengths synchronized is part
  // of the registration contract.
  *renderersField = expandedRenderers;
  *meshesField = expandedMeshes;
  if (expandedRendererStates) *rendererStatesField = expandedRendererStates;
  if (expandedMeshStates) *meshStatesField = expandedMeshStates;

  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (auto &state : s_eiemPartners) {
    for (const auto &candidate : append)
      if (state.partnerRenderer == candidate.renderer) {
        state.skinArrayObserved = true;
        break;
      }
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  Log("[MOD-BASE-ARRAY] stage=%s part=%p sourceRenderers=%zu "
      "sourceMeshes=%zu appended=%zu newRenderers=%zu newMeshes=%zu",
      stage ? stage : "unknown", part, rendererCount, meshCount,
      append.size(), rendererCount + append.size(), meshCount + append.size());
  return true;
}

static void TraceRememberLoadedModelPath(void *model, int64_t pathHash) {
  if (!model || !pathHash) return;
  char path[768] = {};
  TraceLookupHashPath(pathHash, path, sizeof(path));
  if (!path[0]) TraceResolveStringPathHashPath(pathHash, path, sizeof(path));
  if (!path[0]) return;
  AcquireSRWLockExclusive(&s_traceLoadedModelPathLock);
  size_t slot = s_traceLoadedModelPathCount;
  for (size_t index = 0; index < s_traceLoadedModelPathCount; ++index) {
    if (s_traceLoadedModelPaths[index].model == model) {
      slot = index;
      break;
    }
  }
  if (slot == s_traceLoadedModelPathCount) {
    if (slot >= _countof(s_traceLoadedModelPaths)) slot = slot % _countof(s_traceLoadedModelPaths);
    else ++s_traceLoadedModelPathCount;
  }
  s_traceLoadedModelPaths[slot].model = model;
  s_traceLoadedModelPaths[slot].pathHash = pathHash;
  strncpy_s(s_traceLoadedModelPaths[slot].path,
            sizeof(s_traceLoadedModelPaths[slot].path), path, _TRUNCATE);
  ReleaseSRWLockExclusive(&s_traceLoadedModelPathLock);
}

static bool TraceLookupLoadedModelPath(void *model, char *out, size_t outSize) {
  if (!model || !out || outSize == 0) return false;
  out[0] = '\0';
  AcquireSRWLockShared(&s_traceLoadedModelPathLock);
  for (size_t index = 0; index < s_traceLoadedModelPathCount; ++index) {
    if (s_traceLoadedModelPaths[index].model == model) {
      strncpy_s(out, outSize, s_traceLoadedModelPaths[index].path, _TRUNCATE);
      ReleaseSRWLockShared(&s_traceLoadedModelPathLock);
      return out[0] != '\0';
    }
  }
  ReleaseSRWLockShared(&s_traceLoadedModelPathLock);
  return false;
}

// PrefabInstantiateProxy is the normal world-model lifecycle adapter. Other
// lifecycle owners below feed the same instance registry and Render path.
// Diagnostic only: list the renderers a freshly instantiated character prefab
// carries, and whether they already hold Mesh references. Every presentation
// path instantiates its own Prefab for the same character, so the prefab path
// is what lists them. Kept out of the completion hook itself: that hook has a
// size-bounded contract, and inlining this scan pushed the model-registration
// call out of the window that verifies it.
static void TraceDumpPrefabRenderers(const char *path, void *model) {
  if (!kEiemValidationIdentityProbe) return;
  std::vector<EiemModPrefab> configuredPrefabs;
  if (path && path[0]) EiemFindModPrefabs(path, &configuredPrefabs);
  if (!model || !path || !path[0] || configuredPrefabs.empty() ||
      !g_gameObject_GetComponentsInChildren || !g_skinnedMeshRendererClass ||
      !il2cpp_class_get_type || !il2cpp_type_get_object)
    return;
  void *type = il2cpp_class_get_type(g_skinnedMeshRendererClass);
  void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
  if (!typeObject) return;
  bool includeInactive = true;
  void *params[] = {typeObject, &includeInactive};
  void *array = Invoke(g_gameObject_GetComponentsInChildren, model, params);
  const size_t count = EiemManagedArrayLength(array);
  Log("[PREFAB-RENDERERS] path=%s model=%p skinnedRenderers=%zu", path, model,
      count);
  if (!array || count > 256) return;
  void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
  for (size_t index = 0; index < count; ++index) {
    void *renderer = items[index];
    if (!renderer) continue;
    char rendererName[160] = {};
    TraceReadUnityObjectName(renderer, rendererName, sizeof(rendererName));
    void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
    char meshName[192] = {};
    if (mesh) TraceReadUnityObjectName(mesh, meshName, sizeof(meshName));
    void *rootBone =
        g_smr_get_rootBone ? Invoke(g_smr_get_rootBone, renderer) : nullptr;
    char rootBoneName[160] = {};
    if (rootBone)
      TraceReadUnityObjectName(rootBone, rootBoneName, sizeof(rootBoneName));
    Log("[PREFAB-RENDERER] path=%s index=%zu name=%s mesh=%p meshName=%s "
        "rootBoneName=%s",
        path, index, rendererName[0] ? rendererName : "<unnamed>", mesh,
        meshName[0] ? meshName : "<empty>",
        rootBoneName[0] ? rootBoneName : "<empty>");
  }
}

static void TracePrefabInstantiateCompleted(void *self, void *methodInfo) {
  // Capture identity before the game completion method is allowed to release
  // or recycle its asset handle. The instantiated GameObject is read after
  // completion, when the hierarchy is ready for Render actions.
  char path[768] = {};
  if (s_prefabInstantiateGetLogName)
    TraceDescribeString(Invoke(s_prefabInstantiateGetLogName, self), path,
                        sizeof(path));
  uint32_t instanceUid = 0;
  if (s_prefabInstantiateGetInstanceUid) {
    void *boxed = Invoke(s_prefabInstantiateGetInstanceUid, self);
    if (boxed) instanceUid = (uint32_t)EiemTraceUnboxInt(boxed);
  }
  auto original = (TracePrefabInstantiateCompletedFn)
      s_origPrefabInstantiateCompleted;
  if (original) original(self, methodInfo);
  const LONG64 perfStarted = EiemPerfNow();
  void *model = s_prefabInstantiateGetGameObject
                    ? Invoke(s_prefabInstantiateGetGameObject, self)
                    : nullptr;
  std::vector<EiemModPrefab> prefabs;
  EiemFindModPrefabs(path, &prefabs);
  const bool configured = !prefabs.empty();
  if (TraceTakeBudget(&s_tracePrefabIdentityCount, 160))
    Log("[TRACE-PREFAB] completed proxy=%p uid=%u path=%s model=%p configured=%d declarations=%zu",
        self, instanceUid, path[0] ? path : "<none>", model,
        configured ? 1 : 0, prefabs.size());
  // Character prefabs only. Every presentation path instantiates its own Prefab
  // for the same character, so the prefab path is what lists them, and the
  // renderers in the freshly instantiated hierarchy are what that prefab
  // declares. Reporting both here answers two things at once: which prefabs a
  // character has, and whether a path's renderers carry the Mesh references
  // before any mod code runs.
  TraceDumpPrefabRenderers(path, model);
  const bool applied =
      EiemRegisterAndApplyModelInstance(
          EiemModelOwnerKind::PrefabProxy, self, model, path, instanceUid,
          "PrefabInstantiateProxy.OnCompleted", false);
  if (configured)
    Log("[MOD-PREFAB] completed proxy=%p uid=%u path=%s model=%p applied=%d",
        self, instanceUid, path, model, applied ? 1 : 0);
  const LONG64 perfCalls =
      EiemPerfRecord(s_eiemPerfPrefabCompletion, perfStarted);
  if ((perfCalls & 255) == 0) EiemLogPerformanceSummary(perfCalls);
}

static void TracePrefabInstantiateUnload(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::PrefabProxy, self,
                       "PrefabInstantiateProxy.Unload");
  auto original = (TracePrefabInstantiateLifecycleFn)s_origPrefabInstantiateUnload;
  if (original) original(self, methodInfo);
}

static void TracePrefabInstantiateClear(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::PrefabProxy, self,
                       "PrefabInstantiateProxy.Clear");
  auto original = (TracePrefabInstantiateLifecycleFn)s_origPrefabInstantiateClear;
  if (original) original(self, methodInfo);
}

static void TracePrefabInstantiateDispose(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::PrefabProxy, self,
                       "PrefabInstantiateProxy.Dispose");
  auto original = (TracePrefabInstantiateLifecycleFn)s_origPrefabInstantiateDispose;
  if (original) original(self, methodInfo);
}

static void *TraceUIModelLoaderLoadModel(void *self, void *path,
                                         void *parent, void *methodInfo) {
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  auto original =
      (TraceUIModelLoaderLoadModelFn)s_origUIModelLoaderLoadModel;
  void *model = original ? original(self, path, parent, methodInfo) : nullptr;
  const bool applied = EiemRegisterAndApplyModelInstance(
      EiemModelOwnerKind::UIModelLoader, self, model, pathText, 0,
      // Loading the PFB is not the bone-assembly completion boundary. The
      // common EntityRenderHelper/CharUIModel completion hook applies after
      // the game's AssignSkin work has populated every LOD palette.
      "UIModelLoader.LoadModel", false);
  if (pathText[0]) {
    std::vector<EiemModPrefab> prefabs;
    EiemFindModPrefabs(pathText, &prefabs);
    if (!prefabs.empty())
      Log("[MOD-UI] sync completed loader=%p path=%s model=%p applied=%d",
          self, pathText, model, applied ? 1 : 0);
  }
  return model;
}

static int32_t TraceUIModelLoaderLoadModelAsync(
    void *self, void *path, void *parent, void *callback, void *methodInfo) {
  // Preserve the game's managed delegate, including its metadata and lifetime.
  // The removed native-address Action wrapper crashed at the game's invoke_impl
  // call before our completion ran (v29, GameAssembly+0x440d5d8). Completion is
  // observed through PrefabInstantiateProxy/CharUIModelMono instead; request IDs
  // are not GameObjects and are never submitted to the renderer executor.
  auto original =
      (TraceUIModelLoaderLoadModelAsyncFn)s_origUIModelLoaderLoadModelAsync;
  const int32_t requestId =
      original ? original(self, path, parent, callback, methodInfo) : -1;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  Log("[MOD-UI] async request: loader=%p path=%s request=%d callback=%p completion=game-owned",
      self, pathText, requestId, callback);
  return requestId;
}

static void TraceUIModelLoaderUnloadModel(void *self, void *model,
                                          void *methodInfo) {
  EiemForgetModelInstance(model, "UIModelLoader.UnloadModel");
  auto original =
      (TraceUIModelLoaderUnloadModelFn)s_origUIModelLoaderUnloadModel;
  if (original) original(self, model, methodInfo);
}

static void TraceUIModelLoaderClear(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::UIModelLoader, self,
                       "UIModelLoader._Clear");
  auto original = (TraceUIModelLoaderLifecycleFn)s_origUIModelLoaderClear;
  if (original) original(self, methodInfo);
}

static void TraceUIModelLoaderDispose(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::UIModelLoader, self,
                       "UIModelLoader.Dispose");
  auto original = (TraceUIModelLoaderLifecycleFn)s_origUIModelLoaderDispose;
  if (original) original(self, methodInfo);
}

static void TraceCharUIModelOnAwake(void *self, void *methodInfo) {
  auto original =
      (TraceCharUIModelLifecycleFn)s_origCharUIModelOnAwake;
  if (original) original(self, methodInfo);
  EiemRegisterCharUIModelInstance(self, "CharUIModelMono.OnAwake", false);
}

static void TraceCharUIModelSetVisible(void *self, bool visible,
                                       void *methodInfo) {
  auto original =
      (TraceCharUIModelSetVisibleFn)s_origCharUIModelSetVisible;
  if (original) original(self, visible, methodInfo);
  if (visible)
    EiemRegisterCharUIModelInstance(self, "CharUIModelMono.SetVisible", true);
  else
    EiemSetModelOwnerActive(EiemModelOwnerKind::CharUIModel, self, false,
                            "CharUIModelMono.SetVisible(false)");
}

static void TraceCharUIModelOnRelease(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::CharUIModel, self,
                       "CharUIModelMono.OnRelease");
  auto original =
      (TraceCharUIModelLifecycleFn)s_origCharUIModelOnRelease;
  if (original) original(self, methodInfo);
}

// Allocation alone does not prove that a skin palette is complete. The PFB,
// UI and NPC owner adapters register model lifetimes at their own boundaries.
static void TraceModelManagerGameObjectAllocate(void *self, void *model,
                                                 void *methodInfo) {
  auto original = (TraceModelManagerGameObjectFn)
      s_origModelManagerGameObjectAllocate;
  if (original) original(self, model, methodInfo);
}

// Persistent-pool loads return an already constructed GameObject. They may
// re-activate a PFB instance previously registered by OnCompleted, but they do
// not discover new replacement ownership. New ownership comes only from the
// an explicit model owner such as PFB, BaseModelViewPart or CharUIModelMono.
static void *TraceModelManagerLoadFromPersistentPool(void *self,
                                                      int64_t pathHash,
                                                      void *methodInfo) {
  auto original = (TraceModelManagerLoadHashFn)
      s_origModelManagerLoadFromPersistentPool;
  void *model = original ? original(self, pathHash, methodInfo) : nullptr;
  if (model) {
    TraceRememberLoadedModelPath(model, pathHash);
  }
  return model;
}

// Measure where a source Renderer and each of its Partners actually ARE in
// world space, at the moments the game finishes assembling the model.
//
// Every other diagnostic in this plugin reports ownership, registration or
// palette facts, and two runs with opposite visual outcomes produced identical
// values on all of them. World-space bounds are different in kind: they are the
// game's own answer to "where is this thing drawn", so a Partner that has
// collapsed to its bind pose or dropped below the character shows up as a
// centre Y well below its source even though every managed array looks right.
//
// Cost is a few getter calls per (source, partner) pair at a completion
// boundary, deduplicated by stage + pair. It performs no hierarchy walk and no
// per-model loop, so it cannot repeat the v117 ring-buffer load-time regression.
static void EiemTracePartnerWorldBounds(void *part, const char *stage,
                                        LONG generation) {
  if (!kEiemValidationIdentityProbe) return;
  if (!part || !g_renderer_get_bounds || !EiemOnUnityThread()) return;
  if (s_basePartRenderersOffset < 0) return;
  void *renderers = TraceReadObjectField(part, s_basePartRenderersOffset);
  const size_t rendererCount = EiemManagedArrayLength(renderers);
  if (!renderers || !rendererCount || rendererCount > 8192) return;

  struct Row {
    void *source;
    void *partner;
    char section[96];
  };
  std::vector<Row> rows;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  rows.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners) {
    if (!state.sourceRenderer || !state.partnerRenderer) continue;
    if (EiemNativeObjectStatus(state.partnerRenderer) != 1) continue;
    void *source = state.sourceDrawRenderer ? state.sourceDrawRenderer
                                            : state.sourceRenderer;
    if (!EiemManagedArrayFindPointer(renderers, rendererCount, source, nullptr))
      continue;
    Row row = {};
    row.source = source;
    row.partner = state.partnerRenderer;
    strncpy_s(row.section, sizeof(row.section), state.section, _TRUNCATE);
    rows.push_back(row);
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  if (rows.empty()) return;

  for (const auto &row : rows) {
    if (!EiemRegistrationTraceFirst("partner-world-bounds", stage, row.source,
                                    row.partner, nullptr, generation))
      continue;
    EiemBounds sourceBounds = {}, partnerBounds = {};
    const bool sourceRead =
        EiemReadBounds(g_renderer_get_bounds, row.source, &sourceBounds);
    const bool partnerRead =
        EiemReadBounds(g_renderer_get_bounds, row.partner, &partnerBounds);
    // Positive means the Partner's drawn centre sits above the source's.
    const double deltaY = partnerRead && sourceRead
                              ? (double)partnerBounds.center.y -
                                    (double)sourceBounds.center.y
                              : 0.0;
    bool partnerEnabled = false;
    EiemReadRendererEnabled(row.partner, &partnerEnabled);
    Log("[MOD-WORLD-BOUNDS] stage=%s generation=%ld section=%s "
        "sourceRead=%d sourceCenterY=%.3f sourceExtentY=%.3f "
        "partnerRead=%d partnerCenterY=%.3f partnerExtentY=%.3f deltaY=%.3f "
        "partnerEnabled=%d",
        stage ? stage : "unknown", generation,
        row.section[0] ? row.section : "<unknown>", sourceRead ? 1 : 0,
        sourceRead ? sourceBounds.center.y : 0.0f,
        sourceRead ? sourceBounds.extents.y : 0.0f, partnerRead ? 1 : 0,
        partnerRead ? partnerBounds.center.y : 0.0f,
        partnerRead ? partnerBounds.extents.y : 0.0f, deltaY,
        partnerEnabled ? 1 : 0);
  }
}

static void TraceBasePartFinish(void *self, bool success, void *methodInfo) {
  if (success)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart.OnLoadFinish-before-original", false);
  auto original = (TraceBasePartFinishFn)s_origBasePartFinish;
  ++s_eiemEnclosingModelAssemblyDepth;
  if (original) original(self, success, methodInfo);
  --s_eiemEnclosingModelAssemblyDepth;
  if (success)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart.OnLoadFinish", true);
}

// PostDealLoadedModel is normally nested inside OnLoadFinish. Let the game
// finish its native renderer and physics registration first. If it is invoked
// independently, its return becomes the completed commit boundary.
static void TraceBasePartPostDeal(void *self, void *methodInfo) {
  EiemAdoptUnityThreadFromAssemblyHook("BaseModelViewPart.PostDealLoadedModel");
  const bool enclosed = s_eiemEnclosingModelAssemblyDepth != 0;
  ++s_eiemEnclosingModelAssemblyDepth;
  auto original = (TraceBasePartPostDealFn)s_origBasePartPostDeal;
  if (original) original(self, methodInfo);
  --s_eiemEnclosingModelAssemblyDepth;
  if (!enclosed)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart.PostDealLoadedModel", true);
}

// ComplexModelViewPart overrides the virtual method, so a base-class hook is
// not sufficient for the concrete character path. Keep a separate trampoline
// and label to make dispatch visible in the runtime log.
static void TraceComplexPartPostDeal(void *self, void *methodInfo) {
  EiemAdoptUnityThreadFromAssemblyHook("ComplexModelViewPart.PostDealLoadedModel");
  const bool enclosed = s_eiemEnclosingModelAssemblyDepth != 0;
  ++s_eiemEnclosingModelAssemblyDepth;
  auto original = (TraceBasePartPostDealFn)s_origComplexPartPostDeal;
  if (original) original(self, methodInfo);
  --s_eiemEnclosingModelAssemblyDepth;
  if (!enclosed)
    EiemRegisterBaseModelViewPartInstance(
        self, "ComplexModelViewPart.PostDealLoadedModel", true);
}

static void TraceBasePartLoadFinishCallback(void *self, int32_t requestId,
                                            int64_t pathHash, void *model,
                                            void *methodInfo) {
  TraceRememberLoadedModelPath(model, pathHash);
  auto original =
      (TraceBasePartLoadFinishCallbackFn)s_origBasePartLoadFinishCallback;
  if (original) original(self, requestId, pathHash, model, methodInfo);
}

static bool TraceBasePartLoadFinishResult(void *self, int32_t requestId,
                                          int64_t pathHash, void *model,
                                          void *methodInfo) {
  TraceRememberLoadedModelPath(model, pathHash);
  auto original =
      (TraceBasePartLoadFinishResultFn)s_origBasePartLoadFinishResult;
  const bool result = original ? original(self, requestId, pathHash, model,
                                           methodInfo)
                               : false;
  return result;
}

// The handle-based path is separate from _OnLoadModelFinish and is used when
// a previously loaded model is reused. The original method populates m_model;
// inspect that exact object afterwards so no handle ABI or proxy assumptions
// leak into the replacement code.
static void TraceBasePartLoadUseHandleFinishCallback(void *self, bool success,
                                                      void *handle,
                                                      void *methodInfo) {
  auto original = (TraceBasePartLoadUseHandleFinishCallbackFn)
      s_origBasePartLoadUseHandleFinishCallback;
  if (original) original(self, success, handle, methodInfo);
  if (success)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart._OnLoadUseHandleFinishCallback",
        s_eiemEnclosingModelAssemblyDepth == 0);
}

static bool TraceBasePartLoadUseHandleFinish(void *self, bool success,
                                             void *handle,
                                             void *methodInfo) {
  auto original = (TraceBasePartLoadUseHandleFinishResultFn)
      s_origBasePartLoadUseHandleFinishResult;
  const bool result = original ? original(self, success, handle, methodInfo)
                               : false;
  if (result)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart._OnLoadUseHandleFinish",
        s_eiemEnclosingModelAssemblyDepth == 0);
  return result;
}

static void TraceBasePartReleaseModel(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::BaseModelPart, self,
                       "BaseModelViewPart.ReleaseModel");
  auto original =
      (TraceBasePartPostDealFn)s_origBasePartReleaseModel;
  if (original) original(self, methodInfo);
}

static void TraceBasePartOnRelease(void *self, void *methodInfo) {
  EiemForgetModelOwner(EiemModelOwnerKind::BaseModelPart, self,
                       "BaseModelViewPart.OnRelease");
  auto original = (TraceBasePartPostDealFn)s_origBasePartOnRelease;
  if (original) original(self, methodInfo);
}

#include "eiem_render_executor.h"

// The controller owns the game's RendererInfo cache.  This is observation only:
// it records the list passed to Init and the RendererInfo objects created by
// the original method, without changing either list or any Renderer state.
static void EiemLogMaterialControllerRegistry(void *controller,
                                               void *rendererList,
                                               const char *phase) {
  if (!controller) return;
  EiemRegistrationTraceNativeStackContext(
      phase ? phase : "material-controller", controller, rendererList,
      nullptr, InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  if (!kEiemEnableCustomSkinPipelineObservation) return;
  __try {
    const int listCount = TraceManagedListCount(rendererList);
    void *items = rendererList ? *(void **)((char *)rendererList + 0x10)
                                : nullptr;
    const size_t itemCount = EiemManagedArrayLength(items);
    Log("[RENDER-REG-v1] phase=%s controller=%p inputList=%p inputCount=%d "
        "inputArrayCount=%zu", phase ? phase : "unknown", controller,
        rendererList, listCount, itemCount);

    // Compare the exact native skin state immediately before and after the
    // game builds RendererInfo.  This is read-only and uses configured Mesh
    // identities only to keep the bounded probe relevant; bone resolution
    // itself never depends on Transform names.
    if (items && listCount > 0 && listCount <= 8192) {
      void **renderers = (void **)((char *)items + IL2CPP_ARRAY_DATA);
      const size_t rendererLimit =
          (std::min)((size_t)listCount, itemCount);
      for (size_t index = 0; index < rendererLimit; ++index) {
        void *renderer = renderers[index];
        if (!renderer) continue;
        const char *rendererType = nullptr;
        void *mesh = EiemReadLodRendererMesh(renderer, &rendererType);
        if (!EiemModEquals(rendererType, "SkinnedMeshRenderer")) continue;
        char source[768] = {};
        char asset[192] = {};
        if (!mesh ||
            !EiemReadLiveMeshIdentity(mesh, source, sizeof(source), asset,
                                      sizeof(asset)) ||
            !TraceIdentityTextMatchesConfiguredRule(asset))
          continue;
        if (InterlockedIncrement(&s_eiemMaterialBoundarySkinProbeCount) > 192)
          break;
        void *bones = g_smr_get_bones
                          ? EiemBackendInvokeNoThrow(g_smr_get_bones, renderer)
                          : nullptr;
        void *rootBone = g_smr_get_rootBone
                             ? EiemBackendInvokeNoThrow(g_smr_get_rootBone,
                                                        renderer)
                             : nullptr;
        void *skinningRoot =
            g_smr_get_skinningRoot
                ? EiemBackendInvokeNoThrow(g_smr_get_skinningRoot, renderer)
                : nullptr;
        Log("[MOD-SKIN-REGISTRY-BOUNDARY-v1] phase=%s controller=%p "
            "index=%zu renderer=%p mesh=%p asset=%s bones=%p count=%zu "
            "rootBone=%p skinningRoot=%p",
            phase ? phase : "unknown", controller, index, renderer, mesh,
            asset, bones, EiemManagedArrayLength(bones), rootBone,
            skinningRoot);
      }
    }

    // EntityRenderHelperMaterialController.m_rendererInfos is a List<RendererInfo>
    // at 0x10 in the current metadata dump.  Do not use it as a mutation point;
    // this snapshot only answers whether the game's cache sees our Renderer and
    // which Mesh/material arrays it retained after Init.
    void *infos = *(void **)((char *)controller + 0x10);
    const int infoCount = TraceManagedListCount(infos);
    void *infoItems = infos ? *(void **)((char *)infos + 0x10) : nullptr;
    const size_t infoArrayCount = EiemManagedArrayLength(infoItems);
    Log("[RENDER-REG-v1] phase=%s controller=%p infoList=%p infoCount=%d "
        "infoArrayCount=%zu", phase ? phase : "unknown", controller, infos,
        infoCount, infoArrayCount);

    if (!infoItems || infoCount <= 0 || infoCount > 512) return;
    void **entries = (void **)((char *)infoItems + IL2CPP_ARRAY_DATA);
    const size_t limit = (std::min)((size_t)infoCount, infoArrayCount);
    size_t logged = 0;
    for (size_t index = 0; index < limit; ++index) {
      void *info = entries[index];
      if (!info) continue;
      __try {
        // RendererInfo field layout is runtime-verified in the resource dump:
        // m_renderer=0x10, materialReplacing=0x38,
        // sourceMaterials=0x30, replacingMaterials=0x40.
        void *renderer = *(void **)((char *)info + 0x10);
        if (!renderer) continue;
        char path[768] = {};
        TraceBuildRendererHierarchy(renderer, path, sizeof(path));
        if (!path[0] ||
            (!strstr(path, "body_01") && !strstr(path, "cloth_01") &&
             !strstr(path, "cloth_02")))
          continue;
        void *currentMesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
        void *sourceMaterials = *(void **)((char *)info + 0x30);
        void *replacingMaterials = *(void **)((char *)info + 0x40);
        const bool materialReplacing =
            *(bool *)((char *)info + 0x38);
        bool enabled = true;
        bool visible = false;
        EiemReadRendererEnabled(renderer, &enabled);
        EiemReadRendererVisible(renderer, &visible);
        Log("[RENDER-REG-v1] phase=%s index=%zu info=%p renderer=%p "
            "path=%s mesh=%p enabled=%d visible=%d sourceMaterials=%zu "
            "replacingMaterials=%zu materialReplacing=%d", phase ? phase :
            "unknown", index, info, renderer, path, currentMesh,
            enabled ? 1 : 0, visible ? 1 : 0,
            EiemManagedArrayLength(sourceMaterials),
            EiemManagedArrayLength(replacingMaterials),
            materialReplacing ? 1 : 0);
        if (++logged >= 96) break;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("[RENDER-REG-v1] phase=%s index=%zu info=%p read=exception",
            phase ? phase : "unknown", index, info);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[RENDER-REG-v1] phase=%s controller=%p read=exception",
        phase ? phase : "unknown", controller);
  }
}

static void TraceEntityRenderHelperMaterialControllerInit(
    void *self, void *renderers, void *rendererTypeConfigs,
    void *customizeRendererPropertyConfig, bool calculateBoundsWithTransform,
    void *methodInfo) {
  auto original = (TraceEntityRenderHelperMaterialControllerInitFn)
      s_origEntityRenderHelperMaterialControllerInit;
  if (!self || s_eiemEntityRenderHelperMaterialInitGuard) {
    if (original)
      original(self, renderers, rendererTypeConfigs,
               customizeRendererPropertyConfig, calculateBoundsWithTransform,
               methodInfo);
    return;
  }
  s_eiemEntityRenderHelperMaterialInitGuard = true;
  // At this point the renderer list has been assembled and every native
  // SkinnedMeshRenderer observed in the list already has its instance-local
  // bones[] palette.  This is the common game-owned registration boundary:
  // commit the complete replacement transaction here, then let the original
  // method build RendererInfo/material/LOD/native skin state from it.  The
  // optional deferred branch remains source-compatible for evidence builds,
  // but production creation-boundary mode never queues a second live pass.
  EiemLogMaterialControllerRegistry(self, renderers, "before-original");
  if (s_eiemEntityRenderHelperActiveModel &&
      !s_eiemEntityRenderHelperMaterialApplied) {
    s_eiemEntityRenderHelperMaterialApplied =
        EiemApplyStandaloneRenderRules(
            s_eiemEntityRenderHelperActiveModel,
            "EntityRenderHelper.MaterialController.Init-before", nullptr,
            nullptr, nullptr);
    if (s_eiemEntityRenderHelperMaterialApplied)
      Log("[MOD-SKIN-COMMIT-v1] model=%p boundary=MaterialController.Init "
          "phase=before-original resourcesApplied=1",
          s_eiemEntityRenderHelperActiveModel);
  }
  if (original)
    original(self, renderers, rendererTypeConfigs,
             customizeRendererPropertyConfig, calculateBoundsWithTransform,
             methodInfo);
  EiemLogMaterialControllerRegistry(self, renderers, "after-original");
  s_eiemEntityRenderHelperMaterialInitGuard = false;
}

// EntityRenderHelper is the common game-owned registration boundary for the
// world, NPC and character-preview hierarchies. Runtime ordering shows that
// the game reaches it after PostDealLoadedModel (world) or SetSMRRootBone
// (NPC), while its original implementation has not yet constructed the
// RendererInfo/material/visibility/LOD registries. Commit one complete Mesh +
// bones transaction before that walk, then let the game build every downstream
// cache from the replacement generation. The guard only prevents callbacks
// caused by our own Unity setters; it does not suppress a helper merely because
// it is nested inside OnLoadFinish.
static void TraceEntityRenderHelperInitRenderAndMaterial(void *self,
                                                           void *methodInfo) {
  auto original = (TraceEntityRenderHelperInitFn)
      s_origEntityRenderHelperInitRenderAndMaterial;
  if (!self) {
    if (original) original(self, methodInfo);
    return;
  }
  // Setters issued by EIEM can re-enter the helper. Preserve the game's call,
  // but never start another replacement transaction from our own write.
  if (s_eiemApplyingModMeshAssignment) {
    if (original) original(self, methodInfo);
    return;
  }

  const bool outermost = !s_eiemEntityRenderHelperInitGuard;
  s_eiemEntityRenderHelperInitGuard = true;
  EiemAdoptUnityThreadFromAssemblyHook(
      "EntityRenderHelper._InitRenderAndMaterial");
  void *model = nullptr;
  if (g_component_get_gameObject)
    model = Invoke(g_component_get_gameObject, self);
  // The outer helper is only the transaction owner.  Its pre-original state
  // has empty bones[] (captured by MOD-SKIN-CAPTURE-v1), so applying there is
  // intentionally forbidden.  The nested MaterialController.Init hook above
  // performs the one commit after the native palettes exist.
  void *previousActiveModel = s_eiemEntityRenderHelperActiveModel;
  const bool previousMaterialApplied =
      s_eiemEntityRenderHelperMaterialApplied;
  const size_t firstMaterialReapply =
      s_eiemMaterialsToReapplyAfterHelper.size();
  s_eiemEntityRenderHelperActiveModel = model;
  s_eiemEntityRenderHelperMaterialApplied = false;
  if (original) original(self, methodInfo);
  const bool applied = s_eiemEntityRenderHelperMaterialApplied;
  // RendererInfo._Init can run before, inside, or after MaterialController.Init.
  // The outer helper is the only boundary that covers all three cases.
  size_t materialReapplied = 0;
  if (outermost) {
    for (size_t index = firstMaterialReapply;
         index < s_eiemMaterialsToReapplyAfterHelper.size(); ++index) {
      void *renderer = s_eiemMaterialsToReapplyAfterHelper[index];
      if (renderer && EiemNativeObjectStatus(renderer) == 1 &&
          EiemReapplyRendererMaterialsAfterCommit(
              renderer, "EntityRenderHelper.Init-after"))
        ++materialReapplied;
    }
    s_eiemMaterialsToReapplyAfterHelper.resize(firstMaterialReapply);
  }
  if (materialReapplied)
    Log("[MOD-MATERIAL] helper post-init restored renderers=%zu",
        materialReapplied);
  s_eiemEntityRenderHelperActiveModel = previousActiveModel;
  s_eiemEntityRenderHelperMaterialApplied = previousMaterialApplied;
  if (applied ||
      kEiemValidationIdentityProbe &&
      EiemRegistrationTraceFirst("entity-helper-init", "after", self,
                                 model, nullptr,
                                 InterlockedCompareExchange(
                                     &s_eiemModGeneration, 0, 0)))
    Log("[MOD-ASSEMBLY-v113] boundary=EntityRenderHelper._InitRenderAndMaterial "
        "helper=%p model=%p resourcesApplied=%d", self, model,
        applied ? 1 : 0);
  s_eiemEntityRenderHelperInitGuard = !outermost;
}

static bool EiemApplyStandaloneRenderRulesToRenderer(
    void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo, const char *stage) {
  (void)stage;
  if (!EiemOnUnityThread()) return false;
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  return EiemApplyRenderRuleSetToRenderer(
      nullptr, meshOwner, drawRenderer, mesh, rendererType, methodInfo, rules,
      "<mesh setter>", nullptr, nullptr, nullptr, nullptr);
}

// One native skin assembly call supplies the complete bone table through its
// rootBones argument while each Renderer keeps only a local palette. Keep the
// table scoped to the concrete renderer array, so world/NPC/UI instances never
// borrow one another's bones.
struct EiemAssemblyBoneSnapshot {
  void *rendererArray = nullptr;
  void *rootBonesArray = nullptr;
  LONG generation = -1;
  int32_t lod = -1;
  std::vector<EiemRootBoneInfoValue> rootInfos;
  std::vector<void *> renderers;
};
static SRWLOCK s_eiemAssemblyBoneLock = SRWLOCK_INIT;
static std::vector<EiemAssemblyBoneSnapshot> s_eiemAssemblyBoneSnapshots;

// Direct runtime association. The PFB/assembly array is not stable across
// LOD and presentation paths, while the Renderer instance is. Keep the
// game's completed bones[] keyed by that concrete Renderer pointer.
struct EiemRendererBoneSnapshot {
  void *renderer = nullptr;
  void *bones = nullptr;
  LONG generation = -1;
  int32_t lod = -1;
};
static std::vector<EiemRendererBoneSnapshot> s_eiemRendererBoneSnapshots;

static void EiemRememberAssemblyBoneSnapshot(void *renderers, void *rootBones,
                                             int32_t lod, LONG generation) {
  if (!renderers || !rootBones) {
    static LONG loggedNullInput = 0;
    if (InterlockedCompareExchange(&loggedNullInput, 1, 0) == 0)
      Log("[DEBUG-SNAPSHOT] reason=null-input renderers=%p rootBones=%p lod=%d generation=%ld",
          renderers, rootBones, lod, generation);
    return;
  }
  const size_t count = EiemManagedArrayLength(rootBones);
  if (!count || count > 16384) {
    static LONG loggedRootCount = 0;
    if (InterlockedCompareExchange(&loggedRootCount, 1, 0) == 0)
      Log("[DEBUG-SNAPSHOT] reason=root-count-invalid renderers=%p rootBones=%p rootCount=%zu lod=%d generation=%ld",
          renderers, rootBones, count, lod, generation);
    return;
  }
  const size_t rendererCount = EiemManagedArrayLength(renderers);
  if (rendererCount != count) {
    static LONG loggedCountMismatch = 0;
    if (InterlockedCompareExchange(&loggedCountMismatch, 1, 0) == 0)
      Log("[DEBUG-SNAPSHOT] reason=array-count-mismatch renderers=%p rendererCount=%zu rootBones=%p rootCount=%zu lod=%d generation=%ld",
          renderers, rendererCount, rootBones, count, lod, generation);
    return;
  }
  EiemAssemblyBoneSnapshot snapshot;
  snapshot.rendererArray = renderers;
  snapshot.rootBonesArray = rootBones;
  snapshot.generation = generation;
  snapshot.lod = lod;
  const size_t elementSize = EiemManagedArrayValueElementSize(rootBones, nullptr);
  if (elementSize < 16) {
    static LONG loggedElementSize = 0;
    if (InterlockedCompareExchange(&loggedElementSize, 1, 0) == 0)
      Log("[DEBUG-SNAPSHOT] reason=element-size-invalid renderers=%p count=%zu rootBones=%p elementSize=%zu lod=%d generation=%ld",
          renderers, count, rootBones, elementSize, lod, generation);
    return;
  }
  snapshot.rootInfos.resize(count);
  for (size_t index = 0; index < count; ++index)
    EiemReadRootBoneInfoAt(rootBones, index, elementSize, count,
                           &snapshot.rootInfos[index]);
  void **rendererItems = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  snapshot.renderers.assign(rendererItems, rendererItems + rendererCount);
  AcquireSRWLockExclusive(&s_eiemAssemblyBoneLock);
  auto found = std::find_if(s_eiemAssemblyBoneSnapshots.begin(),
                            s_eiemAssemblyBoneSnapshots.end(),
                            [&](const EiemAssemblyBoneSnapshot &value) {
                              return value.rendererArray == renderers;
                            });
  if (found == s_eiemAssemblyBoneSnapshots.end())
    s_eiemAssemblyBoneSnapshots.push_back(std::move(snapshot));
  else
    *found = std::move(snapshot);
  if (s_eiemAssemblyBoneSnapshots.size() > 128)
    s_eiemAssemblyBoneSnapshots.erase(s_eiemAssemblyBoneSnapshots.begin());
  ReleaseSRWLockExclusive(&s_eiemAssemblyBoneLock);
  Log("[MOD-SKIN-INSTANCE] array=%p rootBones=%p lod=%d bones=%zu generation=%ld",
      renderers, rootBones, lod, count, generation);

  if (g_smr_get_bones) {
    void **rendererItems = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
    AcquireSRWLockExclusive(&s_eiemAssemblyBoneLock);
    for (size_t index = 0; index < rendererCount; ++index) {
      void *renderer = rendererItems[index];
      if (!renderer) continue;
      void *bones = EiemBackendInvokeNoThrow(g_smr_get_bones, renderer);
      if (!bones || !EiemManagedArrayLength(bones)) continue;
      auto foundRenderer = std::find_if(
          s_eiemRendererBoneSnapshots.begin(), s_eiemRendererBoneSnapshots.end(),
          [&](const EiemRendererBoneSnapshot &value) {
            return value.renderer == renderer;
          });
      EiemRendererBoneSnapshot value{renderer, bones, generation, lod};
      if (foundRenderer == s_eiemRendererBoneSnapshots.end())
        s_eiemRendererBoneSnapshots.push_back(value);
      else
        *foundRenderer = value;
    }
    if (s_eiemRendererBoneSnapshots.size() > 4096)
      s_eiemRendererBoneSnapshots.erase(
          s_eiemRendererBoneSnapshots.begin(),
          s_eiemRendererBoneSnapshots.begin() + 1024);
    ReleaseSRWLockExclusive(&s_eiemAssemblyBoneLock);
  }
}

static bool EiemResolveMeshBonesFromAssembly(
    const EiemSkinIdentity &identity, void *renderer, void **out,
    char *error, size_t errorSize) {
  if (out) *out = nullptr;
  if (!renderer || (identity.sources.empty() && identity.sourceCandidates.empty()) ||
      !g_smr_get_bones ||
      !il2cpp_array_new || !g_transformClass)
    return false;
  auto reject = [&](const char *message) {
    if (error) strncpy_s(error, errorSize, message, _TRUNCATE);
    return false;
  };
  auto sourceMatchesMeshIdentity =
      [](const EiemSkinIdentity::Source &source, const char *meshPath,
         const char *meshAsset) {
        if (!source.meshAsset.empty())
          return meshAsset && meshAsset[0] &&
                 EiemModEquals(source.meshAsset.c_str(), meshAsset);
        return !source.meshPath.empty() && meshPath && meshPath[0] &&
               EiemModSameLogicalPath(source.meshPath.c_str(), meshPath);
      };
  void *targetRootBone = g_smr_get_rootBone
                             ? EiemBackendInvokeNoThrow(g_smr_get_rootBone,
                                                        renderer)
                             : nullptr;
  void *targetSkinningRoot = g_smr_get_skinningRoot
                                 ? EiemBackendInvokeNoThrow(
                                       g_smr_get_skinningRoot, renderer)
                                 : nullptr;
  auto sameSkeletonContext = [&](void *candidateRenderer) {
    if (!candidateRenderer) return false;
    if (g_smr_get_rootBone && targetRootBone) {
      void *candidateRoot = EiemBackendInvokeNoThrow(
          g_smr_get_rootBone, candidateRenderer);
      if (!candidateRoot || candidateRoot != targetRootBone) return false;
    }
    if (g_smr_get_skinningRoot && targetSkinningRoot) {
      void *candidateSkinningRoot = EiemBackendInvokeNoThrow(
          g_smr_get_skinningRoot, candidateRenderer);
      if (!candidateSkinningRoot ||
          candidateSkinningRoot != targetSkinningRoot)
        return false;
    }
    return true;
  };
  std::vector<void *> renderers;
  AcquireSRWLockShared(&s_eiemAssemblyBoneLock);
  for (const auto &snapshot : s_eiemAssemblyBoneSnapshots) {
    if (std::find(snapshot.renderers.begin(), snapshot.renderers.end(), renderer) ==
        snapshot.renderers.end()) continue;
    renderers = snapshot.renderers;
    break;
  }
  const bool strictCandidates =
      identity.sourceCandidates.size() == identity.paths.size() &&
      !identity.sourceCandidates.empty();
  if (renderers.empty()) {
    void *directBones = nullptr;
    for (const auto &entry : s_eiemRendererBoneSnapshots) {
      if (entry.renderer == renderer) {
        directBones = entry.bones;
        break;
      }
    }
    if (directBones) {
      std::vector<void *> resolved;
      void **items = (void **)((char *)directBones + IL2CPP_ARRAY_DATA);
      const size_t boneCount = EiemManagedArrayLength(directBones);
      if (strictCandidates) {
        void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
        char sourcePath[768] = {}, asset[192] = {};
        EiemReadLiveMeshIdentity(mesh, sourcePath, sizeof(sourcePath), asset,
                                 sizeof(asset));
        for (const auto &candidates : identity.sourceCandidates) {
          void *selected = nullptr;
          for (const auto &source : candidates) {
            const bool matches =
                sourceMatchesMeshIdentity(source, sourcePath, asset);
            if (!matches || source.slot >= boneCount) continue;
            void *bone = items[source.slot];
            if (!bone || EiemNativeObjectStatus(bone) != 1) continue;
            if (selected && selected != bone) {
              ReleaseSRWLockShared(&s_eiemAssemblyBoneLock);
              return reject("Replacement bone source candidates disagree in direct Renderer");
            }
            selected = bone;
          }
          if (!selected) {
            ReleaseSRWLockShared(&s_eiemAssemblyBoneLock);
            return reject("Replacement bone has no native Mesh donor in direct Renderer");
          }
          resolved.push_back(selected);
        }
      } else {
        for (const auto &source : identity.sources) {
          if (source.slot >= boneCount) {
            ReleaseSRWLockShared(&s_eiemAssemblyBoneLock);
            return reject("Source Mesh slot is absent from direct Renderer bones");
          }
          resolved.push_back(items[source.slot]);
        }
      }
      ReleaseSRWLockShared(&s_eiemAssemblyBoneLock);
      void *array = il2cpp_array_new(g_transformClass, resolved.size());
      if (!array) return reject("Unable to allocate direct Renderer bone palette");
      memcpy((char *)array + IL2CPP_ARRAY_DATA, resolved.data(),
             resolved.size() * sizeof(void *));
      if (out) *out = array;
      Log("[MOD-SKIN-%s] renderer=%p binding=direct-renderer slots=%zu",
          strictCandidates ? "V6" : "V5",
          renderer, resolved.size());
      return true;
    }
  }
  ReleaseSRWLockShared(&s_eiemAssemblyBoneLock);

  // The assembly snapshot is an observation channel.  Some native creation
  // paths (most notably the world model path) do not pass their Renderer
  // array through the snapshot hook, even though this model transaction has
  // already captured every original SkinnedMeshRenderer and its bones[].
  // Treating the optional snapshot as a prerequisite made every replacement
  // fail with "No assembly snapshot" and exposed the source Mesh.  Resolve
  // from the same model-local transaction instead.  This resolver uses only
  // EIEMESH v5/v6 source Mesh identity + original slot records and never
  // matches Transform names or borrows bones from another model instance.
  if (renderers.empty() && s_eiemLiveSkinSources) {
    char liveError[256] = {};
    if (EiemResolveMeshBonesFromNativeInstance(identity, renderer, out,
                                               liveError, sizeof(liveError))) {
      Log("[MOD-SKIN-ASSEMBLY] renderer=%p binding=model-transaction slots=%zu",
          renderer, identity.paths.size());
      return true;
    }
    return reject(liveError[0] ? liveError
                               : "No model-local native skeleton donor");
  }
  if (renderers.empty()) return reject("No assembly snapshot for Renderer instance");

  std::vector<void *> resolved;
  resolved.reserve(identity.paths.size());
  auto resolveCandidate = [&](const std::vector<EiemSkinIdentity::Source> &candidates,
                              void **selectedOut) -> bool {
    void *selected = nullptr;
    for (const auto &source : candidates) {
      for (void *candidate : renderers) {
        if (!sameSkeletonContext(candidate)) continue;
        void *mesh = EiemReadSharedMesh(candidate, "SkinnedMeshRenderer");
        char sourcePath[768] = {}, asset[192] = {};
        if (!EiemReadLiveMeshIdentity(mesh, sourcePath, sizeof(sourcePath),
                                      asset, sizeof(asset))) continue;
        if (!sourceMatchesMeshIdentity(source, sourcePath, asset)) continue;
        void *bones = EiemBackendInvokeNoThrow(g_smr_get_bones, candidate);
        const size_t boneCount = EiemManagedArrayLength(bones);
        if (!bones || source.slot >= boneCount) continue;
        void **items = (void **)((char *)bones + IL2CPP_ARRAY_DATA);
        void *bone = items[source.slot];
        if (!bone || EiemNativeObjectStatus(bone) != 1) continue;
        if (selected && selected != bone) return false;
        selected = bone;
      }
    }
    if (selectedOut) *selectedOut = selected;
    return selected != nullptr;
  };
  if (strictCandidates) {
    for (const auto &candidates : identity.sourceCandidates) {
      void *selected = nullptr;
      if (!resolveCandidate(candidates, &selected))
        return reject("Replacement bone has no unique native Mesh donor in assembly instance");
      resolved.push_back(selected);
    }
  } else for (const auto &source : identity.sources) {
    void *foundBones = nullptr;
    size_t count = 0;
    for (void *candidate : renderers) {
      if (!sameSkeletonContext(candidate)) continue;
      void *mesh = EiemReadSharedMesh(candidate, "SkinnedMeshRenderer");
      char sourcePath[768] = {}, asset[192] = {};
      if (!EiemReadLiveMeshIdentity(mesh, sourcePath, sizeof(sourcePath),
                                    asset, sizeof(asset))) continue;
      if (!sourceMatchesMeshIdentity(source, sourcePath, asset)) continue;
      void *bones = EiemBackendInvokeNoThrow(g_smr_get_bones, candidate);
      const size_t boneCount = EiemManagedArrayLength(bones);
      if (!bones || source.slot >= boneCount) continue;
      foundBones = bones;
      count = boneCount;
      break;
    }
    if (!foundBones) return reject("Source Mesh slot is absent from assembly instance");
    void **items = (void **)((char *)foundBones + IL2CPP_ARRAY_DATA);
    resolved.push_back(items[source.slot]);
  }
  void *array = il2cpp_array_new(g_transformClass, resolved.size());
  if (!array) return reject("Unable to allocate assembly bone palette");
  memcpy((char *)array + IL2CPP_ARRAY_DATA, resolved.data(),
         resolved.size() * sizeof(void *));
  if (out) *out = array;
  Log("[MOD-SKIN-%s] renderer=%p binding=assembly-source slots=%zu",
      strictCandidates ? "V6" : "V5", renderer, resolved.size());
  return true;
}
static SRWLOCK s_eiemModelInstanceLock = SRWLOCK_INIT;
static std::vector<EiemModelInstanceState> s_eiemModelInstances;
static bool EiemModelHasActiveOwner(const EiemModelInstanceState &state);

// Find the concrete BaseModelViewPart that owns one model instance. The
// timing probe receives the GameObject/model, while the game's parallel
// renderer caches live on the owning part. This correlation is read-only and
// stays instance-local; it never falls back to a scene-wide search.
static void *EiemFindBaseModelPartForModel(void *model) {
  if (!model) return nullptr;
  void *result = nullptr;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &state : s_eiemModelInstances) {
    if (state.model != model) continue;
    for (size_t index = 0; index < state.owners.size(); ++index) {
      const auto &owner = state.owners[index];
      if (owner.kind == EiemModelOwnerKind::BaseModelPart && owner.owner &&
          owner.active) {
        result = owner.owner;
        break;
      }
    }
    if (result) break;
  }
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  return result;
}

// Snapshot the game's own parallel renderer caches at the same cold/F10
// windows as the public SkinnedMeshRenderer probe. This is deliberately
// observation-only: no HG data getter, renderer setter, array mutation, or
// GPU request is made here. The first question is whether the object the game
// has registered for the draw is the same object we inspect through SMR.
static void EiemLogBaseModelCacheProbe(void *part, LONG transaction,
                                       const char *phase) {
  if (!kEiemEnableCustomSkinPipelineObservation || !part ||
      !EiemOnUnityThread())
    return;

  __try {
    void *model = TraceReadObjectField(part, s_basePartModelOffset);
    void *root = (model && g_gameObject_get_transform)
                     ? Invoke(g_gameObject_get_transform, model)
                     : nullptr;
    void *renderers =
        TraceReadObjectField(part, s_basePartRenderersOffset);
    void *rendererStates =
        TraceReadObjectField(part, s_basePartRenderersInitStateOffset);
    void *hgRenderers =
        TraceReadObjectField(part, s_basePartHgRenderersOffset);
    void *hgRendererStates =
        TraceReadObjectField(part, s_basePartHgRenderersInitStateOffset);
    void *meshes = TraceReadObjectField(part, s_basePartMeshesOffset);
    void *meshStates =
        TraceReadObjectField(part, s_basePartMeshesInitStateOffset);
    void *boneCloths =
        TraceReadObjectField(part, s_basePartBoneClothsOffset);
    void *lodGroups = TraceReadObjectField(part, s_basePartLodGroupsOffset);
    const size_t rendererCount = EiemManagedArrayLength(renderers);
    const size_t rendererStateCount = EiemManagedArrayLength(rendererStates);
    const size_t hgCount = EiemManagedArrayLength(hgRenderers);
    const size_t hgStateCount = EiemManagedArrayLength(hgRendererStates);
    const size_t meshCount = EiemManagedArrayLength(meshes);
    const size_t meshStateCount = EiemManagedArrayLength(meshStates);
    const size_t boneClothCount = EiemManagedArrayLength(boneCloths);
    const size_t lodCount = EiemManagedArrayLength(lodGroups);
    Log("[BASEMODEL-CACHE-v1] tx=%ld phase=%s part=%p model=%p "
        "renderers=%p/%zu rendererStates=%zu hgRenderers=%p/%zu "
        "hgStates=%zu meshes=%p/%zu meshStates=%zu boneCloths=%p/%zu "
        "lodGroups=%p/%zu",
        transaction, phase ? phase : "unknown", part, model, renderers,
        rendererCount, rendererStateCount, hgRenderers, hgCount,
        hgStateCount, meshes, meshCount, meshStateCount, boneCloths,
        boneClothCount, lodGroups, lodCount);

    void **meshItems = meshes
                           ? (void **)((char *)meshes + IL2CPP_ARRAY_DATA)
                           : nullptr;
    const size_t meshLimit = (std::min)(meshCount, (size_t)256);
    for (size_t index = 0; meshItems && index < meshLimit; ++index) {
      void *renderer = meshItems[index];
      if (!renderer) continue;
      char path[768] = {};
      if (root) EiemBuildRelativeRendererPath(root, renderer, path,
                                              sizeof(path));
      const bool target =
          path[0] && (strstr(path, "body_01") || strstr(path, "cloth_01") ||
                      strstr(path, "cloth_02"));
      if (!target) continue;
      void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
      void *bones = g_smr_get_bones ? Invoke(g_smr_get_bones, renderer)
                                    : nullptr;
      bool enabled = false, visible = false;
      const bool enabledRead = EiemReadRendererEnabled(renderer, &enabled);
      const bool visibleRead = EiemReadRendererVisible(renderer, &visible);
      size_t overrideIndex = SIZE_MAX;
      AcquireSRWLockShared(&s_eiemOverrideLock);
      overrideIndex = EiemFindOverrideLocked(renderer);
      ReleaseSRWLockShared(&s_eiemOverrideLock);
      const int init = EiemReadManagedBoolArrayValue(
          meshStates, meshStateCount, index);
      const auto bounds = EiemSkinProbe::ReadRendererBounds(renderer);
      Log("[BASEMODEL-CACHE-v1] tx=%ld phase=%s kind=mesh index=%zu "
          "renderer=%p path=%s mesh=%p bones=%zu override=%s init=%d "
          "enabled=%s visible=%s boundsRead=%d boundsCenterY=%.3f "
          "boundsMaxY=%.3f matrixRefs=%016llX",
          transaction, phase ? phase : "unknown", index, renderer,
          path[0] ? path : "<unknown>", mesh, EiemManagedArrayLength(bones),
          overrideIndex == SIZE_MAX ? "no" : "yes", init,
          enabledRead ? (enabled ? "1" : "0") : "?",
          visibleRead ? (visible ? "1" : "0") : "?", bounds.read ? 1 : 0,
          bounds.read ? bounds.CenterY() : 0.0f,
          bounds.read ? bounds.maxY : 0.0f,
          (unsigned long long)EiemSkinTimingBoneMatrixHash(bones));
    }

    void **hgItems = hgRenderers
                         ? (void **)((char *)hgRenderers + IL2CPP_ARRAY_DATA)
                         : nullptr;
    const size_t hgLimit = (std::min)(hgCount, (size_t)256);
    for (size_t index = 0; hgItems && index < hgLimit; ++index) {
      void *hg = hgItems[index];
      if (!hg) continue;
      char path[768] = {};
      if (root) EiemBuildRelativeRendererPath(root, hg, path, sizeof(path));
      const bool target =
          path[0] && (strstr(path, "body_01") || strstr(path, "cloth_01") ||
                      strstr(path, "cloth_02"));
      if (!target) continue;
      const int init = EiemReadManagedBoolArrayValue(
          hgRendererStates, hgStateCount, index);
      char description[384] = {};
      TraceDescribeObject(hg, description, sizeof(description));
      Log("[BASEMODEL-CACHE-v1] tx=%ld phase=%s kind=hg index=%zu hg=%p "
          "path=%s init=%d description=%s",
          transaction, phase ? phase : "unknown", index, hg,
          path[0] ? path : "<unknown>", init,
          description[0] ? description : "<unknown>");
    }

    void **clothItems = boneCloths
                            ? (void **)((char *)boneCloths + IL2CPP_ARRAY_DATA)
                            : nullptr;
    const size_t clothLimit = (std::min)(boneClothCount, (size_t)128);
    for (size_t index = 0; clothItems && index < clothLimit; ++index) {
      void *cloth = clothItems[index];
      if (!cloth) continue;
      char path[768] = {};
      if (root) EiemBuildRelativeRendererPath(root, cloth, path, sizeof(path));
      Log("[BASEMODEL-CACHE-v1] tx=%ld phase=%s kind=boneCloth index=%zu "
          "cloth=%p path=%s",
          transaction, phase ? phase : "unknown", index, cloth,
          path[0] ? path : "<unknown>");
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[BASEMODEL-CACHE-v1] tx=%ld phase=%s part=%p read=exception",
        transaction, phase ? phase : "unknown", part);
  }
}

// Read-only correlation from a source/Partner renderer back to the model
// registry. Partner creation can run before a lifecycle adapter publishes its
// owner; preserving that distinction is the point of this probe. No Unity
// object is touched and no registry entry is retained by the probe.
static void EiemTraceRendererOwnerCorrelation(void *renderer,
                                              uintptr_t modelKey,
                                              const char *stage,
                                              LONG generation) {
  if (!renderer) return;
  struct OwnerSnapshot {
    EiemModelOwnerKind kind = EiemModelOwnerKind::PrefabProxy;
    void *owner = nullptr;
  };
  std::vector<OwnerSnapshot> owners;
  size_t declaredOwnerCount = 0;
  bool active = false;
  uint32_t instanceUid = 0;
  char path[768] = {};
  bool found = false;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &state : s_eiemModelInstances) {
    if (!modelKey || state.model != (void *)modelKey) continue;
    found = true;
    declaredOwnerCount = state.owners.size();
    active = EiemModelHasActiveOwner(state);
    instanceUid = state.instanceUid;
    strncpy_s(path, sizeof(path), state.path, _TRUNCATE);
    owners.reserve(state.owners.size());
    for (const auto &owner : state.owners)
      owners.push_back({owner.kind, owner.owner});
    break;
  }
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);

  if (!found || owners.empty()) {
    EiemRegistrationTraceRendererOwner(
        renderer, (void *)modelKey, found ? "<model-without-owner>"
                                          : "<unregistered>",
        nullptr, instanceUid, declaredOwnerCount, active, path, stage,
        generation);
    return;
  }
  for (size_t index = 0; index < owners.size(); ++index)
    EiemRegistrationTraceRendererOwner(
        renderer, (void *)modelKey,
        EiemModelOwnerKindName(owners[index].kind), owners[index].owner,
        instanceUid, declaredOwnerCount, active, path, stage, generation);
}

static bool EiemModelHasActiveOwner(const EiemModelInstanceState &state) {
  for (size_t index = 0; index < state.owners.size(); ++index)
    if (state.owners[index].active) return true;
  return false;
}

static void EiemStoreModelPhysicsIntents(
    void *model, std::vector<EiemPhysicsIntent> intents, const char *stage) {
  if (!model) return;
  const auto currentIntents = intents;
  const size_t current = intents.size();
  size_t previous = 0;
  bool found = false;
  bool active = false;
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (auto &state : s_eiemModelInstances) {
    if (state.model != model) continue;
    previous = state.physicsIntents.size();
    state.physicsIntents = std::move(intents);
    active = EiemModelHasActiveOwner(state);
    found = true;
    break;
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  if (found && (previous || current))
    Log("[PHYSICS-PLAN] model=%p previous=%zu current=%zu active=%d stage=%s",
        model, previous, current, active ? 1 : 0,
        stage ? stage : "unknown");
  if (found) {
    // A previous Physics replacement may have been waiting for Unity's
    // deferred Destroy. Collect it before comparing this new intent, then
    // reconcile exactly once for this model boundary.
    if (!s_eiemPhysicsLifecycleTransaction) EiemPhysicsRuntimeBoundary(stage);
    EiemReconcileModelPhysics(model, currentIntents, active, stage);
    if (!s_eiemPhysicsLifecycleTransaction) EiemPhysicsRuntimeBoundary(stage);
  }
}

static void EiemSetModelOwnerActive(EiemModelOwnerKind ownerKind, void *owner,
                                    bool active, const char *stage) {
  if (!owner) return;
  size_t changed = 0;
  size_t matched = 0;
  size_t planned = 0;
  struct PhysicsOwnerState {
    void *model = nullptr;
    std::vector<EiemPhysicsIntent> intents;
    bool active = false;
  };
  std::vector<PhysicsOwnerState> physicsStates;
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (auto &state : s_eiemModelInstances) {
    for (size_t index = 0; index < state.owners.size(); ++index) {
      auto &candidate = state.owners[index];
      if (candidate.kind != ownerKind || candidate.owner != owner) continue;
      ++matched;
      if (candidate.active != active) {
        candidate.active = active;
        ++changed;
        if (!state.physicsIntents.empty())
          physicsStates.push_back({state.model, state.physicsIntents, active});
      }
      planned += state.physicsIntents.size();
    }
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  EiemRegistrationTraceOwnerState(
      EiemModelOwnerKindName(ownerKind), owner, nullptr, active, stage,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  if (changed || planned)
    Log("[PHYSICS-PLAN] owner=%p active=%d models=%zu changed=%zu intents=%zu stage=%s",
        owner, active ? 1 : 0, matched, changed, planned,
        stage ? stage : "unknown");
  for (const auto &physics : physicsStates)
    EiemReconcileModelPhysics(physics.model, physics.intents, physics.active,
                              stage);
  if (!physicsStates.empty() && !s_eiemPhysicsLifecycleTransaction)
    EiemPhysicsRuntimeBoundary(stage);
}

static bool EiemSameRelativePath(const char *left, const char *right) {
  const bool leftEmpty = !left || !left[0];
  const bool rightEmpty = !right || !right[0];
  return leftEmpty || rightEmpty ? leftEmpty == rightEmpty
                                 : EiemModSameLogicalPath(left, right);
}

static bool EiemBuildRelativeRendererPath(void *rootTransform, void *renderer,
                                          char *out, size_t outSize) {
  if (!rootTransform || !renderer || !out || !outSize ||
      !g_component_get_transform || !g_transform_get_parent ||
      !g_object_get_name)
    return false;
  out[0] = '\0';
  char names[64][96] = {};
  size_t count = 0;
  void *transform = Invoke(g_component_get_transform, renderer);
  while (transform && transform != rootTransform && count < _countof(names)) {
    void *name = Invoke(g_object_get_name, transform);
    if (name) ReadStrUtf8(name, names[count], sizeof(names[count]));
    if (!names[count][0]) return false;
    ++count;
    transform = Invoke(g_transform_get_parent, transform);
  }
  if (transform != rootTransform) return false;
  size_t used = 0;
  for (size_t index = count; index > 0; --index) {
    const char *name = names[index - 1];
    const size_t length = strlen(name);
    if (used + (used ? 1 : 0) + length + 1 > outSize) return false;
    if (used) out[used++] = '/';
    memcpy(out + used, name, length);
    used += length;
    out[used] = '\0';
  }
  return true;
}

static bool EiemRenderRuleMatches(const EiemModRule &rule,
                                  const char *relativePath, void *mesh,
                                  const char *asset) {
  // A source Render must identify a node path, a Mesh sub-asset, or both.
  // Selector-free Render sections are valid only as partner declarations.
  if (!rule.path[0] && !rule.asset[0]) return false;
  if (rule.path[0] && !EiemSameRelativePath(rule.path, relativePath))
    return false;
  if (rule.asset[0] && (!asset || !EiemModEquals(rule.asset, asset)))
    return false;
  if (rule.matchVertices >= 0 || rule.matchIndices >= 0 ||
      rule.matchSubMeshes >= 0) {
    int32_t vertices = -1, indices = -1, subMeshes = -1;
    EiemReadLiveMeshShape(mesh, &vertices, &indices, &subMeshes);
    if (rule.matchVertices >= 0 && rule.matchVertices != vertices) return false;
    if (rule.matchIndices >= 0 && rule.matchIndices != indices) return false;
    if (rule.matchSubMeshes >= 0 && rule.matchSubMeshes != subMeshes)
      return false;
  }
  return true;
}

static bool EiemRegisterAndApplyModelInstance(
    EiemModelOwnerKind ownerKind, void *owner, void *model,
    const char *prefabPath, uint32_t instanceUid, const char *stage,
    bool applyResources) {
  if (!model) return false;
  EiemPerfScope perfScope(s_eiemPerfModelRegistration);
  auto modelRef = EiemUnityRef::Capture(model);
  if (!modelRef) {
    Log("[MOD-LIFECYCLE] Cannot observe model lifetime model=%p stage=%s", model, stage);
    return false;
  }

  std::vector<uintptr_t> releasedModels;
  uint32_t ownerCountSnapshot = 0;
  bool ownerActiveSnapshot = false;
  const LONG generationSnapshot =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  // A PrefabInstantiateProxy or BaseModelViewPart owns one live result at a
  // time. UIModelLoader is intentionally different: one loader can own many
  // preview instances.
  if (owner && ownerKind != EiemModelOwnerKind::UIModelLoader) {
    for (size_t stateIndex = 0; stateIndex < s_eiemModelInstances.size();) {
      auto &entry = s_eiemModelInstances[stateIndex];
      if (entry.model == model) {
        ++stateIndex;
        continue;
      }
      for (size_t ownerIndex = 0; ownerIndex < entry.owners.size();
           ++ownerIndex) {
        if (entry.owners[ownerIndex].kind != ownerKind ||
            entry.owners[ownerIndex].owner != owner)
          continue;
        entry.owners.erase(entry.owners.begin() + ownerIndex);
        break;
      }
      if (entry.owners.empty()) {
        releasedModels.push_back((uintptr_t)entry.model);
        s_eiemModelInstances.erase(s_eiemModelInstances.begin() + stateIndex);
      } else {
        ++stateIndex;
      }
    }
  }
  size_t slot = SIZE_MAX;
  for (size_t index = 0; index < s_eiemModelInstances.size(); ++index) {
    if (s_eiemModelInstances[index].model == model) {
      if (s_eiemModelInstances[index].modelRef.Target() != model) {
        // Wrapper addresses can be reused; do not inherit stale owners.
        releasedModels.push_back((uintptr_t)model);
        s_eiemModelInstances.erase(s_eiemModelInstances.begin() + index);
        break;
      }
      slot = index;
      break;
    }
  }
  if (slot == SIZE_MAX) {
    EiemModelInstanceState state = {};
    state.model = model;
    state.modelRef = modelRef;
    s_eiemModelInstances.push_back(state);
    slot = s_eiemModelInstances.size() - 1;
  }
  auto &state = s_eiemModelInstances[slot];
  if (instanceUid) state.instanceUid = instanceUid;
  if (prefabPath && prefabPath[0])
    strncpy_s(state.path, sizeof(state.path), prefabPath, _TRUNCATE);
  if (owner) {
    bool knownOwner = false;
    for (size_t index = 0; index < state.owners.size(); ++index) {
      if (state.owners[index].kind == ownerKind &&
          state.owners[index].owner == owner) {
        state.owners[index].active = true;
        knownOwner = true;
        break;
      }
    }
    if (!knownOwner) state.owners.push_back({ownerKind, owner, true});
  }
  ownerCountSnapshot = (uint32_t)state.owners.size();
  ownerActiveSnapshot = EiemModelHasActiveOwner(state);
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  for (uintptr_t released : releasedModels) {
    EiemReleaseModelPhysics((void *)released, "owner moved to another model");
    EiemRegistrationTraceRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)released,
        "owner moved to another model", generationSnapshot);
    EiemForgetRenderOverrides(released);
  }
  // Existence is recorded before consulting the current program. An empty
  // INI must not make an already-created model undiscoverable at the next F10.
  if (modelRef.Status() != 1) return false;
  bool applied = false;
  std::vector<EiemPhysicsIntent> physicsIntents;
  if (applyResources && EiemHasStandaloneRenderRules())
    applied = EiemApplyStandaloneRenderRules(model, stage, nullptr, nullptr,
                                             &physicsIntents);
  EiemRegistrationTraceModel(
      EiemModelOwnerKindName(ownerKind), owner, model, stage,
      generationSnapshot, ownerCountSnapshot, ownerActiveSnapshot, applied,
      -1, -1, prefabPath);
  EiemStoreModelPhysicsIntents(model, std::move(physicsIntents), stage);
  return applied;
}

// BaseModelViewPart is the game's confirmed character-model completion owner.
// In particular, its handle path reuses an already loaded model without
// creating another PrefabInstantiateProxy. Read the exact model and logical
// path held by that part; never infer identity from a scene-wide Mesh scan.
static bool EiemRegisterBaseModelViewPartInstance(void *part,
                                                   const char *stage,
                                                   bool applyResources) {
  if (!part) return false;
  void *model = TraceReadObjectField(part, s_basePartModelOffset);
  if (!model) return false;

  char path[768] = {};
  const int configPathOffset =
      (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
          ? s_basePartConfigOffset + s_basePartConfigPathOffset
          : -1;
  TraceReadStringField(part, configPathOffset, path, sizeof(path));
  if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));

  const bool applied = EiemRegisterAndApplyModelInstance(
      EiemModelOwnerKind::BaseModelPart, part, model,
      path[0] ? path : nullptr, 0, stage, applyResources);
  if (applied || kEiemValidationIdentityProbe)
    Log("[MOD-MODEL-PART] completed part=%p path=%s model=%p applied=%d stage=%s",
        part, path, model, applied ? 1 : 0, stage ? stage : "unknown");
  return applied;
}

// CharUIModelMono is attached directly to the UI presentation hierarchy. Its
// own GameObject is therefore a sufficient lifecycle root for Mesh-identity
// rules; no PFB name inference or scene-wide search is needed.
static bool EiemRegisterCharUIModelInstance(void *component,
                                             const char *stage,
                                             bool applyResources) {
  if (!component || !g_component_get_gameObject)
    return false;
  void *model = Invoke(g_component_get_gameObject, component);
  const bool applied = EiemRegisterAndApplyModelInstance(
      EiemModelOwnerKind::CharUIModel, component, model, nullptr, 0, stage,
      applyResources);
  if (applied || kEiemValidationIdentityProbe)
    Log("[MOD-CHAR-UI] completed component=%p model=%p applied=%d stage=%s",
        component, model, applied ? 1 : 0, stage ? stage : "unknown");
  return applied;
}

static bool EiemReapplyRegisteredModelInstance(void *model,
                                                const char *stage) {
  if (!model) return false;
  EiemModelInstanceState state = {};
  bool found = false;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &entry : s_eiemModelInstances) {
    if (entry.model == model) {
      state = entry;
      found = true;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  if (!found || state.modelRef.Status() != 1) return false;
  std::vector<EiemPhysicsIntent> physicsIntents;
  bool applied = EiemApplyStandaloneRenderRules(model, stage, nullptr, nullptr,
                                                &physicsIntents);
  EiemStoreModelPhysicsIntents(model, std::move(physicsIntents), stage);
  return applied;
}

static void EiemForgetModelOwner(EiemModelOwnerKind ownerKind, void *owner,
                                 const char *stage) {
  if (!owner) return;
  std::vector<uintptr_t> releasedModels;
  struct PhysicsOwnerState {
    void *model = nullptr;
    std::vector<EiemPhysicsIntent> intents;
    bool active = false;
  };
  std::vector<PhysicsOwnerState> physicsStates;
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (size_t stateIndex = 0; stateIndex < s_eiemModelInstances.size();) {
    auto &state = s_eiemModelInstances[stateIndex];
    const bool wasActive = EiemModelHasActiveOwner(state);
    bool removed = false;
    for (size_t ownerIndex = 0; ownerIndex < state.owners.size();
         ++ownerIndex) {
      if (state.owners[ownerIndex].kind != ownerKind ||
          state.owners[ownerIndex].owner != owner)
        continue;
      state.owners.erase(state.owners.begin() + ownerIndex);
      removed = true;
      break;
    }
    if (removed && !state.owners.empty() && wasActive != EiemModelHasActiveOwner(state) &&
        !state.physicsIntents.empty())
      physicsStates.push_back(
          {state.model, state.physicsIntents, EiemModelHasActiveOwner(state)});
    if (state.owners.empty()) {
      releasedModels.push_back((uintptr_t)state.model);
      s_eiemModelInstances.erase(s_eiemModelInstances.begin() + stateIndex);
    } else {
      ++stateIndex;
    }
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  for (uintptr_t modelOwner : releasedModels) {
    EiemReleaseModelPhysics((void *)modelOwner, stage);
    EiemRegistrationTraceRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)modelOwner, stage,
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
    EiemForgetRenderOverrides(modelOwner);
  }
  for (const auto &physics : physicsStates)
    EiemReconcileModelPhysics(physics.model, physics.intents, physics.active,
                              stage);
  if ((!releasedModels.empty() || !physicsStates.empty()) &&
      !s_eiemPhysicsLifecycleTransaction)
    EiemPhysicsRuntimeBoundary(stage);
}

static void EiemForgetModelInstance(void *model, const char *stage) {
  if (!model) return;
  bool removed = false;
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (size_t index = 0; index < s_eiemModelInstances.size(); ++index) {
    if (s_eiemModelInstances[index].model != model) continue;
    s_eiemModelInstances.erase(s_eiemModelInstances.begin() + index);
    removed = true;
    break;
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  if (!removed) return;
  const uintptr_t modelOwner = (uintptr_t)model;
  EiemReleaseModelPhysics(model, stage);
  EiemRegistrationTraceRelease(
      "model", nullptr, model, stage,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemForgetRenderOverrides(modelOwner);
  Log("[MOD-LIFECYCLE] released model=%p stage=%s", model,
      stage ? stage : "unknown");
  if (!s_eiemPhysicsLifecycleTransaction) EiemPhysicsRuntimeBoundary(stage);
}

static void EiemPruneModelInstances() {
  std::vector<EiemModelInstanceState> observed;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  observed = s_eiemModelInstances;
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &state : observed)
    if (state.modelRef.Status() == 0)
      EiemForgetModelInstance(state.model, "observed model expired");
}

// Endfield owns source/replacement material arrays in
// EntityRenderHelperMaterialController.RendererInfo.  Scene transitions can
// commit those arrays after a prefab and its EIEM Render rule have completed.
// Mesh identity remains the rule key, so enforce only the material portion at
// that game-owned final commit boundary.  This is not a UI-specific rule and
// does not re-run mesh, skip, or partner actions.
static bool EiemIsSkinnedRenderer(void *renderer) {
  if (!renderer || !il2cpp_object_get_class || !g_skinnedMeshRendererClass)
    return false;
  void *klass = nullptr;
  __try { klass = il2cpp_object_get_class(renderer); }
  __except (EXCEPTION_EXECUTE_HANDLER) { klass = nullptr; }
  for (int depth = 0; klass && depth < 10; ++depth) {
    if (klass == g_skinnedMeshRendererClass) return true;
    klass = il2cpp_class_get_parent ? il2cpp_class_get_parent(klass) : nullptr;
  }
  return false;
}

static bool EiemIsKnownSourceRenderer(void *renderer) {
  if (!renderer) return false;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  bool known = false;
  for (const auto &state : s_eiemPartners) {
    if (state.sourceRenderer == renderer) {
      known = true;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  return known;
}

// LODGroup stores Renderer references, while a MeshRenderer's Mesh lives on
// the sibling MeshFilter. Resolve that identity only for the diagnostic dump;
// this helper never writes either component.
static void *EiemReadLodRendererMesh(void *renderer,
                                     const char **rendererTypeOut) {
  if (rendererTypeOut) *rendererTypeOut = "Renderer";
  if (!renderer) return nullptr;
  if (EiemIsSkinnedRenderer(renderer)) {
    if (rendererTypeOut) *rendererTypeOut = "SkinnedMeshRenderer";
    return EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
  }
  if (rendererTypeOut) *rendererTypeOut = "MeshRenderer";
  if (!g_component_get_gameObject || !g_gameObject_GetComponent ||
      !g_meshFilterClass || !il2cpp_class_get_type ||
      !il2cpp_type_get_object)
    return nullptr;
  __try {
    void *gameObject = Invoke(g_component_get_gameObject, renderer);
    void *type = il2cpp_class_get_type(g_meshFilterClass);
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!gameObject || !typeObject) return nullptr;
    void *params[] = {typeObject};
    void *meshFilter = Invoke(g_gameObject_GetComponent, gameObject, params);
    return EiemReadSharedMesh(meshFilter, "MeshFilter");
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static void EiemTraceLodGroupMembers(void *group, void *lods,
                                     size_t lodCount, LONG generation) {
  if (!group || !lods || lodCount == 0 || lodCount > 64) return;
  __try {
    char *lodData = (char *)lods + IL2CPP_ARRAY_DATA;
    size_t logged = 0;
    constexpr size_t kMaxMembersPerSet = 1024;
    for (size_t lodIndex = 0;
         lodIndex < lodCount && logged < kMaxMembersPerSet; ++lodIndex) {
      EiemNativeLod *lod =
          (EiemNativeLod *)(lodData + lodIndex * sizeof(EiemNativeLod));
      void *renderers = lod->renderers;
      const size_t rendererCount = EiemManagedArrayLength(renderers);
      if (!renderers || rendererCount > 4096) continue;
      void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
      for (size_t index = 0;
           index < rendererCount && logged < kMaxMembersPerSet; ++index) {
        void *renderer = items[index];
        if (!renderer) continue;
        bool active = true;
        bool activeRead = false;
        if (g_component_get_gameObject && g_gameObject_get_activeInHierarchy) {
          void *gameObject = Invoke(g_component_get_gameObject, renderer);
          activeRead = EiemReadBoxedBool(
              g_gameObject_get_activeInHierarchy, gameObject, &active);
        }
        bool enabled = true;
        const bool enabledRead = EiemReadRendererEnabled(renderer, &enabled);
        bool forceRenderingOff = false;
        const bool forceRead =
            EiemReadRendererForceRenderingOff(renderer, &forceRenderingOff);
        const char *rendererType = "Renderer";
        void *mesh = EiemReadLodRendererMesh(renderer, &rendererType);
        char objectName[160] = {};
        TraceReadUnityObjectName(renderer, objectName, sizeof(objectName));
        EiemRegistrationTraceLodGroupMember(
            group, lodIndex, renderer, mesh, rendererType, objectName,
            EiemIsKnownSourceRenderer(renderer), EiemIsPartnerRenderer(renderer),
            active, activeRead, enabled, enabledRead, forceRenderingOff,
            forceRead, generation);
        ++logged;
      }
    }
    if (logged == kMaxMembersPerSet)
      Log("%s event=lod-group-member-truncated group=%p lodCount=%zu "
          "limit=%zu generation=%ld",
          EiemRegistrationTraceTag, group, lodCount, kMaxMembersPerSet,
          generation);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("%s event=lod-group-member-failed group=%p lods=%p lodCount=%zu "
        "generation=%ld exception=0x%08lX",
        EiemRegistrationTraceTag, group, lods, lodCount, generation,
        GetExceptionCode());
  }
}

static bool EiemReapplyRendererMaterialsAfterCommit(void *renderer,
                                                     const char *stage) {
  if (!kEiemEnableMaterialLifecycle) return false;
  if (!renderer || EiemMaterialSourceInitActive(renderer) ||
      !EiemOnUnityThread() || !EiemIsSkinnedRenderer(renderer))
    return false;

  void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
  if (!mesh) return false;
  EiemResolvedRenderRule resolved = {};
  if (!EiemFindBoundRenderRule(renderer, &resolved.rule) ||
      (!resolved.rule.materialCount && !resolved.rule.submeshCount))
    return false;
  TraceReadUnityObjectName(mesh, resolved.asset, sizeof(resolved.asset));

  char error[256] = {};
  void *materials = nullptr;
  if (!EiemBuildRendererMaterialsForSource(resolved.rule, renderer, &materials,
                                           error, sizeof(error)) ||
      !materials) {
    Log("[MOD-MATERIAL-COMMIT] rebuild failed: stage=%s renderer=%p asset=%s error=%s",
        stage ? stage : "unknown", renderer, resolved.asset,
        error[0] ? error : "unknown");
    return false;
  }

  void *committed = g_renderer_get_sharedMaterials
                        ? Invoke(g_renderer_get_sharedMaterials, renderer)
                        : nullptr;
  if (!EiemManagedObjectArraySame(materials, committed) &&
      (!EiemCaptureOriginal(renderer, renderer, mesh, "SkinnedMeshRenderer", false, &resolved.rule) ||
       !EiemAssignRendererMaterials(renderer, materials, error, sizeof(error)))) {
    Log("[MOD-MATERIAL-COMMIT] assignment failed: stage=%s renderer=%p asset=%s error=%s",
        stage ? stage : "unknown", renderer, resolved.asset,
        error[0] ? error : "unknown");
    return false;
  }

  if (TraceTakeBudget(&s_traceMaterialCommitCount, 160))
    Log("[DEBUG-matlifecycle] stage=%s renderer=%p asset=%s materials=%u action=%s",
        stage ? stage : "unknown", renderer, resolved.asset,
        resolved.rule.materialCount,
        EiemManagedObjectArraySame(materials, committed) ? "retained" :
                                                          "reapplied");
  return true;
}

static void *EiemReadRendererFromMaterialInfo(void *rendererInfo) {
  if (!rendererInfo || s_materialRendererInfoRendererOffset < 0)
    return nullptr;
  __try {
    return *(void **)((char *)rendererInfo +
                      s_materialRendererInfoRendererOffset);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static void TraceMaterialInfoInit(void *self, void *renderer, void *configs, void *methodInfo) {
  using InitFn = void (*)(void *, void *, void *, void *);
  auto original = (InitFn)s_origMaterialInfoInit;
  if (!kEiemEnableMaterialLifecycle) {
    if (original) original(self, renderer, configs, methodInfo);
    return;
  }
  bool sourceReady = false;
  {
    EiemMaterialSourceInitScope scope(renderer);
    sourceReady = EiemExposeSourceMaterialsForInit(renderer);
    // Never skip game initialization, including when source exposure failed.
    if (original) original(self, renderer, configs, methodInfo);
  }
  if (sourceReady) {
    // EntityRenderHelper owns a complete renderer-registration pass.  Do not
    // replace one Mesh while that pass is still iterating: later game systems
    // can otherwise cache a mixture of source and replacement generations.
    // The enclosing hook commits all rules once the original pass returns.
    // Standalone RendererInfo initialization remains the verified fallback for
    // NPC/UI paths which have no enclosing EntityRenderHelper boundary.
    if (s_eiemEntityRenderHelperInitGuard) {
      if (std::find(s_eiemMaterialsToReapplyAfterHelper.begin(),
                    s_eiemMaterialsToReapplyAfterHelper.end(), renderer) ==
          s_eiemMaterialsToReapplyAfterHelper.end())
        s_eiemMaterialsToReapplyAfterHelper.push_back(renderer);
      return;
    }
    // Without an enclosing helper this per-Renderer callback is the completed
    // boundary available to direct NPC/UI construction. Apply in-place
    // resource/material actions here.
    bool applied = false;
    if (EiemIsSkinnedRenderer(renderer)) {
      void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
      if (mesh)
        applied = EiemApplyStandaloneRenderRulesToRenderer(
            renderer, renderer, mesh, "SkinnedMeshRenderer", methodInfo,
            "RendererInfo._Init");
    }
    // Path-qualified rules cannot be newly resolved without a model root, but
    // a previously bound Renderer still needs its material slots restored
    // after the controller has refreshed them.
    if (!applied)
      EiemReapplyRendererMaterialsAfterCommit(renderer, "RendererInfo._Init");
    else {
      Log("[MOD-RENDERER-INIT] global rule applied renderer=%p", renderer);
      EiemRegistrationTraceRenderer(
          renderer, "RendererInfo._Init",
          InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
    }
  } else
    Log("[MOD-MATERIAL-SOURCE] init source unresolved; mod reapply not attempted renderer=%p", renderer);
}

static bool TraceRendererInfoTrySetSharedMaterial(void *self, void *material,
                                                  void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTrySetSharedMaterial;
  const bool result = original ? original(self, material, methodInfo) : false;
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TrySetSharedMaterial");
  return result;
}

static bool TraceRendererInfoTrySetSharedMaterials(void *self,
                                                   void *materials,
                                                   void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTrySetSharedMaterials;
  const bool result = original ? original(self, materials, methodInfo) : false;
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TrySetSharedMaterials");
  return result;
}

static bool TraceRendererInfoTryReplaceSharedMaterials(void *self,
                                                       void *materials,
                                                       void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTryReplaceSharedMaterials;
  const bool result = original ? original(self, materials, methodInfo) : false;
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TryReplaceSharedMaterials");
  return result;
}

// Legacy LOD-group observation retained only for archived diagnostics. The
// production hook is no longer installed.
static void TraceLodGroupSetLODs(void *self, void *lods, void *methodInfo) {
  auto original = (TraceLodGroupSetLodsFn)s_origLodGroupSetLODs;
  if (original)
    original(self, lods, methodInfo);
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  const size_t lodCount = EiemManagedArrayLength(lods);
  EiemRegistrationTraceLodGroupSet(
      self, lods, lodCount, s_eiemApplyingPartnerLod, generation);
  EiemTraceLodGroupMembers(self, lods, lodCount, generation);
  EiemReconcilePartnerLodGroup(self, lods, lodCount);
}

// Read-only evidence for the concrete Renderer objects that the world skin
// assembly passes to CreateSMSGO/AssignSkinGo. Keep this validation-only,
// bounded and de-duplicated so normal world traffic cannot become a per-frame
// logger.
static void EiemTraceSkinArrayItems(const char *boundary, void *owner,
                                    void *array, int32_t lod,
                                    LONG generation) {
  if (!kEiemValidationIdentityProbe) return;
  const size_t count = EiemManagedArrayLength(array);
  if (!array || count < 2 || count > 64) return;
  const void *lodKey = (const void *)(intptr_t)lod;
  if (!EiemRegistrationTraceFirst("skin-array-items", boundary, array, owner,
                                  (void *)lodKey, generation))
    return;

  __try {
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < count; ++index) {
      void *renderer = items[index];
      if (!renderer) continue;
      const char *rendererType = "Renderer";
      void *mesh = EiemReadLodRendererMesh(renderer, &rendererType);
      char rendererName[160] = {};
      char meshDescription[256] = {};
      char source[768] = {};
      char asset[192] = {};
      char hierarchy[512] = {};
      TraceReadUnityObjectName(renderer, rendererName,
                               sizeof(rendererName));
      TraceDescribeObject(mesh, meshDescription, sizeof(meshDescription));
      EiemReadLiveMeshIdentity(mesh, source, sizeof(source), asset,
                               sizeof(asset));
      TraceBuildRendererHierarchy(renderer, hierarchy, sizeof(hierarchy));
      Log("%s event=skin-array-item boundary=%s owner=%p array=%p lod=%d "
          "index=%zu renderer=%p type=%s rendererName=%s mesh=%p "
          "asset=%s meshDescription=%s sourceKnown=%d "
          "hierarchy=%s generation=%ld",
          EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner,
          array, lod, index, renderer,
          rendererType && rendererType[0] ? rendererType : "Renderer",
          rendererName[0] ? rendererName : "<unknown>", mesh,
          asset[0] ? asset : "<unknown>",
          meshDescription[0] ? meshDescription : "<unknown>",
          EiemIsKnownSourceRenderer(renderer) ? 1 : 0,
          hierarchy[0] ? hierarchy : "<unknown>", generation);
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("%s event=skin-array-items-failed boundary=%s owner=%p array=%p "
        "count=%zu lod=%d generation=%ld exception=0x%08lX",
        EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner,
        array, count, lod, generation, GetExceptionCode());
  }
}

// These methods expose the separate NPC avatar construction order for
// diagnostics. Character model replacement is owned by the model/PFB
// lifecycle above; NPC activity alone is not evidence that a UI character
// presentation uses this pipeline.
static void TraceAssignSkinGo(int32_t lod, void *renderers,
                              void *rootBones, void *closure,
                              void *methodInfo) {
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinGoPre", nullptr, renderers, EiemManagedArrayLength(renderers),
      generation, lod);
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinGo;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  EiemRememberAssemblyBoneSnapshot(
      renderers, rootBones,
      lod, InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  const LONG afterGeneration =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinGoPost", nullptr, renderers,
      EiemManagedArrayLength(renderers), afterGeneration, lod);
  EiemTraceSkinArrayItems("AssignSkinGoPost", nullptr, renderers, lod,
                          afterGeneration);
}

static void TraceAssignSkinPost(int32_t lod, void *renderers,
                                void *rootBones, void *closure,
                                void *methodInfo) {
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinPost;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  EiemRememberAssemblyBoneSnapshot(
      renderers, rootBones,
      lod, InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinPost", nullptr, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
  EiemRememberGameSourceSkinningFromArray(
      renderers, true, true, "AssignSkinPost");
}

static void TraceSetSmrRootBone(void *animator, void *renderers,
                                void *rootBoneInfos, void *methodInfo) {
  auto original = (TraceSetSmrRootBoneFn)s_origSetSmrRootBone;
  if (original) original(animator, renderers, rootBoneInfos, methodInfo);
  EiemRegistrationTraceArrayBoundary(
      "SetSMRRootBone", animator, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), -1);
  EiemRememberGameSourceSkinningFromArray(
      renderers, false, true, "SetSMRRootBone");
}

// CreateSMS returns the exact SkinnedMeshRenderer array for one NPC model and
// one LOD. Resource writes wait for AssignSkin to finish the native palette.
static size_t EiemApplyStandaloneRenderRulesToSkinArray(
    void *renderers, const char *stage) {
  const size_t count = EiemManagedArrayLength(renderers);
  if (!renderers || !count || count > 8192) return 0;
  void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  size_t applied = 0;
  for (size_t index = 0; index < count; ++index) {
    void *renderer = items[index];
    if (!renderer) continue;
    void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
    if (mesh && EiemApplyStandaloneRenderRulesToRenderer(
                    renderer, renderer, mesh, "SkinnedMeshRenderer", nullptr,
                    stage))
      ++applied;
  }
  if (applied)
    Log("[MOD-ASSEMBLY-v111] stage=%s sources=%zu applied=%zu",
        stage ? stage : "unknown", count, applied);
  return applied;
}

static void TraceCreateSmsGo(void *assetLoader, void *meshAssets, int32_t lod,
                             void *goPool, void *parent, void *stringList,
                             void *intList, void **renderers,
                             void **rootBones, bool flag, void *handleMap,
                             bool deferred, void *methodInfo) {
  EiemAdoptUnityThreadFromAssemblyHook("NPCAvatarCreatorUtils.CreateSMSGO");
  auto original = (TraceCreateSmsGoFn)s_origCreateSmsGo;
  if (original)
    original(assetLoader, meshAssets, lod, goPool, parent, stringList,
             intList, renderers, rootBones, flag, handleMap, deferred,
             methodInfo);
  void *array = renderers ? *renderers : nullptr;
  // CreateSMS has produced the Renderer objects, but AssignSkin has not yet
  // supplied their final bones/root state.  Keep this boundary observational;
  // the resource transaction runs after AssignSkin returns.
  EiemRegistrationTraceArrayBoundary(
      "CreateSMSGO", meshAssets, array, EiemManagedArrayLength(array),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
  EiemTraceSkinArrayItems(
      "CreateSMSGO", meshAssets, array, lod,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
}

static void TraceCreateSmsPost(void *meshAssets, int32_t lod, void *goPool,
                               void *parent, void *stringList, void *intList,
                               void **renderers, void **rootBones, bool flag,
                               void *methodInfo) {
  EiemAdoptUnityThreadFromAssemblyHook(
      "NPCAvatarCreatorUtils.CreateSMSInfoForPostModel");
  auto original = (TraceCreateSmsPostFn)s_origCreateSmsPost;
  if (original)
    original(meshAssets, lod, goPool, parent, stringList, intList, renderers,
             rootBones, flag, methodInfo);
  void *array = renderers ? *renderers : nullptr;
  // Defer resource writes until the game's AssignSkin boundary completes.
  EiemRegistrationTraceArrayBoundary(
      "CreateSMSInfoForPostModel", meshAssets, array,
      EiemManagedArrayLength(array),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
}

// Mesh observations are fixed-size metadata records. They intentionally keep
// native object pointers only for the lifetime of the process; the Dump UI
// uses them as selection keys and never persists them as replacement IDs.
struct EiemMeshObservation {
  void *mesh;
  void *renderer;
  char rendererType[32];
  char rendererName[192];
  char meshName[192];
  char hierarchyPath[512];
  void *renderers[16];
  uint32_t rendererCount;
  uint32_t instanceCount;
  ULONGLONG firstSeenMs;
  ULONGLONG lastSeenMs;
};
static SRWLOCK s_meshObservationLock = SRWLOCK_INIT;
static EiemMeshObservation s_meshObservations[4096] = {};
static size_t s_meshObservationCount = 0;

static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  if (!renderer || !g_component_get_transform || !g_transform_get_parent ||
      !g_object_get_name)
    return;

  char names[32][96] = {};
  size_t count = 0;
  void *transform = Invoke(g_component_get_transform, renderer);
  while (transform && count < _countof(names)) {
    void *nameString = Invoke(g_object_get_name, transform);
    if (nameString)
      ReadStrUtf8(nameString, names[count], sizeof(names[count]));
    if (!names[count][0]) strncpy_s(names[count], sizeof(names[count]),
                                    "<unnamed>", _TRUNCATE);
    ++count;
    transform = Invoke(g_transform_get_parent, transform);
  }

  size_t used = 0;
  for (size_t i = count; i > 0; --i) {
    const char *name = names[i - 1];
    const size_t nameLen = strlen(name);
    const size_t separator = used ? 1 : 0;
    if (used + separator + nameLen + 1 >= outSize) break;
    if (separator) out[used++] = '/';
    memcpy(out + used, name, nameLen);
    used += nameLen;
    out[used] = '\0';
  }
}

static void TraceFillMeshObservationDetails(EiemMeshObservation &entry) {
  char rendererText[512] = {};
  char meshText[512] = {};
  TraceDescribeObject(entry.renderer, rendererText, sizeof(rendererText));
  TraceDescribeObject(entry.mesh, meshText, sizeof(meshText));
  strncpy_s(entry.rendererName, sizeof(entry.rendererName), rendererText,
            _TRUNCATE);
  strncpy_s(entry.meshName, sizeof(entry.meshName), meshText, _TRUNCATE);
  TraceBuildRendererHierarchy(entry.renderer, entry.hierarchyPath,
                              sizeof(entry.hierarchyPath));
}

static void TraceRememberMeshObservation(void *renderer, void *mesh,
                                         const char *rendererType) {
  if (!renderer || !mesh || g_shutdownRequested) return;
  AcquireSRWLockExclusive(&s_meshObservationLock);
  for (size_t i = 0; i < s_meshObservationCount; ++i) {
    EiemMeshObservation &entry = s_meshObservations[i];
    if (entry.mesh == mesh) {
      entry.lastSeenMs = GetTickCount64();
      bool knownRenderer = false;
      for (uint32_t r = 0; r < entry.rendererCount; ++r) {
        if (entry.renderers[r] == renderer) {
          knownRenderer = true;
          break;
        }
      }
      if (!knownRenderer) {
        if (entry.rendererCount < _countof(entry.renderers))
          entry.renderers[entry.rendererCount++] = renderer;
        ++entry.instanceCount;
      }
      if (!entry.hierarchyPath[0] || !entry.meshName[0])
        TraceFillMeshObservationDetails(entry);
      ReleaseSRWLockExclusive(&s_meshObservationLock);
      return;
    }
  }
  if (s_meshObservationCount >= _countof(s_meshObservations)) {
    ReleaseSRWLockExclusive(&s_meshObservationLock);
    return;
  }
  EiemMeshObservation &entry = s_meshObservations[s_meshObservationCount++];
  memset(&entry, 0, sizeof(entry));
  entry.renderer = renderer;
  entry.mesh = mesh;
  entry.renderers[0] = renderer;
  entry.rendererCount = 1;
  entry.instanceCount = 1;
  strncpy_s(entry.rendererType, sizeof(entry.rendererType),
            rendererType ? rendererType : "Renderer", _TRUNCATE);
  entry.firstSeenMs = entry.lastSeenMs = GetTickCount64();
  // Object description is protected by SEH and runs on Unity's calling
  // thread. Keep it out of the GUI thread, which cannot invoke IL2CPP safely.
  TraceFillMeshObservationDetails(entry);
  ReleaseSRWLockExclusive(&s_meshObservationLock);
}

static size_t TraceCopyMeshObservations(EiemMeshObservation *out,
                                        size_t capacity) {
  if (!out || capacity == 0) return 0;
  AcquireSRWLockShared(&s_meshObservationLock);
  const size_t count = s_meshObservationCount < capacity
                           ? s_meshObservationCount
                           : capacity;
  if (count) memcpy(out, s_meshObservations,
                    count * sizeof(EiemMeshObservation));
  ReleaseSRWLockShared(&s_meshObservationLock);
  return count;
}

static void TraceClearMeshObservations() {
  AcquireSRWLockExclusive(&s_meshObservationLock);
  s_meshObservationCount = 0;
  memset(s_meshObservations, 0, sizeof(s_meshObservations));
  ReleaseSRWLockExclusive(&s_meshObservationLock);
}

typedef void (*TraceRendererVisitor)(void *renderer, void *mesh,
                                     const char *rendererType, void *context);

static void TraceVisitRendererType(void *rendererClass,
                                   const char *rendererType,
                                   TraceRendererVisitor visitor,
                                   void *context) {
  if (!rendererClass || !rendererType ||
      (!g_object_find_objects_of_type &&
       !g_resources_find_objects_of_type_all) ||
      !il2cpp_class_get_type || !il2cpp_type_get_object || !visitor)
    return;
  __try {
    void *type = il2cpp_class_get_type(rendererClass);
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!typeObject) return;
    void *params[] = {typeObject};
    // FindObjectsOfType omits inactive and persistent objects, including many
    // UI preview models. F10 is an explicit one-shot reconciliation, so use
    // Resources.FindObjectsOfTypeAll when available and keep the scene-only
    // API solely as a compatibility fallback.
    void *enumerator = g_resources_find_objects_of_type_all
                           ? g_resources_find_objects_of_type_all
                           : g_object_find_objects_of_type;
    void *array = Invoke(enumerator, nullptr, params);
    if (!array) return;
    int count = *(int *)((char *)array + 24);
    if (count < 0) return;
    if (count > 100000) count = 100000;
    void **data = (void **)((char *)array + 32);
    void *getter = strcmp(rendererType, "SkinnedMeshRenderer") == 0
                       ? g_smr_get_sharedMesh
                       : g_meshFilter_get_sharedMesh;
    for (int i = 0; i < count; ++i) {
      if (data[i] && getter)
        visitor(data[i], Invoke(getter, data[i]), rendererType, context);
    }
  } __except (1) {
    Log("[SCENE] Renderer enumeration failed for %s", rendererType);
  }
}

static void TraceObserveRendererVisitor(void *renderer, void *mesh,
                                        const char *rendererType,
                                        void *) {
  TraceRememberMeshObservation(renderer, mesh, rendererType);
}

static void TraceEnumerateRendererType(void *rendererClass,
                                       const char *rendererType) {
  TraceVisitRendererType(rendererClass, rendererType, TraceObserveRendererVisitor,
                         nullptr);
}

// Refresh performs a real scene-wide Unity enumeration instead of relying on
// sharedMesh setter events, which may have occurred before the Dump page was
// opened. It runs only in the game's window procedure.
static void TraceRefreshMeshObservations() {
  TraceClearMeshObservations();
  TraceEnumerateRendererType(g_skinnedMeshRendererClass,
                             "SkinnedMeshRenderer");
  TraceEnumerateRendererType(g_meshFilterClass, "MeshFilter");
}

static void EiemReapplyShapeControls(const std::vector<std::string> &affected) {
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  for (auto &state : s_eiemOverrides) {
    EiemModRule rule = {};
    if (EiemModAffected(state.modPath, &affected) &&
        EiemFindRenderRuleBySection(state.modPath, state.renderSection, &rule))
      EiemUpdateRendererShapes(state.renderer, state.rendererType, rule, state.shapes);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static constexpr UINT_PTR kEiemShapeTransitionTimer = 0xE153;
static ULONGLONG s_eiemShapeTransitionTick = 0;

static bool EiemAnyShapeTransitions() {
  bool active = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  for (const auto &state : s_eiemOverrides)
    if (EiemShapeStateAnimating(state.shapes)) { active = true; break; }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  return active;
}

static void EiemRefreshShapeTransitionTimer() {
  if (!g_gameHwnd || !IsWindow(g_gameHwnd)) return;
  if (EiemAnyShapeTransitions()) {
    if (!s_eiemShapeTransitionTick) s_eiemShapeTransitionTick = GetTickCount64();
    SetTimer(g_gameHwnd, kEiemShapeTransitionTimer, 16, nullptr);
  } else {
    KillTimer(g_gameHwnd, kEiemShapeTransitionTimer);
    s_eiemShapeTransitionTick = 0;
  }
}

static void EiemRunShapeTransitions() {
  const ULONGLONG now = GetTickCount64();
  const float elapsed = s_eiemShapeTransitionTick
      ? (float)(now - s_eiemShapeTransitionTick) / 1000.0f : 0.0f;
  s_eiemShapeTransitionTick = now;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  for (auto &state : s_eiemOverrides) {
    if (!EiemShapeStateAnimating(state.shapes)) continue;
    EiemModRule rule = {};
    if (EiemFindRenderRuleBySection(state.modPath, state.renderSection, &rule))
      EiemUpdateRendererShapes(state.renderer, state.rendererType, rule,
                               state.shapes, elapsed);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  EiemRefreshShapeTransitionTimer();
}


static EiemModUpdateQueue s_eiemModUpdates;
// Models whose first native renderer registration was intentionally allowed
// to complete with the game's source Mesh.  A model is queued once and
// replayed through the same EntityRenderHelper boundary after the cache has
// settled; this keeps cold start on the same path as a stable F10 rebuild.
// At most one WM_EIEM_MOD_RECONCILE may be queued at a time.  Requests are
// already coalesced by EiemModUpdateQueue; posting one message per lifecycle
// callback would otherwise flood the game's window queue while a scene is
// assembling.
static volatile LONG s_eiemModUpdateMessagePosted = 0;
// One ordered queue for window-thread keys and render-thread ImGui controls.
static SRWLOCK s_eiemInputLock = SRWLOCK_INIT;
static std::vector<EiemModInputEvent> s_eiemPendingInputs;

static void EiemQueueModInput(EiemModInputEvent event) {
  AcquireSRWLockExclusive(&s_eiemInputLock);
  // Adjacent UI and manager-slider frames merge variable writes; never move
  // one past a key press or a different control source.
  if ((event.directValues || !event.uiSection.empty()) &&
      !s_eiemPendingInputs.empty() &&
      s_eiemPendingInputs.back().generation == event.generation &&
      s_eiemPendingInputs.back().modPath == event.modPath &&
      s_eiemPendingInputs.back().directValues == event.directValues &&
      s_eiemPendingInputs.back().uiSection == event.uiSection) {
    for (const auto &value : event.values) s_eiemPendingInputs.back().values[value.first] = value.second;
  }
  else if (event.holdTick && !s_eiemPendingInputs.empty() &&
           s_eiemPendingInputs.back().holdTick &&
           s_eiemPendingInputs.back().generation == event.generation &&
           s_eiemPendingInputs.back().modPath == event.modPath &&
           s_eiemPendingInputs.back().chord == event.chord &&
           s_eiemPendingInputs.back().keySection == event.keySection &&
           s_eiemPendingInputs.back().uiFocus == event.uiFocus) {
    // A busy Unity thread may leave several 20 ms polls queued. Merge their
    // elapsed time into one transaction so the input queue stays bounded.
    s_eiemPendingInputs.back().holdSeconds += event.holdSeconds;
  }
  else s_eiemPendingInputs.push_back(std::move(event));
  ReleaseSRWLockExclusive(&s_eiemInputLock);
  EiemRequestModUpdate(EiemModUpdate::Reapply, "mod control");
}

static void EiemQueueModKey(EiemKeyChord chord, LONG generation,
                            bool holdTick = false,
                            double holdSeconds = 0.02) {
  if (!EiemOnUnityThread() || !g_pluginActive) return;
  HWND foreground = GetForegroundWindow();
  if (foreground != g_gameHwnd && foreground != g_guiHwnd && foreground != g_modUiHwnd) return;
  EiemModInputEvent event{chord,generation};
  event.modPath = EiemGetSelectedModPath();
  if (event.modPath.empty()) return;
  event.uiFocus = EiemModUsesUiKeyScope(
      foreground == g_gameHwnd, foreground == g_modUiHwnd,
      InterlockedCompareExchange(&s_eiemModManagerOpen, 0, 0) != 0);
  event.holdTick = holdTick;
  event.holdSeconds = holdSeconds;
  EiemRegistrationTraceInput(chord.vk, chord.modifiers, generation,
                             event.uiFocus, event.modPath.c_str(), nullptr, 0);
  EiemQueueModInput(std::move(event));
}

static bool EiemPostPendingModUpdate(const char *reason) {
  if (g_shutdownRequested || !g_gameHwnd || !IsWindow(g_gameHwnd) ||
      !s_eiemModUpdates.HasPending())
    return false;
  // A posted message is only a wake-up.  The atomic queue carries the full
  // bitmask, so a later caller can merge Reconcile/Reapply/Reload without
  // posting another wake-up.
  if (InterlockedCompareExchange(&s_eiemModUpdateMessagePosted, 1, 0) != 0)
    return true;
  if (PostMessageW(g_gameHwnd, WM_EIEM_MOD_RECONCILE, 0, 0)) return true;
  InterlockedExchange(&s_eiemModUpdateMessagePosted, 0);
  SetTimer(g_gameHwnd, kEiemModRetryTimer, 100, nullptr);
  Log("[MOD] Pending reconcile post failed (%s): err=%lu",
      reason ? reason : "unknown", GetLastError());
  return false;
}

static void EiemRequestModUpdate(EiemModUpdate request, const char *reason) {
  if (g_shutdownRequested) {
    Log("[MOD] Reconcile not queued (%s): game window is unavailable",
        reason ? reason : "unknown");
    return;
  }
  const bool first = s_eiemModUpdates.Request(request);
  if (!g_gameHwnd || !IsWindow(g_gameHwnd)) {
    Log("[MOD] Reconcile pending (%s): game window is unavailable",
        reason ? reason : "unknown");
    return;
  }
  if (!EiemPostPendingModUpdate(reason)) return;
  if (first)
    Log("[MOD] Reconcile queued tick=%llu: %s",
        (unsigned long long)GetTickCount64(), reason ? reason : "unknown");
}


static void EiemQueueNativeSkinRefresh(uintptr_t ownerModel) {
  if (!ownerModel) return;
  AcquireSRWLockExclusive(&s_eiemNativeSkinRefreshLock);
  if (std::find(s_eiemNativeSkinRefreshModels.begin(),
                s_eiemNativeSkinRefreshModels.end(), ownerModel) ==
      s_eiemNativeSkinRefreshModels.end())
    s_eiemNativeSkinRefreshModels.push_back(ownerModel);
  ReleaseSRWLockExclusive(&s_eiemNativeSkinRefreshLock);
  EiemRequestModUpdate(EiemModUpdate::SkinRefresh, "native skin refresh");
}

static void EiemQueueModReconcile(const char *reason) {
  EiemRequestModUpdate(EiemModUpdate::Reconcile, reason);
}

static bool EiemSubmeshVisibilityTargets(
    const EiemRenderOverrideState &state,
    const std::vector<EiemSubmeshVisibilityChange> &changes) {
  for (const auto &change : changes)
    if (EiemModEquals(state.modPath, change.modPath.c_str()) &&
        EiemModEquals(state.renderSection, change.section.c_str()))
      return true;
  return false;
}

// A visibility key changes only the generated Mesh index buffers. Existing
// Renderer skin/material/physics state already belongs to the game instance;
// keep the registered Mesh identity and every Renderer field unchanged.
static uint32_t EiemReapplySubmeshVisibility(
    const std::vector<EiemSubmeshVisibilityChange> &changes) {
  if (changes.empty()) return 0;
  std::vector<EiemRenderOverrideState> targets;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  for (const auto &state : s_eiemOverrides)
    if (!state.restorePending && state.ownsMesh &&
        EiemSubmeshVisibilityTargets(state, changes))
      targets.push_back(state);
  ReleaseSRWLockShared(&s_eiemOverrideLock);

  uint32_t applied = 0;
  for (const auto &state : targets) {
    if (!state.renderer || state.rendererRef.Status() != 1 ||
        state.sourceMeshRef.Status() != 1)
      continue;
    void *current = EiemReadSharedMesh(state.renderer, state.rendererType);
    if (!current || current != state.replacementMesh) {
      Log("[MOD] Submesh visibility skipped renderer=%p section=%s "
          "reason=game mesh changed current=%p expected=%p",
          state.renderer, state.renderSection, current,
          state.replacementMesh);
      continue;
    }
    EiemModRule rule = {};
    if (!EiemFindRenderRuleBySection(state.modPath, state.renderSection,
                                     &rule) ||
        !rule.hasMesh)
      continue;
    void *resourceMesh = nullptr;
    char error[256] = {};
    if (!EiemBuildMeshResource(rule, &resourceMesh, error, sizeof(error),
                               state.originalMesh) ||
        !resourceMesh) {
      Log("[MOD] Submesh visibility update failed renderer=%p section=%s "
          "mask=0x%08X error=%s",
          state.renderer, state.renderSection, rule.hiddenSubmeshMask,
          error[0] ? error : "unknown");
      continue;
    }
    if (resourceMesh != current) {
      Log("[MOD] Submesh visibility skipped renderer=%p section=%s "
          "reason=resource identity changed current=%p resource=%p; use F10",
          state.renderer, state.renderSection, current, resourceMesh);
      continue;
    }
    ++applied;
    Log("[MOD] Submesh visibility applied renderer=%p section=%s "
        "mask=0x%08X mesh=%p",
        state.renderer, state.renderSection, rule.hiddenSubmeshMask,
        resourceMesh);
  }
  return applied;
}

// Runs only from MmdWndProc. F10 restores the previous generation, then
// replays configuration against instances registered by either supported
// model lifecycle adapter. No scene-wide Mesh scan exists.
#include "eiem_mod_reconcile.h"

static void *s_origAssetBundleLoadAsset1 = nullptr;
static void *s_origAssetBundleLoadAsset2 = nullptr;
static void *s_origAssetBundleLoadAssetAsync1 = nullptr;
static void *s_origAssetBundleLoadAssetAsync2 = nullptr;
static void *s_origAssetBundleUnload = nullptr;
static void *s_origAssetBundleCreateRequestGetAssetBundle = nullptr;
static void *s_origBundleLoadAssetBundle = nullptr;
static void *s_origBundleLoadAssetBundleAsync = nullptr;
static void *s_origBundleSetAssetBundle = nullptr;
static void *s_origBundleFinishWithBundle = nullptr;
static void *s_origBundleGetFullPath = nullptr;
static void *s_origBundleOnEndUnload = nullptr;
static void *s_origResourceLoadAssetInternal = nullptr;
static void *s_origResourceLoadSubAssetInternal = nullptr;
static void *s_origAssetGetAssetName = nullptr;
static void *s_origAssetFinishWithAsset = nullptr;
static void *s_origAssetOnComplete = nullptr;
static void *s_origGetAssetPathHash = nullptr;
static void *s_origGetAssetPathHashWithoutBurst = nullptr;
static void *s_origResourceLoadAssetInternalHash = nullptr;
static void *s_origResourceLoadSubAssetInternalHash = nullptr;
static void *s_origResourceLoadAsyncString = nullptr;
static void *s_origResourceLoadSubAssetAsyncString = nullptr;
static void *s_origResourceLoadAsyncHash = nullptr;
static void *s_origResourceLoadSubAssetAsyncHash = nullptr;
static void *s_origSimpleAssetLoaderLoadAsync = nullptr;
static void *s_origMonoEntitySimpleAssetLoaderLoadAsync = nullptr;
static void *s_origSimpleAssetLoaderTryLoad = nullptr;
static void *s_origMonoEntitySimpleAssetLoaderTryLoad = nullptr;
static void *s_origCachedPathAssetLoaderLoadDirect = nullptr;
static void *s_origCachedPathAssetLoaderTryLoad = nullptr;
static void *s_origPreloadAutoHash = nullptr;
static void *s_origAssetProxyHandlePath = nullptr;
static void *s_origAssetProxyHandleGet = nullptr;
static void *s_origAssetProxyHandleGetAssetProxy = nullptr;
static void *s_origAssetProxyLoaderHandlePath = nullptr;
static void *s_origAssetProxyLoaderHandleGet = nullptr;
static void *s_origAssetProxyLoaderHandleLoadImmediate = nullptr;
static void *s_origAssetProxyLoaderHandleAddOnProxyCompleted = nullptr;
static void *s_origAssetProxyUntrackedPath = nullptr;
static void *s_origAssetProxyUntrackedGet = nullptr;
static void *s_assetProxyUntrackedGetAssetProxy = nullptr;
static void *s_stringPathHashGetPath = nullptr;
static void *s_origVfsLoadBundleFromFile = nullptr;
static void *s_origVfsLoadBundleFromFileAsync = nullptr;
static void *s_origVfsLoadBundleFromFilePos = nullptr;
static void *s_origVfsLoadBundleFromFileAsyncPos = nullptr;
static void *s_origVfsGetAssetStream = nullptr;
static void *s_origVfsGetAssetStreamHash = nullptr;
static void *s_origVfsFileRead = nullptr;
static void *s_origVfsFileReadSpan = nullptr;
static SRWLOCK s_seenStreamLock = SRWLOCK_INIT;
static void *s_seenStreams[128] = {};
static char s_seenStreamPaths[128][768] = {};
static size_t s_seenStreamCount = 0;
static SRWLOCK s_traceHashPathLock = SRWLOCK_INIT;
static int64_t s_traceHashPaths[1024] = {};
static char s_traceHashPathText[1024][768] = {};
static size_t s_traceHashPathCount = 0;
static void *s_origStringPathHashGetMapping = nullptr;
static void *s_origSubMeshInfoGetMesh = nullptr;
static void *s_origSubMeshInfoSetMesh = nullptr;
static void *s_origLodMeshAssetsGetSubMeshInfo = nullptr;
static void *s_origMeshAssetsGetAvatarSlotMeshAssets = nullptr;
static void *s_origMeshAssetsGetAllAvatarSlotMeshAssets = nullptr;

static bool TraceTakeBudget(volatile LONG *counter, LONG limit) {
  (void)counter;
  (void)limit;
  // Asset-path exploration is complete. The hooks still apply Mesh identity
  // rules, but ordinary game traffic no longer formats or writes trace data.
  return false;
}

static bool TraceMarkHashFirstSeen(int64_t hash) {
  AcquireSRWLockExclusive(&s_traceSeenHashLock);
  for (size_t i = 0; i < s_traceSeenHashCount; ++i) {
    if (s_traceSeenHashes[i] == hash) {
      ReleaseSRWLockExclusive(&s_traceSeenHashLock);
      return false;
    }
  }
  if (s_traceSeenHashCount < _countof(s_traceSeenHashes))
    s_traceSeenHashes[s_traceSeenHashCount++] = hash;
  ReleaseSRWLockExclusive(&s_traceSeenHashLock);
  return true;
}

static int64_t TraceReadLoadableHash(void *loader) {
  if (!loader) return 0;
  __try {
    return *(int64_t *)((char *)loader + 0x20);
  } __except (1) {
    return 0;
  }
}

static void TraceDescribeObject(void *object, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  if (!object) {
    snprintf(out, outSize, "<null>");
    return;
  }

  __try {
    void *klass = il2cpp_object_get_class ? il2cpp_object_get_class(object)
                                          : nullptr;
    const char *ns = (klass && il2cpp_class_get_namespace)
                         ? il2cpp_class_get_namespace(klass)
                         : "";
    const char *name = (klass && il2cpp_class_get_name)
                           ? il2cpp_class_get_name(klass)
                           : "?";
    char objectName[192] = {};
    if (g_object_get_name) {
      void *nameString = Invoke(g_object_get_name, object);
      if (nameString) ReadStrUtf8(nameString, objectName, sizeof(objectName));
    }
    snprintf(out, outSize, "%s.%s @%p name=\"%s\"", ns ? ns : "",
             name ? name : "?", object, objectName[0] ? objectName : "?");
  } __except (1) {
    snprintf(out, outSize, "<invalid @%p>", object);
  }
}

static void TraceDescribeString(void *stringObject, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  if (!stringObject) {
    snprintf(out, outSize, "<null>");
    return;
  }
  if (ReadStrUtf8(stringObject, out, outSize) <= 0)
    snprintf(out, outSize, "<invalid string @%p>", stringObject);
}

static void TraceReadUnityObjectName(void *object, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  if (!object || !g_object_get_name) return;
  __try {
    void *name = Invoke(g_object_get_name, object);
    if (name) ReadStrUtf8(name, out, outSize);
  } __except (1) {
    out[0] = '\0';
  }
}

static void TraceRememberBundlePathText(const char *pathText) {
  if (!pathText || !pathText[0] || g_shutdownRequested)
    return;

  char normalized[768] = {};
  strncpy_s(normalized, sizeof(normalized), pathText, _TRUNCATE);
  for (char *p = normalized; *p; ++p) {
    if (*p == '\\') *p = '/';
  }

  // Only record logical AssetBundle paths. This keeps accidental diagnostics
  // or absolute filesystem paths out of the extraction manifest.
  size_t length = strlen(normalized);
  if (length < 3 || normalized[0] == '<' ||
      _stricmp(normalized + length - 3, ".ab") != 0)
    return;

  AcquireSRWLockExclusive(&s_bundleManifestLock);
  for (const auto &entry : s_bundleManifestEntries) {
    if (entry.path == normalized) {
      ReleaseSRWLockExclusive(&s_bundleManifestLock);
      return;
    }
  }

  try {
    TraceBundleManifestEntry entry;
    entry.path = normalized;
    entry.firstSeenMs = GetTickCount64();
    s_bundleManifestEntries.push_back(std::move(entry));
  } catch (...) {
    // A manifest is diagnostic/ offline tooling state. Never let an
    // allocation failure affect the game's resource-loading path.
  }
  ReleaseSRWLockExclusive(&s_bundleManifestLock);
}

static void TraceRememberBundlePath(void *path) {
  if (!path || g_shutdownRequested) return;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  TraceRememberBundlePathText(pathText);
}

static bool TraceNormalizeBundlePath(void *path, char *out, int outSize) {
  if (!path || !out || outSize <= 0) return false;
  TraceDescribeString(path, out, outSize);
  if (!out[0] || out[0] == '<') return false;
  for (char *p = out; *p; ++p) {
    if (*p == '\\') *p = '/';
  }
  const size_t length = strlen(out);
  return length >= 3 && _stricmp(out + length - 3, ".ab") == 0;
}

static void TraceRememberActiveBundle(void *bundle, void *path) {
  if (!bundle || g_shutdownRequested) return;
  char normalized[768] = {};
  if (!TraceNormalizeBundlePath(path, normalized, sizeof(normalized))) return;

  AcquireSRWLockExclusive(&s_bundleManifestLock);
  try {
    for (auto &entry : s_activeBundleEntries) {
      if (entry.bundle == bundle) {
        entry.path = normalized;
        ReleaseSRWLockExclusive(&s_bundleManifestLock);
        return;
      }
    }
    TraceActiveBundleEntry entry = {};
    entry.bundle = bundle;
    entry.path = normalized;
    s_activeBundleEntries.push_back(std::move(entry));
  } catch (...) {
  }
  ReleaseSRWLockExclusive(&s_bundleManifestLock);
}

static void TraceForgetActiveBundle(void *bundle) {
  if (!bundle) return;
  AcquireSRWLockExclusive(&s_bundleManifestLock);
  for (size_t i = 0; i < s_activeBundleEntries.size(); ++i) {
    if (s_activeBundleEntries[i].bundle == bundle) {
      s_activeBundleEntries.erase(s_activeBundleEntries.begin() + i);
      break;
    }
  }
  ReleaseSRWLockExclusive(&s_bundleManifestLock);
}

static void TraceRememberPendingBundleRequest(void *request, void *path) {
  if (!request || g_shutdownRequested) return;
  char normalized[768] = {};
  if (!TraceNormalizeBundlePath(path, normalized, sizeof(normalized))) return;
  AcquireSRWLockExclusive(&s_bundleManifestLock);
  try {
    for (auto &entry : s_pendingBundleRequests) {
      if (entry.request == request) {
        entry.path = normalized;
        ReleaseSRWLockExclusive(&s_bundleManifestLock);
        return;
      }
    }
    TracePendingBundleRequestEntry entry = {};
    entry.request = request;
    entry.path = normalized;
    s_pendingBundleRequests.push_back(std::move(entry));
  } catch (...) {
  }
  ReleaseSRWLockExclusive(&s_bundleManifestLock);
}

static void TraceResolvePendingBundleRequest(void *request, void *bundle) {
  if (!request || !bundle) return;
  std::string path;
  AcquireSRWLockExclusive(&s_bundleManifestLock);
  for (size_t i = 0; i < s_pendingBundleRequests.size(); ++i) {
    if (s_pendingBundleRequests[i].request == request) {
      path = s_pendingBundleRequests[i].path;
      s_pendingBundleRequests.erase(s_pendingBundleRequests.begin() + i);
      break;
    }
  }
  if (!path.empty()) {
    bool found = false;
    for (auto &entry : s_activeBundleEntries) {
      if (entry.bundle == bundle) {
        entry.path = path;
        found = true;
        break;
      }
    }
    if (!found) {
      TraceActiveBundleEntry entry = {};
      entry.bundle = bundle;
      entry.path = std::move(path);
      try { s_activeBundleEntries.push_back(std::move(entry)); } catch (...) {}
    }
  }
  ReleaseSRWLockExclusive(&s_bundleManifestLock);
}

static void TraceWriteJsonString(FILE *file, const std::string &value) {
  if (!file) return;
  fputc('"', file);
  for (unsigned char c : value) {
    switch (c) {
    case '"': fputs("\\\"", file); break;
    case '\\': fputs("\\\\", file); break;
    case '\b': fputs("\\b", file); break;
    case '\f': fputs("\\f", file); break;
    case '\n': fputs("\\n", file); break;
    case '\r': fputs("\\r", file); break;
    case '\t': fputs("\\t", file); break;
    default:
      if (c < 0x20)
        fprintf(file, "\\u%04x", (unsigned)c);
      else
        fputc(c, file);
      break;
    }
  }
  fputc('"', file);
}

// Capture only small metadata byte arrays returned by the VFS. This is a
// diagnostic sample, not a resource replacement path: the managed return
// value is left untouched and no managed API is invoked from the hook.
static bool TraceReadByteArrayLength(void *array, uint64_t *length) {
  if (!array || !length) return false;
  __try {
    *length = *(uint64_t *)((char *)array + 24);
    return true;
  } __except (1) {
    return false;
  }
}

static bool TraceMarkStreamFirstRead(void *stream) {
  if (!stream) return false;
  AcquireSRWLockExclusive(&s_seenStreamLock);
  for (size_t i = 0; i < s_seenStreamCount; ++i) {
    if (s_seenStreams[i] == stream) {
      ReleaseSRWLockExclusive(&s_seenStreamLock);
      return false;
    }
  }
  if (s_seenStreamCount >= _countof(s_seenStreams)) {
    ReleaseSRWLockExclusive(&s_seenStreamLock);
    return false;
  }
  s_seenStreams[s_seenStreamCount++] = stream;
  ReleaseSRWLockExclusive(&s_seenStreamLock);
  return true;
}

static void TraceRememberStreamPath(void *stream, void *path) {
  if (!stream || !path) return;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (!pathText[0]) return;
  AcquireSRWLockExclusive(&s_seenStreamLock);
  for (size_t i = 0; i < s_seenStreamCount; ++i) {
    if (s_seenStreams[i] == stream) {
      strncpy_s(s_seenStreamPaths[i], sizeof(s_seenStreamPaths[i]), pathText,
                _TRUNCATE);
      ReleaseSRWLockExclusive(&s_seenStreamLock);
      return;
    }
  }
  if (s_seenStreamCount < _countof(s_seenStreams)) {
    size_t i = s_seenStreamCount++;
    s_seenStreams[i] = stream;
    strncpy_s(s_seenStreamPaths[i], sizeof(s_seenStreamPaths[i]), pathText,
              _TRUNCATE);
  }
  ReleaseSRWLockExclusive(&s_seenStreamLock);
}

static void TraceRememberStreamPathText(void *stream, const char *pathText) {
  if (!stream || !pathText || !pathText[0]) return;
  AcquireSRWLockExclusive(&s_seenStreamLock);
  for (size_t i = 0; i < s_seenStreamCount; ++i) {
    if (s_seenStreams[i] == stream) {
      strncpy_s(s_seenStreamPaths[i], sizeof(s_seenStreamPaths[i]), pathText,
                _TRUNCATE);
      ReleaseSRWLockExclusive(&s_seenStreamLock);
      return;
    }
  }
  if (s_seenStreamCount < _countof(s_seenStreams)) {
    size_t i = s_seenStreamCount++;
    s_seenStreams[i] = stream;
    strncpy_s(s_seenStreamPaths[i], sizeof(s_seenStreamPaths[i]), pathText,
              _TRUNCATE);
  }
  ReleaseSRWLockExclusive(&s_seenStreamLock);
}

static void TraceRememberHashPath(int64_t hash, const char *pathText) {
  if (!pathText || !pathText[0]) return;
  AcquireSRWLockExclusive(&s_traceHashPathLock);
  for (size_t i = 0; i < s_traceHashPathCount; ++i) {
    if (s_traceHashPaths[i] == hash) {
      strncpy_s(s_traceHashPathText[i], sizeof(s_traceHashPathText[i]),
                pathText, _TRUNCATE);
      ReleaseSRWLockExclusive(&s_traceHashPathLock);
      return;
    }
  }
  if (s_traceHashPathCount < _countof(s_traceHashPaths)) {
    size_t i = s_traceHashPathCount++;
    s_traceHashPaths[i] = hash;
    strncpy_s(s_traceHashPathText[i], sizeof(s_traceHashPathText[i]), pathText,
              _TRUNCATE);
  }
  ReleaseSRWLockExclusive(&s_traceHashPathLock);
}

static void TraceLookupHashPath(int64_t hash, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  AcquireSRWLockShared(&s_traceHashPathLock);
  for (size_t i = 0; i < s_traceHashPathCount; ++i) {
    if (s_traceHashPaths[i] == hash) {
      strncpy_s(out, (size_t)outSize, s_traceHashPathText[i], _TRUNCATE);
      break;
    }
  }
  ReleaseSRWLockShared(&s_traceHashPathLock);
}

// The hash overload of BundleResourceManager receives StringPathHash after
// its implicit conversion to Int64.  Reconstruct the value type locally and
// ask the game's own getter for the logical path.  This keeps path identity
// in the game's hashing/mapping implementation instead of duplicating it in
// the plugin.  The call is best-effort: a missing mapping leaves the hash in
// the origin table and does not alter the original load result.
static bool TraceResolveStringPathHashPath(int64_t hash, char *out,
                                           size_t outSize) {
  if (!out || outSize == 0) return false;
  out[0] = '\0';
  if (!hash || !s_stringPathHashGetPath || !il2cpp_runtime_invoke) return false;
  struct StringPathHashValue {
    int64_t hash;
  } value = {hash};
  __try {
    void *pathObject = Invoke(s_stringPathHashGetPath, &value, nullptr);
    TraceDescribeString(pathObject, out, (int)outSize);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out[0] = '\0';
  }
  if (!out[0] || out[0] == '<') return false;
  for (char *p = out; *p; ++p)
    if (*p == '\\') *p = '/';
  TraceRememberHashPath(hash, out);
  return true;
}

static void TraceRememberProxyOrigin(void *proxy, int64_t pathHash,
                                     const char *path) {
  if (!proxy || g_shutdownRequested) return;
  AcquireSRWLockExclusive(&s_assetOriginLock);
  constexpr size_t ways = 4;
  const size_t bucketCount = _countof(s_proxyOrigins) / ways;
  const size_t base = (((uintptr_t)proxy >> 4) & (bucketCount - 1)) * ways;
  size_t chosen = base;
  for (size_t way = 0; way < ways; ++way) {
    const size_t index = base + way;
    if (s_proxyOrigins[index].proxy == proxy) { chosen = index; break; }
    if (!s_proxyOrigins[index].proxy) { chosen = index; break; }
    if (s_proxyOrigins[index].stamp < s_proxyOrigins[chosen].stamp)
      chosen = index;
  }
  TraceProxyOriginEntry &entry = s_proxyOrigins[chosen];
  if (entry.proxy != proxy) entry = {};
  entry.proxy = proxy;
  entry.stamp = (uint64_t)InterlockedIncrement64(&s_assetOriginStamp);
  if (pathHash) entry.pathHash = pathHash;
  if (path && path[0])
    strncpy_s(entry.path, sizeof(entry.path), path, _TRUNCATE);
  ReleaseSRWLockExclusive(&s_assetOriginLock);
}

static bool TraceLookupProxyOrigin(void *proxy, int64_t *pathHash,
                                   char *path, size_t pathSize) {
  if (pathHash) *pathHash = 0;
  if (path && pathSize) path[0] = '\0';
  if (!proxy) return false;
  bool found = false;
  AcquireSRWLockShared(&s_assetOriginLock);
  constexpr size_t ways = 4;
  const size_t bucketCount = _countof(s_proxyOrigins) / ways;
  const size_t base = (((uintptr_t)proxy >> 4) & (bucketCount - 1)) * ways;
  for (size_t way = 0; way < ways; ++way) {
    const TraceProxyOriginEntry &entry = s_proxyOrigins[base + way];
    if (entry.proxy != proxy) continue;
    if (pathHash) *pathHash = entry.pathHash;
    if (path && pathSize)
      strncpy_s(path, pathSize, entry.path, _TRUNCATE);
    found = true;
    break;
  }
  ReleaseSRWLockShared(&s_assetOriginLock);
  if (found && path && pathSize && !path[0] && pathHash && *pathHash)
    TraceLookupHashPath(*pathHash, path, (int)pathSize);
  return found && ((path && pathSize && path[0]) ||
                   (pathHash && *pathHash));
}

static void TraceRememberAssetOrigin(void *asset, int64_t pathHash,
                                     const char *path) {
  if (!asset || g_shutdownRequested) return;
  AcquireSRWLockExclusive(&s_assetOriginLock);
  constexpr size_t ways = 4;
  const size_t bucketCount = _countof(s_assetOrigins) / ways;
  const size_t base = (((uintptr_t)asset >> 4) & (bucketCount - 1)) * ways;
  size_t chosen = base;
  for (size_t way = 0; way < ways; ++way) {
    const size_t index = base + way;
    if (s_assetOrigins[index].asset == asset) { chosen = index; break; }
    if (!s_assetOrigins[index].asset) { chosen = index; break; }
    if (s_assetOrigins[index].stamp < s_assetOrigins[chosen].stamp)
      chosen = index;
  }
  TraceAssetOriginEntry &entry = s_assetOrigins[chosen];
  if (entry.asset != asset) entry = {};
  entry.asset = asset;
  entry.stamp = (uint64_t)InterlockedIncrement64(&s_assetOriginStamp);
  if (pathHash) entry.pathHash = pathHash;
  if (path && path[0] && path[0] != '<')
    strncpy_s(entry.path, sizeof(entry.path), path, _TRUNCATE);
  ReleaseSRWLockExclusive(&s_assetOriginLock);
}

static bool TraceBindAssetFromProxy(void *proxy, void *asset) {
  if (!proxy || !asset) return false;
  int64_t pathHash = 0;
  char path[768] = {};
  AcquireSRWLockShared(&s_assetOriginLock);
  constexpr size_t ways = 4;
  const size_t bucketCount = _countof(s_proxyOrigins) / ways;
  const size_t base = (((uintptr_t)proxy >> 4) & (bucketCount - 1)) * ways;
  for (size_t way = 0; way < ways; ++way) {
    const TraceProxyOriginEntry &entry = s_proxyOrigins[base + way];
    if (entry.proxy == proxy) {
      pathHash = entry.pathHash;
      strncpy_s(path, sizeof(path), entry.path, _TRUNCATE);
      break;
    }
  }
  ReleaseSRWLockShared(&s_assetOriginLock);
  if (!path[0] && pathHash)
    TraceLookupHashPath(pathHash, path, (int)sizeof(path));
  if (!pathHash && !path[0]) return false;
  TraceRememberAssetOrigin(asset, pathHash, path);
  return true;
}

static bool TraceLookupAssetOrigin(void *asset, int64_t *pathHash, char *path,
                                   size_t pathSize) {
  if (pathHash) *pathHash = 0;
  if (path && pathSize) path[0] = '\0';
  if (!asset) return false;
  bool found = false;
  AcquireSRWLockShared(&s_assetOriginLock);
  constexpr size_t ways = 4;
  const size_t bucketCount = _countof(s_assetOrigins) / ways;
  const size_t base = (((uintptr_t)asset >> 4) & (bucketCount - 1)) * ways;
  for (size_t way = 0; way < ways; ++way) {
    const TraceAssetOriginEntry &entry = s_assetOrigins[base + way];
    if (entry.asset == asset) {
      if (pathHash) *pathHash = entry.pathHash;
      if (path && pathSize)
        strncpy_s(path, pathSize, entry.path, _TRUNCATE);
      found = true;
      break;
    }
  }
  ReleaseSRWLockShared(&s_assetOriginLock);
  return found;
}

static void TraceLookupStreamPath(void *stream, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  AcquireSRWLockShared(&s_seenStreamLock);
  for (size_t i = 0; i < s_seenStreamCount; ++i) {
    if (s_seenStreams[i] == stream) {
      strncpy_s(out, (size_t)outSize, s_seenStreamPaths[i], _TRUNCATE);
      break;
    }
  }
  ReleaseSRWLockShared(&s_seenStreamLock);
}

typedef int (__fastcall *TraceVfsReadFn)(void *self, void *buffer, int offset,
                                         int count, void *methodInfo);

// VFSFileReadStream.Read is the first managed boundary where decrypted bytes
// are copied into a caller-owned byte[]. Capture one initial chunk per stream
// for format identification; the original buffer and return value are never
// changed.
static int TraceVfsFileRead(void *self, void *buffer, int offset, int count,
                            void *methodInfo) {
  auto original = (TraceVfsReadFn)s_origVfsFileRead;
  int result = original ? original(self, buffer, offset, count, methodInfo) : 0;
  if (!kEiemValidationVfsCapture) return result;
  if (result <= 0 || !buffer || !TraceMarkStreamFirstRead(self)) return result;

  LONG slot = InterlockedIncrement(&s_traceStreamCaptureCount);
  if (slot > 32) return result;
  uint64_t length = 0;
  if (!TraceReadByteArrayLength(buffer, &length) || length < 32 ||
      offset < 0 || (uint64_t)offset >= length)
    return result;
  uint64_t available = length - (uint64_t)offset;
  uint64_t bytes = (uint64_t)result;
  if (bytes > available) bytes = available;
  if (bytes > 1024ull * 1024ull) bytes = 1024ull * 1024ull;
  if (bytes == 0) return result;

  std::vector<uint8_t> copy((size_t)bytes);
  memcpy(copy.data(), (char *)buffer + 32 + offset, (size_t)bytes);
  CreateDirectoryA("plugin", nullptr);
  CreateDirectoryA("plugin\\captures", nullptr);
  char outPath[256] = {};
  snprintf(outPath, sizeof(outPath), "plugin\\captures\\stream_%03ld.bin",
           (long)slot);
  HANDLE file = CreateFileA(outPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return result;
  DWORD written = 0;
  BOOL ok = WriteFile(file, copy.data(), (DWORD)copy.size(), &written, nullptr);
  CloseHandle(file);
  if (ok && written == copy.size()) {
    unsigned char *h = copy.data();
    char sourcePath[768] = {};
    TraceLookupStreamPath(self, sourcePath, sizeof(sourcePath));
    Log("[RES-CAPTURE] VFS stream=%p path=\"%s\" read=%d "
        "head=%02X%02X%02X%02X file=%s",
        self, sourcePath[0] ? sourcePath : "?", result, h[0], h[1], h[2], h[3],
        outPath);
  }
  return result;
}

// Span<T> is a 16-byte value type in the Windows x64 IL2CPP ABI.  IL2CPP
// passes this aggregate by reference, with the data pointer at +0 and the
// logical length at +8.  Validate both objects before touching them because
// this hook runs on VFS worker threads and must never turn a probe into a
// crash.
static bool TraceReadByteSpan(void *span, void **data, int32_t *length) {
  if (!span || !data || !length) return false;
  MEMORY_BASIC_INFORMATION spanInfo = {};
  if (VirtualQuery(span, &spanInfo, sizeof(spanInfo)) != sizeof(spanInfo) ||
      spanInfo.State != MEM_COMMIT ||
      (spanInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0 ||
      (uintptr_t)span + 16 < (uintptr_t)span)
    return false;
  uintptr_t spanEnd = (uintptr_t)span + 16;
  uintptr_t regionEnd = (uintptr_t)spanInfo.BaseAddress +
                        spanInfo.RegionSize;
  if (spanEnd > regionEnd) return false;

  void *pointer = nullptr;
  int32_t size = 0;
  __try {
    pointer = *(void **)span;
    size = *(int32_t *)((char *)span + 8);
  } __except (1) {
    return false;
  }
  if (size < 0 || size > 64 * 1024 * 1024) return false;
  if (size > 0) {
    MEMORY_BASIC_INFORMATION dataInfo = {};
    if (!pointer || VirtualQuery(pointer, &dataInfo, sizeof(dataInfo)) !=
                        sizeof(dataInfo) ||
        dataInfo.State != MEM_COMMIT ||
        (dataInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
      return false;
    uintptr_t dataEnd = (uintptr_t)pointer + (uintptr_t)size;
    uintptr_t dataRegionEnd = (uintptr_t)dataInfo.BaseAddress +
                              dataInfo.RegionSize;
    if (dataEnd < (uintptr_t)pointer || dataEnd > dataRegionEnd) return false;
  }
  *data = pointer;
  *length = size;
  return true;
}

typedef int (__fastcall *TraceVfsReadSpanFn)(void *self, void *span,
                                              void *methodInfo);

static int TraceVfsFileReadSpan(void *self, void *span, void *methodInfo) {
  auto original = (TraceVfsReadSpanFn)s_origVfsFileReadSpan;
  int result = original ? original(self, span, methodInfo) : 0;
  if (!kEiemValidationVfsCapture) return result;
  if (result <= 0 || !span) return result;

  void *data = nullptr;
  int32_t length = 0;
  if (!TraceReadByteSpan(span, &data, &length) || !data || length <= 0)
    return result;
  if (!TraceMarkStreamFirstRead(self)) return result;
  uint64_t bytes = (uint64_t)result;
  if (bytes > (uint64_t)length) bytes = (uint64_t)length;
  if (bytes > 1024ull * 1024ull) bytes = 1024ull * 1024ull;
  if (bytes == 0) return result;

  LONG slot = InterlockedIncrement(&s_traceStreamCaptureCount);
  if (slot > 32) return result;
  CreateDirectoryA("plugin", nullptr);
  CreateDirectoryA("plugin\\captures", nullptr);
  char outPath[256] = {};
  snprintf(outPath, sizeof(outPath), "plugin\\captures\\span_%03ld.bin",
           (long)slot);
  HANDLE file = CreateFileA(outPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return result;
  DWORD written = 0;
  BOOL ok = WriteFile(file, data, (DWORD)bytes, &written, nullptr);
  CloseHandle(file);
  if (ok && written == bytes) {
    unsigned char *head = (unsigned char *)data;
    char sourcePath[768] = {};
    TraceLookupStreamPath(self, sourcePath, sizeof(sourcePath));
    Log("[RES-CAPTURE] VFS span stream=%p path=\"%s\" read=%d "
        "span=%d head=%02X%02X%02X%02X file=%s",
        self, sourcePath[0] ? sourcePath : "?", result, length, head[0],
        head[1], head[2], head[3], outPath);
  }
  return result;
}

typedef void *(__fastcall *TraceLoadAsset1Fn)(void *self, void *path,
                                               void *methodInfo);
typedef void *(__fastcall *TraceLoadAsset2Fn)(void *self, void *path,
                                               void *type, void *methodInfo);
typedef void *(__fastcall *TraceBundleLoadFn)(void *self, void *path,
                                               void *methodInfo);
typedef void (__fastcall *TraceBundleSetFn)(void *self, void *bundle,
                                             void *methodInfo);
typedef void *(__fastcall *TraceBundleGetPathFn)(void *self, void *path,
                                                 void *methodInfo);
typedef void (__fastcall *TraceBundleFinishFn)(void *self, void *bundle,
                                                void *methodInfo);
typedef void *(__fastcall *TraceAssetGetNameFn)(void *self, void *methodInfo);
typedef void (__fastcall *TraceAssetFinishFn)(void *self, void *asset,
                                               void *methodInfo);
typedef void (__fastcall *TraceVoidMethodFn)(void *self, void *methodInfo);
typedef void *(__fastcall *TraceResourceLoadAssetFn)(
    void *self, void *path, void *type, int category, bool immediate,
    int priority, void *methodInfo);
typedef void *(__fastcall *TraceResourceLoadSubAssetFn)(
    void *self, void *path, void *subAsset, void *type, int category,
    bool immediate, int priority, void *methodInfo);
typedef int64_t (__fastcall *TracePathHashFn)(void *path, void *methodInfo);
typedef void *(__fastcall *TraceResourceLoadAssetHashFn)(
    void *self, int64_t pathHash, void *type, int category, bool immediate,
    int priority, void *methodInfo);
typedef void *(__fastcall *TraceResourceLoadSubAssetHashFn)(
    void *self, int64_t pathHash, void *subAsset, void *type, int category,
    bool immediate, int priority, void *methodInfo);
// The callback overloads return void and are therefore safe observation
// boundaries even when FAssetProxyHandle is an IL2CPP value type.  Keep the
// callback opaque: invoking or wrapping it would change the game's loader
// ordering, so these probes only record the request and pass through.
typedef void (__fastcall *TraceResourceLoadAsyncStringFn)(
    void *self, int32_t logChannel, void *path, void *type, int category,
    void *callback, int priority, void *methodInfo);
typedef void (__fastcall *TraceResourceLoadSubAssetAsyncStringFn)(
    void *self, int32_t logChannel, void *path, void *subAsset, void *type,
    int category, void *callback, int priority, void *methodInfo);
typedef void (__fastcall *TraceResourceLoadAsyncHashFn)(
    void *self, int32_t logChannel, int64_t pathHash, void *type, int category,
    void *callback, int priority, void *methodInfo);
typedef void (__fastcall *TraceResourceLoadSubAssetAsyncHashFn)(
    void *self, int32_t logChannel, int64_t pathHash, void *subAsset,
    void *type, int category, void *callback, int priority, void *methodInfo);
typedef void (__fastcall *TraceAssetLoaderAsyncHashFn)(
    void *self, int64_t pathHash, void *type, void *callback, int priority,
    void *methodInfo);
// TryLoad writes the value-type handle through an explicit out pointer and
// returns bool, so it is safe to observe without guessing the value-return ABI
// used by Load(...)->FAssetProxyLoaderHandle.
typedef bool (__fastcall *TraceAssetLoaderTryLoadHashFn)(
    void *self, int64_t pathHash, void *type, void *outHandle,
    void *methodInfo);
typedef void *(__fastcall *TraceCachedLoaderLoadDirectFn)(
    void *self, void *path, void *type, void *methodInfo);
typedef bool (__fastcall *TraceCachedLoaderTryLoadStringFn)(
    void *self, void *path, void *type, void *outHandle, void *methodInfo);
typedef void (__fastcall *TracePreloadAutoHashFn)(void *self, int64_t pathHash,
                                                   void *methodInfo);
typedef void *(__fastcall *TraceProxyObjectFn)(void *self, void *methodInfo);
typedef void (__fastcall *TraceProxyLoaderAddCompletedFn)(
    void *self, int32_t logChannel, void *callback, void *methodInfo);
typedef void *(__fastcall *TraceV11DescriptorGetMeshFn)(void *self,
                                                         void *methodInfo);
typedef void (__fastcall *TraceV11DescriptorSetMeshFn)(void *self, void *mesh,
                                                         void *methodInfo);
typedef void *(__fastcall *TraceLodMeshAssetsGetSubMeshInfoFn)(
    void *self, int32_t lod, bool gpu, void *methodInfo);
typedef void *(__fastcall *TraceVfsPathFn)(void *self, void *path,
                                           void *methodInfo);
typedef void *(__fastcall *TraceVfsPathPosFn)(void *self, void *path,
                                              void *loaderPos, uint32_t crc,
                                              void *methodInfo);
typedef void (__fastcall *TraceAssetBundleUnloadFn)(void *self, bool unloadAll,
                                                    void *methodInfo);
typedef void *(__fastcall *TraceAssetBundleCreateRequestGetAssetBundleFn)(
    void *self, void *methodInfo);
typedef bool (__fastcall *TraceHashMappingFn)(void *self, int64_t pathHash,
                                               void *outPath,
                                               void *methodInfo);

static bool TraceStringPathHashGetMapping(void *self, int64_t pathHash,
                                          void *outPath, void *methodInfo) {
  auto original = (TraceHashMappingFn)s_origStringPathHashGetMapping;
  bool result = original ? original(self, pathHash, outPath, methodInfo) : false;
  if (!result || !outPath || g_shutdownRequested) return result;

  void *pathObject = nullptr;
  __try { pathObject = *(void **)outPath; }
  __except (1) { pathObject = nullptr; }
  char pathText[768] = {};
  TraceDescribeString(pathObject, pathText, sizeof(pathText));
  if (pathText[0] && pathText[0] != '<') {
    TraceRememberHashPath(pathHash, pathText);
    if (!s_traceReentrant && TraceTakeBudget(&s_tracePathHashCount, 400)) {
      s_traceReentrant = true;
      Log("[RES-TRACE] StringPathHashBinary.GetMappingStrByHash: hash=%lld "
          "path=\"%s\"",
          (long long)pathHash, pathText);
      s_traceReentrant = false;
    }
  }
  return result;
}

static void TraceReadAssetName(void *loader, char *out, int outSize) {
  if (!out || outSize <= 0) return;
  out[0] = '\0';
  auto original = (TraceAssetGetNameFn)s_origAssetGetAssetName;
  if (!original || !loader) return;
  __try {
    TraceDescribeString(original(loader, nullptr), out, outSize);
  } __except (1) {
    snprintf(out, outSize, "<unavailable>");
  }
}

static int64_t TraceGetAssetPathHash(void *path, void *methodInfo) {
  auto original = (TracePathHashFn)s_origGetAssetPathHash;
  int64_t hash = original ? original(path, methodInfo) : 0;
  if (!s_traceReentrant && TraceTakeBudget(&s_tracePathHashCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    Log("[RES-TRACE] HashStringPathProcessor.GetABStringPathHash: "
        "path=\"%s\" hash=%lld", pathText, (long long)hash);
    s_traceReentrant = false;
  }
  return hash;
}

static int64_t TraceGetAssetPathHashWithoutBurst(void *path, void *methodInfo) {
  auto original = (TracePathHashFn)s_origGetAssetPathHashWithoutBurst;
  int64_t hash = original ? original(path, methodInfo) : 0;
  if (!s_traceReentrant && TraceTakeBudget(&s_tracePathHashCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    Log("[RES-TRACE] HashStringPathProcessor.GetABStringPathHashWithoutBurst: "
        "path=\"%s\" hash=%lld", pathText, (long long)hash);
    s_traceReentrant = false;
  }
  return hash;
}

static void *TraceAssetProxyHandlePath(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
  void *result = original ? original(self, methodInfo) : nullptr;
  char resolvedPath[768] = {};
  TraceDescribeString(result, resolvedPath, sizeof(resolvedPath));
  if (resolvedPath[0] && resolvedPath[0] != '<')
    TraceRememberProxyOrigin(self, 0, resolvedPath);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyPathCount, 300)) {
    s_traceReentrant = true;
    Log("[RES-TRACE] FAssetProxyHandle.get_pathOrName: path=\"%s\" handle=%p",
        resolvedPath, self);
    s_traceReentrant = false;
  }
  return result;
}

static bool EiemUpstreamMeshObject(void *object) {
  if (!object || !s_eiemMeshClass || !il2cpp_object_get_class) return false;
  void *klass = nullptr;
  __try { klass = il2cpp_object_get_class(object); }
  __except (EXCEPTION_EXECUTE_HANDLER) { klass = nullptr; }
  for (int depth = 0; klass && depth < 12; ++depth) {
    if (klass == s_eiemMeshClass) return true;
    klass = il2cpp_class_get_parent ? il2cpp_class_get_parent(klass) : nullptr;
  }
  return false;
}

static bool EiemUpstreamNameEqualsRule(const char *candidate,
                                       const char *ruleAsset) {
  if (!candidate || !candidate[0] || !ruleAsset || !ruleAsset[0]) return false;
  if (_stricmp(candidate, ruleAsset) == 0) return true;
  char leaf[256] = {};
  const char *start = candidate;
  for (const char *p = candidate; *p; ++p)
    if (*p == '/' || *p == '\\') start = p + 1;
  strncpy_s(leaf, sizeof(leaf), start, _TRUNCATE);
  char *extension = strrchr(leaf, '.');
  if (extension) *extension = '\0';
  return leaf[0] && _stricmp(leaf, ruleAsset) == 0;
}

// Replace a Mesh at the resource return boundary.  This is deliberately
// keyed by the game's logical asset identity (proxy path or Mesh name), not by
// vertex counts or a Renderer address.  The game therefore continues to own
// PFB/UI/world construction, LOD selection, material registration and skin
// submission; EIEM only changes the Mesh object that crosses the boundary.
static void *EiemMaybeUpstreamReplaceMesh(void *sourceMesh,
                                          const char *logicalPath,
                                          const char *descriptorName) {
  if (!kEiemEnableUpstreamMeshBoundary || !sourceMesh ||
      EiemIsReplacementManagedMesh(sourceMesh) ||
      !EiemUpstreamMeshObject(sourceMesh) || !EiemOnUnityThread() ||
      s_eiemUpstreamMeshBuildActive)
    return sourceMesh;

  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  EiemModRule selected = {};
  bool matched = false;
  char sourceName[192] = {};
  TraceReadUnityObjectName(sourceMesh, sourceName, sizeof(sourceName));
  for (const auto &rule : rules) {
    if (!rule.hasMesh || !rule.asset[0]) continue;
    if ((descriptorName && EiemUpstreamNameEqualsRule(descriptorName,
                                                       rule.asset)) ||
        (logicalPath && EiemUpstreamNameEqualsRule(logicalPath,
                                                    rule.asset)) ||
        EiemUpstreamNameEqualsRule(sourceName, rule.asset)) {
      selected = rule;
      matched = true;
      break;  // Same first-match precedence as the Renderer executor.
    }
  }
  if (!matched) return sourceMesh;

  void *replacement = nullptr;
  char error[256] = {};
  std::shared_ptr<const EiemSkinIdentity> skin;
  s_eiemUpstreamMeshBuildActive = true;
  const bool built = EiemBuildMeshResource(selected, &replacement, error,
                                           sizeof(error), sourceMesh, &skin);
  s_eiemUpstreamMeshBuildActive = false;
  if (!built || !replacement) {
    Log("[UPSTREAM-MESH] event=build-failed source=%p sourceName=%s "
        "path=%s rule=%s error=%s",
        sourceMesh, sourceName[0] ? sourceName : "<empty>",
        logicalPath && logicalPath[0] ? logicalPath : "<empty>",
        selected.section, error[0] ? error : "unknown");
    return sourceMesh;
  }
  const LONG sequence = InterlockedIncrement(&s_eiemUpstreamMeshRedirectCount);
  if (sequence <= 256)
    Log("[UPSTREAM-MESH] event=redirect ordinal=%ld source=%p replacement=%p "
        "sourceName=%s path=%s rule=%s generation=%ld",
        sequence, sourceMesh, replacement, sourceName[0] ? sourceName : "<empty>",
        logicalPath && logicalPath[0] ? logicalPath : "<empty>",
        selected.section,
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  return replacement;
}

static void *TraceAssetProxyHandleGet(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyHandleGet;
  void *result = original ? original(self, methodInfo) : nullptr;
  char pathText[768] = {};
  int64_t pathHash = 0;
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
  if (pathGetter)
    TraceDescribeString(pathGetter(self, nullptr), pathText,
                        sizeof(pathText));
  if (!pathText[0] || pathText[0] == '<')
    TraceLookupProxyOrigin(self, &pathHash, pathText, sizeof(pathText));
  // FAssetProxyHandle is a value type.  The address passed to Get() is a
  // short-lived handle copy, while the resource manager recorded the origin
  // on its heap-allocated AssetProxy object.  Resolve that real proxy before
  // attempting the redirect; otherwise every cached handle is indistinguish-
  // able from an unrelated request.
  if ((!pathText[0] || pathText[0] == '<') &&
      s_origAssetProxyHandleGetAssetProxy) {
    void *proxy = ((TraceProxyObjectFn)s_origAssetProxyHandleGetAssetProxy)(
        self, nullptr);
    if (proxy)
      TraceLookupProxyOrigin(proxy, &pathHash, pathText, sizeof(pathText));
  }
  if ((!pathText[0] || pathText[0] == '<') && pathHash)
    TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if ((!pathText[0] || pathText[0] == '<') && pathHash &&
      EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (result) {
    if (!TraceBindAssetFromProxy(self, result)) {
      if (pathText[0] && pathText[0] != '<')
        TraceRememberProxyOrigin(self, 0, pathText);
      TraceBindAssetFromProxy(self, result);
    }
    void *redirected = EiemMaybeUpstreamReplaceMesh(
        result, pathText[0] && pathText[0] != '<' ? pathText : nullptr,
        nullptr);
    if (redirected != result) result = redirected;
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyGetCount, 500)) {
    s_traceReentrant = true;
    char objectText[512] = {};
    TraceDescribeObject(result, objectText, sizeof(objectText));
    Log("[RES-TRACE] FAssetProxyHandle.Get: handle=%p path=%s object=%s",
        self, pathText[0] ? pathText : "<none>", objectText);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceAssetProxyHandleGetAssetProxy(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyHandleGetAssetProxy;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyAssetCount, 300)) {
    s_traceReentrant = true;
    void *klass = result && il2cpp_object_get_class
                      ? il2cpp_object_get_class(result)
                      : nullptr;
    const char *name = klass && il2cpp_class_get_name
                           ? il2cpp_class_get_name(klass)
                           : "?";
    // The path is what ties a proxy to a Mesh asset. Every Prefab loads the same
    // Mesh set, so the path is how the UI, world and NPC chains are told apart;
    // the handle alone does not identify the asset.
    char pathText[512] = {};
    auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
    if (pathGetter) {
      TraceDescribeString(pathGetter(self, nullptr), pathText, sizeof(pathText));
    }
    if (TraceIdentityTextMatchesConfiguredRule(pathText)) {
      Log("[RES-TRACE] ProxyGet handle=%p proxy=%p type=%s path=\"%s\"", self,
          result, name ? name : "?", pathText);
    }
    s_traceReentrant = false;
  }
  return result;
}

static bool TraceIdentityTextMatchesConfiguredRule(const char *text) {
  if (!text || !text[0]) return false;
  std::string identity(text);
  std::transform(identity.begin(), identity.end(), identity.begin(),
                 [](unsigned char value) { return (char)std::tolower(value); });
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  for (const auto &rule : rules) {
    for (const char *selector : {rule.asset, rule.path}) {
      if (!selector || !selector[0]) continue;
      std::string candidate(selector);
      std::transform(candidate.begin(), candidate.end(), candidate.begin(),
                     [](unsigned char value) {
                       return (char)std::tolower(value);
                     });
      if (identity.find(candidate) != std::string::npos) return true;
    }
  }
  return false;
}

// Resource containers use a prefab/part naming family (for example P_*),
// while the replacement rule usually names the rendered S_* asset. Reuse the
// configured rule family for diagnostics without hard-coding a character.
static bool TraceDescriptorTextMatchesConfiguredFamily(const char *text) {
  if (!text || !text[0]) return false;
  std::string identity(text);
  std::transform(identity.begin(), identity.end(), identity.begin(),
                 [](unsigned char value) { return (char)std::tolower(value); });
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  for (const auto &rule : rules) {
    const char *selectors[] = {rule.asset, rule.path};
    for (const char *selector : selectors) {
      if (!selector || !selector[0]) continue;
      std::string candidate(selector);
      std::transform(candidate.begin(), candidate.end(), candidate.begin(),
                     [](unsigned char value) {
                       return (char)std::tolower(value);
                     });
      const size_t familyStart = candidate.find("actor_");
      if (familyStart == std::string::npos) continue;
      size_t familyEnd = candidate.find("_lod", familyStart);
      if (familyEnd == std::string::npos) familyEnd = candidate.size();
      const std::string family = candidate.substr(familyStart,
                                                   familyEnd - familyStart);
      if (family.size() > 6 && identity.find(family) != std::string::npos)
        return true;
    }
  }
  return false;
}

static bool TraceDescriptorTextMatchesTarget(const char *text) {
  return TraceIdentityTextMatchesConfiguredRule(text) ||
         TraceDescriptorTextMatchesConfiguredFamily(text);
}

static bool TraceTakeTargetBudget(volatile LONG *counter, LONG limit,
                                  const char *primary,
                                  const char *secondary) {
  if (!kEiemValidationIdentityProbe ||
      (!TraceIdentityTextMatchesConfiguredRule(primary) &&
       !TraceIdentityTextMatchesConfiguredRule(secondary)))
    return false;
  return InterlockedIncrement(counter) <= limit;
}

static void TraceSubMeshInfoIdentity(void *info, const char *event,
                                     void *meshOverride) {
  if (!info || !kEiemEnableDescriptorDiagnostics) return;
  __try {
    const LONG sample = InterlockedIncrement(&s_eiemDescriptorInfoCallCount);
    void *nameObject = *(void **)((char *)info + 0x30);
    char name[192] = {};
    if (nameObject) ReadStrUtf8(nameObject, name, sizeof(name));
    bool targeted = TraceDescriptorTextMatchesTarget(name);
    if (!targeted && sample > 80) return;
    void *mesh = meshOverride ? meshOverride
                              : *(void **)((char *)info + 0x10);
    char meshObjectName[192] = {};
    if (mesh) TraceReadUnityObjectName(mesh, meshObjectName,
                                       sizeof(meshObjectName));
    if (!targeted &&
        TraceDescriptorTextMatchesTarget(meshObjectName))
      targeted = true;
    const int64_t pathHash = *(int64_t *)((char *)info + 0x28);
    const int active = *(bool *)((char *)info + 0x58) ? 1 : 0;
    const int disabled = *(bool *)((char *)info + 0x6D) ? 1 : 0;
    const int32_t rootBoneId = *(int32_t *)((char *)info + 0x68);
    Log("[V1.1-DESCRIPTOR] sample=%ld targeted=%d event=%s info=%p "
        "name=%s mesh=%p meshObjectName=%s meshPathHash=%lld active=%d "
        "rendererDisabled=%d rootBoneID=%d",
        sample, targeted ? 1 : 0,
        event ? event : "unknown", info, name[0] ? name : "<empty>", mesh,
        meshObjectName[0] ? meshObjectName : "<empty>", (long long)pathHash,
        active, disabled, rootBoneId);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void *TraceV11DescriptorGetMesh(void *self, void *methodInfo) {
  auto original = (TraceV11DescriptorGetMeshFn)s_origSubMeshInfoGetMesh;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (kEiemEnableUpstreamMeshBoundary && result && self) {
    char descriptorName[192] = {};
    __try {
      void *nameObject = *(void **)((char *)self + 0x30);
      if (nameObject) ReadStrUtf8(nameObject, descriptorName,
                                   sizeof(descriptorName));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      descriptorName[0] = '\0';
    }
    void *redirected = EiemMaybeUpstreamReplaceMesh(
        result, nullptr, descriptorName[0] ? descriptorName : nullptr);
    if (redirected != result) result = redirected;
  }
  TraceSubMeshInfoIdentity(self, "SubMeshInfo.get_mesh", result);
  return result;
}

static void TraceV11DescriptorSetMesh(void *self, void *mesh, void *methodInfo) {
  auto original = (TraceV11DescriptorSetMeshFn)s_origSubMeshInfoSetMesh;
  if (original) original(self, mesh, methodInfo);
  TraceSubMeshInfoIdentity(self, "SubMeshInfo.set_mesh", mesh);
}

static void *TraceLodMeshAssetsGetSubMeshInfo(void *self, int32_t lod, bool gpu,
                                              void *methodInfo) {
  auto original = (TraceLodMeshAssetsGetSubMeshInfoFn)
      s_origLodMeshAssetsGetSubMeshInfo;
  void *result = original ? original(self, lod, gpu, methodInfo) : nullptr;
  if (!self || !kEiemEnableDescriptorDiagnostics) return result;
  __try {
    const LONG sample = InterlockedIncrement(&s_eiemDescriptorAssetsCallCount);
    void *nameObject = *(void **)((char *)self + 0x10);
    char ownerName[192] = {};
    if (nameObject) ReadStrUtf8(nameObject, ownerName, sizeof(ownerName));
    const bool targeted = TraceDescriptorTextMatchesTarget(ownerName);
    if (!targeted && sample > 80) return result;
    const size_t count = EiemManagedArrayLength(result);
    Log("[V1.1-DESCRIPTOR] sample=%ld targeted=%d "
        "event=NPCAvatarLodMeshAssets.GetSubMeshInfo owner=%p "
        "ownerName=%s lod=%d gpu=%d array=%p count=%zu",
        sample, targeted ? 1 : 0, self,
        ownerName[0] ? ownerName : "<empty>", lod, gpu ? 1 : 0, result,
        count);
    if (!result || count > 128) return result;
    void **items = (void **)((char *)result + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < count; ++index)
      TraceSubMeshInfoIdentity(items[index], "GetSubMeshInfo.item", nullptr);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  return result;
}

static int TraceManagedListCount(void *list) {
  if (!list) return 0;
  __try { return *(int *)((char *)list + 0x18); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static void *TraceMeshAssetsListGetter(void *self, void *methodInfo,
                                       void *originalPtr,
                                       const char *event) {
  auto original = (TraceProxyObjectFn)originalPtr;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (!self || !kEiemValidationIdentityProbe) return result;
  __try {
    void *pathObject = *(void **)((char *)self + 0x20);
    char path[768] = {};
    if (pathObject) ReadStrUtf8(pathObject, path, sizeof(path));
    if (!TraceIdentityTextMatchesConfiguredRule(path)) {
      void *nameObject = *(void **)((char *)self + 0x50);
      if (nameObject) ReadStrUtf8(nameObject, path, sizeof(path));
    }
    if (TraceIdentityTextMatchesConfiguredRule(path))
      Log("[V1.1-DESCRIPTOR] event=%s assets=%p identity=%s list=%p count=%d",
          event ? event : "mesh-assets", self, path[0] ? path : "<empty>",
          result, TraceManagedListCount(result));
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
  return result;
}

static void *TraceMeshAssetsGetAvatarSlotMeshAssets(void *self,
                                                    void *methodInfo) {
  return TraceMeshAssetsListGetter(self, methodInfo,
      s_origMeshAssetsGetAvatarSlotMeshAssets,
      "NPCAvatarMeshAssetsSO.GetAvatarSlotMeshAssets");
}

static void *TraceMeshAssetsGetAllAvatarSlotMeshAssets(void *self,
                                                       void *methodInfo) {
  return TraceMeshAssetsListGetter(self, methodInfo,
      s_origMeshAssetsGetAllAvatarSlotMeshAssets,
      "NPCAvatarMeshAssetsSO.GetAllAvatarSlotMeshAssets");
}

static void *TraceAssetProxyLoaderHandlePath(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  return original ? original(self, methodInfo) : nullptr;
}

static void *TraceAssetProxyLoaderHandleGet(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyLoaderHandleGet;
  void *result = original ? original(self, methodInfo) : nullptr;
  char path[768] = {};
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  if (pathGetter) TraceDescribeString(pathGetter(self, nullptr), path,
                                       sizeof(path));
  void *redirected = EiemMaybeUpstreamReplaceMesh(
      result, path[0] && path[0] != '<' ? path : nullptr, nullptr);
  if (redirected != result) result = redirected;
  if (!kEiemValidationIdentityProbe) return result;
  char objectName[192] = {};
  TraceReadUnityObjectName(result, objectName, sizeof(objectName));
  if (TraceIdentityTextMatchesConfiguredRule(path) ||
      TraceIdentityTextMatchesConfiguredRule(objectName))
    Log("[V1.1-LOADER] event=FAssetProxyLoaderHandle.Get handle=%p "
        "path=%s object=%p objectName=%s",
        self, path[0] ? path : "<none>", result,
        objectName[0] ? objectName : "<empty>");
  return result;
}

static void TraceAssetProxyLoaderHandleLoadImmediate(void *self,
                                                     void *methodInfo) {
  auto original = (TraceVoidMethodFn)s_origAssetProxyLoaderHandleLoadImmediate;
  if (original) original(self, methodInfo);
  if (!kEiemValidationIdentityProbe) return;
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  char path[768] = {};
  if (pathGetter) TraceDescribeString(pathGetter(self, nullptr), path,
                                      sizeof(path));
  if (TraceIdentityTextMatchesConfiguredRule(path))
    Log("[V1.1-LOADER] event=FAssetProxyLoaderHandle.LoadImmediate "
        "handle=%p path=%s", self, path);
}

static void TraceAssetProxyLoaderHandleAddOnProxyCompleted(
    void *self, int32_t logChannel, void *callback, void *methodInfo) {
  auto original = (TraceProxyLoaderAddCompletedFn)
      s_origAssetProxyLoaderHandleAddOnProxyCompleted;
  if (original) original(self, logChannel, callback, methodInfo);
  if (!kEiemValidationIdentityProbe) return;
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  char path[768] = {};
  if (pathGetter) TraceDescribeString(pathGetter(self, nullptr), path,
                                      sizeof(path));
  if (TraceIdentityTextMatchesConfiguredRule(path))
    Log("[V1.1-LOADER] event=FAssetProxyLoaderHandle.AddOnProxyCompleted "
        "handle=%p path=%s callback=%p", self, path, callback);
}

static void *TraceAssetProxyUntrackedGet(void *self, void *methodInfo) {
  auto original = (TraceProxyObjectFn)s_origAssetProxyUntrackedGet;
  void *result = original ? original(self, methodInfo) : nullptr;
  char pathText[768] = {};
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyUntrackedPath;
  if (!pathGetter)
    pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
  if (pathGetter)
    TraceDescribeString(pathGetter(self, nullptr), pathText,
                        sizeof(pathText));
  int64_t pathHash = 0;
  if (!pathText[0] || pathText[0] == '<')
    TraceLookupProxyOrigin(self, &pathHash, pathText, sizeof(pathText));
  if ((!pathText[0] || pathText[0] == '<') &&
      s_assetProxyUntrackedGetAssetProxy) {
    void *proxy = ((TraceProxyObjectFn)s_assetProxyUntrackedGetAssetProxy)(
        self, nullptr);
    if (proxy)
      TraceLookupProxyOrigin(proxy, &pathHash, pathText, sizeof(pathText));
  }
  if ((!pathText[0] || pathText[0] == '<') && pathHash)
    TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if ((!pathText[0] || pathText[0] == '<') && pathHash &&
      EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (result) {
    // Untracked handles do not expose the tracked proxy's origin table. Their
    // path getter is still a managed, read-only identity source, so bind it
    // before the object reaches a Renderer.
    if (pathText[0] && pathText[0] != '<') {
      TraceRememberProxyOrigin(self, 0, pathText);
      TraceBindAssetFromProxy(self, result);
    }
    void *redirected = EiemMaybeUpstreamReplaceMesh(
        result, pathText[0] && pathText[0] != '<' ? pathText : nullptr,
        nullptr);
    if (redirected != result) result = redirected;
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyGetCount, 500)) {
    s_traceReentrant = true;
    char objectText[512] = {};
    TraceDescribeObject(result, objectText, sizeof(objectText));
    Log("[RES-TRACE] FAssetProxyUntrackedHandle.Get: handle=%p path=%s object=%s",
        self, pathText[0] ? pathText : "<none>", objectText);
    s_traceReentrant = false;
  }
  return result;
}

// VFS observation deliberately treats the path as an opaque string object and
// never invokes a managed getter. These methods may run on worker threads.
static void *TraceVfsLoadBundleFromFile(void *self, void *path,
                                        void *methodInfo) {
  auto original = (TraceVfsPathFn)s_origVfsLoadBundleFromFile;
  TraceRememberBundlePath(path);
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (result) TraceRememberActiveBundle(result, path);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceVfsPathCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    Log("[RES-TRACE] VFS.LoadBundleFromFile: path=\"%s\" result=%p",
        pathText, result);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceVfsLoadBundleFromFileAsync(void *self, void *path,
                                              void *methodInfo) {
  auto original = (TraceVfsPathFn)s_origVfsLoadBundleFromFileAsync;
  TraceRememberBundlePath(path);
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (result) TraceRememberPendingBundleRequest(result, path);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceVfsPathCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    Log("[RES-TRACE] VFS.LoadBundleFromFileAsync: path=\"%s\" result=%p",
        pathText, result);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceVfsLoadBundleFromFilePos(void *self, void *path,
                                            void *loaderPos, uint32_t crc,
                                            void *methodInfo) {
  auto original = (TraceVfsPathPosFn)s_origVfsLoadBundleFromFilePos;
  TraceRememberBundlePath(path);
  void *result = original ? original(self, path, loaderPos, crc, methodInfo)
                          : nullptr;
  if (result) TraceRememberActiveBundle(result, path);
  return result;
}

static void *TraceVfsLoadBundleFromFileAsyncPos(void *self, void *path,
                                                 void *loaderPos, uint32_t crc,
                                                 void *methodInfo) {
  auto original = (TraceVfsPathPosFn)s_origVfsLoadBundleFromFileAsyncPos;
  TraceRememberBundlePath(path);
  void *result = original ? original(self, path, loaderPos, crc, methodInfo)
                          : nullptr;
  if (result) TraceRememberPendingBundleRequest(result, path);
  return result;
}

static void TraceAssetBundleUnload(void *self, bool unloadAll,
                                   void *methodInfo) {
  auto original = (TraceAssetBundleUnloadFn)s_origAssetBundleUnload;
  if (original) original(self, unloadAll, methodInfo);
  TraceForgetActiveBundle(self);
}

static void *TraceAssetBundleCreateRequestGetAssetBundle(void *self,
                                                          void *methodInfo) {
  auto original =
      (TraceAssetBundleCreateRequestGetAssetBundleFn)
          s_origAssetBundleCreateRequestGetAssetBundle;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (result) TraceResolvePendingBundleRequest(self, result);
  return result;
}

static void *TraceVfsGetAssetStream(void *self, void *path,
                                    void *methodInfo) {
  auto original = (TraceVfsPathFn)s_origVfsGetAssetStream;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceVfsPathCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    Log("[RES-TRACE] VFS.GetAssetStream: path=\"%s\" result=%p", pathText,
        result);
    s_traceReentrant = false;
  }
  if (result) TraceRememberStreamPath(result, path);
  return result;
}

typedef void *(__fastcall *TraceVfsGetAssetStreamHashFn)(void *self,
                                                          int64_t pathHash,
                                                          void *methodInfo);

static void *TraceVfsGetAssetStreamHash(void *self, int64_t pathHash,
                                        void *methodInfo) {
  auto original = (TraceVfsGetAssetStreamHashFn)s_origVfsGetAssetStreamHash;
  void *result = original ? original(self, pathHash, methodInfo) : nullptr;
  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (result) {
    if (!pathText[0])
      snprintf(pathText, sizeof(pathText), "#hash:%lld", (long long)pathHash);
    TraceRememberStreamPathText(result, pathText);
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceVfsPathCount, 300)) {
    s_traceReentrant = true;
    Log("[RES-TRACE] VFS.GetAssetStream(hash): hash=%lld path=\"%s\" "
        "result=%p",
        (long long)pathHash, pathText[0] ? pathText : "?", result);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceResourceLoadAssetInternalHash(
    void *self, int64_t pathHash, void *type, int category, bool immediate,
    int priority, void *methodInfo) {
  if (self) s_eiemResourceManagerInstance = self;
  auto original =
      (TraceResourceLoadAssetHashFn)s_origResourceLoadAssetInternalHash;
  void *result = original
                     ? original(self, pathHash, type, category, immediate,
                                priority, methodInfo)
                     : nullptr;
  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (result) {
    TraceRememberProxyOrigin(result, pathHash, pathText);
  }
  if (!s_traceReentrant && TraceMarkHashFirstSeen(pathHash) &&
      TraceTakeTargetBudget(&s_traceHashLoadCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadAssetInternal(hash): "
        "hash=%lld path=\"%s\" type=%s category=%d immediate=%d "
        "priority=%d result=%p",
        (long long)pathHash, pathText[0] ? pathText : "?", typeText,
        category, immediate ? 1 : 0, priority, result);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceResourceLoadSubAssetInternalHash(
    void *self, int64_t pathHash, void *subAsset, void *type, int category,
    bool immediate, int priority, void *methodInfo) {
  auto original =
      (TraceResourceLoadSubAssetHashFn)s_origResourceLoadSubAssetInternalHash;
  void *result = original
                     ? original(self, pathHash, subAsset, type, category,
                                immediate, priority, methodInfo)
                     : nullptr;
  char pathText[768] = {};
  char subAssetText[512] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
  if (result) {
    TraceRememberProxyOrigin(result, pathHash, pathText);
  }
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceHashSubAssetCount, 120, pathText,
                            subAssetText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadSubAssetInternal(hash): "
        "hash=%lld path=\"%s\" subAsset=\"%s\" type=%s category=%d "
        "immediate=%d priority=%d result=%p",
        (long long)pathHash, pathText[0] ? pathText : "?", subAssetText,
        typeText, category,
        immediate ? 1 : 0, priority, result);
    s_traceReentrant = false;
  }
  return result;
}

static void TracePreloadAutoHash(void *self, int64_t pathHash,
                                 void *methodInfo) {
  auto original = (TracePreloadAutoHashFn)s_origPreloadAutoHash;
  if (original) original(self, pathHash, methodInfo);
  if (!s_traceReentrant && TraceTakeBudget(&s_tracePreloadCount, 200)) {
    s_traceReentrant = true;
    Log("[RES-TRACE] PreloadManager.PreloadAuto(hash): hash=%lld",
        (long long)pathHash);
    s_traceReentrant = false;
  }
}

static void *TraceBundleLoadAssetBundle(void *self, void *path,
                                        void *methodInfo) {
  auto original = (TraceBundleLoadFn)s_origBundleLoadAssetBundle;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (result) TraceRememberActiveBundle(self, path);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceBundleLoadCount, 200)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] Bundle._LoadAssetBundle: bundle=%p path=\"%s\" result=%s",
        self, pathText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceBundleLoadAssetBundleAsync(void *self, void *path,
                                             void *methodInfo) {
  auto original = (TraceBundleLoadFn)s_origBundleLoadAssetBundleAsync;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (result) TraceRememberActiveBundle(self, path);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceBundleLoadCount, 200)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] Bundle._LoadAssetBundleAsync: bundle=%p path=\"%s\" result=%s",
        self, pathText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void TraceBundleSetAssetBundle(void *self, void *bundle,
                                      void *methodInfo) {
  auto original = (TraceBundleSetFn)s_origBundleSetAssetBundle;
  if (original) original(self, bundle, methodInfo);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceBundleLoadCount, 200)) {
    s_traceReentrant = true;
    char bundleText[512] = {};
    TraceDescribeObject(bundle, bundleText, sizeof(bundleText));
    Log("[RES-TRACE] Bundle.set_assetBundle: bundle=%p asset=%s", self,
        bundleText);
    s_traceReentrant = false;
  }
}

static void *TraceBundleGetFullPath(void *self, void *path,
                                    void *methodInfo) {
  auto original = (TraceBundleGetPathFn)s_origBundleGetFullPath;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceBundleLoadCount, 200)) {
    s_traceReentrant = true;
    char inputText[768] = {};
    char resultText[768] = {};
    TraceDescribeString(path, inputText, sizeof(inputText));
    TraceDescribeString(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] Bundle._GetBundleFileFullPath: input=\"%s\" path=\"%s\"",
        inputText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void TraceBundleFinishWithBundle(void *self, void *bundle,
                                        void *methodInfo) {
  auto original = (TraceBundleFinishFn)s_origBundleFinishWithBundle;
  if (original) original(self, bundle, methodInfo);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceBundleLoadCount, 200)) {
    s_traceReentrant = true;
    char bundleText[512] = {};
    TraceDescribeObject(bundle, bundleText, sizeof(bundleText));
    Log("[RES-TRACE] Bundle._FinishWithBundle: bundle=%p asset=%s", self,
        bundleText);
    s_traceReentrant = false;
  }
}

static void TraceBundleOnEndUnload(void *self, void *methodInfo) {
  auto original = (TraceVoidMethodFn)s_origBundleOnEndUnload;
  if (original) original(self, methodInfo);
  TraceForgetActiveBundle(self);
}

static void *TraceAssetGetAssetName(void *self, void *methodInfo) {
  auto original = (TraceAssetGetNameFn)s_origAssetGetAssetName;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceAssetNameCount, 150)) {
    s_traceReentrant = true;
    char nameText[768] = {};
    TraceDescribeString(result, nameText, sizeof(nameText));
    Log("[RES-TRACE] Asset.get_assetName: asset=%p name=\"%s\"", self,
        nameText);
    s_traceReentrant = false;
  }
  return result;
}

static void TraceAssetFinishWithAsset(void *self, void *asset,
                                      void *methodInfo) {
  auto original = (TraceAssetFinishFn)s_origAssetFinishWithAsset;
  const int64_t pathHash = TraceReadLoadableHash(self);
  char logicalPath[768] = {};
  if (pathHash) {
    TraceLookupHashPath(pathHash, logicalPath, sizeof(logicalPath));
    if (!logicalPath[0] && EiemOnUnityThread())
      TraceResolveStringPathHashPath(pathHash, logicalPath,
                                     sizeof(logicalPath));
  }

  if (asset) {
    TraceRememberAssetOrigin(
        asset, pathHash,
        logicalPath[0] && logicalPath[0] != '<' ? logicalPath : nullptr);
  }
  // Completion hooks observe identity only. Resource declarations do not
  // replace cached Unity objects; Render rules own all live mutations.
  if (original) original(self, asset, methodInfo);

  char assetName[768] = {};
  TraceReadAssetName(self, assetName, sizeof(assetName));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceAssetCompleteCount, 160, logicalPath,
                            assetName)) {
    s_traceReentrant = true;
    char sourceText[512] = {};
    TraceDescribeObject(asset, sourceText, sizeof(sourceText));
    Log("[RES-TRACE] Asset._FinishWithAsset: hash=%lld loader=%p "
        "path=\"%s\" assetName=\"%s\" asset=%s",
        (long long)pathHash, self, logicalPath[0] ? logicalPath : "?",
        assetName[0] ? assetName : "?", sourceText);
    s_traceReentrant = false;
  }
}

static void TraceAssetOnComplete(void *self, void *methodInfo) {
  auto original = (TraceVoidMethodFn)s_origAssetOnComplete;
  if (original) original(self, methodInfo);
  void *completedAsset = nullptr;
  __try { completedAsset = *(void **)((char *)self + 0xA0); }
  __except (1) { completedAsset = nullptr; }
  char assetName[768] = {};
  const int64_t pathHash = TraceReadLoadableHash(self);
  if (pathHash)
    TraceLookupHashPath(pathHash, assetName, sizeof(assetName));
  if (!assetName[0]) TraceReadAssetName(self, assetName, sizeof(assetName));
  if (completedAsset) {
    TraceRememberAssetOrigin(completedAsset, pathHash, assetName);
  }
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceAssetCompleteCount, 160, assetName)) {
    s_traceReentrant = true;
    void *asset = nullptr;
    __try { asset = *(void **)((char *)self + 0xA0); }
    __except (1) { asset = nullptr; }
    char assetText[512] = {};
    TraceDescribeObject(asset, assetText, sizeof(assetText));
    Log("[RES-TRACE] Asset.OnComplete: hash=%lld loader=%p assetName=\"%s\" "
        "asset=%s",
        (long long)pathHash, self, assetName[0] ? assetName : "?", assetText);
    s_traceReentrant = false;
  }
}

static void *TraceResourceLoadAssetInternal(
    void *self, void *path, void *type, int category, bool immediate,
    int priority, void *methodInfo) {
  if (self) s_eiemResourceManagerInstance = self;
  auto original = (TraceResourceLoadAssetFn)s_origResourceLoadAssetInternal;
  void *result = original
                     ? original(self, path, type, category, immediate,
                                priority, methodInfo)
                     : nullptr;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (result) {
    TraceRememberProxyOrigin(result, 0, pathText);
  }
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_tracePathHashCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadAssetInternal: path=\"%s\" "
        "type=%s category=%d immediate=%d priority=%d result=%p",
        pathText, typeText, category, immediate ? 1 : 0, priority, result);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceResourceLoadSubAssetInternal(
    void *self, void *path, void *subAsset, void *type, int category,
    bool immediate, int priority, void *methodInfo) {
  auto original =
      (TraceResourceLoadSubAssetFn)s_origResourceLoadSubAssetInternal;
  void *result = original
                     ? original(self, path, subAsset, type, category,
                                immediate, priority, methodInfo)
                     : nullptr;
  char pathText[768] = {};
  char subAssetText[512] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
  if (result) {
    TraceRememberProxyOrigin(result, 0, pathText);
  }
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_tracePathHashCount, 120, pathText,
                            subAssetText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadSubAssetInternal: path=\"%s\" "
        "subAsset=\"%s\" type=%s category=%d immediate=%d priority=%d "
        "result=%p",
        pathText, subAssetText, typeText, category, immediate ? 1 : 0,
        priority, result);
    s_traceReentrant = false;
  }
  return result;
}

static void TraceResourceLoadAsyncString(
    void *self, int32_t logChannel, void *path, void *type, int category,
    void *callback, int priority, void *methodInfo) {
  auto original =
      (TraceResourceLoadAsyncStringFn)s_origResourceLoadAsyncString;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_tracePathHashCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager.LoadAsync(string callback): "
        "channel=%d path=\"%s\" type=%s category=%d priority=%d "
        "callback=%p", logChannel, pathText[0] ? pathText : "?", typeText,
        category, priority, callback);
    s_traceReentrant = false;
  }
  if (self) s_eiemResourceManagerInstance = self;
  if (original)
    original(self, logChannel, path, type, category, callback, priority,
             methodInfo);
}

static void TraceResourceLoadSubAssetAsyncString(
    void *self, int32_t logChannel, void *path, void *subAsset, void *type,
    int category, void *callback, int priority, void *methodInfo) {
  auto original = (TraceResourceLoadSubAssetAsyncStringFn)
      s_origResourceLoadSubAssetAsyncString;
  char pathText[768] = {};
  char subAssetText[512] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_tracePathHashCount, 120, pathText,
                            subAssetText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager.LoadSubAssetAsync(string "
        "callback): channel=%d path=\"%s\" subAsset=\"%s\" type=%s "
        "category=%d priority=%d callback=%p", logChannel,
        pathText[0] ? pathText : "?", subAssetText[0] ? subAssetText : "?",
        typeText, category, priority, callback);
    s_traceReentrant = false;
  }
  if (self) s_eiemResourceManagerInstance = self;
  if (original)
    original(self, logChannel, path, subAsset, type, category, callback,
             priority, methodInfo);
}

static void TraceResourceLoadAsyncHash(
    void *self, int32_t logChannel, int64_t pathHash, void *type, int category,
    void *callback, int priority, void *methodInfo) {
  auto original = (TraceResourceLoadAsyncHashFn)s_origResourceLoadAsyncHash;
  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceHashLoadCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager.LoadAsync(hash callback): "
        "channel=%d hash=%lld path=\"%s\" type=%s category=%d "
        "priority=%d callback=%p", logChannel, (long long)pathHash,
        pathText[0] ? pathText : "?", typeText, category, priority, callback);
    s_traceReentrant = false;
  }
  if (self) s_eiemResourceManagerInstance = self;
  if (original)
    original(self, logChannel, pathHash, type, category, callback, priority,
             methodInfo);
}

static void TraceResourceLoadSubAssetAsyncHash(
    void *self, int32_t logChannel, int64_t pathHash, void *subAsset,
    void *type, int category, void *callback, int priority, void *methodInfo) {
  auto original = (TraceResourceLoadSubAssetAsyncHashFn)
      s_origResourceLoadSubAssetAsyncHash;
  char pathText[768] = {};
  char subAssetText[512] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceHashSubAssetCount, 120, pathText,
                            subAssetText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager.LoadSubAssetAsync(hash "
        "callback): channel=%d hash=%lld path=\"%s\" subAsset=\"%s\" "
        "type=%s category=%d priority=%d callback=%p", logChannel,
        (long long)pathHash, pathText[0] ? pathText : "?",
        subAssetText[0] ? subAssetText : "?", typeText, category, priority,
        callback);
    s_traceReentrant = false;
  }
  if (self) s_eiemResourceManagerInstance = self;
  if (original)
    original(self, logChannel, pathHash, subAsset, type, category, callback,
             priority, methodInfo);
}

static void TraceSimpleAssetLoaderLoadAsync(
    void *self, int64_t pathHash, void *type, void *callback, int priority,
    void *methodInfo) {
  auto original = (TraceAssetLoaderAsyncHashFn)s_origSimpleAssetLoaderLoadAsync;
  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceHashLoadCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] SimpleAssetLoader.LoadAsync(hash callback): "
        "loader=%p hash=%lld path=\"%s\" type=%s priority=%d callback=%p",
        self, (long long)pathHash, pathText[0] ? pathText : "?", typeText,
        priority, callback);
    s_traceReentrant = false;
  }
  if (original)
    original(self, pathHash, type, callback, priority, methodInfo);
}

static void TraceMonoEntitySimpleAssetLoaderLoadAsync(
    void *self, int64_t pathHash, void *type, void *callback, int priority,
    void *methodInfo) {
  auto original = (TraceAssetLoaderAsyncHashFn)
      s_origMonoEntitySimpleAssetLoaderLoadAsync;
  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));
  if (!s_traceReentrant &&
      TraceTakeTargetBudget(&s_traceHashLoadCount, 120, pathText)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] MonoEntitySimpleAssetLoader.LoadAsync(hash "
        "callback): loader=%p hash=%lld path=\"%s\" type=%s priority=%d "
        "callback=%p", self, (long long)pathHash,
        pathText[0] ? pathText : "?", typeText, priority, callback);
    s_traceReentrant = false;
  }
  if (original)
    original(self, pathHash, type, callback, priority, methodInfo);
}

static bool TraceAssetLoaderTryLoadHash(void *self, int64_t pathHash,
                                        void *type, void *outHandle,
                                        void *methodInfo, void *originalPtr,
                                        const char *label) {
  auto original = (TraceAssetLoaderTryLoadHashFn)originalPtr;
  const bool result = original
                          ? original(self, pathHash, type, outHandle,
                                     methodInfo)
                          : false;
  if (!kEiemValidationIdentityProbe || s_traceReentrant ||
      InterlockedIncrement(&s_traceAssetLoaderTryLoadCount) > 320)
    return result;

  char pathText[768] = {};
  TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0] && EiemOnUnityThread())
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));

  // The output handle is a value type stored at the caller-provided address.
  // Calling its getter after the original resolves the concrete cache path
  // without reading the value type's fields or changing ownership.
  char handlePath[768] = {};
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  if (result && pathGetter && outHandle) {
    __try {
      TraceDescribeString(pathGetter(outHandle, nullptr), handlePath,
                          sizeof(handlePath));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      handlePath[0] = '\0';
    }
  }
  char typeText[512] = {};
  TraceDescribeObject(type, typeText, sizeof(typeText));

  Log("[RES-TRACE] %s: loader=%p hash=%lld path=\"%s\" type=%s "
      "result=%d outHandle=%p handlePath=\"%s\"",
      label ? label : "AssetLoader.TryLoad(hash,type,out)", self,
      (long long)pathHash, pathText[0] ? pathText : "?",
      typeText[0] ? typeText : "?", result ? 1 : 0, outHandle,
      handlePath[0] ? handlePath : "?");
  return result;
}

static bool TraceSimpleAssetLoaderTryLoad(void *self, int64_t pathHash,
                                          void *type, void *outHandle,
                                          void *methodInfo) {
  return TraceAssetLoaderTryLoadHash(
      self, pathHash, type, outHandle, methodInfo,
      s_origSimpleAssetLoaderTryLoad,
      "SimpleAssetLoader.TryLoad(hash,type,out)");
}

static bool TraceMonoEntitySimpleAssetLoaderTryLoad(
    void *self, int64_t pathHash, void *type, void *outHandle,
    void *methodInfo) {
  return TraceAssetLoaderTryLoadHash(
      self, pathHash, type, outHandle, methodInfo,
      s_origMonoEntitySimpleAssetLoaderTryLoad,
      "MonoEntitySimpleAssetLoader.TryLoad(hash,type,out)");
}

static void *TraceCachedPathAssetLoaderLoadDirect(void *self, void *path,
                                                  void *type,
                                                  void *methodInfo) {
  auto original = (TraceCachedLoaderLoadDirectFn)
      s_origCachedPathAssetLoaderLoadDirect;
  void *result = original ? original(self, path, type, methodInfo) : nullptr;
  if (!kEiemValidationIdentityProbe || s_traceReentrant ||
      InterlockedIncrement(&s_traceCachedLoaderCount) > 240)
    return result;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  char typeText[512] = {};
  TraceDescribeObject(type, typeText, sizeof(typeText));
  char resultText[512] = {};
  TraceDescribeObject(result, resultText, sizeof(resultText));
  if (!TraceIdentityTextMatchesConfiguredRule(pathText) &&
      !TraceIdentityTextMatchesConfiguredRule(resultText))
    return result;
  Log("[RES-TRACE] CachedPathAssetLoader.LoadDirect(string,type): "
      "loader=%p path=\"%s\" type=%s result=%s", self,
      pathText[0] ? pathText : "?", typeText[0] ? typeText : "?",
      resultText[0] ? resultText : "<null>");
  return result;
}

static bool TraceCachedPathAssetLoaderTryLoad(void *self, void *path,
                                              void *type, void *outHandle,
                                              void *methodInfo) {
  auto original = (TraceCachedLoaderTryLoadStringFn)
      s_origCachedPathAssetLoaderTryLoad;
  const bool result = original
                          ? original(self, path, type, outHandle, methodInfo)
                          : false;
  if (!kEiemValidationIdentityProbe || s_traceReentrant ||
      InterlockedIncrement(&s_traceCachedLoaderCount) > 240)
    return result;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (!TraceIdentityTextMatchesConfiguredRule(pathText)) return result;
  char typeText[512] = {};
  TraceDescribeObject(type, typeText, sizeof(typeText));
  Log("[RES-TRACE] CachedPathAssetLoader.TryLoad(string,type,out): "
      "loader=%p path=\"%s\" type=%s result=%d outHandle=%p", self,
      pathText[0] ? pathText : "?", typeText[0] ? typeText : "?",
      result ? 1 : 0, outHandle);
  return result;
}

static void *TraceAssetBundleLoadAsset1(void *self, void *path,
                                         void *methodInfo) {
  auto original = (TraceLoadAsset1Fn)s_origAssetBundleLoadAsset1;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceLoadAssetCount, 300)) {
    s_traceReentrant = true;
    char pathText[512] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] AssetBundle.LoadAsset(string): path=\"%s\" result=%s",
        pathText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceAssetBundleLoadAsset2(void *self, void *path, void *type,
                                        void *methodInfo) {
  // Capture the caller before the original runs; the frame is still ours here.
  void *caller = _ReturnAddress();
  auto original = (TraceLoadAsset2Fn)s_origAssetBundleLoadAsset2;
  void *result = original ? original(self, path, type, methodInfo) : nullptr;
  // Only the target character's own parts are reported. Every Prefab loads this
  // same set of Meshes, so filtering on the asset is what turns the log into a
  // list of the presentation paths that touch it.
  if (!s_traceReentrant && TraceTakeBudget(&s_traceLoadAssetCount, 300)) {
    s_traceReentrant = true;
    char pathText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    if (TraceIdentityTextMatchesConfiguredRule(pathText)) {
      char typeText[512] = {};
      char resultText[512] = {};
      TraceDescribeObject(type, typeText, sizeof(typeText));
      TraceDescribeObject(result, resultText, sizeof(resultText));
      // The caller address is what identifies the presentation path: matching on
      // the asset alone cannot tell the UI, world and NPC chains apart.
      Log("[RES-TRACE] LoadAsset caller=%p path=\"%s\" type=%s result=%s", caller,
          pathText, typeText, resultText);
    }
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceAssetBundleLoadAssetAsync1(void *self, void *path,
                                              void *methodInfo) {
  auto original = (TraceLoadAsset1Fn)s_origAssetBundleLoadAssetAsync1;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  if (!s_traceReentrant &&
      TraceTakeBudget(&s_traceLoadAssetAsyncCount, 300)) {
    s_traceReentrant = true;
    char pathText[512] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] AssetBundle.LoadAssetAsync(string): path=\"%s\" "
        "result=%s",
        pathText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void *TraceAssetBundleLoadAssetAsync2(void *self, void *path,
                                              void *type, void *methodInfo) {
  auto original = (TraceLoadAsset2Fn)s_origAssetBundleLoadAssetAsync2;
  void *result = original ? original(self, path, type, methodInfo) : nullptr;
  if (!s_traceReentrant &&
      TraceTakeBudget(&s_traceLoadAssetAsyncCount, 300)) {
    s_traceReentrant = true;
    char pathText[512] = {};
    char typeText[512] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(type, typeText, sizeof(typeText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] AssetBundle.LoadAssetAsync(string,Type): path=\"%s\" "
        "type=%s result=%s",
        pathText, typeText, resultText);
    s_traceReentrant = false;
  }
  return result;
}

static void TraceSkinnedMeshSetSharedMesh(void *self, void *mesh,
                                           void *methodInfo) {
  auto original = (TraceSetSharedMeshFn)s_origSkinnedMeshSetSharedMesh;
  if (s_eiemApplyingModMeshAssignment) {
    if (original) original(self, mesh, methodInfo);
    return;
  }
  if (InterlockedCompareExchange(&s_traceSetterThreadLogged, 1, 0) == 0)
    Log("[DEBUG-thread] SkinnedMeshRenderer setter tid=%lu recordedUnityTid=%lu",
        (unsigned long)GetCurrentThreadId(), (unsigned long)s_eiemUnityThreadId);
  EiemRegistrationTraceNativeStackContext(
      "SkinnedMeshRenderer.set_sharedMesh.entry", self, mesh, nullptr,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  if (kEiemEnableContinuousMeshObservation)
    TraceRememberMeshObservation(self, mesh, "SkinnedMeshRenderer");
  void *sourceMesh = mesh;
  // A later game-side LOD/skin refresh may assign the original Mesh again.
  // Preserve an existing binding; otherwise this assignment is also a precise
  // lifecycle event at which standalone Mesh-identity rules can be evaluated.
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  EiemLogSkinSetterTimeline("sharedMesh-game", self, sourceMesh, mesh,
                            nullptr, nullptr);
  // This setter is also used while the game's skin/LOD assembly is only
  // partially populated.  It remains observation/reassertion-only; resource
  // rules are committed at the completed assembly boundaries instead.
  if (kEiemValidationIdentityProbe) {
    char identityText[768] = {};
    TraceLookupAssetOrigin(mesh, nullptr, identityText,
                           sizeof(identityText));
    if (!TraceIdentityTextMatchesConfiguredRule(identityText))
      TraceReadUnityObjectName(mesh, identityText, sizeof(identityText));
    if (!s_traceReentrant &&
        TraceTakeTargetBudget(&s_traceSharedMeshCount, 180, identityText)) {
      s_traceReentrant = true;
      char rendererText[512] = {};
      char meshText[512] = {};
      TraceDescribeObject(self, rendererText, sizeof(rendererText));
      TraceDescribeObject(mesh, meshText, sizeof(meshText));
      Log("[RES-TRACE] SkinnedMeshRenderer.set_sharedMesh: renderer=%s "
          "mesh=%s",
          rendererText, meshText);
      s_traceReentrant = false;
    }
  }
}

static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                         void *methodInfo) {
  auto original = (TraceSetSharedMeshFn)s_origMeshFilterSetSharedMesh;
  if (s_eiemApplyingModMeshAssignment) {
    if (original) original(self, mesh, methodInfo);
    return;
  }
  if (kEiemEnableContinuousMeshObservation)
    TraceRememberMeshObservation(self, mesh, "MeshFilter");
  void *sourceMesh = mesh;
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  // MeshFilter follows the same rule as SkinnedMeshRenderer: do not mutate a
  // resource from a low-level setter before the owning game assembly returns;
  // commit only at completed assembly boundaries.
  if (kEiemValidationIdentityProbe) {
    char identityText[768] = {};
    TraceLookupAssetOrigin(mesh, nullptr, identityText,
                           sizeof(identityText));
    if (!TraceIdentityTextMatchesConfiguredRule(identityText))
      TraceReadUnityObjectName(mesh, identityText, sizeof(identityText));
    if (!s_traceReentrant &&
        TraceTakeTargetBudget(&s_traceMeshFilterCount, 120, identityText)) {
      s_traceReentrant = true;
      char rendererText[512] = {};
      char meshText[512] = {};
      TraceDescribeObject(self, rendererText, sizeof(rendererText));
      TraceDescribeObject(mesh, meshText, sizeof(meshText));
      Log("[RES-TRACE] MeshFilter.set_sharedMesh: renderer=%s mesh=%s",
          rendererText, meshText);
      s_traceReentrant = false;
    }
  }
}

// Unity's public Mesh/bones properties can remain valid while the internal
// skin submission path is still waiting for the current-frame matrices. This
// probe records that boundary for the exact F10/cold-start window already
// used by EiemLogSkinTimingProbe. It never calls a setter or asks Unity to
// recalculate anything.
static void EiemLogSkinNativeSubmission(const char *eventName, void *renderer,
                                        bool result, void *arg0,
                                        int32_t arg1, void *buffer) {
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction || !renderer) return;

  bool tracked = false;
  char section[96] = "<untracked>";
  void *owner = nullptr;
  void *expectedMesh = nullptr;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    tracked = true;
    const auto &state = s_eiemOverrides[index];
    owner = (void *)state.ownerPrefabInstance;
    expectedMesh = state.replacementMesh;
    strncpy_s(section, sizeof(section), state.renderSection, _TRUNCATE);
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);

  if (tracked) {
    if (InterlockedIncrement(&s_eiemSkinNativeTrackedCalls) > 256) return;
  } else {
    // A small untracked sample helps detect a different draw branch without
    // turning a busy render loop into a log flood.
    if (InterlockedIncrement(&s_eiemSkinNativeUntrackedCalls) > 64) return;
  }

  void *currentMesh = nullptr;
  void *bones = nullptr;
  void *rootBone = nullptr;
  void *skinningRoot = nullptr;
  size_t boneCount = 0;
  uint64_t boneRefs = 0;
  uint64_t matrixRefs = 0;
  uint64_t rootMatrix = 0;
  if (tracked) {
    currentMesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
    bones = g_smr_get_bones ? Invoke(g_smr_get_bones, renderer) : nullptr;
    rootBone = g_smr_get_rootBone ? Invoke(g_smr_get_rootBone, renderer)
                                  : nullptr;
    skinningRoot = g_smr_get_skinningRoot
                       ? Invoke(g_smr_get_skinningRoot, renderer)
                       : nullptr;
    boneCount = EiemManagedArrayLength(bones);
    boneRefs = EiemSkinTimelineBoneRefs(bones);
    // Transform matrix reads are only performed on Unity's thread.  The
    // submission hook can be called from a render worker on some builds;
    // logging the managed palette there remains safe without dereferencing
    // Unity Transform state from the wrong thread.
    if (EiemOnUnityThread()) {
      matrixRefs = EiemSkinTimingBoneMatrixHash(bones);
      rootMatrix = EiemSkinTimingTransformMatrixHash(rootBone);
    }
  }
  Log("[SKIN-NATIVE-v1] tx=%ld event=%s tid=%lu tracked=%d owner=%p "
      "renderer=%p section=%s result=%d arg0=%p arg1=%d buffer=%p tick=%llu "
      "currentMesh=%p expectedMesh=%p bones=%p boneCount=%zu "
      "boneRefs=%016llX matrixRefs=%016llX rootBone=%p rootMatrix=%016llX "
      "skinningRoot=%p",
      transaction, eventName ? eventName : "unknown",
      (unsigned long)GetCurrentThreadId(), tracked ? 1 : 0, owner, renderer,
      section, result ? 1 : 0, arg0, arg1, buffer,
      (unsigned long long)GetTickCount64(), currentMesh, expectedMesh, bones,
      boneCount, (unsigned long long)boneRefs, (unsigned long long)matrixRefs,
      rootBone, (unsigned long long)rootMatrix, skinningRoot);
}

static bool TraceSkinnedMeshRequestCurrentFrameSkinMatrices(
    void *self, void *skinMatrices, int32_t count, void *methodInfo) {
  auto original = (TraceRequestCurrentFrameSkinMatricesFn)
      s_origSkinnedMeshRequestCurrentFrameSkinMatrices;
  const bool result = original ? original(self, skinMatrices, count, methodInfo)
                               : false;
  EiemLogSkinNativeSubmission("request-current-frame", self, result,
                              skinMatrices, count, nullptr);
  return result;
}

static bool TraceSkinnedMeshSkinMatricesRequestFinished(void *self,
                                                         void *methodInfo) {
  auto original = (TraceSkinMatricesRequestFinishedFn)
      s_origSkinnedMeshSkinMatricesRequestFinished;
  const bool result = original ? original(self, methodInfo) : false;
  EiemLogSkinNativeSubmission("request-finished", self, result, nullptr, 0,
                              nullptr);
  return result;
}

static void *TraceSkinnedMeshGetVertexBuffer(void *self, void *methodInfo) {
  auto original =
      (TraceSkinGraphicsBufferFn)s_origSkinnedMeshGetVertexBuffer;
  void *buffer = original ? original(self, methodInfo) : nullptr;
  EiemLogSkinNativeSubmission("get-current-vertex-buffer", self, buffer != nullptr,
                              nullptr, 0, buffer);
  return buffer;
}

static void *TraceSkinnedMeshGetPreviousVertexBuffer(void *self,
                                                     void *methodInfo) {
  auto original =
      (TraceSkinGraphicsBufferFn)s_origSkinnedMeshGetPreviousVertexBuffer;
  void *buffer = original ? original(self, methodInfo) : nullptr;
  EiemLogSkinNativeSubmission("get-previous-vertex-buffer", self,
                              buffer != nullptr, nullptr, 0, buffer);
  return buffer;
}

// HG.Rendering.Runtime.SkinnedMeshCaptureManager.RequestCapture is the first
// known custom-pipeline boundary that receives both the ordinary MeshRenderer
// and the SkinnedMeshRenderer. It does not expose the eventual ring-buffer
// offset in its managed signature, but the call is still valuable evidence:
// it tells us whether a tracked cloth Renderer enters this path at all and
// gives us the manager's frame counter to correlate with later native traces.
static uint32_t EiemReadSkinCaptureFrame(void *manager) {
  if (!manager) return 0;
  __try { return *(const uint32_t *)((const char *)manager + 0x20); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static void TraceSkinnedMeshCaptureRequest(void *self, void *meshRenderer,
                                           void *skinnedMeshRenderer,
                                           void *propertyBlock,
                                           void *methodInfo) {
  auto original =
      (TraceSkinCaptureRequestFn)s_origSkinnedMeshCaptureRequest;
  if (original)
    original(self, meshRenderer, skinnedMeshRenderer, propertyBlock,
             methodInfo);

  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction || !skinnedMeshRenderer ||
      InterlockedIncrement(&s_eiemSkinCaptureRequestCalls) > 64)
    return;

  bool tracked = false;
  char section[96] = "<untracked>";
  void *owner = nullptr;
  void *expectedMesh = nullptr;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(skinnedMeshRenderer);
  if (index != SIZE_MAX) {
    tracked = true;
    const auto &state = s_eiemOverrides[index];
    owner = (void *)state.ownerPrefabInstance;
    expectedMesh = state.replacementMesh;
    strncpy_s(section, sizeof(section), state.renderSection, _TRUNCATE);
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);

  void *currentMesh = nullptr;
  size_t boneCount = 0;
  if (tracked && EiemOnUnityThread()) {
    currentMesh = EiemReadSharedMesh(skinnedMeshRenderer,
                                     "SkinnedMeshRenderer");
    void *bones = g_smr_get_bones
                      ? Invoke(g_smr_get_bones, skinnedMeshRenderer)
                      : nullptr;
    boneCount = EiemManagedArrayLength(bones);
  }
  Log("[SKIN-CAPTURE-v1] tx=%ld manager=%p managerFrame=%u "
      "meshRenderer=%p skinnedRenderer=%p propertyBlock=%p tracked=%d "
      "owner=%p section=%s currentMesh=%p expectedMesh=%p boneCount=%zu "
      "tick=%llu tid=%lu",
      transaction, self, EiemReadSkinCaptureFrame(self), meshRenderer,
      skinnedMeshRenderer, propertyBlock, tracked ? 1 : 0, owner, section,
      currentMesh, expectedMesh, boneCount,
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
}

// GpuClothManager is the first managed object we found whose fields directly
// name the custom cloth skeleton ComputeBuffer and whose methods feed the
// render graph.  The game keeps these fields in an IL2CPP object; reading the
// already-resolved metadata offsets is observation-only and is guarded so a
// stale object cannot affect the game.  We deliberately do not call
// ComputeBuffer.GetData here: that would synchronize the GPU and could change
// the timing that produces the intermittent ground pose.
struct EiemGpuClothState {
  void *characterMesh;
  void *skeletonBuffer;
  bool isStreamingMode;
  float skeletonFlipped;
  int32_t runtimeClothNum;
  int32_t runtimeClothGroupNum;
};

static EiemGpuClothState EiemReadGpuClothState(void *self) {
  EiemGpuClothState state = {};
  if (!self) return state;
  __try {
    const char *base = (const char *)self;
    state.characterMesh = *(void **)(base + 0x110);
    state.skeletonBuffer = *(void **)(base + 0x148);
    state.isStreamingMode = *(const bool *)(base + 0x290);
    state.skeletonFlipped = *(const float *)(base + 0x294);
    state.runtimeClothNum = *(const int32_t *)(base + 0x2B8);
    state.runtimeClothGroupNum = *(const int32_t *)(base + 0x2BC);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    state = {};
  }
  return state;
}

static void EiemLogGpuClothEvent(const char *event, void *self,
                                 float deltaTime, void *argument,
                                 int result, void *returnedBuffer) {
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (transaction) {
    if (InterlockedIncrement(&s_eiemGpuClothObservationCalls) > 512)
      return;
  } else if (InterlockedIncrement(&s_eiemGpuClothStartupCalls) > 128) {
    // Manager creation/registration often happens before the cold/F10 skin
    // window is armed. Keep a small process-start census for that phase.
    return;
  }

  const LONG sequence = InterlockedIncrement(&s_eiemGpuClothEventSequence);
  const EiemGpuClothState state = EiemReadGpuClothState(self);
  char meshName[192] = {};
  if (state.characterMesh && EiemOnUnityThread())
    TraceReadUnityObjectName(state.characterMesh, meshName,
                             (int)sizeof(meshName));
  char selfDescription[256] = {};
  TraceDescribeObject(self, selfDescription, (int)sizeof(selfDescription));
  Log("[GPU-CLOTH-BOUNDARY-v1] tx=%ld seq=%ld event=%s self=%p "
      "selfType=\"%s\" mesh=%p meshName=\"%s\" skeletonBuffer=%p returned=%p "
      "result=%d dt=%.6f streaming=%d flipped=%.3f clothNum=%d "
      "groupNum=%d argument=%p caller=%p tid=%lu tick=%llu",
      transaction, sequence, event ? event : "unknown", self,
      selfDescription[0] ? selfDescription : "?", state.characterMesh,
      meshName[0] ? meshName : "?",
      state.skeletonBuffer, returnedBuffer, result, (double)deltaTime,
      state.isStreamingMode ? 1 : 0, (double)state.skeletonFlipped,
      state.runtimeClothNum, state.runtimeClothGroupNum, argument,
      _ReturnAddress(), (unsigned long)GetCurrentThreadId(),
      (unsigned long long)GetTickCount64());
}

static void TraceGpuClothTick(void *self, float deltaTime, void *methodInfo) {
  auto original = (TraceGpuClothTickFn)s_origGpuClothTick;
  if (original) original(self, deltaTime, methodInfo);
  EiemLogGpuClothEvent("Tick", self, deltaTime, nullptr, 0, nullptr);
}

static void TraceGpuClothSetPerDrawData(void *self, void *methodInfo) {
  auto original = (TraceVoidMethodFn)s_origGpuClothSetPerDrawData;
  if (original) original(self, methodInfo);
  EiemLogGpuClothEvent("SetPerDrawData", self, 0.0f, nullptr, 0, nullptr);
}

static void TraceGpuClothPipelineUpdateV2(void *self, void *transform,
                                          void *methodInfo) {
  auto original = (TraceGpuClothPipelineUpdateV2Fn)s_origGpuClothPipelineUpdateV2;
  if (original) original(self, transform, methodInfo);
  EiemLogGpuClothEvent("PipelineUpdateV2", self, 0.0f, transform, 0,
                       nullptr);
}

// Some builds expose PipelineUpdateV2 as a static helper.  Keeping a separate
// ABI for that case avoids treating its first Transform argument as a
// GpuClothManager object and reading unrelated memory as manager fields.
static void TraceGpuClothPipelineUpdateV2Static(void *transform,
                                                void *methodInfo) {
  auto original = (TraceGpuClothPipelineUpdateV2StaticFn)
      s_origGpuClothPipelineUpdateV2Static;
  if (original) original(transform, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (transaction) {
    if (InterlockedIncrement(&s_eiemGpuClothObservationCalls) > 512)
      return;
  } else if (InterlockedIncrement(&s_eiemGpuClothStartupCalls) > 128) {
    return;
  }
  const LONG sequence = InterlockedIncrement(&s_eiemGpuClothEventSequence);
  char transformDescription[256] = {};
  TraceDescribeObject(transform, transformDescription,
                      (int)sizeof(transformDescription));
  Log("[GPU-CLOTH-BOUNDARY-v1] tx=%ld seq=%ld event=PipelineUpdateV2.static "
      "transform=%p transformType=\"%s\" caller=%p tid=%lu tick=%llu",
      transaction, sequence, transform,
      transformDescription[0] ? transformDescription : "?", _ReturnAddress(),
      (unsigned long)GetCurrentThreadId(), (unsigned long long)GetTickCount64());
}

static void TraceGpuClothRegisterGroup(void *self, void *clothGroupData,
                                       void *methodInfo) {
  auto original =
      (TraceGpuClothRegisterGroupFn)s_origGpuClothRegisterGroup;
  if (original) original(self, clothGroupData, methodInfo);
  EiemLogGpuClothEvent("RegisterClothGroup", self, 0.0f, clothGroupData, 0,
                       nullptr);
}

static void TraceGpuClothSetCharacterProxyMesh(void *self, void *mesh,
                                               void *methodInfo) {
  auto original = (TraceGpuClothSetCharacterProxyMeshFn)
      s_origGpuClothSetCharacterProxyMesh;
  if (original) original(self, mesh, methodInfo);
  EiemLogGpuClothEvent("_SetCharacterProxyMesh", self, 0.0f, mesh,
                       mesh ? 1 : 0, mesh);
}

static void TraceGpuClothFlipSkeletonFlag(void *self, void *methodInfo) {
  auto original = (TraceVoidMethodFn)s_origGpuClothFlipSkeletonFlag;
  if (original) original(self, methodInfo);
  EiemLogGpuClothEvent("FlipSkeletonFlag", self, 0.0f, nullptr, 0, nullptr);
}

static void *TraceGpuClothGetSkeletonBuffer(void *self, void *methodInfo) {
  auto original =
      (TraceGpuClothGetSkeletonBufferFn)s_origGpuClothGetSkeletonBuffer;
  void *result = original ? original(self, methodInfo) : nullptr;
  EiemLogGpuClothEvent("GetSkeletonBuffer", self, 0.0f, nullptr,
                       result ? 1 : 0, result);
  return result;
}

static bool TraceGpuClothIsSkeletonValid(void *self, void *methodInfo) {
  auto original = (TraceGpuClothBoolFn)s_origGpuClothIsSkeletonValid;
  const bool result = original ? original(self, methodInfo) : false;
  EiemLogGpuClothEvent("IsClothSkeletonValid", self, 0.0f, nullptr,
                       result ? 1 : 0, nullptr);
  return result;
}

static bool TraceGpuClothIsSkeletonFlipped(void *self, void *methodInfo) {
  auto original = (TraceGpuClothBoolFn)s_origGpuClothIsSkeletonFlipped;
  const bool result = original ? original(self, methodInfo) : false;
  EiemLogGpuClothEvent("IsClothSkeletonFlipped", self, 0.0f, nullptr,
                       result ? 1 : 0, nullptr);
  return result;
}

static void TraceMaterialPropertyBlockSetBuffer(
    void *self, int32_t propertyId, void *buffer, int32_t offset,
    int32_t size, void *methodInfo) {
  auto original = (TraceMaterialPropertyBlockSetBufferFn)
      s_origMaterialPropertyBlockSetBuffer;
  if (original)
    original(self, propertyId, buffer, offset, size, methodInfo);

  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction ||
      InterlockedIncrement(&s_eiemSkinBufferBindingCalls) > 256)
    return;

  Log("[SKIN-BUFFER-BIND-v1] tx=%ld kind=buffer propertyId=%d block=%p "
      "buffer=%p offset=%d size=%d caller=%p tick=%llu tid=%lu",
      transaction, propertyId, self, buffer, offset, size,
      _ReturnAddress(), (unsigned long long)GetTickCount64(),
      (unsigned long)GetCurrentThreadId());
}

static void TraceMaterialPropertyBlockSetConstantBuffer(
    void *self, int32_t propertyId, void *buffer, int32_t offset,
    int32_t size, void *methodInfo) {
  auto original = (TraceMaterialPropertyBlockSetBufferFn)
      s_origMaterialPropertyBlockSetConstantBuffer;
  if (original)
    original(self, propertyId, buffer, offset, size, methodInfo);

  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction ||
      InterlockedIncrement(&s_eiemSkinBufferBindingCalls) > 256)
    return;

  Log("[SKIN-BUFFER-BIND-v1] tx=%ld kind=constant propertyId=%d block=%p "
      "buffer=%p offset=%d size=%d caller=%p tick=%llu tid=%lu",
      transaction, propertyId, self, buffer, offset, size,
      _ReturnAddress(), (unsigned long long)GetTickCount64(),
      (unsigned long)GetCurrentThreadId());
}

static void TraceMaterialSetConstantBuffer(
    void *self, int32_t propertyId, void *buffer, int32_t offset,
    int32_t size, void *methodInfo) {
  auto original = (TraceMaterialPropertyBlockSetBufferFn)
      s_origMaterialSetConstantBuffer;
  if (original)
    original(self, propertyId, buffer, offset, size, methodInfo);

  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction ||
      InterlockedIncrement(&s_eiemSkinBufferBindingCalls) > 256)
    return;

  Log("[SKIN-BUFFER-BIND-v1] tx=%ld kind=material-constant propertyId=%d "
      "material=%p buffer=%p offset=%d size=%d caller=%p tick=%llu tid=%lu",
      transaction, propertyId, self, buffer, offset, size,
      _ReturnAddress(), (unsigned long long)GetTickCount64(),
      (unsigned long)GetCurrentThreadId());
}

static void *TraceRenderGraphGetComputeBuffer(void *self, void *handle,
                                              void *methodInfo) {
  auto original = (TraceRenderGraphGetComputeBufferFn)
      s_origRenderGraphGetComputeBuffer;
  void *buffer = original ? original(self, handle, methodInfo) : nullptr;

  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!transaction ||
      InterlockedIncrement(&s_eiemSkinBufferBindingCalls) > 256)
    return buffer;

  uint64_t raw0 = 0;
  uint64_t raw1 = 0;
  if (handle) {
    __try {
      raw0 = *(const uint64_t *)handle;
      raw1 = *((const uint64_t *)handle + 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
      raw0 = 0;
      raw1 = 0;
    }
  }
  Log("[SKIN-BUFFER-RESOURCE-v1] tx=%ld registry=%p handle=%p "
      "raw0=%016llX raw1=%016llX buffer=%p caller=%p tick=%llu tid=%lu",
      transaction, self, handle, (unsigned long long)raw0,
      (unsigned long long)raw1, buffer, _ReturnAddress(),
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
  return buffer;
}

static bool EiemTraceCommandBufferBudget(LONG transaction) {
  return transaction &&
         InterlockedIncrement(&s_eiemSkinBufferBindingCalls) <= 512;
}

static void TraceCommandBufferSetGlobalConstantBuffer0(
    void *self, uint32_t bufferId, int32_t propertyId, int32_t offset,
    int32_t size, void *methodInfo) {
  auto original = (TraceCommandBufferSetGlobalConstantBuffer0Fn)
      s_origCommandBufferSetGlobalConstantBuffer0;
  if (original)
    original(self, bufferId, propertyId, offset, size, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!EiemTraceCommandBufferBudget(transaction)) return;
  Log("[SKIN-CMD-BUFFER-v1] tx=%ld kind=global-constant-id cmd=%p "
      "bufferId=%u propertyId=%d offset=%d size=%d caller=%p tick=%llu tid=%lu",
      transaction, self, bufferId, propertyId, offset, size, _ReturnAddress(),
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
}

static void TraceCommandBufferSetGlobalBufferId(
    void *self, int32_t propertyId, uint32_t bufferId, void *methodInfo) {
  auto original = (TraceCommandBufferSetGlobalBufferIdFn)
      s_origCommandBufferSetGlobalBufferId;
  if (original)
    original(self, propertyId, bufferId, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!EiemTraceCommandBufferBudget(transaction)) return;
  Log("[SKIN-CMD-BUFFER-v1] tx=%ld kind=global-buffer-id cmd=%p "
      "propertyId=%d bufferId=%u caller=%p tick=%llu tid=%lu",
      transaction, self, propertyId, bufferId, _ReturnAddress(),
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
}

static void TraceCommandBufferSetGlobalConstantBuffer(
    void *self, void *buffer, int32_t propertyId, int32_t offset,
    int32_t size, void *methodInfo) {
  auto original = (TraceCommandBufferSetGlobalConstantBufferFn)
      s_origCommandBufferSetGlobalConstantBuffer;
  if (original)
    original(self, buffer, propertyId, offset, size, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!EiemTraceCommandBufferBudget(transaction)) return;
  Log("[SKIN-CMD-BUFFER-v1] tx=%ld kind=global-constant cmd=%p buffer=%p "
      "propertyId=%d offset=%d size=%d caller=%p tick=%llu tid=%lu",
      transaction, self, buffer, propertyId, offset, size, _ReturnAddress(),
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
}

static void TraceCommandBufferSetGlobalBuffer(
    void *self, int32_t propertyId, void *buffer, void *methodInfo) {
  auto original = (TraceCommandBufferSetGlobalBufferFn)
      s_origCommandBufferSetGlobalBuffer;
  if (original)
    original(self, propertyId, buffer, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  if (!EiemTraceCommandBufferBudget(transaction)) return;
  Log("[SKIN-CMD-BUFFER-v1] tx=%ld kind=global-buffer cmd=%p "
      "propertyId=%d buffer=%p caller=%p tick=%llu tid=%lu",
      transaction, self, propertyId, buffer, _ReturnAddress(),
      (unsigned long long)GetTickCount64(), (unsigned long)GetCurrentThreadId());
}

static void EiemTraceGpuDrivenSubmit(const char *kind, void *self,
                                     void *commandBuffer, uint32_t id,
                                     bool flag, LONG transaction) {
  const LONG call = InterlockedIncrement(&s_eiemGpuDrivenCalls);
  if (call > 512) return;
  Log("[SKIN-GPU-SUBMIT-v1] tx=%ld call=%ld kind=%s renderer=%p cmd=%p id=%u flag=%d "
      "caller=%p tick=%llu tid=%lu",
      transaction, call, kind, self, commandBuffer, id, flag ? 1 : 0,
      _ReturnAddress(), (unsigned long long)GetTickCount64(),
      (unsigned long)GetCurrentThreadId());
}

static void TraceGpuV1BindBuffersForRendering(
    void *self, void *commandBuffer, void *methodInfo) {
  auto original = (TraceGpuDrivenBindBuffersForRenderingFn)
      s_origGpuDrivenV1BindBuffersForRendering;
  if (original) original(self, commandBuffer, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v1-bind-render", self, commandBuffer, 0, false,
                           transaction);
}

static void TraceGpuV1PopulatePerFrameData(
    void *self, void *commandBuffer, uint32_t frameDataId,
    uint32_t rendererDataId, bool flag, void *methodInfo) {
  auto original = (TraceGpuDrivenPopulatePerFrameDataFn)
      s_origGpuDrivenV1PopulatePerFrameData;
  if (original)
    original(self, commandBuffer, frameDataId, rendererDataId, flag,
             methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v1-populate-frame", self, commandBuffer,
                           frameDataId, flag, transaction);
  if (transaction && InterlockedCompareExchange(&s_eiemSkinBufferBindingCalls,
                                                 0, 0) <= 512) {
    Log("[SKIN-GPU-FRAME-v1] tx=%ld renderer=%p frameDataId=%u "
        "rendererDataId=%u cmd=%p flag=%d",
        transaction, self, frameDataId, rendererDataId, commandBuffer,
        flag ? 1 : 0);
  }
}

static void TraceGpuV1DrawRendererList(
    void *self, void *commandBuffer, uint32_t rendererListId, bool flag,
    void *methodInfo) {
  auto original = (TraceGpuDrivenDrawRendererListFn)
      s_origGpuDrivenV1DrawRendererList;
  if (original)
    original(self, commandBuffer, rendererListId, flag, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v1-draw-list", self, commandBuffer,
                           rendererListId, flag, transaction);
}

static void TraceGpuV2BindBuffersForRendering(
    void *self, void *commandBuffer, void *methodInfo) {
  auto original = (TraceGpuDrivenBindBuffersForRenderingFn)
      s_origGpuDrivenV2BindBuffersForRendering;
  if (original) original(self, commandBuffer, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v2-bind-render", self, commandBuffer, 0, false,
                           transaction);
}

static void TraceGpuV2PopulatePerFrameData(
    void *self, void *commandBuffer, uint32_t frameDataId,
    uint32_t rendererDataId, bool flag, void *methodInfo) {
  auto original = (TraceGpuDrivenPopulatePerFrameDataFn)
      s_origGpuDrivenV2PopulatePerFrameData;
  if (original)
    original(self, commandBuffer, frameDataId, rendererDataId, flag,
             methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v2-populate-frame", self, commandBuffer,
                           frameDataId, flag, transaction);
  if (transaction && InterlockedCompareExchange(&s_eiemSkinBufferBindingCalls,
                                                 0, 0) <= 512) {
    Log("[SKIN-GPU-FRAME-v1] tx=%ld renderer=%p frameDataId=%u "
        "rendererDataId=%u cmd=%p flag=%d",
        transaction, self, frameDataId, rendererDataId, commandBuffer,
        flag ? 1 : 0);
  }
}

static void TraceGpuV2DrawRendererList(
    void *self, void *commandBuffer, uint32_t rendererListId, bool flag,
    void *methodInfo) {
  auto original = (TraceGpuDrivenDrawRendererListFn)
      s_origGpuDrivenV2DrawRendererList;
  if (original)
    original(self, commandBuffer, rendererListId, flag, methodInfo);
  const LONG transaction =
      InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);
  EiemTraceGpuDrivenSubmit("v2-draw-list", self, commandBuffer,
                           rendererListId, flag, transaction);
}

#define EIEM_DEFINE_GPU_AUX_WRAPPERS(PREFIX, ORIG_PREFIX, TAG)                 \
  static void PREFIX##BindBuffersForCulling(                                  \
      void *self, void *commandBuffer, void *computeShader,                  \
      uint32_t bufferId, void *methodInfo) {                                  \
    auto original = (TraceGpuDrivenBindBuffersForCullingFn)                  \
        ORIG_PREFIX##BindBuffersForCulling;                                   \
    if (original) original(self, commandBuffer, computeShader, bufferId,     \
                           methodInfo);                                      \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    EiemTraceGpuDrivenSubmit(TAG "-bind-cull", self, commandBuffer,          \
                             bufferId, false, transaction);                   \
  }                                                                            \
  static void PREFIX##BindFrameConstants(                                      \
      void *self, void *commandBuffer, void *computeShader,                   \
      uint32_t bufferId, void *methodInfo) {                                  \
    auto original = (TraceGpuDrivenBindFrameConstantsFn)                     \
        ORIG_PREFIX##BindFrameConstants;                                      \
    if (original) original(self, commandBuffer, computeShader, bufferId,     \
                           methodInfo);                                      \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    EiemTraceGpuDrivenSubmit(TAG "-bind-frame-constants", self,             \
                             commandBuffer, bufferId, false, transaction);    \
  }                                                                            \
  static void PREFIX##BindFrameConstantsGlobal(                                \
      void *self, void *commandBuffer, void *methodInfo) {                    \
    auto original = (TraceGpuDrivenBindFrameConstantsGlobalFn)                \
        ORIG_PREFIX##BindFrameConstantsGlobal;                                 \
    if (original) original(self, commandBuffer, methodInfo);                  \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    EiemTraceGpuDrivenSubmit(TAG "-bind-frame-global", self,                 \
                             commandBuffer, 0, false, transaction);           \
  }                                                                            \
  static void PREFIX##DispatchMeshletInstanceCount(                           \
      void *self, void *commandBuffer, void *computeShader,                   \
      uint32_t dispatchId, void *methodInfo) {                                \
    auto original = (TraceGpuDrivenDispatchComputeFn)                         \
        ORIG_PREFIX##DispatchMeshletInstanceCount;                             \
    if (original) original(self, commandBuffer, computeShader, dispatchId,   \
                            methodInfo);                                      \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    EiemTraceGpuDrivenSubmit(TAG "-dispatch-meshlet", self, commandBuffer,  \
                             dispatchId, false, transaction);                  \
  }                                                                            \
  static void PREFIX##DispatchDrawBucketCount(                                \
      void *self, void *commandBuffer, void *computeShader,                   \
      uint32_t dispatchId, void *methodInfo) {                                \
    auto original = (TraceGpuDrivenDispatchComputeFn)                         \
        ORIG_PREFIX##DispatchDrawBucketCount;                                  \
    if (original) original(self, commandBuffer, computeShader, dispatchId,    \
                            methodInfo);                                      \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    EiemTraceGpuDrivenSubmit(TAG "-dispatch-bucket", self, commandBuffer,   \
                             dispatchId, false, transaction);                  \
  }                                                                            \
  static void PREFIX##AdvanceFrame(void *self, void *methodInfo) {             \
    auto original = (TraceGpuDrivenAdvanceFrameFn)                           \
        ORIG_PREFIX##AdvanceFrame;                                             \
    if (original) original(self, methodInfo);                                  \
    const LONG transaction =                                                    \
        InterlockedCompareExchange(&s_eiemSkinTimingProbePending, 0, 0);      \
    if (transaction &&                                                         \
        InterlockedIncrement(&s_eiemSkinBufferBindingCalls) <= 512) {         \
      Log("[SKIN-GPU-SUBMIT-v1] tx=%ld kind=" TAG "-advance-frame "         \
          "renderer=%p cmd=%p id=0 flag=0 caller=%p tick=%llu tid=%lu",       \
          transaction, self, nullptr, _ReturnAddress(),                       \
          (unsigned long long)GetTickCount64(),                                \
          (unsigned long)GetCurrentThreadId());                                \
    }                                                                            \
  }

EIEM_DEFINE_GPU_AUX_WRAPPERS(TraceGpuV1, s_origGpuDrivenV1, "v1")
EIEM_DEFINE_GPU_AUX_WRAPPERS(TraceGpuV2, s_origGpuDrivenV2, "v2")
#undef EIEM_DEFINE_GPU_AUX_WRAPPERS

static void HookTraceMethod(void *klass, const char *methodName, int paramCount,
                            const char *label, void *detour, void **original) {
  if (!klass) return;
  void *method = FindMethodInHierarchy(klass, methodName, paramCount);
  if (!method) {
    Log("[RES-TRACE] %s not found", label);
    return;
  }
  if (Hook(method, label, detour, original))
    Log("[RES-TRACE] %s observation hook installed", label);
  else
    Log("[RES-TRACE] %s hook failed", label);
}

static void *FindMethodWithParamTypes(void *klass, const char *methodName,
                                      const char *const *paramTypes,
                                      int paramCount) {
  for (void *current = klass; current && il2cpp_class_get_methods;
       current = il2cpp_class_get_parent
                     ? il2cpp_class_get_parent(current)
                     : nullptr) {
    void *iterator = nullptr;
    void *method = nullptr;
    while ((method = il2cpp_class_get_methods(current, &iterator))) {
      const char *name = il2cpp_method_get_name(method);
      if (!name || strcmp(name, methodName) != 0 ||
          (int)il2cpp_method_get_param_count(method) != paramCount)
        continue;
      bool matches = true;
      for (int i = 0; i < paramCount; ++i) {
        void *type = il2cpp_method_get_param(method, (uint32_t)i);
        const char *typeName = type && il2cpp_type_get_name
                                   ? il2cpp_type_get_name(type)
                                   : nullptr;
        if (!typeName || strcmp(typeName, paramTypes[i]) != 0) {
          matches = false;
          break;
        }
      }
      if (matches)
        return method;
    }
  }
  return nullptr;
}

// Select the concrete overload when IL2CPP also exposes a stripped generic
// placeholder with the same name and parameter count.
static void *FindMethodWithReturnType(void *klass, const char *methodName,
                                      const char *returnType, int paramCount) {
  for (void *current = klass; current && il2cpp_class_get_methods;
       current = il2cpp_class_get_parent
                     ? il2cpp_class_get_parent(current)
                     : nullptr) {
    void *iterator = nullptr;
    void *method = nullptr;
    while ((method = il2cpp_class_get_methods(current, &iterator))) {
      const char *name = il2cpp_method_get_name(method);
      if (!name || strcmp(name, methodName) != 0 ||
          (int)il2cpp_method_get_param_count(method) != paramCount)
        continue;
      void *retType = il2cpp_method_get_return_type
                          ? il2cpp_method_get_return_type(method)
                          : nullptr;
      const char *retName = retType && il2cpp_type_get_name
                                ? il2cpp_type_get_name(retType)
                                : nullptr;
      if (retName && strcmp(retName, returnType) == 0 &&
          ((MInfo *)method)->mp)
        return method;
    }
  }
  return nullptr;
}

static void *FindMethodWithParamTypesAndReturnType(
    void *klass, const char *methodName, const char *const *paramTypes,
    int paramCount, const char *returnType) {  for (void *current = klass; current && il2cpp_class_get_methods;
       current = il2cpp_class_get_parent
                     ? il2cpp_class_get_parent(current)
                     : nullptr) {
    void *iterator = nullptr;
    void *method = nullptr;
    while ((method = il2cpp_class_get_methods(current, &iterator))) {
      const char *name = il2cpp_method_get_name(method);
      if (!name || strcmp(name, methodName) != 0 ||
          (int)il2cpp_method_get_param_count(method) != paramCount)
        continue;
      void *retType = il2cpp_method_get_return_type
                          ? il2cpp_method_get_return_type(method)
                          : nullptr;
      const char *retName = retType && il2cpp_type_get_name
                                ? il2cpp_type_get_name(retType)
                                : nullptr;
      if (!retName || !returnType || strcmp(retName, returnType) != 0)
        continue;
      bool matches = true;
      for (int index = 0; index < paramCount; ++index) {
        void *paramType = il2cpp_method_get_param(
            method, (uint32_t)index);
        const char *paramName = paramType && il2cpp_type_get_name
                                    ? il2cpp_type_get_name(paramType)
                                    : nullptr;
        if (!paramName || !paramTypes || !paramTypes[index] ||
            strcmp(paramName, paramTypes[index]) != 0) {
          matches = false;
          break;
        }
      }
      if (matches && ((MInfo *)method)->mp) return method;
    }
  }
  return nullptr;
}

static void HookTraceMethodWithParamTypes(
    void *klass, const char *methodName, const char *const *paramTypes,
    int paramCount, const char *label, void *detour, void **original) {
  if (!klass) return;
  void *method =
      FindMethodWithParamTypes(klass, methodName, paramTypes, paramCount);
  if (!method) {
    Log("[RES-TRACE] %s not found", label);
    return;
  }
  if (Hook(method, label, detour, original))
    Log("[RES-TRACE] %s observation hook installed", label);
  else
    Log("[RES-TRACE] %s hook failed", label);
}

static void HookTraceMethodWithParamTypesAndReturnType(
    void *klass, const char *methodName, const char *const *paramTypes,
    int paramCount, const char *label, const char *returnType, void *detour,
    void **original) {
  if (!klass) return;
  void *method = FindMethodWithParamTypesAndReturnType(
      klass, methodName, paramTypes, paramCount, returnType);
  if (!method) {
    Log("[RES-TRACE] %s not found", label);
    return;
  }
  if (Hook(method, label, detour, original))
    Log("[RES-TRACE] %s observation hook installed", label);
  else
    Log("[RES-TRACE] %s hook failed", label);
}

static void HookTraceGpuClothPipelineUpdateV2(void *klass,
                                              const char *const *paramTypes,
                                              int paramCount,
                                              const char *label,
                                              void *instanceDetour,
                                              void **instanceOriginal,
                                              void *staticDetour,
                                              void **staticOriginal) {
  if (!klass) return;
  void *method = FindMethodWithParamTypesAndReturnType(
      klass, "PipelineUpdateV2", paramTypes, paramCount, "System.Void");
  if (!method) {
    Log("[RES-TRACE] %s not found", label);
    return;
  }
  uint32_t impl = 0;
  const uint32_t flags = il2cpp_method_get_flags
                             ? il2cpp_method_get_flags(method, &impl)
                             : 0;
  const bool isStatic = (flags & 0x10u) != 0;
  Log("[RES-TRACE] %s flags=0x%X static=%d impl=0x%X", label, flags,
      isStatic ? 1 : 0, impl);
  void *detour = isStatic ? staticDetour : instanceDetour;
  void **original = isStatic ? staticOriginal : instanceOriginal;
  if (Hook(method, label, detour, original))
    Log("[RES-TRACE] %s observation hook installed", label);
  else
    Log("[RES-TRACE] %s hook failed", label);
}

// Native-only evidence for the renderer-side skin metadata.  The offsets and
// RVAs below come from the current UnityPlayer build and are used only by the
// disposable EIEM_NATIVE_BOUNDARY_STACKS_BUILD.  The hook reads the existing
// pointer/count and then calls the original function unchanged; it never
// writes a flag or repairs the renderer.
using EiemNativeSkinRecordBuildFn = int64_t(__fastcall *)(void *self);
using EiemNativeSkinMetadataResetFn = int64_t(__fastcall *)(void *self);
using EiemNativeSkinValidationFn = int64_t(__fastcall *)(void *self,
                                                         int32_t flags,
                                                         void *context);
using EiemNativeSkinModeFn = uint32_t(__fastcall *)(void *self);
using EiemNativeSkinSupportFn = uint8_t(__fastcall *)();
using EiemNativeFlagRecordAppendFn = int64_t(__fastcall *)(void *records,
                                                            const void *input);
using EiemNativeFlagDrawFlushFn = void *(__fastcall *)(void *context);
using EiemMeshSetterArrayFn = void (__fastcall *)(void *self, void *array,
                                                   void *methodInfo);
using EiemMeshSetterIntFn = void (__fastcall *)(void *self, int32_t value,
                                                 void *methodInfo);
using EiemMeshSetterArrayIntFn = void (__fastcall *)(void *self, void *array,
                                                      int32_t index,
                                                      void *methodInfo);
using EiemMeshSetterUvFn = void (__fastcall *)(void *self, int32_t channel,
                                                void *array,
                                                void *methodInfo);
using EiemMeshUploadFn = void (__fastcall *)(void *self, uint8_t readable,
                                              void *methodInfo);
using EiemMeshNoArgsFn = void (__fastcall *)(void *self, void *methodInfo);
using EiemMeshInternalBoneWeightsFn = void (__fastcall *)(
    void *self, void *bonesPerVertex, int32_t bonesPerVertexSize,
    void *weights, int32_t weightsSize, void *methodInfo);
static EiemNativeSkinRecordBuildFn s_origNativeSkinRecordBuild = nullptr;
static EiemNativeSkinMetadataResetFn s_origNativeSkinMetadataReset = nullptr;
static EiemNativeSkinValidationFn s_origNativeSkinValidation = nullptr;
static EiemNativeSkinModeFn s_origNativeSkinMode = nullptr;
static EiemNativeSkinSupportFn s_origNativeSkinSupport = nullptr;
static EiemNativeFlagRecordAppendFn s_origNativeFlagRecordAppend = nullptr;
static EiemNativeFlagDrawFlushFn s_origNativeFlagDrawFlush = nullptr;
using EiemNativeMesh1c8NormalizerFn = uint8_t(__fastcall *)(void *self);
static EiemNativeMesh1c8NormalizerFn s_origNativeMesh1c8Normalizer = nullptr;
using EiemNativeMeshCtorFn = int64_t(__fastcall *)(void *self,
                                                   uint32_t arg1,
                                                   uint32_t arg2);
static EiemNativeMeshCtorFn s_origNativeMeshCtor = nullptr;
using EiemNativeMeshDeserializeFn = int64_t(__fastcall *)(void *self,
                                                          void *stream);
static EiemNativeMeshDeserializeFn s_origNativeMeshDeserialize = nullptr;
struct EiemNativeMeshDeserializeRecord {
  volatile LONG ready;
  uintptr_t native;
  uintptr_t vtable;
  uintptr_t stream;
  uintptr_t cursorBefore;
  uintptr_t cursorAfter;
  uintptr_t streamBegin;
  uintptr_t streamEnd;
  uintptr_t returnAddress;
  void *stack[6];
  DWORD threadId;
  USHORT stackSize;
  int32_t field1c8Before;
  int32_t field1c8After;
  int32_t field110After;
  int32_t field124After;
};
static constexpr LONG kEiemNativeMeshDeserializeCapacity = 16384;
static EiemNativeMeshDeserializeRecord
    s_eiemNativeMeshDeserializeRecords[kEiemNativeMeshDeserializeCapacity] = {};
static volatile LONG s_eiemNativeMeshDeserializeCalls = 0;
struct EiemNativeMeshCtorRecord {
  volatile LONG ready;
  uintptr_t native;
  uintptr_t vtable;
  uintptr_t returnAddress;
  void *stack[6];
  DWORD threadId;
  USHORT stackSize;
};
static EiemNativeMeshCtorRecord
    s_eiemNativeMeshCtorRecords[kEiemNativeMeshDeserializeCapacity] = {};
static volatile LONG s_eiemNativeMeshCtorTraceCalls = 0;
using EiemNativeMeshPayloadFn = int64_t(__fastcall *)(void *self,
                                                       void *context);
static EiemNativeMeshPayloadFn s_origNativeMeshFieldVisitor = nullptr;
static EiemNativeMeshPayloadFn s_origNativeMeshBinaryStream = nullptr;
struct EiemNativeMeshPayloadRecord {
  volatile LONG ready;
  uintptr_t native;
  uintptr_t context;
  uintptr_t returnAddress;
  uintptr_t namePointerAfter;
  uintptr_t cursorBefore;
  uintptr_t cursorAfter;
  uintptr_t streamBaseBefore;
  uintptr_t streamBaseAfter;
  uintptr_t streamLimitBefore;
  uintptr_t streamLimitAfter;
  int32_t field1c8Before;
  int32_t field1c8After;
  int32_t field110After;
};
static EiemNativeMeshPayloadRecord
    s_eiemNativeMeshFieldVisitorRecords[kEiemNativeMeshDeserializeCapacity] = {};
static EiemNativeMeshPayloadRecord
    s_eiemNativeMeshBinaryStreamRecords[kEiemNativeMeshDeserializeCapacity] = {};
static volatile LONG s_eiemNativeMeshFieldVisitorCalls = 0;
static volatile LONG s_eiemNativeMeshBinaryStreamCalls = 0;
static SRWLOCK s_eiemNativeMeshDeserializeSourceLock = SRWLOCK_INIT;
struct EiemNativeMeshDeserializeSourceSeen {
  uintptr_t native;
  LONG generation;
};
static EiemNativeMeshDeserializeSourceSeen
    s_eiemNativeMeshDeserializeSources[128] = {};
static size_t s_eiemNativeMeshDeserializeSourceCount = 0;
static volatile LONG s_eiemNativeFlagSourceAppendCount = 0;
static volatile LONG s_eiemNativeFlagSourceFlushCount = 0;
static volatile LONG s_eiemNativeFlagDrawTraceCount = 0;
static volatile LONG s_eiemNativeFlagDrawTraceTransaction = 0;
static volatile LONG64 s_eiemNativeFlagTraceWindowStartTick = 0;
static volatile LONG s_eiemNativeSkinSupportCalls = 0;
static volatile LONG s_eiemNativeSkinSupportLast = -1;
static volatile LONG s_eiemNativeSkinRecordProbeCalls = 0;
static volatile LONG s_eiemNativeSkinRecordProbeLastTransaction = 0;
static volatile LONG s_eiemNativeSkinRecordProbeTransactionMatches = 0;
static volatile LONG s_eiemNativeSkinModeProbeLastTransaction = 0;
static volatile LONG s_eiemNativeSkinModeProbeTransactionMatches = 0;
static volatile LONG s_eiemNativeSkinModeAnomalyLastTransaction = 0;
static volatile LONG s_eiemNativeSkinModeAnomalyMatches = 0;
static volatile LONG s_eiemNativeSkinResetLastTransaction = 0;
static volatile LONG s_eiemNativeSkinResetTransactionMatches = 0;
static volatile LONG s_eiemNativeSkinValidationLogs = 0;
static volatile LONG s_eiemNativeSkinValidationLastField = -1;
static volatile LONG s_eiemNativeMesh1c8CallLogs = 0;
static volatile LONG s_eiemNativeMeshCtorLogs = 0;
static volatile LONG s_eiemNativeSkinC7LastTransaction = 0;
static volatile LONG s_eiemNativeSkinC7Logs = 0;
static volatile LONG s_eiemNativeSkinC7ZeroLogs = 0;
static volatile LONG64 s_eiemNativeSkinC7WindowStartTick = -1;
static EiemMeshSetterArrayFn s_origMeshSetVerticesTrace = nullptr;
static EiemMeshSetterArrayFn s_origMeshSetNormalsTrace = nullptr;
static EiemMeshSetterArrayFn s_origMeshSetTangentsTrace = nullptr;
static EiemMeshSetterArrayFn s_origMeshSetColorsTrace = nullptr;
static EiemMeshSetterArrayFn s_origMeshSetBoneWeightsTrace = nullptr;
static EiemMeshSetterArrayFn s_origMeshSetBindPosesTrace = nullptr;
static EiemMeshSetterIntFn s_origMeshSetSubMeshCountTrace = nullptr;
static EiemMeshSetterArrayIntFn s_origMeshSetTrianglesTrace = nullptr;
static EiemMeshSetterUvFn s_origMeshSetUVs2Trace = nullptr;
static EiemMeshSetterUvFn s_origMeshSetUVs3Trace = nullptr;
static EiemMeshSetterUvFn s_origMeshSetUVs4Trace = nullptr;
static EiemMeshInternalBoneWeightsFn s_origMeshInternalBoneWeightsTrace = nullptr;
static EiemMeshUploadFn s_origMeshUploadTrace = nullptr;
static EiemMeshNoArgsFn s_origMeshRecalculateBoundsTrace = nullptr;
static volatile LONG s_eiemMeshSetterTraceLogs = 0;

static uintptr_t EiemTraceManagedMeshNative(void *mesh) {
  if (!mesh || !s_eiemMeshHGGetPtrUnchecked || !il2cpp_object_unbox)
    return 0;
  void *boxed = EiemBackendInvokeNoThrow(s_eiemMeshHGGetPtrUnchecked, mesh);
  void *value = boxed ? il2cpp_object_unbox(boxed) : nullptr;
  uintptr_t native = 0;
  if (value) {
    __try { memcpy(&native, value, sizeof(native)); }
    __except (EXCEPTION_EXECUTE_HANDLER) { native = 0; }
  }
  return native;
}

static int32_t EiemTraceNativeField(uintptr_t native, size_t offset) {
  return static_cast<int32_t>(EiemReadNativeInt32Field(native, offset));
}

static int64_t EiemTraceNativeMeshPayloadCall(
    void *self, void *context, EiemNativeMeshPayloadFn original,
    EiemNativeMeshPayloadRecord *records, volatile LONG *counter,
    uintptr_t returnAddress) {
  const uintptr_t native = reinterpret_cast<uintptr_t>(self);
  const int32_t field1c8Before = EiemTraceNativeField(native, 0x1C8);
  const uintptr_t stream = reinterpret_cast<uintptr_t>(context);
  const uintptr_t cursorBefore = EiemReadNativePointerField(stream, 0x30);
  const uintptr_t streamBaseBefore = EiemReadNativePointerField(stream, 0x38);
  const uintptr_t streamLimitBefore = EiemReadNativePointerField(stream, 0x40);
  const int64_t result = original ? original(self, context) : 0;
  const LONG ordinal = InterlockedIncrement(counter);
  if (ordinal <= 0 || ordinal > kEiemNativeMeshDeserializeCapacity)
    return result;
  auto &record = records[ordinal - 1];
  record.native = native;
  record.context = stream;
  record.returnAddress = returnAddress;
  record.namePointerAfter = EiemReadNativePointerField(native, 0x30);
  record.cursorBefore = cursorBefore;
  record.cursorAfter = EiemReadNativePointerField(stream, 0x30);
  record.streamBaseBefore = streamBaseBefore;
  record.streamBaseAfter = EiemReadNativePointerField(stream, 0x38);
  record.streamLimitBefore = streamLimitBefore;
  record.streamLimitAfter = EiemReadNativePointerField(stream, 0x40);
  record.field1c8Before = field1c8Before;
  record.field1c8After = EiemTraceNativeField(native, 0x1C8);
  record.field110After = EiemTraceNativeField(native, 0x110);
  InterlockedExchange(&record.ready, 1);
  return result;
}

static int64_t __fastcall TraceNativeMeshFieldVisitor(void *self,
                                                       void *context) {
  return EiemTraceNativeMeshPayloadCall(
      self, context, s_origNativeMeshFieldVisitor,
      s_eiemNativeMeshFieldVisitorRecords,
      &s_eiemNativeMeshFieldVisitorCalls,
      reinterpret_cast<uintptr_t>(_ReturnAddress()));
}

static int64_t __fastcall TraceNativeMeshBinaryStream(void *self,
                                                       void *context) {
  return EiemTraceNativeMeshPayloadCall(
      self, context, s_origNativeMeshBinaryStream,
      s_eiemNativeMeshBinaryStreamRecords,
      &s_eiemNativeMeshBinaryStreamCalls,
      reinterpret_cast<uintptr_t>(_ReturnAddress()));
}

static int64_t __fastcall TraceNativeMeshDeserialize(void *self,
                                                      void *stream) {
  const uintptr_t native = reinterpret_cast<uintptr_t>(self);
  const uintptr_t streamAddress = reinterpret_cast<uintptr_t>(stream);
  const int32_t before = EiemTraceNativeField(native, 0x1C8);
  const uintptr_t cursorBefore =
      EiemReadNativePointerField(streamAddress, 0x30);
  auto original = s_origNativeMeshDeserialize;
  const int64_t result = original ? original(self, stream) : 0;
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeMeshDeserializeCalls);
  if (ordinal <= 0 || ordinal > kEiemNativeMeshDeserializeCapacity) return result;

  auto &record = s_eiemNativeMeshDeserializeRecords[ordinal - 1];
  record.native = native;
  record.vtable = EiemReadNativePointerField(native, 0);
  record.stream = streamAddress;
  record.cursorBefore = cursorBefore;
  record.cursorAfter = EiemReadNativePointerField(streamAddress, 0x30);
  record.streamBegin = EiemReadNativePointerField(streamAddress, 0x38);
  record.streamEnd = EiemReadNativePointerField(streamAddress, 0x40);
  record.returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
  record.threadId = GetCurrentThreadId();
  record.field1c8Before = before;
  record.field1c8After = EiemTraceNativeField(native, 0x1C8);
  record.field110After = EiemTraceNativeField(native, 0x110);
  record.field124After = EiemTraceNativeField(native, 0x124);
  record.stackSize = RtlCaptureStackBackTrace(1, 6, record.stack, nullptr);
  InterlockedExchange(&record.ready, 1);
  return result;
}

static void EiemReportNativeMeshDeserializeSource(void *mesh,
                                                  const char *source,
                                                  const char *asset,
                                                  const char *section) {
  if (!kEiemEnableNativeMeshDeserializeTrace || !mesh) return;
  const uintptr_t native = reinterpret_cast<uintptr_t>(
      EiemGetNativeMeshPointer(mesh));
  const LONG generation = InterlockedCompareExchange(
      &s_eiemModGeneration, 0, 0);
  AcquireSRWLockExclusive(&s_eiemNativeMeshDeserializeSourceLock);
  for (size_t i = 0; i < s_eiemNativeMeshDeserializeSourceCount; ++i) {
    const auto &seen = s_eiemNativeMeshDeserializeSources[i];
    if (seen.native == native && seen.generation == generation) {
      ReleaseSRWLockExclusive(&s_eiemNativeMeshDeserializeSourceLock);
      return;
    }
  }
  if (s_eiemNativeMeshDeserializeSourceCount >=
      _countof(s_eiemNativeMeshDeserializeSources)) {
    ReleaseSRWLockExclusive(&s_eiemNativeMeshDeserializeSourceLock);
    return;
  }
  const size_t sourceOrdinal = s_eiemNativeMeshDeserializeSourceCount++;
  s_eiemNativeMeshDeserializeSources[sourceOrdinal] = {native, generation};
  ReleaseSRWLockExclusive(&s_eiemNativeMeshDeserializeSourceLock);

  const LONG calls = InterlockedCompareExchange(
      &s_eiemNativeMeshDeserializeCalls, 0, 0);
  const LONG ctorCalls = InterlockedCompareExchange(
      &s_eiemNativeMeshCtorTraceCalls, 0, 0);
  const LONG stored = calls < kEiemNativeMeshDeserializeCapacity
      ? calls : kEiemNativeMeshDeserializeCapacity;
  const LONG storedCtor = ctorCalls < kEiemNativeMeshDeserializeCapacity
      ? ctorCalls : kEiemNativeMeshDeserializeCapacity;
  const EiemNativeMeshCtorRecord *ctorMatch = nullptr;
  LONG ctorMatchOrdinal = 0;
  for (LONG ordinal = storedCtor; ordinal > 0; --ordinal) {
    auto &record = s_eiemNativeMeshCtorRecords[ordinal - 1];
    if (InterlockedCompareExchange(&record.ready, 0, 0) &&
        record.native == native) {
      ctorMatch = &record;
      ctorMatchOrdinal = ordinal;
      break;
    }
  }
  const EiemNativeMeshDeserializeRecord *match = nullptr;
  LONG matchOrdinal = 0;
  for (LONG ordinal = stored; ordinal > 0; --ordinal) {
    auto &record = s_eiemNativeMeshDeserializeRecords[ordinal - 1];
    if (InterlockedCompareExchange(&record.ready, 0, 0) &&
        record.native == native) {
      match = &record;
      matchOrdinal = ordinal;
      break;
    }
  }
  const uintptr_t base = reinterpret_cast<uintptr_t>(
      GetModuleHandleW(L"UnityPlayer.dll"));
  const uintptr_t vtable = EiemReadNativePointerField(native, 0);
  const uintptr_t ctorReturnRva = ctorMatch && base &&
      ctorMatch->returnAddress >= base
      ? ctorMatch->returnAddress - base : 0;
  const auto payloadMatch = [native](
      const EiemNativeMeshPayloadRecord *records,
      LONG calls) -> const EiemNativeMeshPayloadRecord * {
    const LONG stored = calls < kEiemNativeMeshDeserializeCapacity
        ? calls : kEiemNativeMeshDeserializeCapacity;
    for (LONG ordinal = stored; ordinal > 0; --ordinal) {
      const auto &record = records[ordinal - 1];
      if (InterlockedCompareExchange(
              const_cast<volatile LONG *>(&record.ready), 0, 0) &&
          record.native == native)
        return &record;
    }
    return nullptr;
  };
  const LONG fieldVisitorCalls = InterlockedCompareExchange(
      &s_eiemNativeMeshFieldVisitorCalls, 0, 0);
  const LONG binaryStreamCalls = InterlockedCompareExchange(
      &s_eiemNativeMeshBinaryStreamCalls, 0, 0);
  const auto *fieldVisitorMatch = payloadMatch(
      s_eiemNativeMeshFieldVisitorRecords, fieldVisitorCalls);
  const auto *binaryStreamMatch = payloadMatch(
      s_eiemNativeMeshBinaryStreamRecords, binaryStreamCalls);
  const auto payloadMatchOrdinal = [](const EiemNativeMeshPayloadRecord *records,
                               const EiemNativeMeshPayloadRecord *match) -> LONG {
    return match ? static_cast<LONG>(match - records + 1) : 0;
  };
  // Only the source Mesh already selected by a Render rule is decoded here.
  // The object owns the name pointer until this report, so no global scan or
  // speculative dereference of arbitrary stream bytes is needed.
  char binaryStreamName[96] = {};
  if (binaryStreamMatch) {
    const uintptr_t namePointer = binaryStreamMatch->namePointerAfter;
    if (namePointer >= 0x10000) {
      __try {
        const char *name = reinterpret_cast<const char *>(namePointer);
        size_t i = 0;
        for (; i + 1 < sizeof(binaryStreamName); ++i) {
          const unsigned char c = static_cast<unsigned char>(name[i]);
          if (!c) break;
          if (c < 0x20 || c > 0x7E) break;
          binaryStreamName[i] = static_cast<char>(c);
        }
        binaryStreamName[i] = '\0';
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        binaryStreamName[0] = '\0';
      }
    }
  }
  Log("[MESH-PAYLOAD-PATH-v1] sourceOrdinal=%zu asset=%s native=%p "
      "fieldVisitorMatch=%ld fieldVisitorCalls=%ld fieldVisitorOverflow=%d "
      "fieldVisitorContext=%p fieldVisitorReturnRva=0x%llX "
      "fieldVisitor1c8=%d..%d fieldVisitor110=%d "
      "binaryStreamMatch=%ld binaryStreamCalls=%ld binaryStreamOverflow=%d "
      "binaryStreamContext=%p binaryStreamReturnRva=0x%llX "
      "binaryStream1c8=%d..%d binaryStream110=%d",
      sourceOrdinal + 1, asset ? asset : "", reinterpret_cast<void *>(native),
      payloadMatchOrdinal(s_eiemNativeMeshFieldVisitorRecords, fieldVisitorMatch),
      fieldVisitorCalls,
      fieldVisitorCalls > kEiemNativeMeshDeserializeCapacity ? 1 : 0,
      reinterpret_cast<void *>(fieldVisitorMatch ? fieldVisitorMatch->context : 0),
      static_cast<unsigned long long>(fieldVisitorMatch && base &&
          fieldVisitorMatch->returnAddress >= base
          ? fieldVisitorMatch->returnAddress - base : 0),
      fieldVisitorMatch ? fieldVisitorMatch->field1c8Before : 0,
      fieldVisitorMatch ? fieldVisitorMatch->field1c8After : 0,
      fieldVisitorMatch ? fieldVisitorMatch->field110After : 0,
      payloadMatchOrdinal(s_eiemNativeMeshBinaryStreamRecords, binaryStreamMatch),
      binaryStreamCalls,
      binaryStreamCalls > kEiemNativeMeshDeserializeCapacity ? 1 : 0,
      reinterpret_cast<void *>(binaryStreamMatch ? binaryStreamMatch->context : 0),
      static_cast<unsigned long long>(binaryStreamMatch && base &&
          binaryStreamMatch->returnAddress >= base
          ? binaryStreamMatch->returnAddress - base : 0),
      binaryStreamMatch ? binaryStreamMatch->field1c8Before : 0,
      binaryStreamMatch ? binaryStreamMatch->field1c8After : 0,
      binaryStreamMatch ? binaryStreamMatch->field110After : 0);
  if (binaryStreamMatch) {
    Log("[MESH-PAYLOAD-IDENTITY-v1] asset=%s native=%p "
        "namePtr=%p currentNamePtr=%p name=%s stream=%p "
        "cursor=%p..%p base=%p..%p limit=%p..%p",
        asset ? asset : "", reinterpret_cast<void *>(native),
        reinterpret_cast<void *>(binaryStreamMatch->namePointerAfter),
        reinterpret_cast<void *>(EiemReadNativePointerField(native, 0x30)),
        binaryStreamName[0] ? binaryStreamName : "<empty-or-unreadable>",
        reinterpret_cast<void *>(binaryStreamMatch->context),
        reinterpret_cast<void *>(binaryStreamMatch->cursorBefore),
        reinterpret_cast<void *>(binaryStreamMatch->cursorAfter),
        reinterpret_cast<void *>(binaryStreamMatch->streamBaseBefore),
        reinterpret_cast<void *>(binaryStreamMatch->streamBaseAfter),
        reinterpret_cast<void *>(binaryStreamMatch->streamLimitBefore),
        reinterpret_cast<void *>(binaryStreamMatch->streamLimitAfter));
  }
  const auto ctorStackRva = [base, ctorMatch](int index) -> uintptr_t {
    const uintptr_t address = ctorMatch && index < ctorMatch->stackSize
        ? reinterpret_cast<uintptr_t>(ctorMatch->stack[index]) : 0;
    return base && address >= base ? address - base : 0;
  };
  if (!match) {
    Log("[MESH-DESERIALIZE-v1] sourceOrdinal=%zu source=%s asset=%s "
        "section=%s generation=%ld managed=%p native=%p vtableRva=0x%llX "
        "match=none calls=%ld ctorMatch=%ld ctorCalls=%ld "
        "ctorReturnRva=0x%llX ctorStackRva="
        "0x%llX,0x%llX,0x%llX,0x%llX,0x%llX,0x%llX "
        "capacity=%ld truncated=%d ctorTruncated=%d",
        sourceOrdinal + 1, source ? source : "", asset ? asset : "",
        section ? section : "", generation, mesh,
        reinterpret_cast<void *>(native),
        static_cast<unsigned long long>(vtable >= base ? vtable - base : 0),
        calls, ctorMatchOrdinal, ctorCalls,
        static_cast<unsigned long long>(ctorReturnRva),
        static_cast<unsigned long long>(ctorStackRva(0)),
        static_cast<unsigned long long>(ctorStackRva(1)),
        static_cast<unsigned long long>(ctorStackRva(2)),
        static_cast<unsigned long long>(ctorStackRva(3)),
        static_cast<unsigned long long>(ctorStackRva(4)),
        static_cast<unsigned long long>(ctorStackRva(5)),
        kEiemNativeMeshDeserializeCapacity,
        calls > kEiemNativeMeshDeserializeCapacity ? 1 : 0,
        ctorCalls > kEiemNativeMeshDeserializeCapacity ? 1 : 0);
    return;
  }
  const auto stackRva = [base, match](int index) -> uintptr_t {
    const uintptr_t address = index < match->stackSize
        ? reinterpret_cast<uintptr_t>(match->stack[index]) : 0;
    return base && address >= base ? address - base : 0;
  };
  Log("[MESH-DESERIALIZE-v1] sourceOrdinal=%zu source=%s asset=%s "
      "section=%s generation=%ld managed=%p native=%p vtableRva=0x%llX "
      "match=%ld calls=%ld ctorMatch=%ld ctorCalls=%ld "
      "ctorReturnRva=0x%llX ctorStackRva="
      "0x%llX,0x%llX,0x%llX,0x%llX,0x%llX,0x%llX "
      "thread=%lu stream=%p cursor=%p..%p "
      "streamBounds=%p..%p returnRva=0x%llX stackRva="
      "0x%llX,0x%llX,0x%llX,0x%llX,0x%llX,0x%llX "
      "field1c8=%d..%d field110=%d field124=%d",
      sourceOrdinal + 1, source ? source : "", asset ? asset : "",
      section ? section : "", generation, mesh,
      reinterpret_cast<void *>(native),
      static_cast<unsigned long long>(vtable >= base ? vtable - base : 0),
      matchOrdinal, calls, ctorMatchOrdinal, ctorCalls,
      static_cast<unsigned long long>(ctorReturnRva),
      static_cast<unsigned long long>(ctorStackRva(0)),
      static_cast<unsigned long long>(ctorStackRva(1)),
      static_cast<unsigned long long>(ctorStackRva(2)),
      static_cast<unsigned long long>(ctorStackRva(3)),
      static_cast<unsigned long long>(ctorStackRva(4)),
      static_cast<unsigned long long>(ctorStackRva(5)),
      (unsigned long)match->threadId,
      reinterpret_cast<void *>(match->stream),
      reinterpret_cast<void *>(match->cursorBefore),
      reinterpret_cast<void *>(match->cursorAfter),
      reinterpret_cast<void *>(match->streamBegin),
      reinterpret_cast<void *>(match->streamEnd),
      static_cast<unsigned long long>(match->returnAddress >= base
          ? match->returnAddress - base : 0),
      static_cast<unsigned long long>(stackRva(0)),
      static_cast<unsigned long long>(stackRva(1)),
      static_cast<unsigned long long>(stackRva(2)),
      static_cast<unsigned long long>(stackRva(3)),
      static_cast<unsigned long long>(stackRva(4)),
      static_cast<unsigned long long>(stackRva(5)),
      match->field1c8Before, match->field1c8After,
      match->field110After, match->field124After);
}

static void EiemTraceMeshSetter(const char *name, void *mesh,
                                uintptr_t before, uintptr_t after) {
  if (!kEiemEnableNativeMeshSetterTrace ||
      !EiemIsReplacementManagedMesh(mesh)) return;
  const LONG ordinal = InterlockedIncrement(&s_eiemMeshSetterTraceLogs);
  if (ordinal > 256) return;
  Log("[NATIVE-MESH-SETTER-v1] ordinal=%ld setter=%s mesh=%p "
      "nativeBefore=%p field1c8Before=%d nativeAfter=%p "
      "field1c8After=%d dataBefore=%p dataAfter=%p",
      ordinal, name ? name : "unknown", mesh, (void *)before,
      EiemTraceNativeField(before, 0x1C8), (void *)after,
      EiemTraceNativeField(after, 0x1C8),
      (void *)EiemReadNativePointerField(before, 0x38),
      (void *)EiemReadNativePointerField(after, 0x38));
}

#define EIEM_MESH_ARRAY_TRACE(NAME, ORIGINAL, LABEL)                         \
  static void __fastcall NAME(void *self, void *array, void *methodInfo) {   \
    const uintptr_t before = EiemTraceManagedMeshNative(self);               \
    auto original = ORIGINAL;                                                \
    if (original) original(self, array, methodInfo);                         \
    const uintptr_t after = EiemTraceManagedMeshNative(self);                \
    EiemTraceMeshSetter(LABEL, self, before, after);                         \
  }
EIEM_MESH_ARRAY_TRACE(TraceMeshSetVertices, s_origMeshSetVerticesTrace,
                      "set_vertices")
EIEM_MESH_ARRAY_TRACE(TraceMeshSetNormals, s_origMeshSetNormalsTrace,
                      "set_normals")
EIEM_MESH_ARRAY_TRACE(TraceMeshSetTangents, s_origMeshSetTangentsTrace,
                      "set_tangents")
EIEM_MESH_ARRAY_TRACE(TraceMeshSetColors, s_origMeshSetColorsTrace,
                      "set_colors")
EIEM_MESH_ARRAY_TRACE(TraceMeshSetBoneWeights, s_origMeshSetBoneWeightsTrace,
                      "set_boneWeights")
EIEM_MESH_ARRAY_TRACE(TraceMeshSetBindPoses, s_origMeshSetBindPosesTrace,
                      "set_bindposes")
#undef EIEM_MESH_ARRAY_TRACE

static void __fastcall TraceMeshSetSubMeshCount(void *self, int32_t value,
                                                 void *methodInfo) {
  const uintptr_t before = EiemTraceManagedMeshNative(self);
  auto original = s_origMeshSetSubMeshCountTrace;
  if (original) original(self, value, methodInfo);
  const uintptr_t after = EiemTraceManagedMeshNative(self);
  EiemTraceMeshSetter("set_subMeshCount", self, before, after);
}
static void __fastcall TraceMeshSetTriangles(void *self, void *array,
                                             int32_t index, void *methodInfo) {
  const uintptr_t before = EiemTraceManagedMeshNative(self);
  auto original = s_origMeshSetTrianglesTrace;
  if (original) original(self, array, index, methodInfo);
  const uintptr_t after = EiemTraceManagedMeshNative(self);
  EiemTraceMeshSetter("SetTriangles", self, before, after);
}
#define EIEM_MESH_UV_TRACE(NAME, ORIGINAL, LABEL)                            \
  static void __fastcall NAME(void *self, int32_t channel, void *array,      \
                               void *methodInfo) {                            \
    const uintptr_t before = EiemTraceManagedMeshNative(self);               \
    auto original = ORIGINAL;                                                \
    if (original) original(self, channel, array, methodInfo);                 \
    const uintptr_t after = EiemTraceManagedMeshNative(self);                \
    EiemTraceMeshSetter(LABEL, self, before, after);                         \
  }
EIEM_MESH_UV_TRACE(TraceMeshSetUVs2, s_origMeshSetUVs2Trace, "SetUVs(Vector2)")
EIEM_MESH_UV_TRACE(TraceMeshSetUVs3, s_origMeshSetUVs3Trace, "SetUVs(Vector3)")
EIEM_MESH_UV_TRACE(TraceMeshSetUVs4, s_origMeshSetUVs4Trace, "SetUVs(Vector4)")
#undef EIEM_MESH_UV_TRACE

static void __fastcall TraceMeshInternalSetBoneWeights(
    void *self, void *bonesPerVertex, int32_t bonesPerVertexSize,
    void *weights, int32_t weightsSize, void *methodInfo) {
  const uintptr_t before = EiemTraceManagedMeshNative(self);
  auto original = s_origMeshInternalBoneWeightsTrace;
  if (original)
    original(self, bonesPerVertex, bonesPerVertexSize, weights, weightsSize,
             methodInfo);
  const uintptr_t after = EiemTraceManagedMeshNative(self);
  EiemTraceMeshSetter("InternalSetBoneWeights", self, before, after);
}
static void __fastcall TraceMeshUpload(void *self, uint8_t readable,
                                       void *methodInfo) {
  const uintptr_t before = EiemTraceManagedMeshNative(self);
  auto original = s_origMeshUploadTrace;
  if (original) original(self, readable, methodInfo);
  const uintptr_t after = EiemTraceManagedMeshNative(self);
  EiemTraceMeshSetter("UploadMeshData", self, before, after);
}
static void __fastcall TraceMeshRecalculateBounds(void *self,
                                                  void *methodInfo) {
  const uintptr_t before = EiemTraceManagedMeshNative(self);
  auto original = s_origMeshRecalculateBoundsTrace;
  if (original) original(self, methodInfo);
  const uintptr_t after = EiemTraceManagedMeshNative(self);
  EiemTraceMeshSetter("RecalculateBounds", self, before, after);
}

static void EiemInstallMeshSetterTrace() {
  if (!kEiemEnableNativeMeshSetterTrace) return;
  struct Entry { void *method; const char *label; void *detour; void **original; };
  const Entry entries[] = {
      {s_eiemMeshSetVertices, "Mesh.set_vertices", (void *)TraceMeshSetVertices,
       (void **)&s_origMeshSetVerticesTrace},
      {s_eiemMeshSetNormals, "Mesh.set_normals", (void *)TraceMeshSetNormals,
       (void **)&s_origMeshSetNormalsTrace},
      {s_eiemMeshSetTangents, "Mesh.set_tangents", (void *)TraceMeshSetTangents,
       (void **)&s_origMeshSetTangentsTrace},
      {s_eiemMeshSetColors, "Mesh.set_colors", (void *)TraceMeshSetColors,
       (void **)&s_origMeshSetColorsTrace},
      {s_eiemMeshSetBoneWeights, "Mesh.set_boneWeights",
       (void *)TraceMeshSetBoneWeights, (void **)&s_origMeshSetBoneWeightsTrace},
      {s_eiemMeshSetBindPoses, "Mesh.set_bindposes", (void *)TraceMeshSetBindPoses,
       (void **)&s_origMeshSetBindPosesTrace},
      {s_eiemMeshSetSubMeshCount, "Mesh.set_subMeshCount",
       (void *)TraceMeshSetSubMeshCount, (void **)&s_origMeshSetSubMeshCountTrace},
      {s_eiemMeshSetTriangles, "Mesh.SetTriangles", (void *)TraceMeshSetTriangles,
       (void **)&s_origMeshSetTrianglesTrace},
      {s_eiemMeshSetUVs2, "Mesh.SetUVs(Vector2)", (void *)TraceMeshSetUVs2,
       (void **)&s_origMeshSetUVs2Trace},
      {s_eiemMeshSetUVs3, "Mesh.SetUVs(Vector3)", (void *)TraceMeshSetUVs3,
       (void **)&s_origMeshSetUVs3Trace},
      {s_eiemMeshSetUVs4, "Mesh.SetUVs(Vector4)", (void *)TraceMeshSetUVs4,
       (void **)&s_origMeshSetUVs4Trace},
      {s_eiemMeshInternalSetBoneWeights, "Mesh.InternalSetBoneWeights",
       (void *)TraceMeshInternalSetBoneWeights,
       (void **)&s_origMeshInternalBoneWeightsTrace},
      {s_eiemMeshUploadMeshData, "Mesh.UploadMeshData", (void *)TraceMeshUpload,
       (void **)&s_origMeshUploadTrace},
      {g_mesh_recalculateBounds, "Mesh.RecalculateBounds",
       (void *)TraceMeshRecalculateBounds,
       (void **)&s_origMeshRecalculateBoundsTrace},
  };
  for (const auto &entry : entries) {
    if (!entry.method) continue;
    void *target = ((MInfo *)entry.method)->mp;
    if (!target) {
      Log("[NATIVE-MESH-SETTER] %s no method pointer", entry.label);
      continue;
    }
    const MH_STATUS status = MH_CreateHook(target, entry.detour, entry.original);
    if (status == MH_OK) {
      const MH_STATUS enable = MH_EnableHook(target);
      Log("[NATIVE-MESH-SETTER] %s target=%p create=%d enable=%d",
          entry.label, target, (int)status, (int)enable);
    } else {
      Log("[NATIVE-MESH-SETTER] %s target=%p create=%d", entry.label,
          target, (int)status);
    }
  }
}

static uint8_t EiemReadUnityByteRva(uintptr_t rva) {
  HMODULE unity = GetModuleHandleW(L"UnityPlayer.dll");
  if (!unity) return 0;
  __try { return *(volatile uint8_t *)(reinterpret_cast<uintptr_t>(unity) + rva); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static void *EiemReadUnityPointerRva(uintptr_t rva) {
  HMODULE unity = GetModuleHandleW(L"UnityPlayer.dll");
  if (!unity) return nullptr;
  __try {
    return *(void **)(reinterpret_cast<uintptr_t>(unity) + rva);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

static uint32_t EiemReadUnityU32Rva(uintptr_t rva) {
  HMODULE unity = GetModuleHandleW(L"UnityPlayer.dll");
  if (!unity) return 0;
  __try {
    return *(volatile uint32_t *)(reinterpret_cast<uintptr_t>(unity) + rva);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

// UnityPlayer!0x41FD40 is the no-argument capability gate used by
// C7B750. It is the only operation in this probe that runs on the game's
// native path; the detour returns the original value unchanged.
static uint8_t __fastcall TraceNativeSkinSupport() {
  auto original = s_origNativeSkinSupport;
  const uint8_t result = original ? original() : 0;
  if (!kEiemEnableNativeSkinSupportProbe) return result;
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeSkinSupportCalls);
  if (ordinal > 128) return result;
  const LONG current = result ? 1 : 0;
  const LONG previous = InterlockedExchange(&s_eiemNativeSkinSupportLast,
                                             current);
  if (previous == current) return result;
  const uint8_t gateA = EiemReadUnityByteRva(0x1CF5CC4);
  const uint8_t gateB = EiemReadUnityByteRva(0x1CF5CC5);
  const uint8_t gateC = EiemReadUnityByteRva(0x1C59C90);
  void *frames[6] = {};
  const USHORT frameCount = CaptureStackBackTrace(
      1, static_cast<DWORD>(_countof(frames)), frames, nullptr);
  char stack[256] = {};
  size_t used = 0;
  for (USHORT index = 0; index < frameCount; ++index) {
    const int written = _snprintf_s(
        stack + used, sizeof(stack) - used, _TRUNCATE, "%s%p",
        index ? "," : "", frames[index]);
    if (written <= 0) break;
    used += static_cast<size_t>(written);
    if (used + 24 >= sizeof(stack)) break;
  }
  Log("[NATIVE-SKIN-SUPPORT-v1] ordinal=%ld result=%u gateA=%u gateB=%u "
      "gateC=%u return=%p stack=%s",
      ordinal, result ? 1 : 0, gateA, gateB, gateC, _ReturnAddress(),
      stack[0] ? stack : "<empty>");
  return result;
}

// The native trace is armed by the existing F10 transaction.  Keep the
// window time-based rather than count-based: every append/flush during the
// first three seconds is observable, while idle gameplay produces no trace.
static bool EiemNativeFlagTraceWindowActive(LONG *transactionOut) {
  const LONG transaction = InterlockedCompareExchange(
      &s_eiemSkinTargetTransaction, 0, 0);
  if (transaction <= 0) return false;
  const LONG previousTransaction = InterlockedExchange(
      &s_eiemNativeFlagDrawTraceTransaction, transaction);
  if (previousTransaction != transaction) {
    InterlockedExchange(&s_eiemNativeFlagDrawTraceCount, 0);
    InterlockedExchange(&s_eiemNativeFlagSourceAppendCount, 0);
    InterlockedExchange(&s_eiemNativeFlagSourceFlushCount, 0);
    InterlockedExchange64(&s_eiemNativeFlagTraceWindowStartTick,
                          static_cast<LONG64>(GetTickCount64()));
  }
  const LONG64 start = InterlockedCompareExchange64(
      &s_eiemNativeFlagTraceWindowStartTick, 0, 0);
  if (start <= 0 || GetTickCount64() - static_cast<ULONGLONG>(start) > 3000)
    return false;
  if (transactionOut) *transactionOut = transaction;
  return true;
}

static void *EiemReadNativeFieldPointer(void *self, size_t offset) {
  if (!self) return nullptr;
  __try { return *(void **)((char *)self + offset); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}

static uint32_t EiemReadNativeFieldU32(void *self, size_t offset) {
  if (!self) return 0;
  __try { return *(uint32_t *)((char *)self + offset); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static void EiemFormatNativeStack(char *out, size_t outSize,
                                  DWORD maxFrames = 8) {
  if (!out || outSize == 0) return;
  out[0] = '\0';
  void *frames[8] = {};
  const USHORT frameCount = CaptureStackBackTrace(
      2, maxFrames > _countof(frames) ? static_cast<DWORD>(_countof(frames))
                                      : maxFrames,
      frames, nullptr);
  size_t used = 0;
  for (USHORT index = 0; index < frameCount; ++index) {
    const int written = _snprintf_s(
        out + used, outSize - used, _TRUNCATE, "%s%p",
        index ? "," : "", frames[index]);
    if (written <= 0) break;
    used += static_cast<size_t>(written);
    if (used + 24 >= outSize) break;
  }
}

#if defined(EIEM_NATIVE_MESH_1C8_WRITE_TRACE_BUILD)
// The native +0x1C8 writer was not found by the first static pass.  A guarded
// hardware watch is the definitive next step: unlike PAGE_GUARD it works on
// Vulkan mappings, and unlike a guessed call detour it reports the instruction
// that actually stores into an EIEM replacement Mesh.  The implementation is
// intentionally disposable and bounded; it never changes the watched word.
struct EiemNativeMesh1c8WriteHit {
  volatile LONG ready = 0;
  uint64_t tick = 0;
  uint64_t address = 0;
  uint64_t rip = 0;
  uint64_t thread = 0;
  uint64_t value = 0;
  uint64_t stack[8] = {};
  uint32_t slot = 0;
};

struct EiemNativeMesh1c8SavedThread {
  DWORD id = 0;
  DWORD64 dr[4] = {};
  DWORD64 dr6 = 0;
  DWORD64 dr7 = 0;
};

static constexpr LONG kEiemNativeMesh1c8WriteMaxHits = 64;
static constexpr LONG kEiemNativeMesh1c8WriteMaxTargets = 64;
// The first hardware run armed successfully but the manual F10 can happen
// after the initial renderer commit. Keep the disposable evidence window
// long enough to cover that delayed registration path without making the
// probe permanent.
static constexpr ULONGLONG kEiemNativeMesh1c8WriteWindowMs = 120000;
static EiemNativeMesh1c8WriteHit s_eiemNativeMesh1c8WriteHits[
    kEiemNativeMesh1c8WriteMaxHits] = {};
static volatile LONG s_eiemNativeMesh1c8WriteHitCount = 0;
static volatile LONG s_eiemNativeMesh1c8WriteAccessCount = 0;
static volatile LONG s_eiemNativeMesh1c8WriteTargetCount = 0;
static volatile LONG s_eiemNativeMesh1c8WriteStarted = 0;
static volatile LONG s_eiemNativeMesh1c8WriteInstalled = 0;
static volatile LONG64 s_eiemNativeMesh1c8WriteDeadline = 0;
static volatile LONG64 s_eiemNativeMesh1c8WriteTargets[
    kEiemNativeMesh1c8WriteMaxTargets] = {};
static volatile LONG64 s_eiemNativeMesh1c8WriteSlots[4] = {};
static volatile LONG s_eiemNativeMesh1c8WriteCursor = 0;
static uintptr_t s_eiemNativeMesh1c8WriteUnityBase = 0;
static PVOID s_eiemNativeMesh1c8WriteHandler = nullptr;

static void EiemCaptureNativeMesh1c8WriteStack(uint64_t *out,
                                               CONTEXT context) {
  if (!out) return;
  __try {
    for (size_t i = 0; i < 8 && context.Rip; ++i) {
      out[i] = context.Rip;
      DWORD64 imageBase = 0;
      PRUNTIME_FUNCTION function =
          RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
      if (function) {
        PVOID handlerData = nullptr;
        DWORD64 establisher = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, function,
                         &context, &handlerData, &establisher, nullptr);
      } else {
        context.Rip = *reinterpret_cast<const uint64_t *>(context.Rsp);
        context.Rsp += sizeof(uint64_t);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static LONG CALLBACK EiemNativeMesh1c8WriteException(
    PEXCEPTION_POINTERS info) {
  if (!info || !info->ExceptionRecord || !info->ContextRecord ||
      info->ExceptionRecord->ExceptionCode != STATUS_SINGLE_STEP ||
      !InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteInstalled, 0, 0))
    return EXCEPTION_CONTINUE_SEARCH;
  const uint32_t mask = static_cast<uint32_t>(info->ContextRecord->Dr6) & 15u;
  const LONG targetCount = InterlockedCompareExchange(
      &s_eiemNativeMesh1c8WriteTargetCount, 0, 0);
  if (!mask || targetCount <= 0) return EXCEPTION_CONTINUE_SEARCH;
  InterlockedIncrement(&s_eiemNativeMesh1c8WriteAccessCount);
  unsigned long firstBit = 0;
  _BitScanForward(&firstBit, mask);
  const uint32_t slot = static_cast<uint32_t>(firstBit);
  if (slot >= 4) return EXCEPTION_CONTINUE_SEARCH;
  info->ContextRecord->Dr6 &= ~static_cast<DWORD64>(15);
  const ULONGLONG now = GetTickCount64();
  const ULONGLONG deadline = static_cast<ULONGLONG>(InterlockedCompareExchange64(
      &s_eiemNativeMesh1c8WriteDeadline, 0, 0));
  if (now <= deadline) {
    const uintptr_t address = static_cast<uintptr_t>(InterlockedCompareExchange64(
        &s_eiemNativeMesh1c8WriteSlots[slot], 0, 0));
    LONG index = InterlockedIncrement(&s_eiemNativeMesh1c8WriteHitCount) - 1;
    if (address && index >= 0 && index < kEiemNativeMesh1c8WriteMaxHits) {
      auto &hit = s_eiemNativeMesh1c8WriteHits[index];
      hit.tick = now;
      hit.address = address;
      hit.rip = info->ContextRecord->Rip;
      hit.thread = GetCurrentThreadId();
      hit.slot = slot;
      __try {
        hit.value = *reinterpret_cast<volatile uint32_t *>(address);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        hit.value = UINT64_MAX;
      }
      EiemCaptureNativeMesh1c8WriteStack(hit.stack, *info->ContextRecord);
      InterlockedExchange(&hit.ready, 1);
    }
  }
  return EXCEPTION_CONTINUE_EXECUTION;
}

static void EiemRefreshNativeMesh1c8WriteSlots() {
  const LONG count = InterlockedCompareExchange(
      &s_eiemNativeMesh1c8WriteTargetCount, 0, 0);
  if (count <= 0) return;
  const LONG base = InterlockedIncrement(&s_eiemNativeMesh1c8WriteCursor);
  for (uint32_t slot = 0; slot < 4; ++slot) {
    const LONG index = (base + static_cast<LONG>(slot)) % count;
    const LONG64 address = index >= 0
        ? InterlockedCompareExchange64(
              &s_eiemNativeMesh1c8WriteTargets[index], 0, 0)
        : 0;
    InterlockedExchange64(&s_eiemNativeMesh1c8WriteSlots[slot], address);
  }
}

static bool EiemSetNativeMesh1c8ThreadWatch(
    DWORD threadId, bool install, EiemNativeMesh1c8SavedThread *saved) {
  HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                 THREAD_SET_CONTEXT,
                             FALSE, threadId);
  if (!thread) return false;
  bool success = false;
  if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &context)) {
      if (install && saved && !(context.Dr7 & 0xFFu)) {
        saved->id = threadId;
        saved->dr[0] = context.Dr0;
        saved->dr[1] = context.Dr1;
        saved->dr[2] = context.Dr2;
        saved->dr[3] = context.Dr3;
        saved->dr6 = context.Dr6;
        saved->dr7 = context.Dr7;
        context.Dr0 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemNativeMesh1c8WriteSlots[0], 0, 0));
        context.Dr1 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemNativeMesh1c8WriteSlots[1], 0, 0));
        context.Dr2 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemNativeMesh1c8WriteSlots[2], 0, 0));
        context.Dr3 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemNativeMesh1c8WriteSlots[3], 0, 0));
        context.Dr6 = 0;
        for (uint32_t slot = 0; slot < 4; ++slot) {
          context.Dr7 |= 1ull << (2 * slot);
          context.Dr7 &= ~(15ull << (16 + 4 * slot));
          // RW=01 (data write), LEN=11 (four bytes).
          context.Dr7 |= 0xDull << (16 + 4 * slot);
        }
        success = SetThreadContext(thread, &context) != FALSE;
      } else if (!install && saved) {
        context.Dr0 = saved->dr[0];
        context.Dr1 = saved->dr[1];
        context.Dr2 = saved->dr[2];
        context.Dr3 = saved->dr[3];
        context.Dr6 = saved->dr6;
        context.Dr7 = saved->dr7;
        success = SetThreadContext(thread, &context) != FALSE;
      }
    }
    ResumeThread(thread);
  }
  CloseHandle(thread);
  return success;
}

static bool EiemRefreshNativeMesh1c8ThreadWatch(DWORD threadId) {
  HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                 THREAD_SET_CONTEXT,
                             FALSE, threadId);
  if (!thread) return false;
  bool success = false;
  if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &context) && (context.Dr7 & 0xFFu)) {
      context.Dr0 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemNativeMesh1c8WriteSlots[0], 0, 0));
      context.Dr1 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemNativeMesh1c8WriteSlots[1], 0, 0));
      context.Dr2 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemNativeMesh1c8WriteSlots[2], 0, 0));
      context.Dr3 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemNativeMesh1c8WriteSlots[3], 0, 0));
      context.Dr6 = 0;
      success = SetThreadContext(thread, &context) != FALSE;
    }
    ResumeThread(thread);
  }
  CloseHandle(thread);
  return success;
}

static DWORD WINAPI EiemNativeMesh1c8WriteWorker(void *) {
  s_eiemNativeMesh1c8WriteUnityBase = reinterpret_cast<uintptr_t>(
      GetModuleHandleW(L"UnityPlayer.dll"));
  s_eiemNativeMesh1c8WriteHandler = AddVectoredExceptionHandler(
      1, EiemNativeMesh1c8WriteException);
  if (!s_eiemNativeMesh1c8WriteHandler) {
    Log("[NATIVE-MESH-1C8-WRITE-v1] add-handler failed error=%lu",
        GetLastError());
    InterlockedExchange(&s_eiemNativeMesh1c8WriteStarted, 0);
    return 1;
  }
  InterlockedExchange64(&s_eiemNativeMesh1c8WriteDeadline,
                        static_cast<LONG64>(GetTickCount64() +
                                            kEiemNativeMesh1c8WriteWindowMs));
  EiemRefreshNativeMesh1c8WriteSlots();
  std::vector<EiemNativeMesh1c8SavedThread> watched;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) do {
      if (entry.th32OwnerProcessID != GetCurrentProcessId() ||
          entry.th32ThreadID == GetCurrentThreadId()) continue;
      EiemNativeMesh1c8SavedThread saved;
      if (EiemSetNativeMesh1c8ThreadWatch(entry.th32ThreadID, true, &saved))
        watched.push_back(saved);
    } while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
  }
  InterlockedExchange(&s_eiemNativeMesh1c8WriteInstalled, 1);
  Log("[NATIVE-MESH-1C8-WRITE-v1] armed threads=%zu targets=%ld "
      "deadlineMs=%llu unity=%p",
      watched.size(),
      InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteTargetCount, 0, 0),
      static_cast<unsigned long long>(kEiemNativeMesh1c8WriteWindowMs),
      reinterpret_cast<void *>(s_eiemNativeMesh1c8WriteUnityBase));
  while (GetTickCount64() <= static_cast<ULONGLONG>(InterlockedCompareExchange64(
             &s_eiemNativeMesh1c8WriteDeadline, 0, 0)) &&
         InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteHitCount, 0, 0) <
             kEiemNativeMesh1c8WriteMaxHits) {
    Sleep(10);
    EiemRefreshNativeMesh1c8WriteSlots();
    for (const auto &saved : watched)
      EiemRefreshNativeMesh1c8ThreadWatch(saved.id);
  }
  InterlockedExchange(&s_eiemNativeMesh1c8WriteInstalled, 0);
  for (auto &saved : watched)
    EiemSetNativeMesh1c8ThreadWatch(saved.id, false, &saved);
  const LONG hitCount = (std::min)(
      InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteHitCount, 0, 0),
      kEiemNativeMesh1c8WriteMaxHits);
  for (LONG index = 0; index < hitCount; ++index) {
    const auto &hit = s_eiemNativeMesh1c8WriteHits[index];
    if (!InterlockedCompareExchange(&const_cast<volatile LONG &>(hit.ready),
                                    0, 0))
      continue;
    Log("[NATIVE-MESH-1C8-WRITE-v1] ordinal=%ld tick=%llu tid=%llu "
        "slot=%u address=%llx rip=%llx value=%llu "
        "stack=%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx",
        index + 1, static_cast<unsigned long long>(hit.tick),
        static_cast<unsigned long long>(hit.thread), hit.slot,
        static_cast<unsigned long long>(hit.address),
        static_cast<unsigned long long>(hit.rip),
        static_cast<unsigned long long>(hit.value),
        static_cast<unsigned long long>(hit.stack[0]),
        static_cast<unsigned long long>(hit.stack[1]),
        static_cast<unsigned long long>(hit.stack[2]),
        static_cast<unsigned long long>(hit.stack[3]),
        static_cast<unsigned long long>(hit.stack[4]),
        static_cast<unsigned long long>(hit.stack[5]),
        static_cast<unsigned long long>(hit.stack[6]),
        static_cast<unsigned long long>(hit.stack[7]));
  }
  Log("[NATIVE-MESH-1C8-WRITE-v1] stopped hits=%ld accesses=%ld "
      "targets=%ld GameAssembly=%p UnityPlayer=%p",
      hitCount,
      InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteAccessCount, 0, 0),
      InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteTargetCount, 0, 0),
      GetModuleHandleW(L"GameAssembly.dll"),
      GetModuleHandleW(L"UnityPlayer.dll"));
  if (s_eiemNativeMesh1c8WriteHandler) {
    RemoveVectoredExceptionHandler(s_eiemNativeMesh1c8WriteHandler);
    s_eiemNativeMesh1c8WriteHandler = nullptr;
  }
  InterlockedExchange(&s_eiemNativeMesh1c8WriteStarted, 0);
  return 0;
}

static void EiemArmNativeMesh1c8WriteWatch(uintptr_t native) {
  if (!kEiemEnableNativeMesh1c8WriteTrace || !native) return;
  const uintptr_t address = native + 0x1C8;
  LONG count = InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteTargetCount,
                                          0, 0);
  for (LONG index = 0; index < count; ++index) {
    if (static_cast<uintptr_t>(InterlockedCompareExchange64(
            &s_eiemNativeMesh1c8WriteTargets[index], 0, 0)) == address) {
      InterlockedExchange64(
          &s_eiemNativeMesh1c8WriteDeadline,
          static_cast<LONG64>(GetTickCount64() +
                              kEiemNativeMesh1c8WriteWindowMs));
      return;
    }
  }
  if (count < kEiemNativeMesh1c8WriteMaxTargets) {
    if (InterlockedCompareExchange64(&s_eiemNativeMesh1c8WriteTargets[count],
                                     static_cast<LONG64>(address), 0) == 0)
      InterlockedIncrement(&s_eiemNativeMesh1c8WriteTargetCount);
  }
  if (!InterlockedCompareExchange(&s_eiemNativeMesh1c8WriteStarted, 1, 0)) {
    HANDLE worker = CreateThread(nullptr, 0, EiemNativeMesh1c8WriteWorker,
                                 nullptr, 0, nullptr);
    if (worker) CloseHandle(worker);
    else {
      InterlockedExchange(&s_eiemNativeMesh1c8WriteStarted, 0);
      Log("[NATIVE-MESH-1C8-WRITE-v1] create-worker failed error=%lu",
          GetLastError());
    }
  }
}
#else
static void EiemArmNativeMesh1c8WriteWatch(uintptr_t) {}
#endif

#if defined(EIEM_NATIVE_SKIN_EF230_WRITE_TRACE_BUILD)
// EF230's acceptance result depends on a short native pointer chain. A later
// owner can invalidate that chain without touching Mesh+0x1C8, so watching the
// terminal metadata alone is insufficient: we watch all three pointer fields
// and rotate four hardware slots over the bounded target list.
struct EiemEf230WriteHit {
  volatile LONG ready = 0;
  uint64_t tick = 0;
  uint64_t address = 0;
  uint64_t rip = 0;
  uint64_t thread = 0;
  uint64_t value = 0;
  uint64_t native = 0;
  uint32_t kind = 0;
  uint32_t slot = 0;
  uint64_t stack[8] = {};
};

struct EiemEf230SavedThread {
  DWORD id = 0;
  DWORD64 dr[4] = {};
  DWORD64 dr6 = 0;
  DWORD64 dr7 = 0;
};

static constexpr LONG kEiemEf230WriteMaxHits = 128;
static constexpr LONG kEiemEf230WriteMaxTargets = 128;
static constexpr ULONGLONG kEiemEf230WriteWindowMs = 20000;
static EiemEf230WriteHit s_eiemEf230WriteHits[kEiemEf230WriteMaxHits] = {};
static volatile LONG s_eiemEf230WriteHitCount = 0;
static volatile LONG s_eiemEf230WriteAccessCount = 0;
static volatile LONG s_eiemEf230WriteTargetCount = 0;
static volatile LONG s_eiemEf230WriteStarted = 0;
static volatile LONG s_eiemEf230WriteInstalled = 0;
static volatile LONG64 s_eiemEf230WriteDeadline = 0;
static volatile LONG64 s_eiemEf230WriteTargets[kEiemEf230WriteMaxTargets] = {};
static volatile LONG s_eiemEf230WriteKinds[kEiemEf230WriteMaxTargets] = {};
static volatile LONG64 s_eiemEf230WriteNatives[kEiemEf230WriteMaxTargets] = {};
static volatile LONG64 s_eiemEf230WriteSlots[4] = {};
static volatile LONG s_eiemEf230WriteSlotKinds[4] = {};
static volatile LONG64 s_eiemEf230WriteSlotNatives[4] = {};
static volatile LONG s_eiemEf230WriteCursor = 0;
static PVOID s_eiemEf230WriteHandler = nullptr;

static void EiemCaptureEf230WriteStack(uint64_t *out, CONTEXT context) {
  if (!out) return;
  __try {
    for (size_t i = 0; i < 8 && context.Rip; ++i) {
      out[i] = context.Rip;
      DWORD64 imageBase = 0;
      PRUNTIME_FUNCTION function =
          RtlLookupFunctionEntry(context.Rip, &imageBase, nullptr);
      if (function) {
        PVOID handlerData = nullptr;
        DWORD64 establisher = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, function,
                         &context, &handlerData, &establisher, nullptr);
      } else {
        context.Rip = *reinterpret_cast<const uint64_t *>(context.Rsp);
        context.Rsp += sizeof(uint64_t);
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static LONG CALLBACK EiemEf230WriteException(PEXCEPTION_POINTERS info) {
  if (!info || !info->ExceptionRecord || !info->ContextRecord ||
      info->ExceptionRecord->ExceptionCode != STATUS_SINGLE_STEP ||
      !InterlockedCompareExchange(&s_eiemEf230WriteInstalled, 0, 0))
    return EXCEPTION_CONTINUE_SEARCH;
  const uint32_t mask = static_cast<uint32_t>(info->ContextRecord->Dr6) & 15u;
  if (!mask) return EXCEPTION_CONTINUE_SEARCH;
  InterlockedIncrement(&s_eiemEf230WriteAccessCount);
  unsigned long firstBit = 0;
  _BitScanForward(&firstBit, mask);
  const uint32_t slot = static_cast<uint32_t>(firstBit);
  if (slot >= 4) return EXCEPTION_CONTINUE_SEARCH;
  info->ContextRecord->Dr6 &= ~static_cast<DWORD64>(15);
  const ULONGLONG now = GetTickCount64();
  const ULONGLONG deadline = static_cast<ULONGLONG>(
      InterlockedCompareExchange64(&s_eiemEf230WriteDeadline, 0, 0));
  if (now <= deadline) {
    const uintptr_t address = static_cast<uintptr_t>(
        InterlockedCompareExchange64(&s_eiemEf230WriteSlots[slot], 0, 0));
    LONG index = InterlockedIncrement(&s_eiemEf230WriteHitCount) - 1;
    if (address && index >= 0 && index < kEiemEf230WriteMaxHits) {
      auto &hit = s_eiemEf230WriteHits[index];
      hit.tick = now;
      hit.address = address;
      hit.rip = info->ContextRecord->Rip;
      hit.thread = GetCurrentThreadId();
      hit.slot = slot;
      hit.kind = static_cast<uint32_t>(InterlockedCompareExchange(
          &s_eiemEf230WriteSlotKinds[slot], 0, 0));
      hit.native = static_cast<uint64_t>(InterlockedCompareExchange64(
          &s_eiemEf230WriteSlotNatives[slot], 0, 0));
      __try {
        hit.value = *reinterpret_cast<volatile uint64_t *>(address);
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        hit.value = UINT64_MAX;
      }
      EiemCaptureEf230WriteStack(hit.stack, *info->ContextRecord);
      InterlockedExchange(&hit.ready, 1);
    }
  }
  return EXCEPTION_CONTINUE_EXECUTION;
}

static bool EiemSetEf230ThreadWatch(DWORD threadId, bool install,
                                     EiemEf230SavedThread *saved) {
  HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                 THREAD_SET_CONTEXT, FALSE, threadId);
  if (!thread) return false;
  bool success = false;
  if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &context)) {
      if (install && saved && !(context.Dr7 & 0xFFu)) {
        saved->id = threadId;
        saved->dr[0] = context.Dr0;
        saved->dr[1] = context.Dr1;
        saved->dr[2] = context.Dr2;
        saved->dr[3] = context.Dr3;
        saved->dr6 = context.Dr6;
        saved->dr7 = context.Dr7;
        context.Dr0 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemEf230WriteSlots[0], 0, 0));
        context.Dr1 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemEf230WriteSlots[1], 0, 0));
        context.Dr2 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemEf230WriteSlots[2], 0, 0));
        context.Dr3 = static_cast<DWORD64>(InterlockedCompareExchange64(
            &s_eiemEf230WriteSlots[3], 0, 0));
        context.Dr6 = 0;
        for (uint32_t slot = 0; slot < 4; ++slot) {
          context.Dr7 |= 1ull << (2 * slot);
          context.Dr7 &= ~(15ull << (16 + 4 * slot));
          // RW=01 (data write), LEN=11 (eight-byte pointer field).
          context.Dr7 |= 0xDull << (16 + 4 * slot);
        }
        success = SetThreadContext(thread, &context) != FALSE;
      } else if (!install && saved) {
        context.Dr0 = saved->dr[0];
        context.Dr1 = saved->dr[1];
        context.Dr2 = saved->dr[2];
        context.Dr3 = saved->dr[3];
        context.Dr6 = saved->dr6;
        context.Dr7 = saved->dr7;
        success = SetThreadContext(thread, &context) != FALSE;
      }
    }
    ResumeThread(thread);
  }
  CloseHandle(thread);
  return success;
}

static bool EiemRefreshEf230ThreadWatch(DWORD threadId) {
  HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                                 THREAD_SET_CONTEXT, FALSE, threadId);
  if (!thread) return false;
  bool success = false;
  if (SuspendThread(thread) != static_cast<DWORD>(-1)) {
    CONTEXT context = {};
    context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(thread, &context) && (context.Dr7 & 0xFFu)) {
      context.Dr0 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemEf230WriteSlots[0], 0, 0));
      context.Dr1 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemEf230WriteSlots[1], 0, 0));
      context.Dr2 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemEf230WriteSlots[2], 0, 0));
      context.Dr3 = static_cast<DWORD64>(InterlockedCompareExchange64(
          &s_eiemEf230WriteSlots[3], 0, 0));
      context.Dr6 = 0;
      success = SetThreadContext(thread, &context) != FALSE;
    }
    ResumeThread(thread);
  }
  CloseHandle(thread);
  return success;
}

static void EiemRefreshEf230WriteSlots() {
  const LONG count = InterlockedCompareExchange(
      &s_eiemEf230WriteTargetCount, 0, 0);
  if (count <= 0) return;
  const LONG base = InterlockedIncrement(&s_eiemEf230WriteCursor);
  for (uint32_t slot = 0; slot < 4; ++slot) {
    const LONG index = (base + static_cast<LONG>(slot)) % count;
    InterlockedExchange64(&s_eiemEf230WriteSlots[slot],
                          InterlockedCompareExchange64(
                              &s_eiemEf230WriteTargets[index], 0, 0));
    InterlockedExchange(&s_eiemEf230WriteSlotKinds[slot],
                        InterlockedCompareExchange(
                            &s_eiemEf230WriteKinds[index], 0, 0));
    InterlockedExchange64(&s_eiemEf230WriteSlotNatives[slot],
                          InterlockedCompareExchange64(
                              &s_eiemEf230WriteNatives[index], 0, 0));
  }
}

static void EiemLogEf230Chain(uintptr_t native, const char *stage) {
  if (!native) return;
  void *data = EiemReadNativeFieldPointer(reinterpret_cast<void *>(native),
                                          0x38);
  void *aux = EiemReadNativeFieldPointer(data, 0x190);
  void *terminal = EiemReadNativeFieldPointer(aux, 0x50);
  Log("[EF230-CHAIN-WATCH-v1] stage=%s native=%p data=%p aux=%p "
      "terminal=%p field1c8=%u target38=%p target190=%p target50=%p",
      stage ? stage : "arm", reinterpret_cast<void *>(native), data, aux,
      terminal, EiemReadNativeFieldU32(reinterpret_cast<void *>(native), 0x1C8),
      reinterpret_cast<void *>(native + 0x38),
      data ? reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(data) + 0x190)
           : nullptr,
      aux ? reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(aux) + 0x50)
          : nullptr);
}

static void EiemAddEf230Target(uintptr_t address, uintptr_t native,
                               LONG kind) {
  if (!address) return;
  LONG count = InterlockedCompareExchange(&s_eiemEf230WriteTargetCount, 0, 0);
  for (LONG i = 0; i < count; ++i) {
    if (static_cast<uintptr_t>(InterlockedCompareExchange64(
            &s_eiemEf230WriteTargets[i], 0, 0)) == address)
      return;
  }
  if (count >= kEiemEf230WriteMaxTargets) return;
  if (InterlockedCompareExchange64(&s_eiemEf230WriteTargets[count],
                                   static_cast<LONG64>(address), 0) == 0) {
    InterlockedExchange(&s_eiemEf230WriteKinds[count], kind);
    InterlockedExchange64(&s_eiemEf230WriteNatives[count],
                          static_cast<LONG64>(native));
    InterlockedIncrement(&s_eiemEf230WriteTargetCount);
  }
}

static DWORD WINAPI EiemEf230WriteWorker(void *) {
  s_eiemEf230WriteHandler = AddVectoredExceptionHandler(
      1, EiemEf230WriteException);
  if (!s_eiemEf230WriteHandler) {
    Log("[EF230-CHAIN-WATCH-v1] add-handler failed error=%lu", GetLastError());
    InterlockedExchange(&s_eiemEf230WriteStarted, 0);
    return 1;
  }
  InterlockedExchange64(&s_eiemEf230WriteDeadline,
                        static_cast<LONG64>(GetTickCount64() +
                                            kEiemEf230WriteWindowMs));
  EiemRefreshEf230WriteSlots();
  std::vector<EiemEf230SavedThread> watched;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    if (Thread32First(snapshot, &entry)) do {
      if (entry.th32OwnerProcessID != GetCurrentProcessId() ||
          entry.th32ThreadID == GetCurrentThreadId()) continue;
      EiemEf230SavedThread saved;
      if (EiemSetEf230ThreadWatch(entry.th32ThreadID, true, &saved))
        watched.push_back(saved);
    } while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
  }
  InterlockedExchange(&s_eiemEf230WriteInstalled, 1);
  Log("[EF230-CHAIN-WATCH-v1] armed threads=%zu targets=%ld windowMs=%llu",
      watched.size(),
      InterlockedCompareExchange(&s_eiemEf230WriteTargetCount, 0, 0),
      static_cast<unsigned long long>(kEiemEf230WriteWindowMs));
  while (GetTickCount64() <= static_cast<ULONGLONG>(
             InterlockedCompareExchange64(&s_eiemEf230WriteDeadline, 0, 0)) &&
         InterlockedCompareExchange(&s_eiemEf230WriteHitCount, 0, 0) <
             kEiemEf230WriteMaxHits) {
    Sleep(10);
    EiemRefreshEf230WriteSlots();
    for (const auto &saved : watched) EiemRefreshEf230ThreadWatch(saved.id);
  }
  InterlockedExchange(&s_eiemEf230WriteInstalled, 0);
  for (auto &saved : watched) EiemSetEf230ThreadWatch(saved.id, false, &saved);
  const LONG hitCount = (std::min)(
      InterlockedCompareExchange(&s_eiemEf230WriteHitCount, 0, 0),
      kEiemEf230WriteMaxHits);
  for (LONG index = 0; index < hitCount; ++index) {
    const auto &hit = s_eiemEf230WriteHits[index];
    if (!InterlockedCompareExchange(&const_cast<volatile LONG &>(hit.ready),
                                    0, 0)) continue;
    Log("[EF230-CHAIN-WRITE-v1] ordinal=%ld tick=%llu tid=%llu kind=%u "
        "slot=%u address=%llx native=%llx rip=%llx value=%llx "
        "stack=%llx,%llx,%llx,%llx,%llx,%llx,%llx,%llx",
        index + 1, static_cast<unsigned long long>(hit.tick),
        static_cast<unsigned long long>(hit.thread), hit.kind, hit.slot,
        static_cast<unsigned long long>(hit.address),
        static_cast<unsigned long long>(hit.native),
        static_cast<unsigned long long>(hit.rip),
        static_cast<unsigned long long>(hit.value),
        static_cast<unsigned long long>(hit.stack[0]),
        static_cast<unsigned long long>(hit.stack[1]),
        static_cast<unsigned long long>(hit.stack[2]),
        static_cast<unsigned long long>(hit.stack[3]),
        static_cast<unsigned long long>(hit.stack[4]),
        static_cast<unsigned long long>(hit.stack[5]),
        static_cast<unsigned long long>(hit.stack[6]),
        static_cast<unsigned long long>(hit.stack[7]));
  }
  Log("[EF230-CHAIN-WATCH-v1] stopped hits=%ld accesses=%ld targets=%ld "
      "GameAssembly=%p UnityPlayer=%p",
      hitCount,
      InterlockedCompareExchange(&s_eiemEf230WriteAccessCount, 0, 0),
      InterlockedCompareExchange(&s_eiemEf230WriteTargetCount, 0, 0),
      GetModuleHandleW(L"GameAssembly.dll"), GetModuleHandleW(L"UnityPlayer.dll"));
  if (s_eiemEf230WriteHandler) {
    RemoveVectoredExceptionHandler(s_eiemEf230WriteHandler);
    s_eiemEf230WriteHandler = nullptr;
  }
  InterlockedExchange(&s_eiemEf230WriteStarted, 0);
  return 0;
}

static void EiemArmNativeSkinAcceptanceWriteWatch(uintptr_t native) {
  if (!kEiemEnableNativeSkinEf230WriteTrace || !native) return;
  void *data = EiemReadNativeFieldPointer(reinterpret_cast<void *>(native), 0x38);
  void *aux = EiemReadNativeFieldPointer(data, 0x190);
  EiemLogEf230Chain(native, "arm");
  EiemAddEf230Target(native + 0x38, native, 1);
  if (data) EiemAddEf230Target(reinterpret_cast<uintptr_t>(data) + 0x190,
                               native, 2);
  if (aux) EiemAddEf230Target(reinterpret_cast<uintptr_t>(aux) + 0x50,
                              native, 3);
  if (!InterlockedCompareExchange(&s_eiemEf230WriteStarted, 1, 0)) {
    HANDLE worker = CreateThread(nullptr, 0, EiemEf230WriteWorker,
                                 nullptr, 0, nullptr);
    if (worker) CloseHandle(worker);
    else {
      InterlockedExchange(&s_eiemEf230WriteStarted, 0);
      Log("[EF230-CHAIN-WATCH-v1] create-worker failed error=%lu",
          GetLastError());
    }
  }
}
#else
static void EiemArmNativeSkinAcceptanceWriteWatch(uintptr_t) {}
#endif

// UnityPlayer!0xEF200 is the mode query immediately before C7B750 chooses
// the metadata-derived bone count. Observe its unchanged return value and
// the global mode word that gates the query; this is bounded per F10
// transaction and never mutates the native path.
static uint32_t __fastcall TraceNativeSkinMode(void *self) {
  auto original = s_origNativeSkinMode;
  const uint32_t result = original ? original(self) : 0;
  if (!kEiemEnableNativeSkinModeProbe) return result;
  const LONG transaction = InterlockedCompareExchange(
      &s_eiemSkinTargetTransaction, 0, 0);
  if (transaction <= 0) return result;
  const LONG previousTransaction = InterlockedExchange(
      &s_eiemNativeSkinModeProbeLastTransaction, transaction);
  if (transaction != previousTransaction)
    InterlockedExchange(&s_eiemNativeSkinModeProbeTransactionMatches, 0);
  void *meshOrBindpose = EiemReadNativeFieldPointer(self, 0x298);
  const uint32_t field1c8 = EiemReadNativeFieldU32(meshOrBindpose, 0x1C8);
  const LONG ordinal = field1c8 == 4
      ? InterlockedIncrement(&s_eiemNativeSkinModeProbeTransactionMatches)
      : 0;
  const bool logInitial = ordinal > 0 && ordinal <= 8;
  LONG anomalyOrdinal = 0;
  if (result != 0x200000) {
    const LONG previousAnomalyTransaction = InterlockedExchange(
        &s_eiemNativeSkinModeAnomalyLastTransaction, transaction);
    if (transaction != previousAnomalyTransaction)
      InterlockedExchange(&s_eiemNativeSkinModeAnomalyMatches, 0);
    anomalyOrdinal = InterlockedIncrement(&s_eiemNativeSkinModeAnomalyMatches);
  }
  // Keep the normal sample bounded, but never discard the first 32 calls that
  // take the other EF200 branch. Those are the only calls that can explain a
  // later 0x30 record after an otherwise healthy 0x34 stream.
  const bool logAnomaly = anomalyOrdinal > 0 && anomalyOrdinal <= 32;
  if (!logInitial && !logAnomaly) return result;
  void *globalState = EiemReadUnityPointerRva(0x1CF4458);
  const uint32_t globalMode = EiemReadNativeFieldU32(globalState, 0x158);
  const uint8_t gateA = EiemReadUnityByteRva(0x1CF5CC4);
  const uint8_t gateB = EiemReadUnityByteRva(0x1CF5CC5);
  const uint8_t gateC = EiemReadUnityByteRva(0x1C59C90);
  // EF230 is the native acceptance predicate immediately upstream of this
  // query.  Read the exact chain it tests so a later bad sample can be
  // classified as missing/rebuilt metadata without guessing from the flag.
  void *metadata38 = EiemReadNativeFieldPointer(meshOrBindpose, 0x38);
  void *aux190 = EiemReadNativeFieldPointer(metadata38, 0x190);
  void *aux50 = EiemReadNativeFieldPointer(aux190, 0x50);
  const uint32_t ef230Accept =
      (!meshOrBindpose || !metadata38 || !aux190 || !aux50) ? 1u : 0u;
  Log("[NATIVE-SKIN-MODE-v3] ordinal=%ld anomalyOrdinal=%ld "
      "transaction=%ld self=%p "
      "meshOrBindpose=%p field1c8=%u result=0x%X globalMode=0x%X "
      "gateA=%u gateB=%u gateC=%u meta38=%p aux190=%p aux50=%p "
      "ef230Accept=%u",
      ordinal, anomalyOrdinal, transaction, self, meshOrBindpose, field1c8,
      result,
      globalMode, gateA, gateB, gateC, metadata38, aux190, aux50,
      ef230Accept);
  return result;
}

// UnityPlayer!0x19CE90 is the low-frequency native skin validation/record
// construction boundary.  It is called with the renderer as RCX, the render
// flags in EDX and a scratch/context pointer in R8.  Observe the exact Mesh
// pointer captured from EIEM's replacement commit, call the original first,
// and never change the return value or any native state.
static int64_t __fastcall TraceNativeSkinValidation(void *self,
                                                     int32_t flags,
                                                     void *context) {
  void *meshBefore = self
      ? EiemReadNativeFieldPointer(self, 0x298)
      : nullptr;
  const uintptr_t nativeBefore = reinterpret_cast<uintptr_t>(meshBefore);
  const bool trackedBefore = EiemIsReplacementNativeMesh(nativeBefore);
  auto original = s_origNativeSkinValidation;
  const int64_t result = original ? original(self, flags, context) : 0;
  if (!kEiemEnableNativeSkinValidationProbe || !self) return result;
  void *meshAfter = EiemReadNativeFieldPointer(self, 0x298);
  const uintptr_t nativeAfter = reinterpret_cast<uintptr_t>(meshAfter);
  if (!trackedBefore && !EiemIsReplacementNativeMesh(nativeAfter))
    return result;
  const uintptr_t mesh = trackedBefore ? nativeBefore : nativeAfter;
  const uint32_t field1c8Before = EiemReadNativeFieldU32(meshBefore, 0x1C8);
  const uint32_t field1c8After = EiemReadNativeFieldU32(meshAfter, 0x1C8);
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeSkinValidationLogs);
  const LONG previous = InterlockedExchange(
      &s_eiemNativeSkinValidationLastField,
      static_cast<LONG>(field1c8After));
  if (ordinal > 32 && previous == static_cast<LONG>(field1c8After) &&
      nativeBefore == nativeAfter) return result;
  const LONG transaction = InterlockedCompareExchange(
      &s_eiemSkinTargetTransaction, 0, 0);
  void *nativeDataBefore = EiemReadNativeFieldPointer(meshBefore, 0x38);
  void *nativeDataAfter = EiemReadNativeFieldPointer(meshAfter, 0x38);
  const int64_t nativeBoneCountBefore =
      EiemReadNativeFieldU32(nativeDataBefore, 0x100);
  const int64_t nativeBoneCountAfter =
      EiemReadNativeFieldU32(nativeDataAfter, 0x100);
  const int64_t nativeBindposeBefore =
      EiemReadNativeInt32Field(reinterpret_cast<uintptr_t>(meshBefore), 0x110);
  const int64_t nativeBindposeAfter =
      EiemReadNativeInt32Field(reinterpret_cast<uintptr_t>(meshAfter), 0x110);
  const int64_t nativeMaxIndexBefore =
      EiemReadNativeInt32Field(reinterpret_cast<uintptr_t>(meshBefore), 0x124);
  const int64_t nativeMaxIndexAfter =
      EiemReadNativeInt32Field(reinterpret_cast<uintptr_t>(meshAfter), 0x124);
  const uint32_t rendererField2f8 = EiemReadNativeFieldU32(self, 0x2F8);
  void *rendererField2f0 = EiemReadNativeFieldPointer(self, 0x2F0);
  void *rendererField330 = EiemReadNativeFieldPointer(self, 0x330);
  void *rendererField340 = EiemReadNativeFieldPointer(self, 0x340);
  Log("[NATIVE-SKIN-VALIDATION-v1] ordinal=%ld transaction=%ld self=%p "
      "meshBefore=%p meshAfter=%p field1c8Before=%u field1c8After=%u "
      "flags=0x%X result=%lld nativeDataBefore=%p nativeDataAfter=%p "
      "nativeBoneCountBefore=%lld nativeBoneCountAfter=%lld "
      "nativeBindposeBefore=%lld nativeBindposeAfter=%lld "
      "nativeMaxIndexBefore=%lld nativeMaxIndexAfter=%lld "
      "rendererField2f8=%u rendererField2f0=%p rendererField330=%p "
      "rendererField340=%p context=%p",
      ordinal, transaction, self, meshBefore, meshAfter, field1c8Before,
      field1c8After, static_cast<unsigned int>(flags),
      static_cast<long long>(result), nativeDataBefore, nativeDataAfter,
      static_cast<long long>(nativeBoneCountBefore),
      static_cast<long long>(nativeBoneCountAfter),
      static_cast<long long>(nativeBindposeBefore),
      static_cast<long long>(nativeBindposeAfter),
      static_cast<long long>(nativeMaxIndexBefore),
      static_cast<long long>(nativeMaxIndexAfter), rendererField2f8,
      rendererField2f0, rendererField330, rendererField340, context);
  return result;
}

// Observe real calls to UnityPlayer!0x4A90D0. Static analysis showed a
// +0x1C8 store here, but a direct call with an EIEM native Mesh raised an
// access violation. This hook records the actual caller/object shape and then
// delegates unchanged; it never calls the function itself and never mutates
// the object.
static uint8_t __fastcall TraceNativeMesh1c8Normalizer(void *self) {
  auto original = s_origNativeMesh1c8Normalizer;
  const uint8_t result = original ? original(self) : 0;
  if (!kEiemEnableNativeMesh1c8CallTrace || !self) return result;
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeMesh1c8CallLogs);
  if (ordinal > 128) return result;
  const uintptr_t native = reinterpret_cast<uintptr_t>(self);
  const uintptr_t vtable = EiemReadNativePointerField(native, 0x0);
  const void *field38 = EiemReadNativeFieldPointer(self, 0x38);
  const uintptr_t innerVtable = reinterpret_cast<uintptr_t>(
      EiemReadNativeFieldPointer(const_cast<void *>(field38), 0x0));
  const uint32_t beforeAfter = EiemReadNativeFieldU32(self, 0x1C8);
  char stack[384] = {};
  EiemFormatNativeStack(stack, sizeof(stack));
  Log("[NATIVE-MESH-1C8-CALL-v1] ordinal=%ld self=%p vtable=%p "
      "field38=%p innerVtable=%p field1c8=%u field1d0=%lld "
      "result=%u return=%p stack=%s",
      ordinal, self, reinterpret_cast<void *>(vtable), field38,
      reinterpret_cast<void *>(innerVtable), beforeAfter,
      (long long)EiemReadNativeInt32Field(native, 0x1D0),
      (unsigned)result, _ReturnAddress(), stack[0] ? stack : "<none>");
  return result;
}

// The post-constructor hardware watch cannot see a store that happens inside
// Mesh's native constructor. Observe the constructor itself instead. This is
// diagnostic-only: it never changes the object or the constructor arguments.
static int64_t __fastcall TraceNativeMeshConstructor(void *self,
                                                      uint32_t arg1,
                                                      uint32_t arg2) {
  const uintptr_t native = reinterpret_cast<uintptr_t>(self);
  const int64_t before = self
      ? EiemReadNativeInt32Field(native, 0x1C8)
      : 0;
  auto original = s_origNativeMeshCtor;
  const int64_t result = original ? original(self, arg1, arg2) : 0;
  if (kEiemEnableNativeMeshDeserializeTrace && self) {
    const LONG recordOrdinal = InterlockedIncrement(
        &s_eiemNativeMeshCtorTraceCalls);
    if (recordOrdinal > 0 &&
        recordOrdinal <= kEiemNativeMeshDeserializeCapacity) {
      auto &record = s_eiemNativeMeshCtorRecords[recordOrdinal - 1];
      record.native = native;
      record.vtable = EiemReadNativePointerField(native, 0);
      record.returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
      record.threadId = GetCurrentThreadId();
      record.stackSize = RtlCaptureStackBackTrace(
          1, 6, record.stack, nullptr);
      InterlockedExchange(&record.ready, 1);
    }
  }
  if (!kEiemEnableNativeMeshCtorTrace || !self) return result;
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeMeshCtorLogs);
  if (ordinal > 256) return result;
  const int64_t after = EiemReadNativeInt32Field(native, 0x1C8);
  const uintptr_t vtable = EiemReadNativePointerField(native, 0x0);
  char stack[384] = {};
  EiemFormatNativeStack(stack, sizeof(stack));
  Log("[NATIVE-MESH-CTOR-v1] ordinal=%ld self=%p arg1=%u arg2=%u "
      "before1c8=%lld after1c8=%lld vtable=%p result=%lld "
      "return=%p stack=%s",
      ordinal, self, arg1, arg2, (long long)before, (long long)after,
      reinterpret_cast<void *>(vtable), (long long)result, _ReturnAddress(),
      stack[0] ? stack : "<none>");
  return result;
}

static int64_t __fastcall TraceNativeSkinRecordBuild(void *self) {
  if (kEiemEnableNativeSkinC7TargetProbe) {
    const LONG transaction = InterlockedCompareExchange(
        &s_eiemSkinTargetTransaction, 0, 0);
    const LONG previousTransaction = InterlockedExchange(
        &s_eiemNativeSkinC7LastTransaction, transaction);
    if (transaction > 0 && transaction != previousTransaction) {
      InterlockedExchange(&s_eiemNativeSkinC7Logs, 0);
      InterlockedExchange(&s_eiemNativeSkinC7ZeroLogs, 0);
      InterlockedExchange64(&s_eiemNativeSkinC7WindowStartTick,
                            static_cast<LONG64>(GetTickCount64()));
    }
    const LONG64 windowStart = InterlockedCompareExchange64(
        &s_eiemNativeSkinC7WindowStartTick, -1, -1);
    const bool windowActive = transaction > 0 && windowStart >= 0 &&
        GetTickCount64() - static_cast<ULONGLONG>(windowStart) <= 2000;
    void *metadata = EiemReadNativeFieldPointer(self, 0x298);
    const uint32_t field1c8 = EiemReadNativeFieldU32(metadata, 0x1C8);
    auto original = s_origNativeSkinRecordBuild;
    const int64_t result = original ? original(self) : 0;
    // The previous broad census included unrelated renderers. Restrict this
    // pass to native Mesh objects remembered at EIEM replacement commit; if
    // no target reaches C7B750, that is evidence that the clothing path is
    // submitted through the game's custom pipeline instead.
    if (!windowActive || !EiemIsReplacementNativeMesh(
                            reinterpret_cast<uintptr_t>(metadata)))
      return result;
    const LONG candidateOrdinal = InterlockedIncrement(&s_eiemNativeSkinC7Logs);
    const LONG zeroOrdinal = field1c8 == 0
        ? InterlockedIncrement(&s_eiemNativeSkinC7ZeroLogs) : 0;
    if ((candidateOrdinal > 0 && candidateOrdinal <= 64) ||
        (zeroOrdinal > 0 && zeroOrdinal <= 16)) {
      // The count-0 rows are the only rows that correlate with the failed
      // cloth transaction so far. Capture a very small caller sample for the
      // first four such rows (and one healthy count-4 row) so we can identify
      // the producer of the native record without tracing the hot path.
      char stack[384] = {};
      if ((field1c8 == 0 && zeroOrdinal <= 4) ||
          (field1c8 == 4 && candidateOrdinal == 1))
        EiemFormatNativeStack(stack, sizeof(stack));
      Log("[NATIVE-SKIN-C7-TARGET-v2] ordinal=%ld zeroOrdinal=%ld "
          "transaction=%ld "
          "self=%p metadata=%p field1c8=%u field36e=%u field370=%u "
          "field374=%u metadata38=%p metadataAux190=%p metadataAux50=%p "
          "result=%lld stack=%s",
          candidateOrdinal, zeroOrdinal, transaction, self, metadata, field1c8,
          EiemReadNativeFieldU32(self, 0x36E),
          EiemReadNativeFieldU32(self, 0x370),
          EiemReadNativeFieldU32(self, 0x374),
          EiemReadNativeFieldPointer(metadata, 0x38),
          EiemReadNativeFieldPointer(
              EiemReadNativeFieldPointer(metadata, 0x38), 0x190),
          EiemReadNativeFieldPointer(
              EiemReadNativeFieldPointer(
                  EiemReadNativeFieldPointer(metadata, 0x38), 0x190), 0x50),
          static_cast<long long>(result), stack[0] ? stack : "<none>");
    }
    return result;
  }
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  const LONG transaction = InterlockedCompareExchange(
      &s_eiemSkinTargetTransaction, 0, 0);
  const LONG ordinal = InterlockedIncrement(&s_eiemNativeSkinRecordProbeCalls);
  const LONG previousTransaction = InterlockedExchange(
      &s_eiemNativeSkinRecordProbeLastTransaction, transaction);
  if (transaction != previousTransaction)
    InterlockedExchange(&s_eiemNativeSkinRecordProbeTransactionMatches, 0);
  // C7B750 is reached only while Unity is rebuilding a skin record, but it
  // can still run for many renderers. Keep the evidence bounded and select
  // the four-bone records used by the cloth path after each F10 transaction.
  void *meshOrBindpose = EiemReadNativeFieldPointer(self, 0x298);
  const uint32_t field1c8 = EiemReadNativeFieldU32(meshOrBindpose, 0x1C8);
  const LONG targetLikeOrdinal = (transaction > 0 && field1c8 == 4)
      ? InterlockedIncrement(&s_eiemNativeSkinRecordProbeTransactionMatches)
      : 0;
  const bool logCall = kEiemEnableNativeSkinRecordProbe &&
      (ordinal <= 96 ||
       (transaction > 0 && transaction != previousTransaction));
  const bool logTargetLike = kEiemEnableNativeSkinRecordProbe &&
      targetLikeOrdinal > 0 && targetLikeOrdinal <= 16;
  const uint32_t field36e = EiemReadNativeFieldU32(self, 0x36E);
  const uint32_t field370 = EiemReadNativeFieldU32(self, 0x370);
  const uint32_t field374 = EiemReadNativeFieldU32(self, 0x374);
  char stack[384] = {};
  if (logCall || (logTargetLike && targetLikeOrdinal == 1))
    EiemFormatNativeStack(stack, sizeof(stack));
  auto original = s_origNativeSkinRecordBuild;
  const int64_t result = original ? original(self) : 0;
  if (logCall || logTargetLike) {
    const LONG supportCalls = InterlockedCompareExchange(
        &s_eiemNativeSkinSupportCalls, 0, 0);
    const LONG supportLast = InterlockedCompareExchange(
        &s_eiemNativeSkinSupportLast, -1, -1);
    const uint8_t gateA = EiemReadUnityByteRva(0x1CF5CC4);
    const uint8_t gateB = EiemReadUnityByteRva(0x1CF5CC5);
    const uint8_t gateC = EiemReadUnityByteRva(0x1C59C90);
    Log("[NATIVE-SKIN-RECORD-v2] ordinal=%ld targetLike=%ld transaction=%ld "
        "generation=%ld self=%p meshOrBindpose=%p field1c8=%u "
        "field36e=%u field370=%u field374=%u result=%lld "
        "supportCalls=%ld supportLast=%ld gateA=%u gateB=%u gateC=%u "
        "after398=%p after3d8=%p after3e0=%p stack=%s",
        ordinal, targetLikeOrdinal, transaction, generation, self,
        meshOrBindpose, field1c8,
        field36e, field370, field374, static_cast<long long>(result),
        supportCalls, supportLast, gateA, gateB, gateC,
        EiemReadNativeFieldPointer(self, 0x398),
        EiemReadNativeFieldPointer(self, 0x3D8),
        EiemReadNativeFieldPointer(self, 0x3E0),
        stack[0] ? stack : "<empty>");
  }
  if (kEiemEnableNativeBoundaryStacks) {
    EiemRegistrationTraceNativeStackContext(
        "UnityPlayer.C7B750.entry", self, meshOrBindpose,
        reinterpret_cast<void *>(static_cast<uintptr_t>(field1c8)), generation);
    Log("[NATIVE-SKIN-RECORD] rva=C7B750 self=%p metadata=%p count=%u "
        "field36e=%u field370=%u field374=%u generation=%ld",
        self, meshOrBindpose, field1c8, field36e, field370, field374,
        generation);
  }
  return result;
}

static int64_t __fastcall TraceNativeSkinMetadataReset(void *self) {
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  const LONG transaction = InterlockedCompareExchange(
      &s_eiemSkinTargetTransaction, 0, 0);
  void *before = EiemReadNativeFieldPointer(self, 0x298);
  EiemRegistrationTraceNativeStackContext(
      "UnityPlayer.C7BB30.metadata-reset.entry", self, before, nullptr,
      generation);
  auto original = s_origNativeSkinMetadataReset;
  const int64_t result = original ? original(self) : 0;
  void *after = EiemReadNativeFieldPointer(self, 0x298);
  if (kEiemEnableNativeSkinRecordProbe && transaction > 0) {
    const LONG previousTransaction = InterlockedExchange(
        &s_eiemNativeSkinResetLastTransaction, transaction);
    if (transaction != previousTransaction)
      InterlockedExchange(&s_eiemNativeSkinResetTransactionMatches, 0);
    const LONG ordinal = InterlockedIncrement(
        &s_eiemNativeSkinResetTransactionMatches);
    if (ordinal <= 8) {
      void *before38 = EiemReadNativeFieldPointer(before, 0x38);
      void *beforeAux = EiemReadNativeFieldPointer(before38, 0x190);
      void *before50 = EiemReadNativeFieldPointer(beforeAux, 0x50);
      void *after38 = EiemReadNativeFieldPointer(after, 0x38);
      void *afterAux = EiemReadNativeFieldPointer(after38, 0x190);
      void *after50 = EiemReadNativeFieldPointer(afterAux, 0x50);
      Log("[NATIVE-SKIN-RESET-v1] ordinal=%ld transaction=%ld self=%p "
          "before=%p after=%p result=%lld beforeMeta38=%p "
          "beforeAux190=%p beforeAux50=%p afterMeta38=%p afterAux190=%p "
          "afterAux50=%p generation=%ld",
          ordinal, transaction, self, before, after,
          static_cast<long long>(result), before38, beforeAux, before50,
          after38, afterAux, after50, generation);
    }
  }
  if (kEiemEnableNativeBoundaryStacks)
    Log("[NATIVE-SKIN-METADATA-RESET] rva=C7BB30 self=%p before=%p "
        "after=%p result=%lld generation=%ld",
        self, before, after, static_cast<long long>(result), generation);
  return result;
}

static uint64_t EiemReadNativeU64At(const void *object, size_t offset) {
  if (!object) return 0;
  __try { return *(const uint64_t *)((const char *)object + offset); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

static uint32_t EiemReadNativeU32At(const void *object, size_t offset) {
  if (!object) return 0;
  __try { return *(const uint32_t *)((const char *)object + offset); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
}

// DF9CA0 appends one native source record. The field that reaches the final
// draw record is not yet proven to be input+0x10, so the disposable source
// trace records the first bounded batch of complete input words instead of
// filtering on a guessed flag value.
static int64_t __fastcall TraceNativeFlagRecordAppend(
    void *records, const void *input) {
  LONG transaction = 0;
  const bool traceWindow = EiemNativeFlagTraceWindowActive(&transaction);
  const uint32_t value = EiemReadNativeU32At(input, 0x10);
  const LONG ordinal = traceWindow
      ? InterlockedIncrement(&s_eiemNativeFlagSourceAppendCount) : 0;
  // DF9CA0 is hot. Keep stack capture and output bounded to the first 256
  // records in this disposable evidence window.
  if (traceWindow && ordinal <= 256) {
    const uint64_t base = EiemReadNativeU64At(records, 0);
    const uint32_t countBefore = EiemReadNativeU32At(records, 0x0C);
    const uint32_t stride = EiemReadNativeU32At(records, 0x10);
    void *frames[8] = {};
    const USHORT frameCount = CaptureStackBackTrace(
        1, static_cast<DWORD>(_countof(frames)), frames, nullptr);
    char stack[384] = {};
    size_t used = 0;
    for (USHORT index = 0; index < frameCount; ++index) {
      const int written = _snprintf_s(
          stack + used, sizeof(stack) - used, _TRUNCATE, "%s%p",
          index ? "," : "", frames[index]);
      if (written <= 0) break;
      used += static_cast<size_t>(written);
      if (used + 24 >= sizeof(stack)) break;
    }
    Log("[NATIVE-FLAG-SOURCE-APPEND] ordinal=%ld transaction=%ld "
        "records=%p base=%p "
        "countBefore=%u capacityOrStride=%u input=%p value=%u "
        "word00=%08X word04=%08X word0c=%08X word10=%08X "
        "word14=%08X word18=%08X word1c=%08X word20=%08X "
        "word30=%08X word34=%08X stack=%s",
        ordinal, transaction, records, reinterpret_cast<void *>(base),
        countBefore, stride,
        input, value, EiemReadNativeU32At(input, 0x00),
        EiemReadNativeU32At(input, 0x04), EiemReadNativeU32At(input, 0x0C),
        EiemReadNativeU32At(input, 0x10), EiemReadNativeU32At(input, 0x14),
        EiemReadNativeU32At(input, 0x18), EiemReadNativeU32At(input, 0x1C),
        EiemReadNativeU32At(input, 0x20), EiemReadNativeU32At(input, 0x30),
        EiemReadNativeU32At(input, 0x34), stack[0] ? stack : "<empty>");
  }
  auto original = s_origNativeFlagRecordAppend;
  return original ? original(records, input) : 0;
}

// DF1A60 consumes the source table assembled by DEBE40/DECCA0. The hook only
// records the table/context identity and the current destination-buffer
// pointers; it does not scan or alter the table.
static void * __fastcall TraceNativeFlagDrawFlush(void *context) {
  LONG transaction = 0;
  const bool traceWindow = EiemNativeFlagTraceWindowActive(&transaction);
  const LONG ordinal = traceWindow
      ? InterlockedIncrement(&s_eiemNativeFlagSourceFlushCount) : 0;
  if (kEiemEnableNativeFlagSourceTrace && traceWindow && ordinal <= 64) {
    void *sourceTable = reinterpret_cast<void *>(
        EiemReadNativeU64At(context, 0x28));
    Log("[NATIVE-FLAG-SOURCE-FLUSH] ordinal=%ld transaction=%ld "
        "context=%p sourceTable=%p "
        "drawBuffer=%p drawOffsetPtr=%p streamStride=%u stack=%p,%p,%p,%p",
        ordinal, transaction, context, sourceTable,
        reinterpret_cast<void *>(EiemReadNativeU64At(context, 0x20)),
        reinterpret_cast<void *>(EiemReadNativeU64At(context, 0x08)),
        EiemReadNativeU32At(context, 0x34),
        _ReturnAddress(), nullptr, nullptr, nullptr);
  }
  if (kEiemEnableNativeFlagDrawTrace) {
    const bool traceWindow = EiemNativeFlagTraceWindowActive(&transaction);
    const LONG traceOrdinal = traceWindow
        ? InterlockedIncrement(&s_eiemNativeFlagDrawTraceCount)
        : 0;
    // DF1A60 is a render submission hot path. Keep this disposable evidence
    // build bounded even if the game flushes continuously during the window.
    if (traceWindow && traceOrdinal <= 16) {
      const uint64_t sourceTable = EiemReadNativeU64At(context, 0x28);
      const uint64_t groupState = EiemReadNativeU64At(context, 0x10);
      const uint64_t indexState = EiemReadNativeU64At(context, 0x18);
      const uint64_t entryBegin = EiemReadNativeU64At(
          reinterpret_cast<const void *>(groupState), 0x00);
      // DF1A60 uses groupState+0x00 as the first entry and
      // groupState+0x10 as an entry COUNT (not an end pointer):
      //     end = begin + count * 8
      // Likewise indexState is a small wrapper whose +0x00 field is the
      // actual pair array.  The previous observer treated the count as a
      // pointer and indexState itself as the pair array, so it emitted only
      // the context line and silently skipped all records.
      const uint64_t entryCount = EiemReadNativeU64At(
          reinterpret_cast<const void *>(groupState), 0x10);
      const uint64_t indexBase = EiemReadNativeU64At(
          reinterpret_cast<const void *>(indexState), 0x00);
      Log("[NATIVE-FLAG-DRAW-TRACE] ordinal=%ld context=%p sourceTable=%p "
          "groupState=%p indexState=%p indexBase=%p entryBegin=%p "
          "entryCount=%llu streamStride=%u transaction=%ld",
          traceOrdinal, context, reinterpret_cast<void *>(sourceTable),
          reinterpret_cast<void *>(groupState),
          reinterpret_cast<void *>(indexState),
          reinterpret_cast<void *>(indexBase),
          reinterpret_cast<void *>(entryBegin),
          static_cast<unsigned long long>(entryCount),
          EiemReadNativeU32At(context, 0x34), transaction);

      // Each group entry is 8 bytes.  The entry's +4 dword indexes a pair in
      // indexState; that pair supplies the source-record index and count used
      // by DF1A60.  The count is trusted only within a generous sanity
      // bound; there is no per-run observation quota.
      if (entryBegin && entryCount > 0 && entryCount <= 4096 && indexBase &&
          sourceTable) {
        const uint64_t groupCount = entryCount;
        for (uint64_t group = 0; group < groupCount && group < 128; ++group) {
          const uint64_t entry = entryBegin + group * 8u;
          const uint32_t pairIndex = EiemReadNativeU32At(
              reinterpret_cast<const void *>(entry), 4);
          const uint64_t pair = indexBase +
              static_cast<uint64_t>(pairIndex) * 8u;
          const uint32_t recordIndex = EiemReadNativeU32At(
              reinterpret_cast<const void *>(pair), 0);
          const uint32_t recordCount = EiemReadNativeU32At(
              reinterpret_cast<const void *>(pair), 4);
          const uint64_t record = sourceTable +
              static_cast<uint64_t>(recordIndex) * 0x40u;
          Log("[NATIVE-FLAG-DRAW-RECORD] ordinal=%ld transaction=%ld "
              "group=%llu pairIndex=%u "
              "recordIndex=%u recordCount=%u record=%p value10=%u "
              "stride1e=%u source30=%p",
              traceOrdinal, transaction,
              static_cast<unsigned long long>(group), pairIndex,
              recordIndex, recordCount, reinterpret_cast<void *>(record),
              EiemReadNativeU32At(reinterpret_cast<const void *>(record), 0x10),
              EiemReadNativeU32At(reinterpret_cast<const void *>(record), 0x1E),
              reinterpret_cast<void *>(EiemReadNativeU64At(
                  reinterpret_cast<const void *>(record), 0x30)));
        }
      }
    }
  }
  auto original = s_origNativeFlagDrawFlush;
  return original ? original(context) : nullptr;
}

static void EiemInstallNativeSkinMetadataTrace() {
  if (!kEiemEnableNativeBoundaryStacks && !kEiemEnableNativeFlagSourceTrace &&
      !kEiemEnableNativeFlagDrawTrace && !kEiemEnableNativeSkinSupportProbe &&
      !kEiemEnableNativeSkinRecordProbe && !kEiemEnableNativeSkinModeProbe &&
      !kEiemEnableNativeSkinValidationProbe &&
       !kEiemEnableNativeSkinC7TargetProbe &&
       !kEiemEnableNativeMeshSetterTrace &&
       !kEiemEnableNativeMesh1c8CallTrace &&
       !kEiemEnableNativeMeshCtorTrace &&
       !kEiemEnableNativeMeshDeserializeTrace)
    return;
  EiemInstallMeshSetterTrace();
  if (!kEiemEnableNativeBoundaryStacks && !kEiemEnableNativeFlagSourceTrace &&
      !kEiemEnableNativeFlagDrawTrace && !kEiemEnableNativeSkinSupportProbe &&
      !kEiemEnableNativeSkinRecordProbe && !kEiemEnableNativeSkinModeProbe &&
       !kEiemEnableNativeSkinValidationProbe &&
       !kEiemEnableNativeSkinC7TargetProbe &&
       !kEiemEnableNativeMesh1c8CallTrace &&
       !kEiemEnableNativeMeshCtorTrace &&
       !kEiemEnableNativeMeshDeserializeTrace)
    return;
  HMODULE unity = GetModuleHandleW(L"UnityPlayer.dll");
  if (!unity) {
    Log("[NATIVE-SKIN-TRACE] UnityPlayer.dll not loaded");
    return;
  }
  const uintptr_t base = reinterpret_cast<uintptr_t>(unity);
  void *recordTarget = reinterpret_cast<void *>(base + 0xC7B750);
  void *resetTarget = reinterpret_cast<void *>(base + 0xC7BB30);
  void *modeTarget = reinterpret_cast<void *>(base + 0xEF200);
  void *validationTarget = reinterpret_cast<void *>(base + 0x19CE90);
  void *mesh1c8Target = reinterpret_cast<void *>(base + 0x4A90D0);
  void *meshCtorTarget = reinterpret_cast<void *>(base + 0xD9E70);
  void *meshDeserializeTarget = reinterpret_cast<void *>(base + 0xDA400);
  void *meshFieldVisitorTarget = reinterpret_cast<void *>(base + 0x1706F04);
  void *meshBinaryStreamTarget = reinterpret_cast<void *>(base + 0x1EA2A0);
  MH_STATUS recordStatus = MH_ERROR_DISABLED;
  MH_STATUS resetStatus = MH_ERROR_DISABLED;
  MH_STATUS modeStatus = MH_ERROR_DISABLED;
  MH_STATUS validationStatus = MH_ERROR_DISABLED;
  MH_STATUS mesh1c8Status = MH_ERROR_DISABLED;
  MH_STATUS meshCtorStatus = MH_ERROR_DISABLED;
  MH_STATUS meshDeserializeStatus = MH_ERROR_DISABLED;
  MH_STATUS meshFieldVisitorStatus = MH_ERROR_DISABLED;
  MH_STATUS meshBinaryStreamStatus = MH_ERROR_DISABLED;
  MH_STATUS supportStatus = MH_ERROR_DISABLED;
  if (kEiemEnableNativeSkinSupportProbe) {
    void *supportTarget = reinterpret_cast<void *>(base + 0x41FD40);
    supportStatus = MH_CreateHook(
        supportTarget, (void *)TraceNativeSkinSupport,
        (void **)&s_origNativeSkinSupport);
    if (supportStatus == MH_OK) MH_EnableHook(supportTarget);
  }
  if (kEiemEnableNativeSkinModeProbe) {
    modeStatus = MH_CreateHook(
        modeTarget, (void *)TraceNativeSkinMode,
        (void **)&s_origNativeSkinMode);
    if (modeStatus == MH_OK) MH_EnableHook(modeTarget);
  }
  if (kEiemEnableNativeSkinValidationProbe) {
    validationStatus = MH_CreateHook(
        validationTarget, (void *)TraceNativeSkinValidation,
        (void **)&s_origNativeSkinValidation);
    if (validationStatus == MH_OK) MH_EnableHook(validationTarget);
  }
  if (kEiemEnableNativeMesh1c8CallTrace) {
    mesh1c8Status = MH_CreateHook(
        mesh1c8Target, (void *)TraceNativeMesh1c8Normalizer,
        (void **)&s_origNativeMesh1c8Normalizer);
    if (mesh1c8Status == MH_OK) MH_EnableHook(mesh1c8Target);
  }
  if (kEiemEnableNativeMeshCtorTrace ||
      kEiemEnableNativeMeshDeserializeTrace) {
    meshCtorStatus = MH_CreateHook(
        meshCtorTarget, (void *)TraceNativeMeshConstructor,
        (void **)&s_origNativeMeshCtor);
    if (meshCtorStatus == MH_OK) MH_EnableHook(meshCtorTarget);
  }
  if (kEiemEnableNativeMeshDeserializeTrace) {
    meshDeserializeStatus = MH_CreateHook(
        meshDeserializeTarget, (void *)TraceNativeMeshDeserialize,
        (void **)&s_origNativeMeshDeserialize);
    if (meshDeserializeStatus == MH_OK)
      meshDeserializeStatus = MH_EnableHook(meshDeserializeTarget);
    meshFieldVisitorStatus = MH_CreateHook(
        meshFieldVisitorTarget, (void *)TraceNativeMeshFieldVisitor,
        (void **)&s_origNativeMeshFieldVisitor);
    if (meshFieldVisitorStatus == MH_OK)
      meshFieldVisitorStatus = MH_EnableHook(meshFieldVisitorTarget);
    meshBinaryStreamStatus = MH_CreateHook(
        meshBinaryStreamTarget, (void *)TraceNativeMeshBinaryStream,
        (void **)&s_origNativeMeshBinaryStream);
    if (meshBinaryStreamStatus == MH_OK)
      meshBinaryStreamStatus = MH_EnableHook(meshBinaryStreamTarget);
    Log("[MESH-PAYLOAD-HOOK-v1] fieldVisitorTarget=%p status=%d "
        "binaryStreamTarget=%p status=%d",
        meshFieldVisitorTarget, (int)meshFieldVisitorStatus,
        meshBinaryStreamTarget, (int)meshBinaryStreamStatus);
  }
  if (kEiemEnableNativeBoundaryStacks || kEiemEnableNativeSkinRecordProbe ||
      kEiemEnableNativeSkinC7TargetProbe) {
    recordStatus = MH_CreateHook(
        recordTarget, (void *)TraceNativeSkinRecordBuild,
        (void **)&s_origNativeSkinRecordBuild);
    if (recordStatus == MH_OK) MH_EnableHook(recordTarget);
    if (kEiemEnableNativeBoundaryStacks || kEiemEnableNativeSkinRecordProbe) {
      resetStatus = MH_CreateHook(
          resetTarget, (void *)TraceNativeSkinMetadataReset,
          (void **)&s_origNativeSkinMetadataReset);
      if (resetStatus == MH_OK) MH_EnableHook(resetTarget);
    }
  }
  MH_STATUS sourceAppendStatus = MH_ERROR_DISABLED;
  MH_STATUS sourceFlushStatus = MH_ERROR_DISABLED;
  if (kEiemEnableNativeFlagSourceTrace || kEiemEnableNativeFlagDrawTrace) {
    void *sourceFlushTarget = reinterpret_cast<void *>(base + 0xDF1A60);
    if (kEiemEnableNativeFlagSourceTrace) {
      void *sourceAppendTarget = reinterpret_cast<void *>(base + 0xDF9CA0);
      sourceAppendStatus = MH_CreateHook(
          sourceAppendTarget, (void *)TraceNativeFlagRecordAppend,
          (void **)&s_origNativeFlagRecordAppend);
      if (sourceAppendStatus == MH_OK) MH_EnableHook(sourceAppendTarget);
    }
    sourceFlushStatus = MH_CreateHook(
        sourceFlushTarget, (void *)TraceNativeFlagDrawFlush,
        (void **)&s_origNativeFlagDrawFlush);
    if (sourceFlushStatus == MH_OK) MH_EnableHook(sourceFlushTarget);
  }
  Log("[NATIVE-SKIN-TRACE] recordTarget=%p recordStatus=%d resetTarget=%p "
      "resetStatus=%d modeTarget=%p modeStatus=%d validationTarget=%p "
      "validationStatus=%d mesh1c8Target=%p mesh1c8Status=%d "
      "meshCtorTarget=%p meshCtorStatus=%d meshDeserializeTarget=%p "
      "meshDeserializeStatus=%d supportStatus=%d "
      "sourceAppendStatus=%d sourceFlushStatus=%d",
      recordTarget, (int)recordStatus, resetTarget, (int)resetStatus,
      modeTarget, (int)modeStatus, validationTarget, (int)validationStatus,
      mesh1c8Target, (int)mesh1c8Status, meshCtorTarget,
      (int)meshCtorStatus, meshDeserializeTarget,
      (int)meshDeserializeStatus, (int)supportStatus,
      (int)sourceAppendStatus, (int)sourceFlushStatus);
}

// Nested IL2CPP classes are not reliably addressable through
// il2cpp_class_from_name across game builds. Locate Endfield's material
// RendererInfo by its behaviour and then resolve the Renderer reference from
// metadata. No native field offset is version-hardcoded.
static void *FindMaterialRendererInfoClass(void **assemblies,
                                           size_t assemblyCount) {
  if (!assemblies || !assemblyCount) return nullptr;
  for (size_t assemblyIndex = 0; assemblyIndex < assemblyCount;
       ++assemblyIndex) {
    void *image = il2cpp_assembly_get_image(assemblies[assemblyIndex]);
    if (!image) continue;
    const size_t classCount = il2cpp_image_get_class_count(image);
    for (size_t classIndex = 0; classIndex < classCount; ++classIndex) {
      void *klass = il2cpp_image_get_class(image, classIndex);
      if (!klass) continue;
      const char *name = il2cpp_class_get_name(klass);
      if (!name || strcmp(name, "RendererInfo") != 0 ||
          !FindMethodInHierarchy(klass, "TrySetSharedMaterial", 1) ||
          !FindMethodInHierarchy(klass, "TrySetSharedMaterials", 1) ||
          !FindMethodInHierarchy(klass, "TryReplaceSharedMaterials", 1))
        continue;

      const char *rendererFields[] = {"m_renderer",
                                      "<renderer>k__BackingField"};
      int rendererOffset = FindFieldInHierarchy(
          klass, rendererFields, _countof(rendererFields), nullptr);
      if (rendererOffset < 0)
        rendererOffset = FindFieldByTypeInHierarchy(
            klass, "UnityEngine.Renderer", nullptr);
      if (rendererOffset < 0) continue;

      s_materialRendererInfoRendererOffset = rendererOffset;
      return klass;
    }
  }
  return nullptr;
}

#include "eiem_native_physics_runtime.h"
#include "eiem_npc_model_owner.h"
#include "eiem_metadata_probe.h"

static void InitIl2CppResourceTrace(void **assemblies, size_t assemblyCount) {
  if (!assemblies || assemblyCount == 0) return;
  // Read-only metadata reconnaissance. The static route to these names is
  // blocked (Il2CppDumper cannot resolve this build's registration pointers), and
  // several earlier hooks were guessed wrong, so the exact class names and field
  // offsets are enumerated once here instead.
  // Metadata enumeration and the part-table mutation are research-only paths.
  // The former floods the startup log; the latter writes SubMeshInfo.isActive.
  // Re-enable them only in a dedicated evidence build.
  Log("[VALIDATION] mode=static-resource-baseline metadata-enumeration=%s "
      "part-table-mutation=off",
      kEiemEnableCustomSkinPipelineMetadata ? "custom-skin-only" : "off");
  Log("[PHYSICS-MODE] experimentalRuntime=%s nativeObservation=%s",
      kEiemEnableExperimentalPhysicsRuntime ? "enabled" : "disabled",
      kEiemEnableNativePhysicsObservation ? "enabled" : "disabled");
  EiemInstallNativeSkinMetadataTrace();
  EiemInitUnityLifetime(assemblies, assemblyCount);
  EiemInstallNpcModelOwner(assemblies, assemblyCount);
  if (kEiemEnableCustomSkinPipelineMetadata)
    EiemDumpCustomSkinPipelineMetadata(assemblies, assemblyCount);

  // This manager is a candidate custom-pipeline boundary. The hook is
  // observation-only and bounded by the existing cold/F10 timing window; it
  // is intentionally installed only with the custom-pipeline observation
  // switch enabled, so production rendering pays no per-frame logging cost.
  if (kEiemEnableCustomSkinPipelineObservation) {
    void *captureManagerClass = FindClass(
        "HG.Rendering.Runtime", "SkinnedMeshCaptureManager", assemblies,
        assemblyCount);
    if (captureManagerClass) {
      static const char *const captureRequestTypes[] = {
          "UnityEngine.MeshRenderer", "UnityEngine.SkinnedMeshRenderer",
          "UnityEngine.MaterialPropertyBlock"};
      HookTraceMethodWithParamTypesAndReturnType(
          captureManagerClass, "RequestCapture", captureRequestTypes, 3,
          "SkinnedMeshCaptureManager.RequestCapture", "System.Void",
          (void *)TraceSkinnedMeshCaptureRequest,
          &s_origSkinnedMeshCaptureRequest);
    } else {
      Log("[RES-TRACE] HG.Rendering.Runtime.SkinnedMeshCaptureManager class not found");
    }

    // This is the game's custom cloth simulation/upload owner.  Unlike the
    // public Unity skin APIs, its metadata exposes the actual cloth skeleton
    // ComputeBuffer and the render-graph handoff methods.  Keep every hook
    // observation-only and resolve overloads by both parameter and return
    // type; value-type cloth data is passed through opaquely.
    void *gpuClothManagerClass = FindClass(
        "HG.Rendering.Runtime", "GpuClothManager", assemblies,
        assemblyCount);
    if (gpuClothManagerClass) {
      static const char *const gpuClothTickTypes[] = {"System.Single"};
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "Tick", gpuClothTickTypes, 1,
          "GpuClothManager.Tick", "System.Void",
          (void *)TraceGpuClothTick, &s_origGpuClothTick);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "_SetPerDrawData", nullptr, 0,
          "GpuClothManager._SetPerDrawData", "System.Void",
          (void *)TraceGpuClothSetPerDrawData,
          &s_origGpuClothSetPerDrawData);
      static const char *const gpuClothRegisterTypes[] = {
          "HG.Rendering.Runtime.ClothGroupData&"};
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "RegisterClothGroup", gpuClothRegisterTypes,
          1, "GpuClothManager.RegisterClothGroup", "System.Void",
          (void *)TraceGpuClothRegisterGroup, &s_origGpuClothRegisterGroup);
      static const char *const gpuClothMeshTypes[] = {"UnityEngine.Mesh"};
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "_SetCharacterProxyMesh", gpuClothMeshTypes,
          1, "GpuClothManager._SetCharacterProxyMesh", "System.Void",
          (void *)TraceGpuClothSetCharacterProxyMesh,
          &s_origGpuClothSetCharacterProxyMesh);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "GetSkeletonBuffer", nullptr, 0,
          "GpuClothManager.GetSkeletonBuffer",
          "UnityEngine.ComputeBuffer",
          (void *)TraceGpuClothGetSkeletonBuffer,
          &s_origGpuClothGetSkeletonBuffer);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "IsClothSkeletonValid", nullptr, 0,
          "GpuClothManager.IsClothSkeletonValid", "System.Boolean",
          (void *)TraceGpuClothIsSkeletonValid,
          &s_origGpuClothIsSkeletonValid);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "IsClothSkeletonFlipped", nullptr, 0,
          "GpuClothManager.IsClothSkeletonFlipped", "System.Boolean",
          (void *)TraceGpuClothIsSkeletonFlipped,
          &s_origGpuClothIsSkeletonFlipped);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuClothManagerClass, "FlipSkeletonFlag", nullptr, 0,
          "GpuClothManager.FlipSkeletonFlag", "System.Void",
          (void *)TraceGpuClothFlipSkeletonFlag,
          &s_origGpuClothFlipSkeletonFlag);
      static const char *const gpuClothPipelineTypes[] = {
          "UnityEngine.Transform"};
      HookTraceGpuClothPipelineUpdateV2(
          gpuClothManagerClass, gpuClothPipelineTypes, 1,
          "GpuClothManager.PipelineUpdateV2",
          (void *)TraceGpuClothPipelineUpdateV2,
          &s_origGpuClothPipelineUpdateV2,
          (void *)TraceGpuClothPipelineUpdateV2Static,
          &s_origGpuClothPipelineUpdateV2Static);
    } else {
      Log("[RES-TRACE] HG.Rendering.Runtime.GpuClothManager class not found");
    }

    // The target may bypass SkinnedMeshCaptureManager entirely and bind the
    // palette through a MaterialPropertyBlock.  Observe both regular and
    // constant-buffer bindings so the next run can distinguish "no upload"
    // from "upload to a wrong ring-buffer segment" without touching the draw.
    void *propertyBlockClass = FindClass(
        "UnityEngine", "MaterialPropertyBlock", assemblies, assemblyCount);
    if (propertyBlockClass) {
      static const char *const bufferTypes[] = {
          "System.Int32", "UnityEngine.ComputeBuffer", "System.Int32",
          "System.Int32"};
      HookTraceMethodWithParamTypesAndReturnType(
          propertyBlockClass, "SetBufferImpl", bufferTypes, 4,
          "MaterialPropertyBlock.SetBufferImpl", "System.Void",
          (void *)TraceMaterialPropertyBlockSetBuffer,
          &s_origMaterialPropertyBlockSetBuffer);
      HookTraceMethodWithParamTypesAndReturnType(
          propertyBlockClass, "SetConstantBufferImpl", bufferTypes, 4,
          "MaterialPropertyBlock.SetConstantBufferImpl", "System.Void",
          (void *)TraceMaterialPropertyBlockSetConstantBuffer,
          &s_origMaterialPropertyBlockSetConstantBuffer);
    } else {
      Log("[RES-TRACE] UnityEngine.MaterialPropertyBlock class not found");
    }

    void *materialClass =
        FindClass("UnityEngine", "Material", assemblies, assemblyCount);
    if (materialClass) {
      static const char *const materialBufferTypes[] = {
          "System.Int32", "UnityEngine.ComputeBuffer", "System.Int32",
          "System.Int32"};
      HookTraceMethodWithParamTypesAndReturnType(
          materialClass, "SetConstantBufferImpl", materialBufferTypes, 4,
          "Material.SetConstantBufferImpl", "System.Void",
          (void *)TraceMaterialSetConstantBuffer,
          &s_origMaterialSetConstantBuffer);
    } else {
      Log("[RES-TRACE] UnityEngine.Material class not found");
    }

    void *renderGraphRegistryClass = FindClass(
        "HG.Rendering.RenderGraphModule", "HGRenderGraphResourceRegistry",
        assemblies, assemblyCount);
    if (renderGraphRegistryClass) {
      static const char *const computeBufferHandleTypes[] = {
          "HG.Rendering.RenderGraphModule.ComputeBufferHandle&"};
      HookTraceMethodWithParamTypesAndReturnType(
          renderGraphRegistryClass, "GetComputeBuffer",
          computeBufferHandleTypes, 1,
          "HGRenderGraphResourceRegistry.GetComputeBuffer",
          "UnityEngine.ComputeBuffer",
          (void *)TraceRenderGraphGetComputeBuffer,
          &s_origRenderGraphGetComputeBuffer);
    } else {
      Log("[RES-TRACE] HGRenderGraphResourceRegistry class not found");
    }

    // HG's renderer can bypass Material/MaterialPropertyBlock and record
    // the palette directly on UnityEngine.Rendering.CommandBuffer.  These
    // descriptors are the last managed command-recording boundary before
    // the native backend sees the buffer segment.
    void *commandBufferClass = FindClass(
        "UnityEngine.Rendering", "CommandBuffer", assemblies, assemblyCount);
    if (commandBufferClass) {
      static const char *const globalConstantIdTypes[] = {
          "System.UInt32", "System.Int32", "System.Int32", "System.Int32"};
      HookTraceMethodWithParamTypesAndReturnType(
          commandBufferClass, "SetGlobalConstantBufferInternal0",
          globalConstantIdTypes, 4,
          "CommandBuffer.SetGlobalConstantBufferInternal0", "System.Void",
          (void *)TraceCommandBufferSetGlobalConstantBuffer0,
          &s_origCommandBufferSetGlobalConstantBuffer0);

      static const char *const globalBufferIdTypes[] = {
          "System.Int32", "System.UInt32"};
      HookTraceMethodWithParamTypesAndReturnType(
          commandBufferClass, "SetGlobalBufferIDInternal", globalBufferIdTypes,
          2, "CommandBuffer.SetGlobalBufferIDInternal", "System.Void",
          (void *)TraceCommandBufferSetGlobalBufferId,
          &s_origCommandBufferSetGlobalBufferId);

      static const char *const globalConstantTypes[] = {
          "UnityEngine.ComputeBuffer", "System.Int32", "System.Int32",
          "System.Int32"};
      HookTraceMethodWithParamTypesAndReturnType(
          commandBufferClass, "SetGlobalConstantBufferInternal",
          globalConstantTypes, 4,
          "CommandBuffer.SetGlobalConstantBufferInternal", "System.Void",
          (void *)TraceCommandBufferSetGlobalConstantBuffer,
          &s_origCommandBufferSetGlobalConstantBuffer);

      static const char *const globalBufferTypes[] = {
          "System.Int32", "UnityEngine.ComputeBuffer"};
      HookTraceMethodWithParamTypesAndReturnType(
          commandBufferClass, "SetGlobalBufferInternal", globalBufferTypes, 2,
          "CommandBuffer.SetGlobalBufferInternal", "System.Void",
          (void *)TraceCommandBufferSetGlobalBuffer,
          &s_origCommandBufferSetGlobalBuffer);
    } else {
      Log("[RES-TRACE] UnityEngine.Rendering.CommandBuffer class not found");
    }

    // Endfield's HG graphics module has a second submission layer above the
    // backend.  Capture both renderer versions in one pass; all three methods
    // carry the command buffer/list identifiers needed to correlate a skin
    // transaction without touching the renderer data.
    const char *const gpuBindTypes[] = {"UnityEngine.Rendering.CommandBuffer"};
    const char *const gpuPopulateTypes[] = {
        "UnityEngine.Rendering.CommandBuffer", "System.UInt32",
        "System.UInt32", "System.Boolean"};
    const char *const gpuDrawTypes[] = {
        "UnityEngine.Rendering.CommandBuffer", "System.UInt32",
        "System.Boolean"};
    const char *const gpuComputeTypes[] = {
        "UnityEngine.Rendering.CommandBuffer", "UnityEngine.ComputeShader",
        "System.UInt32"};
    const char *const gpuFrameGlobalTypes[] = {
        "UnityEngine.Rendering.CommandBuffer"};
    void *gpuV1Class = FindClass("UnityEngine.HyperGryph",
                                 "GPUDrivenRendererV1", assemblies,
                                 assemblyCount);
    if (gpuV1Class) {
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "BindBuffersForRendering", gpuBindTypes, 1,
          "GPUDrivenRendererV1.BindBuffersForRendering", "System.Void",
          (void *)TraceGpuV1BindBuffersForRendering,
          &s_origGpuDrivenV1BindBuffersForRendering);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "PopulatePerFrameData", gpuPopulateTypes, 4,
          "GPUDrivenRendererV1.PopulatePerFrameData", "System.Void",
          (void *)TraceGpuV1PopulatePerFrameData,
          &s_origGpuDrivenV1PopulatePerFrameData);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "DrawRendererList", gpuDrawTypes, 3,
          "GPUDrivenRendererV1.DrawRendererList", "System.Void",
          (void *)TraceGpuV1DrawRendererList,
          &s_origGpuDrivenV1DrawRendererList);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "BindBuffersForCulling", gpuComputeTypes, 3,
          "GPUDrivenRendererV1.BindBuffersForCulling", "System.Void",
          (void *)TraceGpuV1BindBuffersForCulling,
          &s_origGpuDrivenV1BindBuffersForCulling);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "BindFrameConstantsBuffer", gpuComputeTypes, 3,
          "GPUDrivenRendererV1.BindFrameConstantsBuffer", "System.Void",
          (void *)TraceGpuV1BindFrameConstants,
          &s_origGpuDrivenV1BindFrameConstants);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "BindFrameConstantsBufferGlobal", gpuFrameGlobalTypes,
          1, "GPUDrivenRendererV1.BindFrameConstantsBufferGlobal",
          "System.Void", (void *)TraceGpuV1BindFrameConstantsGlobal,
          &s_origGpuDrivenV1BindFrameConstantsGlobal);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "DispatchComputeMeshletInstanceCount", gpuComputeTypes,
          3, "GPUDrivenRendererV1.DispatchComputeMeshletInstanceCount",
          "System.Void", (void *)TraceGpuV1DispatchMeshletInstanceCount,
          &s_origGpuDrivenV1DispatchMeshletInstanceCount);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "DispatchComputeDrawBucketCount", gpuComputeTypes, 3,
          "GPUDrivenRendererV1.DispatchComputeDrawBucketCount", "System.Void",
          (void *)TraceGpuV1DispatchDrawBucketCount,
          &s_origGpuDrivenV1DispatchDrawBucketCount);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV1Class, "AdvanceFrame", nullptr, 0,
          "GPUDrivenRendererV1.AdvanceFrame", "System.Void",
          (void *)TraceGpuV1AdvanceFrame, &s_origGpuDrivenV1AdvanceFrame);
    } else {
      Log("[RES-TRACE] GPUDrivenRendererV1 class not found");
    }
    void *gpuV2Class = FindClass("UnityEngine.HyperGryph",
                                 "GPUDrivenRendererV2", assemblies,
                                 assemblyCount);
    if (gpuV2Class) {
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "BindBuffersForRendering", gpuBindTypes, 1,
          "GPUDrivenRendererV2.BindBuffersForRendering", "System.Void",
          (void *)TraceGpuV2BindBuffersForRendering,
          &s_origGpuDrivenV2BindBuffersForRendering);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "PopulatePerFrameData", gpuPopulateTypes, 4,
          "GPUDrivenRendererV2.PopulatePerFrameData", "System.Void",
          (void *)TraceGpuV2PopulatePerFrameData,
          &s_origGpuDrivenV2PopulatePerFrameData);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "DrawRendererList", gpuDrawTypes, 3,
          "GPUDrivenRendererV2.DrawRendererList", "System.Void",
          (void *)TraceGpuV2DrawRendererList,
          &s_origGpuDrivenV2DrawRendererList);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "BindBuffersForCulling", gpuComputeTypes, 3,
          "GPUDrivenRendererV2.BindBuffersForCulling", "System.Void",
          (void *)TraceGpuV2BindBuffersForCulling,
          &s_origGpuDrivenV2BindBuffersForCulling);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "BindFrameConstantsBuffer", gpuComputeTypes, 3,
          "GPUDrivenRendererV2.BindFrameConstantsBuffer", "System.Void",
          (void *)TraceGpuV2BindFrameConstants,
          &s_origGpuDrivenV2BindFrameConstants);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "BindFrameConstantsBufferGlobal", gpuFrameGlobalTypes,
          1, "GPUDrivenRendererV2.BindFrameConstantsBufferGlobal",
          "System.Void", (void *)TraceGpuV2BindFrameConstantsGlobal,
          &s_origGpuDrivenV2BindFrameConstantsGlobal);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "DispatchComputeMeshletInstanceCount", gpuComputeTypes,
          3, "GPUDrivenRendererV2.DispatchComputeMeshletInstanceCount",
          "System.Void", (void *)TraceGpuV2DispatchMeshletInstanceCount,
          &s_origGpuDrivenV2DispatchMeshletInstanceCount);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "DispatchComputeDrawBucketCount", gpuComputeTypes, 3,
          "GPUDrivenRendererV2.DispatchComputeDrawBucketCount", "System.Void",
          (void *)TraceGpuV2DispatchDrawBucketCount,
          &s_origGpuDrivenV2DispatchDrawBucketCount);
      HookTraceMethodWithParamTypesAndReturnType(
          gpuV2Class, "AdvanceFrame", nullptr, 0,
          "GPUDrivenRendererV2.AdvanceFrame", "System.Void",
          (void *)TraceGpuV2AdvanceFrame, &s_origGpuDrivenV2AdvanceFrame);
    } else {
      Log("[RES-TRACE] GPUDrivenRendererV2 class not found");
    }
  }

  // Install this before the per-Renderer material hook. EntityRenderHelper's
  // original _InitRenderAndMaterial builds the internal renderer registry by
  // scanning its hierarchy; Partners must already be present at that point.
  // This single boundary is shared by world, NPC and character-preview model
  // paths, so no context-specific array mutation is needed for assembly.
  void *entityRenderHelperClass = FindClass(
      "Beyond.Gameplay.View", "EntityRenderHelper", assemblies,
      assemblyCount);
  if (entityRenderHelperClass) {
    s_entityRenderHelperClass = entityRenderHelperClass;
    HookTraceMethod(
        entityRenderHelperClass, "_InitRenderAndMaterial", 0,
        "EntityRenderHelper._InitRenderAndMaterial",
        (void *)TraceEntityRenderHelperInitRenderAndMaterial,
        &s_origEntityRenderHelperInitRenderAndMaterial);
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.EntityRenderHelper class not found");
  }

  // Observe the cache commit made by EntityRenderHelperMaterialController.
  // Its Init signature is resolved by parameter type, so this probe cannot
  // silently bind a same-name overload with a different ABI.
  void *materialControllerClass = FindClass(
      "Beyond.Rendering", "EntityRenderHelperMaterialController", assemblies,
      assemblyCount);
  if (materialControllerClass) {
    const char *controllerInitTypes[] = {
        "System.Collections.Generic.List<UnityEngine.Renderer>",
        "System.Collections.Generic.List<Beyond.Rendering.EntityRendererTypeConfig>",
        "Beyond.Rendering.EntityCustomizeRendererPropertyConfig",
        "System.Boolean"};
    HookTraceMethodWithParamTypesAndReturnType(
        materialControllerClass, "Init", controllerInitTypes, 4,
        "EntityRenderHelperMaterialController.Init", "System.Void",
        (void *)TraceEntityRenderHelperMaterialControllerInit,
        &s_origEntityRenderHelperMaterialControllerInit);
  } else {
    Log("[RES-TRACE] Beyond.Rendering.EntityRenderHelperMaterialController class not found");
  }

  if (kEiemValidationIdentityProbe) {
    void *assetBundleClass = FindClass("UnityEngine", "AssetBundle", assemblies,
                                       assemblyCount);
    if (assetBundleClass) {
    HookTraceMethod(assetBundleClass, "LoadAsset", 1,
                    "AssetBundle.LoadAsset(string)",
                    (void *)TraceAssetBundleLoadAsset1,
                    &s_origAssetBundleLoadAsset1);
    HookTraceMethod(assetBundleClass, "LoadAsset", 2,
                    "AssetBundle.LoadAsset(string,Type)",
                    (void *)TraceAssetBundleLoadAsset2,
                    &s_origAssetBundleLoadAsset2);
    HookTraceMethod(assetBundleClass, "LoadAssetAsync", 1,
                    "AssetBundle.LoadAssetAsync(string)",
                    (void *)TraceAssetBundleLoadAssetAsync1,
                    &s_origAssetBundleLoadAssetAsync1);
    HookTraceMethod(assetBundleClass, "LoadAssetAsync", 2,
                    "AssetBundle.LoadAssetAsync(string,Type)",
                    (void *)TraceAssetBundleLoadAssetAsync2,
                    &s_origAssetBundleLoadAssetAsync2);
    HookTraceMethod(assetBundleClass, "Unload", 1,
                    "AssetBundle.Unload(bool)",
                    (void *)TraceAssetBundleUnload,
                    &s_origAssetBundleUnload);

    void *bundleRequestClass = FindClass(
        "UnityEngine", "AssetBundleCreateRequest", assemblies, assemblyCount);
    HookTraceMethod(bundleRequestClass, "get_assetBundle", 0,
                    "AssetBundleCreateRequest.get_assetBundle",
                    (void *)TraceAssetBundleCreateRequestGetAssetBundle,
                    &s_origAssetBundleCreateRequestGetAssetBundle);
    } else {
      Log("[RES-TRACE] UnityEngine.AssetBundle not found");
    }
  }

  // Endfield's RendererInfo owns the source/replacement material arrays and
  // is the actual commit boundary used during scene changes. Hook all three
  // commit forms on that one controller type; Unity's public property wrappers
  // are not retained because runtime evidence showed that this path bypasses
  // them.
  void *materialRendererInfoClass =
      FindMaterialRendererInfoClass(assemblies, assemblyCount);
  if (materialRendererInfoClass) {
    const char *infoInitTypes[] = {"UnityEngine.Renderer", "System.Collections.Generic.List<Beyond.Rendering.EntityRendererTypeConfig>"};
    void *infoInit = FindMethodWithParamTypesAndReturnType(materialRendererInfoClass, "_Init", infoInitTypes, 2, "System.Void");
    if (kEiemEnableMaterialLifecycle) {
      if (!infoInit || !Hook(infoInit, "RendererInfo._Init source isolation", (void *)TraceMaterialInfoInit,
                            &s_origMaterialInfoInit))
        Log("[MOD-MATERIAL-SOURCE] RendererInfo._Init source isolation unavailable");
    } else {
      Log("[VALIDATION] RendererInfo source isolation disabled");
    }
    Log("[RES-TRACE] Material RendererInfo renderer field: 0x%X",
        s_materialRendererInfoRendererOffset);
    HookTraceMethod(materialRendererInfoClass, "TrySetSharedMaterial", 1,
                    "RendererInfo.TrySetSharedMaterial",
                    (void *)TraceRendererInfoTrySetSharedMaterial,
                    &s_origRendererInfoTrySetSharedMaterial);
    HookTraceMethod(materialRendererInfoClass, "TrySetSharedMaterials", 1,
                    "RendererInfo.TrySetSharedMaterials",
                    (void *)TraceRendererInfoTrySetSharedMaterials,
                    &s_origRendererInfoTrySetSharedMaterials);
    HookTraceMethod(materialRendererInfoClass, "TryReplaceSharedMaterials", 1,
                    "RendererInfo.TryReplaceSharedMaterials",
                    (void *)TraceRendererInfoTryReplaceSharedMaterials,
                    &s_origRendererInfoTryReplaceSharedMaterials);
  } else {
    Log("[RES-TRACE] Endfield material RendererInfo not found; material lifecycle hook disabled");
  }

  void *smrClass = FindClass("UnityEngine", "SkinnedMeshRenderer", assemblies,
                            assemblyCount);
  EiemInitShapeGuard(FindClass("Beyond.Gameplay.Core", "SkeletalMorphCore",
                              assemblies, assemblyCount));
  HookTraceMethod(smrClass, "set_sharedMesh", 1,
                  "SkinnedMeshRenderer.set_sharedMesh",
                  (void *)TraceSkinnedMeshSetSharedMesh,
                  &s_origSkinnedMeshSetSharedMesh);
  HookTraceMethod(smrClass, "set_bones", 1,
                  "SkinnedMeshRenderer.set_bones",
                  (void *)TraceSkinnedMeshSetBones,
                  &s_origSkinnedMeshSetBones);
  if (kEiemEnableSkinTimingProbe) {
    // Disabled in the normal build. These hooks belong to the old GPU/skin
    // submission probe and must not be installed during ordinary rendering.
    HookTraceMethod(
        smrClass, "RequestCurrentFrameSkinMatrices", 2,
        "SkinnedMeshRenderer.RequestCurrentFrameSkinMatrices",
        (void *)TraceSkinnedMeshRequestCurrentFrameSkinMatrices,
        &s_origSkinnedMeshRequestCurrentFrameSkinMatrices);
    HookTraceMethod(
        smrClass, "SkinMatricesRequestFinished", 0,
        "SkinnedMeshRenderer.SkinMatricesRequestFinished",
        (void *)TraceSkinnedMeshSkinMatricesRequestFinished,
        &s_origSkinnedMeshSkinMatricesRequestFinished);
    HookTraceMethod(
        smrClass, "GetVertexBuffer", 0,
        "SkinnedMeshRenderer.GetVertexBuffer",
        (void *)TraceSkinnedMeshGetVertexBuffer,
        &s_origSkinnedMeshGetVertexBuffer);
    HookTraceMethod(
        smrClass, "GetPreviousVertexBuffer", 0,
        "SkinnedMeshRenderer.GetPreviousVertexBuffer",
        (void *)TraceSkinnedMeshGetPreviousVertexBuffer,
        &s_origSkinnedMeshGetPreviousVertexBuffer);
  }

  void *prefabInstantiateClass = FindClass(
      "Beyond.Resource.Runtime", "PrefabInstantiateProxy", assemblies,
      assemblyCount);
  if (prefabInstantiateClass) {
    s_prefabInstantiateGetGameObject = FindMethodWithReturnType(
        prefabInstantiateClass, "get_gameObject", "UnityEngine.GameObject",
        0);
    s_prefabInstantiateGetLogName = FindMethodWithReturnType(
        prefabInstantiateClass, "GetLogName", "System.String", 0);
    s_prefabInstantiateGetInstanceUid = FindMethodWithReturnType(
        prefabInstantiateClass, "get_instanceUid", "System.UInt32", 0);
    void *completed = FindMethodWithReturnType(
        prefabInstantiateClass, "OnCompleted", "System.Void", 0);
    if (completed &&
        Hook(completed, "PrefabInstantiateProxy.OnCompleted",
             (void *)TracePrefabInstantiateCompleted,
             &s_origPrefabInstantiateCompleted)) {
      Log("[RES-TRACE] PrefabInstantiateProxy lifecycle anchor installed; gameObject=%p logName=%p instanceUid=%p",
          s_prefabInstantiateGetGameObject, s_prefabInstantiateGetLogName,
          s_prefabInstantiateGetInstanceUid);
    } else {
      Log("[RES-TRACE] PrefabInstantiateProxy.OnCompleted hook failed/not found");
    }
    HookTraceMethod(prefabInstantiateClass, "Unload", 0,
                    "PrefabInstantiateProxy.Unload",
                    (void *)TracePrefabInstantiateUnload,
                    &s_origPrefabInstantiateUnload);
    HookTraceMethod(prefabInstantiateClass, "Clear", 0,
                    "PrefabInstantiateProxy.Clear",
                    (void *)TracePrefabInstantiateClear,
                    &s_origPrefabInstantiateClear);
    HookTraceMethod(prefabInstantiateClass, "Dispose", 0,
                    "PrefabInstantiateProxy.Dispose",
                    (void *)TracePrefabInstantiateDispose,
                    &s_origPrefabInstantiateDispose);
  } else {
    Log("[RES-TRACE] Beyond.Resource.Runtime.PrefabInstantiateProxy class not found");
  }

  void *uiModelLoaderClass = FindClass(
      "Beyond.UI", "UIModelLoader", assemblies, assemblyCount);
  if (uiModelLoaderClass) {
    static const char *const uiLoadTypes[] = {
        "System.String", "UnityEngine.Transform"};
    void *loadModel = FindMethodWithParamTypesAndReturnType(
        uiModelLoaderClass, "LoadModel", uiLoadTypes, 2,
        "UnityEngine.GameObject");
    if (loadModel &&
        Hook(loadModel, "UIModelLoader.LoadModel",
             (void *)TraceUIModelLoaderLoadModel,
             &s_origUIModelLoaderLoadModel))
      Log("[RES-TRACE] UIModelLoader synchronous lifecycle adapter installed");
    else
      Log("[RES-TRACE] UIModelLoader.LoadModel hook failed/not found");

    static const char *const uiLoadAsyncTypes[] = {
        "System.String", "UnityEngine.Transform",
        "System.Action<UnityEngine.GameObject>"};
    void *loadModelAsync = FindMethodWithParamTypesAndReturnType(
        uiModelLoaderClass, "LoadModelAsync", uiLoadAsyncTypes, 3,
        "System.Int32");
    if (loadModelAsync &&
        Hook(loadModelAsync, "UIModelLoader.LoadModelAsync",
             (void *)TraceUIModelLoaderLoadModelAsync,
             &s_origUIModelLoaderLoadModelAsync))
      Log("[RES-TRACE] UIModelLoader async passthrough installed (game callback unchanged)");
    else
      Log("[RES-TRACE] UIModelLoader.LoadModelAsync hook failed/not found");

    static const char *const uiUnloadTypes[] = {"UnityEngine.GameObject"};
    HookTraceMethodWithParamTypes(
        uiModelLoaderClass, "UnloadModel", uiUnloadTypes, 1,
        "UIModelLoader.UnloadModel", (void *)TraceUIModelLoaderUnloadModel,
        &s_origUIModelLoaderUnloadModel);
    HookTraceMethod(uiModelLoaderClass, "_Clear", 0,
                    "UIModelLoader._Clear",
                    (void *)TraceUIModelLoaderClear,
                    &s_origUIModelLoaderClear);
    HookTraceMethod(uiModelLoaderClass, "Dispose", 0,
                    "UIModelLoader.Dispose",
                    (void *)TraceUIModelLoaderDispose,
                    &s_origUIModelLoaderDispose);
  } else {
    Log("[RES-TRACE] Beyond.UI.UIModelLoader class not found");
  }

  // Both UIModelLoader and CharUIModelMono ran in the v29 crash trace. Observe
  // the completed model's own lifecycle without replacing managed callbacks.
  // SetVisible also covers known models reactivated from a persistent pool.
  void *charUIModelClass = FindClass(
      "Beyond.Gameplay.View", "CharUIModelMono", assemblies, assemblyCount);
  if (charUIModelClass) {
    HookTraceMethod(charUIModelClass, "OnAwake", 0,
                    "CharUIModelMono.OnAwake",
                    (void *)TraceCharUIModelOnAwake,
                    &s_origCharUIModelOnAwake);
    static const char *const visibleTypes[] = {"System.Boolean"};
    HookTraceMethodWithParamTypes(
        charUIModelClass, "SetVisible", visibleTypes, 1,
        "CharUIModelMono.SetVisible", (void *)TraceCharUIModelSetVisible,
        &s_origCharUIModelSetVisible);
    HookTraceMethod(charUIModelClass, "OnRelease", 0,
                    "CharUIModelMono.OnRelease",
                    (void *)TraceCharUIModelOnRelease,
                    &s_origCharUIModelOnRelease);
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.CharUIModelMono class not found");
  }

  void *meshFilterClass =
      FindClass("UnityEngine", "MeshFilter", assemblies, assemblyCount);
  HookTraceMethod(meshFilterClass, "set_sharedMesh", 1,
                  "MeshFilter.set_sharedMesh",
                  (void *)TraceMeshFilterSetSharedMesh,
                  &s_origMeshFilterSetSharedMesh);

  // Generic character/model lifecycle. BaseModelViewPart is also the explicit
  // completion owner for cached/handle-reused character models.
  void *modelManagerClass = FindClass("Beyond.Gameplay.View", "ModelManager",
                                      assemblies, assemblyCount);
  if (modelManagerClass) {
    static const char *const modelGameObjectType[] = {
        "UnityEngine.GameObject"};
    HookTraceMethodWithParamTypes(
        modelManagerClass, "_OnGameObjectAllocate", modelGameObjectType, 1,
        "ModelManager._OnGameObjectAllocate",
        (void *)TraceModelManagerGameObjectAllocate,
        &s_origModelManagerGameObjectAllocate);

    static const char *const modelPathHashType[] = {
        "Beyond.Resource.StringPathHash"};
    void *loadPersistent = FindMethodWithParamTypesAndReturnType(
        modelManagerClass, "LoadFromPersistentPool", modelPathHashType, 1,
        "UnityEngine.GameObject");
    if (loadPersistent &&
        Hook(loadPersistent, "ModelManager.LoadFromPersistentPool",
             (void *)TraceModelManagerLoadFromPersistentPool,
             &s_origModelManagerLoadFromPersistentPool))
      Log("[RES-TRACE] ModelManager.LoadFromPersistentPool replacement hook installed");
    else
      Log("[RES-TRACE] ModelManager.LoadFromPersistentPool hook failed/not found");
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.ModelManager class not found");
  }

  void *basePartClass = FindClass("Beyond.Gameplay.View", "BaseModelViewPart",
                                  assemblies, assemblyCount);
  if (basePartClass) {
    const char *partModelFields[] = {"m_model"};
    const char *partConfigFields[] = {"m_cfg"};
    const char *partRenderersFields[] = {"m_renderers"};
    const char *partRenderersInitStateFields[] = {
        "m_renderersInitState"};
    const char *partHgRenderersFields[] = {"m_hgRenderers"};
    const char *partHgRenderersInitStateFields[] = {
        "m_hgRenderersInitState"};
    const char *partMeshesFields[] = {"m_meshes"};
    const char *partMeshesInitStateFields[] = {"m_meshesInitState"};
    const char *partBoneClothsFields[] = {"m_boneCloths"};
    const char *partLodGroupsFields[] = {"m_lodGroups"};
    s_basePartModelOffset = FindFieldInHierarchy(
        basePartClass, partModelFields, _countof(partModelFields), nullptr);
    s_basePartConfigOffset = FindFieldInHierarchy(
        basePartClass, partConfigFields, _countof(partConfigFields), nullptr);
    s_basePartRenderersOffset = FindFieldInHierarchy(
        basePartClass, partRenderersFields, _countof(partRenderersFields),
        nullptr);
    s_basePartRenderersInitStateOffset = FindFieldInHierarchy(
        basePartClass, partRenderersInitStateFields,
        _countof(partRenderersInitStateFields), nullptr);
    s_basePartHgRenderersOffset = FindFieldInHierarchy(
        basePartClass, partHgRenderersFields,
        _countof(partHgRenderersFields), nullptr);
    s_basePartHgRenderersInitStateOffset = FindFieldInHierarchy(
        basePartClass, partHgRenderersInitStateFields,
        _countof(partHgRenderersInitStateFields), nullptr);
    s_basePartMeshesOffset = FindFieldInHierarchy(
        basePartClass, partMeshesFields, _countof(partMeshesFields), nullptr);
    s_basePartMeshesInitStateOffset = FindFieldInHierarchy(
        basePartClass, partMeshesInitStateFields,
        _countof(partMeshesInitStateFields), nullptr);
    s_basePartBoneClothsOffset = FindFieldInHierarchy(
        basePartClass, partBoneClothsFields,
        _countof(partBoneClothsFields), nullptr);
    s_basePartLodGroupsOffset = FindFieldInHierarchy(
        basePartClass, partLodGroupsFields, _countof(partLodGroupsFields),
        nullptr);
    void *partDataClass = FindClass("Beyond.Gameplay.View",
                                    "BaseModelViewPartData", assemblies,
                                    assemblyCount);
    if (partDataClass) {
      const char *partPathFields[] = {"modelPath"};
      s_basePartConfigPathOffset = FindFieldInHierarchy(
          partDataClass, partPathFields, _countof(partPathFields), nullptr);
    }
    Log("[RES-TRACE] BaseModelViewPart fields: model=0x%X cfg=0x%X "
        "cfg.modelPath=0x%X renderers=0x%X rendererStates=0x%X "
        "hgRenderers=0x%X hgStates=0x%X meshes=0x%X meshStates=0x%X "
        "boneCloths=0x%X lodGroups=0x%X",
        s_basePartModelOffset, s_basePartConfigOffset,
        s_basePartConfigPathOffset, s_basePartRenderersOffset,
        s_basePartRenderersInitStateOffset, s_basePartHgRenderersOffset,
        s_basePartHgRenderersInitStateOffset, s_basePartMeshesOffset,
        s_basePartMeshesInitStateOffset, s_basePartBoneClothsOffset,
        s_basePartLodGroupsOffset);
    HookTraceMethod(basePartClass, "OnLoadFinish", 1,
                    "BaseModelViewPart.OnLoadFinish",
                    (void *)TraceBasePartFinish, &s_origBasePartFinish);
    HookTraceMethod(basePartClass, "PostDealLoadedModel", 0,
                    "BaseModelViewPart.PostDealLoadedModel",
                    (void *)TraceBasePartPostDeal, &s_origBasePartPostDeal);

    static const char *const loadFinishTypes[] = {
        "System.Int32", "Beyond.Resource.StringPathHash",
        "UnityEngine.GameObject"};
    void *loadFinishCallback = FindMethodWithParamTypes(
        basePartClass, "_OnLoadModelFinishCallback", loadFinishTypes,
        _countof(loadFinishTypes));
    if (loadFinishCallback &&
        Hook(loadFinishCallback, "BaseModelViewPart._OnLoadModelFinishCallback",
             (void *)TraceBasePartLoadFinishCallback,
             &s_origBasePartLoadFinishCallback))
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinishCallback path hook installed");
    else
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinishCallback hook failed/not found");

    void *loadFinishResult = FindMethodWithParamTypesAndReturnType(
        basePartClass, "_OnLoadModelFinish", loadFinishTypes,
        _countof(loadFinishTypes), "System.Boolean");
    if (loadFinishResult &&
        Hook(loadFinishResult, "BaseModelViewPart._OnLoadModelFinish",
             (void *)TraceBasePartLoadFinishResult,
             &s_origBasePartLoadFinishResult))
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinish path hook installed");
    else
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinish hook failed/not found");

    static const char *const loadUseHandleTypes[] = {
        "System.Boolean", "Beyond.Resource.FAssetProxyHandle"};
    void *loadUseHandleCallback = FindMethodWithParamTypesAndReturnType(
        basePartClass, "_OnLoadUseHandleFinishCallback", loadUseHandleTypes,
        _countof(loadUseHandleTypes), "System.Void");
    if (loadUseHandleCallback &&
        Hook(loadUseHandleCallback,
             "BaseModelViewPart._OnLoadUseHandleFinishCallback",
             (void *)TraceBasePartLoadUseHandleFinishCallback,
             &s_origBasePartLoadUseHandleFinishCallback))
      Log("[RES-TRACE] BaseModelViewPart._OnLoadUseHandleFinishCallback replacement hook installed");
    else
      Log("[RES-TRACE] BaseModelViewPart._OnLoadUseHandleFinishCallback hook failed/not found");

    void *loadUseHandleResult = FindMethodWithParamTypesAndReturnType(
        basePartClass, "_OnLoadUseHandleFinish", loadUseHandleTypes,
        _countof(loadUseHandleTypes), "System.Boolean");
    if (loadUseHandleResult &&
        Hook(loadUseHandleResult, "BaseModelViewPart._OnLoadUseHandleFinish",
             (void *)TraceBasePartLoadUseHandleFinish,
             &s_origBasePartLoadUseHandleFinishResult))
      Log("[RES-TRACE] BaseModelViewPart._OnLoadUseHandleFinish replacement hook installed");
    else
      Log("[RES-TRACE] BaseModelViewPart._OnLoadUseHandleFinish hook failed/not found");

    HookTraceMethod(basePartClass, "ReleaseModel", 0,
                    "BaseModelViewPart.ReleaseModel",
                    (void *)TraceBasePartReleaseModel,
                    &s_origBasePartReleaseModel);
    HookTraceMethod(basePartClass, "OnRelease", 0,
                    "BaseModelViewPart.OnRelease",
                    (void *)TraceBasePartOnRelease,
                    &s_origBasePartOnRelease);

    void *complexPartClass = FindClass("Beyond.Gameplay.View",
                                       "ComplexModelViewPart", assemblies,
                                       assemblyCount);
    if (complexPartClass) {
      HookTraceMethod(complexPartClass, "PostDealLoadedModel", 0,
                      "ComplexModelViewPart.PostDealLoadedModel",
                      (void *)TraceComplexPartPostDeal,
                      &s_origComplexPartPostDeal);
    } else {
      Log("[RES-TRACE] Beyond.Gameplay.View.ComplexModelViewPart class not found");
    }
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.BaseModelViewPart class not found");
  }

  void *npcAvatarCreatorUtils = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarCreatorUtils", assemblies,
      assemblyCount);

  HookTraceMethod(npcAvatarCreatorUtils, "CreateSMSGO", 12,
                  "NPCAvatarCreatorUtils.CreateSMSGO",
                  (void *)TraceCreateSmsGo, &s_origCreateSmsGo);
  HookTraceMethod(npcAvatarCreatorUtils, "CreateSMSInfoForPostModel", 9,
                  "NPCAvatarCreatorUtils.CreateSMSInfoForPostModel",
                  (void *)TraceCreateSmsPost, &s_origCreateSmsPost);
  HookTraceMethod(
      npcAvatarCreatorUtils, "<CreateMeshAssetsGo>g__AssignSkin|14_0", 4,
      "NPCAvatarCreatorUtils.CreateMeshAssetsGo.AssignSkin",
      (void *)TraceAssignSkinGo, &s_origAssignSkinGo);
  HookTraceMethod(
      npcAvatarCreatorUtils,
      "<CreateMeshAssetsInfoForPostModel>g__AssignSkin|15_0", 4,
      "NPCAvatarCreatorUtils.CreateMeshAssetsInfoForPostModel.AssignSkin",
      (void *)TraceAssignSkinPost, &s_origAssignSkinPost);
  HookTraceMethod(npcAvatarCreatorUtils, "SetSMRRootBone", 3,
                  "NPCAvatarCreatorUtils.SetSMRRootBone",
                  (void *)TraceSetSmrRootBone, &s_origSetSmrRootBone);

  // Descriptor diagnostics are intentionally installable without enabling
  // the legacy high-volume identity probe. The upstream build also installs
  // only the Mesh getter/setter pair so SubMeshInfo consumers receive the
  // same resource-level replacement as proxy consumers.
  if ((kEiemEnableDescriptorDiagnostics || kEiemEnableUpstreamMeshBoundary) &&
      !kEiemValidationIdentityProbe) {
    void *subMeshInfoClass =
        FindClass("Beyond.NPC.Avatar", "SubMeshInfo", assemblies,
                 assemblyCount);
    if (subMeshInfoClass) {
      HookTraceMethod(subMeshInfoClass, "get_mesh", 0,
                      "SubMeshInfo.get_mesh",
                      (void *)TraceV11DescriptorGetMesh,
                      &s_origSubMeshInfoGetMesh);
      HookTraceMethod(subMeshInfoClass, "set_mesh", 1,
                      "SubMeshInfo.set_mesh",
                      (void *)TraceV11DescriptorSetMesh,
                      &s_origSubMeshInfoSetMesh);
    } else {
      Log("[V1.1] SubMeshInfo class not found");
    }

    void *lodMeshAssetsClass = FindClass(
        "Beyond.NPC.Avatar", "NPCAvatarLodMeshAssets", assemblies,
        assemblyCount);
    if (lodMeshAssetsClass) {
      static const char *const getSubMeshInfoTypes[] = {
          "Beyond.NPC.Lod.ELODLevel", "System.Boolean"};
      HookTraceMethodWithParamTypes(
          lodMeshAssetsClass, "GetSubMeshInfo", getSubMeshInfoTypes, 2,
          "NPCAvatarLodMeshAssets.GetSubMeshInfo",
          (void *)TraceLodMeshAssetsGetSubMeshInfo,
          &s_origLodMeshAssetsGetSubMeshInfo);
    } else {
      Log("[V1.1] NPCAvatarLodMeshAssets class not found");
    }
    Log("[RES-TRACE] Descriptor/upstream Mesh hooks ready diagnostics=%d upstream=%d",
        kEiemEnableDescriptorDiagnostics ? 1 : 0,
        kEiemEnableUpstreamMeshBoundary ? 1 : 0);
  }

  if (kEiemValidationIdentityProbe) {
    void *bundleClass =
        FindClass("Beyond.Resource.Runtime", "Bundle", assemblies,
                  assemblyCount);
  HookTraceMethod(bundleClass, "_LoadAssetBundle", 1,
                  "Beyond.Resource.Runtime.Bundle._LoadAssetBundle",
                  (void *)TraceBundleLoadAssetBundle,
                  &s_origBundleLoadAssetBundle);
  HookTraceMethod(bundleClass, "_LoadAssetBundleAsync", 1,
                  "Beyond.Resource.Runtime.Bundle._LoadAssetBundleAsync",
                  (void *)TraceBundleLoadAssetBundleAsync,
                  &s_origBundleLoadAssetBundleAsync);
  HookTraceMethod(bundleClass, "_GetBundleFileFullPath", 1,
                  "Beyond.Resource.Runtime.Bundle._GetBundleFileFullPath",
                  (void *)TraceBundleGetFullPath, &s_origBundleGetFullPath);
  HookTraceMethod(bundleClass, "_FinishWithBundle", 1,
                  "Beyond.Resource.Runtime.Bundle._FinishWithBundle",
                  (void *)TraceBundleFinishWithBundle,
                  &s_origBundleFinishWithBundle);
  HookTraceMethod(bundleClass, "OnEndUnload", 0,
                  "Beyond.Resource.Runtime.Bundle.OnEndUnload",
                  (void *)TraceBundleOnEndUnload, &s_origBundleOnEndUnload);

  void *vfsClass =
      FindClass("Beyond.VFS", "VirtualFileSystem", assemblies, assemblyCount);
  if (vfsClass) {
    static const char *const vfsStringType[] = {"System.String"};
    HookTraceMethodWithParamTypes(
        vfsClass, "LoadBundleFromFile", vfsStringType, 1,
        "VFS.VirtualFileSystem.LoadBundleFromFile(string)",
        (void *)TraceVfsLoadBundleFromFile, &s_origVfsLoadBundleFromFile);
    HookTraceMethodWithParamTypes(
        vfsClass, "LoadBundleFromFileAsync", vfsStringType, 1,
        "VFS.VirtualFileSystem.LoadBundleFromFileAsync(string)",
        (void *)TraceVfsLoadBundleFromFileAsync,
        &s_origVfsLoadBundleFromFileAsync);
    static const char *const vfsBundlePosTypes[] = {
        "System.String", "Beyond.VFS.EFileLoaderPosType&", "System.UInt32"};
    HookTraceMethodWithParamTypes(
        vfsClass, "LoadBundleFromFile", vfsBundlePosTypes, 3,
        "VFS.VirtualFileSystem.LoadBundleFromFile(string,pos,crc)",
        (void *)TraceVfsLoadBundleFromFilePos,
        &s_origVfsLoadBundleFromFilePos);
    HookTraceMethodWithParamTypes(
        vfsClass, "LoadBundleFromFileAsync", vfsBundlePosTypes, 3,
        "VFS.VirtualFileSystem.LoadBundleFromFileAsync(string,pos,crc)",
        (void *)TraceVfsLoadBundleFromFileAsyncPos,
        &s_origVfsLoadBundleFromFileAsyncPos);
    HookTraceMethodWithParamTypes(
        vfsClass, "GetAssetStream", vfsStringType, 1,
        "VFS.VirtualFileSystem.GetAssetStream(string)",
        (void *)TraceVfsGetAssetStream, &s_origVfsGetAssetStream);
    static const char *const vfsHashType[] = {"Beyond.Resource.StringPathHash"};
    HookTraceMethodWithParamTypes(
        vfsClass, "GetAssetStream", vfsHashType, 1,
        "VFS.VirtualFileSystem.GetAssetStream(StringPathHash)",
        (void *)TraceVfsGetAssetStreamHash, &s_origVfsGetAssetStreamHash);
  } else {
    Log("[RES-TRACE] VFS.VirtualFileSystem class not found");
  }

  void *pathHashBinaryClass =
      FindClass("Beyond.Resource", "StringPathHashBinary", assemblies,
                assemblyCount);
  static const char *const hashMappingTypes[] = {"System.Int64",
                                                  "System.String&"};
  HookTraceMethodWithParamTypes(
      pathHashBinaryClass, "GetMappingStrByHash", hashMappingTypes, 2,
      "StringPathHashBinary.GetMappingStrByHash(hash,out string)",
      (void *)TraceStringPathHashGetMapping, &s_origStringPathHashGetMapping);

  void *vfsStreamClass =
      FindClass("Beyond.VFS", "VFSFileReadStream", assemblies, assemblyCount);
  static const char *const vfsReadTypes[] = {"System.Byte[]", "System.Int32",
                                               "System.Int32"};
  HookTraceMethodWithParamTypes(
      vfsStreamClass, "Read", vfsReadTypes, 3,
      "VFS.VFSFileReadStream.Read(byte[],int,int)", (void *)TraceVfsFileRead,
      &s_origVfsFileRead);
  static const char *const vfsSpanReadTypes[] = {"System.Span<System.Byte>"};
  HookTraceMethodWithParamTypes(
      vfsStreamClass, "Read", vfsSpanReadTypes, 1,
      "VFS.VFSFileReadStream.Read(Span<byte>)", (void *)TraceVfsFileReadSpan,
      &s_origVfsFileReadSpan);

  void *resourceManagerClass =
      FindClass("Beyond.Resource.Runtime", "BundleResourceManager",
               assemblies, assemblyCount);
  // Cache StringPathHash.get_path for the hash-based loader boundary.  The
  // overload receives only Int64 in the native detour, so this getter is the
  // authoritative way to recover the same logical path used by the game.
  void *stringPathHashClass =
      FindClass("Beyond.Resource", "StringPathHash", assemblies,
                assemblyCount);
  s_stringPathHashGetPath = FindMethodWithReturnType(
      stringPathHashClass, "get_path", "System.String", 0);
  if (s_stringPathHashGetPath)
    Log("[RES-TRACE] StringPathHash.get_path resolver ready");
  else
    Log("[RES-TRACE] StringPathHash.get_path resolver unavailable");
  static const char *const loadStringTypes[] = {
      "System.String", "System.Type", "Beyond.Resource.RootCategory",
      "System.Boolean", "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadSubStringTypes[] = {
      "System.String", "System.String", "System.Type",
      "Beyond.Resource.RootCategory", "System.Boolean",
      "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadHashTypes[] = {
      "Beyond.Resource.StringPathHash", "System.Type",
      "Beyond.Resource.RootCategory", "System.Boolean",
      "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadSubHashTypes[] = {
      "Beyond.Resource.StringPathHash", "System.String", "System.Type",
      "Beyond.Resource.RootCategory", "System.Boolean",
      "Beyond.Resource.EResourceRequestPriority"};
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "_LoadAssetInternal", loadStringTypes, 5,
      "BundleResourceManager._LoadAssetInternal(string)",
      (void *)TraceResourceLoadAssetInternal, &s_origResourceLoadAssetInternal);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "_LoadSubAssetInternal", loadSubStringTypes, 6,
      "BundleResourceManager._LoadSubAssetInternal(string)",
      (void *)TraceResourceLoadSubAssetInternal,
      &s_origResourceLoadSubAssetInternal);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "_LoadAssetInternal", loadHashTypes, 5,
      "BundleResourceManager._LoadAssetInternal(hash)",
      (void *)TraceResourceLoadAssetInternalHash,
      &s_origResourceLoadAssetInternalHash);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "_LoadSubAssetInternal", loadSubHashTypes, 6,
      "BundleResourceManager._LoadSubAssetInternal(hash)",
      (void *)TraceResourceLoadSubAssetInternalHash,
      &s_origResourceLoadSubAssetInternalHash);

  static const char *const loadAsyncStringCallbackTypes[] = {
      "Beyond.ELogChannel", "System.String", "System.Type",
      "Beyond.Resource.RootCategory",
      "System.Action<System.Boolean,Beyond.Resource.FAssetProxyHandle>",
      "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadSubAssetAsyncStringCallbackTypes[] = {
      "Beyond.ELogChannel", "System.String", "System.String", "System.Type",
      "Beyond.Resource.RootCategory",
      "System.Action<System.Boolean,Beyond.Resource.FAssetProxyHandle>",
      "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadAsyncHashCallbackTypes[] = {
      "Beyond.ELogChannel", "Beyond.Resource.StringPathHash", "System.Type",
      "Beyond.Resource.RootCategory",
      "System.Action<System.Boolean,Beyond.Resource.FAssetProxyHandle>",
      "Beyond.Resource.EResourceRequestPriority"};
  static const char *const loadSubAssetAsyncHashCallbackTypes[] = {
      "Beyond.ELogChannel", "Beyond.Resource.StringPathHash", "System.String",
      "System.Type", "Beyond.Resource.RootCategory",
      "System.Action<System.Boolean,Beyond.Resource.FAssetProxyHandle>",
      "Beyond.Resource.EResourceRequestPriority"};
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "LoadAsync", loadAsyncStringCallbackTypes, 6,
      "BundleResourceManager.LoadAsync(string callback)",
      (void *)TraceResourceLoadAsyncString, &s_origResourceLoadAsyncString);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "LoadSubAssetAsync",
      loadSubAssetAsyncStringCallbackTypes, 7,
      "BundleResourceManager.LoadSubAssetAsync(string callback)",
      (void *)TraceResourceLoadSubAssetAsyncString,
      &s_origResourceLoadSubAssetAsyncString);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "LoadAsync", loadAsyncHashCallbackTypes, 6,
      "BundleResourceManager.LoadAsync(hash callback)",
      (void *)TraceResourceLoadAsyncHash, &s_origResourceLoadAsyncHash);
  HookTraceMethodWithParamTypes(
      resourceManagerClass, "LoadSubAssetAsync",
      loadSubAssetAsyncHashCallbackTypes, 7,
      "BundleResourceManager.LoadSubAssetAsync(hash callback)",
      (void *)TraceResourceLoadSubAssetAsyncHash,
      &s_origResourceLoadSubAssetAsyncHash);

  static const char *const assetLoaderAsyncHashCallbackTypes[] = {
      "Beyond.Resource.StringPathHash", "System.Type",
      "System.Action<System.Boolean,Beyond.Resource.FAssetProxyLoaderHandle>",
      "Beyond.Resource.EResourceRequestPriority"};
  void *simpleAssetLoaderClass =
      FindClass("Beyond.Resource", "SimpleAssetLoader", assemblies,
                assemblyCount);
  HookTraceMethodWithParamTypes(
      simpleAssetLoaderClass, "LoadAsync", assetLoaderAsyncHashCallbackTypes,
      4, "SimpleAssetLoader.LoadAsync(hash callback)",
      (void *)TraceSimpleAssetLoaderLoadAsync,
      &s_origSimpleAssetLoaderLoadAsync);
  static const char *const assetLoaderTryLoadHashTypes[] = {
      "Beyond.Resource.StringPathHash", "System.Type",
      "Beyond.Resource.FAssetProxyLoaderHandle&"};
  HookTraceMethodWithParamTypesAndReturnType(
      simpleAssetLoaderClass, "TryLoad", assetLoaderTryLoadHashTypes, 3,
      "SimpleAssetLoader.TryLoad(hash,type,out)", "System.Boolean",
      (void *)TraceSimpleAssetLoaderTryLoad, &s_origSimpleAssetLoaderTryLoad);
  void *monoEntitySimpleAssetLoaderClass =
      FindClass("Beyond.Resource", "MonoEntitySimpleAssetLoader", assemblies,
                assemblyCount);
  HookTraceMethodWithParamTypes(
      monoEntitySimpleAssetLoaderClass, "LoadAsync",
      assetLoaderAsyncHashCallbackTypes, 4,
      "MonoEntitySimpleAssetLoader.LoadAsync(hash callback)",
      (void *)TraceMonoEntitySimpleAssetLoaderLoadAsync,
      &s_origMonoEntitySimpleAssetLoaderLoadAsync);
  HookTraceMethodWithParamTypesAndReturnType(
      monoEntitySimpleAssetLoaderClass, "TryLoad",
      assetLoaderTryLoadHashTypes, 3,
      "MonoEntitySimpleAssetLoader.TryLoad(hash,type,out)", "System.Boolean",
      (void *)TraceMonoEntitySimpleAssetLoaderTryLoad,
      &s_origMonoEntitySimpleAssetLoaderTryLoad);

  void *cachedPathAssetLoaderClass =
      FindClass("Beyond.Resource", "CachedPathAssetLoader", assemblies,
                assemblyCount);
  static const char *const cachedLoadDirectTypes[] = {
      "System.String", "System.Type"};
  HookTraceMethodWithParamTypesAndReturnType(
      cachedPathAssetLoaderClass, "LoadDirect", cachedLoadDirectTypes, 2,
      "CachedPathAssetLoader.LoadDirect(string,type)",
      "UnityEngine.Object", (void *)TraceCachedPathAssetLoaderLoadDirect,
      &s_origCachedPathAssetLoaderLoadDirect);
  static const char *const cachedTryLoadTypes[] = {
      "System.String", "System.Type",
      "Beyond.Resource.FAssetProxyLoaderHandle&"};
  HookTraceMethodWithParamTypesAndReturnType(
      cachedPathAssetLoaderClass, "TryLoad", cachedTryLoadTypes, 3,
      "CachedPathAssetLoader.TryLoad(string,type,out)", "System.Boolean",
      (void *)TraceCachedPathAssetLoaderTryLoad,
      &s_origCachedPathAssetLoaderTryLoad);

  void *hashProcessorClass =
      FindClass("Beyond.Resource", "HashStringPathProcessor", assemblies,
                assemblyCount);
  static const char *const stringType[] = {"System.String"};
  HookTraceMethodWithParamTypes(
      hashProcessorClass, "GetABStringPathHash", stringType, 1,
      "HashStringPathProcessor.GetABStringPathHash",
      (void *)TraceGetAssetPathHash, &s_origGetAssetPathHash);
  HookTraceMethodWithParamTypes(
      hashProcessorClass, "GetABStringPathHashWithoutBurst", stringType, 1,
      "HashStringPathProcessor.GetABStringPathHashWithoutBurst",
      (void *)TraceGetAssetPathHashWithoutBurst,
      &s_origGetAssetPathHashWithoutBurst);

  void *preloadManagerClass =
      FindClass("Beyond.Resource.Runtime", "PreloadManager", assemblies,
                assemblyCount);
  static const char *const int64Type[] = {"System.Int64"};
  HookTraceMethodWithParamTypes(
      preloadManagerClass, "PreloadAuto", int64Type, 1,
      "PreloadManager.PreloadAuto(hash)", (void *)TracePreloadAutoHash,
      &s_origPreloadAutoHash);

  void *proxyHandleClass =
      FindClass("Beyond.Resource", "FAssetProxyHandle", assemblies,
                assemblyCount);
  if (proxyHandleClass) {
    void *pathMethod = FindMethodWithReturnType(
        proxyHandleClass, "get_pathOrName", "System.String", 0);
    if (pathMethod && Hook(pathMethod, "FAssetProxyHandle.get_pathOrName",
                           (void *)TraceAssetProxyHandlePath,
                           &s_origAssetProxyHandlePath))
      Log("[RES-TRACE] FAssetProxyHandle.get_pathOrName observation hook installed");

    void *getMethod = FindMethodWithReturnType(
        proxyHandleClass, "Get", "UnityEngine.Object", 0);
    if (getMethod && Hook(getMethod, "FAssetProxyHandle.Get",
                          (void *)TraceAssetProxyHandleGet,
                          &s_origAssetProxyHandleGet))
      Log("[RES-TRACE] FAssetProxyHandle.Get observation hook installed");

    void *proxyMethod = FindMethodWithReturnType(
        proxyHandleClass, "GetAssetProxy", "Beyond.Resource.IAssetProxy", 0);
    if (proxyMethod && Hook(proxyMethod, "FAssetProxyHandle.GetAssetProxy",
                            (void *)TraceAssetProxyHandleGetAssetProxy,
                            &s_origAssetProxyHandleGetAssetProxy))
      Log("[RES-TRACE] FAssetProxyHandle.GetAssetProxy observation hook installed");
  } else {
    Log("[RES-TRACE] FAssetProxyHandle not found");
  }

  void *proxyLoaderHandleClass =
      FindClass("Beyond.Resource", "FAssetProxyLoaderHandle", assemblies,
                assemblyCount);
  if (proxyLoaderHandleClass) {
    void *pathMethod = FindMethodWithReturnType(
        proxyLoaderHandleClass, "get_pathOrName", "System.String", 0);
    if (pathMethod && Hook(pathMethod, "FAssetProxyLoaderHandle.get_pathOrName",
                           (void *)TraceAssetProxyLoaderHandlePath,
                           &s_origAssetProxyLoaderHandlePath))
      Log("[V1.1] FAssetProxyLoaderHandle.get_pathOrName observation hook installed");

    void *getMethod = FindMethodWithReturnType(
        proxyLoaderHandleClass, "Get", "UnityEngine.Object", 0);
    if (getMethod && Hook(getMethod, "FAssetProxyLoaderHandle.Get",
                          (void *)TraceAssetProxyLoaderHandleGet,
                          &s_origAssetProxyLoaderHandleGet))
      Log("[V1.1] FAssetProxyLoaderHandle.Get observation hook installed");

    void *loadImmediate = FindMethodWithReturnType(
        proxyLoaderHandleClass, "LoadImmediate", "System.Void", 0);
    if (loadImmediate && Hook(
            loadImmediate, "FAssetProxyLoaderHandle.LoadImmediate",
            (void *)TraceAssetProxyLoaderHandleLoadImmediate,
            &s_origAssetProxyLoaderHandleLoadImmediate))
      Log("[V1.1] FAssetProxyLoaderHandle.LoadImmediate observation hook installed");

    static const char *const addCompletedTypes[] = {
        "Beyond.ELogChannel",
        "System.Action<System.Boolean,Beyond.Resource.FAssetProxyUntrackedHandle>"};
    HookTraceMethodWithParamTypes(
        proxyLoaderHandleClass, "AddOnProxyCompleted", addCompletedTypes, 2,
        "FAssetProxyLoaderHandle.AddOnProxyCompleted",
        (void *)TraceAssetProxyLoaderHandleAddOnProxyCompleted,
        &s_origAssetProxyLoaderHandleAddOnProxyCompleted);
  } else {
    Log("[V1.1] FAssetProxyLoaderHandle not found");
  }

  void *untrackedClass =
      FindClass("Beyond.Resource", "FAssetProxyUntrackedHandle", assemblies,
                assemblyCount);
  if (untrackedClass) {
    s_origAssetProxyUntrackedPath = FindMethodWithReturnType(
        untrackedClass, "get_pathOrName", "System.String", 0);
    void *getMethod = FindMethodWithReturnType(
        untrackedClass, "Get", "UnityEngine.Object", 0);
    Log("[RES-TRACE] FAssetProxyUntrackedHandle path=%p get=%p",
        s_origAssetProxyUntrackedPath, getMethod);
    if (getMethod && Hook(getMethod, "FAssetProxyUntrackedHandle.Get",
                          (void *)TraceAssetProxyUntrackedGet,
                          &s_origAssetProxyUntrackedGet))
      Log("[RES-TRACE] FAssetProxyUntrackedHandle.Get observation hook installed");
    s_assetProxyUntrackedGetAssetProxy = FindMethodWithReturnType(
        untrackedClass, "get_assetProxy", "Beyond.Resource.IAssetProxy", 0);
  } else {
    Log("[RES-TRACE] FAssetProxyUntrackedHandle not found");
  }

  void *assetClass = FindClass("Beyond.Resource.Runtime", "Asset", assemblies,
                               assemblyCount);
  HookTraceMethod(assetClass, "get_assetName", 0,
                  "Beyond.Resource.Runtime.Asset.get_assetName",
                  (void *)TraceAssetGetAssetName, &s_origAssetGetAssetName);
  HookTraceMethod(assetClass, "_FinishWithAsset", 1,
                  "Beyond.Resource.Runtime.Asset._FinishWithAsset",
                  (void *)TraceAssetFinishWithAsset,
                  &s_origAssetFinishWithAsset);
  HookTraceMethod(assetClass, "OnComplete", 0,
                  "Beyond.Resource.Runtime.Asset.OnComplete",
                  (void *)TraceAssetOnComplete, &s_origAssetOnComplete);

  void *subMeshInfoClass =
      FindClass("Beyond.NPC.Avatar", "SubMeshInfo", assemblies, assemblyCount);
  if (subMeshInfoClass) {
    HookTraceMethod(subMeshInfoClass, "get_mesh", 0,
                    "SubMeshInfo.get_mesh",
                    (void *)TraceV11DescriptorGetMesh,
                    &s_origSubMeshInfoGetMesh);
    HookTraceMethod(subMeshInfoClass, "set_mesh", 1,
                    "SubMeshInfo.set_mesh",
                    (void *)TraceV11DescriptorSetMesh,
                    &s_origSubMeshInfoSetMesh);
  } else {
    Log("[V1.1] SubMeshInfo class not found");
  }

  void *lodMeshAssetsClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarLodMeshAssets", assemblies,
      assemblyCount);
  if (lodMeshAssetsClass) {
    static const char *const getSubMeshInfoTypes[] = {
        "Beyond.NPC.Lod.ELODLevel", "System.Boolean"};
    HookTraceMethodWithParamTypes(
        lodMeshAssetsClass, "GetSubMeshInfo", getSubMeshInfoTypes, 2,
        "NPCAvatarLodMeshAssets.GetSubMeshInfo",
        (void *)TraceLodMeshAssetsGetSubMeshInfo,
        &s_origLodMeshAssetsGetSubMeshInfo);
  } else {
    Log("[V1.1] NPCAvatarLodMeshAssets class not found");
  }

  void *meshAssetsClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarMeshAssetsSO", assemblies,
      assemblyCount);
  if (meshAssetsClass) {
    HookTraceMethod(
        meshAssetsClass, "GetAvatarSlotMeshAssets", 0,
        "NPCAvatarMeshAssetsSO.GetAvatarSlotMeshAssets",
        (void *)TraceMeshAssetsGetAvatarSlotMeshAssets,
        &s_origMeshAssetsGetAvatarSlotMeshAssets);
    HookTraceMethod(
        meshAssetsClass, "GetAllAvatarSlotMeshAssets", 0,
        "NPCAvatarMeshAssetsSO.GetAllAvatarSlotMeshAssets",
        (void *)TraceMeshAssetsGetAllAvatarSlotMeshAssets,
        &s_origMeshAssetsGetAllAvatarSlotMeshAssets);
  } else {
    Log("[V1.1] NPCAvatarMeshAssetsSO class not found");
  }

    Log("[RES-TRACE] Identity observation hooks ready");
  } else {
    Log("[RES-TRACE] Identity observation hooks disabled");
  }
}
