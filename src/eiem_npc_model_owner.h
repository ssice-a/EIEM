#pragma once

// Exact NPC model-owner adapter. StartNPC registers the completed model with
// the shared Render/Physics executor; both observed release boundaries remove
// that owner before the game clears its references. The remaining records are
// automatic diagnostics and have no Dump, ImGui, or hotkey entry point.
static constexpr const char *EiemPhysicsOwnerProbeTag =
    "[NPC-MODEL-OWNER-v70]";

struct EiemPhysicsOwnerNpcRecord {
  void *model = nullptr;
  EiemUnityRef modelRef;
  void *animator = nullptr;
  EiemUnityRef animatorRef;
  void *avatar = nullptr;
  void *component = nullptr;
  int boneCloths = -1;
  bool sawBuild = false;
  bool sawStart = false;
  bool target = false;
};

struct EiemPhysicsOwnerRendererRecord {
  void *renderer = nullptr;
  EiemUnityRef rendererRef;
};

struct EiemPhysicsOwnerModelRecord {
  void *model = nullptr;
  EiemUnityRef modelRef;
  void *owner = nullptr;
  char ownerKind[32] = {};
};

static EiemPhysicsOwnerNpcRecord s_eiemPhysicsOwnerNpcs[2048] = {};
static size_t s_eiemPhysicsOwnerNpcCount = 0;
static EiemPhysicsOwnerRendererRecord s_eiemPhysicsOwnerRenderers[256] = {};
static size_t s_eiemPhysicsOwnerRendererCount = 0;
static EiemPhysicsOwnerModelRecord s_eiemPhysicsOwnerModels[256] = {};
static size_t s_eiemPhysicsOwnerModelCount = 0;

static void *s_eiemPhysicsOwnerAnimatorClass = nullptr;
static void *s_eiemPhysicsOwnerBoneClothClass = nullptr;
static void *s_eiemPhysicsOwnerGetComponentsInChildren = nullptr;
static void *s_eiemPhysicsOwnerGameObjectGetTransform = nullptr;
static void *s_eiemPhysicsOwnerComponentGetTransform = nullptr;
static void *s_eiemPhysicsOwnerTransformGetParent = nullptr;
static void *s_eiemPhysicsOwnerNpcGetModelGo = nullptr;
static int s_eiemPhysicsOwnerNpcAvatarGoRefOffset = -1;
static int s_eiemPhysicsOwnerNpcComponentAvatarOffset = -1;
static int s_eiemPhysicsOwnerGoRefAnimatorOffset = -1;
static int s_eiemPhysicsOwnerGoRefGameObjectOffset = -1;
static int s_eiemPhysicsOwnerGoRefBoneClothsOffset = -1;

using EiemPhysicsOwnerVoid0 = void(__fastcall *)(void *, void *);
using EiemPhysicsOwnerVoid1 = void(__fastcall *)(void *, void *, void *);
using EiemPhysicsOwnerBuildCloth = void(__fastcall *)(
    void *, void *, void *, void *, void *);
static void *s_eiemPhysicsOwnerOrigStartNpc = nullptr;
static void *s_eiemPhysicsOwnerOrigBuildCloth = nullptr;
static void *s_eiemPhysicsOwnerOrigReleaseAvatar = nullptr;
static void *s_eiemPhysicsOwnerOrigNpcOnRelease = nullptr;
static volatile LONG s_eiemPhysicsOwnerBaselineLogs = 0;

static void *EiemPhysicsOwnerReadPointer(void *base, int offset) {
  if (!base || offset < 0) return nullptr;
  __try {
    return *(void **)((char *)base + offset);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return nullptr;
  }
}

// Metadata offsets for value-type members include the boxed object header.
// Byref and embedded values point at the payload, so subtract it once.
static void *EiemPhysicsOwnerReadValueMember(void *payload, int boxedOffset) {
  constexpr int objectHeader = (int)(sizeof(void *) * 2);
  return boxedOffset >= objectHeader
             ? EiemPhysicsOwnerReadPointer(payload, boxedOffset - objectHeader)
             : nullptr;
}

static int EiemPhysicsOwnerReadListCount(void *list) {
  if (!list) return 0;
  __try {
    const int count = *(int *)((char *)list + IL2CPP_LIST_SIZE);
    return count >= 0 && count <= 4096 ? count : -1;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    return -1;
  }
}

