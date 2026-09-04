#pragma once

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <cstdint>
#include <cerrno>
#include <climits>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_set>

// Authoring syntax -> typed resource declarations and Render actions. No
// published configuration, Unity objects, hotkeys or lifecycle state live here.
struct EiemModRule {
  char modPath[MAX_PATH] = {};
  char section[96] = {};
  char path[768] = {};
  char handling[32] = {};
  char mesh[192] = {};
  char asset[192] = {};
  int32_t matchVertices = -1;
  int32_t matchIndices = -1;
  int32_t matchSubMeshes = -1;
  char skeleton[192] = {};
  bool hasMesh = false;
  bool hasSkeleton = false;
  char materials[16][192] = {};
  int32_t materialSlots[16] = {};
  uint32_t materialCount = 0;
  int32_t submeshSlots[32] = {};
  uint32_t submeshCount = 0;
  char partners[16][96] = {};
  uint32_t partnerCount = 0;
};

// A Prefab declaration groups the Render actions for one logical model. Each
// supported lifecycle owner resolves that PFB identity first, then invokes the
// same Render executor on its completed model hierarchy.
struct EiemModPrefab {
  char modPath[MAX_PATH] = {};
  char section[96] = {};
  char path[768] = {};
  char renders[64][96] = {};
  uint32_t renderCount = 0;
};

struct EiemModResource {
  char modPath[MAX_PATH] = {};
  char section[96] = {};
  char path[768] = {};
  char kind[24] = {};
  char source[768] = {};
  // Optional logical identity of the original game resource represented by
  // this editable payload. Render rules use that identity for authoring and
  // diagnostics; resource declarations never execute replacement logic.
  char targetPath[768] = {};
  char targetAsset[192] = {};
  bool textureLinear = false;
  bool textureMipmaps = true;
  int32_t textureFilter = 1;
  int32_t textureWrap = 0;
  int32_t textureAniso = 1;
  float textureMipBias = 0.0f;
};

static void EiemModInitRule(EiemModRule *rule) {
  if (!rule) return;
  memset(rule, 0, sizeof(*rule));
  for (auto &slot : rule->materialSlots) slot = -1;
  for (auto &slot : rule->submeshSlots) slot = -1;
  rule->matchVertices = -1;
  rule->matchIndices = -1;
  rule->matchSubMeshes = -1;
}

static inline void EiemModTrim(std::string &value) {
  auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
  value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
}

static inline bool EiemModEquals(const char *a, const char *b) {
  return a && b && _stricmp(a, b) == 0;
}

static bool EiemModSameLogicalPath(const char *a, const char *b) {
  if (!a || !b || !a[0] || !b[0]) return false;
  while (*a && *b) {
    const unsigned char ca = (unsigned char)(*a == '\\' ? '/' : *a);
    const unsigned char cb = (unsigned char)(*b == '\\' ? '/' : *b);
    if (tolower(ca) != tolower(cb)) return false;
    ++a;
    ++b;
  }
  return !*a && !*b;
}

static inline void EiemModCopy(char *out, size_t capacity, const std::string &value) {
  if (!out || capacity == 0) return;
  strncpy_s(out, capacity, value.c_str(), _TRUNCATE);
}

static bool EiemModInteger(const std::string &text, int32_t *out,
                            int32_t minimum = 0, int32_t maximum = INT32_MAX) {
  if (text.empty()) return false;
  char *end = nullptr;
  errno = 0;
  const long long value = strtoll(text.c_str(), &end, 10);
  if (errno || end == text.c_str() || *end || value < minimum || value > maximum)
    return false;
  *out = (int32_t)value;
  return true;
}

