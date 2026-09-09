#pragma once

// One-shot development probe for the native BeyondBoneCloth factory.  It is
// fed by a real Typhoea Mesh match and a registered live model, but it does not
// consume Render.physics and does not participate in production ownership.
// The probe deliberately keeps every created object until process exit so this
// experiment cannot mistake a managed return value for a native completion
// fence.
static constexpr const char *EiemPhysicsFactoryProbeTag =
    "[DEBUG-PHYS-FACTORY-v66]";

struct EiemPhysicsFactoryProbeApi {
  void *clothClass=nullptr,*processClass=nullptr,*setupClass=nullptr;
  void *selectionClass=nullptr,*gameObjectClass=nullptr,*transformClass=nullptr;
  void *gameObjectCtor=nullptr,*gameObjectTransform=nullptr,*gameObjectActive=nullptr;
  void *addComponent=nullptr,*transformSetParent=nullptr,*setLocalPosition=nullptr;
  void *setLocalRotation=nullptr,*setLocalScale=nullptr;
  void *getLocalPosition=nullptr,*getLocalRotation=nullptr;
  void *getWorldPosition=nullptr,*getWorldRotation=nullptr;
  void *disableAutoBuild=nullptr,*setSerializeData=nullptr,*getSerializeData=nullptr;
  void *getSerializeData2=nullptr,*buildAndRun=nullptr;
  void *processField=nullptr;
  void *processValid=nullptr,*processRunning=nullptr,*processTeamId=nullptr;
  void *processBuildField=nullptr,*processDestroyField=nullptr;
  void *processDestroyInternalField=nullptr,*processAnimatorField=nullptr;
  void *processSetupField=nullptr,*setupTransformsField=nullptr,*setupSkinCountField=nullptr;
  void *selectionField=nullptr,*selectionCount=nullptr,*selectionUserEditField=nullptr;
  EiemPhysicsConfigApi config;

