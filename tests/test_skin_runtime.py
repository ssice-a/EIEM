"""Run the actual Unity adapter with fake nodes and managed arrays, not a new algorithm."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from test_mod_controls import function

ROOT=Path(__file__).resolve().parents[1]
SOURCE=r'''
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include "eiem_skin_binding.h"
struct Array { char pad[24]{}; size_t count=0; void *items[32]{}; };
struct Node { std::string name; Node *parent=nullptr; std::vector<Node *> children; bool alive=true; };
struct Renderer { Array *bones; };
struct Box { char pad[16]{}; int value=0; } box;
static constexpr size_t IL2CPP_ARRAY_DATA=32;
static int method[8],writes=0;
static bool failSetter=false, s_eiemApplyingModMeshAssignment=false;
static void *g_smr_get_bones=&method[0], *g_transform_get_parent=&method[1], *g_object_get_name=&method[2];
static void *g_transform_get_childCount=&method[3], *g_transform_GetChild=&method[4], *g_transformClass=&method[5], *g_smr_set_bones=&method[6];
static std::vector<std::unique_ptr<Array>> arrays;
static void *NewArray(void *,size_t n) { auto a=std::make_unique<Array>(); a->count=n; arrays.push_back(std::move(a)); return arrays.back().get(); }
static auto il2cpp_array_new=&NewArray;
static std::map<uint32_t,void *> handles;
static uint32_t serial=0;
static auto il2cpp_gchandle_new=+[](void *p,bool)->uint32_t { handles[++serial]=p; return serial; };
static auto il2cpp_gchandle_free=+[](uint32_t h) { assert(handles.erase(h)==1); };
static bool EiemOnUnityThread() { return true; }
static int EiemNativeObjectStatus(void *p) { return p && ((Node *)p)->alive ? 1:0; }
static size_t EiemManagedArrayLength(void *p) { return p ? ((Array *)p)->count : 0; }
static bool EiemManagedObjectArraySame(void *x,void *y) {
 if(!x || !y) return x==y;
 auto a=(Array *)x,b=(Array *)y;
 return a->count==b->count && std::equal(a->items,a->items+a->count,b->items);
}
static void *Invoke(void *m,void *p,void **args=nullptr) {
 if(m==g_smr_get_bones) return ((Renderer *)p)->bones;
 auto n=(Node *)p;
 if(m==g_transform_get_parent) return n->parent;
 if(m==g_object_get_name) return &n->name;
 if(m==g_transform_get_childCount) { box.value=(int)n->children.size(); return &box; }
 if(m==g_transform_GetChild) return n->children.at(*(int *)args[0]);
 if(m==g_smr_set_bones) { ++writes; if(!failSetter) ((Renderer *)p)->bones=(Array *)args[0]; return nullptr; }
 return nullptr;
}
static void *EiemBackendInvokeNoThrow(void *m,void *p) { return Invoke(m,p); }
static void ReadStrUtf8(void *p,char *out,size_t n) { strncpy_s(out,n,((std::string *)p)->c_str(),_TRUNCATE); }
static bool InvokeChecked(void *m,void *p,void **args,void **out) { *out=Invoke(m,p,args); return !failSetter; }
static void EiemCollectBonePathHashes(void *,std::vector<uint32_t> *) {}
struct State { uint32_t replacementBonesHandle=0; };
static std::vector<State> s_eiemOverrides(1);
static SRWLOCK s_eiemOverrideLock=SRWLOCK_INIT;
static size_t EiemFindOverrideLocked(void *) { return 0; }
static void Log(const char *,...) {}
// FUNCTIONS
int main() {
 Node scene{"Scene"},actor{"ActorA",&scene},root{"Root",&actor},chest{"Chest",&root},pelvis{"Pelvis",&root},foot{"Foot",&pelvis};
 scene.children={&actor}; actor.children={&root}; root.children={&chest,&pelvis}; pelvis.children={&foot};
 Array source; source.count=1; source.items[0]=&chest; Renderer renderer{&source};
 EiemSkinIdentity identity; identity.paths={"Root/Chest","Root/Pelvis/Foot"}; identity.hashes={1,2};
 void *out=nullptr; char error[256]{};
 assert(EiemResolveMeshBones(identity,&renderer,&out,error,sizeof(error)));
 auto expanded=(Array *)out; assert(expanded->count==2 && expanded->items[1]==&foot && renderer.bones==&source);
 assert(EiemPreserveSourceSkinning(&renderer,out,error,sizeof(error)) && writes==1 && renderer.bones==expanded);
 assert(handles.size()==1);
 assert(EiemPreserveSourceSkinning(&renderer,out,error,sizeof(error)) && writes==1 && handles.size()==1); // no needless setter
 renderer.bones=&source; // game resets to source palette
 assert(EiemPreserveSourceSkinning(&renderer,out,error,sizeof(error)) && writes==2 && handles.size()==1);
 Node otherActor{"ActorB",&scene},otherRoot{"Root",&otherActor},otherChest{"Chest",&otherRoot},otherPelvis{"Pelvis",&otherRoot},otherFoot{"Foot",&otherPelvis};
 otherActor.children={&otherRoot}; otherRoot.children={&otherChest,&otherPelvis}; otherPelvis.children={&otherFoot};
 Array otherSource; otherSource.count=1; otherSource.items[0]=&otherChest; Renderer other{&otherSource};
 assert(EiemResolveMeshBones(identity,&other,&out,error,sizeof(error)) && ((Array *)out)->items[1]==&otherFoot);
 assert(((Array *)out)->items[1]!=expanded->items[1]); // never borrow actor A's transforms
 identity.paths[1]="Root/Missing"; out=(void *)1;
 assert(!EiemResolveMeshBones(identity,&renderer,&out,error,sizeof(error)) && !out && renderer.bones==expanded);
 foot.alive=false;
 assert(!EiemPreserveSourceSkinning(&renderer,expanded,error,sizeof(error)) && writes==2 && handles.size()==1);
 foot.alive=true; failSetter=true; renderer.bones=&source;
 assert(!EiemPreserveSourceSkinning(&renderer,expanded,error,sizeof(error)) && renderer.bones==&source && handles.size()==1);
 il2cpp_gchandle_free(s_eiemOverrides[0].replacementBonesHandle); assert(handles.empty());
}
'''

class SkinRuntimeTests(unittest.TestCase):
    def test_actual_resolver_and_assignment(self):
        if not shutil.which('cl'): self.skipTest('Requires MSVC')
        trace=(ROOT/'src/il2cpp_trace.h').read_text(encoding='utf-8')
        funcs='\n'.join(function(trace,s) for s in ['static bool EiemResolveMeshBones(', 'static bool EiemPreserveSourceSkinning('])
        with tempfile.TemporaryDirectory(prefix='eiem-skin-runtime-') as directory:
            folder=Path(directory); cpp=folder/'skin.cpp'; cpp.write_text(SOURCE.replace('// FUNCTIONS',funcs),encoding='utf-8'); exe=folder/'skin.exe'
            build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(cpp),f'/Fe{exe}'],cwd=folder,capture_output=True,text=True,encoding='utf-8',errors='replace')
            self.assertEqual(build.returncode,0,build.stdout+build.stderr)
            self.assertEqual(subprocess.run([str(exe)]).returncode,0)

if __name__=='__main__': unittest.main()
