#pragma once

#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "eiem_mods.h"
#include "eiem_resource_backend.h"

// Resource-loading observation hooks for the first investigation. Unity and
// game-resource hooks remain pass-through; VFS path hooks additionally probe
// plugin\\mods\\override and fall back to the original path on a miss or
// failed load. They are intentionally limited so startup remains low-noise.

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
static void TraceBuildRendererHierarchy(void *renderer, char *out,
                                        size_t outSize);
static bool TraceLookupAssetOrigin(void *asset, int64_t *pathHash, char *path,
                                   size_t pathSize);
static void EiemExtractObjectName(const char *description, char *out,
                                   size_t outSize);
typedef void (__fastcall *TraceSetSharedMeshFn)(void *self, void *mesh,
                                                 void *methodInfo);
static void *s_origSkinnedMeshSetSharedMesh = nullptr;
static void *s_origMeshFilterSetSharedMesh = nullptr;
// Set while the reconciliation pass calls Unity's managed setter. The setter
// hooks then forward to their original trampoline instead of recursively
// resolving the replacement that is already being assigned.
static thread_local bool s_eiemApplyingModMeshAssignment = false;
static thread_local bool s_eiemCreatingPartner = false;
static void TraceSkinnedMeshSetSharedMesh(void *self, void *mesh,
                                           void *methodInfo);
static void TraceMeshFilterSetSharedMesh(void *self, void *mesh,
                                          void *methodInfo);

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
    return true;
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
  return true;
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
  bool originalEnabled = true;
  bool hasEnabled = false;
  bool hasMaterials = false;
  char rendererType[32] = {};
};
static SRWLOCK s_eiemOverrideLock = SRWLOCK_INIT;
static std::vector<EiemRenderOverrideState> s_eiemOverrides;
static volatile LONG s_eiemAppliedModGeneration = -1;

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
      state.hasEnabled = EiemReadRendererEnabled(renderer, &state.originalEnabled);
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
    state.hasEnabled = EiemReadRendererEnabled(renderer, &state.originalEnabled);
    if (g_renderer_get_sharedMaterials && il2cpp_gchandle_new) {
      void *materials = Invoke(g_renderer_get_sharedMaterials, renderer);
      if (materials) {
        state.originalMaterialsHandle = il2cpp_gchandle_new(materials, false);
        state.hasMaterials = state.originalMaterialsHandle != 0;
      }
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
    Log("[DEBUG-hr1] restore renderer=%p original=%p replacement=%p originalEnabled=%d",
        state.renderer, state.originalMesh, state.replacementMesh,
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
    if (state.originalMaterialsHandle && il2cpp_gchandle_free)
      il2cpp_gchandle_free(state.originalMaterialsHandle);
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
      (!g_gameObject_ctor && !g_gameObject_ctorDefault) || !g_gameObject_AddComponent ||
      !g_component_get_gameObject || !g_component_get_transform ||
      !g_transform_set_parent || !il2cpp_class_get_type ||
      !il2cpp_type_get_object) {
    if (error) strncpy_s(error, errorSize,
                         "Unity GameObject/Transform creation APIs are unavailable",
                         _TRUNCATE);
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
  void *partnerTransform = g_gameObject_get_transform
                               ? Invoke(g_gameObject_get_transform, partnerGo)
                               : nullptr;
  if (!partnerTransform) {
    if (error) strncpy_s(error, errorSize, "Partner GameObject has no Transform", _TRUNCATE);
    cleanupPartner();
    return nullptr;
  }
  bool worldPositionStays = false;
  void *parentParams[] = {sourceTransform, &worldPositionStays};
  Invoke(g_transform_set_parent, partnerTransform, parentParams);
  EiemCopyPartnerTransform(sourceTransform, partnerTransform);

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
  void *partnerRenderer = Invoke(g_gameObject_AddComponent, partnerGo,
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
                             sizeof(buildError))) {
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
  }
  if (partnerRule.materialCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterials(partnerRule, &materials, buildError,
                                    sizeof(buildError)) ||
        !materials || !s_eiemRendererSetSharedMaterials) {
      if (error) strncpy_s(error, errorSize, buildError[0] ? buildError : "Partner materials failed", _TRUNCATE);
      cleanupPartner();
      return nullptr;
    }
    void *params[] = {materials};
    Invoke(s_eiemRendererSetSharedMaterials, partnerRenderer, params);
  } else if (g_renderer_get_sharedMaterials && s_eiemRendererSetSharedMaterials) {
    void *materials = Invoke(g_renderer_get_sharedMaterials, sourceRenderer);
    if (materials) { void *params[] = {materials}; Invoke(s_eiemRendererSetSharedMaterials, partnerRenderer, params); }
  }
  bool enabled = !EiemModEquals(partnerRule.handling, "skip");
  if (g_renderer_set_enabled) { void *params[] = {&enabled}; Invoke(g_renderer_set_enabled, partnerRenderer, params); }
  EiemSetPartnerLodMembership(sourceRenderer, partnerRenderer, true);

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

