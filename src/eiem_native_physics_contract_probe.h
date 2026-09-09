#pragma once
#include "eiem_native_physics_api.h"

// Resolve a fixed list of engine entry points for offline correlation. Never
// call these targets: resolving an icall does not create an Animator binding.
static void *EiemPhysicsProbeResolveIcall(const char *name) {
  if (!EiemOnUnityThread() || !il2cpp_resolve_icall) return nullptr;
  __try { return il2cpp_resolve_icall(name); }
  __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
static void EiemPhysicsWriteIcallTargets(FILE *file,void *animatorClass) {
  fputs("\"engineEntryPoints\":{\"proof\":\"resolved-addresses-only-targets-not-invoked\",\"entries\":[",file);
  bool first=true;
  for (const char *name:{"CreateClothBindings_Injected","CreateClothBindingsByNameLst_Injected",
                         "EnableClothBindings","DisableClothBindings","DestroyClothBindings"}) {
    if (!first) fputc(',',file); first=false;
    const std::string full=std::string("UnityEngine.Animator::")+name;
    fputs("{\"name\":",file); EiemWriteJsonString(file,full.c_str());
    void *iterator=nullptr,*method=nullptr; size_t matches=0;
    while (animatorClass && (method=il2cpp_class_get_methods(animatorClass,&iterator))) {
      const char *label=il2cpp_method_get_name(method); uint32_t impl=0;
      il2cpp_method_get_flags(method,&impl);
      if (label && !strcmp(label,name) && (impl&0x1000)) ++matches; // InternalCall
    }
    if (matches!=1) { fputs(",\"status\":\"icall-metadata-unavailable-or-ambiguous\"}",file); continue; }
    if (!il2cpp_resolve_icall) { fputs(",\"status\":\"resolver-unavailable\"}",file); continue; }
    void *address=EiemPhysicsProbeResolveIcall(full.c_str());
    if (!address) { fputs(",\"status\":\"unresolved\"}",file); continue; }
    MEMORY_BASIC_INFORMATION memory={};
    bool queried=VirtualQuery(address,&memory,sizeof(memory))==sizeof(memory);
    const DWORD protection=memory.Protect&0xff;
    bool executable=queried && memory.State==MEM_COMMIT && !(memory.Protect&PAGE_GUARD) &&
      (protection==PAGE_EXECUTE || protection==PAGE_EXECUTE_READ ||
       protection==PAGE_EXECUTE_READWRITE || protection==PAGE_EXECUTE_WRITECOPY);
    fprintf(file,",\"status\":\"resolved\",\"address\":\"%p\",\"executable\":%s",address,executable?"true":"false");
    // GetModuleFileName does not retain the module. The address is an
    // observation, never a stored dispatch target or a module-lifetime lease.
    char path[32768]={};
    DWORD length=queried && memory.Type==MEM_IMAGE ?
      GetModuleFileNameA((HMODULE)memory.AllocationBase,path,_countof(path)) : 0;
    if (length && length<_countof(path)) {
      const char *label=strrchr(path,'\\'); label=label?label+1:path;
      fputs(",\"module\":",file); EiemWriteJsonString(file,label);
      fprintf(file,",\"rva\":\"0x%llx\"",(unsigned long long)((uintptr_t)address-(uintptr_t)memory.AllocationBase));
    } else fputs(",\"module\":null,\"rva\":null",file);
    fputc('}',file);
  }
  fputs("]},",file);
}

static void *EiemPhysicsProbeValueAddress(void *boxed) {
  if (!EiemOnUnityThread() || !boxed || !il2cpp_object_unbox) return nullptr;
  __try { return il2cpp_object_unbox(boxed); }
  __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
static void EiemPhysicsWriteBuildResult(FILE *file,void *process,void *processClass,void *resultClass) {
  fputs(",\"buildResult\":{",file);
  void *getter=EiemPhysicsMethod(processClass,"get_Result","BeyondDynamicBone.ResultCode",false);
  if (!resultClass || !getter || !il2cpp_type_get_type || !il2cpp_class_get_type ||
      il2cpp_type_get_type(il2cpp_class_get_type(resultClass))!=0x11) {
    fputs("\"status\":\"result-value-contract-unavailable\"}",file); return;
  }
  void *boxed=nullptr;
  if (!InvokeChecked(getter,process,nullptr,&boxed) || !boxed || il2cpp_object_get_class(boxed)!=resultClass) {
    fputs("\"status\":\"result-read-unavailable\"}",file); return;
  }
  // Invoke instance methods on a PINNED boxed COPY's unboxed data, not on the
  // process field or a guessed native value layout. Predicates have exact types.
  struct Pin {
    uint32_t handle=0;
    ~Pin() { if (handle) il2cpp_gchandle_free(handle); }
  } pin;
  if (il2cpp_gchandle_new && il2cpp_gchandle_free && il2cpp_gchandle_get_target)
    pin.handle=il2cpp_gchandle_new(boxed,true);
  if (!pin.handle) { fputs("\"status\":\"result-pin-unavailable\"}",file); return; }
  fputs("\"status\":\"observed-result-copy-not-completion-fence\"",file);
  for (const char *name:{"IsSuccess","IsProcess","IsCancel","IsError","IsWarning"}) {
    void *predicate=EiemPhysicsMethod(resultClass,name,"System.Boolean",false);
    void *value=EiemPhysicsProbeValueAddress(il2cpp_gchandle_get_target(pin.handle));
    bool result=false;
    fputc(',',file); EiemWriteJsonString(file,name); fputc(':',file);
    fputs(value && EiemPhysicsReadValue(value,predicate,result)?(result?"true":"false"):"null",file);
  }
  fputc('}',file);
}
