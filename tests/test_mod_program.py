"""Native tests of the actual INI compiler and main-thread update protocol."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = r'''
#include <sstream>
#include <string>
#include <cstdio>
#include "eiem_mod_document.h"
#include "eiem_mod_update.h"
static void Log(const char *, ...) {}
#include "eiem_mods.h"
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (false)
static bool parse(const std::string &text, EiemModProgram &p, std::string &error,
                  const char *file = "test/mod.ini") {
  std::istringstream stream(text);
  return EiemModParseStream(stream, file, p.prefabs, p.rules, p.resources, &error);
}
int main(int argc, char **argv) {
  CHECK(argc >= 2);
  std::string scenario = argv[1], error;
  EiemModProgram p;
  if (scenario == "file") {
    CHECK(argc == 3);
    CHECK(EiemModParseFile(argv[2], p.prefabs, p.rules, p.resources, &error));
    EiemCompileModProgram(p);
    std::printf("rules=%zu resources=%zu standalone=%zu\n", p.rules.size(), p.resources.size(), p.standaloneRules.size());
  } else if (scenario == "semantics") {
    CHECK(parse("\xEF\xBB\xBF[RenderBody]\r\nasset=Body\r\nhandling=skip\r\n; mesh=MeshNew\r\n"
                "material.0=MaterialOld\nmaterial.0=MaterialNew\n[MeshNew]\npath=body.mesh\ntarget.asset=Body\n", p, error));
    CHECK(p.rules.size() == 1 && p.resources.size() == 1);
    CHECK(!p.rules[0].hasMesh && EiemModEquals(p.rules[0].handling, "skip"));
    CHECK(p.rules[0].materialCount == 1 && EiemModEquals(p.rules[0].materials[0], "MaterialNew"));
    EiemModProgram mesh;
    CHECK(parse("[RenderBody]\nasset=Body\nmesh=MeshNew\n; handling=skip\n", mesh, error));
    CHECK(mesh.rules[0].hasMesh && !mesh.rules[0].handling[0]);
    CHECK(parse("[RenderAdd]\npartner.0=RenderExtra\n", mesh, error));
    CHECK(!mesh.rules[1].hasMesh && mesh.rules[1].partnerCount == 1);
  } else if (scenario == "invalid") {
    const char *bad[] = {"match.vertices=12garbage", "match.indices=-1", "match.submeshes=99999999999999999",
      "material.0tail=M", "partner.-1=R", "submesh.1x=0", "submesh.0=no", "handling=clone",
      "if $toggle", "if $toggle == 1", "if($toggle==1)", "else", "endif"};
    CHECK(parse("[RenderExisting]\nasset=Existing\n", p, error));
    for (const char *line : bad) {
      CHECK(!parse(std::string("[RenderValidFirst]\nasset=Valid\n[RenderBad]\nasset=Bad\n") + line, p, error));
      CHECK(p.rules.size() == 1 && !error.empty()); // no partial file published
    }
    CHECK(!parse("[RenderDuplicate]\nasset=A\n[renderduplicate]\nasset=B", p, error));
    CHECK(p.rules.size() == 1);
    CHECK(!parse("[TextureBad]\npath=x.png\nlinear=perhaps", p, error));
    CHECK(!parse("[TextureBad]\npath=x.png\nmip_bias=nan", p, error));
  } else if (scenario == "compile") {
    CHECK(parse("[PrefabA]\npath=actors/A.prefab\nrender.0=RenderScoped\n"
                "[RenderScoped]\nasset=Body\n[RenderMain]\nasset=Body\npartner.0=RenderExtra\n"
                "[RenderExtra]\nasset=Extra\n[RenderUnusedTemplate]\nmesh=MeshBody\n"
                "[MeshBody]\npath=body.mesh\ntarget.asset=Body\n", p, error));
    EiemCompileModProgram(p);
    CHECK(p.standaloneRules.size() == 1 && EiemModEquals(p.rules[p.standaloneRules[0]].section, "RenderMain"));
    // Same identifier in another mod is independent, not captured by A's PFB.
    CHECK(parse("[RenderScoped]\nasset=Other\n", p, error, "other/mod.ini"));
    EiemCompileModProgram(p);
    CHECK(p.standaloneRules.size() == 2);
    EiemCompileModProgram(p);
    CHECK(p.standaloneRules.size() == 2); // recompilation is idempotent
  } else if (scenario == "reload") {
    EiemModUpdateQueue queue;
    std::string order;
    int config = 1, applied = 1;
    // First lifecycle reconcile must not discard effects already applied by startup hooks.
    CHECK(queue.Request(EiemModUpdate::Reconcile));
    EiemDispatchModUpdate(queue.Take(), [&] { order += 'S'; applied = 0; },
                         [&] { order += 'L'; ++config; }, [&] { order += 'A'; applied = config; });
    CHECK(order == "A" && applied == 1);
    order.clear();
    CHECK(queue.Request(EiemModUpdate::Reconcile));
    CHECK(!queue.Request(EiemModUpdate::Reload)); // must merge, not drop F10
    CHECK(!queue.Request(EiemModUpdate::Reapply));
    bool sawOldConfigDuringRestore = false, restoredBeforePublish = false;
    EiemDispatchModUpdate(queue.Take(), [&] { order += 'S'; sawOldConfigDuringRestore = config == 1; applied = 0; },
                         [&] { order += 'L'; restoredBeforePublish = applied == 0; ++config; },
                         [&] { order += 'A'; applied = config; });
    CHECK(order == "SLA" && applied == 2 && sawOldConfigDuringRestore && restoredBeforePublish);
    order.clear();
    CHECK(queue.Request(EiemModUpdate::Reapply));
    EiemDispatchModUpdate(queue.Take(), [&] { order += 'S'; }, [&] { order += 'L'; }, [&] {
      order += 'A';
    });
    CHECK(order == "SA"); // future state changes do not reparse disk
    CHECK(queue.Take() == 0);
    CHECK(queue.Request(EiemModUpdate::Reload)); // requests after drain are schedulable
  } else if (scenario == "publish") {
    EiemReloadMods();
    std::vector<EiemModRule> rules;
    EiemFindStandaloneRenderRules(&rules);
    CHECK(rules.size() == 2);
    CHECK(EiemModEquals(rules[0].asset, "A") && EiemModEquals(rules[1].asset, "Z"));
  } else return 2;
  return 0;
}
'''


class ModProgramTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("cl"):
            raise unittest.SkipTest("Requires MSVC developer environment (cl)")
        cls.temp = tempfile.TemporaryDirectory(prefix="eiem-program-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.folder = Path(cls.temp.name)
        source = cls.folder / "program_test.cpp"
        source.write_text(SOURCE, encoding="utf-8")
        cls.exe = cls.folder / "program_test.exe"
        build = subprocess.run(["cl", "/nologo", "/EHsc", "/std:c++17", "/utf-8",
                                f"/I{ROOT / 'src'}", str(source), f"/Fe{cls.exe}"],
                               cwd=cls.folder, capture_output=True, text=True)
        if build.returncode:
            raise AssertionError(build.stdout + build.stderr)

    def run_case(self, name):
        result = subprocess.run([str(self.exe), name], cwd=self.folder, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_independent_actions_and_repeated_material_slot(self): self.run_case("semantics")
    def test_bad_file_cannot_publish_partial_or_broaden_match(self): self.run_case("invalid")
    def test_compiled_rule_scope_is_mod_local(self): self.run_case("compile")
    def test_reload_and_state_reapply_order(self): self.run_case("reload")

    def test_directory_loading_is_deterministic(self):
        for name, asset in (("z_last", "Z"), ("a_first", "A")):
            directory = self.folder / "plugin" / "mods" / name
            directory.mkdir(parents=True)
            (directory / "mod.ini").write_text(f"[RenderMain]\nasset={asset}\n", encoding="utf-8")
        self.run_case("publish")


if __name__ == "__main__":
    unittest.main()
