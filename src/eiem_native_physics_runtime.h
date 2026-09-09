#pragma once

// Per-model execution of validated Physics v1 author resources. Matching and
// immutable resource preparation stay in the Mod program; this adapter owns
// only Unity objects and native processes for one concrete model generation.
static constexpr const char *EiemPhysicsRuntimeTag = "[PHYSICS-RUNTIME-v70]";

struct EiemPhysicsRuntimeApi {
  void *clothClass = nullptr, *processClass = nullptr;
  void *gameObjectClass = nullptr, *transformClass = nullptr;
  void *gameObjectCtor = nullptr, *gameObjectTransform = nullptr;
  void *gameObjectActive = nullptr, *addComponent = nullptr;
  void *transformSetParent = nullptr, *setLocalPosition = nullptr;
  void *setLocalRotation = nullptr, *setLocalScale = nullptr;
  void *disableAutoBuild = nullptr, *setSerializeData = nullptr;
  void *getSerializeData = nullptr, *getSerializeData2 = nullptr;
  void *buildAndRun = nullptr, *processField = nullptr;
  void *processValid = nullptr, *processRunning = nullptr;
  void *processTeamId = nullptr, *processAnimatorField = nullptr;
  EiemPhysicsConfigApi config;

  bool Resolve(void **assemblies, size_t count, std::string &error) {
    *this = {};
    if (!config.Resolve(assemblies, count, error)) return false;
    clothClass = EiemPhysicsClass(assemblies, count, "BeyondDynamicBone.dll",
                                  "BeyondDynamicBone", "BeyondBoneCloth");
    processClass = EiemPhysicsClass(assemblies, count, "BeyondDynamicBone.dll",
                                    "BeyondDynamicBone", "ClothProcess");
    gameObjectClass = EiemPhysicsClass(assemblies, count,
        "UnityEngine.CoreModule.dll", "UnityEngine", "GameObject");
    transformClass = EiemPhysicsClass(assemblies, count,
        "UnityEngine.CoreModule.dll", "UnityEngine", "Transform");
    gameObjectCtor = EiemPhysicsMethod(gameObjectClass, ".ctor", "System.Void",
                                       false, "System.String");
    gameObjectTransform = EiemPhysicsMethod(
        gameObjectClass, "get_transform", "UnityEngine.Transform", false);
    gameObjectActive = EiemPhysicsMethod(gameObjectClass, "SetActive",
        "System.Void", false, "System.Boolean");
    addComponent = EiemPhysicsMethod(gameObjectClass, "AddComponent",
        "UnityEngine.Component", false, "System.Type");
    transformSetParent = EiemPhysicsMethod(transformClass, "SetParent",
        "System.Void", false, "UnityEngine.Transform", "System.Boolean");
    setLocalPosition = EiemPhysicsMethod(transformClass, "set_localPosition",
        "System.Void", false, "UnityEngine.Vector3");
    setLocalRotation = EiemPhysicsMethod(transformClass, "set_localRotation",
        "System.Void", false, "UnityEngine.Quaternion");
    setLocalScale = EiemPhysicsMethod(transformClass, "set_localScale",
        "System.Void", false, "UnityEngine.Vector3");
    disableAutoBuild = EiemPhysicsMethod(clothClass, "DisableAutoBuild",
                                         "System.Void", false);
    setSerializeData = EiemPhysicsMethod(clothClass, "set_SerializeData",
        "System.Void", false, "BeyondDynamicBone.ClothSerializeData");
    getSerializeData = EiemPhysicsMethod(clothClass, "get_SerializeData",
        "BeyondDynamicBone.ClothSerializeData", false);
    getSerializeData2 = EiemPhysicsMethod(clothClass, "GetSerializeData2",
        "BeyondDynamicBone.ClothSerializeData2", false);
    buildAndRun = EiemPhysicsMethod(clothClass, "BuildAndRun",
                                    "System.Boolean", false);
    processField = EiemPhysicsField(clothClass, "process",
                                    "BeyondDynamicBone.ClothProcess");
    processValid = EiemPhysicsMethod(processClass, "IsValid",
                                     "System.Boolean", false);
    processRunning = EiemPhysicsMethod(processClass, "IsRunning",
                                       "System.Boolean", false);
    processTeamId = EiemPhysicsMethod(processClass, "get_TeamId",
                                      "System.Int32", false);
    processAnimatorField = EiemPhysicsField(
        processClass, "interlockingAnimator", "UnityEngine.Animator");
    void *required[] = {
        clothClass, processClass, gameObjectClass, transformClass,
        gameObjectCtor, gameObjectTransform, gameObjectActive, addComponent,
        transformSetParent, setLocalPosition, setLocalRotation, setLocalScale,
        disableAutoBuild, setSerializeData, getSerializeData, getSerializeData2,
        buildAndRun, processField, processValid, processRunning, processTeamId,
        processAnimatorField};
    for (void *entry : required)
      if (!entry) {
        error = "Native Physics runtime metadata contract is incomplete or ambiguous";
        *this = {};
        return false;
      }
    error.clear();
    return true;
  }
};

