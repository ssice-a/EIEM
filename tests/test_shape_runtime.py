"""Execute the production IL2CPP shape adapter with a simulated native API."""
from pathlib import Path
from runtime_source import read_runtime_source
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PREFIX = r'''
#include <windows.h>
#include <cstdio>
#include <cstring>
#include "eiem_shape_state.h"
#include "eiem_shape_binding.h"
struct EiemShapeRuntimeBinding {bool initialized=false;};
template<class Backend> static bool EiemPrepareShapeBinding(EiemShapeState &,void *,void *,const std::vector<EiemShapeBaseline> &,Backend &,std::string &) {return true;}
static void EiemSyncShapeClaims(EiemShapeState &,const EiemModRule &) {}
static bool EiemShapeBindingMatches(const EiemShapeState &,void *,void *) {return true;}
static void EiemRetireShapeBinding(const std::shared_ptr<EiemShapeRuntimeBinding> &) {}
static void Log(const char*,...) {}
struct Mesh { std::vector<std::string> names; };
struct Renderer { Mesh *mesh; std::vector<float> weights; int writes=0; };
static bool unityThread=true, fail=false, ignoreWrite=false;
static bool EiemOnUnityThread() { return unityThread; }
static void *EiemReadSharedMesh(void *r,const char*) { return ((Renderer*)r)->mesh; }
static void ReadStrUtf8(void *s,char *out,size_t n) { strncpy_s(out,n,(const char*)s,_TRUNCATE); }
static void *g_mesh_get_blendShapeCount=(void*)1, *g_mesh_GetBlendShapeName=(void*)2;
static void *g_smr_GetBlendShapeWeight=(void*)3, *g_smr_SetBlendShapeWeight=(void*)4;
static void *SimInvoke(void *m,void *o,void **args,void **exception) {
  alignas(16) static char box[32];
  *exception=nullptr;
  if(m==(void*)1) { *(int*)(box+16)=(int)((Mesh*)o)->names.size(); return box; }
  int index=*(int*)args[0];
  if(m==(void*)2) return (void*)((Mesh*)o)->names.at(index).c_str();
  auto &r=*(Renderer*)o;
  if(m==(void*)3) { *(float*)(box+16)=r.weights.at(index); return box; }
  if(fail) { *exception=(void*)99; return nullptr; }
  if(!ignoreWrite) { r.weights.at(index)=*(float*)args[1]; ++r.writes; }
  return nullptr;
}
static auto il2cpp_runtime_invoke=SimInvoke;
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
'''
MAIN = r'''
int main() {
  Mesh mesh{{"Other","Inflate"}};
  Renderer r{&mesh,{25,10}};
  EiemUnityShapes backend;
  std::vector<EiemShapeBaseline> snapshot;
  CHECK(backend.Snapshot(&r,&mesh,snapshot) && snapshot.size()==2 && snapshot[1].value==10);
  EiemModRule rule{}; std::string error;
  CHECK(EiemSetRenderField(rule,"shape.Inflate",".75",error));
  EiemShapeState state;
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",rule,state);
  CHECK(s_eiemShapeMessage.empty() && r.weights[1]==75 && r.weights[0]==25);
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",rule,state);
  CHECK(r.writes==1);
  EiemModRule empty{};
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",empty,state);
  CHECK(r.weights[1]==10);
  fail=true;
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",rule,state);
  CHECK(!s_eiemShapeMessage.empty() && r.weights[1]==10);
  fail=false; ignoreWrite=true; s_eiemShapeMessage.clear();
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",rule,state);
  CHECK(!s_eiemShapeMessage.empty()); // a void return is not proof of a write
  ignoreWrite=false; unityThread=false; s_eiemShapeMessage.clear();
  EiemUpdateRendererShapes(&r,"SkinnedMeshRenderer",rule,state);
  CHECK(s_eiemShapeMessage.find("Unity thread")!=std::string::npos && r.weights[1]==10);
  unityThread=true; s_eiemShapeMessage.clear();
  EiemUpdateRendererShapes(&r,"MeshFilter",rule,state);
  CHECK(s_eiemShapeMessage.find("SkinnedMeshRenderer")!=std::string::npos);
  return 0;
}
'''


