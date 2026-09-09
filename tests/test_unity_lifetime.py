"""The actual handle wrapper: weak observation, strong baseline, native death."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
SOURCE=r'''
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <map>
static bool onThread=true;
static bool EiemOnUnityThread() { return onThread; }
struct Object { bool native=true; };
struct Handle { void *target; bool weak; };
static std::map<uint32_t,Handle> handles;
static uint32_t next=0;
static int strongCount=0,weakCount=0;
static uint32_t Strong(void *o,bool) { ++strongCount; handles[++next]={o,false}; return next; }
static uint32_t Weak(void *o,bool resurrect) { if(resurrect) abort(); ++weakCount; handles[++next]={o,true}; return next; }
static void *Target(uint32_t h) { if(!handles.count(h)) abort(); return handles[h].target; }
static void Free(uint32_t h) { if(handles.erase(h)!=1) abort(); }
static auto il2cpp_gchandle_new=Strong;
static auto il2cpp_gchandle_get_target=Target;
static auto il2cpp_gchandle_free=Free;
static void *NativeInvoke(void *,void *,void **p,void **e) { *e=nullptr; static bool alive; alive=((Object *)p[0])->native; return &alive; }
static auto il2cpp_runtime_invoke=NativeInvoke;
static auto il2cpp_object_unbox=+[](void *p)->void * { return p; };
static HMODULE hGA=nullptr;
static void *FindClass(const char *,const char *,void **,size_t) { return nullptr; }
static void *FindMethod(void *,const char *,int) { return nullptr; }
static void Log(const char *,...) {}
#include "eiem_unity_lifetime.h"
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %d %s\n",__LINE__,#x); return 1; } } while(false)
int main() {
  s_eiemNativeAliveMethod=(void *)1; s_eiemNewWeakHandle=Weak;
  Object observed,source;
  {
    auto model=EiemUnityRef::Capture(&observed);
    CHECK(weakCount==1 && strongCount==0 && model.Status()==1);
    auto snapshot=model;
    CHECK(handles.size()==1); // copying a registry snapshot doesn't duplicate or free handles
    model={}; CHECK(snapshot.Status()==1);
    observed.native=false; CHECK(snapshot.Status()==0 && snapshot.Target()==&observed);
    observed.native=true;
    for(auto &h:handles) if(h.second.weak) h.second.target=nullptr;
    CHECK(snapshot.Status()==0 && snapshot.Target()==nullptr);
    auto sourceRef=EiemUnityRef::Capture(&source,false);
    CHECK(strongCount==1 && sourceRef.Status()==1);
    source.native=false; CHECK(sourceRef.Status()==0); // strong GC root isn't native validity
    source.native=true; onThread=false; CHECK(sourceRef.Status()==-1);
    onThread=true; s_eiemNativeAliveMethod=nullptr; CHECK(sourceRef.Status()==-1);
  }
  CHECK(handles.empty());
  s_eiemNewWeakHandle=nullptr;
  CHECK(!EiemUnityRef::Capture(&observed)); // no strong fallback for missing weak API
  return 0;
}
'''

class UnityLifetime(unittest.TestCase):
    def test_actual_reference_ownership_and_native_validity(self):
        if not shutil.which('cl'): self.skipTest('Requires MSVC')
        with tempfile.TemporaryDirectory(prefix='eiem-lifetime-') as tmp:
            folder=Path(tmp); source=folder/'test.cpp'; exe=folder/'test.exe'
            source.write_text(SOURCE,encoding='utf-8')
            build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17','/utf-8',f'/I{ROOT/"src"}',str(source),f'/Fe{exe}'],cwd=folder,capture_output=True,text=True)
            self.assertEqual(build.returncode,0,build.stdout+build.stderr)
            result=subprocess.run([str(exe)],cwd=folder,capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
