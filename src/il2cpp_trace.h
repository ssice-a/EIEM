#pragma once

#include <windows.h>
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
#include "eiem_resource_backend.h"
static size_t EiemManagedArrayLength(void *array);
#include "eiem_skeleton_runtime.h"

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
static volatile LONG s_smsArrayProbeLogged = 0;
static volatile LONG s_traceLogicalMeshFlowCount = 0;
static volatile LONG s_traceSubMeshSetterCount = 0;
static volatile LONG s_tracePrefabIdentityCount = 0;
static volatile LONG s_traceBonesSetterCount = 0;
static volatile LONG s_traceHgDataCount = 0;
static volatile LONG s_traceHgStateCount = 0;
static volatile LONG s_traceCharacterFlowCount = 0;
static volatile LONG s_traceMaterialCommitCount = 0;
static volatile LONG s_traceAvatarAssemblyCount = 0;

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
static void TraceRememberAssetOrigin(void *asset, int64_t pathHash,
                                     const char *path);
static bool TraceTakeBudget(volatile LONG *counter, LONG limit);
static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize);
static void TraceReadUnityObjectName(void *object, char *out, int outSize);
static void EiemPhysicsOwnerProbeObserveRenderer(void *renderer,
                                                 const char *stage);
static void EiemPhysicsOwnerProbeObserveModel(const char *ownerKind,
                                              void *owner, void *model,
                                              const char *stage);
static void EiemPhysicsOwnerProbeObserveOwnerActive(
    const char *ownerKind, void *owner, bool active, const char *stage);
static void EiemPhysicsOwnerProbeObserveRelease(
    const char *ownerKind, void *owner, void *model, const char *stage);
static void EiemReconcileModelPhysics(
    void *model, const std::vector<EiemPhysicsIntent> &intents, bool active,
    const char *stage);
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
static int s_eiemProbeSourceMaterialsOffset = -1, s_eiemProbeReplacingMaterialsOffset = -1;
static void *s_origMaterialInfoInit = nullptr;
typedef void (__fastcall *TraceSetBonesFn)(void *self, void *bones,
                                           void *methodInfo);
static void *s_origSkinnedMeshSetBones = nullptr;
typedef void(__fastcall *TraceCreateSmsGoFn)(
    void *assetLoader, void *meshAssets, int32_t lod, void *goPool,
    void *parent, void *stringList, void *intList, void **renderers,
    void **rootBones, bool flag, void *handleMap, bool deferred,
    void *methodInfo);
typedef void(__fastcall *TraceCreateSmsPostFn)(
    void *meshAssets, int32_t lod, void *goPool, void *parent,
    void *stringList, void *intList, void **renderers, void **rootBones,
    bool flag, void *methodInfo);
// The post-model helper is a compiler-generated static local function. Its
// explicit parameters are (lod, renderer array, root-bone array, closure).
// It is the last point before the game's AssignSkin code consumes the array.
typedef void(__fastcall *TraceAssignSkinPostFn)(
    int32_t lod, void *renderers, void *rootBones, void *closure,
    void *methodInfo);
static void *s_origCreateSmsGo = nullptr;
static void *s_origCreateSmsPost = nullptr;
static void *s_origAssignSkinPost = nullptr;
// NPCAvatarCreatorUtils assigns the final Animator bone palette after the
// renderer array has been created. This remains an ordering observation; the
// concrete NPC Renderer is handled at RendererInfo._Init.
typedef void(__fastcall *TraceSetSmrRootBoneFn)(
    void *animator, void *renderers, void *rootBoneInfos, void *methodInfo);
static void *s_origSetSmrRootBone = nullptr;
// Logical mesh assignment performed by Endfield's avatar data model. This is
// early construction coverage for cached paths that bypass the resource
// proxy. Live reconciliation uses the same resolver for models that become
// visible after their original construction callback was missed.
typedef void(__fastcall *TraceSubMeshInfoSetMeshFn)(void *self, void *mesh,
                                                    void *methodInfo);
static void *s_origSubMeshInfoSetMesh = nullptr;
static void *s_subMeshInfoSetMeshMethodInfo = nullptr;
typedef void *(__fastcall *TraceSubMeshInfoGetMeshFn)(void *self,
                                                      void *methodInfo);
static void *s_origSubMeshInfoGetMesh = nullptr;
static void *s_subMeshInfoGetMeshMethodInfo = nullptr;
static void *s_subMeshInfoClass = nullptr;
typedef void *(__fastcall *TraceLodGetSubMeshInfoFn)(void *self, int32_t lod,
                                                     bool includeGpu,
                                                     void *methodInfo);
static void *s_origLodGetSubMeshInfo = nullptr;
static void *s_lodGetSubMeshInfoMethodInfo = nullptr;
typedef void *(__fastcall *TraceGetPartCpuMeshFn)(void *meshAssets,
                                                   int32_t lod,
                                                   void *methodInfo);
static void *s_origGetPartCpuMesh = nullptr;
static int s_subMeshInfoMeshOffset = -1;

typedef void(__fastcall *TraceHgRendererSetDataFn)(void *self, void *data,
                                                    void *methodInfo);
typedef void *(__fastcall *TraceHgRendererGetDataFn)(void *self,
                                                      void *methodInfo);
typedef void *(__fastcall *TraceHgDataGetMeshesFn)(void *self,
                                                    void *methodInfo);
typedef void(__fastcall *TraceHgDataSetMaterialsFn)(void *self, void *materials,
                                                     void *methodInfo);
typedef void(__fastcall *TraceHgStateInitFn)(void *self, void *renderer,
                                               void *methodInfo);
typedef void(__fastcall *TraceHgStateInvalidateFn)(void *self,
                                                     void *methodInfo);
typedef void(__fastcall *TraceHgStateSetVisibleFn)(void *self, bool visible,
                                                     void *methodInfo);
static void *s_origHgRendererSetData = nullptr;
static void *s_origHgRendererGetData = nullptr;
static void *s_origHgDataGetMeshes = nullptr;
static void *s_origHgDataSetMaterials = nullptr;
static void *s_origHgStateInit = nullptr;
static void *s_origHgStateInvalidate = nullptr;
static void *s_origHgStateSetVisible = nullptr;
typedef void *(__fastcall *TraceModelManagerLoadStringFn)(void *self,
                                                            void *path,
                                                            void *methodInfo);
typedef int32_t (__fastcall *TraceModelManagerLoadAsyncStringFn)(
    void *self, void *path, void *callback, void *methodInfo);
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
typedef void (__fastcall *TraceBaseModelLoadSyncFn)(void *self,
                                                     void *methodInfo);
typedef void (__fastcall *TraceBaseModelLoadAsyncFn)(void *self,
                                                      void *callback,
                                                      void *methodInfo);
typedef void (__fastcall *TraceBaseModelFinishFn)(void *self, bool success,
                                                   void *part,
                                                   void *methodInfo);
typedef void (__fastcall *TraceBasePartFinishFn)(void *self, bool success,
                                                  void *methodInfo);
typedef void (__fastcall *TraceBasePartPostDealFn)(void *self,
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
static void *s_origModelManagerLoadString = nullptr;
static void *s_origModelManagerLoadAsyncString = nullptr;
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
static void *s_origBaseModelLoadSync = nullptr;
static void *s_origBaseModelLoadAsync = nullptr;
static void *s_origBaseModelFinish = nullptr;
static void *s_origBasePartFinish = nullptr;
static void *s_origBasePartPostDeal = nullptr;
static void *s_origComplexPartPostDeal = nullptr;
static void *s_origBasePartLoadFinishCallback = nullptr;
static void *s_origBasePartLoadFinishResult = nullptr;
static void *s_origBasePartLoadUseHandleFinishCallback = nullptr;
static void *s_origBasePartLoadUseHandleFinishResult = nullptr;
static void *s_origBasePartReleaseModel = nullptr;
static void *s_origBasePartOnRelease = nullptr;
static int s_baseModelIdOffset = -1;
static int s_baseModelPathOffset = -1;
static int s_basePartModelOffset = -1;
static int s_basePartConfigOffset = -1;
static int s_basePartConfigPathOffset = -1;
static int s_subMeshInfoMeshNameOffset = -1;
static int s_subMeshInfoPathHashOffset = -1;
static int s_lodMeshAssetNameOffset = -1;
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
static void TraceSkinnedMeshSetSharedMesh(void *self, void *mesh,
                                           void *methodInfo);
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo);
static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                          void *methodInfo);
static void *EiemReadSharedMesh(void *renderer, const char *rendererType);
static void TraceSubMeshInfoSetMesh(void *self, void *mesh, void *methodInfo);
static void *TraceLodGetSubMeshInfo(void *self, int32_t lod, bool includeGpu,
                                    void *methodInfo);
static void TraceHgRendererSetData(void *self, void *data, void *methodInfo);
static void *TraceHgRendererGetData(void *self, void *methodInfo);
static void *TraceHgDataGetMeshes(void *self, void *methodInfo);
static void TraceHgDataSetMaterials(void *self, void *materials,
                                    void *methodInfo);
static void TraceHgStateInit(void *self, void *renderer, void *methodInfo);
static void TraceHgStateInvalidate(void *self, void *methodInfo);
static void TraceHgStateSetVisible(void *self, bool visible, void *methodInfo);
static void *TraceModelManagerLoadString(void *self, void *path,
                                          void *methodInfo);
static int32_t TraceModelManagerLoadAsyncString(void *self, void *path,
                                                 void *callback,
                                                 void *methodInfo);
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
static void TraceBaseModelLoadSync(void *self, void *methodInfo);
static void TraceBaseModelLoadAsync(void *self, void *callback,
                                    void *methodInfo);
static void TraceBaseModelFinish(void *self, bool success, void *part,
                                 void *methodInfo);
static void TraceBasePartFinish(void *self, bool success, void *methodInfo);
static void TraceBasePartPostDeal(void *self, void *methodInfo);
static void TraceComplexPartPostDeal(void *self, void *methodInfo);
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
enum class EiemModelOwnerKind : uint8_t {
  PrefabProxy,
  UIModelLoader,
  BaseModelPart,
  CharUIModel,
  NpcAvatar,
};
static const char *EiemModelOwnerKindName(EiemModelOwnerKind kind) {
  switch (kind) {
    case EiemModelOwnerKind::PrefabProxy: return "PrefabProxy";
    case EiemModelOwnerKind::UIModelLoader: return "UIModelLoader";
    case EiemModelOwnerKind::BaseModelPart: return "BaseModelPart";
    case EiemModelOwnerKind::CharUIModel: return "CharUIModel";
    case EiemModelOwnerKind::NpcAvatar: return "NpcAvatar";
  }
  return "unknown";
}
static bool EiemRegisterAndApplyModelInstance(
    EiemModelOwnerKind ownerKind, void *owner, void *model,
    const char *prefabPath, uint32_t instanceUid, const char *stage);
static bool EiemRegisterBaseModelViewPartInstance(void *part,
                                                   const char *stage);
static bool EiemRegisterCharUIModelInstance(void *component,
                                             const char *stage);
static bool EiemReapplyRegisteredModelInstance(void *model,
                                                const char *stage);
static bool EiemApplyStandaloneRenderRules(void *model, const char *stage,
                                           bool *matched = nullptr,
                                           const std::vector<std::string> *affected = nullptr,
                                           std::vector<EiemPhysicsIntent> *physicsIntents = nullptr);
static bool EiemApplyStandaloneRenderRulesToRenderer(
    void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo, const char *stage);
static bool EiemBuildRelativeRendererPath(void *rootTransform, void *renderer,
                                          char *out, size_t outSize);
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
static void EiemUpdateRendererShapes(void *renderer, const char *type,
                                      const EiemModRule &rule, EiemShapeState &state) {
  if (!rule.shapeCount && state.owned.empty()) return;
  if (!EiemOnUnityThread()) { EiemShapeMessage(rule, "Shape update requires Unity thread"); return; }
  if (!EiemModEquals(type, "SkinnedMeshRenderer")) {
    EiemShapeMessage(rule, "Shape weights require SkinnedMeshRenderer"); return;
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
      EiemShapeMessage(rule,error.empty()?"Cannot capture native shape state":error);return;
    }
    if(state.binding)state.binding->initialized=true; // No mesh assignment, keep current weights.
  }
  EiemSyncShapeClaims(state,rule);
  if (!EiemApplyShapeWeights(renderer, mesh, rule, state, backend, error))
    EiemShapeMessage(rule, error);
  EiemModRule none={};EiemSyncShapeClaims(state,none);
}

struct EiemRenderOverrideState {
  // `renderer` is the component that owns the Mesh: SkinnedMeshRenderer or
  // MeshFilter. `drawRenderer` owns materials and enabled state. They are the
  // same object for skinned meshes and sibling components for static meshes.
  void *renderer = nullptr;
  void *drawRenderer = nullptr;
  void *originalMesh = nullptr;
  void *replacementMesh = nullptr;
  EiemUnityRef rendererRef, drawRendererRef, sourceMeshRef;
  std::shared_ptr<EiemSkeletonInstance> skeleton;
  bool restorePending = false;
  uint32_t originalMaterialsHandle = 0;
  uint32_t originalBonesHandle = 0;
  uint32_t replacementBonesHandle = 0;
  uint32_t originalRootBoneHandle = 0;
  bool originalEnabled = true;
  bool hasEnabled = false;
  bool hasMaterials = false;
  bool hasSkinning = false;
  bool ownsMesh = false;
  std::vector<size_t> materialSlots;
  EiemShapeState shapes;
  std::vector<EiemShapeBaseline> sourceShapeWeights;
  bool hasSourceShapeWeights = false;
  char rendererType[32] = {};
  uintptr_t ownerPrefabInstance = 0;
  char modPath[MAX_PATH] = {};
  char renderSection[96] = {};
};
struct EiemBounds {
  Vector3 center;
  Vector3 extents;
};
static SRWLOCK s_eiemOverrideLock = SRWLOCK_INIT;
static std::vector<EiemRenderOverrideState> s_eiemOverrides;

// A game controller must capture the game's materials, not our current output.
// Suppress material reapplication only for the Renderer being initialized;
// nested initialization of another model remains independent.
static thread_local std::vector<void *> s_eiemMaterialSourceInitRenderers;
static bool EiemMaterialSourceInitActive(void *renderer) {
  return std::find(s_eiemMaterialSourceInitRenderers.begin(),
                   s_eiemMaterialSourceInitRenderers.end(), renderer) !=
         s_eiemMaterialSourceInitRenderers.end();
}
struct EiemMaterialSourceInitScope {
  explicit EiemMaterialSourceInitScope(void *renderer) {
    s_eiemMaterialSourceInitRenderers.push_back(renderer);
  }
  ~EiemMaterialSourceInitScope() { s_eiemMaterialSourceInitRenderers.pop_back(); }
  EiemMaterialSourceInitScope(const EiemMaterialSourceInitScope &) = delete;
  EiemMaterialSourceInitScope &operator=(const EiemMaterialSourceInitScope &) = delete;
};

static bool EiemReadRendererEnabled(void *renderer, bool *enabled) {
  if (enabled) *enabled = true;
  if (!renderer || !enabled || !g_renderer_get_enabled) return false;
  __try {
    void *boxed = Invoke(g_renderer_get_enabled, renderer);
    if (!boxed) return false;
    *enabled = *(bool *)((char *)boxed + 16);
    return true;
  } __except (1) {
    return false;
  }
}

