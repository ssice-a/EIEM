#pragma once
#include "eiem_shape_state.h"
#include <atomic>
#include <unordered_map>

static void *FindMethodWithParamTypesAndReturnType(void *,const char *,const char *const *,int,const char *);

struct EiemShapeRuntimeBinding {
  EiemUnityRef renderer, mesh;
  void *address=nullptr;
  std::mutex mutex;
  EiemShapeChannels channels;
  std::atomic<bool> active{true};
  bool initialized=false;
  uint64_t suppressed=0;
};
// Non-owning lookup only. The existing override/partner state owns each binding.
static std::mutex s_eiemShapeBindingMutex;
static std::unordered_map<void *,std::weak_ptr<EiemShapeRuntimeBinding>> s_eiemShapeBindings;
static std::atomic<bool> s_eiemShapeGuardReady{false};
static std::atomic<bool> s_eiemShapeGuardThreadError{false};
static bool EiemShapeBindingMatches(const EiemShapeState &state,void *renderer,void *mesh) {
  return state.binding && state.binding->active && state.binding->renderer.Target()==renderer &&
         state.binding->mesh.Target()==mesh;
}
using EiemShapeNativeGet=float (__fastcall *)(void *,int);
using EiemShapeNativeSet=void (__fastcall *)(void *,int,float);
using EiemShapeNativeMesh=void *(__fastcall *)(void *);
using EiemShapeNativeApply=void (__fastcall *)(void *,void *);
static void *s_eiemShapeOriginalGet=nullptr,*s_eiemShapeOriginalSet=nullptr,*s_eiemShapeOriginalApply=nullptr;
static EiemShapeNativeMesh s_eiemShapeCurrentMesh=nullptr;

static void EiemRetireShapeBinding(const std::shared_ptr<EiemShapeRuntimeBinding> &binding) {
  if(!binding)return;
  binding->active.store(false,std::memory_order_release);
  std::lock_guard<std::mutex> lock(s_eiemShapeBindingMutex);
  auto it=s_eiemShapeBindings.find(binding->address);
  if(it!=s_eiemShapeBindings.end() && it->second.lock()==binding)s_eiemShapeBindings.erase(it);
}
static void EiemPublishShapeBinding(const std::shared_ptr<EiemShapeRuntimeBinding> &binding) {
  std::lock_guard<std::mutex> lock(s_eiemShapeBindingMutex);
  s_eiemShapeBindings[binding->address]=binding;
}
static std::shared_ptr<EiemShapeRuntimeBinding> EiemFindShapeBinding(void *renderer) {
  std::shared_ptr<EiemShapeRuntimeBinding> binding;
  {
    std::lock_guard<std::mutex> lock(s_eiemShapeBindingMutex);
    auto it=s_eiemShapeBindings.find(renderer);
    if(it!=s_eiemShapeBindings.end())binding=it->second.lock();
  }
  if(!binding || !binding->active.load(std::memory_order_acquire) || binding->renderer.Target()!=renderer)return {};
  // Only the verified Unity-thread Apply path may forward a protected write.
  // Never call Unity mesh getters from an unexpected worker thread.
  if(!EiemOnUnityThread()) {
    if(!s_eiemShapeGuardThreadError.exchange(true))Log("[SHAPE] Protected game shape call on unsupported thread; native write suppressed");
    return binding;
  }
  void *expected=binding->mesh.Target();
  if(!expected || !s_eiemShapeCurrentMesh || s_eiemShapeCurrentMesh(renderer)!=expected)return {};
  return binding;
}
static float __fastcall EiemShapeGuardGet(void *renderer,int index) {
  if(s_eiemShapeGameDepth && !s_eiemShapeAuthorDepth) {
    auto binding=EiemFindShapeBinding(renderer);
    if(binding) {std::lock_guard<std::mutex> lock(binding->mutex);return binding->channels.GameRead(index);}
  }
  return ((EiemShapeNativeGet)s_eiemShapeOriginalGet)(renderer,index);
}
static void __fastcall EiemShapeGuardSet(void *renderer,int index,float value) {
  if(s_eiemShapeGameDepth && !s_eiemShapeAuthorDepth) {
    auto binding=EiemFindShapeBinding(renderer);
    if(binding) {
      int target=-1;
      {
        std::lock_guard<std::mutex> lock(binding->mutex);
        if(EiemOnUnityThread())target=binding->channels.GameWrite(index,value);
        if(target<0)++binding->suppressed;
      }
      if(target>=0)((EiemShapeNativeSet)s_eiemShapeOriginalSet)(renderer,target,value);
      return;
    }
  }
  ((EiemShapeNativeSet)s_eiemShapeOriginalSet)(renderer,index,value);
}
static void __fastcall EiemShapeGuardApply(void *self,void *method) {
  EiemShapeGameScope scope;
  ((EiemShapeNativeApply)s_eiemShapeOriginalApply)(self,method);
}
static bool EiemInitShapeGuard(void *coreClass) {
  if(s_eiemShapeGuardReady)return true;
  if(!il2cpp_resolve_icall || !coreClass)return false;
  void *get=il2cpp_resolve_icall("UnityEngine.SkinnedMeshRenderer::GetBlendShapeWeight(System.Int32)");
  void *set=il2cpp_resolve_icall("UnityEngine.SkinnedMeshRenderer::SetBlendShapeWeight(System.Int32,System.Single)");
  auto mesh=(EiemShapeNativeMesh)il2cpp_resolve_icall("UnityEngine.SkinnedMeshRenderer::get_sharedMesh()");
  void *apply=FindMethodWithParamTypesAndReturnType(coreClass,"_ApplyShaderPropDataToRenderers",nullptr,0,"System.Void");
  void *entry=apply?((MInfo *)apply)->mp:nullptr;
  if(!get || !set || !mesh || !entry) {
    Log("[SHAPE] Channel guard unavailable get=%p set=%p mesh=%p apply=%p",get,set,mesh,entry);return false;
  }
  struct HookEntry {void *target,*detour;void **original;};
  HookEntry hooks[]={{get,(void *)EiemShapeGuardGet,&s_eiemShapeOriginalGet},
    {set,(void *)EiemShapeGuardSet,&s_eiemShapeOriginalSet},{entry,(void *)EiemShapeGuardApply,&s_eiemShapeOriginalApply}};
  size_t created=0;MH_STATUS status=MH_OK;
  for(auto &hook:hooks) {
    status=MH_CreateHook(hook.target,hook.detour,hook.original);
    if(status!=MH_OK)break;
    ++created;
  }
  if(created==3) {
    for(auto &hook:hooks) {status=MH_QueueEnableHook(hook.target);if(status!=MH_OK)break;}
    if(status==MH_OK)status=MH_ApplyQueued();
  }
  if(created!=3 || status!=MH_OK) {
    for(size_t i=0;i<created;++i) {MH_DisableHook(hooks[i].target);MH_RemoveHook(hooks[i].target);}
    Log("[SHAPE] Channel guard install failed status=%d; installation rolled back",(int)status);return false;
  }
  s_eiemShapeCurrentMesh=mesh;s_eiemShapeGuardReady=true;
  Log("[SHAPE] Source-channel guard ready get=%p set=%p apply=%p",get,set,entry);return true;
}

