#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include "eiem_native_physics_api.h"

// Existing-group scalar parameter adapter only. No topology/selection edits,
// no collider-buffer writes, BuildAndRun, Dispose, or Transform destruction.
// The model/physics owner retains the journal until Restore succeeds. It is
// independent of Renderer visibility and cannot be copied into per-Mesh state.
// It is not yet wired to Render.physics; native integration needs game testing.
struct EiemPhysicsFloatEdit { std::string name; float value = 0; };
struct EiemPhysicsParameterApi {
  void *cloth = nullptr, *data = nullptr, *process = nullptr;
  void *dataField = nullptr, *processField = nullptr;
  void *building = nullptr, *destroying = nullptr;
  void *valid = nullptr, *ctor = nullptr, *import = nullptr;
  void *setData = nullptr, *notify = nullptr;

  bool Resolve(void **assemblies, size_t count, std::string &error) {
    *this = {};
    if (!il2cpp_class_get_fields || !il2cpp_field_get_name || !il2cpp_field_get_flags ||
        !il2cpp_field_get_type || !il2cpp_field_get_value || !il2cpp_field_set_value ||
        !il2cpp_class_get_methods || !il2cpp_method_get_name || !il2cpp_method_get_param_count ||
        !il2cpp_method_get_return_type || !il2cpp_method_get_param || !il2cpp_method_get_flags ||
        !il2cpp_type_get_name || !il2cpp_free || !il2cpp_object_new || !il2cpp_object_get_class ||
        !il2cpp_object_unbox || !il2cpp_assembly_get_image || !il2cpp_image_get_name ||
        !il2cpp_class_from_name || !assemblies || !count) {
      error = "Native physics metadata APIs unavailable"; return false;
    }
    cloth = EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","BeyondBoneCloth");
    data = EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","ClothSerializeData");
    process = EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","ClothProcess");
    dataField = EiemPhysicsField(cloth,"serializeData","BeyondDynamicBone.ClothSerializeData");
    processField = EiemPhysicsField(cloth,"process","BeyondDynamicBone.ClothProcess");
    building = EiemPhysicsField(process,"isBuild","System.Boolean");
    destroying = EiemPhysicsField(process,"isDestory","System.Boolean");
    valid = EiemPhysicsMethod(cloth,"IsValid","System.Boolean",false);
    ctor = EiemPhysicsMethod(data,".ctor","System.Void",false);
    import = EiemPhysicsMethod(data,"Import","System.Void",false,"BeyondDynamicBone.ClothSerializeData","System.Boolean");
    setData = EiemPhysicsMethod(cloth,"set_SerializeData","System.Void",false,"BeyondDynamicBone.ClothSerializeData");
    notify = EiemPhysicsMethod(cloth,"SetParameterChange","System.Void",false);
    if (!cloth || !data || !process || !dataField || !processField || !building ||
        !destroying || !valid || !ctor || !import || !setData || !notify) {
      error = "Native physics parameter contract incomplete or ambiguous"; return false;
    }
    error.clear(); return true;
  }
};

static bool EiemPhysicsScalarSupported(const EiemPhysicsFloatEdit &edit) {
  if (!std::isfinite(edit.value)) return false;
  if (edit.name == "gravity" || edit.name == "stablizationTimeAfterReset") return edit.value >= 0;
  if (edit.name == "gravityFalloff" || edit.name == "blendWeight" || edit.name == "animationPoseRatio")
    return edit.value >= 0 && edit.value <= 1;
  return false; // topology, curve subobjects and arbitrary fields are not scalar edits
}

class EiemPhysicsParameters {
  struct Binding {
  EiemUnityRef component, source, replacement;
  bool ownsBinding = false;

  // No native calls in destructors: release/restoration is explicit on Unity's
  // thread. Shared physics owners must not drop this on a Mesh visibility change.
  bool Ready(const EiemPhysicsParameterApi &api, std::string &error) const {
    if (!EiemOnUnityThread()) { error="Physics parameters require Unity thread"; return false; }
    if (component.Status()!=1) { error="Physics component unavailable"; return false; }
    void *p=nullptr; bool build=false, destroy=false, isValid=false;
    if (!EiemPhysicsReadField(component.Target(),api.processField,p) || !p ||
        il2cpp_object_get_class(p)!=api.process ||
        !EiemPhysicsReadField(p,api.building,build) || !EiemPhysicsReadField(p,api.destroying,destroy) ||
        !EiemPhysicsReadValue(component.Target(),api.valid,isValid)) {
      error="Cannot read native physics parameter-update state"; return false;
    }
    if (build || destroy || !isValid) {
      error="Native physics is building, retiring or invalid; parameter update not applied"; return false;
    }
    return true; // Admission for parameter API only, NOT a Task/Job release fence.
  }