static int32_t EiemReadRendererMaterialCount(void *renderer) {
  if (!renderer || !g_renderer_get_sharedMaterials) return -1;
  __try {
    void *array = Invoke(g_renderer_get_sharedMaterials, renderer);
    if (!array) return 0;
    const uintptr_t count = *(uintptr_t *)((char *)array + 24);
    return count <= INT32_MAX ? (int32_t)count : -1;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return -1;
  }
}

static bool EiemReadRendererVisible(void *renderer, bool *visible) {
  if (visible) *visible = false;
  if (!renderer || !visible || !g_renderer_get_isVisible) return false;
  __try {
    void *boxed = Invoke(g_renderer_get_isVisible, renderer);
    if (!boxed) return false;
    *visible = *(bool *)((char *)boxed + 16);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return false;
  }
}

static void EiemPreserveSourceDrawState(void *renderer,
                                        const char *rendererType) {
  if (!renderer || !EiemModEquals(rendererType, "SkinnedMeshRenderer"))
    return;
  // SkinnedMeshRenderer culling uses localBounds independently of Mesh.bounds.
  // Keep the game's bounds when the replacement geometry is a subset or has a
  // different vertex distribution.
  if (g_smr_get_localBounds && g_smr_set_localBounds) {
    void *boxedBounds = Invoke(g_smr_get_localBounds, renderer);
    if (boxedBounds) {
      EiemBounds bounds = *(EiemBounds *)((char *)boxedBounds + 16);
      void *params[] = {&bounds};
      Invoke(g_smr_set_localBounds, renderer, params);
    }
  }
}

#include "eiem_residue_probe.h"

static size_t EiemFindOverrideLocked(void *renderer) {
  for (size_t index = 0; index < s_eiemOverrides.size(); ++index)
    if (s_eiemOverrides[index].renderer == renderer &&
        s_eiemOverrides[index].rendererRef.Target() == renderer) return index;
  return SIZE_MAX;
}

