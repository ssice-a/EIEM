"""Real Lua + ImGui CPU frames; no injection, game process or user Blender scene."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include "eiem_lua_ui.h"
#include <cstdio>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(false)
static bool frame(EiemLuaUi &ui, const EiemVariables &vars, EiemVariables &writes) {
  ImGui::GetIO().DisplaySize={800,600}; ImGui::GetIO().DeltaTime=1.f/60;
  ImGui::NewFrame(); bool ok=ui.Draw(vars,writes); ImGui::Render(); return ok;
}
int main(int argc, char **argv) {
  ImGui::CreateContext();
  auto &io=ImGui::GetIO(); io.IniFilename=nullptr; io.Fonts->AddFontDefault();
  unsigned char *pixels; int w,h; io.Fonts->GetTexDataAsRGBA32(&pixels,&w,&h);
  EiemVariables vars{{"$size",.25},{"$other",0}}, writes;
  EiemLuaUi a("a/UI"), b("b/UI"); a.open=b.open=true;
  const char *source=R"lua(
    local count=0
    return function()
      count=count+1
      assert(io==nil and os==nil and package==nil and debug==nil and load==nil and pcall==nil)
      imgui.SetNextWindowSize(300,350)
      imgui.PushStyleColor(imgui.Col.Button, 0.3,0.2,0.4,1)
      if imgui.Begin("Same title") then
        imgui.PopStyleColor()
        imgui.PushStyleVar(imgui.StyleVar.FramePadding,4,3)
        imgui.Text("100% safe text")
        local changed,v=imgui.SliderFloat("Size",mod.get("$size"),0,1)
        imgui.DragFloat("Drag",v,.01)
        imgui.InputFloat("Input",v)
        imgui.Checkbox("Check",true)
        imgui.ColorEdit4("Color",1,.2,.3,1)
        if imgui.BeginChild("child",0,60,true) then imgui.Text("child") end
        imgui.EndChild()
        if imgui.BeginTable("table",2) then
          imgui.TableNextRow(); imgui.TableNextColumn(); imgui.Text("cell")
          imgui.EndTable()
        end
        imgui.PopStyleVar()
      else imgui.PopStyleColor() end
      imgui.End()
      mod.set("$size",mod.get("$size")+.25)
      mod.set("$other",count)
    end
  )lua";
  CHECK(a.Load(source,"a.lua") && b.Load(source,"b.lua"));
  CHECK(frame(a,vars,writes)); CHECK(writes.at("$size")==.5 && writes.at("$other")==1);
  CHECK(frame(a,vars,writes)); CHECK(writes.at("$other")==2);
  CHECK(frame(b,vars,writes)); CHECK(writes.at("$other")==1);
  CHECK(ImGui::FindWindowByName("Same title###a/UI") != ImGui::FindWindowByName("Same title###b/UI"));
  a.open=false; CHECK(!frame(a,vars,writes) && writes.empty()); a.open=true;
  CHECK(a.Load(source,"a.lua")); CHECK(frame(a,vars,writes) && writes.at("$other")==1);

  for (const char *bad : {
    "return function() mod.set('$size',1); error('intentional') end",
    "return function() mod.set('$missing',1) end",
    "return function() mod.set('$size',0/0) end",
    "return function() while true do end end",
    "return function() local t=string.rep('x',32*1024*1024) end",
    "return function() imgui.Begin('bad'); imgui.PushStyleColor(0,1,0,0,1) end",
    "return function() imgui.PopStyleColor() end",
    "return function() imgui.End() end",
    "return function() imgui.Begin('bad'); imgui.Begin('nested') end",
    "return function() imgui.Begin('bad'); imgui.PushID('x'); imgui.End() end"
  }) {
    CHECK(a.Load(bad,"broken.lua"));
    CHECK(!frame(a,vars,writes) && !a.error.empty() && writes.empty());
    CHECK(frame(b,vars,writes)); // other VM/ImGui stacks survive every failure
  }
  CHECK(!a.Load("this is not Lua", "syntax.lua"));
  CHECK(a.error.find("syntax.lua")!=std::string::npos);
  CHECK(!a.Load("return 3", "not_function.lua"));
  CHECK(!a.Load("while true do end", "init_loop.lua"));
  CHECK(!a.LoadFile("mod.ini","../outside.lua"));

  // Drive the actual native slider with ImGui input events, not an unconditional
  // mod.set in the script. Measure its real rectangle via the public bindings.
  EiemLuaUi click("click/UI"); click.open=true;
  EiemVariables inputVars{{"$size",0},{"$x",0},{"$y",0},{"$right",0}};
  CHECK(click.Load(R"lua(return function()
    imgui.SetNextWindowPos(40,40)
    imgui.SetNextWindowSize(300,180)
    if imgui.Begin("Slider interaction") then
      imgui.SetNextItemWidth(200)
      local changed,value=imgui.SliderFloat("##size",mod.get("$size"),0,1)
      if changed then mod.set("$size",value) end
      local x,y=imgui.GetItemRectMin()
      local r,b=imgui.GetItemRectMax()
      mod.set("$x",x); mod.set("$y",(y+b)/2); mod.set("$right",r)
    end
    imgui.End()
  end)lua","click.lua"));
  CHECK(frame(click,inputVars,writes)); CHECK(frame(click,inputVars,writes));
  io.AddMousePosEvent((float)((writes.at("$x")+writes.at("$right"))/2),(float)writes.at("$y"));
  io.AddMouseButtonEvent(0,true);
  CHECK(frame(click,inputVars,writes));
  CHECK(writes.count("$size") && writes.at("$size")>.4 && writes.at("$size")<.6);
  io.AddMouseButtonEvent(0,false); CHECK(frame(click,inputVars,writes));
  auto *window=ImGui::FindWindowByName("Slider interaction###click/UI");
  CHECK(window);
  float closeX=window->Pos.x+window->Size.x-ImGui::GetStyle().FramePadding.x-ImGui::GetFontSize()/2;
  float closeY=window->Pos.y+ImGui::GetStyle().FramePadding.y+ImGui::GetFontSize()/2;
  io.AddMousePosEvent(closeX,closeY); io.AddMouseButtonEvent(0,true);
  CHECK(frame(click,inputVars,writes));
  io.AddMouseButtonEvent(0,false); CHECK(frame(click,inputVars,writes));
  CHECK(!click.open); // Begin's native X closes only this UI

  // Optional generated Blender packages use the same real VM and bindings.
  for (int i=1;i<argc;++i) {
    EiemModProgram program; std::string error;
    CHECK(EiemModParseFile(argv[i],program,&error));
    for (const auto &state:program.states) for(const auto &ui:state.uis) {
      EiemLuaUi generated(state.path+ui.section); generated.open=true;
      CHECK(generated.LoadFile(state.path,ui.path));
      CHECK(frame(generated,state.variables,writes));
    }
  }
  ImGui::DestroyContext();
  return 0;
}
'''


def build_harness(folder, source=SOURCE):
    cpp = folder / "lua_ui.cpp"
    cpp.write_text(source, encoding="utf-8")
    exe = folder / "lua_ui.exe"
    command = ["cl", "/nologo", "/MD", "/EHsc", "/std:c++17", "/utf-8",
               f"/I{ROOT / 'src'}", f"/I{ROOT / 'deps/imgui'}", f"/I{ROOT / 'deps/lua'}",
               str(cpp), *(str(ROOT / 'deps/imgui' / name) for name in (
                   'imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp')),
               str(ROOT / 'bin/lua.lib'), f"/Fe{exe}", "user32.lib", "imm32.lib"]
    build = subprocess.run(command, cwd=folder, capture_output=True, text=True,
                           encoding="utf-8", errors="replace", timeout=120)
    if build.returncode:
        raise AssertionError(build.stdout + build.stderr)
    return exe


class LuaUiTests(unittest.TestCase):
    def test_real_vm_frames_errors_isolation_and_reload(self):
        if not shutil.which("cl") or not (ROOT / "bin/lua.lib").exists():
            self.skipTest("Run build.bat, then tests from MSVC environment")
        with tempfile.TemporaryDirectory(prefix="eiem-lua-ui-") as temp:
            folder = Path(temp)
            exe = build_harness(folder)
            run = subprocess.run([str(exe)], cwd=folder, capture_output=True, text=True,
                                 encoding="utf-8", errors="replace", timeout=30)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == "__main__":
    unittest.main()
