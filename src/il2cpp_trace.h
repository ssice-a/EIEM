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
#include "eiem_registration_trace.h"
#include "eiem_skin_probe.h"

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
// Bounds the skinned-mesh measurements, which transform a sample of vertices
// and therefore must not run unbounded on the Unity thread.
static volatile LONG s_traceSkinProbeCount = 0;
// Short diagnostic pass for resource replacements. It compares the game's
// source skin with the generated Mesh only for the first few cloth instances,
// so a bad bind/weight result is visible without reintroducing periodic work.
static volatile LONG s_traceResourceSkinProbeCount = 0;
// Armed where Partners are created, which is above the per-frame driver that
// consumes it. Defined next to that driver.
static void EiemArmSkinProbeSweep();
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
static bool TraceIdentityTextMatchesTyphoea(const char *text);
static bool TraceTakeTargetBudget(volatile LONG *counter, LONG limit,
                                  const char *primary,
                                  const char *secondary = nullptr);
static void TraceRememberAssetOrigin(void *asset, int64_t pathHash,
                                     const char *path);
static bool TraceTakeBudget(volatile LONG *counter, LONG limit);
static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize);
static void TraceReadUnityObjectName(void *object, char *out, int outSize);
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
static thread_local bool s_eiemEntityRenderHelperInitGuard = false;
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
static void TraceSkinnedMeshSetSharedMesh(void *self, void *mesh,
                                           void *methodInfo);
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo);
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
                                           std::vector<EiemPhysicsIntent> *physicsIntents = nullptr,
                                           bool allowPartnerCreation = false);
