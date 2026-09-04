#pragma once

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// The rule layer is deliberately independent from Unity. It only turns the
// user-facing mod.ini files into immutable, fixed-size match records. Unity
// object creation is kept in the resource backend and is not hidden here.
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

struct EiemModResource {
  char modPath[MAX_PATH] = {};
  char section[96] = {};
  char path[768] = {};
  char kind[24] = {};
  char source[768] = {};
  // Optional logical identity of the game resource to redirect globally.
  // Resource declarations remain usable without these fields as ordinary
  // payloads referenced by Render rules.
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

static SRWLOCK s_eiemModLock = SRWLOCK_INIT;
static std::vector<EiemModRule> s_eiemModRules;
static std::vector<EiemModResource> s_eiemModResources;
static volatile LONG s_eiemModGeneration = 0;

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

static bool EiemModParseFile(const char *path, std::vector<EiemModRule> &out,
                             std::vector<EiemModResource> &resources) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return false;
  std::string line, section;
  EiemModRule current = {};
  EiemModInitRule(&current);
  EiemModResource resource = {};
  bool inRender = false;
  bool inResource = false;
  auto flush = [&]() {
    if (!inRender || !current.section[0]) return;
    strncpy_s(current.modPath, sizeof(current.modPath), path, _TRUNCATE);
    out.push_back(current);
    EiemModInitRule(&current);
  };
  auto flushResource = [&]() {
    if (!inResource || !resource.path[0]) return;
    strncpy_s(resource.modPath, sizeof(resource.modPath), path, _TRUNCATE);
    resources.push_back(resource);
    resource = {};
  };
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    EiemModTrim(line);
    if (line.empty() || line[0] == ';' || line[0] == '#') continue;
    if (line.front() == '[' && line.back() == ']') {
      flush();
      flushResource();
      section = line.substr(1, line.size() - 2);
      EiemModTrim(section);
      inRender = section.size() >= 6 &&
                 _strnicmp(section.c_str(), "Render", 6) == 0;
      inResource = !inRender &&
                   (_strnicmp(section.c_str(), "Mesh", 4) == 0 ||
                    _strnicmp(section.c_str(), "Skeleton", 8) == 0 ||
                    _strnicmp(section.c_str(), "Material", 8) == 0 ||
                    _strnicmp(section.c_str(), "Texture", 7) == 0);
      if (inRender) EiemModCopy(current.section, sizeof(current.section), section);
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
    if (!inRender && !inResource) continue;
    const size_t equals = line.find('=');
    if (equals == std::string::npos) continue;
    std::string key = line.substr(0, equals), value = line.substr(equals + 1);
    EiemModTrim(key); EiemModTrim(value);
    if (_stricmp(key.c_str(), "path") == 0) {
      if (inRender) EiemModCopy(current.path, sizeof(current.path), value);
      else EiemModCopy(resource.path, sizeof(resource.path), value);
    }
    if (inResource) {
      if (_stricmp(key.c_str(), "source") == 0)
        EiemModCopy(resource.source, sizeof(resource.source), value);
      else if (_stricmp(key.c_str(), "target.path") == 0)
        EiemModCopy(resource.targetPath, sizeof(resource.targetPath), value);
      else if (_stricmp(key.c_str(), "target.asset") == 0)
        EiemModCopy(resource.targetAsset, sizeof(resource.targetAsset), value);
      else if (_stricmp(key.c_str(), "linear") == 0)
        resource.textureLinear = EiemModEquals(value.c_str(), "true") || value == "1";
      else if (_stricmp(key.c_str(), "mipmaps") == 0)
        resource.textureMipmaps = EiemModEquals(value.c_str(), "true") || value == "1";
      else if (_stricmp(key.c_str(), "filter") == 0)
        resource.textureFilter = (int32_t)strtol(value.c_str(), nullptr, 10);
      else if (_stricmp(key.c_str(), "wrap") == 0)
        resource.textureWrap = (int32_t)strtol(value.c_str(), nullptr, 10);
      else if (_stricmp(key.c_str(), "aniso") == 0)
        resource.textureAniso = (int32_t)strtol(value.c_str(), nullptr, 10);
      else if (_stricmp(key.c_str(), "mip_bias") == 0)
        resource.textureMipBias = strtof(value.c_str(), nullptr);
    }
    if (!inRender) continue;
    if (_stricmp(key.c_str(), "asset") == 0)
      EiemModCopy(current.asset, sizeof(current.asset), value);
    else if (_stricmp(key.c_str(), "match.vertices") == 0)
      current.matchVertices = (int32_t)strtol(value.c_str(), nullptr, 10);
    else if (_stricmp(key.c_str(), "match.indices") == 0)
      current.matchIndices = (int32_t)strtol(value.c_str(), nullptr, 10);
    else if (_stricmp(key.c_str(), "match.submeshes") == 0)
      current.matchSubMeshes = (int32_t)strtol(value.c_str(), nullptr, 10);
    else if (_stricmp(key.c_str(), "handling") == 0)
      EiemModCopy(current.handling, sizeof(current.handling), value);
    else if (_stricmp(key.c_str(), "mesh") == 0) {
      EiemModCopy(current.mesh, sizeof(current.mesh), value);
      current.hasMesh = !value.empty();
    } else if (_stricmp(key.c_str(), "skeleton") == 0) {
      EiemModCopy(current.skeleton, sizeof(current.skeleton), value);
      current.hasSkeleton = !value.empty();
    } else if (_strnicmp(key.c_str(), "material.", 9) == 0) {
      char *end = nullptr;
      const long slot = strtol(key.c_str() + 9, &end, 10);
      if (end != key.c_str() + 9 && *end == '\0' && slot >= 0 &&
          slot < (long)_countof(current.materialSlots) &&
          current.materialCount < _countof(current.materials)) {
        current.materialSlots[current.materialCount] = (int32_t)slot;
        EiemModCopy(current.materials[current.materialCount++],
                    sizeof(current.materials[0]), value);
      }
    } else if (_strnicmp(key.c_str(), "partner.", 8) == 0) {
      char *end = nullptr;
      const long index = strtol(key.c_str() + 8, &end, 10);
      if (end != key.c_str() + 8 && *end == '\0' && index >= 0 &&
          index < (long)_countof(current.partners)) {
        EiemModCopy(current.partners[index], sizeof(current.partners[0]), value);
        if ((uint32_t)(index + 1) > current.partnerCount)
          current.partnerCount = (uint32_t)(index + 1);
      }
    } else if (_strnicmp(key.c_str(), "submesh.", 8) == 0) {
      char *end = nullptr;
      const long submesh = strtol(key.c_str() + 8, &end, 10);
      const long slot = strtol(value.c_str(), nullptr, 10);
      if (end != key.c_str() + 8 && submesh >= 0 && submesh < (long)_countof(current.submeshSlots)) {
        current.submeshSlots[submesh] = (int32_t)slot;
        if ((uint32_t)(submesh + 1) > current.submeshCount)
          current.submeshCount = (uint32_t)(submesh + 1);
      }
    }
  }
  flush();
  flushResource();
  return true;
}

