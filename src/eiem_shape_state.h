#pragma once
#include "eiem_mod_document.h"
#include "eiem_shape_binding.h"

struct EiemShapeRuntimeBinding;

struct EiemShapeBaseline { std::string name; float value; };
struct EiemShapeMotion { std::string name; float target; float speed; };
struct EiemShapeState {
  void *mesh = nullptr;
  std::vector<EiemShapeBaseline> owned;
  std::vector<EiemShapeMotion> motions;
  std::shared_ptr<EiemShapeRuntimeBinding> binding;
};

static bool EiemShapeStateAnimating(const EiemShapeState &state) {
  return !state.motions.empty();
}

// Per Renderer ownership. Resolve names against the CURRENT Mesh; an index is
// not a resource identity. Backend must report failed calls, never guess index 0.
template <typename Backend>
static bool EiemApplyShapeWeights(void *renderer, void *mesh, const EiemModRule &rule,
                                   EiemShapeState &state, Backend &backend, std::string &error,
                                   float elapsedSeconds = -1.0f) {
  if (!mesh) { error = "Shape Mesh is unavailable"; return false; }
  if (state.mesh != mesh) { state.mesh = mesh; state.owned.clear(); state.motions.clear(); }
  struct Write { int index; float value; std::string name; };
  std::vector<Write> writes;
  std::vector<EiemShapeBaseline> next;
  std::vector<EiemShapeMotion> motions;
  auto fail = [&](const std::string &name) { error = "Shape channel unavailable/call failed: " + name; return false; };
  for (uint32_t i = 0; i < rule.shapeCount; ++i) {
    const std::string name = rule.shapeNames[i];
    const int index = backend.Index(mesh, name);
    float current = 0;
    if (index < 0 || !backend.Read(renderer, index, current)) return fail(name);
    const float target = rule.shapeWeights[i] * 100.0f;
    if (!std::isfinite(target)) return fail(name);
    float speed = 0;
    for (uint32_t j = 0; j < rule.shapeSpeedCount; ++j)
      if (name == rule.shapeSpeedNames[j]) { speed = rule.shapeSpeeds[j] * 100.0f; break; }
    auto old = std::find_if(state.owned.begin(), state.owned.end(), [&](const auto &entry) { return entry.name == name; });
    next.push_back({name, old != state.owned.end() ? old->value : current});
    float value = target;
    if (speed > 0 && current != target) {
      motions.push_back({name,target,speed});
      if (elapsedSeconds < 0) continue;
      const float distance = speed * elapsedSeconds;
      value = current < target ? (std::min)(current + distance,target)
                               : (std::max)(current - distance,target);
    }
    if (current != value) writes.push_back({index, value, name});
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
  state.motions = std::move(motions);
  for (const auto &write : writes) {
    float actual = 0;
    if (!backend.Write(renderer, write.index, write.value) ||
        !backend.Read(renderer, write.index, actual) || std::abs(actual - write.value) > .0001f) return fail(write.name);
  }
  state.motions.erase(std::remove_if(state.motions.begin(), state.motions.end(),
    [&](const auto &motion) {
      auto write = std::find_if(writes.begin(), writes.end(), [&](const auto &entry) {
        return entry.name == motion.name;
      });
      return write != writes.end() && write->value == motion.target;
    }), state.motions.end());
  state.owned = std::move(next);
  return true;
}
