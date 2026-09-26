#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "eiem_native_physics_order_probe.h"

// Wrappers record scalar arguments only. No Unity/IL2CPP calls, allocation,
// object dereference or file I/O is allowed on the intercepted thread.
enum class EiemPhysicsOperation : uint8_t {
  BuildAndRun, StartRuntimeBuild, Init, UpdateUse, DataUpdate,
  OnPreSimulationApplyRWBufferCovaryOnce, Dispose, DisposeInternal,
  RemoveMonitoringProcess, CompleteMasterJob, TeamUpdateAnimatorData,
  TeamClearAnimatorData, TeamAddAnimatorData, TeamAddAnimatorTransform,
  TeamMarkAnimatorTransformDirty,
  AnimatorCreateClothBindings, AnimatorCreateClothBindingsByName,
  AnimatorEnableClothBindings, AnimatorDisableClothBindings,
  AnimatorDestroyClothBindings,
  DynamicBoneWriteAnimatorBufferData, DynamicBoneReadAnimatorBufferData,
  DynamicBoneCopyDoubleBuffer, DynamicBoneWriteDoubleBufferTransform, Count
};
static const char *EiemPhysicsOperationName(EiemPhysicsOperation op) {
  const char *names[]={"BeyondBoneCloth.BuildAndRun", "ClothProcess.StartRuntimeBuild",
    "ClothProcess.Init", "ClothProcess.UpdateUse", "ClothProcess.DataUpdate",
    "BeyondBoneCloth.OnPreSimulationApplyRWBufferCovaryOnce",
    "ClothProcess.Dispose", "ClothProcess.DisposeInternal",
    "TeamManager.RemoveMonitoringProcess", "ClothManager.CompleteMasterJob",
    "TeamManager.UpdateTeamAnimatorData", "TeamManager.ClearTeamAnimatorData",
    "TeamManager.AddTeamAnimatorData", "TeamManager.AddAnimatorTransform",
    "TeamManager.MarkAnimatorTransformDirty",
    "Animator.CreateClothBindings_Injected", "Animator.CreateClothBindingsByNameLst_Injected",
    "Animator.EnableClothBindings", "Animator.DisableClothBindings",
    "Animator.DestroyClothBindings",
    "DynamicBoneTransformManager.WriteAnimatorBufferData",
    "DynamicBoneTransformManager.ReadAnimatorBufferData",
    "DynamicBoneTransformManager.CopyDoubleBuffer",
    "DynamicBoneTransformManager.WriteDoubleBufferTransform"};
  return (size_t)op<_countof(names) ? names[(size_t)op] : "unknown";
}
struct EiemPhysicsTraceTicket { uint64_t session=0, call=0; };
struct EiemPhysicsTraceReceipt { uint64_t session=0, sequence=0; };
struct EiemPhysicsCallEvent {
  uint64_t sequence=0, call=0, tick=0;
  uintptr_t object=0, related=0;
  DWORD thread=0;
  EiemPhysicsOperation operation=EiemPhysicsOperation::BuildAndRun;
  uint8_t phase=0; // enter / normal return / unwind without normal return
  int8_t result=-1; // not a Boolean result / false / true
  int32_t bindingCount=-1, invalidBindingCount=-1;
  int32_t teamId=-1;
  uint64_t jobInput=0, jobOutput=0;
};
struct EiemPhysicsTargetAssociation {
  size_t matchedTransforms=0;
  size_t matchedTeams=0;
  uint64_t latestSequence=0;
  int32_t teamIds[16]={};
  uintptr_t clothProcesses[16]={};
};
static constexpr size_t EiemPhysicsTraceCapacity=65536;
static SRWLOCK s_eiemPhysicsEventLock=SRWLOCK_INIT;
static EiemPhysicsCallEvent s_eiemPhysicsEvents[EiemPhysicsTraceCapacity];
static size_t s_eiemPhysicsEventCount=0;
static uint64_t s_eiemPhysicsTraceSession=0, s_eiemPhysicsTraceSequence=0;
static uint64_t s_eiemPhysicsTraceCalls=0, s_eiemPhysicsTraceInFlight=0;
static uint64_t s_eiemPhysicsTraceDropped=0, s_eiemPhysicsTraceSaved=0;
static uint64_t s_eiemPhysicsTraceDeadlineTick=0;
static bool s_eiemPhysicsTraceActive=false;
static volatile LONG s_eiemPhysicsTraceActiveFast=0;

