#pragma once
#include <algorithm>
#include <cmath>
#include <utility>
#include "eiem_native_physics_probe.h"
#include "eiem_native_physics_trace.h"

// Development-build instrumentation. It is deliberately absent from the
// user-facing Dump page and never constructs or mutates a Physics component.
static char g_eiemPhysicsDiagnosticDir[512] = "plugin\\physics_diagnostics";
static volatile LONG s_eiemPhysicsDiagnosticWriting = 0;
static bool s_eiemPhysicsAutoTraceAttempted = false;
static bool s_eiemPhysicsAutoTraceStarted = false;
static bool s_eiemPhysicsAutoTraceFinished = false;

// A focused runtime observation for the unresolved Typhoea breast-bone
// question.  The source Prefab owns an Animator but no controller, so clip
// bindings can only be observed after the character system assembles a live
// model.  This probe reads transforms after FinalIK LateUpdate; it does not
// create components, change poses, or participate in the Mod loader.
static const char *s_eiemChestMotionBoneNames[] = {
    "Bip001_Spine2", "breast_base_L_a_01_jnt",
    "breast_base_L_a_02_jnt", "breast_base_R_a_01_jnt",
    "breast_base_R_a_02_jnt"};

struct EiemChestMotionPose {
  Vector3 localPosition = {};
  Quaternion localRotation = {0, 0, 0, 1};
  Vector3 worldPosition = {};
  Quaternion worldRotation = {0, 0, 0, 1};
};

struct EiemChestMotionBone {
  EiemUnityRef transform;
  EiemChestMotionPose baseline = {};
  bool hasBaseline = false;
  uint32_t samples = 0;
  float maxLocalPositionDelta = 0.0f;
  float maxLocalRotationDeltaDegrees = 0.0f;
  float maxWorldPositionDelta = 0.0f;
  float maxWorldRotationDeltaDegrees = 0.0f;
};

struct EiemChestMotionTarget {
  uintptr_t modelIdentity = 0;
  EiemUnityRef model;
  char modelName[192] = {};
  char prefabPath[768] = {};
  EiemChestMotionBone bones[_countof(s_eiemChestMotionBoneNames)] = {};
};

static FILE *s_eiemChestMotionFile = nullptr;
static char s_eiemChestMotionPath[768] = {};
static bool s_eiemChestMotionStartAttempted = false;
static ULONGLONG s_eiemChestMotionStartTick = 0;
static ULONGLONG s_eiemChestMotionNextSampleTick = 0;
static ULONGLONG s_eiemChestMotionNextDiscoveryTick = 0;
static uint32_t s_eiemChestMotionSequence = 0;
static void *s_eiemChestMotionGetWorldRotation = nullptr;
static std::vector<EiemChestMotionTarget> s_eiemChestMotionTargets;

static void EiemChestMotionText(char *text) {
  if (!text) return;
  for (; *text; ++text)
    if (*text == '\t' || *text == '\r' || *text == '\n') *text = ' ';
}

static void *EiemChestMotionFindTransform(void *transform,
                                          const char *target, int depth) {
  if (!transform || !target || depth < 0 || !g_object_get_name ||
      !g_transform_get_childCount || !g_transform_GetChild)
    return nullptr;
  char name[192] = {};
  void *nameString = Invoke(g_object_get_name, transform);
  if (nameString) ReadStrUtf8(nameString, name, sizeof(name));
  if (strcmp(name, target) == 0) return transform;
  if (!depth) return nullptr;
  void *countBoxed = Invoke(g_transform_get_childCount, transform);
  const int childCount = countBoxed ? *(int *)((char *)countBoxed + 16) : -1;
  if (childCount < 0 || childCount > 2048) return nullptr;
  for (int index = 0; index < childCount; ++index) {
    int32_t childIndex = index;
    void *params[] = {&childIndex};
    void *child = Invoke(g_transform_GetChild, transform, params);
    void *found = EiemChestMotionFindTransform(child, target, depth - 1);
    if (found) return found;
  }
  return nullptr;
}

