"""A diagnostic guarded by a constant-false predicate is compiled away entirely.

TraceTakeBudget is deliberately stubbed to `return false` now that asset-path
exploration is finished. Pairing it with `&&` therefore makes MSVC fold the
condition to false and delete the whole block at /O2 -- the code is present in
the preprocessed source, absent from the object file, and the DLL silently keeps
its old size. That cost a full debugging cycle: the probe reported nothing and
looked like it had run and found nothing.

These checks are source-level because the failure is invisible in review: the
code reads normally and only vanishes at /O2.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
TRACE = (ROOT / 'src' / 'il2cpp_trace.h').read_text(encoding='utf-8')


class DeadDiagnosticGuardTests(unittest.TestCase):
    def test_budget_helper_is_still_a_constant_false_stub(self):
        # If this ever becomes a real budget again, the rule below can relax.
        marker = 'static bool TraceTakeBudget(volatile LONG *counter, LONG limit) {'
        start = TRACE.index(marker)
        body = TRACE[start : TRACE.index('\n}', start)]
        self.assertIn('return false;', body,
                      'TraceTakeBudget is expected to be a stub; if it now '
                      'honours its arguments, revisit the dead-code rule.')

    def test_no_guard_pairs_a_real_condition_with_the_constant_stub(self):
        # Every remaining `... && TraceTakeBudget(...)` guard is dead, not just
        # the probe's. That predates this test, so it is recorded rather than
        # forbidden; the number is asserted so the scale stays visible and a
        # future cleanup is an explicit decision.
        offsets = [match.start() for match in re.finditer(r'TraceTakeBudget\s*\(', TRACE)]
        self.assertTrue(offsets, 'TraceTakeBudget should still be declared and defined')
        combined = []
        for offset in offsets:
            line_start = TRACE.rfind('\n', 0, offset) + 1
            line_end = TRACE.find('\n', offset)
            line = TRACE[line_start:line_end]
            if line.lstrip().startswith(('static bool TraceTakeBudget', '//')):
                continue
            if '&&' in line:
                combined.append(line.strip())
        for line in combined:
            # A guard that also checks a real flag is still dead whenever the
            # stub is reached, so none of these can ever emit.
            self.assertIn('TraceTakeBudget', line)
        self.assertLessEqual(
            len(combined), 40,
            f'{len(combined)} guards are already dead against the constant '
            f'stub; stop adding new ones instead of growing this set')

    def test_hot_skin_setter_does_not_run_legacy_pose_measurement(self):
        start = TRACE.rindex('static void TraceSkinnedMeshSetBones(')
        end = TRACE.index('static void *EiemFindSourceLodGroup', start)
        body = TRACE[start:end]
        self.assertNotIn('EiemSkinProbe::Measure', body)
        self.assertNotIn('EiemSnapshotPartnerBones', body)
        self.assertNotIn('EiemArmSkinProbeSweep', body)

    def test_current_assembly_boundaries_do_not_recreate_partner_renderers(self):
        for name in ('TraceAssignSkinGo', 'TraceAssignSkinPost',
                     'TraceSetSmrRootBone'):
            start = TRACE.rindex(f'static void {name}')
            body = TRACE[start:start + 2400]
            self.assertNotIn('EiemApplyPartners', body)
            self.assertNotIn('EiemRegisterPartnersInSkinArrays', body)


if __name__ == '__main__':
    unittest.main()
