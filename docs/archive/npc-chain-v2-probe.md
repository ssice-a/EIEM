# NPC Chain V2 Probe

This probe is read-only. It does not replace Mesh or materials, change bones or
rootBone, create Partner objects, or run Physics changes.

## Correlation

`NPCAvatar.StartNPC` records the completed owner fields:

- `npcId` and `proxyId` from `NPCCrowdEntityComponent`
- `avatarMeshConfig`, `avatarTempletAssetSo`, and `avatarGoRef.go`
- `mainPrefabPath`, `mainPrefabPathHash`, and the loaded prefab name
- each `NPCAvatarLodMeshAssets` entry in `avatarSlotMeshDatas`

The probe keeps a bounded process-local map from each concrete
`NPCAvatarLodMeshAssets` pointer to its Avatar/component/model owner. Release
boundaries remove those links before pooled objects can be reused.

`CreateSMSGO` and `CreateSMSInfoForPostModel` then record whether their concrete
`INPCMeshAssets` pointer matches that map, together with the output Renderer
array and LOD. The records use the prefix `[NPC-CHAIN-v2]`:

- `event=start` and `event=mesh-config` establish the owner and asset table.
- `event=start-thread` marks a `StartNPC` call that completed on a worker
  thread; its raw managed fields are still indexed, while Unity-only calls are
  intentionally skipped.
- `event=asset` identifies Typhoea asset entries.
- `event=render` proves an asset-to-owner-to-Renderer association.
- `event=render-unmatched` identifies a missing association without guessing.
- `event=forget` records lifecycle cleanup.

## Deployment

The probe DLL currently deployed at the game plugin directory has SHA-256:

`C9DC29EE2B16A24540F2AEE490BCA7DB59333776BBE29E06635DFD950FE4A70D`

The Typhoea `mod.ini` remains commented for this run. Start the game and enter
a scene containing a Typhoea NPC. The next analysis should compare `start`,
`asset`, and `render` records for the same `avatar` and `meshAssets` pointers.

## Observed Run (2026-09-15)

The new worker-thread probe established the chain for
`chr_0034_typhoea_postmodel`:

- The first `CreateSMSInfoForPostModel` calls for LOD 0-3 occurred before the
  first `StartNPC`, so those four records were expected
  `render-unmatched` records.
- `StartNPC` then read `avatarMeshName=NPC/Ability/Humanoid/girl`, one
  `meshConfig` slot, and `assetName=chr_0034_typhoea_postmodel` from the
  worker thread.
- The subsequent LOD 0-3 calls were all `event=render` records with counts
  `17/17/15/13` and the same `meshAssets` pointer.
- A second Typhoea avatar registered the same
  `meshAssets=000000101171CB40`. This is shared resource state, not a unique
  per-instance object; an index keyed to one owner per asset pointer would
  overwrite the first owner.
- The world character prefab reported 72 renderers while the UI prefab
  reported 17, but both reported the same runtime Mesh pointers for the LOD0
  Typhoea parts (for example `S_actor_typhoea_cloth_01_lod0` at
  `0000000FC9B00FA0`). The prefab instance and the shared Mesh resource are
  therefore separate identities.

The next probe revision should therefore treat the asset name/prefab path as
the resource identity and retain a set of owners only for diagnostics. It must
also accept resource creation before an owner callback instead of treating the
early calls as a failed instance association.

## Targeted Resource Probe (2026-09-15)

The previous build had all generic `TraceTakeBudget` calls disabled, so the
resource hooks were installed but emitted no runtime load records. The current
build keeps the original load results and enables bounded logging only when a
path, sub-asset name, asset completion name, or final Mesh identity contains
`typhoea`. Hash-based loads first consult the game's `StringPathHash` mapping
and only resolve it on Unity's thread. This keeps the evidence pass readable
without reopening VFS, bundle, or per-frame setter logging globally.

Deployment backup: `E:\EIEM_Workspace\plugin-releases\before-resource-trace-target-20260915-231905`.
The Typhoea `mod.ini` remains fully commented and no replacement, Partner,
Skeleton, Physics, LOD, or lifecycle mutation is enabled.

## Public Loader Probe (2026-09-16)

