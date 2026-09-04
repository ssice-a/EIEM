#pragma once
#include <atomic>
#include <cstdint>

// Requests, not Unity work. Hotkeys/manager callbacks only enqueue. The model
// dispatcher consumes on Unity's thread; a burst must not lose a reload.
enum class EiemModUpdate : uint32_t {
  Reconcile = 1, // lifecycle event: retry registered models, no config change
  Reapply = 2,   // state change: restore/replay, no disk parsing
  Reload = 4,    // F10: restore, load/publish config, replay
};

class EiemModUpdateQueue {
 public:
  bool Request(EiemModUpdate request) {
    return pending_.fetch_or((uint32_t)request) == 0;
  }
  uint32_t Take() { return pending_.exchange(0); }
 private:
  std::atomic<uint32_t> pending_{0};
};

// The same ordering is used by runtime and native tests. Resource creation and
// restoration stay in their existing implementations, not in the input layer.
template <typename Restore, typename Reload, typename Replay>
static void EiemDispatchModUpdate(uint32_t requests, Restore restore,
                                  Reload reload, Replay replay) {
  if (!requests) return;
  if (requests & ((uint32_t)EiemModUpdate::Reapply | (uint32_t)EiemModUpdate::Reload))
    restore();
  if (requests & (uint32_t)EiemModUpdate::Reload) reload();
  replay();
}
