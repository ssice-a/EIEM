#pragma once
#include <windows.h>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdlib>

struct EiemKeyChord {
  UINT vk = 0;
  UINT modifiers = 0;
  bool operator==(const EiemKeyChord &other) const {
    return vk == other.vk && modifiers == other.modifiers;
  }
};

static void EiemKeyTrim(std::string &s) {
  auto nonspace = [](unsigned char c) { return !std::isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), nonspace));
  s.erase(std::find_if(s.rbegin(), s.rend(), nonspace).base(), s.end());
}

static UINT EiemKeyCode(std::string name) {
  EiemKeyTrim(name);
  std::transform(name.begin(), name.end(), name.begin(),
                 [](unsigned char c) { return (char)std::toupper(c); });
  if (name.size() == 1 && std::isalnum((unsigned char)name[0])) return name[0];
  if (name.size() > 1 && name[0] == 'F') {
    char *end = nullptr;
    long n = std::strtol(name.c_str() + 1, &end, 10);
    if (!*end && n >= 1 && n <= 24) return VK_F1 + (UINT)n - 1;
  }
  struct Entry { const char *name; UINT vk; };
  static const Entry entries[] = {
    {"INSERT", VK_INSERT}, {"DELETE", VK_DELETE}, {"HOME", VK_HOME},
    {"END", VK_END}, {"PAGEUP", VK_PRIOR}, {"PAGEDOWN", VK_NEXT},
    {"LEFT", VK_LEFT}, {"RIGHT", VK_RIGHT}, {"UP", VK_UP}, {"DOWN", VK_DOWN},
    {"SPACE", VK_SPACE}, {"ENTER", VK_RETURN}, {"ESC", VK_ESCAPE},
    {"TAB", VK_TAB}, {"BACKSPACE", VK_BACK}, {"CAPSLOCK", VK_CAPITAL},
    {"TILDE", VK_OEM_3},
    {"NUMPAD0", VK_NUMPAD0}, {"NUMPAD1", VK_NUMPAD1},
    {"NUMPAD2", VK_NUMPAD2}, {"NUMPAD3", VK_NUMPAD3},
    {"NUMPAD4", VK_NUMPAD4}, {"NUMPAD5", VK_NUMPAD5},
    {"NUMPAD6", VK_NUMPAD6}, {"NUMPAD7", VK_NUMPAD7},
    {"NUMPAD8", VK_NUMPAD8}, {"NUMPAD9", VK_NUMPAD9},
    {"NUMPADPLUS", VK_ADD}, {"NUMPADMINUS", VK_SUBTRACT},
    {"NUMPADMULTIPLY", VK_MULTIPLY}, {"NUMPADDIVIDE", VK_DIVIDE},
    {"NUMPADDECIMAL", VK_DECIMAL}
  };
  for (const auto &entry : entries) if (name == entry.name) return entry.vk;
  return 0;
}

// One spelling for chords: Ctrl+Shift+F6. No OS-wide registration happens here.
static bool EiemParseKeyChord(const std::string &text, EiemKeyChord *out) {
  EiemKeyChord chord;
  size_t start = 0;
  while (start <= text.size()) {
    const size_t end = text.find('+', start);
    std::string part = text.substr(start, end == std::string::npos ? end : end - start);
    EiemKeyTrim(part);
    std::transform(part.begin(), part.end(), part.begin(),
                   [](unsigned char c) { return (char)std::toupper(c); });
    UINT modifier = part == "CTRL" ? MOD_CONTROL : part == "SHIFT" ? MOD_SHIFT :
                    part == "ALT" ? MOD_ALT : 0;
    if (modifier) {
      if (chord.modifiers & modifier) return false;
      chord.modifiers |= modifier;
    } else {
      UINT vk = EiemKeyCode(part);
      if (!vk || chord.vk) return false;
      chord.vk = vk;
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (!chord.vk) return false;
  *out = chord;
  return true;
}

static std::string EiemFormatKeyChord(EiemKeyChord chord) {
  std::string result;
  if (chord.modifiers & MOD_CONTROL) result += "Ctrl+";
  if (chord.modifiers & MOD_SHIFT) result += "Shift+";
  if (chord.modifiers & MOD_ALT) result += "Alt+";
  if (chord.vk >= '0' && chord.vk <= '9')
    result.push_back((char)chord.vk);
  else if (chord.vk >= 'A' && chord.vk <= 'Z')
    result.push_back((char)chord.vk);
  else if (chord.vk >= VK_F1 && chord.vk <= VK_F24)
    result += "F" + std::to_string(chord.vk - VK_F1 + 1);
  else if (chord.vk >= VK_NUMPAD0 && chord.vk <= VK_NUMPAD9)
    result += "Numpad" + std::to_string(chord.vk - VK_NUMPAD0);
  else {
    struct Entry { UINT vk; const char *name; };
    static const Entry entries[] = {
      {VK_INSERT,"Insert"},{VK_DELETE,"Delete"},{VK_HOME,"Home"},
      {VK_END,"End"},{VK_PRIOR,"PageUp"},{VK_NEXT,"PageDown"},
      {VK_LEFT,"Left"},{VK_RIGHT,"Right"},{VK_UP,"Up"},{VK_DOWN,"Down"},
      {VK_SPACE,"Space"},{VK_RETURN,"Enter"},{VK_ESCAPE,"Esc"},
      {VK_TAB,"Tab"},{VK_BACK,"Backspace"},{VK_CAPITAL,"CapsLock"},
      {VK_ADD,"Numpad+"},{VK_SUBTRACT,"Numpad-"},
      {VK_MULTIPLY,"Numpad*"},{VK_DIVIDE,"Numpad/"},
      {VK_DECIMAL,"Numpad."}
    };
    for (const auto &entry : entries)
      if (entry.vk == chord.vk) { result += entry.name; break; }
    if (result.empty() || result.back() == '+')
      result += "VK" + std::to_string(chord.vk);
  }
  return result;
}
