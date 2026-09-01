# EIEM Model Replacement Design

Status: extraction/browser vertical slice implemented; runtime Mesh/material
binding backend implemented but not yet live-game validated

## Current Runtime Backend (2026-08-31)

The plugin parses `plugin/mods/<mod>/mod.ini` into reusable resource records
and Render records. A Render record combines two responsibilities: its
identity fields (`path`, `asset`, and optional `match.*`) find the source
Renderer, while its resource fields modify that source Renderer. A Render may
also name one or more `partner.N` Render records; those records never match a
source and are created as additional Renderers with the source object's
lifecycle.

```text
[Mesh...].path       -> EIEMESH binary written by AnimeStudio/Blender
[Material...].path   -> EIEMMAT text file
[Render...].path     -> source Mesh logical path (resource match key)
[Render...].asset    -> Mesh sub-resource name inside that container
[Render...].match.*  -> original Mesh structure fingerprint
[Render...].mesh     -> Mesh resource section
[Render...].material.N -> Material resource section
[Render...].partner.N -> additional Render section, never a match rule
```

On `SkinnedMeshRenderer.set_sharedMesh` or `MeshFilter.set_sharedMesh`, a
matching Render record now performs the following work on Unity's own thread:

1. Reads `EIEMESH` v1 and validates vertices, normals, tangents, colours, eight
   UV channels, triangle submeshes, skin weights and bind poses.
2. Constructs a real `UnityEngine.Mesh` with the game's IL2CPP APIs and assigns
   it to the existing renderer. Reusing the renderer deliberately preserves the
   game-owned transform, Animator binding, bones/rootBone, LOD selection,
   culling and entity lifetime.
3. Reads each requested `EIEMMAT`, loads its `source=` Material through the
   already-running `BundleResourceManager`, clones it with `new Material(source)`,
   and applies exported `float.*`, `int.*`, `value4.*`,
   `texture_scale.*`, and `texture_offset.*` overrides before assigning
   `sharedMaterials`.
4. When the current game build exposes `Texture2D` plus
   `ImageConversion.LoadImage(Texture2D, Byte[], Boolean)`, reads a PNG declared
   by `texture.Property=TextureSection` and assigns it with `SetTexture`.
5. For a `partner.N` record, creates one child GameObject/Renderer, copies the
   source Transform and SkinnedMeshRenderer bone references, and adds the new
   Renderer to every `LODGroup` level that already contains the source Renderer.
   On reload it removes that reference before destroying the partner object.
   If a build does not expose `LODGroup.GetLODs/SetLODs`, partner creation still
   succeeds and the limitation is logged.

Generated Mesh, Material and Texture objects are cached by the mod file,
resource section and file timestamp. Reconciliation therefore reuses unchanged
objects instead of rebuilding them for every Renderer or F10 press. When a
resource file changes, the old generated object for that section is released
after the previous Renderer state has been restored, then the new object is
created and rooted for the next pass.

The implementation has compiled successfully but must not be described as an
end-to-end replacement until a deliberately small package is tested in-game.
One part remains explicit work rather than a hidden fallback: validating the
PNG API probe in a live Endfield build (the current static dump did not include
a confirmed `ImageConversion` declaration). EIEMESH BlendShape frames are now
decoded and reconstructed with `Mesh.AddBlendShapeFrame`; if that API is absent,
the replacement fails and the original Mesh remains active.

## Current offline browser implementation (2026-08-28)

The chosen inspection path is now a direct, indexed VFS browser inside the
normal AnimeStudio GUI. It does not load or decrypt the complete 53 GB VFS.

```text
Endfield .blc metadata + encrypted .chk ranges
    -> one-Bundle-at-a-time in-memory decrypt and Unity parse
    -> compact resource/CAB dependency index with checkpoint/resume
    -> unified virtual Container directory tree
    -> native AnimeStudio type filters and search
    -> user selects one resource
    -> read/decrypt its Bundle and transitive CAB dependencies on demand
    -> native Texture/Mesh/GameObject preview and selected export
```

