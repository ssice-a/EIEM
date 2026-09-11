#pragma once
#include <algorithm>
#include <map>
#include <unordered_map>
#include "eiem_native_physics_parameters.h"
#include "eiem_physics_document.h"

struct EiemPhysicsManagedKeyframe {
  float time,value,inTangent,outTangent;
  int32_t weightedMode;
  float inWeight,outWeight;
};
static_assert(sizeof(EiemPhysicsManagedKeyframe)==28,"UnityEngine.Keyframe layout mismatch");

// Factory preparation ONLY: detached managed configuration objects. Nothing
// here creates a Unity component, starts a process, or owns/releases bones.
// V1 roles map only to the verified root/ignore input accepted by BoneCloth.
// V2 selection graphs remain a separate contract. Author collider components
// are instantiated and attached by the per-model runtime after this detached
// ClothSerializeData draft has been prepared.
struct EiemPhysicsConfigApi {
  void *data=nullptr,*data2=nullptr,*transform=nullptr,*rootList=nullptr;
  void *curveData=nullptr,*animationCurve=nullptr,*keyframe=nullptr;
  void *dataCtor=nullptr,*data2Ctor=nullptr,*listCtor=nullptr,*listAdd=nullptr;
  void *listCount=nullptr,*listItem=nullptr,*rootsField=nullptr,*ignoresField=nullptr;
  void *clothTypeField=nullptr,*connectionModeField=nullptr,*getParent=nullptr;
  void *getChildCount=nullptr,*getChild=nullptr;
  void *radiusField=nullptr,*radiusValueField=nullptr,*radiusUseCurveField=nullptr,*radiusCurveField=nullptr;
  void *curveSetValue=nullptr,*animationCurveCtor=nullptr,*animationCurveGetKeys=nullptr;
  std::array<void *,5> scalars={};