The deployed revision adds observation-only hooks for the `void` callback
overloads of `BundleResourceManager.LoadAsync` and `LoadSubAssetAsync` using
String and `StringPathHash` identities. Each record includes the log channel,
logical path/hash, requested type, root category, priority, and callback
address. The callback itself is never invoked or wrapped. These overloads are
safe to detour because they do not return the `FAssetProxyHandle` value type.

The next restart should establish whether the target resource request reaches
one of these public loader boundaries. If no Typhoea records appear, the next
candidate is the GUIDProxy overload family; no GUID value-type detour is
enabled until its ABI is confirmed.

## Concrete Loader Probe (2026-09-16)

The public manager callbacks were installed but produced no runtime request
records. The next build therefore also observes the concrete
`SimpleAssetLoader` and `MonoEntitySimpleAssetLoader` StringPathHash callback
overloads used by `IAssetLoader`. These hooks are also `void` callbacks and
leave the loader and callback untouched. Deployment backup:
`E:\vscode\EIEM\backups\probe-concrete-loader-20260916-011746`.

## Synchronous TryLoad Probe (2026-09-16)

The next revision observes the concrete `TryLoad(StringPathHash, Type, out
FAssetProxyLoaderHandle)` methods on `SimpleAssetLoader` and
`MonoEntitySimpleAssetLoader`. The out handle avoids guessing the ABI of the
value-type return used by `Load(...)`; the original result and handle are
passed through unchanged. The probe records the hash, resolved path when
available, requested type, result, and handle path. No resource or Renderer is
modified.

Build SHA-256 (safe output-handle read):
`E7DEC9D511734AF02192887C890869C25AEFF5B89D4E6E944EC07BF408E0A4A6`

Deployment backup:
`E:\vscode\EIEM\backups\probe-sync-tryload-safe-20260916-013549`.

## Cached Loader Probe (2026-09-16)

The next revision also observes `CachedPathAssetLoader.LoadDirect(string,
Type)` and `TryLoad(string, Type, out FAssetProxyLoaderHandle)`. These are
read-only boundaries; the original object, bool result, and output handle are
unchanged. Records are limited to Typhoea paths or returned objects so normal
resource traffic stays out of the log.

Build SHA-256:
`FA80D5A4B7CB8EC3544DF6DA0298C63F89263C6C279BE7746825EE2FA07CDF80`

Deployment backup:
`E:\vscode\EIEM\backups\probe-cached-loader-20260916-081316`.

## Prefab Context Probe (2026-09-16)

The next read-only revision keeps more than one owner link for a shared
`NPCAvatarLodMeshAssets` pointer. It also records the `prefabItems` entries on
`NPCAvatarMeshAssetsSO`, the concrete `IAssetLoader` object passed to
`CreateSMSGO`, and the parent Transform passed to both `CreateSMS*` methods.
These records use the prefix `[NPC-CHAIN-v3]` and do not modify any managed or
Unity object.

The previous restart already showed two Typhoea avatars sharing
`meshAssets=0000000FA247C0C0` while their model and Renderer pointers differed.
The old one-link index therefore could select the last owner arbitrarily; this
revision treats that as an ambiguity and includes the concrete parent in the
record instead of presenting a false per-instance association.

Build SHA-256:
`981B273D6ABCDEF6D484D04CEA152028626881676389C3BFC4F71C86D87B8F95`

Deployment backup:
`E:\vscode\EIEM\backups\probe-prefab-context-20260916-082619`.

## V1.2 Runtime Result (2026-09-16)

The restart loaded the current build (`dll=Sep 16 2026 08:25:26`) with the
configuration still empty (`0 Prefab declarations, 0 Render rules, 0
resources`). The read-only probe produced 144 `NPC-CHAIN-v3` records, including
eight `create-sms` records for `chr_0034_typhoea_postmodel`.

The Typhoea NPC path exposed two distinct owners:

- `avatar=000000100C557000`, `model=0000000FAAC5DC00`
- `avatar=0000000FBFDB4D20`, `model=0000000FBF27A4E0`

Both owners use `meshAssets=000000100961D480` and the same
`meshConfig=0000000FBF81FA00`. Their `CreateSMSInfoForPostModel` calls return
different parent Transform pointers and different Renderer arrays. The first
four calls (LOD 0-3, counts `17/17/15/13`) happen before the first `StartNPC`
and correctly report `ownerCount=0`. The next four calls use another parent and
another set of arrays, but occur after only the first `StartNPC`; because the
probe selects the first matching link for a shared asset pointer, they report
`ownerCount=1` and temporarily show the first Avatar. The second `StartNPC`
arrives afterwards and adds the second link. This is a probe correlation issue,
not evidence that the game sent the second Renderer array to the first model.