The `.blc` parser and ChaCha20 implementation are local code and were verified
against the previous full extraction byte-for-byte. The completed direct scan
indexed 237,794 Bundles into 3,385,891 resource records and 237,794 CAB mappings
with zero failed Bundles. The compact `endfield_assets.eidx` is 702 MB, loads in
14.44 seconds and retains about 546 MiB of managed data. The old external XML
AssetMap and full plaintext `.ab` directory are no longer part of the Endfield
GUI workflow.

Prefab dependency traversal is included. Two real model Prefabs automatically
resolved closures of 13 and 19 Bundles. The export validation produced an FBX
with four meshes and seven PNG textures. This completes the VFS-to-Blender
extraction slice; Blender-to-runtime Bundle authoring is still a separate,
unfinished slice.

## 1. Objective

Add a runtime model replacement system for the Unity IL2CPP game without
modifying the original game installation. The plugin should redirect selected
game resources to user-provided assets and fall back to the original resource
when a replacement is unavailable or incompatible.

The first supported target is a same-skeleton visual replacement: preserve the
game character's Animator, Avatar, gameplay components, IK, cloth, colliders,
and entity lifecycle while replacing the rendered mesh and materials.

Whole-Prefab replacement is a later capability, not the first implementation
target.

## 2. Terms and asset relationships

### Prefab

A Prefab is a serialized Unity object graph or template. It may contain a
root GameObject, child Transforms, Components, Animator references,
SkinnedMeshRenderer components, material references, colliders, scripts, and
other serialized fields.

### Mesh

A Mesh is the geometry asset. For a skinned character it normally contains
vertices, indices, bind poses, bone weights, sub-meshes, and possibly blend
shape data. A Mesh does not contain the complete character behavior.

### Material and texture

Materials and textures are separate assets referenced by Renderer components or
by a Prefab. A material change does not inherently require changing the Prefab:

- Runtime assignment to `SkinnedMeshRenderer.materials` or `sharedMaterials`
  only changes the instantiated object.
- A persistent serialized change to the Prefab does require editing the Prefab
  or replacing the serialized asset that contains the reference.

The first implementation should keep the original Prefab and replace the
Renderer's Mesh/material references at runtime.

## 3. Proposed runtime architecture

```text
Game resource request
    -> game ResourceManager / Addressables / native resource API
    -> EIEM replacement decision
       -> replacement artifact, if present and compatible
       -> original resource on miss or failure
    -> selected replacement backend applies the result
```

AssetBundle is one possible replacement artifact, not a design prerequisite.
The actual artifact format must be selected only after observing the game's
resource loader. A path may identify a Bundle while the actual asset is
selected by an internal asset name, an Addressables key, a GUID/FileID pair,
or a catalog entry.

The plugin's core responsibility is to decide whether a replacement exists. It
must not embed character-specific asset authoring rules. The replacement
decision will eventually use:

- Game build identifier and compatibility rules
- Original resource key/path
- Replacement file or artifact path
- Replacement artifact type and identifier
- Fallback policy and diagnostic reason

The actual replacement mechanism is a separate backend. Possible backends are
file/path redirection, resource API redirection, or patching an already
instantiated Unity object. Backend-specific lifetime and dependency rules must
not leak into the replacement decision layer.

### Responsibility split

```text
Extractor / third-party tools:
  unpack and inspect original resources; export editable reference data

Authoring pipeline:
  modify the exported model; produce a replacement artifact

EIEM replacement decision:
  scan the replacement directory and decide whether a matching artifact exists

Replacement backend:
  perform the selected file, resource, or instantiated-object replacement
```

The term `PFB` must be defined before implementation. It may mean an editor
`.prefab` YAML export, a game's custom `.pfb` binary, or a logical prefab dump.
An editor Prefab export is not automatically loadable by a Unity player. It can
only be directly redirected if the game's runtime loader accepts that exact
format.

## 4. Discovery phase: IL2CPP dump layer

The first engineering task is metadata discovery, not resource replacement.

`src/il2cpp_dump.h` now runs once after the IL2CPP domain and assemblies are
available. It writes:

