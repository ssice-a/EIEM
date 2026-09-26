"""Real write block and restore implementation, with controlled Unity failures."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import function

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = r'''
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <memory>
#include <map>
#include "eiem_mod_document.h"
#include "eiem_shape_state.h"
#include "eiem_render_state.h"
#include "eiem_skin_binding.h"
struct EiemSkeletonInstance {};
static bool EiemSkeletonMeshBones(const EiemSkinIdentity &,const EiemSkeletonInstance &,void **,char *,size_t) {return true;}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
static bool alive=true, sourceAlive=true;
static int tokens[4];
struct EiemUnityRef {
  void *p=nullptr;
  static EiemUnityRef Capture(void *p,bool=true) { return {p}; }
  explicit operator bool() const { return p!=nullptr; }
  void *Target() const { return p; }
  int Status() const { return p==&tokens[1] ? (sourceAlive?1:0) : (alive?1:0); }
};
static std::vector<void *> retainedSkeletonConsumers;
static void EiemRetainSkeletonConsumer(EiemSkeletonInstance &,const EiemUnityRef &reference) {
  retainedSkeletonConsumers.push_back(reference.Target());
}
static int EiemNativeObjectStatus(void *p) { return p ? (p==&tokens[1] ? (sourceAlive?1:0) : (alive?1:0)) : 0; }
static bool failShapeRestore=false;
static float liveShapeWeight=0;
struct EiemUnityShapes {
  int Index(void *,const std::string &name) { return name=="Blink"?0:-1; }
  bool Write(void *,int,float value) { if(!failShapeRestore)liveShapeWeight=value;return true; }
  bool Read(void *,int,float &value) {value=liveShapeWeight;return true;}
};
// STATE
static SRWLOCK s_eiemOverrideLock=SRWLOCK_INIT;
static std::vector<EiemRenderOverrideState> s_eiemOverrides;
static void Log(const char *,...) {}
static bool EiemModAffected(const char *,const std::vector<std::string> *) { return true; }
static void EiemProbeTrackedRenderer(const char *,void *) {}
static bool EiemUpdateRendererShapes(void *,const char *,const EiemModRule &,EiemShapeState &) {return true;}
static void *liveMesh=&tokens[1];
static int writes=0;
static bool failAssignment=false, failRestore=false;
static bool failMaterialRestore=false, failEnabledRestore=false, liveEnabled=false;
static bool failBonesRestore=false;
struct Array { char header[32]{}; void *items[16]{}; size_t count=0; };
static Array originalMaterials,currentMaterials,writtenMaterials;
static Array originalBones,replacementBones,currentBones;
static std::vector<uint32_t> releasedHandles;
static void *EiemReadSharedMesh(void *,const char *) { if(!alive) abort(); return liveMesh; }
static bool EiemSetSharedMesh(void *,void *mesh,const char *,void *) {
  if(!alive) abort(); ++writes;
  if(mesh==&tokens[2] && failAssignment) { liveMesh=nullptr; return false; }
  if(mesh==&tokens[1] && failRestore) return false;
  liveMesh=mesh; return true;
}
static bool EiemBuildMeshResource(const EiemModRule &,void **out,char *,size_t,void *,std::shared_ptr<const EiemSkinIdentity> *) { *out=&tokens[2]; return true; }
static bool EiemResolveMeshBones(const EiemSkinIdentity &,void *,void **,char *,size_t) { return true; }
static bool EiemPreserveSourceSkinning(void *,void *,char *,size_t) { return true; }
static bool EiemResolveMeshBonesFromNativeInstance(const EiemSkinIdentity &,void *,void **,char *,size_t) { return true; }
static bool EiemResolveMeshBonesFromAssembly(const EiemSkinIdentity &,void *,void **,char *,size_t) { return true; }
static unsigned long long EiemSkinTimelineBoneRefs(void *) { return 0; }
static long long EiemPerfNow() { return 0; }
static double EiemPerfMilliseconds(long long) { return 0; }
static void EiemReportNativeMeshDeserializeSource(void *,const char *,const char *,const char *) {}
static void EiemLogNativeMeshFlagState(const char *,void *,void *) {}
static uintptr_t s_eiemActivePrefabInstance=0;
static constexpr bool kEiemEnableNativeMeshFlagProbe=false;
static constexpr bool kEiemEnableLifecycleDiagnostics=false;
static constexpr bool kEiemEnableSkinBindingDiagnostics=false;
static bool EiemSetRendererEnabled(void *,bool) { return true; }
struct EiemBounds { float value[6]{}; };
static void *g_smr_get_localBounds=nullptr;
static bool EiemReadBounds(void *,void *,EiemBounds *) { return false; }
static void EiemSetReplacementDrawBounds(void *,const char *,void *,const EiemBounds *) {}
static bool EiemPrepareRendererShapeBinding(void *,void *,const char *,char *,size_t) {return true;}
static bool EiemInitializeRendererShapeBinding(void *,const char *,char *,size_t) {return true;}
static void EiemRetireShapeBinding(const std::shared_ptr<EiemShapeRuntimeBinding> &) {}
static auto EiemShapeSourceWeights(const EiemShapeState &,const std::vector<EiemShapeBaseline> &v) {return v;}
static void EiemPreserveSourceDrawState(void *,const char *) {}
static void *s_eiemRendererSetSharedMaterials=nullptr,*g_renderer_get_sharedMaterials=nullptr,*s_eiemMaterialClass=nullptr;
static void *g_renderer_set_enabled=nullptr,*g_smr_set_bones=nullptr,*g_smr_get_bones=nullptr;
static void *g_smr_set_rootBone=nullptr,*g_smr_get_rootBone=nullptr;
static auto il2cpp_gchandle_get_target=+[](uint32_t h)->void * {
  return h==42?&originalMaterials:h==43?&originalBones:h==44?&replacementBones:nullptr;
};
static auto il2cpp_gchandle_free=+[](uint32_t h) { releasedHandles.push_back(h); };
static void *il2cpp_array_new(void *,size_t n) { writtenMaterials.count=n; return &writtenMaterials; }
static void *Invoke(void *method,void *,void **p = nullptr) {
  if(method && method==g_renderer_get_sharedMaterials) return &currentMaterials;
  if(method && method==s_eiemRendererSetSharedMaterials && !failMaterialRestore) currentMaterials=*(Array *)p[0];
  if(method && method==g_renderer_set_enabled && !failEnabledRestore) liveEnabled=*(bool *)p[0];
  if(method && method==g_smr_get_bones) return &currentBones;
  if(method && method==g_smr_set_bones && !failBonesRestore) currentBones=*(Array *)p[0];
  return nullptr;
}
static size_t EiemManagedArrayLength(void *p) { return p?((Array *)p)->count:0; }
static bool EiemManagedObjectArraySame(void *a,void *b) {
  if(!a || !b) return a==b;
  auto x=(Array *)a,y=(Array *)b;
  return x->count==y->count && std::equal(x->items,x->items+x->count,y->items);
}
static bool EiemReadRendererEnabled(void *,bool *out) { *out=liveEnabled; return true; }
static constexpr size_t IL2CPP_ARRAY_DATA=32;
static constexpr bool kEiemEnableSkinDiagnostics=false;
static volatile LONG64 s_eiemLastSkinCommitTick=0;
// FUNCTIONS
static void Apply() {
  void *renderer=&tokens[0],*drawRenderer=renderer,*mesh=&tokens[1];
  const char *rendererType="SkinnedMeshRenderer",*source="test",*asset="Body";
  EiemModRule rule{}; rule.hasMesh=true; strcpy_s(rule.mesh,"MeshBody");
  const bool applyMesh=true;
  const bool resourcesReady=true;
  const bool skeletonReady=true;
  double meshBuildMs=0, boneResolveMs=0, meshCommitMs=0;
  std::shared_ptr<EiemSkeletonInstance> skeleton;
  // WRITE_BLOCK
}
int main(int argc,char **argv) {
  CHECK(argc==2); std::string scenario=argv[1];
  EiemRenderOverrideState state;
  state.skeleton=std::make_shared<EiemSkeletonInstance>();
  std::weak_ptr<EiemSkeletonInstance> skeletonLease=state.skeleton;
  state.renderer=state.drawRenderer=&tokens[0]; state.originalMesh=&tokens[1];
  state.ownerPrefabInstance=77;
  state.rendererRef=state.drawRendererRef=EiemUnityRef::Capture(state.renderer);
  state.sourceMeshRef=EiemUnityRef::Capture(state.originalMesh,false);
  if(scenario=="shape_failure") {
    state.hasSourceShapeWeights=true;state.sourceShapeWeights={{"Blink",31}};failShapeRestore=true;
  }
  if(scenario=="material_failure") {
    state.hasMaterials=true; state.originalMaterialsHandle=42; state.materialSlots={0};
    originalMaterials.count=currentMaterials.count=1;
    originalMaterials.items[0]=&tokens[3]; currentMaterials.items[0]=&tokens[2];
    g_renderer_get_sharedMaterials=(void *)1; s_eiemRendererSetSharedMaterials=(void *)2; s_eiemMaterialClass=(void *)3;
    failMaterialRestore=true;
  }
  if(scenario=="enabled_failure") {
    state.hasEnabled=true; state.originalEnabled=true;
    g_renderer_set_enabled=(void *)4; failEnabledRestore=true;
  }
  if(scenario=="bones_failure" || scenario=="bones_success") {
    state.hasSkinning=true; state.originalBonesHandle=43; state.replacementBonesHandle=44;
    originalBones.count=1; originalBones.items[0]=&tokens[3];
    replacementBones.count=2; replacementBones.items[0]=&tokens[3]; replacementBones.items[1]=&tokens[2];
    currentBones=replacementBones;
    g_smr_get_bones=(void *)5; g_smr_set_bones=(void *)6;
    failBonesRestore=scenario=="bones_failure";
  }
  strcpy_s(state.rendererType,"SkinnedMeshRenderer");
  s_eiemOverrides.push_back(state);
  state.skeleton.reset();
  failAssignment=scenario=="assignment_null";
  Apply();
  CHECK(s_eiemOverrides[0].ownsMesh); // responsibility exists before native write, not only after success
  if(scenario=="forget") {
    auto registryLease=s_eiemOverrides[0].skeleton;
    EiemForgetRenderOverrides(77);
    CHECK(s_eiemOverrides.empty() && retainedSkeletonConsumers==std::vector<void *>({&tokens[0]}));
    CHECK(!skeletonLease.expired());
    registryLease.reset(); CHECK(skeletonLease.expired());
    return 0;
  }
  failRestore=scenario=="restore_failure";
  if(scenario=="destroyed_renderer") alive=false;
  if(scenario=="destroyed_source") sourceAlive=false;
  const int before=writes;
  EiemRestoreRenderOverrides();
  if(scenario=="restore_failure" || scenario=="destroyed_source" || scenario=="material_failure" || scenario=="enabled_failure" || scenario=="bones_failure" || scenario=="shape_failure") {
    CHECK(s_eiemOverrides.size()==1 && s_eiemOverrides[0].restorePending);
    CHECK(s_eiemOverrides[0].originalMesh==&tokens[1]);
    CHECK(releasedHandles.empty()); // failed restore still owns its baseline and replacement references
    CHECK(!skeletonLease.expired()); // new bone nodes cannot retire while restore is incomplete
    failRestore=false; sourceAlive=true; failMaterialRestore=false; failEnabledRestore=false; failBonesRestore=false;failShapeRestore=false;
    EiemRestoreRenderOverrides();
    CHECK(liveMesh==&tokens[1] && s_eiemOverrides.empty());
    if(scenario=="material_failure") CHECK(currentMaterials.items[0]==originalMaterials.items[0]);
    if(scenario=="enabled_failure") CHECK(liveEnabled);
    if(scenario=="shape_failure") CHECK(liveShapeWeight==31);
  } else if(scenario=="destroyed_renderer") {
    CHECK(writes==before && s_eiemOverrides.empty());
  } else CHECK(liveMesh==&tokens[1] && s_eiemOverrides.empty());
  CHECK(skeletonLease.expired()); // final release only after source restoration or native death
  if(scenario=="bones_failure" || scenario=="bones_success") {
    CHECK(EiemManagedObjectArraySame(&currentBones,&originalBones));
    CHECK(releasedHandles==std::vector<uint32_t>({43,44}));
    EiemRestoreRenderOverrides();
    CHECK(releasedHandles.size()==2); // no double release on a second F10 restore
  }
  return 0;
}
'''

class RestoreLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which('cl'): raise unittest.SkipTest('Requires MSVC')
        cls.temp=tempfile.TemporaryDirectory(prefix='eiem-restore-test-')
        cls.addClassCleanup(cls.temp.cleanup)
        folder=Path(cls.temp.name)
        from runtime_source import read_runtime_source
        trace=read_runtime_source(ROOT)
        start=trace.index('struct EiemRenderOverrideState {')
        state=trace[start:trace.index('\n};',start)+3]
        signatures=['static size_t EiemFindOverrideLocked(', 'static void EiemBeginMeshWrite(', 'static bool EiemRememberReplacement(',
                    'static void EiemReleaseOverrideHandles(', 'static bool EiemRestoreRenderOverrides(',
                    'static void EiemForgetRenderOverrides(']
        funcs='\n'.join(function(trace,s) for s in signatures)
        start=trace.index('  bool meshApplied = false;', trace.index('static bool EiemApplyResolvedRenderRule('))
        end=trace.index('  // A mesh rule is a transaction boundary.',start)
        source=folder/'restore.cpp'
        source.write_text(FIXTURE.replace('// STATE',state).replace('// FUNCTIONS',funcs).replace('// WRITE_BLOCK',trace[start:end]),encoding='utf-8')
        cls.exe=folder/'restore.exe'
        build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(source),f'/Fe{cls.exe}'],cwd=folder,capture_output=True,text=True,encoding='utf-8',errors='replace')
        if build.returncode: raise AssertionError(build.stdout+build.stderr)

    def test_assignment_failure_remains_restorable(self): self.run_case('assignment_null')
    def test_failed_restore_retains_baseline_until_success(self): self.run_case('restore_failure')
    def test_success_releases_record(self): self.run_case('success')
    def test_unload_retains_native_skeleton_consumer(self): self.run_case('forget')
    def test_destroyed_renderer_is_not_called(self): self.run_case('destroyed_renderer')
    def test_destroyed_source_does_not_clear_the_renderer(self): self.run_case('destroyed_source')
    def test_failed_material_restore_retains_record(self): self.run_case('material_failure')
    def test_failed_enabled_restore_retains_record(self): self.run_case('enabled_failure')
    def test_expanded_bones_restore_and_release(self): self.run_case('bones_success')
    def test_failed_bones_restore_retains_both_arrays(self): self.run_case('bones_failure')
    def test_silent_shape_restore_failure_retains_baseline(self): self.run_case('shape_failure')
    def run_case(self,scenario):
        result=subprocess.run([str(self.exe),scenario],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
