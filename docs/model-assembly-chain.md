# 模型创建链路与改动点

记录 2026-09-14 通过 IL2CPP 元数据枚举 + 运行期探针确认的链路。所有字段偏移都是从运行中的
游戏实测输出（`[META-FIELD]`），不是从反编译片段推的。

## 三端是三条不同的链，但共享资源与解析层

| 端 | 加载入口类 | 命名空间 |
|---|---|---|
| 大世界 | `BaseModelViewPart` | `Beyond.Gameplay.View` |
| UI | `UIModelLoader` + `CharUIModelMono` | `Beyond.UI` / `Beyond.Gameplay.View` |
| NPC | `NPCAvatarCreatorUtils` | `Beyond.NPC.Avatar` |

**三端 PFB 不同，但引用同一批 mesh / 材质 / 骨骼资源**（用户确认：mesh 是同一份）。

**三端都持有 `Beyond.Resource.FAssetProxyHandle`：**

```
BaseModelViewPart.m_assetHandle        FAssetProxyHandle                            (0x68)
UIModelLoader.m_instance2AssetHandle   Dictionary<GameObject, FAssetProxyHandle>    (0x10)
ModelManager.m_go2Handle               Dictionary<GameObject, FAssetProxyHandle>    (0xE8)
```

→ **资源解析层是三端共同经过的层。**

## 各阶段在创建什么

### 阶段 1：资源解析

```
VFS.VirtualFileSystem.GetAssetStream(path / StringPathHash)
BundleResourceManager._LoadAssetInternal(string / hash)
BundleResourceManager._LoadSubAssetInternal(string / hash)
Beyond.Resource.Runtime.Bundle._LoadAssetBundle / _FinishWithBundle
AssetBundle.LoadAsset(string, Type)
FAssetProxyHandle.Get / GetAssetProxy / get_pathOrName
        ↓
Mesh / Material / Texture / Avatar / ScriptableObject 对象
```

**实测**：本次运行中 `AssetBundle.LoadAsset(string,Type)` 与
`FAssetProxyHandle.GetAssetProxy` **都是 0 次调用**。所以角色模型的 mesh 不是在渲染阶段
按需加载的，而是在更早的 prefab 实例化阶段就已解析完成。

### 阶段 2：部件表（描述符）

```
NPCAvatarMeshAssetsSO                                   ↖ 每个模型的根资源
    0x58  avatarSlotMeshDatas      List<NPCAvatarLodMeshAssets>
    0x60  boneClothItems           List<BoneClothItem>        ← 物理声明
    0x68  m_allSlotMeshAssetsCache List<...>                  ← 运行时缓存
    0x70  m_slotMeshAssetByNameCache Dictionary<...>          ← 运行时缓存
    GetAvatarSlotMeshAssets() / GetAllAvatarSlotMeshAssets()   ← 无参 getter

NPCAvatarLodMeshAssets                                  ↖ 每个部件槽
    0x68/0x70/0x78/0x80  partSubMeshsLOD0..3   SubMeshInfo[]
    0x88                 partSubMeshGPU        SubMeshInfo[]
    0x20/0x28            parentBoneTransformName / parentBoneName

SubMeshInfo                                             ↖ 每个 renderer 的描述符
    0x10  <mesh>k__BackingField    UnityEngine.Mesh      ← 已解析的 Mesh 引用
    0x18  meshGuid                 GUIDProxy             ← 资源身份
    0x28  meshPathHash             Int64                 ← 资源身份
    0x30  meshName                 String
    0x38  meshHashName             Int32
    0x40  materialPathHashes       Int64[]
    0x48  backupMaterialPathHashes Int64[]
    0x50  backupMaterialNameToPathHashes  SerializedDictionary<Int32,Int64>
    0x58  isActive                 Boolean               ← 该部件是否参与装配
    0x60  rootBoneName             String
    0x68  rootBoneID               Int32                 ← 根骨（按 ID）
    0x6C  realTimeShadowCaster     Boolean
    0x6D  isRendererDisabled       Boolean
    0x70  platformAvailability     PlatformLayers
```

### 阶段 3：装配（建 renderer + 蒙皮 + 物理）

```
NPCAvatarCreatorUtils
    CreateSMSGO(IAssetLoader, INPCMeshAssets, ELODLevel, NPCGoPool, Transform,
                List<String>, List<Int32>, out SMR[], out RootBoneInfo[],
                bool, Dictionary<Int64,FAssetProxyHandle>, bool)              ← NPC
    CreateSMSInfoForPostModel(INPCMeshAssets, ELODLevel, NPCGoPool, Transform,
                List<String>, List<Int32>, out SMR[], out RootBoneInfo[], bool) ← NPC post
    CreateMeshAssetsGo(IAssetLoader, GameObject, INPCMeshAssets, NPCGoPool, ...)  ← 9 参数
    CreateMeshAssetsInfoForPostModel(GameObject, GameObject, INPCMeshAssets, ...) ← 8 参数
    CreateColoredGpuMeshRenders(IAssetLoader, NPCAvatarMeshAssetsSO, ...)     ← 直接收部件表
    LoadWithCache(Dictionary<Int64,FAssetProxyHandle>, IAssetLoader, Int64)   ← 按哈希加载
    SetSMRRootBone(Animator, SMR[], RootBoneInfo[])                          ← 绑根骨
    GetBonePath / GetBoneShortName(NPCAvatarTempletAssetsSO, Int32)           ← 骨骼按 ID 解析
```

### 阶段 4：名册冻结（三端共用）