static void EiemReloadMods() {
  std::vector<EiemModRule> loaded;
  std::vector<EiemModResource> resources;
  WIN32_FIND_DATAA data = {};
  HANDLE root = FindFirstFileA("plugin\\mods\\*", &data);
  if (root != INVALID_HANDLE_VALUE) {
    do {
      if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
          data.cFileName[0] == '.') continue;
      char path[MAX_PATH] = {};
      snprintf(path, sizeof(path), "plugin\\mods\\%s\\mod.ini", data.cFileName);
      EiemModParseFile(path, loaded, resources);
    } while (FindNextFileA(root, &data));
    FindClose(root);
  }
  EiemModParseFile("plugin\\mods\\mod.ini", loaded, resources);
  AcquireSRWLockExclusive(&s_eiemModLock);
  s_eiemModRules.swap(loaded);
  s_eiemModResources.swap(resources);
  const LONG generation = InterlockedIncrement(&s_eiemModGeneration);
  const size_t count = s_eiemModRules.size();
  const size_t resourceCount = s_eiemModResources.size();
  for (const auto &rule : s_eiemModRules) {
    Log("[MOD] rule section=%s path=%s asset=%s handling=%s hasMesh=%d mesh=%s partners=%u",
        rule.section, rule.path, rule.asset,
        rule.handling[0] ? rule.handling : "<none>", rule.hasMesh ? 1 : 0,
        rule.hasMesh ? rule.mesh : "<none>", rule.partnerCount);
  }
  ReleaseSRWLockExclusive(&s_eiemModLock);
  for (const auto &resource : s_eiemModResources) {
    if (resource.targetPath[0])
      Log("[MOD] global resource=%s kind=%s target=%s asset=%s",
          resource.section, resource.kind, resource.targetPath,
          resource.targetAsset[0] ? resource.targetAsset : "<any>");
  }
  Log("[MOD] Reloaded %zu Render rule(s), %zu resource declaration(s), generation=%ld",
      count, resourceCount, generation);
}