  bool Resolve(void **assemblies,size_t count,std::string &error) {
    *this={};
    if (!config.Resolve(assemblies,count,error)) return false;
    clothClass=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","BeyondBoneCloth");
    processClass=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","ClothProcess");
    setupClass=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","RenderSetupData");
    selectionClass=EiemPhysicsClass(assemblies,count,"BeyondDynamicBone.dll","BeyondDynamicBone","SelectionData");
    gameObjectClass=EiemPhysicsClass(assemblies,count,"UnityEngine.CoreModule.dll","UnityEngine","GameObject");
    transformClass=EiemPhysicsClass(assemblies,count,"UnityEngine.CoreModule.dll","UnityEngine","Transform");
    gameObjectCtor=EiemPhysicsMethod(gameObjectClass,".ctor","System.Void",false,"System.String");
    gameObjectTransform=EiemPhysicsMethod(gameObjectClass,"get_transform","UnityEngine.Transform",false);
    gameObjectActive=EiemPhysicsMethod(gameObjectClass,"SetActive","System.Void",false,"System.Boolean");
    addComponent=EiemPhysicsMethod(gameObjectClass,"AddComponent","UnityEngine.Component",false,"System.Type");
    transformSetParent=EiemPhysicsMethod(transformClass,"SetParent","System.Void",false,"UnityEngine.Transform","System.Boolean");
    setLocalPosition=EiemPhysicsMethod(transformClass,"set_localPosition","System.Void",false,"UnityEngine.Vector3");
    setLocalRotation=EiemPhysicsMethod(transformClass,"set_localRotation","System.Void",false,"UnityEngine.Quaternion");
    setLocalScale=EiemPhysicsMethod(transformClass,"set_localScale","System.Void",false,"UnityEngine.Vector3");
    getLocalPosition=EiemPhysicsMethod(transformClass,"get_localPosition","UnityEngine.Vector3",false);
    getLocalRotation=EiemPhysicsMethod(transformClass,"get_localRotation","UnityEngine.Quaternion",false);
    getWorldPosition=EiemPhysicsMethod(transformClass,"get_position","UnityEngine.Vector3",false);
    getWorldRotation=EiemPhysicsMethod(transformClass,"get_rotation","UnityEngine.Quaternion",false);
    disableAutoBuild=EiemPhysicsMethod(clothClass,"DisableAutoBuild","System.Void",false);
    setSerializeData=EiemPhysicsMethod(clothClass,"set_SerializeData","System.Void",false,"BeyondDynamicBone.ClothSerializeData");
    getSerializeData=EiemPhysicsMethod(clothClass,"get_SerializeData","BeyondDynamicBone.ClothSerializeData",false);
    getSerializeData2=EiemPhysicsMethod(clothClass,"GetSerializeData2","BeyondDynamicBone.ClothSerializeData2",false);
    buildAndRun=EiemPhysicsMethod(clothClass,"BuildAndRun","System.Boolean",false);
    processField=EiemPhysicsField(clothClass,"process","BeyondDynamicBone.ClothProcess");
    processValid=EiemPhysicsMethod(processClass,"IsValid","System.Boolean",false);
    processRunning=EiemPhysicsMethod(processClass,"IsRunning","System.Boolean",false);
    processTeamId=EiemPhysicsMethod(processClass,"get_TeamId","System.Int32",false);
    processBuildField=EiemPhysicsField(processClass,"isBuild","System.Boolean");
    processDestroyField=EiemPhysicsField(processClass,"isDestory","System.Boolean");
    processDestroyInternalField=EiemPhysicsField(processClass,"isDestoryInternal","System.Boolean");
    processAnimatorField=EiemPhysicsField(processClass,"interlockingAnimator","UnityEngine.Animator");
    processSetupField=EiemPhysicsField(processClass,"boneClothSetupData","BeyondDynamicBone.RenderSetupData");
    setupTransformsField=EiemPhysicsField(setupClass,"transformList","System.Collections.Generic.List<UnityEngine.Transform>");
    setupSkinCountField=EiemPhysicsField(setupClass,"skinBoneCount","System.Int32");
    selectionField=EiemPhysicsField(config.data2,"selectionData","BeyondDynamicBone.SelectionData");
    selectionCount=EiemPhysicsMethod(selectionClass,"get_Count","System.Int32",false);
    selectionUserEditField=EiemPhysicsField(selectionClass,"userEdit","System.Boolean");
    void *required[]={clothClass,processClass,setupClass,selectionClass,gameObjectClass,
      transformClass,gameObjectCtor,gameObjectTransform,gameObjectActive,addComponent,
      transformSetParent,setLocalPosition,setLocalRotation,setLocalScale,disableAutoBuild,
      getLocalPosition,getLocalRotation,getWorldPosition,getWorldRotation,
      setSerializeData,getSerializeData,getSerializeData2,buildAndRun,
      processField,processValid,processRunning,processTeamId,processBuildField,
      processDestroyField,processDestroyInternalField,processAnimatorField,
      processSetupField,setupTransformsField,setupSkinCountField,selectionField,
      selectionCount,selectionUserEditField};
    for (void *entry:required) if (!entry) {
      error="Native factory metadata contract incomplete or ambiguous"; *this={}; return false;
    }
    error.clear(); return true;
  }
};

struct EiemPhysicsFactoryVector3 { float x,y,z; };
struct EiemPhysicsFactoryQuaternion { float x,y,z,w; };

struct EiemPhysicsFactoryMatchedRenderer {
  void *renderer=nullptr;
  EiemUnityRef reference;
};

struct EiemPhysicsFactoryProbeState {
  bool attempted=false,built=false;
  uint32_t polls=0;
  void *model=nullptr,*renderer=nullptr,*animator=nullptr,*component=nullptr,*process=nullptr;
  EiemUnityRef modelRef,rendererRef,animatorRef,componentRef,processRef,componentData2Ref;
  std::vector<EiemUnityRef> objects,transforms;
  std::array<EiemPhysicsFactoryVector3,3> baselineLocalPositions{},baselineWorldPositions{};
  std::array<EiemPhysicsFactoryQuaternion,3> baselineLocalRotations{},baselineWorldRotations{};
  bool poseBaselineReady=false;
  EiemPhysicsConfigDraft config;
  EiemPhysicsFactoryProbeApi api;
};

