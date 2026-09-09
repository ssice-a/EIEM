#pragma once
#include <cctype>
#include <mutex>
#include "eiem_skeleton_document.h"

// Only instantiated on the Unity thread. Consumers keep leases until their
// source Mesh/bones are restored. The registry owns the final retirement ref.
struct EiemSkeletonInstance {
  std::string key;
  uint64_t stamp = 0;
  bool ready = false;
  EiemUnityRef anchor;
  EiemSkeletonDocument document;
  std::vector<EiemUnityRef> nodes;
  std::vector<EiemUnityRef> createdObjects;
  // Deferred destruction / owner-unload callbacks do not prove the native
  // consumer stopped using these Transforms. Wait for native death.
  std::vector<EiemUnityRef> retiringConsumers;
};
static std::vector<std::shared_ptr<EiemSkeletonInstance>> s_eiemSkeletonInstances;
static std::mutex s_eiemSkeletonConsumerMutex;

// Teardown callbacks may run outside Unity's thread. They only retain an
// existing managed reference here; all native queries/destruction stay below.
static void EiemRetainSkeletonConsumer(EiemSkeletonInstance &instance, const EiemUnityRef &reference) {
  std::lock_guard<std::mutex> guard(s_eiemSkeletonConsumerMutex);
  instance.retiringConsumers.push_back(reference);
}

static bool EiemSkeletonCall(void *method, void *object, void **args = nullptr, void **out = nullptr) {
  void *result = nullptr;
  const bool ok = method && InvokeChecked(method,object,args,&result);
  if (out) *out = result;
  return ok;
}

static void EiemCollectSkeletonInstances() {
  if (!EiemOnUnityThread()) return;
  for (size_t i = 0; i < s_eiemSkeletonInstances.size();) {
    auto &instance = s_eiemSkeletonInstances[i];
    if (instance.use_count() != 1) { ++i; continue; }
    instance->ready=false;
    std::vector<EiemUnityRef> consumers;
    {
      std::lock_guard<std::mutex> guard(s_eiemSkeletonConsumerMutex);
      consumers=instance->retiringConsumers;
    }
    bool consumed=false;
    for (const auto &consumer:consumers)
      if (consumer.Status()!=0) { consumed=true; break; }
    if (consumed) { ++i; continue; }
    bool complete = true;
    // Reverse topological order; detach before deferred Destroy, so this
    // reconcile cannot see retired children in the game's live hierarchy.
    for (size_t j = instance->createdObjects.size(); j > 0; --j) {
      auto &object = instance->createdObjects[j-1];
      if (!object) continue;
      const int status = object.Status();
      if (status < 0) { complete = false; continue; }
      if (status == 1) {
        void *transform = nullptr; bool keepWorld = false;
        void *detach[] = {nullptr,&keepWorld}; void *destroy[] = {object.Target()};
        if (!EiemSkeletonCall(g_gameObject_get_transform,object.Target(),nullptr,&transform) || !transform ||
            !EiemSkeletonCall(g_transform_set_parent,transform,detach) ||
            !EiemSkeletonCall(g_object_destroy,nullptr,destroy)) { complete = false; continue; }
      }
      object = {};
    }
    if (!complete) {
      Log("[SKELETON] Retirement pending; references retained: %s",instance->key.c_str()); ++i; continue;
    }
    Log("[SKELETON] Retired %zu owned nodes: %s",instance->createdObjects.size(),instance->key.c_str());
    s_eiemSkeletonInstances.erase(s_eiemSkeletonInstances.begin()+i);
  }
}

