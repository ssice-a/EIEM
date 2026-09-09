#pragma once

// Temporary v39 read-only diagnostic. Remove this file and EiemProbe* call sites
// after the map-reuse investigation. No setters, strong roots, cache eviction,
// or replacement decisions belong here. Names select diagnostic candidates only.
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

static void *s_eiemProbeAlive = nullptr, *s_eiemProbeId = nullptr;
static void *s_eiemProbeTextureNames = nullptr;
static uint32_t (*s_eiemProbeWeak)(void *, bool) = nullptr;
static thread_local bool s_eiemProbeReading = false;
static uint64_t s_eiemProbeSequence = 0;
static unsigned s_eiemProbeEvents = 0;

struct EiemProbeObject {
  uint32_t weak = 0;
  std::string kind, label;
  bool renderer = false;
};
struct EiemProbeWatch {
  std::string mod, section, meshName;
};
static std::vector<EiemProbeObject> s_eiemProbeObjects;
static std::vector<EiemProbeWatch> s_eiemProbeWatches;
static std::unordered_map<std::string, std::string> s_eiemProbeLast;

// This RAII guard covers every Unity read, including weak-handle resolution.
// Observer re-entry never changes the caller's normal execution path.
struct EiemProbeReadScope {
  bool entered = false;
  EiemProbeReadScope() {
    entered = !s_eiemProbeReading && EiemOnUnityThread();
    if (entered) s_eiemProbeReading = true;
  }
  ~EiemProbeReadScope() { if (entered) s_eiemProbeReading = false; }
};

static void *EiemProbeInvoke(void *method, void *self, void **params = nullptr) {
  if (!method || !il2cpp_runtime_invoke) return nullptr;
  void *exception = nullptr;
  void *result = nullptr;
  __try { result = il2cpp_runtime_invoke(method, self, params, &exception); }
  __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
  return exception ? nullptr : result;
}

// 1 = native object alive, 0 = native object absent, -1 = unavailable/read error.
// A non-null managed handle alone is deliberately NOT treated as native alive.
static int EiemProbeNativeAlive(void *object) {
  if (!object) return 0;
  void *params[] = {object};
  void *boxed = EiemProbeInvoke(s_eiemProbeAlive, nullptr, params);
  if (!boxed || !il2cpp_object_unbox) return -1;
  void *value = il2cpp_object_unbox(boxed);
  return value ? (*(bool *)value ? 1 : 0) : -1;
}

static std::string EiemProbeObjectText(void *object) {
  const int alive = EiemProbeNativeAlive(object);
  int id = 0;
  char name[192] = {};
  if (alive == 1) {
    void *boxed = EiemProbeInvoke(s_eiemProbeId, object);
    void *value = boxed && il2cpp_object_unbox ? il2cpp_object_unbox(boxed) : nullptr;
    if (value) id = *(int *)value;
    TraceReadUnityObjectName(object, name, sizeof(name));
  }
  char text[320] = {};
  snprintf(text, sizeof(text), "%p/native=%d/id=%d/name=%s", object, alive, id, name);
  return text;
}

static bool EiemProbeLogAllowed() {
  // Cap expensive detail per explicit update, not by the game's unrelated meshes.
  if (s_eiemProbeEvents++ < 8192) return true;
  if (s_eiemProbeEvents == 8193)
    Log("[DEBUG-residue-v39] detail limit reached; next F10 opens another capture window");
  return false;
}

static void EiemProbeLine(const char *stage, const char *kind, const std::string &detail) {
  if (!EiemProbeLogAllowed()) return;
  Log("[DEBUG-residue-v39] seq=%llu ms=%llu tid=%lu gen=%ld stage=%s kind=%s %s",
      ++s_eiemProbeSequence, GetTickCount64(), GetCurrentThreadId(),
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0), stage, kind, detail.c_str());
}