static void EiemPrepareRenderInput(void *renderer, void *mesh,
                                   const char *rendererType,
                                   void **identityMesh) {
  if (identityMesh) *identityMesh = mesh;
  if (!renderer || !mesh) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index = EiemFindOverrideLocked(renderer);
  // Do not create an entry while merely observing an unrelated Renderer.
  // Entries are created only after a rule has matched and a mutation is about
  // to happen, so reload restores exactly the objects we changed.
  if (index != SIZE_MAX) {
    EiemRenderOverrideState &state = s_eiemOverrides[index];
    if (state.restorePending) {
      if (identityMesh) *identityMesh = state.originalMesh;
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      return;
    }
    if (state.replacementMesh && state.replacementMesh != mesh) {
      auto sourceRef = EiemUnityRef::Capture(mesh, false);
      if (!sourceRef) {
        ReleaseSRWLockExclusive(&s_eiemOverrideLock);
        Log("[MOD] Cannot retain changed source Mesh renderer=%p", renderer);
        return;
      }
      if (state.hasSourceShapeWeights) {
        EiemUnityShapes shapes;std::vector<EiemShapeBaseline> baseline;
        if (!shapes.Snapshot(renderer,mesh,baseline)) {
          ReleaseSRWLockExclusive(&s_eiemOverrideLock);
          Log("[SHAPE] Cannot capture changed game Mesh layout renderer=%p",renderer);
          return;
        }
        // A game-assigned Mesh is a new source, not the old replacement's
        // channels. Never carry its control claims or numeric mapping across.
        EiemRetireShapeBinding(state.shapes.binding);state.shapes={};
        state.sourceShapeWeights=std::move(baseline);
      }
      state.sourceMeshRef = std::move(sourceRef);
      state.originalMesh = mesh;
      state.replacementMesh = nullptr;
    }
    if (identityMesh && state.replacementMesh == mesh)
      *identityMesh = state.originalMesh;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static bool EiemCaptureOriginal(void *renderer, void *drawRenderer, void *mesh,
                                const char *rendererType, bool meshEdit = false,
                                const EiemModRule *materialRule = nullptr, bool shapeEdit = false) {
  if (!renderer || !mesh) return false;
  if (!drawRenderer) drawRenderer = renderer;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index = EiemFindOverrideLocked(renderer);
  if (index == SIZE_MAX) {
    EiemRenderOverrideState state = {};
    state.rendererRef = EiemUnityRef::Capture(renderer);
    state.drawRendererRef = drawRenderer == renderer ? state.rendererRef : EiemUnityRef::Capture(drawRenderer);
    if (!state.rendererRef || !state.drawRendererRef) {
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      Log("[MOD] Cannot observe Renderer lifetime; refusing mutation renderer=%p", renderer);
      return false;
    }
    state.renderer = renderer;
    state.drawRenderer = drawRenderer;
    state.originalMesh = mesh;
    strncpy_s(state.rendererType, sizeof(state.rendererType),
              rendererType ? rendererType : "Renderer", _TRUNCATE);
    state.ownerPrefabInstance = s_eiemActivePrefabInstance;
    index = s_eiemOverrides.size();
    s_eiemOverrides.push_back(state);
  }
  auto &state = s_eiemOverrides[index];
  if (state.restorePending) {
    ReleaseSRWLockExclusive(&s_eiemOverrideLock);
    Log("[MOD] Restore incomplete; refusing new mutation renderer=%p", renderer);
    return false;
  }
  if (!state.ownsMesh) state.originalMesh = mesh;
  if (meshEdit && !state.sourceMeshRef) {
    state.sourceMeshRef = EiemUnityRef::Capture(state.originalMesh, false);
    if (!state.sourceMeshRef) {
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      Log("[MOD] Cannot retain source Mesh; refusing assignment renderer=%p", renderer);
      return false;
    }
  }
  if ((meshEdit || shapeEdit) && !state.ownsMesh && !state.hasSourceShapeWeights &&
      EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    EiemUnityShapes shapes;
    if (!shapes.Snapshot(renderer, mesh, state.sourceShapeWeights)) {
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      Log("[SHAPE] Cannot capture source shape weights before mesh replacement renderer=%p", renderer);
      return false;
    }
    state.hasSourceShapeWeights = true;
  }
  if (materialRule && (materialRule->materialCount || materialRule->submeshCount)) {
    if (!state.hasMaterials && g_renderer_get_sharedMaterials && il2cpp_gchandle_new) {
      void *materials = Invoke(g_renderer_get_sharedMaterials, drawRenderer);
      if (materials) {
        state.originalMaterialsHandle = il2cpp_gchandle_new(materials, false);
        state.hasMaterials = state.originalMaterialsHandle != 0;
      }
    }
    if (!state.hasMaterials) {
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      Log("[MOD] Cannot capture source material slots; renderer=%p", renderer);
      return false;
    }
    auto ownSlot = [&](size_t slot) {
      if (std::find(state.materialSlots.begin(), state.materialSlots.end(), slot) == state.materialSlots.end())
        state.materialSlots.push_back(slot);
    };
    for (uint32_t i = 0; i < materialRule->materialCount; ++i) ownSlot(materialRule->materialSlots[i]);
    for (uint32_t i = 0; i < materialRule->submeshCount; ++i)
      if (materialRule->submeshSlots[i] >= 0) ownSlot(i);
    // Array padding between the old end and an added slot belongs to us too.
    void *baseline = il2cpp_gchandle_get_target(state.originalMaterialsHandle);
    const size_t baselineCount = EiemManagedArrayLength(baseline);
    size_t required = baselineCount;
    for (size_t slot : state.materialSlots) required = (std::max)(required, slot + 1);
    for (size_t slot = baselineCount; slot < required; ++slot) ownSlot(slot);
  }
  // Mesh assignment may cause game skin setup to change its palette. Materials
  // and skip never own that palette and must not capture/restore it.
  if (meshEdit && !state.hasSkinning &&
      EiemModEquals(rendererType, "SkinnedMeshRenderer") && il2cpp_gchandle_new) {
    if (g_smr_get_bones) {
      void *bones = Invoke(g_smr_get_bones, renderer);
      if (bones) state.originalBonesHandle = il2cpp_gchandle_new(bones, false);
    }
    if (g_smr_get_rootBone) {
      void *rootBone = Invoke(g_smr_get_rootBone, renderer);
      if (rootBone) state.originalRootBoneHandle = il2cpp_gchandle_new(rootBone, false);
    }
    state.hasSkinning = state.originalBonesHandle != 0 || state.originalRootBoneHandle != 0;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  return true;
}

static void EiemRememberRuleBinding(void *renderer,
                                    const EiemModRule &rule) {
  if (!renderer) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    auto &state = s_eiemOverrides[index];
    // A later resource-assembly refresh has no completed Prefab owner. Do not
    // erase an owner learned by the normal Prefab lifecycle adapter.
    if (s_eiemActivePrefabInstance)
      state.ownerPrefabInstance = s_eiemActivePrefabInstance;
    strncpy_s(state.modPath, sizeof(state.modPath), rule.modPath, _TRUNCATE);
    strncpy_s(state.renderSection, sizeof(state.renderSection), rule.section,
              _TRUNCATE);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static bool EiemFindBoundRenderRule(void *renderer, EiemModRule *out) {
  if (!renderer || !out) return false;
  char modPath[MAX_PATH] = {};
  char section[96] = {};
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX && !s_eiemOverrides[index].restorePending) {
    strncpy_s(modPath, sizeof(modPath), s_eiemOverrides[index].modPath,
              _TRUNCATE);
    strncpy_s(section, sizeof(section),
              s_eiemOverrides[index].renderSection, _TRUNCATE);
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  return modPath[0] && section[0] &&
         EiemFindRenderRuleBySection(modPath, section, out);
}

static void EiemBeginMeshWrite(void *renderer) {
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) s_eiemOverrides[index].ownsMesh = true;
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static bool EiemPrepareRendererShapeBinding(void *renderer,void *target,const char *type,char *error,size_t size) {
  if(!EiemModEquals(type,"SkinnedMeshRenderer"))return true;
  bool ok=false;std::string reason="Original shape baseline unavailable";
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index=EiemFindOverrideLocked(renderer);
  if(index!=SIZE_MAX && s_eiemOverrides[index].hasSourceShapeWeights) {
    auto &state=s_eiemOverrides[index];EiemUnityShapes backend;
    ok=EiemPrepareShapeBinding(state.shapes,renderer,target,state.sourceShapeWeights,backend,reason);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  if(!ok && error)strncpy_s(error,size,reason.c_str(),_TRUNCATE);return ok;
}
static bool EiemInitializeRendererShapeBinding(void *renderer,const char *type,char *error,size_t size) {
  if(!EiemModEquals(type,"SkinnedMeshRenderer"))return true;
  bool ok=false;std::string reason="Shape state unavailable";
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index=EiemFindOverrideLocked(renderer);
  if(index!=SIZE_MAX) {EiemUnityShapes backend;ok=EiemInitializeBoundShapes(s_eiemOverrides[index].shapes,renderer,backend,reason);}
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  if(!ok && error)strncpy_s(error,size,reason.c_str(),_TRUNCATE);return ok;
}

static void EiemRememberReplacement(void *renderer, void *replacementMesh,
                                    const char *rendererType) {
  if (!renderer) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX) {
    s_eiemOverrides[index].replacementMesh = replacementMesh;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

// Once a Renderer has been bound to a generated resource, the game may
// re-assign its original Mesh during LOD/skin refreshes.  Keep the replacement
// attached for those source-only writes.  Reload restoration sets
// s_eiemApplyingModMeshAssignment and bypasses this guard explicitly.
static void *EiemReplacementForSourceMesh(void *renderer, void *mesh) {
  if (!renderer || !mesh) return nullptr;
  void *replacement = nullptr;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX && !s_eiemOverrides[index].restorePending &&
      s_eiemOverrides[index].originalMesh == mesh)
    replacement = s_eiemOverrides[index].replacementMesh;
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  return replacement;
}

// A Mesh/material override must never own Renderer.enabled. Only an explicit
// handling=skip directive changes it, and therefore only skip needs a value
// restored on reload. LOD setup is free to toggle enabled while a mesh-only
// rule is active.
static void EiemCaptureEnabledForSkip(void *renderer, void *drawRenderer) {
  bool enabled = true;
  if (!renderer || !drawRenderer ||
      !EiemReadRendererEnabled(drawRenderer, &enabled))
    return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX && !s_eiemOverrides[index].hasEnabled) {
    s_eiemOverrides[index].originalEnabled = enabled;
    s_eiemOverrides[index].hasEnabled = true;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static bool EiemSetRendererEnabled(void *renderer, bool enabled) {
  if (!renderer || !g_renderer_set_enabled) return false;
  void *params[] = {&enabled};
  __try {
    Invoke(g_renderer_set_enabled, renderer, params);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[MOD] Renderer.enabled assignment failed: renderer=%p exception=0x%08lX",
        renderer, GetExceptionCode());
    return false;
  }
}

static void EiemReleaseOverrideHandles(const EiemRenderOverrideState &state) {
  EiemRetireShapeBinding(state.shapes.binding);
  if (state.originalMaterialsHandle && il2cpp_gchandle_free)
    il2cpp_gchandle_free(state.originalMaterialsHandle);
  if (state.originalBonesHandle && il2cpp_gchandle_free)
    il2cpp_gchandle_free(state.originalBonesHandle);
  if (state.replacementBonesHandle && il2cpp_gchandle_free)
    il2cpp_gchandle_free(state.replacementBonesHandle);
  if (state.originalRootBoneHandle && il2cpp_gchandle_free)
    il2cpp_gchandle_free(state.originalRootBoneHandle);
}

static void EiemRestoreRenderOverrides(const std::vector<std::string> *affected = nullptr) {
  std::vector<EiemRenderOverrideState> states;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  for (auto &state : s_eiemOverrides) {
    if (!EiemModAffected(state.modPath, affected)) continue;
    state.restorePending = true; // stop re-entrant commits from reinstating old rules
    states.push_back(state);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  size_t completed = 0;
  for (auto &state : states) {
    auto finish = [&](bool success) {
      bool removed = false;
      AcquireSRWLockExclusive(&s_eiemOverrideLock);
      for (size_t i = 0; i < s_eiemOverrides.size(); ++i) {
        if (s_eiemOverrides[i].renderer != state.renderer) continue;
        if (success) { s_eiemOverrides.erase(s_eiemOverrides.begin() + i); removed = true; }
        else s_eiemOverrides[i].shapes = state.shapes;
        break;
      }
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
      if (removed) { EiemReleaseOverrideHandles(state); ++completed; }
      if (!success) Log("[MOD] Restore incomplete; baseline retained renderer=%p mod=%s", state.renderer, state.modPath);
    };
    const int rendererStatus = state.rendererRef.Status();
    if (rendererStatus != 1) { finish(rendererStatus == 0); continue; }
    bool restoredAll = true;
    EiemProbeTrackedRenderer("restore-before", state.renderer);
    Log("[DEBUG-residue-v39] restore-plan renderer=%p original=%p replacement=%p ownsMesh=%d ownsMaterials=%d ownsEnabled=%d owner=%p",
        state.renderer, state.originalMesh, state.replacementMesh, state.ownsMesh,
        state.hasMaterials, state.hasEnabled, (void *)state.ownerPrefabInstance);
    EiemModRule noShapes = {};
    EiemUpdateRendererShapes(state.renderer, state.rendererType, noShapes, state.shapes);
    if (!state.shapes.owned.empty()) restoredAll = false;
    Log("[DEBUG-hr1] restore renderer=%p original=%p replacement=%p restoreEnabled=%d originalEnabled=%d",
        state.renderer, state.originalMesh, state.replacementMesh,
        state.hasEnabled ? 1 : 0,
        state.originalEnabled ? 1 : 0);
    const bool sourceAlive = !state.ownsMesh ||
        (state.sourceMeshRef.Target() == state.originalMesh && state.sourceMeshRef.Status() == 1);
    if (!sourceAlive) restoredAll = false;
    if (state.ownsMesh && sourceAlive &&
        EiemReadSharedMesh(state.renderer, state.rendererType) != state.originalMesh)
      EiemSetSharedMesh(state.renderer, state.originalMesh, state.rendererType, nullptr);
    if (state.ownsMesh && state.renderer) {
      void *restored = EiemReadSharedMesh(state.renderer, state.rendererType);
      if (restored != state.originalMesh) restoredAll = false;
      if (sourceAlive && restored == state.originalMesh && state.hasSourceShapeWeights) {
        EiemUnityShapes shapes;
        for (const auto &entry : EiemShapeSourceWeights(state.shapes,state.sourceShapeWeights)) {
          const int index = shapes.Index(restored, entry.name);
          float actual=0;
          if (index < 0 || !shapes.Write(state.renderer, index, entry.value) ||
              !shapes.Read(state.renderer,index,actual) || std::abs(actual-entry.value)>.0001f) {
            restoredAll = false;
            Log("[SHAPE] Cannot restore source channel renderer=%p name=%s", state.renderer, entry.name.c_str());
          }
        }
      }
      Log("[DEBUG-hr1] restore result renderer=%p actual=%p expected=%p ok=%d",
          state.renderer, restored, state.originalMesh,
          restored == state.originalMesh ? 1 : 0);
    }
    const int drawStatus = state.drawRendererRef.Status();
    if (drawStatus < 0 && (state.hasMaterials || state.hasEnabled)) restoredAll = false;
    if (drawStatus == 1 && state.hasMaterials &&
        state.originalMaterialsHandle &&
        s_eiemRendererSetSharedMaterials && il2cpp_gchandle_get_target) {
      void *original = il2cpp_gchandle_get_target(state.originalMaterialsHandle);
      void *current = g_renderer_get_sharedMaterials ? Invoke(g_renderer_get_sharedMaterials, state.drawRenderer) : nullptr;
      if (original && current && s_eiemMaterialClass) {
        const size_t originalCount = EiemManagedArrayLength(original), currentCount = EiemManagedArrayLength(current);
        void **originalItems = (void **)((char *)original + IL2CPP_ARRAY_DATA);
        void **currentItems = (void **)((char *)current + IL2CPP_ARRAY_DATA);
        std::vector<void *> live(currentItems, currentItems + currentCount);
        std::vector<void *> baseline(originalItems, originalItems + originalCount);
        auto restored = EiemRestoreOwnedSlots(live, baseline, state.materialSlots);
        bool valid = true;
        for (size_t slot : state.materialSlots)
          if (slot < restored.size() && restored[slot] && EiemNativeObjectStatus(restored[slot]) != 1) valid = false;
        if (!valid) restoredAll = false;
        if (valid && restored != live) {
          void *array = il2cpp_array_new(s_eiemMaterialClass, restored.size());
          if (array) {
            if (!restored.empty()) memcpy((char *)array + IL2CPP_ARRAY_DATA, restored.data(), restored.size() * sizeof(void *));
            void *params[] = {array};
            Invoke(s_eiemRendererSetSharedMaterials, state.drawRenderer, params);
            if (!EiemManagedObjectArraySame(array, Invoke(g_renderer_get_sharedMaterials, state.drawRenderer))) restoredAll = false;
          } else { restoredAll = false; Log("[MOD] Cannot allocate restored material slots renderer=%p", state.drawRenderer); }
        }
      } else restoredAll = false;
    } else if (drawStatus == 1 && state.hasMaterials) restoredAll = false;
    if (drawStatus == 1 && state.hasEnabled && g_renderer_set_enabled) {
      bool enabled = state.originalEnabled;
      void *params[] = {&enabled};
      Invoke(g_renderer_set_enabled, state.drawRenderer, params);
      bool actual = !enabled;
      if (!EiemReadRendererEnabled(state.drawRenderer, &actual) || actual != enabled) restoredAll = false;
    } else if (drawStatus == 1 && state.hasEnabled) restoredAll = false;
    if (sourceAlive && state.ownsMesh &&
        EiemReadSharedMesh(state.renderer, state.rendererType) == state.originalMesh && state.hasSkinning &&
        il2cpp_gchandle_get_target) {
      if (state.originalBonesHandle) {
        void *bones = il2cpp_gchandle_get_target(state.originalBonesHandle);
        if (!bones || !g_smr_get_bones || !g_smr_set_bones) restoredAll = false;
        else if (!EiemManagedObjectArraySame(bones, Invoke(g_smr_get_bones, state.renderer))) {
          void *params[] = {bones};
          Invoke(g_smr_set_bones, state.renderer, params);
          if (!EiemManagedObjectArraySame(bones, Invoke(g_smr_get_bones, state.renderer))) restoredAll = false;
        }
      }
      if (state.originalRootBoneHandle) {
        void *rootBone = il2cpp_gchandle_get_target(state.originalRootBoneHandle);
        if (!rootBone || !g_smr_get_rootBone || !g_smr_set_rootBone || EiemNativeObjectStatus(rootBone) != 1) restoredAll = false;
        else if (rootBone != Invoke(g_smr_get_rootBone, state.renderer)) {
          void *params[] = {rootBone};
          Invoke(g_smr_set_rootBone, state.renderer, params);
          if (rootBone != Invoke(g_smr_get_rootBone, state.renderer)) restoredAll = false;
        }
      }
    }
    EiemProbeTrackedRenderer("restore-after", state.renderer);
    finish(restoredAll);
  }
  if (!states.empty())
    Log("[MOD] Restore complete: released=%zu pending=%zu", completed, states.size() - completed);
}

// The game is about to unload this Prefab. Its Renderer objects must not be
// called while restoring: their native side may already be entering teardown.
// Drop only EIEM bookkeeping and managed handles owned by this instance.
static void EiemForgetRenderOverrides(uintptr_t ownerPrefabInstance) {
  if (!ownerPrefabInstance) return;
  std::vector<EiemRenderOverrideState> forgotten;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  for (size_t index = 0; index < s_eiemOverrides.size();) {
    if (s_eiemOverrides[index].ownerPrefabInstance != ownerPrefabInstance) {
      ++index;
      continue;
    }
    forgotten.push_back(s_eiemOverrides[index]);
    s_eiemOverrides.erase(s_eiemOverrides.begin() + index);
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  for (const auto &state : forgotten) {
    // No extra dereference of objects already entering teardown.
    // An owner can release a pooled or deferred-destroy Renderer while its
    // native bones array still references our nodes. Preserve that dependency
    // without calling any native method from this teardown callback.
    if (state.skeleton && state.ownsMesh)
      EiemRetainSkeletonConsumer(*state.skeleton,state.rendererRef);
    Log("[DEBUG-residue-v39] forget owner=%p renderer=%p original=%p replacement=%p ownsMesh=%d",
        (void *)ownerPrefabInstance, state.renderer, state.originalMesh, state.replacementMesh, state.ownsMesh);
    EiemReleaseOverrideHandles(state);
  }
  if (!forgotten.empty())
    Log("[MOD-PREFAB] forgot %zu Renderer override(s) for instance=%p",
        forgotten.size(), (void *)ownerPrefabInstance);
}

struct EiemResolvedRenderRule {
  EiemModRule rule = {};
  char source[768] = {};
  char asset[192] = {};
};

struct EiemPartnerState {
  std::shared_ptr<EiemSkeletonInstance> skeleton;
  void *sourceRenderer = nullptr;
  void *sourceDrawRenderer = nullptr;
  void *partnerObject = nullptr;
  void *partnerRenderer = nullptr;
  LONG generation = -1;
  uintptr_t ownerPrefabInstance = 0;
  char section[96] = {};
  char modPath[MAX_PATH] = {};
  EiemShapeState shapes;
  char rendererType[32] = {};
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
  if (!EiemOnUnityThread() || !g_smr_get_bones || !g_transform_get_parent ||
      !g_object_get_name || !il2cpp_array_new || !g_transformClass)
    return reject("Shared skeleton APIs are unavailable");
  void *current = EiemBackendInvokeNoThrow(g_smr_get_bones, renderer);
  const size_t count = EiemManagedArrayLength(current);
  if (!current || !count || count > 4096) return reject("Source renderer has no valid bone palette");
  void **items = (void **)((char *)current + IL2CPP_ARRAY_DATA);
  std::vector<void *> resolved;
  if (identity.paths.empty()) {
    // Pathless source packages retain hash identity. No count equality or
    // index-order guess: each slot must resolve uniquely within this palette.
    for (uint32_t hash : identity.hashes) {
      void *found = nullptr;
      for (size_t i=0; i<count; ++i) {
        std::vector<uint32_t> hashes; EiemCollectBonePathHashes(items[i], &hashes);
        if (std::find(hashes.begin(), hashes.end(), hash) == hashes.end()) continue;
        if (found && found != items[i]) return reject("Ambiguous source bone hash");
        found=items[i];
      }
      if (!found) return reject("Source bone hash not found; re-export with bone paths");
      resolved.push_back(found);
    }
  } else {
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
    if (!EiemSkinRootPath(sourcePaths,identity.paths,root,why)) return reject(why);
    auto ancestor=ancestors.find(root);
    if (ancestor==ancestors.end()) return reject("Shared skeleton root is absent");
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
    std::vector<size_t> slots;
    if (!EiemResolveSkinPathIndices(identity.paths,paths,slots,why)) return reject(why);
    for (size_t index:slots) resolved.push_back(nodes[index]);
  }
  if (resolved.empty()) return reject("Replacement mesh has no resolved bones");
  bool same=count==resolved.size();
  for (size_t i=0; same && i<count; ++i) same=items[i]==resolved[i];
  if (same) { if (out) *out=current; return true; }
  void *array=il2cpp_array_new(g_transformClass,resolved.size());
  if (!array) return reject("Cannot allocate expanded bone palette");
  memcpy((char *)array+IL2CPP_ARRAY_DATA,resolved.data(),resolved.size()*sizeof(void *));
  if (out) *out=array;
  return true;
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

// A later game skin refresh must not shrink an extended palette back to the
// source slots. Reassert only this instance's owned replacement binding.
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo) {
  auto original = (TraceSetBonesFn)s_origSkinnedMeshSetBones;
  if (original) original(self, bones, methodInfo);
  if (!self || s_eiemApplyingModMeshAssignment) {
    return;
  }
  bool tracked = false;
  uint32_t binding = 0;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(self);
  if (index != SIZE_MAX && !s_eiemOverrides[index].restorePending && s_eiemOverrides[index].replacementMesh) {
    tracked = true;
    binding = s_eiemOverrides[index].replacementBonesHandle;
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (tracked && binding && EiemOnUnityThread() && il2cpp_gchandle_get_target) {
    void *expected = il2cpp_gchandle_get_target(binding);
    if (expected && !EiemManagedObjectArraySame(bones, expected)) {
      char error[256] = {};
      if (!EiemPreserveSourceSkinning(self, expected, error, sizeof(error)))
        Log("[MOD-SKIN] game refresh binding failed renderer=%p error=%s", self, error);
    }
  }
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

static bool EiemSetPartnerLodMembership(void *sourceRenderer,
                                        void *partnerRenderer,
                                        bool add) {
  if (!sourceRenderer || !partnerRenderer || !g_lodGroup_get_lods ||
      !g_lodGroup_set_lods || !il2cpp_array_new_specific ||
      !il2cpp_object_get_class)
    return false;
  __try {
    void *group = EiemFindSourceLodGroup(sourceRenderer);
    if (!group) return false;
    void *lods = Invoke(g_lodGroup_get_lods, group);
    const size_t lodCount = EiemManagedArrayLength(lods);
    if (!lods || lodCount == 0 || lodCount > 64) return false;
    bool changed = false;
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
      if (add) {
        if (sourceIndex == SIZE_MAX || partnerIndex != SIZE_MAX) continue;
        void *arrayClass = il2cpp_object_get_class(renderers);
        if (!arrayClass) continue;
        void *replacement = il2cpp_array_new_specific(arrayClass, rendererCount + 1);
        if (!replacement) continue;
        void **outItems = (void **)((char *)replacement + 32);
        memcpy(outItems, items, rendererCount * sizeof(void *));
        outItems[rendererCount] = partnerRenderer;
        lod->renderers = replacement;
        changed = true;
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
    if (!changed) return false;
    void *params[] = {lods};
    Invoke(g_lodGroup_set_lods, group, params);
    Log("[MOD] %s partner Renderer %p in LODGroup %p (%zu LOD level(s))",
        add ? "Added" : "Removed", partnerRenderer, group, lodCount);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[MOD] LODGroup %s failed with exception=0x%08lX",
        add ? "insert" : "remove", GetExceptionCode());
    return false;
  }
}

static void EiemRetirePartner(const EiemPartnerState &state) {
  EiemRetireShapeBinding(state.shapes.binding);
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
                                       const EiemModRule &partnerRule,
                                       void *sourceMesh, char *error,
                                       size_t errorSize) {
  if (!sourceMeshOwner || !sourceDrawRenderer || !rendererType ||
      !g_gameObjectClass ||
      (!g_gameObject_ctor && !g_gameObject_ctorDefault) ||
      !g_gameObject_AddComponent || !g_component_get_gameObject ||
      !g_component_get_transform || !g_transform_set_parent || !il2cpp_class_get_type ||
      !il2cpp_type_get_object) {
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
  if (g_gameObject_get_activeSelf && g_gameObject_set_active) {
    void *boxedActive = Invoke(g_gameObject_get_activeSelf, sourceGo);
    if (boxedActive) {
      bool active = *(bool *)((char *)boxedActive + 16);
      void *activeParams[] = {&active};
      Invoke(g_gameObject_set_active, partnerGo, activeParams);
    }
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
    if (g_smr_get_localBounds && g_smr_set_localBounds) {
      // SkinnedMeshRenderer culling uses localBounds, which is independent of
      // Mesh.bounds. Copy the source bounds so the replacement is not culled
      // before its first skinning update.
      void *boxedBounds = Invoke(g_smr_get_localBounds, sourceMeshOwner);
      if (boxedBounds) {
        EiemBounds bounds = *(EiemBounds *)((char *)boxedBounds + 16);
        void *boundsParams[] = {&bounds};
        Invoke(g_smr_set_localBounds, partnerMeshOwner, boundsParams);
      }
    }
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
  bool enabled = !EiemModEquals(partnerRule.handling, "skip");
  if (g_renderer_set_enabled) {
    void *params[] = {&enabled};
    Invoke(g_renderer_set_enabled, partnerDrawRenderer, params);
  }
  EiemSetPartnerLodMembership(sourceDrawRenderer, partnerDrawRenderer, true);

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

  EiemPartnerState state = {};
  state.skeleton=std::move(partnerSkeleton);
  state.sourceRenderer = sourceMeshOwner;
  state.sourceDrawRenderer = sourceDrawRenderer;
  state.partnerObject = partnerGo;
  state.partnerRenderer = partnerDrawRenderer;
  state.shapes=std::move(partnerShapes);
  strncpy_s(state.rendererType, sizeof(state.rendererType), rendererType, _TRUNCATE);
  EiemUpdateRendererShapes(partnerDrawRenderer, rendererType, partnerRule, state.shapes);
  state.generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  state.ownerPrefabInstance = s_eiemActivePrefabInstance;
  strncpy_s(state.section, sizeof(state.section), partnerRule.section, _TRUNCATE);
  strncpy_s(state.modPath, sizeof(state.modPath), partnerRule.modPath, _TRUNCATE);
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  s_eiemPartners.push_back(state);
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  Log("[MOD] partner Renderer created: source=%p section=%s renderer=%p mesh=%s",
      sourceMeshOwner, partnerRule.section, partnerDrawRenderer,
      partnerRule.hasMesh ? partnerRule.mesh : "<source>");
  return partnerDrawRenderer;
}

static void EiemApplyPartners(void *sourceMeshOwner,
                              void *sourceDrawRenderer, void *sourceMesh,
                              const char *rendererType,
                              const EiemModRule &sourceRule) {
  if (!sourceMeshOwner || !sourceDrawRenderer || !sourceRule.partnerCount ||
      s_eiemCreatingPartner)
    return;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  for (uint32_t index = 0; index < sourceRule.partnerCount; ++index) {
    const char *section = sourceRule.partners[index];
    if (!section[0]) continue;
    AcquireSRWLockShared(&s_eiemPartnerLock);
    const bool exists = EiemFindPartnerLocked(sourceMeshOwner, sourceRule.modPath, section,
                                               generation) != SIZE_MAX;
    ReleaseSRWLockShared(&s_eiemPartnerLock);
    if (exists) continue;
    EiemModRule partner = {};
    EiemModInitRule(&partner);
    if (!EiemFindRenderRuleBySection(sourceRule.modPath, section, &partner)) {
      Log("[MOD] partner section not found: source=%s partner=%s", sourceRule.section, section);
      continue;
    }
    char error[256] = {};
    s_eiemCreatingPartner = true;
    EiemCreatePartnerRenderer(sourceMeshOwner, sourceDrawRenderer,
                              rendererType, partner, sourceMesh, error,
                              sizeof(error));
    s_eiemCreatingPartner = false;
    if (error[0])
      Log("[MOD] partner creation failed: source=%s partner=%s error=%s",
          sourceRule.section, section, error);
  }
}

static void *TraceSubMeshInfoGetMesh(void *self, void *methodInfo) {
  auto original = (TraceSubMeshInfoGetMeshFn)s_origSubMeshInfoGetMesh;
  return original ? original(self, methodInfo) : nullptr;
}

static void TraceSubMeshInfoSetMesh(void *self, void *mesh, void *methodInfo) {
  auto original = (TraceSubMeshInfoSetMeshFn)s_origSubMeshInfoSetMesh;
  if (!original) return;
  // IL2CPP shares the native setter thunk between many unrelated reference
  // properties. The MethodInfo/object guard is mandatory; without it every
  // setter in the process would be misread as SubMeshInfo.mesh.
  bool isSubMeshInfo = false;
  if (s_subMeshInfoClass && self && il2cpp_object_get_class) {
    __try { isSubMeshInfo = il2cpp_object_get_class(self) == s_subMeshInfoClass; }
    __except (EXCEPTION_EXECUTE_HANDLER) { isSubMeshInfo = false; }
  }
  if (!isSubMeshInfo && methodInfo && s_subMeshInfoSetMeshMethodInfo)
    isSubMeshInfo = methodInfo == s_subMeshInfoSetMeshMethodInfo;
  if (!isSubMeshInfo) {
    original(self, mesh, methodInfo);
    return;
  }
  if (!self || !mesh) {
    original(self, mesh, methodInfo);
    return;
  }

  // SubMeshInfo carries the logical asset name/path hash even when the Unity
  // Mesh itself is an embedded object with no useful origin record. Read only
  // fields resolved from metadata; offsets are never hard-coded in the hook.
  char assetName[192] = {};
  if (s_subMeshInfoMeshNameOffset >= 0) {
    __try {
      void *name = *(void **)((char *)self + s_subMeshInfoMeshNameOffset);
      if (name) ReadStrUtf8(name, assetName, sizeof(assetName));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      assetName[0] = '\0';
    }
  }
  if (!assetName[0]) TraceReadUnityObjectName(mesh, assetName, sizeof(assetName));

  if (TraceTakeBudget(&s_traceSubMeshSetterCount, 240)) {
    int64_t pathHash = 0;
    if (self && s_subMeshInfoPathHashOffset >= 0) {
      __try { pathHash = *(int64_t *)((char *)self + s_subMeshInfoPathHashOffset); }
      __except (EXCEPTION_EXECUTE_HANDLER) { pathHash = 0; }
    }
    char meshText[256] = {};
    TraceDescribeObject(mesh, meshText, sizeof(meshText));
    Log("[TRACE-MESH-FLOW] SubMeshInfo.set_mesh info=%p mesh=%p asset=%s pathHash=%lld meshText=%s",
        self, mesh, assetName[0] ? assetName : "<none>",
        (long long)pathHash, meshText[0] ? meshText : "<none>");
  }

  original(self, mesh, methodInfo);
}

static void TraceLogSubMeshInfoArray(void *owner, int32_t lod, bool includeGpu,
                                     void *array, const char *stage) {
  if (!array) return;
  const size_t count = EiemManagedArrayLength(array);
  char ownerText[256] = {};
  char ownerName[192] = {};
  TraceDescribeObject(owner, ownerText, sizeof(ownerText));
  if (owner && s_lodMeshAssetNameOffset >= 0) {
    __try {
      void *name = *(void **)((char *)owner + s_lodMeshAssetNameOffset);
      if (name) ReadStrUtf8(name, ownerName, sizeof(ownerName));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      ownerName[0] = '\0';
    }
  }
  // Capture the first calls regardless of the active mod rule.  The owner
  // name is a part name (not necessarily the serialized asset path), so
  // filtering on it here can hide the actual logical path used by the game.
  if (!TraceTakeBudget(&s_traceLogicalMeshFlowCount, 240)) return;
  Log("[TRACE-MESH-FLOW] %s owner=%p ownerName=%s ownerText=%s lod=%d gpu=%d array=%p count=%zu",
      stage ? stage : "SubMeshInfo[]", owner,
      ownerName[0] ? ownerName : "<none>", ownerText[0] ? ownerText : "<none>",
      lod, includeGpu ? 1 : 0, array, count);
  if (count > 128) return;
  void **items = (void **)((char *)array + 32);
  for (size_t index = 0; index < count; ++index) {
    void *info = items[index];
    if (!info) {
      Log("[TRACE-MESH-FLOW]   item[%zu]=null", index);
      continue;
    }
    void *mesh = nullptr;
    if (s_subMeshInfoMeshOffset >= 0) {
      __try { mesh = *(void **)((char *)info + s_subMeshInfoMeshOffset); }
      __except (EXCEPTION_EXECUTE_HANDLER) { mesh = nullptr; }
    }
    char meshName[192] = {};
    if (s_subMeshInfoMeshNameOffset >= 0) {
      __try {
        void *name = *(void **)((char *)info + s_subMeshInfoMeshNameOffset);
        if (name) ReadStrUtf8(name, meshName, sizeof(meshName));
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        meshName[0] = '\0';
      }
    }
    int64_t pathHash = 0;
    if (s_subMeshInfoPathHashOffset >= 0) {
      __try { pathHash = *(int64_t *)((char *)info + s_subMeshInfoPathHashOffset); }
      __except (EXCEPTION_EXECUTE_HANDLER) { pathHash = 0; }
    }
    char meshText[256] = {};
    TraceDescribeObject(mesh, meshText, sizeof(meshText));
    Log("[TRACE-MESH-FLOW]   item[%zu] info=%p mesh=%p meshName=%s pathHash=%lld meshText=%s",
        index, info, mesh, meshName[0] ? meshName : "<none>",
        (long long)pathHash, meshText[0] ? meshText : "<none>");
  }
}

static void *TraceLodGetSubMeshInfo(void *self, int32_t lod, bool includeGpu,
                                    void *methodInfo) {
  auto original = (TraceLodGetSubMeshInfoFn)s_origLodGetSubMeshInfo;
  void *result = original ? original(self, lod, includeGpu, methodInfo) : nullptr;
  TraceLogSubMeshInfoArray(self, lod, includeGpu, result, "GetSubMeshInfo");
  return result;
}

// The CPU avatar path materializes SubMeshInfo.mesh in this helper rather
// than through the managed property setter. Patch the returned logical data
// before CreateSMS/AssignSkin consumes it, preserving the game's own renderer
// and bone/material assembly.
static void *TraceGetPartCpuMesh(void *meshAssets, int32_t lod,
                                 void *methodInfo) {
  auto original = (TraceGetPartCpuMeshFn)s_origGetPartCpuMesh;
  return original ? original(meshAssets, lod, methodInfo) : nullptr;
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

static void TraceLogCharacterFlow(const char *stage, void *self,
                                   const char *kind, void *aux = nullptr,
                                   int flag = -1) {
  if (!TraceTakeBudget(&s_traceCharacterFlowCount, 320)) return;
  char selfText[512] = {};
  char auxText[512] = {};
  char modelId[256] = {};
  char modelPath[768] = {};
  char partPath[768] = {};
  void *model = nullptr;
  if (_stricmp(kind ? kind : "", "ModelManager") != 0)
    TraceDescribeObject(self, selfText, sizeof(selfText));
  // Async callbacks and ModelManager return values are not guaranteed to be
  // UnityEngine.Object instances. Only describe the part passed to the
  // completion callback; the other auxiliary pointers are logged as addresses.
  if (aux && _stricmp(kind ? kind : "", "BaseModelComponent") == 0 &&
      flag >= 0)
    TraceDescribeObject(aux, auxText, sizeof(auxText));
  if (_stricmp(kind ? kind : "", "BaseModelComponent") == 0) {
    TraceReadStringField(self, s_baseModelIdOffset, modelId,
                         sizeof(modelId));
    TraceReadStringField(self, s_baseModelPathOffset, modelPath,
                         sizeof(modelPath));
  } else if (_stricmp(kind ? kind : "", "BaseModelViewPart") == 0) {
    model = TraceReadObjectField(self, s_basePartModelOffset);
    // m_cfg is an embedded BaseModelViewPartData value type, so its
    // modelPath field is addressed relative to the part object itself.
    const int configPathOffset =
        (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
            ? s_basePartConfigOffset + s_basePartConfigPathOffset
            : -1;
    TraceReadStringField(self, configPathOffset, partPath, sizeof(partPath));
    if (model) TraceDescribeObject(model, modelPath, sizeof(modelPath));
  }
  Log("[TRACE-CHAR-FLOW] stage=%s kind=%s self=%p selfText=%s aux=%p "
      "auxText=%s flag=%d modelId=%s modelPath=%s partPath=%s",
      stage ? stage : "<unknown>", kind ? kind : "<unknown>", self,
      selfText[0] ? selfText : "<none>", aux, auxText[0] ? auxText : "<none>",
      flag, modelId[0] ? modelId : "<none>",
      modelPath[0] ? modelPath : "<none>",
      partPath[0] ? partPath : "<none>");
}

static void *TraceModelManagerLoadString(void *self, void *path,
                                          void *methodInfo) {
  auto original = (TraceModelManagerLoadStringFn)s_origModelManagerLoadString;
  void *result = original ? original(self, path, methodInfo) : nullptr;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  TraceLogCharacterFlow("ModelManager.Load(string)", self, "ModelManager",
                        nullptr);
  if (TraceTakeBudget(&s_traceCharacterFlowCount, 320))
    Log("[TRACE-CHAR-PATH] stage=ModelManager.Load path=%s result=%p",
        pathText[0] ? pathText : "<none>", result);
  return result;
}

static int32_t TraceModelManagerLoadAsyncString(void *self, void *path,
                                                 void *callback,
                                                 void *methodInfo) {
  auto original =
      (TraceModelManagerLoadAsyncStringFn)s_origModelManagerLoadAsyncString;
  const int32_t result =
      original ? original(self, path, callback, methodInfo) : -1;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (TraceTakeBudget(&s_traceCharacterFlowCount, 320))
    Log("[TRACE-CHAR-PATH] stage=ModelManager.LoadAsync path=%s callback=%p "
        "request=%d",
        pathText[0] ? pathText : "<none>", callback, result);
  return result;
}

// PrefabInstantiateProxy is the normal world-model lifecycle adapter. Other
// lifecycle owners below feed the same instance registry and Render path.
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
  const bool applied =
      EiemRegisterAndApplyModelInstance(
          EiemModelOwnerKind::PrefabProxy, self, model, path, instanceUid,
          "PrefabInstantiateProxy.OnCompleted");
  if (configured && !applied) EiemQueueModReconcile("Prefab completed");
  if (configured)
    Log("[MOD-PREFAB] completed proxy=%p uid=%u path=%s model=%p applied=%d",
        self, instanceUid, path, model, applied ? 1 : 0);
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
      "UIModelLoader.LoadModel");
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
  EiemRegisterCharUIModelInstance(self, "CharUIModelMono.OnAwake");
}

static void TraceCharUIModelSetVisible(void *self, bool visible,
                                       void *methodInfo) {
  auto original =
      (TraceCharUIModelSetVisibleFn)s_origCharUIModelSetVisible;
  if (original) original(self, visible, methodInfo);
  if (visible)
    EiemRegisterCharUIModelInstance(self, "CharUIModelMono.SetVisible");
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

// ModelManager calls this for both freshly instantiated and cached models.
// It is the common point where a GameObject becomes a live allocation again,
// so resource rules must be applied here instead of assuming every model went
// through one particular async loader callback.
static void TraceModelManagerGameObjectAllocate(void *self, void *model,
                                                 void *methodInfo) {
  auto original = (TraceModelManagerGameObjectFn)
      s_origModelManagerGameObjectAllocate;
  if (original) original(self, model, methodInfo);
  EiemReapplyRegisteredModelInstance(
      model, "ModelManager._OnGameObjectAllocate");
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
    EiemReapplyRegisteredModelInstance(
        model, "ModelManager.LoadFromPersistentPool");
  }
  return model;
}

static void TraceBaseModelLoadSync(void *self, void *methodInfo) {
  auto original = (TraceBaseModelLoadSyncFn)s_origBaseModelLoadSync;
  if (original) original(self, methodInfo);
  TraceLogCharacterFlow("BaseModelComponent.LoadMainModelSync", self,
                        "BaseModelComponent");
}

static void TraceBaseModelLoadAsync(void *self, void *callback,
                                    void *methodInfo) {
  auto original = (TraceBaseModelLoadAsyncFn)s_origBaseModelLoadAsync;
  if (original) original(self, callback, methodInfo);
  TraceLogCharacterFlow("BaseModelComponent.LoadMainModelAsync", self,
                        "BaseModelComponent");
}

static void TraceBaseModelFinish(void *self, bool success, void *part,
                                 void *methodInfo) {
  auto original = (TraceBaseModelFinishFn)s_origBaseModelFinish;
  if (original) original(self, success, part, methodInfo);
  TraceLogCharacterFlow("BaseModelComponent.OnMainPartLoadFinish", self,
                        "BaseModelComponent", part, success ? 1 : 0);
}

static void TraceBasePartFinish(void *self, bool success, void *methodInfo) {
  auto original = (TraceBasePartFinishFn)s_origBasePartFinish;
  if (original) original(self, success, methodInfo);
  TraceLogCharacterFlow("BaseModelViewPart.OnLoadFinish", self,
                        "BaseModelViewPart", nullptr, success ? 1 : 0);
  if (success)
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart.OnLoadFinish");
}

static void TraceBasePartPostDeal(void *self, void *methodInfo) {
  auto original = (TraceBasePartPostDealFn)s_origBasePartPostDeal;
  if (original) original(self, methodInfo);
  TraceLogCharacterFlow("BaseModelViewPart.PostDealLoadedModel", self,
                        "BaseModelViewPart");
}

static void TraceComplexPartPostDeal(void *self, void *methodInfo) {
  auto original = (TraceBasePartPostDealFn)s_origComplexPartPostDeal;
  if (original) original(self, methodInfo);
  TraceLogCharacterFlow("ComplexModelViewPart.PostDealLoadedModel", self,
                        "ComplexModelViewPart");
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
        self, "BaseModelViewPart._OnLoadUseHandleFinishCallback");
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
        self, "BaseModelViewPart._OnLoadUseHandleFinish");
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

static void *FindMethodWithFirstParamType(void *klass, const char *methodName,
                                          const char *firstParamType,
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
      void *type = il2cpp_method_get_param(method, 0);
      const char *typeName = type && il2cpp_type_get_name
                                 ? il2cpp_type_get_name(type)
                                 : nullptr;
      if (typeName && strcmp(typeName, firstParamType) == 0)
        return method;
    }
  }
  return nullptr;
}

static void TraceHgRendererSetData(void *self, void *data, void *methodInfo) {
  auto original = (TraceHgRendererSetDataFn)s_origHgRendererSetData;
  if (original) original(self, data, methodInfo);
  if (!TraceTakeBudget(&s_traceHgDataCount, 160)) return;
  char rendererText[256] = {};
  char dataText[256] = {};
  TraceDescribeObject(self, rendererText, sizeof(rendererText));
  TraceDescribeObject(data, dataText, sizeof(dataText));
  Log("[TRACE-HG-FLOW] HGMeshRenderer.set_data renderer=%p data=%p rendererText=%s dataText=%s",
      self, data, rendererText[0] ? rendererText : "<none>",
      dataText[0] ? dataText : "<none>");
}

static void *TraceHgRendererGetData(void *self, void *methodInfo) {
  auto original = (TraceHgRendererGetDataFn)s_origHgRendererGetData;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (result && TraceTakeBudget(&s_traceHgDataCount, 120)) {
    char text[256] = {};
    TraceDescribeObject(result, text, sizeof(text));
    Log("[TRACE-HG-FLOW] HGMeshRenderer.get_data renderer=%p data=%p text=%s",
        self, result, text[0] ? text : "<none>");
  }
  return result;
}

static void *TraceHgDataGetMeshes(void *self, void *methodInfo) {
  auto original = (TraceHgDataGetMeshesFn)s_origHgDataGetMeshes;
  void *result = original ? original(self, methodInfo) : nullptr;
  if (!TraceTakeBudget(&s_traceHgDataCount, 140)) return result;
  const size_t count = EiemManagedArrayLength(result);
  Log("[TRACE-HG-FLOW] HGMeshRendererData.GetMeshes data=%p array=%p count=%zu",
      self, result, count);
  if (result && count <= 32) {
    void **items = (void **)((char *)result + 32);
    for (size_t index = 0; index < count; ++index) {
      char text[256] = {};
      TraceDescribeObject(items[index], text, sizeof(text));
      Log("[TRACE-HG-FLOW]   mesh[%zu]=%p text=%s", index, items[index],
          text[0] ? text : "<none>");
    }
  }
  return result;
}

static void TraceHgDataSetMaterials(void *self, void *materials,
                                    void *methodInfo) {
  auto original = (TraceHgDataSetMaterialsFn)s_origHgDataSetMaterials;
  if (original) original(self, materials, methodInfo);
  if (!TraceTakeBudget(&s_traceHgDataCount, 120)) return;
  Log("[TRACE-HG-FLOW] HGMeshRendererData.SetMaterials data=%p materials=%p count=%zu",
      self, materials, EiemManagedArrayLength(materials));
}

static void TraceHgStateInit(void *self, void *renderer, void *methodInfo) {
  auto original = (TraceHgStateInitFn)s_origHgStateInit;
  if (original) original(self, renderer, methodInfo);
  if (TraceTakeBudget(&s_traceHgStateCount, 120))
    Log("[TRACE-HG-FLOW] HGRendererStateController.Init state=%p renderer=%p",
        self, renderer);
}

static void TraceHgStateInvalidate(void *self, void *methodInfo) {
  auto original = (TraceHgStateInvalidateFn)s_origHgStateInvalidate;
  if (original) original(self, methodInfo);
  if (TraceTakeBudget(&s_traceHgStateCount, 160))
    Log("[TRACE-HG-FLOW] HGRendererStateController.InvalidateMeshCache state=%p",
        self);
}

static void TraceHgStateSetVisible(void *self, bool visible, void *methodInfo) {
  auto original = (TraceHgStateSetVisibleFn)s_origHgStateSetVisible;
  if (original) original(self, visible, methodInfo);
  if (TraceTakeBudget(&s_traceHgStateCount, 160))
    Log("[TRACE-HG-FLOW] HGRendererStateController.SetVisible state=%p visible=%d",
        self, visible ? 1 : 0);
}

static bool EiemApplyResolvedRenderRule(void *renderer, void *drawRenderer,
                                        void *mesh,
                                        const char *rendererType,
                                        void *methodInfo,
                                        const EiemResolvedRenderRule &resolved,
                                        bool allowMeshReplacement = true) {
  (void)methodInfo;
  if (!renderer || !mesh) return false;
  if (!drawRenderer) drawRenderer = renderer;
  const EiemModRule &rule = resolved.rule;
  const char *source = resolved.source[0] ? resolved.source : "<unknown>";
  const char *asset = resolved.asset[0] ? resolved.asset : "<unknown>";

  // `mesh` and `handling=skip` are independent directives. A rule with
  // neither directive is a match-only declaration and must pass through the
  // game's setter unchanged.
  const bool skipOriginal = EiemModEquals(rule.handling, "skip");
  const bool applyMesh = allowMeshReplacement && rule.hasMesh;
  if (!applyMesh && !(allowMeshReplacement && rule.hasSkeleton) && !skipOriginal && !rule.materialCount && !rule.submeshCount && !rule.shapeCount &&
      !(allowMeshReplacement && rule.partnerCount))
    return false;

  EiemProbeRememberRule(rule, mesh);
  EiemProbeRenderer("apply-before", renderer, drawRenderer, rendererType, mesh, true);
  if (!EiemCaptureOriginal(renderer, drawRenderer, mesh, rendererType, applyMesh, nullptr, rule.shapeCount != 0)) return false;
  EiemRememberRuleBinding(renderer, rule);

  std::shared_ptr<EiemSkeletonInstance> skeleton;
  bool skeletonReady=true;
  if (allowMeshReplacement && rule.hasSkeleton) {
    char skeletonError[256]={};
    skeletonReady=EiemModEquals(rendererType,"SkinnedMeshRenderer") &&
        EiemAcquireSkeleton(rule,renderer,skeleton,skeletonError,sizeof(skeletonError));
    if (skeletonReady) {
      AcquireSRWLockExclusive(&s_eiemOverrideLock);
      const auto index=EiemFindOverrideLocked(renderer);
      if (index!=SIZE_MAX) {
        auto &owned=s_eiemOverrides[index].skeleton;
        if (owned && owned!=skeleton) {
          // A later conflicting rule must not drop nodes still referenced by
          // the previously assigned Mesh. Restore its binding first (F10).
          skeletonReady=false;
          Log("[SKELETON] Conflicting Skeleton on renderer=%p; restore before rebinding",renderer);
        } else owned=skeleton;
      } else skeletonReady=false;
      ReleaseSRWLockExclusive(&s_eiemOverrideLock);
    } else Log("[SKELETON] %s / %s: %s",rule.modPath,rule.section,
        skeletonError[0]?skeletonError:"Skeleton requires a skinned source Renderer");
  }

  // `handling=skip` owns only the source Renderer state. It is intentionally
  // independent from every resource mount below.
  if (skipOriginal) {
    EiemCaptureEnabledForSkip(renderer, drawRenderer);
    if (g_renderer_set_enabled)
      EiemSetRendererEnabled(drawRenderer, false);
    Log("[MOD] %s resource skip applied: source=%s asset=%s", rendererType,
        source, asset);
  }

  // `mesh=` replaces the source Renderer's shared Mesh in place. It does not
  // create another Renderer; an additional Renderer must be declared and
  // referenced explicitly through `partner.N`.
  bool meshApplied = false;
  if (applyMesh && skeletonReady) {
    void *assignedMesh = nullptr;
    void *assignedBones = nullptr;
    std::shared_ptr<const EiemSkinIdentity> skin;
    EiemUnityRef assignedBonesRoot;
    char meshError[256] = {};
    if (EiemBuildMeshResource(rule, &assignedMesh, meshError,
                              sizeof(meshError), mesh, &skin) && assignedMesh &&
        (!skin || (EiemModEquals(rendererType,"SkinnedMeshRenderer") &&
                   (skeleton ? EiemSkeletonMeshBones(*skin,*skeleton,&assignedBones,meshError,sizeof(meshError))
                             : EiemResolveMeshBones(*skin,renderer,&assignedBones,meshError,sizeof(meshError))))) &&
        (!assignedBones || (assignedBonesRoot=EiemUnityRef::Capture(assignedBones,false))) &&
        EiemPrepareRendererShapeBinding(renderer,assignedMesh,rendererType,meshError,sizeof(meshError))) {
      // A failed native setter can already have cleared the field. Own the
      // restoration BEFORE calling it, independently of skip and success.
      EiemBeginMeshWrite(renderer);
      meshApplied = EiemSetSharedMesh(renderer, assignedMesh, rendererType, nullptr);
      if(meshApplied)meshApplied=EiemInitializeRendererShapeBinding(renderer,rendererType,meshError,sizeof(meshError));
      if (meshApplied && assignedBones)
        meshApplied = EiemPreserveSourceSkinning(renderer, assignedBones, meshError, sizeof(meshError));
    }
    if (meshApplied) EiemRememberReplacement(renderer, assignedMesh, rendererType);
    if (!meshApplied)
      Log("[MOD] %s resource mesh replacement failed: source=%s asset=%s mesh=%s error=%s",
          rendererType, source, asset, rule.mesh,
          meshError[0] ? meshError : "assignment failed");
    Log("[MOD] %s resource mesh replaced: source=%s asset=%s mesh=%s "
        "applied=%s actual=%p",
        rendererType, source, asset, rule.mesh,
        meshApplied ? "true" : "false",
        EiemReadSharedMesh(renderer, rendererType));
    if (meshApplied)
      EiemPreserveSourceDrawState(renderer, rendererType);
  }

  // Material edits remain source-Renderer edits and are likewise independent
  // of skip/mesh. They are applied after the two resource actions above.
  char error[256] = {};
  if ((rule.materialCount || rule.submeshCount) && !EiemMaterialSourceInitActive(drawRenderer)) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(rule, drawRenderer, &materials,
                                             error,
                                             sizeof(error))) {
      Log("[MOD] %s material resource failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
    } else if (materials &&
               (!EiemCaptureOriginal(renderer, drawRenderer, mesh, rendererType, false, &rule) ||
                !EiemAssignRendererMaterials(drawRenderer, materials, error, sizeof(error)))) {
      Log("[MOD] %s material assignment failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
    }
  }
  if (!applyMesh || meshApplied) {
    AcquireSRWLockExclusive(&s_eiemOverrideLock);
    const size_t index = EiemFindOverrideLocked(renderer);
    if (index != SIZE_MAX) EiemUpdateRendererShapes(renderer, rendererType, rule, s_eiemOverrides[index].shapes);
    ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  }
  bool currentEnabled = true;
  const bool readEnabled = EiemReadRendererEnabled(drawRenderer,
                                                   &currentEnabled);
  bool currentVisible = false;
  const bool readVisible = EiemReadRendererVisible(drawRenderer,
                                                   &currentVisible);
  const int32_t materialCount = EiemReadRendererMaterialCount(drawRenderer);
  char rendererDescription[512] = {};
  TraceDescribeObject(drawRenderer, rendererDescription,
                      sizeof(rendererDescription));
  Log("[MOD] %s resource rule applied: source=%s asset=%s mesh=%s materials=%u skip=%s enabled=%s",
      rendererType, source, asset,
      applyMesh ? rule.mesh : "<none>", rule.materialCount,
      skipOriginal ? "true" : "false",
      readEnabled ? (currentEnabled ? "true" : "false") : "unknown");
  if (applyMesh) {
    Log("[DEBUG-DRAW-STATE] renderer=%s mesh=%p materials=%d enabled=%s visible=%s",
        rendererDescription[0] ? rendererDescription : "<unknown>",
        EiemReadSharedMesh(renderer, rendererType), materialCount,
        readEnabled ? (currentEnabled ? "true" : "false") : "unknown",
        readVisible ? (currentVisible ? "true" : "false") : "unknown");
  }
  if (allowMeshReplacement)
    EiemApplyPartners(renderer, drawRenderer, mesh, rendererType, rule);
  EiemProbeRenderer("apply-after", renderer, drawRenderer, rendererType, nullptr, true);
  return true;
}

static void *EiemFindMeshFilterDrawRenderer(void *meshFilter) {
  if (!meshFilter || !g_rendererClass || !g_component_get_gameObject ||
      !g_gameObject_GetComponent || !il2cpp_class_get_type ||
      !il2cpp_type_get_object)
    return nullptr;
  __try {
    void *gameObject = Invoke(g_component_get_gameObject, meshFilter);
    void *type = il2cpp_class_get_type(g_rendererClass);
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!gameObject || !typeObject) return nullptr;
    void *params[] = {typeObject};
    return Invoke(g_gameObject_GetComponent, gameObject, params);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

static bool EiemApplyRenderRuleSetToRenderer(
    void *rootTransform, void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo,
    const std::vector<EiemModRule> &rules, const char *sourceLabel,
    bool *referenced = nullptr, bool *matched = nullptr,
    const std::vector<std::string> *affected = nullptr,
    std::vector<EiemPhysicsIntent> *physicsIntents = nullptr) {
  if (!meshOwner || !drawRenderer || !mesh || !rendererType) return false;
  if (s_eiemCreatingPartner || EiemIsPartnerRenderer(drawRenderer)) return false;
  void *identityMesh = mesh;
  EiemPrepareRenderInput(meshOwner, mesh, rendererType, &identityMesh);
  if (!identityMesh) return false;

  char source[768] = {};
  char asset[192] = {};
  if (!EiemReadLiveMeshIdentity(identityMesh, source, sizeof(source), asset,
                                sizeof(asset)))
    return false;

  char relativePath[768] = {};
  bool relativePathAttempted = false;
  bool relativePathResolved = false;
  for (size_t ruleIndex = 0; ruleIndex < rules.size(); ++ruleIndex) {
    const EiemModRule &rule = rules[ruleIndex];
    if (rule.path[0]) {
      if (!relativePathAttempted) {
        relativePathAttempted = true;
        relativePathResolved =
            rootTransform && EiemBuildRelativeRendererPath(
                                 rootTransform, drawRenderer, relativePath,
                                 sizeof(relativePath));
      }
      if (!relativePathResolved) continue;
    }
    if (!EiemRenderRuleMatches(rule, relativePath, identityMesh, asset))
      continue;
    if (referenced) referenced[ruleIndex] = true;
    if (matched) *matched = true;
    if (rule.hasPhysics && physicsIntents &&
        !EiemCollectPhysicsIntent(rule, physicsIntents, drawRenderer))
      Log("[PHYSICS] Cannot resolve matched intent: mod=%s render=%s resource=%s",
          rule.modPath, rule.section, rule.physics);
    // Preserve first-match precedence even during a mod-scoped key update.
    // Filtering the rule list before matching would promote a lower-priority mod.
    if (!EiemModAffected(rule.modPath, affected)) return false;
    EiemResolvedRenderRule resolved = {};
    resolved.rule = rule;
    strncpy_s(resolved.source, sizeof(resolved.source),
              source[0] ? source : (sourceLabel ? sourceLabel : "<mesh>"),
              _TRUNCATE);
    strncpy_s(resolved.asset, sizeof(resolved.asset), asset, _TRUNCATE);
    return EiemApplyResolvedRenderRule(meshOwner, drawRenderer, identityMesh,
                                       rendererType, methodInfo, resolved,
                                       true);
  }
  return false;
}

static bool EiemApplyRenderRuleSet(void *model,
                                   const std::vector<EiemModRule> &rules,
                                   const char *sourceLabel,
                                    const char *stage, bool *referenced = nullptr,
                                    bool *matched = nullptr,
                                    const std::vector<std::string> *affected = nullptr,
                                    std::vector<EiemPhysicsIntent> *physicsIntents = nullptr) {
  if (!model || rules.empty() || !EiemOnUnityThread() ||
      !g_gameObject_GetComponentsInChildren || !il2cpp_class_get_type ||
      !il2cpp_type_get_object)
    return false;

  void *root = g_gameObject_get_transform
                   ? Invoke(g_gameObject_get_transform, model)
                   : nullptr;
  uint32_t applied = 0;
  size_t visited = 0;
  const uintptr_t previousOwner = s_eiemActivePrefabInstance;
  s_eiemActivePrefabInstance = (uintptr_t)model;

  auto visitType = [&](void *componentClass, const char *rendererType) {
    if (!componentClass) return;
    void *type = il2cpp_class_get_type(componentClass);
    void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
    if (!typeObject) return;
    bool includeInactive = true;
    void *params[] = {typeObject, &includeInactive};
    void *array = Invoke(g_gameObject_GetComponentsInChildren, model, params);
    const size_t count = EiemManagedArrayLength(array);
    if (!array || count > 8192) return;
    visited += count;
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < count; ++index) {
      void *meshOwner = items[index];
      if (!meshOwner) continue;
      void *drawRenderer = EiemModEquals(rendererType, "SkinnedMeshRenderer")
                               ? meshOwner
                               : EiemFindMeshFilterDrawRenderer(meshOwner);
      if (!drawRenderer) continue;
      void *mesh = EiemReadSharedMesh(meshOwner, rendererType);
      if (mesh && EiemApplyRenderRuleSetToRenderer(
                      root, meshOwner, drawRenderer, mesh, rendererType,
                      nullptr, rules, sourceLabel, referenced, matched, affected,
                      physicsIntents))
        ++applied;
    }
  };

  visitType(g_skinnedMeshRendererClass, "SkinnedMeshRenderer");
  visitType(g_meshFilterClass, "MeshFilter");
  s_eiemActivePrefabInstance = previousOwner;
  Log("[MOD-MESH] applied model=%p components=%zu actions=%u stage=%s",
      model, visited, applied, stage ? stage : "unknown");
  return applied != 0;
}

static bool EiemApplyStandaloneRenderRules(void *model, const char *stage,
                                           bool *matched, const std::vector<std::string> *affected,
                                           std::vector<EiemPhysicsIntent> *physicsIntents) {
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  return EiemApplyRenderRuleSet(model, rules, "<mesh identity>", stage, nullptr,
                                matched, affected, physicsIntents);
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
      "<mesh setter>");
}

struct EiemModelOwnerRef {
  EiemModelOwnerKind kind = EiemModelOwnerKind::PrefabProxy;
  void *owner = nullptr;
  bool active = true;
};

struct EiemModelInstanceState {
  void *model = nullptr;
  EiemUnityRef modelRef;
  uint32_t instanceUid = 0;
  char path[768] = {};
  EiemModelOwnerRef owners[4] = {};
  uint32_t ownerCount = 0;
  // Declarative snapshots only. Native component handles and retirement
  // state live in the Physics runtime adapter, not in this match plan.
  std::vector<EiemPhysicsIntent> physicsIntents;
};
static SRWLOCK s_eiemModelInstanceLock = SRWLOCK_INIT;
static std::vector<EiemModelInstanceState> s_eiemModelInstances;

static bool EiemModelHasActiveOwner(const EiemModelInstanceState &state) {
  for (uint32_t index = 0; index < state.ownerCount; ++index)
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
  if (found)
    EiemReconcileModelPhysics(model, currentIntents, active, stage);
}

static void EiemSetModelOwnerActive(EiemModelOwnerKind ownerKind, void *owner,
                                    bool active, const char *stage) {
  if (!owner) return;
  size_t changed = 0;
  size_t matched = 0;
  size_t planned = 0;
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (auto &state : s_eiemModelInstances) {
    for (uint32_t index = 0; index < state.ownerCount; ++index) {
      auto &candidate = state.owners[index];
      if (candidate.kind != ownerKind || candidate.owner != owner) continue;
      ++matched;
      if (candidate.active != active) {
        candidate.active = active;
        ++changed;
      }
      planned += state.physicsIntents.size();
    }
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  EiemPhysicsOwnerProbeObserveOwnerActive(
      EiemModelOwnerKindName(ownerKind), owner, active, stage);
  if (changed || planned)
    Log("[PHYSICS-PLAN] owner=%p active=%d models=%zu changed=%zu intents=%zu stage=%s",
        owner, active ? 1 : 0, matched, changed, planned,
        stage ? stage : "unknown");
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
    const char *prefabPath, uint32_t instanceUid, const char *stage) {
  if (!model) return false;
  EiemProbeObserveModel(model, stage);
  auto modelRef = EiemUnityRef::Capture(model);
  if (!modelRef) {
    Log("[MOD-LIFECYCLE] Cannot observe model lifetime model=%p stage=%s", model, stage);
    return false;
  }

  std::vector<uintptr_t> releasedModels;
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
      for (uint32_t ownerIndex = 0; ownerIndex < entry.ownerCount;
           ++ownerIndex) {
        if (entry.owners[ownerIndex].kind != ownerKind ||
            entry.owners[ownerIndex].owner != owner)
          continue;
        for (uint32_t move = ownerIndex + 1; move < entry.ownerCount; ++move)
          entry.owners[move - 1] = entry.owners[move];
        --entry.ownerCount;
        break;
      }
      if (entry.ownerCount == 0) {
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
    for (uint32_t index = 0; index < state.ownerCount; ++index) {
      if (state.owners[index].kind == ownerKind &&
          state.owners[index].owner == owner) {
        state.owners[index].active = true;
        knownOwner = true;
        break;
      }
    }
    if (!knownOwner && state.ownerCount < _countof(state.owners)) {
      state.owners[state.ownerCount].kind = ownerKind;
      state.owners[state.ownerCount].owner = owner;
      state.owners[state.ownerCount].active = true;
      ++state.ownerCount;
    }
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  for (uintptr_t released : releasedModels) {
    EiemReleaseModelPhysics((void *)released, "owner moved to another model");
    EiemPhysicsOwnerProbeObserveRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)released,
        "owner moved to another model");
    EiemDestroyPartnerObjects(released);
    EiemForgetRenderOverrides(released);
  }
  // Existence is recorded before consulting the current program. An empty
  // INI must not make an already-created model undiscoverable at the next F10.
  if (modelRef.Status() != 1) return false;
  bool applied = false;
  std::vector<EiemPhysicsIntent> physicsIntents;
  if (EiemHasStandaloneRenderRules())
    applied = EiemApplyStandaloneRenderRules(model, stage, nullptr, nullptr,
                                             &physicsIntents);
  EiemStoreModelPhysicsIntents(model, std::move(physicsIntents), stage);
  if (applied) {
    EiemPhysicsOwnerProbeObserveModel(
        EiemModelOwnerKindName(ownerKind), owner, model, stage);
  }
  return applied;
}

// BaseModelViewPart is the game's confirmed character-model completion owner.
// In particular, its handle path reuses an already loaded model without
// creating another PrefabInstantiateProxy. Read the exact model and logical
// path held by that part; never infer identity from a scene-wide Mesh scan.
static bool EiemRegisterBaseModelViewPartInstance(void *part,
                                                   const char *stage) {
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
      path[0] ? path : nullptr, 0, stage);
  Log("[MOD-MODEL-PART] completed part=%p path=%s model=%p applied=%d stage=%s",
      part, path, model, applied ? 1 : 0, stage ? stage : "unknown");
  return applied;
}

// CharUIModelMono is attached directly to the UI presentation hierarchy. Its
// own GameObject is therefore a sufficient lifecycle root for Mesh-identity
// rules; no PFB name inference or scene-wide search is needed.
static bool EiemRegisterCharUIModelInstance(void *component,
                                             const char *stage) {
  if (!component || !g_component_get_gameObject)
    return false;
  void *model = Invoke(g_component_get_gameObject, component);
  const bool applied = EiemRegisterAndApplyModelInstance(
      EiemModelOwnerKind::CharUIModel, component, model, nullptr, 0, stage);
  Log("[MOD-CHAR-UI] completed component=%p model=%p applied=%d stage=%s",
      component, model, applied ? 1 : 0, stage ? stage : "unknown");
  return applied;
}

static bool EiemReapplyRegisteredModelInstance(void *model,
                                                const char *stage) {
  if (!model) return false;
  EiemProbeObserveModel(model, stage); // Observe pooled arrivals even if not registered.
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
  AcquireSRWLockExclusive(&s_eiemModelInstanceLock);
  for (size_t stateIndex = 0; stateIndex < s_eiemModelInstances.size();) {
    auto &state = s_eiemModelInstances[stateIndex];
    for (uint32_t ownerIndex = 0; ownerIndex < state.ownerCount;
         ++ownerIndex) {
      if (state.owners[ownerIndex].kind != ownerKind ||
          state.owners[ownerIndex].owner != owner)
        continue;
      for (uint32_t move = ownerIndex + 1; move < state.ownerCount; ++move)
        state.owners[move - 1] = state.owners[move];
      --state.ownerCount;
      break;
    }
    if (state.ownerCount == 0) {
      releasedModels.push_back((uintptr_t)state.model);
      s_eiemModelInstances.erase(s_eiemModelInstances.begin() + stateIndex);
    } else {
      ++stateIndex;
    }
  }
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  for (uintptr_t modelOwner : releasedModels) {
    EiemReleaseModelPhysics((void *)modelOwner, stage);
    EiemPhysicsOwnerProbeObserveRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)modelOwner, stage);
    EiemDestroyPartnerObjects(modelOwner);
    EiemForgetRenderOverrides(modelOwner);
  }
  if (!releasedModels.empty())
    Log("[MOD-LIFECYCLE] released owner=%p instances=%zu stage=%s", owner,
        releasedModels.size(), stage ? stage : "unknown");
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
  EiemPhysicsOwnerProbeObserveRelease("model", nullptr, model, stage);
  EiemDestroyPartnerObjects(modelOwner);
  EiemForgetRenderOverrides(modelOwner);
  Log("[MOD-LIFECYCLE] released model=%p stage=%s", model,
      stage ? stage : "unknown");
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

static bool EiemReapplyRendererMaterialsAfterCommit(void *renderer,
                                                     const char *stage) {
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

static void EiemProbeMaterialInfo(void *info, const char *stage) {
  EiemProbeReadScope scope;
  if (!scope.entered) return;
  void *renderer = EiemReadRendererFromMaterialInfo(info);
  if (EiemProbeNativeAlive(renderer) != 1 || !il2cpp_object_get_class) return;
  void *klass = il2cpp_object_get_class(renderer);
  while (klass && klass != g_skinnedMeshRendererClass)
    klass = il2cpp_class_get_parent ? il2cpp_class_get_parent(klass) : nullptr;
  if (!klass) return;
  void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
  if (!EiemProbeMatches(renderer, mesh)) return;
  char infoText[48] = {};
  snprintf(infoText, sizeof(infoText), "info=%p ", info);
  const std::string owner = std::string(infoText) + "renderer=" + EiemProbeObjectText(renderer);
  EiemProbeRendererRead(stage, renderer, renderer, "SkinnedMeshRenderer", nullptr, true);
  EiemProbeMaterials(stage, TraceReadObjectField(info, s_eiemProbeSourceMaterialsOffset), owner + " sourceMaterials");
  EiemProbeMaterials(stage, TraceReadObjectField(info, s_eiemProbeReplacingMaterialsOffset), owner + " replacingMaterials");
}

static void TraceMaterialInfoInit(void *self, void *renderer, void *configs, void *methodInfo) {
  using InitFn = void (*)(void *, void *, void *, void *);
  auto original = (InitFn)s_origMaterialInfoInit;
  EiemProbeTrackedRenderer("material-info-init-before", renderer);
  bool sourceReady = false;
  {
    EiemMaterialSourceInitScope scope(renderer);
    sourceReady = EiemExposeSourceMaterialsForInit(renderer);
    // Never skip game initialization, including when source exposure failed.
    if (original) original(self, renderer, configs, methodInfo);
  }
  EiemProbeMaterialInfo(self, "material-info-init-after");
  if (sourceReady) {
    // NPC avatar construction can populate SkinnedMeshRenderer.sharedMesh
    // without entering Unity's public setter and without publishing a model
    // root through the Prefab/UI adapters. RendererInfo._Init is the observed
    // per-Renderer boundary shared by those instances. The game has now
    // captured its clean material source table, so offer the concrete Renderer
    // to the same global Mesh-identity executor used by every other consumer.
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
      EiemPhysicsOwnerProbeObserveRenderer(renderer, "RendererInfo._Init");
    }
  } else
    Log("[MOD-MATERIAL-SOURCE] init source unresolved; mod reapply not attempted renderer=%p", renderer);
}

static bool TraceRendererInfoTrySetSharedMaterial(void *self, void *material,
                                                  void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTrySetSharedMaterial;
  EiemProbeMaterialInfo(self, "material-commit-before-single");
  const bool result = original ? original(self, material, methodInfo) : false;
  EiemProbeMaterialInfo(self, "material-commit-game-after-single");
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TrySetSharedMaterial");
  EiemProbeMaterialInfo(self, "material-commit-mod-after-single");
  return result;
}

static bool TraceRendererInfoTrySetSharedMaterials(void *self,
                                                   void *materials,
                                                   void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTrySetSharedMaterials;
  EiemProbeMaterialInfo(self, "material-commit-before-array");
  const bool result = original ? original(self, materials, methodInfo) : false;
  EiemProbeMaterialInfo(self, "material-commit-game-after-array");
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TrySetSharedMaterials");
  EiemProbeMaterialInfo(self, "material-commit-mod-after-array");
  return result;
}

static bool TraceRendererInfoTryReplaceSharedMaterials(void *self,
                                                       void *materials,
                                                       void *methodInfo) {
  auto original = (TraceRendererInfoMaterialCommitFn)
      s_origRendererInfoTryReplaceSharedMaterials;
  EiemProbeMaterialInfo(self, "material-commit-before-replace");
  const bool result = original ? original(self, materials, methodInfo) : false;
  EiemProbeMaterialInfo(self, "material-commit-game-after-replace");
  EiemReapplyRendererMaterialsAfterCommit(
      EiemReadRendererFromMaterialInfo(self), "TryReplaceSharedMaterials");
  EiemProbeMaterialInfo(self, "material-commit-mod-after-replace");
  return result;
}

// These methods expose the separate NPC avatar construction order for
// diagnostics. Character model replacement is owned by the model/PFB
// lifecycle above; NPC activity alone is not evidence that a UI character
// presentation uses this pipeline.
static void TraceAssignSkinPost(int32_t lod, void *renderers,
                                void *rootBones, void *closure,
                                void *methodInfo) {
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinPost;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  if (TraceTakeBudget(&s_traceAvatarAssemblyCount, 160))
    Log("[TRACE-ASSIGN-SKIN-BOUNDARY] lod=%d array=%p", lod, renderers);
}

static void TraceSetSmrRootBone(void *animator, void *renderers,
                                void *rootBoneInfos, void *methodInfo) {
  auto original = (TraceSetSmrRootBoneFn)s_origSetSmrRootBone;
  if (original) original(animator, renderers, rootBoneInfos, methodInfo);
  if (TraceTakeBudget(&s_traceAvatarAssemblyCount, 160))
    Log("[TRACE-ROOT-BONE-BOUNDARY] animator=%p array=%p count=%zu",
        animator, renderers, EiemManagedArrayLength(renderers));
}

static void TraceCreateSmsGo(void *assetLoader, void *meshAssets, int32_t lod,
                             void *goPool, void *parent, void *stringList,
                             void *intList, void **renderers,
                             void **rootBones, bool flag, void *handleMap,
                             bool deferred, void *methodInfo) {
  auto original = (TraceCreateSmsGoFn)s_origCreateSmsGo;
  if (original)
    original(assetLoader, meshAssets, lod, goPool, parent, stringList,
             intList, renderers, rootBones, flag, handleMap, deferred,
             methodInfo);
  void *array = renderers ? *renderers : nullptr;
  if (InterlockedCompareExchange(&s_smsArrayProbeLogged, 1, 0) == 0) {
    auto readWord = [](void *address, size_t offset) -> uintptr_t {
      __try { return *(uintptr_t *)((char *)address + offset); }
      __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
    };
    Log("[DEBUG-SMS-ARRAY] out=%p outValue=%p outLen=%zu outLen16=%llu "
        "outLen24=%llu valueLen16=%llu valueLen24=%llu",
        renderers, array, EiemManagedArrayLength(renderers),
        (unsigned long long)readWord(renderers, 16),
        (unsigned long long)readWord(renderers, 24),
        (unsigned long long)readWord(array, 16),
        (unsigned long long)readWord(array, 24));
  }
  if (TraceTakeBudget(&s_traceAvatarAssemblyCount, 160))
    Log("[TRACE-SMS-BOUNDARY] CreateSMSGO lod=%d array=%p", lod, array);
}

static void TraceCreateSmsPost(void *meshAssets, int32_t lod, void *goPool,
                               void *parent, void *stringList, void *intList,
                               void **renderers, void **rootBones, bool flag,
                               void *methodInfo) {
  auto original = (TraceCreateSmsPostFn)s_origCreateSmsPost;
  if (original)
    original(meshAssets, lod, goPool, parent, stringList, intList, renderers,
             rootBones, flag, methodInfo);
  void *array = renderers ? *renderers : nullptr;
  if (InterlockedCompareExchange(&s_smsArrayProbeLogged, 1, 0) == 0) {
    auto readWord = [](void *address, size_t offset) -> uintptr_t {
      __try { return *(uintptr_t *)((char *)address + offset); }
      __except (EXCEPTION_EXECUTE_HANDLER) { return 0; }
    };
    Log("[DEBUG-SMS-ARRAY] post out=%p outValue=%p outLen=%zu outLen16=%llu "
        "outLen24=%llu valueLen16=%llu valueLen24=%llu",
        renderers, array, EiemManagedArrayLength(renderers),
        (unsigned long long)readWord(renderers, 16),
        (unsigned long long)readWord(renderers, 24),
        (unsigned long long)readWord(array, 16),
        (unsigned long long)readWord(array, 24));
  }
  if (TraceTakeBudget(&s_traceAvatarAssemblyCount, 160))
    Log("[TRACE-SMS-BOUNDARY] CreateSMSInfoForPostModel lod=%d array=%p",
        lod, array);
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
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (auto &state : s_eiemPartners) {
    EiemModRule rule = {};
    if (EiemModAffected(state.modPath, &affected) &&
        EiemFindRenderRuleBySection(state.modPath, state.section, &rule))
      EiemUpdateRendererShapes(state.partnerRenderer, state.rendererType, rule, state.shapes);
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
}


static EiemModUpdateQueue s_eiemModUpdates;
// One ordered queue for window-thread keys and render-thread ImGui controls.
static SRWLOCK s_eiemInputLock = SRWLOCK_INIT;
static std::vector<EiemModInputEvent> s_eiemPendingInputs;

static void EiemQueueModInput(EiemModInputEvent event) {
  AcquireSRWLockExclusive(&s_eiemInputLock);
  // Adjacent UI frames merge variable writes; never move one past a key press.
  if (!event.uiSection.empty() && !s_eiemPendingInputs.empty() &&
      s_eiemPendingInputs.back().generation == event.generation &&
      s_eiemPendingInputs.back().modPath == event.modPath &&
      s_eiemPendingInputs.back().uiSection == event.uiSection) {
    for (const auto &value : event.values) s_eiemPendingInputs.back().values[value.first] = value.second;
  }
  else s_eiemPendingInputs.push_back(std::move(event));
  ReleaseSRWLockExclusive(&s_eiemInputLock);
  EiemRequestModUpdate(EiemModUpdate::Reapply, "mod control");
}

static void EiemQueueModKey(EiemKeyChord chord, LONG generation) {
  if (!EiemOnUnityThread() || !g_pluginActive) return;
  HWND foreground = GetForegroundWindow();
  if (foreground != g_gameHwnd && foreground != g_guiHwnd && foreground != g_modUiHwnd) return;
  EiemModInputEvent event{chord,generation};
  event.uiFocus = foreground != g_gameHwnd;
  EiemQueueModInput(std::move(event));
}

static void EiemRequestModUpdate(EiemModUpdate request, const char *reason) {
  if (g_shutdownRequested || !g_gameHwnd || !IsWindow(g_gameHwnd)) {
    Log("[MOD] Reconcile not queued (%s): game window is unavailable",
        reason ? reason : "unknown");
    return;
  }
  if (!s_eiemModUpdates.Request(request)) return;
  if (!PostMessageW(g_gameHwnd, WM_EIEM_MOD_RECONCILE, 0, 0)) {
    s_eiemModUpdates.Take();
    Log("[MOD] Reconcile post failed (%s): err=%lu",
        reason ? reason : "unknown", GetLastError());
    return;
  }
  Log("[MOD] Reconcile queued: %s", reason ? reason : "unknown");
}

static void EiemQueueModReconcile(const char *reason) {
  EiemRequestModUpdate(EiemModUpdate::Reconcile, reason);
}

// Runs only from MmdWndProc. F10 restores the previous generation, then
// replays configuration against instances registered by either supported
// model lifecycle adapter. No scene-wide Mesh scan exists.
static void EiemRunModReconcile() {
  const uint32_t requests = s_eiemModUpdates.Take();
  if (g_shutdownRequested) return;
  // This function is dispatched from the game's window procedure, which is
  // the safe Unity thread for creating generated Mesh/Material/Texture
  // objects. Startup hooks may run on the plugin worker thread instead.
  if (!s_eiemUnityThreadId) s_eiemUnityThreadId = GetCurrentThreadId();
  if (!g_gameObject_GetComponentsInChildren) {
    if (requests & (uint32_t)EiemModUpdate::Reload) {
      LoadEiemConfig();
      EiemReportCameraFade();
    }
    Log("[MOD] Reconcile skipped: renderer APIs are not ready");
    return;
  }
  std::vector<EiemModInputEvent> inputs;
  AcquireSRWLockExclusive(&s_eiemInputLock);
  inputs.swap(s_eiemPendingInputs);
  ReleaseSRWLockExclusive(&s_eiemInputLock);
  const bool reload = (requests & (uint32_t)EiemModUpdate::Reload) != 0;
  if (reload) EiemProbeCheckpoint("reload-before", true);
  EiemModProgram next;
  std::vector<std::string> affectedMods;
  const std::vector<std::string> *affected = nullptr;
  if (!reload && !inputs.empty()) {
    bool shapesOnly = false;
    if (EiemPrepareInputUpdate(inputs, &next, &affectedMods, &shapesOnly)) {
      if (shapesOnly && !(requests & (uint32_t)EiemModUpdate::Reconcile)) {
        EiemPublishModState(std::move(next));
        EiemReapplyShapeControls(affectedMods);
        return;
      }
      affected = &affectedMods;
    } else if (!(requests & (uint32_t)EiemModUpdate::Reconcile)) return;
  }
  EiemDispatchModUpdate(requests, [&] {
    EiemDestroyPartnerObjects(affected);
    EiemRestoreRenderOverrides(affected);
    EiemCollectSkeletonInstances();
    // Configuration reload must not release Unity Mesh objects that may still
    // be referenced by a Renderer. The resource backend reuses unchanged files
    // and creates a new rooted object only when the file stamp changes.
  }, [] {
    EiemProbeCheckpoint("reload-restored-old-program");
    LoadEiemConfig(); EiemReportCameraFade(); EiemReloadMods();
    EiemProbeCheckpoint("reload-published");
  }, [&] {
  if (affected) EiemPublishModState(std::move(next));
  const char *stage = reload ? "global reload" : affected ? "control state change" : "lifecycle reconcile";
  const ULONGLONG started = GetTickCount64();
  EiemPruneModelInstances();
  std::vector<EiemModelInstanceState> instances;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  instances = s_eiemModelInstances;
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  uint32_t matched = 0;
  for (const auto &instance : instances) {
    if (!instance.model) continue;
    if (instance.modelRef.Status() != 1) {
      Log("[MOD-LIFECYCLE] Cannot validate observed model=%p; not applying rules", instance.model);
      continue;
    }
    std::vector<EiemPhysicsIntent> physicsIntents;
    bool applied = EiemApplyStandaloneRenderRules(
        instance.model, stage, nullptr, affected, &physicsIntents);
    EiemStoreModelPhysicsIntents(instance.model, std::move(physicsIntents),
                                 stage);
    if (applied) ++matched;
  }
  const ULONGLONG elapsed = GetTickCount64() - started;
  Log("[MOD] Reconcile complete: registeredModels=%zu matchedModels=%u elapsed=%llums",
      instances.size(), matched, elapsed);
  EiemCollectSkeletonInstances();
  if (reload) EiemProbeCheckpoint("reload-complete");
  });
}

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
static void *s_origPreloadAutoHash = nullptr;
static void *s_origAssetProxyHandlePath = nullptr;
static void *s_origAssetProxyHandleGet = nullptr;
static void *s_origAssetProxyHandleGetAssetProxy = nullptr;
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

static bool TraceTakeBudget(volatile LONG *counter, LONG limit) {
  if (g_shutdownRequested) return false;
  return InterlockedIncrement((volatile LONG *)counter) <= limit;
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
typedef void (__fastcall *TracePreloadAutoHashFn)(void *self, int64_t pathHash,
                                                   void *methodInfo);
typedef void *(__fastcall *TraceProxyObjectFn)(void *self, void *methodInfo);
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
    Log("[RES-TRACE] FAssetProxyHandle.GetAssetProxy: handle=%p proxy=%p "
        "type=%s",
        self, result, name ? name : "?");
    s_traceReentrant = false;
  }
  return result;
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
  if (result) {
    char pathText[768] = {};
    TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
    TraceRememberProxyOrigin(result, pathHash, pathText);
  }
  if (!s_traceReentrant && TraceMarkHashFirstSeen(pathHash) &&
      TraceTakeBudget(&s_traceHashLoadCount, 600)) {
    s_traceReentrant = true;
    char typeText[512] = {};
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadAssetInternal(hash): "
        "hash=%lld type=%s category=%d immediate=%d priority=%d result=%p",
        (long long)pathHash, typeText, category, immediate ? 1 : 0, priority,
        result);
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
  if (result) {
    char pathText[768] = {};
    TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
    TraceRememberProxyOrigin(result, pathHash, pathText);
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceHashSubAssetCount, 300)) {
    s_traceReentrant = true;
    char subAssetText[512] = {};
    char typeText[512] = {};
    TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
    TraceDescribeObject(type, typeText, sizeof(typeText));
    Log("[RES-TRACE] BundleResourceManager._LoadSubAssetInternal(hash): "
        "hash=%lld subAsset=\"%s\" type=%s category=%d immediate=%d "
        "priority=%d result=%p",
        (long long)pathHash, subAssetText, typeText, category,
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

  if (!s_traceReentrant &&
      TraceTakeBudget(&s_traceAssetCompleteCount, 600)) {
    s_traceReentrant = true;
    char sourceText[512] = {};
    char assetName[768] = {};
    TraceReadAssetName(self, assetName, sizeof(assetName));
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
  if (completedAsset) {
    char assetName[768] = {};
    const int64_t pathHash = TraceReadLoadableHash(self);
    if (pathHash)
      TraceLookupHashPath(pathHash, assetName, sizeof(assetName));
    if (!assetName[0]) TraceReadAssetName(self, assetName, sizeof(assetName));
    TraceRememberAssetOrigin(completedAsset, pathHash, assetName);
  }
  if (!s_traceReentrant &&
      TraceTakeBudget(&s_traceAssetCompleteCount, 600)) {
    s_traceReentrant = true;
    void *asset = nullptr;
    __try { asset = *(void **)((char *)self + 0xA0); }
    __except (1) { asset = nullptr; }
    char assetText[512] = {};
    char assetName[768] = {};
    int64_t pathHash = TraceReadLoadableHash(self);
    TraceReadAssetName(self, assetName, sizeof(assetName));
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
  if (result) {
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceRememberProxyOrigin(result, 0, pathText);
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_tracePathHashCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    char typeText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
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
  if (result) {
    char pathText[768] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceRememberProxyOrigin(result, 0, pathText);
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_tracePathHashCount, 300)) {
    s_traceReentrant = true;
    char pathText[768] = {};
    char subAssetText[512] = {};
    char typeText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeString(subAsset, subAssetText, sizeof(subAssetText));
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
  auto original = (TraceLoadAsset2Fn)s_origAssetBundleLoadAsset2;
  void *result = original ? original(self, path, type, methodInfo) : nullptr;
  if (!s_traceReentrant && TraceTakeBudget(&s_traceLoadAssetCount, 300)) {
    s_traceReentrant = true;
    char pathText[512] = {};
    char typeText[512] = {};
    char resultText[512] = {};
    TraceDescribeString(path, pathText, sizeof(pathText));
    TraceDescribeObject(type, typeText, sizeof(typeText));
    TraceDescribeObject(result, resultText, sizeof(resultText));
    Log("[RES-TRACE] AssetBundle.LoadAsset(string,Type): path=\"%s\" "
        "type=%s result=%s",
        pathText, typeText, resultText);
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
  TraceRememberMeshObservation(self, mesh, "SkinnedMeshRenderer");
  EiemProbeRenderer("game-mesh-set-before", self, self, "SkinnedMeshRenderer", mesh);
  void *sourceMesh = mesh;
  // A later game-side LOD/skin refresh may assign the original Mesh again.
  // Preserve an existing binding; otherwise this assignment is also a precise
  // lifecycle event at which standalone Mesh-identity rules can be evaluated.
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  if (!retained && sourceMesh)
    EiemApplyStandaloneRenderRulesToRenderer(
        self, self, sourceMesh, "SkinnedMeshRenderer", methodInfo,
        "SkinnedMeshRenderer.set_sharedMesh");
  EiemProbeRenderer("game-mesh-set-after", self, self, "SkinnedMeshRenderer", sourceMesh);
  if (!s_traceReentrant && TraceTakeBudget(&s_traceSharedMeshCount, 500)) {
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

static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                         void *methodInfo) {
  auto original = (TraceSetSharedMeshFn)s_origMeshFilterSetSharedMesh;
  if (s_eiemApplyingModMeshAssignment) {
    if (original) original(self, mesh, methodInfo);
    return;
  }
  TraceRememberMeshObservation(self, mesh, "MeshFilter");
  EiemProbeRenderer("game-mesh-set-before", self, nullptr, "MeshFilter", mesh);
  void *sourceMesh = mesh;
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  if (!retained && sourceMesh) {
    void *drawRenderer = EiemFindMeshFilterDrawRenderer(self);
    if (drawRenderer)
      EiemApplyStandaloneRenderRulesToRenderer(
          self, drawRenderer, sourceMesh, "MeshFilter", methodInfo,
          "MeshFilter.set_sharedMesh");
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceMeshFilterCount, 300)) {
    s_traceReentrant = true;
    char rendererText[512] = {};
    char meshText[512] = {};
    TraceDescribeObject(self, rendererText, sizeof(rendererText));
    TraceDescribeObject(mesh, meshText, sizeof(meshText));
    Log("[RES-TRACE] MeshFilter.set_sharedMesh: renderer=%s mesh=%s",
        rendererText, meshText);
    s_traceReentrant = false;
  }
  EiemProbeRenderer("game-mesh-set-after", self, nullptr, "MeshFilter", sourceMesh);
}

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
    int paramCount, const char *returnType) {
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
      const char *sourceFields[] = {"sourceMaterials"};
      const char *replacingFields[] = {"replacingMaterials"};
      s_eiemProbeSourceMaterialsOffset = FindFieldInHierarchy(klass, sourceFields, 1, nullptr);
      s_eiemProbeReplacingMaterialsOffset = FindFieldInHierarchy(klass, replacingFields, 1, nullptr);
      return klass;
    }
  }
  return nullptr;
}

#include "eiem_native_physics_runtime.h"
#include "eiem_npc_model_owner.h"

static void InitIl2CppResourceTrace(void **assemblies, size_t assemblyCount) {
  if (!assemblies || assemblyCount == 0) return;
  EiemInitUnityLifetime(assemblies, assemblyCount);
  EiemProbeInit(assemblies, assemblyCount);
  EiemInstallNpcModelOwner(assemblies, assemblyCount);

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
    if (!infoInit || !Hook(infoInit, "RendererInfo._Init source isolation", (void *)TraceMaterialInfoInit,
                          &s_origMaterialInfoInit))
      Log("[MOD-MATERIAL-SOURCE] RendererInfo._Init source isolation unavailable");
    Log("[DEBUG-residue-v39] material controller fields source=%d replacing=%d",
        s_eiemProbeSourceMaterialsOffset, s_eiemProbeReplacingMaterialsOffset);
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
    static const char *const modelStringType[] = {"System.String"};
    HookTraceMethodWithParamTypes(
        modelManagerClass, "Load", modelStringType, 1,
        "ModelManager.Load(string)", (void *)TraceModelManagerLoadString,
        &s_origModelManagerLoadString);
    void *loadAsync = FindMethodWithFirstParamType(
        modelManagerClass, "LoadAsync", "System.String", 2);
    if (loadAsync &&
        Hook(loadAsync, "ModelManager.LoadAsync(string)",
             (void *)TraceModelManagerLoadAsyncString,
             &s_origModelManagerLoadAsyncString))
      Log("[RES-TRACE] ModelManager.LoadAsync(string) observation hook installed");
    else
      Log("[RES-TRACE] ModelManager.LoadAsync(string) hook failed/not found");

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

  void *baseModelClass = FindClass("Beyond.Gameplay.View", "BaseModelComponent",
                                  assemblies, assemblyCount);
  if (baseModelClass) {
    const char *modelIdFields[] = {"m_modelId"};
    const char *modelPathFields[] = {"m_modelPath"};
    s_baseModelIdOffset = FindFieldInHierarchy(
        baseModelClass, modelIdFields, _countof(modelIdFields), nullptr);
    s_baseModelPathOffset = FindFieldInHierarchy(
        baseModelClass, modelPathFields, _countof(modelPathFields), nullptr);
    Log("[RES-TRACE] BaseModelComponent fields: modelId=0x%X modelPath=0x%X",
        s_baseModelIdOffset, s_baseModelPathOffset);
    HookTraceMethod(baseModelClass, "LoadMainModelSync", 0,
                    "BaseModelComponent.LoadMainModelSync",
                    (void *)TraceBaseModelLoadSync, &s_origBaseModelLoadSync);
    HookTraceMethod(baseModelClass, "LoadMainModelAsync", 1,
                    "BaseModelComponent.LoadMainModelAsync",
                    (void *)TraceBaseModelLoadAsync, &s_origBaseModelLoadAsync);
    HookTraceMethod(baseModelClass, "OnMainPartLoadFinish", 2,
                    "BaseModelComponent.OnMainPartLoadFinish",
                    (void *)TraceBaseModelFinish, &s_origBaseModelFinish);
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.BaseModelComponent class not found");
  }

  void *basePartClass = FindClass("Beyond.Gameplay.View", "BaseModelViewPart",
                                  assemblies, assemblyCount);
  if (basePartClass) {
    const char *partModelFields[] = {"m_model"};
    const char *partConfigFields[] = {"m_cfg"};
    s_basePartModelOffset = FindFieldInHierarchy(
        basePartClass, partModelFields, _countof(partModelFields), nullptr);
    s_basePartConfigOffset = FindFieldInHierarchy(
        basePartClass, partConfigFields, _countof(partConfigFields), nullptr);
    void *partDataClass = FindClass("Beyond.Gameplay.View",
                                    "BaseModelViewPartData", assemblies,
                                    assemblyCount);
    if (partDataClass) {
      const char *partPathFields[] = {"modelPath"};
      s_basePartConfigPathOffset = FindFieldInHierarchy(
          partDataClass, partPathFields, _countof(partPathFields), nullptr);
    }
    Log("[RES-TRACE] BaseModelViewPart fields: model=0x%X cfg=0x%X "
        "cfg.modelPath=0x%X",
        s_basePartModelOffset, s_basePartConfigOffset,
        s_basePartConfigPathOffset);
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

  // This is the game's logical resource boundary. SubMeshInfo.mesh is the
  // value later consumed by CreateSMS and AssignSkin; replacing it here lets the
  // original pipeline initialize bones, materials and LODs from our Mesh.
  void *subMeshInfoClass =
      FindClass("Beyond.NPC.Avatar", "SubMeshInfo", assemblies, assemblyCount);
  if (subMeshInfoClass) {
    s_subMeshInfoClass = subMeshInfoClass;
    const char *meshFields[] = {"mesh", "<mesh>k__BackingField"};
    const char *meshNameFields[] = {"meshName"};
    const char *meshPathFields[] = {"meshPathHash"};
    s_subMeshInfoMeshOffset =
        FindFieldInHierarchy(subMeshInfoClass, meshFields, _countof(meshFields), nullptr);
    if (s_subMeshInfoMeshOffset < 0)
      s_subMeshInfoMeshOffset = FindFieldByTypeInHierarchy(
          subMeshInfoClass, "UnityEngine.Mesh", nullptr);
    s_subMeshInfoMeshNameOffset =
        FindFieldInHierarchy(subMeshInfoClass, meshNameFields, 1, nullptr);
    s_subMeshInfoPathHashOffset =
        FindFieldInHierarchy(subMeshInfoClass, meshPathFields, 1, nullptr);
    Log("[RES-TRACE] SubMeshInfo fields: mesh=0x%X meshName=0x%X meshPathHash=0x%X",
        s_subMeshInfoMeshOffset, s_subMeshInfoMeshNameOffset,
        s_subMeshInfoPathHashOffset);
    void *setMeshMethod = FindMethodInHierarchy(subMeshInfoClass, "set_mesh", 1);
    s_subMeshInfoSetMeshMethodInfo = setMeshMethod;
    if (Hook(setMeshMethod, "Beyond.NPC.Avatar.SubMeshInfo.set_mesh",
             (void *)TraceSubMeshInfoSetMesh, &s_origSubMeshInfoSetMesh))
      Log("[RES-TRACE] Beyond.NPC.Avatar.SubMeshInfo.set_mesh observation hook installed");
    else
      Log("[RES-TRACE] Beyond.NPC.Avatar.SubMeshInfo.set_mesh hook failed");
    void *getMeshMethod = FindMethodInHierarchy(subMeshInfoClass, "get_mesh", 0);
    s_subMeshInfoGetMeshMethodInfo = getMeshMethod;
    if (Hook(getMeshMethod, "Beyond.NPC.Avatar.SubMeshInfo.get_mesh",
             (void *)TraceSubMeshInfoGetMesh, &s_origSubMeshInfoGetMesh))
      Log("[RES-TRACE] Beyond.NPC.Avatar.SubMeshInfo.get_mesh observation hook installed");
    else
      Log("[RES-TRACE] Beyond.NPC.Avatar.SubMeshInfo.get_mesh hook failed");
  } else {
    Log("[RES-TRACE] Beyond.NPC.Avatar.SubMeshInfo not found; logical mesh hook disabled");
  }
  HookTraceMethod(npcAvatarCreatorUtils, "CreateSMSGO", 12,
                  "NPCAvatarCreatorUtils.CreateSMSGO",
                  (void *)TraceCreateSmsGo, &s_origCreateSmsGo);
  HookTraceMethod(npcAvatarCreatorUtils, "CreateSMSInfoForPostModel", 9,
                  "NPCAvatarCreatorUtils.CreateSMSInfoForPostModel",
                  (void *)TraceCreateSmsPost, &s_origCreateSmsPost);
  HookTraceMethod(
      npcAvatarCreatorUtils,
      "<CreateMeshAssetsInfoForPostModel>g__AssignSkin|15_0", 4,
      "NPCAvatarCreatorUtils.CreateMeshAssetsInfoForPostModel.AssignSkin",
      (void *)TraceAssignSkinPost, &s_origAssignSkinPost);
  HookTraceMethod(npcAvatarCreatorUtils, "SetSMRRootBone", 3,
                  "NPCAvatarCreatorUtils.SetSMRRootBone",
                  (void *)TraceSetSmrRootBone, &s_origSetSmrRootBone);

  void *npcAvatarUtils =
      FindClass("Beyond.NPC.Avatar", "NPCAvatarUtils", assemblies,
                assemblyCount);
  if (npcAvatarUtils) {
    static const char *const cpuMeshTypes[] = {
        "Beyond.NPC.Avatar.NPCAvatarLodMeshAssets",
        "Beyond.NPC.Lod.ELODLevel"};
    HookTraceMethodWithParamTypes(
        npcAvatarUtils, "_GetPartCPUMesh", cpuMeshTypes, 2,
        "NPCAvatarUtils._GetPartCPUMesh", (void *)TraceGetPartCpuMesh,
        &s_origGetPartCpuMesh);
  } else {
    Log("[RES-TRACE] Beyond.NPC.Avatar.NPCAvatarUtils class not found");
  }

  void *lodMeshAssetsClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarLodMeshAssets", assemblies,
      assemblyCount);
  if (lodMeshAssetsClass) {
    const char *nameFields[] = {"name"};
    s_lodMeshAssetNameOffset =
        FindFieldInHierarchy(lodMeshAssetsClass, nameFields, 1, nullptr);
    static const char *const getSubMeshTypes[] = {
        "Beyond.NPC.Lod.ELODLevel", "System.Boolean"};
    HookTraceMethodWithParamTypes(
        lodMeshAssetsClass, "GetSubMeshInfo", getSubMeshTypes, 2,
        "NPCAvatarLodMeshAssets.GetSubMeshInfo",
        (void *)TraceLodGetSubMeshInfo, &s_origLodGetSubMeshInfo);
    s_lodGetSubMeshInfoMethodInfo =
        FindMethodWithParamTypes(lodMeshAssetsClass, "GetSubMeshInfo",
                                 getSubMeshTypes, _countof(getSubMeshTypes));
    Log("[RES-TRACE] NPCAvatarLodMeshAssets fields: name=0x%X",
        s_lodMeshAssetNameOffset);
  } else {
    Log("[RES-TRACE] NPCAvatarLodMeshAssets class not found");
  }

  void *hgRendererClass =
      FindClass("UnityEngine", "HGMeshRenderer", assemblies, assemblyCount);
  if (hgRendererClass) {
    HookTraceMethod(hgRendererClass, "set_data", 1,
                    "UnityEngine.HGMeshRenderer.set_data",
                    (void *)TraceHgRendererSetData, &s_origHgRendererSetData);
    HookTraceMethod(hgRendererClass, "get_data", 0,
                    "UnityEngine.HGMeshRenderer.get_data",
                    (void *)TraceHgRendererGetData, &s_origHgRendererGetData);
  } else {
    Log("[RES-TRACE] UnityEngine.HGMeshRenderer class not found");
  }

  void *hgDataClass = FindClass("UnityEngine", "HGMeshRendererData",
                                assemblies, assemblyCount);
  if (hgDataClass) {
    HookTraceMethod(hgDataClass, "GetMeshes", 0,
                    "UnityEngine.HGMeshRendererData.GetMeshes",
                    (void *)TraceHgDataGetMeshes, &s_origHgDataGetMeshes);
    HookTraceMethod(hgDataClass, "SetMaterials", 1,
                    "UnityEngine.HGMeshRendererData.SetMaterials",
                    (void *)TraceHgDataSetMaterials, &s_origHgDataSetMaterials);
  } else {
    Log("[RES-TRACE] UnityEngine.HGMeshRendererData class not found");
  }

  void *hgStateClass = FindClass("Beyond.Rendering.ECS",
                                 "HGRendererStateController", assemblies,
                                 assemblyCount);
  if (hgStateClass) {
    HookTraceMethod(hgStateClass, "Init", 1,
                    "HGRendererStateController.Init",
                    (void *)TraceHgStateInit, &s_origHgStateInit);
    HookTraceMethod(hgStateClass, "InvalidateMeshCache", 0,
                    "HGRendererStateController.InvalidateMeshCache",
                    (void *)TraceHgStateInvalidate,
                    &s_origHgStateInvalidate);
    HookTraceMethod(hgStateClass, "SetVisible", 1,
                    "HGRendererStateController.SetVisible",
                    (void *)TraceHgStateSetVisible,
                    &s_origHgStateSetVisible);
  } else {
    Log("[RES-TRACE] Beyond.Rendering.ECS.HGRendererStateController not found");
  }

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

  Log("[RES-TRACE] Observation hooks ready; original VFS loading preserved");
}
