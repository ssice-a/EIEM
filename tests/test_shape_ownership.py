"""Neutral channels through real MinHook and the production IL2CPP shape adapter."""
from pathlib import Path
from runtime_source import read_runtime_source
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import function

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#include <windows.h>
#include "MinHook.h"
#include <map>
#include <cstdio>
#include <cstring>
#include <thread>
#include <stdexcept>
#include "eiem_shape_state.h"
struct EiemSkeletonInstance;
static void Log(const char*,...) {}
static DWORD mainThread=GetCurrentThreadId();
static bool EiemOnUnityThread(){return mainThread==GetCurrentThreadId();}
static std::map<void*,int> serials;
struct EiemUnityRef {
  void *p=nullptr;int serial=0;
  static EiemUnityRef Capture(void *p,bool=true){return {p,serials[p]};}
  void *Target()const{return p && serial==serials[p]?p:nullptr;}
  explicit operator bool()const{return p!=nullptr;}
};
struct Mesh {std::vector<std::string> names;};
struct Renderer {Mesh *mesh;std::vector<float> weights;};
static bool throwSetter=false,throwCore=false,missingGet=false;
static int setters=0,cores=0;
static __declspec(noinline) float __fastcall NativeGet(void *p,int index){return ((Renderer*)p)->weights.at(index);}
static __declspec(noinline) void __fastcall NativeSet(void *p,int index,float value){
  if(throwSetter)throw std::runtime_error("setter");++setters;((Renderer*)p)->weights.at(index)=value;
}
static __declspec(noinline) void *__fastcall NativeMesh(void *p){return ((Renderer*)p)->mesh;}
struct Core {Renderer *r;std::vector<float> values;Core *nested=nullptr;};
static __declspec(noinline) void __fastcall NativeApply(void *p,void *method){
  if(method!=(void*)123)throw std::runtime_error("changed argument");++cores;
  auto &c=*(Core*)p;if(c.nested)NativeApply(c.nested,method);
  if(throwCore)throw std::runtime_error("core");
  for(int i=0;i<(int)c.values.size();++i)
    if(i<(int)c.r->mesh->names.size() && std::abs(NativeGet(c.r,i)-c.values[i])>.01f)NativeSet(c.r,i,c.values[i]);
}
struct MInfo {void *mp;};
static void *Resolve(const char *name){
  if(!strcmp(name,"UnityEngine.SkinnedMeshRenderer::GetBlendShapeWeight(System.Int32)"))return missingGet?nullptr:(void*)NativeGet;
  if(!strcmp(name,"UnityEngine.SkinnedMeshRenderer::SetBlendShapeWeight(System.Int32,System.Single)"))return (void*)NativeSet;
  if(!strcmp(name,"UnityEngine.SkinnedMeshRenderer::get_sharedMesh()"))return (void*)NativeMesh;
  throw std::runtime_error("unexpected icall");
}
static auto il2cpp_resolve_icall=Resolve;
static void *FindMethodWithParamTypesAndReturnType(void *klass,const char *name,const char *const *types,int count,const char *ret){
  if(klass!=(void*)1 || strcmp(name,"_ApplyShaderPropDataToRenderers") || types || count || strcmp(ret,"System.Void"))throw std::runtime_error("method signature");
  static MInfo method{(void*)NativeApply};return &method;
}
#include "eiem_shape_guard.h"
static void *EiemReadSharedMesh(void *r,const char*){return NativeMesh(r);}
static void *g_mesh_get_blendShapeCount=(void*)1,*g_mesh_GetBlendShapeName=(void*)2;
static void *g_smr_GetBlendShapeWeight=(void*)3,*g_smr_SetBlendShapeWeight=(void*)4;
static void ReadStrUtf8(void *p,char *dst,size_t n){strncpy_s(dst,n,(const char*)p,_TRUNCATE);}
static bool InvokeChecked(void *method,void *p,void **args,void **out){
  alignas(16) static char box[32];
  if(method==(void*)1){*(int*)(box+16)=(int)((Mesh*)p)->names.size();*out=box;return true;}
  int index=*(int*)args[0];
  if(method==(void*)2){*out=(void*)((Mesh*)p)->names.at(index).c_str();return true;}
  if(method==(void*)3){*(float*)(box+16)=NativeGet(p,index);*out=box;return true;}
  NativeSet(p,index,*(float*)args[1]);*out=nullptr;return true;
}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
'''
MAIN = r'''
int main(int argc,char**argv){
  Mesh empty{{}},added{{"Inflate","Sleeve"}},source{{"Blink","Smile"}},reordered{{"Inflate","Smile","Blink"}};
  Renderer r{&added,{0,0}},other{&source,{0,0}};Core core{&r,{60,40}};
  // Reproduce the actual failure BEFORE installing the correction: count-only
  // game logic treats the new indices as its own. No UI or shape rule exists.
  NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({60,40}));r.weights={0,0};
  CHECK(MH_Initialize()==MH_OK);missingGet=argc>1;
  if(missingGet){CHECK(!EiemInitShapeGuard((void*)1));EiemShapeState state;EiemUnityShapes backend;std::string error;
    CHECK(!EiemPrepareShapeBinding(state,&r,&added,{},backend,error) && !state.binding);
    CHECK(MH_Uninitialize()==MH_OK);return 0;}
  CHECK(EiemInitShapeGuard((void*)1));EiemUnityShapes backend;std::string error;EiemShapeState state;
  CHECK(EiemPrepareShapeBinding(state,&r,&added,{},backend,error));
  CHECK(EiemInitializeBoundShapes(state,&r,backend,error));
  NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({0,0}) && state.binding->suppressed==2);
  EiemModRule inflate{};CHECK(EiemSetRenderField(inflate,"shape.Inflate",".25",error));
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",inflate,state);
  NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({25,0}));
  CHECK(EiemInitializeBoundShapes(state,&r,backend,error) && r.weights[0]==25); // no repeated zeroing
  EiemRetireShapeBinding(state.binding);state={};
  r.mesh=&reordered;r.weights={0,20,10};
  std::vector<EiemShapeBaseline> baseline{{"Blink",10},{"Smile",20}};
  CHECK(EiemPrepareShapeBinding(state,&r,&reordered,baseline,backend,error));
  CHECK(EiemInitializeBoundShapes(state,&r,backend,error));
  core.values={60,40,99};NativeApply(&core,(void*)123);
  CHECK(r.weights==std::vector<float>({0,40,60}));
  {EiemShapeGameScope game;CHECK(NativeGet(&r,0)==60 && NativeGet(&r,1)==40 && NativeGet(&r,2)==0);
   EiemShapeAuthorScope author;CHECK(NativeGet(&r,0)==0 && NativeGet(&r,2)==60);}
  EiemModRule smile{};CHECK(EiemSetRenderField(smile,"shape.Smile",".75",error));
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",smile,state);CHECK(r.weights[1]==75);
  core.values={30,90};NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({0,75,30}));
  EiemModRule none{};EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",none,state);
  CHECK(r.weights[1]==90 && state.owned.empty());
  auto latest=EiemShapeSourceWeights(state,baseline);CHECK(latest[0].value==30 && latest[1].value==90);
  // Removing/renaming native channels is rejected, not mapped to slot zero.
  Mesh missing{{"Inflate","Blink"}};auto before=state.binding;
  CHECK(!EiemPrepareShapeBinding(state,&r,&missing,baseline,backend,error) && state.binding==before);
  Mesh duplicate{{"Blink","Blink","Smile"}};
  CHECK(!EiemPrepareShapeBinding(state,&r,&duplicate,baseline,backend,error));
  // Same shared Mesh, independent consumers/values and nested core scopes.
  Renderer partner{&reordered,{0,2,1}};EiemShapeState partnerState;
  CHECK(EiemPrepareShapeBinding(partnerState,&partner,&reordered,{{"Blink",1},{"Smile",2}},backend,error));
  Core nested{&partner,{5,6}};core.nested=&nested;NativeApply(&core,(void*)123);
  CHECK(partner.weights==std::vector<float>({0,6,5}) && r.weights==std::vector<float>({0,90,30}));
  CHECK(s_eiemShapeGameDepth==0);core.nested=nullptr;
  throwCore=true;bool threw=false;try{NativeApply(&core,(void*)123);}catch(...){threw=true;}
  CHECK(threw && s_eiemShapeGameDepth==0);throwCore=false;
  throwSetter=true;core.values={15};threw=false;try{NativeApply(&core,(void*)123);}catch(...){threw=true;}
  CHECK(threw && s_eiemShapeGameDepth==0);throwSetter=false;
  // Actual current Mesh check prevents a stale mapping after source restoration.
  r.mesh=&source;r.weights={30,90};core.values={7,8};NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({7,8}));
  EiemRetireShapeBinding(state.binding);state={};CHECK(s_eiemShapeBindings.count(&r)==0);
  // An expired/recycled managed identity must not receive the old mapping.
  serials[&partner]++;Core recycled{&partner,{12,13}};NativeApply(&recycled,(void*)123);
  CHECK(partner.weights[0]==12 && partner.weights[1]==13);
  EiemRetireShapeBinding(partnerState.binding);partnerState={};CHECK(s_eiemShapeBindings.empty());
  // F10-like new binding starts a fresh native view; no stale values inherited.
  r.mesh=&reordered;r.weights={0,8,7};
  CHECK(EiemPrepareShapeBinding(state,&r,&reordered,{{"Blink",7},{"Smile",8}},backend,error));
  CHECK(state.binding->channels.gameValues==std::vector<float>({7,8}));
  // Known protected targets on an unsupported thread are reported, not written.
  std::thread worker([&]{NativeApply(&core,(void*)123);});worker.join();
  CHECK(s_eiemShapeGuardThreadError && r.weights==std::vector<float>({0,8,7}));
  EiemRetireShapeBinding(state.binding);state={};
  // Shape-only rules protect native channels too, with no Mesh assignment.
  r.mesh=&source;r.weights={21,22};core.values={51,52};
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",smile,state);
  CHECK(state.binding && state.binding->initialized && r.weights[1]==75);
  NativeApply(&core,(void*)123);CHECK(r.weights==std::vector<float>({51,75}));
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",none,state);
  CHECK(r.weights==std::vector<float>({51,52}));
  // A shape-only consumer can be reused with a different game Mesh.
  Mesh newSource{{"Smile","Blink"}};r.mesh=&newSource;r.weights={32,33};
  auto oldBinding=state.binding;
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",smile,state);
  CHECK(!oldBinding->active && state.binding!=oldBinding && r.weights[0]==75);
  core.values={61,62};NativeApply(&core,(void*)123);
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",none,state);
  CHECK(r.weights==std::vector<float>({61,62}));
  EiemRetireShapeBinding(state.binding);state={};
  CHECK(MH_Uninitialize()==MH_OK);return 0;
}
'''


class ShapeOwnershipTests(unittest.TestCase):
    def test_game_mesh_change_rebases_actual_consumer_state(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        trace = read_runtime_source(ROOT)
        adapter = trace[trace.index("struct EiemUnityShapes"):trace.index("struct EiemRenderOverrideState")]
        state_start = trace.index("struct EiemRenderOverrideState {")
        state = trace[state_start:trace.index("\n};", state_start) + 3]
        code = PREFIX + adapter + state + r'''
