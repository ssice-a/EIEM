"""Run the production read-only observer against simulated Unity object lifetimes.

These test diagnostic capture/observer non-interference, NOT the game's map cache.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <map>
static std::string logs;
static void Log(const char *fmt, ...) {
  char text[4096]; va_list args; va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args); va_end(args); logs += text; logs += '\n';
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d %s\n",__LINE__,#x); return 1; } } while(false)
static bool onUnity = true;
static bool EiemOnUnityThread() { return onUnity; }
static volatile LONG s_eiemModGeneration = 1;
struct EiemModRule { char modPath[260] = "mod.ini", section[96] = "RenderTest"; };
static bool EiemModEquals(const char *a, const char *b) { return _stricmp(a,b)==0; }
static int tokens[8];
struct Array { char header[32]{}; void *items[8]{}; size_t count = 0; };
struct Object { bool alive=true; int id=1; std::string name; Object *mesh=nullptr; Array materials, children; };
static size_t EiemManagedArrayLength(void *p) { return p ? ((Array *)p)->count : 0; }
static constexpr size_t IL2CPP_ARRAY_DATA=32;
static void *g_renderer_get_sharedMaterials=&tokens[3];
static void *g_gameObject_GetComponentsInChildren=&tokens[2];
static void *g_skinnedMeshRendererClass=&tokens[4], *g_meshFilterClass=nullptr;
static void *s_eiemMaterialGetTexture=nullptr;
static HMODULE hGA=nullptr;
static std::map<uint32_t, void *> weakTargets;
static uint32_t nextWeak=0;
static uint32_t NewWeak(void *p, bool resurrect) { if(resurrect) abort(); weakTargets[++nextWeak]=p; return nextWeak; }
static void *GetTarget(uint32_t h) { if(!weakTargets.count(h)) abort(); return weakTargets[h]; }
static void FreeWeak(uint32_t h) { if(!weakTargets.erase(h)) abort(); }
static auto il2cpp_gchandle_get_target=&GetTarget;
static auto il2cpp_gchandle_free=&FreeWeak;
static void *Unbox(void *p) { return p; }
static auto il2cpp_object_unbox=&Unbox;
static int nativeReads=0, meshReads=0;
static void *RuntimeInvoke(void *method, void *self, void **params, void **exc) {
  ++nativeReads; *exc=nullptr;
  if(method==&tokens[0]) { static bool alive; alive=params[0] && ((Object *)params[0])->alive; return &alive; }
  if(method==&tokens[1]) return &((Object *)self)->id;
  if(method==&tokens[2]) return &((Object *)self)->children;
  if(method==&tokens[3]) return &((Object *)self)->materials;
  return nullptr;
}
static auto il2cpp_runtime_invoke=&RuntimeInvoke;
static void *Identity(void *p) { return p; }
static auto il2cpp_class_get_type=&Identity, il2cpp_type_get_object=&Identity;
static void TraceReadUnityObjectName(void *p,char *out,int n) { strncpy_s(out,n,((Object *)p)->name.c_str(),_TRUNCATE); }
static void ReadStrUtf8(void *,char *,int) {}
static void *EiemReadSharedMesh(void *p,const char *) { ++meshReads; if(!((Object*)p)->alive) abort(); return ((Object*)p)->mesh; }
static void EiemReadLiveMeshShape(void *,int *v,int *i,int *s) { *v=12; *i=24; *s=1; }
static bool EiemReadRendererEnabled(void *,bool *x) { *x=true; return true; }
static bool EiemReadRendererVisible(void *,bool *x) { *x=true; return true; }
static void *FindClass(const char *,const char *,void **,size_t) { return nullptr; }
static void *FindMethod(void *,const char *,int) { return nullptr; }
static void (*s_eiemResourceDiagnostic)(const char *,const char *,const char *,void *)=nullptr;
#include "eiem_residue_probe.h"
int main() {
  s_eiemProbeAlive=&tokens[0]; s_eiemProbeId=&tokens[1]; s_eiemProbeWeak=NewWeak;
  Object original, generated, material, renderer, copiedRenderer, model;
  original.name="MeshExample"; original.id=10;
  generated.name="MeshExample"; generated.id=20;
  material.name="MaterialExample"; material.id=30;
  renderer.name="ConsumerA"; renderer.id=40; renderer.mesh=&original;
  renderer.materials.count=1; renderer.materials.items[0]=&material;
  EiemProbeRememberRule(EiemModRule{}, &original);
  EiemProbeRenderer("before", &renderer, &renderer, "SkinnedMeshRenderer", nullptr, true);
  renderer.mesh=&generated;
  EiemProbeResource("mesh-built","mod.ini","MeshExample",&generated);
  // Disable/reload restores consumer A, but a simulated second consumer retains a reference.
  renderer.mesh=&original; s_eiemModGeneration=2;
  copiedRenderer=renderer; copiedRenderer.id=41; copiedRenderer.name="ConsumerB"; copiedRenderer.mesh=&generated;
  model.name="Model"; model.children.count=1; model.children.items[0]=&copiedRenderer;
  EiemProbeObserveModel(&model,"arrive-with-no-rules");
  CHECK(logs.find("stage=arrive-with-no-rules")!=std::string::npos);
  CHECK(logs.find("id=41/name=ConsumerB")!=std::string::npos);
  CHECK(copiedRenderer.mesh==&generated && renderer.mesh==&original); // observer never repairs or replaces
  CHECK(s_eiemProbeWatches.size()==1);
  // Native resource destruction can leave a live managed wrapper in a weak handle.
  generated.alive=false;
  EiemProbeCheckpoint("native-dead",true);
  CHECK(logs.find("native=0")!=std::string::npos);
  // Expired wrappers are released, not dereferenced; all other observers remain intact.
  for(auto &entry:weakTargets) if(entry.second==&generated) entry.second=nullptr;
  EiemProbeCheckpoint("wrapper-expired");
  CHECK(logs.find("weak-expired")!=std::string::npos);
  renderer.alive=false;
  int beforeMeshReads=meshReads;
  EiemProbeTrackedRenderer("dead-renderer",&renderer);
  CHECK(meshReads==beforeMeshReads);
  // Unsupported native-alive API reports unknown; it never falls back to a raw pointer guess.
  s_eiemProbeAlive=nullptr;
  CHECK(EiemProbeNativeAlive(&original)==-1);
  s_eiemProbeAlive=&tokens[0];
  onUnity=false; int before=nativeReads;
  EiemProbeCheckpoint("wrong-thread");
  EiemProbeObserveModel(&model,"wrong-thread");
  EiemProbeResource("wrong-thread","mod.ini","MeshExample",&generated);
  CHECK(before==nativeReads);
  onUnity=true; s_eiemProbeReading=true;
  EiemProbeCheckpoint("reentrant"); CHECK(before==nativeReads);
  s_eiemProbeReading=false;
  // Suppress unchanged snapshots, but retain a return to an earlier state.
  const auto beforeDedup = s_eiemProbeSequence;
  EiemProbeChanged("dedup", "test", "same-object", "A");
  EiemProbeChanged("dedup", "test", "same-object", "A");
  CHECK(s_eiemProbeSequence == beforeDedup + 1);
  EiemProbeChanged("dedup", "test", "same-object", "B");
  EiemProbeChanged("dedup", "test", "same-object", "A");
  CHECK(s_eiemProbeSequence == beforeDedup + 3);
  puts("PASS: disabled-rule arrivals, native death, weak expiry, no mutation, thread/reentry isolation, state-change dedup");
  return 0;
}
'''


class ResidueProbeTests(unittest.TestCase):
    def test_production_observer_does_not_change_lifetime_or_binding(self):
        if not shutil.which('cl'):
            self.skipTest('Requires MSVC developer environment')
        with tempfile.TemporaryDirectory(prefix='eiem-residue-probe-') as folder:
            folder = Path(folder)
            source, exe = folder / 'probe.cpp', folder / 'probe.exe'
            source.write_text(SOURCE, encoding='utf-8')
            build = subprocess.run(['cl', '/nologo', '/EHsc', '/std:c++17', '/utf-8',
                                    f'/I{ROOT / "src"}', str(source), f'/Fe{exe}'],
                                   cwd=folder, capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_diagnostic_has_no_native_mutation_or_strong_root_calls(self):
        source = (ROOT / 'src/eiem_residue_probe.h').read_text(encoding='utf-8')
        for forbidden in ('EiemSetSharedMesh(', 'il2cpp_gchandle_new(', 'EiemAssignRendererMaterials(',
                          'il2cpp_object_new(', 'Destroy(', 'DontDestroyOnLoad('):
            self.assertNotIn(forbidden, source)
        trace = (ROOT / 'src/il2cpp_trace.h').read_text(encoding='utf-8')
        self.assertNotIn('#include "eiem_residue_probe.h"', trace)
        self.assertNotIn('EiemProbeObserveModel', trace)

if __name__ == '__main__':
    unittest.main()