static bool EiemApplyStandaloneRenderRulesToRenderer(
    void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo, const char *stage,
    bool allowPartnerCreation = false);
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
static void EiemUpdateRendererShapes(void *renderer, const char *type,
                                      const EiemModRule &rule, EiemShapeState &state,
                                      float elapsedSeconds = -1.0f) {
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
  if (!EiemApplyShapeWeights(renderer, mesh, rule, state, backend, error,
                             elapsedSeconds))
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

static bool EiemReadBounds(void *getter, void *object, EiemBounds *out) {
  if (!getter || !object || !out) return false;
  void *boxed = Invoke(getter, object);
  if (!boxed) return false;
  *out = *(EiemBounds *)((char *)boxed + 16);
  return std::isfinite(out->center.x) && std::isfinite(out->center.y) &&
         std::isfinite(out->center.z) && std::isfinite(out->extents.x) &&
         std::isfinite(out->extents.y) && std::isfinite(out->extents.z) &&
         out->extents.x >= 0 && out->extents.y >= 0 && out->extents.z >= 0;
}

static void EiemSetReplacementDrawBounds(void *renderer,
                                         const char *rendererType,
                                         void *replacementMesh,
                                         const EiemBounds *sourceBounds) {
  if (!renderer || !replacementMesh ||
      !EiemModEquals(rendererType, "SkinnedMeshRenderer") ||
      !g_smr_set_localBounds || !s_eiemMeshGetBounds)
    return;
  EiemBounds replacement = {};
  if (!EiemReadBounds(s_eiemMeshGetBounds, replacementMesh, &replacement))
    return;
  if (sourceBounds) {
    Vector3 minimum = {
        (std::min)(sourceBounds->center.x - sourceBounds->extents.x,
                   replacement.center.x - replacement.extents.x),
        (std::min)(sourceBounds->center.y - sourceBounds->extents.y,
                   replacement.center.y - replacement.extents.y),
        (std::min)(sourceBounds->center.z - sourceBounds->extents.z,
                   replacement.center.z - replacement.extents.z)};
    Vector3 maximum = {
        (std::max)(sourceBounds->center.x + sourceBounds->extents.x,
                   replacement.center.x + replacement.extents.x),
        (std::max)(sourceBounds->center.y + sourceBounds->extents.y,
                   replacement.center.y + replacement.extents.y),
        (std::max)(sourceBounds->center.z + sourceBounds->extents.z,
                   replacement.center.z + replacement.extents.z)};
    replacement.center = {(minimum.x + maximum.x) * .5f,
                          (minimum.y + maximum.y) * .5f,
                          (minimum.z + maximum.z) * .5f};
    replacement.extents = {(maximum.x - minimum.x) * .5f,
                           (maximum.y - minimum.y) * .5f,
                           (maximum.z - minimum.z) * .5f};
  }
  void *params[] = {&replacement};
  Invoke(g_smr_set_localBounds, renderer, params);
}

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
  if (!rootBones || index >= count || elementSize < 24) return false;
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
  if (pathSlots && hashSlots && pathSlots != hashSlots)
    return reject("Replacement bone path/hash counts differ");
  const size_t payloadSlots = pathSlots ? pathSlots : hashSlots;
  if (!payloadSlots) return reject("Replacement mesh has no bone palette identity");
  if (payloadSlots < count)
    return reject("Replacement bone palette removes source slots");
  if (payloadSlots == count) {
    Log("[MOD-SKIN] renderer=%p binding=source-index slots=%zu reason=source-prefix", renderer, count);
    if (out) *out = current;
    return true;
  }
  if (identity.paths.empty())
    return reject("Added bone slots require authored bone paths");
  if (!g_transform_get_parent || !g_object_get_name || !il2cpp_array_new ||
      !g_transformClass)
    return reject("Shared skeleton traversal APIs are unavailable");

  std::vector<void *> resolved(items, items + count);
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
    // Only the source-prefix paths are needed to anchor the live skeleton.
    // A renamed source slot is therefore harmless as long as another source
    // slot establishes the same root.  The appended paths are resolved below.
    std::vector<std::string> sourcePrefixPaths;
    sourcePrefixPaths.reserve((std::min)(count, identity.paths.size()));
    for (size_t index = 0; index < count && index < identity.paths.size(); ++index)
      sourcePrefixPaths.push_back(identity.paths[index]);
    if (!EiemSkinRootPath(sourcePaths,sourcePrefixPaths,root,why))
      return reject(why.c_str());
    auto ancestor=ancestors.find(root);
    if (ancestor==ancestors.end())
      return reject("Shared skeleton root is absent");
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
  if (resolved.size() != payloadSlots)
    return reject("Replacement bone palette size is inconsistent");
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
  auto original = (TraceSetBonesFn)s_origSkinnedMeshSetBones;
  if (original) original(self, bones, methodInfo);
  if (!self || s_eiemApplyingModMeshAssignment) {
    return;
  }

  // Measure the skinning result at the one boundary where the bone palette
  // changes. `localBounds` cannot answer this: it is authored data and does not
  // move when the binding breaks, whereas the skinned extent does.
  //
  // The guard is written out rather than routed through TraceTakeBudget: that
  // function is a stub returning a constant false (asset-path exploration is
  // finished), so MSVC folds `budget(...)` to false and deletes the whole block
  // as dead code at /O2. A diagnostic that silently compiles away is worse than
  // no diagnostic, so this block must stay on a real condition.
  if (InterlockedIncrement(&s_traceSkinProbeCount) <= 200) {
    const EiemSkinProbe::Result measurement = EiemSkinProbe::Measure(self);
    EiemSkinProbe::LogResult(
        EiemIsPartnerRenderer(self) ? "[SKIN-PROBE] phase=setBones partner=1"
                                    : "[SKIN-PROBE] phase=setBones partner=0",
        measurement);
  }

  // A Partner is intentionally not treated as a source override.  Observe
  // its game-owned setter calls separately so a later skin/Animator refresh
  // that replaces the extended palette cannot be mistaken for a successful
  // public bones assignment.  This probe has no write side effects.
  EiemPartnerBonesSnapshot partnerSnapshot;
  if (EiemSnapshotPartnerBones(self, &partnerSnapshot)) {
    const size_t incomingCount = EiemManagedArrayLength(bones);
    void *afterBones = nullptr;
    bool afterRead = false;
    if (g_smr_get_bones && EiemOnUnityThread()) {
      afterBones = Invoke(g_smr_get_bones, self);
      afterRead = true;
    }
    const size_t afterCount = EiemManagedArrayLength(afterBones);
    const bool expectedKnown = partnerSnapshot.expectedBoneCount != 0;
    const bool incomingMatches =
        expectedKnown && incomingCount == partnerSnapshot.expectedBoneCount;
    const bool afterMatches =
        expectedKnown && afterRead &&
        afterCount == partnerSnapshot.expectedBoneCount;
    if (EiemRegistrationTraceFirst("partner-set-bones", "game", self,
                                   partnerSnapshot.sourceRenderer, nullptr,
                                   partnerSnapshot.generation)) {
      Log("[PARTNER-BONES-v101] event=set partner=%p source=%p section=%s "
          "generation=%ld incoming=%p incomingCount=%zu after=%p "
          "afterCount=%zu afterRead=%d expected=%zu incomingMatches=%d "
          "afterMatches=%d visible=%d tid=%lu",
          self, partnerSnapshot.sourceRenderer,
          partnerSnapshot.section[0] ? partnerSnapshot.section : "<unknown>",
          partnerSnapshot.generation, bones, incomingCount, afterBones,
          afterCount, afterRead ? 1 : 0, partnerSnapshot.expectedBoneCount,
          incomingMatches ? 1 : 0, afterMatches ? 1 : 0,
          partnerSnapshot.controlVisible ? 1 : 0,
          (unsigned long)GetCurrentThreadId());
    }
    if (expectedKnown &&
        (!incomingMatches || (afterRead && !afterMatches)) &&
        EiemRegistrationTraceFirst("partner-set-bones-mismatch", "game",
                                   self, partnerSnapshot.sourceRenderer,
                                   nullptr, partnerSnapshot.generation)) {
      Log("[PARTNER-BONES-v101] event=mismatch partner=%p source=%p "
          "section=%s generation=%ld expected=%zu incomingCount=%zu "
          "afterCount=%zu afterRead=%d incomingSameAfter=%d",
          self, partnerSnapshot.sourceRenderer,
          partnerSnapshot.section[0] ? partnerSnapshot.section : "<unknown>",
          partnerSnapshot.generation, partnerSnapshot.expectedBoneCount,
          incomingCount, afterCount, afterRead ? 1 : 0,
          afterRead && afterBones == bones ? 1 : 0);
    }
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
  if (tracked && bones)
    EiemRememberGameSourceBones(self, bones, "SkinnedMeshRenderer.set_bones");
  if (tracked && binding && EiemOnUnityThread() && il2cpp_gchandle_get_target) {
    void *expected = il2cpp_gchandle_get_target(binding);
    if (expected && !EiemManagedObjectArraySame(bones, expected)) {
      char error[256] = {};
      if (!EiemPreserveSourceSkinning(self, expected, error, sizeof(error)))
        Log("[MOD-SKIN] game refresh binding failed renderer=%p error=%s", self, error);
    }
  }
  if (tracked) EiemProbePartnerBoneBindings(self, "source-set-bones");
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
static bool EiemRendererEligibleForRule(void *renderer, void *drawRenderer) {
  if (!drawRenderer) return false;

  bool active = true;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  const bool activeRead =
      g_component_get_gameObject && g_gameObject_get_activeInHierarchy &&
      EiemReadBoxedBool(
           g_gameObject_get_activeInHierarchy,
           Invoke(g_component_get_gameObject, drawRenderer), &active);
  if (activeRead && !active) {
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
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer") &&
      InterlockedIncrement(&s_traceSkinProbeCount) <= 200) {
    const EiemSkinProbe::Result sourceSide =
        EiemSkinProbe::Measure(sourceMeshOwner);
    EiemSkinProbe::LogResult("[SKIN-PROBE] phase=create source=1", sourceSide);
    const EiemSkinProbe::Result partnerSide =
        EiemSkinProbe::Measure(partnerDrawRenderer);
    EiemSkinProbe::LogResult("[SKIN-PROBE] phase=create partner=1", partnerSide);
    // Ask for one sweep of the settled state. Only the first creation arms it,
    // so the measurement lands after the whole model is assembled.
    EiemArmSkinProbeSweep();
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
  if (!model || !path || !path[0] || strstr(path, "typhoea") == nullptr ||
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
        self, "BaseModelViewPart.OnLoadFinish-before-original");
  auto original = (TraceBasePartFinishFn)s_origBasePartFinish;
  if (original) original(self, success, methodInfo);
  if (success) {
    EiemTraceBaseModelPartnerArrays(self, "OnLoadFinish-before-register");
    EiemRegisterBaseModelViewPartInstance(
        self, "BaseModelViewPart.OnLoadFinish");
    EiemTracePartnerWorldBounds(
        self, "BaseModelViewPart.OnLoadFinish",
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
    EiemTraceBaseModelPartnerArrays(self, "OnLoadFinish-after-register");
  }
}

// PostDealLoadedModel is the game's model-assembly boundary. Create configured
// Partner renderers before the original scans the hierarchy, then inspect the
// exact BaseModel arrays it published. Per-renderer initialization callbacks
// must not grow the hierarchy while the game is already walking it.
static void TraceBasePartPostDeal(void *self, void *methodInfo) {
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  EiemAdoptUnityThreadFromAssemblyHook("BaseModelViewPart.PostDealLoadedModel");
  if (model)
    EiemApplyStandaloneRenderRules(
        model, "BaseModelViewPart.PostDealLoadedModel-before");
  auto original = (TraceBasePartPostDealFn)s_origBasePartPostDeal;
  if (original) original(self, methodInfo);
  EiemTracePartnerWorldBounds(
      self, "BaseModelViewPart.PostDealLoadedModel-after",
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemTraceBaseModelPartnerArrays(self, "PostDealLoadedModel-after");
}

// ComplexModelViewPart overrides the virtual method, so a base-class hook is
// not sufficient for the concrete character path. Keep a separate trampoline
// and label to make dispatch visible in the runtime log.
static void TraceComplexPartPostDeal(void *self, void *methodInfo) {
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  EiemAdoptUnityThreadFromAssemblyHook("ComplexModelViewPart.PostDealLoadedModel");
  if (model)
    EiemApplyStandaloneRenderRules(
        model, "ComplexModelViewPart.PostDealLoadedModel-before");
  auto original = (TraceBasePartPostDealFn)s_origComplexPartPostDeal;
  if (original) original(self, methodInfo);
  EiemTraceBaseModelPartnerArrays(
      self, "ComplexModelViewPart.PostDealLoadedModel-after");
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

static bool EiemApplyResolvedRenderRule(void *renderer, void *drawRenderer,
                                        void *mesh,
                                        const char *rendererType,
                                        void *methodInfo,
                                        const EiemResolvedRenderRule &resolved,
                                        bool allowMeshReplacement = true,
                                        bool allowPartnerCreation = false) {
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
      !(allowPartnerCreation && rule.partnerCount))
    return false;

  if (!EiemCaptureOriginal(renderer, drawRenderer, mesh, rendererType, applyMesh, nullptr, rule.shapeCount != 0)) return false;

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
    EiemBounds sourceBounds = {};
    const bool hasSourceBounds =
        EiemModEquals(rendererType, "SkinnedMeshRenderer") &&
        EiemReadBounds(g_smr_get_localBounds, renderer, &sourceBounds);
    void *assignedMesh = nullptr;
    void *assignedBones = nullptr;
    std::shared_ptr<const EiemSkinIdentity> skin;
    EiemUnityRef assignedBonesRoot;
    const bool probeResourceSkin =
        EiemModEquals(rendererType, "SkinnedMeshRenderer") &&
        (strstr(asset, "cloth_01") || strstr(asset, "cloth_02")) &&
        InterlockedIncrement(&s_traceResourceSkinProbeCount) <= 12;
    EiemSkinProbe::Result sourceSkinProbe;
    if (probeResourceSkin)
      sourceSkinProbe = EiemSkinProbe::Measure(drawRenderer);
    char meshError[256] = {};
    bool skinPaletteReady = false;
    if (EiemBuildMeshResource(rule, &assignedMesh, meshError,
                              sizeof(meshError), mesh, &skin) && assignedMesh) {
      skinPaletteReady = true;
      if (skin) {
        // A generated SkinnedMesh carries its own local bone palette.  Unity
        // requires that palette to agree with the Renderer.bones array at the
        // moment sharedMesh is assigned.  Mesh-only replacement still keeps
        // animation and bone ownership in the game: resolve every authored
        // path against this instance's existing Transform hierarchy and assign
        // only that managed Transform[]; no Transform or skeleton is created.
        if (!EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
          skinPaletteReady = false;
          strncpy_s(meshError, sizeof(meshError),
                    "Skinned replacement requires SkinnedMeshRenderer", _TRUNCATE);
        } else {
          skinPaletteReady = skeleton
              ? EiemSkeletonMeshBones(*skin, *skeleton, &assignedBones,
                                      meshError, sizeof(meshError))
              : EiemResolveMeshBones(*skin, renderer, &assignedBones,
                                     meshError, sizeof(meshError));
        }
      }
    }
    if (assignedMesh && skinPaletteReady &&
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
    if (meshApplied) {
      Log("[MOD] %s resource mesh replaced: source=%s asset=%s mesh=%s actual=%p",
          rendererType, source, asset, rule.mesh,
          EiemReadSharedMesh(renderer, rendererType));
      EiemSetReplacementDrawBounds(renderer, rendererType, assignedMesh,
                                   hasSourceBounds ? &sourceBounds : nullptr);
      if (probeResourceSkin) {
        EiemSkinProbe::LogResult("[MOD-SKIN-PROBE] stage=source-before",
                                 sourceSkinProbe);
        const EiemSkinProbe::Result replacementSkinProbe =
            EiemSkinProbe::Measure(drawRenderer);
        EiemSkinProbe::LogResult("[MOD-SKIN-PROBE] stage=replacement-after",
                                 replacementSkinProbe);
      }
    }
  }

  // A mesh rule is a transaction boundary. Material/submesh and shape edits
  // describe the replacement Mesh's slot layout; committing them to the
  // source Renderer after mesh construction failed would corrupt the source
  // asset and leave a rule that can never be restored consistently. Rules
  // without mesh= remain ordinary source material edits.
  const bool resourceCommitted = !applyMesh || meshApplied;
  if (resourceCommitted)
    EiemRememberRuleBinding(renderer, rule);
  else
    Log("[MOD] %s resource rule not bound after mesh failure: source=%s "
        "asset=%s mesh=%s",
        rendererType, source, asset, rule.mesh);
  char error[256] = {};
  if ((rule.materialCount || rule.submeshCount) && resourceCommitted &&
      !EiemMaterialSourceInitActive(drawRenderer)) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(rule, drawRenderer, &materials,
                                             error,
                                             sizeof(error))) {
      Log("[MOD] %s material resource failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
    } else if (materials) {
      const bool captured = EiemCaptureOriginal(
          renderer, drawRenderer, mesh, rendererType, false, &rule);
      const bool assigned = captured && EiemAssignRendererMaterials(
          drawRenderer, materials, error, sizeof(error));
      if (!assigned) {
        Log("[MOD] %s material assignment failed: source=%s asset=%s section=%s error=%s",
            rendererType, source, asset, rule.section,
            error[0] ? error : (captured ? "assignment failed" : "capture failed"));
      } else {
        Log("[MOD-MATERIAL] applied renderer=%p source=%s asset=%s section=%s slots=%u array=%p stage=resource-rule",
            drawRenderer, source, asset, rule.section, rule.materialCount,
            materials);
      }
    }
  } else if ((rule.materialCount || rule.submeshCount) && !resourceCommitted) {
    Log("[MOD] %s resource dependent edits skipped after mesh failure: "
        "source=%s asset=%s mesh=%s materials=%u submeshes=%u",
        rendererType, source, asset, rule.mesh, rule.materialCount,
        rule.submeshCount);
  }
  if (resourceCommitted) {
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
  if (allowPartnerCreation)
    EiemApplyPartners(renderer, drawRenderer, mesh, rendererType, rule);
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
    std::vector<EiemPhysicsIntent> *physicsIntents = nullptr,
    bool allowPartnerCreation = false) {
  if (!meshOwner || !drawRenderer || !mesh || !rendererType) return false;
  if (s_eiemCreatingPartner || EiemIsPartnerRenderer(drawRenderer)) return false;
  if (!EiemRendererEligibleForRule(meshOwner, drawRenderer)) return false;
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
                                       true, allowPartnerCreation);
  }
  return false;
}

static bool EiemApplyRenderRuleSet(void *model,
                                   const std::vector<EiemModRule> &rules,
                                   const char *sourceLabel,
                                    const char *stage, bool *referenced = nullptr,
                                    bool *matched = nullptr,
                                    const std::vector<std::string> *affected = nullptr,
                                    std::vector<EiemPhysicsIntent> *physicsIntents = nullptr,
                                    bool allowPartnerCreation = false) {
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
    // LOD/skin assembly can expose every authored Renderer, including levels
    // that are only being prepared. Rule matching is for the current model
    // view; inactive hierarchy entries are not source hits.
    bool includeInactive = false;
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
                      physicsIntents, allowPartnerCreation))
        ++applied;
    }
  };

  visitType(g_skinnedMeshRendererClass, "SkinnedMeshRenderer");
  visitType(g_meshFilterClass, "MeshFilter");
  s_eiemActivePrefabInstance = previousOwner;
  if (applied)
    Log("[MOD-MESH] applied model=%p components=%zu actions=%u stage=%s",
        model, visited, applied, stage ? stage : "unknown");
  return applied != 0;
}

