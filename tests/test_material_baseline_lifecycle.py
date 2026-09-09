"""Real source capture, controller-init/commit hooks and restore; simulated Unity.

Replays v41's observed order. This does not simulate GPU drawing or map loading.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_restore_lifecycle import FIXTURE
from test_mod_controls import function

ROOT = Path(__file__).resolve().parents[1]

EXTRA = r'''
static uintptr_t s_eiemActivePrefabInstance=0;
static bool programEnabled=true,unityThread=true;
static bool EiemOnUnityThread() { return unityThread; }
static EiemModRule activeRule{};
static bool EiemFindRenderRuleBySection(const char *,const char *,EiemModRule *out) {
  if(!programEnabled) return false; *out=activeRule; return true;
}
static std::map<uint32_t,Array> roots;
static uint32_t nextRoot=100;
static auto il2cpp_gchandle_new=+[](void *p,bool)->uint32_t { roots[++nextRoot]=*(Array *)p; return nextRoot; };
static auto il2cpp_gchandle_get_target=+[](uint32_t h)->void * { auto it=roots.find(h); return it==roots.end()?nullptr:&it->second; };
static auto il2cpp_gchandle_free=+[](uint32_t h) { roots.erase(h); };
static void *g_skinnedMeshRendererClass=(void *)5;
static auto il2cpp_object_get_class=+[](void *)->void * { return g_skinnedMeshRendererClass; };
static auto il2cpp_class_get_parent=+[](void *)->void * { return nullptr; };
static void TraceReadUnityObjectName(void *,char *out,size_t n) { strcpy_s(out,n,"Cloth"); }
struct EiemResolvedRenderRule { EiemModRule rule{}; char asset[192]{}; };
static LONG s_traceMaterialCommitCount=0;
static bool TraceTakeBudget(LONG *,int) { return false; }
static int originalMaterial,modMaterial,otherOriginal,otherGameMaterial;
static Array builtMaterials;
static int materialBuilds=0;
static bool EiemBuildRendererMaterialsForSource(const EiemModRule &,void *,void **out,char *,size_t) {
  ++materialBuilds; builtMaterials=currentMaterials;
  for(uint32_t i=0;i<activeRule.materialCount;++i) {
    size_t slot=activeRule.materialSlots[i];
    builtMaterials.count=(std::max)(builtMaterials.count,slot+1);
    builtMaterials.items[slot]=&modMaterial;
  }
  *out=&builtMaterials; return true;
}
static thread_local std::vector<void *> s_eiemMaterialSourceInitRenderers;
'''

CONTROLLER = r'''
struct Controller { void *renderer=nullptr; Array sourceMaterials; };
static void *s_origMaterialInfoInit=nullptr;
static void *s_origRendererInfoTrySetSharedMaterials=nullptr;
using TraceRendererInfoMaterialCommitFn=bool(*)(void *,void *,void *);
static void EiemProbeMaterialInfo(void *,const char *) {}
static void *EiemReadRendererFromMaterialInfo(void *info) { return ((Controller *)info)->renderer; }
static bool TraceRendererInfoTrySetSharedMaterials(void *,void *,void *);
static bool nestedCommit=false;
static int initCalls=0;
static void GameInit(void *info,void *renderer,void *,void *) {
  ++initCalls;
  auto c=(Controller *)info; c->renderer=renderer;
  if(nestedCommit) {
    Array values=currentMaterials;
    TraceRendererInfoTrySetSharedMaterials(info,&values,nullptr);
  }
  c->sourceMaterials=currentMaterials;
}
static bool GameCommit(void *,void *materials,void *) {
  currentMaterials=*(Array *)materials; return true;
}
'''

MAIN = r'''
static bool Enable() {
  programEnabled=true;
  if(!EiemCaptureOriginal(&tokens[0],&tokens[0],&tokens[1],"SkinnedMeshRenderer",false,&activeRule)) return false;
  EiemRememberRuleBinding(&tokens[0],activeRule);
  for(uint32_t i=0;i<activeRule.materialCount;++i) {
    size_t slot=activeRule.materialSlots[i];
    currentMaterials.count=(std::max)(currentMaterials.count,slot+1);
    currentMaterials.items[slot]=&modMaterial;
  }
  return true;
}
static void Disable() { EiemRestoreRenderOverrides(); programEnabled=false; }
int main(int argc,char **argv) {
  CHECK(argc==2); std::string scenario=argv[1];
  g_renderer_get_sharedMaterials=(void *)1;
  s_eiemRendererSetSharedMaterials=(void *)2;
  s_eiemMaterialClass=(void *)3;
  s_origMaterialInfoInit=(void *)GameInit;
  s_origRendererInfoTrySetSharedMaterials=(void *)GameCommit;
  strcpy_s(activeRule.modPath,"test"); strcpy_s(activeRule.section,"RenderCloth");
  activeRule.materialCount=1; activeRule.materialSlots[0]=0;
  if(scenario=="added_slot") { activeRule.materialCount=2; activeRule.materialSlots[1]=2; }
  currentMaterials.count=2; currentMaterials.items[0]=&originalMaterial;
  currentMaterials.items[1]=&otherOriginal;
  Controller controller;
  if(scenario=="early_init") INIT_HOOK(&controller,&tokens[0],nullptr,nullptr);
  CHECK(Enable());
  currentMaterials.items[1]=&otherGameMaterial; // an unowned game slot changed
  if(scenario=="failed_exposure") {
    failMaterialRestore=true;
    INIT_HOOK(&controller,&tokens[0],nullptr,nullptr);
    CHECK(initCalls==1 && materialBuilds==0 && s_eiemOverrides.size()==1);
    auto baseline=(Array *)il2cpp_gchandle_get_target(s_eiemOverrides[0].originalMaterialsHandle);
    CHECK(baseline->items[0]==&originalMaterial);
    CHECK(!EiemMaterialSourceInitActive(&tokens[0]));
    failMaterialRestore=false; // next explicit initialization can expose the source
  }
  nestedCommit=scenario=="nested_commit";
  if(scenario!="early_init") INIT_HOOK(&controller,&tokens[0],nullptr,nullptr);
  CHECK(currentMaterials.items[0]==&modMaterial); // init cannot disable an active mod
  CHECK(liveMesh==&tokens[1]); // material init must not touch mesh or skin
  CHECK(currentMaterials.items[1]==&otherGameMaterial);
  CHECK(controller.sourceMaterials.items[0]==&originalMaterial);
  CHECK(controller.sourceMaterials.count==2);
  if(scenario=="added_slot") CHECK(currentMaterials.count==3 && currentMaterials.items[2]==&modMaterial);
  if(scenario!="early_init") CHECK(controller.sourceMaterials.items[1]==&otherGameMaterial);
  if(scenario=="repeated_init") {
    INIT_HOOK(&controller,&tokens[0],nullptr,nullptr);
    CHECK(controller.sourceMaterials.items[0]==&originalMaterial);
    CHECK(currentMaterials.items[0]==&modMaterial);
  }
  if(scenario=="nested_commit") CHECK(materialBuilds==1); // only after the init boundary
  for(int cycle=0;cycle<3;++cycle) {
    Disable();
    CHECK(currentMaterials.items[0]==&originalMaterial && s_eiemOverrides.empty());
    CHECK(currentMaterials.count==2);
    TraceRendererInfoTrySetSharedMaterials(&controller,&controller.sourceMaterials,nullptr);
    CHECK(currentMaterials.items[0]==&originalMaterial);
    Disable();
    CHECK(currentMaterials.items[0]==&originalMaterial);
    CHECK(Enable());
    const auto &s=s_eiemOverrides[0];
    auto captured=(Array *)il2cpp_gchandle_get_target(s.originalMaterialsHandle);
    CHECK(captured->items[0]==&originalMaterial);
  }
  Disable();
  CHECK(currentMaterials.items[0]==&originalMaterial && s_eiemOverrides.empty());
  CHECK(roots.empty());
  {
    EiemMaterialSourceInitScope first(&tokens[0]);
    CHECK(EiemMaterialSourceInitActive(&tokens[0]) && !EiemMaterialSourceInitActive(&tokens[3]));
    {
      EiemMaterialSourceInitScope second(&tokens[3]);
      CHECK(EiemMaterialSourceInitActive(&tokens[0]) && EiemMaterialSourceInitActive(&tokens[3]));
    }
    CHECK(EiemMaterialSourceInitActive(&tokens[0]) && !EiemMaterialSourceInitActive(&tokens[3]));
  }
  CHECK(s_eiemMaterialSourceInitRenderers.empty());
  return 0;
}
'''


class MaterialBaselineLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'):
            raise unittest.SkipTest('Requires MSVC')
        trace=(ROOT/'src/il2cpp_trace.h').read_text(encoding='utf-8')
        begin=trace.index('struct EiemRenderOverrideState {')
        state=trace[begin:trace.index('\n};',begin)+3]
        fixture=FIXTURE.split('static void Apply()')[0]
        fixture=fixture.replace('struct EiemUnityShapes {', 'struct EiemUnityShapes { template<class T> bool Snapshot(void *,void *,T &) { return true; }')
        begin=fixture.index('static auto il2cpp_gchandle_get_target=')
        end=fixture.index('static void *il2cpp_array_new(',begin)
        fixture=fixture[:begin]+EXTRA+fixture[end:]
        signatures=[
            'static size_t EiemFindOverrideLocked(',
            'static bool EiemCaptureOriginal(',
            'static void EiemRememberRuleBinding(',
            'static bool EiemFindBoundRenderRule(',
            'static void EiemBeginMeshWrite(',
            'static void EiemRememberReplacement(',
            'static void EiemReleaseOverrideHandles(',
            'static void EiemRestoreRenderOverrides(',
            'static bool EiemAssignRendererMaterials(',
        ]
        funcs='\n'.join(function(trace,s) for s in signatures)
        for s in ['static bool EiemMaterialSourceInitActive(', 'static bool EiemExposeSourceMaterialsForInit(']:
            funcs+='\n'+function(trace,s)
        begin=trace.index('struct EiemMaterialSourceInitScope {')
        funcs+='\n'+trace[begin:trace.index('\n};',begin)+3]
        funcs+='\n'+function(trace,'static bool EiemReapplyRendererMaterialsAfterCommit(')
        fixture=fixture.replace('// STATE',state).replace('// FUNCTIONS',funcs)
        init='TraceMaterialInfoInit'
        hooks='\n'.join(function(trace,s) for s in [f'static void {init}(', 'static bool TraceRendererInfoTrySetSharedMaterials('])
        cls.temp=tempfile.TemporaryDirectory(prefix='eiem-material-baseline-')
        cls.addClassCleanup(cls.temp.cleanup)
        folder=Path(cls.temp.name)
        source=folder/'test.cpp'; cls.exe=folder/'test.exe'
        source.write_text(fixture+CONTROLLER+hooks+MAIN.replace('INIT_HOOK',init),encoding='utf-8')
        build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(source),f'/Fe{cls.exe}'],cwd=folder,capture_output=True,encoding='utf-8',errors='replace')
        if build.returncode: raise AssertionError(build.stdout+build.stderr)

    def run_case(self,scenario):
        result=subprocess.run([str(self.exe),scenario],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_controller_initialized_after_mod(self): self.run_case('late_init')
    def test_controller_initialized_before_mod(self): self.run_case('early_init')
    def test_controller_reinitialized_while_mod_active(self): self.run_case('repeated_init')
    def test_commit_inside_init_cannot_capture_mod(self): self.run_case('nested_commit')
    def test_source_table_excludes_mod_added_slots(self): self.run_case('added_slot')
    def test_failed_exposure_preserves_baseline_and_calls_game_init(self): self.run_case('failed_exposure')