static EiemPhysicsFactoryProbeState s_eiemPhysicsFactoryProbe;
static EiemPhysicsFactoryMatchedRenderer s_eiemPhysicsFactoryRenderers[64]={};
static size_t s_eiemPhysicsFactoryRendererCount=0;
static ULONGLONG s_eiemPhysicsFactoryNextPeriodicPoll=0;

static bool EiemPhysicsFactoryNewObject(
    EiemPhysicsFactoryProbeState &state,const char *name,void *parent,
    EiemPhysicsFactoryVector3 position,void **objectOut,void **transformOut) {
  auto &api=state.api;
  if (!name || !parent || !il2cpp_object_new || !il2cpp_string_new) return false;
  void *object=il2cpp_object_new(api.gameObjectClass),*result=nullptr;
  auto objectRef=EiemUnityRef::Capture(object,false);
  void *nameObject=il2cpp_string_new(name); void *ctorArgs[]={nameObject};
  if (!objectRef || !nameObject ||
      !InvokeChecked(api.gameObjectCtor,object,ctorArgs,&result)) return false;
  void *transform=nullptr;
  if (!InvokeChecked(api.gameObjectTransform,object,nullptr,&transform) || !transform)
    return false;
  auto transformRef=EiemUnityRef::Capture(transform,false);
  bool worldPositionStays=false; void *parentArgs[]={parent,&worldPositionStays};
  EiemPhysicsFactoryQuaternion rotation={0,0,0,1};
  EiemPhysicsFactoryVector3 scale={1,1,1};
  void *positionArgs[]={&position},*rotationArgs[]={&rotation},*scaleArgs[]={&scale};
  if (!transformRef ||
      !InvokeChecked(api.transformSetParent,transform,parentArgs,&result) ||
      !InvokeChecked(api.setLocalPosition,transform,positionArgs,&result) ||
      !InvokeChecked(api.setLocalRotation,transform,rotationArgs,&result) ||
      !InvokeChecked(api.setLocalScale,transform,scaleArgs,&result)) return false;
  state.objects.push_back(std::move(objectRef));
  state.transforms.push_back(std::move(transformRef));
  if (objectOut) *objectOut=object;
  if (transformOut) *transformOut=transform;
  return true;
}

static float EiemPhysicsFactoryVectorDeltaSq(
    EiemPhysicsFactoryVector3 a,EiemPhysicsFactoryVector3 b) {
  const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
  return x*x+y*y+z*z;
}

static float EiemPhysicsFactoryQuaternionDeltaSq(
    EiemPhysicsFactoryQuaternion a,EiemPhysicsFactoryQuaternion b) {
  const float x0=a.x-b.x,y0=a.y-b.y,z0=a.z-b.z,w0=a.w-b.w;
  const float x1=a.x+b.x,y1=a.y+b.y,z1=a.z+b.z,w1=a.w+b.w;
  const float direct=x0*x0+y0*y0+z0*z0+w0*w0;
  const float negated=x1*x1+y1*y1+z1*z1+w1*w1;
  return direct<negated?direct:negated;
}

static bool EiemPhysicsFactoryReadPose(
    const EiemPhysicsFactoryProbeApi &api,void *transform,
    EiemPhysicsFactoryVector3 &localPosition,
    EiemPhysicsFactoryQuaternion &localRotation,
    EiemPhysicsFactoryVector3 &worldPosition,
    EiemPhysicsFactoryQuaternion &worldRotation) {
  return transform &&
      EiemPhysicsReadValue(transform,api.getLocalPosition,localPosition) &&
      EiemPhysicsReadValue(transform,api.getLocalRotation,localRotation) &&
      EiemPhysicsReadValue(transform,api.getWorldPosition,worldPosition) &&
      EiemPhysicsReadValue(transform,api.getWorldRotation,worldRotation);
}

static bool EiemPhysicsFactoryCapturePoseBaseline(
    EiemPhysicsFactoryProbeState &state) {
  if (state.transforms.size()<4) return false;
  for (size_t i=0;i<3;++i) {
    void *transform=state.transforms[i+1].Target();
    if (state.transforms[i+1].Status()!=1 ||
        !EiemPhysicsFactoryReadPose(state.api,transform,
            state.baselineLocalPositions[i],state.baselineLocalRotations[i],
            state.baselineWorldPositions[i],state.baselineWorldRotations[i])) return false;
  }
  state.poseBaselineReady=true;
  return true;
}