static bool EiemApplyStandaloneRenderRules(void *model, const char *stage,
                                           bool *matched, const std::vector<std::string> *affected,
                                           std::vector<EiemPhysicsIntent> *physicsIntents,
                                           bool allowPartnerCreation) {
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  return EiemApplyRenderRuleSet(model, rules, "<mesh identity>", stage, nullptr,
                                matched, affected, physicsIntents,
                                allowPartnerCreation);
}

// EntityRenderHelper is the common game-owned assembly boundary for the
// world, NPC and character-preview hierarchies. Its original implementation
// walks the hierarchy and constructs RendererInfo/material/visibility state.
// Create EIEM Partners immediately before that walk so they are discovered by
// the same registry as their source Renderer. The guard only prevents nested
// helper callbacks caused by Unity AddComponent; it does not change the
// game's method or perform any per-frame work.
static void TraceEntityRenderHelperInitRenderAndMaterial(void *self,
                                                           void *methodInfo) {
  auto original = (TraceEntityRenderHelperInitFn)
      s_origEntityRenderHelperInitRenderAndMaterial;
  if (!self) {
    if (original) original(self, methodInfo);
    return;
  }
  if (s_eiemEntityRenderHelperInitGuard) {
    if (original) original(self, methodInfo);
    return;
  }

  s_eiemEntityRenderHelperInitGuard = true;
  EiemAdoptUnityThreadFromAssemblyHook(
      "EntityRenderHelper._InitRenderAndMaterial");
  void *model = nullptr;
  if (g_component_get_gameObject)
    model = Invoke(g_component_get_gameObject, self);
  if (original) original(self, methodInfo);
  // The game's implementation must finish RendererInfo/material/visibility
  // setup before a generated SkinnedMesh is committed.  Applying before the
  // original call observes a partial bone palette and can produce Unity's
  // bindpose mismatch error.
  bool applied = false;
  if (model)
    applied = EiemApplyStandaloneRenderRules(
        model, "EntityRenderHelper._InitRenderAndMaterial-after", nullptr,
        nullptr, nullptr);
  if (applied ||
      kEiemValidationIdentityProbe &&
      EiemRegistrationTraceFirst("entity-helper-init", "after", self,
                                 model, nullptr,
                                 InterlockedCompareExchange(
                                     &s_eiemModGeneration, 0, 0)))
    Log("[MOD-ASSEMBLY-v113] boundary=EntityRenderHelper._InitRenderAndMaterial "
        "helper=%p model=%p resourcesApplied=%d", self, model,
        applied ? 1 : 0);
  s_eiemEntityRenderHelperInitGuard = false;
}

