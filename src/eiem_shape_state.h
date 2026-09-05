#pragma once
#include "eiem_mod_document.h"

struct EiemShapeBaseline { std::string name; float value; };
struct EiemShapeState {
  void *mesh = nullptr;
  std::vector<EiemShapeBaseline> owned;
};

// Per Renderer ownership. Resolve names against the CURRENT Mesh; an index is
// not a resource identity. Backend must report failed calls, never guess index 0.
template <typename Backend>
static bool EiemApplyShapeWeights(void *renderer, void *mesh, const EiemModRule &rule,
                                   EiemShapeState &state, Backend &backend, std::string &error) {
  if (!mesh) { error = "Shape Mesh is unavailable"; return false; }
  if (state.mesh != mesh) { state.mesh = mesh; state.owned.clear(); }
  struct Write { int index; float value; std::string name; };
  std::vector<Write> writes;
  std::vector<EiemShapeBaseline> next;
  auto fail = [&](const std::string &name) { error = "Shape channel unavailable/call failed: " + name; return false; };
  for (uint32_t i = 0; i < rule.shapeCount; ++i) {
    const std::string name = rule.shapeNames[i];
    const int index = backend.Index(mesh, name);
    float current = 0;
    if (index < 0 || !backend.Read(renderer, index, current)) return fail(name);
    const float target = rule.shapeWeights[i] * 100.0f;
    if (!std::isfinite(target)) return fail(name);
    auto old = std::find_if(state.owned.begin(), state.owned.end(), [&](const auto &entry) { return entry.name == name; });
    next.push_back({name, old != state.owned.end() ? old->value : current});
    if (current != target) writes.push_back({index, target, name});
  }
  for (const auto &old : state.owned) {
    if (std::find_if(next.begin(), next.end(), [&](const auto &entry) { return entry.name == old.name; }) != next.end()) continue;
    int index = backend.Index(mesh, old.name);
    float current = 0;
    if (index < 0 || !backend.Read(renderer, index, current)) return fail(old.name);
    if (current != old.value) writes.push_back({index, old.value, old.name});
  }
  // Track every potentially written channel even if a later native call fails,
  // so partial writes remain restorable. Commit reduced ownership only on success.
  for (const auto &entry : next)
    if (std::find_if(state.owned.begin(), state.owned.end(), [&](const auto &old) { return old.name == entry.name; }) == state.owned.end())
      state.owned.push_back(entry);
  for (const auto &write : writes) {
    float actual = 0;
    if (!backend.Write(renderer, write.index, write.value) ||
        !backend.Read(renderer, write.index, actual) || std::abs(actual - write.value) > .0001f) return fail(write.name);
  }
  state.owned = std::move(next);
  return true;
}
