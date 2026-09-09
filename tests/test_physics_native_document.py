"""Native authoring exchange and actual DLL reader, with no Unity objects."""
import base64
import copy
import hashlib
import importlib.util
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("physics_source_test", ROOT / "tools/Blender/eiem_physics_source.py")
codec = importlib.util.module_from_spec(spec); spec.loader.exec_module(codec)


def fixture():
    raw = b"preserved source bytes"
    transforms = [{"identity":"CAB:101", "owner":"CAB:201", "parent":None, "bone":"", "path":"Actor",
                   "localPosition":[0,0,0], "localRotation":[0,0,0,1], "localScale":[1,1,1]},
                  {"identity":"CAB:102", "owner":"CAB:202", "parent":"CAB:101", "bone":"Collider", "path":"Actor/Collider",
                   "localPosition":[0,1,0], "localRotation":[0,0,0,1], "localScale":[1,1,1]}]
    group = {"id":"1"*32, "source":"CAB:301", "type":"BeyondBoneCloth", "name":"尾链", "owner":"CAB:201",
             "ownerPath":"Actor", "bone":"", "operation":"override",
             "fields":{"serializeData":{"gravity":1.25, "rootBones":[{"m_FileID":0,"m_PathID":101}],
                  "damping":{"value":.3,"useCurve":1,"curve":{"m_Curve":[{"time":0.0,"value":1.0,"inSlope":.5,"outSlope":.25}]}}},
                  "serializeData2":{"selectionData":{"positions":[{"x":0,"y":0,"z":0}],"attributes":[{"Value":5}]},
                                    "preBuildData":{"enabled":0,"arrayBytes":[1,2,3,255]}}},
             "schema":[{"name":"Base","type":"MonoBehaviour","level":0}],
             "references":[{"field":"$.serializeData.colliderCollisionConstraint.colliderList[0]","isNull":False,"resolved":True,"type":"MonoBehaviour","identity":"CAB:302"},
                           {"field":"$.serializeData.rootBones[0]","isNull":False,"resolved":True,"type":"Transform","identity":"CAB:101"}],
             "raw":base64.b64encode(raw).decode(), "sha256":hashlib.sha256(raw).hexdigest()}
    collider = copy.deepcopy(group)
    collider.update(id="2"*32, source="CAB:302", type="BeyondBonePlaneCollider", name="平面", owner="CAB:202",
                    ownerPath="Actor/Collider", bone="Collider", references=[],
                    fields={"center":{"x":0,"y":0,"z":-.05},"size":{"x":0,"y":0,"z":0}})
    other = copy.deepcopy(group); other.update(id="3"*32,source="CAB:303",name="第二组")
    return {"version":2,"purpose":"native-authoring","coordinate":"unity-y-up-left-handed","backend":"BeyondDynamicBone",
            "id":"0"*32,"skeleton":"skeletons/shared.skeleton","source":{"prefab":"assets/actor.prefab","fingerprint":"a"*64},
            "components":[group,collider,other],"transforms":transforms}


CPP = r'''
#include "eiem_physics_document.h"
#include <fstream>
#include <cstdio>
struct Reader {
  std::ifstream file;
  Reader(const char *path):file(path,std::ios::binary) {}
  bool Bytes(void *p,size_t n) {if(!n)return true; file.read((char *)p,n);return file.good();}
  bool End() {return file.peek()==std::char_traits<char>::eof();}
};
int main(int argc,char **argv) {
  if(argc!=3)return 2;
  Reader r(argv[1]); EiemPhysicsDocument d; d.id="unchanged"; std::string error;
  const bool ok=EiemReadPhysicsAuthor(r,d,error), expected=std::string(argv[2])=="ok";
  if(ok!=expected || (!ok && (d.id!="unchanged" || error.empty()))) {puts(error.c_str());return 1;}
  if(ok) {
    if(d.version!=2 || !d.native || d.nativeComponents.size()!=3)return 3;
    const auto *components=d.native->Get("components");
    const auto *fields=components->array[0].Get("fields");
    const auto *gravity=fields->Get("serializeData")->Get("gravity");
    if(gravity->tag!=4 || gravity->number!=1.25)return 4;
    printf("2 %zu %.2f\n",d.nativeComponents.size(),gravity->number);
  }
}
'''


