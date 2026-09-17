#pragma once

// Low-volume evidence for the one lifecycle shared by Render and Physics.
// This header deliberately has no Unity calls and no ownership side effects.
// Each event is keyed by the concrete managed objects and stage, so repeated
// callbacks from cached/persistent models do not flood the game log.

#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

void Log(const char *fmt, ...);

static constexpr const char *EiemRegistrationTraceTag = "[INSTANCE-REG-v3]";

struct EiemRegistrationTraceSlot {
  uint64_t event = 0;
  uint64_t stage = 0;
  void *first = nullptr;
  void *second = nullptr;
  void *third = nullptr;
  LONG generation = -1;
  bool used = false;
};

static SRWLOCK s_eiemRegistrationTraceLock = SRWLOCK_INIT;
static EiemRegistrationTraceSlot s_eiemRegistrationTraceSlots[4096] = {};
static uint32_t s_eiemRegistrationTraceNext = 0;

static uint64_t EiemRegistrationTraceHash(const char *text) {
  // FNV-1a is sufficient here; this is a log de-duplication key, not an ID.
  uint64_t hash = 1469598103934665603ULL;
  if (!text) return hash;
  for (const unsigned char *cursor = (const unsigned char *)text; *cursor;
       ++cursor) {
    hash ^= *cursor;
    hash *= 1099511628211ULL;
  }
  return hash;
}

static bool EiemRegistrationTraceFirst(const char *event, const char *stage,
                                        void *first, void *second,
                                        void *third, LONG generation) {
  const uint64_t eventHash = EiemRegistrationTraceHash(event);
  const uint64_t stageHash = EiemRegistrationTraceHash(stage);
  AcquireSRWLockExclusive(&s_eiemRegistrationTraceLock);
  for (const auto &slot : s_eiemRegistrationTraceSlots) {
    if (slot.used && slot.event == eventHash && slot.stage == stageHash &&
        slot.first == first && slot.second == second && slot.third == third &&
        slot.generation == generation) {
      ReleaseSRWLockExclusive(&s_eiemRegistrationTraceLock);
      return false;
    }
  }
  auto &slot = s_eiemRegistrationTraceSlots[
      s_eiemRegistrationTraceNext++ % _countof(s_eiemRegistrationTraceSlots)];
  slot.event = eventHash;
  slot.stage = stageHash;
  slot.first = first;
  slot.second = second;
  slot.third = third;
  slot.generation = generation;
  slot.used = true;
  ReleaseSRWLockExclusive(&s_eiemRegistrationTraceLock);
  return true;
}

static void EiemRegistrationTraceModel(
    const char *ownerKind, void *owner, void *model, const char *stage,
    LONG generation, size_t ownerCount, bool active, bool applied,
    int animatorCount, int clothCount, const char *path) {
  if (!EiemRegistrationTraceFirst("model", stage, owner, model, nullptr,
                                  generation))
    return;
  Log("%s event=model ownerKind=%s owner=%p model=%p generation=%ld "
      "owners=%zu active=%d applied=%d animators=%zu cloths=%zu path=%s stage=%s",
      EiemRegistrationTraceTag, ownerKind ? ownerKind : "unknown", owner,
      model, generation, ownerCount, active ? 1 : 0, applied ? 1 : 0,
      animatorCount, clothCount, path && path[0] ? path : "<unknown>",
      stage ? stage : "unknown");
}

static void EiemRegistrationTraceOwnerState(
    const char *ownerKind, void *owner, void *model, bool active,
    const char *stage, LONG generation) {
  if (!EiemRegistrationTraceFirst("owner-state", stage, owner, model,
                                  nullptr, generation))
    return;
  Log("%s event=owner-state ownerKind=%s owner=%p model=%p generation=%ld "
      "active=%d stage=%s",
      EiemRegistrationTraceTag, ownerKind ? ownerKind : "unknown", owner,
      model, generation, active ? 1 : 0, stage ? stage : "unknown");
}

