# v80 lifecycle follow-up

This note records the first follow-up changes made on top of the recovered v80
baseline (`762f220`, `feat: 完成物理碰撞解析`). It is a source and offline-test
record; it is not gameplay acceptance.

## Update requests

The mod update bitmask is retained when the game window is temporarily
unavailable, when `PostMessage` fails, or when Unity renderer APIs have not
finished resolving. A short window timer retries the same request. Shutdown
clears the retry timer. This keeps F10, Reapply, and lifecycle reconciliation
from being silently dropped during a scene transition.

## LOD membership

Partner membership is reconciled in both directions. When a source Renderer is
present in a LOD level, its partner is added once. When the source is absent,
any stale partner is removed from that level. The change does not infer a
character, Mesh, LOD number, or switch name. The existing skin assembly
boundaries re-run this reconciliation after the game rebuilds its LOD arrays.

## Validation boundary

The offline suite passed after the change and `build.bat` completed. No claim is
made here about in-game teleport, hot reload, LOD switching, native Physics
completion, Animator writeback, or teardown safety. Those require a separate
runtime validation pass with the rebuilt DLL.

## F10 reload trace

The reported failure sequence is now recorded as: overwrite Mesh/INI/texture,
press F10, observe some parts at the scene origin, then encounter a stalled
scene transition. The current runtime has temporary in-game log markers for
the F10 restore, configuration load, per-model replay, and Physics
`BuildAndRun` boundaries (`[MOD-RELOAD-TRACE]`). These markers are written to
the normal plugin log and are not part of Dump. The next reproduction must
identify the last completed marker before changing the handoff order; existing
logs without an F10 marker cannot establish the cause.
### F10 ground-position reproduction: current evidence

The next run reproduced ground-positioned parts but did not reproduce the loading
stall. F10 issued Physics retirement requests for 930 registered models before
restoring Renderers, then replay created a fresh Physics runtime (generation=1)
with five colliders. All five collider records reported `parentMatch=1` and
`poseRead=1`; the runtime later reported readable, changing motion samples. The
ground symptom therefore persisted after a new runtime build. The earlier
hypothesis that an old Physics instance was still writing during F10 is not
established by this run and must not be treated as the cause.

The DLL now contains diagnostic-only `[LIFECYCLE-PROBE]` markers. They compare
source/Partner `SkinnedMeshRenderer.bones` arrays and Transform parents at
Partner creation, `AssignSkin`, `SetSmrRootBone`, and source `SetBones` calls;
they also record Skeleton cache hits and rebuilds. These probes do not assign,
release, or otherwise change runtime state. No further functional lifecycle
change is justified until one of these boundaries shows palette drift, a wrong
parent, or an unexpected Skeleton generation. Offline tests (117 total, 47
skipped) and the full DLL build pass; gameplay verification of the probes is
pending.

### Cold-start control run

The next captured log was built with the earlier probe DLL (`21:32:38`) and
contains no `[MOD-RELOAD-TRACE]` marker, so it does not cover the required
overwrite → F10 → key-1 sequence. In this cold-start/control-only run,
`cloth_02` Partners reported matching 126-slot bone arrays (`firstDiff=-1`)
and matching `rootBone` values at creation and `AssignSkin`. `cloth_01` Part4
reported 120 source slots versus 138 Partner slots, which is the expected
expanded palette for its 18 authored skirt bones. The log also contains 11
Partner control diffs with repeated retire/recreate cycles. These observations
rule out a static `cloth_02` palette mismatch for this run, but cannot explain
the post-F10 T-pose until the current generation probe captures that sequence.

### Control-only T-pose isolation

The same cold-start log narrows the ground-positioned T-pose to the Partner
control lifecycle rather than requiring F10. It contains no reload marker, but
it records eleven `mod control` reconciliations. Their retire/create pattern is
the exact three-state cycle for the clothing switch: two Partners are retired
and Part1 is created, then Part1 is retired, then Part8 and the `cloth_02`
Partner are created again. Repeating that pattern leaves the persisted clothing
state at 2, matching `state.ini` after the run.

