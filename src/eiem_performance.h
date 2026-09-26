#pragma once

#include <windows.h>

// Low-overhead aggregate timers for startup/runtime boundaries.  They never
// log per call; callers decide when to emit a compact snapshot.
struct EiemPerfCounter {
  volatile LONG64 calls = 0;
  volatile LONG64 ticks = 0;
  volatile LONG64 maximum = 0;
};

static LONG64 EiemPerfNow() {
  LARGE_INTEGER value = {};
  QueryPerformanceCounter(&value);
  return value.QuadPart;
}

static LONG64 EiemPerfFrequency() {
  static LONG64 frequency = []() {
    LARGE_INTEGER value = {};
    QueryPerformanceFrequency(&value);
    return value.QuadPart ? value.QuadPart : 1;
  }();
  return frequency;
}

static LONG64 EiemPerfRecord(EiemPerfCounter &counter, LONG64 started) {
  const LONG64 elapsed = EiemPerfNow() - started;
  InterlockedAdd64(&counter.ticks, elapsed);
  LONG64 observed = InterlockedCompareExchange64(&counter.maximum, 0, 0);
  while (elapsed > observed) {
    const LONG64 previous =
        InterlockedCompareExchange64(&counter.maximum, elapsed, observed);
    if (previous == observed) break;
    observed = previous;
  }
  return InterlockedIncrement64(&counter.calls);
}

static LONG64 EiemPerfRead(const volatile LONG64 &value) {
  return InterlockedCompareExchange64(
      const_cast<volatile LONG64 *>(&value), 0, 0);
}

static double EiemPerfMilliseconds(LONG64 ticks) {
  return 1000.0 * static_cast<double>(ticks) /
         static_cast<double>(EiemPerfFrequency());
}

struct EiemPerfScope {
  EiemPerfCounter *counter = nullptr;
  LONG64 started = 0;

  explicit EiemPerfScope(EiemPerfCounter &target)
      : counter(&target), started(EiemPerfNow()) {}
  ~EiemPerfScope() {
    if (counter) EiemPerfRecord(*counter, started);
  }

  EiemPerfScope(const EiemPerfScope &) = delete;
  EiemPerfScope &operator=(const EiemPerfScope &) = delete;
};