static void EiemPhysicsFactoryMeasurePose(
    EiemPhysicsFactoryProbeState &state,int &readNodes,int &localMoved,
    int &worldMoved,float &maxLocalPositionDeltaSq,float &maxLocalRotationDeltaSq,
    float &maxWorldPositionDeltaSq,float &maxWorldRotationDeltaSq) {
  readNodes=localMoved=worldMoved=0;
  maxLocalPositionDeltaSq=maxLocalRotationDeltaSq=0;
  maxWorldPositionDeltaSq=maxWorldRotationDeltaSq=0;
  if (!state.poseBaselineReady || state.transforms.size()<4) return;
  constexpr float epsilon=1.0e-10f;
  for (size_t i=0;i<3;++i) {
    void *transform=state.transforms[i+1].Target();
    EiemPhysicsFactoryVector3 localPosition{},worldPosition{};
    EiemPhysicsFactoryQuaternion localRotation{},worldRotation{};
    if (state.transforms[i+1].Status()!=1 ||
        !EiemPhysicsFactoryReadPose(state.api,transform,localPosition,localRotation,
                                    worldPosition,worldRotation)) continue;
    ++readNodes;
    const float localPositionDelta=EiemPhysicsFactoryVectorDeltaSq(
        localPosition,state.baselineLocalPositions[i]);
    const float localRotationDelta=EiemPhysicsFactoryQuaternionDeltaSq(
        localRotation,state.baselineLocalRotations[i]);
    const float worldPositionDelta=EiemPhysicsFactoryVectorDeltaSq(
        worldPosition,state.baselineWorldPositions[i]);
    const float worldRotationDelta=EiemPhysicsFactoryQuaternionDeltaSq(
        worldRotation,state.baselineWorldRotations[i]);
    if (localPositionDelta>epsilon || localRotationDelta>epsilon) ++localMoved;
    if (worldPositionDelta>epsilon || worldRotationDelta>epsilon) ++worldMoved;
    if (localPositionDelta>maxLocalPositionDeltaSq) maxLocalPositionDeltaSq=localPositionDelta;
    if (localRotationDelta>maxLocalRotationDeltaSq) maxLocalRotationDeltaSq=localRotationDelta;
    if (worldPositionDelta>maxWorldPositionDeltaSq) maxWorldPositionDeltaSq=worldPositionDelta;
    if (worldRotationDelta>maxWorldRotationDeltaSq) maxWorldRotationDeltaSq=worldRotationDelta;
  }
}

static bool EiemPhysicsFactoryRendererUnderModel(void *renderer,void *model) {
  if (!renderer || !model || !g_component_get_transform ||
      !g_gameObject_get_transform || !g_transform_get_parent) return false;
  void *cursor=Invoke(g_component_get_transform,renderer);
  void *root=Invoke(g_gameObject_get_transform,model);
  for (int depth=0;cursor && root && depth<256;++depth) {
    if (cursor==root) return true;
    cursor=Invoke(g_transform_get_parent,cursor);
  }
  return false;
}

static int EiemPhysicsFactoryRendererDepth(void *renderer,void *component) {
  if (!renderer || !component || !g_component_get_transform ||
      !g_transform_get_parent) return -1;
  void *cursor=Invoke(g_component_get_transform,renderer);
  void *root=Invoke(g_component_get_transform,component);
  for (int depth=0;cursor && root && depth<256;++depth) {
    if (cursor==root) return depth;
    cursor=Invoke(g_transform_get_parent,cursor);
  }
  return -1;
}

static void *EiemPhysicsFactoryComponents(void *model,void *klass) {
  if (!model || !klass || !g_gameObject_GetComponentsInChildren ||
      !il2cpp_class_get_type || !il2cpp_type_get_object) return nullptr;
  void *type=il2cpp_class_get_type(klass);
  void *typeObject=type?il2cpp_type_get_object(type):nullptr;
  bool includeInactive=true; void *args[]={typeObject,&includeInactive};
  return typeObject?Invoke(g_gameObject_GetComponentsInChildren,model,args):nullptr;
}