static void EiemProbeChanged(const char *stage, const char *kind,
                             const std::string &key, const std::string &detail) {
  auto &previous = s_eiemProbeLast[std::string(stage) + "|" + kind + "|" + key];
  if (previous == detail) return;
  previous = detail;
  EiemProbeLine(stage, kind, detail);
}

static void EiemProbeTrack(void *object, const char *kind,
                            const std::string &label, bool renderer = false) {
  if (!object || !s_eiemProbeWeak || !il2cpp_gchandle_get_target || !il2cpp_gchandle_free) return;
  for (auto it = s_eiemProbeObjects.begin(); it != s_eiemProbeObjects.end();) {
    void *target = il2cpp_gchandle_get_target(it->weak);
    if (!target) {
      ++it; // Pruned at checkpoint, never while a copied handle list is in use.
      continue;
    }
    if (target == object && it->kind == kind && it->label == label) return;
    ++it;
  }
  if (s_eiemProbeObjects.size() >= 2048) {
    EiemProbeLine("capacity", "weak-watch", "observation truncated; no strong-reference fallback");
    return;
  }
  uint32_t weak = s_eiemProbeWeak(object, false);
  if (weak) s_eiemProbeObjects.push_back({weak, kind, label, renderer});
  else EiemProbeLine("weak-failed", kind, label);
}

static bool EiemProbeMatches(void *renderer, void *mesh) {
  for (const auto &entry : s_eiemProbeObjects)
    if (entry.renderer && il2cpp_gchandle_get_target &&
        il2cpp_gchandle_get_target(entry.weak) == renderer) return true;
  if (EiemProbeNativeAlive(mesh) != 1) return false;
  char name[192] = {};
  TraceReadUnityObjectName(mesh, name, sizeof(name));
  for (const auto &watch : s_eiemProbeWatches)
    if (EiemModEquals(watch.meshName.c_str(), name)) return true;
  return false;
}

static void EiemProbeMaterials(const char *stage, void *array, const std::string &owner) {
  if (!array) { EiemProbeLine(stage, "materials", owner + " array=null"); return; }
  size_t count = EiemManagedArrayLength(array);
  if (count > 128) { EiemProbeLine(stage, "materials", owner + " slot limit exceeded"); return; }
  void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
  for (size_t slot = 0; slot < count; ++slot) {
    void *material = items[slot];
    std::string label = owner + " slot=" + std::to_string(slot);
    EiemProbeChanged(stage, "material", label, label + " object=" + EiemProbeObjectText(material));
    if (EiemProbeNativeAlive(material) != 1 || !s_eiemProbeTextureNames || !s_eiemMaterialGetTexture) continue;
    void *names = EiemProbeInvoke(s_eiemProbeTextureNames, material);
    size_t textures = EiemManagedArrayLength(names);
    if (!names || textures > 128) continue;
    void **properties = (void **)((char *)names + IL2CPP_ARRAY_DATA);
    for (size_t i = 0; i < textures; ++i) {
      char property[160] = {};
      if (properties[i]) ReadStrUtf8(properties[i], property, sizeof(property));
      void *params[] = {properties[i]};
      void *texture = EiemProbeInvoke(s_eiemMaterialGetTexture, material, params);
      EiemProbeChanged(stage, "texture", label + property, label + " property=" + property + " object=" + EiemProbeObjectText(texture));
    }
  }
}