The initial Partners are created before `BaseModelViewPart.OnLoadFinish` and
are subsequently present during the game's `AssignSkin` and root-bone assembly
callbacks. Partners created by a later control reconciliation appear after the
last recorded assembly callback and receive no corresponding game assembly
pass. Their public `bones` arrays and `rootBone` values are assigned by EIEM,
but the log does not establish that the game's internal skin/Animator
registration was performed for these late-created Renderers. This is the first
ranked cause of the T-pose. The INI evaluator selected the expected sections,
shape-only handling was not used by this switch, and no Mod UI transaction was
involved. A behavior change should therefore target Partner activation and
reuse at the model assembly boundary, not add another parser, shape, or UI
special case.

### Partner visibility lifecycle

Conditional partner links now control visibility instead of Renderer lifetime.
The runtime enumerates every partner section referenced by every branch of a
source Render and constructs those Partners during the source model's initial
assembly. Inactive choices remain attached with their Renderer disabled and
outside the source LOD levels. A key transition only changes
`Renderer.enabled` and LOD membership; it does not erase the Partner state,
release its Skeleton lease, deactivate/detach its GameObject, or call
`Object.Destroy`. Full Mod reload and owner teardown retain their separate
retirement paths because they end a resource/model generation.

This removes late `SkinnedMeshRenderer` creation from ordinary key switching.
The full DLL build and the offline suite pass (120 tests, 47 skipped). Runtime
validation still needs to confirm that repeated clothing cycles retain the
same Partner addresses and no longer produce a ground-positioned T-pose.

### F10 Partner generation rollover

The F10 path previously retired every Partner before replaying the registered
model. That replay ran after the game's model/skin assembly, so each newly
created Partner had public `bones` and `rootBone` values but no evidence of the
game's internal Renderer registration; this matched the reported post-reload
ground-positioned T-pose. F10 now keeps live Partner GameObjects and refreshes
their Mesh, Skeleton bone array, materials, shape state, and visibility in
place. Partners are still destroyed for model/prefab teardown, and a removed
Partner declaration is retired during replay. This is an implementation fix,
not gameplay acceptance; the next in-game run must verify that Partner
addresses remain stable across F10 and that no part loses its pose.

### Reconcile wake-up and the former Physics polling path

Lifecycle callbacks and hotkeys share one update bitmask. A pending bitmask now
owns one `WM_EIEM_MOD_RECONCILE` wake-up token; callbacks that arrive while
that token is queued only merge their request. The window handler clears the
token before taking the bitmask so a request arriving during a reconcile posts
one follow-up message. This prevents scene assembly from filling the game's
window queue with duplicate reconciles.

This section records the intermediate v85 implementation. It was superseded by
the v86 event-boundary change below: the 250 ms poll, candidate scan, and
`[PERF] physics-periodic` records were removed rather than merely filtered.

### v85 source-skin baseline and scheduled-work consolidation

The previous F10 baseline was captured when EIEM first replaced a Renderer.
Endfield can finish `bones` and `rootBone` assignment later. The bones hook
reasserted EIEM's replacement palette after those game writes, but it did not
save the incoming game palette as the next restore baseline. F10 could therefore
restore an early construction palette. That explains why a cold start can be
correct while a later reload leaves a timing-dependent subset of Partners in an
unanimated/T-pose binding.

Tracked source Renderers now retain the latest game-owned `bones` and
`rootBone` values separately from the EIEM replacement palette. The individual
`SkinnedMeshRenderer.set_bones` hook records the incoming game value before
reasserting the replacement. `AssignSkinPost` and `SetSMRRootBone` also refresh
the source baseline for direct game writes that bypass the public setter. F10
continues to restore from `originalBonesHandle`/`originalRootBoneHandle`, which
now mean the last observed game baseline rather than the first construction
snapshot. This is keyed by live Renderer relationship and contains no character,
Mesh, bone, LOD, or switch name exceptions.

The reload retirement pass previously called the Physics release adapter for
every registered model (930 in the reproduction), even though almost all models
had no Physics intent. It now submits only models with nonempty Physics intents.
The 250 ms poll snapshots only Physics candidates and performs native collection
and failure pruning once per poll, rather than once globally and again per model.
Slow polls of at least 4 ms emit a bounded `[PERF] physics-periodic` record to the
normal runtime log. This is historical evidence from v85; v86 removes that
polling path. Reconcile requests retain one posted-window-message token, so
lifecycle callbacks merge into the pending bitmask instead of flooding the
window queue.

