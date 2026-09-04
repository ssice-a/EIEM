#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "eiem_mods.h"
#include "eiem_resource_backend.h"

// Resource-loading hooks preserve the game's VFS/decryption pipeline and
// observe logical identities. At the final proxy object boundary they can
// redirect declared Mesh/Material/Texture resources; VFS path hooks retain
// their plaintext-bundle diagnostic override as a separate legacy probe.

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
static thread_local bool s_vfsThreadAttached = false;
static thread_local bool s_traceReentrant = false;
static thread_local bool s_traceBuildingGlobalResource = false;
static volatile LONG s_traceSetterThreadLogged = 0;
static volatile LONG s_smsArrayProbeLogged = 0;
static volatile LONG s_traceLogicalMeshFlowCount = 0;
static volatile LONG s_traceSubMeshSetterCount = 0;
static volatile LONG s_traceBonesSetterCount = 0;
static volatile LONG s_traceHgDataCount = 0;
static volatile LONG s_traceHgStateCount = 0;
static volatile LONG s_traceCharacterFlowCount = 0;
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
static void TraceRememberAssetOrigin(void *asset, int64_t pathHash,
                                     const char *path);
static bool TraceTakeBudget(volatile LONG *counter, LONG limit);
static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize);
static void TraceReadUnityObjectName(void *object, char *out, int outSize);
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
// renderer array has been created. Keep this as an observation boundary; Mesh
// replacement must already have happened before this stage.
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
static void *s_prefabInstantiateGetGameObject = nullptr;
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
static int s_baseModelIdOffset = -1;
static int s_baseModelPathOffset = -1;
static int s_basePartModelOffset = -1;
static int s_basePartConfigOffset = -1;
static int s_basePartConfigPathOffset = -1;
static int s_subMeshInfoMeshNameOffset = -1;
static int s_subMeshInfoPathHashOffset = -1;
static int s_lodMeshAssetNameOffset = -1;
static thread_local bool s_eiemApplyingSubMeshAssignment = false;
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
static void TraceApplyLoadedModelRenderers(void *model, int64_t pathHash,
                                           const char *stage,
                                           const char *explicitPath = nullptr);

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

struct EiemRenderOverrideState {
  void *renderer = nullptr;
  void *originalMesh = nullptr;
  void *replacementMesh = nullptr;
  uint32_t originalMaterialsHandle = 0;
  uint32_t originalBonesHandle = 0;
  uint32_t originalRootBoneHandle = 0;
  bool originalEnabled = true;
  bool hasEnabled = false;
  bool hasMaterials = false;
  bool hasSkinning = false;
  char rendererType[32] = {};
};
struct EiemBounds {
  Vector3 center;
  Vector3 extents;
};
static SRWLOCK s_eiemOverrideLock = SRWLOCK_INIT;
static std::vector<EiemRenderOverrideState> s_eiemOverrides;
static volatile LONG s_eiemAppliedModGeneration = -1;

struct EiemSubMeshOverrideState {
  void *info = nullptr;
  uint32_t infoHandle = 0;
  void *originalMesh = nullptr;
  void *replacementMesh = nullptr;
  bool originalWasNull = false;
};
static SRWLOCK s_eiemSubMeshOverrideLock = SRWLOCK_INIT;
static std::vector<EiemSubMeshOverrideState> s_eiemSubMeshOverrides;

static size_t EiemFindSubMeshOverrideLocked(void *info) {
  for (size_t i = 0; i < s_eiemSubMeshOverrides.size(); ++i)
    if (s_eiemSubMeshOverrides[i].info == info) return i;
  return SIZE_MAX;
}

static void EiemRememberSubMeshReplacement(void *info, void *originalMesh,
                                           void *replacementMesh) {
  if (!info || !replacementMesh) return;
  AcquireSRWLockExclusive(&s_eiemSubMeshOverrideLock);
  size_t index = EiemFindSubMeshOverrideLocked(info);
  if (index == SIZE_MAX) {
    EiemSubMeshOverrideState state = {};
    state.info = info;
    state.infoHandle = il2cpp_gchandle_new ? il2cpp_gchandle_new(info, false) : 0;
    state.originalMesh = originalMesh;
    state.replacementMesh = replacementMesh;
    state.originalWasNull = originalMesh == nullptr;
    s_eiemSubMeshOverrides.push_back(state);
  } else {
    // A logical record can be observed first with mesh=null and later with
    // the game's source Mesh. Preserve the source once it becomes available.
    if (originalMesh && originalMesh != s_eiemSubMeshOverrides[index].replacementMesh) {
      s_eiemSubMeshOverrides[index].originalMesh = originalMesh;
      s_eiemSubMeshOverrides[index].originalWasNull = false;
    }
    s_eiemSubMeshOverrides[index].replacementMesh = replacementMesh;
  }
  ReleaseSRWLockExclusive(&s_eiemSubMeshOverrideLock);
}

static bool EiemRestoreOneSubMeshOverride(TraceSubMeshInfoSetMeshFn original,
                                          void *info, void *mesh) {
  if (!original || !info || !mesh) return false;
  __try {
    original(info, mesh, nullptr);
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("[MOD-SUBMESH] restore failed info=%p exception=0x%08lX", info,
        GetExceptionCode());
    return false;
  }
}

