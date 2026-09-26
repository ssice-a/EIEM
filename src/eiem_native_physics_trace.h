#pragma once
#include "eiem_native_physics_api.h"
#include "eiem_native_physics_events.h"

// Optional observation wrappers. The original native call runs exactly once;
// its return and exceptions are preserved. No callbacks invoke Unity APIs.
using EiemPhysicsBool0=bool (*)(void *,void *);
using EiemPhysicsVoid0=void (*)(void *,void *);
using EiemPhysicsVoid1=void (*)(void *,void *,void *);
using EiemPhysicsVoid2=void (*)(void *,void *,void *,void *);
using EiemPhysicsVoidInt=void (*)(void *,int32_t,void *);
using EiemPhysicsVoidIntObject=void (*)(void *,int32_t,void *,void *);
using EiemPhysicsIcallCreate=void (*)(void *,void *,void *);
using EiemPhysicsIcallVoid0=void (*)(void *);
// Unity.Jobs.JobHandle is two 32-bit fields in this build.  Keep it as a
// value type so the detour preserves the original ABI and only records the
// dependency/result token returned by the game's scheduler.
struct EiemPhysicsJobHandle { uint32_t handle=0, version=0; };
static uint64_t EiemPhysicsPackJobHandle(EiemPhysicsJobHandle value) {
  return (uint64_t)value.handle | ((uint64_t)value.version<<32);
}
using EiemPhysicsJobBuffer4=EiemPhysicsJobHandle (*)(
    void *,EiemPhysicsJobHandle,void *,void *,void *,void *);
using EiemPhysicsJobBuffer1=EiemPhysicsJobHandle (*)(
    void *,EiemPhysicsJobHandle,void *);
static void *s_eiemPhysicsTraceOriginal[(size_t)EiemPhysicsOperation::Count]={};