static bool EiemApplyStandaloneRenderRulesToRenderer(
    void *meshOwner, void *drawRenderer, void *mesh,
    const char *rendererType, void *methodInfo, const char *stage,
    bool allowPartnerCreation) {
  (void)stage;
  if (!EiemOnUnityThread()) return false;
  std::vector<EiemModRule> rules;
  EiemFindStandaloneRenderRules(&rules);
  return EiemApplyRenderRuleSetToRenderer(
      nullptr, meshOwner, drawRenderer, mesh, rendererType, methodInfo, rules,
      "<mesh setter>", nullptr, nullptr, nullptr, nullptr,
      allowPartnerCreation);
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
static bool EiemModelHasActiveOwner(const EiemModelInstanceState &state);

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
  } owners[4] = {};
  size_t ownerCount = 0;
  size_t declaredOwnerCount = 0;
  bool active = false;
  uint32_t instanceUid = 0;
  char path[768] = {};
  bool found = false;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &state : s_eiemModelInstances) {
    if (!modelKey || state.model != (void *)modelKey) continue;
    found = true;
    declaredOwnerCount = state.ownerCount;
    active = EiemModelHasActiveOwner(state);
    instanceUid = state.instanceUid;
    strncpy_s(path, sizeof(path), state.path, _TRUNCATE);
    ownerCount = (std::min)(static_cast<size_t>(state.ownerCount),
                            _countof(owners));
    for (size_t index = 0; index < ownerCount; ++index) {
      owners[index].kind = state.owners[index].kind;
      owners[index].owner = state.owners[index].owner;
    }
    break;
  }
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);

  if (!found || !ownerCount) {
    EiemRegistrationTraceRendererOwner(
        renderer, (void *)modelKey, found ? "<model-without-owner>"
                                          : "<unregistered>",
        nullptr, instanceUid, declaredOwnerCount, active, path, stage,
        generation);
    return;
  }
  for (size_t index = 0; index < ownerCount; ++index)
    EiemRegistrationTraceRendererOwner(
        renderer, (void *)modelKey,
        EiemModelOwnerKindName(owners[index].kind), owners[index].owner,
        instanceUid, declaredOwnerCount, active, path, stage, generation);
}

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
    for (uint32_t index = 0; index < state.ownerCount; ++index) {
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
    const char *prefabPath, uint32_t instanceUid, const char *stage) {
  if (!model) return false;
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
  ownerCountSnapshot = state.ownerCount;
  ownerActiveSnapshot = EiemModelHasActiveOwner(state);
  ReleaseSRWLockExclusive(&s_eiemModelInstanceLock);
  for (uintptr_t released : releasedModels) {
    EiemReleaseModelPhysics((void *)released, "owner moved to another model");
    EiemRegistrationTraceRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)released,
        "owner moved to another model", generationSnapshot);
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
  EiemRegistrationTraceModel(
      EiemModelOwnerKindName(ownerKind), owner, model, stage,
      generationSnapshot, ownerCountSnapshot, ownerActiveSnapshot, applied,
      -1, -1, prefabPath);
  EiemStoreModelPhysicsIntents(model, std::move(physicsIntents), stage);
  if (applied) {
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
  // Publish all Partners into the same parallel caches that the game uses for
  // the source model. This runs after resource application because that pass
  // is what creates the Partners; the surrounding completion hook retries it
  // after the game has populated its arrays.
  EiemRegisterPartnersInBaseModelArrays(part, stage);
  if (applied || kEiemValidationIdentityProbe)
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
    for (uint32_t ownerIndex = 0; ownerIndex < state.ownerCount;
         ++ownerIndex) {
      if (state.owners[ownerIndex].kind != ownerKind ||
          state.owners[ownerIndex].owner != owner)
        continue;
      for (uint32_t move = ownerIndex + 1; move < state.ownerCount; ++move)
          state.owners[move - 1] = state.owners[move];
      --state.ownerCount;
      removed = true;
      break;
    }
    if (removed && state.ownerCount && wasActive != EiemModelHasActiveOwner(state) &&
        !state.physicsIntents.empty())
      physicsStates.push_back(
          {state.model, state.physicsIntents, EiemModelHasActiveOwner(state)});
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
    EiemRegistrationTraceRelease(
        EiemModelOwnerKindName(ownerKind), owner, (void *)modelOwner, stage,
        InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
    EiemDestroyPartnerObjects(modelOwner);
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
  EiemDestroyPartnerObjects(modelOwner);
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
    // RendererInfo._Init is a per-Renderer callback and can run while the game
    // is iterating its skin assembly. Apply in-place resource/material actions
    // here, but defer hierarchy-growing Partner creation to the enclosing
    // BaseModel/CreateSMS assembly boundary.
    bool applied = false;
    if (EiemIsSkinnedRenderer(renderer)) {
      void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
      if (mesh)
        applied = EiemApplyStandaloneRenderRulesToRenderer(
            renderer, renderer, mesh, "SkinnedMeshRenderer", methodInfo,
            "RendererInfo._Init", false);
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

// Observe both game-owned and EIEM-owned LOD array writes. The hook is
// read-only: the original setter runs first, then the resulting array size is
// recorded at the same boundary as the partner membership trace.
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
// assembly passes to CreateSMSGO/AssignSkinGo. The existing membership probe
// compares only exact pointers against the Partner table; that is insufficient
// when the game has a duplicate/intermediate Renderer carrying the same Mesh.
// Keep this bounded and de-duplicated so ordinary world traffic cannot turn the
// diagnostic DLL into a per-frame logger.
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
          "asset=%s meshDescription=%s sourceKnown=%d partnerKnown=%d "
          "hierarchy=%s generation=%ld",
          EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner,
          array, lod, index, renderer,
          rendererType && rendererType[0] ? rendererType : "Renderer",
          rendererName[0] ? rendererName : "<unknown>", mesh,
          asset[0] ? asset : "<unknown>",
          meshDescription[0] ? meshDescription : "<unknown>",
          EiemIsKnownSourceRenderer(renderer) ? 1 : 0,
          EiemIsPartnerRenderer(renderer) ? 1 : 0,
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
  // The world path must remain observational here. In particular, do not
  // append Partners or rewrite root-bone data before proving that this is the
  // game's own skin assembly boundary.
  EiemTraceKnownPartnerParallelArrays("AssignSkinGoPre", renderers, rootBones,
                                      generation);
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinGoPre", nullptr, renderers, EiemManagedArrayLength(renderers),
      generation, lod);
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinGo;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  // At this point the game's own AssignSkin has populated the Renderer bone
  // palette.  Commit resource rules only now, using that completed array.
  const size_t resourcesApplied =
      EiemApplyStandaloneRenderRulesToSkinArray(
          renderers, "NPCAvatarCreatorUtils.AssignSkinGoPost");
  const LONG afterGeneration =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinGoPost", nullptr, renderers,
      EiemManagedArrayLength(renderers), afterGeneration, lod);
  EiemTraceSkinArrayItems("AssignSkinGoPost", nullptr, renderers, lod,
                          afterGeneration);
  EiemTraceKnownPartnerArrayMembers(
      "AssignSkinGoPost", nullptr, renderers,
      EiemManagedArrayLength(renderers), afterGeneration);
  if (resourcesApplied)
    Log("[MOD-ASSEMBLY-v114] boundary=AssignSkinGoPost resourcesApplied=%zu",
        resourcesApplied);
}