static void EiemPhysicsFactoryProbeRememberRenderer(void *renderer,
                                                     const char *asset) {
  if (!renderer || !asset || _stricmp(asset,"S_actor_typhoea_body_01_lod0"))
    return;
  for (size_t i=0;i<s_eiemPhysicsFactoryRendererCount;++i)
    if (s_eiemPhysicsFactoryRenderers[i].renderer==renderer &&
        s_eiemPhysicsFactoryRenderers[i].reference.Target()==renderer) return;
  if (s_eiemPhysicsFactoryRendererCount>=_countof(s_eiemPhysicsFactoryRenderers))
    return;
  auto &entry=s_eiemPhysicsFactoryRenderers[s_eiemPhysicsFactoryRendererCount++];
  entry.renderer=renderer; entry.reference=EiemUnityRef::Capture(renderer);
  Log("%s matched-renderer renderer=%p asset=%s",EiemPhysicsFactoryProbeTag,
      renderer,asset);
}

static void EiemPhysicsFactoryProbePoll(const char *stage) {
  auto &state=s_eiemPhysicsFactoryProbe;
  if (!state.component || state.polls>=32 || !EiemOnUnityThread()) return;
  ++state.polls;
  const int componentAlive=state.componentRef.Status();
  void *process=nullptr;
  bool isBuild=false,isDestroy=false,isDestroyInternal=false,valid=false,running=false;
  int32_t team=-1,skinBones=-1,selectionPoints=-1,setupTransforms=-1;
  bool userEdit=false; void *animator=nullptr,*setup=nullptr,*selection=nullptr;
  int probeTransforms=0;
  int poseNodes=0,localMoved=0,worldMoved=0;
  float localPositionDeltaSq=0,localRotationDeltaSq=0;
  float worldPositionDeltaSq=0,worldRotationDeltaSq=0;
  EiemPhysicsFactoryMeasurePose(state,poseNodes,localMoved,worldMoved,
      localPositionDeltaSq,localRotationDeltaSq,worldPositionDeltaSq,
      worldRotationDeltaSq);
  if (componentAlive==1 && EiemPhysicsReadField(state.component,state.api.processField,process) && process) {
    state.process=process;
    if (!state.processRef || state.processRef.Target()!=process)
      state.processRef=EiemUnityRef::Capture(process,false);
    EiemPhysicsReadField(process,state.api.processBuildField,isBuild);
    EiemPhysicsReadField(process,state.api.processDestroyField,isDestroy);
    EiemPhysicsReadField(process,state.api.processDestroyInternalField,isDestroyInternal);
    EiemPhysicsReadValue(process,state.api.processValid,valid);
    EiemPhysicsReadValue(process,state.api.processRunning,running);
    EiemPhysicsReadValue(process,state.api.processTeamId,team);
    EiemPhysicsReadField(process,state.api.processAnimatorField,animator);
    if (EiemPhysicsReadField(process,state.api.processSetupField,setup) && setup) {
      EiemPhysicsReadField(setup,state.api.setupSkinCountField,skinBones);
      void *list=nullptr;
      if (EiemPhysicsReadField(setup,state.api.setupTransformsField,list) && list) {
        void *listClass=il2cpp_object_get_class(list);
        void *countMethod=EiemPhysicsMethod(listClass,"get_Count","System.Int32",false);
        void *itemMethod=EiemPhysicsMethod(listClass,"get_Item","UnityEngine.Transform",false,"System.Int32");
        if (EiemPhysicsReadValue(list,countMethod,setupTransforms) && setupTransforms>=0 && setupTransforms<=4096) {
          for (int32_t i=0;i<setupTransforms && itemMethod;++i) {
            void *item=nullptr,*args[]={&i};
            if (!InvokeChecked(itemMethod,list,args,&item)) continue;
            for (size_t n=1;n<state.transforms.size();++n)
              if (state.transforms[n].Target()==item) {++probeTransforms;break;}
          }
        }
      }
    }
    void *data2=state.componentData2Ref.Target();
    if (data2 && EiemPhysicsReadField(data2,state.api.selectionField,selection) && selection) {
      EiemPhysicsReadValue(selection,state.api.selectionCount,selectionPoints);
      EiemPhysicsReadField(selection,state.api.selectionUserEditField,userEdit);
    }
  }
  Log("%s poll=%u stage=%s component=%p alive=%d process=%p isBuild=%d destroying=%d/%d valid=%d running=%d team=%d animator=%p expectedAnimator=%p sameAnimator=%d selection=%d userEdit=%d skinBones=%d setupTransforms=%d probeTransforms=%d poseNodes=%d localMoved=%d worldMoved=%d localPositionDeltaSq=%.9g localRotationDeltaSq=%.9g worldPositionDeltaSq=%.9g worldRotationDeltaSq=%.9g",
      EiemPhysicsFactoryProbeTag,state.polls,stage?stage:"unknown",state.component,
      componentAlive,process,isBuild?1:0,isDestroy?1:0,isDestroyInternal?1:0,
      valid?1:0,running?1:0,team,animator,state.animator,
      animator && animator==state.animator?1:0,selectionPoints,userEdit?1:0,
      skinBones,setupTransforms,probeTransforms,poseNodes,localMoved,worldMoved,
      localPositionDeltaSq,localRotationDeltaSq,worldPositionDeltaSq,
      worldRotationDeltaSq);
}