static float EiemChestMotionPositionDelta(const Vector3 &left,
                                           const Vector3 &right) {
  const float x = left.x - right.x, y = left.y - right.y,
              z = left.z - right.z;
  return sqrtf(x * x + y * y + z * z);
}

static float EiemChestMotionRotationDelta(const Quaternion &left,
                                           const Quaternion &right) {
  const float leftLength = sqrtf(left.x * left.x + left.y * left.y +
                                 left.z * left.z + left.w * left.w);
  const float rightLength = sqrtf(right.x * right.x + right.y * right.y +
                                  right.z * right.z + right.w * right.w);
  if (leftLength <= 0.0f || rightLength <= 0.0f) return 0.0f;
  float dot = fabsf((left.x * right.x + left.y * right.y +
                     left.z * right.z + left.w * right.w) /
                    (leftLength * rightLength));
  if (dot > 1.0f) dot = 1.0f;
  return 2.0f * acosf(dot) * 57.29577951308232f;
}

static bool EiemChestMotionReadPose(void *transform,
                                    EiemChestMotionPose &pose) {
  if (!transform || !g_transform_get_localPosition ||
      !g_transform_get_localRotation || !g_transform_get_position ||
      !s_eiemChestMotionGetWorldRotation)
    return false;
  void *localPosition = Invoke(g_transform_get_localPosition, transform);
  void *localRotation = Invoke(g_transform_get_localRotation, transform);
  void *worldPosition = Invoke(g_transform_get_position, transform);
  void *worldRotation = Invoke(s_eiemChestMotionGetWorldRotation, transform);
  if (!localPosition || !localRotation || !worldPosition || !worldRotation)
    return false;
  pose.localPosition = *(Vector3 *)((char *)localPosition + 16);
  pose.localRotation = *(Quaternion *)((char *)localRotation + 16);
  pose.worldPosition = *(Vector3 *)((char *)worldPosition + 16);
  pose.worldRotation = *(Quaternion *)((char *)worldRotation + 16);
  return true;
}

static bool EiemChestMotionKnownModel(uintptr_t identity) {
  for (const auto &target : s_eiemChestMotionTargets)
    if (target.modelIdentity == identity) return true;
  return false;
}

static void EiemChestMotionDiscoverTargets() {
  if (!s_eiemChestMotionFile || !g_gameObject_get_transform) return;
  std::vector<EiemModelInstanceState> models;
  AcquireSRWLockShared(&s_eiemModelInstanceLock);
  models = s_eiemModelInstances;
  ReleaseSRWLockShared(&s_eiemModelInstanceLock);
  for (const auto &state : models) {
    void *model = state.modelRef.Target();
    const uintptr_t identity = (uintptr_t)model;
    if (!model || state.modelRef.Status() != 1 ||
        EiemChestMotionKnownModel(identity))
      continue;
    void *root = Invoke(g_gameObject_get_transform, model);
    if (!root) continue;
    void *transforms[_countof(s_eiemChestMotionBoneNames)] = {};
    for (size_t index = 0; index < _countof(transforms); ++index)
      transforms[index] = EiemChestMotionFindTransform(
          root, s_eiemChestMotionBoneNames[index], 24);
    if (!transforms[0] || (!transforms[1] && !transforms[3])) continue;

    EiemChestMotionTarget target = {};
    target.modelIdentity = identity;
    target.model = state.modelRef;
    strncpy_s(target.prefabPath, sizeof(target.prefabPath), state.path,
              _TRUNCATE);
    if (g_object_get_name) {
      void *name = Invoke(g_object_get_name, model);
      if (name) ReadStrUtf8(name, target.modelName, sizeof(target.modelName));
    }
    EiemChestMotionText(target.modelName);
    EiemChestMotionText(target.prefabPath);
    for (size_t index = 0; index < _countof(transforms); ++index)
      target.bones[index].transform = EiemUnityRef::Capture(transforms[index]);
    s_eiemChestMotionTargets.push_back(std::move(target));
    const auto &added = s_eiemChestMotionTargets.back();
    fprintf(s_eiemChestMotionFile,
            "# target\t0x%p\t%s\t%s\n", model, added.modelName,
            added.prefabPath);
    fflush(s_eiemChestMotionFile);
    Log("[CHEST-MOTION] target model=%p name=%s path=%s", model,
        added.modelName, added.prefabPath);
  }
}