static void EiemProbeRendererRead(const char *stage, void *renderer, void *drawRenderer,
                                 const char *type, void *incoming = nullptr, bool force = false) {
  if (!renderer || s_eiemProbeEvents > 8192) return;
  const int alive = EiemProbeNativeAlive(renderer);
  void *mesh = alive == 1 ? EiemReadSharedMesh(renderer, type) : nullptr;
  if (!force && !EiemProbeMatches(renderer, incoming ? incoming : mesh)) return;
  EiemProbeTrack(renderer, type, "consumer", true);
  std::string owner = "renderer=" + EiemProbeObjectText(renderer);
  EiemProbeChanged(stage, type, owner, owner + " mesh=" + EiemProbeObjectText(mesh) +
                " incoming=" + EiemProbeObjectText(incoming));
  if (EiemProbeNativeAlive(mesh) == 1) {
    int vertices = -1, indices = -1, submeshes = -1;
    EiemReadLiveMeshShape(mesh, &vertices, &indices, &submeshes);
    EiemProbeChanged(stage, "geometry", owner, owner + " vertices=" + std::to_string(vertices) +
                  " indices=" + std::to_string(indices) + " submeshes=" + std::to_string(submeshes));
  }
  if (EiemProbeNativeAlive(drawRenderer) == 1) {
    bool enabled = false, visible = false;
    bool hasEnabled = EiemReadRendererEnabled(drawRenderer, &enabled);
    bool hasVisible = EiemReadRendererVisible(drawRenderer, &visible);
    EiemProbeChanged(stage, "draw", owner, owner + " enabled=" + std::to_string(hasEnabled ? (int)enabled : -1) +
                  " visible=" + std::to_string(hasVisible ? (int)visible : -1));
    EiemProbeMaterials(stage, EiemProbeInvoke(g_renderer_get_sharedMaterials, drawRenderer), owner);
  }
}

static void EiemProbeRenderer(const char *stage, void *renderer, void *drawRenderer,
                              const char *type, void *incoming = nullptr, bool force = false) {
  EiemProbeReadScope scope;
  if (scope.entered) EiemProbeRendererRead(stage, renderer, drawRenderer, type, incoming, force);
}

static void EiemProbeRememberRule(const EiemModRule &rule, void *mesh) {
  EiemProbeReadScope scope;
  if (!scope.entered) return;
  char name[192] = {};
  if (EiemProbeNativeAlive(mesh) == 1) TraceReadUnityObjectName(mesh, name, sizeof(name));
  if (!name[0]) return;
  for (const auto &watch : s_eiemProbeWatches)
    if (watch.mod == rule.modPath && watch.section == rule.section && watch.meshName == name) return;
  s_eiemProbeWatches.push_back({rule.modPath, rule.section, name});
  EiemProbeTrack(mesh, "source-Mesh", std::string(rule.modPath) + "/" + rule.section);
  EiemProbeLine("watch", "source", std::string(rule.modPath) + " section=" + rule.section +
                " object=" + EiemProbeObjectText(mesh) + " names-are-diagnostic-candidates-only");
}

static void EiemProbeResource(const char *stage, const char *mod, const char *section, void *object) {
  EiemProbeReadScope scope;
  if (!scope.entered) return;
  std::string label = std::string(mod) + "/" + section;
  EiemProbeTrack(object, "generated-resource", label);
  EiemProbeChanged(stage, "resource", label, label + " object=" + EiemProbeObjectText(object));
}

static void EiemProbeObserveModel(void *model, const char *stage) {
  EiemProbeReadScope scope;
  if (!scope.entered || s_eiemProbeWatches.empty() || EiemProbeNativeAlive(model) != 1 ||
      !g_gameObject_GetComponentsInChildren || !il2cpp_class_get_type || !il2cpp_type_get_object) return;
  bool reported = false;
  auto visit = [&](void *klass, const char *type) {
    if (!klass) return;
    void *nativeType = il2cpp_class_get_type(klass);
    void *typeObject = nativeType ? il2cpp_type_get_object(nativeType) : nullptr;
    if (!typeObject) return;
    bool includeInactive = true;
    void *params[] = {typeObject, &includeInactive};
    void *array = EiemProbeInvoke(g_gameObject_GetComponentsInChildren, model, params);
    size_t count = EiemManagedArrayLength(array);
    if (!array || count > 8192) return;
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    for (size_t i = 0; i < count; ++i) {
      if (EiemProbeNativeAlive(items[i]) != 1) continue;
      void *mesh = EiemReadSharedMesh(items[i], type);
      if (!EiemProbeMatches(items[i], mesh)) continue;
      if (!reported) { EiemProbeChanged(stage, "model", EiemProbeObjectText(model), EiemProbeObjectText(model)); reported = true; }
      void *draw = EiemModEquals(type, "SkinnedMeshRenderer") ? items[i] : nullptr;
      EiemProbeRendererRead(stage, items[i], draw, type, nullptr, true);
    }
  };
  visit(g_skinnedMeshRendererClass, "SkinnedMeshRenderer");
  visit(g_meshFilterClass, "MeshFilter");
}

