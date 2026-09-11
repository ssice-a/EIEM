#pragma once

#include "eiem_mod_document.h"
#include "eiem_persistent_state.h"
#include <utility>

// Configuration publication and lookups; parsing has no runtime state.
static SRWLOCK s_eiemModLock = SRWLOCK_INIT;
static EiemModProgram s_eiemModProgram;
static volatile LONG s_eiemModGeneration = 0;
static EiemPersistentStore s_eiemPersistentStates;

static bool EiemModAffected(const char *path, const std::vector<std::string> *mods) {
  if (!mods) return true;
  for (const auto &mod : *mods) if (EiemModEquals(mod.c_str(), path)) return true;
  return false;
}

static std::vector<EiemKeyChord> EiemGetModKeyChords(LONG *generation, bool uiFocus = false) {
  std::vector<EiemKeyChord> result;
  AcquireSRWLockShared(&s_eiemModLock);
  *generation = s_eiemModGeneration;
  for (const auto &state : s_eiemModProgram.states) for (const auto &key : state.keys)
    if (EiemKeyInScope(key.scope,uiFocus) && std::find(result.begin(), result.end(), key.chord) == result.end()) result.push_back(key.chord);
  ReleaseSRWLockShared(&s_eiemModLock);
  return result;
}

struct EiemModInputEvent {
  EiemKeyChord chord; LONG generation;
  std::string modPath, uiSection; // empty for a key event
  EiemVariables values; // one successful Lua frame, committed atomically
  bool uiFocus = false; // focus at key dispatch; used only by generic Key scope
};

struct EiemUiSnapshot {
  std::string modPath;
  EiemModUi ui;
  EiemVariables variables;
  LONG generation;
  EiemVariables defaults;
};

// A Physics action is selected by the same Render rule that selected its Mesh
// consumer.  It is nevertheless owned by the completed model instance, not by
// an individual Renderer: several Renderer/LOD hits can reference one prepared
// Physics snapshot and must produce one native instance when that adapter is
// connected.
struct EiemPhysicsIntent {
  char modPath[MAX_PATH] = {};
  char resourceSection[96] = {};
  char firstRenderSection[96] = {};
  EiemModRule firstRule = {};
  std::shared_ptr<const EiemPhysicsAsset> asset;
  // Rebuilt on every model pass. These are observation addresses only; the
  // runtime adapter captures and validates its own weak references before use.
  std::vector<void *> matchedRenderers;
  uint32_t rendererMatches = 0;
};

static std::vector<EiemUiSnapshot> EiemGetModUis(LONG *generation) {
  std::vector<EiemUiSnapshot> result;
  AcquireSRWLockShared(&s_eiemModLock);
  *generation = s_eiemModGeneration;
  for (const auto &state : s_eiemModProgram.states)
    for (const auto &ui : state.uis)
      result.push_back({state.path, ui, state.variables, s_eiemModGeneration,state.defaults});
  ReleaseSRWLockShared(&s_eiemModLock);
  return result;
}

static bool EiemSameRenderAssembly(EiemModRule a, EiemModRule b) {
  // New/removed channels may need to discover a previously match-only consumer.
  if (a.shapeCount != b.shapeCount) return false;
  for (uint32_t i = 0; i < a.shapeCount; ++i)
    if (strcmp(a.shapeNames[i], b.shapeNames[i])) return false;
  memset(a.shapeNames, 0, sizeof(a.shapeNames)); memset(b.shapeNames, 0, sizeof(b.shapeNames));
  memset(a.shapeWeights, 0, sizeof(a.shapeWeights)); memset(b.shapeWeights, 0, sizeof(b.shapeWeights));
  a.shapeCount = b.shapeCount = 0;
  memset(a.shapeSpeedNames, 0, sizeof(a.shapeSpeedNames));
  memset(b.shapeSpeedNames, 0, sizeof(b.shapeSpeedNames));
  memset(a.shapeSpeeds, 0, sizeof(a.shapeSpeeds));
  memset(b.shapeSpeeds, 0, sizeof(b.shapeSpeeds));
  a.shapeSpeedCount = b.shapeSpeedCount = 0;
  return memcmp(&a, &b, sizeof(a)) == 0;
}

static bool EiemSameRenderWithoutPartnerLinks(EiemModRule a,
                                              EiemModRule b) {
  memset(a.partners, 0, sizeof(a.partners));
  memset(b.partners, 0, sizeof(b.partners));
  a.partnerCount = b.partnerCount = 0;
  return memcmp(&a, &b, sizeof(a)) == 0;
}