static SRWLOCK s_eiemOverrideLock=SRWLOCK_INIT;
static std::vector<EiemRenderOverrideState> s_eiemOverrides;
''' + "\n".join(function(trace, sig) for sig in (
            "static size_t EiemFindOverrideLocked(", "static void EiemPrepareRenderInput(")) + r'''
int main() {
  CHECK(MH_Initialize()==MH_OK && EiemInitShapeGuard((void*)1));
  Mesh oldSource{{}},oldMod{{"Inflate"}},newSource{{"Blink"}},newMod{{"Inflate","Blink"}};
  Renderer r{&oldMod,{25}};EiemUnityShapes backend;std::string error;
  EiemRenderOverrideState state;state.renderer=&r;state.rendererRef=EiemUnityRef::Capture(&r);
  state.originalMesh=&oldSource;state.replacementMesh=&oldMod;state.hasSourceShapeWeights=true;
  CHECK(EiemPrepareShapeBinding(state.shapes,&r,&oldMod,{},backend,error));
  state.shapes.owned={{"Inflate",0}};auto oldBinding=state.shapes.binding;s_eiemOverrides.push_back(state);
  r.mesh=&newSource;r.weights={17};void *identity=nullptr;
  EiemPrepareRenderInput(&r,&newSource,"SkinnedMeshRenderer",&identity);
  auto &live=s_eiemOverrides[0];
  CHECK(identity==&newSource && live.originalMesh==&newSource && !live.replacementMesh);
  CHECK(!oldBinding->active && !live.shapes.binding && live.shapes.owned.empty());
  CHECK(live.sourceShapeWeights.size()==1 && live.sourceShapeWeights[0].name=="Blink" && live.sourceShapeWeights[0].value==17);
  CHECK(EiemPrepareShapeBinding(live.shapes,&r,&newMod,live.sourceShapeWeights,backend,error));
  r.mesh=&newMod;r.weights={99,0};CHECK(EiemInitializeBoundShapes(live.shapes,&r,backend,error));
  CHECK(r.weights==std::vector<float>({0,17}));Core core{&r,{63}};NativeApply(&core,(void*)123);
  CHECK(r.weights==std::vector<float>({0,63}));
  live.replacementMesh=&newMod;live.restorePending=true;
  EiemPrepareRenderInput(&r,&oldSource,"SkinnedMeshRenderer",&identity);
  CHECK(identity==&newSource && live.originalMesh==&newSource && live.shapes.binding->active);
  EiemRetireShapeBinding(live.shapes.binding);s_eiemOverrides.clear();CHECK(s_eiemShapeBindings.empty());
  CHECK(MH_Uninitialize()==MH_OK);return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix="eiem-shape-rebase-") as temp:
            folder = Path(temp)
            source, exe = folder / "test.cpp", folder / "test.exe"
            source.write_text(code, encoding="utf-8")
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8", "/O2",
                f"/I{ROOT / 'src'}", f"/I{ROOT / 'deps/minhook_lib/include'}", str(source), f"/Fe{exe}",
                str(ROOT / "deps/minhook_lib/lib/libMinHook.x64.lib"), "user32.lib"], cwd=folder,
                capture_output=True, text=True, encoding="utf-8", errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=20)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)

    def test_native_call_chain_and_real_adapter(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        trace = read_runtime_source(ROOT)
        adapter = trace[trace.index("struct EiemUnityShapes"):trace.index("struct EiemRenderOverrideState")]
        with tempfile.TemporaryDirectory(prefix="eiem-shape-ownership-") as temp:
            folder = Path(temp)
            source = folder / "test.cpp"
            source.write_text(PREFIX + adapter + MAIN, encoding="utf-8")
            for opt in ("/Od", "/O2"):
                exe = folder / "test.exe"
                build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8", opt,
                    f"/I{ROOT / 'src'}", f"/I{ROOT / 'deps/minhook_lib/include'}", str(source), f"/Fe{exe}",
                    str(ROOT / "deps/minhook_lib/lib/libMinHook.x64.lib"), "user32.lib"], cwd=folder,
                    capture_output=True, text=True, encoding="utf-8", errors="replace")
                self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
                for args in ([], ["missing-get"]):
                    with self.subTest(optimization=opt, args=args):
                        run = subprocess.run([str(exe)] + args, cwd=folder, capture_output=True, text=True, timeout=20)
                        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