The concrete Renderer arrays reach `AssignSkinGoPost` unchanged with the
expected Typhoea names and LOD counts. `parentName` is empty because these
callbacks run off the Unity thread; the raw parent pointer is still distinct
and must be used for correlation. No Mesh, material, bone, Partner, Physics,
LOD or visibility state was written.

No `prefab-item` record was emitted. The class dump confirms that
`NPCAvatarMeshAssetsSO.prefabItems` is field `0x40`, but the runtime list is
empty/null at the direct field read (the populated `GetAllPrefabItems()` result
has not been observed). This does not invalidate the `mainPrefabPath` and
`meshAssets` evidence; the next probe should log the list pointer/count and, if
needed, observe the getter without changing its result.

## Instance-Key Probe (2026-09-16)

The next read-only build keeps the same empty Mod configuration and adds only
diagnostic fields. `NPCAvatar.StartNPC` now records the inline
`FNPCAvatarGOReference` address and its `animator`, `go`, `lodGroup`, and
`parts` fields. `CreateSMSGO` and `CreateSMSInfoForPostModel` now record the
concrete `NPCGoPool` pointer and managed types for both the pool and parent
Transform, plus the callback thread and Unity-thread flag. No original result,
Renderer array, Mesh, material, bone, Physics, LOD, or visibility state is
changed.

Build and deployed SHA-256:
`6F101A6E5DC81312DE095FAD5AAB1ABBF0EC43F4A37E8B21136330B4C24223F9`

Deployment backup:
`E:\vscode\EIEM\backups\probe-instance-key-20260916-090338`.

The next startup should compare the two Typhoea groups by `(parent, goPool,
array)` and compare those values with each Avatar's `avatarGo` and
`avatarLodGroup`. A shared `meshAssets` pointer alone must remain classified as
resource identity; it is not evidence of an instance collision.