static bool EiemPhysicsOwnerRendererUnderModel(void *renderer, void *model) {
  if (!renderer || !model || !s_eiemPhysicsOwnerComponentGetTransform ||
      !s_eiemPhysicsOwnerGameObjectGetTransform ||
      !s_eiemPhysicsOwnerTransformGetParent)
    return false;
  void *cursor = Invoke(s_eiemPhysicsOwnerComponentGetTransform, renderer);
  void *root = Invoke(s_eiemPhysicsOwnerGameObjectGetTransform, model);
  for (int depth = 0; cursor && root && depth < 256; ++depth) {
    if (cursor == root) return true;
    cursor = Invoke(s_eiemPhysicsOwnerTransformGetParent, cursor);
  }
  return false;
}

static int EiemPhysicsOwnerRendererDepthUnderComponent(void *renderer,
                                                        void *component) {
  if (!renderer || !component || !s_eiemPhysicsOwnerComponentGetTransform ||
      !s_eiemPhysicsOwnerTransformGetParent)
    return -1;
  void *cursor = Invoke(s_eiemPhysicsOwnerComponentGetTransform, renderer);
  void *root = Invoke(s_eiemPhysicsOwnerComponentGetTransform, component);
  for (int depth = 0; cursor && root && depth < 256; ++depth) {
    if (cursor == root) return depth;
    cursor = Invoke(s_eiemPhysicsOwnerTransformGetParent, cursor);
  }
  return -1;
}

static void *EiemPhysicsOwnerComponents(void *model, void *klass) {
  if (!model || !klass || !s_eiemPhysicsOwnerGetComponentsInChildren ||
      !il2cpp_class_get_type || !il2cpp_type_get_object)
    return nullptr;
  void *type = il2cpp_class_get_type(klass);
  void *typeObject = type ? il2cpp_type_get_object(type) : nullptr;
  if (!typeObject) return nullptr;
  bool includeInactive = true;
  void *params[] = {typeObject, &includeInactive};
  return Invoke(s_eiemPhysicsOwnerGetComponentsInChildren, model, params);
}

static void EiemPhysicsOwnerLogRendererModel(
    void *renderer, const char *ownerKind, void *owner, void *model,
    const char *stage) {
  if (!EiemPhysicsOwnerRendererUnderModel(renderer, model)) return;

  void *animators = EiemPhysicsOwnerComponents(
      model, s_eiemPhysicsOwnerAnimatorClass);
  void *cloths = EiemPhysicsOwnerComponents(
      model, s_eiemPhysicsOwnerBoneClothClass);
  const size_t animatorCount = EiemManagedArrayLength(animators);
  const size_t clothCount = EiemManagedArrayLength(cloths);
  size_t ancestorCount = 0;
  void *nearestAnimator = nullptr;
  int nearestDepth = -1;
  if (animators && animatorCount <= 64) {
    void **items = (void **)((char *)animators + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < animatorCount; ++index) {
      const int depth = EiemPhysicsOwnerRendererDepthUnderComponent(
          renderer, items[index]);
      if (depth < 0) continue;
      ++ancestorCount;
      if (nearestDepth < 0 || depth < nearestDepth) {
        nearestAnimator = items[index];
        nearestDepth = depth;
      }
    }
  }
  Log("%s model-render-match stage=%s renderer=%p ownerKind=%s owner=%p model=%p animators=%zu animatorAncestors=%zu nearestAnimator=%p nearestDepth=%d existingCloths=%zu",
      EiemPhysicsOwnerProbeTag, stage ? stage : "unknown", renderer,
      ownerKind ? ownerKind : "unknown", owner, model, animatorCount,
      ancestorCount, nearestAnimator, nearestDepth, clothCount);
}

static EiemPhysicsOwnerNpcRecord *EiemPhysicsOwnerFindNpc(void *model) {
  for (size_t index = 0; index < s_eiemPhysicsOwnerNpcCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerNpcs[index];
    if (entry.model == model && entry.modelRef.Target() == model) return &entry;
  }
  return nullptr;
}

