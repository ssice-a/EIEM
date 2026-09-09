"""Exercise the real native-parameter adapter; host simulation is not game acceptance."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_native_physics_probe import SOURCE as PROBE_HOST

ROOT = Path(__file__).resolve().parents[1]
SOURCE = PROBE_HOST.replace('int main(int argc,char **argv)', 'static int unusedProbeMain(int argc,char **argv)') + r'''
#include "eiem_native_physics_parameters.h"
struct Config {
  float gravity=5,blendWeight=1;
  std::shared_ptr<std::vector<float>> curve=std::make_shared<std::vector<float>>(std::initializer_list<float>{2,3});
};
static Klass dataClass{"ClothSerializeData","BeyondDynamicBone",
  {{".ctor","System.Void",0,{}},{"Import","System.Void",0,{"BeyondDynamicBone.ClothSerializeData","System.Boolean"}}},
  {{"gravity","System.Single"},{"blendWeight","System.Single"}}};
static Config sourceConfig,foreignConfig;
static std::vector<std::unique_ptr<Config>> allocatedConfigs;
static std::map<void *,Config *> liveConfigs;
static bool failImport=false,failSetter=false,failNotify=false,nativeDead=false;
static int setters=0,notifications=0,deepCopies=0;
static void *ParamCall(void *m,void *o,void **args,void **exc) {
  *exc=nullptr;
  if(m==(void *)1) { static bool alive; alive=!nativeDead;return &alive; }
  std::string name=((Method *)m)->name;
  if(name==".ctor") return nullptr;
  if(name=="Import") {
    if(!*(bool *)args[1]) abort(); ++deepCopies;
    if(failImport) {*exc=(void *)1;return nullptr;}
    auto &src=*(Config *)args[0], &dst=*(Config *)o;
    dst=src;dst.curve=std::make_shared<std::vector<float>>(*src.curve);return nullptr;
  }
  if(name=="IsValid" && (o==&c0||o==&c1)) { static bool valid;valid=!((Component *)o)->process->destroy;return &valid; }
  if(name=="set_SerializeData") {
    ++setters;liveConfigs[o]=failSetter?nullptr:(Config *)args[0];
    if(failSetter) *exc=(void *)1;
    return nullptr;
  }
  if(name=="SetParameterChange") {++notifications;if(failNotify)*exc=(void *)1;return nullptr;}
  return Call(m,o,args,exc);
}
static void ParamRead(void *o,void *f,void *out) {
  auto name=std::string(((Field *)f)->name);
  if(name=="serializeData") {*(void **)out=liveConfigs[o];return;}
  if(name=="gravity") {*(float *)out=((Config *)o)->gravity;return;}
  if(name=="blendWeight") {*(float *)out=((Config *)o)->blendWeight;return;}
  ReadField(o,f,out);
}
static void SetupParameters() {
  Setup(); nativeImage.classes.push_back(&dataClass);
  cloth.fields.push_back({"serializeData","BeyondDynamicBone.ClothSerializeData"});
  cloth.methods={{"IsValid","System.Boolean",0,{}},
    {"set_SerializeData","System.Void",0,{"BeyondDynamicBone.ClothSerializeData"}},
    {"SetParameterChange","System.Void",0,{}}};
  liveConfigs[&c0]=liveConfigs[&c1]=&sourceConfig;
  il2cpp_object_new=+[](void *k)->void * {if(k!=&dataClass)abort(); allocatedConfigs.push_back(std::make_unique<Config>());return allocatedConfigs.back().get();};
  il2cpp_object_get_class=+[](void *o)->void * {
    if(o==&c0||o==&c1)return &cloth;
    if(o==&p0||o==&p1)return &processClass;
    return &dataClass;
  };
  il2cpp_field_get_value=ParamRead;
  il2cpp_field_set_value=+[](void *o,void *f,void *v) {
    std::string name=((Field *)f)->name;
    if(name=="gravity") ((Config *)o)->gravity=*(float *)v;
    else if(name=="blendWeight") ((Config *)o)->blendWeight=*(float *)v;
    else abort();
  };
  il2cpp_runtime_invoke=ParamCall;
}
int main(int argc,char **argv) {
  if(argc!=2)return 1;SetupParameters();std::string mode=argv[1],error;
  EiemPhysicsParameterApi api; CHECK(api.Resolve(assemblies,2,error));
  if(mode=="resolve-contract") {
    dataClass.methods[1].params[1]="System.Int32";
    CHECK(!api.Resolve(assemblies,2,error)); CHECK(deepCopies==0);return 0;
  }
  EiemPhysicsParameters journal;
  const std::string owner="mod/PhysicsBody";
  auto prepare=[&](void *target,float gravity) {return journal.Prepare(api,target,owner,{{"gravity",gravity}},error);};
  auto apply=[&](void *target) {return journal.Apply(api,target,owner,error);};
  auto restore=[&](void *target) {return journal.Restore(api,target,owner,error);};
  if(mode=="invalid") {
    CHECK(!journal.Prepare(api,&c0,owner,{{"rootBones",1}},error));
    CHECK(!journal.Prepare(api,&c0,owner,{{"gravity",-1}},error));
    CHECK(!journal.Prepare(api,&c0,owner,{{"gravity",1},{"gravity",2}},error));
    CHECK(!journal.Prepare(api,&c0,owner,{},error));
    CHECK(!journal.Prepare(api,&c0,"",{{"gravity",1}},error));
    CHECK(deepCopies==0 && journal.Count()==0);CHECK(roots.empty());return 0;
  }
  if(mode=="copy-failed") {
    failImport=true;CHECK(!prepare(&c0,1));
    CHECK(liveConfigs[&c0]==&sourceConfig && sourceConfig.gravity==5);
    CHECK(roots.empty() && setters==0 && journal.Count()==0);return 0;
  }
  if(mode=="admission") {
    for(int state=0;state<3;++state) {
      CHECK(prepare(&c0,1));
      p0.building=state==0;p0.destroy=state==1;onThread=state!=2;
      CHECK(!apply(&c0));CHECK(setters==0);
      p0.building=false;p0.destroy=false;onThread=true;
      CHECK(restore(&c0));CHECK(journal.Count()==0);
    }
    CHECK(roots.empty());return 0;
  }
  if(mode=="edit-order") {
    CHECK(journal.Prepare(api,&c0,owner,{{"gravity",1},{"blendWeight",0.5f}},error));
    CHECK(journal.Prepare(api,&c0,owner,{{"blendWeight",0.5f},{"gravity",1}},error));
    CHECK(deepCopies==1 && journal.Count()==1);CHECK(apply(&c0));CHECK(restore(&c0));
    CHECK(sourceConfig.blendWeight==1 && roots.empty());return 0;
  }
  CHECK(prepare(&c0,1));
  auto *copy=allocatedConfigs.back().get();
  CHECK(copy!=&sourceConfig && copy->gravity==1 && sourceConfig.gravity==5);
  CHECK(copy->curve!=sourceConfig.curve && *copy->curve==*sourceConfig.curve);
  CHECK(liveConfigs[&c0]==&sourceConfig && journal.Count()==1 && setters==0);
  if(mode=="shared-component") {
    CHECK(prepare(&c0,1));CHECK(deepCopies==1 && journal.Count()==1);
  }
  if(mode=="ownership") {
    liveConfigs[&c0]=&foreignConfig;CHECK(!apply(&c0));CHECK(setters==0);
    CHECK(!prepare(&c0,1));CHECK(journal.Count()==1);
    CHECK(restore(&c0));CHECK(liveConfigs[&c0]==&foreignConfig);
    liveConfigs[&c0]=&sourceConfig;
    CHECK(prepare(&c0,1));copy=allocatedConfigs.back().get();
  }
  if(mode=="setter-failed") {
    failSetter=true;CHECK(!apply(&c0));CHECK(journal.Count()==1 && !roots.empty());
    CHECK(!prepare(&c0,1));CHECK(!apply(&c0));CHECK(setters==1);
    CHECK(liveConfigs[&c0]==nullptr);failSetter=false;
  } else {
    CHECK(apply(&c0));CHECK(liveConfigs[&c0]==copy);
  }
  if(mode=="shared-component") {
    int before=setters;
    CHECK(prepare(&c0,1));CHECK(apply(&c0));
    CHECK(deepCopies==1 && journal.Count()==1 && setters==before);
  }
  if(mode=="conflicting-owner") {
    int before=setters;
    CHECK(!journal.Prepare(api,&c0,"other/PhysicsBody",{{"gravity",2}},error));
    CHECK(!journal.Apply(api,&c0,"other/PhysicsBody",error));
    CHECK(!journal.Restore(api,&c0,"other/PhysicsBody",error));
    CHECK(deepCopies==1 && setters==before && journal.Count()==1);
  }
  if(mode=="new-generation") {
    CHECK(!prepare(&c0,2));CHECK(deepCopies==1);
    CHECK(restore(&c0));CHECK(liveConfigs[&c0]==&sourceConfig);
    CHECK(prepare(&c0,2));CHECK(apply(&c0));
    copy=liveConfigs[&c0];CHECK(copy->gravity==2 && sourceConfig.gravity==5);
  }
  if(mode=="wrong-thread-journal") {
    int before=setters;onThread=false;
    CHECK(!prepare(&c0,1));CHECK(!apply(&c0));CHECK(!restore(&c0));
    CHECK(!journal.RestoreAll(api,error));onThread=true;
    CHECK(journal.Count()==1 && deepCopies==1 && setters==before);
  }
  if(mode=="two-instances") {
    CHECK(prepare(&c1,2));CHECK(apply(&c1));
    CHECK(liveConfigs[&c1]!=copy && liveConfigs[&c1]->gravity==2);
    CHECK(restore(&c0)); CHECK(liveConfigs[&c0]==&sourceConfig);
    CHECK(liveConfigs[&c1]->gravity==2);CHECK(restore(&c1));
  } else if(mode=="restore-all-partial") {
    CHECK(prepare(&c1,2));CHECK(apply(&c1));
    p0.building=true;CHECK(!journal.RestoreAll(api,error));
    CHECK(journal.Count()==1 && liveConfigs[&c1]==&sourceConfig);
    CHECK(liveConfigs[&c0]==copy && !prepare(&c0,1));
    p0.building=false;CHECK(journal.RestoreAll(api,error));
  } else if(mode=="restore-failed") {
    failNotify=true;CHECK(!restore(&c0));
    CHECK(journal.Count()==1 && liveConfigs[&c0]==&sourceConfig && !roots.empty());
    CHECK(!prepare(&c0,1));CHECK(!apply(&c0));
    failNotify=false;CHECK(restore(&c0));
  } else if(mode=="foreign-after-apply") {
    liveConfigs[&c0]=&foreignConfig;int before=setters;
    CHECK(!apply(&c0));CHECK(!restore(&c0));CHECK(setters==before && journal.Count()==1);
    liveConfigs[&c0]=copy;CHECK(restore(&c0));
  } else if(mode=="destroyed") {
    int before=setters;nativeDead=true;CHECK(restore(&c0));CHECK(setters==before);
  } else CHECK(restore(&c0));
  CHECK(journal.Count()==0);
  CHECK(sourceConfig.gravity==5 && sourceConfig.blendWeight==1);
  CHECK(roots.empty());CHECK(allocated==freed);CHECK(forbidden==0);return 0;
}
'''


class NativePhysicsParameters(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'):
            raise unittest.SkipTest('Requires MSVC')
        cls.temp = tempfile.TemporaryDirectory(prefix='eiem-physics-parameters-')
        cls.addClassCleanup(cls.temp.cleanup)
        cls.folder = Path(cls.temp.name)
        source = cls.folder / 'test.cpp'
        source.write_text(SOURCE, encoding='utf-8')
        cls.exe = cls.folder / 'test.exe'
        result = subprocess.run(['cl','/nologo','/O2','/EHsc','/std:c++17','/utf-8',
                                 f'/I{ROOT / "src"}', f'/I{ROOT / "deps/minhook_lib/include"}',
                                 str(source),f'/Fe{cls.exe}'],cwd=cls.folder,capture_output=True,text=True,errors='replace')
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)

    def run_case(self, name):
        result = subprocess.run([str(self.exe),name],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout + result.stderr)

    def test_exact_deep_copy_signature(self): self.run_case('resolve-contract')
    def test_invalid_edits_do_not_allocate_or_write(self): self.run_case('invalid')
    def test_copy_failure_leaves_source_unchanged(self): self.run_case('copy-failed')
    def test_private_copy_and_original_restoration(self): self.run_case('normal')
    def test_shared_source_isolated_between_instances(self): self.run_case('two-instances')
    def test_reject_build_destroy_and_wrong_thread(self): self.run_case('admission')
    def test_detect_binding_changed_before_apply(self): self.run_case('ownership')
    def test_setter_clears_then_throws_retains_baseline(self): self.run_case('setter-failed')
    def test_failed_restore_keeps_record_until_retry(self): self.run_case('restore-failed')
    def test_do_not_overwrite_foreign_binding(self): self.run_case('foreign-after-apply')
    def test_native_death_does_not_call_setter(self): self.run_case('destroyed')
    def test_same_component_reuses_baseline_and_notification(self): self.run_case('shared-component')
    def test_different_owner_cannot_replace_or_restore(self): self.run_case('conflicting-owner')
    def test_changed_values_require_restore_then_prepare(self): self.run_case('new-generation')
    def test_edit_order_does_not_duplicate_shared_binding(self): self.run_case('edit-order')
    def test_restore_all_retains_only_failed_entries(self): self.run_case('restore-all-partial')
    def test_all_journal_entrypoints_require_unity_thread(self): self.run_case('wrong-thread-journal')