static bool EiemModParseStream(std::istream &input, const char *path,
                                std::vector<EiemModPrefab> &outputPrefabs,
                                std::vector<EiemModRule> &outputRules,
                                std::vector<EiemModResource> &outputResources,
                                std::string *error = nullptr) {
  // A bad file must not publish half of its rules or broaden an invalid match.
  std::vector<EiemModPrefab> prefabs;
  std::vector<EiemModRule> out;
  std::vector<EiemModResource> resources;
  std::unordered_set<std::string> sections;
  size_t lineNumber = 0;
  if (error) error->clear();
  auto fail = [&](const std::string &message) {
    if (error) *error = std::to_string(lineNumber) + ": " + message;
    return false;
  };
  std::string line, section;
  EiemModRule current = {};
  EiemModInitRule(&current);
  EiemModResource resource = {};
  EiemModPrefab prefab = {};
  bool inPrefab = false;
  bool inRender = false;
  bool inResource = false;
  auto flush = [&]() {
    if (!inRender || !current.section[0]) return;
    strncpy_s(current.modPath, sizeof(current.modPath), path, _TRUNCATE);
    out.push_back(current);
    EiemModInitRule(&current);
  };
  auto flushResource = [&]() {
    if (!inResource) return;
    if (resource.path[0]) {
      strncpy_s(resource.modPath, sizeof(resource.modPath), path, _TRUNCATE);
      resources.push_back(resource);
    }
    resource = {};
  };
  auto flushPrefab = [&]() {
    if (!inPrefab) return;
    if (prefab.section[0] && prefab.path[0]) {
      strncpy_s(prefab.modPath, sizeof(prefab.modPath), path, _TRUNCATE);
      prefabs.push_back(prefab);
    }
    prefab = {};
  };
  while (std::getline(input, line)) {
    ++lineNumber;
    if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0)
      line.erase(0, 3);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    EiemModTrim(line);
    if (line.empty() || line[0] == ';' || line[0] == '#') continue;
    // Until the ordered statement parser exists, do not mistake a condition
    // containing '=' for an unknown metadata field and run its body unconditionally.
    const std::string command = line.substr(0, line.find_first_of(" \t=("));
    if (EiemModEquals(command.c_str(), "if") || EiemModEquals(command.c_str(), "else") ||
        EiemModEquals(command.c_str(), "elif") || EiemModEquals(command.c_str(), "endif"))
      return fail("Conditional commands are not implemented");
    if (line.front() == '[' && line.back() == ']') {
      flush();
      flushResource();
      flushPrefab();
      section = line.substr(1, line.size() - 2);
      EiemModTrim(section);
      std::string sectionKey = section;
      std::transform(sectionKey.begin(), sectionKey.end(), sectionKey.begin(),
                     [](unsigned char c) { return (char)std::tolower(c); });
      if (section.empty() || !sections.insert(sectionKey).second)
        return fail("Empty or duplicate section: " + section);
      inPrefab = section.size() >= 6 &&
                 _strnicmp(section.c_str(), "Prefab", 6) == 0;
      inRender = !inPrefab && section.size() >= 6 &&
                 _strnicmp(section.c_str(), "Render", 6) == 0;
      inResource = !inPrefab && !inRender &&
                   (_strnicmp(section.c_str(), "Mesh", 4) == 0 ||
                    _strnicmp(section.c_str(), "Skeleton", 8) == 0 ||
                    _strnicmp(section.c_str(), "Material", 8) == 0 ||
                    _strnicmp(section.c_str(), "Texture", 7) == 0);
      if (inRender) EiemModCopy(current.section, sizeof(current.section), section);
      if (inPrefab) EiemModCopy(prefab.section, sizeof(prefab.section), section);
      if (inResource) {
        EiemModCopy(resource.section, sizeof(resource.section), section);
        if (_strnicmp(section.c_str(), "Mesh", 4) == 0)
          EiemModCopy(resource.kind, sizeof(resource.kind), "Mesh");
        else if (_strnicmp(section.c_str(), "Skeleton", 8) == 0)
          EiemModCopy(resource.kind, sizeof(resource.kind), "Skeleton");
        else if (_strnicmp(section.c_str(), "Material", 8) == 0)
          EiemModCopy(resource.kind, sizeof(resource.kind), "Material");
        else
          EiemModCopy(resource.kind, sizeof(resource.kind), "Texture");
      }
      continue;
    }
    if (!inPrefab && !inRender && !inResource) continue;
    const size_t equals = line.find('=');
    if (equals == std::string::npos)
      return fail("Expected key=value; condition/command syntax is not implemented");
    std::string key = line.substr(0, equals), value = line.substr(equals + 1);
    EiemModTrim(key); EiemModTrim(value);
    if (_stricmp(key.c_str(), "path") == 0) {
      if (inPrefab) EiemModCopy(prefab.path, sizeof(prefab.path), value);
      else if (inRender) EiemModCopy(current.path, sizeof(current.path), value);
      else EiemModCopy(resource.path, sizeof(resource.path), value);
    }
    if (inPrefab) {
      if (_strnicmp(key.c_str(), "render.", 7) == 0) {
        int32_t index = 0;
        if (!EiemModInteger(key.substr(7), &index, 0, _countof(prefab.renders) - 1))
          return fail("Invalid Render reference index: " + key);
        EiemModCopy(prefab.renders[index], sizeof(prefab.renders[0]), value);
        if ((uint32_t)(index + 1) > prefab.renderCount)
          prefab.renderCount = (uint32_t)(index + 1);
      }
      continue;
    }
    if (inResource) {
      if (_stricmp(key.c_str(), "source") == 0)
        EiemModCopy(resource.source, sizeof(resource.source), value);
      else if (_stricmp(key.c_str(), "target.path") == 0)
        EiemModCopy(resource.targetPath, sizeof(resource.targetPath), value);
      else if (_stricmp(key.c_str(), "target.asset") == 0)
        EiemModCopy(resource.targetAsset, sizeof(resource.targetAsset), value);
      else if (EiemModEquals(key.c_str(), "linear") || EiemModEquals(key.c_str(), "mipmaps")) {
        const bool yes = EiemModEquals(value.c_str(), "true") || value == "1";
        const bool no = EiemModEquals(value.c_str(), "false") || value == "0";
        if (!yes && !no) return fail("Invalid boolean: " + key);
        if (EiemModEquals(key.c_str(), "linear")) resource.textureLinear = yes;
        else resource.textureMipmaps = yes;
      } else if (EiemModEquals(key.c_str(), "filter")) {
        if (!EiemModInteger(value, &resource.textureFilter, 0, 2)) return fail("Invalid filter");
      } else if (EiemModEquals(key.c_str(), "wrap")) {
        if (!EiemModInteger(value, &resource.textureWrap, 0, 3)) return fail("Invalid wrap");
      } else if (EiemModEquals(key.c_str(), "aniso")) {
        if (!EiemModInteger(value, &resource.textureAniso, 0, 16)) return fail("Invalid aniso");
      } else if (EiemModEquals(key.c_str(), "mip_bias")) {
        char *end = nullptr;
        errno = 0;
        const float bias = strtof(value.c_str(), &end);
        if (errno || end == value.c_str() || *end || !std::isfinite(bias))
          return fail("Invalid mip_bias");
        resource.textureMipBias = bias;
      }
    }
    if (!inRender) continue;
    if (_stricmp(key.c_str(), "asset") == 0)
      EiemModCopy(current.asset, sizeof(current.asset), value);
    else if (_stricmp(key.c_str(), "match.vertices") == 0) {
      if (!EiemModInteger(value, &current.matchVertices)) return fail("Invalid match.vertices");
    } else if (_stricmp(key.c_str(), "match.indices") == 0) {
      if (!EiemModInteger(value, &current.matchIndices)) return fail("Invalid match.indices");
    } else if (_stricmp(key.c_str(), "match.submeshes") == 0) {
      if (!EiemModInteger(value, &current.matchSubMeshes)) return fail("Invalid match.submeshes");
    } else if (_stricmp(key.c_str(), "handling") == 0) {
      if (!value.empty() && !EiemModEquals(value.c_str(), "skip"))
        return fail("Unsupported handling: " + value);
      EiemModCopy(current.handling, sizeof(current.handling), value);
    } else if (_stricmp(key.c_str(), "mesh") == 0) {
      EiemModCopy(current.mesh, sizeof(current.mesh), value);
      current.hasMesh = !value.empty();
    } else if (_stricmp(key.c_str(), "skeleton") == 0) {
      EiemModCopy(current.skeleton, sizeof(current.skeleton), value);
      current.hasSkeleton = !value.empty();
    } else if (_strnicmp(key.c_str(), "material.", 9) == 0) {
      int32_t slot = 0;
      if (!EiemModInteger(key.substr(9), &slot, 0, _countof(current.materialSlots) - 1))
        return fail("Invalid material slot: " + key);
      uint32_t entry = 0;
      while (entry < current.materialCount && current.materialSlots[entry] != slot) ++entry;
      if (entry == current.materialCount) ++current.materialCount;
      current.materialSlots[entry] = slot;
      EiemModCopy(current.materials[entry], sizeof(current.materials[0]), value);
    } else if (_strnicmp(key.c_str(), "partner.", 8) == 0) {
      int32_t index = 0;
      if (!EiemModInteger(key.substr(8), &index, 0, _countof(current.partners) - 1))
        return fail("Invalid partner index: " + key);
      EiemModCopy(current.partners[index], sizeof(current.partners[0]), value);
      if ((uint32_t)(index + 1) > current.partnerCount)
        current.partnerCount = (uint32_t)(index + 1);
    } else if (_strnicmp(key.c_str(), "submesh.", 8) == 0) {
      int32_t submesh = 0, slot = 0;
      if (!EiemModInteger(key.substr(8), &submesh, 0, _countof(current.submeshSlots) - 1) ||
          !EiemModInteger(value, &slot))
        return fail("Invalid submesh material mapping: " + key);
      current.submeshSlots[submesh] = slot;
      if ((uint32_t)(submesh + 1) > current.submeshCount)
        current.submeshCount = (uint32_t)(submesh + 1);
    }
  }
  flush();
  flushResource();
  flushPrefab();
  outputPrefabs.insert(outputPrefabs.end(), prefabs.begin(), prefabs.end());
  outputRules.insert(outputRules.end(), out.begin(), out.end());
  outputResources.insert(outputResources.end(), resources.begin(), resources.end());
  return true;
}