static bool EiemPhysicsOwnerCorrelate(EiemPhysicsOwnerNpcRecord &entry) {
  bool found = false;
  for (size_t index = 0; index < s_eiemPhysicsOwnerRendererCount; ++index) {
    auto &candidate = s_eiemPhysicsOwnerRenderers[index];
    void *renderer = candidate.rendererRef.Target();
    if (renderer != candidate.renderer ||
        !EiemPhysicsOwnerRendererUnderModel(renderer, entry.model))
      continue;
    found = true;
    Log("%s npc-render-match renderer=%p model=%p animator=%p avatar=%p component=%p boneCloths=%d",
        EiemPhysicsOwnerProbeTag, renderer, entry.model, entry.animator,
        entry.avatar, entry.component, entry.boneCloths);
  }
  entry.target = entry.target || found;
  return found;
}

static void EiemPhysicsOwnerProbeObserveRenderer(void *renderer,
                                                 const char *stage) {
  if (!renderer || !EiemOnUnityThread()) return;
  bool known = false;
  for (size_t index = 0; index < s_eiemPhysicsOwnerRendererCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerRenderers[index];
    if (entry.renderer == renderer && entry.rendererRef.Target() == renderer) {
      known = true;
      break;
    }
  }
  if (!known && s_eiemPhysicsOwnerRendererCount <
                    _countof(s_eiemPhysicsOwnerRenderers)) {
    auto &entry =
        s_eiemPhysicsOwnerRenderers[s_eiemPhysicsOwnerRendererCount++];
    entry.renderer = renderer;
    entry.rendererRef = EiemUnityRef::Capture(renderer);
  }

  bool correlated = false;
  for (size_t index = 0; index < s_eiemPhysicsOwnerNpcCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerNpcs[index];
    if (entry.modelRef.Target() != entry.model ||
        !EiemPhysicsOwnerRendererUnderModel(renderer, entry.model))
      continue;
    entry.target = true;
    correlated = true;
    Log("%s renderer-hit stage=%s renderer=%p npcModel=%p animator=%p avatar=%p component=%p boneCloths=%d",
        EiemPhysicsOwnerProbeTag, stage ? stage : "unknown", renderer,
        entry.model, entry.animator, entry.avatar, entry.component,
        entry.boneCloths);
  }
  for (size_t index = 0; index < s_eiemPhysicsOwnerModelCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerModels[index];
    if (entry.modelRef.Target() != entry.model) continue;
    EiemPhysicsOwnerLogRendererModel(
        renderer, entry.ownerKind, entry.owner, entry.model, stage);
  }
  if (!correlated)
    Log("%s renderer-hit stage=%s renderer=%p npcOwner=pending-or-non-npc",
        EiemPhysicsOwnerProbeTag, stage ? stage : "unknown", renderer);
}

static void EiemPhysicsOwnerProbeObserveModel(const char *ownerKind,
                                              void *owner, void *model,
                                              const char *stage) {
  if (!model || !EiemOnUnityThread()) return;
  void *animators = EiemPhysicsOwnerComponents(
      model, s_eiemPhysicsOwnerAnimatorClass);
  void *cloths = EiemPhysicsOwnerComponents(
      model, s_eiemPhysicsOwnerBoneClothClass);
  const size_t animatorCount = EiemManagedArrayLength(animators);
  const size_t clothCount = EiemManagedArrayLength(cloths);
  bool known = false;
  for (size_t index = 0; index < s_eiemPhysicsOwnerModelCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerModels[index];
    if (entry.model == model && entry.owner == owner &&
        entry.modelRef.Target() == model &&
        strcmp(entry.ownerKind, ownerKind ? ownerKind : "unknown") == 0) {
      known = true;
      break;
    }
  }
  if (!known && s_eiemPhysicsOwnerModelCount <
                    _countof(s_eiemPhysicsOwnerModels)) {
    auto &entry = s_eiemPhysicsOwnerModels[s_eiemPhysicsOwnerModelCount++];
    entry.model = model;
    entry.modelRef = EiemUnityRef::Capture(model);
    entry.owner = owner;
    strncpy_s(entry.ownerKind, sizeof(entry.ownerKind),
              ownerKind ? ownerKind : "unknown", _TRUNCATE);
  }
  Log("%s model-hit stage=%s ownerKind=%s owner=%p model=%p animators=%zu existingCloths=%zu",
      EiemPhysicsOwnerProbeTag, stage ? stage : "unknown",
      ownerKind ? ownerKind : "unknown", owner, model, animatorCount,
      clothCount);
  if (animators && animatorCount <= 64) {
    void **items = (void **)((char *)animators + IL2CPP_ARRAY_DATA);
    for (size_t index = 0; index < animatorCount; ++index)
      Log("%s model-animator ownerKind=%s model=%p index=%zu animator=%p",
          EiemPhysicsOwnerProbeTag, ownerKind ? ownerKind : "unknown", model,
          index, items[index]);
  }
  for (size_t index = 0; index < s_eiemPhysicsOwnerRendererCount; ++index) {
    auto &entry = s_eiemPhysicsOwnerRenderers[index];
    void *renderer = entry.rendererRef.Target();
    if (renderer != entry.renderer) continue;
    EiemPhysicsOwnerLogRendererModel(
        renderer, ownerKind, owner, model, stage);
  }
}