template<class Backend>
static bool EiemPrepareShapeBinding(EiemShapeState &state,void *renderer,void *mesh,
    const std::vector<EiemShapeBaseline> &source,Backend &backend,std::string &error) {
  if(EiemShapeBindingMatches(state,renderer,mesh))return true;
  std::vector<std::string> target,names;std::vector<float> values;
  if(!backend.Names(mesh,target)){error="Cannot read replacement shape layout";return false;}
  for(const auto &entry:source){names.push_back(entry.name);values.push_back(entry.value);}
  if(names.empty() && target.empty()) {EiemRetireShapeBinding(state.binding);state.binding.reset();return true;}
  if(!s_eiemShapeGuardReady){error="Source-channel guard is unavailable; refusing unprotected shape binding";return false;}
  // Rebinding to another resource keeps the current game-side values, not an
  // old value from before an earlier replacement.
  if(state.binding) {
    std::lock_guard<std::mutex> lock(state.binding->mutex);
    if(state.binding->channels.sourceNames==names)values=state.binding->channels.gameValues;
  }
  auto binding=std::make_shared<EiemShapeRuntimeBinding>();
  if(!binding->channels.Build(names,values,target,error))return false;
  binding->renderer=EiemUnityRef::Capture(renderer);binding->mesh=EiemUnityRef::Capture(mesh);binding->address=renderer;
  if(!binding->renderer || !binding->mesh){error="Cannot observe shape binding lifetime";return false;}
  EiemRetireShapeBinding(state.binding);state.binding=binding;EiemPublishShapeBinding(binding);
  Log("[SHAPE] Bound renderer=%p sourceChannels=%zu targetChannels=%zu",renderer,names.size(),target.size());
  return true;
}
template<class Backend>
static bool EiemInitializeBoundShapes(EiemShapeState &state,void *renderer,Backend &backend,std::string &error) {
  auto binding=state.binding;if(!binding || binding->initialized)return true;
  std::vector<float> values;
  { std::lock_guard<std::mutex> lock(binding->mutex);
    values.assign(binding->channels.targetNames.size(),0.0f);
    for(size_t i=0;i<binding->channels.targetIndices.size();++i)values[binding->channels.targetIndices[i]]=binding->channels.gameValues[i];
  }
  EiemShapeAuthorScope scope;
  for(int index=0;index<(int)values.size();++index) {
    float actual=0;
    if(!backend.Read(renderer,index,actual) ||
       (actual!=values[index] && (!backend.Write(renderer,index,values[index]) || !backend.Read(renderer,index,actual))) ||
       std::abs(actual-values[index])>.0001f){error="Cannot initialize bound shape weight";return false;}
  }
  binding->initialized=true;return true;
}
// Keep claims active until the real weight operation has completed (including
// partial failure). No engine calls while the binding lock is held.
static void EiemSyncShapeClaims(EiemShapeState &state,const EiemModRule &pending) {
  auto binding=state.binding;if(!binding)return;
  std::lock_guard<std::mutex> lock(binding->mutex);
  auto &channels=binding->channels;
  std::fill(channels.controlled.begin(),channels.controlled.end(),false);
  auto index=[&](const std::string &name){auto it=std::find(channels.targetNames.begin(),channels.targetNames.end(),name);
    return it==channels.targetNames.end()?-1:(int)(it-channels.targetNames.begin());};
  for(auto &entry:state.owned) {int slot=index(entry.name);if(slot>=0){channels.controlled[slot]=true;channels.Latest(slot,entry.value);}}
  for(uint32_t i=0;i<pending.shapeCount;++i){int slot=index(pending.shapeNames[i]);if(slot>=0)channels.controlled[slot]=true;}
}
static std::vector<EiemShapeBaseline> EiemShapeSourceWeights(const EiemShapeState &state,
    const std::vector<EiemShapeBaseline> &baseline) {
  auto binding=state.binding;if(!binding)return baseline;
  std::lock_guard<std::mutex> lock(binding->mutex);
  std::vector<EiemShapeBaseline> result;
  for(size_t i=0;i<binding->channels.sourceNames.size();++i)result.push_back({binding->channels.sourceNames[i],binding->channels.gameValues[i]});
  return result;
}
