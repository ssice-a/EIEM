"""Exercise production managed-list diagnostics; never a game registration test."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest

from test_native_physics_probe import SOURCE as PROBE_HOST

ROOT = Path(__file__).resolve().parents[1]
# Reuse the neutral IL2CPP reflection host, not its entry point or assertions.
SOURCE = PROBE_HOST.split('\nint main(', 1)[0] + r'''
struct Bone { int32_t id; };
struct ManagedList {
  bool integers;
  std::vector<void *> objects;
  std::vector<int32_t> values;
  int countReads=0;
  int32_t Size() const { return int32_t(integers?values.size():objects.size()); }
};
struct BoneSetup {
  // Managed fields are distinct objects, not embedded at the owner's address.
  uintptr_t hostTag=0;
  ManagedList transforms{false},ids{true},parents{true},roots{true};
  int32_t skinBoneCount=2,renderTransformIndex=2;
};
static Klass setupClass{"RenderSetupData","BeyondDynamicBone",
  {{"GetTransformIndexFromId","System.Int32",0,{"System.Int32"}}},
  {{"transformList","System.Collections.Generic.List<UnityEngine.Transform>"},
   {"transformIdList","System.Collections.Generic.List<System.Int32>"},
   {"transformParentIdList","System.Collections.Generic.List<System.Int32>"},
   {"rootTransformIdList","System.Collections.Generic.List<System.Int32>"},
   {"skinBoneCount","System.Int32"},{"renderTransformIndex","System.Int32"}}};
static Klass transformListClass{"List<Transform>","System.Collections.Generic",
  {{"get_Count","System.Int32",0,{}},{"get_Item","UnityEngine.Transform",0,{"System.Int32"}}},{}};
static Klass intListClass{"List<Int32>","System.Collections.Generic",
  {{"get_Count","System.Int32",0,{}},{"get_Item","System.Int32",0,{"System.Int32"}}},{}};
static Klass transformClass{"Transform","UnityEngine",{},{}},animatorClass{"Animator","UnityEngine",{},{}};
static Klass handleClass{"AnimationTransformRWBufferHandle","UnityEngine",{},
  {{"count","System.UInt16"},{"invalidCount","System.UInt16"},
   {"validTransformIndexsPtr","System.IntPtr"},{"invalidTransformIndexsPtr","System.IntPtr"}}};
static Image animationImage{"UnityEngine.AnimationModule.dll",{&animatorClass,&handleClass}};
static void *extendedAssemblies[]={&nativeImage,&unityImage,&animationImage};
static Bone firstBones[]={{22},{11},{99}},secondBones[]={{222},{111},{999}};
static Bone firstAnimator{501},secondAnimator{502};
static BoneSetup firstSetup,secondSetup;
static std::map<void *,Klass *> types;
static std::map<void *,int32_t *> nativeIds;
static std::map<void *,ManagedList *> lists;
static std::string mode;
static int setupReads=0,listReads=0,setupCalls=0;
static void ReadBoneField(void *o,void *f,void *out) {
  const std::string name=((Field *)f)->name;
  if(name=="boneClothSetupData") {
    ++setupReads;
    *(void **)out=o==&p0?(void *)&firstSetup:(void *)&secondSetup;
    if(o==&p0 && mode=="no-setup") *(void **)out=nullptr;
    if(o==&p0 && mode=="changed-setup" && setupReads>1) *(void **)out=&secondSetup;
    return;
  }
  if(name=="interlockingAnimator") {
    *(void **)out=o==&p0?(void *)&firstAnimator:(void *)&secondAnimator; return;
  }
  if(o==&firstSetup||o==&secondSetup) {
    auto &s=*(BoneSetup *)o;
    if(name=="skinBoneCount") *(int32_t *)out=s.skinBoneCount;
    else if(name=="renderTransformIndex") *(int32_t *)out=s.renderTransformIndex;
    else if(name=="transformList") {
      ++listReads;
      *(void **)out=o==&firstSetup && mode=="changed-list" && listReads>1?
        (void *)&secondSetup.transforms:(void *)&s.transforms;
    }
    else if(name=="transformIdList") *(void **)out=&s.ids;
    else if(name=="transformParentIdList") *(void **)out=&s.parents;
    else if(name=="rootTransformIdList") *(void **)out=&s.roots;
    else abort();
    return;
  }
  ReadField(o,f,out);
}
static void *CallBone(void *m,void *o,void **args,void **exc) {
  *exc=nullptr;
  if(m==(void *)1) {
    if(mode=="dead-node" && args[0]==&firstBones[1]) {
      static bool dead=false; ++invoked; return &dead;
    }
    return Call(m,o,args,exc);
  }
  const std::string name=((Method *)m)->name;
  if(name=="GetInstanceID" && nativeIds.count(o)) {
    ++invoked; return nativeIds.at(o);
  }
  if(name=="GetTransformIndexFromId") {
    ++invoked; ++setupCalls;
    static int32_t index; index=-1;
    const auto &ids=((BoneSetup *)o)->ids.values;
    for(size_t i=0;i<ids.size();++i) if(ids[i]==*(int32_t *)args[0]) {index=int32_t(i);break;}
    return &index;
  }
  auto found=lists.find(o);
  if(found!=lists.end()) {
    ++invoked; ++setupCalls;
    auto &list=*found->second;
    if(name=="get_Count") {
      static int32_t count; count=list.Size(); ++list.countReads;
      if(o==&firstSetup.transforms && mode=="changed-count" && list.countReads>1) ++count;
      return &count;
    }
    if(name=="get_Item") {
      int32_t i=*(int32_t *)args[0];
      if(i<0||i>=list.Size() || (mode=="item-exception" && o==&firstSetup.ids && i==1)) {
        *exc=(void *)1; return nullptr;
      }
      return list.integers?(void *)&list.values[i]:list.objects[i];
    }
  }
  return Call(m,o,args,exc);
}
static void SetupBoneHost() {
  Setup();
  nativeImage.classes.push_back(&setupClass);
  unityImage.classes.push_back(&transformClass);
  processClass.fields.push_back({"boneClothSetupData","BeyondDynamicBone.RenderSetupData"});
  processClass.fields.push_back({"interlockingAnimator","UnityEngine.Animator"});
  for(auto item: {std::make_pair(&firstSetup,firstBones),std::make_pair(&secondSetup,secondBones)}) {
    auto &setup=*item.first;
    types[item.first]=&setupClass;
    for(int i=0;i<3;++i) {
      auto bone=item.second+i;
      nativeIds[bone]=&bone->id; types[bone]=&transformClass;
      setup.transforms.objects.push_back(bone);
      setup.ids.values.push_back(bone->id);
    }
    setup.parents.values={item.second[2].id,item.second[0].id,0};
    setup.roots.values={item.second[0].id};
    for(auto list: {&setup.transforms,&setup.ids,&setup.parents,&setup.roots}) {
      types[list]=list->integers?&intListClass:&transformListClass; lists[list]=list;
    }
  }
  for(auto animator: {&firstAnimator,&secondAnimator}) {nativeIds[animator]=&animator->id;types[animator]=&animatorClass;}
  il2cpp_domain_get_assemblies=+[](void *,size_t *n)->void ** {*n=3;return extendedAssemblies;};
  il2cpp_object_get_class=+[](void *o)->void * {
    auto k=types.find(o); if(k!=types.end())return k->second;
    return o==&p0||o==&p1?(void *)&processClass:(void *)&cloth;
  };
  il2cpp_field_get_value=ReadBoneField;
  il2cpp_runtime_invoke=CallBone;
}
int main(int argc,char **argv) {
  if(argc!=3)return 1; mode=argv[1]; SetupBoneHost();
  if(mode=="wrong-list-type") setupClass.fields[0].type="System.Collections.Generic.List<System.Object>";
  if(mode=="wrong-item-type") intListClass.methods[1].ret="System.Object";
  if(mode=="length-mismatch") firstSetup.parents.values.pop_back();
  if(mode=="id-mismatch") firstSetup.ids.values[1]=77;
  if(mode=="building") {p0.building=true;p1.building=true;}
  if(mode=="destroying") {p0.destroy=true;p1.destroy=true;}
  if(mode=="no-lookup") setupClass.methods.clear();
  if(mode=="off-thread") onThread=false;
  FILE *f=fopen(argv[2],"wb"); CHECK(f);
  bool ok=EiemWriteNativePhysicsProbe(f);fclose(f);
  CHECK(ok==(mode!="off-thread"));
  CHECK(roots.empty()); CHECK(allocated==freed); CHECK(forbidden==0);
  CHECK(firstBones[0].id==22 && firstBones[1].id==11 && firstBones[2].id==99);
  CHECK(firstSetup.skinBoneCount==2 && firstSetup.renderTransformIndex==2);
  if(mode=="building"||mode=="destroying"||mode=="off-thread") CHECK(setupCalls==0 && setupReads==0);
  if(mode=="off-thread") CHECK(invoked==0);
  return 0;
}
'''


class NativePhysicsBoneProbe(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'):
            raise unittest.SkipTest('Requires MSVC')
        cls.temp = tempfile.TemporaryDirectory(prefix='eiem-physics-bones-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.folder = Path(cls.temp.name)
        source = cls.folder / 'test.cpp'
        source.write_text(SOURCE, encoding='utf-8')
        cls.exe = cls.folder / 'test.exe'
        result = subprocess.run(['cl', '/nologo', '/O2', '/EHsc', '/std:c++17', '/utf-8',
                                 f'/I{ROOT / "src"}', f'/I{ROOT / "deps/minhook_lib/include"}',
                                 str(source), f'/Fe{cls.exe}'], cwd=cls.folder,
                                capture_output=True, text=True, errors='replace')
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def probe(self, mode='ok'):
        path = self.folder / f'{mode}.json'
        result = subprocess.run([str(self.exe), mode, str(path)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return json.loads(path.read_text(encoding='utf-8')) if mode != 'off-thread' else path.read_bytes()

    def test_component_anchor_is_not_a_mesh_palette_or_moving_bone(self):
        setup = self.probe()['components'][0]['boneSetup']
        self.assertEqual(setup['indexSpace'], 'native-bone-setup-not-mesh-palette')
        self.assertEqual((setup['skinBoneCount'], setup['transformCount'], setup['renderTransformIndex']), (2, 3, 2))
        self.assertEqual(setup['rootTransformIds'], [22])
        self.assertEqual([n['recordedId'] for n in setup['nodes']], [22, 11, 99])
        self.assertEqual([n['nativeLookupIndex'] for n in setup['nodes']], [0, 1, 2])
        self.assertEqual([n['parentRecordedId'] for n in setup['nodes']], [99, 22, 0])
        self.assertTrue(all(n['idMatches'] for n in setup['nodes']))
        self.assertTrue(setup['sameBindingsAndLengthsAtEnd'])
        self.assertEqual(setup['status'], 'observed-not-atomic')

    def test_same_labels_do_not_merge_different_instances(self):
        first, second = self.probe()['components']
        self.assertEqual((first['interlockingAnimatorId'], second['interlockingAnimatorId']), (501, 502))
        a, b = first['boneSetup']['nodes'][0], second['boneSetup']['nodes'][0]
        self.assertEqual(a['hierarchyLabel'], b['hierarchyLabel'])
        self.assertEqual((a['instanceId'], b['instanceId']), (22, 222))

    def test_metadata_contracts_include_valid_and_invalid_engine_bindings(self):
        contract = next(c for c in self.probe()['contracts'] if c['type'] == 'AnimationTransformRWBufferHandle')
        self.assertTrue(contract['available'])
        self.assertEqual({f['name']: f['type'] for f in contract['fields']}, {
            'count': 'System.UInt16', 'invalidCount': 'System.UInt16',
            'validTransformIndexsPtr': 'System.IntPtr', 'invalidTransformIndexsPtr': 'System.IntPtr'})

    def test_length_mismatch_does_not_zip_or_pad(self):
        setup = self.probe('length-mismatch')['components'][0]['boneSetup']
        self.assertEqual(setup['status'], 'bone-list-length-mismatch')
        self.assertNotIn('nodes', setup)

    def test_wrong_list_and_item_types_are_unavailable(self):
        for mode in ('wrong-list-type', 'wrong-item-type'):
            with self.subTest(mode=mode):
                self.assertEqual(self.probe(mode)['components'][0]['boneSetup']['status'], 'managed-bone-lists-unavailable')

    def test_id_mismatch_does_not_fall_back_to_zero(self):
        node = self.probe('id-mismatch')['components'][0]['boneSetup']['nodes'][1]
        self.assertFalse(node['idMatches'])
        self.assertEqual((node['recordedId'], node['instanceId'], node['nativeLookupIndex']), (77, 11, -1))

    def test_dead_node_is_not_dereferenced_for_lookup(self):
        node = self.probe('dead-node')['components'][0]['boneSetup']['nodes'][1]
        self.assertEqual(node['nativeAlive'], 0)
        self.assertIsNone(node['instanceId'])
        self.assertIsNone(node['nativeLookupIndex'])
        self.assertEqual(node['hierarchyLabel'], '')

    def test_building_or_destroying_does_not_walk_bones(self):
        for mode in ('building', 'destroying'):
            with self.subTest(mode=mode):
                self.assertTrue(all(c['boneSetup']['status'] == 'unavailable-or-transitioning' for c in self.probe(mode)['components']))

    def test_changed_references_and_lengths_are_not_reported_stable(self):
        for mode in ('changed-count', 'changed-list', 'changed-setup'):
            with self.subTest(mode=mode):
                self.assertFalse(self.probe(mode)['components'][0]['boneSetup']['sameBindingsAndLengthsAtEnd'])

    def test_missing_lookup_is_unknown_not_zero(self):
        nodes = self.probe('no-lookup')['components'][0]['boneSetup']['nodes']
        self.assertTrue(all(n['nativeLookupIndex'] is None for n in nodes))

    def test_item_exception_is_unknown_not_a_bone_identity(self):
        node = self.probe('item-exception')['components'][0]['boneSetup']['nodes'][1]
        self.assertIsNone(node['recordedId'])
        self.assertIsNone(node['idMatches'])

    def test_no_setup_and_non_unity_thread(self):
        self.assertEqual(self.probe('no-setup')['components'][0]['boneSetup']['status'], 'no-bone-setup')
        self.assertEqual(self.probe('off-thread'), b'')
