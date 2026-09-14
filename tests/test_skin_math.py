"""The skinning math the mesh probe reports with must match Unity's convention."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include "eiem_skin_math.h"
#include <cassert>
#include <cstdio>
using namespace EiemSkinProbe;

static bool Near(float a, float b) { return a > b - 1e-4f && a < b + 1e-4f; }

int main() {
  // --- storage convention -------------------------------------------------
  assert(sizeof(Vector3) == 12);
  assert(sizeof(BoneWeight) == 32);
  assert(sizeof(Matrix) == 64);
  assert(Near(Identity().m[0], 1.0f) && Near(Identity().m[5], 1.0f));
  assert(Near(Identity().m[15], 1.0f) && Near(Identity().m[1], 0.0f));

  // --- a translation must land in the last column -------------------------
  const Matrix move = Translation(3.0f, -4.0f, 5.0f);
  assert(Near(move.m[12], 3.0f) && Near(move.m[13], -4.0f) && Near(move.m[14], 5.0f));
  Vector3 point = {1.0f, 2.0f, 3.0f};
  Vector3 moved;
  TransformPoint(move, point, &moved);
  assert(Near(moved.x, 4.0f) && Near(moved.y, -2.0f) && Near(moved.z, 8.0f));

  // --- identity preserves a point ----------------------------------------
  TransformPoint(Identity(), point, &moved);
  assert(Near(moved.x, 1.0f) && Near(moved.y, 2.0f) && Near(moved.z, 3.0f));

  // --- multiplication order: (A*B)*p == A*(B*p) ---------------------------
  const Matrix scale = [] { Matrix m = Identity(); m.m[0] = m.m[5] = m.m[10] = 2.0f; return m; }();
  const Matrix both = Multiply(move, scale);
  Vector3 viaProduct, viaSerial;
  TransformPoint(both, point, &viaProduct);
  TransformPoint(move, [] { Vector3 v = {2.0f, 4.0f, 6.0f}; return v; }(), &viaSerial);
  assert(Near(viaProduct.x, viaSerial.x) && Near(viaProduct.y, viaSerial.y) &&
         Near(viaProduct.z, viaSerial.z));
  // and it is NOT commutative for these two
  const Matrix reversed = Multiply(scale, move);
  assert(!Near(reversed.m[12], both.m[12]));

  // --- determinant catches a degenerate bindpose -------------------------
  assert(Near(Determinant3x3(Identity()), 1.0f));
  assert(Near(Determinant3x3(scale), 8.0f));
  Matrix flat = Identity();
  flat.m[10] = 0.0f;  // collapses Z
  assert(Near(Determinant3x3(flat), 0.0f));

  // --- skinning ----------------------------------------------------------
  // A vertex fully bound to a bone translated by +5 in Y is skinned to +5.
  std::vector<Matrix> skin;
  skin.push_back(SkinningMatrix(Translation(0.0f, 5.0f, 0.0f), nullptr));
  BoneWeight full;
  full.weights[0] = 1.0f;
  full.indices[0] = 0;
  Vector3 skinned = SkinVertex(point, full, skin, nullptr);
  assert(Near(skinned.y, 7.0f));
  assert(Near(skinned.x, 1.0f));

  // A vertex bound to a bone outside the palette keeps its mesh position.
  BoneWeight missing;
  missing.weights[0] = 1.0f;
  missing.indices[0] = 7;
  SkinCounters counters;
  skinned = SkinVertex(point, missing, skin, &counters);
  assert(Near(skinned.y, 2.0f));
  assert(counters.outOfRange == 1 && counters.unweighted == 1);

  // Halved weights are renormalised rather than shrinking the vertex home.
  BoneWeight half;
  half.weights[0] = 0.5f;
  half.indices[0] = 0;
  skinned = SkinVertex(point, half, skin, nullptr);
  assert(Near(skinned.y, 7.0f));

  // Two bones at half weight each blend to the midpoint.
  skin.push_back(SkinningMatrix(Translation(0.0f, -5.0f, 0.0f), nullptr));
  BoneWeight blended;
  blended.weights[0] = 0.5f;
  blended.indices[0] = 0;
  blended.weights[1] = 0.5f;
  blended.indices[1] = 1;
  skinned = SkinVertex(point, blended, skin, nullptr);
  assert(Near(skinned.y, 2.0f));

  // The real failure mode: every bone slot dead collapses the box.
  Bounds staticBox, collapsed;
  staticBox.Add(-1.0f, -1.0f, -1.0f);
  staticBox.Add(1.0f, 1.0f, 1.0f);
  collapsed.Add(0.0f, 0.0f, 0.0f);
  assert(staticBox.Longest() > 1.9f && staticBox.Longest() < 2.1f);
  assert(collapsed.Longest() < 1e-4f);

  // --- verdicts ----------------------------------------------------------
  assert(!strcmp(Verdict(false, 3, 0, 10, true, 2.0f, 2.0f, 0.0f), "unmeasured"));
  assert(!strcmp(Verdict(true, 0, 0, 10, true, 2.0f, 2.0f, 0.0f), "NO-BONES-ARRAY"));
  assert(!strcmp(Verdict(true, 3, 3, 10, true, 2.0f, 2.0f, 0.0f), "ALL-BONES-DEAD"));
  assert(!strcmp(Verdict(true, 3, 0, 0, true, 2.0f, 0.0f, 0.0f), "NO-VERTICES-SAMPLED"));
  assert(!strcmp(Verdict(true, 3, 0, 10, true, 2.0f, 0.0f, 0.0f), "SKIN-COLLAPSED"));
  assert(!strcmp(Verdict(true, 3, 0, 10, true, 2.0f, 0.1f, 0.0f), "SKIN-SHRUNK"));
  assert(!strcmp(Verdict(true, 3, 0, 10, true, 2.0f, 2.0f, 9.0f), "SKIN-DISPLACED"));
  // A correct bind stays "ok": same extent, same place.
  assert(!strcmp(Verdict(true, 3, 0, 10, true, 2.0f, 2.0f, 0.0f), "ok"));
  // A vertex standing still is not a collapse.
  assert(!strcmp(Verdict(true, 3, 0, 10, true, 0.0f, 0.0f, 0.0f), "ok"));

  printf("skin math ok\n");
  return 0;
}
'''


class SkinMathTests(unittest.TestCase):
    def test_matches_unity_convention(self):
        if not shutil.which('cl'):
            self.skipTest('Requires MSVC')
        with tempfile.TemporaryDirectory(prefix='eiem-skin-math-') as directory:
            folder = Path(directory)
            cpp = folder / 'test.cpp'
            cpp.write_text(SOURCE, encoding='utf-8')
            exe = folder / 'test.exe'
            build = subprocess.run(
                ['cl', '/nologo', '/EHsc', '/std:c++17', f'/I{ROOT / "src"}',
                 str(cpp), f'/Fe{exe}'],
                cwd=folder, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn('skin math ok', run.stdout)


if __name__ == '__main__':
    unittest.main()
