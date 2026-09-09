"""Real parser, publication and disk reload in an isolated Mod directory."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <filesystem>
#include <cstdio>
static void Log(const char*,...) {}
#include "eiem_mods.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(false)
static const char *ini=R"ini([Constants]
persist $size=0.25
$open=0
[KeySize]
key=F6
type=cycle
$size=0.25,0.75
[UISize]
path=ui.lua
[RenderBody]
asset=Body
shape.Inflate=$size
)ini";
static void put(const char *path,const char *value){std::ofstream(path)<<value;}
int main(){
  namespace fs=std::filesystem;fs::create_directories("plugin/mods/a");fs::create_directories("plugin/mods/b");
  put("plugin/mods/a/mod.ini",ini);put("plugin/mods/b/mod.ini",ini);
  EiemReloadMods();CHECK(s_eiemModProgram.states.size()==2);
  auto next=s_eiemModProgram;std::string error;
  CHECK(EiemApplyUiValues(next,0,{{"$size",.75},{"$open",1}},error));EiemPublishModState(next);
  s_eiemPersistentStates.Flush(true);
  CHECK(fs::exists("plugin/mods/a/state.ini"));
  EiemReloadMods();CHECK(s_eiemModProgram.states[0].variables.at("$size")==.75);
  CHECK(s_eiemModProgram.states[0].variables.at("$open")==0);
  CHECK(s_eiemModProgram.states[1].variables.at("$size")==.25);
  // Empty/commented programs never recover actions or variables from state.ini.
  put("plugin/mods/a/mod.ini","; disabled\n");EiemReloadMods();
  CHECK(s_eiemModProgram.states[0].variables.empty());
  put("plugin/mods/a/mod.ini",ini);EiemReloadMods();CHECK(s_eiemModProgram.states[0].variables.at("$size")==.75);
  // Disk re-read, not a stale in-memory preference after explicit reset.
  fs::remove("plugin/mods/a/state.ini");EiemReloadMods();CHECK(s_eiemModProgram.states[0].variables.at("$size")==.25);
  put("plugin/mods/a/state.ini","[Values]\n$size=nan\n");EiemReloadMods();CHECK(s_eiemModProgram.states[0].variables.at("$size")==.25);
  put("plugin/mods/a/state.ini","[Values]\n$size=1e300\n");EiemReloadMods();CHECK(s_eiemModProgram.states[0].variables.at("$size")==.25);
  put("plugin/mods/a/state.ini","[Values]\n$size=.5\n$gone=1\n$open=1\n");EiemReloadMods();
  CHECK(s_eiemModProgram.states[0].variables.at("$size")==.5 && s_eiemModProgram.states[0].variables.at("$open")==0);
  // Separate store simulates process restart, retaining only declared values.
  EiemPersistentStore restarted;auto p=s_eiemModProgram;p.states[0].variables=p.states[0].defaults;
  restarted.Load(p);CHECK(p.states[0].variables.at("$size")==.5);
  // Invalid declarations are rejected by the actual grammar.
  for(auto bad:{"[Constants]\npersist size=1", "[Constants]\npersist $x=1\n$x=2"}){
    std::istringstream input(bad);EiemModProgram invalid;CHECK(!EiemModParseStream(input,"bad",invalid,&error));
  }
  // Failed atomic replacement retains the previously saved file for retry.
  HANDLE held=CreateFileW(L"plugin/mods/a/state.ini",GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
  CHECK(held!=INVALID_HANDLE_VALUE);
  p.states[0].variables["$size"]=.9;restarted.Queue(p);restarted.Flush(true);
  CloseHandle(held);restarted.Flush(true);
  EiemPersistentStore again;p.states[0].variables=p.states[0].defaults;again.Load(p);CHECK(p.states[0].variables.at("$size")==.9);
  return 0;
}
'''

class PersistentStateTests(unittest.TestCase):
    def test_parser_publication_reload_and_restart(self):
        if not shutil.which("cl"):
            self.skipTest("Requires MSVC")
        with tempfile.TemporaryDirectory(prefix="eiem-persist-") as temp:
            folder=Path(temp); source=folder/"test.cpp"; exe=folder/"test.exe"
            source.write_text(SOURCE,encoding="utf-8")
            build=subprocess.run(["cl","/nologo","/EHsc","/std:c++17","/utf-8",f"/I{ROOT/'src'}",str(source),f"/Fe{exe}"],cwd=folder,capture_output=True,text=True,encoding="utf-8",errors="replace")
            self.assertEqual(build.returncode,0,build.stdout+build.stderr)
            run=subprocess.run([str(exe)],cwd=folder,capture_output=True,text=True,timeout=20)
            self.assertEqual(run.returncode,0,run.stdout+run.stderr)

if __name__=="__main__":unittest.main()