// Match one public Renderer palette against the Transform arguments observed
// at TeamManager.AddAnimatorTransform. This runs only from the bounded Unity
// evidence window; detours merely append scalar call records. A team is
// considered associated only when at least one concrete Transform pointer
// from its animator data is present in the Renderer bones[] array.
static EiemPhysicsTargetAssociation EiemPhysicsAssociateTargetBones(
    void **bones, size_t boneCount) {
  EiemPhysicsTargetAssociation result;
  if (!bones || !boneCount || boneCount > 16384) return result;
  __try {
    AcquireSRWLockShared(&s_eiemPhysicsEventLock);
    for (size_t i=0; i<s_eiemPhysicsEventCount; ++i) {
      const EiemPhysicsCallEvent &transform=s_eiemPhysicsEvents[i];
      if (transform.operation!=EiemPhysicsOperation::TeamAddAnimatorTransform ||
          transform.phase!=1 || transform.teamId<0 || !transform.related)
        continue;
      bool found=false;
      for (size_t b=0; b<boneCount; ++b) {
        if (bones[b]==(void *)transform.related) { found=true; break; }
      }
      if (!found) continue;
      ++result.matchedTransforms;
      if (transform.sequence>result.latestSequence)
        result.latestSequence=transform.sequence;
      size_t teamIndex=SIZE_MAX;
      for (size_t t=0; t<result.matchedTeams; ++t)
        if (result.teamIds[t]==transform.teamId) { teamIndex=t; break; }
      if (teamIndex!=SIZE_MAX || result.matchedTeams>=_countof(result.teamIds))
        continue;
      teamIndex=result.matchedTeams++;
      result.teamIds[teamIndex]=transform.teamId;
      // Search the same trace session for the native ClothProcess registered
      // against this team. The pointer is retained as an address only.
      for (size_t j=0; j<s_eiemPhysicsEventCount; ++j) {
        const EiemPhysicsCallEvent &data=s_eiemPhysicsEvents[j];
        if (data.operation==EiemPhysicsOperation::TeamAddAnimatorData &&
            data.phase==1 && data.teamId==transform.teamId && data.related) {
          result.clothProcesses[teamIndex]=data.related;
          break;
        }
      }
    }
    ReleaseSRWLockShared(&s_eiemPhysicsEventLock);
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    // The trace is observational; a stale managed array must never affect
    // the game thread or leave the shared lock held.
    ReleaseSRWLockShared(&s_eiemPhysicsEventLock);
    result={};
  }
  return result;
}

// A cloth binding is a small native descriptor owned by Unity's Animator.
// Keep a bounded, read-only snapshot of descriptors created during the manual
// window. This is separate from call events so creation arguments and the
// resulting descriptor can be compared without dereferencing a live handle
// during JSON export.
struct EiemPhysicsBindingSnapshot {
  uint64_t sequence=0, tick=0;
  uintptr_t animator=0, handle=0;
  DWORD thread=0;
  EiemPhysicsOperation operation=EiemPhysicsOperation::AnimatorCreateClothBindings;
  uint16_t count=0, invalidCount=0;
  uintptr_t validIndices=0, invalidIndices=0;
  uintptr_t localPositions=0, localRotations=0, localScales=0;
  uintptr_t positions=0, rotations=0, scales=0;
  uintptr_t physicsRatio=0, writeFlag=0;
  uintptr_t relativePosition=0, relativeRotation=0, relativeScale=0;
  int32_t firstValidIndex=INT32_MIN;
  float firstLocalPosition[3]={}, firstLocalRotation[4]={}, firstLocalScale[3]={};
  float firstPosition[3]={}, firstRotation[4]={}, firstScale[3]={};
  float firstPhysicsRatio=0.0f;
  uint8_t firstWriteFlag=0;
  uint8_t readableMask=0;
};
static constexpr size_t EiemPhysicsBindingSnapshotCapacity=2048;
static SRWLOCK s_eiemPhysicsBindingSnapshotLock=SRWLOCK_INIT;
static EiemPhysicsBindingSnapshot s_eiemPhysicsBindingSnapshots[EiemPhysicsBindingSnapshotCapacity];
static size_t s_eiemPhysicsBindingSnapshotCount=0;
static uint64_t s_eiemPhysicsBindingSnapshotSequence=0;

