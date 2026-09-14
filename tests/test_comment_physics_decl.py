"""A declared Physics resource is loaded unconditionally, so it must be commentable."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "comment_physics_decl", ROOT / "tools/comment_physics_decl.py")
tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tool)


def lines(*text: str) -> list[str]:
    return [f"{line}\n" for line in text]


class CommentPhysicsDeclaration(unittest.TestCase):
    def test_comments_header_and_path_together(self):
        # The header must go too: the runtime keys off `[section]` lines, so a
        # surviving `path=` would be re-attached to the previous section.
        source = lines(
            "[SkeletonRig]", "path=rig.skeleton", "source=", "",
            "[PhysicsRig]", "path=rig.physics", "",
            "[RenderBody]", "asset=Body", "mesh=MeshBody")
        result, found, edits = tool.EiemCommentPhysics(source)
        self.assertEqual(found, ["PhysicsRig"])
        self.assertEqual(edits, [5, 6])
        self.assertEqual(result[4], "; PHYSICS-OFF [PhysicsRig]\n")
        self.assertEqual(result[5], "; PHYSICS-OFF path=rig.physics\n")
        # Neighbours are untouched, including the skeleton resource next to it.
        self.assertEqual(result[0:3], source[0:3])
        self.assertEqual(result[7:], source[7:])

    def test_input_is_not_mutated(self):
        source = lines("[PhysicsRig]", "path=rig.physics")
        before = list(source)
        tool.EiemCommentPhysics(source)
        self.assertEqual(source, before)

    def test_is_idempotent(self):
        # Running twice must be a no-op, not "no Physics declaration found":
        # the tool has to recognise its own tag.
        source = lines("[PhysicsRig]", "path=rig.physics", "")
        once, _f, _e = tool.EiemCommentPhysics(source)
        twice, found, edits = tool.EiemCommentPhysics(once)
        self.assertEqual(found, ["PhysicsRig"])
        self.assertEqual(edits, [])
        self.assertEqual(twice, once)

    def test_detection_ignores_rule_references(self):
        # `physics=` on a rule is a reference, not a declaration; commenting the
        # declarations must not depend on whether any rule still names one.
        source = lines(
            "[RenderCloth]", "mesh=MeshCloth",
            "; PHYSICS-TEST physics=PhysicsRig", "",
            "[PhysicsRig]", "path=rig.physics")
        _result, found, _edits = tool.EiemCommentPhysics(source)
        self.assertEqual(found, ["PhysicsRig"])

    def test_skeleton_resource_ending_in_skeleton_is_left_alone(self):
        # A sibling `.skeleton` file shares the directory but is not Physics.
        source = lines("[PhysicsRig]", "path=rig.physics", "",
                       "[SkeletonRig]", "path=rig.skeleton", "source=")
        result, found, _edits = tool.EiemCommentPhysics(source)
        self.assertEqual(found, ["PhysicsRig"])
        self.assertEqual(result[3], "[SkeletonRig]\n")
        self.assertEqual(result[4], "path=rig.skeleton\n")

    def test_inline_metadata_stays_inert_but_is_commented(self):
        source = lines("[PhysicsRig]", "path=rig.physics", "; note", "source=")
        result, _found, edits = tool.EiemCommentPhysics(source)
        self.assertEqual(edits, [1, 2, 4])
        self.assertEqual(result[2], "; note\n")

    def test_missing_declaration_reports_nothing(self):
        source = lines("[SkeletonRig]", "path=rig.skeleton", "source=")
        _result, found, edits = tool.EiemCommentPhysics(source)
        self.assertEqual(found, [])
        self.assertEqual(edits, [])


if __name__ == "__main__":
    unittest.main()