struct EiemPhysicsRuntimeVector3 { float x, y, z; };
struct EiemPhysicsRuntimeQuaternion { float x, y, z, w; };

static bool EiemPhysicsRuntimeRendererUnderModel(void *renderer, void *model) {
  if (!renderer || !model || !g_component_get_transform ||
      !g_gameObject_get_transform || !g_transform_get_parent)
    return false;
  void *cursor = Invoke(g_component_get_transform, renderer);
  void *root = Invoke(g_gameObject_get_transform, model);
  for (int depth = 0; cursor && root && depth < 256; ++depth) {
    if (cursor == root) return true;
    cursor = Invoke(g_transform_get_parent, cursor);
  }
  return false;
}

static int EiemPhysicsRuntimeRendererDepth(void *renderer, void *component) {
  if (!renderer || !component || !g_component_get_transform ||
      !g_transform_get_parent)
    return -1;
  void *cursor = Invoke(g_component_get_transform, renderer);
  void *root = Invoke(g_component_get_transform, component);
  for (int depth = 0; cursor && root && depth < 256; ++depth) {
    if (cursor == root) return depth;
    cursor = Invoke(g_transform_get_parent, cursor);
  }
  return -1;
}

static void *EiemPhysicsRuntimeComponents(void *model, void *klass) {
  if (!model || !klass || !g_gameObject_GetComponentsInChildren ||
      !il2cpp_class_get_type || !il2cpp_type_get_object)
    return nullptr;
  void *type = il2cpp_class_get_type(klass);
  void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
  bool includeInactive = true;
  void *args[] = {typeObject, &includeInactive};
  return typeObject
             ? Invoke(g_gameObject_GetComponentsInChildren, model, args)
             : nullptr;
}

struct EiemPhysicsRuntimeComponent {
  void *component = nullptr;
  void *process = nullptr;
  EiemUnityRef componentRef, processRef, componentData2Ref;
  int32_t teamId = -1;
};

struct EiemPhysicsRuntimeInstance {
  uint64_t generation = 0;
  std::string key;
  std::shared_ptr<const EiemPhysicsAsset> asset;
  void *model = nullptr;
  void *renderer = nullptr;
  void *animator = nullptr;
  EiemUnityRef modelRef, rendererRef, animatorRef, hostRef;
  std::shared_ptr<EiemSkeletonInstance> skeleton;
  EiemPhysicsConfigDraft config;
  std::vector<EiemPhysicsRuntimeComponent> components;
  bool retiring = false;
  bool destroyNeeded = false;
  bool destroyRequested = false;
  bool ready = false;
};

static std::vector<std::shared_ptr<EiemPhysicsRuntimeInstance>>
    s_eiemPhysicsRuntimeInstances;
static EiemPhysicsRuntimeApi s_eiemPhysicsRuntimeApi;
static bool s_eiemPhysicsRuntimeApiReady = false;
static uint64_t s_eiemPhysicsRuntimeGeneration = 0;
static ULONGLONG s_eiemPhysicsRuntimeNextPoll = 0;
struct EiemPhysicsPendingRelease {
  void *model = nullptr;
  std::string stage;
};
static SRWLOCK s_eiemPhysicsPendingReleaseLock = SRWLOCK_INIT;
static std::vector<EiemPhysicsPendingRelease> s_eiemPhysicsPendingReleases;

static std::string EiemPhysicsRuntimeKey(const EiemPhysicsIntent &intent) {
  std::string key = std::string(intent.modPath) + "|" +
                    (intent.asset ? intent.asset->path.u8string() : "<missing>");
  std::transform(key.begin(), key.end(), key.begin(),
                 [](unsigned char c) { return (char)std::tolower(c); });
  return key;
}