```
EntityRenderHelper._InitRenderAndMaterial()
    → _ValidRenderers(List<Renderer>, bool)       ← 只扫一次
    → EntityRenderHelperMaterialController        ← 材质
    → EntityRenderHelperVisibleController         ← 可见性
```

**实测**：`_InitRenderAndMaterial` 调用 335 次，`_ValidRenderers` 只调用 1 次且 count=0。

### 阶段 5：各端持有的缓存

```
BaseModelViewPart                                          ← 大世界
    0x60  m_model                 GameObject
    0x68  m_assetHandle           FAssetProxyHandle
    0x88  m_renderers             Renderer[]
    0x98  m_hgRenderers           HGMeshRenderer[]
    0xA8  m_meshes                SkinnedMeshRenderer[]
    0xB0  m_boneCloths            BeyondDynamicBone.BeyondBoneCloth[]   ← 物理
    0xC0  m_lodGroups             LODGroup[]

CharUIModelMono                                            ← UI
    0x70  entityRenderHelper      EntityRenderHelper
    0xE8  m_boneCloths            List<BeyondDynamicBone.BeyondBoneCloth> ← 物理
```

## 各要素在哪个阶段被创建

| 要素 | 创建阶段 | 载体 |
|---|---|---|
| **Mesh** | 1 资源解析 | `SubMeshInfo.mesh` (0x10) / `meshPathHash` (0x28) |
| **材质** | 1 资源解析 → 3 装配绑定 | `SubMeshInfo.materialPathHashes` (0x40) → `Renderer.sharedMaterials` |
| **贴图** | 1 资源解析 | 材质属性（`texture._BaseMap` 等） |
| **骨架** | 1 资源解析 | `Avatar` / `NPCAvatarTempletAssetsSO`（骨骼按 ID 解析） |
| **蒙皮** | 3 装配 | `SMR.bones` + `bindposes` + `RootBoneInfo[]` |
| **物理** | 3 装配 | `BoneClothItem.boneClothData: ClothSerializeData` → `BeyondBoneCloth` 组件 |
| **透明/显隐** | 4 名册 + 5 缓存 | `isActive` / `isRendererDisabled` / `VisibleController` |

## 物理的独立声明

```
BoneClothItem
    0x10  boneClothName           String
    0x18  boneClothData           BeyondDynamicBone.ClothSerializeData   ← 与 .physics 同类型
    0x20  selectionData           BeyondDynamicBone.SelectionData
    0x28  rootBoneList            List<String>
    0x30  ignoredFromrootBoneList List<String>
    0x38  skinningBoneList        List<String>
    0x40  colliderParentBoneList  List<String>
```

**物理不在 `SubMeshInfo` 里**，而是 `NPCAvatarMeshAssetsSO.boneClothItems` 上独立的一个 List。

## 我们目前在哪

**阶段 4/5 的下游，事后补加**：

```
EiemApplyResolvedRenderRule (:4643 applyMesh)
    → EiemSetSharedMesh           对已存在的 renderer 改 sharedMesh
    → EiemBuildRendererMaterialsForSource   改材质数组
    → EiemPhysicsRuntimeBuild     自己 AddComponent(BeyondBoneCloth)
```

**后果**：renderer 已经建好、名册已经冻结，我们的改动要么事后追加（漏点名册 → 随机躺地），
要么自己建物理组件（不进游戏的 Team 调度）。

## 改动点的取舍

| 阶段 | 覆盖三端 | 能做物理 | 风险 |
|---|---|---|---|
| 1 资源解析 | ✅（共享解析层） | ❌ | 全局影响，需资源身份 |
| 2 部件表 | ⚠️ **待验证**（三端是否读同一张表未确认） | ✅ `boneClothItems` | 中 |
| 3 装配 | ❌ 三端三个入口 | ✅ | 要写三套 |
| 4 名册 | ✅ | ❌ | 已定型，只能事后改 |
| 5 缓存 | ⚠️ | ⚠️ | 各端字段名不同 |

**已确认的**：物理只能在阶段 2/3 加（`BoneClothItem` 只在阶段 2 的表里）。
**未确认的**：三端是否共用阶段 2 那张表。

## 正确蒙皮怎么保证

蒙皮正确性依赖三条同时成立：

1. **`SMR.bones` 的调色盘与 mesh 的 `BoneWeight` 下标一致**
   → mesh 的 `bonePaths` 决定调色盘顺序；`SubMeshInfo.rootBoneID` 决定根骨
2. **`SMR.rootBone` 与动画驱动的是同一棵层级**
   → 我们补出来的骨骼必须挂在游戏层级里，由游戏 Animator 或我们的物理驱动同一个 Transform
3. **bindposes 与骨骼当前姿态匹配**
   → 同源导出的 bindposes 只差浮点精度（实测 1.19e-07）

**已实测的失败模式**：事后注册的对象不在名册里 → 材质/可见性控制器不认它 → 停在绑定姿态
（T-pose / 躺地）。**合并到游戏自己的 renderer 上消除了这个模式**（5/5 验证），
但副作用是骨骼归游戏 Animator 管，我们的物理被移除。

## 未验证的开放问题

1. **三端是否都读 `NPCAvatarMeshAssetsSO`** —— `NPCAvatar*` 命名空间暗示它可能只服务 NPC
2. **`SubMeshInfo.mesh` 是装配读的引用，还是 `meshPathHash` 才是加载路径**
   —— 决定我们的 mesh 是否需要游戏资源身份
3. **`AssetBundle.LoadAsset` / `GetAssetProxy` 零调用** —— mesh 在哪个更早的阶段被解析
4. **`BP_Avatar_SourceData` 未在元数据里找到** —— 那层派生类可能运行时生成或叫别的名字
