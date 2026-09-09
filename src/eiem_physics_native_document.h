#pragma once
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

// Lossless typed authoring data. These values are not IL2CPP object layouts.
struct EiemPhysicsNativeValue {
  uint8_t tag=0;
  int64_t integer=0;
  double number=0;
  std::string text;
  std::vector<EiemPhysicsNativeValue> array;
  std::map<std::string,EiemPhysicsNativeValue> object;
  const EiemPhysicsNativeValue *Get(const char *key) const {
    const auto it=object.find(key); return it==object.end()?nullptr:&it->second;
  }
  bool String(const char *key,std::string &out) const {
    const auto *v=Get(key); if (!v || v->tag!=5) return false; out=v->text; return true;
  }
};

struct EiemPhysicsNativeComponent {
  std::string id, source, type, name, bone, ownerPath, operation;
  std::vector<std::string> components;
};

template<typename Reader>
static bool EiemReadPhysicsNativeValue(Reader &r,EiemPhysicsNativeValue &out,unsigned depth=0,size_t *budget=nullptr) {
  size_t initialBudget=524288;
  if (!budget) budget=&initialBudget;
  if (!*budget) return false;
  --*budget;
  if (depth>64 || !r.Value(out.tag) || out.tag>7) return false;
  if (out.tag<3) return true;
  if (out.tag==3) return r.Value(out.integer);
  if (out.tag==4) return r.Value(out.number) && std::isfinite(out.number);
  uint32_t count=0;
  if (!r.Count(count,out.tag==5?16*1024*1024:262144) || count>r.remaining) return false;
  if (out.tag==5) {
    out.text.resize(count);
    return (!count || r.Bytes(out.text.data(),count)) && EiemPhysicsAuthorUtf8(out.text);
  }
  if (out.tag==6) {
    // Grow after decoding each value instead of allocating from an untrusted count.
    for (uint32_t i=0;i<count;++i) {
      EiemPhysicsNativeValue child;
      if (!EiemReadPhysicsNativeValue(r,child,depth+1,budget)) return false;
      out.array.push_back(std::move(child));
    }
  } else for (uint32_t i=0;i<count;++i) {
    EiemPhysicsNativeValue key,child;
    if (!EiemReadPhysicsNativeValue(r,key,depth+1,budget) || key.tag!=5 ||
        out.object.count(key.text) || !EiemReadPhysicsNativeValue(r,child,depth+1,budget)) return false;
    out.object.emplace(std::move(key.text),std::move(child));
  }
  return true;
}

