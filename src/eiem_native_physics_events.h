#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Wrappers record scalar arguments only. No Unity/IL2CPP calls, allocation,
// object dereference or file I/O is allowed on the intercepted thread.
enum class EiemPhysicsOperation : uint8_t {
  BuildAndRun, StartRuntimeBuild, Init, Dispose, DisposeInternal,
  RemoveMonitoringProcess, CompleteMasterJob, TeamUpdateAnimatorData,
  TeamClearAnimatorData, TeamAddAnimatorData, TeamAddAnimatorTransform,
  TeamMarkAnimatorTransformDirty,
  AnimatorCreateClothBindings, AnimatorCreateClothBindingsByName,
  AnimatorEnableClothBindings, AnimatorDisableClothBindings,
  AnimatorDestroyClothBindings, Count
};
static const char *EiemPhysicsOperationName(EiemPhysicsOperation op) {
  const char *names[]={"BeyondBoneCloth.BuildAndRun", "ClothProcess.StartRuntimeBuild",
    "ClothProcess.Init", "ClothProcess.Dispose", "ClothProcess.DisposeInternal",
    "TeamManager.RemoveMonitoringProcess", "ClothManager.CompleteMasterJob",
    "TeamManager.UpdateTeamAnimatorData", "TeamManager.ClearTeamAnimatorData",
    "TeamManager.AddTeamAnimatorData", "TeamManager.AddAnimatorTransform",
    "TeamManager.MarkAnimatorTransformDirty",
    "Animator.CreateClothBindings_Injected", "Animator.CreateClothBindingsByNameLst_Injected",
    "Animator.EnableClothBindings", "Animator.DisableClothBindings",
    "Animator.DestroyClothBindings"};
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
};
static constexpr size_t EiemPhysicsTraceCapacity=65536;
static SRWLOCK s_eiemPhysicsEventLock=SRWLOCK_INIT;
static EiemPhysicsCallEvent s_eiemPhysicsEvents[EiemPhysicsTraceCapacity];
static size_t s_eiemPhysicsEventCount=0;
static uint64_t s_eiemPhysicsTraceSession=0, s_eiemPhysicsTraceSequence=0;
static uint64_t s_eiemPhysicsTraceCalls=0, s_eiemPhysicsTraceInFlight=0;
static uint64_t s_eiemPhysicsTraceDropped=0, s_eiemPhysicsTraceSaved=0;
static bool s_eiemPhysicsTraceActive=false;

static bool EiemPhysicsTraceActive() {
  AcquireSRWLockShared(&s_eiemPhysicsEventLock);
  const bool active=s_eiemPhysicsTraceActive;
  ReleaseSRWLockShared(&s_eiemPhysicsEventLock); return active;
}
static bool EiemPhysicsBeginTrace(std::string &error) {
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  const char *why=s_eiemPhysicsTraceActive ? "Physics trace already recording" :
    s_eiemPhysicsTraceInFlight ? "Native calls still in flight; retain this trace session" :
    s_eiemPhysicsTraceSaved!=s_eiemPhysicsTraceSequence ? "Export the previous physics trace before starting another" : nullptr;
  if (!why) {
    ++s_eiemPhysicsTraceSession;
    s_eiemPhysicsTraceSequence=s_eiemPhysicsTraceCalls=s_eiemPhysicsTraceDropped=s_eiemPhysicsTraceSaved=0;
    s_eiemPhysicsEventCount=0; s_eiemPhysicsTraceActive=true;
  }
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
  error=why?why:""; return !why;
}
static void EiemPhysicsStopTrace() {
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  s_eiemPhysicsTraceActive=false;
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock);
}
// Caller holds the lock. A full buffer drops NEW events with explicit counts.
static void EiemPhysicsRecordCall(EiemPhysicsOperation op,void *object,void *related,
                                 uint64_t call,uint8_t phase,int8_t result,
                                 int32_t bindingCount=-1,int32_t invalidBindingCount=-1,
                                 int32_t teamId=-1) {
  const uint64_t sequence=++s_eiemPhysicsTraceSequence;
  if (s_eiemPhysicsEventCount==EiemPhysicsTraceCapacity) { ++s_eiemPhysicsTraceDropped; return; }
  auto &event=s_eiemPhysicsEvents[s_eiemPhysicsEventCount++];
  event={sequence,call,GetTickCount64(),(uintptr_t)object,(uintptr_t)related,
         GetCurrentThreadId(),op,phase,result,bindingCount,invalidBindingCount,teamId};
}
static EiemPhysicsTraceTicket EiemPhysicsTraceEnter(EiemPhysicsOperation op,void *object,void *related=nullptr) {
  EiemPhysicsTraceTicket ticket;
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  if (s_eiemPhysicsTraceActive) {
    ticket={s_eiemPhysicsTraceSession,++s_eiemPhysicsTraceCalls};
    ++s_eiemPhysicsTraceInFlight;
    EiemPhysicsRecordCall(op,object,related,ticket.call,0,-1);
  }
  ReleaseSRWLockExclusive(&s_eiemPhysicsEventLock); return ticket;
}
static void EiemPhysicsTraceLeave(EiemPhysicsTraceTicket ticket,EiemPhysicsOperation op,
                                 void *object,void *related,bool returned,int8_t result=-1,
                                 int32_t bindingCount=-1,int32_t invalidBindingCount=-1,
                                 int32_t teamId=-1) {
  if (!ticket.session) return;
  AcquireSRWLockExclusive(&s_eiemPhysicsEventLock);
  // Stop does not discard the return of a call that entered this session.
  if (ticket.session==s_eiemPhysicsTraceSession) {
    EiemPhysicsRecordCall(op,object,related,ticket.call,returned?1:2,returned?result:-1,
                          returned?bindingCount:-1,returned?invalidBindingCount:-1,
                          teamId);
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
    fputc('}',file);
  }
  fputs("]}",file);
  if (receipt) *receipt={session,sequence};
  return !ferror(file);
}