static bool EiemApplyResolvedRenderRule(void *renderer, void *mesh,
                                        const char *rendererType,
                                        void *methodInfo,
                                        const EiemResolvedRenderRule &resolved) {
  if (!renderer || !mesh) return false;
  const EiemModRule &rule = resolved.rule;
  const char *source = resolved.source[0] ? resolved.source : "<unknown>";
  const char *asset = resolved.asset[0] ? resolved.asset : "<unknown>";

  // `mesh` and `handling=skip` are independent directives. A rule with
  // neither directive is a match-only declaration and must pass through the
  // game's setter unchanged.
  const bool skipOriginal = EiemModEquals(rule.handling, "skip");
  if (!rule.hasMesh && !skipOriginal && !rule.materialCount && !rule.partnerCount)
    return false;

  EiemCaptureOriginal(renderer, mesh, rendererType);
  if (!rule.hasMesh && skipOriginal) {
    // A setter hook has not yet forwarded the game's assignment, so preserve
    // it before disabling the original Renderer. Reconcile already observes
    // the current mesh and does not need a redundant setter call.
    if (methodInfo && !EiemSetSharedMesh(renderer, mesh, rendererType, methodInfo))
      return false;
    // Skip changes only the renderer state. The original Mesh remains the
    // identity resource and must never be recorded as a replacement.
    EiemRememberReplacement(renderer, nullptr, rendererType);
    if (g_renderer_set_enabled) {
      EiemSetRendererEnabled(renderer, false);
      Log("[MOD] %s resource skip applied: source=%s asset=%s", rendererType,
          source, asset);
    }
    EiemApplyPartners(renderer, mesh, rendererType, rule);
    return true;
  }
  char error[256] = {};
  void *assignedMesh = mesh;
  if (rule.hasMesh) {
    if (!EiemBuildMeshResource(rule, &assignedMesh, error, sizeof(error))) {
      Log("[MOD] %s mesh resource failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.mesh,
          error[0] ? error : "unknown");
      assignedMesh = mesh;
    }
  }
  if (!EiemSetSharedMesh(renderer, assignedMesh, rendererType, methodInfo))
    return false;
  EiemRememberReplacement(renderer, assignedMesh != mesh ? assignedMesh : nullptr,
                          rendererType);
  Log("[DEBUG-hr1] assign renderer=%p incoming=%p assigned=%p methodInfo=%p",
      renderer, mesh, assignedMesh, methodInfo);
  void *actualMesh = EiemReadSharedMesh(renderer, rendererType);
  Log("[DEBUG-hr1] assign result renderer=%p actual=%p expected=%p ok=%d",
      renderer, actualMesh, assignedMesh, actualMesh == assignedMesh ? 1 : 0);
  // A previous generation may have disabled this renderer. Explicitly restore
  // the captured game state after assigning a replacement instead of relying
  // on the ordering of Unity setter calls.
  bool originalEnabled = true;
  if (skipOriginal) {
    EiemSetRendererEnabled(renderer, false);
  } else if (EiemGetOriginalEnabled(renderer, &originalEnabled)) {
    EiemSetRendererEnabled(renderer, originalEnabled);
  }
  if (rule.materialCount) {
    void *materials = nullptr;
    if (!EiemBuildRendererMaterialsForSource(rule, renderer, &materials, error,
                                             sizeof(error))) {
      Log("[MOD] %s material resource failed: source=%s asset=%s section=%s error=%s",
          rendererType, source, asset, rule.section,
          error[0] ? error : "unknown");
      // Do not leave a partially applied Render. The source Mesh and enabled
      // state are restored on both setter-hook and reconcile paths; returning
      // true prevents the caller from applying a second, different mutation.
      if (assignedMesh != mesh)
        EiemSetSharedMesh(renderer, mesh, rendererType, methodInfo);
      EiemRememberReplacement(renderer, nullptr, rendererType);
      bool restoreEnabled = true;
      if (EiemGetOriginalEnabled(renderer, &restoreEnabled))
        EiemSetRendererEnabled(renderer, restoreEnabled);
      return true;
    } else if (materials && s_eiemRendererSetSharedMaterials) {
      void *materialParams[] = {materials};
      Invoke(s_eiemRendererSetSharedMaterials, renderer, materialParams);
    }
  }
  bool currentEnabled = true;
  const bool readEnabled = EiemReadRendererEnabled(renderer, &currentEnabled);
  Log("[MOD] %s resource rule applied: source=%s asset=%s mesh=%s materials=%u skip=%s enabled=%s",
      rendererType, source, asset,
      rule.hasMesh ? rule.mesh : "<original>", rule.materialCount,
      skipOriginal ? "true" : "false",
      readEnabled ? (currentEnabled ? "true" : "false") : "unknown");
  EiemApplyPartners(renderer, assignedMesh, rendererType, rule);
  return true;
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
    void *enumerator = g_object_find_objects_of_type
                           ? g_object_find_objects_of_type
                           : g_resources_find_objects_of_type_all;
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
                                  reconcile->matches[(size_t)resolvedIndex])) {
    ++reconcile->stats.appliedRenderers;
  }
}