- `plugin/eiem_il2cpp_classes.txt`: all assemblies and type names
- `plugin/eiem_il2cpp_resource_dump.txt`: detailed candidate classes, fields,
  methods, parameter types, return types, `MethodInfo` addresses, and native
  method pointers

The candidate filter includes Unity and game types containing terms such as:
`AssetBundle`, `Addressable`, `ResourceManager`, `Resource`, `Catalog`,
`GameObject`, `Transform`, `SkinnedMeshRenderer`, `Mesh`, `Material`,
`Animator`, `Avatar`, `Character`, `Entity`, and `Visual`.

The dump is metadata-only. It does not instantiate objects or invoke managed
methods. This makes it suitable for the first run in a live game process.

`src/il2cpp_trace.h` adds the next observation-only step. When the corresponding
methods are present, it hooks `AssetBundle.LoadAsset`,
`AssetBundle.LoadAssetAsync`, `SkinnedMeshRenderer.set_sharedMesh`, and
`MeshFilter.set_sharedMesh`. The next build also observes
`FAssetProxyHandle.get_pathOrName`, `FAssetProxyHandle.Get`,
`FAssetProxyHandle.GetAssetProxy`, and
`FAssetProxyUntrackedHandle.Get`. Each hook calls the original method and only
logs the request/result or object relationship; it never substitutes an asset.

After collecting the dump, the next investigation is to identify:

1. Whether the game uses direct AssetBundle APIs, Addressables, a custom
   resource manager, or a combination.
2. Which methods are synchronous and which are asynchronous.
3. The asset key/path and type passed through the load pipeline.
4. The point at which a loaded Prefab or Mesh becomes attached to a character.
5. Whether the game performs integrity checks or custom decryption before the
   Unity resource API is reached.

## 5. Hook strategy

Preferred order:

1. Game-specific resource manager methods discovered from metadata.
2. Addressables/ResourceManager load methods.
3. `AssetBundle.LoadAsset` and `LoadAssetAsync`.
4. File I/O redirection only as a last-resort diagnostic or compatibility
   layer.

The initial Hook should capture arguments and return values without changing
them. It should record the resource key, requested type, Bundle identity,
thread, and whether the operation is synchronous or asynchronous.

Only after the call chain is understood should the resolver return a
replacement asset.

### Current validation result (2026-08-27)

The first live observation pass confirmed that Endfield's common resource
entry point is the game-specific overload:

```text
BundleResourceManager._LoadAssetInternal(
    Beyond.Resource.StringPathHash, System.Type, RootCategory, immediate, priority)
    -> FAssetProxyHandle / asset proxy
```

During one normal startup, the hook observed 500 requests representing 267
distinct 64-bit hashes. Repeated hashes are expected cache/proxy reuse and are
not evidence that the underlying Bundle was read repeatedly.

This is evidence for a resource-manager/object-layer backend as the first
replacement candidate. It is not evidence that raw file interception is
impossible; the Bundle and decryption boundary has not yet been observed.

The observation runs confirmed that the runtime installs and executes the VFS
bundle hooks:

```text
VFS.VirtualFileSystem.LoadBundleFromFile(string)
VFS.VirtualFileSystem.LoadBundleFromFileAsync(string)
VFS.VirtualFileSystem.GetAssetStream(string)
```

It also observed repeated `FAssetProxyHandle.Get` and
`FAssetProxyHandle.GetAssetProxy` calls, while
`FAssetProxyHandle.get_pathOrName` and `Asset._FinishWithAsset` did not produce
runtime records in this session. The resource requests still enter through
`BundleResourceManager._LoadAssetInternal(StringPathHash, ...)` and resolve to
proxy objects. The replacement boundary is now explicit: `LoadBundleFromFile`
and its async overload check `plugin\\mods\\override\\<logical path>` for a
plaintext Unity AssetBundle. When present, the plugin calls Unity's
`AssetBundle.LoadFromFile` or `LoadFromFileAsync` on that file; a null result
falls back to the original encrypted VFS request. `GetAssetStream` remains
observation-only because redirecting it to a
filesystem path would not produce a valid VFS stream. The lower
`ReadFileByLowIO` method returns a large value-type handle and is intentionally
not used for replacement.

