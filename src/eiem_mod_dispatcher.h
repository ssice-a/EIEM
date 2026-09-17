#pragma once

// Static resource replacement needs a Unity-thread dispatcher for Mod keys and
// F10, but it must not reactivate the historical animation/physics window
// procedure. This procedure owns only the Mod transaction messages.
static LRESULT CALLBACK EiemModWndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam) {
  if (!s_eiemUnityThreadId) s_eiemUnityThreadId = GetCurrentThreadId();
  WNDPROC original = g_origWndProc;
  if (msg == WM_CLOSE || msg == WM_DESTROY || msg == WM_NCDESTROY ||
      msg == WM_ENDSESSION) {
    g_shutdownRequested = true;
    g_guiRunning = false;
    KillTimer(hwnd, kEiemShapeTransitionTimer);
    KillTimer(hwnd, kEiemModRetryTimer);
    KillTimer(hwnd, kEiemModReplayTimer);
    s_eiemShapeTransitionTick = 0;
    if (original &&
        GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)EiemModWndProc) {
      SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)original);
      g_origWndProc = nullptr;
    }
    const LRESULT result =
        original ? CallWindowProcW(original, hwnd, msg, wParam, lParam)
                 : DefWindowProcW(hwnd, msg, wParam, lParam);
    InterlockedExchange(&g_shutdownWndProcDone, 1);
    return result;
  }
  if (msg == WM_EIEM_MOD_RECONCILE) {
    InterlockedExchange(&s_eiemModUpdateMessagePosted, 0);
    EiemRunModReconcile();
    return 0;
  }
  if (msg == WM_TIMER && wParam == kEiemModRetryTimer) {
    KillTimer(hwnd, kEiemModRetryTimer);
    EiemRunModReconcile();
    return 0;
  }
  if (msg == WM_TIMER && wParam == kEiemModReplayTimer) {
    KillTimer(hwnd, kEiemModReplayTimer);
    EiemRunModReconcile();
    return 0;
  }
  if (msg == WM_TIMER && wParam == kEiemShapeTransitionTimer) {
    EiemRunShapeTransitions();
    return 0;
  }
  if (msg == WM_EIEM_MOD_KEY) {
    EiemQueueModKey({LOWORD(wParam), HIWORD(wParam)}, (LONG)lParam);
    return 0;
  }
  if (msg == WM_EIEM_MOD_HOLD) {
    EiemQueueModKey({LOWORD(wParam), HIWORD(wParam)}, (LONG)lParam,
                    true, 0.02);
    return 0;
  }
  return original ? CallWindowProcW(original, hwnd, msg, wParam, lParam)
                  : DefWindowProcW(hwnd, msg, wParam, lParam);
}