static bool EiemPhysicsRuntimeResolve(std::string &error) {
  if (s_eiemPhysicsRuntimeApiReady) return true;
  void *domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
  size_t count = 0;
  void **assemblies = domain && il2cpp_domain_get_assemblies
                          ? il2cpp_domain_get_assemblies(domain, &count)
                          : nullptr;
  if (!assemblies || !count ||
      !s_eiemPhysicsRuntimeApi.Resolve(assemblies, count, error))
    return false;
  s_eiemPhysicsRuntimeApiReady = true;
  return true;
}

static bool EiemPhysicsRuntimeSelectBinding(
    const EiemPhysicsIntent &intent, void *model, void **rendererOut,
    void **animatorOut, std::string &error) {
  *rendererOut = nullptr;
  *animatorOut = nullptr;
  if (!model || intent.matchedRenderers.empty()) {
    error = "Physics Render matched no live renderer";
    return false;
  }
  void *domain = il2cpp_domain_get ? il2cpp_domain_get() : nullptr;
  size_t count = 0;
  void **assemblies = domain && il2cpp_domain_get_assemblies
                          ? il2cpp_domain_get_assemblies(domain, &count)
                          : nullptr;
  void *animatorClass = assemblies
      ? EiemPhysicsClass(assemblies, count, EiemPhysicsAnimatorImage,
                         "UnityEngine", "Animator")
      : nullptr;
  void *animators = EiemPhysicsRuntimeComponents(model, animatorClass);
  const size_t animatorCount = EiemManagedArrayLength(animators);
  if (!animators || !animatorCount || animatorCount > 64) {
    error = "Physics model has no valid Animator set";
    return false;
  }
  void **items = (void **)((char *)animators + IL2CPP_ARRAY_DATA);
  for (void *candidate : intent.matchedRenderers) {
    if (!candidate || EiemNativeObjectStatus(candidate) != 1 ||
        !EiemPhysicsRuntimeRendererUnderModel(candidate, model))
      continue;
    void *nearest = nullptr;
    int nearestDepth = -1;
    size_t ancestors = 0;
    for (size_t index = 0; index < animatorCount; ++index) {
      const int depth = EiemPhysicsRuntimeRendererDepth(candidate, items[index]);
      if (depth < 0) continue;
      ++ancestors;
      if (nearestDepth < 0 || depth < nearestDepth) {
        nearest = items[index];
        nearestDepth = depth;
      }
    }
    if (ancestors != 1 || !nearest) continue;
    if (*animatorOut && *animatorOut != nearest) {
      error = "Physics Render matches span different Animators";
      return false;
    }
    if (!*rendererOut) *rendererOut = candidate;
    *animatorOut = nearest;
  }
  if (!*rendererOut || !*animatorOut) {
    error = "Physics renderer does not have one unique Animator ancestor";
    return false;
  }
  return true;
}

static bool EiemPhysicsRuntimeNewHost(EiemPhysicsRuntimeInstance &instance,
                                      void **hostOut, std::string &error) {
  auto &api = s_eiemPhysicsRuntimeApi;
  void *modelTransform = nullptr;
  if (!InvokeChecked(api.gameObjectTransform, instance.model, nullptr,
                     &modelTransform) ||
      !modelTransform || !il2cpp_object_new || !il2cpp_string_new) {
    error = "Cannot resolve Physics model Transform";
    return false;
  }
  void *host = il2cpp_object_new(api.gameObjectClass);
  instance.hostRef = EiemUnityRef::Capture(host, false);
  const std::string name = "EIEM_Physics_" +
                           std::to_string(instance.generation);
  void *nameObject = il2cpp_string_new(name.c_str());
  void *result = nullptr;
  void *ctorArgs[] = {nameObject};
  if (!host || !instance.hostRef || !nameObject ||
      !InvokeChecked(api.gameObjectCtor, host, ctorArgs, &result)) {
    error = "Cannot construct Physics host";
    return false;
  }
  void *transform = nullptr;
  bool keepWorld = false;
  EiemPhysicsRuntimeVector3 position{0, 0, 0}, scale{1, 1, 1};
  EiemPhysicsRuntimeQuaternion rotation{0, 0, 0, 1};
  void *parentArgs[] = {modelTransform, &keepWorld};
  void *positionArgs[] = {&position};
  void *rotationArgs[] = {&rotation};
  void *scaleArgs[] = {&scale};
  bool active = false;
  void *activeArgs[] = {&active};
  if (!InvokeChecked(api.gameObjectTransform, host, nullptr, &transform) ||
      !transform ||
      !InvokeChecked(api.transformSetParent, transform, parentArgs, &result) ||
      !InvokeChecked(api.setLocalPosition, transform, positionArgs, &result) ||
      !InvokeChecked(api.setLocalRotation, transform, rotationArgs, &result) ||
      !InvokeChecked(api.setLocalScale, transform, scaleArgs, &result) ||
      !InvokeChecked(api.gameObjectActive, host, activeArgs, &result)) {
    error = "Cannot attach inactive Physics host";
    return false;
  }
  *hostOut = host;
  return true;
}