The run also confirmed a character mesh attachment directly:

```text
SkinnedMeshRenderer.set_sharedMesh
  renderer=S_npc_gentleman_glass_common_b_01_lod3
  mesh=S_npc_gentleman_glass_common_b_01_lod3
```

No replacement artifact was present, so no `[VFS-OVERRIDE]` record was
expected. End-to-end validation needs one valid plaintext replacement Bundle
at the exact logical path and one normal startup; a failed replacement must
fall back to the original path.

The current build captures the actual low-level read result instead of calling
`GetAssetBytes` after the fact. `FVFSUntrackedLowIOReadHandle.GetData`/
`GetDataThread` and the corresponding tracked-handle methods return a
`NativeArray<byte>` after the VFS has read and decrypted a block. The hooks copy
that native range unchanged into `plugin/captures/lowio/lowio_XXXX.bin` and
write a sidecar containing the physical `.chk` path, `fileNameHash`, offset,
encrypted flag, length, and a first-header classification (`UnityFS`,
`UnityRaw`, `UnityWeb`, or `custom`). F9 only stops the capture and reports
file/byte totals; it does not invoke IL2CPP or Unity APIs and therefore cannot
trigger the previous unknown-thread GC failure. Capture is bounded (4096 files,
64 MiB per read) and deduplicates the two data accessors for the same handle.

Because the startup trace showed Bundle reads through the low-level async path,
the build still observes both
`VFSFileReadStream.Read(byte[],int,int)` and the confirmed
`VFSFileReadStream.Read(Span<byte>)` overload. The Span hook uses the IL2CPP
Windows x64 value-type ABI (a reference to a 16-byte Span containing pointer
and length), validates the memory ranges, and captures the first read buffer
for up to 32 distinct stream instances (maximum 1 MiB each) into
`plugin/captures/span_XXX.bin`. These stream probes remain observation-only and
are not used as the bundle dump source. All probes call the original method
first and leave its return value and caller buffer unchanged.

The previous `GetAssetBytes` batch dump produced 4096 files but none had a
UnityFS/UnityRaw/UnityWeb header; those files were VFS wrapper/table data, not
complete Unity bundles. It has been removed. The lowIO capture is now the
correct validation point because it runs on the successful decrypted read and
records the request metadata needed to correlate blocks with the source VFS
container. A `custom` header is expected until we confirm whether the game
stores complete bundles or compressed/encrypted blocks at this boundary.

## 6. Packaging and unpacking assumptions

Unpacking the original model is useful for discovering:

- Skeleton and bone names
- Mesh/sub-mesh layout
- Material and shader requirements
- BlendShape names
- Prefab hierarchy and component dependencies
- Bundle and Addressables relationships

Extraction does not automatically produce a Bundle that Unity can load again.
Rebuilding may require matching the game's Unity version, platform, asset
serialization version, compression mode, shader setup, and dependency graph.

Possible obstacles include:

- Encrypted or custom-wrapped Bundle files
- LZ4/LZMA compression and block metadata
- Addressables catalogs and hash files
- GUID/FileID references across Bundles
- Runtime decryption in a game-specific loader
- Version-specific Unity serialization
- Asset integrity checks or signatures

The authoring pipeline is intentionally separate from `eiem.dll`. The target
workflow is a round-trip resource workspace: decrypt and unpack the source
game data, import a versioned dump into a Blender add-on, edit it, then build a
replacement artifact for the runtime loader. Third-party parsers may be used
inside the offline pipeline, but they are not runtime plugin dependencies.

### Better-Endfield comparison

Better-Endfield confirms a useful architecture boundary. Its host dynamically
resolves IL2CPP classes and methods by assembly, namespace, type, signature,
and field descriptors, while a HookBroker owns hook creation, conflict
detection, and per-module release. Its model module consumes a generated
resource manifest containing a logical asset path, a `pathHash`, and the
corresponding `bundleHash`; it then calls the game's
`I18NAssetLoader.Load(hash, type, category)` for a configured login actor.

