"""Actual C++ read-only probe against a neutral managed-object host, not game acceptance."""
from pathlib import Path
import json
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include "il2cpp_api.h"
void Log(const char *, ...) {}
static bool onThread=true;
static bool EiemOnUnityThread() { return onThread; }
#include "eiem_unity_lifetime.h"
static void EiemWriteJsonString(FILE *f,const char *s) {
  fputc('"',f); for(const char *p=s?s:"";*p;++p) { if(*p=='"'||*p=='\\') fputc('\\',f); fputc(*p,f); } fputc('"',f);
}
static void EiemReadObjectLabel(void *,char *out,size_t n) { strncpy_s(out,n,"NeutralCloth",_TRUNCATE); }
static void TraceBuildRendererHierarchy(void *,char *out,size_t n) { strncpy_s(out,n,"NeutralRig/Cloth",_TRUNCATE); }
#include "eiem_native_physics_probe.h"
struct Method { const char *name,*ret; uint32_t flags; std::vector<const char *> params; };
struct Field { const char *name,*type; int flags=0; };
struct Klass { const char *name,*ns; std::vector<Method> methods; std::vector<Field> fields; };
struct Image { const char *name; std::vector<Klass *> classes; };
struct Process { bool building=false,destroy=false,disposed=false,running=true; int32_t team=7; };
struct Vertex { int value=0; };
struct BoxedVertex { uintptr_t klass=0x11111111,monitor=0x22222222; Vertex value; };
struct Selection { std::vector<Vertex> *attributes=nullptr; };
struct Data2 { Selection *selection=nullptr; };
struct Component { Process *process; Data2 *data2; int32_t id; bool active; };
static Klass cloth{"BeyondBoneCloth","BeyondDynamicBone",
  {{"GetSerializeData2","BeyondDynamicBone.ClothSerializeData2",0,{}}},
  {{"process","BeyondDynamicBone.ClothProcess"},{"OnBuildComplete","System.Action<System.Boolean>"}}};
static Klass processClass{"ClothProcess","BeyondDynamicBone",
  {{"IsValid","System.Boolean",0,{}},{"IsRunning","System.Boolean",0,{}},{"get_TeamId","System.Int32",0,{}}},
  {{"isBuild","System.Boolean"},{"isDestory","System.Boolean"},{"isDestoryInternal","System.Boolean"}}};
static Klass data2Class{"ClothSerializeData2","BeyondDynamicBone",{},
  {{"selectionData","BeyondDynamicBone.SelectionData"}}};
static Klass selectionClass{"SelectionData","BeyondDynamicBone",
  {{"get_Count","System.Int32",0,{}}},{{"attributes","BeyondDynamicBone.VertexAttribute[]"}}};
static Klass vertexClass{"VertexAttribute","BeyondDynamicBone",
  {{"IsFixed","System.Boolean",0,{}},{"IsMove","System.Boolean",0,{}},{"IsInvalid","System.Boolean",0,{}}},{}};
static Klass resources{"Resources","UnityEngine",{{"FindObjectsOfTypeAll","UnityEngine.Object[]",0x10,{"System.Type"}}},{}};
static Klass arrayClass{"Array","System",{{"GetValue","System.Object",0,{"System.Int32"}}},{}};
static Klass behaviour{"Behaviour","UnityEngine",{{"get_isActiveAndEnabled","System.Boolean",0,{}}},{}};
static Klass unityObject{"Object","UnityEngine",{{"GetInstanceID","System.Int32",0,{}}},{}};
static Image nativeImage{"BeyondDynamicBone.dll",{&cloth,&processClass,&data2Class,&selectionClass,&vertexClass}};
static Image unityImage{"NeutralCore.dll",{&resources,&arrayClass,&behaviour,&unityObject}};
static void *assemblies[]={&nativeImage,&unityImage};
static Process p0,p1;
static std::vector<Vertex> attributes0{{1},{2},{0}},attributes1{{1},{2},{2},{0}};
static std::vector<BoxedVertex> boxedAttributes0{{0x11111111,0x22222222,{1}},
  {0x11111111,0x22222222,{2}},{0x11111111,0x22222222,{0}}};
