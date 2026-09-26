#pragma once
#include "eiem_runtime_features.h"
#include "eiem_native_physics_probe.h"
#include "eiem_native_physics_trace.h"

// Development-build instrumentation. It is deliberately absent from the
// user-facing Dump page and never constructs or mutates a Physics component.
static char g_eiemPhysicsDiagnosticDir[512] = "plugin\\physics_diagnostics";
static volatile LONG s_eiemPhysicsDiagnosticWriting = 0;
static bool s_eiemPhysicsManualCaptureActive = false;
static constexpr UINT kEiemPhysicsManualCaptureMs = 2000;

static bool EiemWriteNativePhysicsDiagnostic() {
  if (!EiemOnUnityThread()) {
    Log("[PHYSICS-DIAGNOSTIC] export requires Unity thread"); return false;
  }
  if (InterlockedCompareExchange(&s_eiemPhysicsDiagnosticWriting, 1, 0) != 0) return false;
  if (!EiemEnsureDirectory(g_eiemPhysicsDiagnosticDir)) {
    Log("[PHYSICS-DIAGNOSTIC] cannot create %s",g_eiemPhysicsDiagnosticDir);
    InterlockedExchange(&s_eiemPhysicsDiagnosticWriting, 0); return false;
  }
  char path[768] = {};
  snprintf(path, sizeof(path), "%s\\physics_runtime_%lu_%llu.json", g_eiemPhysicsDiagnosticDir,
           GetCurrentProcessId(), (unsigned long long)GetTickCount64());
  HANDLE handle = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    Log("[PHYSICS-DIAGNOSTIC] cannot create output err=%lu",GetLastError());
    InterlockedExchange(&s_eiemPhysicsDiagnosticWriting, 0); return false;
  }
  int descriptor = _open_osfhandle((intptr_t)handle, _O_WRONLY | _O_BINARY);
  FILE *file = descriptor < 0 ? nullptr : _fdopen(descriptor, "wb");
  if (!file) {
    if (descriptor < 0) CloseHandle(handle); else _close(descriptor);
    DeleteFileA(path);
    Log("[PHYSICS-DIAGNOSTIC] cannot open output stream");
    InterlockedExchange(&s_eiemPhysicsDiagnosticWriting, 0); return false;
  }
  EiemPhysicsTraceReceipt receipt;
  bool ok = EiemWriteNativePhysicsProbe(file,&receipt);
  if (fflush(file) || ferror(file)) ok = false;
  if (fclose(file)) ok = false;
  if (ok) {
    EiemPhysicsTraceExported(receipt);
    Log("[PHYSICS-DIAGNOSTIC] wrote %s",path);
  } else {
    DeleteFileA(path);
    Log("[PHYSICS-DIAGNOSTIC] export failed");
  }
  InterlockedExchange(&s_eiemPhysicsDiagnosticWriting, 0);
  return ok;
}

static void EiemFinishPhysicsManualCaptureOnUnityThread(HWND hwnd,
                                                        const char *reason) {
  if (hwnd) KillTimer(hwnd,kEiemPhysicsCaptureTimer);
  if (!s_eiemPhysicsManualCaptureActive) return;
  s_eiemPhysicsManualCaptureActive=false;
  EiemPhysicsStopTrace();
  Log("[PHYSICS-DIAGNOSTIC] manual capture stopped reason=%s",
      reason ? reason : "unknown");
  EiemWriteNativePhysicsDiagnostic();
}

static void EiemStartPhysicsManualCaptureOnUnityThread(HWND hwnd) {
  if (!kEiemEnableNativePhysicsObservation || !hwnd) return;
  if (s_eiemPhysicsManualCaptureActive || EiemPhysicsTraceActive()) {
    Log("[PHYSICS-DIAGNOSTIC] F12 ignored; capture already active");
    return;
  }
  std::string error;
  const ULONGLONG setupTick=GetTickCount64();
  if (!EiemStartPhysicsTrace(error,kEiemPhysicsManualCaptureMs)) {
    Log("[PHYSICS-DIAGNOSTIC] manual F12 capture start failed setupMs=%llu: %s",
        (unsigned long long)(GetTickCount64()-setupTick),error.c_str());
    return;
  }
  s_eiemPhysicsManualCaptureActive=true;
  if (!SetTimer(hwnd,kEiemPhysicsCaptureTimer,kEiemPhysicsManualCaptureMs,
                nullptr)) {
    Log("[PHYSICS-DIAGNOSTIC] capture timer failed err=%lu",GetLastError());
    EiemFinishPhysicsManualCaptureOnUnityThread(hwnd,"timer-failed");
    return;
  }
  Log("[PHYSICS-DIAGNOSTIC] manual F12 capture started durationMs=%u setupMs=%llu",
      (unsigned)kEiemPhysicsManualCaptureMs,
      (unsigned long long)(GetTickCount64()-setupTick));
}