static void TraceAssignSkinPost(int32_t lod, void *renderers,
                                void *rootBones, void *closure,
                                void *methodInfo) {
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemTraceKnownPartnerParallelArrays("AssignSkinPre", renderers, rootBones,
                                      generation);
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinPost;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  const size_t resourcesApplied =
      EiemApplyStandaloneRenderRulesToSkinArray(
          renderers, "NPCAvatarCreatorUtils.AssignSkinPost");
  EiemRegistrationTraceArrayBoundary(
      "AssignSkinPost", nullptr, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
  EiemTraceKnownPartnerArrayMembers(
      "AssignSkinPost", nullptr, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemRememberGameSourceSkinningFromArray(
      renderers, true, true, "AssignSkinPost");
  EiemSyncPartnerRootBonesFromArray(renderers);
  EiemProbePartnerBoneBindings(nullptr, "assign-skin");
  if (resourcesApplied)
    Log("[MOD-ASSEMBLY-v114] boundary=AssignSkinPost resourcesApplied=%zu",
        resourcesApplied);
}

static void TraceSetSmrRootBone(void *animator, void *renderers,
                                void *rootBoneInfos, void *methodInfo) {
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemTraceKnownPartnerParallelArrays("SetSMRRootBonePre", renderers,
                                      rootBoneInfos, generation);
  auto original = (TraceSetSmrRootBoneFn)s_origSetSmrRootBone;
  if (original) original(animator, renderers, rootBoneInfos, methodInfo);
  EiemRegistrationTraceArrayBoundary(
      "SetSMRRootBone", animator, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), -1);
  EiemTraceKnownPartnerArrayMembers(
      "SetSMRRootBone", animator, renderers, EiemManagedArrayLength(renderers),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemRememberGameSourceSkinningFromArray(
      renderers, false, true, "SetSMRRootBone");
  EiemSyncPartnerRootBonesFromArray(renderers);
  EiemProbePartnerBoneBindings(nullptr, "set-root-bone");
}

// CreateSMS returns the exact SkinnedMeshRenderer array for one NPC model and
// one LOD. Apply Mesh-identity rules to those concrete source instances before
// extending that same array with their Partners. The caller continues into
// AssignSkin only after this hook returns, so every new Renderer enters the
// game's normal skin assembly as part of the owning instance.
static size_t EiemApplyStandaloneRenderRulesToSkinArray(
    void *renderers, const char *stage) {
  const size_t count = EiemManagedArrayLength(renderers);
  if (!renderers || !count || count > 8192) return 0;
  void **items = (void **)((char *)renderers + IL2CPP_ARRAY_DATA);
  size_t applied = 0;
  for (size_t index = 0; index < count; ++index) {
    void *renderer = items[index];
    if (!renderer || EiemIsPartnerRenderer(renderer)) continue;
    void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
    if (mesh && EiemApplyStandaloneRenderRulesToRenderer(
                    renderer, renderer, mesh, "SkinnedMeshRenderer", nullptr,
                    stage, false))
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
  EiemRegisterPartnersInSkinArrays(
      renderers, rootBones,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
      "CreateSMSGO");
  array = renderers ? *renderers : nullptr;
  EiemTraceKnownPartnerParallelArrays(
      "CreateSMSGOPost", array, rootBones ? *rootBones : nullptr,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemRegistrationTraceArrayBoundary(
      "CreateSMSGO", meshAssets, array, EiemManagedArrayLength(array),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
  EiemTraceSkinArrayItems(
      "CreateSMSGO", meshAssets, array, lod,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemTraceKnownPartnerArrayMembers(
      "CreateSMSGO", meshAssets, array, EiemManagedArrayLength(array),
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
  EiemRegisterPartnersInSkinArrays(
      renderers, rootBones,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0),
      "CreateSMSInfoForPostModel");
  array = renderers ? *renderers : nullptr;
  EiemTraceKnownPartnerParallelArrays(
      "CreateSMSInfoForPostModelPost", array,
      rootBones ? *rootBones : nullptr,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
  EiemRegistrationTraceArrayBoundary(
      "CreateSMSInfoForPostModel", meshAssets, array,
      EiemManagedArrayLength(array),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), lod);
  EiemTraceKnownPartnerArrayMembers(
      "CreateSMSInfoForPostModel", meshAssets, array,
      EiemManagedArrayLength(array),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0));
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

static constexpr UINT_PTR kEiemShapeTransitionTimer = 0xE153;
static ULONGLONG s_eiemShapeTransitionTick = 0;
// Partner skinning is measured once at creation, but the reported failure --
// a mesh lying on the ground -- is a later state: the source Animator assigns
// bone arrays after the Mod builds the Renderer, and that is what can displace
// the mesh. Arm a one-shot sweep so the same Renderers are measured again once
// the character has settled, and trigger the frame timer for it.
static ULONGLONG s_eiemSkinProbeArmTick = 0;
static bool s_eiemSkinProbeSweepDone = false;
static constexpr ULONGLONG kEiemSkinProbeSettleMs = 1500;
static volatile LONG s_traceSkinProbeSweepCount = 0;

static void EiemArmSkinProbeSweep() {
  if (s_eiemSkinProbeArmTick) return;
  s_eiemSkinProbeArmTick = GetTickCount64();
  if (g_gameHwnd && IsWindow(g_gameHwnd))
    SetTimer(g_gameHwnd, kEiemShapeTransitionTimer, 16, nullptr);
}

static bool EiemAnyShapeTransitions() {
  bool active = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  for (const auto &state : s_eiemOverrides)
    if (EiemShapeStateAnimating(state.shapes)) { active = true; break; }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (active) return true;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  for (const auto &state : s_eiemPartners)
    if (EiemShapeStateAnimating(state.shapes)) { active = true; break; }
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  return active;
}

static void EiemRefreshShapeTransitionTimer() {
  if (!g_gameHwnd || !IsWindow(g_gameHwnd)) return;
  // The probe sweep needs the same frame pacing as an animating shape.
  if (EiemAnyShapeTransitions() || !s_eiemSkinProbeSweepDone) {
    if (!s_eiemShapeTransitionTick) s_eiemShapeTransitionTick = GetTickCount64();
    SetTimer(g_gameHwnd, kEiemShapeTransitionTimer, 16, nullptr);
  } else {
    KillTimer(g_gameHwnd, kEiemShapeTransitionTimer);
    s_eiemShapeTransitionTick = 0;
  }
}

// Measure every Partner again, after the game's own skin/Animator passes have
// run. This is the state the player actually sees, and the only point where a
// mesh that lies on the ground can be compared against one that does not.
static void EiemRunSkinProbeSweep() {
  if (s_eiemSkinProbeSweepDone || !s_eiemSkinProbeArmTick) return;
  if (GetTickCount64() - s_eiemSkinProbeArmTick < kEiemSkinProbeSettleMs) return;
  s_eiemSkinProbeSweepDone = true;
  std::vector<void *> partners;
  AcquireSRWLockShared(&s_eiemPartnerLock);
  partners.reserve(s_eiemPartners.size());
  for (const auto &state : s_eiemPartners)
    if (state.partnerRenderer) partners.push_back(state.partnerRenderer);
  ReleaseSRWLockShared(&s_eiemPartnerLock);
  Log("[SKIN-PROBE] phase=settled sweep partners=%zu", partners.size());
  for (void *partner : partners) {
    if (!partner || InterlockedIncrement(&s_traceSkinProbeSweepCount) > 400) break;
    const EiemSkinProbe::Result measurement = EiemSkinProbe::Measure(partner);
    EiemSkinProbe::LogResult("[SKIN-PROBE] phase=settled partner=1", measurement);
  }
}

static void EiemRunShapeTransitions() {
  EiemRunSkinProbeSweep();
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
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  for (auto &state : s_eiemPartners) {
    if (!EiemShapeStateAnimating(state.shapes)) continue;
    EiemModRule rule = {};
    if (EiemFindRenderRuleBySection(state.modPath, state.section, &rule))
      EiemUpdateRendererShapes(state.partnerRenderer, state.rendererType, rule,
                               state.shapes, elapsed);
  }
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  EiemRefreshShapeTransitionTimer();
}


static EiemModUpdateQueue s_eiemModUpdates;
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
  EiemRegistrationTraceInput(chord.vk, chord.modifiers, generation,
                             event.uiFocus, nullptr, nullptr, 0);
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
  // A stale wake-up message must not replay every registered model. The
  // request bitmask is the transaction authority; an empty mask is a no-op.
  if (!requests) return;
  // This function is dispatched from the game's window procedure, which is
  // the safe Unity thread for creating generated Mesh/Material/Texture
  // objects. Startup hooks may run on the plugin worker thread instead.
  if (!s_eiemUnityThreadId) s_eiemUnityThreadId = GetCurrentThreadId();
  if (!g_gameObject_GetComponentsInChildren) {
    if (requests & (uint32_t)EiemModUpdate::Reload) {
      LoadEiemConfig();
      EiemReportCameraFade();
    }
    s_eiemModUpdates.Requeue(requests);
    if (g_gameHwnd)
      SetTimer(g_gameHwnd, kEiemModRetryTimer, 100, nullptr);
    Log("[MOD] Reconcile skipped: renderer APIs are not ready");
    return;
  }
  std::vector<EiemModInputEvent> inputs;
  AcquireSRWLockExclusive(&s_eiemInputLock);
  inputs.swap(s_eiemPendingInputs);
  ReleaseSRWLockExclusive(&s_eiemInputLock);
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceReconcile("begin", requests, generation, inputs.size(),
                                 0, 0, 0);
  const bool reload = (requests & (uint32_t)EiemModUpdate::Reload) != 0;
  const ULONGLONG reconcileStarted = GetTickCount64();
  if (reload)
    Log("[MOD-RELOAD-TRACE] begin requests=0x%X", requests);
  EiemModProgram next;
  std::vector<std::string> affectedMods;
  const std::vector<std::string> *affected = nullptr;
  bool partnerLinksOnly = false;
  bool submeshVisibilityOnly = false;
  if (!reload && !inputs.empty()) {
    bool shapesOnly = false;
    if (EiemPrepareInputUpdate(inputs, &next, &affectedMods, &shapesOnly,
                               &partnerLinksOnly, &submeshVisibilityOnly)) {
      if (shapesOnly && !(requests & (uint32_t)EiemModUpdate::Reconcile)) {
        EiemPublishModState(std::move(next));
        EiemReapplyShapeControls(affectedMods);
        EiemRefreshShapeTransitionTimer();
        EiemRegistrationTraceReconcile(
            "end", requests, generation, inputs.size(), 0, 0,
            GetTickCount64() - reconcileStarted);
        return;
      }
      if (submeshVisibilityOnly &&
          !(requests & (uint32_t)EiemModUpdate::Reconcile)) {
        // The source Renderer and its skinning already own the current Mesh.
        // Rebuild only the visibility variant; do not restore/destroy model
        // objects or replay Physics just because a key changed a submesh mask.
        EiemPublishModState(std::move(next));
        EiemPruneModelInstances();
        std::vector<EiemModelInstanceState> instances;
        AcquireSRWLockShared(&s_eiemModelInstanceLock);
        instances = s_eiemModelInstances;
        ReleaseSRWLockShared(&s_eiemModelInstanceLock);
        uint32_t matched = 0;
        for (const auto &instance : instances) {
          if (!instance.model || instance.modelRef.Status() != 1) continue;
          if (EiemApplyStandaloneRenderRules(
                  instance.model, "submesh visibility key", nullptr,
                  &affectedMods, nullptr, false))
            ++matched;
        }
        Log("[MOD] Submesh visibility reconcile: models=%zu matched=%u",
            instances.size(), matched);
        EiemRefreshShapeTransitionTimer();
        EiemRegistrationTraceReconcile(
            "end", requests, generation, inputs.size(), instances.size(),
            matched, GetTickCount64() - reconcileStarted);
        return;
      }
      affected = &affectedMods;
    } else if (!(requests & (uint32_t)EiemModUpdate::Reconcile)) {
      EiemRegistrationTraceReconcile(
          "end", requests, generation, inputs.size(), 0, 0,
          GetTickCount64() - reconcileStarted);
      return;
    }
  }
  // Native Physics teardown/collection belongs to this same Unity-thread
  // transaction. There is no background candidate scan. Lightweight shape
  // or submesh visibility keys have already returned above and must not enter
  // this boundary at all.
  EiemPhysicsRuntimeBoundary("mod reconcile begin");
  s_eiemPhysicsLifecycleTransaction = true;
  EiemDispatchModUpdate(requests, [&] {
    const ULONGLONG phaseStarted = GetTickCount64();
    if (reload)
      Log("[MOD-RELOAD-TRACE] phase=restore begin");
    if (reload) {
      EiemPhysicsRuntimeRetireChangedAssets("mod reload changed Physics");
      Log("[MOD-RELOAD-TRACE] Physics remains owned by model generation; "
          "only changed intents retire during replay");
    }
    if (partnerLinksOnly) {
      EiemApplyPartnerControlVisibility(affected, next);
    } else {
      // F10 changes resource/config generations while the game's model and
      // skin instances remain alive. Keep existing Partners in place; the
      // replay path refreshes them in place so their internal skin/LOD
      // registration is not lost. Owner teardown still destroys them through
      // EiemForgetModelOwner/EiemForgetModelInstance.
      if (!reload) EiemDestroyPartnerObjects(affected);
      EiemRestoreRenderOverrides(affected);
    }
    EiemCollectSkeletonInstances();
    // Configuration reload must not release Unity Mesh objects that may still
    // be referenced by a Renderer. The resource backend reuses unchanged files
    // and creates a new rooted object only when the file stamp changes.
    if (reload)
      Log("[MOD-RELOAD-TRACE] phase=restore end elapsed=%llums",
          GetTickCount64() - phaseStarted);
  }, [] {
    const ULONGLONG phaseStarted = GetTickCount64();
    Log("[MOD-RELOAD-TRACE] phase=load begin");
    LoadEiemConfig(); EiemReportCameraFade(); EiemReloadMods();
    Log("[MOD-RELOAD-TRACE] phase=load end elapsed=%llums",
        GetTickCount64() - phaseStarted);
  }, [&] {
   if (affected) EiemPublishModState(std::move(next));
   const char *stage = reload ? "global reload" : affected ? "control state change" : "lifecycle reconcile";
   const ULONGLONG started = GetTickCount64();
   EiemPruneModelInstances();
  std::vector<EiemModelInstanceState> instances;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  instances = s_eiemModelInstances;
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
   if (reload)
     Log("[MOD-RELOAD-TRACE] phase=replay begin models=%zu", instances.size());
   uint32_t matched = 0;
   size_t replayIndex = 0;
   // A partner-link key transaction changes only Renderer.enabled.  Replaying
   // every model here would also re-store Physics intents and can start a
   // native Physics generation merely because a hidden Partner was shown.
   // Resource/Physics replay remains owned by F10 or a lifecycle boundary.
   if (partnerLinksOnly) {
     Log("[MOD] Reconcile partner visibility only: skipped model resource/Physics replay");
   } else {
     for (const auto &instance : instances) {
       if (!instance.model) continue;
       if (reload)
         Log("[MOD-RELOAD-TRACE] model=%zu/%zu begin ptr=%p path=%s",
             replayIndex + 1, instances.size(), instance.model,
             instance.path[0] ? instance.path : "<unknown>");
       if (instance.modelRef.Status() != 1) {
         Log("[MOD-LIFECYCLE] Cannot validate observed model=%p; not applying rules", instance.model);
         ++replayIndex;
         continue;
       }
       std::vector<EiemPhysicsIntent> physicsIntents;
       bool applied = EiemApplyStandaloneRenderRules(
           instance.model, stage, nullptr, affected, &physicsIntents);
       EiemStoreModelPhysicsIntents(instance.model, std::move(physicsIntents),
                                    stage);
       if (applied) ++matched;
       if (reload)
         Log("[MOD-RELOAD-TRACE] model=%zu/%zu end ptr=%p matched=%d elapsed=%llums",
             replayIndex + 1, instances.size(), instance.model, applied ? 1 : 0,
             GetTickCount64() - reconcileStarted);
       ++replayIndex;
     }
   }
  const ULONGLONG elapsed = GetTickCount64() - started;
  Log("[MOD] Reconcile complete: registeredModels=%zu matchedModels=%u elapsed=%llums",
      instances.size(), matched, elapsed);
  EiemCollectSkeletonInstances();
  EiemRefreshShapeTransitionTimer();
  if (reload)
    Log("[MOD-RELOAD-TRACE] end elapsed=%llums",
        GetTickCount64() - reconcileStarted);
  EiemRegistrationTraceReconcile(
      "end", requests,
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), inputs.size(),
      instances.size(), matched, elapsed);
  EiemPhysicsRuntimeBoundary("mod reconcile end");
  });
  s_eiemPhysicsLifecycleTransaction = false;
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
    // The path is what ties a proxy to a Mesh asset. Every Prefab loads the same
    // Mesh set, so the path is how the UI, world and NPC chains are told apart;
    // the handle alone does not identify the asset.
    char pathText[512] = {};
    auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
    if (pathGetter) {
      TraceDescribeString(pathGetter(self, nullptr), pathText, sizeof(pathText));
    }
    if (pathText[0] && strstr(pathText, "typhoea") != nullptr) {
      Log("[RES-TRACE] ProxyGet handle=%p proxy=%p type=%s path=\"%s\"", self,
          result, name ? name : "?", pathText);
    }
    s_traceReentrant = false;
  }
  return result;
}

static bool TraceIdentityTextMatchesTyphoea(const char *text) {
  return text && (strstr(text, "typhoea") || strstr(text, "Typhoea"));
}

static bool TraceTakeTargetBudget(volatile LONG *counter, LONG limit,
                                  const char *primary,
                                  const char *secondary) {
  if (!kEiemValidationIdentityProbe ||
      (!TraceIdentityTextMatchesTyphoea(primary) &&
       !TraceIdentityTextMatchesTyphoea(secondary)))
    return false;
  return InterlockedIncrement(counter) <= limit;
}

static void TraceSubMeshInfoIdentity(void *info, const char *event,
                                     void *meshOverride) {
  if (!info || !kEiemValidationIdentityProbe) return;
  __try {
    void *nameObject = *(void **)((char *)info + 0x30);
    char name[192] = {};
    if (nameObject) ReadStrUtf8(nameObject, name, sizeof(name));
    if (!TraceIdentityTextMatchesTyphoea(name)) return;
    void *mesh = meshOverride ? meshOverride
                              : *(void **)((char *)info + 0x10);
    const int64_t pathHash = *(int64_t *)((char *)info + 0x28);
    const int active = *(bool *)((char *)info + 0x58) ? 1 : 0;
    const int disabled = *(bool *)((char *)info + 0x6D) ? 1 : 0;
    const int32_t rootBoneId = *(int32_t *)((char *)info + 0x68);
    Log("[V1.1-DESCRIPTOR] event=%s info=%p name=%s mesh=%p "
        "meshPathHash=%lld active=%d rendererDisabled=%d rootBoneID=%d",
        event ? event : "unknown", info, name[0] ? name : "<empty>", mesh,
        (long long)pathHash, active, disabled, rootBoneId);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

static void *TraceV11DescriptorGetMesh(void *self, void *methodInfo) {
  auto original = (TraceV11DescriptorGetMeshFn)s_origSubMeshInfoGetMesh;
  void *result = original ? original(self, methodInfo) : nullptr;
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
  if (!self || !kEiemValidationIdentityProbe) return result;
  __try {
    void *nameObject = *(void **)((char *)self + 0x10);
    char ownerName[192] = {};
    if (nameObject) ReadStrUtf8(nameObject, ownerName, sizeof(ownerName));
    if (!TraceIdentityTextMatchesTyphoea(ownerName)) return result;
    const size_t count = EiemManagedArrayLength(result);
    Log("[V1.1-DESCRIPTOR] event=NPCAvatarLodMeshAssets.GetSubMeshInfo "
        "owner=%p ownerName=%s lod=%d gpu=%d array=%p count=%zu",
        self, ownerName[0] ? ownerName : "<empty>", lod, gpu ? 1 : 0,
        result, count);
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
    if (!TraceIdentityTextMatchesTyphoea(path)) {
      void *nameObject = *(void **)((char *)self + 0x50);
      if (nameObject) ReadStrUtf8(nameObject, path, sizeof(path));
    }
    if (TraceIdentityTextMatchesTyphoea(path))
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
  if (!kEiemValidationIdentityProbe) return result;
  char path[768] = {};
  auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyLoaderHandlePath;
  if (pathGetter) TraceDescribeString(pathGetter(self, nullptr), path,
                                       sizeof(path));
  char objectName[192] = {};
  TraceReadUnityObjectName(result, objectName, sizeof(objectName));
  if (TraceIdentityTextMatchesTyphoea(path) ||
      TraceIdentityTextMatchesTyphoea(objectName))
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
  if (TraceIdentityTextMatchesTyphoea(path))
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
  if (TraceIdentityTextMatchesTyphoea(path))
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
  if (!TraceIdentityTextMatchesTyphoea(pathText) &&
      !TraceIdentityTextMatchesTyphoea(resultText))
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
  if (!TraceIdentityTextMatchesTyphoea(pathText)) return result;
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
    if (pathText[0] && strstr(pathText, "typhoea") != nullptr) {
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
  TraceRememberMeshObservation(self, mesh, "SkinnedMeshRenderer");
  void *sourceMesh = mesh;
  // A later game-side LOD/skin refresh may assign the original Mesh again.
  // Preserve an existing binding; otherwise this assignment is also a precise
  // lifecycle event at which standalone Mesh-identity rules can be evaluated.
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  // This setter is also used while the game's skin/LOD assembly is only
  // partially populated.  It remains observation/reassertion-only; resource
  // rules are committed at the completed assembly boundaries instead.
  char identityText[768] = {};
  TraceLookupAssetOrigin(mesh, nullptr, identityText,
                         sizeof(identityText));
  if (!TraceIdentityTextMatchesTyphoea(identityText))
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

static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                         void *methodInfo) {
  auto original = (TraceSetSharedMeshFn)s_origMeshFilterSetSharedMesh;
  if (s_eiemApplyingModMeshAssignment) {
    if (original) original(self, mesh, methodInfo);
    return;
  }
  TraceRememberMeshObservation(self, mesh, "MeshFilter");
  void *sourceMesh = mesh;
  void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
  if (retained) mesh = retained;
  if (original) original(self, mesh, methodInfo);
  // MeshFilter follows the same rule as SkinnedMeshRenderer: do not mutate a
  // resource from a low-level setter before the owning game assembly returns;
  // commit only at completed assembly boundaries.
  char identityText[768] = {};
  TraceLookupAssetOrigin(mesh, nullptr, identityText,
                         sizeof(identityText));
  if (!TraceIdentityTextMatchesTyphoea(identityText))
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
  Log("[VALIDATION] mode=static-resource-baseline metadata-enumeration=off "
      "part-table-mutation=off");
  EiemInitUnityLifetime(assemblies, assemblyCount);
  EiemInstallNpcModelOwner(assemblies, assemblyCount);

  // Install this before the per-Renderer material hook. EntityRenderHelper's
  // original _InitRenderAndMaterial builds the internal renderer registry by
  // scanning its hierarchy; Partners must already be present at that point.
  // This single boundary is shared by world, NPC and character-preview model
  // paths, so no context-specific array mutation is needed for assembly.
  void *entityRenderHelperClass = FindClass(
      "Beyond.Gameplay.View", "EntityRenderHelper", assemblies,
      assemblyCount);
  if (entityRenderHelperClass) {
    HookTraceMethod(
        entityRenderHelperClass, "_InitRenderAndMaterial", 0,
        "EntityRenderHelper._InitRenderAndMaterial",
        (void *)TraceEntityRenderHelperInitRenderAndMaterial,
        &s_origEntityRenderHelperInitRenderAndMaterial);
  } else {
    Log("[RES-TRACE] Beyond.Gameplay.View.EntityRenderHelper class not found");
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
    const char *partMeshesFields[] = {"m_meshes"};
    const char *partMeshesInitStateFields[] = {"m_meshesInitState"};
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
    s_basePartMeshesOffset = FindFieldInHierarchy(
        basePartClass, partMeshesFields, _countof(partMeshesFields), nullptr);
    s_basePartMeshesInitStateOffset = FindFieldInHierarchy(
        basePartClass, partMeshesInitStateFields,
        _countof(partMeshesInitStateFields), nullptr);
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
        "meshes=0x%X meshStates=0x%X lodGroups=0x%X",
        s_basePartModelOffset, s_basePartConfigOffset,
        s_basePartConfigPathOffset, s_basePartRenderersOffset,
        s_basePartRenderersInitStateOffset, s_basePartMeshesOffset,
        s_basePartMeshesInitStateOffset, s_basePartLodGroupsOffset);
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