// Anchor a document to ONE existing rig through original bone paths. This
// also supports the empty prefab-root path exported by AnimeStudio.
static bool EiemSkeletonSourceNodes(const EiemSkeletonDocument &document, void *renderer,
                                     std::vector<void *> &nodes, std::string &error) {
  nodes.assign(document.nodes.size(),nullptr);
  void *palette = nullptr;
  if (!EiemSkeletonCall(g_smr_get_bones,renderer,nullptr,&palette)) { error="Cannot read source bone palette"; return false; }
  const size_t count = EiemManagedArrayLength(palette);
  if (!palette || !count || count>16384) { error="Source bone palette is empty or invalid"; return false; }
  const auto paletteRoot=EiemUnityRef::Capture(palette,false);
  if (!paletteRoot) { error="Cannot retain source bone palette"; return false; }
  void **items = (void **)((char *)palette+IL2CPP_ARRAY_DATA);
  std::unordered_map<std::string,void *> ancestors;
  std::string prefix; bool anchored = false;
  for (size_t i=0;i<count;++i) {
    std::vector<std::pair<std::string,void *>> chain;
    for (void *node=items[i];node;) {
      if (chain.size()>=128 || EiemNativeObjectStatus(node)!=1) { error="Invalid source skeleton ancestry"; return false; }
      char name[256]={}; void *value=nullptr;
      if (!EiemSkeletonCall(g_object_get_name,node,nullptr,&value) || !value) return false;
      ReadStrUtf8(value,name,sizeof(name));
      if (!name[0]) { error="Unnamed source bone"; return false; }
      chain.emplace_back(name,node);
      if (!EiemSkeletonCall(g_transform_get_parent,node,nullptr,&node)) return false;
    }
    std::string full;
    for (auto it=chain.rbegin();it!=chain.rend();++it) {
      if (!full.empty()) full+='/';
      full+=it->first;
      auto inserted=ancestors.emplace(full,it->second);
      if (!inserted.second && inserted.first->second!=it->second) { error="Ambiguous source bone ancestry"; return false; }
    }
    for (const auto &node:document.nodes) {
      if (!node.source || node.path.empty() || !EiemSkinPathSuffix(full,node.path)) continue;
      const auto candidate=full.substr(0,full.size()-node.path.size());
      if (anchored && prefix!=candidate) { error="Skeleton document spans multiple instances"; return false; }
      prefix=candidate; anchored=true;
    }
  }
  if (!anchored) { error="No source bone anchors this Skeleton resource"; return false; }
  std::string rootPath=prefix+document.nodes.front().path;
  if (!rootPath.empty() && rootPath.back()=='/') rootPath.pop_back();
  const auto root=ancestors.find(rootPath);
  if (root==ancestors.end()) { error="Skeleton source root is absent"; return false; }
  std::vector<void *> live{root->second};
  std::vector<std::string> paths{document.nodes.front().path};
  std::unordered_map<std::string,void *> byPath;
  for (size_t i=0;i<live.size();++i) {
    if (live.size()>16384 || EiemNativeObjectStatus(live[i])!=1) { error="Invalid live Skeleton hierarchy"; return false; }
    if (!byPath.emplace(paths[i],live[i]).second) { error="Ambiguous live Skeleton path: "+paths[i]; return false; }
    void *boxed=nullptr;
    if (!EiemSkeletonCall(g_transform_get_childCount,live[i],nullptr,&boxed) || !boxed) return false;
    int count=*(int *)((char *)boxed+16);
    if (count<0 || count>16384) return false;
    for (int j=0;j<count;++j) {
      void *child=nullptr,*name=nullptr; void *args[]={&j}; char text[256]={};
      if (!EiemSkeletonCall(g_transform_GetChild,live[i],args,&child) || !child ||
          !EiemSkeletonCall(g_object_get_name,child,nullptr,&name) || !name) return false;
      ReadStrUtf8(name,text,sizeof(text));
      if (!text[0]) return false;
      live.push_back(child); paths.push_back(paths[i].empty()?text:paths[i]+"/"+text);
    }
  }
  for (size_t i=0;i<document.nodes.size();++i) if (document.nodes[i].source) {
    const auto found=byPath.find(document.nodes[i].path);
    if (found==byPath.end()) { error="Source bone is missing (not a new bone): "+document.nodes[i].path; return false; }
    nodes[i]=found->second;
  }
  return true;
}

static std::string EiemSkeletonInstanceKey(const char *modPath,
                                            const std::string &diskPath) {
  std::string key=std::string(modPath?modPath:"")+"|"+diskPath;
  std::transform(key.begin(),key.end(),key.begin(),
                 [](unsigned char c){return (char)std::tolower(c);});
  return key;
}