static volatile LONG s_eiemModReconcileQueued = 0;

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

// Runs only from MmdWndProc. It enumerates actual scene Renderers once after
// lifecycle events, while setter hooks cover assignments made later.
static void EiemRunModReconcile() {
  InterlockedExchange(&s_eiemModReconcileQueued, 0);
  if (g_shutdownRequested) return;
  if (!g_smr_get_sharedMesh || !g_meshFilter_get_sharedMesh) {
    Log("[MOD] Reconcile skipped: renderer APIs are not ready");
    return;
  }
  const LONG generation = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  if (generation != s_eiemAppliedModGeneration) {
    Log("[MOD] Generation transition: %ld -> %ld",
        s_eiemAppliedModGeneration, generation);
    EiemDestroyPartnerObjects();
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
  if (result) {
    if (!TraceBindAssetFromProxy(self, result)) {
      auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
      char pathText[768] = {};
      if (pathGetter)
        TraceDescribeString(pathGetter(self, nullptr), pathText,
                            sizeof(pathText));
      if (pathText[0] && pathText[0] != '<')
        TraceRememberProxyOrigin(self, 0, pathText);
      TraceBindAssetFromProxy(self, result);
    }
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyGetCount, 500)) {
    s_traceReentrant = true;
    char objectText[512] = {};
    TraceDescribeObject(result, objectText, sizeof(objectText));
    Log("[RES-TRACE] FAssetProxyHandle.Get: handle=%p object=%s", self,
        objectText);
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
  if (result) {
    // Untracked handles do not expose the tracked proxy's origin table. Their
    // path getter is still a managed, read-only identity source, so bind it
    // before the object reaches a Renderer.
    auto pathGetter = (TraceProxyObjectFn)s_origAssetProxyUntrackedPath;
    if (!pathGetter)
      pathGetter = (TraceProxyObjectFn)s_origAssetProxyHandlePath;
    char pathText[768] = {};
    if (pathGetter)
      TraceDescribeString(pathGetter(self, nullptr), pathText,
                          sizeof(pathText));
    if (pathText[0] && pathText[0] != '<') {
      TraceRememberProxyOrigin(self, 0, pathText);
      TraceBindAssetFromProxy(self, result);
    }
  }
  if (!s_traceReentrant && TraceTakeBudget(&s_traceProxyGetCount, 500)) {
    s_traceReentrant = true;
    char objectText[512] = {};
    TraceDescribeObject(result, objectText, sizeof(objectText));
    Log("[RES-TRACE] FAssetProxyUntrackedHandle.Get: handle=%p object=%s",
        self, objectText);
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
  TraceRememberMeshObservation(self, mesh, "SkinnedMeshRenderer");
  const bool handled = EiemApplyRenderRules(self, mesh, "SkinnedMeshRenderer", methodInfo);
  if (!handled && original) original(self, mesh, methodInfo);
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
  const bool handled = EiemApplyRenderRules(self, mesh, "MeshFilter", methodInfo);
  if (!handled && original) original(self, mesh, methodInfo);
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

  void *smrClass = FindClass("UnityEngine", "SkinnedMeshRenderer", assemblies,
                            assemblyCount);
  HookTraceMethod(smrClass, "set_sharedMesh", 1,
                  "SkinnedMeshRenderer.set_sharedMesh",
                  (void *)TraceSkinnedMeshSetSharedMesh,
                  &s_origSkinnedMeshSetSharedMesh);

  void *meshFilterClass =
      FindClass("UnityEngine", "MeshFilter", assemblies, assemblyCount);
  HookTraceMethod(meshFilterClass, "set_sharedMesh", 1,
                  "MeshFilter.set_sharedMesh",
                  (void *)TraceMeshFilterSetSharedMesh,
                  &s_origMeshFilterSetSharedMesh);

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
