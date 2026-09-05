#pragma once
#include <vector>
#include <algorithm>
#include <cstddef>

// Undo only the slots this mod wrote. Preserve the game's other slots and
// remove only trailing slots that were added by this override.
template <typename T>
static std::vector<T> EiemRestoreOwnedSlots(std::vector<T> current,
                                           const std::vector<T> &original,
                                           const std::vector<size_t> &owned) {
  for (size_t slot : owned)
    if (slot < current.size()) current[slot] = slot < original.size() ? original[slot] : T{};
  while (current.size() > original.size() &&
         std::find(owned.begin(), owned.end(), current.size() - 1) != owned.end())
    current.pop_back();
  return current;
}