static void EiemRegistrationTraceRelease(
    const char *ownerKind, void *owner, void *model, const char *stage,
    LONG generation) {
  if (!EiemRegistrationTraceFirst("release", stage, owner, model, nullptr,
                                  generation))
    return;
  Log("%s event=release ownerKind=%s owner=%p model=%p generation=%ld stage=%s",
      EiemRegistrationTraceTag, ownerKind ? ownerKind : "unknown", owner,
      model, generation, stage ? stage : "unknown");
}

static void EiemRegistrationTraceRenderer(void *renderer, const char *stage,
                                          LONG generation) {
  if (!EiemRegistrationTraceFirst("renderer", stage, renderer, nullptr,
                                  nullptr, generation))
    return;
  Log("%s event=renderer renderer=%p generation=%ld stage=%s",
      EiemRegistrationTraceTag, renderer, generation,
      stage ? stage : "unknown");
}

// Correlate a concrete source/Partner renderer with the model registry entry
// that owns it. This is evidence only: it does not retain objects or alter
// ownership. A renderer may be created before its model owner is published,
// so the caller also records the unregistered case explicitly.
static void EiemRegistrationTraceRendererOwner(
    void *renderer, void *model, const char *ownerKind, void *owner,
    uint32_t instanceUid, size_t ownerCount, bool active, const char *path,
    const char *stage, LONG generation) {
  if (!EiemRegistrationTraceFirst("renderer-owner", stage, renderer, model,
                                  owner, generation))
    return;
  Log("%s event=renderer-owner renderer=%p model=%p ownerKind=%s owner=%p "
      "instanceUid=%u owners=%zu active=%d path=%s generation=%ld stage=%s",
      EiemRegistrationTraceTag, renderer, model,
      ownerKind && ownerKind[0] ? ownerKind : "<unregistered>", owner,
      instanceUid, ownerCount, active ? 1 : 0,
      path && path[0] ? path : "<unknown>", generation,
      stage && stage[0] ? stage : "unknown");
}

static void EiemRegistrationTraceArrayBoundary(
    const char *boundary, void *owner, void *array, size_t count,
    LONG generation, int32_t lod) {
  if (!kEiemValidationIdentityProbe) return;
  if (!EiemRegistrationTraceFirst("array-boundary", boundary, owner, array,
                                  nullptr, generation))
    return;
  Log("%s event=array-boundary boundary=%s owner=%p array=%p count=%zu "
      "lod=%d generation=%ld",
      EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner, array,
      count, lod, generation);
}

// The game passes a Renderer array together with a second, index-related
// skinning array. Before changing either array, record their managed types,
// lengths and one concrete source index. This establishes whether the two
// arrays can be extended as a pair; public SMR setters do not prove that.
static void EiemRegistrationTraceParallelArrays(
    const char *boundary, void *primary, const char *primaryType,
    size_t primaryCount, void *parallel, const char *parallelType,
    size_t parallelCount, size_t sourceIndex, const char *sourceSection,
    LONG generation) {
  if (!kEiemValidationIdentityProbe) return;
  if (!EiemRegistrationTraceFirst("parallel-arrays", boundary, primary,
                                  parallel, nullptr, generation))
    return;
  Log("%s event=parallel-arrays boundary=%s primary=%p primaryType=%s "
      "primaryCount=%zu parallel=%p parallelType=%s parallelCount=%zu "
      "sourceIndex=%zu sourceSection=%s generation=%ld",
      EiemRegistrationTraceTag, boundary ? boundary : "unknown", primary,
      primaryType && primaryType[0] ? primaryType : "<unknown>",
      primaryCount, parallel,
      parallelType && parallelType[0] ? parallelType : "<unknown>",
      parallelCount, sourceIndex,
      sourceSection && sourceSection[0] ? sourceSection : "<unknown>",
      generation);
}

// A public bones/root-bone assignment does not prove that a dynamically added
// Renderer was accepted by the game's internal skin/Animator registry.  Keep
// the array membership check separate from the boundary record so the probe
// can compare the concrete Partner pointers without touching Unity state.
struct EiemRegistrationTracePartnerEntry {
  void *source = nullptr;
  void *partner = nullptr;
  // Model pointer captured from the Partner lifecycle record.  This is only
  // a correlation key; the probe never retains or dereferences it.
  void *ownerModel = nullptr;
  bool visible = false;
  char section[96] = {};
};