static bool EiemValidatePhysicsNative(const EiemPhysicsNativeValue &root,std::string &id,std::string &skeleton,
                                    std::vector<EiemPhysicsNativeComponent> &out,std::string &error) {
  auto fail=[&](const char *why) {error=why; return false;};
  std::string value;
  const auto *version=root.Get("version");
  if (root.tag!=7 || !version || version->tag!=3 || version->integer!=2 ||
      !root.String("purpose",value) || value!="native-authoring" ||
      !root.String("coordinate",value) || value!="unity-y-up-left-handed" ||
      !root.String("backend",value) || value!="BeyondDynamicBone" ||
      !root.String("id",id) || !EiemPhysicsAuthorId(id) ||
      !root.String("skeleton",skeleton) || !EiemPhysicsAuthorPath(skeleton) ||
      skeleton.size()<9 || skeleton.substr(skeleton.size()-9)!=".skeleton") return fail("Invalid Physics v2 header");
  const auto *components=root.Get("components"), *transforms=root.Get("transforms"), *source=root.Get("source");
  if (!components || components->tag!=6 || components->array.empty() || components->array.size()>4096 ||
      !transforms || transforms->tag!=6 || transforms->array.empty() || transforms->array.size()>16384 ||
      !source || source->tag!=7 || !source->String("prefab",value) || !EiemPhysicsAuthorPath(value))
    return fail("Invalid Physics v2 source graph");
  std::set<std::string> ids,sources,owners,transformIds,bones;
  std::map<std::string,std::string> parents;
  for (const auto &t:transforms->array) {
    std::string identity,bone,owner,parent;
    const auto *p=t.Get("parent");
    if (!t.String("identity",identity) || identity.empty() || !transformIds.insert(identity).second ||
        !t.String("bone",bone) || !EiemPhysicsAuthorPath(bone,true) || !bones.insert(bone).second ||
        !t.String("owner",owner) || !owners.insert(owner).second || !p || (p->tag!=0 && p->tag!=5))
      return fail("Invalid Physics v2 Transform identity");
    if (p->tag==5) parent=p->text;
    parents[identity]=parent;
    for (const char *key:{"localPosition","localRotation","localScale"}) {
      const auto *a=t.Get(key); const size_t count=std::string(key)=="localRotation"?4:3;
      if (!a || a->tag!=6 || a->array.size()!=count) return fail("Invalid Physics v2 Transform data");
      double norm=0;
      for (const auto &n:a->array) {
        if (n.tag!=3 && n.tag!=4) return fail("Invalid Physics v2 Transform number");
        const double f=n.tag==3?double(n.integer):n.number;
        if (!std::isfinite(f) || (std::string(key)=="localScale" && f<=0)) return fail("Invalid Physics v2 Transform scale");
        norm+=f*f;
      }
      if (count==4 && std::abs(norm-1)>.002) return fail("Invalid Physics v2 Transform rotation");
    }
  }
  std::set<std::string> finished;
  for (const auto &item:parents) {
    std::string key=item.first; std::set<std::string> visiting;
    while (!key.empty() && !finished.count(key)) {
      const auto it=parents.find(key);
      if (it==parents.end() || !visiting.insert(key).second) return fail("Missing or cyclic Physics v2 ancestor");
      key=it->second;
    }
    finished.insert(visiting.begin(),visiting.end());
  }
  for (const auto &c:components->array) {
    EiemPhysicsNativeComponent item; std::string owner;
    if (!c.String("id",item.id) || !EiemPhysicsAuthorId(item.id) || !ids.insert(item.id).second ||
        !c.String("source",item.source) || item.source.empty() || !sources.insert(item.source).second ||
        !c.String("type",item.type) || !c.String("name",item.name) ||
        !c.String("bone",item.bone) || !bones.count(item.bone) || !c.String("ownerPath",item.ownerPath) ||
        !c.String("owner",owner) || !owners.count(owner) || !c.String("operation",item.operation) ||
        (item.operation!="create" && item.operation!="override" && item.operation!="disable"))
      return fail("Invalid Physics v2 component identity/action");
    if (item.type!="BeyondBoneCloth" && item.type!="BeyondBoneCapsuleCollider" &&
        item.type!="BeyondBoneSphereCollider" && item.type!="BeyondBonePlaneCollider")
      return fail("Unsupported Physics v2 native component type");
    const auto *fields=c.Get("fields"), *schema=c.Get("schema"), *refs=c.Get("references");
    if (!fields || fields->tag!=7 || !schema || schema->tag!=6 || !refs || refs->tag!=6 ||
        !c.String("raw",value) || value.empty() || !c.String("sha256",value) || value.size()!=64)
      return fail("Missing Physics v2 preserved source data");
    if (item.type=="BeyondBoneCloth") {
      const auto *s=fields->Get("serializeData2"); s=s?s->Get("selectionData"):nullptr;
      const auto *positions=s?s->Get("positions"):nullptr,*attributes=s?s->Get("attributes"):nullptr;
      if (!positions || !attributes || positions->tag!=6 || attributes->tag!=6 ||
          positions->array.size()!=attributes->array.size()) return fail("Invalid Physics v2 selection arrays");
    }
    for (const auto &ref:refs->array) {
      const auto *isNull=ref.Get("isNull"),*resolved=ref.Get("resolved");
      if (!isNull || (isNull->tag!=1 && isNull->tag!=2)) return fail("Invalid Physics v2 reference null state");
      if (isNull->tag==2) continue;
      std::string type,target;
      if (!resolved || resolved->tag!=2 || !ref.String("type",type) || !ref.String("identity",target))
        return fail("Unresolved Physics v2 source reference");
      if (type=="Transform" && !transformIds.count(target)) return fail("Missing Physics v2 bone reference");
      if (type=="MonoBehaviour") item.components.push_back(target);
    }
    out.push_back(std::move(item));
  }
  for (const auto &c:out) for (const auto &ref:c.components)
    if (!sources.count(ref)) return fail("Missing Physics v2 component dependency");
  return true;
}
