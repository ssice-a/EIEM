"""Execute the production mesh cache path with checked locks and GC handles.

Only OS/IL2CPP dependencies are fixtures. The complete EiemBuildMeshResource
function is compiled unchanged, including cache lookup, eviction and building.
Run in an MSVC developer environment (cl on PATH).
"""

from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def production_builder():
    source = (ROOT / "src/eiem_resource_backend.h").read_text(encoding="utf-8")
    start = source.index("static bool EiemBuildMeshResource(")
    end = source.index("static bool EiemResolveResourceDiskPath(", start)
    return source[start:end]


def production_cache():
    source = (ROOT / "src/eiem_resource_backend.h").read_text(encoding="utf-8")
    lifetime = (ROOT / "src/eiem_unity_lifetime.h").read_text(encoding="utf-8")
    ref = lifetime[lifetime.index("class EiemUnityRef {"):lifetime.index("static void EiemInitUnityLifetime")]
    return ref + source[source.index("struct EiemMeshSubmeshVisibilityState {"):source.index("static void *EiemFindMethodWithParamTypes(")]


FIXTURE = r'''
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
#include "eiem_skin_binding.h"
#include "eiem_file_io.h"
#define MAX_PATH 260
#define SRWLOCK_INIT {}
using LONG = long;
static int heldLocks = 0;
static bool unityThread = true;
static bool EiemOnUnityThread() { return unityThread; }
struct SRWLOCK { bool held = false; int acquired = 0; int released = 0; };
static void AcquireSRWLockExclusive(SRWLOCK *lock) {
    if (lock->held) std::abort();
    lock->held = true; ++lock->acquired; ++heldLocks;
}
static void ReleaseSRWLockExclusive(SRWLOCK *lock) {
    if (!lock->held) std::abort();
    lock->held = false; ++lock->released; --heldLocks;
}
static void AcquireSRWLockShared(SRWLOCK *lock) { AcquireSRWLockExclusive(lock); }
static void ReleaseSRWLockShared(SRWLOCK *lock) { ReleaseSRWLockExclusive(lock); }
struct EiemModRule { bool hasMesh = true; char modPath[MAX_PATH] = "mod.ini";
    char mesh[96] = "MeshA"; uint32_t hiddenSubmeshMask = 0; };
struct EiemModResource { char modPath[MAX_PATH] = "mod.ini";
    char section[96] = "MeshA"; char path[MAX_PATH] = "mesh.mesh"; };
static SRWLOCK s_eiemMeshResourceCacheLock;
// F10 publishes a generation and intentionally rebuilds generated resources.
static volatile LONG s_eiemModGeneration = 1;
static constexpr bool kEiemEnableNativeMeshBoneSlots = false;
static constexpr bool kEiemEnableLifecycleDiagnostics = false;
static bool EiemValidateNativeMeshBoneSlots(void *, const char *, char *, size_t) {
    return true;
}
static bool EiemModEquals(const char *left, const char *right) {
    return left && right && std::strcmp(left, right) == 0;
}
static std::map<uint32_t, void *> targets;
static std::vector<uint32_t> freed;
static int tokens[32];
static int buildCalls = 0;
static int visibilityCalls = 0;
static uint32_t nextHandle = 100;
static bool failBuild = false;
static void *deadNative = nullptr;
static void *unknownNative = nullptr;
static int EiemNativeObjectStatus(void *object) {
    if (heldLocks) std::abort();
    if (!unityThread) return -1;
    if (!object || object == deadNative) return 0;
    return object == unknownNative ? -1 : 1;
}
static void (*s_eiemResourceDiagnostic)(const char *, const char *, const char *, void *) = nullptr;
static uint32_t NewHandle(void *object, bool) {
    targets[++nextHandle] = object; return nextHandle;
}
static void *GetTarget(uint32_t handle) { return targets[handle]; }
static void FreeHandle(uint32_t handle) { freed.push_back(handle); targets.erase(handle); }
static auto il2cpp_gchandle_new = &NewHandle;
static auto il2cpp_gchandle_get_target = &GetTarget;
static auto il2cpp_gchandle_free = &FreeHandle;
static uint32_t (*s_eiemNewWeakHandle)(void *, bool) = nullptr;
static void Log(const char *, ...) {}
static LONG InterlockedCompareExchange(volatile LONG *value, LONG, LONG) { return *value; }
static uint64_t EiemMeshCacheStamp(uint64_t fileStamp) {
    const uint64_t generation = (uint64_t)(uint32_t)InterlockedCompareExchange(
        &s_eiemModGeneration, 0, 0);
    uint64_t stamp = fileStamp ^
        (generation + 0x9E3779B97F4A7C15ULL + (fileStamp << 6) + (fileStamp >> 2));
    stamp ^= stamp >> 30;
    stamp *= 0xBF58476D1CE4E5B9ULL;
    stamp ^= stamp >> 27;
    stamp *= 0x94D049BB133111EBULL;
    return stamp ^ (stamp >> 31);
}
// PRODUCTION_CACHE
static std::vector<EiemObjectResourceCacheEntry> s_eiemMeshResourceCache;
static bool EiemFindModResource(const char *, const char *, const char *,
                               EiemModResource *) { return true; }
static unsigned GetFullPathNameA(const char *path, size_t size, char *out, void *) {
    strncpy_s(out, size, path, _TRUNCATE); return (unsigned)std::strlen(out);
}
static bool EiemResolveResourceDiskPath(const EiemModResource &resource,
                                        char *out, size_t size) {
    return GetFullPathNameA(resource.path, size, out, nullptr) != 0;
}
static uint64_t EiemMeshResourceFileStamp(const char *) { return 10; }
static bool EiemApplyMeshSubmeshVisibility(
    void *, const std::shared_ptr<EiemMeshSubmeshVisibilityState> &visibility,
    uint32_t mask, char *, size_t) {
    if (!visibility) return false;
    visibility->appliedMask = mask; ++visibilityCalls; return true;
}
static void *EiemBuildNativeMesh(const char *, void *, char *, size_t,
                                 std::shared_ptr<const EiemSkinIdentity> *identity,
                                 std::shared_ptr<EiemMeshSubmeshVisibilityState> *visibility) {
    if (s_eiemMeshResourceCacheLock.held) std::abort();
    if(identity) { auto value=std::make_shared<EiemSkinIdentity>(); value->paths={"Root/Foot"}; *identity=value; }
    if(visibility) { auto value=std::make_shared<EiemMeshSubmeshVisibilityState>(); value->triangles.resize(3); *visibility=value; }
    ++buildCalls; return failBuild ? nullptr : &tokens[6 + buildCalls];
}
// PRODUCTION_BUILDER
#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); return 1; \
} } while (false)
static void Add(uint32_t handle, void *object, uint64_t stamp = 10,
                const char *section = "MeshA") {
    EiemObjectResourceCacheEntry entry;
    nextHandle = handle - 1;
    entry.object = EiemUnityRef::Capture(object ? object : &tokens[6], false);
    entry.fileStamp = EiemMeshCacheStamp(stamp);
    auto identity=std::make_shared<EiemSkinIdentity>(); identity->paths={"Root/Foot"}; entry.skin=identity;
    entry.submeshVisibility=std::make_shared<EiemMeshSubmeshVisibilityState>();
    entry.submeshVisibility->triangles.resize(3);
    strcpy_s(entry.modPath, "mod.ini");
    strncpy_s(entry.section, section, _TRUNCATE);
    targets[handle] = object; s_eiemMeshResourceCache.push_back(entry);
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    const char *scenario = argv[1];
    const bool hit = std::strcmp(scenario, "hit") == 0;
    const bool deadThenHit = std::strcmp(scenario, "dead_then_hit") == 0;
    const bool expired = std::strcmp(scenario, "expired") == 0;
    const bool unrelated = std::strcmp(scenario, "unrelated") == 0;
    const bool nativeDead = std::strcmp(scenario, "native_dead") == 0;
    const bool nativeUnknown = std::strcmp(scenario, "native_unknown") == 0;
    unityThread = std::strcmp(scenario, "wrong_thread") != 0;
    const bool dead = std::strcmp(scenario, "dead") == 0 || deadThenHit;
    failBuild = std::strcmp(scenario, "build_failure") == 0;
    if (dead) Add(1, nullptr);
    if (expired) Add(1, &tokens[0], 9);
    if (hit || deadThenHit) Add(2, &tokens[1]);
    if (nativeDead) { Add(1, &tokens[0]); deadNative = &tokens[0]; }
    if (nativeUnknown) { Add(1, &tokens[0]); unknownNative = &tokens[0]; }
    if (unrelated) { Add(3, &tokens[2], 10, "MeshC"); Add(4, &tokens[3], 10, "MeshB"); }
    EiemModRule rule = {};
    rule.hiddenSubmeshMask = std::strcmp(scenario, "hidden") == 0 ? (1u << 2) : 0;
    void *mesh = &tokens[6]; char error[256] = {};
    const bool result = EiemBuildMeshResource(rule, &mesh, error, sizeof(error));
    CHECK(!s_eiemMeshResourceCacheLock.held);
    CHECK(s_eiemMeshResourceCacheLock.acquired == s_eiemMeshResourceCacheLock.released);
    if (!unityThread) {
        CHECK(!result && mesh == nullptr && error[0] && buildCalls == 0);
        CHECK(s_eiemMeshResourceCache.empty());
        return 0;
    }
    if (nativeUnknown) {
        CHECK(!result && mesh == nullptr && error[0]);
        CHECK(buildCalls == 0 && freed.empty());
        CHECK(s_eiemMeshResourceCache.size() == 1);
        return 0;
    }
    CHECK(result == !failBuild);
    CHECK(buildCalls == (hit || deadThenHit ? 0 : 1));
    CHECK(mesh == (failBuild ? nullptr : hit || deadThenHit ? &tokens[1] : &tokens[7]));
    CHECK(freed.size() == (dead || expired || nativeDead ? 1 : 0));
    if (!freed.empty()) CHECK(freed[0] == 1);
    CHECK(s_eiemMeshResourceCache.size() == (failBuild ? 0 : unrelated ? 3 : 1));
    if (unrelated) CHECK(targets[3] == &tokens[2] && targets[4] == &tokens[3]);
    if (!failBuild) {
        // Reusing the same resource must not build another Unity Mesh.
        void *again = nullptr;
        std::shared_ptr<const EiemSkinIdentity> identity;
        CHECK(EiemBuildMeshResource(rule, &again, error, sizeof(error), nullptr, &identity));
        CHECK(identity && identity->paths==std::vector<std::string>{"Root/Foot"});
        CHECK(again == mesh);
        CHECK(buildCalls == (hit || deadThenHit ? 0 : 1));
        CHECK(visibilityCalls == 2);
        CHECK(s_eiemMeshResourceCacheLock.acquired == s_eiemMeshResourceCacheLock.released);
    }
    if (std::strcmp(scenario, "hidden") == 0) {
        void *before = mesh;
        rule.hiddenSubmeshMask = 0;
        CHECK(EiemBuildMeshResource(rule, &mesh, error, sizeof(error)));
        CHECK(mesh == before && buildCalls == 1 && visibilityCalls == 3);
        CHECK(s_eiemMeshResourceCache.size() == 1);
        CHECK(s_eiemMeshResourceCache[0].variant == 0);
        CHECK(s_eiemMeshResourceCache[0].submeshVisibility->appliedMask == 0);
    }
    if (std::strcmp(scenario, "cycles") == 0) {
        for (int cycle = 0; cycle < 3; ++cycle) {
            deadNative = mesh;
            ++s_eiemModGeneration;
            CHECK(EiemBuildMeshResource(EiemModRule{}, &mesh, error, sizeof(error)));
            CHECK(mesh != deadNative && EiemNativeObjectStatus(mesh) == 1);
            CHECK(buildCalls == cycle + 2 && s_eiemMeshResourceCache.size() == 1);
            void *shared = nullptr;
            CHECK(EiemBuildMeshResource(EiemModRule{}, &shared, error, sizeof(error)));
            CHECK(shared == mesh && buildCalls == cycle + 2);
        }
        CHECK(freed.size() == 3);
    }
    if (std::strcmp(scenario, "generation") == 0) {
        void *before = mesh;
        ++s_eiemModGeneration;
        CHECK(EiemBuildMeshResource(rule, &mesh, error, sizeof(error)));
        CHECK(mesh != before && buildCalls == 2);
        void *again = nullptr;
        CHECK(EiemBuildMeshResource(rule, &again, error, sizeof(error)));
        CHECK(again == mesh && buildCalls == 2);
    }
    return 0;
}
'''