That is valuable for our host/runtime design, but it is not a generic model
replacement backend. The model module still contains a feature-specific
contract, login-scene discovery, prefab cloning, Animator/PlayableGraph
setup, and explicit character configuration. We should reuse the principles:

- resolve IL2CPP methods by metadata descriptors instead of fixed addresses;
- centralize Hook ownership and release so shutdown does not deadlock;
- generate path/hash/bundle indexes offline and keep them outside the DLL;
- let a missing or incompatible contract disable one feature only.

We should not copy its login actor logic or make the replacement engine depend
on character IDs. Our plugin's generic path is:

```text
VFS extractor -> logical files and manifest/index -> external Unity tools
              -> edited replacement Bundle at the same logical path
              -> EIEM LoadBundleFromFile hook -> original VFS fallback
```

The repository includes `tools/extract_endfield_vfs.ps1` as an orchestration
script. It invokes the current EndfieldUnpacker Python extractor, processes
the base `StreamingAssets` VFS, and writes an extraction report. It does not
put VFS decryption or Unity serialization code into `eiem.dll`.

During validation, the early FkArkEnd `Beyond.VFS` build failed with
`EndOfStreamException` on the installed client's small version-3 BLC files.
The current `endGuaGua/EndfieldUnpacker` Python implementation succeeds on the
same installation: its dry run reports `code_ver=4`, identifies the
`Bundle`/`Audio` layers, and enumerates real paths such as
`Bundles/Windows/initial/<hash>.ab`. This establishes the correct extraction
tool and explains why the earlier C# tool must not be used as the baseline.

The resulting `.ab` files are still Endfield's custom Unity bundle wrapper.
They require the matching AnimeStudio CLI (or an equivalent version-aware
decoder) before Unity objects can be consumed by the authoring pipeline. The
extraction pipeline is therefore:

```text
VFS .blc/.chk --EndfieldUnpacker--> logical .ab files
logical .ab --AnimeStudio--> parsed Unity objects + source metadata
parsed objects --EIEM Blender add-on--> versioned authoring dump
authoring dump --Blender--> edited authoring dump
edited dump --bundle builder--> loadable replacement artifact
replacement artifact --> plugin/mods/override/<same logical path>
```

AssetRipper/UABEA are optional inspection backends in this pipeline. They are
not the authoring format and are not assumed to provide a working Endfield
round trip. The Blender add-on and runtime plugin will consume the same
EIEM-owned resource files. They retain the data needed for round-trip editing:
prefab hierarchy, transforms, mesh buffers and submeshes, original normals and
tangents, bind poses and weights, blend shapes, skeleton metadata, material and
texture references, and physics metadata where the source parser exposes it.

User replacements can be stored in whatever artifact the selected backend can
consume. A custom AssetBundle is one option, but it is not required. A custom
PFB file is only valid if the game or a replacement backend can deserialize it.

## 7. Replacement levels

### Level 1: Renderer patch

Keep the original Prefab and character hierarchy. Replace the existing
`SkinnedMeshRenderer` Mesh and materials after the model is instantiated.

Requirements:

- Compatible bone count and ordering, or an explicit bone remapping table
- Compatible bind poses and weights
- Compatible shader/material properties
- BlendShape name/index mapping where facial animation is required

Expected result: visual model replacement with original gameplay behavior.

### Level 2: Asset load redirection

When the game requests a known Mesh or Material asset, return or apply the
equivalent replacement artifact. The original Prefab remains unchanged.

Expected result: replacement happens at the resource boundary and survives
character recreation, provided the selected backend covers all load paths.

### Level 3: Prefab replacement

Return a replacement Prefab or instantiate a replacement GameObject. This may
require reattaching Animator, gameplay scripts, IK, colliders, cloth, and
entity-specific references.

Expected result: complete visual hierarchy replacement, with substantially
higher compatibility risk.

## 8. Authoring package and configuration (agreed v1)

The package contains one generated `mod.ini` and one EIEM-owned set of resource
files. The offline exporter, Blender add-on, and runtime plugin read and write
this exact structure. FBX is not part of the authoring or runtime format.