static void EiemStartChestMotionProbeOnUnityThread() {
  if (s_eiemChestMotionStartAttempted) return;
  s_eiemChestMotionStartAttempted = true;
  if (!EiemOnUnityThread() || !EiemEnsureDirectory(g_eiemPhysicsDiagnosticDir))
    return;
  s_eiemChestMotionGetWorldRotation =
      g_transformClass ? FindMethod(g_transformClass, "get_rotation", 0)
                       : nullptr;
  if (!s_eiemChestMotionGetWorldRotation) {
    Log("[CHEST-MOTION] Transform.get_rotation unavailable");
    return;
  }
  s_eiemChestMotionStartTick = GetTickCount64();
  snprintf(s_eiemChestMotionPath, sizeof(s_eiemChestMotionPath),
           "%s\\chest_motion_%lu_%llu.tsv", g_eiemPhysicsDiagnosticDir,
           GetCurrentProcessId(),
           (unsigned long long)s_eiemChestMotionStartTick);
  s_eiemChestMotionFile = fopen(s_eiemChestMotionPath, "wb");
  if (!s_eiemChestMotionFile) {
    Log("[CHEST-MOTION] cannot create %s", s_eiemChestMotionPath);
    return;
  }
  fputs("elapsed_ms\tsequence\tmodel\tmodel_name\tprefab_path\tbone\t"
        "local_px\tlocal_py\tlocal_pz\tlocal_rx\tlocal_ry\tlocal_rz\tlocal_rw\t"
        "world_px\tworld_py\tworld_pz\tworld_rx\tworld_ry\tworld_rz\tworld_rw\n",
        s_eiemChestMotionFile);
  fflush(s_eiemChestMotionFile);
  Log("[CHEST-MOTION] automatic post-LateUpdate trace started: %s",
      s_eiemChestMotionPath);
}

static void EiemSampleChestMotionAfterLateUpdate() {
  if (!s_eiemChestMotionFile || !EiemOnUnityThread()) return;
  const ULONGLONG now = GetTickCount64();
  if (now < s_eiemChestMotionNextSampleTick) return;
  s_eiemChestMotionNextSampleTick = now + 50;
  if (now >= s_eiemChestMotionNextDiscoveryTick) {
    s_eiemChestMotionNextDiscoveryTick = now + 500;
    EiemChestMotionDiscoverTargets();
  }
  ++s_eiemChestMotionSequence;
  for (auto &target : s_eiemChestMotionTargets) {
    if (target.model.Status() != 1) continue;
    for (size_t index = 0; index < _countof(target.bones); ++index) {
      auto &bone = target.bones[index];
      if (bone.samples >= 3600 || bone.transform.Status() != 1) continue;
      EiemChestMotionPose pose = {};
      if (!EiemChestMotionReadPose(bone.transform.Target(), pose)) continue;
      if (!bone.hasBaseline) {
        bone.baseline = pose;
        bone.hasBaseline = true;
      }
      bone.maxLocalPositionDelta =
          (std::max)(bone.maxLocalPositionDelta,
                     EiemChestMotionPositionDelta(
                         pose.localPosition, bone.baseline.localPosition));
      bone.maxLocalRotationDeltaDegrees =
          (std::max)(bone.maxLocalRotationDeltaDegrees,
                     EiemChestMotionRotationDelta(
                         pose.localRotation, bone.baseline.localRotation));
      bone.maxWorldPositionDelta =
          (std::max)(bone.maxWorldPositionDelta,
                     EiemChestMotionPositionDelta(
                         pose.worldPosition, bone.baseline.worldPosition));
      bone.maxWorldRotationDeltaDegrees =
          (std::max)(bone.maxWorldRotationDeltaDegrees,
                     EiemChestMotionRotationDelta(
                         pose.worldRotation, bone.baseline.worldRotation));
      ++bone.samples;
      fprintf(s_eiemChestMotionFile,
              "%llu\t%u\t0x%p\t%s\t%s\t%s\t"
              "%.9g\t%.9g\t%.9g\t%.9g\t%.9g\t%.9g\t%.9g\t"
              "%.9g\t%.9g\t%.9g\t%.9g\t%.9g\t%.9g\t%.9g\n",
              (unsigned long long)(now - s_eiemChestMotionStartTick),
              s_eiemChestMotionSequence, (void *)target.modelIdentity,
              target.modelName, target.prefabPath,
              s_eiemChestMotionBoneNames[index], pose.localPosition.x,
              pose.localPosition.y, pose.localPosition.z,
              pose.localRotation.x, pose.localRotation.y,
              pose.localRotation.z, pose.localRotation.w,
              pose.worldPosition.x, pose.worldPosition.y,
              pose.worldPosition.z, pose.worldRotation.x,
              pose.worldRotation.y, pose.worldRotation.z,
              pose.worldRotation.w);
    }
  }
  if ((s_eiemChestMotionSequence % 20) == 0) fflush(s_eiemChestMotionFile);
}