static bool EiemPhysicsReadableRange(const void *address,size_t bytes) {
  if (!address || !bytes) return false;
  MEMORY_BASIC_INFORMATION mbi={};
  if (VirtualQuery(address,&mbi,sizeof(mbi))!=sizeof(mbi) ||
      mbi.State!=MEM_COMMIT || (mbi.Protect&PAGE_GUARD) ||
      (mbi.Protect&(PAGE_NOACCESS|PAGE_EXECUTE)) ||
      (uintptr_t)address > UINTPTR_MAX-bytes) return false;
  const uintptr_t begin=(uintptr_t)address, end=begin+bytes;
  const uintptr_t region=(uintptr_t)mbi.BaseAddress;
  return begin>=region && end<=region+mbi.RegionSize;
}

template<typename T>
static bool EiemPhysicsCopyReadable(T &out,const void *address) {
  if (!EiemPhysicsReadableRange(address,sizeof(T))) return false;
  __try { memcpy(&out,address,sizeof(T)); return true; }
  __except(EXCEPTION_EXECUTE_HANDLER) { memset(&out,0,sizeof(T)); return false; }
}
static bool EiemPhysicsCopyBytes(void *out,const void *address,size_t bytes) {
  if (!out || !EiemPhysicsReadableRange(address,bytes)) return false;
  __try { memcpy(out,address,bytes); return true; }
  __except(EXCEPTION_EXECUTE_HANDLER) { memset(out,0,bytes); return false; }
}