static void EiemProbeCheckpoint(const char *stage, bool newWindow = false) {
  EiemProbeReadScope scope;
  if (!scope.entered) return;
  if (newWindow) { s_eiemProbeEvents = 0; s_eiemProbeLast.clear(); }
  for (auto it = s_eiemProbeObjects.begin(); it != s_eiemProbeObjects.end();) {
    if (il2cpp_gchandle_get_target && !il2cpp_gchandle_get_target(it->weak)) {
      EiemProbeLine(stage, "weak-expired", it->kind + " " + it->label);
      il2cpp_gchandle_free(it->weak);
      it = s_eiemProbeObjects.erase(it);
    } else ++it;
  }
  EiemProbeLine(stage, "checkpoint", "weak-observers=" + std::to_string(s_eiemProbeObjects.size()));
  // Copy handles only. No new roots, no retention of Unity native objects.
  const auto entries = s_eiemProbeObjects;
  for (const auto &entry : entries) {
    void *object = il2cpp_gchandle_get_target ? il2cpp_gchandle_get_target(entry.weak) : nullptr;
    EiemProbeLine(stage, entry.kind.c_str(), entry.label + " object=" + EiemProbeObjectText(object));
    if (entry.renderer && object)
      EiemProbeRendererRead(stage, object, entry.kind == "SkinnedMeshRenderer" ? object : nullptr,
                            entry.kind.c_str(), nullptr, true);
  }
}

static void EiemProbeTrackedRenderer(const char *stage, void *address) {
  EiemProbeReadScope scope;
  if (!scope.entered || !il2cpp_gchandle_get_target) return;
  for (const auto &entry : s_eiemProbeObjects) {
    if (!entry.renderer) continue;
    void *renderer = il2cpp_gchandle_get_target(entry.weak);
    if (renderer != address || !renderer) continue;
    // Only a live weak target is inspected; never dereference an old state-table address.
    const std::string type = entry.kind;
    EiemProbeRendererRead(stage, renderer, type == "SkinnedMeshRenderer" ? renderer : nullptr,
                          type.c_str(), nullptr, true);
    return;
  }
  EiemProbeLine(stage, "renderer", "weak target unavailable; state-table address not dereferenced");
}

static void EiemProbeInit(void **assemblies, size_t count) {
  void *object = FindClass("UnityEngine", "Object", assemblies, count);
  void *material = FindClass("UnityEngine", "Material", assemblies, count);
  s_eiemProbeAlive = FindMethod(object, "op_Implicit", 1);
  s_eiemProbeId = FindMethod(object, "GetInstanceID", 0);
  s_eiemProbeTextureNames = FindMethod(material, "GetTexturePropertyNames", 0);
  s_eiemProbeWeak = hGA ? (decltype(s_eiemProbeWeak))GetProcAddress(hGA, "il2cpp_gchandle_new_weakref") : nullptr;
  s_eiemResourceDiagnostic = EiemProbeResource;
  Log("[DEBUG-residue-v39] installed alive=%p id=%p textureNames=%p weak=%p; read-only, no strong roots",
      s_eiemProbeAlive, s_eiemProbeId, s_eiemProbeTextureNames, s_eiemProbeWeak);
}
