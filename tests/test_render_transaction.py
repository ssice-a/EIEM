"""Fault injection through the complete production Render executor."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_restore_lifecycle import FIXTURE
from test_mod_controls import function
from runtime_source import read_runtime_source
ROOT=Path(__file__).resolve().parents[1]
EXTRA=r'''
static bool failShapeInit=false,failMaterials=false,failBones=false,failEnable=false;
static bool EiemCaptureOriginal(void *renderer,void *,void *mesh,const char *,bool meshEdit=false,const EiemModRule *rule=nullptr,bool=false) {
  auto &s=s_eiemOverrides[0];
  if(s.restorePending)return false;
  if(meshEdit) {s.sourceMeshRef=EiemUnityRef::Capture(mesh,false);s.hasSkinning=true;s.originalBonesHandle=43;}
  if(rule && rule->materialCount) {s.hasMaterials=true;s.originalMaterialsHandle=42;s.materialSlots={0};}
  return true;
}
static bool EiemAcquireSkeleton(const EiemModRule &,void *,std::shared_ptr<EiemSkeletonInstance> &,char *,size_t) {return true;}
static bool EiemMaterialSourceInitActive(void *) {return false;}
static bool EiemBuildRendererMaterialsForSource(const EiemModRule &,void *,void **out,char *,size_t) {*out=&writtenMaterials;return true;}
static bool EiemAssignRendererMaterials(void *,void *p,char *,size_t) {currentMaterials=*(Array *)p;bool ok=!failMaterials;failMaterials=false;return ok;}
static void EiemRememberRuleBinding(void *,const EiemModRule &) {}
static void TraceDescribeObject(void *,char *,size_t) {}
static bool EiemReadRendererVisible(void *,bool *) {return false;}
static int EiemReadRendererMaterialCount(void *) {return 1;}
struct EiemResolvedRenderRule {EiemModRule rule{};char source[768]{},asset[192]{};};
'''
MAIN=r'''
int main(int argc,char **argv) {
 CHECK(argc==2);std::string mode=argv[1];
 EiemRenderOverrideState state{};
 state.renderer=state.drawRenderer=&tokens[0];state.originalMesh=&tokens[1];
 state.rendererRef=state.drawRendererRef=EiemUnityRef::Capture(&tokens[0]);
 strcpy_s(state.rendererType,"SkinnedMeshRenderer");s_eiemOverrides.push_back(state);
 g_renderer_set_enabled=(void *)4;g_smr_get_bones=(void *)5;g_smr_set_bones=(void *)6;
 g_renderer_get_sharedMaterials=(void *)1;s_eiemRendererSetSharedMaterials=(void *)2;s_eiemMaterialClass=(void *)3;
 originalBones.count=currentBones.count=1;originalBones.items[0]=currentBones.items[0]=&tokens[3];
 replacementBones.count=2;replacementBones.items[0]=&tokens[3];replacementBones.items[1]=&tokens[2];
 originalMaterials.count=currentMaterials.count=writtenMaterials.count=1;
 originalMaterials.items[0]=currentMaterials.items[0]=&tokens[3];writtenMaterials.items[0]=&tokens[2];
 liveEnabled=true;
 EiemResolvedRenderRule resolved;resolved.rule.hasMesh=true;strcpy_s(resolved.rule.mesh,"MeshTest");
 resolved.rule.materialCount=1;resolved.rule.materialSlots[0]=0;
 if(mode=="mesh")failAssignment=true;
 if(mode=="bones")failBones=true;
 if(mode=="shape")failShapeInit=true;
 if(mode=="material" || mode=="rollback")failMaterials=true;
 if(mode=="enable")failEnable=true;
 if(mode=="rollback")failRestore=true;
 if(mode=="owner") {
   CHECK(s_eiemOverrides[0].ownerPrefabInstance==0);
   EiemClaimRendererOverrideOwner(&tokens[0],77);
   CHECK(s_eiemOverrides[0].ownerPrefabInstance==77);
   EiemClaimRendererOverrideOwner(&tokens[0],88);
   CHECK(s_eiemOverrides[0].ownerPrefabInstance==77);
   EiemForgetRenderOverrides(77);
   CHECK(s_eiemOverrides.empty());return 0;
 }
 if(mode=="skip" || mode=="skiponly")strcpy_s(resolved.rule.handling,"skip");
 if(mode=="skiponly") {resolved.rule.hasMesh=false;resolved.rule.materialCount=0;}
 const bool result=EiemApplyResolvedRenderRule(&tokens[0],&tokens[0],&tokens[1],"SkinnedMeshRenderer",nullptr,resolved);
 if(mode=="skiponly") {
   CHECK(result && liveMesh==&tokens[1] && !liveEnabled);
   CHECK(EiemRestoreRenderOverrides() && liveEnabled);
 } else if(mode=="skip" || mode=="success") {
   CHECK(result && liveMesh==&tokens[2]);CHECK(liveEnabled==(mode=="success"));
 } else {
   CHECK(!result);
   if(mode=="rollback") {
     CHECK(s_eiemOverrides.size()==1 && s_eiemOverrides[0].restorePending);
     CHECK(!liveEnabled && releasedHandles.empty());
     failRestore=false;CHECK(EiemRestoreRenderOverrides());
   }
   CHECK(liveMesh==&tokens[1] && liveEnabled);
   CHECK(EiemManagedObjectArraySame(&originalBones,&currentBones));
   CHECK(EiemManagedObjectArraySame(&originalMaterials,&currentMaterials));
   CHECK(s_eiemOverrides.empty());
 }
 return 0;
}
'''
class RenderTransaction(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  if not shutil.which('cl'):raise unittest.SkipTest('Requires MSVC')
  trace=read_runtime_source(ROOT)
  start=trace.index('struct EiemRenderOverrideState {');state=trace[start:trace.index('\n};',start)+3]
  fixture=FIXTURE.split('static void Apply()')[0]
  fixture=fixture.replace('static bool EiemSetRendererEnabled(void *,bool) { return true; }','')
  fixture=fixture.replace('static bool EiemInitializeRendererShapeBinding(void *,const char *,char *,size_t) {return true;}','static bool EiemInitializeRendererShapeBinding(void *,const char *,char *,size_t) {bool ok=!failShapeInit;failShapeInit=false;return ok;}')
  fixture=fixture.replace('static bool EiemPreserveSourceSkinning(void *,void *,char *,size_t) { return true; }','static bool EiemPreserveSourceSkinning(void *,void *,char *,size_t) {currentBones=replacementBones;bool ok=!failBones;failBones=false;return ok;}')
  # Place fault switches / palettes before mocks that use them.
  fixture=fixture.replace('static bool failShapeRestore=false;', 'static bool failShapeInit=false,failMaterials=false,failBones=false,failEnable=false;\nstruct Array { char header[32]{}; void *items[16]{}; size_t count=0; };\nstatic Array originalMaterials,currentMaterials,writtenMaterials;\nstatic Array originalBones,replacementBones,currentBones;\nstatic bool failShapeRestore=false;')
  for declaration in ['struct Array { char header[32]{}; void *items[16]{}; size_t count=0; };','static Array originalMaterials,currentMaterials,writtenMaterials;','static Array originalBones,replacementBones,currentBones;']:
   pos=fixture.rindex(declaration);fixture=fixture[:pos]+fixture[pos:].replace(declaration,'',1)
  fixture=fixture.replace('*out=&tokens[2]; return true;', '*out=&tokens[2];*skin=std::make_shared<EiemSkinIdentity>();return true;').replace('std::shared_ptr<const EiemSkinIdentity> *) {', 'std::shared_ptr<const EiemSkinIdentity> *skin) {')
  fixture=fixture.replace('static bool EiemResolveMeshBonesFromAssembly(const EiemSkinIdentity &,void *,void **,char *,size_t) { return true; }','static bool EiemResolveMeshBonesFromAssembly(const EiemSkinIdentity &,void *,void **out,char *,size_t) {*out=&replacementBones;return true;}')
  fixture=fixture.replace('if(method && method==g_renderer_set_enabled && !failEnabledRestore) liveEnabled=*(bool *)p[0];','if(method && method==g_renderer_set_enabled && !failEnabledRestore) {bool enabled=*(bool *)p[0];if(enabled && failEnable)failEnable=false;else liveEnabled=enabled;}')
  funcs='\n'.join(function(trace,s) for s in ['static size_t EiemFindOverrideLocked(', 'static void EiemClaimRendererOverrideOwner(', 'static void EiemBeginMeshWrite(', 'static bool EiemRememberReplacement(', 'static void EiemReleaseOverrideHandles(', 'static bool EiemCaptureEnabledForSkip(', 'static bool EiemSetRendererEnabled(', 'static bool EiemRestoreRenderOverrides(', 'static void EiemForgetRenderOverrides('])
  for sig in ['static bool EiemCaptureCommitEnabled(', 'static void EiemFinishRenderCommit(']:
   if sig in trace:funcs+='\n'+function(trace,sig)
  cls.temp=tempfile.TemporaryDirectory(prefix='eiem-transaction-');cls.addClassCleanup(cls.temp.cleanup)
  folder=Path(cls.temp.name);source=folder/'test.cpp';cls.exe=folder/'test.exe'
  source.write_text(fixture.replace('// STATE',state).replace('// FUNCTIONS',funcs)+EXTRA.replace('static bool failShapeInit=false,failMaterials=false,failBones=false,failEnable=false;','')+function(trace,'static bool EiemApplyResolvedRenderRule(')+MAIN,encoding='utf-8')
  build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(source),f'/Fe{cls.exe}'],cwd=folder,capture_output=True,text=True,encoding='utf-8',errors='replace')
  if build.returncode:raise AssertionError(build.stdout+build.stderr)
 def run_case(self,mode):
  r=subprocess.run([str(self.exe),mode],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stdout+r.stderr)
 def test_mesh_failure(self):self.run_case('mesh')
 def test_bones_failure(self):self.run_case('bones')
 def test_shape_failure(self):self.run_case('shape')
 def test_material_failure(self):self.run_case('material')
 def test_enable_failure(self):self.run_case('enable')
 def test_rollback_failure_stays_disabled_and_retryable(self):self.run_case('rollback')
 def test_success(self):self.run_case('success')
 def test_skip(self):self.run_case('skip')
 def test_skip_without_mesh(self):self.run_case('skiponly')
 def test_direct_renderer_owner_is_claimed_and_released(self):self.run_case('owner')