```text
mod/
  mod.ini
  meshes/<name>.mesh
  skeletons/<name>.skeleton
  materials/<name>.mat
  textures/<name>.png
```

`mod.ini` uses EFMI-like section/key/value syntax. Blender generates it, while
advanced users may edit it. There are two deliberately separate concerns:

1. `Texture`, `Material`, and `Mesh` declare reusable resources.
2. `Render` finds a scene Renderer and organises an operation using those
   resources.

Resource declarations never alter the game on their own.

### 8.1 Resource files

`*.mesh` is one Unity Mesh-equivalent resource. It must preserve every source
field required for a faithful round trip: positions, original normals and
tangents, every UV channel, vertex colours, index buffers, submeshes, skin
weights, bind poses, blend shapes, and the submesh-to-material-slot mapping.
The writer chooses 16- or 32-bit indices according to the data. It also records
the coordinate-system conversion used between Unity and Blender so a round trip
does not change axes, handedness, scale, winding, normals, or tangents.

The first binary layout is little-endian and versioned. It begins with
`EIEMESH\0`, version, coordinate-space marker, source logical path and name;
it then writes vertex arrays, eight independent UV arrays, normalized triangle
indices, submesh index ranges, four-weight skinning data, bind poses, bone-name
hashes, and complete blend-shape data. Submesh ranges address the normalized
triangle index list, never Unity's original byte offsets. A reader must reject
an unknown version instead of guessing its layout.

`*.skeleton` is the same file on import and export. It records the ordered bone
list, complete relative paths, parent relationships, local transforms and bind
poses. An author may elect not to export a skeleton and thereby inherit the
original game's bones unchanged. If a skeleton is exported and modified, the
runtime must really consume it; adding or removing bones is not silently
claimed as supported until Animator, Avatar and physics compatibility are
implemented.

Its first binary layout starts with `EIESKEL\0`, version and coordinate-space
marker, followed by the complete Transform tree, the ordered renderer
`bones[]` indices and `rootBone` index. This deliberately mirrors Unity's
actual binding relation: Mesh weights index `SkinnedMeshRenderer.bones[]`; a
Mesh does not directly refer to a standalone skeleton asset.

`*.mat` is one material authoring file. Its source field names the original
game Material by logical resource path. It records only the shader parameters
that differ from that source Material, with their real types (`Texture`,
`Float`, `Range`, `Color`, `Vector`, and integer where required). Texture values
refer to declared texture resources. Unspecified properties inherit from the
source Material.

Textures are external PNG files for the first version. The plugin decodes them
through Unity's `ImageConversion.LoadImage` API and leaves the source material's
sampler/shader state intact; texture scale and offset are applied explicitly
when present in the material file. Users do not need to author BC5, BC7, DDS,
mipmap or sampler settings.

### 8.2 INI syntax

```ini
[TextureWulfaBody]
path=textures/wulfa_body.png

[MaterialWulfaBody]
path=materials/wulfa_body.mat

[MeshWulfaBodyLod0]
path=meshes/wulfa_body_lod0.mesh

[RenderWulfaBodyLod0]
path=assets/beyond/arts/entity/actor/loli/wulfa/models/s_actor_wulfa_body_01_lod0.asset
asset=S_actor_wulfa_body_01_lod0
match.vertices=4902
match.indices=23220
match.submeshes=1
handling=skip
mesh=MeshWulfaBodyLod0
material.0=MaterialWulfaBody
partner.0=RenderWulfaShoe
```

The section name inside `[]` uses plain identifiers without dots. Dots are
reserved for indexed keys such as `material.0` and `partner.0`.

The `.mat` file referenced above can be represented as simple typed text while
the final parser is implemented, for example:

```ini
source=assets/beyond/.../M_actor_other_body
texture._BaseMap=TextureWulfaBody
color._Color=1,1,1,1
float._Metallic=0.2
```

The source Material is always loaded by its game logical path and then copied.
There is no `from=original` branch and no material source chosen from only the
currently visible scene. This permits using a material from an unloaded
character.