// Keep the per-array pair evidence bounded.  The normal array-boundary and
// item records remain the primary evidence; these records only add the exact
// source/Partner membership result needed to distinguish a missing handoff
// from a pose/root-bone problem.
static volatile LONG s_eiemRegistrationTracePairCount = 0;

static void EiemRegistrationTraceArrayMembers(
    const char *boundary, void *owner, void *array, size_t count,
    LONG generation, const EiemRegistrationTracePartnerEntry *entries,
    size_t entryCount) {
  if (!kEiemValidationIdentityProbe) return;
  if (!array || !entries || entryCount == 0 || count > 8192) return;
  size_t sourceHits = 0, partnerHits = 0, visibleHits = 0;
  size_t sourceRefs = 0, partnerRefs = 0, visibleRefs = 0;
  const char *firstMissing = nullptr;
  const char *firstVisibleMissing = nullptr;
  const char *firstVisibleHit = nullptr;
  bool sourceHitFlags[256] = {};
  bool partnerHitFlags[256] = {};
  size_t sourceIndices[256] = {};
  size_t partnerIndices[256] = {};
  void *sourcePointers[256] = {};
  bool sourceMatched[256] = {};
  size_t sourcePointerCount = 0;
  bool partnerSeen[256] = {};
  bool visibleSeen[256] = {};
  const size_t boundedEntries = entryCount < _countof(partnerSeen)
                                    ? entryCount
                                    : _countof(partnerSeen);
  for (size_t entryIndex = 0; entryIndex < boundedEntries; ++entryIndex) {
    sourceIndices[entryIndex] = SIZE_MAX;
    partnerIndices[entryIndex] = SIZE_MAX;
  }
  __try {
    void **items = (void **)((char *)array + IL2CPP_ARRAY_DATA);
    for (size_t entryIndex = 0; entryIndex < boundedEntries; ++entryIndex) {
      const auto &entry = entries[entryIndex];
      bool sourceHit = false, partnerHit = false;
      for (size_t itemIndex = 0; itemIndex < count; ++itemIndex) {
        if (items[itemIndex] == entry.source) {
          sourceHit = true;
          if (sourceIndices[entryIndex] == SIZE_MAX)
            sourceIndices[entryIndex] = itemIndex;
        }
        if (items[itemIndex] == entry.partner) {
          partnerHit = true;
          if (partnerIndices[entryIndex] == SIZE_MAX)
            partnerIndices[entryIndex] = itemIndex;
        }
        if (sourceHit && partnerHit) break;
      }
      sourceHitFlags[entryIndex] = sourceHit;
      partnerHitFlags[entryIndex] = partnerHit;
    }
    // Count only source/Partner pairs belonging to this managed array. The
    // global Partner table spans every active model instance; unrelated pairs
    // must not turn a useful registration probe into a false all-missing
    // result. If a Partner is present while its source is absent, retain that
    // concrete hit as a separate late-registration signal.
    for (size_t entryIndex = 0; entryIndex < boundedEntries; ++entryIndex) {
      const auto &entry = entries[entryIndex];
      if ((!sourceHitFlags[entryIndex] && !partnerHitFlags[entryIndex]) ||
          !entry.source)
        continue;
      if (entry.source) {
        bool knownSource = false;
        for (size_t sourceIndex = 0; sourceIndex < sourcePointerCount;
             ++sourceIndex) {
          if (sourcePointers[sourceIndex] == entry.source) {
            knownSource = true;
            if (sourceHitFlags[entryIndex]) sourceMatched[sourceIndex] = true;
            break;
          }
        }
        if (!knownSource && sourcePointerCount < _countof(sourcePointers)) {
          sourcePointers[sourcePointerCount++] = entry.source;
          ++sourceRefs;
          sourceMatched[sourcePointerCount - 1] = sourceHitFlags[entryIndex];
        }
      }
    }
    for (size_t sourceIndex = 0; sourceIndex < sourcePointerCount;
         ++sourceIndex)
      if (sourceMatched[sourceIndex]) ++sourceHits;
    for (size_t entryIndex = 0; entryIndex < boundedEntries; ++entryIndex) {
      const auto &entry = entries[entryIndex];
      const bool relevant = sourceHitFlags[entryIndex] ||
                            partnerHitFlags[entryIndex];
      if (!relevant || !entry.partner) continue;
      ++partnerRefs;
      if (partnerHitFlags[entryIndex] && !partnerSeen[entryIndex]) {
        partnerSeen[entryIndex] = true;
        ++partnerHits;
      } else if (sourceHitFlags[entryIndex] && !firstMissing)
        firstMissing = entry.section;
      if (entry.visible) {
        ++visibleRefs;
        if (partnerHitFlags[entryIndex] && !visibleSeen[entryIndex]) {
          visibleSeen[entryIndex] = true;
          ++visibleHits;
          if (!firstVisibleHit) firstVisibleHit = entry.section;
        } else if (sourceHitFlags[entryIndex] && !firstVisibleMissing)
          firstVisibleMissing = entry.section;
      }
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    Log("%s event=array-members-failed boundary=%s owner=%p array=%p "
        "count=%zu generation=%ld exception=0x%08lX",
        EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner,
        array, count, generation, GetExceptionCode());
    return;
  }
  // A negative result is meaningful only for a concrete AssignSkin array.
  // Emit a small, deduplicated pair record there even when neither pointer is
  // present, so a world model can be distinguished from an unrelated NPC
  // array.  The ownerModel is later correlated with the model registry by the
  // caller's renderer-owner trace; no Unity object is touched here.
  const bool finalSkinBoundary =
      boundary && (strstr(boundary, "AssignSkin") != nullptr);
  if (!finalSkinBoundary) return;
  for (size_t entryIndex = 0; entryIndex < boundedEntries; ++entryIndex) {
    const auto &entry = entries[entryIndex];
    if (!entry.source || !entry.partner) continue;
    if (!EiemRegistrationTraceFirst("array-pair", boundary, entry.source,
                                    entry.partner, array, generation))
      continue;
    if (InterlockedIncrement(&s_eiemRegistrationTracePairCount) > 512)
      break;
    Log("%s event=array-pair boundary=%s array=%p count=%zu generation=%ld "
        "ownerModel=%p section=%s source=%p sourceHit=%d sourceIndex=%zu "
        "partner=%p partnerHit=%d partnerIndex=%zu visible=%d",
        EiemRegistrationTraceTag, boundary, array, count, generation,
        entry.ownerModel, entry.section[0] ? entry.section : "<unknown>",
        entry.source, sourceHitFlags[entryIndex] ? 1 : 0,
        sourceIndices[entryIndex], entry.partner,
        partnerHitFlags[entryIndex] ? 1 : 0, partnerIndices[entryIndex],
        entry.visible ? 1 : 0);
  }

  if (!sourceHits && !partnerHits) return;
  if (!EiemRegistrationTraceFirst("array-members", boundary, owner, array,
                                  nullptr, generation))
    return;
  Log("%s event=array-members boundary=%s owner=%p array=%p count=%zu "
      "sourceHits=%zu/%zu partnerHits=%zu/%zu visibleHits=%zu/%zu "
      "firstMissing=%s firstVisibleMissing=%s firstVisibleHit=%s "
      "generation=%ld",
      EiemRegistrationTraceTag, boundary ? boundary : "unknown", owner, array,
      count, sourceHits, sourceRefs, partnerHits, partnerRefs, visibleHits,
      visibleRefs, firstMissing && firstMissing[0] ? firstMissing : "<none>",
      firstVisibleMissing && firstVisibleMissing[0] ? firstVisibleMissing
                                                   : "<none>",
      firstVisibleHit && firstVisibleHit[0] ? firstVisibleHit : "<none>",
      generation);
}

static void EiemRegistrationTracePartner(
    const char *boundary, void *source, void *partner, LONG generation,
    const char *section, size_t sourceBones, size_t partnerBones,
    long long firstDiff, void *sourceParent, void *partnerParent,
    void *sourceRoot, void *partnerRoot, void *sourceMesh = nullptr,
    void *partnerMesh = nullptr, void *skeletonAnchor = nullptr,
    size_t skeletonNodes = 0, size_t skeletonAdded = 0,
    uint64_t sourceBoneHash = 0, uint64_t partnerBoneHash = 0,
    size_t sourceBindposes = 0, size_t partnerBindposes = 0,
    uint64_t sourceBindposeHash = 0, uint64_t partnerBindposeHash = 0) {
  if (!kEiemValidationIdentityProbe) return;
  if (!EiemRegistrationTraceFirst("partner", boundary, source, partner,
                                  nullptr, generation))
    return;
  Log("%s event=partner boundary=%s generation=%ld source=%p partner=%p "
      "section=%s mesh=%p/%p bones=%zu/%zu firstDiff=%lld parent=%p/%p "
      "root=%p/%p skeletonAnchor=%p skeletonNodes=%zu skeletonAdded=%zu "
      "boneHash=%016llX/%016llX bindposes=%zu/%zu "
      "bindposeHash=%016llX/%016llX",
      EiemRegistrationTraceTag, boundary ? boundary : "unknown", generation,
      source, partner, section && section[0] ? section : "<unknown>",
      sourceMesh, partnerMesh, sourceBones, partnerBones, firstDiff,
      sourceParent, partnerParent, sourceRoot, partnerRoot, skeletonAnchor,
      skeletonNodes, skeletonAdded, (unsigned long long)sourceBoneHash,
      (unsigned long long)partnerBoneHash, sourceBindposes, partnerBindposes,
      (unsigned long long)sourceBindposeHash,
      (unsigned long long)partnerBindposeHash);
}

static void EiemRegistrationTracePartnerPose(
    const char *boundary, void *source, void *partner, LONG generation,
    void *sourceTransform, void *partnerTransform, void *sourceRoot,
    void *partnerRoot, bool sourcePositionRead, float sourceX, float sourceY,
    float sourceZ, bool partnerPositionRead, float partnerX, float partnerY,
    float partnerZ, bool sourceLocalPositionRead, float sourceLocalX,
    float sourceLocalY, float sourceLocalZ, bool partnerLocalPositionRead,
    float partnerLocalX, float partnerLocalY, float partnerLocalZ,
    bool sourceRootPositionRead, float sourceRootX, float sourceRootY,
    float sourceRootZ, bool partnerRootPositionRead, float partnerRootX,
    float partnerRootY, float partnerRootZ) {
  if (!kEiemValidationIdentityProbe) return;
  char stateKey[192] = {};
  _snprintf_s(stateKey, _countof(stateKey), _TRUNCATE,
              "pose|boundary=%s|src=%p|dst=%p|srcRead=%d|dstRead=%d|srcRootRead=%d|"
              "dstRootRead=%d", boundary ? boundary : "unknown", source,
              partner, sourcePositionRead ? 1 : 0,
              partnerPositionRead ? 1 : 0, sourceRootPositionRead ? 1 : 0,
              partnerRootPositionRead ? 1 : 0);
  if (!EiemRegistrationTraceFirst("partner-pose", stateKey, source, partner,
                                  sourceTransform, generation))
    return;
  Log("%s event=partner-pose boundary=%s generation=%ld source=%p partner=%p "
      "sourceTransform=%p partnerTransform=%p sourceRoot=%p partnerRoot=%p "
      "sourcePosRead=%d sourcePos=%.6g,%.6g,%.6g "
      "partnerPosRead=%d partnerPos=%.6g,%.6g,%.6g "
      "sourceLocalRead=%d sourceLocal=%.6g,%.6g,%.6g "
      "partnerLocalRead=%d partnerLocal=%.6g,%.6g,%.6g "
      "sourceRootPosRead=%d sourceRootPos=%.6g,%.6g,%.6g "
      "partnerRootPosRead=%d partnerRootPos=%.6g,%.6g,%.6g",
      EiemRegistrationTraceTag, boundary ? boundary : "unknown", generation,
      source, partner, sourceTransform, partnerTransform, sourceRoot,
      partnerRoot, sourcePositionRead ? 1 : 0, sourceX, sourceY, sourceZ,
      partnerPositionRead ? 1 : 0, partnerX, partnerY, partnerZ,
      sourceLocalPositionRead ? 1 : 0, sourceLocalX, sourceLocalY,
      sourceLocalZ, partnerLocalPositionRead ? 1 : 0, partnerLocalX,
      partnerLocalY, partnerLocalZ, sourceRootPositionRead ? 1 : 0,
      sourceRootX, sourceRootY, sourceRootZ, partnerRootPositionRead ? 1 : 0,
      partnerRootX, partnerRootY, partnerRootZ);
}

static void EiemRegistrationTraceLod(void *source, void *partner, bool add,
                                     bool changed, size_t lodCount,
                                     const char *stage, LONG generation,
                                     void *group = nullptr,
                                     uint64_t sourceLevels = 0,
                                     uint64_t partnerLevels = 0,
                                     bool sourceEnabled = true,
                                     bool partnerEnabled = true,
                                     bool sourceForceOff = false,
                                     bool partnerForceOff = false) {
  if (!kEiemValidationIdentityProbe) return;
  const char *operation = add ? "add" : "remove";
  char stateKey[256] = {};
  _snprintf_s(stateKey, _countof(stateKey), _TRUNCATE,
              "%s|group=%p|changed=%d|lodCount=%zu|sourceLevels=%016llX|"
              "partnerLevels=%016llX|sourceEnabled=%d|partnerEnabled=%d|"
              "sourceForceOff=%d|partnerForceOff=%d",
              stage ? stage : "unknown", group, changed ? 1 : 0, lodCount,
              (unsigned long long)sourceLevels,
              (unsigned long long)partnerLevels, sourceEnabled ? 1 : 0,
              partnerEnabled ? 1 : 0, sourceForceOff ? 1 : 0,
              partnerForceOff ? 1 : 0);
  if (!EiemRegistrationTraceFirst(operation, stateKey, source, partner, group,
                                  generation))
    return;
  Log("%s event=lod operation=%s source=%p partner=%p generation=%ld "
      "group=%p changed=%d lodCount=%zu sourceLevels=0x%016llX "
      "partnerLevels=0x%016llX sourceEnabled=%d partnerEnabled=%d "
      "sourceForceOff=%d partnerForceOff=%d stage=%s",
      EiemRegistrationTraceTag, operation, source, partner, generation,
      group, changed ? 1 : 0, lodCount,
      (unsigned long long)sourceLevels, (unsigned long long)partnerLevels,
      sourceEnabled ? 1 : 0, partnerEnabled ? 1 : 0,
      sourceForceOff ? 1 : 0, partnerForceOff ? 1 : 0,
      stage ? stage : "unknown");
}

static void EiemRegistrationTraceLodGroupSet(
    void *group, void *lods, size_t lodCount, bool fromEiem, LONG generation) {
  if (!kEiemValidationIdentityProbe) return;
  char stateKey[128] = {};
  _snprintf_s(stateKey, _countof(stateKey), _TRUNCATE,
              "SetLODs|count=%zu|origin=%d", lodCount, fromEiem ? 1 : 0);
  if (!EiemRegistrationTraceFirst("lod-group-set", stateKey, group, lods,
                                  nullptr, generation))
    return;
  Log("%s event=lod-group-set group=%p lods=%p lodCount=%zu origin=%s "
      "generation=%ld",
      EiemRegistrationTraceTag, group, lods, lodCount,
      fromEiem ? "eiem" : "game", generation);
}

// A SetLODs call carries the actual Renderer membership used by Unity's
// standard LOD path. Keep this separate from the mutation trace so the
// investigation can correlate a game-owned array with a known source or
// Partner without changing the array.
static void EiemRegistrationTraceLodGroupMember(
    void *group, size_t lodIndex, void *renderer, void *mesh,
    const char *rendererType, const char *objectName, bool knownSource,
    bool knownPartner, bool active, bool activeRead, bool enabled,
    bool enabledRead, bool forceRenderingOff, bool forceRenderingOffRead,
    LONG generation) {
  if (!kEiemValidationIdentityProbe) return;
  char stateKey[256] = {};
  _snprintf_s(
      stateKey, _countof(stateKey), _TRUNCATE,
      "lod=%zu|type=%s|source=%d|partner=%d|active=%d|enabled=%d|force=%d",
      lodIndex, rendererType ? rendererType : "Renderer", knownSource ? 1 : 0,
      knownPartner ? 1 : 0, active ? 1 : 0, enabled ? 1 : 0,
      forceRenderingOff ? 1 : 0);
  if (!EiemRegistrationTraceFirst("lod-group-member", stateKey, renderer,
                                  mesh, group, generation))
    return;
  Log("%s event=lod-group-member group=%p lod=%zu renderer=%p mesh=%p "
      "type=%s name=%s source=%d partner=%d active=%d activeRead=%d "
      "enabled=%d enabledRead=%d forceRenderingOff=%d forceRead=%d "
      "generation=%ld",
      EiemRegistrationTraceTag, group, lodIndex, renderer, mesh,
      rendererType && rendererType[0] ? rendererType : "Renderer",
      objectName && objectName[0] ? objectName : "<unnamed>",
      knownSource ? 1 : 0, knownPartner ? 1 : 0, active ? 1 : 0,
      activeRead ? 1 : 0, enabled ? 1 : 0, enabledRead ? 1 : 0,
      forceRenderingOff ? 1 : 0, forceRenderingOffRead ? 1 : 0, generation);
}

static void EiemRegistrationTraceEligibility(
    void *renderer, void *drawRenderer, LONG generation, bool active,
    bool enabled, bool enabledRead, bool forceRenderingOff,
    bool forceRenderingOffRead, bool accepted, const char *reason) {
  if (!kEiemValidationIdentityProbe) return;
  if (!EiemRegistrationTraceFirst("eligibility", reason, renderer,
                                  drawRenderer, nullptr, generation))
    return;
  Log("%s event=eligibility renderer=%p draw=%p generation=%ld active=%d "
      "enabled=%d enabledRead=%d forceRenderingOff=%d forceRead=%d "
      "accepted=%d reason=%s",
      EiemRegistrationTraceTag, renderer, drawRenderer, generation,
      active ? 1 : 0, enabled ? 1 : 0, enabledRead ? 1 : 0,
      forceRenderingOff ? 1 : 0, forceRenderingOffRead ? 1 : 0,
      accepted ? 1 : 0, reason && reason[0] ? reason : "unknown");
}

// Key dispatch is part of the same generation boundary as a reload.  Keep a
// single record at the point where the hotkey worker hands the event to the
// Unity-thread queue; this distinguishes a lost/rebound key from a later
// replay or renderer eligibility decision without adding per-frame logging.
static void EiemRegistrationTraceInput(
    UINT vk, UINT modifiers, LONG generation, bool uiFocus,
    const char *modPath, const char *uiSection, size_t valueCount) {
  Log("%s event=input vk=%u modifiers=0x%X generation=%ld uiFocus=%d "
      "mod=%s ui=%s values=%zu",
      EiemRegistrationTraceTag, vk, modifiers, generation, uiFocus ? 1 : 0,
      modPath && modPath[0] ? modPath : "<key>",
      uiSection && uiSection[0] ? uiSection : "<key>", valueCount);
}

static void EiemRegistrationTraceReconcile(
    const char *phase, uint32_t requests, LONG generation, size_t inputs,
    size_t models, uint32_t matched, ULONGLONG elapsedMs) {
  Log("%s event=reconcile phase=%s requests=0x%X generation=%ld "
      "inputs=%zu models=%zu matched=%u elapsed=%llums",
      EiemRegistrationTraceTag, phase && phase[0] ? phase : "unknown",
      requests, generation, inputs, models, matched, elapsedMs);
}
