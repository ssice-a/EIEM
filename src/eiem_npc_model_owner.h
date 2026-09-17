#pragma once

// NPC model lifecycle adapter. StartNPC is the registration boundary and
// ReleaseAvatar/OnRelease are the two owner boundaries.

using EiemNpcLifecycleVoid0 = void(__fastcall *)(void *, void *);
using EiemNpcLifecycleVoid1 = void(__fastcall *)(void *, void *, void *);

static void *s_eiemNpcOrigStartNpc = nullptr;
static void *s_eiemNpcOrigReleaseAvatar = nullptr;
static void *s_eiemNpcOrigOnRelease = nullptr;
static void *s_eiemNpcGetModelGo = nullptr;

static bool EiemNpcHookExact(
    void *klass, const char *methodName, const char *const *paramTypes,
    int paramCount, const char *returnType, const char *label, void *detour,
    void **original) {
  void *method = FindMethodWithParamTypesAndReturnType(
      klass, methodName, paramTypes, paramCount, returnType);
  const bool installed = method && Hook(method, label, detour, original);
  Log("%s event=hook label=%s installed=%d method=%p",
      EiemRegistrationTraceTag, label ? label : "unknown", installed ? 1 : 0,
      method);
  return installed;
}

static void EiemNpcTraceStartNpc(void *avatar, void *component,
                                 void *methodInfo) {
  auto original = (EiemNpcLifecycleVoid1)s_eiemNpcOrigStartNpc;
  if (original) original(avatar, component, methodInfo);
  if (!EiemOnUnityThread()) return;

  void *model = s_eiemNpcGetModelGo && avatar
                    ? Invoke(s_eiemNpcGetModelGo, avatar)
                    : nullptr;
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceOwnerState(
      "NpcAvatar", component, model, true, "NPCAvatar.StartNPC", generation);
  if (model && component)
    EiemRegisterAndApplyModelInstance(
        EiemModelOwnerKind::NpcAvatar, component, model, nullptr, 0,
        "NPCAvatar.StartNPC");
}

static void EiemNpcTraceReleaseAvatar(void *manager, void *component,
                                      void *methodInfo) {
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceRelease(
      "NpcAvatar", component, nullptr, "NPCAvatarManager.ReleaseAvatar.enter",
      generation);
  EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar, component,
                       "NPCAvatarManager.ReleaseAvatar");
  auto original = (EiemNpcLifecycleVoid1)s_eiemNpcOrigReleaseAvatar;
  if (original) original(manager, component, methodInfo);
  EiemRegistrationTraceRelease(
      "NpcAvatar", component, nullptr, "NPCAvatarManager.ReleaseAvatar.return",
      generation);
}

static void EiemNpcTraceOnRelease(void *component, void *methodInfo) {
  const LONG generation =
      InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
  EiemRegistrationTraceRelease(
      "NpcAvatar", component, nullptr,
      "NPCCrowdEntityComponent.OnRelease.enter", generation);
  EiemForgetModelOwner(EiemModelOwnerKind::NpcAvatar, component,
                       "NPCCrowdEntityComponent.OnRelease");
  auto original = (EiemNpcLifecycleVoid0)s_eiemNpcOrigOnRelease;
  if (original) original(component, methodInfo);
  EiemRegistrationTraceRelease(
      "NpcAvatar", component, nullptr,
      "NPCCrowdEntityComponent.OnRelease.return", generation);
}

static void EiemInstallNpcModelOwner(void **assemblies, size_t count) {
  void *avatarClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatar", assemblies, count);
  void *npcComponentClass = FindClass(
      "Beyond.NPC", "NPCCrowdEntityComponent", assemblies, count);
  void *managerClass = FindClass(
      "Beyond.NPC.Avatar", "NPCAvatarManager", assemblies, count);
  s_eiemNpcGetModelGo = FindMethodWithReturnType(
      avatarClass, "GetModelGo", "UnityEngine.GameObject", 0);
  Log("%s event=metadata npcGetModelGo=%p", EiemRegistrationTraceTag,
      s_eiemNpcGetModelGo);

  static const char *const startTypes[] = {
      "Beyond.NPC.NPCCrowdEntityComponent"};
  EiemNpcHookExact(
      avatarClass, "StartNPC", startTypes, 1, "System.Void",
      "NPC lifecycle NPCAvatar.StartNPC", (void *)EiemNpcTraceStartNpc,
      &s_eiemNpcOrigStartNpc);

  static const char *const releaseTypes[] = {
      "Beyond.NPC.NPCCrowdEntityComponent"};
  EiemNpcHookExact(
      managerClass, "ReleaseAvatar", releaseTypes, 1, "System.Void",
      "NPC lifecycle NPCAvatarManager.ReleaseAvatar",
      (void *)EiemNpcTraceReleaseAvatar, &s_eiemNpcOrigReleaseAvatar);
  EiemNpcHookExact(
      npcComponentClass, "OnRelease", nullptr, 0, "System.Void",
      "NPC lifecycle NPCCrowdEntityComponent.OnRelease",
      (void *)EiemNpcTraceOnRelease, &s_eiemNpcOrigOnRelease);
}
