#pragma once
#include <memory>

// Managed reachability and Unity native lifetime are separate. Call Status on
// the Unity thread. Unknown is not alive, and is not proof of destruction.
static void *s_eiemNativeAliveMethod = nullptr;
static uint32_t (*s_eiemNewWeakHandle)(void *, bool) = nullptr;

static int EiemNativeObjectStatus(void *object) {
  if (!object) return 0;
  if (!EiemOnUnityThread() || !s_eiemNativeAliveMethod ||
      !il2cpp_runtime_invoke || !il2cpp_object_unbox) return -1;
  __try {
    void *exception = nullptr;
    void *params[] = {object};
    void *boxed = il2cpp_runtime_invoke(s_eiemNativeAliveMethod, nullptr, params, &exception);
    if (!boxed || exception) return -1;
    void *value = il2cpp_object_unbox(boxed);
    return value ? (*(bool *)value ? 1 : 0) : -1;
  } __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

// Copies share ONE managed handle. Instance observations use weak references;
// a write's source snapshot may use a strong one until restoration finishes.
class EiemUnityRef {
  std::shared_ptr<uint32_t> handle_;
 public:
  static EiemUnityRef Capture(void *object, bool weak = true) {
    EiemUnityRef ref;
    if (!object || !il2cpp_gchandle_free || !il2cpp_gchandle_get_target) return ref;
    uint32_t handle = weak ? (s_eiemNewWeakHandle ? s_eiemNewWeakHandle(object, false) : 0)
                           : (il2cpp_gchandle_new ? il2cpp_gchandle_new(object, false) : 0);
    if (handle) ref.handle_ = std::shared_ptr<uint32_t>(new uint32_t(handle), [](uint32_t *p) {
      il2cpp_gchandle_free(*p); delete p;
    });
    return ref;
  }
  explicit operator bool() const { return bool(handle_); }
  uint32_t Handle() const { return handle_ ? *handle_ : 0; }
  void *Target() const {
    return handle_ && il2cpp_gchandle_get_target ? il2cpp_gchandle_get_target(*handle_) : nullptr;
  }
  int Status() const { return EiemNativeObjectStatus(Target()); }
};

static void EiemInitUnityLifetime(void **assemblies, size_t count) {
  void *object = FindClass("UnityEngine", "Object", assemblies, count);
  s_eiemNativeAliveMethod = FindMethod(object, "op_Implicit", 1);
  s_eiemNewWeakHandle = hGA ? (decltype(s_eiemNewWeakHandle))GetProcAddress(hGA, "il2cpp_gchandle_new_weakref") : nullptr;
  Log("[MOD-LIFECYCLE] native validity=%p weak references=%p", s_eiemNativeAliveMethod, s_eiemNewWeakHandle);
}
