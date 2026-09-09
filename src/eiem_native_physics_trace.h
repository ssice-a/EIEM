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
    returned=true; EiemPhysicsReadBindingCounts(handle,count,invalidCount);
  } __finally { EiemPhysicsTraceLeave(ticket,Op,animator,values,returned,-1,count,invalidCount); }
}
template<EiemPhysicsOperation Op>
static void EiemPhysicsTraceIcallVoid(void *animator) {
  const auto ticket=EiemPhysicsTraceEnter(Op,animator);
  bool returned=false;
  __try { ((EiemPhysicsIcallVoid0)s_eiemPhysicsTraceOriginal[(size_t)Op])(animator); returned=true; }
  __finally { EiemPhysicsTraceLeave(ticket,Op,animator,nullptr,returned); }
}
struct EiemPhysicsTraceHook {
  const char *klass,*name,*returns,*parameter0,*parameter1;
  void *detour,*target=nullptr;
  bool created=false,enabled=false;
};
static EiemPhysicsTraceHook s_eiemPhysicsTraceHooks[]={
  {"BeyondBoneCloth","BuildAndRun","System.Boolean",nullptr,nullptr,(void *)EiemPhysicsTraceBool<EiemPhysicsOperation::BuildAndRun>},
  {"ClothProcess","StartRuntimeBuild","System.Boolean",nullptr,nullptr,(void *)EiemPhysicsTraceBool<EiemPhysicsOperation::StartRuntimeBuild>},
  {"ClothProcess","Init","System.Void",nullptr,nullptr,(void *)EiemPhysicsTraceVoid<EiemPhysicsOperation::Init>},
  {"ClothProcess","Dispose","System.Void",nullptr,nullptr,(void *)EiemPhysicsTraceVoid<EiemPhysicsOperation::Dispose>},
  {"ClothProcess","DisposeInternal","System.Void",nullptr,nullptr,(void *)EiemPhysicsTraceVoid<EiemPhysicsOperation::DisposeInternal>},
  {"TeamManager","RemoveMonitoringProcess","System.Void","BeyondDynamicBone.ClothProcess",nullptr,(void *)EiemPhysicsTraceRemoveMonitoring},
  {"ClothManager","CompleteMasterJob","System.Void",nullptr,nullptr,(void *)EiemPhysicsTraceVoid<EiemPhysicsOperation::CompleteMasterJob>},
  {"TeamManager","UpdateTeamAnimatorData","System.Void","BeyondDynamicBone.ExNativeArray<System.Int16>","UnityEngine.Jobs.TransformAccessArray",(void *)EiemPhysicsTraceUpdateAnimatorData},
  {"TeamManager","ClearTeamAnimatorData","System.Void","System.Int32",nullptr,(void *)EiemPhysicsTraceTeamId<EiemPhysicsOperation::TeamClearAnimatorData>},
  {"TeamManager","AddTeamAnimatorData","System.Void","System.Int32","BeyondDynamicBone.ClothProcess",(void *)EiemPhysicsTraceTeamObject<EiemPhysicsOperation::TeamAddAnimatorData>},
  {"TeamManager","AddAnimatorTransform","System.Void","System.Int32","UnityEngine.Transform",(void *)EiemPhysicsTraceTeamObject<EiemPhysicsOperation::TeamAddAnimatorTransform>},
  {"TeamManager","MarkAnimatorTransformDirty","System.Void","System.Int32","UnityEngine.Transform",(void *)EiemPhysicsTraceTeamObject<EiemPhysicsOperation::TeamMarkAnimatorTransformDirty>}
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
   EiemPhysicsOperation::AnimatorCreateClothBindingsByName,(void *)EiemPhysicsTraceIcallCreate<EiemPhysicsOperation::AnimatorCreateClothBindingsByName>},
  {"EnableClothBindings",nullptr,nullptr,EiemPhysicsOperation::AnimatorEnableClothBindings,
   (void *)EiemPhysicsTraceIcallVoid<EiemPhysicsOperation::AnimatorEnableClothBindings>},
  {"DisableClothBindings",nullptr,nullptr,EiemPhysicsOperation::AnimatorDisableClothBindings,
   (void *)EiemPhysicsTraceIcallVoid<EiemPhysicsOperation::AnimatorDisableClothBindings>},
  {"DestroyClothBindings",nullptr,nullptr,EiemPhysicsOperation::AnimatorDestroyClothBindings,
   (void *)EiemPhysicsTraceIcallVoid<EiemPhysicsOperation::AnimatorDestroyClothBindings>}
};
static_assert(_countof(s_eiemPhysicsTraceHooks)+_countof(s_eiemPhysicsIcallTraceHooks)==
              (size_t)EiemPhysicsOperation::Count,"Trace operation mismatch");

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
      const auto status=MH_CreateHook(targets[i],hook.detour,&s_eiemPhysicsTraceOriginal[i]);
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
  error.clear(); return true;
}
static bool EiemStartPhysicsTrace(std::string &error) {
  if (!EiemOnUnityThread() || !il2cpp_domain_get || !il2cpp_domain_get_assemblies) {
    error="Physics trace requires Unity thread and an initialized domain"; return false;
  }
  void *domain=il2cpp_domain_get(); size_t count=0;
  void **assemblies=domain?il2cpp_domain_get_assemblies(domain,&count):nullptr;
  return EiemInstallPhysicsTrace(assemblies,count,error) && EiemPhysicsBeginTrace(error);
}