static void EiemPhysicsRuntimeBeginRetire(
    const std::shared_ptr<EiemPhysicsRuntimeInstance> &instance,
    bool destroyHost, const char *stage) {
  if (!instance) return;
  instance->destroyNeeded = instance->destroyNeeded || destroyHost;
  if (instance->retiring) return;
  instance->retiring = true;
  instance->ready = false;
  if (instance->destroyNeeded && EiemOnUnityThread() &&
      instance->hostRef.Status() == 1 && g_object_destroy) {
    void *host = instance->hostRef.Target();
    void *args[] = {host};
    instance->destroyRequested = EiemSkeletonCall(g_object_destroy, nullptr, args);
  }
  Log("%s retire generation=%llu model=%p host=%p components=%zu destroy=%d stage=%s",
      EiemPhysicsRuntimeTag,
      (unsigned long long)instance->generation, instance->model,
      instance->hostRef.Target(), instance->components.size(),
      instance->destroyRequested ? 1 : 0, stage ? stage : "unknown");
}

static bool EiemPhysicsRuntimeDead(
    const EiemPhysicsRuntimeInstance &instance) {
  if (instance.hostRef.Status() != 0) return false;
  for (const auto &component : instance.components)
    if (component.componentRef.Status() != 0) return false;
  return true;
}

static void EiemPhysicsRuntimeCollect() {
  if (!EiemOnUnityThread()) return;
  for (size_t index = 0; index < s_eiemPhysicsRuntimeInstances.size();) {
    auto &instance = s_eiemPhysicsRuntimeInstances[index];
    if (!instance->retiring && instance->modelRef.Status() == 0)
      EiemPhysicsRuntimeBeginRetire(instance, false, "model native object expired");
    if (instance->retiring && instance->destroyNeeded &&
        !instance->destroyRequested && instance->hostRef.Status() == 1 &&
        g_object_destroy) {
      void *host = instance->hostRef.Target();
      void *args[] = {host};
      instance->destroyRequested =
          EiemSkeletonCall(g_object_destroy, nullptr, args);
      Log("%s destroy-request generation=%llu model=%p host=%p accepted=%d",
          EiemPhysicsRuntimeTag,
          (unsigned long long)instance->generation, instance->model, host,
          instance->destroyRequested ? 1 : 0);
    }
    if (!instance->retiring || !EiemPhysicsRuntimeDead(*instance)) {
      ++index;
      continue;
    }
    Log("%s retired generation=%llu model=%p key=%s",
        EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
        instance->model, instance->key.c_str());
    s_eiemPhysicsRuntimeInstances.erase(
        s_eiemPhysicsRuntimeInstances.begin() + index);
  }
  EiemCollectSkeletonInstances();
}

static bool EiemAcquirePhysicsSkeleton(
    const EiemPhysicsIntent &intent, void *renderer,
    std::shared_ptr<EiemSkeletonInstance> &out, char *message,
    size_t messageSize) {
  out.reset();
  if (!intent.asset) {
    if (message)
      strncpy_s(message, messageSize, "Physics asset is unavailable",
                _TRUNCATE);
    return false;
  }
  const std::string path = intent.asset->skeletonPath.u8string();
  return EiemAcquireSkeletonDocument(
      EiemSkeletonInstanceKey(intent.modPath, path),
      EiemMeshResourceFileStamp(path.c_str()), intent.asset->skeleton, renderer,
      out, message, messageSize);
}