`Render.path` is the source Mesh's logical game path. `Render.asset` identifies
the Mesh sub-resource inside that file, and `match.vertices`, `match.indices`
and `match.submeshes` are optional structure fingerprints. The exporter may
provide them to make a match stricter; they are not required when `path` and
`asset` are already unique. The exporter never exposes Bundle hashes,
physical `.ab` paths or pointers.

Submesh/material assembly is declared by `Render`, while Mesh, Material and
Texture sections remain resource declarations. A Render's identity fields are
evaluated wherever the matching source Mesh is assigned, including ordinary,
shadow and LOD Renderers that share it. A separate Render rule is used when
those LODs or instances need different edits.

The operation set is intentionally small:

```text
mesh present                     assign the named Mesh to the matched source Renderer
handling=skip                    disable the matched source Renderer
partner.N                        create an additional Renderer from that Render record
neither                          leave the matched source Renderer unchanged
```

`mesh` and `handling=skip` are independent. With both present, the source
Renderer is disabled and the named Mesh is assigned as the replacement. With
`mesh` and no `handling=skip`, the source Renderer receives the named Mesh.
`partner.N` always means an additional Renderer; it never replaces the source
Renderer and it never performs another match. There is no `clone`, `override`,
implicit duplication, polling reload, or condition language in v1. Resource
declarations remain separate from the Render record that organizes them.

When `mesh` is present on the matching Render, the plugin assigns one cached
Mesh resource to the existing source Renderer. The existing `bones[]`,
`rootBone`, Animator, LOD and physics remain owned by the game. Omitted
material slots retain the source Renderer values. For each `partner.N`, the
plugin creates a separate Renderer, copies the source Transform's local
position, rotation and scale, plus bones,
`rootBone`, relevant Renderer settings and LOD membership, then applies the
partner's Mesh and material slots. Partner objects are tracked and destroyed
or recreated together with their source Renderer.

### 8.3 LOD and Blender contract

Each distinct source LOD Mesh remains an independent resource and Render rule.
Repeated Renderer instances that reference the same Mesh do not create extra
Blender objects or package records. Users may intentionally reuse one edited
Mesh across several LOD rules, but the exporter never does so implicitly.

The Blender add-on presents the original Mesh resource path, LOD,
material slots, material source paths, shader parameter names/types and texture
bindings as an explicit editing workflow. It must not require a user to inspect
Bundle hashes or maintain hidden metadata. The exact Blender UI may use
dedicated panels, collections or helper objects, but it must remain visible and
low-friction; that UI is designed separately from this file format.

The runtime reload is manual: a configured hotkey schedules reload on the game
thread, removes objects/resources created by the previous application, reads
the changed package, and reapplies its rules. It does not poll the filesystem.

## 9. Milestones and acceptance criteria

### Dump UI contract (agreed)

The in-game Ins panel exposes exactly two dump actions:

- `Dump Current`: dump the selected Mesh and its complete dependency closure.
- `Full Dump`: dump every Mesh/Renderer resource reachable from the current
  scene, including inactive LOD objects and their dependency closures.

Both actions include all resolvable LOD levels. `Full Dump` means the current
scene and its serialized/resource dependencies; it does not scan or decrypt the
entire VFS. The user chooses the output directory. The dump writes a readable
directory tree plus a machine-readable `manifest.json` containing the game
build, logical paths, type, PathID, source Bundle, renderer path, LOD,
SubMesh/material-slot mapping, and unresolved references.

The Mesh list has an independent `Highlight` checkbox. Highlighting is
temporary and applies only to Mesh users in the scene; it must be removed on
uncheck, scene unload, object destruction, or plugin shutdown. It must not
mutate the original Material or Shader.

The first implementation must preserve the distinction between an asset file
and the objects serialized inside it. A Prefab is one file in the dump tree;
its contained Meshes and references are represented in `manifest.json` and are
not emitted as duplicate virtual files.

Known implementation boundaries:

- inactive LODs that are present in the instantiated hierarchy can be dumped;
  LODs held only in an unresolved/unloaded external asset must be reported as
  unresolved rather than silently omitted;
- true 3D outline rendering is a separate renderer task and must not block the
  first metadata/export pass;