// This inexpensive prefilter keeps the reconciliation pass from asking Unity
// for vertex/index data for every unrelated scene Mesh.
static bool EiemHasResourceRenderRuleAsset(const char *asset) {
  if (!asset || !asset[0]) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (const auto &rule : s_eiemModRules) {
    if (rule.asset[0] && EiemModEquals(rule.asset, asset)) {
      found = true;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  return found;
}

static bool EiemFindResourceRenderRule(const char *source, const char *asset,
                                       int32_t vertices, int32_t indices,
                                       int32_t subMeshes, EiemModRule *out) {
  if (!asset || !asset[0] || !out) return false;
  bool found = false;
  EiemModRule candidate = {};
  AcquireSRWLockShared(&s_eiemModLock);
  // Prefer the full logical identity whenever the runtime origin tracker has
  // it. Later declarations win for the same source resource.
  for (auto it = s_eiemModRules.rbegin(); it != s_eiemModRules.rend(); ++it) {
    const auto &rule = *it;
    if (!rule.asset[0] || !EiemModEquals(rule.asset, asset)) continue;
    // Logical SubMeshInfo records may expose the asset name before Unity has
    // materialized a Mesh. Unknown shape values must not reject the rule;
    // once a Mesh exists these fields remain strict checks.
    if (rule.matchVertices >= 0 && vertices >= 0 &&
        rule.matchVertices != vertices) continue;
    if (rule.matchIndices >= 0 && indices >= 0 && rule.matchIndices != indices) continue;
    if (rule.matchSubMeshes >= 0 && subMeshes >= 0 &&
        rule.matchSubMeshes != subMeshes) continue;
    if (!source || !source[0] || !EiemModSameLogicalPath(rule.path, source)) continue;
    candidate = rule;
    found = true;
    break;
  }
  // Some embedded Mesh objects never receive an origin record from the game
  // proxy. In that case a structural match is accepted only when all matching
  // rules refer to the same logical source path.
  if (!found) {
    char fallbackPath[768] = {};
    bool ambiguous = false;
    for (auto it = s_eiemModRules.rbegin(); it != s_eiemModRules.rend(); ++it) {
      const auto &rule = *it;
      if (!rule.asset[0] || !EiemModEquals(rule.asset, asset)) continue;
      if (rule.matchVertices >= 0 && vertices >= 0 &&
          rule.matchVertices != vertices) continue;
      if (rule.matchIndices >= 0 && indices >= 0 && rule.matchIndices != indices) continue;
      if (rule.matchSubMeshes >= 0 && subMeshes >= 0 &&
          rule.matchSubMeshes != subMeshes) continue;
      if (!fallbackPath[0]) {
        strncpy_s(fallbackPath, sizeof(fallbackPath), rule.path, _TRUNCATE);
        candidate = rule;
        found = true;
      } else if (!EiemModSameLogicalPath(fallbackPath, rule.path)) {
        ambiguous = true;
        break;
      }
    }
    if (ambiguous) found = false;
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  if (found) *out = candidate;
  return found;
}

// Resource names in Render sections are section names, never file paths. Keep
// this lookup here so the Unity backend does not have to parse mod.ini again.
static bool EiemFindModResource(const char *modIni, const char *section,
                                const char *kind, EiemModResource *out) {
  if (!modIni || !section || !out) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (const auto &resource : s_eiemModResources) {
    if (_stricmp(resource.modPath, modIni) != 0 ||
        _stricmp(resource.section, section) != 0)
      continue;
    if (kind && kind[0] && _stricmp(resource.kind, kind) != 0) continue;
    *out = resource;
    found = true;
    break;
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  return found;
}

// Finds a resource declaration that opted into global logical redirection.
// Matching is intentionally strict on type and path; asset name is an
// optional disambiguator for bundles containing multiple sub-assets.
static bool EiemFindGlobalResource(const char *path, const char *asset,
                                   const char *kind, EiemModResource *out) {
  if (!path || !path[0] || !out) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (auto it = s_eiemModResources.rbegin();
       it != s_eiemModResources.rend(); ++it) {
    const auto &resource = *it;
    if (!resource.targetPath[0] ||
        !EiemModSameLogicalPath(resource.targetPath, path))
      continue;
    if (kind && kind[0] && _stricmp(resource.kind, kind) != 0) continue;
    if (resource.targetAsset[0] &&
        (!asset || !asset[0] || !EiemModEquals(resource.targetAsset, asset)))
      continue;
    *out = resource;
    found = true;
    break;
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  return found;
}

// A few game paths expose only the final sub-asset name at the proxy boundary.
// Allow an asset-only fallback only when that name identifies one declaration
// unambiguously; never choose between resources with the same name.
static bool EiemFindGlobalResourceByAsset(const char *asset, const char *kind,
                                          EiemModResource *out) {
  if (!asset || !asset[0] || !out) return false;
  bool found = false;
  EiemModResource candidate = {};
  AcquireSRWLockShared(&s_eiemModLock);
  for (auto it = s_eiemModResources.rbegin();
       it != s_eiemModResources.rend(); ++it) {
    const auto &resource = *it;
    if (!resource.targetPath[0] || !resource.targetAsset[0] ||
        !EiemModEquals(resource.targetAsset, asset))
      continue;
    if (kind && kind[0] && _stricmp(resource.kind, kind) != 0) continue;
    if (!found) {
      candidate = resource;
      found = true;
      continue;
    }
    if (!EiemModSameLogicalPath(candidate.targetPath, resource.targetPath) ||
        _stricmp(candidate.modPath, resource.modPath) != 0 ||
        _stricmp(candidate.section, resource.section) != 0) {
      found = false;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  if (found) *out = candidate;
  return found;
}

static bool EiemFindRenderRuleBySection(const char *modIni, const char *section,
                                        EiemModRule *out) {
  if (!modIni || !section || !section[0] || !out) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (auto it = s_eiemModRules.rbegin(); it != s_eiemModRules.rend(); ++it) {
    if (_stricmp(it->modPath, modIni) == 0 &&
        _stricmp(it->section, section) == 0) {
      *out = *it;
      found = true;
      break;
    }
  }
  ReleaseSRWLockShared(&s_eiemModLock);
  return found;
}