  static const char *ScalarName(size_t i) {
    static const char *names[]={"gravity","stablizationTimeAfterReset","gravityFalloff","blendWeight","animationPoseRatio"};
    return i<_countof(names)?names[i]:nullptr;
  }
  bool Resolve(void **assemblies,size_t count,std::string &error) {
    *this={};
    if (!EiemOnUnityThread() || !assemblies || !count ||
        !il2cpp_assembly_get_image || !il2cpp_image_get_name || !il2cpp_class_from_name ||
        !il2cpp_class_get_methods || !il2cpp_method_get_name || !il2cpp_method_get_flags ||
        !il2cpp_method_get_param_count || !il2cpp_method_get_param || !il2cpp_method_get_return_type ||
        !il2cpp_class_get_fields || !il2cpp_field_get_name || !il2cpp_field_get_flags ||
        !il2cpp_field_get_type || !il2cpp_type_get_type || !il2cpp_class_from_type || !il2cpp_class_get_type ||
        !il2cpp_type_get_name || !il2cpp_free || !il2cpp_field_get_value || !il2cpp_field_set_value ||
        !il2cpp_object_new || !il2cpp_object_get_class || !il2cpp_object_unbox ||
        !il2cpp_class_value_size || !il2cpp_array_new || !il2cpp_array_length ||
        !il2cpp_runtime_invoke || !il2cpp_gchandle_new || !il2cpp_gchandle_free || !il2cpp_gchandle_get_target) {
      error="Detached physics configuration requires Unity thread and complete metadata APIs"; return false;
    }
    data=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","ClothSerializeData");
    data2=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","ClothSerializeData2");
    curveData=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","CurveSerializeData");
    transform=EiemPhysicsClass(assemblies,count,"UnityEngine.CoreModule.dll","UnityEngine","Transform");
    animationCurve=EiemPhysicsClass(assemblies,count,"UnityEngine.CoreModule.dll","UnityEngine","AnimationCurve");
    keyframe=EiemPhysicsClass(assemblies,count,"UnityEngine.CoreModule.dll","UnityEngine","Keyframe");
    dataCtor=EiemPhysicsMethod(data,".ctor","System.Void",false);
    data2Ctor=EiemPhysicsMethod(data2,".ctor","System.Void",false);
    rootsField=EiemPhysicsField(data,"rootBones","System.Collections.Generic.List<UnityEngine.Transform>");
    ignoresField=EiemPhysicsField(data,"ignoreFromRootBones","System.Collections.Generic.List<UnityEngine.Transform>");
    clothTypeField=EiemPhysicsField(data,"clothType","BeyondDynamicBone.ClothProcess.ClothType");
    connectionModeField=EiemPhysicsField(data,"connectionMode","BeyondDynamicBone.RenderSetupData.BoneConnectionMode");
    radiusField=EiemPhysicsField(data,"radius","BeyondDynamicBone.CurveSerializeData");
    radiusValueField=EiemPhysicsField(curveData,"value","System.Single");
    radiusUseCurveField=EiemPhysicsField(curveData,"useCurve","System.Boolean");
    radiusCurveField=EiemPhysicsField(curveData,"curve","UnityEngine.AnimationCurve");
    curveSetValue=EiemPhysicsMethod(curveData,"SetValue","System.Void",false,
                                    "System.Single","UnityEngine.AnimationCurve");
    animationCurveCtor=EiemPhysicsMethod(animationCurve,".ctor","System.Void",false,"UnityEngine.Keyframe[]");
    animationCurveGetKeys=EiemPhysicsMethod(animationCurve,"get_keys","UnityEngine.Keyframe[]",false);
    rootList=rootsField?il2cpp_class_from_type(il2cpp_field_get_type(rootsField)):nullptr;
    if (rootList && EiemPhysicsTypeName(il2cpp_class_get_type(rootList))!=
        "System.Collections.Generic.List<UnityEngine.Transform>") rootList=nullptr;
    listCtor=EiemPhysicsMethod(rootList,".ctor","System.Void",false);
    listAdd=EiemPhysicsMethod(rootList,"Add","System.Void",false,"UnityEngine.Transform");
    listCount=EiemPhysicsMethod(rootList,"get_Count","System.Int32",false);
    listItem=EiemPhysicsMethod(rootList,"get_Item","UnityEngine.Transform",false,"System.Int32");
    getParent=EiemPhysicsMethod(transform,"get_parent","UnityEngine.Transform",false);
    getChildCount=EiemPhysicsMethod(transform,"get_childCount","System.Int32",false);
    getChild=EiemPhysicsMethod(transform,"GetChild","UnityEngine.Transform",false,"System.Int32");
    for (size_t i=0;i<scalars.size();++i) scalars[i]=EiemPhysicsField(data,ScalarName(i),"System.Single");
    uint32_t keyframeAlign=0;
    const int32_t keyframeSize=keyframe?il2cpp_class_value_size(keyframe,&keyframeAlign):0;
    if (!data || !data2 || !curveData || !transform || !animationCurve || !keyframe ||
        keyframeSize!=sizeof(EiemPhysicsManagedKeyframe) ||
        !rootList || !dataCtor || !data2Ctor || !listCtor ||
        !listAdd || !listCount || !listItem || !rootsField || !ignoresField ||
        !clothTypeField || !connectionModeField || !radiusField || !radiusValueField ||
        !radiusUseCurveField || !radiusCurveField || !curveSetValue ||
        !animationCurveCtor || !animationCurveGetKeys ||
        !getParent || !getChildCount || !getChild ||
        std::find(scalars.begin(),scalars.end(),nullptr)!=scalars.end()) {
      error="Detached physics configuration metadata contract incomplete or ambiguous"; *this={}; return false;
    }
    error.clear(); return true;
  }
};

struct EiemPhysicsConfigGroup {
  std::string id,name;
  EiemUnityRef data,data2,roots,ignores;
  EiemUnityRef radiusData,radiusCurve,radiusKeys;
  std::vector<EiemUnityRef> parameterObjects,parameterCurves,parameterKeys;
  std::vector<EiemUnityRef> transforms;
  // BoneCloth expands every root through the live Transform hierarchy. Keep
  // author-omitted child branches invalid by listing their first node here.
  std::vector<EiemUnityRef> boundaryIgnores;
  // Preserve author roles verbatim. This is NOT a native selection buffer.
  std::vector<EiemPhysicsAuthorNode> authorNodes;
};