template<EiemPhysicsOperation Op>
static bool EiemPhysicsTraceBool(void *object,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,object);
  bool result=false,returned=false;
  __try {
    result=((EiemPhysicsBool0)s_eiemPhysicsTraceOriginal[(size_t)Op])(object,method);
    returned=true;
  } __finally { EiemPhysicsTraceLeave(ticket,Op,object,nullptr,returned,result?1:0); }
  return result;
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceVoid(void *object,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,object);
  bool returned=false;
  __try { ((EiemPhysicsVoid0)s_eiemPhysicsTraceOriginal[(size_t)Op])(object,method); returned=true; }
  __finally { EiemPhysicsTraceLeave(ticket,Op,object,nullptr,returned); }
}
static void EiemPhysicsTraceRemoveMonitoring(void *object,void *process,void *method) {
  constexpr auto op=EiemPhysicsOperation::RemoveMonitoringProcess;
  const auto ticket=EiemPhysicsTraceEnter(op,object,process);
  bool returned=false;
  __try { ((EiemPhysicsVoid1)s_eiemPhysicsTraceOriginal[(size_t)op])(object,process,method); returned=true; }
  __finally { EiemPhysicsTraceLeave(ticket,op,object,process,returned); }
}
static void EiemPhysicsTraceUpdateAnimatorData(void *object,void *teamIds,
                                               void *transforms,void *method) {
  constexpr auto op=EiemPhysicsOperation::TeamUpdateAnimatorData;
  const auto ticket=EiemPhysicsTraceEnter(op,object,transforms);
  bool returned=false;
  __try {
    ((EiemPhysicsVoid2)s_eiemPhysicsTraceOriginal[(size_t)op])(
        object,teamIds,transforms,method);
    returned=true;
  } __finally { EiemPhysicsTraceLeave(ticket,op,object,transforms,returned); }
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceTeamId(void *object,int32_t teamId,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,object);
  bool returned=false;
  __try {
    ((EiemPhysicsVoidInt)s_eiemPhysicsTraceOriginal[(size_t)Op])(
        object,teamId,method);
    returned=true;
  } __finally {
    EiemPhysicsTraceLeave(ticket,Op,object,nullptr,returned,-1,-1,-1,teamId);
  }
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceTeamObject(void *object,int32_t teamId,
                                       void *related,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,object,related);
  bool returned=false;
  __try {
    ((EiemPhysicsVoidIntObject)s_eiemPhysicsTraceOriginal[(size_t)Op])(
        object,teamId,related,method);
    returned=true;
  } __finally {
    EiemPhysicsTraceLeave(ticket,Op,object,related,returned,-1,-1,-1,teamId);
  }
}
static void EiemPhysicsReadBindingCounts(void *handle,int32_t &count,int32_t &invalidCount) {
  count=invalidCount=-1;
  if (!handle) return;
  __try {
    const auto *values=(const uint16_t *)handle;
    count=values[0]; invalidCount=values[1];
  } __except(EXCEPTION_EXECUTE_HANDLER) { count=invalidCount=-1; }
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceIcallCreate(void *animator,void *values,void *handle) {
  const auto ticket=EiemPhysicsTraceEnter(Op,animator,values);
  bool returned=false; int32_t count=-1,invalidCount=-1;
  __try {
    ((EiemPhysicsIcallCreate)s_eiemPhysicsTraceOriginal[(size_t)Op])(animator,values,handle);
    returned=true;
    // Descriptor dereferences and snapshot locking are bounded by the manual
    // capture.  Outside it this detour only forwards to the original icall.
    if (ticket.session) {
      EiemPhysicsReadBindingCounts(handle,count,invalidCount);
      EiemPhysicsRecordBindingSnapshot(Op,animator,handle);
    }
  } __finally { EiemPhysicsTraceLeave(ticket,Op,animator,values,returned,-1,count,invalidCount); }
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceIcallVoid(void *animator) {
  const auto ticket=EiemPhysicsTraceEnter(Op,animator);
  bool returned=false;
  __try { ((EiemPhysicsIcallVoid0)s_eiemPhysicsTraceOriginal[(size_t)Op])(animator); returned=true; }
  __finally { EiemPhysicsTraceLeave(ticket,Op,animator,nullptr,returned); }
}
template<EiemPhysicsOperation Op>
static EiemPhysicsJobHandle EiemPhysicsTraceJobBuffer4(
    void *manager,EiemPhysicsJobHandle dependency,void *teamMap,
    void *animatorMap,void *transformMap,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,manager,animatorMap);
  EiemPhysicsJobHandle result{}; bool returned=false;
  __try {
    result=((EiemPhysicsJobBuffer4)s_eiemPhysicsTraceOriginal[(size_t)Op])(
      manager,dependency,teamMap,animatorMap,transformMap,method);
    returned=true;
  } __finally {
    EiemPhysicsTraceLeave(ticket,Op,manager,animatorMap,returned,-1,-1,-1,-1,
      EiemPhysicsPackJobHandle(dependency),EiemPhysicsPackJobHandle(result));
  }
  return result;
}
template<EiemPhysicsOperation Op>
static EiemPhysicsJobHandle EiemPhysicsTraceJobBuffer1(
    void *manager,EiemPhysicsJobHandle dependency,void *method) {
  const auto ticket=EiemPhysicsTraceEnter(Op,manager);
  EiemPhysicsJobHandle result{}; bool returned=false;
  __try {
    result=((EiemPhysicsJobBuffer1)s_eiemPhysicsTraceOriginal[(size_t)Op])(
      manager,dependency,method);
    returned=true;
  } __finally {
    EiemPhysicsTraceLeave(ticket,Op,manager,nullptr,returned,-1,-1,-1,-1,
      EiemPhysicsPackJobHandle(dependency),EiemPhysicsPackJobHandle(result));
  }
  return result;
}
struct EiemPhysicsTraceHook {
  const char *klass,*name,*returns,*parameter0,*parameter1;
  EiemPhysicsOperation operation;
  void *detour,*target=nullptr;
  bool created=false,enabled=false;
};
static EiemPhysicsTraceHook s_eiemPhysicsTraceHooks[]={
  {"ClothManager","CompleteMasterJob","System.Void",nullptr,nullptr,
   EiemPhysicsOperation::CompleteMasterJob,(void *)EiemPhysicsTraceVoid<EiemPhysicsOperation::CompleteMasterJob>},
  {"TeamManager","AddTeamAnimatorData","System.Void","System.Int32","BeyondDynamicBone.ClothProcess",
   EiemPhysicsOperation::TeamAddAnimatorData,(void *)EiemPhysicsTraceTeamObject<EiemPhysicsOperation::TeamAddAnimatorData>},
  {"TeamManager","AddAnimatorTransform","System.Void","System.Int32","UnityEngine.Transform",
   EiemPhysicsOperation::TeamAddAnimatorTransform,(void *)EiemPhysicsTraceTeamObject<EiemPhysicsOperation::TeamAddAnimatorTransform>}
};
struct EiemPhysicsIcallTraceHook {
  const char *name,*parameter0,*parameter1;
  EiemPhysicsOperation operation;
  void *detour,*target=nullptr;
  bool created=false,enabled=false;
};
static EiemPhysicsIcallTraceHook s_eiemPhysicsIcallTraceHooks[]={
  {"CreateClothBindings_Injected","UnityEngine.Transform[]","UnityEngine.AnimationTransformRWBufferHandle&",
   EiemPhysicsOperation::AnimatorCreateClothBindings,(void *)EiemPhysicsTraceIcallCreate<EiemPhysicsOperation::AnimatorCreateClothBindings>},
  {"CreateClothBindingsByNameLst_Injected","System.String[]","UnityEngine.AnimationTransformRWBufferHandle&",
   EiemPhysicsOperation::AnimatorCreateClothBindingsByName,(void *)EiemPhysicsTraceIcallCreate<EiemPhysicsOperation::AnimatorCreateClothBindingsByName>}
};
struct EiemPhysicsJobTraceHook {
  const char *name,*returns;
  bool hasMaps;
  const char *parameter0,*parameter1,*parameter2,*parameter3;
  EiemPhysicsOperation operation;
  void *detour,*target=nullptr;
  bool created=false,enabled=false;
};
static EiemPhysicsJobTraceHook s_eiemPhysicsJobTraceHooks[]={
  {"WriteAnimatorBufferData","Unity.Jobs.JobHandle",true,
   "Unity.Jobs.JobHandle",
   "Unity.Collections.NativeParallelHashMap<System.Int32,System.Int32>&",
   "Unity.Collections.NativeParallelHashMap<System.Int32,UnityEngine.AnimationTransformRWBufferHandle>&",
   "Unity.Collections.NativeParallelHashMap<System.Int32,System.Int32>&",
   EiemPhysicsOperation::DynamicBoneWriteAnimatorBufferData,
   (void *)EiemPhysicsTraceJobBuffer4<EiemPhysicsOperation::DynamicBoneWriteAnimatorBufferData>},
  {"ReadAnimatorBufferData","Unity.Jobs.JobHandle",true,
   "Unity.Jobs.JobHandle",
   "Unity.Collections.NativeParallelHashMap<System.Int32,System.Int32>&",
   "Unity.Collections.NativeParallelHashMap<System.Int32,UnityEngine.AnimationTransformRWBufferHandle>&",
   "Unity.Collections.NativeParallelHashMap<System.Int32,System.Int32>&",
   EiemPhysicsOperation::DynamicBoneReadAnimatorBufferData,
   (void *)EiemPhysicsTraceJobBuffer4<EiemPhysicsOperation::DynamicBoneReadAnimatorBufferData>},
  {"CopyDoubleBuffer","Unity.Jobs.JobHandle",false,
   "Unity.Jobs.JobHandle",nullptr,nullptr,nullptr,
   EiemPhysicsOperation::DynamicBoneCopyDoubleBuffer,
   (void *)EiemPhysicsTraceJobBuffer1<EiemPhysicsOperation::DynamicBoneCopyDoubleBuffer>},
  {"WriteDoubleBufferTransform","Unity.Jobs.JobHandle",false,
   "Unity.Jobs.JobHandle",nullptr,nullptr,nullptr,
   EiemPhysicsOperation::DynamicBoneWriteDoubleBufferTransform,
   (void *)EiemPhysicsTraceJobBuffer1<EiemPhysicsOperation::DynamicBoneWriteDoubleBufferTransform>}
};
// The operation enum remains a stable JSON schema, while the installed arrays
// intentionally contain only the nine boundaries needed by the focused run.

static void *EiemPhysicsTraceAddress(void *method) {
  if (!method) return nullptr;
  __try { return ((MInfo *)method)->mp; }
  __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
static void *EiemPhysicsTraceResolveIcall(const char *name) {
  if (!name || !il2cpp_resolve_icall) return nullptr;
  char full[256]={};
  if (snprintf(full,sizeof(full),"UnityEngine.Animator::%s",name)<0) return nullptr;
  __try { return il2cpp_resolve_icall(full); }
  __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
static bool EiemPhysicsTraceExecutable(void *address) {
  MEMORY_BASIC_INFORMATION memory={};
  if (!address || VirtualQuery(address,&memory,sizeof(memory))!=sizeof(memory) ||
      memory.State!=MEM_COMMIT || (memory.Protect&PAGE_GUARD)) return false;
  const DWORD protection=memory.Protect&0xff;
  return protection==PAGE_EXECUTE || protection==PAGE_EXECUTE_READ ||
         protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY;
}
static bool EiemInstallPhysicsTrace(void **assemblies,size_t count,std::string &error) {
  if (!EiemOnUnityThread() || !assemblies || !count ||
      !il2cpp_assembly_get_image || !il2cpp_image_get_name || !il2cpp_class_from_name ||
      !il2cpp_class_get_methods || !il2cpp_method_get_name || !il2cpp_method_get_param_count ||
      !il2cpp_method_get_flags || !il2cpp_method_get_return_type || !il2cpp_method_get_param ||
      !il2cpp_type_get_name || !il2cpp_free || !il2cpp_image_get_class_count || !il2cpp_image_get_class ||
      !il2cpp_resolve_icall) {
    error="Physics trace requires Unity thread and complete metadata APIs"; return false;
  }
  void *methods[_countof(s_eiemPhysicsTraceHooks)]={};
  void *targets[_countof(s_eiemPhysicsTraceHooks)]={};
  for (size_t i=0;i<_countof(s_eiemPhysicsTraceHooks);++i) {
    const auto &hook=s_eiemPhysicsTraceHooks[i];
    void *klass=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone",hook.klass);
    methods[i]=hook.parameter1 ?
      EiemPhysicsMethod(klass,hook.name,hook.returns,false,hook.parameter0,hook.parameter1) :
      hook.parameter0 ? EiemPhysicsMethod(klass,hook.name,hook.returns,false,hook.parameter0) :
                        EiemPhysicsMethod(klass,hook.name,hook.returns,false);
    targets[i]=EiemPhysicsTraceAddress(methods[i]);
    if (!targets[i]) { error=std::string("Physics trace signature/address unavailable: ")+hook.klass+"."+hook.name; return false; }
    if (hook.created && hook.target!=targets[i]) { error="Physics trace target changed after installation"; return false; }
    for (size_t j=0;j<i;++j) if (targets[i]==targets[j]) {
      error="Physics trace methods share an implementation; refusing ambiguous hook"; return false;
    }
  }
  void *icallMethods[_countof(s_eiemPhysicsIcallTraceHooks)]={};
  void *icallTargets[_countof(s_eiemPhysicsIcallTraceHooks)]={};
  void *animator=EiemPhysicsClass(assemblies,count,EiemPhysicsAnimatorImage,"UnityEngine","Animator");
  for (size_t i=0;i<_countof(s_eiemPhysicsIcallTraceHooks);++i) {
    auto &hook=s_eiemPhysicsIcallTraceHooks[i];
    icallMethods[i]=hook.parameter0 ?
      EiemPhysicsMethod(animator,hook.name,"System.Void",false,hook.parameter0,hook.parameter1) :
      EiemPhysicsMethod(animator,hook.name,"System.Void",false);
    uint32_t implementation=0;
    if (!icallMethods[i] || !(il2cpp_method_get_flags(icallMethods[i],&implementation),implementation&0x1000)) {
      error=std::string("Physics Animator icall signature unavailable: ")+hook.name; return false;
    }
    icallTargets[i]=EiemPhysicsTraceResolveIcall(hook.name);
    if (!EiemPhysicsTraceExecutable(icallTargets[i])) {
      error=std::string("Physics Animator icall target unavailable/non-executable: ")+hook.name; return false;
    }
    if (hook.created && hook.target!=icallTargets[i]) { error="Physics Animator icall target changed after installation"; return false; }
    for (size_t j=0;j<i;++j) if (icallTargets[i]==icallTargets[j]) {
      error="Physics Animator icalls share a target; refusing ambiguous hook"; return false;
    }
    for (void *target:targets) if (icallTargets[i]==target) {
      error="Physics managed method and Animator icall share a target"; return false;
    }
  }
  void *jobMethods[_countof(s_eiemPhysicsJobTraceHooks)]={};
  void *jobTargets[_countof(s_eiemPhysicsJobTraceHooks)]={};
  void *transformManager=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll",
                                          "BeyondDynamicBone","DynamicBoneTransformManager");
  if (!transformManager) {
    error="DynamicBoneTransformManager class unavailable"; return false;
  }
  for (size_t i=0;i<_countof(s_eiemPhysicsJobTraceHooks);++i) {
    auto &hook=s_eiemPhysicsJobTraceHooks[i];
    jobMethods[i]=hook.hasMaps ?
      EiemPhysicsMethod(transformManager,hook.name,hook.returns,false,
                        hook.parameter0,hook.parameter1,hook.parameter2,hook.parameter3) :
      EiemPhysicsMethod(transformManager,hook.name,hook.returns,false,hook.parameter0);
    jobTargets[i]=EiemPhysicsTraceAddress(jobMethods[i]);
    if (!jobTargets[i]) {
      error=std::string("DynamicBone buffer signature/address unavailable: ")+hook.name;
      return false;
    }
    if (hook.created && hook.target!=jobTargets[i]) {
      error="DynamicBone buffer target changed after installation"; return false;
    }
    for (size_t j=0;j<i;++j) if (jobTargets[i]==jobTargets[j]) {
      error="DynamicBone buffer methods share an implementation"; return false;
    }
    for (void *target:targets) if (jobTargets[i]==target) {
      error="DynamicBone buffer method shares a managed target"; return false;
    }
    for (void *target:icallTargets) if (jobTargets[i]==target) {
      error="DynamicBone buffer method shares an Animator icall target"; return false;
    }
  }
  // Even a different method outside our list may share folded machine code.
  // Refuse such entries instead of calling it the wrong domain operation.
  for (size_t a=0;a<count;++a) {
    void *image=il2cpp_assembly_get_image(assemblies[a]);
    const char *name=image?il2cpp_image_get_name(image):nullptr;
    if (!image) continue;
    for (size_t c=0;c<il2cpp_image_get_class_count(image);++c) {
      void *klass=il2cpp_image_get_class(image,c),*iterator=nullptr,*method=nullptr;
      if (!klass) continue;
      while ((method=il2cpp_class_get_methods(klass,&iterator))) {
        void *address=EiemPhysicsTraceAddress(method);
        for (size_t i=0;i<_countof(targets);++i) if (address==targets[i] && method!=methods[i]) {
          const char *methodName=il2cpp_method_get_name(method);
          error=std::string("Physics trace implementation also used by: ")+(name?name:"<unknown-image>")+"/"+
            (methodName?methodName:"<unnamed-method>"); return false;
        }
      }
    }
  }
  // Install without holding the event lock: MinHook may suspend a recording
  // thread. Trampolines remain resident after Stop or a partial install error.
  for (size_t i=0;i<_countof(s_eiemPhysicsTraceHooks);++i) {
    auto &hook=s_eiemPhysicsTraceHooks[i];
    if (!hook.created) {
      const auto status=MH_CreateHook(
        targets[i],hook.detour,
        &s_eiemPhysicsTraceOriginal[(size_t)hook.operation]);
      if (status!=MH_OK) {
        error=std::string("Cannot create physics trace hook: ")+hook.name+" status="+std::to_string((int)status); return false;
      }
      hook.target=targets[i]; hook.created=true;
    }
  }
  for (size_t i=0;i<_countof(s_eiemPhysicsIcallTraceHooks);++i) {
    auto &hook=s_eiemPhysicsIcallTraceHooks[i];
    if (!hook.created) {
      const auto status=MH_CreateHook(icallTargets[i],hook.detour,&s_eiemPhysicsTraceOriginal[(size_t)hook.operation]);
      if (status!=MH_OK) {
        error=std::string("Cannot create Physics Animator icall hook: ")+hook.name+" status="+std::to_string((int)status); return false;
      }
      hook.target=icallTargets[i]; hook.created=true;
    }
  }
  for (size_t i=0;i<_countof(s_eiemPhysicsJobTraceHooks);++i) {
    auto &hook=s_eiemPhysicsJobTraceHooks[i];
    if (!hook.created) {
      const auto status=MH_CreateHook(jobTargets[i],hook.detour,
        &s_eiemPhysicsTraceOriginal[(size_t)hook.operation]);
      if (status!=MH_OK) {
        error=std::string("Cannot create DynamicBone buffer hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.target=jobTargets[i]; hook.created=true;
    }
  }
  for (auto &hook:s_eiemPhysicsTraceHooks) {
    if (!hook.enabled) {
      const auto status=MH_EnableHook(hook.target);
      if (status!=MH_OK) {
        error=std::string("Cannot enable physics trace hook: ")+hook.name+" status="+std::to_string((int)status); return false;
      }
      hook.enabled=true;
    }
  }
  for (auto &hook:s_eiemPhysicsIcallTraceHooks) {
    if (!hook.enabled) {
      const auto status=MH_EnableHook(hook.target);
      if (status!=MH_OK) {
        error=std::string("Cannot enable Physics Animator icall hook: ")+hook.name+" status="+std::to_string((int)status); return false;
      }
      hook.enabled=true;
    }
  }
  for (auto &hook:s_eiemPhysicsJobTraceHooks) {
    if (!hook.enabled) {
      const auto status=MH_EnableHook(hook.target);
      if (status!=MH_OK) {
        error=std::string("Cannot enable DynamicBone buffer hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.enabled=true;
    }
  }
  error.clear(); return true;
}
// Optional helper retained for dedicated startup-binding experiments.  The
// normal diagnostic path no longer calls it: F12 installs the focused hook set
// on demand so ordinary gameplay has no native detours at all.
static bool EiemInstallPhysicsBindingHooksEarly(void **assemblies,size_t count,
                                                std::string &error) {
  if (!assemblies || !count || !il2cpp_method_get_flags || !il2cpp_resolve_icall) {
    error="Early Animator binding hooks require complete metadata APIs"; return false;
  }
  void *animator=EiemPhysicsClass(assemblies,count,EiemPhysicsAnimatorImage,"UnityEngine","Animator");
  if (!animator) { error="Animator class unavailable for early binding hooks"; return false; }
  void *targets[_countof(s_eiemPhysicsIcallTraceHooks)]={};
  for (size_t i=0;i<_countof(s_eiemPhysicsIcallTraceHooks);++i) {
    auto &hook=s_eiemPhysicsIcallTraceHooks[i];
    void *method=hook.parameter0 ?
      EiemPhysicsMethod(animator,hook.name,"System.Void",false,hook.parameter0,hook.parameter1) :
      EiemPhysicsMethod(animator,hook.name,"System.Void",false);
    uint32_t implementation=0;
    if (!method || !(il2cpp_method_get_flags(method,&implementation),implementation&0x1000)) {
      error=std::string("Early Animator binding signature unavailable: ")+hook.name; return false;
    }
    targets[i]=EiemPhysicsTraceResolveIcall(hook.name);
    if (!EiemPhysicsTraceExecutable(targets[i])) {
      error=std::string("Early Animator binding target unavailable: ")+hook.name; return false;
    }
    if (hook.created && hook.target!=targets[i]) {
      error="Early Animator binding target changed after installation"; return false;
    }
    for (size_t j=0;j<i;++j) if (targets[i]==targets[j]) {
      error="Early Animator binding icalls share a target"; return false;
    }
  }
  for (size_t i=0;i<_countof(s_eiemPhysicsIcallTraceHooks);++i) {
    auto &hook=s_eiemPhysicsIcallTraceHooks[i];
    if (!hook.created) {
      const auto status=MH_CreateHook(targets[i],hook.detour,
        &s_eiemPhysicsTraceOriginal[(size_t)hook.operation]);
      if (status!=MH_OK) {
        error=std::string("Cannot create early Animator binding hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.target=targets[i]; hook.created=true;
    }
    if (!hook.enabled) {
      const auto status=MH_EnableHook(hook.target);
      if (status!=MH_OK) {
        error=std::string("Cannot enable early Animator binding hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.enabled=true;
    }
  }
  // Install the DynamicBone scheduler boundaries in the same early pass. The
  // normal trace may start from the first window message, which can be after
  // the first animation/physics jobs have already been scheduled.
  void *transformManager=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll",
                                          "BeyondDynamicBone","DynamicBoneTransformManager");
  if (!transformManager) {
    error="DynamicBoneTransformManager class unavailable for early hooks"; return false;
  }
  void *jobTargets[_countof(s_eiemPhysicsJobTraceHooks)]={};
  for (size_t i=0;i<_countof(s_eiemPhysicsJobTraceHooks);++i) {
    auto &hook=s_eiemPhysicsJobTraceHooks[i];
    void *method=hook.hasMaps ?
      EiemPhysicsMethod(transformManager,hook.name,hook.returns,false,
                        hook.parameter0,hook.parameter1,hook.parameter2,hook.parameter3) :
      EiemPhysicsMethod(transformManager,hook.name,hook.returns,false,hook.parameter0);
    jobTargets[i]=EiemPhysicsTraceAddress(method);
    if (!jobTargets[i]) {
      error=std::string("Early DynamicBone buffer signature unavailable: ")+hook.name;
      return false;
    }
    if (hook.created && hook.target!=jobTargets[i]) {
      error="Early DynamicBone buffer target changed after installation"; return false;
    }
    for (size_t j=0;j<i;++j) if (jobTargets[i]==jobTargets[j]) {
      error="Early DynamicBone buffer methods share a target"; return false;
    }
  }
  for (size_t i=0;i<_countof(s_eiemPhysicsJobTraceHooks);++i) {
    auto &hook=s_eiemPhysicsJobTraceHooks[i];
    if (!hook.created) {
      const auto status=MH_CreateHook(jobTargets[i],hook.detour,
        &s_eiemPhysicsTraceOriginal[(size_t)hook.operation]);
      if (status!=MH_OK) {
        error=std::string("Cannot create early DynamicBone buffer hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.target=jobTargets[i]; hook.created=true;
    }
    if (!hook.enabled) {
      const auto status=MH_EnableHook(hook.target);
      if (status!=MH_OK) {
        error=std::string("Cannot enable early DynamicBone buffer hook: ")+hook.name+
          " status="+std::to_string((int)status); return false;
      }
      hook.enabled=true;
    }
  }
  error.clear(); return true;
}
static bool EiemStartPhysicsTrace(std::string &error,uint32_t durationMs=0) {
  if (!EiemOnUnityThread() || !il2cpp_domain_get || !il2cpp_domain_get_assemblies) {
    error="Physics trace requires Unity thread and an initialized domain"; return false;
  }
  void *domain=il2cpp_domain_get(); size_t count=0;
  void **assemblies=domain?il2cpp_domain_get_assemblies(domain,&count):nullptr;
  return EiemInstallPhysicsTrace(assemblies,count,error) &&
         EiemPhysicsBeginTrace(error,durationMs);
}