static void EiemPhysicsOwnerProbeObserveOwnerActive(
    const char *ownerKind, void *owner, bool active, const char *stage) {
  for (size_t index = 0; index < s_eiemPhysicsOwnerModelCount; ++index) {
    const auto &entry = s_eiemPhysicsOwnerModels[index];
    if (entry.owner != owner ||
        strcmp(entry.ownerKind, ownerKind ? ownerKind : "unknown") != 0)
      continue;
    Log("%s owner-active stage=%s ownerKind=%s owner=%p model=%p active=%d",
        EiemPhysicsOwnerProbeTag, stage ? stage : "unknown",
        ownerKind ? ownerKind : "unknown", owner, entry.model,
        active ? 1 : 0);
  }
}

static void EiemPhysicsOwnerProbeObserveRelease(
    const char *ownerKind, void *owner, void *model, const char *stage) {
  for (size_t index = 0; index < s_eiemPhysicsOwnerModelCount;) {
    auto &entry = s_eiemPhysicsOwnerModels[index];
    const bool sameOwner = owner && entry.owner == owner;
    const bool sameModelWithoutOwner = !owner && model && entry.model == model;
    if (sameOwner || sameModelWithoutOwner) {
      Log("%s owner-release stage=%s ownerKind=%s owner=%p model=%p",
          EiemPhysicsOwnerProbeTag, stage ? stage : "unknown",
          ownerKind ? ownerKind : entry.ownerKind,
          owner ? owner : entry.owner, entry.model);
      entry = s_eiemPhysicsOwnerModels[--s_eiemPhysicsOwnerModelCount];
      continue;
    }
    ++index;
  }
}

static void EiemPhysicsOwnerTraceBuildCloth(
    void *meshConfig, void *animator, void *goRef, void *model,
    void *methodInfo) {
  auto original = (EiemPhysicsOwnerBuildCloth)s_eiemPhysicsOwnerOrigBuildCloth;
  if (original)
    original(meshConfig, animator, goRef, model, methodInfo);
  if (!EiemOnUnityThread()) return;

  void *refAnimator = EiemPhysicsOwnerReadValueMember(
      goRef, s_eiemPhysicsOwnerGoRefAnimatorOffset);
  void *refModel = EiemPhysicsOwnerReadValueMember(
      goRef, s_eiemPhysicsOwnerGoRefGameObjectOffset);
  void *clothList = EiemPhysicsOwnerReadValueMember(
      goRef, s_eiemPhysicsOwnerGoRefBoneClothsOffset);
  const int clothCount = EiemPhysicsOwnerReadListCount(clothList);

  auto *entry = EiemPhysicsOwnerFindNpc(model);
  if (!entry && s_eiemPhysicsOwnerNpcCount <
                    _countof(s_eiemPhysicsOwnerNpcs)) {
    entry = &s_eiemPhysicsOwnerNpcs[s_eiemPhysicsOwnerNpcCount++];
    entry->model = model;
    entry->modelRef = EiemUnityRef::Capture(model);
  }
  bool target = false;
  if (entry) {
    entry->animator = animator;
    entry->animatorRef = EiemUnityRef::Capture(animator);
    entry->boneCloths = clothCount;
    entry->sawBuild = true;
    target = EiemPhysicsOwnerCorrelate(*entry);
  }
  if (target || InterlockedIncrement(&s_eiemPhysicsOwnerBaselineLogs) <= 8)
    Log("%s npc-build config=%p model=%p animator=%p goRef=%p refAnimator=%p sameAnimator=%d refModel=%p sameModel=%d boneCloths=%d target=%d",
        EiemPhysicsOwnerProbeTag, meshConfig, model, animator, goRef,
        refAnimator, refAnimator == animator ? 1 : 0, refModel,
        refModel == model ? 1 : 0, clothCount, target ? 1 : 0);
}