static bool EiemPhysicsRuntimeBuild(const EiemPhysicsIntent &intent,
                                    void *model, const char *stage) {
  std::string error;
  if (!EiemOnUnityThread() || !intent.asset ||
      !EiemPhysicsRuntimeResolve(error)) {
    Log("%s rejected model=%p resource=%s error=%s", EiemPhysicsRuntimeTag,
        model, intent.resourceSection,
        error.empty() ? "runtime APIs unavailable" : error.c_str());
    return false;
  }
  void *renderer = nullptr, *animator = nullptr;
  if (!EiemPhysicsRuntimeSelectBinding(intent, model, &renderer, &animator,
                                       error)) {
    Log("%s pending model=%p resource=%s error=%s stage=%s",
        EiemPhysicsRuntimeTag, model, intent.resourceSection, error.c_str(),
        stage ? stage : "unknown");
    return false;
  }
  auto instance = std::make_shared<EiemPhysicsRuntimeInstance>();
  instance->generation = ++s_eiemPhysicsRuntimeGeneration;
  instance->key = EiemPhysicsRuntimeKey(intent);
  instance->asset = intent.asset;
  instance->model = model;
  instance->renderer = renderer;
  instance->animator = animator;
  instance->modelRef = EiemUnityRef::Capture(model);
  instance->rendererRef = EiemUnityRef::Capture(renderer);
  instance->animatorRef = EiemUnityRef::Capture(animator);
  char skeletonError[256] = {};
  if (!instance->modelRef || !instance->rendererRef || !instance->animatorRef ||
      !EiemAcquirePhysicsSkeleton(intent, renderer, instance->skeleton,
                                  skeletonError, sizeof(skeletonError))) {
    Log("%s rejected generation=%llu model=%p resource=%s error=%s",
        EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
        model, intent.resourceSection,
        skeletonError[0] ? skeletonError : "Cannot bind Physics Skeleton");
    return false;
  }
  std::unordered_map<std::string, void *> bindings;
  if (instance->skeleton->nodes.size() != intent.asset->skeleton.nodes.size()) {
    Log("%s rejected generation=%llu model=%p resource=%s error=Skeleton node count changed",
        EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
        model, intent.resourceSection);
    return false;
  }
  for (size_t index = 0; index < intent.asset->skeleton.nodes.size(); ++index) {
    void *node = instance->skeleton->nodes[index].Target();
    if (!node || instance->skeleton->nodes[index].Status() != 1) {
      Log("%s rejected generation=%llu model=%p resource=%s error=Skeleton node expired",
          EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
          model, intent.resourceSection);
      return false;
    }
    bindings.emplace(intent.asset->skeleton.nodes[index].path, node);
  }
  if (!instance->config.Prepare(s_eiemPhysicsRuntimeApi.config,
                                intent.asset->physics, bindings, error)) {
    Log("%s rejected generation=%llu model=%p resource=%s error=%s",
        EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
        model, intent.resourceSection, error.c_str());
    return false;
  }
  size_t paletteCount = 0, selectedCount = 0, paletteHits = 0;
  size_t boundaryIgnores = 0;
  void *palette = nullptr;
  if (g_smr_get_bones &&
      EiemSkeletonCall(g_smr_get_bones, renderer, nullptr, &palette)) {
    paletteCount = EiemManagedArrayLength(palette);
    if (palette && paletteCount <= 16384) {
      void **items = (void **)((char *)palette + IL2CPP_ARRAY_DATA);
      for (const auto &group : instance->config.Groups())
        for (const auto &node : group.transforms) {
          ++selectedCount;
          void *selected = node.Target();
          for (size_t index = 0; index < paletteCount; ++index)
            if (items[index] == selected) {
              ++paletteHits;
              break;
            }
        }
    }
  }
  for (const auto &group : instance->config.Groups())
    boundaryIgnores += group.boundaryIgnores.size();
  Log("%s binding generation=%llu model=%p renderer=%p palette=%zu selected=%zu paletteHits=%zu boundaryIgnores=%zu",
      EiemPhysicsRuntimeTag, (unsigned long long)instance->generation, model,
      renderer, paletteCount, selectedCount, paletteHits, boundaryIgnores);
  s_eiemPhysicsRuntimeInstances.push_back(instance);
  void *host = nullptr;
  if (!EiemPhysicsRuntimeNewHost(*instance, &host, error)) {
    EiemPhysicsRuntimeBeginRetire(instance, true, "host construction failed");
    return false;
  }
  void *clothType = il2cpp_type_get_object(
      il2cpp_class_get_type(s_eiemPhysicsRuntimeApi.clothClass));
  auto typeRef = EiemUnityRef::Capture(clothType, false);
  if (!typeRef) {
    EiemPhysicsRuntimeBeginRetire(instance, true, "component type unavailable");
    return false;
  }
  for (const auto &prepared : instance->config.Groups()) {
    void *component = nullptr, *result = nullptr;
    void *addArgs[] = {clothType};
    if (!InvokeChecked(s_eiemPhysicsRuntimeApi.addComponent, host, addArgs,
                       &component) ||
        !component ||
        il2cpp_object_get_class(component) !=
            s_eiemPhysicsRuntimeApi.clothClass) {
      EiemPhysicsRuntimeBeginRetire(instance, true, "AddComponent failed");
      return false;
    }
    EiemPhysicsRuntimeComponent runtime;
    runtime.component = component;
    runtime.componentRef = EiemUnityRef::Capture(component, false);
    void *data = prepared.data.Target();
    void *dataArgs[] = {data};
    void *actualData = nullptr, *actualData2 = nullptr;
    if (!runtime.componentRef ||
        !InvokeChecked(s_eiemPhysicsRuntimeApi.disableAutoBuild, component,
                       nullptr, &result) ||
        !InvokeChecked(s_eiemPhysicsRuntimeApi.setSerializeData, component,
                       dataArgs, &result) ||
        !InvokeChecked(s_eiemPhysicsRuntimeApi.getSerializeData, component,
                       nullptr, &actualData) ||
        actualData != data ||
        !InvokeChecked(s_eiemPhysicsRuntimeApi.getSerializeData2, component,
                       nullptr, &actualData2) ||
        !actualData2 ||
        il2cpp_object_get_class(actualData2) !=
            s_eiemPhysicsRuntimeApi.config.data2 ||
        !(runtime.componentData2Ref =
              EiemUnityRef::Capture(actualData2, false))) {
      EiemPhysicsRuntimeBeginRetire(instance, true,
                                    "component configuration failed");
      return false;
    }
    EiemPhysicsReadField(component, s_eiemPhysicsRuntimeApi.processField,
                         runtime.process);
    if (runtime.process)
      runtime.processRef = EiemUnityRef::Capture(runtime.process, false);
    instance->components.push_back(std::move(runtime));
  }
  bool active = true;
  void *activeArgs[] = {&active};
  void *result = nullptr;
  if (!InvokeChecked(s_eiemPhysicsRuntimeApi.gameObjectActive, host, activeArgs,
                     &result)) {
    EiemPhysicsRuntimeBeginRetire(instance, true, "host activation failed");
    return false;
  }
  for (auto &runtime : instance->components) {
    void *boxed = nullptr;
    bool started = false;
    if (!InvokeChecked(s_eiemPhysicsRuntimeApi.buildAndRun, runtime.component,
                       nullptr, &boxed) ||
        !EiemPhysicsUnbox(boxed, started) || !started) {
      EiemPhysicsRuntimeBeginRetire(instance, true, "BuildAndRun failed");
      return false;
    }
    EiemPhysicsReadField(runtime.component,
                         s_eiemPhysicsRuntimeApi.processField,
                         runtime.process);
    if (runtime.process)
      runtime.processRef = EiemUnityRef::Capture(runtime.process, false);
  }
  Log("%s build-started generation=%llu model=%p renderer=%p animator=%p groups=%zu resource=%s matches=%u stage=%s",
      EiemPhysicsRuntimeTag, (unsigned long long)instance->generation, model,
      renderer, animator, instance->components.size(), intent.resourceSection,
      intent.rendererMatches, stage ? stage : "unknown");
  return true;
}

