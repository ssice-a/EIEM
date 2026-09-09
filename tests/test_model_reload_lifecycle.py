"""Actual registration + complete F10 reconcile, with a simulated Unity sink."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import PREFIX, function

ROOT=Path(__file__).resolve().parents[1]
EXTRA=r'''
#include "eiem_mod_update.h"
static EiemModUpdateQueue s_eiemModUpdates;
static bool g_shutdownRequested=false;
static DWORD s_eiemUnityThreadId=0;
static void *g_gameObject_GetComponentsInChildren=(void *)1;
static SRWLOCK s_eiemInputLock=SRWLOCK_INIT;
static std::vector<EiemModInputEvent> s_eiemPendingInputs;
static void EiemProbeCheckpoint(const char *,bool=false) {}
static void EiemReportCameraFade() {}
static void EiemReapplyShapeControls(const std::vector<std::string> &) {}
static void EiemDestroyPartnerObjects(const std::vector<std::string> *) {}
static void EiemCollectSkeletonInstances() {}
static void EiemRestoreRenderOverrides(const std::vector<std::string> *) { appliedModels.clear(); }
'''
SINK=r'''
static std::vector<void *> appliedModels;
static bool EiemApplyStandaloneRenderRules(void *model,const char *,bool *matched=nullptr,
                                         const std::vector<std::string> * = nullptr,
                                         std::vector<EiemPhysicsIntent> * = nullptr) {
  std::vector<EiemModRule> rules; EiemFindStandaloneRenderRules(&rules);
  const bool hit=simulateMatch && !rules.empty();
  if(matched) *matched=hit;
  if(hit && rules[0].hasMesh) { appliedModels.push_back(model); return true; }
  return false;
}
'''
MAIN=r'''
static void WriteMod(bool enabled) {
  CreateDirectoryA("plugin",nullptr); CreateDirectoryA("plugin/mods",nullptr); CreateDirectoryA("plugin/mods/test",nullptr);
  std::ofstream file("plugin/mods/test/mod.ini");
  if(enabled) file << "[MeshBody]\npath=meshes/body.mesh\n[RenderBody]\nasset=Body\nmesh=MeshBody\n";
  else file << "; disabled\n";
}
static void Reload() { s_eiemModUpdates.Request(EiemModUpdate::Reload); EiemRunModReconcile(); }
int main() {
  WriteMod(false); EiemReloadMods();
  EiemRegisterAndApplyModelInstance(EiemModelOwnerKind::BaseModelPart,(void *)1,(void *)11,nullptr,1,"first-arrival");
  EiemRegisterAndApplyModelInstance(EiemModelOwnerKind::BaseModelPart,(void *)2,(void *)12,nullptr,2,"second-arrival");
  CHECK(s_eiemModelInstances.size()==2 && appliedModels.empty());
  for(int i=0;i<3;++i) {
    WriteMod(true); Reload();
    CHECK(appliedModels==std::vector<void *>({(void *)11,(void *)12}));
    WriteMod(false); Reload();
    CHECK(appliedModels.empty() && s_eiemModelInstances.size()==2);
  }
  // Recycled owner, while rules are completely absent: release previous model too.
  EiemRegisterAndApplyModelInstance(EiemModelOwnerKind::BaseModelPart,(void *)1,(void *)13,nullptr,3,"owner-reuse");
  CHECK(s_eiemModelInstances.size()==2);
  WriteMod(true); Reload();
  CHECK(appliedModels==std::vector<void *>({(void *)12,(void *)13}));
  // A disappeared native model must be pruned even if no release hook arrived.
  objectStatus[(void *)12]=0;
  Reload(); CHECK(appliedModels==std::vector<void *>({(void *)13}));
  CHECK(s_eiemModelInstances.size()==1);
  // Unknown validity is not treated as alive or silently discarded.
  objectStatus[(void *)13]=-1;
  Reload(); CHECK(appliedModels.empty() && s_eiemModelInstances.size()==1);
  objectStatus[(void *)13]=1;
  Reload(); CHECK(appliedModels==std::vector<void *>({(void *)13}));
  // A new wrapper at the same address must not inherit the former owner.
  ++objectSerials[(void *)13];
  EiemRegisterAndApplyModelInstance(EiemModelOwnerKind::BaseModelPart,(void *)3,(void *)13,nullptr,99,"address-reuse");
  CHECK(s_eiemModelInstances.size()==1 && s_eiemModelInstances[0].ownerCount==1);
  CHECK(s_eiemModelInstances[0].owners[0].owner==(void *)3);
  EiemForgetModelOwner(EiemModelOwnerKind::BaseModelPart,(void *)1,"owner-release");
  CHECK(s_eiemModelInstances.size()==1);
  EiemForgetModelOwner(EiemModelOwnerKind::BaseModelPart,(void *)3,"owner-release");
  CHECK(s_eiemModelInstances.empty());
  return 0;
}
'''

class ModelReloadLifecycle(unittest.TestCase):
    def test_empty_start_multimodel_reload_reuse_expiry_and_release(self):
        if not shutil.which('cl'): self.skipTest('Requires MSVC')
        trace=(ROOT/'src/il2cpp_trace.h').read_text(encoding='utf-8')
        start=PREFIX.index('static bool EiemApplyStandaloneRenderRules(')
        end=PREFIX.index('static void EiemStoreModelPhysicsIntents(',start)
        prefix=PREFIX[:start]+SINK+PREFIX[end:]
        prefix=prefix.replace('struct EiemUnityRef {', '#include <map>\nstatic std::map<void *,int> objectStatus,objectSerials;\nstruct EiemUnityRef {')
        prefix=prefix.replace('void *p=nullptr;', 'void *p=nullptr; int serial=0;')
        prefix=prefix.replace('return {p};', 'return {p,objectSerials[p]};')
        prefix=prefix.replace('return p;', 'return p && serial==objectSerials[p]?p:nullptr;')
        prefix=prefix.replace('return p?1:0;', 'return !Target()?0:objectStatus.count(p)?objectStatus[p]:1;')
        registration='static bool EiemRegisterAndApplyModelInstance('
        code=function(trace[trace.rindex(registration):],registration)
        for sig in ('static void EiemForgetModelOwner(', 'static void EiemForgetModelInstance(',
                    'static void EiemPruneModelInstances(', 'static void EiemRunModReconcile()'):
            code+='\n'+function(trace[trace.rindex(sig):],sig)
        with tempfile.TemporaryDirectory(prefix='eiem-model-reload-') as tmp:
            folder=Path(tmp); source=folder/'test.cpp'; exe=folder/'test.exe'
            source.write_text(prefix+EXTRA+code+MAIN,encoding='utf-8')
            build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(source),f'/Fe{exe}'],cwd=folder,capture_output=True,text=True,errors='replace')
            self.assertEqual(build.returncode,0,build.stdout+build.stderr)
            result=subprocess.run([str(exe)],cwd=folder,capture_output=True,text=True,errors='replace')
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