static std::vector<BoxedVertex> boxedAttributes1{{0x11111111,0x22222222,{1}},
  {0x11111111,0x22222222,{2}},{0x11111111,0x22222222,{2}},
  {0x11111111,0x22222222,{0}}};
static Selection selection0{&attributes0},selection1{&attributes1};
static Data2 data20{&selection0},data21{&selection1};
static Component c0{&p0,&data20,101,true},c1{&p1,&data21,102,false};
static std::vector<void *> components{&c0,&c1};
static std::map<uint32_t,void *> roots;
static uint32_t nextRoot=0;
static int allocated=0,freed=0,forbidden=0,invoked=0;
static void *NextMethod(void *k,void **it) { auto &v=((Klass *)k)->methods; auto i=(uintptr_t)*it; if(i>=v.size())return nullptr; *it=(void *)(i+1);return &v[i]; }
static void *NextField(void *k,void **it) { auto &v=((Klass *)k)->fields; auto i=(uintptr_t)*it; if(i>=v.size())return nullptr; *it=(void *)(i+1);return &v[i]; }
static void ReadField(void *o,void *f,void *out) {
  auto &field=*(Field *)f;
  if(!strcmp(field.name,"process")) { *(void **)out=((Component *)o)->process; return; }
  if(!strcmp(field.name,"selectionData")) { *(void **)out=((Data2 *)o)->selection; return; }
  if(!strcmp(field.name,"attributes")) { *(void **)out=((Selection *)o)->attributes; return; }
  auto &p=*(Process *)o;
  if(!strcmp(field.name,"isBuild")) *(bool *)out=p.building;
  else if(!strcmp(field.name,"isDestory")) *(bool *)out=p.destroy;
  else if(!strcmp(field.name,"isDestoryInternal")) *(bool *)out=p.disposed;
  else abort();
}
static void *Call(void *m,void *o,void **args,void **exc) {
  ++invoked; *exc=nullptr; static bool boolean; static int32_t integer;
  if(m==(void *)1) { boolean=true;return &boolean; }
  const std::string name=((Method *)m)->name;
  if(name=="FindObjectsOfTypeAll") return &components;
  if(name=="GetValue") {
    int32_t index=*(int32_t *)args[0];
    if(o==&components) return components.at(index);
    if(o==&attributes0) return &boxedAttributes0.at(index);
    if(o==&attributes1) return &boxedAttributes1.at(index);
    abort();
  }
  if(name=="GetSerializeData2") return ((Component *)o)->data2;
  if(name=="get_Count") { integer=(int32_t)((Selection *)o)->attributes->size(); return &integer; }
  if(name=="IsFixed") { boolean=((Vertex *)o)->value==1; return &boolean; }
  if(name=="IsMove") { boolean=((Vertex *)o)->value==2; return &boolean; }
  if(name=="IsInvalid") { boolean=((Vertex *)o)->value==0; return &boolean; }
  if(name=="get_TeamId") return &((Process *)o)->team;
  if(name=="IsRunning") return &((Process *)o)->running;
  if(name=="IsValid") { boolean=!((Process *)o)->destroy; return &boolean; }
  if(name=="get_isActiveAndEnabled") return &((Component *)o)->active;
  if(name=="GetInstanceID") return &((Component *)o)->id;
  ++forbidden; *exc=(void *)1;return nullptr;
}
static void Setup() {
  il2cpp_domain_get=+[]()->void * {return &nativeImage;};
  il2cpp_domain_get_assemblies=+[](void *,size_t *n)->void ** {*n=2;return assemblies;};
  il2cpp_assembly_get_image=+[](void *a)->void * {return a;};
  il2cpp_image_get_name=+[](void *a)->const char * {return ((Image *)a)->name;};
  il2cpp_image_get_class_count=+[](void *a)->size_t {return ((Image *)a)->classes.size();};
  il2cpp_image_get_class=+[](void *a,size_t i)->void * {return ((Image *)a)->classes.at(i);};
  il2cpp_class_get_name=+[](void *k)->const char * {return ((Klass *)k)->name;};
  il2cpp_class_get_namespace=+[](void *k)->const char * {return ((Klass *)k)->ns;};
  il2cpp_class_from_name=+[](void *a,const char *ns,const char *name)->void * {for(auto k:((Image *)a)->classes) if(!strcmp(k->name,name)&&!strcmp(k->ns,ns))return k;return nullptr;};
  il2cpp_class_get_methods=NextMethod; il2cpp_class_get_fields=NextField;
  il2cpp_method_get_name=+[](void *m)->const char * {return ((Method *)m)->name;};
  il2cpp_method_get_param_count=+[](void *m)->uint32_t {return (uint32_t)((Method *)m)->params.size();};
  il2cpp_method_get_flags=+[](void *m,uint32_t *impl)->uint32_t {*impl=0;return ((Method *)m)->flags;};
  il2cpp_method_get_param=+[](void *m,uint32_t i)->void * {return (void *)((Method *)m)->params.at(i);};
  il2cpp_method_get_return_type=+[](void *m)->void * {return (void *)((Method *)m)->ret;};
  il2cpp_field_get_name=+[](void *f)->const char * {return ((Field *)f)->name;};
  il2cpp_field_get_type=+[](void *f)->void * {return (void *)((Field *)f)->type;};
  il2cpp_field_get_flags=+[](void *f)->int {return ((Field *)f)->flags;};
  il2cpp_field_get_value=ReadField;
  il2cpp_type_get_name=+[](void *t)->const char * {++allocated;return _strdup((const char *)t);};
  il2cpp_free=+[](void *p) {++freed;free(p);};
  il2cpp_class_get_type=+[](void *k)->void * {return k;};
  il2cpp_type_get_object=+[](void *t)->void * {return t;};
  il2cpp_array_length=+[](void *o)->uintptr_t {
    if(o==&components)return components.size();
    if(o==&attributes0)return attributes0.size();
    if(o==&attributes1)return attributes1.size();
    abort();
  };
  il2cpp_object_get_class=+[](void *o)->void * {
    if(o==&p0||o==&p1)return &processClass;
    if(o==&data20||o==&data21)return &data2Class;
    if(o==&selection0||o==&selection1)return &selectionClass;
    for(auto &value:boxedAttributes0)if(o==&value)return &vertexClass;
    for(auto &value:boxedAttributes1)if(o==&value)return &vertexClass;
    return &cloth;
  };
  il2cpp_object_unbox=+[](void *b)->void * {
    for(auto &value:boxedAttributes0)if(b==&value)return &value.value;
    for(auto &value:boxedAttributes1)if(b==&value)return &value.value;
    return b;
  };
  il2cpp_runtime_invoke=Call;
  il2cpp_gchandle_new=+[](void *p,bool)->uint32_t {roots[++nextRoot]=p;return nextRoot;};
  il2cpp_gchandle_get_target=+[](uint32_t h)->void * {return roots.at(h);};
  il2cpp_gchandle_free=+[](uint32_t h) {if(roots.erase(h)!=1)abort();};
  s_eiemNativeAliveMethod=(void *)1;
}
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 2;} }while(false)
int main(int argc,char **argv) {
  if(argc!=3)return 1; Setup(); std::string mode=argv[1];
  CHECK(EiemPhysicsMethod(&resources,"FindObjectsOfTypeAll","UnityEngine.Object[]",true,"System.Type"));
  CHECK(!EiemPhysicsMethod(&resources,"FindObjectsOfTypeAll","System.Object",true,"System.Type"));
  CHECK(!EiemPhysicsMethod(&resources,"FindObjectsOfTypeAll","UnityEngine.Object[]",false,"System.Type"));
  resources.methods.push_back(resources.methods[0]);
  CHECK(!EiemPhysicsMethod(&resources,"FindObjectsOfTypeAll","UnityEngine.Object[]",true,"System.Type"));
  resources.methods.pop_back();
  CHECK(!EiemPhysicsField(&processClass,"isBuild","System.Int32"));
  p1.team=9; p1.building=true; p1.destroy=true; p1.running=false;
  if(mode=="missing-api") il2cpp_field_get_value=nullptr;
  if(mode=="wrong-field-type") processClass.fields[0].type="System.Int32";
  if(mode=="wrong-process-type") cloth.fields[0].type="System.Object";
  if(mode=="wrong-return") processClass.methods[1].ret="System.Int32";
  if(mode=="no-process") c0.process=nullptr;
  if(mode=="no-enumerator") resources.methods.clear();
  if(mode=="off-thread") onThread=false;
  FILE *f=fopen(argv[2],"wb"); CHECK(f);
  bool ok=EiemWriteNativePhysicsProbe(f);fclose(f);
  CHECK(ok==(mode!="off-thread"));
  CHECK(roots.empty());CHECK(allocated==freed);CHECK(forbidden==0);
  CHECK(c1.process==&p1 && p1.building && p1.destroy && !p1.disposed);
  if(mode=="off-thread" || mode=="missing-api") CHECK(invoked==0);
  return 0;
}
'''


class NativePhysicsProbe(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'):
            raise unittest.SkipTest('Requires MSVC')
        cls.temp = tempfile.TemporaryDirectory(prefix='eiem-physics-probe-')
        cls.folder = Path(cls.temp.name)
        cls.addClassCleanup(cls.temp.cleanup)
        source = cls.folder / 'test.cpp'
        source.write_text(SOURCE, encoding='utf-8')
        cls.exe = cls.folder / 'test.exe'
        result = subprocess.run(['cl', '/nologo', '/O2', '/EHsc', '/std:c++17', '/utf-8',
                                 f'/I{ROOT / "src"}', f'/I{ROOT / "deps/minhook_lib/include"}',
                                 str(source), f'/Fe{cls.exe}'], cwd=cls.folder, capture_output=True, text=True, errors='replace')
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def probe(self, mode='ok'):
        path = self.folder / f'{mode}.json'
        result = subprocess.run([str(self.exe), mode, str(path)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return json.loads(path.read_text(encoding='utf-8')) if mode != 'off-thread' else path.read_bytes()

    def test_multiple_native_instances_and_actual_contract(self):
        data = self.probe()
        self.assertEqual(data['status'], 'enumerated')
        self.assertEqual(data['count'], 2)
        first, second = data['components']
        self.assertEqual((first['teamId'], second['teamId']), (7, 9))
        self.assertEqual((first['instanceId'], second['instanceId']), (101, 102))
        self.assertTrue(first['activeAndEnabled'])
        self.assertFalse(second['activeAndEnabled'])
        self.assertTrue(second['isBuild'] and second['isDestory'])
        self.assertFalse(second['isDestoryInternal'])
        self.assertEqual(first['status'], 'observed-not-atomic')
        self.assertEqual(first['selection']['status'], 'observed-not-atomic')
        self.assertEqual(
            (first['selection']['count'], first['selection']['fixed'],
             first['selection']['move'], first['selection']['invalid']),
            (3, 1, 1, 1))
        self.assertTrue(first['selection']['sameAttributeArrayAtEnd'])
        callback = data['contracts'][0]['fields'][1]
        self.assertEqual(callback['type'], 'System.Action<System.Boolean>')

    def test_missing_api_is_not_empty_success(self):
        self.assertEqual(self.probe('missing-api')['status'], 'metadata-api-unavailable')

    def test_wrong_field_type_is_unknown_not_false(self):
        self.assertIsNone(self.probe('wrong-field-type')['components'][0]['isBuild'])

    def test_missing_process_and_wrong_process_type_are_distinct(self):
        self.assertEqual(self.probe('no-process')['components'][0]['status'], 'no-process')
        self.assertEqual(self.probe('wrong-process-type')['components'][0]['status'], 'process-field-unavailable')

    def test_wrong_method_signature_is_not_invoked(self):
        self.assertIsNone(self.probe('wrong-return')['components'][0]['IsRunning'])

    def test_no_enumerator_is_reported(self):
        self.assertEqual(self.probe('no-enumerator')['status'], 'enumeration-unavailable')

    def test_no_calls_from_wrong_thread(self):
        self.assertEqual(self.probe('off-thread'), b'')