class EiemPhysicsConfigDraft {
  std::vector<EiemPhysicsConfigGroup> groups_;
  static bool Write(void *object,void *field,void *value) {
    if (!EiemOnUnityThread() || !object || !field || !value || !il2cpp_field_set_value) return false;
    __try { il2cpp_field_set_value(object,field,value); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
  }
  static bool New(void *klass,void *ctor,EiemUnityRef &out) {
    void *object=il2cpp_object_new(klass),*result=nullptr;
    out=EiemUnityRef::Capture(object,false);
    return out && il2cpp_object_get_class(object)==klass && InvokeChecked(ctor,object,nullptr,&result);
  }
  static bool PrepareCurve(const EiemPhysicsConfigApi &api,void *curveData,
                           const EiemPhysicsAuthorRadius &source,
                           EiemUnityRef &dataRef,EiemUnityRef &curveRef,EiemUnityRef &keysRef,
                           std::string &error,const std::string &label) {
    std::vector<EiemPhysicsManagedKeyframe> values;
    values.reserve(source.keys.size());
    for (const auto &key:source.keys)
      values.push_back({key.time,key.value,key.inSlope,key.outSlope,
                        (int32_t)key.weightedMode,key.inWeight,key.outWeight});
    void *array=il2cpp_array_new(api.keyframe,values.size());
    if (!array || il2cpp_array_length(array)!=values.size()) {
      error="Cannot allocate native Physics Keyframe array: "+label; return false;
    }
    memcpy((char *)array+IL2CPP_ARRAY_DATA,values.data(),values.size()*sizeof(values[0]));
    keysRef=EiemUnityRef::Capture(array,false);
    void *curve=il2cpp_object_new(api.animationCurve),*result=nullptr;
    curveRef=EiemUnityRef::Capture(curve,false);
    void *curveArgs[]={array};
    if (!keysRef || !curveRef ||
        il2cpp_object_get_class(curve)!=api.animationCurve ||
        !InvokeChecked(api.animationCurveCtor,curve,curveArgs,&result)) {
      error="Cannot construct native Physics AnimationCurve: "+label; return false;
    }
    if (!curveData || il2cpp_object_get_class(curveData)!=api.curveData ||
        !(dataRef=EiemUnityRef::Capture(curveData,false))) {
      error="ClothSerializeData constructor did not provide curve data: "+label; return false;
    }
    float base=source.value; void *setArgs[]={&base,curve};
    if (!InvokeChecked(api.curveSetValue,curveData,setArgs,&result)) {
      error="Cannot assign native Physics value and curve: "+label; return false;
    }
    bool useCurve=source.useCurve!=0;
    if (!Write(curveData,api.radiusUseCurveField,&useCurve)) {
      error="Cannot assign native Physics curve switch: "+label; return false;
    }
    float actualBase=0; bool actualUse=false; void *actualCurve=nullptr,*actualArray=nullptr;
    if (!EiemPhysicsReadField(curveData,api.radiusValueField,actualBase) || actualBase!=base ||
        !EiemPhysicsReadField(curveData,api.radiusUseCurveField,actualUse) || actualUse!=useCurve ||
        !EiemPhysicsReadField(curveData,api.radiusCurveField,actualCurve) || !actualCurve ||
        il2cpp_object_get_class(actualCurve)!=api.animationCurve ||
        !(curveRef=EiemUnityRef::Capture(actualCurve,false)) ||
        !InvokeChecked(api.animationCurveGetKeys,actualCurve,nullptr,&actualArray) || !actualArray ||
        il2cpp_array_length(actualArray)!=values.size() ||
        memcmp((char *)actualArray+IL2CPP_ARRAY_DATA,values.data(),values.size()*sizeof(values[0]))) {
      error="Native Physics curve readback mismatch: "+label; return false;
    }
    return true;
  }

  static bool PrepareRadius(const EiemPhysicsConfigApi &api,const EiemPhysicsAuthorRadius &source,
                            EiemPhysicsConfigGroup &candidate,std::string &error) {
    void *radius=nullptr;
    if (!EiemPhysicsReadField(candidate.data.Target(),api.radiusField,radius)) {
      error="Cannot read native node-radius data"; return false;
    }
    return PrepareCurve(api,radius,source,candidate.radiusData,candidate.radiusCurve,
                        candidate.radiusKeys,error,"radius");
  }

  static std::vector<std::string> ParameterParts(const std::string &path) {
    std::vector<std::string> result;
    size_t start=std::string("serializeData.").size();
    while (start<path.size()) {
      const auto end=path.find('.',start);
      result.push_back(path.substr(start,end-start));
      if (end==std::string::npos) break;
      start=end+1;
    }
    return result;
  }

  static bool ResolveReferencePath(void *root,void *rootClass,
                                   const std::vector<std::string> &parts,size_t count,
                                   void *&object,void *&klass,std::string &error,
                                   const std::string &label) {
    object=root; klass=rootClass;
    for (size_t i=0;i<count;++i) {
      void *field=EiemPhysicsField(klass,parts[i].c_str());
      void *type=field?il2cpp_field_get_type(field):nullptr;
      // IL2CPP_TYPE_CLASS. Author parameters never traverse arrays or inline
      // structs; the only editable inline struct is gravityDirection below.
      if (!field || !type || il2cpp_type_get_type(type)!=0x12) {
        error="Native Physics parameter path is not a reference object: "+label; return false;
      }
      void *expected=il2cpp_class_from_type(type),*child=nullptr;
      if (!expected || !EiemPhysicsReadField(object,field,child) || !child ||
          il2cpp_object_get_class(child)!=expected) {
        error="Cannot resolve native Physics parameter object: "+label; return false;
      }
      object=child; klass=expected;
    }
    return true;
  }

  static const EiemPhysicsAuthorParameter *FindParameter(
      const std::vector<EiemPhysicsAuthorParameter> &parameters,const std::string &path) {
    for (const auto &parameter:parameters) if (parameter.path==path) return &parameter;
    return nullptr;
  }

  static bool DecodeCurve(const std::vector<EiemPhysicsAuthorParameter> &parameters,
                          const std::string &prefix,EiemPhysicsAuthorRadius &curve,
                          std::set<std::string> &handled,std::string &error) {
    std::map<size_t,std::map<std::string,const EiemPhysicsAuthorParameter *>> keys;
    const auto *base=FindParameter(parameters,prefix+".value");
    const auto *use=FindParameter(parameters,prefix+".useCurve");
    bool present=false;
    for (const auto &parameter:parameters) {
      if (parameter.path.compare(0,prefix.size()+1,prefix+".")) continue;
      present=true; handled.insert(parameter.path);
      const auto suffix=parameter.path.substr(prefix.size()+1);
      if (suffix=="value" || suffix=="useCurve" || suffix=="curve.m_PreInfinity" ||
          suffix=="curve.m_PostInfinity" || suffix=="curve.m_RotationOrder") continue;
      constexpr const char marker[]="curve.m_Curve.";
      if (suffix.compare(0,sizeof(marker)-1,marker)) {
        error="Unsupported native Physics curve field: "+parameter.path; return false;
      }
      const auto indexStart=sizeof(marker)-1,indexEnd=suffix.find('.',indexStart);
      if (indexEnd==std::string::npos) { error="Invalid native Physics curve key: "+parameter.path; return false; }
      const auto indexText=suffix.substr(indexStart,indexEnd-indexStart);
      if (indexText.empty() || indexText.size()>3 ||
          indexText.find_first_not_of("0123456789")!=std::string::npos) {
        error="Invalid native Physics curve index: "+parameter.path; return false;
      }
      const size_t index=(size_t)strtoul(indexText.c_str(),nullptr,10);
      if (index>=64) { error="Native Physics curve has too many keys: "+prefix; return false; }
      keys[index][suffix.substr(indexEnd+1)]=&parameter;
    }
    if (!present) return true;
    if (!base || !base->floating || !use || use->floating ||
        (use->integerValue!=0 && use->integerValue!=1) || keys.size()<2 || keys.size()>64) {
      error="Native Physics curve is incomplete: "+prefix; return false;
    }
    curve={}; curve.value=(float)base->floatingValue; curve.useCurve=(uint8_t)use->integerValue;
    curve.keys.clear(); curve.keys.reserve(keys.size());
    size_t expected=0; float previous=-1;
    for (const auto &entry:keys) {
      if (entry.first!=expected++) { error="Native Physics curve key indices are not contiguous: "+prefix; return false; }
      const auto &fields=entry.second;
      auto floating=[&](const char *name,float &out) {
        auto found=fields.find(name);
        if (found==fields.end() || !found->second->floating) return false;
        out=(float)found->second->floatingValue; return true;
      };
      EiemPhysicsAuthorCurveKey key;
      const auto mode=fields.find("weightedMode");
      if (!floating("time",key.time) || !floating("value",key.value) ||
          !floating("inSlope",key.inSlope) || !floating("outSlope",key.outSlope) ||
          !floating("inWeight",key.inWeight) || !floating("outWeight",key.outWeight) ||
          mode==fields.end() || mode->second->floating || mode->second->integerValue<0 ||
          mode->second->integerValue>3 || fields.size()!=7 || key.time<0 || key.time>1 ||
          key.time<=previous || key.inWeight<0 || key.inWeight>1 ||
          key.outWeight<0 || key.outWeight>1) {
        error="Invalid native Physics curve key: "+prefix; return false;
      }
      key.weightedMode=(uint32_t)mode->second->integerValue;
      previous=key.time; curve.keys.push_back(key);
    }
    const auto *pre=FindParameter(parameters,prefix+".curve.m_PreInfinity");
    const auto *post=FindParameter(parameters,prefix+".curve.m_PostInfinity");
    const auto *rotation=FindParameter(parameters,prefix+".curve.m_RotationOrder");
    if ((pre && pre->floating) || (post && post->floating) || (rotation && rotation->floating)) {
      error="Invalid native Physics curve metadata: "+prefix; return false;
    }
    if (pre) curve.preInfinity=(int32_t)pre->integerValue;
    if (post) curve.postInfinity=(int32_t)post->integerValue;
    if (rotation) curve.rotationOrder=(int32_t)rotation->integerValue;
    return true;
  }

  static bool WritePrimitive(const EiemPhysicsAuthorParameter &parameter,void *object,
                             void *field,std::string &error) {
    void *type=il2cpp_field_get_type(field);
    const std::string typeName=EiemPhysicsTypeName(type);
    if (parameter.floating) {
      if (typeName!="System.Single") {
        error="Native Physics floating parameter type changed: "+parameter.path; return false;
      }
      const float value=(float)parameter.floatingValue; float actual=0;
      if (!std::isfinite(value) || !Write(object,field,(void *)&value) ||
          !EiemPhysicsReadField(object,field,actual) || actual!=value) {
        error="Native Physics floating parameter readback mismatch: "+parameter.path; return false;
      }
      return true;
    }
    if (typeName=="System.Boolean") {
      if (parameter.integerValue!=0 && parameter.integerValue!=1) {
        error="Native Physics Boolean parameter is not zero or one: "+parameter.path; return false;
      }
      const bool value=parameter.integerValue!=0; bool actual=!value;
      if (!Write(object,field,(void *)&value) || !EiemPhysicsReadField(object,field,actual) || actual!=value) {
        error="Native Physics Boolean parameter readback mismatch: "+parameter.path; return false;
      }
      return true;
    }
    if (typeName=="System.Int32" || typeName=="System.UInt32" ||
        (type && il2cpp_type_get_type(type)==0x11)) {
      if (il2cpp_type_get_type(type)==0x11) {
        void *enumClass=il2cpp_class_from_type(type); uint32_t alignment=0;
        if (!enumClass || il2cpp_class_value_size(enumClass,&alignment)!=4) {
          error="Native Physics enum layout changed: "+parameter.path; return false;
        }
      }
      const int32_t value=(int32_t)parameter.integerValue; int32_t actual=~value;
      if (!Write(object,field,(void *)&value) || !EiemPhysicsReadField(object,field,actual) || actual!=value) {
        error="Native Physics integer parameter readback mismatch: "+parameter.path; return false;
      }
      return true;
    }
    error="Unsupported native Physics primitive type "+typeName+": "+parameter.path; return false;
  }

  static bool ApplyNativeParameters(const EiemPhysicsConfigApi &api,
                                    const std::vector<EiemPhysicsAuthorParameter> &parameters,
                                    EiemPhysicsConfigGroup &candidate,std::string &error) {
    if (parameters.empty()) return true;
    std::set<std::string> handled;
    const char *curves[]={
      "serializeData.damping","serializeData.radius",
      "serializeData.distanceConstraint.stiffness",
      "serializeData.angleRestorationConstraint.stiffness",
      "serializeData.angleLimitConstraint.limitAngle",
      "serializeData.motionConstraint.maxDistance",
      "serializeData.motionConstraint.backstopDistance",
      "serializeData.colliderCollisionConstraint.limitDistance",
      "serializeData.selfCollisionConstraint.surfaceThickness"};
    for (const char *prefix:curves) {
      EiemPhysicsAuthorRadius curve; curve.keys.clear();
      if (!DecodeCurve(parameters,prefix,curve,handled,error)) return false;
      if (curve.keys.empty() || !strcmp(prefix,"serializeData.radius")) continue;
      const auto parts=ParameterParts(prefix);
      void *curveData=nullptr,*curveClass=nullptr;
      if (!ResolveReferencePath(candidate.data.Target(),api.data,parts,parts.size(),
                                curveData,curveClass,error,prefix) || curveClass!=api.curveData)
        return false;
      candidate.parameterObjects.emplace_back(); candidate.parameterCurves.emplace_back();
      candidate.parameterKeys.emplace_back();
      if (!PrepareCurve(api,curveData,curve,candidate.parameterObjects.back(),
                        candidate.parameterCurves.back(),candidate.parameterKeys.back(),error,prefix))
        return false;
    }
    const char *axes[]={"serializeData.gravityDirection.x","serializeData.gravityDirection.y",
                        "serializeData.gravityDirection.z"};
    const EiemPhysicsAuthorParameter *axis[3]={
      FindParameter(parameters,axes[0]),FindParameter(parameters,axes[1]),
      FindParameter(parameters,axes[2])};
    if (axis[0] || axis[1] || axis[2]) {
      if (!axis[0] || !axis[1] || !axis[2] || !axis[0]->floating ||
          !axis[1]->floating || !axis[2]->floating) {
        error="Native Physics gravityDirection requires three floating components"; return false;
      }
      Vector3 value{(float)axis[0]->floatingValue,(float)axis[1]->floatingValue,
                    (float)axis[2]->floatingValue},actual{};
      void *field=EiemPhysicsField(api.data,"gravityDirection");
      if (!field) {
        error="Native Physics gravityDirection field is missing or ambiguous"; return false;
      }
      void *fieldType=il2cpp_field_get_type(field);
      const std::string typeName=EiemPhysicsTypeName(fieldType);
      if (typeName!="UnityEngine.Vector3" &&
          typeName!="Unity.Mathematics.float3") {
        error="Native Physics gravityDirection type changed: "+typeName; return false;
      }
      void *fieldClass=il2cpp_class_from_type(fieldType);
      uint32_t fieldAlign=0;
      const int32_t fieldSize=fieldClass?
          il2cpp_class_value_size(fieldClass,&fieldAlign):0;
      if (fieldSize!=sizeof(Vector3) || fieldAlign!=alignof(float)) {
        char details[256]={};
        snprintf(details,sizeof(details),
                 "Native Physics gravityDirection layout changed: type=%s size=%d align=%u",
                 typeName.c_str(),fieldSize,fieldAlign);
        error=details; return false;
      }
      if (!Write(candidate.data.Target(),field,&value)) {
        error="Native Physics gravityDirection write failed"; return false;
      }
      if (!EiemPhysicsReadField(candidate.data.Target(),field,actual)) {
        error="Native Physics gravityDirection readback failed"; return false;
      }
      if (actual.x!=value.x || actual.y!=value.y || actual.z!=value.z) {
        char details[256]={};
        snprintf(details,sizeof(details),
                 "Native Physics gravityDirection readback mismatch expected=(%.9g,%.9g,%.9g) actual=(%.9g,%.9g,%.9g)",
                 value.x,value.y,value.z,actual.x,actual.y,actual.z);
        error=details; return false;
      }
      handled.insert(axes[0]); handled.insert(axes[1]); handled.insert(axes[2]);
    }
    for (const auto &parameter:parameters) {
      if (handled.count(parameter.path)) continue;
      const auto parts=ParameterParts(parameter.path);
      if (parts.empty() || std::any_of(parts.begin(),parts.end(),[](const std::string &part) {
            return !part.empty() && part.find_first_not_of("0123456789")==std::string::npos;
          })) {
        error="Unsupported native Physics parameter path: "+parameter.path; return false;
      }
      void *object=nullptr,*klass=nullptr;
      if (!ResolveReferencePath(candidate.data.Target(),api.data,parts,parts.size()-1,
                                object,klass,error,parameter.path)) return false;
      void *field=EiemPhysicsField(klass,parts.back().c_str());
      if (!field || !WritePrimitive(parameter,object,field,error)) return false;
    }
    return true;
  }

 public:
  const std::vector<EiemPhysicsConfigGroup> &Groups() const { return groups_; }
  // These refs keep managed Transforms reachable during preparation only.
  // They do not keep Unity native objects alive or grant release authority.
  bool Prepare(const EiemPhysicsConfigApi &api,const EiemPhysicsDocument &document,
               const std::unordered_map<std::string,void *> &bindings,std::string &error) {
    auto fail=[&](const std::string &why) { error=why; return false; };
    if (!EiemOnUnityThread()) return fail("Detached physics configuration requires Unity thread");
    if (!api.dataCtor || !api.data2Ctor || !api.listCtor || !api.listAdd || !api.getParent ||
        !api.getChildCount || !api.getChild ||
        !api.rootsField || !api.ignoresField || !api.clothTypeField ||
        !api.connectionModeField || !api.listCount || !api.listItem || !api.transform ||
        std::find(api.scalars.begin(),api.scalars.end(),nullptr)!=api.scalars.end())
      return fail("Resolve detached physics configuration APIs before preparation");
    if (document.version!=1 && document.version!=3 && document.version!=4 && document.version!=5)
      return fail("Native source v2 graph construction is not implemented; no author coercion");
    if (!EiemValidatePhysicsAuthor(document,error)) return false;
    std::vector<EiemPhysicsConfigGroup> staged;
    std::vector<std::vector<size_t>> rootIndices;
    std::vector<std::vector<size_t>> ignoreIndices;
    // Validate and retain the complete request before allocating any configs.
    for (const auto &group:document.groups) {
      EiemPhysicsConfigGroup candidate; candidate.id=group.id; candidate.name=group.name;
      candidate.authorNodes=group.nodes;
      std::unordered_map<std::string,size_t> indices;
      std::set<void *> unique;
      for (const auto &node:group.nodes) {
        auto found=bindings.find(node.bone);
        if (found==bindings.end() || !found->second || !unique.insert(found->second).second ||
            il2cpp_object_get_class(found->second)!=api.transform)
          return fail("Missing, aliased or wrong-type physics Transform: "+node.bone);
        auto ref=EiemUnityRef::Capture(found->second,false);
        if (!ref || ref.Status()!=1) return fail("Physics Transform is unavailable: "+node.bone);
        indices.emplace(node.bone,candidate.transforms.size()); candidate.transforms.push_back(std::move(ref));
      }
      std::vector<size_t> roots;
      std::vector<size_t> ignored;
      for (size_t i=0;i<group.nodes.size();++i) {
        const auto &path=group.nodes[i].bone;
        const auto slash=path.find_last_of('/');
        auto parent=indices.find(slash==std::string::npos?"":path.substr(0,slash));
        if (path.empty() || parent==indices.end()) {roots.push_back(i); continue;}
        if (group.nodes[i].role==0)
          return fail("Only the chain root can be a fixed native BoneCloth node: "+path);
        if (group.nodes[i].role==2) ignored.push_back(i);
        void *actual=nullptr;
        if (!InvokeChecked(api.getParent,candidate.transforms[i].Target(),nullptr,&actual) ||
            actual!=candidate.transforms[parent->second].Target())
          return fail("Physics Transform parent does not match the author chain: "+path);
      }
      std::set<void *> boundary;
      for (size_t i=0;i<group.nodes.size();++i) {
        // An explicit IGNORE node already excludes its complete subtree.
        if (group.nodes[i].role==2) continue;
        void *boxed=nullptr;
        if (!InvokeChecked(api.getChildCount,candidate.transforms[i].Target(),nullptr,&boxed) || !boxed)
          return fail("Cannot enumerate Physics Transform children: "+group.nodes[i].bone);
        int32_t childCount=-1;
        if (!EiemPhysicsUnbox(boxed,childCount) || childCount<0 || childCount>16384)
          return fail("Invalid Physics Transform child count: "+group.nodes[i].bone);
        for (int32_t childIndex=0;childIndex<childCount;++childIndex) {
          void *child=nullptr,*args[]={&childIndex};
          if (!InvokeChecked(api.getChild,candidate.transforms[i].Target(),args,&child) || !child ||
              il2cpp_object_get_class(child)!=api.transform)
            return fail("Cannot read Physics Transform child: "+group.nodes[i].bone);
          if (unique.find(child)!=unique.end() || !boundary.insert(child).second) continue;
          auto ref=EiemUnityRef::Capture(child,false);
          if (!ref || ref.Status()!=1)
            return fail("Physics boundary Transform is unavailable: "+group.nodes[i].bone);
          candidate.boundaryIgnores.push_back(std::move(ref));
        }
      }
      rootIndices.push_back(std::move(roots)); ignoreIndices.push_back(std::move(ignored));
      staged.push_back(std::move(candidate));
    }
    for (size_t g=0;g<staged.size();++g) {
      auto &candidate=staged[g];
      if (!New(api.data,api.dataCtor,candidate.data) ||
          !New(api.data2,api.data2Ctor,candidate.data2))
        return fail("Detached native configuration constructor failed");
      if (document.version>=3 && !PrepareRadius(api,document.groups[g].radius,candidate,error))
        return false;
      // ClothSerializeData::.ctor creates its two List<Transform> fields and
      // stores them with the IL2CPP write barrier.  Keep and populate those
      // exact lists; replacing the references via il2cpp_field_set_value did
      // not survive readback in the v63 game process.
      void *roots=nullptr,*ignores=nullptr;
      if (!EiemPhysicsReadField(candidate.data.Target(),api.rootsField,roots) ||
          !EiemPhysicsReadField(candidate.data.Target(),api.ignoresField,ignores) ||
          !roots || !ignores || roots==ignores ||
          il2cpp_object_get_class(roots)!=api.rootList ||
          il2cpp_object_get_class(ignores)!=api.rootList ||
          !(candidate.roots=EiemUnityRef::Capture(roots,false)) ||
          !(candidate.ignores=EiemUnityRef::Capture(ignores,false)))
        return fail("ClothSerializeData constructor did not provide distinct Transform lists");
      int32_t initialRoots=-1,initialIgnores=-1;
      if (!EiemPhysicsReadValue(roots,api.listCount,initialRoots) || initialRoots!=0 ||
          !EiemPhysicsReadValue(ignores,api.listCount,initialIgnores) || initialIgnores!=0)
        return fail("New ClothSerializeData Transform lists are not empty");
      // The v1 authoring contract describes a BoneCloth hierarchy.  Current
      // game assets and GenerateBoneClothSelection use these exact enum values.
      int32_t clothType=1,connectionMode=0,actualEnum=-1;
      if (!Write(candidate.data.Target(),api.clothTypeField,&clothType) ||
          !EiemPhysicsReadField(candidate.data.Target(),api.clothTypeField,actualEnum) ||
          actualEnum!=clothType ||
          !Write(candidate.data.Target(),api.connectionModeField,&connectionMode) ||
          !EiemPhysicsReadField(candidate.data.Target(),api.connectionModeField,actualEnum) ||
          actualEnum!=connectionMode)
        return fail("Detached BoneCloth enum readback mismatch");
      for (size_t i=0;i<api.scalars.size();++i) {
        float value=document.groups[g].parameters[i],actual=0;
        if (!Write(candidate.data.Target(),api.scalars[i],&value) ||
            !EiemPhysicsReadField(candidate.data.Target(),api.scalars[i],actual) || actual!=value)
          return fail(std::string("Detached physics scalar readback mismatch: ")+api.ScalarName(i));
      }
      if (document.version>=4 &&
          !ApplyNativeParameters(api,document.groups[g].nativeParameters,candidate,error))
        return false;
      void *result=nullptr;
      for (size_t rootIndex:rootIndices[g]) {
        void *root=candidate.transforms[rootIndex].Target(); void *args[]={root};
        if (!InvokeChecked(api.listAdd,candidate.roots.Target(),args,&result))
          return fail("Cannot populate detached rootBones list");
      }
      int32_t count=-1;
      if (!EiemPhysicsReadValue(candidate.roots.Target(),api.listCount,count) ||
          count!=(int32_t)rootIndices[g].size())
        return fail("Detached rootBones list readback mismatch");
      for (int32_t i=0;i<count;++i) {
        void *actual=nullptr; void *args[]={&i};
        if (!InvokeChecked(api.listItem,candidate.roots.Target(),args,&actual) ||
            actual!=candidate.transforms[rootIndices[g][i]].Target())
          return fail("Detached rootBones member mismatch");
      }
      for (size_t ignored:ignoreIndices[g]) {
        void *node=candidate.transforms[ignored].Target(); void *args[]={node};
        if (!InvokeChecked(api.listAdd,candidate.ignores.Target(),args,&result))
          return fail("Cannot populate detached ignoreFromRootBones list");
      }
      for (const auto &ignored:candidate.boundaryIgnores) {
        void *node=ignored.Target(); void *args[]={node};
        if (!InvokeChecked(api.listAdd,candidate.ignores.Target(),args,&result))
          return fail("Cannot populate detached boundary ignore list");
      }
      count=-1;
      if (!EiemPhysicsReadValue(candidate.ignores.Target(),api.listCount,count) ||
          count!=(int32_t)(ignoreIndices[g].size()+candidate.boundaryIgnores.size()))
        return fail("Detached ignoreFromRootBones list readback mismatch");
      for (int32_t i=0;i<count;++i) {
        void *item=nullptr; void *args[]={&i};
        void *expected=i<(int32_t)ignoreIndices[g].size()
          ? candidate.transforms[ignoreIndices[g][i]].Target()
          : candidate.boundaryIgnores[(size_t)i-ignoreIndices[g].size()].Target();
        if (!InvokeChecked(api.listItem,candidate.ignores.Target(),args,&item) ||
            item!=expected)
          return fail("Detached ignoreFromRootBones member mismatch");
      }
      void *actualRoots=nullptr,*actualIgnores=nullptr;
      if (!EiemPhysicsReadField(candidate.data.Target(),api.rootsField,actualRoots) ||
          !EiemPhysicsReadField(candidate.data.Target(),api.ignoresField,actualIgnores) ||
          actualRoots!=candidate.roots.Target() || actualIgnores!=candidate.ignores.Target())
        return fail("ClothSerializeData constructor-owned Transform list changed during preparation");
    }
    for (const auto &group:staged) {
      for (const auto &node:group.transforms)
        if (node.Status()!=1) return fail("Physics Transform expired during detached preparation");
      for (const auto &node:group.boundaryIgnores)
        if (node.Status()!=1) return fail("Physics boundary Transform expired during detached preparation");
    }
    // Only managed, unattached drafts are replaced. No process or component
    // has ever consumed them, and no native bone ownership is released here.
    groups_=std::move(staged); error.clear(); return true;
  }
};