static bool EiemModParseFile(const char *path,
                             std::vector<EiemModPrefab> &prefabs,
                             std::vector<EiemModRule> &rules,
                             std::vector<EiemModResource> &resources,
                             std::string *error = nullptr) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  return EiemModParseStream(input, path, prefabs, rules, resources, error);
}

// Loaded/compiled configuration. Indices are derived once at publication, not
// recomputed in Mesh setter hooks. A resource's target.* is only metadata.
struct EiemModProgram {
  std::vector<EiemModPrefab> prefabs;
  std::vector<EiemModRule> rules;
  std::vector<EiemModResource> resources;
  std::vector<size_t> standaloneRules;
};

static void EiemCompileModProgram(EiemModProgram &program) {
  program.standaloneRules.clear();
  auto key = [](const char *file, const char *section) {
    std::string result = std::string(file) + '\n' + section;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return result;
  };
  std::unordered_set<std::string> referenced;
  for (const auto &prefab : program.prefabs)
    for (uint32_t i = 0; i < prefab.renderCount; ++i)
      if (prefab.renders[i][0]) referenced.insert(key(prefab.modPath, prefab.renders[i]));
  for (const auto &rule : program.rules)
    for (uint32_t i = 0; i < rule.partnerCount; ++i)
      if (rule.partners[i][0]) referenced.insert(key(rule.modPath, rule.partners[i]));
  for (size_t i = 0; i < program.rules.size(); ++i) {
    const auto &rule = program.rules[i];
    if ((rule.path[0] || rule.asset[0]) && !referenced.count(key(rule.modPath, rule.section)))
      program.standaloneRules.push_back(i);
  }
}