class PhysicsNativeDocumentTests(unittest.TestCase):
    def test_preserve_curves_unknown_flags_prebuild_and_large_id(self):
        value = fixture(); value["components"][0]["fields"]["largeSourceId"] = -8764786506078925009
        encoded = codec.encode(value)
        self.assertEqual(codec.decode(encoded), value)
        self.assertEqual(codec.encode(codec.decode(encoded)), encoded)

    def test_selected_group_closure_shares_one_collider(self):
        value = fixture()
        only = codec.closure(value, ["CAB:301"])
        self.assertEqual([c["source"] for c in only["components"]], ["CAB:301","CAB:302"])
        both = codec.closure(value, ["CAB:301","CAB:303"])
        self.assertEqual(len(both["components"]), 3)
        self.assertEqual(value, fixture())

    def test_native_group_relations_and_transform_graph_are_explicit(self):
        value = fixture(); group = value["components"][0]
        self.assertEqual(codec.group_collider_sources(group), ["CAB:302"])
        self.assertEqual(codec.group_transform_paths(value, group), ["", "Collider"])
        group["references"].append({"field":"$.serializeData.ignoreFromRootBones[0]",
            "isNull":False,"resolved":True,"type":"Transform","identity":"CAB:102"})
        self.assertEqual(codec.group_transform_paths(value, group), [""])

    def test_native_collider_geometry_uses_source_axis_alignment_and_radii(self):
        plane = fixture()["components"][1]
        self.assertEqual(codec.collider_geometry(plane),
                         {"shape":"PLANE","center":(0.0,0.0,-.05),"normal":(0.0,1.0,0.0)})
        sphere = copy.deepcopy(plane); sphere["type"] = "BeyondBoneSphereCollider"
        sphere["fields"]["size"] = {"x":.078,"y":0,"z":0}
        self.assertEqual(codec.collider_geometry(sphere)["radius"], .078)
        capsule = copy.deepcopy(plane); capsule["type"] = "BeyondBoneCapsuleCollider"
        capsule["fields"].update(size={"x":.05,"y":.08,"z":.3}, direction=0,
            reverseDirection=0, radiusSeparation=1, alignedOnCenter=1)
        geometry = codec.collider_geometry(capsule)
        for actual, expected in zip(geometry["start"] + geometry["end"],
                                    (.1,0.0,-.05,-.07,0.0,-.05)):
            self.assertAlmostEqual(actual, expected)
        self.assertEqual((geometry["startRadius"], geometry["endRadius"]), (.05,.08))
        self.assertAlmostEqual(geometry["segmentLength"], .17)
        capsule["fields"].update(direction=2, reverseDirection=1,
                                  radiusSeparation=0, alignedOnCenter=0)
        geometry = codec.collider_geometry(capsule)
        for actual, expected in zip(geometry["start"] + geometry["end"],
                                    (0.0,0.0,-.05,0.0,0.0,.15)):
            self.assertAlmostEqual(actual, expected)
        self.assertEqual(geometry["endRadius"], .05)
        self.assertAlmostEqual(geometry["segmentLength"], .2)

        # Native StartSimulationStepJob clamps both sphere-center offsets when
        # the requested total length is shorter than the two end radii.
        capsule["fields"].update(size={"x":.2,"y":.2,"z":.1},
                                  reverseDirection=0, radiusSeparation=1,
                                  alignedOnCenter=0)
        geometry = codec.collider_geometry(capsule)
        self.assertEqual(geometry["start"], geometry["end"])
        self.assertEqual(geometry["segmentLength"], 0.0)

    def test_invalid_references_identity_and_cycles(self):
        mutations = [lambda d:d["components"].pop(1),
                     lambda d:d["components"][1].update(id="1"*32),
                     lambda d:d["components"][0]["references"][0].update(resolved=False),
                     lambda d:d["transforms"][0].update(parent="CAB:102"),
                     lambda d:d["transforms"][0].update(localScale=[1,0,1]),
                     lambda d:d.update(skeleton="../shared.skeleton"),
                     lambda d:d["components"][0].update(raw="eA==")]
        for mutation in mutations:
            value = fixture(); mutation(value)
            with self.assertRaises(ValueError): codec.encode(value)

    def test_reject_truncation_trailing_data_tags_and_depth(self):
        encoded = codec.encode(fixture())
        for bad in (encoded[:10], encoded[:-1], encoded+b"x", encoded[:12]+b"\xff"+encoded[13:]):
            with self.assertRaises(ValueError): codec.decode(bad)
        value = fixture(); nest = []
        for _ in range(66): nest = [nest]
        value["extra"] = nest
        with self.assertRaises(ValueError): codec.encode(value)

    def test_numeric_editor_excludes_reference_and_prebuild_bytes(self):
        fields = dict(codec.numeric_fields(fixture()["components"][0]["fields"]))
        self.assertNotIn(("serializeData","rootBones",0,"m_PathID"), fields)
        self.assertNotIn(("serializeData2","preBuildData","arrayBytes",0), fields)
        self.assertIn(("serializeData","damping","curve","m_Curve",0,"inSlope"), fields)

    def test_schema_float_type_preserved_for_json_integer_zero(self):
        schema = [{"name":"Base","type":"MonoBehaviour","level":0},
                  {"name":"gravity","type":"float","level":1}]
        self.assertEqual(codec.schema_types(schema)[("gravity",)], "float")
        value = {"gravity":0}; codec.set_field(value,["gravity"],.25,True)
        self.assertEqual(value,{"gravity":.25})

    def test_real_cpp_reader_preserves_native_tree_and_rejects_bad_graph(self):
        if not shutil.which("cl"): self.skipTest("Requires MSVC")
        with tempfile.TemporaryDirectory(prefix="eiem-native-reader-") as directory:
            folder=Path(directory); cpp=folder/"reader.cpp"; exe=folder/"reader.exe"
            cpp.write_text(CPP, encoding="utf-8")
            result=subprocess.run(["cl","/nologo","/EHsc","/std:c++17","/utf-8",f"/I{ROOT/'src'}",str(cpp),f"/Fe{exe}"],
                                  cwd=folder,capture_output=True,text=True,errors="replace")
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            def run(data, expected):
                file=folder/"data.physics"; file.write_bytes(data)
                result=subprocess.run([str(exe),str(file),expected],capture_output=True,text=True,errors="replace")
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                return result.stdout
            encoded=codec.encode(fixture())
            self.assertEqual(run(encoded,"ok").strip(),"2 3 1.25")
            for data in (encoded[:-1],encoded+b"x",encoded[:12]+b"\xff"+encoded[13:]): run(data,"bad")
            for mutation in (lambda d:d["components"].pop(1), lambda d:d["transforms"][0].update(parent="CAB:102")):
                value=fixture(); mutation(value)
                with patch.object(codec,"validate",lambda value:value): bad=codec.encode(value)
                run(bad,"bad")


if __name__ == "__main__": unittest.main()