The MSVC test suite passed 247 tests with 12 environment-dependent skips, and
`build.bat` completed. The deployed build marker is
`resource-runtime-v85-coalesced-skin-baseline`; SHA256 is
`304531E6FFA14FF8A78EEF9303537A6B5A1696FEB37C8415B117DC2C63AB5B44`.
The replaced DLL is backed up on `E:` at
`E:\EIEM_Workspace\plugin-releases\before-v85-coalesced-skin-baseline-20260913-005856`.
Deployment did not modify Mod configuration, persistent state, Mesh, material,
texture, Skeleton, or Physics resources. Gameplay verification is pending; the
next run must confirm the build marker, one F10 reload trace, stable Partner pose,
and whether any bounded periodic performance records appear.

### v86 event-boundary Physics lifecycle

The 250 ms Physics candidate poll and its call from every window message were
removed. Physics reconciliation now runs only at explicit model/owner
lifecycle callbacks and inside the Unity-thread F10 transaction. The boundary
drains off-thread release notices, collects Unity objects whose deferred
`Destroy` has completed, and performs one readiness observation. A model
boundary collects the previous generation before comparing its new intent; it
does not scan every registered model and it does not sample bone motion in the
background.

F10 no longer releases every Physics host before restoring Renderers. An
unchanged intent is retained by a resource stamp composed from the Physics
file and its Skeleton dependency; a changed or removed intent retires its old
host (changed on-disk assets are marked before Renderer restoration) and is
rebuilt only after the Unity destruction boundary is observed.
`BuildAndRun` remains asynchronous, so the one-time readiness check is an
observation and never a completion fence. If Unity has not completed deferred
destruction in the same F10 transaction, the replacement is left pending for
the next explicit lifecycle boundary rather than creating a second native
writer against the same bones.

LOD matching also treats Unity `Renderer.forceRenderingOff` as suppression when
that optional property is available. This prevents a source Renderer's
inactive LOD sibling from being treated as a live Mesh hit while preserving the
existing owner and Partner visibility rules. The new build marker is
`resource-runtime-v86-event-boundary`; no in-game F10, teleport, LOD, Physics,
Animator, or teardown acceptance is claimed by this source change alone.

### v92 ground-pose investigation: path identity before array order

The remaining ground/T-pose report cannot be diagnosed from the earlier
`firstDiff` value. That value compares two `SkinnedMeshRenderer.bones` arrays by
slot, while EIEM intentionally resolves a replacement palette by the Mesh's
resource-local bone paths. A differing slot therefore does not identify a bad
bone. The v92 diagnostic records the actual Mesh path mapping, counts Partner
entries that resolve to an `EIEM_Bone_*` private Transform, and records source
nodes that were absent from the live hierarchy. It also reuses an exact
Animator-owned Transform found in the source palette ancestry before creating a
private node; this does not guess by slot or by character name.

The v92 build marker is
`resource-runtime-v92-ground-diagnostic`; it was built and deployed locally,
but no gameplay result is claimed until a fresh run produces the
`[SKELETON-BIND]` and `event=partner-skin-map` records.

### v93 ground-pose identity probe

The v93 diagnostic does not change Mesh, LOD, key-switch, Skeleton, or Physics
behavior. It extends the bounded binding records with the concrete
`EiemSkeletonInstance` pointer, anchor, node/owned-node counts, and the first
Physics-selected Transform. The visible-binding record also includes the
Partner-side Skeleton instance and the first selected author path. These fields
are needed to distinguish a split Render/Physics Skeleton generation from a
single generation whose selected nodes are not present in a visible Mesh
palette. `build.bat` and the 119-test offline suite passed; the DLL was
deployed to the game's `plugin\eiem.dll` with a backup under
`E:\EIEM_Workspace\plugin-releases\before-v93-ground-diagnostic-20260913-072736`.
No gameplay result is claimed until a fresh cold start and one F10 reproduction
produce the v93 marker and corresponding records.

### v96 cold-start Partner registration probe

The latest report changes the cold-start diagnosis: the ground-positioned Mesh
is not consistently the Physics target and can change between launches. The
common factor is a key-controlled Partner. v93 already showed that the Physics
host was built and that its selected Transforms were present in the visible
Partner palette, but it did not show whether those Partner Renderers were part
of the arrays consumed by the game's skin/Animator assembly.

The v96 probe records that missing boundary without changing behavior. At
`CreateSMSGO`, `CreateSMSInfoForPostModel`, `AssignSkinPost`, and
`SetSMRRootBone`, it compares the concrete source and Partner Renderer
pointers with the managed Renderer array received by the game. The record
reports `sourceHits`, `partnerHits`, and the first currently visible Partner
that is absent. It is deduplicated by callback boundary, array, and Mod
generation, so it does not poll each frame or create another lifecycle path.