  bool Prepare(const EiemPhysicsParameterApi &api, void *target,
               const std::vector<EiemPhysicsFloatEdit> &edits, std::string &error) {
    if (!EiemOnUnityThread() || component || !target || edits.empty() ||
        il2cpp_object_get_class(target)!=api.cloth) {
      error="Physics parameter preparation requires a new record and a source component"; return false;
    }
    std::vector<void *> fields;
    for (const auto &edit:edits) {
      void *field=EiemPhysicsField(api.data,edit.name.c_str(),"System.Single");
      if (!EiemPhysicsScalarSupported(edit) || !field ||
          std::find(fields.begin(),fields.end(),field)!=fields.end()) {
        error="Unsupported, duplicate or invalid physics scalar: "+edit.name; return false;
      }
      fields.push_back(field);
    }
    Binding staged;
    staged.component=EiemUnityRef::Capture(target,false);
    if (!staged.Ready(api,error)) return false;
    void *original=nullptr;
    if (!EiemPhysicsReadField(target,api.dataField,original) || !original ||
        il2cpp_object_get_class(original)!=api.data ||
        !(staged.source=EiemUnityRef::Capture(original,false))) {
      error="Cannot retain original native physics configuration"; return false;
    }
    void *copy=il2cpp_object_new(api.data), *result=nullptr;
    staged.replacement=EiemUnityRef::Capture(copy,false);
    bool deep=true; void *args[]={original,&deep};
    if (!staged.replacement || copy==original ||
        !InvokeChecked(api.ctor,copy,nullptr,&result) || !InvokeChecked(api.import,copy,args,&result)) {
      error="Native physics deep-copy construction failed"; return false;
    }
    for (size_t i=0;i<edits.size();++i) {
      float value=edits[i].value;
      if (!WriteScalar(copy,fields[i],value)) { error="Cannot write private physics parameter"; return false; }
      float actual=0;
      if (!EiemPhysicsReadField(copy,fields[i],actual) || actual!=value) {
        error="Private physics parameter readback mismatch"; return false;
      }
    }
    *this=std::move(staged); error.clear(); return true;
  }

  bool Apply(const EiemPhysicsParameterApi &api, std::string &error) {
    if (!source || !replacement || !Ready(api,error)) return false;
    void *live=nullptr;
    if (!EiemPhysicsReadField(component.Target(),api.dataField,live) ||
        (live!=source.Target() && !(ownsBinding && live==replacement.Target()))) {
      error="Physics configuration changed ownership; refusing to overwrite it"; return false;
    }
    // The setter may change the binding before throwing. Retain source even
    // on failure; only explicit successful restoration relinquishes ownership.
    ownsBinding=true;
    if (!AssignAndNotify(api,replacement.Target(),error)) return false;
    error.clear(); return true;
  }

  bool CheckApplied(const EiemPhysicsParameterApi &api, std::string &error) const {
    if (!Ready(api,error)) return false;
    void *live=nullptr;
    if (!EiemPhysicsReadField(component.Target(),api.dataField,live) || live!=replacement.Target()) {
      error="Applied physics configuration was changed outside its owner"; return false;
    }
    error.clear(); return true;
  }

  bool Restore(const EiemPhysicsParameterApi &api, std::string &error) {
    if (!EiemOnUnityThread()) { error="Physics restoration requires Unity thread"; return false; }
    if (!ownsBinding) { *this={}; error.clear(); return true; }
    if (component.Status()==0) { *this={}; error.clear(); return true; }
    if (!Ready(api,error)) return false;
    void *live=nullptr;
    if (!EiemPhysicsReadField(component.Target(),api.dataField,live) ||
        (live && live!=source.Target() && live!=replacement.Target())) {
      error="Physics configuration no longer owned; restoration retained for inspection"; return false;
    }
    if (!AssignAndNotify(api,source.Target(),error)) return false;
    *this={}; error.clear(); return true;
  }

 private:
  static bool WriteScalar(void *object,void *field,float value) {
    __try { il2cpp_field_set_value(object,field,&value); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
  }
  bool AssignAndNotify(const EiemPhysicsParameterApi &api,void *value,std::string &error) {
    void *result=nullptr,*actual=nullptr; void *args[]={value};
    if (!InvokeChecked(api.setData,component.Target(),args,&result) ||
        !EiemPhysicsReadField(component.Target(),api.dataField,actual) || actual!=value ||
        !InvokeChecked(api.notify,component.Target(),nullptr,&result)) {
      error="Native physics binding/parameter notification failed; source retained"; return false;
    }
    return true;
  }
  };

