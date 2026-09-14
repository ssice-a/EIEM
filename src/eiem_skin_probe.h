#pragma once
// Skinned-mesh measurement probe.
//
// A SkinnedMeshRenderer's `localBounds` does not move when the binding breaks:
// it is authored data, not a skinning result. So "the mesh lies on the ground"
// cannot be seen from that field. What can be seen is the mesh's actual skinned
// extent, computed the way Unity skins (see eiem_skin_math.h):
//
//     skin(i) = bone[i].localToWorldMatrix * mesh.bindposes[i]
//     p'      = sum over the four weight slots of w * skin(index) * p
//
// Comparing that against the mesh's static vertex AABB separates the failure
// modes: a collapsed skinned box means vertices lost their bones, a box in the
// wrong place means the bones are alive but resolve to the wrong Transforms,
// and a box matching the static one means skinning is not being applied.
//
// This header is deliberately self-contained with respect to the trace layer:
// it is included before model_dump.h, so it must not use that file's helpers,
// and it must not use __try (MSVC rejects SEH in functions that own objects
// needing unwinding). The only dereferences of managed memory happen inside
// CopyManagedArray, which has its own handler.
#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "globals.h"
#include "eiem_skin_math.h"

namespace EiemSkinProbe {

typedef EiemSkinProbe::Matrix Matrix;
typedef EiemSkinProbe::Bounds Bounds;
typedef EiemSkinProbe::Vector3 Vector3;
typedef EiemSkinProbe::BoneWeight BoneWeight;

// Reads a managed array's raw elements. IL2CPP lays a managed array out as a
// 16-byte object header, then the bounds pointer, then the length at +24, then
// the elements at +32.
static bool CopyManagedArray(void *array, void *out, size_t elementSize,
                             size_t maximum, size_t *outCount) {
  if (outCount) *outCount = 0;
  if (!array || !out || !elementSize) return false;
  __try {
    const int32_t count = *(int32_t *)((char *)array + 24);
    if (count < 0 || (size_t)count > maximum) return false;
    if (count) memcpy(out, (char *)array + 32, (size_t)count * elementSize);
    if (outCount) *outCount = (size_t)count;
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    if (outCount) *outCount = 0;
    return false;
  }
}

static bool ReadLocalToWorld(void *transform, Matrix *out) {
  if (!transform || !out || !g_transform_get_localToWorldMatrix) return false;
  bool ok = false;
  __try {
    void *boxed = Invoke(g_transform_get_localToWorldMatrix, transform);
    if (boxed) {
      memcpy(out->m, (char *)boxed + 16, sizeof(out->m));
      ok = true;
    }
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    ok = false;
  }
  if (!ok) return false;
  for (int i = 0; i < 16; ++i)
    if (!std::isfinite(out->m[i])) return false;
  return true;
}

static void ReadObjectName(void *object, char *out, size_t outSize) {
  if (!out || !outSize) return;
  out[0] = '\0';
  if (!object || !g_object_get_name) return;
  __try {
    void *name = Invoke(g_object_get_name, object);
    if (!name) return;
    // A managed string keeps its length at +16 and its UTF-8 bytes at +20.
    const int32_t length = *(int32_t *)((char *)name + 16);
    if (length <= 0 || (size_t)length >= outSize) return;
    memcpy(out, (char *)name + 20, (size_t)length);
    out[length] = '\0';
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    out[0] = '\0';
  }
}

// Skinning every vertex of a 15k mesh on the Unity thread would stall a frame.
// A strided sample separates a collapsed box from a correct one; the stride is
// reported so the sample is never mistaken for a full measurement.
static const size_t kSampleLimit = 2000;
static const size_t kMaxVertices = 2000000;
static const size_t kMaxBones = 16384;

struct Result {
  bool measured = false;
  char reason[128] = {};
  void *mesh = nullptr;
  char rendererName[160] = {};
  size_t boneCount = 0;
  size_t nullBones = 0;
  size_t boneIndexOutOfRange = 0;
  size_t degenerateBindposes = 0;
  size_t vertices = 0;
  size_t sampled = 0;
  size_t unweighted = 0;
  Bounds vertexBounds;
  Bounds skinnedBounds;
  float staticLongest = 0.0f;
  float skinnedLongest = 0.0f;
  float centerShift = 0.0f;
  uint64_t bindposeHash = 0;
  const char *verdict = "unmeasured";
};

static bool ReadArray(void *array, void *out, size_t elementSize,
                      size_t maximum, size_t *outCount) {
  return CopyManagedArray(array, out, elementSize, maximum, outCount);
}

// Observation only: never allocates managed memory and never writes to the
// renderer.
static Result Measure(void *renderer) {
  Result result;
  if (!renderer || !g_smr_get_sharedMesh || !g_smr_get_bones) {
    snprintf(result.reason, sizeof(result.reason), "renderer APIs unavailable");
    return result;
  }
  ReadObjectName(renderer, result.rendererName, sizeof(result.rendererName));

  // Read sharedMesh through the raw method pointer: this header is included
  // before the trace layer's own wrapper is declared.
  void *mesh = Invoke(g_smr_get_sharedMesh, renderer);
  result.mesh = mesh;
  if (!mesh) {
    snprintf(result.reason, sizeof(result.reason), "no sharedMesh");
    return result;
  }
  if (!g_mesh_get_vertices || !g_mesh_get_boneWeights || !g_mesh_get_bindposes) {
    snprintf(result.reason, sizeof(result.reason), "mesh getters unavailable");
    return result;
  }

  std::vector<Vector3> vertices;
  {
    size_t count = 0;
    vertices.resize(kMaxVertices);
    if (!ReadArray(Invoke(g_mesh_get_vertices, mesh), vertices.data(),
                   sizeof(Vector3), kMaxVertices, &count)) {
      snprintf(result.reason, sizeof(result.reason), "vertices unreadable");
      return result;
    }
    vertices.resize(count);
  }
  result.vertices = vertices.size();
  if (vertices.empty()) {
    snprintf(result.reason, sizeof(result.reason), "mesh has no vertices");
    return result;
  }
  for (const Vector3 &vertex : vertices) result.vertexBounds.Add(vertex.x, vertex.y, vertex.z);
  result.staticLongest = result.vertexBounds.Longest();

  std::vector<Matrix> bindposes;
  {
    size_t count = 0;
    bindposes.resize(4096);
    if (ReadArray(Invoke(g_mesh_get_bindposes, mesh), bindposes.data(),
                  sizeof(Matrix), 4096, &count))
      bindposes.resize(count);
    else
      bindposes.clear();
  }
  for (const Matrix &pose : bindposes)
    if (std::fabs(EiemSkinProbe::Determinant3x3(pose)) < 1e-12f)
      ++result.degenerateBindposes;
  {
    // A cheap fingerprint, so two measurements can be told apart without
    // dumping 64 floats per bone into the log.
    uint64_t hash = 1469598103934665603ULL;
    for (const Matrix &pose : bindposes) {
      const unsigned char *bytes = (const unsigned char *)pose.m;
      for (size_t byte = 0; byte < sizeof(pose.m); ++byte) {
        hash ^= bytes[byte];
        hash *= 1099511628211ULL;
      }
    }
    result.bindposeHash = hash;
  }

  std::vector<void *> bones;
  {
    size_t count = 0;
    bones.resize(kMaxBones);
    if (ReadArray(Invoke(g_smr_get_bones, renderer), bones.data(),
                  sizeof(void *), kMaxBones, &count))
      bones.resize(count);
    else
      bones.clear();
  }
  result.boneCount = bones.size();
  for (void *bone : bones)
    if (!bone) ++result.nullBones;

  std::vector<BoneWeight> weights;
  {
    size_t count = 0;
    weights.resize(kMaxVertices);
    if (ReadArray(Invoke(g_mesh_get_boneWeights, mesh), weights.data(),
                  sizeof(BoneWeight), kMaxVertices, &count))
      weights.resize(count);
    else
      weights.clear();
  }

  std::vector<Matrix> skin(bones.size());
  for (size_t index = 0; index < bones.size(); ++index) {
    Matrix world;
    if (!ReadLocalToWorld(bones[index], &world)) continue;
    skin[index] = EiemSkinProbe::SkinningMatrix(
        world, index < bindposes.size() ? &bindposes[index] : nullptr);
  }

  const size_t stride =
      vertices.size() > kSampleLimit ? vertices.size() / kSampleLimit : 1;
  EiemSkinProbe::SkinCounters counters;
  for (size_t vertex = 0; vertex < vertices.size(); vertex += stride) {
    const Vector3 &position = vertices[vertex];
    BoneWeight weight;
    if (vertex < weights.size()) weight = weights[vertex];
    const Vector3 skinned =
        EiemSkinProbe::SkinVertex(position, weight, skin, &counters);
    result.skinnedBounds.Add(skinned.x, skinned.y, skinned.z);
    ++result.sampled;
  }
  result.boneIndexOutOfRange = counters.outOfRange;
  result.unweighted = counters.unweighted;
  result.skinnedLongest = result.skinnedBounds.Longest();
  if (result.vertexBounds.valid && result.skinnedBounds.valid) {
    const float dx = result.skinnedBounds.CenterX() - result.vertexBounds.CenterX();
    const float dy = result.skinnedBounds.CenterY() - result.vertexBounds.CenterY();
    const float dz = result.skinnedBounds.CenterZ() - result.vertexBounds.CenterZ();
    result.centerShift = std::sqrt(dx * dx + dy * dy + dz * dz);
  }
  result.measured = result.skinnedBounds.valid;
  result.verdict = EiemSkinProbe::Verdict(
      result.measured, result.boneCount, result.nullBones, result.sampled,
      result.vertexBounds.valid, result.staticLongest, result.skinnedLongest,
      result.centerShift);
  return result;
}

static void LogResult(const char *tag, const Result &result) {
  if (!result.measured) {
    Log("%s measured=0 name=%s reason=%s", tag,
        result.rendererName[0] ? result.rendererName : "<unnamed>",
        result.reason[0] ? result.reason : "<none>");
    return;
  }
  Log("%s measured=1 name=%s mesh=%p bones=%zu nullBones=%zu oob=%zu "
      "degenPose=%zu verts=%zu sampled=%zu stride=%zu unweighted=%zu "
      "static=[%.4f %.4f %.4f] staticMax=%.4f "
      "skinned=[%.4f %.4f %.4f] skinnedMax=%.4f centerShift=%.4f "
      "poseHash=%016llX verdict=%s",
      tag, result.rendererName[0] ? result.rendererName : "<unnamed>",
      result.mesh, result.boneCount, result.nullBones,
      result.boneIndexOutOfRange, result.degenerateBindposes, result.vertices,
      result.sampled, result.sampled ? (result.vertices / result.sampled) : 1,
      result.unweighted, result.vertexBounds.SizeX(), result.vertexBounds.SizeY(),
      result.vertexBounds.SizeZ(), result.staticLongest,
      result.skinnedBounds.SizeX(), result.skinnedBounds.SizeY(),
      result.skinnedBounds.SizeZ(), result.skinnedLongest, result.centerShift,
      (unsigned long long)result.bindposeHash, result.verdict);
}

}  // namespace EiemSkinProbe
