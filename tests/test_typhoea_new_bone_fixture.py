"""Offline regression tests for the v70 visible new-bone diagnostic package."""

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MODULE = ROOT / "tools" / "diagnostics" / "make_v70_typhoea_new_bone_fixture.py"
SPEC = importlib.util.spec_from_file_location("make_v70_fixture", MODULE)
fixture = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(fixture)


def matrix(tx=0.0):
    return (1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            tx, 0.0, 0.0, 1.0)


class TyphoeaNewBoneFixtureTests(unittest.TestCase):
    def mesh(self):
        root = "Root/Rig"
        paths = [root, root + fixture.FINGER0_SUFFIX,
                 root + fixture.FINGER01_SUFFIX, root + fixture.FINGER02_SUFFIX]
        return {
            "version": 3, "coordinate": fixture.COORDINATE,
            "source": "body.asset", "name": "Body", "vertex_count": 2,
            "arrays": [[0.0] * 6, [0.0] * 6, [0.0] * 8, [1.0] * 8],
            "uvs": [[0.0] * 4] + [[] for _ in range(7)],
            "indices": [0, 1, 0], "submeshes": [(0, 0, 3, 0, 0, 2)],
            "skin": [([1.0, 0.0, 0.0, 0.0], [3, 0, 0, 0]),
                     ([0.5, 0.5, 0.0, 0.0], [2, 3, 0, 0])],
            "bindposes": [matrix(), matrix(), matrix(), matrix(2.0)],
            "bone_hashes": [1, 2, 3, 4], "bone_paths": paths,
            "blend_vertices": [], "blend_frames": [], "blend_channels": [],
            "blend_weights": [], "additional": [],
        }

    def test_mesh_roundtrip_and_new_bindpose_preserve_rest_pose(self):
        source = self.mesh()
        target, details = fixture.modify_mesh(source)
        self.assertEqual(details["oldSlot"], 3)
        self.assertEqual(details["newSlot"], 4)
        self.assertEqual(details["remappedVertices"], 2)
        self.assertEqual(details["remappedInfluences"], 2)
        self.assertEqual(target["skin"][0][1][0], 4)
        self.assertEqual(target["skin"][1][1][1], 4)
        self.assertEqual(source["skin"][0][1][0], 3)
        expected = fixture.multiply(fixture.inverse_translation(fixture.NEW_LOCAL_POSITION),
                                    fixture.stored_to_matrix(source["bindposes"][3]))
        self.assertEqual(fixture.stored_to_matrix(target["bindposes"][4]), expected)
        with tempfile.TemporaryDirectory(prefix="eiem-v70-fixture-") as folder:
            path = Path(folder) / "body.mesh"
            fixture.write_mesh(path, target)
            decoded = fixture.read_mesh(path)
        self.assertEqual(decoded["bone_paths"], target["bone_paths"])
        self.assertEqual(decoded["skin"], target["skin"])
        self.assertTrue(fixture.unchanged_mesh_fields(source, decoded))

    def test_skeleton_and_physics_share_the_new_author_path(self):
        target, details = fixture.modify_mesh(self.mesh())
        nodes = fixture.hierarchy(self.mesh()["bone_paths"], details["newPath"])
        self.assertEqual(sum(not node["source"] for node in nodes), 1)
        self.assertEqual(nodes[-1]["path"], details["newPath"])
        self.assertEqual(nodes[nodes[-1]["parent"]]["path"], details["finger02"])
        document = fixture.physics_document.decode(fixture.physics_bytes(details))
        self.assertEqual(document["skeleton"], fixture.PHYSICS_SKELETON_RELATIVE)
        self.assertEqual([node["role"] for node in document["groups"][0]["nodes"]],
                         ["FIXED", "MOVE", "MOVE", "MOVE"])
        self.assertEqual(document["groups"][0]["nodes"][-1]["bone"], details["newPath"])
        self.assertIn(details["newPath"], target["bone_paths"])

    def test_ini_connects_one_skeleton_and_physics_to_the_same_render(self):
        source = ("[PhysicsTyphoeaLeftIndexV67]\npath=" + fixture.OLD_PHYSICS_RELATIVE +
                  "\n\n[RenderBody]\nasset=Body\nmesh=MeshBody\n"
                  "physics=PhysicsTyphoeaLeftIndexV67\nmaterial.0=MaterialBody\n")
        result = fixture.modify_ini(source)
        self.assertIn("[SkeletonTyphoeaNewBoneV70]", result)
        self.assertIn("skeleton=SkeletonTyphoeaNewBoneV70\nphysics=PhysicsTyphoeaNewBoneV70", result)
        self.assertNotIn("V67", result)


if __name__ == "__main__":
    unittest.main()
