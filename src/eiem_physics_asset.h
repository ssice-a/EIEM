#pragma once
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_set>
#include "eiem_physics_document.h"
#include "eiem_skeleton_document.h"

// Immutable disk snapshot. No native component, simulation state or Unity
// reference is created by preparing this resource.
struct EiemPhysicsAsset {
  std::filesystem::path path, skeletonPath;
  EiemPhysicsDocument physics;
  EiemSkeletonDocument skeleton;
};

static bool EiemPhysicsResolveFile(const std::filesystem::path &base, const std::string &relative,
                                   std::filesystem::path &out, std::string &error) {
  if (!EiemPhysicsAuthorPath(relative) || !EiemPhysicsAuthorUtf8(relative)) {
    error="Physics requires a relative resource path: "+relative; return false;
  }
  try {
    const auto root=std::filesystem::canonical(base);
    const auto candidate=std::filesystem::canonical(root/std::filesystem::u8path(relative));
    auto r=root.begin(), c=candidate.begin();
    for (;r!=root.end();++r,++c)
      if (c==candidate.end() || _wcsicmp(r->c_str(),c->c_str())) {
        error="Physics dependency escapes its resource directory: "+relative; return false;
      }
    if (c==candidate.end() || !std::filesystem::is_regular_file(candidate)) {
      error="Physics dependency is not a file: "+relative; return false;
    }
    out=candidate; return true;
  } catch (const std::filesystem::filesystem_error &) {
    error="Cannot resolve Physics dependency: "+relative; return false;
  }
}

struct EiemPhysicsFileReader {
  std::vector<char> bytes;
  size_t offset=0;
  bool Load(const std::filesystem::path &path, std::string &error) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    const auto size=file ? file.tellg() : std::streampos(-1);
    if (size<=0 || size>16*1024*1024) {
      error="Unreadable or oversized Physics dependency: "+path.u8string(); return false;
    }
    bytes.resize(static_cast<size_t>(size)); offset=0;
    file.seekg(0);
    if (!file.read(bytes.data(),static_cast<std::streamsize>(bytes.size())) ||
        file.peek()!=std::char_traits<char>::eof()) {
      error="Physics dependency changed or was truncated while reading: "+path.u8string(); return false;
    }
    return true;
  }
  bool Bytes(void *out,size_t count) {
    if (count>bytes.size()-offset) return false;
    if (count) std::memcpy(out,bytes.data()+offset,count);
    offset+=count; return true;
  }
  template<typename T> bool Value(T *out) { return Bytes(out,sizeof(T)); }
  bool Count(uint32_t *out,uint32_t limit) { return Value(out) && *out<=limit; }
  // Skeleton uses .NET's seven-bit string length, unlike Physics author formats' u32.
  bool String(std::string *out) {
    uint32_t count=0;
    for (unsigned shift=0;shift<35;shift+=7) {
      uint8_t part=0;
      if (!Value(&part) || (shift==28 && (part&0xf0))) return false;
      count|=uint32_t(part&0x7f)<<shift;
      if (!(part&0x80)) {
        if (count>4096) return false;
        out->resize(count);
        return Bytes(out->data(),count) && EiemPhysicsAuthorUtf8(*out);
      }
    }
    return false;
  }
  bool End() const { return offset==bytes.size(); }
};

static bool EiemLoadPhysicsSkeleton(const std::filesystem::path &path,
                                    EiemSkeletonDocument &out,std::string &error) {
  EiemPhysicsFileReader reader;
  return reader.Load(path,error) && EiemReadSkeleton(reader,out,error);
}

static bool EiemSamePhysicsSkeleton(const EiemSkeletonDocument &a,const EiemSkeletonDocument &b) {
  if (a.nodes.size()!=b.nodes.size()) return false;
  for (size_t i=0;i<a.nodes.size();++i) {
    const auto &x=a.nodes[i], &y=b.nodes[i];
    if (x.path!=y.path || x.parent!=y.parent || x.source!=y.source) return false;
    for (size_t j=0;j<3;++j)
      if (x.position[j]!=y.position[j] || x.scale[j]!=y.scale[j]) return false;
    for (size_t j=0;j<4;++j) if (x.rotation[j]!=y.rotation[j]) return false;
  }
  return true;
}

static bool EiemLoadPhysicsAsset(const std::filesystem::path &path,
                                 std::shared_ptr<const EiemPhysicsAsset> &out,std::string &error) {
  auto next=std::make_shared<EiemPhysicsAsset>(); next->path=path;
  EiemPhysicsFileReader reader;
  if (!reader.Load(path,error) || !EiemReadPhysicsAuthor(reader,next->physics,error) ||
      !EiemPhysicsResolveFile(path.parent_path(),next->physics.skeleton,next->skeletonPath,error) ||
      !EiemLoadPhysicsSkeleton(next->skeletonPath,next->skeleton,error)) return false;
  std::unordered_set<std::string> paths;
  for (const auto &node:next->skeleton.nodes) paths.insert(node.path);
  for (const auto &group:next->physics.groups)
    for (const auto &node:group.nodes) if (!paths.count(node.bone)) {
      error="Physics group "+group.id+" references missing Skeleton bone: "+node.bone; return false;
    }
  for (const auto &collider:next->physics.colliders) if (!paths.count(collider.bone)) {
    error="Physics collider "+collider.id+" references missing Skeleton bone: "+collider.bone; return false;
  }
  if (next->physics.native) {
    const auto *transforms=next->physics.native->Get("transforms");
    for (const auto &transform:transforms->array) {
      std::string bone;
      if (!transform.String("bone",bone) || !paths.count(bone)) {
        error="Physics v2 references missing Skeleton node: "+bone; return false;
      }
    }
  }
  out=std::move(next); error.clear(); return true;
}