static void EiemPhysicsOwnerTraceStartNpc(void *avatar, void *component,
                                          void *methodInfo) {
  auto original = (EiemPhysicsOwnerVoid1)s_eiemPhysicsOwnerOrigStartNpc;
  if (original) original(avatar, component, methodInfo);
  if (!EiemOnUnityThread()) return;

  void *model = s_eiemPhysicsOwnerNpcGetModelGo
                    ? Invoke(s_eiemPhysicsOwnerNpcGetModelGo, avatar)
                    : nullptr;
  void *payload = avatar && s_eiemPhysicsOwnerNpcAvatarGoRefOffset >= 0
                      ? (char *)avatar +
                            s_eiemPhysicsOwnerNpcAvatarGoRefOffset
                      : nullptr;
  void *embeddedAnimator = EiemPhysicsOwnerReadValueMember(
      payload, s_eiemPhysicsOwnerGoRefAnimatorOffset);
  void *embeddedModel = EiemPhysicsOwnerReadValueMember(
      payload, s_eiemPhysicsOwnerGoRefGameObjectOffset);
  void *embeddedCloths = EiemPhysicsOwnerReadValueMember(
      payload, s_eiemPhysicsOwnerGoRefBoneClothsOffset);
  void *componentAvatar = EiemPhysicsOwnerReadPointer(
      component, s_eiemPhysicsOwnerNpcComponentAvatarOffset);
  const int embeddedClothCount = EiemPhysicsOwnerReadListCount(embeddedCloths);

  auto *entry = EiemPhysicsOwnerFindNpc(model);
  if (!entry && s_eiemPhysicsOwnerNpcCount <
                    _countof(s_eiemPhysicsOwnerNpcs)) {
    entry = &s_eiemPhysicsOwnerNpcs[s_eiemPhysicsOwnerNpcCount++];
    entry->model = model;
    entry->modelRef = EiemUnityRef::Capture(model);
    entry->animator = embeddedAnimator;
    entry->animatorRef = EiemUnityRef::Capture(embeddedAnimator);
    entry->boneCloths = embeddedClothCount;
  }
  bool target = false;
  void *buildAnimator = nullptr;
  int buildCloths = -1;
  bool buildSeen = false;
  if (entry) {
    buildSeen = entry->sawBuild;
    if (buildSeen) {
      buildAnimator = entry->animator;
      buildCloths = entry->boneCloths;
    } else {
      entry->animator = embeddedAnimator;
      entry->animatorRef = EiemUnityRef::Capture(embeddedAnimator);
      entry->boneCloths = embeddedClothCount;
    }
    entry->avatar = avatar;
    entry->component = component;
    entry->sawStart = true;
    target = entry->target || EiemPhysicsOwnerCorrelate(*entry);
  }
  if (target || InterlockedIncrement(&s_eiemPhysicsOwnerBaselineLogs) <= 8)
    Log("%s npc-start avatar=%p component=%p componentAvatar=%p sameAvatar=%d model=%p embeddedModel=%p sameModel=%d buildSeen=%d buildAnimator=%p embeddedAnimator=%p sameAnimator=%d embeddedCloths=%d buildCloths=%d target=%d",
        EiemPhysicsOwnerProbeTag, avatar, component, componentAvatar,
        componentAvatar == avatar ? 1 : 0, model, embeddedModel,
        embeddedModel == model ? 1 : 0, buildSeen ? 1 : 0, buildAnimator,
        embeddedAnimator,
        buildSeen && embeddedAnimator == buildAnimator ? 1 : 0,
        embeddedClothCount, buildCloths,
        target ? 1 : 0);
  if (model && component)
    EiemRegisterAndApplyModelInstance(
        EiemModelOwnerKind::NpcAvatar, component, model, nullptr, 0,
        "NPCAvatar.StartNPC");
}

