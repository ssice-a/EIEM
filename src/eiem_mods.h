#pragma once

#include "eiem_mod_document.h"
#include <utility>

// Configuration publication and lookups; parsing has no runtime state.
static SRWLOCK s_eiemModLock = SRWLOCK_INIT;
static EiemModProgram s_eiemModProgram;
static volatile LONG s_eiemModGeneration = 0;

// Called before hooks at startup; subsequently only by the Unity-thread update
// dispatcher, after restoring effects of the previously published program.
static void EiemReloadMods() {
  EiemModProgram next;
  std::vector<std::string> files;
  WIN32_FIND_DATAA data = {};
  HANDLE root = FindFirstFileA("plugin\\mods\\*", &data);
  if (root != INVALID_HANDLE_VALUE) {
    do {
      if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || data.cFileName[0] == '.')
        continue;
      files.push_back(std::string("plugin\\mods\\") + data.cFileName + "\\mod.ini");
    } while (FindNextFileA(root, &data));
    FindClose(root);
  }
  // File enumeration order is unspecified. Preserve deterministic conflict
  // precedence: sorted mod folders first, optional root mod.ini last.
  std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) {
    return _stricmp(a.c_str(), b.c_str()) < 0;
  });
  files.push_back("plugin\\mods\\mod.ini");
  for (const auto &file : files) {
    std::string error;
    if (!EiemModParseFile(file.c_str(), next.prefabs, next.rules, next.resources, &error) &&
        !error.empty())
      Log("[MOD] Invalid configuration %s:%s (file skipped)", file.c_str(), error.c_str());
  }
  EiemCompileModProgram(next);
  for (const auto &prefab : next.prefabs)
    Log("[MOD] prefab section=%s path=%s renders=%u", prefab.section, prefab.path, prefab.renderCount);
  for (const auto &rule : next.rules)
    Log("[MOD] rule section=%s path=%s asset=%s handling=%s mesh=%s partners=%u",
        rule.section, rule.path, rule.asset, rule.handling[0] ? rule.handling : "<none>",
        rule.hasMesh ? rule.mesh : "<none>", rule.partnerCount);
  const size_t prefabCount = next.prefabs.size(), ruleCount = next.rules.size();
  const size_t resourceCount = next.resources.size(), standaloneCount = next.standaloneRules.size();
  AcquireSRWLockExclusive(&s_eiemModLock);
  s_eiemModProgram = std::move(next);
  const LONG generation = InterlockedIncrement(&s_eiemModGeneration);
  ReleaseSRWLockExclusive(&s_eiemModLock);
  Log("[MOD] Reloaded %zu Prefab declarations, %zu Render rules (%zu standalone), %zu resources, generation=%ld",
      prefabCount, ruleCount, standaloneCount, resourceCount, generation);
}

static void EiemFindModPrefabs(const char *path,
                               std::vector<EiemModPrefab> *out) {
  if (!out) return;
  out->clear();
  if (!path || !path[0]) return;
  AcquireSRWLockShared(&s_eiemModLock);
  for (const auto &prefab : s_eiemModProgram.prefabs) {
    if (EiemModSameLogicalPath(prefab.path, path)) out->push_back(prefab);
  }
  ReleaseSRWLockShared(&s_eiemModLock);
}

// Top-level rules match Mesh identity on every live consumer. Referenced
// Render sections are scoped PFB actions or partner templates, never promoted.
static void EiemFindStandaloneRenderRules(std::vector<EiemModRule> *out) {
  if (!out) return;
  out->clear();
  AcquireSRWLockShared(&s_eiemModLock);
  for (size_t index : s_eiemModProgram.standaloneRules)
    out->push_back(s_eiemModProgram.rules[index]);
  ReleaseSRWLockShared(&s_eiemModLock);
}

static bool EiemHasStandaloneRenderRules() {
  AcquireSRWLockShared(&s_eiemModLock);
  const bool found = !s_eiemModProgram.standaloneRules.empty();
  ReleaseSRWLockShared(&s_eiemModLock);
  return found;
}

// Resource names in Render sections are section names, never file paths. Keep
// this lookup here so the Unity backend does not have to parse mod.ini again.
static bool EiemFindModResource(const char *modIni, const char *section,
                                const char *kind, EiemModResource *out) {
  if (!modIni || !section || !out) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (const auto &resource : s_eiemModProgram.resources) {
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

static bool EiemFindRenderRuleBySection(const char *modIni, const char *section,
                                        EiemModRule *out) {
  if (!modIni || !section || !section[0] || !out) return false;
  bool found = false;
  AcquireSRWLockShared(&s_eiemModLock);
  for (auto it = s_eiemModProgram.rules.rbegin(); it != s_eiemModProgram.rules.rend(); ++it) {
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
