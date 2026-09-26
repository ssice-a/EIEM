#pragma once

#include <filesystem>
#include <fstream>

// Mod strings are UTF-8. Convert at the disk boundary so the narrow CRT never
// interprets a filename in the system code page.
static std::ifstream EiemOpenUtf8Input(
    const char *path, std::ios::openmode mode = std::ios::binary) {
  std::ifstream input;
  if (path && path[0]) {
    try {
      input.open(std::filesystem::u8path(path), mode);
    } catch (const std::exception &) {
      input.setstate(std::ios::failbit);
    }
  } else {
    input.setstate(std::ios::failbit);
  }
  return input;
}

// Absolute paths include the game directory, which can itself be Unicode.
// Declaration limits are separate from this resolved UTF-8 disk path buffer.
static constexpr size_t kEiemResourceDiskPathCapacity = 4096;