static void EiemPhysicsRecordBindingSnapshot(EiemPhysicsOperation operation,
                                              void *animator,void *handle) {
  EiemPhysicsBindingSnapshot snapshot;
  snapshot.tick=GetTickCount64(); snapshot.thread=GetCurrentThreadId();
  snapshot.animator=(uintptr_t)animator; snapshot.handle=(uintptr_t)handle;
  snapshot.operation=operation;
  if (!handle || !EiemPhysicsReadableRange(handle,0x80)) return;
  EiemPhysicsCopyReadable(snapshot.count,(const char *)handle+0x10);
  EiemPhysicsCopyReadable(snapshot.invalidCount,(const char *)handle+0x12);
  EiemPhysicsCopyReadable(snapshot.validIndices,(const char *)handle+0x18);
  EiemPhysicsCopyReadable(snapshot.invalidIndices,(const char *)handle+0x20);
  EiemPhysicsCopyReadable(snapshot.localPositions,(const char *)handle+0x28);
  EiemPhysicsCopyReadable(snapshot.localRotations,(const char *)handle+0x30);
  EiemPhysicsCopyReadable(snapshot.localScales,(const char *)handle+0x38);
  EiemPhysicsCopyReadable(snapshot.positions,(const char *)handle+0x40);
  EiemPhysicsCopyReadable(snapshot.rotations,(const char *)handle+0x48);
  EiemPhysicsCopyReadable(snapshot.scales,(const char *)handle+0x50);
  EiemPhysicsCopyReadable(snapshot.physicsRatio,(const char *)handle+0x58);
  EiemPhysicsCopyReadable(snapshot.writeFlag,(const char *)handle+0x60);
  EiemPhysicsCopyReadable(snapshot.relativePosition,(const char *)handle+0x68);
  EiemPhysicsCopyReadable(snapshot.relativeRotation,(const char *)handle+0x70);
  EiemPhysicsCopyReadable(snapshot.relativeScale,(const char *)handle+0x78);
  if (snapshot.validIndices && EiemPhysicsReadableRange((void *)snapshot.validIndices,sizeof(int32_t))) {
    snapshot.readableMask|=1;
    EiemPhysicsCopyReadable(snapshot.firstValidIndex,(void *)snapshot.validIndices);
  }
  if (snapshot.localPositions && EiemPhysicsCopyBytes(snapshot.firstLocalPosition,(void *)snapshot.localPositions,sizeof(snapshot.firstLocalPosition))) {
    snapshot.readableMask|=2;
  }
  if (snapshot.localRotations && EiemPhysicsCopyBytes(snapshot.firstLocalRotation,(void *)snapshot.localRotations,sizeof(snapshot.firstLocalRotation))) {
    snapshot.readableMask|=4;
  }
  if (snapshot.localScales && EiemPhysicsCopyBytes(snapshot.firstLocalScale,(void *)snapshot.localScales,sizeof(snapshot.firstLocalScale))) {
    snapshot.readableMask|=8;
  }
  if (snapshot.positions && EiemPhysicsCopyBytes(snapshot.firstPosition,(void *)snapshot.positions,sizeof(snapshot.firstPosition))) {
    snapshot.readableMask|=16;
  }
  if (snapshot.rotations && EiemPhysicsCopyBytes(snapshot.firstRotation,(void *)snapshot.rotations,sizeof(snapshot.firstRotation))) {
    snapshot.readableMask|=32;
  }
  if (snapshot.scales && EiemPhysicsCopyBytes(snapshot.firstScale,(void *)snapshot.scales,sizeof(snapshot.firstScale))) {
    snapshot.readableMask|=64;
  }
  if (snapshot.physicsRatio && EiemPhysicsReadableRange((void *)snapshot.physicsRatio,sizeof(float)))
    EiemPhysicsCopyReadable(snapshot.firstPhysicsRatio,(void *)snapshot.physicsRatio);
  if (snapshot.writeFlag && EiemPhysicsReadableRange((void *)snapshot.writeFlag,sizeof(uint8_t)))
    EiemPhysicsCopyReadable(snapshot.firstWriteFlag,(void *)snapshot.writeFlag);
  AcquireSRWLockExclusive(&s_eiemPhysicsBindingSnapshotLock);
  if (s_eiemPhysicsBindingSnapshotCount==EiemPhysicsBindingSnapshotCapacity) {
    memmove(s_eiemPhysicsBindingSnapshots,s_eiemPhysicsBindingSnapshots+1,
            sizeof(EiemPhysicsBindingSnapshot)*(EiemPhysicsBindingSnapshotCapacity-1));
    --s_eiemPhysicsBindingSnapshotCount;
  }
  snapshot.sequence=++s_eiemPhysicsBindingSnapshotSequence;
  s_eiemPhysicsBindingSnapshots[s_eiemPhysicsBindingSnapshotCount++]=snapshot;
  ReleaseSRWLockExclusive(&s_eiemPhysicsBindingSnapshotLock);
}