static bool EiemAcquireSkeletonDocument(
    std::string key,uint64_t stamp,EiemSkeletonDocument document,void *renderer,
    std::shared_ptr<EiemSkeletonInstance> &out,char *message,size_t messageSize) {
  out.reset(); std::string error;
  auto fail=[&](const std::string &why) {
    if (message) strncpy_s(message,messageSize,why.c_str(),_TRUNCATE);
    return false;
  };
  if (!EiemOnUnityThread()) return fail("Skeleton requires Unity thread");
  std::vector<void *> source;
  if (!EiemSkeletonSourceNodes(document,renderer,source,error)) return fail(error.empty()?"Cannot resolve source Skeleton":error);
  EiemCollectSkeletonInstances();
  for (const auto &cached:s_eiemSkeletonInstances) {
    if (cached->key!=key || cached->anchor.Target()!=source.front() || cached->anchor.Status()!=1) continue;
    // Old generations can be waiting for deferred partner destruction.
    // Their private nodes must not be reused by the new generation.
    if (!cached->ready) continue;
    if (cached->stamp!=stamp) return fail("Skeleton changed while in use; reload before rebinding");
    for (const auto &node:cached->nodes) if (node.Status()!=1) return fail("Skeleton node was destroyed; reload to rebuild");
    out=cached; return true;
  }
  auto instance=std::make_shared<EiemSkeletonInstance>();
  instance->key=key; instance->stamp=stamp; instance->document=std::move(document);
  instance->anchor=EiemUnityRef::Capture(source.front());
  if (!instance->anchor) return fail("Cannot retain Skeleton instance identity");
  // Registry also keeps failed builds alive until checked retirement succeeds.
  s_eiemSkeletonInstances.push_back(instance);
  for (size_t i=0;i<instance->document.nodes.size();++i) {
    const auto &node=instance->document.nodes[i]; void *transform=source[i];
    if (!node.source) {
      if (!g_gameObjectClass || !il2cpp_object_new || !il2cpp_string_new ||
          !g_gameObject_ctor || !g_transform_set_parent || !g_transform_set_localPosition ||
          !g_transform_set_localRotation || !g_transform_set_localScale || !g_object_destroy)
        return fail("New Skeleton node APIs are unavailable");
      void *object=il2cpp_object_new(g_gameObjectClass);
      auto hold=EiemUnityRef::Capture(object,false);
      if (!object || !hold) return fail("Cannot allocate Skeleton node");
      // Private runtime names avoid collisions between Mod-owned nodes.
      // Mesh paths resolve through this instance's explicit node map.
      static uint64_t serial=0;
      std::string name="EIEM_Bone_"+std::to_string(++serial);
      void *args[]={il2cpp_string_new(name.c_str())};
      instance->createdObjects.push_back(hold);
      if (!args[0] || !EiemSkeletonCall(g_gameObject_ctor,object,args)) return fail("Cannot construct Skeleton node");
      if (!EiemSkeletonCall(g_gameObject_get_transform,object,nullptr,&transform) || !transform) return fail("Skeleton node has no Transform");
      bool keepWorld=false; void *parent=instance->nodes[node.parent].Target();
      void *parentArgs[]={parent,&keepWorld};
      void *position[]={const_cast<float *>(node.position)};
      void *rotation[]={const_cast<float *>(node.rotation)};
      void *scale[]={const_cast<float *>(node.scale)};
      if (EiemNativeObjectStatus(parent)!=1 || !EiemSkeletonCall(g_transform_set_parent,transform,parentArgs) ||
          !EiemSkeletonCall(g_transform_set_localPosition,transform,position) ||
          !EiemSkeletonCall(g_transform_set_localRotation,transform,rotation) ||
          !EiemSkeletonCall(g_transform_set_localScale,transform,scale)) return fail("Cannot attach Skeleton node: "+node.path);
    }
    auto reference=EiemUnityRef::Capture(transform,false);
    if (!reference || reference.Status()!=1) return fail("Cannot retain Skeleton Transform: "+node.path);
    instance->nodes.push_back(std::move(reference));
  }
  instance->ready=true;
  Log("[SKELETON] Ready resource=%s sourceRoot=%p nodes=%zu added=%zu (GPU validation separate)",
      key.c_str(),source.front(),instance->nodes.size(),instance->createdObjects.size());
  out=std::move(instance); return true;
}

static bool EiemAcquireSkeleton(const EiemModRule &rule, void *renderer,
                                 std::shared_ptr<EiemSkeletonInstance> &out,
                                 char *message, size_t messageSize) {
  out.reset();
  EiemModResource resource={}; char path[MAX_PATH]={};
  if (!EiemFindModResource(rule.modPath,rule.skeleton,"Skeleton",&resource) ||
      !EiemResolveResourceDiskPath(resource,path,sizeof(path))) {
    if (message) strncpy_s(message,messageSize,
        "Skeleton resource is not declared or has no path",_TRUNCATE);
    return false;
  }
  std::string error; EiemNativeReader reader(path); EiemSkeletonDocument document;
  if (!EiemReadSkeleton(reader,document,error)) {
    if (message) strncpy_s(message,messageSize,error.c_str(),_TRUNCATE);
    return false;
  }
  return EiemAcquireSkeletonDocument(
      EiemSkeletonInstanceKey(rule.modPath,path),EiemMeshResourceFileStamp(path),
      std::move(document),renderer,out,message,messageSize);
}

static bool EiemWatchSkeletonPartner(EiemSkeletonInstance &instance, void *partner) {
  auto reference=EiemUnityRef::Capture(partner);
  if (!reference || reference.Status()!=1) return false;
  EiemRetainSkeletonConsumer(instance,reference);
  return true;
}

static bool EiemSkeletonMeshBones(const EiemSkinIdentity &skin, const EiemSkeletonInstance &instance,
                                   void **out, char *message, size_t size) {
  *out=nullptr; std::string error; std::vector<std::string> paths;
  if (!instance.ready || instance.nodes.size()!=instance.document.nodes.size() ||
      !il2cpp_array_new || !g_transformClass) {
    if (message) strncpy_s(message,size,"Skeleton instance or Transform array API is unavailable",_TRUNCATE);
    return false;
  }
  for (const auto &node:instance.document.nodes) paths.push_back(node.path);
  std::vector<size_t> indices;
  if (skin.paths.empty() || !EiemResolveSkinPathIndices(skin.paths,paths,indices,error)) {
    if (message) strncpy_s(message,size,error.empty()?"Skeleton binding requires Mesh bone paths":error.c_str(),_TRUNCATE);
    return false;
  }
  std::vector<void *> values;
  for (auto i:indices) {
    if (instance.nodes[i].Status()!=1) return false;
    values.push_back(instance.nodes[i].Target());
  }
  void *array=il2cpp_array_new(g_transformClass,values.size());
  if (!array) return false;
  memcpy((char *)array+IL2CPP_ARRAY_DATA,values.data(),values.size()*sizeof(void *));
  *out=array; return true;
}
