#pragma once

#include <windows.h>
#include <cstdint>

// Read-only ordering evidence. The native hook exposes a raw Physics object,
// not a stable model owner, so this records a global completion sequence and
// lets the render path compare its commit against the latest candidate. It is
// intentionally not a gate and never queues or mutates Unity work.
static volatile LONG64 s_eiemPhysicsOrderCompletionSequence = 0;
static volatile LONG64 s_eiemPhysicsOrderLastCompletionTick = 0;
static volatile LONG64 s_eiemPhysicsOrderLastCompletionObject = 0;
static volatile LONG64 s_eiemPhysicsOrderLastCompletionThread = 0;
static volatile LONG64 s_eiemPhysicsOrderLastRenderTick = 0;
static volatile LONG64 s_eiemPhysicsOrderLastRenderModel = 0;
static volatile LONG s_eiemPhysicsOrderRenderCount = 0;

// A CompleteMasterJob return is only one observable boundary.  Clothes can
// also be affected by later native-buffer writes from the data/update stages,
// so keep a bounded, scalar-only sequence for those stages as well.  The
// render path snapshots these counters at commit and reports any increments
// seen by the later skin probe.  This is evidence only; it never gates work.
enum EiemPhysicsOrderStage : uint8_t {
  EiemPhysicsStageDataUpdate = 0,
  EiemPhysicsStagePreSimulation = 1,
  EiemPhysicsStageTeamAnimator = 2,
  EiemPhysicsStageCompleteMaster = 3,
  EiemPhysicsStageCount = 4
};
static volatile LONG64 s_eiemPhysicsOrderOperationSequence = 0;
static volatile LONG64 s_eiemPhysicsOrderLastStageSequence[EiemPhysicsStageCount] = {};
static volatile LONG64 s_eiemPhysicsOrderLastStageTick[EiemPhysicsStageCount] = {};
static volatile LONG64 s_eiemPhysicsOrderRenderStageSequence[EiemPhysicsStageCount] = {};

static void EiemPhysicsOrderProbeMarkStage(uint8_t stage, void *object) {
  if (stage >= EiemPhysicsStageCount) return;
  const LONG64 sequence = InterlockedIncrement64(
      &s_eiemPhysicsOrderOperationSequence);
  InterlockedExchange64(&s_eiemPhysicsOrderLastStageSequence[stage], sequence);
  InterlockedExchange64(&s_eiemPhysicsOrderLastStageTick[stage],
                        (LONG64)GetTickCount64());
  (void)object; // the existing completion probe retains object/thread identity
}

static void EiemPhysicsOrderProbeMarkCompletion(void *object) {
  EiemPhysicsOrderProbeMarkStage(EiemPhysicsStageCompleteMaster, object);
  InterlockedIncrement64(&s_eiemPhysicsOrderCompletionSequence);
  InterlockedExchange64(&s_eiemPhysicsOrderLastCompletionTick,
                        (LONG64)GetTickCount64());
  InterlockedExchange64(&s_eiemPhysicsOrderLastCompletionObject,
                        (LONG64)(uintptr_t)object);
  InterlockedExchange64(&s_eiemPhysicsOrderLastCompletionThread,
                        (LONG64)GetCurrentThreadId());
}

static void EiemPhysicsOrderProbeRememberRender(void *model) {
  InterlockedExchange64(&s_eiemPhysicsOrderLastRenderTick,
                        (LONG64)GetTickCount64());
  InterlockedExchange64(&s_eiemPhysicsOrderLastRenderModel,
                        (LONG64)(uintptr_t)model);
  InterlockedIncrement(&s_eiemPhysicsOrderRenderCount);
  for (uint8_t stage = 0; stage < EiemPhysicsStageCount; ++stage) {
    InterlockedExchange64(
        &s_eiemPhysicsOrderRenderStageSequence[stage],
        InterlockedCompareExchange64(
            &s_eiemPhysicsOrderLastStageSequence[stage], 0, 0));
  }
}

static uint64_t EiemPhysicsOrderStageSequenceSinceRender(uint8_t stage) {
  if (stage >= EiemPhysicsStageCount) return 0;
  const uint64_t current = (uint64_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderLastStageSequence[stage], 0, 0);
  const uint64_t baseline = (uint64_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderRenderStageSequence[stage], 0, 0);
  return current >= baseline ? current - baseline : 0;
}

static uint64_t EiemPhysicsOrderStageLastTick(uint8_t stage) {
  if (stage >= EiemPhysicsStageCount) return 0;
  return (uint64_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderLastStageTick[stage], 0, 0);
}

static void EiemPhysicsOrderProbeLogSinceRender(LONG transaction,
                                                const char *phase) {
  Log("[PHYSICS-ORDER-PROBE-v2] transaction=%ld phase=%s "
      "dataUpdateSinceRender=%llu preSimulationSinceRender=%llu "
      "teamAnimatorSinceRender=%llu completeMasterSinceRender=%llu "
      "lastDataUpdateTick=%llu lastPreSimulationTick=%llu "
      "lastTeamAnimatorTick=%llu lastCompleteMasterTick=%llu",
      transaction, phase ? phase : "unknown",
      (unsigned long long)EiemPhysicsOrderStageSequenceSinceRender(
          EiemPhysicsStageDataUpdate),
      (unsigned long long)EiemPhysicsOrderStageSequenceSinceRender(
          EiemPhysicsStagePreSimulation),
      (unsigned long long)EiemPhysicsOrderStageSequenceSinceRender(
          EiemPhysicsStageTeamAnimator),
      (unsigned long long)EiemPhysicsOrderStageSequenceSinceRender(
          EiemPhysicsStageCompleteMaster),
      (unsigned long long)EiemPhysicsOrderStageLastTick(
          EiemPhysicsStageDataUpdate),
      (unsigned long long)EiemPhysicsOrderStageLastTick(
          EiemPhysicsStagePreSimulation),
      (unsigned long long)EiemPhysicsOrderStageLastTick(
          EiemPhysicsStageTeamAnimator),
      (unsigned long long)EiemPhysicsOrderStageLastTick(
          EiemPhysicsStageCompleteMaster));
}

static uint64_t EiemPhysicsOrderCompletionSequence() {
  return (uint64_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderCompletionSequence, 0, 0);
}

static uint64_t EiemPhysicsOrderLastCompletionTick() {
  return (uint64_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderLastCompletionTick, 0, 0);
}

static uintptr_t EiemPhysicsOrderLastCompletionObject() {
  return (uintptr_t)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderLastCompletionObject, 0, 0);
}

static DWORD EiemPhysicsOrderLastCompletionThread() {
  return (DWORD)InterlockedCompareExchange64(
      &s_eiemPhysicsOrderLastCompletionThread, 0, 0);
}