static void EiemPhysicsFactoryProbePeriodic(const char *stage) {
  auto &state=s_eiemPhysicsFactoryProbe;
  if (!state.component || state.polls>=32 || !EiemOnUnityThread()) return;
  const ULONGLONG now=GetTickCount64();
  if (now<s_eiemPhysicsFactoryNextPeriodicPoll) return;
  s_eiemPhysicsFactoryNextPeriodicPoll=now+500;
  EiemPhysicsFactoryProbePoll(stage);
}

static bool EiemPhysicsFactoryProbeBuild(void *renderer,void *model,void *animator,
                                         const char *ownerKind,const char *stage) {
  auto &state=s_eiemPhysicsFactoryProbe;
  state.attempted=true; state.model=model; state.renderer=renderer; state.animator=animator;
  state.modelRef=EiemUnityRef::Capture(model); state.rendererRef=EiemUnityRef::Capture(renderer);
  state.animatorRef=EiemUnityRef::Capture(animator);
  std::string error;
  void *domain=il2cpp_domain_get?il2cpp_domain_get():nullptr; size_t count=0;
  void **assemblies=domain && il2cpp_domain_get_assemblies
      ? il2cpp_domain_get_assemblies(domain,&count):nullptr;
  if (!assemblies || !count || !state.modelRef || !state.rendererRef ||
      !state.animatorRef || !state.api.Resolve(assemblies,count,error)) {
    Log("%s failed stage=resolve error=%s",EiemPhysicsFactoryProbeTag,
        error.empty()?"runtime APIs unavailable":error.c_str()); return false;
  }
  void *modelTransform=Invoke(state.api.gameObjectTransform,model);
  void *animatorTransform=Invoke(g_component_get_transform,animator);
  void *host=nullptr,*hostTransform=nullptr,*root=nullptr,*middle=nullptr,*tip=nullptr;
  if (!modelTransform || !animatorTransform ||
      !EiemPhysicsFactoryNewObject(state,"EIEM_PhysicsFactoryProbe",modelTransform,{0,0,0},&host,&hostTransform)) {
    Log("%s failed stage=create-host",EiemPhysicsFactoryProbeTag); return false;
  }
  bool active=false; void *activeArgs[]={&active}; void *result=nullptr;
  if (!InvokeChecked(state.api.gameObjectActive,host,activeArgs,&result) ||
      !EiemPhysicsFactoryNewObject(state,"EIEM_PhysicsProbeRoot",animatorTransform,{0,0,0},nullptr,&root) ||
      !EiemPhysicsFactoryNewObject(state,"EIEM_PhysicsProbeMiddle",root,{0,0.12f,0},nullptr,&middle) ||
      !EiemPhysicsFactoryNewObject(state,"EIEM_PhysicsProbeTip",middle,{0,0.12f,0},nullptr,&tip)) {
    Log("%s failed stage=create-chain",EiemPhysicsFactoryProbeTag); return false;
  }
  if (!EiemPhysicsFactoryCapturePoseBaseline(state)) {
    Log("%s failed stage=capture-pose-baseline",EiemPhysicsFactoryProbeTag); return false;
  }
  Log("%s pose-baseline nodes=3",EiemPhysicsFactoryProbeTag);

  EiemPhysicsDocument document; document.id=std::string(32,'6');
  document.skeleton="diagnostics/v66.skeleton";
  EiemPhysicsAuthorGroup group; group.id=std::string(32,'2'); group.name="v66 factory probe";
  group.nodes={{"Probe",0},{"Probe/Middle",1},{"Probe/Middle/Tip",1}};
  const float parameters[]={10.0f,0.1f,0.0f,1.0f,1.0f};
  std::copy(parameters,parameters+5,group.parameters); document.groups.push_back(group);
  std::unordered_map<std::string,void *> bindings={{"Probe",root},
    {"Probe/Middle",middle},{"Probe/Middle/Tip",tip}};
  EiemPhysicsConfigDraft draft;
  if (!draft.Prepare(state.api.config,document,bindings,error)) {
    Log("%s failed stage=prepare error=%s",EiemPhysicsFactoryProbeTag,error.c_str()); return false;
  }

  void *clothType=il2cpp_type_get_object(il2cpp_class_get_type(state.api.clothClass));
  auto typeRef=EiemUnityRef::Capture(clothType,false); void *component=nullptr;
  void *componentArgs[]={clothType};
  if (!typeRef || !InvokeChecked(state.api.addComponent,host,componentArgs,&component) || !component ||
      il2cpp_object_get_class(component)!=state.api.clothClass) {
    Log("%s failed stage=add-component",EiemPhysicsFactoryProbeTag); return false;
  }
  state.component=component; state.componentRef=EiemUnityRef::Capture(component,false);
  if (!state.componentRef ||
      !InvokeChecked(state.api.disableAutoBuild,component,nullptr,&result)) {
    Log("%s failed stage=disable-auto-build component=%p",EiemPhysicsFactoryProbeTag,component); return false;
  }
  const auto &prepared=draft.Groups()[0];
  void *data=prepared.data.Target(),*detachedData2=prepared.data2.Target();
  void *dataArgs[]={data};
  if (!InvokeChecked(state.api.setSerializeData,component,dataArgs,&result)) {
    Log("%s failed stage=bind-config component=%p",EiemPhysicsFactoryProbeTag,component); return false;
  }
  void *actualData=nullptr,*actualData2=nullptr;
  const bool gotData=InvokeChecked(state.api.getSerializeData,component,nullptr,&actualData);
  const bool gotData2=InvokeChecked(state.api.getSerializeData2,component,nullptr,&actualData2);
  void *actualData2Class=actualData2?il2cpp_object_get_class(actualData2):nullptr;
  if (!gotData || actualData!=data || !gotData2 || !actualData2 ||
      actualData2Class!=state.api.config.data2) {
    Log("%s failed stage=config-readback component=%p gotData=%d expectedData=%p actualData=%p gotData2=%d detachedData2=%p componentData2=%p componentData2Class=%p expectedData2Class=%p",
        EiemPhysicsFactoryProbeTag,component,gotData?1:0,data,actualData,gotData2?1:0,
        detachedData2,actualData2,actualData2Class,state.api.config.data2);
    return false;
  }
  state.componentData2Ref=EiemUnityRef::Capture(actualData2,false);
  if (!state.componentData2Ref) {
    Log("%s failed stage=capture-component-data2 component=%p data2=%p",
        EiemPhysicsFactoryProbeTag,component,actualData2); return false;
  }
  Log("%s config-bound component=%p serializeData=%p detachedData2=%p componentData2=%p",
      EiemPhysicsFactoryProbeTag,component,actualData,detachedData2,actualData2);
  state.config=std::move(draft);
  active=true;
  if (!InvokeChecked(state.api.gameObjectActive,host,activeArgs,&result)) {
    Log("%s failed stage=activate component=%p",EiemPhysicsFactoryProbeTag,component); return false;
  }
  void *boxed=nullptr; bool started=false;
  if (!InvokeChecked(state.api.buildAndRun,component,nullptr,&boxed) ||
      !EiemPhysicsUnbox(boxed,started)) {
    Log("%s failed stage=build-call component=%p",EiemPhysicsFactoryProbeTag,component); return false;
  }
  state.built=started;
  EiemPhysicsReadField(component,state.api.processField,state.process);
  if (state.process) state.processRef=EiemUnityRef::Capture(state.process,false);
  Log("%s build-return=%d component=%p process=%p renderer=%p model=%p animator=%p ownerKind=%s stage=%s nodes=3",
      EiemPhysicsFactoryProbeTag,started?1:0,component,state.process,renderer,model,
      animator,ownerKind?ownerKind:"unknown",stage?stage:"unknown");
  EiemPhysicsFactoryProbePoll("BuildAndRun-return");
  s_eiemPhysicsFactoryNextPeriodicPoll=GetTickCount64()+500;
  return started;
}