static bool EiemPrepareInputUpdate(const std::vector<EiemModInputEvent> &events,
                                  EiemModProgram *next, std::vector<std::string> *affected,
                                  bool *shapesOnly = nullptr,
                                  bool *partnerLinksOnly = nullptr) {
  AcquireSRWLockShared(&s_eiemModLock);
  *next = s_eiemModProgram;
  const LONG generation = s_eiemModGeneration;
  ReleaseSRWLockShared(&s_eiemModLock);
  const auto before = next->rules;
  affected->clear();
  for (const auto &event : events) {
    if (event.generation != generation) continue; // queued before an F10 reset
    std::vector<std::string> changed;
    if (event.uiSection.empty()) changed = EiemCycleModKey(*next, event.chord, event.uiFocus);
    else {
      for (size_t i = 0; i < next->states.size(); ++i) {
        auto &state = next->states[i];
        if (!EiemModEquals(state.path.c_str(), event.modPath.c_str())) continue;
        for (const auto &ui : state.uis) {
          if (ui.section != event.uiSection) continue;
          const auto previous = state.variables;
          std::string error;
          if (!EiemApplyUiValues(*next, i, event.values, error))
            Log("[UI] Rejected transaction %s/%s: %s", state.path.c_str(), ui.section.c_str(), error.c_str());
          else if (previous != state.variables) changed.push_back(state.path);
        }
      }
      if (!changed.empty()) EiemEvaluateModProgram(*next);
    }
    for (const auto &path : changed)
      if (std::find(affected->begin(), affected->end(), path) == affected->end()) affected->push_back(path);
  }
  if (shapesOnly) {
    *shapesOnly = before.size() == next->rules.size();
    for (size_t i = 0; *shapesOnly && i < before.size(); ++i)
      *shapesOnly = EiemSameRenderAssembly(before[i], next->rules[i]);
  }
  if (partnerLinksOnly) {
    *partnerLinksOnly = before.size() == next->rules.size();
    for (size_t i = 0; *partnerLinksOnly && i < before.size(); ++i)
      *partnerLinksOnly =
          EiemSameRenderWithoutPartnerLinks(before[i], next->rules[i]);
  }
  return !affected->empty();
}

static void EiemPublishModState(EiemModProgram next) {
  s_eiemPersistentStates.Queue(next);
  AcquireSRWLockExclusive(&s_eiemModLock);
  s_eiemModProgram = std::move(next);
  ReleaseSRWLockExclusive(&s_eiemModLock);
}

// Called before hooks at startup; subsequently only by the Unity-thread update
// dispatcher, after restoring effects of the previously published program.
static void EiemReloadMods() {
  s_eiemPersistentStates.Flush(true);
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
    std::string error; EiemModProgram document;
    if (!EiemModParseFile(file.c_str(), document, &error)) {
      if (!error.empty()) Log("[MOD] Invalid configuration %s:%s (file skipped)", file.c_str(), error.c_str());
      continue;
    }
    EiemAppendModDocument(next,std::move(document));
  }
  s_eiemPersistentStates.Load(next);
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

// Source Render rules match Mesh identity on every live consumer. A PFB
// reference is only a resource relationship and does not remove a rule from
// this set. Only Render sections used as partner templates are excluded.
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

// Called only after a Render rule has won normal first-match precedence for a
// concrete Renderer under a registered model root.  Aliases of the same
// prepared file share a snapshot, so snapshot identity plus Mod identity is the
// stable deduplication key.  The resource name of the first hit is retained for
// diagnostics; it is not the runtime ownership key.
static bool EiemCollectPhysicsIntent(
    const EiemModRule &rule, std::vector<EiemPhysicsIntent> *out,
    void *matchedRenderer = nullptr) {
  if (!rule.hasPhysics) return true;
  if (!out) return false;
  EiemModResource resource = {};
  if (!EiemFindModResource(rule.modPath, rule.physics, "Physics", &resource) ||
      !resource.physicsAsset)
    return false;
  for (auto &intent : *out) {
    if (!EiemModEquals(intent.modPath, rule.modPath) ||
        intent.asset.get() != resource.physicsAsset.get())
      continue;
    ++intent.rendererMatches;
    if (matchedRenderer &&
        std::find(intent.matchedRenderers.begin(), intent.matchedRenderers.end(),
                  matchedRenderer) == intent.matchedRenderers.end())
      intent.matchedRenderers.push_back(matchedRenderer);
    return true;
  }
  EiemPhysicsIntent intent = {};
  strncpy_s(intent.modPath, sizeof(intent.modPath), rule.modPath, _TRUNCATE);
  strncpy_s(intent.resourceSection, sizeof(intent.resourceSection),
            rule.physics, _TRUNCATE);
  strncpy_s(intent.firstRenderSection, sizeof(intent.firstRenderSection),
            rule.section, _TRUNCATE);
  intent.firstRule = rule;
  intent.asset = std::move(resource.physicsAsset);
  if (matchedRenderer) intent.matchedRenderers.push_back(matchedRenderer);
  intent.rendererMatches = 1;
  out->push_back(std::move(intent));
  return true;
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