static DWORD WINAPI HotkeyThread(LPVOID) {
  Log("[OK] Hotkey thread started");

  void *domain = il2cpp_domain_get();
  if (domain)
    il2cpp_thread_attach(domain);

  HWND hwnd = nullptr;
  while (!hwnd && !g_shutdownRequested) {
    hwnd = FindGameWindow();
    if (!hwnd) Sleep(200);
  }
  if (g_shutdownRequested) return 0;
  g_gameHwnd = hwnd;
  Log("[OK] Game window found: %p (pid=%lu)", hwnd, GetCurrentProcessId());

  const WNDPROC dispatcher =
      kEiemEnableLegacyWorkers ? MmdWndProc : EiemModWndProc;
  g_origWndProc =
      (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)dispatcher);
  if (g_origWndProc) {
    Log("[OK] Game window subclassed for %s execution",
        kEiemEnableLegacyWorkers ? "legacy/main-thread" : "Mod transaction");
  } else {
    Log("[WARN] Failed to subclass game window (err=%lu)", GetLastError());
  }
  EiemPostPendingModUpdate("game window attached");

  // Cycle keys use OS edges. Hold keys use the same registration only as a
  // capture gate, then are sampled below so their speed is independent of the
  // keyboard's repeat delay/rate.
  enum class KeyAction { Manager, Reload, Cycle };
  struct Binding {
    int id; EiemKeyChord chord; EiemModKeyBehavior behavior;
    KeyAction action; LONG generation; ULONGLONG holdTick = 0;
  };
  std::vector<Binding> registered;
  LONG boundModGeneration = -1, boundControlGeneration = -1;
  LONG boundConfigGeneration = -1;
  HWND boundForeground = nullptr;
  MSG message = {};
  PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
  auto clearBindings = [&] {
    for (const auto &binding : registered) UnregisterHotKey(nullptr, binding.id);
    registered.clear();
  };
  while (g_guiRunning && !g_shutdownRequested && IsWindowAlive(hwnd)) {
    s_eiemPersistentStates.Flush(); // batched writes only; no disk polling or Unity calls
    HWND foreground = GetForegroundWindow();
    LONG modGeneration = InterlockedCompareExchange(&s_eiemModGeneration, 0, 0);
    LONG controlGeneration =
        InterlockedCompareExchange(&s_eiemModControlGeneration, 0, 0);
    LONG configGeneration = InterlockedCompareExchange(&s_eiemGlobalConfigGeneration, 0, 0);
    if (foreground != boundForeground || modGeneration != boundModGeneration ||
        controlGeneration != boundControlGeneration ||
        configGeneration != boundConfigGeneration) {
      clearBindings();
      // Discard messages from the old registration before reusing OS IDs.
      while (PeekMessageW(&message, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {}
      EiemGlobalConfig config = EiemGetGlobalConfig();
      std::string selectedModPath;
      std::vector<EiemModHotkeyBinding> modKeys = EiemGetModKeyBindings(
          &modGeneration, &controlGeneration, &selectedModPath,
          foreground != hwnd);
      int nextId = 1;
      auto bind = [&](EiemKeyChord chord, EiemModKeyBehavior behavior,
                      KeyAction action) {
        int id = nextId++;
        if (id > 0xBFFF || !RegisterHotKey(nullptr, id, chord.modifiers | MOD_NOREPEAT, chord.vk)) {
          Log("[HOTKEY] registration failed vk=%u modifiers=%u err=%lu; change key in INI",
              chord.vk, chord.modifiers, GetLastError());
          return;
        }
        registered.push_back({id, chord, behavior, action, modGeneration});
      };
      const bool ownedForeground = foreground == hwnd || (g_guiHwnd && foreground == g_guiHwnd) ||
                                  (g_modUiHwnd && foreground == g_modUiHwnd);
      if (ownedForeground) bind(config.gui, EiemModKeyBehavior::Cycle,
                                KeyAction::Manager);
      if (ownedForeground) {
        bind(config.reload, EiemModKeyBehavior::Cycle, KeyAction::Reload);
        for (const auto &binding : modKeys) {
          if (binding.chord == config.reload || binding.chord == config.gui) {
            Log("[HOTKEY] mod key conflicts with global shortcut vk=%u modifiers=%u; mod shortcut disabled",
                binding.chord.vk, binding.chord.modifiers);
            continue;
          }
          bind(binding.chord, binding.behavior, KeyAction::Cycle);
        }
      }
      boundForeground = foreground;
      boundModGeneration = modGeneration;
      boundControlGeneration = controlGeneration;
      boundConfigGeneration = configGeneration;
      Log("[HOTKEY] bindings updated count=%zu modGeneration=%ld configGeneration=%ld",
          registered.size(), modGeneration, configGeneration);
    }
    while (PeekMessageW(&message, nullptr, WM_HOTKEY, WM_HOTKEY, PM_REMOVE)) {
      if (!g_pluginActive) continue;
      for (const auto &binding : registered) {
        if (binding.id != (int)message.wParam) continue;
        HWND currentForeground = GetForegroundWindow();
        if (binding.action == KeyAction::Manager) {
          if (currentForeground == hwnd || currentForeground == g_guiHwnd || currentForeground == g_modUiHwnd)
            EiemToggleModManager();
        } else if (currentForeground == hwnd || currentForeground == g_guiHwnd || currentForeground == g_modUiHwnd) {
          if (binding.action == KeyAction::Reload)
            EiemRequestModUpdate(EiemModUpdate::Reload, "global reload hotkey");
          else if (!PostMessageW(hwnd, WM_EIEM_MOD_KEY,
                                 MAKEWPARAM(binding.chord.vk, binding.chord.modifiers),
                                 binding.generation))
            Log("[HOTKEY] could not queue mod key err=%lu", GetLastError());
        }
        break;
      }
    }
    if (foreground == hwnd || (g_guiHwnd && foreground == g_guiHwnd) ||
        (g_modUiHwnd && foreground == g_modUiHwnd)) {
      const ULONGLONG now = GetTickCount64();
      for (auto &binding : registered) {
        if (binding.action != KeyAction::Cycle ||
            binding.behavior != EiemModKeyBehavior::Hold)
          continue;
        const bool down = (GetAsyncKeyState((int)binding.chord.vk) & 0x8000) != 0 &&
          (!(binding.chord.modifiers & MOD_CONTROL) ||
           (GetAsyncKeyState(VK_CONTROL) & 0x8000)) &&
          (!(binding.chord.modifiers & MOD_SHIFT) ||
           (GetAsyncKeyState(VK_SHIFT) & 0x8000)) &&
          (!(binding.chord.modifiers & MOD_ALT) ||
           (GetAsyncKeyState(VK_MENU) & 0x8000));
        if (!down) {
          binding.holdTick = 0;
          continue;
        }
        if (!binding.holdTick) binding.holdTick = now;
        const ULONGLONG elapsed = now - binding.holdTick;
        if (elapsed < 16) continue;
        if (!PostMessageW(hwnd, WM_EIEM_MOD_HOLD,
                          MAKEWPARAM(binding.chord.vk, binding.chord.modifiers),
                          binding.generation))
          Log("[HOTKEY] could not queue hold key err=%lu", GetLastError());
        binding.holdTick = now;
      }
    }
    Sleep(20);
  }
  clearBindings();

  // The game owns this window. Restore its original procedure before the
  // plugin thread exits so late close/destroy messages bypass EIEM.
  if (g_origWndProc && IsWindow(hwnd) &&
      GetWindowLongPtrW(hwnd, GWLP_WNDPROC) == (LONG_PTR)dispatcher) {
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)g_origWndProc);
    g_origWndProc = nullptr;
  }
  s_eiemPersistentStates.Flush(true);
  g_guiRunning = false;
  Log("[INFO] Game window closed, hotkey thread exiting");
  return 0;
}