static void EiemPhysicsFactoryProbeObserveModel(const char *ownerKind,void *owner,
                                                 void *model,const char *stage) {
  (void)owner;
  auto &state=s_eiemPhysicsFactoryProbe;
  if (state.component) { EiemPhysicsFactoryProbePoll(stage); return; }
  const bool npcOwner=ownerKind && !strcmp(ownerKind,"NPC");
  const bool prefabOwner=ownerKind && !strcmp(ownerKind,"PrefabProxy");
  if (state.attempted || !model || (!npcOwner && !prefabOwner) ||
      !EiemOnUnityThread() || !g_component_get_transform ||
      !g_gameObject_get_transform || !g_transform_get_parent ||
      !g_gameObject_GetComponentsInChildren) return;
  void *renderer=nullptr;
  for (size_t i=0;i<s_eiemPhysicsFactoryRendererCount;++i) {
    auto &entry=s_eiemPhysicsFactoryRenderers[i];
    void *candidate=entry.reference.Target();
    if (candidate==entry.renderer && EiemPhysicsFactoryRendererUnderModel(candidate,model)) {
      renderer=candidate; break;
    }
  }
  if (!renderer) return;
  void *domain=il2cpp_domain_get?il2cpp_domain_get():nullptr; size_t count=0;
  void **assemblies=domain && il2cpp_domain_get_assemblies
      ? il2cpp_domain_get_assemblies(domain,&count):nullptr;
  void *animatorClass=assemblies?EiemPhysicsClass(assemblies,count,
      EiemPhysicsAnimatorImage,"UnityEngine","Animator"):nullptr;
  void *clothClass=assemblies?EiemPhysicsClass(assemblies,count,
      "BeyondDynamicBone.dll","BeyondDynamicBone","BeyondBoneCloth"):nullptr;
  void *animators=EiemPhysicsFactoryComponents(model,animatorClass);
  void *cloths=EiemPhysicsFactoryComponents(model,clothClass);
  const size_t animatorCount=EiemManagedArrayLength(animators);
  const size_t clothCount=EiemManagedArrayLength(cloths);
  // PrefabProxy is also used by character-preview UI, where the target model
  // has no native Cloth.  For that generic owner keep the native-Cloth
  // discriminator.  A target NPCAvatar correlation is already an exact live
  // scene-owner signal and NPC models legitimately report zero embedded Cloth.
  if (!animators || animatorCount>64 || (prefabOwner && clothCount==0)) return;
  void *nearest=nullptr; int nearestDepth=-1; size_t ancestors=0;
  void **items=(void **)((char *)animators+IL2CPP_ARRAY_DATA);
  for (size_t i=0;i<animatorCount;++i) {
    const int depth=EiemPhysicsFactoryRendererDepth(renderer,items[i]);
    if (depth<0) continue;
    ++ancestors;
    if (nearestDepth<0 || depth<nearestDepth) {nearest=items[i];nearestDepth=depth;}
  }
  Log("%s candidate renderer=%p model=%p ownerKind=%s nativeCloths=%zu animatorAncestors=%zu nearestAnimator=%p depth=%d stage=%s",
      EiemPhysicsFactoryProbeTag,renderer,model,ownerKind,clothCount,ancestors,
      nearest,nearestDepth,stage?stage:"unknown");
  if (nearest && ancestors==1)
    EiemPhysicsFactoryProbeBuild(renderer,model,nearest,ownerKind,stage);
}