class MeshResourceCache(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("cl"):
            raise unittest.SkipTest("Requires MSVC developer environment (cl)")
        cls.temp = tempfile.TemporaryDirectory(prefix="eiem-mesh-cache-")
        cls.addClassCleanup(cls.temp.cleanup)
        folder = Path(cls.temp.name)
        source = folder / "mesh_cache_fixture.cpp"
        source.write_text(FIXTURE.replace("// PRODUCTION_CACHE", production_cache()).replace("// PRODUCTION_BUILDER", production_builder()),
                          encoding="utf-8")
        cls.executable = folder / "mesh_cache_fixture.exe"
        build = subprocess.run(
            ["cl", "/nologo", "/EHsc", "/std:c++17", f"/I{ROOT / 'src'}", str(source), f"/Fe{cls.executable}"],
            cwd=folder, capture_output=True, text=True, encoding="utf-8",
            errors="replace",
        )
        if build.returncode:
            raise AssertionError(build.stdout + build.stderr)

    def test_hit_miss_eviction_and_rebuild_keep_lock_and_handle_ownership(self):
        for scenario in ("empty", "hit", "dead", "dead_then_hit", "expired",
                         "unrelated", "build_failure", "native_dead", "native_unknown", "wrong_thread", "cycles", "generation", "hidden"):
            with self.subTest(scenario=scenario):
                result = subprocess.run([str(self.executable), scenario],
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