static void EiemReconcileModelPhysics(
    void *model, const std::vector<EiemPhysicsIntent> &intents, bool active,
    const char *stage) {
  (void)active;
  if (!model || !EiemOnUnityThread()) return;
  EiemPhysicsRuntimeCollect();
  for (const auto &instance : s_eiemPhysicsRuntimeInstances) {
    if (instance->model != model || instance->retiring) continue;
    bool keep = false;
    for (const auto &intent : intents)
      if (intent.asset == instance->asset &&
          EiemPhysicsRuntimeKey(intent) == instance->key) {
        keep = true;
        break;
      }
    if (!keep) EiemPhysicsRuntimeBeginRetire(instance, true, stage);
  }
  for (const auto &instance : s_eiemPhysicsRuntimeInstances)
    if (instance->model == model && instance->retiring) return;
  for (const auto &intent : intents) {
    if (!intent.asset) continue;
    const std::string key = EiemPhysicsRuntimeKey(intent);
    bool exists = false;
    for (const auto &instance : s_eiemPhysicsRuntimeInstances)
      if (!instance->retiring && instance->model == model &&
          instance->asset == intent.asset && instance->key == key) {
        exists = true;
        break;
      }
    if (!exists) EiemPhysicsRuntimeBuild(intent, model, stage);
  }
}

