"""Run the production hotkey worker against a deterministic fake OS message queue."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import function

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#include <windows.h>
#include <cstdio>
#include <sstream>
#include <map>
#include <deque>
#include <vector>
static int g_guiToggleVK = VK_INSERT, g_modReloadVK = VK_F10;
static void Log(const char *, ...) {}
#include "eiem_config.h"
#include "eiem_mods.h"
#include "eiem_mod_update.h"
static bool g_shutdownRequested = false, g_guiRunning = true, g_pluginActive = true;
static HWND g_gameHwnd = nullptr, g_guiHwnd = (HWND)2, g_modUiHwnd = (HWND)3;
static WNDPROC g_origWndProc = nullptr;
static HWND foreground = (HWND)1;
static void *il2cpp_domain_get() { return nullptr; }
static void il2cpp_thread_attach(void *) {}
static HWND FindGameWindow() { return (HWND)1; }
static bool IsWindowAlive(HWND) { return true; }
static LRESULT CALLBACK MmdWndProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
#define WM_EIEM_MOD_KEY (WM_APP + 0x317)
static int guiCalls = 0, reloadCalls = 0, ticks = 0;
static bool rebound = false, unregisteredAway = false, norepeat = true, uiOnly = false;
static std::map<int, EiemKeyChord> osBindings;
static std::deque<MSG> incoming;
static std::vector<EiemModInputEvent> posted;
static void ToggleGui() { ++guiCalls; }
static void EiemRequestModUpdate(EiemModUpdate update, const char *) {
  if (update == EiemModUpdate::Reload) ++reloadCalls;
}
static BOOL FakeRegisterHotKey(HWND, int id, UINT flags, UINT vk) {
  norepeat = norepeat && (flags & MOD_NOREPEAT);
  osBindings[id] = {vk, flags & ~MOD_NOREPEAT}; return TRUE;
}
static BOOL FakeUnregisterHotKey(HWND, int id) { osBindings.erase(id); return TRUE; }
static BOOL FakePeekMessageW(LPMSG out, HWND, UINT, UINT, UINT remove) {
  if (incoming.empty()) return FALSE;
  *out = incoming.front(); if (remove & PM_REMOVE) incoming.pop_front(); return TRUE;
}
static BOOL FakePostMessageW(HWND, UINT msg, WPARAM key, LPARAM generation) {
  if (msg == WM_EIEM_MOD_KEY) posted.push_back({{LOWORD(key), HIWORD(key)}, (LONG)generation});
  return TRUE;
}
static LONG_PTR FakeSetWindowLongPtrW(HWND, int, LONG_PTR) { return (LONG_PTR)MmdWndProc; }
static BOOL FakeIsWindow(HWND) { return TRUE; }
static HWND FakeGetForegroundWindow() { return foreground; }
static void Press(UINT vk) {
  for (const auto &pair : osBindings) if (pair.second.vk == vk) {
    MSG msg = {}; msg.message = WM_HOTKEY; msg.wParam = pair.first;
    incoming.push_back(msg);
  }
}
static void FakeSleep(DWORD) {
  ++ticks;
  if (ticks == 1) { Press(VK_F6); Press(VK_F6); }
  if (ticks == 2) { s_eiemGlobalConfig.reload.vk = VK_F8; ++s_eiemGlobalConfigGeneration; }
  if (ticks == 3) {
    bool hasNew = false, hasOld = false;
    for (const auto &pair : osBindings) { hasNew |= pair.second.vk == VK_F8; hasOld |= pair.second.vk == VK_F10; }
    rebound = hasNew && !hasOld;
    Press(VK_F8);
  }
  if (ticks == 4) foreground = (HWND)99;
  if (ticks == 5) { unregisteredAway = osBindings.empty(); foreground = (HWND)1; }
  if (ticks == 6) Press(VK_F6);
  if (ticks == 7) foreground = g_modUiHwnd;
  if (ticks == 8) {
    bool hasUi=false, hasCycle=false;
    for (const auto &pair:osBindings) { hasUi |= pair.second.vk==VK_F9; hasCycle |= pair.second.vk==VK_F6; }
    uiOnly=hasUi && !hasCycle;
    Press(VK_F9); Press(VK_F8); Press(VK_INSERT);
  }
  if (ticks >= 9) g_guiRunning = false;
}
#define RegisterHotKey FakeRegisterHotKey
#define UnregisterHotKey FakeUnregisterHotKey
#define PeekMessageW FakePeekMessageW
#define PostMessageW FakePostMessageW
#define SetWindowLongPtrW FakeSetWindowLongPtrW
#define IsWindow FakeIsWindow
#define GetForegroundWindow FakeGetForegroundWindow
#define Sleep FakeSleep
'''

MAIN = r'''
int main() {
  EiemModProgram program;
  std::string error;
  std::istringstream input("[Constants]\n$a=0\n[KeyA]\nkey=F6\ntype=cycle\n$a=0,1\n"
    "[UIA]\npath=ui.lua\nkey=F9\n[UIB]\npath=other.lua\nkey=F9\n");
  if (!EiemModParseStream(input, "a/mod.ini", program, &error)) return 1;
  EiemPublishModState(program); s_eiemModGeneration = 9;
  HotkeyThread(nullptr);
  if (!rebound || !unregisteredAway || !norepeat || !uiOnly || !osBindings.empty() || reloadCalls != 2 || guiCalls != 1 || posted.size() != 4) {
    std::fprintf(stderr, "rebound=%d away=%d norepeat=%d bindings=%zu reload=%d posts=%zu\n",
      rebound, unregisteredAway, norepeat, osBindings.size(), reloadCalls, posted.size());
    return 2;
  }
  for (size_t i=0;i<posted.size();++i)
    if (posted[i].chord.vk != (i<3 ? VK_F6 : VK_F9) || posted[i].generation != 9) return 3;
  return 0;
}
'''


class HotkeyWorkerTests(unittest.TestCase):
    def test_real_worker_rebind_focus_edges_and_order(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC developer environment")
        source_text = (ROOT / "src/init.h").read_text(encoding="utf-8")
        implementation = function(source_text, "static DWORD WINAPI HotkeyThread(")
        with tempfile.TemporaryDirectory(prefix="eiem-hotkey-") as folder:
            folder = Path(folder)
            source = folder / "hotkeys.cpp"
            source.write_text(PREFIX + implementation + MAIN, encoding="utf-8")
            exe = folder / "hotkeys.exe"
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}", "user32.lib"],
                                   cwd=folder, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(exe)], cwd=folder, capture_output=True, text=True, timeout=10)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__": unittest.main()