static void EiemFinishChestMotionProbeOnUnityThread() {
  if (!s_eiemChestMotionFile) return;
  fputs("# summary\tmodel\tbone\tsamples\tmax_local_position_delta\t"
        "max_local_rotation_delta_degrees\tmax_world_position_delta\t"
        "max_world_rotation_delta_degrees\n", s_eiemChestMotionFile);
  for (const auto &target : s_eiemChestMotionTargets) {
    for (size_t index = 0; index < _countof(target.bones); ++index) {
      const auto &bone = target.bones[index];
      if (!bone.samples) continue;
      fprintf(s_eiemChestMotionFile,
              "# summary\t0x%p\t%s\t%u\t%.9g\t%.9g\t%.9g\t%.9g\n",
              (void *)target.modelIdentity, s_eiemChestMotionBoneNames[index],
              bone.samples, bone.maxLocalPositionDelta,
              bone.maxLocalRotationDeltaDegrees, bone.maxWorldPositionDelta,
              bone.maxWorldRotationDeltaDegrees);
      Log("[CHEST-MOTION] model=%p bone=%s samples=%u localPos=%.6g localDeg=%.6g worldPos=%.6g worldDeg=%.6g",
          (void *)target.modelIdentity, s_eiemChestMotionBoneNames[index],
          bone.samples, bone.maxLocalPositionDelta,
          bone.maxLocalRotationDeltaDegrees, bone.maxWorldPositionDelta,
          bone.maxWorldRotationDeltaDegrees);
    }
  }
  fflush(s_eiemChestMotionFile);
  fclose(s_eiemChestMotionFile);
  s_eiemChestMotionFile = nullptr;
  Log("[CHEST-MOTION] wrote %s", s_eiemChestMotionPath);
}

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

static void EiemStartPhysicsAutoTraceOnUnityThread() {
  if (s_eiemPhysicsAutoTraceAttempted) return;
  s_eiemPhysicsAutoTraceAttempted=true;
  EiemStartChestMotionProbeOnUnityThread();
  std::string error;
  if (!EiemStartPhysicsTrace(error)) {
    Log("[PHYSICS-DIAGNOSTIC] automatic trace start failed: %s",error.c_str()); return;
  }
  s_eiemPhysicsAutoTraceStarted=true;
  Log("[PHYSICS-DIAGNOSTIC] automatic native physics trace started");
}

static void EiemFinishPhysicsAutoTraceOnUnityThread() {
  if (s_eiemPhysicsAutoTraceFinished) return;
  s_eiemPhysicsAutoTraceFinished=true;
  EiemFinishChestMotionProbeOnUnityThread();
  if (s_eiemPhysicsAutoTraceStarted) EiemPhysicsStopTrace();
  EiemWriteNativePhysicsDiagnostic();
}