  struct Entry {
    std::string owner;
    std::vector<EiemPhysicsFloatEdit> edits;
    Binding binding;
    bool applied=false, restorePending=false;
  };
  // Component references stay rooted while journalled, preventing address
  // reuse from being mistaken for the same live component. No team ID keys.
  std::vector<Entry> entries_;

  auto Find(void *target) {
    return std::find_if(entries_.begin(),entries_.end(),[&](const Entry &entry) {
      return entry.binding.component.Target()==target;
    });
  }
  static bool SameEdits(const std::vector<EiemPhysicsFloatEdit> &left,
                        const std::vector<EiemPhysicsFloatEdit> &right) {
    return left.size()==right.size() && std::equal(left.begin(),left.end(),right.begin(),
        [](const EiemPhysicsFloatEdit &a,const EiemPhysicsFloatEdit &b) {
          return a.name==b.name && a.value==b.value;
        });
  }
  static bool OnThread(std::string &error) {
    if (EiemOnUnityThread()) return true;
    error="Physics journal requires Unity thread"; return false;
  }

 public:
  EiemPhysicsParameters()=default;
  EiemPhysicsParameters(const EiemPhysicsParameters &)=delete;
  EiemPhysicsParameters &operator=(const EiemPhysicsParameters &)=delete;
  // This journal must live at model/physics subsystem scope, not per Renderer.
  // RestoreAll must finish before disposing it or publishing a new generation.
  size_t Count() const { return entries_.size(); }

  bool Prepare(const EiemPhysicsParameterApi &api, void *target, const std::string &owner,
               std::vector<EiemPhysicsFloatEdit> edits, std::string &error) {
    if (!OnThread(error)) return false;
    if (!target || owner.empty()) { error="Physics component and resource owner are required"; return false; }
    std::sort(edits.begin(),edits.end(),[](const EiemPhysicsFloatEdit &a,const EiemPhysicsFloatEdit &b) {
      return a.name<b.name;
    });
    auto found=Find(target);
    if (found!=entries_.end()) {
      if (found->owner!=owner) { error="Physics component already has a different resource owner"; return false; }
      if (found->restorePending) { error="Physics restoration must finish before preparing again"; return false; }
      if (!SameEdits(found->edits,edits)) { error="Physics parameters changed; restore before preparing a new generation"; return false; }
      // Repeated references to one Physics resource share the SAME baseline.
      // Do not deep-copy the currently attached Mod config as another source.
      error.clear(); return true;
    }
    Entry staged; staged.owner=owner; staged.edits=std::move(edits);
    if (!staged.binding.Prepare(api,target,staged.edits,error)) return false;
    entries_.push_back(std::move(staged)); error.clear(); return true;
  }

  bool Apply(const EiemPhysicsParameterApi &api, void *target, const std::string &owner, std::string &error) {
    if (!OnThread(error)) return false;
    auto found=Find(target);
    if (found==entries_.end() || found->owner!=owner) { error="Physics configuration is not prepared by this owner"; return false; }
    if (found->restorePending) { error="Physics restoration is pending; refusing to reapply"; return false; }
    const bool ok=found->applied ? found->binding.CheckApplied(api,error) : found->binding.Apply(api,error);
    if (!ok) { found->restorePending=true; return false; }
    found->applied=true; error.clear(); return true;
  }

  bool Restore(const EiemPhysicsParameterApi &api, void *target, const std::string &owner, std::string &error) {
    if (!OnThread(error)) return false;
    auto found=Find(target);
    if (found==entries_.end()) { error.clear(); return true; }
    if (found->owner!=owner) { error="Cannot restore another physics resource owner's configuration"; return false; }
    found->restorePending=true;
    if (!found->binding.Restore(api,error)) return false;
    entries_.erase(found); error.clear(); return true;
  }

  bool RestoreAll(const EiemPhysicsParameterApi &api, std::string &error) {
    if (!OnThread(error)) return false;
    error.clear();
    for (auto it=entries_.begin();it!=entries_.end();) {
      it->restorePending=true; std::string detail;
      if (it->binding.Restore(api,detail)) it=entries_.erase(it);
      else { if (error.empty()) error=it->owner+": "+detail; ++it; }
    }
    return entries_.empty(); // Failed restores retain their original refs.
  }
};