static bool EiemPhysicsTraceActive() {
  return InterlockedCompareExchange(&s_eiemPhysicsTraceActiveFast,0,0)!=0;
}
static bool EiemPhysicsBeginTrace(std::string &error,uint32_t durationMs=0) {
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  const char *why=s_eiemPhysicsTraceActive ? "Physics trace already recording" :
    s_eiemPhysicsTraceInFlight ? "Native calls still in flight; retain this trace session" :
    s_eiemPhysicsTraceSaved!=s_eiemPhysicsTraceSequence ? "Export the previous physics trace before starting another" : nullptr;
  if (!why) {
    ++s_eiemPhysicsTraceSession;
    s_eiemPhysicsTraceSequence=s_eiemPhysicsTraceCalls=s_eiemPhysicsTraceDropped=s_eiemPhysicsTraceSaved=0;
    s_eiemPhysicsEventCount=0;
    AcquireSRWLockExclusive(&s_eiemPhysicsBindingSnapshotLock);
    s_eiemPhysicsBindingSnapshotCount=0;
    s_eiemPhysicsBindingSnapshotSequence=0;
    ReleaseSRWLockExclusive(&s_eiemPhysicsBindingSnapshotLock);
    s_eiemPhysicsTraceDeadlineTick=durationMs?GetTickCount64()+durationMs:0;
    s_eiemPhysicsTraceActive=true;
    InterlockedExchange(&s_eiemPhysicsTraceActiveFast,1);
  }
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
  error=why?why:""; return !why;
}
static void EiemPhysicsStopTrace() {
  InterlockedExchange(&s_eiemPhysicsTraceActiveFast,0);
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  s_eiemPhysicsTraceActive=false;
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
}
// Caller holds the lock. A full buffer drops NEW events with explicit counts.
static void EiemPhysicsRecordCall(EiemPhysicsOperation op,void *object,void *related,
                                 uint64_t call,uint8_t phase,int8_t result,
                                 int32_t bindingCount=-1,int32_t invalidBindingCount=-1,
                                 int32_t teamId=-1,uint64_t jobInput=0,
                                 uint64_t jobOutput=0) {
  const uint64_t sequence=++s_eiemPhysicsTraceSequence;
  if (s_eiemPhysicsEventCount==EiemPhysicsTraceCapacity) { ++s_eiemPhysicsTraceDropped; return; }
  auto &event=s_eiemPhysicsEvents[s_eiemPhysicsEventCount++];
  event={sequence,call,GetTickCount64(),(uintptr_t)object,(uintptr_t)related,
         GetCurrentThreadId(),op,phase,result,bindingCount,invalidBindingCount,teamId};
  event.jobInput=jobInput; event.jobOutput=jobOutput;
}
static EiemPhysicsTraceTicket EiemPhysicsTraceEnter(EiemPhysicsOperation op,void *object,void *related=nullptr) {
  EiemPhysicsTraceTicket ticket;
  if (InterlockedCompareExchange(&s_eiemPhysicsTraceActiveFast,0,0)==0)
    return ticket;
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  if (s_eiemPhysicsTraceActive) {
    // The window timer owns export, but a busy Unity message loop can delay
    // WM_TIMER.  Expire the recording at the call boundary as well so native
    // calls become forwarding-only after the requested duration.
    if (s_eiemPhysicsTraceDeadlineTick &&
        GetTickCount64()>=s_eiemPhysicsTraceDeadlineTick) {
      s_eiemPhysicsTraceActive=false;
      InterlockedExchange(&s_eiemPhysicsTraceActiveFast,0);
    } else {
      ticket={s_eiemPhysicsTraceSession,++s_eiemPhysicsTraceCalls};
      ++s_eiemPhysicsTraceInFlight;
      EiemPhysicsRecordCall(op,object,related,ticket.call,0,-1);
    }
  }
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock); return ticket;
}
static void EiemPhysicsTraceLeave(EiemPhysicsTraceTicket ticket,EiemPhysicsOperation op,
                                 void *object,void *related,bool returned,int8_t result=-1,
                                 int32_t bindingCount=-1,int32_t invalidBindingCount=-1,
                                 int32_t teamId=-1,uint64_t jobInput=0,
                                 uint64_t jobOutput=0) {
  // An installed detour outside a manual capture must be a forwarding-only
  // fast path.  In particular, do not update the global ordering ledger for
  // every CompleteMasterJob after the bounded window has stopped.
  if (!ticket.session) return;
  if (returned && op==EiemPhysicsOperation::CompleteMasterJob)
    EiemPhysicsOrderProbeMarkCompletion(object);
  if (returned) {
    switch (op) {
      case EiemPhysicsOperation::DataUpdate:
        EiemPhysicsOrderProbeMarkStage(EiemPhysicsStageDataUpdate, object);
        break;
      case EiemPhysicsOperation::OnPreSimulationApplyRWBufferCovaryOnce:
        EiemPhysicsOrderProbeMarkStage(EiemPhysicsStagePreSimulation, object);
        break;
      case EiemPhysicsOperation::TeamUpdateAnimatorData:
        EiemPhysicsOrderProbeMarkStage(EiemPhysicsStageTeamAnimator, object);
        break;
      default:
        break;
    }
  }
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  // Stop does not discard the return of a call that entered this session.
  if (ticket.session==s_eiemPhysicsTraceSession) {
    EiemPhysicsRecordCall(op,object,related,ticket.call,returned?1:2,returned?result:-1,
                          returned?bindingCount:-1,returned?invalidBindingCount:-1,
                          teamId,jobInput,jobOutput);
    --s_eiemPhysicsTraceInFlight;
  }
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
}
static void EiemPhysicsTraceExported(const EiemPhysicsTraceReceipt &receipt) {
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  if (receipt.session==s_eiemPhysicsTraceSession && receipt.sequence>s_eiemPhysicsTraceSaved &&
      receipt.sequence<=s_eiemPhysicsTraceSequence) s_eiemPhysicsTraceSaved=receipt.sequence;
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
}
static void EiemPhysicsWriteFloatArray(FILE *file,const float *values,size_t count) {
  fputc('[',file);
  for (size_t i=0;i<count;++i) {
    if (i) fputc(',',file);
    fprintf(file,"%.9g",(double)values[i]);
  }
  fputc(']',file);
}
static void EiemPhysicsWriteBindingSnapshots(FILE *file) {
  if (!file) return;
  std::vector<EiemPhysicsBindingSnapshot> snapshots;
  AcquireSRWLockShared(&s_eiemPhysicsBindingSnapshotLock);
  snapshots.insert(snapshots.end(),s_eiemPhysicsBindingSnapshots,
                   s_eiemPhysicsBindingSnapshots+s_eiemPhysicsBindingSnapshotCount);
  ReleaseSRWLockShared(&s_eiemPhysicsBindingSnapshotLock);
  fputs(",\"bindingSnapshots\":[",file);
  for (size_t i=0;i<snapshots.size();++i) {
    const auto &s=snapshots[i]; if (i) fputc(',',file);
    fprintf(file,"{\"seq\":%llu,\"tick\":%llu,\"thread\":%lu,"
      "\"operation\":\"%s\",\"animator\":\"%p\",\"handle\":\"%p\","
      "\"count\":%u,\"invalidCount\":%u,\"validIndices\":\"%p\","
      "\"localPositions\":\"%p\",\"localRotations\":\"%p\",\"localScales\":\"%p\","
      "\"positions\":\"%p\",\"rotations\":\"%p\",\"scales\":\"%p\","
      "\"physicsRatio\":\"%p\",\"writeFlag\":\"%p\","
      "\"relativePosition\":\"%p\",\"relativeRotation\":\"%p\",\"relativeScale\":\"%p\","
      "\"firstValidIndex\":%d,\"readableMask\":%u,\"firstLocalPosition\":",
      (unsigned long long)s.sequence,(unsigned long long)s.tick,s.thread,
      EiemPhysicsOperationName(s.operation),(void *)s.animator,(void *)s.handle,
      (unsigned)s.count,(unsigned)s.invalidCount,(void *)s.validIndices,
      (void *)s.localPositions,(void *)s.localRotations,(void *)s.localScales,
      (void *)s.positions,(void *)s.rotations,(void *)s.scales,
      (void *)s.physicsRatio,(void *)s.writeFlag,
      (void *)s.relativePosition,(void *)s.relativeRotation,(void *)s.relativeScale,
      s.firstValidIndex,(unsigned)s.readableMask);
    EiemPhysicsWriteFloatArray(file,s.firstLocalPosition,3);
    fputs(",\"firstLocalRotation\":",file); EiemPhysicsWriteFloatArray(file,s.firstLocalRotation,4);
    fputs(",\"firstLocalScale\":",file); EiemPhysicsWriteFloatArray(file,s.firstLocalScale,3);
    fputs(",\"firstPosition\":",file); EiemPhysicsWriteFloatArray(file,s.firstPosition,3);
    fputs(",\"firstRotation\":",file); EiemPhysicsWriteFloatArray(file,s.firstRotation,4);
    fputs(",\"firstScale\":",file); EiemPhysicsWriteFloatArray(file,s.firstScale,3);
    fprintf(file,",\"firstPhysicsRatio\":%.9g,\"firstWriteFlag\":%u}",
      (double)s.firstPhysicsRatio,(unsigned)s.firstWriteFlag);
  }
  fputc(']',file);
}
static bool EiemWritePhysicsCallTrace(FILE *file,EiemPhysicsTraceReceipt *receipt=nullptr) {
  if (!file) return false;
  // Reserve before locking; hooked threads never wait on filesystem I/O or
  // heap allocation. The bounded copy is the only work in the shared section.
  std::vector<EiemPhysicsCallEvent> events; events.reserve(EiemPhysicsTraceCapacity);
  uint64_t session=0,sequence=0,dropped=0,inFlight=0; bool active=false;
  AcquireSRWLockShared(&s_eiemPhysicsEventLock);
  events.insert(events.end(),s_eiemPhysicsEvents,s_eiemPhysicsEvents+s_eiemPhysicsEventCount);
  session=s_eiemPhysicsTraceSession; sequence=s_eiemPhysicsTraceSequence;
  dropped=s_eiemPhysicsTraceDropped; inFlight=s_eiemPhysicsTraceInFlight; active=s_eiemPhysicsTraceActive;
  ReleaseSRWLockShared(&s_eiemPhysicsEventLock);
  fprintf(file,"{\"session\":%llu,\"pid\":%lu,\"recording\":%s,\"sequence\":%llu,"
    "\"dropped\":%llu,\"inFlightCalls\":%llu,\"capacity\":%zu,"
    "\"proof\":\"observed-calls-only-not-async-task-or-job-quiescence\","
    "\"objectIdentity\":\"raw-call-address-may-be-reused-not-a-stable-instance-id\",\"events\":[",
    (unsigned long long)session,GetCurrentProcessId(),active?"true":"false",(unsigned long long)sequence,
    (unsigned long long)dropped,(unsigned long long)inFlight,EiemPhysicsTraceCapacity);
  for (size_t i=0;i<events.size();++i) {
    const auto &e=events[i]; if (i) fputc(',',file);
    fprintf(file,"{\"seq\":%llu,\"call\":%llu,\"tick\":%llu,\"thread\":%lu,\"operation\":\"%s\","
      "\"phase\":\"%s\",\"object\":\"%p\",\"related\":\"%p\",\"result\":",
      (unsigned long long)e.sequence,(unsigned long long)e.call,(unsigned long long)e.tick,e.thread,
      EiemPhysicsOperationName(e.operation),e.phase==0?"enter":e.phase==1?"return":"unwind",
      (void *)e.object,(void *)e.related);
    fputs(e.result<0?"null":e.result?"true":"false",file);
    fputs(",\"bindingCount\":",file);
    if (e.bindingCount<0) fputs("null",file); else fprintf(file,"%d",e.bindingCount);
    fputs(",\"invalidBindingCount\":",file);
    if (e.invalidBindingCount<0) fputs("null",file); else fprintf(file,"%d",e.invalidBindingCount);
    fputs(",\"teamId\":",file);
    if (e.teamId<0) fputs("null",file); else fprintf(file,"%d",e.teamId);
    fprintf(file,",\"jobInput\":\"%016llX\",\"jobOutput\":\"%016llX\"",
      (unsigned long long)e.jobInput,(unsigned long long)e.jobOutput);
    fputc('}',file);
  }
  // Close the lifecycle event array before appending the independent
  // descriptor snapshot array.
  fputc(']',file);
  EiemPhysicsWriteBindingSnapshots(file);
  fputc('}',file);
  if (receipt) *receipt={session,sequence};
  return !ferror(file);
}