- Unity/IL2CPP object enumeration and object creation run on the game main
  thread; disk serialization runs off-thread after data has been copied;
- a failed dependency or unsupported object type falls back to the original
  game object and is recorded in the manifest/report.

### M0: Metadata discovery

- Dump is generated on game startup.
- Resource-related classes and methods are present in the dump.
- No managed calls or model changes occur during discovery.

### M1: Resource observation

- Log one complete original model load path.
- Identify sync/async behavior and requested asset types.
- Confirm whether the game uses encryption or a custom resource layer.

### M2: Same-skeleton renderer replacement

- Replace one known character Mesh and material.
- Original Animator, IK, cloth, and gameplay remain active.
- Stop/reload/character-switch restores or reapplies the correct state.

### M3: Resource or object redirection

- Redirect or patch one known asset using the backend selected from M1.
- Keep dependencies and replacement artifact lifetime valid where applicable.
- Fall back cleanly on load or type mismatch.

### M4: Multi-profile and versioning

- Separate replacement profiles from code.
- Detect unsupported game builds.
- Disable only the incompatible profile instead of installing unsafe hooks.

## 10. Current limitations and risks

- Shutdown handling now sets a process-wide exit flag, restores the game
  window procedure before forwarding close messages, wakes the GUI thread, and
  disables MinHook asynchronously after the original close callback returns.
  The new build still needs one normal in-game exit verification before any
  replacement work is considered stable.
- IL2CPP metadata and generated method layouts are not a stable public ABI.
- Unity object creation and resource APIs generally need the Unity main thread.
- Existing EIEM hooks and face/IK logic may overwrite replacement transforms.
- The game may use custom native resource code that is invisible to managed
  type metadata.
- Game Terms of Service and account risk remain outside the technical design.

## 11. Implemented model dump v1

The in-game Dump tab now keeps the two agreed actions. `Dump Current` writes a
reference manifest for the checked, deduplicated Mesh objects; `Full Dump`
writes every Mesh observed in the current scene. These actions do not read
vertex buffers or instantiate assets. They serialize runtime names, hierarchy,
LOD and identity metadata plus the observed Bundle list; the offline
AnimeStudio index resolves the actual files.

The runtime reference dump contains:

```text
plugin/dumps/
  scene_dump_current.json
  scene_dump_full.json
```

Schema 2 records include `lookupType`, `lookupName`, `hierarchy`, `lod`, and a
process-independent `identityHash`. They also snapshot the selected
Renderer's material slots, shader names, texture-property-to-texture bindings,
and, for `SkinnedMeshRenderer`, the bone names and root bone. `lookupName` and
the dependency names are selectors for the offline index; native object
pointers remain diagnostic only and are never used as persistent identity.

The resource-load hooks connect `_LoadAssetInternal` proxy handles and
`Asset._FinishWithAsset` completions to the resulting Unity object. When this
evidence is available, Mesh, Material and Texture records also carry
`runtimePathHash`, `runtimePath`, and a logical `container`. The mapping uses a
fixed-size four-way pointer table: it allocates nothing in resource hooks and
does not grow with play time. A resource that cannot be tied to a logical path
is explicitly marked `resolution: name-only`.

AnimeStudio's `Export EIEM from JSON...` action resolves the Mesh, Material and
Texture selectors from the compact Endfield index, loads each required source
Bundle only once, and exports the actual resources. Resolution performs one
pass over the compact index rather than one full scan per dependency. The
exported Material JSON supplies serialized shader/material parameters; the
runtime plugin does not guess or duplicate those values. If a name-only
selector matches multiple indexed resources, AnimeStudio reports it as
ambiguous and skips it instead of silently exporting an arbitrary same-named
asset.

The older geometry exporter is retained only as an internal validation tool;
it is not used by the Dump-tab buttons because runtime vertex reads were
already shown to be unreliable for this client. Texture pixels and serialized
material parameter values are resolved from the offline Bundle index instead
of guessed from native memory. Complete prefab, animation and cloth/physics
dependency traversal remains a later layer; it must not be presented as part
of this Mesh/Material/Texture milestone.