static bool EiemPhysicsOwnerReleaseInfo(void *component, void **avatarOut,
                                        void **modelOut, void **animatorOut) {
  void *avatar = EiemPhysicsOwnerReadPointer(
      component, s_eiemPhysicsOwnerNpcComponentAvatarOffset);
  bool target = false;
  void *model = nullptr;
  void *animator = nullptr;
  for (size_t index = 0; index < s_eiemPhysicsOwnerNpcCount; ++index) {
    const auto &entry = s_eiemPhysicsOwnerNpcs[index];
    if ((avatar && entry.avatar == avatar) ||
        (component && entry.component == component)) {
      target = entry.target;
      model = entry.model;
      animator = entry.animator;
      break;
    }
  }
  if (avatarOut) *avatarOut = avatar;
  if (modelOut) *modelOut = model;
  if (animatorOut) *animatorOut = animator;
  return target;
}

static void EiemPhysicsOwnerTraceReleaseAvatar(void *manager, void *component,
                                                void *methodInfo) {
  void *avatar = nullptr, *model = nullptr, *animator = nullptr;
  const bool target = EiemPhysicsOwnerReleaseInfo(
      component, &avatar, &model, &animator);
  if (target)
    Log("%s npc-release-enter manager=%p component=%p avatar=%p model=%p animator=%p",
        EiemPhysicsOwnerProbeTag, manager, component, avatar, model, animator);
  EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar, component,
                       "NPCAvatarManager.ReleaseAvatar");
  auto original =
      (EiemPhysicsOwnerVoid1)s_eiemPhysicsOwnerOrigReleaseAvatar;
  if (original) original(manager, component, methodInfo);
  if (target)
    Log("%s npc-release-return manager=%p component=%p avatar=%p",
        EiemPhysicsOwnerProbeTag, manager, component, avatar);
}

static void EiemPhysicsOwnerTraceNpcOnRelease(void *component,
                                              void *methodInfo) {
  void *avatar = nullptr, *model = nullptr, *animator = nullptr;
  const bool target = EiemPhysicsOwnerReleaseInfo(
      component, &avatar, &model, &animator);
  if (target)
    Log("%s npc-component-release-enter component=%p avatar=%p model=%p animator=%p",
        EiemPhysicsOwnerProbeTag, component, avatar, model, animator);
  EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar, component,
                       "NPCCrowdEntityComponent.OnRelease");
  auto original = (EiemPhysicsOwnerVoid0)s_eiemPhysicsOwnerOrigNpcOnRelease;
  if (original) original(component, methodInfo);
  if (target)
    Log("%s npc-component-release-return component=%p avatar=%p",
        EiemPhysicsOwnerProbeTag, component, avatar);
}

static bool EiemPhysicsOwnerHookExact(
    void *klass, const char *methodName, const char *const *paramTypes,
    int paramCount, const char *returnType, const char *label, void *detour,
    void **original) {
  void *method = FindMethodWithParamTypesAndReturnType(
      klass, methodName, paramTypes, paramCount, returnType);
  const bool installed = method && Hook(method, label, detour, original);
  Log("%s hook label=%s installed=%d method=%p", EiemPhysicsOwnerProbeTag,
      label, installed ? 1 : 0, method);
  return installed;
}

