"""Compile the DLL diagnostics against a managed host; no game acceptance claim."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest

from test_native_physics_probe import SOURCE as HOST

ROOT = Path(__file__).resolve().parents[1]
SOURCE = HOST.split('\nint main(', 1)[0] + r'''
static Klass animator{"Animator","UnityEngine",{},{}},resultClass{"ResultCode","BeyondDynamicBone",{},{}},wrongClass{"Wrong","Neutral",{}, {}};
static Image animationImage{"UnityEngine.AnimationModule.dll",{&animator}};
static void *allAssemblies[]={&nativeImage,&unityImage,&animationImage};
struct ResultValue { int code; };
struct ResultBox { uintptr_t header=0x1234; ResultValue data; };
static ResultBox result0{0x1234,{101}},result1{0x1234,{307}};
static std::string mode;
static int resolved=0,targetCalled=0,pinned=0,resultReads=0,predicates=0;
__declspec(noinline) static void EngineTarget() { ++targetCalled; }
static void *ResultCall(void *m,void *o,void **args,void **exc) {
  if (m==(void *)1) return Call(m,o,args,exc);
  const std::string name=((Method *)m)->name;
  if (name=="get_Result") {
    ++resultReads; *exc=mode=="read-exception"?(void *)1:nullptr;
    return o==&p0?(void *)&result0:(void *)&result1;
  }
  if (name=="IsSuccess" || name=="IsProcess" || name=="IsCancel" || name=="IsError" || name=="IsWarning") {
    if(o!=&result0.data && o!=&result1.data) abort(); // never pass box header or process
    ++predicates; *exc=nullptr;
    if(mode=="predicate-exception" && name=="IsCancel") {*exc=(void *)1;return nullptr;}
    static bool value;
    value=(((ResultValue *)o)->code==101 && name=="IsProcess") ||
          (((ResultValue *)o)->code==307 && name=="IsCancel");
    return &value;
  }
  return Call(m,o,args,exc);
}
int main(int argc,char **argv) {
  if(argc!=3) return 1; mode=argv[1]; Setup();
  for (const char *name:{"CreateClothBindings_Injected","CreateClothBindingsByNameLst_Injected",
      "EnableClothBindings","DisableClothBindings","DestroyClothBindings"}) animator.methods.push_back({name,"System.Void",0,{}});
  for (const char *name:{"IsSuccess","IsProcess","IsCancel","IsError","IsWarning"})
    resultClass.methods.push_back({name,"System.Boolean",0,{}});
  processClass.methods.push_back({"get_Result","BeyondDynamicBone.ResultCode",0,{}});
  nativeImage.classes.push_back(&resultClass);
  il2cpp_domain_get_assemblies=+[](void *,size_t *n)->void ** {*n=3;return allAssemblies;};
  il2cpp_method_get_flags=+[](void *m,uint32_t *impl)->uint32_t {
    auto &method=*(Method *)m; *impl=0;
    for(auto &entry:animator.methods) if(m==&entry && mode!="wrong-icall-flags") *impl=0x1000;
    return method.flags;
  };
  il2cpp_resolve_icall=+[](const char *name)->void * {
    if(strncmp(name,"UnityEngine.Animator::",22))abort();
    ++resolved;
    if(mode=="unresolved")return nullptr;
    if(mode=="resolver-exception")RaiseException(0xe1000042,0,0,nullptr);
    return mode=="non-code-target"?(void *)&result0:(void *)EngineTarget;
  };
  il2cpp_type_get_type=+[](void *t)->int {return t==&resultClass && mode!="wrong-value-type"?0x11:0x12;};
  il2cpp_object_get_class=+[](void *o)->void * {
    if(o==&result0||o==&result1)return mode=="wrong-result-class"?(void *)&wrongClass:(void *)&resultClass;
    return o==&p0||o==&p1?(void *)&processClass:(void *)&cloth;
  };
  il2cpp_object_unbox=+[](void *b)->void * {
    if(b==&result0||b==&result1)return mode=="unbox-failed"?nullptr:(void *)&((ResultBox *)b)->data;
    return b;
  };
  il2cpp_gchandle_new=+[](void *p,bool pin)->uint32_t {
    if(pin) {++pinned; if(mode=="pin-failed")return 0;}
    roots[++nextRoot]=p; return nextRoot;
  };
  il2cpp_runtime_invoke=ResultCall;
  if(mode=="no-resolver") il2cpp_resolve_icall=nullptr;
  if(mode=="duplicate-icall") animator.methods.push_back(animator.methods[0]);
  if(mode=="wrong-getter") processClass.methods.back().ret="System.Object";
  if(mode=="wrong-predicate") resultClass.methods[0].ret="System.Int32";
  if(mode=="off-thread") onThread=false;
  FILE *f=fopen(argv[2],"wb");CHECK(f);
  bool ok=EiemWriteNativePhysicsProbe(f);fclose(f);
  CHECK(ok==(mode!="off-thread"));
  CHECK(roots.empty() && allocated==freed && forbidden==0 && targetCalled==0);
  CHECK(result0.header==0x1234 && result0.data.code==101 && result1.data.code==307);
  if(mode=="ok") CHECK(resolved==5 && resultReads==2 && predicates==10 && pinned==2);
  if(mode=="wrong-value-type"||mode=="wrong-getter") CHECK(resultReads==0 && predicates==0);
  if(mode=="wrong-icall-flags"||mode=="no-resolver")CHECK(resolved==0);
  if(mode=="duplicate-icall")CHECK(resolved==4);
  if(mode=="off-thread")CHECK(invoked==0 && resolved==0 && resultReads==0);
  return 0;
}
'''


class NativePhysicsContractProbe(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'):
            raise unittest.SkipTest('Requires MSVC')
        cls.temp = tempfile.TemporaryDirectory(prefix='eiem-physics-contract-')
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

    def test_addresses_resolve_without_calling_targets(self):
        data = self.probe()['engineEntryPoints']
        self.assertEqual(data['proof'], 'resolved-addresses-only-targets-not-invoked')
        self.assertEqual(len(data['entries']), 5)
        for entry in data['entries']:
            self.assertEqual(entry['status'], 'resolved')
            self.assertTrue(entry['executable'])
            self.assertEqual(entry['module'], 'test.exe')
            self.assertGreater(int(entry['rva'], 16), 0)

    def test_resolution_failures_and_non_code_are_explicit(self):
        for mode, status in [('no-resolver', 'resolver-unavailable'), ('unresolved', 'unresolved'),
                             ('resolver-exception', 'unresolved'),
                             ('wrong-icall-flags', 'icall-metadata-unavailable-or-ambiguous')]:
            with self.subTest(mode=mode):
                self.assertTrue(all(e['status'] == status for e in self.probe(mode)['engineEntryPoints']['entries']))
        self.assertFalse(self.probe('non-code-target')['engineEntryPoints']['entries'][0]['executable'])

    def test_ambiguous_icall_is_not_resolved(self):
        self.assertEqual(self.probe('duplicate-icall')['engineEntryPoints']['entries'][0]['status'],
                         'icall-metadata-unavailable-or-ambiguous')

    def test_result_copy_distinguishes_building_and_cancelled(self):
        a, b = [c['buildResult'] for c in self.probe()['components']]
        self.assertEqual(a['status'], 'observed-result-copy-not-completion-fence')
        self.assertTrue(a['IsProcess'])
        self.assertFalse(a['IsSuccess'])
        self.assertTrue(b['IsCancel'])
        self.assertFalse(b['IsSuccess'])

    def test_wrong_result_contract_does_not_invoke_getter(self):
        for mode in ('wrong-value-type', 'wrong-getter'):
            self.assertEqual(self.probe(mode)['components'][0]['buildResult']['status'], 'result-value-contract-unavailable')

    def test_result_failure_does_not_become_false_success(self):
        for mode in ('read-exception', 'wrong-result-class'):
            self.assertEqual(self.probe(mode)['components'][0]['buildResult']['status'], 'result-read-unavailable')
        self.assertEqual(self.probe('pin-failed')['components'][0]['buildResult']['status'], 'result-pin-unavailable')
        for mode, field in [('unbox-failed', 'IsSuccess'), ('wrong-predicate', 'IsSuccess'), ('predicate-exception', 'IsCancel')]:
            self.assertIsNone(self.probe(mode)['components'][0]['buildResult'][field])

    def test_wrong_thread_does_no_work(self):
        self.assertEqual(self.probe('off-thread'), b'')