class ShapeRuntimeTests(unittest.TestCase):
    def _compile_and_run(self, contents):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        with tempfile.TemporaryDirectory(prefix="eiem-shape-runtime-") as directory:
            folder = Path(directory)
            source = folder / "test.cpp"
            source.write_text(contents, encoding="utf-8")
            exe = folder / "test.exe"
            build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                    f"/I{ROOT / 'src'}", str(source), f"/Fe{exe}"], cwd=folder,
                                   capture_output=True, text=True, encoding="utf-8", errors="replace")
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=15)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)

    def test_native_adapter_readback_thread_and_failures(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        trace = read_runtime_source(ROOT)
        api = (ROOT / "src/il2cpp_api.h").read_text(encoding="utf-8")
        invoke = api[api.index("static bool InvokeChecked"):api.index("static void DumpClassMethods")]
        start = trace.index("struct EiemUnityShapes")
        adapter = trace[start:trace.index("struct EiemRenderOverrideState", start)]
        self._compile_and_run(PREFIX + invoke + adapter + MAIN)

    def test_native_mesh_shape_upload_and_failed_void_setters(self):
        backend = (ROOT / "src/eiem_resource_backend.h").read_text(encoding="utf-8")
        types = backend[backend.index("struct EiemNativeSubMesh"):backend.index("// AnimeStudio's Unity")]
        upload = backend[backend.index("static bool EiemWriteMeshShapes"):backend.index("static void *EiemBuildNativeMesh")]
        api = (ROOT / "src/il2cpp_api.h").read_text(encoding="utf-8")
        invoke = api[api.index("static bool InvokeChecked"):api.index("static void DumpClassMethods")]
        prefix = r'''
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
struct Vector2 { float x,y; };
struct Vector3 { float x,y,z; };
struct Vector4 { float x,y,z,w; };
using Color=Vector4;
struct Matrix4x4 { float m[16]; };
struct BoneWeight { int unused; };
struct Mesh {
  std::vector<std::string> names;
  std::vector<int> frames;
  std::vector<Vector3> vertices,normals,tangents;
  float weight=0;
};
static std::vector<std::unique_ptr<std::vector<Vector3>>> arrays;
static void *s_eiemMeshAddBlendShapeFrame=(void*)1, *g_mesh_get_blendShapeCount=(void*)2;
static void *g_mesh_GetBlendShapeName=(void*)3, *s_eiemMeshGetBlendShapeFrameCount=(void*)4;
static void *s_eiemVector3Class=(void*)10;
static bool fail=false,ignore=false,dropFrame=false;
static void ReadStrUtf8(void *s,char *out,size_t n) { strncpy_s(out,n,(const char*)s,_TRUNCATE); }
static void *il2cpp_string_new(const char *s) { return (void*)s; }
static void *EiemMakeValueArray(void*, const std::vector<Vector3> &v) {
  arrays.push_back(std::make_unique<std::vector<Vector3>>(v)); return arrays.back().get();
}
static void *SimInvoke(void *method,void *obj,void **args,void **exception) {
  *exception=nullptr; auto &mesh=*(Mesh*)obj;
  alignas(16) static char box[32];
  if(method==(void*)1) {
    if(fail) { *exception=(void*)99; return nullptr; }
    if(ignore) return nullptr;
    std::string name=(char*)args[0];
    if(mesh.names.empty() || mesh.names.back()!=name) { mesh.names.push_back(name); mesh.frames.push_back(0); }
    if(!dropFrame) ++mesh.frames.back();
    mesh.weight=*(float*)args[1];
    mesh.vertices=*(std::vector<Vector3>*)args[2];
    mesh.normals=*(std::vector<Vector3>*)args[3];
    mesh.tangents=*(std::vector<Vector3>*)args[4];
    return nullptr;
  }
  if(method==(void*)2) { *(int*)(box+16)=(int)mesh.names.size(); return box; }
  int index=*(int*)args[0];
  if(method==(void*)3) return (void*)mesh.names.at(index).c_str();
  *(int*)(box+16)=mesh.frames.at(index); return box;
}
static auto il2cpp_runtime_invoke=SimInvoke;
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
'''
        main = r'''
int main() {
  EiemNativeMeshDocument doc; doc.vertexCount=3;
  EiemNativeBlendShapeVertex v{}; v.index=1; v.vertex.z=.5f; v.normal.y=.25f; v.tangent.x=2;
  doc.blendShapeVertices.push_back(v);
  EiemNativeBlendShapeFrame frame{}; frame.vertexCount=1; frame.hasNormals=true;
  doc.blendShapeFrames.push_back(frame);
  EiemNativeBlendShapeChannel channel{}; channel.name="Inflate"; channel.frameCount=1;
  doc.blendShapeChannels.push_back(channel); doc.blendShapeWeights.push_back(100);
  Mesh mesh; char error[256]={};
  CHECK(EiemWriteMeshShapes(&mesh,doc,error,sizeof(error)));
  CHECK(mesh.vertices.size()==3 && mesh.vertices[0].z==0 && mesh.vertices[1].z==.5f);
  CHECK(mesh.normals[1].y==.25f && mesh.tangents[1].x==0 && mesh.weight==100);
  mesh={}; fail=true;
  CHECK(!EiemWriteMeshShapes(&mesh,doc,error,sizeof(error)) && strstr(error,"AddBlendShapeFrame failed"));
  fail=false; ignore=true;
  CHECK(!EiemWriteMeshShapes(&mesh,doc,error,sizeof(error)) && strstr(error,"channel count"));
  ignore=false; dropFrame=true;
  CHECK(!EiemWriteMeshShapes(&mesh,doc,error,sizeof(error)) && strstr(error,"metadata"));
  dropFrame=false; mesh={}; doc.blendShapeVertices[0].index=3;
  CHECK(!EiemWriteMeshShapes(&mesh,doc,error,sizeof(error)) && mesh.names.empty());
  return 0;
}
'''
        self._compile_and_run(prefix + types + invoke + upload + main)


if __name__ == "__main__": unittest.main()