static void EiemInstallNpcModelOwner(void **assemblies, size_t count) {
  void *gameObject = FindClass("UnityEngine", "GameObject", assemblies, count);
  void *component = FindClass("UnityEngine", "Component", assemblies, count);
  void *transform = FindClass("UnityEngine", "Transform", assemblies, count);
  s_eiemPhysicsOwnerAnimatorClass =
      FindClass("UnityEngine", "Animator", assemblies, count);
  s_eiemPhysicsOwnerBoneClothClass =
      FindClass("BeyondDynamicBone", "BeyondBoneCloth", assemblies, count);
  s_eiemPhysicsOwnerGetComponentsInChildren =
      FindMethod(gameObject, "GetComponentsInChildren", 2);
  s_eiemPhysicsOwnerGameObjectGetTransform =
      FindMethod(gameObject, "get_transform", 0);
  s_eiemPhysicsOwnerComponentGetTransform =
      FindMethod(component, "get_transform", 0);
  s_eiemPhysicsOwnerTransformGetParent =
      FindMethod(transform, "get_parent", 0);

  void *avatarClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatar", assemblies, count);
  void *goRefClass = FindClass(
      "Beyond.NPC.Avatar", "FNPCAvatarGOReference", assemblies, count);
  void *npcComponentClass = FindClass(
      "Beyond.NPC", "NPCCrowdEntityComponent", assemblies, count);
  void *managerClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarManager", assemblies, count);

  const char *goRefFields[] = {"avatarGoRef"};
  const char *componentAvatarFields[] = {"avatar"};
  const char *animatorFields[] = {"animator"};
  const char *gameObjectFields[] = {"go"};
  const char *clothFields[] = {"boneCloths"};
  s_eiemPhysicsOwnerNpcAvatarGoRefOffset = FindFieldInHierarchy(
      avatarClass, goRefFields, 1, nullptr);
  s_eiemPhysicsOwnerNpcComponentAvatarOffset = FindFieldInHierarchy(
      npcComponentClass, componentAvatarFields, 1, nullptr);
  s_eiemPhysicsOwnerGoRefAnimatorOffset = FindFieldInHierarchy(
      goRefClass, animatorFields, 1, nullptr);
  s_eiemPhysicsOwnerGoRefGameObjectOffset = FindFieldInHierarchy(
      goRefClass, gameObjectFields, 1, nullptr);
  s_eiemPhysicsOwnerGoRefBoneClothsOffset = FindFieldInHierarchy(
      goRefClass, clothFields, 1, nullptr);
  s_eiemPhysicsOwnerNpcGetModelGo = FindMethodWithReturnType(
      avatarClass, "GetModelGo", "UnityEngine.GameObject", 0);

  Log("%s metadata avatarGoRef=0x%X componentAvatar=0x%X goRefAnimator=0x%X goRefGo=0x%X goRefCloths=0x%X getModelGo=%p",
      EiemPhysicsOwnerProbeTag, s_eiemPhysicsOwnerNpcAvatarGoRefOffset,
      s_eiemPhysicsOwnerNpcComponentAvatarOffset,
      s_eiemPhysicsOwnerGoRefAnimatorOffset,
      s_eiemPhysicsOwnerGoRefGameObjectOffset,
      s_eiemPhysicsOwnerGoRefBoneClothsOffset,
      s_eiemPhysicsOwnerNpcGetModelGo);

  static const char *const startTypes[] = {
      "Beyond.NPC.NPCCrowdEntityComponent"};
  EiemPhysicsOwnerHookExact(
      avatarClass, "StartNPC", startTypes, 1, "System.Void",
      "NPC model owner NPCAvatar.StartNPC",
      (void *)EiemPhysicsOwnerTraceStartNpc,
      &s_eiemPhysicsOwnerOrigStartNpc);

  static const char *const buildTypes[] = {
      "Beyond.NPC.Avatar.NPCAvatarMeshAssetsSO", "UnityEngine.Animator",
      "Beyond.NPC.Avatar.FNPCAvatarGOReference&", "UnityEngine.GameObject"};
  EiemPhysicsOwnerHookExact(
      managerClass, "_BuildBeyondCloth", buildTypes, 4, "System.Void",
      "NPC model owner NPCAvatarManager._BuildBeyondCloth",
      (void *)EiemPhysicsOwnerTraceBuildCloth,
      &s_eiemPhysicsOwnerOrigBuildCloth);

  static const char *const releaseTypes[] = {
      "Beyond.NPC.NPCCrowdEntityComponent"};
  EiemPhysicsOwnerHookExact(
      managerClass, "ReleaseAvatar", releaseTypes, 1, "System.Void",
      "NPC model owner NPCAvatarManager.ReleaseAvatar",
      (void *)EiemPhysicsOwnerTraceReleaseAvatar,
      &s_eiemPhysicsOwnerOrigReleaseAvatar);
  EiemPhysicsOwnerHookExact(
      npcComponentClass, "OnRelease", nullptr, 0, "System.Void",
      "NPC model owner NPCCrowdEntityComponent.OnRelease",
      (void *)EiemPhysicsOwnerTraceNpcOnRelease,
      &s_eiemPhysicsOwnerOrigNpcOnRelease);
}
