"""Instance-scoped bone resolution, including slots absent from source Mesh."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
SOURCE=r'''
#include "eiem_skin_binding.h"
#include <cassert>
int main() {
  std::string root,error; std::vector<size_t> slots;
  const std::vector<std::string> source={"Scene/ActorA/Root/Chest","Scene/ActorA/Root/Unused"};
  const std::vector<std::string> payload={"Root/Chest","Root/Unused","Root/Pelvis","Root/Pelvis/Foot"};
  assert(EiemSkinRootPath(source,payload,root,error) && root=="Scene/ActorA/Root");
  assert(EiemResolveSkinPathIndices(payload,{"Root/Pelvis/Foot","Root/Unused","Root/Pelvis","Root/Chest"},slots,error));
  assert((slots==std::vector<size_t>{3,1,2,0}));
  assert(!EiemResolveSkinPathIndices(payload,{"Root/Chest","Root/Unused"},slots,error) && slots.empty());
  assert(!EiemSkinRootPath({source[0],"Scene/ActorB/Root/Chest"},payload,root,error));
  assert(!EiemResolveSkinPathIndices({"Root/Chest"},{"Root/Chest","Root/Chest"},slots,error));
  assert(!EiemSkinRootPath({"Scene/ActorA/OtherRoot/Chest"},payload,root,error));
  assert(EiemSkinRootPath({"UI/ActorB/Root/Chest"},payload,root,error) && root=="UI/ActorB/Root");
}
'''

class SkinBindingTests(unittest.TestCase):
    def test_instance_scoped_full_skeleton(self):
        if not shutil.which('cl'): self.skipTest('Requires MSVC')
        with tempfile.TemporaryDirectory(prefix='eiem-skin-binding-') as directory:
            folder=Path(directory); cpp=folder/'test.cpp'; cpp.write_text(SOURCE,encoding='utf-8')
            exe=folder/'test.exe'
            build=subprocess.run(['cl','/nologo','/EHsc','/std:c++17',f'/I{ROOT / "src"}',str(cpp),f'/Fe{exe}'],cwd=folder,capture_output=True,text=True)
            self.assertEqual(build.returncode,0,build.stdout+build.stderr)
            self.assertEqual(subprocess.run([str(exe)]).returncode,0)

if __name__=='__main__': unittest.main()
