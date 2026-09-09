#pragma once
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Pure per-consumer policy. Indices supplied by the game refer to its SOURCE
// layout; indices supplied by EIEM refer to the current TARGET layout.
struct EiemShapeChannels {
  std::vector<std::string> sourceNames, targetNames;
  std::vector<int> targetIndices;
  std::vector<float> gameValues;
  std::vector<bool> controlled;
  bool Build(const std::vector<std::string> &source, const std::vector<float> &values,
             const std::vector<std::string> &target, std::string &error) {
    auto unique=[](const auto &names) {
      for (size_t i=0;i<names.size();++i)
        if (names[i].empty() || std::find(names.begin(),names.begin()+i,names[i])!=names.begin()+i) return false;
      return true;
    };
    if (source.size()!=values.size() || !unique(source) || !unique(target) ||
        !std::all_of(values.begin(),values.end(),[](float v){return std::isfinite(v);})) {
      error="Invalid/duplicate source or target shape layout"; return false;
    }
    std::vector<int> indices;
    for (const auto &name:source) {
      auto it=std::find(target.begin(),target.end(),name);
      if (it==target.end()) { error="Source shape channel missing from replacement: "+name;return false; }
      indices.push_back((int)(it-target.begin()));
    }
    sourceNames=source;targetNames=target;targetIndices=std::move(indices);gameValues=values;
    controlled.assign(target.size(),false);return true;
  }
  // Zero here is the deliberate absent-source-channel view, not a failed read.
  float GameRead(int index) const { return index>=0 && (size_t)index<gameValues.size()?gameValues[index]:0.0f; }
  int GameWrite(int index,float value) {
    if (index<0 || (size_t)index>=targetIndices.size()) return -1;
    gameValues[index]=value;
    int target=targetIndices[index];return controlled[target]?-1:target;
  }
  bool Latest(int target,float &value) const {
    auto it=std::find(targetIndices.begin(),targetIndices.end(),target);
    if(it==targetIndices.end())return false;
    value=gameValues[it-targetIndices.begin()];return true;
  }
};

static thread_local unsigned s_eiemShapeAuthorDepth=0, s_eiemShapeGameDepth=0;
struct EiemShapeAuthorScope {
  EiemShapeAuthorScope(){++s_eiemShapeAuthorDepth;}
  ~EiemShapeAuthorScope(){--s_eiemShapeAuthorDepth;}
  EiemShapeAuthorScope(const EiemShapeAuthorScope &)=delete;
};
struct EiemShapeGameScope {
  EiemShapeGameScope(){++s_eiemShapeGameDepth;}
  ~EiemShapeGameScope(){--s_eiemShapeGameDepth;}
  EiemShapeGameScope(const EiemShapeGameScope &)=delete;
};