static void EiemClearSubMeshInfoMesh(void *info) {
  if (!info || s_subMeshInfoMeshOffset < 0) return;
  __try { *(void **)((char *)info + s_subMeshInfoMeshOffset) = nullptr; }
  __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void EiemRestoreSubMeshOverrides() {
  std::vector<EiemSubMeshOverrideState> states;
  AcquireSRWLockExclusive(&s_eiemSubMeshOverrideLock);
  states.swap(s_eiemSubMeshOverrides);
  ReleaseSRWLockExclusive(&s_eiemSubMeshOverrideLock);
  auto original = (TraceSubMeshInfoSetMeshFn)s_origSubMeshInfoSetMesh;
  if (!original) return;
  for (const auto &state : states) {
    void *info = state.infoHandle && il2cpp_gchandle_get_target
                     ? il2cpp_gchandle_get_target(state.infoHandle)
                     : state.info;
    if (!info || (!state.originalMesh && !state.originalWasNull)) {
      if (state.infoHandle && il2cpp_gchandle_free)
        il2cpp_gchandle_free(state.infoHandle);
      continue;
    }
    s_eiemApplyingSubMeshAssignment = true;
    if (state.originalMesh) {
      EiemRestoreOneSubMeshOverride(original, info, state.originalMesh);
    } else {
      EiemClearSubMeshInfoMesh(info);
    }
    s_eiemApplyingSubMeshAssignment = false;
    if (state.infoHandle && il2cpp_gchandle_free)
      il2cpp_gchandle_free(state.infoHandle);
  }
  if (!states.empty())
    Log("[MOD-SUBMESH] restored %zu logical mesh assignment(s)", states.size());
}

// The logical SubMeshInfo hook can run before a Renderer exists. When that
// Renderer is later constructed it already exposes the replacement Mesh, so
// recover the source identity from the logical assignment table before
// capturing reload state or resolving the Render rule.
static void *EiemOriginalForLogicalReplacement(void *mesh) {
  if (!mesh) return nullptr;
  void *source = nullptr;
  AcquireSRWLockShared(&s_eiemSubMeshOverrideLock);
  for (const auto &state : s_eiemSubMeshOverrides) {
    if (state.replacementMesh == mesh && state.originalMesh) {
      source = state.originalMesh;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemSubMeshOverrideLock);
  return source;
}

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

static size_t EiemFindOverrideLocked(void *renderer) {
  for (size_t index = 0; index < s_eiemOverrides.size(); ++index)
    if (s_eiemOverrides[index].renderer == renderer) return index;
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
    if (state.replacementMesh && state.replacementMesh != mesh) {
      state.originalMesh = mesh;
      state.replacementMesh = nullptr;
    }
    if (identityMesh && state.replacementMesh == mesh)
      *identityMesh = state.originalMesh;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static void EiemCaptureOriginal(void *renderer, void *mesh,
                                const char *rendererType) {
  if (!renderer || !mesh) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index = EiemFindOverrideLocked(renderer);
  if (index == SIZE_MAX) {
    EiemRenderOverrideState state = {};
    state.renderer = renderer;
    state.originalMesh = mesh;
    if (g_renderer_get_sharedMaterials && il2cpp_gchandle_new) {
      void *materials = Invoke(g_renderer_get_sharedMaterials, renderer);
      if (materials) {
        state.originalMaterialsHandle = il2cpp_gchandle_new(materials, false);
        state.hasMaterials = state.originalMaterialsHandle != 0;
      }
    }
    if (EiemModEquals(rendererType, "SkinnedMeshRenderer") &&
        il2cpp_gchandle_new) {
      if (g_smr_get_bones) {
        void *bones = Invoke(g_smr_get_bones, renderer);
        if (bones)
          state.originalBonesHandle = il2cpp_gchandle_new(bones, false);
      }
      if (g_smr_get_rootBone) {
        void *rootBone = Invoke(g_smr_get_rootBone, renderer);
        if (rootBone)
          state.originalRootBoneHandle = il2cpp_gchandle_new(rootBone, false);
      }
      state.hasSkinning = state.originalBonesHandle != 0 ||
                          state.originalRootBoneHandle != 0;
    }
    strncpy_s(state.rendererType, sizeof(state.rendererType),
              rendererType ? rendererType : "Renderer", _TRUNCATE);
    s_eiemOverrides.push_back(state);
  } else if (!s_eiemOverrides[index].replacementMesh) {
    // A game-side reassignment can arrive between reconcile passes. Preserve
    // the newest original mesh until the next mutation is applied.
    s_eiemOverrides[index].originalMesh = mesh;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static void EiemRememberReplacement(void *renderer, void *replacementMesh,
                                    const char *rendererType) {
  if (!renderer) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX)
    s_eiemOverrides[index].replacementMesh = replacementMesh;
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
  if (index != SIZE_MAX && s_eiemOverrides[index].originalMesh == mesh)
    replacement = s_eiemOverrides[index].replacementMesh;
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  return replacement;
}

// A Mesh/material override must never own Renderer.enabled. Only an explicit
// handling=skip directive changes it, and therefore only skip needs a value
// restored on reload. LOD setup is free to toggle enabled while a mesh-only
// rule is active.
static void EiemCaptureEnabledForSkip(void *renderer) {
  bool enabled = true;
  if (!renderer || !EiemReadRendererEnabled(renderer, &enabled)) return;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index != SIZE_MAX && !s_eiemOverrides[index].hasEnabled) {
    s_eiemOverrides[index].originalEnabled = enabled;
    s_eiemOverrides[index].hasEnabled = true;
  }
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
}

static bool EiemGetOriginalEnabled(void *renderer, bool *enabled) {
  if (enabled) *enabled = true;
  if (!renderer || !enabled) return false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(renderer);
  if (index == SIZE_MAX || !s_eiemOverrides[index].hasEnabled) {
    ReleaseSRWLockShared(&s_eiemOverrideLock);
    return false;
  }
  *enabled = s_eiemOverrides[index].originalEnabled;
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  return true;
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

static void EiemRestoreRenderOverrides() {
  std::vector<EiemRenderOverrideState> states;
  AcquireSRWLockExclusive(&s_eiemOverrideLock);
  states.swap(s_eiemOverrides);
  ReleaseSRWLockExclusive(&s_eiemOverrideLock);
  for (const auto &state : states) {
    Log("[DEBUG-hr1] restore renderer=%p original=%p replacement=%p restoreEnabled=%d originalEnabled=%d",
        state.renderer, state.originalMesh, state.replacementMesh,
        state.hasEnabled ? 1 : 0,
        state.originalEnabled ? 1 : 0);
    if (state.renderer && state.originalMesh)
      EiemSetSharedMesh(state.renderer, state.originalMesh, state.rendererType, nullptr);
    if (state.renderer) {
      void *restored = EiemReadSharedMesh(state.renderer, state.rendererType);
      Log("[DEBUG-hr1] restore result renderer=%p actual=%p expected=%p ok=%d",
          state.renderer, restored, state.originalMesh,
          restored == state.originalMesh ? 1 : 0);
    }
    if (state.renderer && state.hasMaterials && state.originalMaterialsHandle &&
        s_eiemRendererSetSharedMaterials && il2cpp_gchandle_get_target) {
      void *materials = il2cpp_gchandle_get_target(state.originalMaterialsHandle);
      if (materials) {
        void *params[] = {materials};
        Invoke(s_eiemRendererSetSharedMaterials, state.renderer, params);
      }
    }
    if (state.renderer && state.hasEnabled && g_renderer_set_enabled) {
      bool enabled = state.originalEnabled;
      void *params[] = {&enabled};
      Invoke(g_renderer_set_enabled, state.renderer, params);
    }
    if (state.renderer && state.hasSkinning &&
        il2cpp_gchandle_get_target) {
      if (state.originalBonesHandle && g_smr_set_bones) {
        void *bones = il2cpp_gchandle_get_target(state.originalBonesHandle);
        if (bones) {
          void *params[] = {bones};
          Invoke(g_smr_set_bones, state.renderer, params);
        }
      }
      if (state.originalRootBoneHandle && g_smr_set_rootBone) {
        void *rootBone = il2cpp_gchandle_get_target(state.originalRootBoneHandle);
        if (rootBone) {
          void *params[] = {rootBone};
          Invoke(g_smr_set_rootBone, state.renderer, params);
        }
      }
    }
    if (state.originalMaterialsHandle && il2cpp_gchandle_free)
      il2cpp_gchandle_free(state.originalMaterialsHandle);
    if (state.originalBonesHandle && il2cpp_gchandle_free)
      il2cpp_gchandle_free(state.originalBonesHandle);
    if (state.originalRootBoneHandle && il2cpp_gchandle_free)
      il2cpp_gchandle_free(state.originalRootBoneHandle);
  }
  if (!states.empty())
    Log("[MOD] Restored %zu renderer override(s) before reload", states.size());
}

struct EiemResolvedRenderRule {
  EiemModRule rule = {};
  char source[768] = {};
  char asset[192] = {};
};

struct EiemPartnerState {
  void *sourceRenderer = nullptr;
  void *partnerObject = nullptr;
  void *partnerRenderer = nullptr;
  LONG generation = -1;
  char section[96] = {};
};
static SRWLOCK s_eiemPartnerLock = SRWLOCK_INIT;
static std::vector<EiemPartnerState> s_eiemPartners;

static size_t EiemFindPartnerLocked(void *sourceRenderer, const char *section,
                                    LONG generation) {
  for (size_t i = 0; i < s_eiemPartners.size(); ++i) {
    const auto &state = s_eiemPartners[i];
    if (state.sourceRenderer == sourceRenderer &&
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

// A replacement Mesh carries its own bindpose/weight palette, but the
// SkinnedMeshRenderer still owns the live Transform palette used by Unity's
// skinning job.  Endfield can rebuild that palette while assigning a Mesh;
// restore it only when the game actually changed it, because reassigning an
// unchanged palette itself can force the renderer back to bind pose.
static void EiemPreserveSourceSkinning(void *renderer) {
  if (!renderer) return;
  void *bones = nullptr;
  void *rootBone = nullptr;
  bool capturedBones = false;
  bool capturedRootBone = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t overrideIndex = EiemFindOverrideLocked(renderer);
  if (overrideIndex != SIZE_MAX && il2cpp_gchandle_get_target) {
    const EiemRenderOverrideState &state = s_eiemOverrides[overrideIndex];
    if (state.originalBonesHandle) {
      bones = il2cpp_gchandle_get_target(state.originalBonesHandle);
      capturedBones = bones != nullptr;
    }
    if (state.originalRootBoneHandle) {
      rootBone = il2cpp_gchandle_get_target(state.originalRootBoneHandle);
      capturedRootBone = rootBone != nullptr;
    }
  }
  ReleaseSRWLockShared(&s_eiemOverrideLock);
  if (g_smr_get_bones) {
    if (!capturedBones) __try { bones = Invoke(g_smr_get_bones, renderer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { bones = nullptr; }
  }
  if (g_smr_get_rootBone) {
    if (!capturedRootBone) __try { rootBone = Invoke(g_smr_get_rootBone, renderer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { rootBone = nullptr; }
  }
  // Assigning these properties is not a harmless readback in Endfield: its
  // skin setup observes the setter and can rebuild the pose palette at bind
  // pose.  The replacement Mesh does not change the renderer's palette, so
  // leave it alone unless the game actually replaced the array/root bone.
  bool bonesChanged = false;
  bool rootBoneChanged = false;
  if (bones && g_smr_get_bones && g_smr_set_bones) {
    void *current = nullptr;
    __try { current = Invoke(g_smr_get_bones, renderer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { current = nullptr; }
    bonesChanged = !EiemManagedObjectArraySame(current, bones);
    if (bonesChanged) {
      void *params[] = {bones};
      __try { Invoke(g_smr_set_bones, renderer, params); }
      __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
  }
  if (rootBone && g_smr_get_rootBone && g_smr_set_rootBone) {
    void *current = nullptr;
    __try { current = Invoke(g_smr_get_rootBone, renderer); }
    __except (EXCEPTION_EXECUTE_HANDLER) { current = nullptr; }
    rootBoneChanged = current != rootBone;
    if (rootBoneChanged) {
      void *params[] = {rootBone};
      __try { Invoke(g_smr_set_rootBone, renderer, params); }
      __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
  }
  Log("[DEBUG-SKIN-STATE] renderer=%p bones=%zu rootBone=%p changed=%d/%d",
      renderer, EiemManagedArrayLength(bones), rootBone,
      bonesChanged ? 1 : 0, rootBoneChanged ? 1 : 0);
  EiemLogRendererBoneFingerprint("after-assignment", renderer);
}

// The game may assign the Renderer bone palette after sharedMesh.  This is
// observation-only: record the final palette for an actively replaced
// renderer so a twisted result can be distinguished from bad mesh weights.
static void TraceSkinnedMeshSetBones(void *self, void *bones,
                                     void *methodInfo) {
  auto original = (TraceSetBonesFn)s_origSkinnedMeshSetBones;
  if (original) original(self, bones, methodInfo);
  if (!self || s_eiemApplyingModMeshAssignment) {
    return;
  }
  bool tracked = false;
  AcquireSRWLockShared(&s_eiemOverrideLock);
  const size_t index = EiemFindOverrideLocked(self);
  if (index != SIZE_MAX && s_eiemOverrides[index].replacementMesh)
    tracked = true;
  ReleaseSRWLockShared(&s_eiemOverrideLock);
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

static void EiemDestroyPartnerObjects() {
  std::vector<EiemPartnerState> states;
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  states.swap(s_eiemPartners);
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  for (const auto &state : states) {
    if (state.partnerRenderer)
      EiemSetPartnerLodMembership(state.sourceRenderer, state.partnerRenderer,
                                  false);
    if (state.partnerObject && g_object_destroy) {
      void *params[] = {state.partnerObject};
      Invoke(g_object_destroy, nullptr, params);
    }
  }
  if (!states.empty())
    Log("[MOD] Destroyed %zu partner Renderer(s) before reload", states.size());
}

static void EiemCopyPartnerTransform(void *sourceTransform,
                                      void *partnerTransform) {
  if (!sourceTransform || !partnerTransform) return;
  if (g_transform_get_localPosition && g_transform_set_localPosition) {
    void *boxed = Invoke(g_transform_get_localPosition, sourceTransform);
    if (boxed) {
      Vector3 value = *(Vector3 *)((char *)boxed + 16);
      void *params[] = {&value};
      Invoke(g_transform_set_localPosition, partnerTransform, params);
    }
  }
  if (g_transform_get_localRotation && g_transform_set_localRotation) {
    void *boxed = Invoke(g_transform_get_localRotation, sourceTransform);
    if (boxed) {
      Quaternion value = *(Quaternion *)((char *)boxed + 16);
      void *params[] = {&value};
      Invoke(g_transform_set_localRotation, partnerTransform, params);
    }
  }
  if (g_transform_get_localScale && g_transform_set_localScale) {
    void *boxed = Invoke(g_transform_get_localScale, sourceTransform);
    if (boxed) {
      Vector3 value = *(Vector3 *)((char *)boxed + 16);
      void *params[] = {&value};
      Invoke(g_transform_set_localScale, partnerTransform, params);
    }
  }
}

static void *EiemCreatePartnerRenderer(void *sourceRenderer,
                                       const char *rendererType,
                                       const EiemModRule &partnerRule,
                                       void *sourceMesh, char *error,
                                       size_t errorSize) {
  if (!sourceRenderer || !rendererType || !g_gameObjectClass ||
      (!g_gameObject_ctor && !g_gameObject_ctorDefault) ||
      !g_gameObject_AddComponent || !g_component_get_gameObject ||
      !g_component_get_transform || !il2cpp_class_get_type ||
      !il2cpp_type_get_object) {
    if (error) strncpy_s(error, errorSize,
                         "Unity GameObject/Transform creation APIs are unavailable",
                         _TRUNCATE);
    return nullptr;
  }
  void *sourceGo = Invoke(g_component_get_gameObject, sourceRenderer);
  if (!sourceGo) {
    if (error) strncpy_s(error, errorSize, "Source Renderer has no GameObject", _TRUNCATE);
    return nullptr;
  }
  void *sourceTransform = Invoke(g_component_get_transform, sourceRenderer);
  if (!sourceTransform) {
    if (error) strncpy_s(error, errorSize, "Source Renderer has no Transform", _TRUNCATE);
    return nullptr;
  }
  void *partnerGo = il2cpp_object_new(g_gameObjectClass);
  if (!partnerGo) {
    if (error) strncpy_s(error, errorSize, "Unable to allocate partner GameObject", _TRUNCATE);
    return nullptr;
  }
  void *partnerRenderer = nullptr;
  auto cleanupPartner = [&]() {
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
  // Preserve the source world transform. This avoids applying a parent/local
  // transform twice and also works when the source has a nonuniform parent.
  void *parent = g_transform_get_parent
                     ? Invoke(g_transform_get_parent, sourceTransform)
                     : nullptr;
  bool worldPositionStays = true;
  void *parentParams[] = {parent, &worldPositionStays};
  if (g_transform_set_parent)
    Invoke(g_transform_set_parent, partnerTransform, parentParams);

  void *rendererClass = EiemModEquals(rendererType, "SkinnedMeshRenderer")
                            ? g_skinnedMeshRendererClass
                            : g_meshFilterClass;
  void *type = rendererClass ? il2cpp_class_get_type(rendererClass) : nullptr;
  void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
  if (!typeObject) {
    if (error) strncpy_s(error, errorSize, "Partner Renderer type is unavailable", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  void *componentParams[] = {typeObject};
  partnerRenderer = Invoke(g_gameObject_AddComponent, partnerGo,
                           componentParams);
  if (!partnerRenderer) {
    if (error) strncpy_s(error, errorSize, "GameObject.AddComponent returned null", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }

  void *assignedMesh = sourceMesh;
  char buildError[256] = {};
  if (partnerRule.hasMesh &&
      !EiemBuildMeshResource(partnerRule, &assignedMesh, buildError,
                             sizeof(buildError), sourceMesh, sourceRenderer)) {
    if (error) strncpy_s(error, errorSize, buildError, _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  if (assignedMesh && !EiemSetSharedMesh(partnerRenderer, assignedMesh,
                                          rendererType, nullptr)) {
    if (error) strncpy_s(error, errorSize, "Partner mesh assignment failed", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  if (EiemModEquals(rendererType, "SkinnedMeshRenderer")) {
    if (g_smr_get_bones && g_smr_set_bones) {
      void *bones = Invoke(g_smr_get_bones, sourceRenderer);
      if (bones) { void *params[] = {bones}; Invoke(g_smr_set_bones, partnerRenderer, params); }
    }
    if (g_smr_get_rootBone && g_smr_set_rootBone) {
      void *rootBone = Invoke(g_smr_get_rootBone, sourceRenderer);
      if (rootBone) { void *params[] = {rootBone}; Invoke(g_smr_set_rootBone, partnerRenderer, params); }
    }
    if (g_smr_get_localBounds && g_smr_set_localBounds) {
      // SkinnedMeshRenderer culling uses localBounds, which is independent of
      // Mesh.bounds. Copy the source bounds so the replacement is not culled
      // before its first skinning update.
      void *boxedBounds = Invoke(g_smr_get_localBounds, sourceRenderer);
      if (boxedBounds) {
        EiemBounds bounds = *(EiemBounds *)((char *)boxedBounds + 16);
        void *boundsParams[] = {&bounds};
        Invoke(g_smr_set_localBounds, partnerRenderer, boundsParams);
      }
    }
  }
  if (partnerRule.materialCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterials(partnerRule, &materials, buildError,
                                    sizeof(buildError)) ||
        !materials ||
        !EiemAssignRendererMaterials(partnerRenderer, materials, buildError,
                                     sizeof(buildError))) {
      if (error) strncpy_s(error, errorSize, buildError[0] ? buildError : "Partner materials failed", _TRUNCATE);
      cleanupPartner();
      return nullptr;
    }
  } else if (g_renderer_get_sharedMaterials && s_eiemRendererSetSharedMaterials) {
    void *materials = Invoke(g_renderer_get_sharedMaterials, sourceRenderer);
    if (materials &&
        !EiemAssignRendererMaterials(partnerRenderer, materials, buildError,
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
  if (g_renderer_set_enabled) { void *params[] = {&enabled}; Invoke(g_renderer_set_enabled, partnerRenderer, params); }
  EiemSetPartnerLodMembership(sourceRenderer, partnerRenderer, true);

  bool sourceEnabled = true;
  bool partnerEnabled = true;
  EiemReadRendererEnabled(sourceRenderer, &sourceEnabled);
  EiemReadRendererEnabled(partnerRenderer, &partnerEnabled);
  int sourceMaterials = -1;
  int partnerMaterials = -1;
  if (g_renderer_get_sharedMaterials) {
    void *sourceArray = Invoke(g_renderer_get_sharedMaterials, sourceRenderer);
    void *partnerArray = Invoke(g_renderer_get_sharedMaterials, partnerRenderer);
    if (sourceArray) sourceMaterials = *(int *)((char *)sourceArray + 24);
    if (partnerArray) partnerMaterials = *(int *)((char *)partnerArray + 24);
  }
  Log("[DEBUG-partner] source=%p enabled=%d mesh=%p materials=%d partner=%p "
      "enabled=%d mesh=%p materials=%d",
      sourceRenderer, sourceEnabled ? 1 : 0,
      EiemReadSharedMesh(sourceRenderer, rendererType), sourceMaterials,
      partnerRenderer, partnerEnabled ? 1 : 0,
      EiemReadSharedMesh(partnerRenderer, rendererType), partnerMaterials);

  EiemPartnerState state = {};
  state.sourceRenderer = sourceRenderer;
  state.partnerObject = partnerGo;
  state.partnerRenderer = partnerRenderer;
  state.generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  strncpy_s(state.section, sizeof(state.section), partnerRule.section, _TRUNCATE);
  AcquireSRWLockExclusive(&s_eiemPartnerLock);
  s_eiemPartners.push_back(state);
  ReleaseSRWLockExclusive(&s_eiemPartnerLock);
  Log("[MOD] partner Renderer created: source=%p section=%s renderer=%p mesh=%s",
      sourceRenderer, partnerRule.section, partnerRenderer,
      partnerRule.hasMesh ? partnerRule.mesh : "<source>");
  return partnerRenderer;
}

static void EiemApplyPartners(void *sourceRenderer, void *sourceMesh,
                              const char *rendererType,
                              const EiemModRule &sourceRule) {
  if (!sourceRenderer || !sourceRule.partnerCount || s_eiemCreatingPartner) return;
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  for (uint32_t index = 0; index < sourceRule.partnerCount; ++index) {
    const char *section = sourceRule.partners[index];
    if (!section[0]) continue;
    AcquireSRWLockShared(&s_eiemPartnerLock);
    const bool exists = EiemFindPartnerLocked(sourceRenderer, section, generation) != SIZE_MAX;
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
    EiemCreatePartnerRenderer(sourceRenderer, rendererType, partner, sourceMesh,
                              error, sizeof(error));
    s_eiemCreatingPartner = false;
    if (error[0])
      Log("[MOD] partner creation failed: source=%s partner=%s error=%s",
          sourceRule.section, section, error);
  }
}

// Resolve a Mesh once, then apply its result to every Renderer that shares
// that resource. This preserves the resource-level replacement model rather
// than treating shadow, LOD, or instanced Renderers as separate targets.
static bool EiemResolveRenderRule(void *mesh, EiemResolvedRenderRule *out) {
  if (!mesh || !out) return false;
  char source[768] = {}, asset[192] = {};
  if (!EiemReadLiveMeshIdentity(mesh, source, sizeof(source), asset,
                                sizeof(asset)))
    return false;
  if (!EiemHasResourceRenderRuleAsset(asset)) return false;
  int32_t vertices = -1, indices = -1, subMeshes = -1;
  EiemReadLiveMeshShape(mesh, &vertices, &indices, &subMeshes);
  EiemModRule rule = {};
  if (!EiemFindResourceRenderRule(source, asset, vertices, indices, subMeshes, &rule))
    return false;
  out->rule = rule;
  strncpy_s(out->source, sizeof(out->source), source, _TRUNCATE);
  strncpy_s(out->asset, sizeof(out->asset), asset, _TRUNCATE);
  return true;
}

// Prefab-backed post-model meshes can be assigned before Unity has populated
// the Mesh object's name/origin metadata.  The Renderer already carries the
// serialized resource name at that point, so keep the same structural match
// rules but resolve through that name as a narrowly-scoped fallback.  This is
// used only on the early setter path, before the game's skin/GPU caches read
// the renderer, and never guesses between rules with different shapes.
static bool EiemResolveRenderRuleForAsset(void *mesh, const char *asset,
                                          EiemResolvedRenderRule *out) {
  if (!asset || !asset[0] || !out) return false;
  int32_t vertices = -1, indices = -1, subMeshes = -1;
  if (mesh) EiemReadLiveMeshShape(mesh, &vertices, &indices, &subMeshes);
  EiemModRule rule = {};
  if (!EiemFindResourceRenderRule(nullptr, asset, vertices, indices, subMeshes,
                                  &rule))
    return false;
  out->rule = rule;
  out->source[0] = '\0';
  strncpy_s(out->asset, sizeof(out->asset), asset, _TRUNCATE);
  return true;
}

// SubMeshInfo records are frequently materialized with mesh=null and the
// actual Mesh is produced by get_mesh on demand.  Mutating a temporary array
// returned by GetSubMeshInfo therefore cannot affect CreateSMSGO.  Resolve at
// the getter itself so the game's own renderer/skin assembly receives the
// generated Mesh as its source object.
static void *TraceSubMeshInfoGetMesh(void *self, void *methodInfo) {
  auto original = (TraceSubMeshInfoGetMeshFn)s_origSubMeshInfoGetMesh;
  void *sourceMesh = original ? original(self, methodInfo) : nullptr;
  if (!self || s_eiemApplyingSubMeshAssignment || !EiemOnUnityThread())
    return sourceMesh;
  if (s_subMeshInfoMeshNameOffset < 0) return sourceMesh;

  char assetName[192] = {};
  __try {
    void *name = *(void **)((char *)self + s_subMeshInfoMeshNameOffset);
    if (name) ReadStrUtf8(name, assetName, sizeof(assetName));
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    assetName[0] = '\0';
  }
  if (!assetName[0] && sourceMesh)
    TraceReadUnityObjectName(sourceMesh, assetName, sizeof(assetName));
  if (!assetName[0] || !EiemHasResourceRenderRuleAsset(assetName))
    return sourceMesh;

  EiemResolvedRenderRule resolved = {};
  if (!EiemResolveRenderRuleForAsset(sourceMesh, assetName, &resolved) ||
      !resolved.rule.hasMesh)
    return sourceMesh;

  void *replacement = nullptr;
  char error[256] = {};
  if (!EiemBuildMeshResource(resolved.rule, &replacement, error,
                             sizeof(error), sourceMesh) || !replacement) {
    Log("[MOD-LOGICAL-GET] build failed info=%p asset=%s error=%s", self,
        assetName, error[0] ? error : "unknown");
    return sourceMesh;
  }
  EiemRememberSubMeshReplacement(self, sourceMesh, replacement);
  Log("[MOD-LOGICAL-GET] SubMeshInfo=%p asset=%s source=%p replacement=%p",
      self, assetName, sourceMesh, replacement);
  return replacement;
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
  if (s_eiemApplyingSubMeshAssignment || !self || !mesh) {
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

  EiemResolvedRenderRule resolved = {};
  bool matched = assetName[0] &&
                 EiemHasResourceRenderRuleAsset(assetName) &&
                 EiemResolveRenderRuleForAsset(mesh, assetName, &resolved);
  if (!matched) {
    // A path hash is still useful evidence for diagnostics, but it is not a
    // replacement key by itself: the ini contract requires the asset name and
    // optional shape checks, avoiding collisions between same-named subassets.
    original(self, mesh, methodInfo);
    return;
  }

  const bool safeThread = EiemOnUnityThread();
  if (safeThread && resolved.rule.hasMesh) {
    void *replacement = nullptr;
    char error[256] = {};
    if (EiemBuildMeshResource(resolved.rule, &replacement, error,
                              sizeof(error), mesh, nullptr) && replacement) {
      EiemRememberSubMeshReplacement(self, mesh, replacement);
      original(self, replacement, methodInfo);
      Log("[MOD-LOGICAL-MESH] SubMeshInfo=%p asset=%s source=%p replacement=%p",
          self, assetName, mesh, replacement);
      return;
    }
    Log("[MOD-LOGICAL-MESH] build failed info=%p asset=%s error=%s", self,
        assetName, error[0] ? error : "unknown");
  } else if (!safeThread) {
    Log("[MOD-LOGICAL-MESH] deferred unsafe thread info=%p asset=%s tid=%lu unityTid=%lu",
        self, assetName, (unsigned long)GetCurrentThreadId(),
        (unsigned long)s_eiemUnityThreadId);
  }
  // The original assignment is always preserved if construction cannot happen
  // at this boundary; the main-thread reconcile will retry the same rule.
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

static size_t EiemApplyLogicalSubMeshInfoArray(void *array, int32_t lod,
                                                bool includeGpu) {
  if (!array || s_subMeshInfoMeshOffset < 0 || !EiemOnUnityThread()) return 0;
  const size_t count = EiemManagedArrayLength(array);
  if (!count || count > 128) return 0;
  void **items = (void **)((char *)array + 32);
  size_t applied = 0;
  for (size_t index = 0; index < count; ++index) {
    void *info = items[index];
    if (!info) continue;
    void *sourceMesh = nullptr;
    char assetName[192] = {};
    __try {
      sourceMesh = *(void **)((char *)info + s_subMeshInfoMeshOffset);
      if (s_subMeshInfoMeshNameOffset >= 0) {
        void *name = *(void **)((char *)info + s_subMeshInfoMeshNameOffset);
        if (name) ReadStrUtf8(name, assetName, sizeof(assetName));
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      sourceMesh = nullptr;
      assetName[0] = '\0';
    }
    // Some post-model records leave meshName unset and only populate the
    // Mesh reference.  The reference's Unity Object name is the same logical
    // sub-asset identity used by Render matching; use it before giving up.
    if (!assetName[0] && sourceMesh)
      TraceReadUnityObjectName(sourceMesh, assetName, sizeof(assetName));
    if (!assetName[0]) continue;
    const bool configuredTarget = EiemHasResourceRenderRuleAsset(assetName);
    if (configuredTarget) {
      Log("[DEBUG-LOGICAL-MESH] GetSubMeshInfo lod=%d gpu=%d info=%p "
          "source=%p asset=%s tid=%lu unityTid=%lu",
          lod, includeGpu ? 1 : 0, info, sourceMesh, assetName,
          (unsigned long)GetCurrentThreadId(),
          (unsigned long)s_eiemUnityThreadId);
    }
    EiemResolvedRenderRule resolved = {};
    if (!EiemResolveRenderRuleForAsset(sourceMesh, assetName, &resolved) ||
        !resolved.rule.hasMesh)
      continue;
    void *replacement = nullptr;
    char error[256] = {};
    if (!EiemBuildMeshResource(resolved.rule, &replacement, error,
                               sizeof(error), sourceMesh) || !replacement) {
      if (configuredTarget)
        Log("[DEBUG-LOGICAL-MESH] GetSubMeshInfo build failed lod=%d "
            "info=%p asset=%s source=%p error=%s",
            lod, info, assetName, sourceMesh,
            error[0] ? error : "unknown");
      continue;
    }
    if (sourceMesh == replacement) continue;
    EiemRememberSubMeshReplacement(info, sourceMesh, replacement);
    bool assigned = false;
    if (s_origSubMeshInfoSetMesh && s_subMeshInfoSetMeshMethodInfo) {
      s_eiemApplyingSubMeshAssignment = true;
      ((TraceSubMeshInfoSetMeshFn)s_origSubMeshInfoSetMesh)(
          info, replacement, s_subMeshInfoSetMeshMethodInfo);
      s_eiemApplyingSubMeshAssignment = false;
      assigned = true;
    } else {
      __try {
        *(void **)((char *)info + s_subMeshInfoMeshOffset) = replacement;
        assigned = true;
      } __except (EXCEPTION_EXECUTE_HANDLER) {
        assigned = false;
      }
    }
    if (assigned) ++applied;
    Log("[MOD-LOGICAL-MESH] GetSubMeshInfo lod=%d gpu=%d info=%p "
        "asset=%s source=%p replacement=%p assigned=%d error=%s",
        lod, includeGpu ? 1 : 0, info, assetName, sourceMesh, replacement,
        assigned ? 1 : 0, error[0] ? error : "<none>");
  }
  return applied;
}

static void *TraceLodGetSubMeshInfo(void *self, int32_t lod, bool includeGpu,
                                    void *methodInfo) {
  auto original = (TraceLodGetSubMeshInfoFn)s_origLodGetSubMeshInfo;
  void *result = original ? original(self, lod, includeGpu, methodInfo) : nullptr;
  TraceLogSubMeshInfoArray(self, lod, includeGpu, result, "GetSubMeshInfo");
  // Capture the thread only when it is verifiably the game's window thread;
  // resource queries can also arrive from worker threads during startup.
  EiemOnUnityThread();
  EiemApplyLogicalSubMeshInfoArray(result, lod, includeGpu);
  return result;
}

// The CPU avatar path materializes SubMeshInfo.mesh in this helper rather
// than through the managed property setter. Patch the returned logical data
// before CreateSMS/AssignSkin consumes it, preserving the game's own renderer
// and bone/material assembly.
static void *TraceGetPartCpuMesh(void *meshAssets, int32_t lod,
                                 void *methodInfo) {
  auto original = (TraceGetPartCpuMeshFn)s_origGetPartCpuMesh;
  void *result = original ? original(meshAssets, lod, methodInfo) : nullptr;
  EiemOnUnityThread();
  if (!result || s_subMeshInfoMeshOffset < 0 || !EiemOnUnityThread())
    return result;
  const size_t count = EiemManagedArrayLength(result);
  if (!count || count > 128) return result;
  void **items = (void **)((char *)result + 32);
  for (size_t index = 0; index < count; ++index) {
    void *info = items[index];
    if (!info) continue;
    void *sourceMesh = nullptr;
    char assetName[192] = {};
    __try {
      sourceMesh = *(void **)((char *)info + s_subMeshInfoMeshOffset);
      if (s_subMeshInfoMeshNameOffset >= 0) {
        void *name = *(void **)((char *)info + s_subMeshInfoMeshNameOffset);
        if (name) ReadStrUtf8(name, assetName, sizeof(assetName));
      }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
      sourceMesh = nullptr;
      assetName[0] = '\0';
    }
    if (!assetName[0] && sourceMesh)
      TraceReadUnityObjectName(sourceMesh, assetName, sizeof(assetName));
    // The game's logical record is commonly produced before the asset loader
    // materializes its Unity Mesh.  `meshName` is the stable sub-asset
    // identity at this boundary; requiring sourceMesh here silently skipped
    // the only early replacement point for post-model parts.
    if (!assetName[0]) continue;
    const bool configuredTarget = EiemHasResourceRenderRuleAsset(assetName);
    if (configuredTarget) {
      Log("[DEBUG-LOGICAL-MESH] GetPartCPUMesh lod=%d info=%p source=%p "
          "asset=%s tid=%lu unityTid=%lu",
          lod, info, sourceMesh, assetName,
          (unsigned long)GetCurrentThreadId(),
          (unsigned long)s_eiemUnityThreadId);
    }
    EiemResolvedRenderRule resolved = {};
    if (!EiemResolveRenderRuleForAsset(sourceMesh, assetName, &resolved) ||
        !resolved.rule.hasMesh)
      continue;
    void *replacement = nullptr;
    char error[256] = {};
    if (!EiemBuildMeshResource(resolved.rule, &replacement, error,
                               sizeof(error), sourceMesh) || !replacement) {
      if (configuredTarget)
        Log("[DEBUG-LOGICAL-MESH] build failed lod=%d info=%p asset=%s "
            "source=%p error=%s",
            lod, info, assetName, sourceMesh,
            error[0] ? error : "unknown");
      continue;
    }
    EiemRememberSubMeshReplacement(info, sourceMesh, replacement);
    // Use the game's setter with its own MethodInfo so any side effects remain
    // intact. The recursion guard prevents the shared thunk from re-entering
    // replacement logic.
    if (s_origSubMeshInfoSetMesh && s_subMeshInfoSetMeshMethodInfo) {
      s_eiemApplyingSubMeshAssignment = true;
      ((TraceSubMeshInfoSetMeshFn)s_origSubMeshInfoSetMesh)(
          info, replacement, s_subMeshInfoSetMeshMethodInfo);
      s_eiemApplyingSubMeshAssignment = false;
    } else {
      __try { *(void **)((char *)info + s_subMeshInfoMeshOffset) = replacement; }
      __except (EXCEPTION_EXECUTE_HANDLER) {}
    }
    Log("[MOD-LOGICAL-MESH] GetPartCPUMesh lod=%d info=%p asset=%s source=%p replacement=%p",
        lod, info, assetName, sourceMesh, replacement);
  }
  return result;
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

// Every gameplay/UI/NPC prefab created through BundleResourceManager reaches
// this common completion boundary. Apply resource rules to the completed
// object graph here instead of adding one hook for each consumer-specific
// loader. The call remains idempotent: the same resolver and original-state
// registry are used by live reconciliation and renderer setters.
static void TracePrefabInstantiateCompleted(void *self, void *methodInfo) {
  auto original = (TracePrefabInstantiateCompletedFn)
      s_origPrefabInstantiateCompleted;
  if (original) original(self, methodInfo);
  void *model = s_prefabInstantiateGetGameObject
                    ? Invoke(s_prefabInstantiateGetGameObject, self)
                    : nullptr;
  if (model)
    TraceApplyLoadedModelRenderers(model, 0, "post-prefab-completed");
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
  TraceApplyLoadedModelRenderers(model, 0, "post-gameobject-allocate");
}

// Persistent-pool loads return an already constructed GameObject. Remember
// the logical path and re-apply the resource rule before the borrower uses it.
static void *TraceModelManagerLoadFromPersistentPool(void *self,
                                                      int64_t pathHash,
                                                      void *methodInfo) {
  auto original = (TraceModelManagerLoadHashFn)
      s_origModelManagerLoadFromPersistentPool;
  void *model = original ? original(self, pathHash, methodInfo) : nullptr;
  if (model) {
    TraceRememberLoadedModelPath(model, pathHash);
    TraceApplyLoadedModelRenderers(model, pathHash,
                                   "post-load-from-persistent-pool");
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
  if (success) {
    void *model = TraceReadObjectField(self, s_basePartModelOffset);
    char path[768] = {};
    const int configPathOffset =
        (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
            ? s_basePartConfigOffset + s_basePartConfigPathOffset
            : -1;
    TraceReadStringField(self, configPathOffset, path, sizeof(path));
    if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));
    TraceApplyLoadedModelRenderers(model, 0, "post-on-load-finish", path);
  }
}

static void TraceBasePartPostDeal(void *self, void *methodInfo) {
  auto original = (TraceBasePartPostDealFn)s_origBasePartPostDeal;
  if (original) original(self, methodInfo);
  TraceLogCharacterFlow("BaseModelViewPart.PostDealLoadedModel", self,
                        "BaseModelViewPart");
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  char path[768] = {};
  const int configPathOffset =
      (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
          ? s_basePartConfigOffset + s_basePartConfigPathOffset
          : -1;
  TraceReadStringField(self, configPathOffset, path, sizeof(path));
  if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));
  TraceApplyLoadedModelRenderers(model, 0, "post-base-post-deal", path);
}

static void TraceComplexPartPostDeal(void *self, void *methodInfo) {
  auto original = (TraceBasePartPostDealFn)s_origComplexPartPostDeal;
  if (original) original(self, methodInfo);
  TraceLogCharacterFlow("ComplexModelViewPart.PostDealLoadedModel", self,
                        "ComplexModelViewPart");
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  char path[768] = {};
  const int configPathOffset =
      (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
          ? s_basePartConfigOffset + s_basePartConfigPathOffset
          : -1;
  TraceReadStringField(self, configPathOffset, path, sizeof(path));
  if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));
  TraceApplyLoadedModelRenderers(model, 0, "post-complex-post-deal", path);
}

// The callback receives the fully instantiated prefab GameObject. Apply the
// replacement before and after the game's own completion method: before lets
// the game's renderer/skin cache observe the generated Mesh, while the second
// pass catches renderers created by the completion method itself.
static void TraceBasePartLoadFinishCallback(void *self, int32_t requestId,
                                            int64_t pathHash, void *model,
                                            void *methodInfo) {
  TraceRememberLoadedModelPath(model, pathHash);
  TraceApplyLoadedModelRenderers(model, pathHash, "pre-load-finish-callback");
  auto original =
      (TraceBasePartLoadFinishCallbackFn)s_origBasePartLoadFinishCallback;
  if (original) original(self, requestId, pathHash, model, methodInfo);
  TraceApplyLoadedModelRenderers(model, pathHash, "post-load-finish-callback");
}

static bool TraceBasePartLoadFinishResult(void *self, int32_t requestId,
                                          int64_t pathHash, void *model,
                                          void *methodInfo) {
  TraceRememberLoadedModelPath(model, pathHash);
  TraceApplyLoadedModelRenderers(model, pathHash, "pre-load-finish-result");
  auto original =
      (TraceBasePartLoadFinishResultFn)s_origBasePartLoadFinishResult;
  const bool result = original ? original(self, requestId, pathHash, model,
                                           methodInfo)
                               : false;
  TraceApplyLoadedModelRenderers(model, pathHash, "post-load-finish-result");
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
  if (!success) return;
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  char path[768] = {};
  const int configPathOffset =
      (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
          ? s_basePartConfigOffset + s_basePartConfigPathOffset
          : -1;
  TraceReadStringField(self, configPathOffset, path, sizeof(path));
  if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));
  TraceApplyLoadedModelRenderers(model, 0,
                                 "post-load-use-handle-callback", path);
}

static bool TraceBasePartLoadUseHandleFinish(void *self, bool success,
                                             void *handle,
                                             void *methodInfo) {
  auto original = (TraceBasePartLoadUseHandleFinishResultFn)
      s_origBasePartLoadUseHandleFinishResult;
  const bool result = original ? original(self, success, handle, methodInfo)
                               : false;
  if (!result) return result;
  void *model = TraceReadObjectField(self, s_basePartModelOffset);
  char path[768] = {};
  const int configPathOffset =
      (s_basePartConfigOffset >= 0 && s_basePartConfigPathOffset >= 0)
          ? s_basePartConfigOffset + s_basePartConfigPathOffset
          : -1;
  TraceReadStringField(self, configPathOffset, path, sizeof(path));
  if (!path[0]) TraceLookupLoadedModelPath(model, path, sizeof(path));
  TraceApplyLoadedModelRenderers(model, 0, "post-load-use-handle", path);
  return result;
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

static bool EiemApplyResolvedRenderRule(void *renderer, void *mesh,
                                        const char *rendererType,
                                        void *methodInfo,
                                        const EiemResolvedRenderRule &resolved,
                                        bool allowMeshReplacement = true) {
  (void)methodInfo;
  if (!renderer || !mesh) return false;
  const EiemModRule &rule = resolved.rule;
  const char *source = resolved.source[0] ? resolved.source : "<unknown>";
  const char *asset = resolved.asset[0] ? resolved.asset : "<unknown>";

  // `mesh` and `handling=skip` are independent directives. A rule with
  // neither directive is a match-only declaration and must pass through the
  // game's setter unchanged.
  const bool skipOriginal = EiemModEquals(rule.handling, "skip");
  const bool applyMesh = allowMeshReplacement && rule.hasMesh;
  if (!applyMesh && !skipOriginal && !rule.materialCount &&
      !(allowMeshReplacement && rule.partnerCount))
    return false;

  EiemCaptureOriginal(renderer, mesh, rendererType);

  // `handling=skip` owns only the source Renderer state. It is intentionally
  // independent from every resource mount below.
  if (skipOriginal) {
    EiemCaptureEnabledForSkip(renderer);
    EiemRememberReplacement(renderer, nullptr, rendererType);
    if (g_renderer_set_enabled)
      EiemSetRendererEnabled(renderer, false);
    Log("[MOD] %s resource skip applied: source=%s asset=%s", rendererType,
        source, asset);
  }

  // `mesh=` replaces the source Renderer's shared Mesh in place. It does not
  // create another Renderer; an additional Renderer must be declared and
  // referenced explicitly through `partner.N`.
  bool meshApplied = false;
  if (applyMesh) {
    void *assignedMesh = nullptr;
    char meshError[256] = {};
    meshApplied = EiemBuildMeshResource(rule, &assignedMesh, meshError,
                                        sizeof(meshError), mesh, renderer) &&
                  assignedMesh &&
                  EiemSetSharedMesh(renderer, assignedMesh, rendererType,
                                    nullptr);
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
    if (meshApplied && EiemModEquals(rendererType, "SkinnedMeshRenderer"))
      EiemPreserveSourceSkinning(renderer);
    if (meshApplied)
      EiemPreserveSourceDrawState(renderer, rendererType);
  }

  // Material edits remain source-Renderer edits and are likewise independent
  // of skip/mesh. They are applied after the two resource actions above.
  char error[256] = {};
  if (rule.materialCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(rule, renderer, &materials, error,
                                             sizeof(error))) {
      Log("[MOD] %s material resource failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
      // Keep skip and mesh decisions intact even when a material block fails.
      return true;
    } else if (materials &&
               !EiemAssignRendererMaterials(renderer, materials, error,
                                            sizeof(error))) {
      Log("[MOD] %s material assignment failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
    }
  }
  bool currentEnabled = true;
  const bool readEnabled = EiemReadRendererEnabled(renderer, &currentEnabled);
  bool currentVisible = false;
  const bool readVisible = EiemReadRendererVisible(renderer, &currentVisible);
  const int32_t materialCount = EiemReadRendererMaterialCount(renderer);
  char rendererDescription[512] = {};
  TraceDescribeObject(renderer, rendererDescription,
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
    EiemApplyPartners(renderer, mesh, rendererType, rule);
  return true;
}

// The generic character path receives a fully instantiated prefab before its
// ComplexModelViewPart builds the game's renderer/skin caches. Walk that
// object graph and apply only explicit Render rules to its SkinnedMeshRenderers
// while the original mesh is still available.
static void TraceApplyLoadedModelRenderers(void *model, int64_t pathHash,
                                           const char *stage,
                                           const char *explicitPath) {
  if (!model || !g_gameObject_get_transform || !g_transform_get_childCount ||
      !g_transform_GetChild || !g_component_get_gameObject ||
      !g_gameObject_GetComponent || !g_skinnedMeshRendererClass ||
      !g_smr_get_sharedMesh)
    return;

  char pathText[768] = {};
  if (explicitPath && explicitPath[0])
    strncpy_s(pathText, sizeof(pathText), explicitPath, _TRUNCATE);
  else
    TraceLookupHashPath(pathHash, pathText, sizeof(pathText));
  if (!pathText[0])
    TraceResolveStringPathHashPath(pathHash, pathText, sizeof(pathText));

  void *smrType = il2cpp_class_get_type(g_skinnedMeshRendererClass);
  if (!smrType || !il2cpp_type_get_object) return;
  void *smrTypeObject = il2cpp_type_get_object(smrType);
  if (!smrTypeObject) return;

  void *root = Invoke(g_gameObject_get_transform, model);
  if (!root) return;
  uint32_t visited = 0;
  uint32_t renderers = 0;
  uint32_t matched = 0;
  uint32_t configured = 0;
  const bool safeThread = EiemOnUnityThread();

  auto inspectRenderer = [&](void *renderer) {
    if (!renderer) return;
    ++renderers;
    void *sourceMesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
    char rendererText[512] = {};
    char rendererName[192] = {};
    char meshText[512] = {};
    TraceDescribeObject(renderer, rendererText, sizeof(rendererText));
    EiemExtractObjectName(rendererText, rendererName, sizeof(rendererName));
    TraceDescribeObject(sourceMesh, meshText, sizeof(meshText));
    char meshName[192] = {};
    EiemExtractObjectName(meshText, meshName, sizeof(meshName));
    const char *identity = meshName[0] ? meshName : rendererName;
    const bool configuredTarget =
        identity[0] && EiemHasResourceRenderRuleAsset(identity);
    if (configuredTarget) ++configured;
    EiemResolvedRenderRule resolved = {};
    const bool hasRule = sourceMesh && identity[0] &&
                         EiemResolveRenderRuleForAsset(sourceMesh, identity,
                                                       &resolved);
    if (hasRule) ++matched;
    // Configured targets are never hidden behind a global trace budget. A
    // target that reaches this lifecycle boundary but fails its full match is
    // precisely the diagnostic needed to distinguish identity from timing.
    if (configuredTarget || hasRule)
      Log("[TRACE-CHAR-RENDER] stage=%s path=%s renderer=%s mesh=%s "
          "source=%p configured=%d matched=%d safe=%d",
          stage ? stage : "unknown", pathText[0] ? pathText : "<unresolved>",
          rendererName[0] ? rendererName : "<unnamed>",
          meshName[0] ? meshName : "<unnamed>", sourceMesh,
          configuredTarget ? 1 : 0, hasRule ? 1 : 0,
          safeThread ? 1 : 0);
    const bool hasAction =
        resolved.rule.hasMesh ||
        EiemModEquals(resolved.rule.handling, "skip") ||
        resolved.rule.materialCount || resolved.rule.partnerCount;
    if (hasRule && safeThread && hasAction) {
      const bool applied = EiemApplyResolvedRenderRule(
          renderer, sourceMesh, "SkinnedMeshRenderer", nullptr, resolved, true);
      Log("[TRACE-CHAR-RENDER-APPLY] stage=%s renderer=%p source=%p applied=%d",
          stage ? stage : "unknown", renderer, sourceMesh, applied ? 1 : 0);
    }
  };

  if (g_gameObject_GetComponentsInChildren) {
    bool includeInactive = true;
    void *params[] = {smrTypeObject, &includeInactive};
    void *rendererArray = Invoke(g_gameObject_GetComponentsInChildren, model,
                                 params);
    const size_t count = EiemManagedArrayLength(rendererArray);
    if (rendererArray && count <= 8192) {
      void **items = (void **)((char *)rendererArray + IL2CPP_ARRAY_DATA);
      for (size_t index = 0; index < count; ++index) inspectRenderer(items[index]);
      if (configured || matched)
        Log("[TRACE-CHAR-RENDER-SUMMARY] stage=%s path=%s visited=%zu "
            "renderers=%u configured=%u matched=%u safe=%d "
            "source=GetComponentsInChildren",
            stage ? stage : "unknown",
            pathText[0] ? pathText : "<unresolved>", count, renderers,
            configured, matched,
            safeThread ? 1 : 0);
      return;
    }
  }

  void *stack[4096] = {};
  size_t stackCount = 1;
  stack[0] = root;
  while (stackCount && visited < _countof(stack)) {
    void *transform = stack[--stackCount];
    if (!transform) continue;
    ++visited;
    void *go = Invoke(g_component_get_gameObject, transform);
    if (go) {
      void *params[] = {smrTypeObject};
      inspectRenderer(Invoke(g_gameObject_GetComponent, go, params));
    }
    const int childCount = EiemTraceUnboxInt(
        Invoke(g_transform_get_childCount, transform));
    if (childCount <= 0) continue;
    const int bounded = childCount > 256 ? 256 : childCount;
    for (int index = 0; index < bounded && stackCount < _countof(stack);
         ++index) {
      void *params[] = {&index};
      void *child = Invoke(g_transform_GetChild, transform, params);
      if (child) stack[stackCount++] = child;
    }
  }
  if (configured || matched)
    Log("[TRACE-CHAR-RENDER-SUMMARY] stage=%s path=%s visited=%u renderers=%u "
        "configured=%u matched=%u safe=%d",
        stage ? stage : "unknown",
        pathText[0] ? pathText : "<unresolved>", visited, renderers,
        configured, matched,
        safeThread ? 1 : 0);
}

// Render sections organize a resource replacement, but their match key is the
// source Mesh resource. Consequently one rule applies to every Renderer that
// references that Mesh (including shadow and LOD instances).
static bool EiemApplyRenderRules(void *renderer, void *mesh,
                                 const char *rendererType, void *methodInfo) {
  void *identityMesh = mesh;
  EiemPrepareRenderInput(renderer, mesh, rendererType, &identityMesh);
  EiemResolvedRenderRule resolved = {};
  return EiemResolveRenderRule(identityMesh, &resolved) &&
         EiemApplyResolvedRenderRule(renderer, mesh, rendererType, methodInfo,
                                     resolved);
}

// Endfield owns source/replacement material arrays in
// EntityRenderHelperMaterialController.RendererInfo.  Scene transitions can
// commit those arrays after a prefab and its EIEM Render rule have completed.
// Mesh identity remains the rule key, so enforce only the material portion at
// that game-owned final commit boundary.  This is not a UI-specific rule and
// does not re-run mesh, skip, or partner actions.
static bool EiemReapplyRendererMaterialsAfterCommit(void *renderer,
                                                     const char *stage) {
  if (!renderer || !EiemOnUnityThread() || !il2cpp_object_get_class ||
      !il2cpp_class_get_parent || !g_skinnedMeshRendererClass)
    return false;

  bool isSkinnedRenderer = false;
  void *klass = nullptr;
  __try { klass = il2cpp_object_get_class(renderer); }
  __except (EXCEPTION_EXECUTE_HANDLER) { klass = nullptr; }
  for (int depth = 0; klass && depth < 10; ++depth) {
    if (klass == g_skinnedMeshRendererClass) {
      isSkinnedRenderer = true;
      break;
    }
    klass = il2cpp_class_get_parent(klass);
  }
  if (!isSkinnedRenderer) return false;

  void *mesh = EiemReadSharedMesh(renderer, "SkinnedMeshRenderer");
  if (!mesh) return false;
  void *identityMesh = mesh;
  EiemPrepareRenderInput(renderer, mesh, "SkinnedMeshRenderer", &identityMesh);
  EiemResolvedRenderRule resolved = {};
  if (!EiemResolveRenderRule(identityMesh, &resolved) ||
      !resolved.rule.materialCount)
    return false;

  EiemCaptureOriginal(renderer, mesh, "SkinnedMeshRenderer");
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
      !EiemAssignRendererMaterials(renderer, materials, error,
                                   sizeof(error))) {
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

// Endfield initializes its custom skinning/GPU renderer state inside
// NPCAvatarCreatorUtils.AssignSkin. Replacing sharedMesh after that point is
// too late even when the Unity Mesh arrays are valid. Mount mesh resources
// immediately before the game consumes the renderer array, then let the
// original function initialize every game-owned cache from that Mesh.
static void TraceAssignSkinPost(int32_t lod, void *renderers,
                                void *rootBones, void *closure,
                                void *methodInfo) {
  auto original = (TraceAssignSkinPostFn)s_origAssignSkinPost;
  if (original)
    original(lod, renderers, rootBones, closure, methodInfo);
  Log("[TRACE-ASSIGN-SKIN-BOUNDARY] lod=%d array=%p", lod, renderers);
}

static void TraceSetSmrRootBone(void *animator, void *renderers,
                                void *rootBoneInfos, void *methodInfo) {
  auto original = (TraceSetSmrRootBoneFn)s_origSetSmrRootBone;
  if (original) original(animator, renderers, rootBoneInfos, methodInfo);
  Log("[TRACE-ROOT-BONE-BOUNDARY] animator=%p array=%p", animator, renderers);
}

static void TraceCreateSmsGo(void *assetLoader, void *meshAssets, int32_t lod,
                             void *goPool, void *parent, void *stringList,
                             void *intList, void **renderers,
                             void **rootBones, bool flag, void *handleMap,
                             bool deferred, void *methodInfo) {
  auto original = (TraceCreateSmsGoFn)s_origCreateSmsGo;
  // CreateSMSGO consumes the logical SubMeshInfo records while constructing
  // its renderers. Apply the declared resource before entering that function;
  // a post-return Renderer assignment is too late for the game's skin/GPU
  // caches. This call uses the original method directly to avoid re-entering
  // the observation detour, then applies the same mutation routine used by
  // GetSubMeshInfo.
  if (meshAssets && s_origLodGetSubMeshInfo &&
      s_lodGetSubMeshInfoMethodInfo && EiemOnUnityThread()) {
    void *infos = ((TraceLodGetSubMeshInfoFn)s_origLodGetSubMeshInfo)(
        meshAssets, lod, false, s_lodGetSubMeshInfoMethodInfo);
    const size_t logicalApplied =
        EiemApplyLogicalSubMeshInfoArray(infos, lod, false);
    if (logicalApplied)
      Log("[MOD-LOGICAL-PRECREATE] CreateSMSGO lod=%d meshAssets=%p applied=%zu",
          lod, meshAssets, logicalApplied);
  }
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
  Log("[TRACE-SMS-BOUNDARY] CreateSMSGO lod=%d array=%p", lod, array);
}

static void TraceCreateSmsPost(void *meshAssets, int32_t lod, void *goPool,
                               void *parent, void *stringList, void *intList,
                               void **renderers, void **rootBones, bool flag,
                               void *methodInfo) {
  auto original = (TraceCreateSmsPostFn)s_origCreateSmsPost;
  // Post-model construction consumes SubMeshInfo inside the original
  // function. Mutating the returned Renderer array is too late: AssignSkin
  // has already built its skin/GPU state from the source Mesh.  Resolve the
  // same logical records before entering the game implementation, just as
  // the CreateSMSGO path does.
  if (meshAssets && s_origLodGetSubMeshInfo &&
      s_lodGetSubMeshInfoMethodInfo && EiemOnUnityThread()) {
    void *infos = ((TraceLodGetSubMeshInfoFn)s_origLodGetSubMeshInfo)(
        meshAssets, lod, false, s_lodGetSubMeshInfoMethodInfo);
    const size_t logicalApplied =
        EiemApplyLogicalSubMeshInfoArray(infos, lod, false);
    if (logicalApplied)
      Log("[MOD-LOGICAL-PREPOST] CreateSMSInfoForPostModel lod=%d "
          "meshAssets=%p applied=%zu",
          lod, meshAssets, logicalApplied);
  }
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

struct EiemModReconcileStats {
  uint32_t renderers = 0;
  uint32_t meshes = 0;
  uint32_t uniqueMeshes = 0;
  uint32_t matchedMeshes = 0;
  uint32_t appliedRenderers = 0;
};

struct EiemModReconcileContext {
  EiemModReconcileStats stats = {};
  // Mesh address -> matching entry, or -1 for a known non-match. This keeps
  // one identity/shape lookup per source resource during a full scene pass.
  std::unordered_map<void *, int> resolvedMeshes;
  std::vector<EiemResolvedRenderRule> matches;
};

static void EiemReconcileRendererVisitor(void *renderer, void *mesh,
                                         const char *rendererType,
                                         void *context) {
  auto *reconcile = (EiemModReconcileContext *)context;
  if (!reconcile || !renderer) return;
  ++reconcile->stats.renderers;
  if (!mesh) return;
  ++reconcile->stats.meshes;
  void *identityMesh = mesh;
  EiemPrepareRenderInput(renderer, mesh, rendererType, &identityMesh);
  int resolvedIndex = -1;
  const auto known = reconcile->resolvedMeshes.find(identityMesh);
  if (known != reconcile->resolvedMeshes.end()) {
    resolvedIndex = known->second;
  } else {
    ++reconcile->stats.uniqueMeshes;
    EiemResolvedRenderRule resolved = {};
    if (EiemResolveRenderRule(identityMesh, &resolved)) {
      resolvedIndex = (int)reconcile->matches.size();
      reconcile->matches.push_back(resolved);
      ++reconcile->stats.matchedMeshes;
    }
    reconcile->resolvedMeshes.emplace(identityMesh, resolvedIndex);
  }
  if (resolvedIndex >= 0 &&
      EiemApplyResolvedRenderRule(renderer, mesh, rendererType, nullptr,
                                  reconcile->matches[(size_t)resolvedIndex],
                                  true)) {
    ++reconcile->stats.appliedRenderers;
  }
}

static volatile LONG s_eiemModReconcileQueued = 0;
static volatile LONG s_eiemLifecycleRetryRemaining = 0;
static volatile LONG64 s_eiemLifecycleRetryDueMs = 0;

static void EiemQueueModReconcile(const char *reason) {
  if (g_shutdownRequested || !g_gameHwnd || !IsWindow(g_gameHwnd)) {
    Log("[MOD] Reconcile not queued (%s): game window is unavailable",
        reason ? reason : "unknown");
    return;
  }
  if (InterlockedCompareExchange(&s_eiemModReconcileQueued, 1, 0) != 0) return;
  if (!PostMessageW(g_gameHwnd, WM_EIEM_MOD_RECONCILE, 0, 0)) {
    InterlockedExchange(&s_eiemModReconcileQueued, 0);
    Log("[MOD] Reconcile post failed (%s): err=%lu",
        reason ? reason : "unknown", GetLastError());
    return;
  }
  Log("[MOD] Reconcile queued: %s", reason ? reason : "unknown");
}

// Character construction continues for several frames after
// SetMainCharacter returns. Serialized non-Mesh Renderer fields can be
// assigned without calling their setters, so schedule a short event-bound
// window of scene reconciles instead of polling for the lifetime of the
// process. The pass may mount a Mesh through the same assignment function;
// unchanged generated resources are cached, so this does not rebuild them.
static void EiemScheduleLifecycleReconcile(const char *reason) {
  InterlockedExchange(&s_eiemLifecycleRetryRemaining, 8);
  InterlockedExchange64(&s_eiemLifecycleRetryDueMs,
                        (LONG64)(GetTickCount64() + 250));
  Log("[MOD] Lifecycle reconcile window scheduled: %s",
      reason ? reason : "unknown");
}

static void EiemPumpLifecycleReconcile() {
  if (InterlockedCompareExchange(&s_eiemLifecycleRetryRemaining, 0, 0) <= 0)
    return;
  const ULONGLONG now = GetTickCount64();
  const LONG64 due = InterlockedCompareExchange64(
      &s_eiemLifecycleRetryDueMs, 0, 0);
  if ((LONG64)now < due) return;
  if (InterlockedCompareExchange64(&s_eiemLifecycleRetryDueMs,
                                   (LONG64)(now + 500), due) != due)
    return;
  InterlockedDecrement(&s_eiemLifecycleRetryRemaining);
  EiemQueueModReconcile("character construction settle");
}

// Runs only from MmdWndProc. It enumerates actual scene Renderers once after
// lifecycle events and after F10. Construction hooks remain the earliest path,
// while this pass is also allowed to re-apply Mesh resources to already-live
// renderers after a configuration or payload change.
static void EiemRunModReconcile() {
  InterlockedExchange(&s_eiemModReconcileQueued, 0);
  if (g_shutdownRequested) return;
  // This function is dispatched from the game's window procedure, which is
  // the safe Unity thread for creating generated Mesh/Material/Texture
  // objects. Startup hooks may run on the plugin worker thread instead.
  if (!s_eiemUnityThreadId) s_eiemUnityThreadId = GetCurrentThreadId();
  Log("[DEBUG-thread] reconcile tid=%lu recordedUnityTid=%lu",
      (unsigned long)GetCurrentThreadId(), (unsigned long)s_eiemUnityThreadId);
  if (!g_smr_get_sharedMesh || !g_meshFilter_get_sharedMesh) {
    Log("[MOD] Reconcile skipped: renderer APIs are not ready");
    return;
  }
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  if (generation != s_eiemAppliedModGeneration) {
    Log("[MOD] Generation transition: %ld -> %ld",
        s_eiemAppliedModGeneration, generation);
    EiemDestroyPartnerObjects();
    // Restore the game's logical SubMeshInfo values before restoring live
    // Renderers. Otherwise a later character rebuild can copy an old mod Mesh
    // back into a fresh Renderer even when the ini no longer references it.
    EiemRestoreSubMeshOverrides();
    EiemRestoreRenderOverrides();
    // Configuration reload must not release Unity Mesh objects that may still
    // be referenced by a Renderer. The resource backend reuses unchanged files
    // and creates a new rooted object only when the file stamp changes.
    s_eiemAppliedModGeneration = generation;
  }
  const ULONGLONG started = GetTickCount64();
  EiemModReconcileContext reconcile = {};
  TraceVisitRendererType(g_skinnedMeshRendererClass, "SkinnedMeshRenderer",
                         EiemReconcileRendererVisitor, &reconcile);
  TraceVisitRendererType(g_meshFilterClass, "MeshFilter",
                         EiemReconcileRendererVisitor, &reconcile);
  const ULONGLONG elapsed = GetTickCount64() - started;
  Log("[MOD] Reconcile complete: renderers=%u meshes=%u unique=%u matched=%u "
      "applied=%u elapsed=%llums",
      reconcile.stats.renderers, reconcile.stats.meshes,
      reconcile.stats.uniqueMeshes, reconcile.stats.matchedMeshes,
      reconcile.stats.appliedRenderers, elapsed);
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
static void *s_assetBundleLoadFromFile = nullptr;
static void *s_assetBundleLoadFromFileAsync = nullptr;
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

static const char *TraceGlobalResourceKind(void *object) {
  if (!object || !il2cpp_object_get_class || !il2cpp_class_get_name)
    return nullptr;
  void *klass = il2cpp_object_get_class(object);
  const char *name = klass ? il2cpp_class_get_name(klass) : nullptr;
  if (!name) return nullptr;
  if (_stricmp(name, "Mesh") == 0) return "Mesh";
  if (_stricmp(name, "Material") == 0) return "Material";
  if (_stricmp(name, "Texture2D") == 0 || _stricmp(name, "Texture") == 0)
    return "Texture";
  return nullptr;
}

// Resource declarations with target.path opt into this path. It is executed
// immediately after the original proxy resolves the logical resource and
// before the caller can build a Renderer. The original VFS/decryption chain
// remains authoritative for identity and timing; only the returned Unity
// object is redirected. The generated replacement is cached and shared by
// every later proxy/Renderer consumer.
static void *TraceTryGlobalResourceRedirect(void *originalObject,
                                            const char *logicalPath) {
  if (!originalObject || s_traceBuildingGlobalResource)
    return originalObject;
  const char *kind = TraceGlobalResourceKind(originalObject);
  if (!kind) return originalObject;
  char assetName[256] = {};
  TraceReadUnityObjectName(originalObject, assetName, sizeof(assetName));
  EiemModResource resource = {};
  bool found = logicalPath && logicalPath[0] &&
               EiemFindGlobalResource(logicalPath, assetName, kind, &resource);
  if (!found && assetName[0])
    found = EiemFindGlobalResourceByAsset(assetName, kind, &resource);
  if (assetName[0] && EiemHasResourceRenderRuleAsset(assetName)) {
    Log("[DEBUG-RESOURCE-ROUTE] kind=%s path=%s asset=%s found=%d tid=%lu",
        kind ? kind : "?", logicalPath && logicalPath[0] ? logicalPath : "<none>",
        assetName, found ? 1 : 0, (unsigned long)GetCurrentThreadId());
  }
  if (!found) return originalObject;
  const char *resolvedPath = logicalPath && logicalPath[0]
                                 ? logicalPath
                                 : resource.targetPath;
  if (!EiemOnUnityThread()) {
    Log("[RES-REDIRECT] matched but unsafe thread kind=%s target=%s asset=%s "
        "tid=%lu unityTid=%lu",
        kind, resolvedPath, assetName[0] ? assetName : "<any>",
        (unsigned long)GetCurrentThreadId(),
        (unsigned long)s_eiemUnityThreadId);
    return originalObject;
  }

  void *replacement = nullptr;
  char error[256] = {};
  s_traceBuildingGlobalResource = true;
  const bool built = EiemBuildGlobalResource(resource, originalObject,
                                             &replacement, error,
                                             sizeof(error));
  s_traceBuildingGlobalResource = false;
  if (!built || !replacement) {
    Log("[RES-REDIRECT] failed kind=%s target=%s asset=%s error=%s", kind,
        resolvedPath, assetName[0] ? assetName : "<any>",
        error[0] ? error : "unknown");
    return originalObject;
  }
  TraceRememberAssetOrigin(replacement, 0, resolvedPath);
  Log("[RES-REDIRECT] applied kind=%s target=%s asset=%s replacement=%p",
      kind, resolvedPath, assetName[0] ? assetName : "<any>", replacement);
  return replacement;
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

static bool TraceFindVfsOverride(void *path, char *overridePath,
                                 int overridePathSize) {
  if (!path || !overridePath || overridePathSize <= 0) return false;
  char pathText[768] = {};
  TraceDescribeString(path, pathText, sizeof(pathText));
  if (!pathText[0] || pathText[0] == '?' || pathText[0] == '\\' ||
      pathText[0] == '/' || strstr(pathText, ".."))
    return false;

  for (char *p = pathText; *p; ++p)
    if (*p == '/') *p = '\\';

  int written = snprintf(overridePath, overridePathSize,
                         "plugin\\mods\\override\\%s", pathText);
  if (written <= 0 || written >= overridePathSize) return false;
  DWORD attr = GetFileAttributesA(overridePath);
  return attr != INVALID_FILE_ATTRIBUTES &&
         !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void *TraceCreateManagedPath(const char *pathText) {
  if (!pathText || !il2cpp_string_new || !il2cpp_domain_get ||
      !il2cpp_thread_attach)
    return nullptr;
  if (!s_vfsThreadAttached) {
    void *domain = il2cpp_domain_get();
    if (!domain) return nullptr;
    void *thread = il2cpp_thread_attach(domain);
    if (!thread) return nullptr;
    s_vfsThreadAttached = true;
  }
  __try {
    return il2cpp_string_new(pathText);
  } __except (1) {
    return nullptr;
  }
}

// Replacement bundles are plaintext Unity bundles in the mod directory. They
// bypass the encrypted VFS and are loaded by Unity; the original VFS call is
// retained as the fallback on any failure.
static void *TraceLoadPlaintextBundle(const char *replacementPath,
                                      bool asynchronous) {
  if (!replacementPath || !replacementPath[0] ||
      !il2cpp_runtime_invoke || !il2cpp_string_new)
    return nullptr;
  void *method = asynchronous ? s_assetBundleLoadFromFileAsync
                              : s_assetBundleLoadFromFile;
  if (!method) return nullptr;
  void *managedPath = TraceCreateManagedPath(replacementPath);
  if (!managedPath) return nullptr;
  void *params[] = {managedPath};
  return Invoke(method, nullptr, params);
}

static void *TraceTryBundleOverride(void *path, bool asynchronous) {
  char overridePath[1024] = {};
  if (!TraceFindVfsOverride(path, overridePath, sizeof(overridePath)))
    return nullptr;
  void *replacement = TraceLoadPlaintextBundle(overridePath, asynchronous);
  if (replacement) {
    if (TraceTakeBudget(&s_traceVfsPathCount, 300))
      Log("[VFS-OVERRIDE] bundle %s loaded plaintext path=%s",
          asynchronous ? "async" : "sync", overridePath);
    return replacement;
  }
  if (TraceTakeBudget(&s_traceVfsPathCount, 300))
    Log("[VFS-OVERRIDE] bundle %s failed, falling back path=%s",
        asynchronous ? "async" : "sync", overridePath);
  return nullptr;
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
  // A resolved proxy may expose only an asset name (its logical path is
  // available on another internal handle).  The redirect routine deliberately
  // supports an empty path and falls back to a unique asset declaration.
  if (result) {
    void *redirected = TraceTryGlobalResourceRedirect(
        result, (pathText[0] && pathText[0] != '<') ? pathText : nullptr);
    if (redirected != result) result = redirected;
  }
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
    void *redirected = TraceTryGlobalResourceRedirect(
        result, (pathText[0] && pathText[0] != '<') ? pathText : nullptr);
    if (redirected != result) result = redirected;
  }
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
  if (void *replacement = TraceTryBundleOverride(path, false)) {
    TraceRememberActiveBundle(replacement, path);
    return replacement;
  }
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
  if (void *replacement = TraceTryBundleOverride(path, true)) {
    TraceRememberPendingBundleRequest(replacement, path);
    return replacement;
  }
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
  if (void *replacement = TraceTryBundleOverride(path, false)) {
    TraceRememberActiveBundle(replacement, path);
    return replacement;
  }
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
  if (void *replacement = TraceTryBundleOverride(path, true)) {
    TraceRememberPendingBundleRequest(replacement, path);
    return replacement;
  }
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
  if (original) original(self, asset, methodInfo);
  if (asset) {
    char assetName[768] = {};
    const int64_t pathHash = TraceReadLoadableHash(self);
    if (pathHash)
      TraceLookupHashPath(pathHash, assetName, sizeof(assetName));
    if (!assetName[0]) TraceReadAssetName(self, assetName, sizeof(assetName));
    TraceRememberAssetOrigin(asset, pathHash, assetName);
  }
  if (!s_traceReentrant &&
      TraceTakeBudget(&s_traceAssetCompleteCount, 600)) {
    s_traceReentrant = true;
    char assetText[512] = {};
    char assetName[768] = {};
    int64_t pathHash = TraceReadLoadableHash(self);
    TraceReadAssetName(self, assetName, sizeof(assetName));
    TraceDescribeObject(asset, assetText, sizeof(assetText));
    Log("[RES-TRACE] Asset._FinishWithAsset: hash=%lld loader=%p "
        "assetName=\"%s\" asset=%s",
        (long long)pathHash, self, assetName[0] ? assetName : "?", assetText);
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
  char redirectPath[768] = {};
  TraceDescribeString(path, redirectPath, sizeof(redirectPath));
  if (result) {
    void *redirected = TraceTryGlobalResourceRedirect(
        result, redirectPath[0] ? redirectPath : nullptr);
    if (redirected != result) result = redirected;
  }
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
  char redirectPath[768] = {};
  TraceDescribeString(path, redirectPath, sizeof(redirectPath));
  if (result) {
    void *redirected = TraceTryGlobalResourceRedirect(
        result, redirectPath[0] ? redirectPath : nullptr);
    if (redirected != result) result = redirected;
  }
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
  void *sourceMesh = mesh;
  // Some game paths bypass FAssetProxyHandle.Get and assign the resolved Mesh
  // directly. Use the origin table populated by the resource hooks as a
  // second global-redirection boundary before applying per-Renderer rules.
  char meshPath[768] = {};
  if (TraceLookupAssetOrigin(mesh, nullptr, meshPath, sizeof(meshPath))) {
    void *redirected = TraceTryGlobalResourceRedirect(mesh, meshPath);
    if (redirected != mesh) mesh = redirected;
  }
  // Setter callbacks can run on asset-loader threads. The resource redirect
  // above is the preferred path and runs before this setter when the proxy
  // exposes a logical path. This block only handles direct assignments that
  // bypass the proxy, and only on the Unity thread.
  char assetName[192] = {};
  bool hasModRule =
      EiemReadLiveMeshIdentity(mesh, nullptr, 0, assetName, sizeof(assetName)) &&
      EiemHasResourceRenderRuleAsset(assetName);
  // A prefab can assign an embedded Mesh before Object.name is initialized.
  // In that short window the Renderer name is the only stable logical asset
  // identity available. It is still constrained by the same explicit rule
  // asset and shape checks below; this is not a name-only global redirect.
  if (!hasModRule) {
    char rendererText[512] = {};
    char rendererName[192] = {};
    TraceDescribeObject(self, rendererText, sizeof(rendererText));
    EiemExtractObjectName(rendererText, rendererName, sizeof(rendererName));
    if (rendererName[0] && EiemHasResourceRenderRuleAsset(rendererName)) {
      strncpy_s(assetName, sizeof(assetName), rendererName, _TRUNCATE);
      hasModRule = true;
      Log("[MOD-EARLY-IDENTITY] renderer=%p asset=%s mesh=%p",
          self, assetName, mesh);
    }
  }
  bool earlyApplied = false;
  bool hasMeshDirective = false;
  const bool safeThread = EiemOnUnityThread();
  if (hasModRule && safeThread) {
    void *identityMesh = sourceMesh;
    EiemPrepareRenderInput(self, mesh, "SkinnedMeshRenderer", &identityMesh);
    EiemResolvedRenderRule resolved = {};
    bool resolvedRule = EiemResolveRenderRule(identityMesh, &resolved);
    if (!resolvedRule && assetName[0])
      resolvedRule = EiemResolveRenderRuleForAsset(identityMesh, assetName,
                                                   &resolved);
    if (resolvedRule) {
      hasMeshDirective = resolved.rule.hasMesh;
      const bool globalAlreadyApplied = mesh != sourceMesh;
      const bool hasRenderAction = resolved.rule.hasMesh ||
                                   EiemModEquals(resolved.rule.handling, "skip") ||
                                   resolved.rule.materialCount ||
                                   resolved.rule.partnerCount;
      if (hasRenderAction) {
        EiemApplyResolvedRenderRule(
            self, globalAlreadyApplied ? sourceMesh : mesh,
            "SkinnedMeshRenderer", methodInfo, resolved,
            !globalAlreadyApplied);
        void *currentMesh = EiemReadSharedMesh(self, "SkinnedMeshRenderer");
        earlyApplied = globalAlreadyApplied ||
                       (currentMesh && currentMesh != sourceMesh);
        Log("[MOD-EARLY-SETTER] renderer=%p source=%p current=%p global=%d applied=%d",
            self, sourceMesh, currentMesh, globalAlreadyApplied ? 1 : 0,
            earlyApplied ? 1 : 0);
      }
    }
  }
  if (!earlyApplied && original) original(self, mesh, methodInfo);
  if (hasMeshDirective && !earlyApplied)
    Log("[MOD-EARLY-SETTER] mesh directive was not applied before setter renderer=%p source=%p",
        self, sourceMesh);
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
  char meshPath[768] = {};
  if (TraceLookupAssetOrigin(mesh, nullptr, meshPath, sizeof(meshPath))) {
    void *redirected = TraceTryGlobalResourceRedirect(mesh, meshPath);
    if (redirected != mesh) mesh = redirected;
  }
  char assetName[192] = {};
  const bool hasModRule =
      EiemReadLiveMeshIdentity(mesh, nullptr, 0, assetName, sizeof(assetName)) &&
      EiemHasResourceRenderRuleAsset(assetName);
  if (original) original(self, mesh, methodInfo);
  // Mesh replacement is completed at the resource boundary. Only non-mesh
  // Render directives still need a main-thread reconcile after direct
  // MeshFilter assignments.
  if (hasModRule) {
    EiemResolvedRenderRule resolved = {};
    if (EiemResolveRenderRule(mesh, &resolved) &&
        !resolved.rule.hasMesh &&
        (EiemModEquals(resolved.rule.handling, "skip") ||
         resolved.rule.materialCount))
      EiemQueueModReconcile("MeshFilter non-mesh directive");
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
      return klass;
    }
  }
  return nullptr;
}

static void InitIl2CppResourceTrace(void **assemblies, size_t assemblyCount) {
  if (!assemblies || assemblyCount == 0) return;

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
    void *completed = FindMethodWithReturnType(
        prefabInstantiateClass, "OnCompleted", "System.Void", 0);
    if (completed &&
        Hook(completed, "PrefabInstantiateProxy.OnCompleted",
             (void *)TracePrefabInstantiateCompleted,
             &s_origPrefabInstantiateCompleted)) {
      Log("[RES-TRACE] PrefabInstantiateProxy.OnCompleted replacement hook installed; getGameObject=%p",
          s_prefabInstantiateGetGameObject);
    } else {
      Log("[RES-TRACE] PrefabInstantiateProxy.OnCompleted hook failed/not found");
    }
  } else {
    Log("[RES-TRACE] Beyond.Resource.Runtime.PrefabInstantiateProxy class not found");
  }

  void *meshFilterClass =
      FindClass("UnityEngine", "MeshFilter", assemblies, assemblyCount);
  HookTraceMethod(meshFilterClass, "set_sharedMesh", 1,
                  "MeshFilter.set_sharedMesh",
                  (void *)TraceMeshFilterSetSharedMesh,
                  &s_origMeshFilterSetSharedMesh);

  // Generic character/model lifecycle observation. NPC Avatar has a separate
  // construction path below; these hooks identify the path used by player and
  // other BaseModelComponent-backed models without changing any render state.
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
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinishCallback early replacement hook installed");
    else
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinishCallback hook failed/not found");

    void *loadFinishResult = FindMethodWithParamTypesAndReturnType(
        basePartClass, "_OnLoadModelFinish", loadFinishTypes,
        _countof(loadFinishTypes), "System.Boolean");
    if (loadFinishResult &&
        Hook(loadFinishResult, "BaseModelViewPart._OnLoadModelFinish",
             (void *)TraceBasePartLoadFinishResult,
             &s_origBasePartLoadFinishResult))
      Log("[RES-TRACE] BaseModelViewPart._OnLoadModelFinish early replacement hook installed");
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
  if (assetBundleClass) {
    s_assetBundleLoadFromFile = FindMethod(assetBundleClass, "LoadFromFile", 1);
    s_assetBundleLoadFromFileAsync =
        FindMethod(assetBundleClass, "LoadFromFileAsync", 1);
    Log("[RES-TRACE] Unity AssetBundle plaintext loaders: sync=%p async=%p",
        s_assetBundleLoadFromFile, s_assetBundleLoadFromFileAsync);
  }
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

  Log("[RES-TRACE] Observation hooks ready; VFS path override probe enabled");
}
