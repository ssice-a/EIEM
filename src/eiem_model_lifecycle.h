#pragma once

#include <cstdint>
#include <vector>

// Model ownership is independent from any one world, NPC, or UI hook. Each
// adapter publishes the same concrete model instance to this shared state.
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
  // state live in the Physics runtime adapter, not in this registry.
  std::vector<EiemPhysicsIntent> physicsIntents;
};