static void EiemPhysicsRuntimeReleaseOnUnityThread(void *model,
                                                   const char *stage) {
  if (!model) return;
  for (const auto &instance : s_eiemPhysicsRuntimeInstances)
    if (instance->model == model && !instance->retiring)
      EiemPhysicsRuntimeBeginRetire(instance, true, stage);
}

static void EiemReleaseModelPhysics(void *model, const char *stage) {
  if (!model) return;
  if (EiemOnUnityThread()) {
    EiemPhysicsRuntimeReleaseOnUnityThread(model, stage);
    return;
  }
  AcquireSRWLockExclusive(&s_eiemPhysicsPendingReleaseLock);
  s_eiemPhysicsPendingReleases.push_back(
      {model, stage ? stage : "off-thread model release"});
  ReleaseSRWLockExclusive(&s_eiemPhysicsPendingReleaseLock);
}

static void EiemPhysicsRuntimeDrainReleases() {
  std::vector<EiemPhysicsPendingRelease> pending;
  AcquireSRWLockExclusive(&s_eiemPhysicsPendingReleaseLock);
  pending.swap(s_eiemPhysicsPendingReleases);
  ReleaseSRWLockExclusive(&s_eiemPhysicsPendingReleaseLock);
  for (const auto &release : pending)
    EiemPhysicsRuntimeReleaseOnUnityThread(release.model, release.stage.c_str());
}

static void EiemPhysicsRuntimePollReady() {
  for (const auto &instance : s_eiemPhysicsRuntimeInstances) {
    if (instance->retiring || instance->ready) continue;
    bool ready = !instance->components.empty();
    for (auto &runtime : instance->components) {
      if (runtime.componentRef.Status() != 1) {
        ready = false;
        continue;
      }
      void *process = nullptr;
      if (!EiemPhysicsReadField(runtime.component,
                                s_eiemPhysicsRuntimeApi.processField,
                                process) ||
          !process) {
        ready = false;
        continue;
      }
      runtime.process = process;
      if (!runtime.processRef)
        runtime.processRef = EiemUnityRef::Capture(process, false);
      bool valid = false, running = false;
      int32_t team = -1;
      void *animator = nullptr;
      if (!EiemPhysicsReadValue(process,
                                s_eiemPhysicsRuntimeApi.processValid, valid) ||
          !EiemPhysicsReadValue(process,
                                s_eiemPhysicsRuntimeApi.processRunning,
                                running) ||
          !EiemPhysicsReadValue(process,
                                s_eiemPhysicsRuntimeApi.processTeamId, team) ||
          !EiemPhysicsReadField(process,
                                s_eiemPhysicsRuntimeApi.processAnimatorField,
                                animator) ||
          !valid || !running || team < 0 || animator != instance->animator) {
        ready = false;
        continue;
      }
      runtime.teamId = team;
    }
    if (!ready) continue;
    instance->ready = true;
    std::string teams;
    for (const auto &component : instance->components) {
      if (!teams.empty()) teams += ',';
      teams += std::to_string(component.teamId);
    }
    Log("%s ready generation=%llu model=%p animator=%p teams=%s key=%s",
        EiemPhysicsRuntimeTag, (unsigned long long)instance->generation,
        instance->model, instance->animator, teams.c_str(),
        instance->key.c_str());
  }
}

static void EiemPhysicsRuntimePeriodic(const char *stage) {
  if (!EiemOnUnityThread()) return;
  const ULONGLONG now = GetTickCount64();
  if (now < s_eiemPhysicsRuntimeNextPoll) return;
  s_eiemPhysicsRuntimeNextPoll = now + 250;
  EiemPhysicsRuntimeDrainReleases();
  EiemPhysicsRuntimeCollect();
  EiemPhysicsRuntimePollReady();
  std::vector<EiemModelInstanceState> models;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  models = s_eiemModelInstances;
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &model : models)
    if (model.model && model.modelRef.Status() == 1)
      EiemReconcileModelPhysics(model.model, model.physicsIntents,
                                EiemModelHasActiveOwner(model), stage);
}