Interpretation for the next cold run:

* `partnerHits=0/...` at the initial skin/Animator boundary means the public
  `bones`/`rootBone` assignment was never accepted into the game's registration
  array. A key-selected Partner can therefore remain in a bind/T-pose even
  though its Transform pointers look valid.
* `partnerHits=.../...` for the initial boundary moves suspicion to the
  per-Partner palette/bindpose data or a later game write that excludes a
  selected Partner. The existing `partner-skin-map` and `partner-bone-diff`
  records then identify that branch.
* A missing Partner only after a key transition would confirm a late
  registration path. That is distinct from Physics creation and from LOD
  membership.

The v96 DLL must be tested with a cold start and the persisted key state left
unchanged long enough to capture the initial boundary. No fix or in-game
acceptance is claimed until those array-members records are present.

The first v96 cold-start capture with a visibly grounded
`MeshS_actor_typhoea_cloth_01_lod0_2.qun` resolves that boundary. For the
target LOD0 model, both source Renderers were present in the 17-entry game
skinning array, while all 12 pre-created Partner Renderers were absent. All
seven Partners selected by the current key state were absent as well. The same
array then entered `AssignSkin` unchanged. Public `bones`, `rootBone`, parent
pose and LOD membership can therefore look correct while the game has never
accepted the Partner into its internal skinning assembly.

v97 added one bounded diagnostic before any registration mutation: only when a
known Partner source is found, it records the managed types and lengths of the
Renderer array and the parallel `rootBones`/`rootBoneInfos` input, together
with the concrete source index. The capture proved that the existing
`CreateSMSInfoForPostModel` output can be extended as an index-preserving pair.

v98 now performs that extension at the same boundary. It appends every known
Partner whose source Renderer is in the returned array, duplicates the source
`RootBoneInfo` value at the matching index, and publishes both new arrays
before `AssignSkin` runs. It is data-driven by the source/Partner relationship;
keys still change only visibility. Build and offline contract tests do not
prove in-game Animator acceptance, so a fresh run is still required.

The first v98 capture exposed an implementation error: the new Renderer slots
had copied the source pointers instead of the Partner pointers. v99 corrects
that copy at the same boundary; this distinction is recorded separately so a
later `partnerHits` result can be attributed to the actual deployed code.


### v100 early Partner owner binding

The v98/v99 ordering records show that `RendererInfo._Init` can create the
Partner before `BaseModelViewPart.OnLoadFinish` or `NPCAvatar.StartNPC` registers
the model owner. The direct Renderer path has no active model owner, so a
newly created Partner can otherwise retain `ownerPrefabInstance=0`. The
model-scoped replay now binds an existing Partner to the active model owner
when it reuses that component. This keeps key visibility per live Renderer,
while making owner teardown able to retire the Partner with its source model.

This is a lifecycle fix only; it does not assume that a public bone setter or
`DisposeInternal` is an Animator completion fence. A fresh cold-start run of
v100 followed by one F10 reload is required to correlate owner binding with
the reported ground pose. The previous installed log was v98 and predates the
v100 DLL.


### v101 Partner bone-setter observation

The grounded-model report remains unresolved after v100. Existing traces
record Partner creation, parent pose, public `bones` assignment, and source
array registration, but they did not record a later game-owned
`SkinnedMeshRenderer.set_bones` call on the Partner itself. That leaves two
different causes indistinguishable: a game skin/Animator refresh can replace
the extended palette after F10/key activity, or the Partner can keep its
palette while the failure is elsewhere.

v101 adds one observational probe to the existing setter hook. For each known
Partner and generation it records the incoming array length, the array read
back after the original setter (when the hook is on Unity's thread), the
expected length published for that Partner, source/section, visibility, and
thread id. A separate de-duplicated mismatch record is emitted only when a
known expected palette is replaced by a different-length array. The probe is
skipped for our own guarded F10 assignment and does not call a repair or
destroy path.

This is not evidence that the game's Animator accepts the Partner. A cold
start plus one F10 reproduction is required. A Partner setter event or
mismatch immediately before the grounded pose would support the late-write
hypothesis; no Partner setter event with a correct palette moves the next
inspection to the game's internal Animator/skin registration boundary.
