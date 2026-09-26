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
static constexpr bool kEiemEnableLegacyWorkers = true;
static HWND foreground = (HWND)1;
static void EiemPostPendingModUpdate(const char *) {}
static void *il2cpp_domain_get() { return nullptr; }
static void il2cpp_thread_attach(void *) {}
static HWND FindGameWindow() { return (HWND)1; }
static bool IsWindowAlive(HWND) { return true; }
static LRESULT CALLBACK MmdWndProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
static LRESULT CALLBACK EiemModWndProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
#define WM_EIEM_MOD_KEY (WM_APP + 0x317)
#define WM_EIEM_MOD_HOLD (WM_APP + 0x318)
#define WM_EIEM_PHYSICS_CAPTURE (WM_APP + 0x319)
static int guiCalls = 0, reloadCalls = 0, physicsCaptureCalls = 0, ticks = 0;
static bool rebound = false, unregisteredAway = false, norepeat = true, uiOnly = false, managerGameKeys = false;
static bool blockF12 = false, fallbackBound = false;
static std::map<int, EiemKeyChord> osBindings;
static std::map<UINT, bool> keyDown;
static std::deque<MSG> incoming;
static std::vector<EiemModInputEvent> posted;
static void EiemToggleModManager() { ++guiCalls; InterlockedExchange(&s_eiemModManagerOpen, 1); }
static void EiemRequestModUpdate(EiemModUpdate update, const char *) {
  if (update == EiemModUpdate::Reload) {
    ++reloadCalls;
    if (reloadCalls == 1) ++s_eiemModGeneration;
  }
}
static BOOL FakeRegisterHotKey(HWND, int id, UINT flags, UINT vk) {
  if (blockF12 && vk == VK_F12 && !(flags & MOD_CONTROL)) {
    SetLastError(ERROR_HOTKEY_ALREADY_REGISTERED); return FALSE;
  }
  norepeat = norepeat && (flags & MOD_NOREPEAT);
  osBindings[id] = {vk, flags & ~MOD_NOREPEAT};
  if (vk == VK_F12 && (flags & MOD_CONTROL)) fallbackBound = true;
  if (keyDown[vk]) {
    MSG msg = {}; msg.message = WM_HOTKEY; msg.wParam = id;
    incoming.push_back(msg);
  }
  return TRUE;
}
static BOOL FakeUnregisterHotKey(HWND, int id) { osBindings.erase(id); return TRUE; }
static BOOL FakePeekMessageW(LPMSG out, HWND, UINT, UINT, UINT remove) {
  if (incoming.empty()) return FALSE;
  *out = incoming.front(); if (remove & PM_REMOVE) incoming.pop_front(); return TRUE;
}
static BOOL FakePostMessageW(HWND, UINT msg, WPARAM key, LPARAM generation) {
  if (msg == WM_EIEM_MOD_KEY) posted.push_back({{LOWORD(key), HIWORD(key)}, (LONG)generation});
  if (msg == WM_EIEM_PHYSICS_CAPTURE) ++physicsCaptureCalls;
  return TRUE;
}
static LONG_PTR FakeSetWindowLongPtrW(HWND, int, LONG_PTR) { return (LONG_PTR)MmdWndProc; }
static LONG_PTR FakeGetWindowLongPtrW(HWND, int) { return (LONG_PTR)MmdWndProc; }
static BOOL FakeIsWindow(HWND) { return TRUE; }
static HWND FakeGetForegroundWindow() { return foreground; }
static void Press(UINT vk) {
  keyDown[vk] = true;
  for (const auto &pair : osBindings) if (pair.second.vk == vk) {
    MSG msg = {}; msg.message = WM_HOTKEY; msg.wParam = pair.first;
    incoming.push_back(msg);
  }
}
static void Release(UINT vk) { keyDown[vk] = false; }
static SHORT FakeGetAsyncKeyState(int vk) { return keyDown[(UINT)vk] ? (SHORT)0x8000 : 0; }
static void FakeSleep(DWORD) {
  ++ticks;
  if (ticks == 1) { Press(VK_F6); Press(VK_F6); }
  if (ticks == 2) { Release(VK_F6); s_eiemGlobalConfig.reload.vk = VK_F8; ++s_eiemGlobalConfigGeneration; }
  if (ticks == 3) {
    bool hasNew = false, hasOld = false;
    for (const auto &pair : osBindings) { hasNew |= pair.second.vk == VK_F8; hasOld |= pair.second.vk == VK_F10; }
    rebound = hasNew && !hasOld;
    Press(VK_F8);
  }
  if (ticks == 4) { if (blockF12) keyDown[VK_CONTROL] = true; Press(VK_F12); }
  // Keep F8 held for one complete loop. The reload increments the Mod
  // generation, forcing a re-registration while the key is still down. The
  // fake OS emits another WM_HOTKEY from FakeRegisterHotKey; the persistent
  // edge latch must suppress it.
  if (ticks == 5) { Release(VK_F8); Release(VK_F12); keyDown[VK_CONTROL] = false; foreground = (HWND)99; }
  if (ticks == 6) {
    unregisteredAway = osBindings.empty(); foreground = (HWND)1;
    EiemSelectControlledMod("b/mod.ini");
  }
  if (ticks == 7) Press(VK_F7);
  if (ticks == 8) { Release(VK_F7); foreground = g_modUiHwnd; }
  if (ticks == 9) {
    bool hasUi=false, hasCycle=false;
    for (const auto &pair:osBindings) { hasUi |= pair.second.vk==VK_F11; hasCycle |= pair.second.vk==VK_F7; }
    uiOnly=hasUi && !hasCycle;
    Press(VK_F11); Press(VK_F8); Press(VK_INSERT);
  }
  if (ticks == 11) {
    bool hasGame=false, hasUiOnly=false;
    for (const auto &pair:osBindings) {
      hasGame |= pair.second.vk==VK_F7;
      hasUiOnly |= pair.second.vk==VK_F5;
    }
    managerGameKeys=hasGame && !hasUiOnly;
    Release(VK_F11); Press(VK_F7);
  }
  if (ticks >= 12) g_guiRunning = false;
}
#define RegisterHotKey FakeRegisterHotKey
#define UnregisterHotKey FakeUnregisterHotKey
#define PeekMessageW FakePeekMessageW
#define PostMessageW FakePostMessageW
#define SetWindowLongPtrW FakeSetWindowLongPtrW
#define GetWindowLongPtrW FakeGetWindowLongPtrW
#define IsWindow FakeIsWindow
#define GetForegroundWindow FakeGetForegroundWindow
#define GetAsyncKeyState FakeGetAsyncKeyState
#define Sleep FakeSleep
'''

MAIN = r'''
int main(int argc, char **argv) {
  blockF12 = argc > 1 && std::string(argv[1]) == "fallback";
  EiemModProgram program;
  std::string error;
  std::istringstream input("[Constants]\n$a=0\n[KeyA]\nkey=F6\ntype=cycle\n$a=0,1\n"
    "[UIA]\npath=ui.lua\n[UIB]\npath=other.lua\n"
    "[KeyUiA]\nkey=F9\ntype=cycle\nscope=both\n$a=0,1\n"
    "[KeyUiB]\nkey=F9\ntype=cycle\nscope=both\n$a=0,1\n");
  if (!EiemModParseStream(input, "a/mod.ini", program, &error)) return 1;
  EiemModProgram second;
  std::istringstream inputB("[Constants]\n$b=0\n"
    "[KeyGame]\nkey=F7\ntype=cycle\n$b=0,1\n"
    "[KeyUi]\nkey=F11\ntype=cycle\nscope=both\n$b=0,1\n"
    "[KeyOnlyUi]\nkey=F5\ntype=cycle\nscope=ui\n$b=0,1\n");
  if (!EiemModParseStream(inputB, "b/mod.ini", second, &error)) return 1;
  EiemAppendModDocument(program, std::move(second));
  EiemPublishModState(program); s_eiemModGeneration = 9;
  EiemSelectControlledMod("a/mod.ini");
  HotkeyThread(nullptr);
  const int expectedPhysicsCaptures =
      kEiemEnableNativePhysicsObservation ? 1 : 0;
  if (!rebound || !unregisteredAway || !norepeat || !uiOnly || !managerGameKeys || !osBindings.empty() || reloadCalls != 2 || guiCalls != 1 || physicsCaptureCalls != expectedPhysicsCaptures || posted.size() != 4 || fallbackBound != (kEiemEnableNativePhysicsObservation && blockF12)) {
    std::fprintf(stderr, "rebound=%d away=%d norepeat=%d uiOnly=%d managerGame=%d bindings=%zu reload=%d gui=%d posts=%zu\n",
      rebound, unregisteredAway, norepeat, uiOnly, managerGameKeys, osBindings.size(), reloadCalls, guiCalls, posted.size());
    return 2;
  }
  const UINT expected[] = {VK_F6, VK_F7, VK_F11, VK_F7};
  for (size_t i=0;i<posted.size();++i)
    if (posted[i].chord.vk != expected[i] ||
        posted[i].generation != (i == 0 ? 9 : 10)) return 3;
  return 0;
}
'''


class HotkeyWorkerTests(unittest.TestCase):
    def test_real_worker_rebind_focus_edges_and_order(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC developer environment")
        source_text = (ROOT / "src/eiem_mod_dispatcher.h").read_text(encoding="utf-8")
        implementation = function(source_text, "static DWORD WINAPI HotkeyThread(")
        with tempfile.TemporaryDirectory(prefix="eiem-hotkey-") as folder:
            folder = Path(folder)
            source = folder / "hotkeys.cpp"
            source.write_text(PREFIX + implementation + MAIN, encoding="utf-8")
            exe = folder / "hotkeys.exe"
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}", "user32.lib"],
                                   cwd=folder, capture_output=True, text=True,
                                   encoding="utf-8", errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            for mode in ("normal", "fallback"):
                with self.subTest(mode=mode):
                    run = subprocess.run([str(exe), mode], cwd=folder,
                                         capture_output=True, text=True, timeout=10)
                    self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__": unittest.main()
