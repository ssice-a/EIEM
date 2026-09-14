#pragma once
// Pure skinning math for the mesh probe, split out so it can be compiled and
// tested without Windows, IL2CPP or the plugin's globals.
//
// Unity's Matrix4x4 is column-major: m[column * 4 + row]. Transforms are
// applied as p' = M * p. A SkinnedMeshRenderer skins with
//
//     skin(i) = bone[i].localToWorldMatrix * mesh.bindposes[i]
//     p'      = sum over the four weight slots of w * skin(index) * p
//
// Vector3 is 12 bytes with no padding; BoneWeight is four floats followed by
// four int32 (32 bytes). Both match what the Blender exporter writes.
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace EiemSkinProbe {

struct Bounds {
  bool valid = false;
  float minX = 0, minY = 0, minZ = 0;
  float maxX = 0, maxY = 0, maxZ = 0;

  void Add(float x, float y, float z) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return;
    if (!valid) {
      minX = maxX = x; minY = maxY = y; minZ = maxZ = z;
      valid = true;
      return;
    }
    if (x < minX) minX = x; else if (x > maxX) maxX = x;
    if (y < minY) minY = y; else if (y > maxY) maxY = y;
    if (z < minZ) minZ = z; else if (z > maxZ) maxZ = z;
  }
  float SizeX() const { return valid ? maxX - minX : 0.0f; }
  float SizeY() const { return valid ? maxY - minY : 0.0f; }
  float SizeZ() const { return valid ? maxZ - minZ : 0.0f; }
  float Longest() const {
    const float longest = SizeX() > SizeY() ? SizeX() : SizeY();
    return longest > SizeZ() ? longest : SizeZ();
  }
  float CenterX() const { return valid ? (minX + maxX) * 0.5f : 0.0f; }
  float CenterY() const { return valid ? (minY + maxY) * 0.5f : 0.0f; }
  float CenterZ() const { return valid ? (minZ + maxZ) * 0.5f : 0.0f; }
};

struct Matrix {
  float m[16] = {};
};

struct Vector3 {
  float x = 0, y = 0, z = 0;
};

struct BoneWeight {
  float weights[4] = {};
  int32_t indices[4] = {};
};

static inline Matrix Identity() {
  Matrix out;
  out.m[0] = out.m[5] = out.m[10] = out.m[15] = 1.0f;
  return out;
}

static inline Matrix Translation(float x, float y, float z) {
  Matrix out = Identity();
  out.m[12] = x;
  out.m[13] = y;
  out.m[14] = z;
  return out;
}

// Unity's Matrix4x4 operator*: out[col*4+row] = sum_k a[k*4+row] * b[col*4+k].
static inline Matrix Multiply(const Matrix &a, const Matrix &b) {
  Matrix out;
  for (int column = 0; column < 4; ++column)
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k)
        sum += a.m[k * 4 + row] * b.m[column * 4 + k];
      out.m[column * 4 + row] = sum;
    }
  return out;
}

static inline void TransformPoint(const Matrix &matrix, const Vector3 &point,
                                  Vector3 *out) {
  if (!out) return;
  out->x = matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8] * point.z +
           matrix.m[12];
  out->y = matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9] * point.z +
           matrix.m[13];
  out->z = matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z +
           matrix.m[14];
}

// Determinant of the upper-left 3x3. A bindpose with a near-zero determinant
// cannot be inverted, so it destroys every vertex bound to it.
static inline float Determinant3x3(const Matrix &matrix) {
  const float a = matrix.m[0], b = matrix.m[4], c = matrix.m[8];
  const float d = matrix.m[1], e = matrix.m[5], f = matrix.m[9];
  const float g = matrix.m[2], h = matrix.m[6], i = matrix.m[10];
  return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
}

// skin(i) = world(i) * bindpose(i). When the mesh carries no bindpose for a
// slot, the world matrix is used unchanged, which is what Unity does for a
// palette that was already baked into mesh space.
static inline Matrix SkinningMatrix(const Matrix &world, const Matrix *bindpose) {
  return bindpose ? Multiply(world, *bindpose) : world;
}

struct SkinCounters {
  size_t outOfRange = 0;
  size_t unweighted = 0;
  size_t contributing = 0;
};

// Blend one vertex. A slot is skipped when its weight is zero or its bone index
// is outside the palette. A vertex whose usable weights sum to zero keeps its
// mesh-space position, which is what Unity leaves behind when nothing binds it.
//
// The result is divided by the summed weight only when the weights do not
// already sum to one. Unity normalises at import, but a mesh that lost a slot
// (a bone that no longer resolves) sums to less than one, and treating that as
// a partial translation is exactly how a section of mesh collapses.
static inline Vector3 SkinVertex(const Vector3 &position,
                                 const BoneWeight &weight,
                                 const std::vector<Matrix> &skin,
                                 SkinCounters *counters = nullptr) {
  Vector3 blended = {};
  float total = 0.0f;
  for (int slot = 0; slot < 4; ++slot) {
    if (weight.weights[slot] == 0.0f) continue;
    const int32_t index = weight.indices[slot];
    if (index < 0 || (size_t)index >= skin.size()) {
      if (counters) ++counters->outOfRange;
      continue;
    }
    Vector3 transformed;
    TransformPoint(skin[(size_t)index], position, &transformed);
    blended.x += weight.weights[slot] * transformed.x;
    blended.y += weight.weights[slot] * transformed.y;
    blended.z += weight.weights[slot] * transformed.z;
    total += weight.weights[slot];
  }
  if (total == 0.0f) {
    if (counters) ++counters->unweighted;
    return position;
  }
  if (counters) ++counters->contributing;
  if (std::fabs(total - 1.0f) > 1e-4f) {
    blended.x /= total;
    blended.y /= total;
    blended.z /= total;
  }
  return blended;
}

// A collapsed or displaced skinned box is the signature of a broken binding; a
// box equal to the static one means skinning was never applied at all.
static inline const char *Verdict(bool measured, size_t boneCount,
                                  size_t nullBones, size_t sampled,
                                  bool staticValid, float staticLongest,
                                  float skinnedLongest, float centerShift) {
  if (!measured) return "unmeasured";
  if (boneCount == 0) return "NO-BONES-ARRAY";
  if (nullBones == boneCount) return "ALL-BONES-DEAD";
  if (!sampled) return "NO-VERTICES-SAMPLED";
  if (staticValid && staticLongest > 1e-3f && skinnedLongest <= 1e-4f)
    return "SKIN-COLLAPSED";
  if (staticValid && staticLongest > 1e-3f &&
      skinnedLongest < staticLongest * 0.25f)
    return "SKIN-SHRUNK";
  if (staticValid && centerShift > staticLongest) return "SKIN-DISPLACED";
  return "ok";
}

}  // namespace EiemSkinProbe