The next correlation key is therefore the concrete parent Transform (or the
parent's owning GameObject) paired with the Avatar's `avatarGoRef.go`, while the
shared `NPCAvatarLodMeshAssets` pointer remains a resource identity only.

## Instance-Key Probe Runtime Result (2026-09-16 15:40)

The deployed build was loaded successfully (`dll=Sep 16 2026 09:02:40`) with
the Mod configuration still empty. The log was freshly recreated and contains
144 `NPC-CHAIN-v3` records.

For `chr_0034_typhoea_postmodel`, the event order is decisive:

1. LOD 0-3 are created with `parent=0000000FC3F9B7E0` and no owner link.
2. The first `StartNPC` then registers
   `avatar=00000011D3C5D2A0`, `avatarGo=0000000FC3FA9F00`, and
   `avatarLodGroup=00000011D3C779C0`.
3. A second LOD 0-3 group is created with
   `parent=00000011D4A382E0` and different Renderer arrays.
4. The second `StartNPC` registers
   `avatar=00000011D4A36D20`, `avatarGo=00000011D4A49A40`, and
   `avatarLodGroup=00000011D4A35D80`.

Both groups use the same `meshAssets=000000100EE50780`. The second group is
temporarily reported as belonging to the first Avatar because its resource
creation callback runs before the second `StartNPC`; the parent and array are
already distinct. This is a limitation of the diagnostic join, not evidence of
game-side instance crossover.

The two parent values are stable per creation group and are the only concrete
instance-side key currently present at `CreateSMSInfoForPostModel`. The next
probe must resolve the parent Transform to its owning GameObject on the Unity
thread, or observe the later native registration that binds that parent to the
Avatar. It must not apply replacement rules from the shared `meshAssets` pointer
alone.

## Pre-Start Correlation Probe (2026-09-16)

The next diagnostic revision primes the `(meshAssets, avatar, component)` link
before calling the original `NPCAvatar.StartNPC`. This matches the actual call
order: the original method creates the Renderer arrays internally, while the
existing trace registered the owner only after the original returned. The probe
remains read-only and the Mod configuration remains empty.

Build and deployed SHA-256:
`5B3475C02BAC7780CC81A17CFEFAE35A9DFE8EFE950CE2A5F1809A4C0D4F1C5B`

Deployment backup:
`E:\vscode\EIEM\backups\probe-pre-start-20260916-154812`.

The next run should show `event=pre-start` before each Typhoea
`CreateSMSInfoForPostModel` group. The expected result is that each group is
associated with its own Avatar immediately, without an `ownerCount=1` false
selection caused by a late post-call registration. This tests probe correlation
only; it does not yet test resource replacement.

## Pre-Start Result and Active-Call Probe (2026-09-16)

The pre-start revision loaded successfully (`dll=Sep 16 2026 15:47:41`), but
the Typhoea asset table was empty before the original `StartNPC` entered its
resource creation path. Consequently no `event=pre-start` records were
emitted. The first Typhoea LOD 0-3 group was created before the post-call
`StartNPC` record, and the same pattern repeated for the second group.

The runtime also confirmed that `CreateSMSInfoForPostModel` and `StartNPC`
execute on the same worker thread (`34436`) for these instances. The next
revision therefore keeps a thread-local active `StartNPC` context while the
original method runs. A `CreateSMS` callback can bind its concrete
`meshAssets` pointer to that active Avatar without using an uninitialized asset
table or a shared resource pointer as an instance key.

Build and deployed SHA-256:
`B6175EFA6113C6421B115A6803CABAA367DC5DCAC32A65D23CBAB35871BD3B75`

Deployment backup:
`E:\vscode\EIEM\backups\probe-active-start-20260916-160237`.

The acceptance condition for the next run is an `active-start-bind` before
each Typhoea Renderer group, followed by a `render` record naming the same
Avatar and parent/array group. No replacement, visibility, Partner, Skeleton,
or Physics mutation is enabled.

## Active-Call Result and CreateAvatar Probe (2026-09-16)

The active-call build loaded successfully (`dll=Sep 16 2026 16:01:36`), but
the run produced zero `active-start-bind` records. The four LOD groups for the
first Typhoea creation were emitted before the post-call `StartNPC` record, and
the four groups for the second creation were emitted after the first
`StartNPC` but before the second one. Therefore those `CreateSMS` calls are
outside the `StartNPC` invocation; keeping a thread-local `StartNPC` context
cannot identify their owner.

The groups still have distinct `parent` and Renderer-array pointers, while
the two creations share one `NPCAvatarLodMeshAssets` pointer. The current
`meshAssets`-only join consequently reports the second group as the first
Avatar. This is a probe limitation, not evidence that the game crossed the
two instances.

The next read-only probe wraps
`NPCAvatarManager.CreateAvatar`, which owns the construction parent and
returns the concrete Avatar after the Renderer groups are built. It records
the `(parent, meshAssets, LOD mask)` groups during that call, then binds them
to the returned Avatar. It does not mutate any Unity object or enable any Mod
rule.

Build and deployed SHA-256:
`C2032EE2FC92DD4CB15030DA3CA482B9ADF635D5618546D9FE528AD8C83D5A6A`

Deployment backup:
`E:\vscode\EIEM\backups\probe-create-avatar-20260916-162520`.

## CreateAvatar Probe Result and Clean Rerun (2026-09-16)

The probe installed and captured two Typhoea `CreateAvatar` calls. Its
`parent` argument was null on both calls, so that parameter is not the
instance key for this path. The first run also exposed a probe-only defect:
the thread-local group slot was reused without clearing its previous groups,
which inflated the binding count and inserted component-less observations into
the normal owner index. Those records were diagnostic state only, but they
made the result unusable.

The probe was corrected to clear each slot on entry and to keep a
component-less `CreateAvatar` result out of the render-owner index. The
complete component link remains established at `StartNPC`, while the
creation-side event records the returned Avatar and its Renderer groups.

Clean build and deployment SHA-256:
`1D9D564F2687ED779C285715871D24A2ACDBF76AD6DF47EF265C1C152988F103`

Deployment backup:
`E:\vscode\EIEM\backups\probe-create-avatar-clean-20260916-164706`.

## Clean CreateAvatar Result and Descriptor Boundary (2026-09-16)

The clean run produced the expected one group per Avatar:

| creation | returned Avatar | parent used by `CreateSMS` | mesh-assets | LOD mask |
|---|---|---|---|---|
| 1 | `0000000FC81F8A80` | `0000000FC7FE2C40` | `0000000FC7E7E480` | `0xF` |
| 2 | `00000011E07AF7E0` | `00000011E07F27C0` | `0000000FC7E7E480` | `0xF` |

The later `StartNPC` records use exactly those Avatar and model identities.
This establishes the NPC instance join: the shared `meshAssets` pointer is a
resource identity, while the creation's parent/Renderer group belongs to the
returned Avatar.

The same run also shows the upstream descriptor boundary. For both creations,
`NPCAvatarMeshAssetsSO.GetAvatarSlotMeshAssets` returns the same one-item slot
list, and `GetSubMeshInfo` returns the same descriptor array and the same
`SubMeshInfo` pointers. The descriptor records contain stable mesh path hashes
while their `mesh` references are null; `CreateSMSInfoForPostModel` then emits
different parent and Renderer-array objects for each Avatar. Thus the safest
resource-level replacement candidate is before `CreateSMS` consumes the
descriptor path/hash input, not a downstream Renderer or Partner.

The current run remains read-only: no descriptor fields, Renderer arrays,
materials, visibility, Partner, Skeleton, or Physics state are changed.

## Shared Mesh Identity Across NPC and UI (2026-09-16)

The same run also loaded the UI prefab
`Assets/Beyond/DynamicAssets/Gameplay/Prefabs/UIModels/chr_0034_typhoea_uimodel.prefab`.
Its `PREFAB-RENDERER` records use the same concrete Mesh pointers and names as
the world character prefab, including `S_actor_typhoea_body_01_lod0`,
`S_actor_typhoea_cloth_01_lod0`, and the remaining LOD-0 parts. NPC creation
resolves the same logical part set from `SubMeshInfo.meshPathHash` before
building its own Renderer arrays.

This is the first runtime evidence that the three presentations converge on
shared Mesh resource identity while keeping separate Renderer/Avatar instances.
The next implementation probe should therefore observe or replace the shared
resource resolution once, then verify that world, UI, and NPC consume the
result. It should not create a second Renderer or copy per-instance bone state.

## Resource-to-Renderer Pair Probe (2026-09-16)

The deployed read-only probe records `[NPC-RESOURCE-PAIR]` at the return of
`CreateSMSGO` and `CreateSMSInfoForPostModel`. For each Typhoea LOD it joins the
descriptor's `meshPathHash` and name with the concrete Renderer and Mesh at the
same index. It reads the existing LOD descriptor array (`0x68..0x80`) and the
Renderer array returned by the game's factory; it does not call a setter,
extend an array, create a Partner, or alter visibility, materials, skeleton,
physics, or lifecycle state.

This is the final read-only check before implementing a resource redirect. The
expected result is a stable name/hash-to-Mesh mapping for all three consumers.
If that holds, the replacement boundary is the shared descriptor/resource
resolution feeding `CreateSMS`, while the game retains Renderer creation,
skinning, LOD and animation ownership.

## Pair Probe Result (2026-09-16)

The first clean run after deployment produced 124 Typhoea pair records. They
cover two NPC instances and LOD 0-3 (17, 17, 15 and 13 descriptors per LOD).
There were 62 unique descriptor hashes; every record had the same descriptor
name and concrete Mesh name, with zero name mismatches and zero hashes mapping
to more than one Mesh pointer. The second NPC instance reused the same Mesh
objects while receiving different parent Transforms and Renderer arrays.

This closes the NPC resource identity question. A replacement keyed by the
logical Mesh path/hash can be shared by instances without sharing their
Renderer, Animator or lifecycle state. The world and UI prefab observations in
the same run use those same concrete Mesh pointers for `body_01_lod0` and
`cloth_01_lod0`. No additional NPC instance probe is required before moving to
the resource redirect implementation.

The same run recorded no target calls through `BundleResourceManager`,
`AssetBundle.LoadAsset`, `FAssetProxyHandle`, or the asset-loader `TryLoad`
hooks. The game's currently loaded PFB data is therefore already resolved
before those observation points. The implementation must keep one logical
resource-rule table but apply it at the game-owned assembly consumers: the
world/UI `EntityRenderHelper._InitRenderAndMaterial` pre-scan boundary and the
NPC `CreateSMS*` return boundary before `AssignSkin`. Both paths operate on
the existing Renderer and Mesh objects; neither path needs a second Renderer,
Partner, or copied bone state.
