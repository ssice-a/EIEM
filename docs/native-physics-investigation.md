# 原生物理接入调查

状态：**authoritative**（原生接入证据、证据限制与当前验证状态）。中间计划不能作为已验证事实；
有冲突时以最新实测结果及其适用范围为准。当前工作区验证记录见第 20.25 节。

本文按调查阶段保留静态取证、宿主实现及当时验证结果。第 1～9 节为原生调查，
第 10～14 节记录诊断器与适配器进展；各节“本轮/尚未”对应该节时点。
当前工作区状态以[文档索引](README.md)和本文末尾核对记录为准；设计契约见[物理设计](physics-authoring-design.md)。

当前诊断入口以第 20.24 节的 v70 实现为准：物理诊断已从 Dump 页面和主功能窗口消息中移除，
测试 DLL 在进入游戏主线程入口后自动开始跟踪，并在正常关闭路径写入独立的
`plugin/physics_diagnostics` 目录。第 10～17 节中关于 Dump 按钮、手动开始/停止和旧 dump
目录的文字只保留为版本演进记录，不再描述当前 UI 或当前诊断入口。

## 1. 本轮结论

不是缺少一个我们自己的物理求解器。当前游戏的 `BeyondDynamicBone.dll` 元数据和
GameAssembly 方法体中已经有运行时构建、代理/约束/碰撞注册、原生 Job 模拟及 Animator 缓冲写回。
EIEM 要补的是**数据读写、实例引用重绑定、原生构建与生命周期接入**。

静态调用调查把“接口名称线索”推进到“已定位方法体及实际调用点”；
后续离线取证已读出 Typhoea 的 11 个原生布料/骨骼模拟组和 27 个碰撞体，详见第 8 节。
v65～v70 已依次实测最小新增链、Animator Team 接纳、Transform 写回、UI 注销、精确作者节点链，以及
新增 Skeleton 节点对替换 Mesh 的可见蒙皮；这些结果足以结束对“能否用原生后端增加 BoneCloth”的宽泛调查。
尚未完成的是其余四个标量的行为语义、碰撞体和 v2 源图实例化，
因此不能把核心机制实验写成完整 Physics 功能已经交付。

最重要的新证据：`TeamManager.UpdateTeamAnimatorData` 真正调用
`Animator.DisableClothBindings → CreateClothBindings(Transform[]) → EnableClothBindings`，
并按 Animator 身份保存返回句柄。新建 Transform 与进入这个绑定系统是不同步骤。

## 2. 样本与复现边界

输入位于 `D:\Hypergryph Launcher\games\Endfield Game`：

| 文件 | SHA256 |
|---|---|
| `GameAssembly.dll`，256677352 字节 | `C24495E51B406F03B03890C4788EE618AE022C991405BE5D5B8B787CB775AE89` |
| `Endfield_Data/il2cpp_data/Metadata/global-metadata.dat`，57021288 字节 | `0076743397ACADF03D3B0064343A963C7C88863B8160526D397E4B3EFB96F02E` |

此元数据版本为 29，类型记录步长明确选 92。通过程序集范围、方法 owner/token 和指针表范围核对，
`BeyondDynamicBone.dll` 代码生成模块 RVA 为 `0xAC33E10`，方法指针槽数 3221。
类清单记载 641 个类型，包括内部类型；不能把 641 理解成用户需要配置的物理类型数量。

使用已有只读工具：

```powershell
python tools/diagnostics/inspect_il2cpp_type.py '<游戏目录>\Endfield_Data\il2cpp_data\Metadata\global-metadata.dat' BeyondBoneCloth --namespace BeyondDynamicBone --type-stride 92 --image '<游戏目录>\GameAssembly.dll' --module-rva 0xac33e10
python tools/diagnostics/inspect_native_shapes.py '<游戏目录>\GameAssembly.dll' --rva 0x5a2283c --limit 6000 --text
```

原始方法表及指令报告保存于 `E:\EIEM_Workspace\physics-diagnostics\20260907-native-contract`：
`beyond-bone-cloth.json`、`cloth-process.json`、`team-manager.json`、`native-functions.json`。
报告为本轮新建，没有覆盖旧实验。这里所有 RVA 只用于复查这份文件，不准硬编码进 DLL。

字段名另由元数据 fieldStart/field_count 提取，核对字段 token 为 `0x04xxxxxx`；
它们是声明字段，不等于 Unity 序列化字段清单，也不提供资产值、单位或文件偏移。

反汇编工具的 unwind range 不是可靠的完整托管方法边界：
`StartRuntimeBuild` 被分成多个范围；`ForceCompleteAllJob` 则位于包含多个小跳板的范围内。
前者另读 `0x343B480:216` 指令窗口，后者仅确认 `0x507ECA8` 开始 7 字节的尾跳。
未解析的间接调用、冷分支不视为不存在；相同 RVA 的代码折叠不意味着不同名字属于同一领域方法。

## 3. 原生数据分层已经找到

| 对象 | 本轮确认的内容 | 含义 |
|---|---|---|
| `BeyondBoneCloth` | 声明 33 个字段，含 serializeData、serializeData2、process、OnBuildComplete、传送处理状态和动画参数 | 配置、运行状态和完成通知分开；不能整体 memcpy |
| `ClothSerializeData` | 声明 42 个字段，见下表 | 是主要作者参数入口；不等于 42 个都能直接热改 |
| `ClothSerializeData2` | selectionData、boneAttributeDict、vertexAttributeList、preBuildData | 固定/运动属性与预构建数据另存，不能只导出骨架 TRS |
| `ClothProcess` | 声明 52 个字段，含 State_Build/Running/UsePreBuild、TeamId、colliderList、cts、lockObject、isDestory/isDestoryInternal/isBuild | 组身份、异步构建和释放状态属于运行时，不能写回作者资源 |
| `ColliderComponent` | center、size、teamIdSet | 几何定义与各模拟组的注册关系分开 |
| `BeyondBoneCapsuleCollider` | direction、reverseDirection、radiusSeparation、alignedOnCenter | 胶囊不只是“一个统一半径”；第 8 节确认 size 分量，第 20.30 节确认端球心与总长算法 |

`ClothSerializeData` 字段按编辑用途归类，以下名称来自本机元数据而非 RE 字段套用：

| 类别 | 声明字段 |
|---|---|
| 输入与拓扑 | clothType、sourceRenderers、meshWriteMode、paintMode、paintMaps、rootBones、ignoreFromRootBones、connectionMode、rotationalInterpolation、rootRotation |
| 调度/混合/LOD | updateMode、clothAnimatorAbilityLODThreshold、clothAnimatorLODThreshold、clothLodFadeTime、clothSimulateWeight、resetSimulationToAnimationPoseWhenWeightLow、resetSimulationToAnimationPoseWeightThreshold、animationPoseRatio |
| 构建与方向 | reductionSetting、customSkinningSetting、normalAlignmentSetting、cullingSettings、normalAxis |
| 基础动力参数 | gravity、gravityDirection、gravityFalloff、stablizationTimeAfterReset、blendWeight、damping、radius |
| 约束与环境 | inertiaConstraint、tetherConstraint、distanceConstraint、triangleBendingConstraint、angleRestorationConstraint、angleLimitConstraint、motionConstraint、colliderCollisionConstraint、selfCollisionConstraint、wind、springConstraint |
| 校验状态 | verificationResult |

本节记录的是最初元数据调查；第 8 节已经从真实资产展开类型、曲线和嵌套值。
不能把 verificationResult、teamIdSet 或异步状态当作可编辑物理参数。

### 目标角色资源证据

现有 `E:\EIEM_Workspace\assets\beyond\dynamicassets\gameplay\actors\postmodels\characters\chr_0034_typhoea_postmodel.prefab.structure.txt`
有 `MBC_Typhoea_Cloth_Coat/Skirt/Skirt_Rope/Skirt_Bag` 节点，以及多个
`Magica Capsule Collider (...)` 节点。但它只列 `[MonoBehaviour] PathID`，未展开脚本类或尺寸参数。
节点名只能指示调查对象，不能单靠名字认定其后端和参数。

已有类型快照中的 `BoneClothItem` 还保存 boneClothData、selectionData、rootBoneList、
ignoredFromrootBoneList、skinningBoneList、colliderParentBoneList；
`NPCAvatarMeshAssetsSO` 有 boneClothItems 和 referencedMeshAssets。
因此离线导出需要追踪配置引用闭包，不能只读当前 Renderer 或只收有权重的骨骼。
这不是宣称 Typhoea 所有物理都经过 NPC 路径。

## 4. 已确认的构建调用

| 本机方法 RVA | 实际静态证据 |
|---|---|
| `BeyondBoneCloth.BuildAndRun` `0x59DC2D8` | 调 DisableAutoBuild；检查 Process 状态和 GenerateInitialization；根据数据分支调用 PreBuildDataConstruction 或 StartRuntimeBuild |
| `ClothProcess.Init` `0x343AA00` | 参数/状态校验、GetClothParameters、TransformRecord、CreateBoneRenderSetupData；也有预构建数据验证和登记 |
| `ClothProcess.StartRuntimeBuild` `0x343B480` | 设置构建状态，取得取消令牌，`0x343B4F3` 调 RuntimeBuildAsync，随后返回 true |
| `<RuntimeBuildAsync>d__10.MoveNext` `0x38E31C0` | Task.Run/awaiter、取消检查、注册、UpdateUse 等真实代码；不是一个空占位入口 |
| 构建工作函数 `0x3DDCEC0` | VirtualMesh.ImportFrom、Selection、Reduction、Optimization、ConvertProxyMesh、Mapping |
| 约束工作函数 `0x343A390` | DistanceConstraint.CreateData、TriangleBendingConstraint.CreateData、InertiaConstraint.CreateData，含取消检查 |

RuntimeBuildAsync 的状态机中可见以下注册调用点：

1. `0x38E56B9`：ClothManager.AddCloth。
2. `0x38E632F`：DynamicBoneTransformManager.AddTransform。
3. `0x38E6925`：SimulationManager.RegisterProxyMesh。
4. `0x38E6945`：ColliderManager.Register。
5. `0x38E6A15`：SimulationManager.RegisterConstraint。
6. `0x38E6C3A`：VirtualMeshManager.RegisterMappingMesh。
7. `0x38E6E21`：ClothProcess.UpdateUse。

这是所检查方法中的调用位置摘要，不是绕过 BuildAndRun 手工串调底层接口的操作指南；
中间有等待、状态检查、数据操作和错误路径。预构建分支也找到对应的代理/碰撞/约束登记。

**BuildAndRun 返回 true 不等于异步已构建成功。** StartRuntimeBuild 的实现已经证明启动和完成是两个时刻。
OnBuildComplete 字段、Result、IsRunning 是核对完成语义的入口。第 9 节进一步确认了单 Boolean 通知载荷、
同步/异步触发路径和取消时不通知的分支；实际泛型字段类型、回调线程和调度安全点尚待运行时核验。
旧 `trojan.h` 调用后直接增加 rebuilt 计数，不能作为新增物理成功的判断。

## 5. 模拟结果如何送回骨架

`TeamManager` 有 teamId2AnimatorInstnceId、animatorID2RWHandler、transformID2RWHandlerID、
teamId2Animator 和 dirtyAnimatorTransformTeams；`DynamicBoneTransformManager` 有各类位置/旋转数组、
transformAccessArray、animatorTransformMap 和 teamIdArray。

### 注册到 Animator 的真实调用点

`TeamManager.UpdateTeamAnimatorData`：`0x5A2283C`。

- `0x5A231A4`：对已有绑定调用 Animator.DisableClothBindings。
- `0x5A231C9`：将收集列表转成数组后，调用 Animator.CreateClothBindings。
- 随后按 Animator 的 GetInstanceID 保存返回的 AnimationTransformRWBufferHandle。
- `0x5A2325F`：Animator.EnableClothBindings。

`AddAnimatorTransform(teamId, t)`（`0x34408A0`）本身不是完整的新增骨骼构建器：
所读方法检查 Transform 后，向管理器集合登记 teamId；没有在这个方法里直接调用 CreateClothBindings。
不能凭名称调用一次就声称已获得新骨骼缓冲槽。

由此得到的接入约束：一个 Animator 的多条物理链必须由原生管理器共同管理绑定，
不能让每个 Mod/Renderer 各自 DestroyClothBindings 或只用自己的骨骼数组覆盖共享句柄。
游戏会怎样处理未在原 Avatar 的新增 Transform，仍需确认原生 CreateClothBindings 实现和实例结果。

### 每帧原生更新的调用证据

`ClothManager.OnAfterLateUpdate` 尾跳 ClothUpdate（`0x32AD500`）。后者包含：

- Team/时间/风更新、ReadTransform 和 ReadAnimatorBufferData。
- PreProxyMeshUpdate、PreSimulationUpdate、SimulationStepUpdate。
- CalcDisplayPosition、PostProxyMeshUpdate、PostMappingMeshUpdate。
- WriteTransform、CopyDoubleBuffer、WriteAnimatorBufferData、PostTeamUpdate、CompleteMasterJob。

其中 Animator 缓冲读写调用点为 `0x32AD88C`、`0x32ADED2`；
SimulationStepUpdate 调用点为 `0x32ADB31`。
有条件、剔除和跨帧路径，不代表所有条目每帧对所有对象都执行，也尚未还原整个引擎的 PlayerLoop。

物理计算与 GPU 蒙皮是两个阶段。上面证实原生物理 Job 的结果会进入 Animator 缓冲；
EIEM 不需要因此自己实现物理，也不能用每帧晚期覆盖 Transform 代替原生写回注册。

## 6. 停用不是销毁，启动任务不是可立即释放

- BeyondBoneCloth.OnEnable `0x343B560` → ClothProcess.StartUse。
- BeyondBoneCloth.OnDisable `0x44C3640` → ClothProcess.EndUse。
- BeyondBoneCloth.OnDestroy `0x59DCF04` → DisposeTeleportResources → Process.Dispose。
- Process.Dispose `0x59DFE8C` 中有加锁、失效/销毁状态、取消相关调用，随后进入 DisposeInternal。
- DisposeInternal `0x59DEEBC` 先检查释放/构建状态，再退出代理、Transform、Mapping、碰撞、Cloth，
  释放构建数据和 Renderer 记录、注销预构建资源、移除监视关系。存在构建中不立即执行全部释放的分支。
- TeamManager.RemoveTeam `0x5A22470` 中有 RemoveComponentTransform 和 ClearTeamAnimatorData。
- ForceCompleteAllJob `0x507ECA8` 是转到 CompleteMasterJob 的短跳板，不是另一个万能清理入口。

这里只确认代码中存在这些行为；尚未证明 DLL 从任意线程调用 Dispose 或 CompleteMasterJob 都安全，
也未证明一个主 Job 完成就代表后台构建 Task 和全部跨帧缓冲已退出。
F10/换图时须服从原生完成/取消契约，不能抢先销毁仍被任务引用的 Mod 骨骼。

## 7. 下一步具体补什么

1. **真实配置样本 → 作者数据契约**：第 8 节已完成 Typhoea 的脚本、TypeTree、selection、
   preBuild、碰撞体和骨骼引用读取。接着确认 selection 点序到 Transform 的映射、胶囊端点/缩放，
   再制作 Blender 辅助体；不把读取完成当成已经支持修改后写回。
2. **注册边界**：继续核对 OnBuildComplete、取消后完成和原生管理器更新安全点；
   查 CreateClothBindings 对新增 Transform 的处理及返回槽映射，不直接重绑整个 Animator 试错。
3. **三端实现**：在上述契约清楚后，实现参数/引用无损导入导出、修改原生组件及新增原生链。
   运行状态和缓冲句柄不进资源文件，共享骨架不按 Mesh 复制。
4. **集中验收**：一次记录配置源、骨骼路径、team/Animator/缓冲映射、构建开始/结束及注销，
   覆盖世界、展示 UI、F10 和换图；不靠看到网格会动就认为物理正确。

本轮没有增加 Hook、模拟器或回退，没有部署新 DLL。当前能确定方向和关键调用，
还不能把“已能导入原生物理、新增物理链并正确热重载”标为完成。

## 8. 2026-09-07 追加：真实 Prefab 组件与引用闭包

### 8.1 取证方法与结果

新增离线入口 [PrefabComponentProbe](../tools/EndfieldVfsProbe/PrefabComponentProbe.cs)，
通过现有 VFS 索引定位逻辑 Prefab，并加载源 Bundle 的依赖闭包。它只读取资源，不加载或执行游戏 DLL。
按 `MonoScript` 识别实际组件类，按资产自带 TypeTree 读取字段，**没有将 IL2CPP 内存偏移当作序列化偏移**。
引用身份使用 CAB + PathID，另保存完整 Transform 路径和 local TRS；不能单用 PathID 跨 CAB 合并。

输入：

- `assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0034_typhoea_postmodel.prefab`。
- 索引 `E:\EIEM_Workspace\index\endfield_assets.eidx`。
- 源 Bundle `Bundles/Windows/main/f96b038ea799224659a99f83.ab`，依赖闭包 103 个 Bundle。
- VFS fingerprint：`D6631362E327706C9DF00AE92D2E794209BAD6C49302F92BA6ED94C801526B50`。

最新证据目录：`E:\EIEM_Workspace\physics-diagnostics\20260907-typhoea-reference-graph`。
其中 `components.json` 包含组件身份、脚本、归属、引用与 Transform；每个编号另有原始 `.bin`、
`.schema.json`，完整解码成功时才生成 `.data.json`。文件 SHA256 和原始字节数逐项复核通过。
这是**诊断输出，不是可加载的 Physics Mod 包**。

复现命令（输出目录必须尚不存在，避免覆盖旧实验）：

```powershell
dotnet build tools/EndfieldVfsProbe/EndfieldVfsProbe.csproj -c Release --nologo
dotnet tools/EndfieldVfsProbe/bin/Release/net9.0/EndfieldVfsProbe.dll --inspect-prefab-components `
  '<游戏目录>\Endfield_Data\StreamingAssets\VFS' 'E:\EIEM_Workspace' `
  'assets/beyond/dynamicassets/gameplay/actors/postmodels/characters/chr_0034_typhoea_postmodel.prefab' `
  '<新的诊断目录>' 'E:\EIEM_Workspace\index\endfield_assets.eidx'
```

该工具要求 VFS 与索引指纹完全相符，过期索引会报错，不混用不同版本的引用。

| 实际脚本类型 | 所选 Prefab 内组件数 | TypeTree 读取 |
|---|---:|---|
| BeyondBoneCloth | 11 | 全部完整消费原始组件字节 |
| BeyondBoneCapsuleCollider | 25 | 全部完整消费原始组件字节 |
| BeyondBonePlaneCollider | 1 | 完整 |
| BeyondBoneSphereCollider | 1 | 完整 |

这 38 个组件的 317 个非空 PPtr 全部解析到目标。计数包含脚本、归属、骨骼、碰撞与预构建引用，
**不是 317 根物理骨骼**。闭包总共读到 49 个 MonoBehaviour、556 个 Transform；
通过 `inSelectedPrefab` 区分目标与依赖，不能把依赖里的对象全算到角色上。

已明确保留一个非物理解析失败：`00046` 的 `AnimatorMono` 只消费 `336/448` 字节，
未标记为成功，未生成该组件的数据 JSON；原始字节和 TypeTree 仍保留。
其他 48 个组件解码长度完整。长度吻合是结构读取检查，**不等于所有字段语义已知，也不等于修改后可无损写回**。

### 8.2 模拟组的真实配置

下表组名省略 `MBC_Typhoea_` 前缀。一个 BeyondBoneCloth 可以有多个 rootBones；
11 个组件不能称作只有 11 条单根链。此样本 rootBones 数组合计 31 项。
“选择点”是 `selectionData` 数组长度，不直接当作共享骨架骨骼总数。

| 组名 | 根引用 | 碰撞体引用 | 选择点 | gravity | damping.value | radius.value |
|---|---:|---:|---:|---:|---:|---:|
| Hair_Front_Bangs_Short | 5 | 1 | 15 | 5 | 0.05 | 0.006 |
| Acc_Back_Left_Bag | 1 | 2 | 4 | 10 | 0.05 | 0.038 |
| Cloth_Skirt | 7 | 5 | 37 | 5 | 0.05 | 0.065 |
| Cloth_Skirt_Bag | 2 | 3 | 6 | 5 | 0.05 | 0.031 |
| Cloth_Coat | 6 | 1 | 12 | 5 | 0.05 | 0.020 |
| Hair_Back_Ponytail_Knot | 2 | 2 | 24 | 0 | 0.20 | 0.020 |
| Hair_Back_Ponytail_Long | 2 | 8 | 38 | 0 | 0.30 | 0.125 |
| Acc_Back_Right_Lantern | 1 | 1 | 4 | 2 | 0.20 | 0.055 |
| Hair_Front_Side_Long | 2 | 10 | 10 | 6 | 0.10 | 0.020 |
| Cloth_Skirt_Rope | 2 | 2 | 6 | 8 | 0.01 | 0.045 |
| Tail | 1 | 0 | 8 | 0 | 0.08 | 0.020 |

这些是序列化原值，不擅自换成 RE 单位。damping/radius 还有曲线结构，不能仅保留表中的 value。
全部 11 个组的 `clothType=1`、`sourceRenderers=[]`，且 `preBuildData.enabled=0`，
但文件内仍有预构建数据。**不能因为预构建块存在就选预构建路径，更不能用禁用的旧块覆盖当前选择数据**。
Prefab 原值也不能证明所有实例初始化后都不再改这些设置。

例如长发组当前 selection 有 38 点，而禁用的预构建属性数组 count 为 16；两者不能互相替代。
`gravityProperty=0` 不代表重力为零：例如 `00016` 同时有 `gravityProperty=0` 和
`serializeData.gravity=10`。要区分动画属性绑定字段与实际物理配置。

### 8.3 选择属性与碰撞体不能靠名称猜

本机方法体进一步确认 `VertexAttribute` 的判定：

- `IsFixed` `0x5A01E7C`：读取 Value 的 `0x01` 位。
- `IsMove` `0x5A01F44`：检查 `0x02` 位是否非零。
- `IsInvalid` `0x5A01EC4`：检查 `(Value & 0x03) == 0`。

它们是标志位，不是可随意重新编号的三个枚举；其他位必须保留。
长发组的实际选择是 2 个固定点、12 个运动点、24 个无效点；ignoreFromRootBones 也有 24 个引用。
数量相等尚不证明点序和骨骼顺序相同，不能直接 zip 对应，更不能把根下所有子骨都当成运动点。
`ClothSerializeData2` 的运行时声明里有 boneAttributeDict/vertexAttributeList，
但此次 TypeTree 实际只序列化 selectionData/preBuildData；不能用运行时字段清单冒充文件字段。

碰撞体参数通过元数据参数名与短方法体相互核对：

- 胶囊 `SetSize(startRadius, endRadius, length)` `0x59DC0E4`：依次写入 size.x/y/z，
  并按两端半径是否不同设置 radiusSeparation。
- 球体 `SetSize(radius)` `0x4A46EC0`：写 size.x，size.y/z 清零。
- 第 20.30 节已从 `GetColliderType`、`GetSize` 和 `StartSimulationStepJob.Execute` 的本机方法体确认：
  `size.z` 是包含两端半球的外部长，`alignedOnCenter` 决定旋转中心，`reverseDirection` 翻转端点轴；
  本节早期仅凭参数名不能确认端点的限制现已补齐。

真实反例：`00002` 的 GameObject 名称含 `Magica Capsule Collider (skirt_base_R_c_02_jnt)`，
脚本却是 **BeyondBonePlaneCollider**，center=(0,0,-0.05)、size=(0,0,0)。
不能按名字创建胶囊，也不能把零 size 的平面判为无效碰撞体。

11 个组共有 35 次碰撞引用，指向 25 个不同碰撞组件；其中一个被 4 个组共用。
27 个碰撞组件中还有 2 个未被这 11 个组引用，应保留并标记关系，不自动删除或复制给每条链。
`Hair_Front_Side_Long` 有 10 个碰撞引用，已经实际超过旧 `cloth.h` 的 8 个上限；
新导入器/后端必须按资源真实数组长度处理，不继承此旧限制。

### 8.4 验证与下一项实施

- 离线工具 net9.0 Release 编译通过，0 warning / 0 error。
- 两次真实资源取证均保留原始字节；最新报告的 49 个原始组件 hash/长度复核通过。
- 38 个原生物理组件全量解码且非空引用已解析，11 组 position/attribute 数量一致。
- 元数据检查工具新增参数名/类型索引读取与范围、token 检查，7 项中性测试通过。

接下来优先确认 **selection 点序 → 完整骨骼路径** 和 **碰撞体端点/缩放**，
然后把此图纳入正式共享 Physics/Skeleton 资源，而非让用户编辑这些诊断 JSON。
Blender 只创建一份共享碰撞辅助体，由多个组引用；原始未知字段和曲线保留。
运行端仍遵守第 4—6 节的原生构建、完成、注销契约，不自写求解器，也不逐 Mesh 复制模拟组。
本次追加没有部署 DLL、调用游戏构建接口或修改用户的 Mod/Blender 工程。

## 9. 2026-09-07 追加：初始化、完成和延迟释放

证据沿用第 2 节同一 PE/metadata hash，新增只读报告目录：
`E:\EIEM_Workspace\physics-diagnostics\20260907-native-lifecycle`。
包含 `entry-dispose.json`、`startup-callback-tail.json`、`runtime-build-completion.json`、
`async-finally.json`、`result-code.json`。以下地址仅供复查，不进入生产偏移表。

### 9.1 不能先 AddComponent，再假定有充足时间配置

- `Awake` `0x45D82E0` 检查全局模式；值为 1 时进入冷块 `0x5000C2E`，
  调用 get_Process、Process.Init 和管理器操作 `0x343A2E0`。
- `Start` `0x343A1F0` 在同一模式为 0 时执行初始化；随后进入 `AutoBuild`。
- 模式字段的领域名称/枚举含义尚未确认，不擅自命名成“编辑/运行模式”。
  `0x343A2E0` 已在第 12 节追加确认是 `TeamManager.RemoveMonitoringProcess`，不是模拟任务完成接口。
- `get_Process` `0x32B6860` **不纯只读**：取 process 后把当前组件写回 process 引用。
  调查工具读取元数据解析到的 `process` 字段，不为“看看状态”调用这个 getter。

组件工厂必须明确阻止配置完成前自动初始化/构建；不能只靠 BuildAndRun 内部的 DisableAutoBuild，
因为 Awake/Start 可能已经发生。创建方式、停用时序尚未实测，不能把“放到 inactive 对象上”写成已验证接入方案。

### 9.2 构建通知有同步路径，也有完全不通知的取消路径

- BuildAndRun 的完成清理块 `0xBF20C0` 读取组件 OnBuildComplete，
  经 `0x3596EC0` 调用带一个 Boolean 载荷的委托。同步失败/预构建路径可在 BuildAndRun 返回前通知。
- StartRuntimeBuild 返回成功仅代表接受启动；异步状态机进入构建状态，随后执行注册。
- 异步 finally `0xBF0D30..0xBF205C` 在 `0xBF1E10` 清 isBuild，之后检查 isDestory。
  若未销毁且组件有效，则在 `0xBF1F9C` 通知 Boolean 结果；其成功判断比较 ResultCode 的成功状态。
- **若请求销毁，分支在 `0xBF1FCB` 调 DisposeInternal，直接退出，不通知 OnBuildComplete。**
  因而取消完成不能只等待这个事件，否则可能永远保留旧骨架。

单 Boolean 调用约定是本机指令证据，不等于已验证字段一定是某个标准 `Action<bool>` 类型。
运行时诊断会输出真实字段类型；不能全局 Hook 共享委托 Invoke（可能被其他委托共用）。
异步 continuation 所在线程尚未确认，回调里不能直接操作 Unity 对象。
正式适配器需要处理回调早于返回、取消无回调以及过期 program 代际，不能按“每次正好一次异步回调”设计。

### 9.3 Dispose 返回不是允许释放新增骨骼的凭证

`Dispose` `0x59DFE8C` 在 lockObject 保护下标记销毁、清有效状态/结果，取消 CTS，
随后进入 DisposeInternal。后者：

1. 已完成内部释放则退出。
2. **仍在构建则暂不清理，直接返回**；异步 finally 会在退出构建状态后再次进入清理。
3. 非构建状态才执行原生注销与资源清理。
4. `0x59DFC55` 标记 isDestoryInternal 后，释放锁，仍要调用管理器 `0x343A2E0` 再返回。

因此，读到 isDestoryInternal=true 也不等于该 native 调用栈已经退出。
不能把上述字段或 IsRunning=false 用作释放骨架的充分条件；还需确认原生 Task/Job 与管理器安全边界。
原生 `ResultCode` 有 IsSuccess/IsProcess/IsCancel 等方法，正式实现应使用已解析的语义接口，
而不是把此次成功数值 2、标志位或字段偏移写死。

## 10. DLL 侧按需原生物理诊断（v52 及以前的历史实现）

新增 [eiem_native_physics_probe.h](../src/eiem_native_physics_probe.h)，通过 Dump 页的
**原生物理诊断** 按钮发送独立 WM_APP 请求，在既有 Unity 线程通道执行。
输出到配置的 dump 目录，文件名 `physics_runtime_<pid>_<tick>.json`，使用 CREATE_NEW 保留前次证据。
这不是 Physics 资源包，不改变 INI、源配置、Mesh、骨架或原生求解器。

实际记录：

- BeyondDynamicBone 程序集中的 10 个相关类型：字段名/类型/flags、方法返回类型/参数类型/flags。
  包含 OnBuildComplete 的真实字段类型，不把静态推断冒充运行时结果。
- 已加载的 BeyondBoneCloth 组件（包括非激活对象/缓存资产）：实例 ID、显示层级、活动状态、
  独立 process、teamId，以及 isBuild/isDestory/isDestoryInternal/IsValid/IsRunning。
- 非激活不自动判作 PFB，显示层级仅用于查找，**不是稳定资源身份**。
  每个实际组件分别记录，不从选中 Mesh 推断“一 Mesh 一套物理”。

约束：

- 只在点击时枚举，不逐帧扫描，不增加物理 Hook，不调用 get_Process、BuildAndRun、Dispose 或任何参数 setter。
- 字段按实际名称、精确类型和实例/static 属性匹配；方法按返回值、参数、实例/static 匹配。
  缺失/歧义不按同参数数目猜重载，不回退硬编码偏移，不把异常/缺失解读成 false。
- 使用 GC 强引用保持本次读取的数组、组件和托管 process 可达，读取结束即释放；
  **强引用不保证 Unity native 对象存活**，组件另做原生有效性检查。
- 状态是各字段独立采样，明确标记 `observed-not-atomic`；既不是整组原子快照，也不是释放栅栏。
  诊断不承担实例状态机/释放权限判断，不能用一次 dump 证明所有构建/注销事件已经发生。
- 用托管 Array.GetValue 和实际数组长度，不继承旧裙摆代码的 8 个碰撞/组件限制。

验证：`build.bat` 全量构建通过；MSVC 下实际编译 probe 头文件的 7 项中性宿主测试通过，
覆盖多实例、缺 API、字段/返回类型不符、空 process、枚举失败和非 Unity 线程；同时检查无修改调用、
GC 引用释放和类型名内存释放。与 Unity lifetime、Skeleton、Skin、Mod controls 一起执行共 16 项通过，无跳过。
这验证代码契约，不等同于游戏中原生物理注册成功。
本次最终本地 `bin/eiem.dll` SHA256：
`DDD495ABB2084CF3537DE175D61A953BFEA33A91FDA3C2876FB5A16DA6442468`。
**Render.physics 原生装配尚未实现**，新增链、共享碰撞编辑、Animator 新节点写回和安全注销仍是下一步。
本轮没有部署 DLL，也没有修改游戏 Mod/Blender 工程。

## 11. 2026-09-07 追加：参数副本、通知和共享所有权

范围更正：自制物理链需要创建额外骨架节点；隐藏或删除 Mesh 不应驱动这些骨骼的增删。
当前先实现已有模拟组的参数更新基础，同时继续将新增骨骼、原生构建/注册、动画缓冲写回纳入必需范围。
以下不是新增物理链已可用的结论，也不是对现有游戏热重载问题的部署修复。

### 11.1 本机原生参数更新与复制

沿用第 2 节同一游戏文件，报告仍位于第 9 节目录。新增保留：
`serialize-hot-update.json`、`serialize-import-body.json`、`collider-hot-update.json`。
地址只用于本机静态复核，不写入运行时分支。

- `BeyondBoneCloth.SetParameterChange`（`0x59DE748`）进入 `ClothProcess.DataUpdate`（`0x59DED38`）。
  后者会对 serializeData 调 `DataValidate`（`0x59E2084`），再进入管理器的更新登记路径。
  所以通知不只是“把一个布尔标志置真”；不能让多实例共享的源配置被这条路径连带修改。
- `set_SerializeData`（`0x5697544`）是带 GC 写屏障的引用赋值，不会替我们复制或隔离配置。
- `ClothSerializeData.Import(ClothSerializeData, Boolean)`（`0x59E2CC0`）确实检查 deepCopy，
  深复制分支创建列表及多个嵌套参数对象；因此适配器使用原生构造函数和 `Import(source, true)`，
  不用结构体 memcpy 或 MemberwiseClone 冒充深复制。
- **尚未逐字段证明 Import 保留该游戏所有扩展参数和引用。** 原生存在 deepCopy 分支，不等于任意字段均已无损验收。
  启用真实覆写前，还须比较实际组件的源/副本图，尤其是曲线、根骨列表和游戏自定义字段。

上游 API 也要求 SerializeData 变化后通知，但只能作为参照；实际签名及行为以本机证据为准。
[MagicaCloth2 组件 API](https://magicasoft.jp/en/mc2_api_magicacloth/)。

### 11.2 碰撞更新直接写共享数组，不能随意调用

`ColliderComponent.UpdateParameters`（`0x59E36C0`）在 DataValidate 后遍历其 teamIdSet，
逐组调用 `ColliderManager.UpdateParameters`（`0x5A5D2F0`）。
后者按模拟组的 collider chunk 和局部序号，**直接写 flag、center、size 数组**，不是只排队一个请求。
一个碰撞体被多组引用时，应通过组件的原生更新操作覆盖其注册组，不能复制出每组一份的假共享组件。

这也意味着“已经在 Unity 主线程”本身不证明与原生模拟 Job 同步完成。
参数与碰撞更新不能共用一个猜测的任意 WndProc 写入时机；本次适配器不调用碰撞 setter 或更新函数。
原生 SetSize 的具体分量证据仍见第 8.3 节；没有修改既有裙摆 UI/代码。

### 11.3 已写入的代码及边界

[元数据 API](../src/eiem_native_physics_api.h) 统一诊断与适配器的精确查找：
程序集、类、字段类型，以及方法返回类型、参数类型、静态性都必须相符。
双参数 Import 与单参数重载分开匹配；不按参数数量或固定地址取第一个候选。

[参数适配器](../src/eiem_native_physics_parameters.h) 目前只接受 5 个已命名的标量字段：
`gravity`、`stablizationTimeAfterReset`（有限非负值），
`gravityFalloff`、`blendWeight`、`animationPoseRatio`（有限的 0～1 值）。
这是本次受支持编辑范围，不是完整 Physics schema；拒绝其他字段、重复字段、空编辑和无效数值，不截断源参数。

操作分为准备、应用、恢复：

1. 准备阶段强持有原配置和组件，建立自己的配置副本；只修改副本并回读，尚不改组件绑定。
2. 应用时确认组件非构建/销毁状态、原生有效、当前绑定仍受本次所有权约束，然后赋值并通知。
   这些状态只是参数调用的准入检查，**不是 Task/Job 释放栅栏**。
3. setter 可能改了引用后才报错，因此在调用前记录恢复责任。失败不能丢失原配置。
4. 恢复时写回保留的原配置并重新通知。恢复失败保留账本；确认组件原生死亡后可以只清账本，不调用 setter。
   这条规则只适用于已有组件的参数引用，不能推广成“组件死亡就可直接删新增物理骨骼”。

`EiemPhysicsParameters` 账本按实际组件管理记录，归模型/物理子系统，不能每个 Renderer 各建一份：

- 同一资源重复引用同一组件共用一份源基线，已应用状态回读成功时不重复赋值/通知。
- 两个实例即使共享同一份源配置，也分别持有 Mod 副本；恢复一个不影响另一个。
- 不同资源争用同一组件明确拒绝；不按 Render 遍历顺序互相覆盖。
- 值变化要求先恢复旧记录，再准备新记录。恢复失败期间禁止再次捕获配置作新源。
- 批量恢复只清除成功项，失败项保持引用与原因，不因下一次 F10 丢账本。
- 不在析构函数中调用 Unity；所属子系统必须活到显式恢复完成，再销毁账本或发布新代际。

**尚未接入 INI、Render 执行器或 F10**：当前包含头文件可保证 DLL 编译，但没有创建生产账本或调用参数写入。
截至本节记录时，尚未实现 Physics 文件读取、新组件工厂、原生新链注册、物理持有 Skeleton 的接线及安全注销。
文件读取后续见第 13 节及第 15 节；原生组件工厂和安全注销接线仍未完成。
不得将本次适配器当作一个可直接通过 `physics=...` 使用的功能，也不需要用户现在修改 INI 或重启游戏测试它。

### 11.4 验证记录和下一接入条件

完整 `build.bat` 构建成功。本次本地 `bin/eiem.dll` SHA256：
`F1882AC90EE627157B5AE21D4CC611FFBF67B32AE6C75BB86B0C532A86C76F9F`。

在 MSVC 环境执行：

```text
python -m unittest test_native_physics_parameters test_native_physics_probe test_unity_lifetime test_skeleton_runtime test_skin_runtime test_mod_controls -v
```

32 项通过，无跳过；其中参数账本 17 项、只读 probe 7 项。
参数测试实际编译生产头文件，使用中性 IL2CPP 宿主模拟 API；覆盖同组件复用、两个实例共享源、
争用、重新准备、线程、setter 改绑定后抛错、通知失败、部分恢复、原生死亡和引用释放。
这是适配器的离线契约验证，不是游戏求解、深复制完整性或画面效果验收。

下一接入必须先补齐：真实源/副本图比较；参数与碰撞更新的原生安全时机；
新组件配置前的自动构建抑制；新增节点的选择序/Animator 写回；构建取消及模拟任务退出后的骨架释放。
再把物理实例所有权接到共享骨架与 Reconcile/F10，最后做正式资源与 Blender 可视化往返。
本次没有部署 DLL，没有编辑游戏 Mod/Blender 工程，也没有加入新求解器或任意字段写入接口。

## 12. 2026-09-07 追加：骨链输入顺序与 Animator 写回映射

### 12.1 证据及适用边界

仍使用第 2 节同一 GameAssembly/metadata。第 9 节目录新增保留：
`render-setup-fields.json`、`process-binding-fields.json`、`team-binding-fields.json`、
`vertex-attribute-fields.json`、`bone-input-mapping.json`、`animator-binding-map.json`。

静态 metadata 工具现在输出字段名称、type index 和 token，并检查字段范围及 token；
**type index 不是字段偏移，也不是已解析出的类型名称**。运行时精确类型来自 DLL 元数据 API，
不把静态表序号写进生产内存访问。

本节地址仅供本机反汇编复核，不加入 Hook 偏移表。已有实时类型 dump 中的
`UnityEngine.AnimationTransformRWBufferHandle` 用于核对字段含义；其 boxed 字段偏移不能直接当作未装箱结构布局。

### 12.2 原生 BoneCloth 如何从 Transform 生成选择点

`ClothProcess.CreateBoneRenderSetupData`（`0x3439960`）在 `0x3439A1C` 调用七参数
`RenderSetupData` 构造函数（`0x38D4A50`），输入包含组件 Transform、rootTransforms、
ignoreFromRootBones、collisionBones、connectionMode。注意入口 unwind 范围只覆盖短前段，
本次另保留后续指令窗口；不把短 unwind 范围当作完整函数。

在此次检查的骨链构造路径内：

1. 从根列表遍历 Transform 子层级，经去重和忽略判断，建立 `transformList`。
   不是从 Mesh 的顶点组或原局部 bones 数组复制索引。
2. 记录此时列表长度为 `skinBoneCount`，并将同一位置记作 `renderTransformIndex`。
3. **随后还会把组件/render Transform 作为空间锚点加入列表**，再读 Transform 信息。
   所以这里的 transformCount 不应直接解释为“全部物理可动骨骼数”。字段 skinBoneCount 的名字也不能拿来冒充 Mesh palette 长度。
4. `ReadTransformInformation` 建立对应的 Transform ID、父 ID 等数据。

`GenerateBoneClothSelection`（`0x59DFF84`）按 `skinBoneCount` 创建选择数据，
把点位置转换到组件空间，并填默认运动属性；随后对序列化 rootBones 调
`Object.GetInstanceID → RenderSetupData.GetTransformIndexFromId`，在实际查找位置写固定属性。
`VertexAttribute` 的静态字段分别有 `Invalid / Fixed / Move / DisableCollision`；不在插件里硬编码其字节值。

`GetTransformIndexFromId`（`0x5A51EF0`）读取 `transformIdList` 后调用 IndexOf。
`GetParentTransformIndex`（`0x346FF20`）同样通过父 ID 查表，并可排除组件锚点。
因此按“骨骼短名相同”或“索引恰好从 0 开始”推断映射都不成立。

**尚不能将以上默认生成规则直接套到所有已编辑 selectionData。** 源选择数据可能经过转换、代理简化或重映射；
本次未证明任意离线 selection 数组都可与自行遍历骨架的顺序直接 zip。
Blender 正式导入仍需记录已证实的点—Transform 对应，不能猜完就把未知点归到根骨。

### 12.3 原生写回存在第二次索引重映射

`TeamManager.UpdateTeamAnimatorData`（`0x5A2283C`）不是将每个组的骨序直接交给 Animator：

- 汇总同 Animator 下的相关 Transform/模拟槽，处理重复引用。
- 原绑定需要更新时走 DisableClothBindings，随后将汇总的 Transform 数组传给
  `Animator.CreateClothBindings`（调用点 `0x5A231C9`）。返回句柄按 Animator 实例 ID 保存，再 EnableClothBindings。
- 句柄包含 `count`、`invalidCount`、`validTransformIndexsPtr`、`invalidTransformIndexsPtr` 及多种读写缓冲指针。
- `0x5A23293` 读取有效 Transform 索引列表，用它找到输入 Transform 对应的模拟槽；
  `0x5A23347` 将这些模拟槽映射到本轮 Animator 写回序号。

需要严格区分：

| 索引/身份 | 表达什么 | 不能当作什么 |
|---|---|---|
| 骨链 setupIndex | 本次骨链构建列表的位置 | Mesh 权重里的局部骨索引 |
| Transform instance ID | 当前原生对象身份，用来查表 | 跨重启稳定资源身份 |
| 模拟 Transform 全局槽 | 原生管理器共享数组中的位置 | 当前组的局部点序 |
| Animator 写回槽 | 引擎接纳、重排后的缓冲序号 | 原输入 Transform 数组序号 |
| Mesh palette 索引 | 顶点权重引用该 Mesh 的局部 bones/bind poses | 全角色骨架序号 |

因此新增物理组应走游戏原生组注册，让 TeamManager 更新共享 Animator 绑定，
而不是每个 Mod/Renderer 自己 CreateClothBindings、覆盖同一个 Animator 的整套写回状态。
这不是要求每个角色类型加一套 Hook，而是同一原生注册链路内部必须遵守的映射关系。

**引擎是否接纳新加、原 Avatar 中没有的 Transform 仍待确认。**
GameAssembly 的 CreateClothBindings/Injected 包装只解析 icall 并转发，不能据此断言任意节点都会成功。
已定位 UnityPlayer 中对应 icall 名称，但尚未确认最终实现和筛选条件；未匹配到静态 xref 不代表接口不存在。
本次检查的 UnityPlayer SHA256 为
`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`。
有效/无效计数必须成为后续注册验收证据，不能以指针非空替代。

另：metadata token/RVA 已确认第 9 节的 `0x343A2E0` 为 `TeamManager.RemoveMonitoringProcess`。
它移除监控登记，不等于模拟 Job 或动画写回任务已经退出，第 9 节的安全释放限制不变。

### 12.4 本轮已实现的 DLL 诊断

[原生物理诊断](../src/eiem_native_physics_probe.h) 在原按钮上增加：

- 原生类型契约由 10 个增至 14 个，并另外输出 Animator 和 AnimationTransformRWBufferHandle 的字段/方法契约。
- 每个 process 的 `interlockingAnimatorId`。
- 非构建/非销毁状态下，读取 boneClothSetupData 的托管 Transform、ID、父 ID 和根 ID 列表。
- 输出 setupIndex、实际 Transform instance ID、记录 ID 是否相同、原生查找返回索引、组件锚点索引和 skinBoneCount。
- 不同实例即便显示层级相同，也不合并。显示层级可能截断，仅供阅读，不作节点身份。
- 数组长度不符不填充、不 zip；查找缺失输出 null，真实返回 -1 原样保留，不兜底为 0。
- 结束时重新检查列表引用/长度、setup 引用及构建/销毁状态。
  `sameBindingsAndLengthsAtEnd=true` **也不是原子快照**，因为列表内容可在相同长度下变化。

诊断只读托管列表及已确认的查找函数，不调用构建、注册、参数写入或绑定更新；
不会解引用 NativeArray/Job/RW buffer 的裸指针，也不把一次 dump 当作允许释放新增骨骼的依据。
托管引用在读取期间强持有，Unity native 有效性单独检查；结果不能证明下一时刻对象仍然存活。

### 12.5 验证与当前完成程度

完整 `build.bat` 构建通过；本次 `bin/eiem.dll` SHA256：
`79B61A5956E7A92EAF7B2218C1DA8703DDA0010F81E996DEED029A54503C2A28`。

MSVC 下执行以下测试，**44 项通过，无跳过**：

```text
python -m unittest test_native_physics_bone_probe test_native_physics_probe test_native_physics_parameters test_unity_lifetime test_skeleton_runtime test_skin_runtime test_mod_controls -v
```

其中新骨映射诊断 12 项，实际编译生产头文件，覆盖组件锚点、多实例重名、类型不符、
长度不符、ID 不符、缺查找、死亡节点、构建/销毁期间拒读、列表/对象被换、调用异常及线程边界。
类型名分配与 GC 句柄均检查释放，无写入调用。首轮测试宿主把嵌入成员地址和所属对象地址混同，
导致 setup 类型检查拒绝；修正宿主的独立对象建模后通过，未放宽生产类型检查来迎合测试。
静态 metadata 解析另外 **8 项通过**，包括新增字段范围及 token 校验。

本轮完成的是骨链/动画映射取证、按需诊断和离线回归，**不是新增物理链已接通**。
尚未接入 Render.physics、未部署 DLL、未修改游戏 Mod 或 Blender 工程。
下一步集中确认原生新增节点接纳、构建/模拟/动画绑定退出的安全边界，再接共享 Skeleton 持有权及注册/注销执行器；
保持“自制物理链要新增骨骼、隐藏 Mesh 不删除骨骼、由游戏原生系统解算”的约定。

## 13. 2026-09-07：Physics 作者资源独立交付

Blender 0.11 已实现新增物理组、共享球/等半径胶囊、五个标量参数及独立 `.physics`/`.skeleton`
读写，详情见 [Physics 作者资源 v1](physics-authoring-v1.md)。新增 Python 编解码、C++ reader/validator
及真实 Blender 编辑/保存/往返测试，连同既有 Blender 回归共 14 项通过，无跳过。

这是作者端与资源格式的交付，不包含 `Render.physics` 接线，也不新增原生构建/写入调用。
文件中 `purpose=authoring`；胶囊 span 为作者定义的球心间距，节点角色为作者枚举，
不能直接当作原生 SetSize 长度、VertexAttribute 位值或 Animator 索引。
源物理曲线、未知字段及预构建数据尚未转换，不能称作原生物理完整导入导出。
第 9、11、12 节中的新增节点接纳和 Task/Job 安全边界仍未完成游戏验证。

本轮只修改仓库文件并生成本地开发包；没有部署游戏 DLL、修改用户 Mod、开发目录插件或已有 `.blend`。

开发包：`bin/EIEM_Blender-0.11.0-physics-authoring.zip`，55,019 字节，SHA256：
`30b2ccd354e582a91137b2d9089babd65ba9385939dc504463fe5e00d943c29e`。
已从 ZIP 解压至临时目录，验证全部分发源码与仓库一致，并通过包注册/三次重载及作者往返测试。

## 14. 2026-09-07：回到 DLL 验证，增加按需调用跟踪

按用户要求，先补齐并证明 DLL 有效，再补 Blender。第 13 节的作者资源测试不构成原生运行时证据，
本轮未继续修改 Blender。新增链的 Animator 接纳、网格驱动和 Task/Job 安全注销依然没有实机结论。

### 14.1 本轮实现及限制

[调用跟踪](../src/eiem_native_physics_trace.h) 在 Dump 页提供“开始原生物理跟踪”和“停止并导出跟踪”。
仅手动开始时安装 Hook，按运行时 assembly/type、完整参数、返回类型和实例方法属性解析下列入口：

- `BeyondBoneCloth.BuildAndRun`
- `ClothProcess.StartRuntimeBuild / Init / Dispose / DisposeInternal`
- `TeamManager.RemoveMonitoringProcess`
- `ClothManager.CompleteMasterJob`

不使用静态 RVA；元数据缺失、签名不符、入口空指针或与其他可枚举方法共用地址时拒绝开始。
同一地址可能来自跨程序集代码折叠，因此检查所有已加载程序集，而非只检查待跟踪列表。
Hook 部分安装失败时不启动记录，保留已创建的被动转发入口供重试；停止记录不卸载 trampoline。
这避免在可能仍有调用经过时回收转发代码，但不意味着插件支持运行中卸载 DLL。

包装器原样转发参数、返回值和异常，只记录标量，不解引用对象、不调用 Unity API、不分配堆内存、不写文件。
[记录器](../src/eiem_native_physics_events.h) 使用有界缓冲保存进入、正常返回或异常退出、调用配对编号、
线程 ID、tick 和对象地址。地址可被复用，不是稳定实例身份；`BuildAndRun=true` 仍只表示接受开始构建。

停止后继续记录已进入调用的返回，不等待原生工作完成。尚有调用未返回或新事件未成功导出时，
拒绝用新一轮记录覆盖旧证据。缓冲满后丢弃新事件并明确累计 `dropped`，不伪装成完整记录。
导出在既有诊断 JSON 中增加 `lifecycle`；只有写入、flush、close 全部成功才确认已导出。
如果停止时 `inFlightCalls>0`，之后再次使用诊断/导出按钮保存迟到的返回。

**`inFlightCalls=0` 只代表本轮观察到的函数调用已返回，不代表其启动的 Task、Job 或动画写回已退出。**
这些函数的调用次序也不能证明新增节点被 Animator 接纳，仍需构建和写回的独立实机证据。

集成检查发现既有物理诊断 `WM_APP+0x316` 与 Mod reconcile 冲突，新跟踪入口原先选择的 `0x317`
也与 Mod key 冲突；物理诊断和启停命令已统一移至 `0x320..0x322`，加入编译期及回归检查。
否则游戏窗口会先匹配诊断分支，误吞正常的 Mod 更新/输入消息。

### 14.2 已执行验证与下一步

完整 `build.bat` 构建通过，构建标识 `resource-runtime-v49-physics-lifecycle-trace`。
`bin/eiem.dll`：4,900,864 字节，SHA256：
`A5A2757540A0EAC4846AD0D950DDF312D78EED7CF8D1BD56F056994B820E968F`。
首次命令重复初始化 MSVC 环境导致脚本解析失败，直接运行构建脚本后通过；未修改系统环境或构建脚本。

MSVC 宿主测试共 **56 项通过，无跳过**：原有物理/骨架/蒙皮/Mod 测试 44 项，
新增跟踪测试 11 项及热键工作线程回归 1 项。跟踪测试编译生产头文件，
覆盖参数和返回值转发、异常传播、嵌套及多线程配对、停止后的迟到返回、缓冲溢出、导出确认、
签名及共享地址拒绝、Hook 部分失败后重试、目标变更和窗口消息编号冲突。
测试中的 MinHook 安装由宿主模拟，不代表真实游戏代码已被成功 Hook。

本轮仅生成本地 DLL，未部署到游戏目录；检查时未发现运行中的 Endfield，因此没有游戏日志或画面验收。
下一步先采集原生组件一次构建/销毁的实际调用记录，并核实安全边界，再在 DLL 内实现不依赖 Blender 的最小新增链实验：
必须同时记录新增 Transform 身份、Animator 有效/无效绑定、骨骼随模拟变化及网格表现，
并覆盖构建中取消、F10 和实例销毁。上述证据齐全后才接完整 `Render.physics` 与作者端流程。

## 15. 2026-09-07 文档核对时的工作区状态

本节保留清理文档时的快照；随后进行的源码整理、测试和构建见第 16 节。

- `eiem_mod_document.h`、`eiem_physics_asset.h` 新增了 Physics 声明、不可变文件快照、Skeleton 引用校验草稿。
  启动标识已改为 v50，但这些新改动尚未编译或测试，不能称为已交付 v50。
- `eiem_mods.h` 明确拒绝发布含非空 Physics 动作的 Mod；没有原生组件工厂、注册和安全注销的生产接线。
- 此前复验的 61 项是上述改动之前的宿主/格式测试；没有新增链的实机验收证据，不能证明 v50 草稿正确。
- 曾新增的独立路线图含有未经证明的组合退出判据，现已删除，不再作为实施依据。
  DisposeInternal / CompleteMasterJob 返回与 inFlightCalls=0、dropped=0 的组合仍不能证明
  构建任务、模拟 Job 和动画写回全部停止；第 9、14 节的证据边界继续有效。
- 文档整理不修改生产调用，也未构建或部署新的 DLL；功能接入暂时暂停，待文档核对结束后再继续。

## 16. 2026-09-07：v50 资源草稿的独立审查与本地验证

本轮继续不依赖原生退出结论的工作。**object lifetime / teardown 尚未验证**；
普通函数或 hook 返回、trace 计数归零及宿主测试成功，都不构成原生 Tasks、Jobs 或 Animator 写回完成的证明。

### 16.1 源码与测试范围

- 复核 `eiem_physics_asset.h` 的文件快照、路径解析、Skeleton 解码及骨骼引用校验，
  以及 `eiem_mod_document.h` / `eiem_mods.h` 的配置准备和发布分支。
- 将流解析与文件解析的 Mod 合并统一到 `EiemAppendModDocument`，共用状态索引调整逻辑。
- 新增 [Physics 资源测试](../tests/test_physics_resources.py)，编译并执行实际 C++ 读取器和 Mod 加载器。
  文件全部在测试临时目录生成，不创建 Unity 对象，也不运行原生物理。
- 12 项新增测试覆盖：Python 作者文件到 C++ 的读取、Skeleton v1/v2、七位长度字符串、UTF-8 路径、
  不可变快照、缺失骨骼、损坏/超限资源、路径穿越与目录 junction、资源别名、未激活分支、
  Skeleton 不一致、跨 Mod 同名资源隔离，以及完整加载/重复加载。
- 实测加载器拒绝含非空 Physics 动作的整份 Mod，包含未激活或随后清空的动作；其规则、变量、
  快捷键和 UI 均不发布。普通 Mod 与仅声明资源、没有非空 Physics 动作的 Mod 仍可发布。

### 16.2 验证记录

在 MSVC x64 开发环境中，源码合并整理后执行下列两组检查，分别 29 项与 35 项，全通过、无跳过：

```text
cd tests
python -m unittest test_physics_resources test_mod_program test_mod_controls test_hotkey_worker test_persistent_state -v
python -m unittest test_model_reload_lifecycle test_mesh_resource_cache test_material_resource_cache test_lua_ui test_resource_pipeline_contracts -v
```

合计 64 项包含宿主可执行测试和静态契约检查，不是新增物理链的游戏验收。
此前先运行的 43 项相关基线检查也通过，但与上述检查有重复，不相加计算覆盖数量。

`build.bat` 完整本地构建成功，包含 EIEM 及两个代理 DLL。产物使用当前整个工作区源码，
不能将其中已有的其他修改归为本轮实现，也不表示这些功能全部完成实机验收。
`bin/eiem.dll`：4,950,528 字节；SHA256：
`4cefb9e6cf1e547b4c89c4218c9ca327d0e70d112ab39297e030516994cd8671`。
本地构建日志：`bin/diagnostics/v50-resource-contract/build.log`。本轮没有部署或游戏目录写入。

### 16.3 仍缺少的证据与可继续的工作

新增链的 Animator 接纳、模拟到网格的驱动、构建取消和注销后的全部原生引用退出仍未实机验证。
允许释放新增 Skeleton 节点，需要明确覆盖构建 Task、模拟 Job、Animator/跨帧写回及其他消费者的
completion fence 或对应的真实运行时证据；当前跟踪不提供该凭证。

依赖该假设的自动骨架释放和完整原生装配/注销接线，必须先补齐其具体前置条件。
不依赖该假设的资源格式、Blender 编辑、诊断、源码实现、编译、静态分析和测试可以继续。
这次构建与测试推进了资源层验证，没有关闭 Physics 执行拒绝分支，也没有补出原生完成栅栏。

## 17. 2026-09-07：审查纠正与 v51 诊断构建

### 17.1 当前状态及此前误判

此前对话声称“DLL 运行基础已经就绪，只需连接 Render.physics”，并据此建议先做 Blender，
这一判断不受仓库证据支持，现明确撤回。v50 只验证了资源读取与依赖检查；
原生组件创建、Animator 接入、执行、取消、实例所有权及安全注销尚未接通或验证。
`eiem_mods.h` 对非空 Physics 动作的整份 Mod 拒绝仍然存在。
这不是只剩下一处字段赋值或接口转发的状态。

按用户最新决定，后续以 DLL 原生机制和最小链实证为优先。此前获授权写入的 Blender v2 工作保留为离线作者实现：

- 新增源图、完整字段/曲线/预构建字节保留、共享碰撞引用、可选源导入、独立作者导出和 C++ v2 树读取。
  完整范围和限制见[源数据作者 v2](physics-authoring-v2.md)。离线样本往返不等于原生配置实例化成功。
- 删除未经验证的胶囊端点/长度/方向推导；原生碰撞体暂只显示源中心标记，参数仍可保留/编辑。
- 撤回组合 Mesh+Physics Mod 导出 UI 和动作生成。非空组合参数在写入前报错；独立 `.physics` 导出和 Mesh-only 保留。
  回归测试确认拒绝时不覆盖原 `mod.ini`。未删除生产加载器的拒绝分支。
- 早期试作生成的 `bin/diagnostics/blender-physics-v2/mesh-physics` 是旧测试输出，
  不作为当前可用 Mod 或交付样例；该测试目录增加说明保留历史证据。

### 17.2 DLL 新增的按需观测

[契约诊断](../src/eiem_native_physics_contract_probe.h) 接入既有“原生物理诊断”按钮：

1. `engineEntryPoints`：检查 Animator 元数据确有对应 InternalCall 后，通过运行时
   `il2cpp_resolve_icall` 解析 Create/CreateByName/Enable/Disable/DestroyClothBindings 的实际地址，
   记录所属模块、RVA、是否位于可执行页。**不调用这些地址，不创建绑定，不保存为生产分派表**。
   元数据缺失/歧义、解析 API 缺失、空返回和不可执行地址均明确报告；不退回固定 RVA。
2. `buildResult`：精确匹配 `ClothProcess.get_Result → BeyondDynamicBone.ResultCode`，
   从返回的值类型副本读取 IsSuccess/IsProcess/IsCancel/IsError/IsWarning。
   副本使用 pinned GC handle；在解箱数据上调用精确匹配的 Boolean 谓词，不写 process 字段。
   缺失、异常或类型错误输出 unavailable/null，不伪装为 false 或成功。

这些观测用来区分“发起构建”和“结果状态”，并为静态分析提供真实引擎地址。
结果副本、BuildAndRun 返回值、函数跟踪返回和计数归零均**不是 Task/Job/Animator 完成栅栏**。
现有组件的 ResultCode 也不能证明新增骨骼进入 Animator 或驱动网格。
本轮未加入自动组件工厂、重绑 Animator、碰撞 setter 或骨骼释放调用。

### 17.3 UnityPlayer 静态线索（等待运行时地址交叉确认）

只读检查本机 UnityPlayer.dll，大小 33069624 字节，SHA256：
`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`。
证据保存于 `bin/diagnostics/v51-physics-contract-probe/engine-binding-candidates.json`。
静态字串和函数指针数组按 Animation 子表配对得到候选入口；**尚未以运行时 resolver 核对**，
不能将候选地址写入生产 Hook/调用表。连续字符串区还包含 Android 子表，不能把整个连续区当成同一张配对表。

候选 CreateClothBindings_Injected 为 `0xFDBA20`，转入 `0x132E8E8`；包装复制 112 字节的返回数据。
后续候选路径 `0x327E70 → 0x2231F0` 对输入逐项构造 32 位字符串哈希并查另一份现有表；
未命中分支 `0x2236E4` 将原输入序号加入另一列表。`0x328530` 汇集两个 16 位数量及多组缓冲指针。
这与第 12 节已知的有效/无效输入索引机制相符，但输入字符串来源、现有表的生成/扩展以及新增骨骼接纳条件仍需核实。
不能由这些线索断言任意新增 Transform 能被接纳，或断言新增节点一定不受支持。

候选 Disable 入口 `0xFDBE44` 只写启用标志；候选 Destroy 路径
`0xFDBDA0 → 0x2E4E30 → 0xEFC680` 进入对象清理。
这些静态片段没有建立跨构建 Task、模拟 Job 和 Animator 写回的完成证明。

### 17.4 本轮实际验证与产物

MSVC x64 环境执行两组检查，**54 + 28 = 82 项通过，无跳过**：

```text
python -m unittest test_native_physics_contract_probe test_native_physics_probe test_native_physics_bone_probe test_native_physics_parameters test_native_physics_trace -v
python -m unittest test_physics_native_document test_physics_document test_physics_resources test_blender_physics test_blender_native_physics test_blender_selection_export test_blender_registration -v
```

第二组使用 Blender 5.0.1，显式指定 `EIEM_BLENDER` 和真实 Typhoea 引用图 `EIEM_PHYSICS_EVIDENCE`。
测试在独立后台进程和临时目录运行，不修改用户打开的 Blender 工程。
新增 7 项 DLL 契约诊断测试编译生产头文件；测试中的反射、返回对象和 resolver 由宿主模拟，
不构成真实游戏 API 成功证据。实际 Blender 测试验证 11 组、27 碰撞体的源图和编辑往返及修正后的导出边界。

完整 `build.bat` 已通过，包含 EIEM 和两份代理 DLL。构建标识：
`resource-runtime-v51-physics-contract-probe`。
`bin/eiem.dll`：4976128 字节，SHA256：
`40B89B6B0BFC413EF7611EDC440D2DD938072A653A08F50B80FEA6ACF2A8B67E`。
构建日志：`bin/diagnostics/v51-physics-contract-probe/build.log`；验证摘要在同目录 `validation.json`。
构建包含整个既有工作区的改动，不能将其他已存在功能全部算作本轮新增或游戏验收。

**未部署，未写入游戏目录，未进行游戏画面或生命周期验收；检查时没有运行中的 Endfield。**
当前可交付的是本地诊断 DLL 和离线作者数据实现，仍不是可运行且可安全注销的游戏 Physics 集成。
下一实机证据需要记录实际 resolver 地址、原生组件构建/取消/销毁、输入 Transform 身份及有效/无效绑定、
模拟姿态与网格驱动，并核实消费者全部退出的真实条件；完成后才连接生产创建、运行及注销路径。

## 18. 2026-09-07：DLL 工厂的独立配置准备

### 18.1 纠正接入顺序和构造状态判断

第 17 节之后，对话再次将离线测试解释为“原生运行基础已验证”，并建议删除加载器保护、
以 DisposeInternal 返回作为骨骼释放条件。**这两条建议错误且已撤回；本轮没有执行它们。**
用户随后明确要求保留这些边界。本节只实现尚未被组件或原生任务消费的配置草稿，
不据测试数量、返回状态、诊断计数或本地构建推导原生工作已退出。

只读复核同一 GameAssembly 的 `BeyondBoneCloth..ctor`（`0x33F8C20`）及
`ClothProcess..ctor`（`0x33F8EF0`），证据在 `bin/diagnostics/v52-physics-config-draft/constructors.json`。
组件构造函数已分别构造 serializeData、serializeData2 和 process；所以 **process 非空不证明 Awake/Init 已发生**，
也不能用“新组件的 process 应为空”来检测初始化是否受到抑制。
这些地址仅用于静态复查，没有写入运行时分派表。

### 18.2 实现范围

[独立配置草稿](../src/eiem_native_physics_config.h) 已纳入 DLL 编译：

- 按程序集、类、完整方法签名与字段类型解析原生 ClothSerializeData / ClothSerializeData2 构造器、
  `List<Transform>` 构造/添加/读取方法、五个 Single 字段和 Transform 父级 getter。
  缺失或歧义时拒绝准备，不按同参数数量猜重载。
- 当前仅处理 **v1、无碰撞体的作者组**。v2 源图构造和原生碰撞外形转换显式拒绝，不降级或丢字段。
- 准备前校验全部作者节点的路径映射、实际 Transform 类型、原生存活及组内真实父子关系；
  不因传入数组首项恰好在索引 0 就把它当成根。
- 新建彼此独立的托管配置和根骨骼列表，填写五个标量并回读；列表数量、成员以及字段引用也回读。
  作者 FIXED/MOVE/IGNORE 数据单独保留，**尚未转换成原生 SelectionData**。
- 只有整个请求成功才替换旧草稿；任意组失败都保留已有配置，释放此次未提交草稿的托管引用。
  这些 Transform 引用仅提供托管可达性，不提供 Unity native 存活保证或骨骼释放授权。

没有调用 AddComponent、BuildAndRun、Init、DisposeInternal、Destroy 或 Animator 绑定接口；
没有修改现有源组件，没有接入 Render/F10，也没有创建生产配置草稿实例。
因此本轮是组件工厂的**配置准备实现**，不是已完成的 Unity 组件工厂或可执行物理链。
默认构造配置、节点角色、选择点顺序和游戏扩展字段仍需进一步映射及实机验收。

### 18.3 验证记录

新增 11 项宿主测试编译并执行生产配置头文件，覆盖根节点乱序、配置独立、路径/父级/类型/存活检查、
不支持格式、构造/写入异常、回读不符、后续组失败、准备中节点失效及非 Unity 线程。
测试首次编译因宿主变量与 MSVC 的 `unexpected` 函数同名失败；重命名测试变量后复验通过。
相关参数适配器 17 项和契约诊断 7 项也通过，本轮合计 35 项成功检查，无跳过；不是游戏调用证据。

最终配置测试日志：`bin/diagnostics/v52-physics-config-draft/tests.log`。
完整构建和产物记录见同目录 `build.log` 与 `validation.json`；启动标识为
`resource-runtime-v52-physics-config-draft`。本轮未部署、未写入游戏目录、未执行实机物理。
完整构建成功，`bin/eiem.dll` 为 4976128 字节，SHA256：
`40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360`。
生产加载器的非空 Physics 拒绝分支以及骨骼释放路径保持原状。

后续优先确认配置完成前抑制原生初始化的真实机制、默认选择数据生成及 Animator 接纳条件；
原生组件实验和生产接线仍需在这些具体条件得到证明后推进，不把本节草稿作为它们的完成凭证。

## 19. 2026-09-07：v52 诊断 DLL 首次部署与启动核对

确认 Endfield、UnityCrashHandler64 和 PlatformProcess 均未运行后，将本地 v52 `bin/eiem.dll`
部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。安装文件为 4976128 字节，
SHA256 为 `40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360`，与构建产物一致。
原安装 DLL 和未修改的 `eiem.ini` 备份到
`E:\EIEM_Workspace\plugin-releases\before-v52-physics-diagnostic-20260907-183816`；旧 DLL SHA256 为
`2DC3590245DB6ED7A19C14F2C1EF5E3905A47078DB5AC7D61347D78AD7B74031`。

随后启动 Endfield，`plugin/eiem_log.txt` 第 2 行记录
`resource-runtime-v52-physics-config-draft`，证明本次安装的 DLL 已被游戏进程加载。现有 Mesh Mod 正常解析，
生产加载器的 Physics 拒绝分支未修改，也没有执行自制 Physics 动作。

本轮没有取得原生物理运行记录：从当前终端向游戏窗口投递 `WM_EIEM_PHYSICS_TRACE_START` 和
`WM_EIEM_PHYSICS_PROBE` 均返回失败；日志中没有 `PHYSICS-TRACE` / `PHYSICS-PROBE` 开始记录，
`plugin/dumps` 也没有新增 `physics_runtime_*.json`。诊断入口本身独立于 Mod 加载器，失败不应归因于
非空 Physics 动作的拒绝分支；下一次由游戏内 EIEM Dump 页按钮启动跟踪和快照。

游戏随后收到 `WM_CLOSE`，日志记录现有模型实例的卸载。同期 Windows Application 日志没有
Endfield Application Error，也没有新的本地 Endfield 崩溃转储。CrashSight 日志出现
`reportException`，但该记录不含足以判定退出原因的异常详情，不能据此声称发生或未发生插件崩溃。

截至本节，只能确认 DLL 部署及加载。原生组件创建、模拟、Animator 对新增 Transform 的接纳、
构建取消、全部消费者退出和安全释放仍未验证；本轮没有删除加载保护、调用骨骼释放或改动 Mod、
Mesh、Physics 作者文件和 Blender 工程。机器可读记录见
`bin/diagnostics/v52-physics-config-draft/deployment.json`。

## 20. 2026-09-07：运行时快照与 v53 自动跟踪

### 20.1 v52 手动快照实际记录了什么

用户重新启动游戏后，通过当时的手动入口生成两份运行时快照。两份文件内容完全相同，SHA256 均为
`9AA21B4029ADC814A2D079A5E99994D88FB5D5C06328DC470391906414A085E3`，已复制到
`bin/diagnostics/v53-physics-auto-trace/v52-runtime-snapshot.json` 和
`v52-runtime-snapshot-repeat.json`。

运行时 `il2cpp_resolve_icall` 将五个 Animator cloth 接口全部解析到当前 `UnityPlayer.dll` 可执行页：

| 接口 | 运行时 RVA |
|---|---:|
| `CreateClothBindings_Injected` | `0xFDBA20` |
| `CreateClothBindingsByNameLst_Injected` | `0xFDB8E8` |
| `EnableClothBindings` | `0xFDBFA0` |
| `DisableClothBindings` | `0xFDBE44` |
| `DestroyClothBindings` | `0xFDBDA0` |

这确认了第 17.3 节对应静态候选的地址配对，但快照没有调用这些接口，仍未验证新增 Transform 的接纳。

本次枚举到 96 个已加载 BeyondBoneCloth 组件，其中 46 个 `activeAndEnabled/IsValid/IsRunning`
均为 true 且 Result 为 success，50 个均为 false。45 个运行组件有可读 BoneCloth setup；
Yvonne 的 `MC_ultMachine` 运行有效但没有 BoneCloth setup，说明不能把所有有效 ClothProcess 都当作骨链。

Typhoea 当前实例的 11 个组使用同一 Animator 实例 ID `-115842`，team ID 连续为 17～27。
每个 BoneCloth setup 的 `transformCount = skinBoneCount + 1`，最后一项对应此前静态分析发现的组件锚点。
尤其 `MBC_Typhoea_Hair_Back_Ponytail_Long` 的离线 selection 点数为 38，而运行时
`skinBoneCount=14`、`transformCount=15`。这是不能按作者节点/selection 数组和运行时 Transform 列表直接 zip
的实际反例；其他组数量偶然相等也不能推翻该反例。

调用跟踪只记录到同一线程上的 88 次 `ClothManager.CompleteMasterJob` 进入/返回，共 176 个事件、
无丢失、无未返回调用，事件跨度 31 ms。没有记录 Init、BuildAndRun、StartRuntimeBuild、Dispose 或
DisposeInternal，说明手动开始发生在相关构建之后；这份快照不能回答创建、取消或注销时序。

### 20.2 v53 改为测试构建自动采集

按用户要求，物理诊断不再作为 Dump 页主功能：删除“原生物理诊断”“开始原生物理跟踪”
“停止并导出跟踪”三个控件及其三条窗口消息。v53 在游戏窗口进入 EIEM 主线程入口后自动安装并开始
一次原生调用跟踪；正常关闭首次进入关闭分支时停止记录，并将组件快照和调用事件写到独立目录
`plugin/physics_diagnostics/physics_runtime_<pid>_<tick>.json`。它不需要 Physics Mod，也不创建组件、
调用 BuildAndRun、修改参数或释放骨骼。

诊断实现已从 `scene_dump.h` 移到独立的 `eiem_native_physics_diagnostic.h`；Dump 模块和页面均不再承担
物理诊断入口。`test_native_physics_trace` 与 `test_hotkey_worker` 共 13 项通过；完整 `build.bat` 构建成功。
最终 v53 `bin/eiem.dll` 为 4974592 字节，SHA256：
`1403CADBB80F61A5EDA31556B49832AE13FF33328B00309214E983840BD4649B`。
确认游戏进程退出后已部署，安装文件哈希与构建产物一致，未修改 `eiem.ini`、Mod 或作者资源。

部署前创建 v52 备份的命令错误使用了本机 PowerShell 不支持的 `New-Item -LiteralPath`，目录和备份未创建；
命令后续仍覆盖了安装 DLL。现有发布归档中未找到 v52 哈希
`40E04FAFABE85C6B353BACCDB85C0F96B62FDEF48394565C2B5AB6BD6C72C360` 的副本；
第 19 节备份目录中仍保留部署 v52 之前的旧 DLL。v53 安装文件和仓库构建产物已经重新核对一致。

随后使用功能相同、诊断代码尚未拆出独立头文件的首个 v53 构建启动 PID 40436。日志记录构建标识
`resource-runtime-v53-physics-auto-trace`，并在游戏窗口完成子类化后记录
`[PHYSICS-DIAGNOSTIC] automatic native physics trace started`。正常关闭时自动写出
`plugin/physics_diagnostics/physics_runtime_40436_31457484.json`，证明自动开始和关闭导出均已实际执行。
该运行 DLL SHA256 为 `A42B802E53C3B626893D56C45F7DFF04D107CDE81CF1A3A475191E7FE8698220`，
现备份于 `E:\EIEM_Workspace\plugin-releases\before-v53-diagnostic-header-split-20260907-191207`。
最终安装版仅将相同诊断逻辑移入独立头文件；其后续实机加载结果见第 20.3 节。

自动记录为 492974 字节，SHA256
`F3D2724C44A5BA7D97FBEC419F8BE2E7C2911F91C5E39904E2667B33FA561079`，仓库副本为
`bin/diagnostics/v53-physics-auto-trace/runtime-auto-shutdown.json`。记录覆盖 357016 ms，634 个事件，
`dropped=0`、`inFlightCalls=0`：46 次 Init、46 次 RemoveMonitoringProcess、45 次 StartRuntimeBuild
和 180 次 CompleteMasterJob 均有进入/返回；45 次 StartRuntimeBuild 全部返回 true。

对 45 条 BoneCloth 链，实机顺序均为 `Init → RemoveMonitoringProcess → StartRuntimeBuild`；唯一没有
BoneCloth setup 的 `MC_ultMachine` 只有 Init 和 RemoveMonitoringProcess。这证明在本次路径里
RemoveMonitoringProcess 是初始化/重建序列的一部分，不能按名字解释成注销完成通知。关闭前快照仍有
46 个有效运行 process，记录没有 Dispose 或 DisposeInternal；导出发生在转发 `WM_CLOSE` 之前，
所以它没有观察游戏后续关闭阶段的原生清理。

即使本次调用全部成对且无丢失，也只说明所 Hook 函数的观察结果，不会自动成为 Task、Job、Animator
写回或骨骼释放的完成栅栏。新增 Transform 接纳、取消和完整注销仍需后续有针对性的运行记录。

### 20.3 最终 v53 构建的中途注销记录

最终拆分版 v53 DLL（SHA256
`1403CADBB80F61A5EDA31556B49832AE13FF33328B00309214E983840BD4649B`）随后由 PID 31592 实际加载。
正常退出生成 `plugin/physics_diagnostics/physics_runtime_31592_32160296.json`；仓库副本为
`bin/diagnostics/v53-physics-auto-trace/runtime-final-header-split.json`，1061368 字节，SHA256
`802C6EE9DF8462A8ECF62ECB2DCC410DFE332BD53B6E5E221125D3537054C35A`。

记录覆盖 52125 ms，共 3402 个事件、1701 次调用。全部事件来自线程 20672，1701 次调用均有配对的
进入和返回，`dropped=0`、`inFlightCalls=0`。操作统计如下：

| 操作 | 调用数 | 观察到的返回 |
|---|---:|---|
| `ClothProcess.Init` | 298 | 298 次返回 |
| `TeamManager.RemoveMonitoringProcess` | 293 | 293 次返回 |
| `ClothProcess.StartRuntimeBuild` | 137 | 137 次均为 true |
| `ClothManager.CompleteMasterJob` | 735 | 735 次返回 |
| `ClothProcess.Dispose` | 119 | 119 次返回 |
| `ClothProcess.DisposeInternal` | 119 | 119 次返回 |

119 个被注销的 process 均呈现同一嵌套顺序：

`Dispose enter → DisposeInternal enter → RemoveMonitoringProcess enter/return → DisposeInternal return → Dispose return`

注销集中在 tick 32137562～32137718。此后 tick 32150796 再次出现 `CompleteMasterJob`，tick 32154156
又出现新的 Init 和 StartRuntimeBuild，随后才停止记录并导出。因此本次捕获的是运行中途的生命周期切换，
不是转发 `WM_CLOSE` 之后的进程终止清理。它确认了现有游戏对象在该路径中的同步调用嵌套；同时也进一步
说明 `RemoveMonitoringProcess` 同时出现在初始化/重建和注销上下文，不能脱离调用栈按名称解释其语义。

导出时枚举到 123 个组件：55 个 `activeAndEnabled/IsValid/IsRunning` 均为 true 且 Result 为 success，
68 个三项均为 false。54 个运行组件有 BoneCloth setup；运行中的 `MC_ultMachine` 仍没有该 setup。
本次依旧只观察游戏原有组件，没有由 EIEM 创建新组件或提交作者物理数据。

`DisposeInternal` 和外层 `Dispose` 返回现在属于已观察事实，但 Hook 没有覆盖原生 Task/Job 完成通知、
Animator 写回结束或管理器内部引用退休。因此这些返回仍不能单独作为释放 EIEM Transform/骨骼的完成栅栏；
新增 Transform 接纳、构建取消和 EIEM 自建组件的完整注销仍未验证。

### 20.4 v54 自动跟踪 Animator binding（实机启动失败，未安装 Hook）

下一轮诊断继续留在独立测试模块，不恢复 Dump 控件或窗口消息。v54 在既有自动跟踪中加入五个已由
运行时 resolver 和元数据共同确认的 Animator InternalCall：

- `CreateClothBindings_Injected`
- `CreateClothBindingsByNameLst_Injected`
- `EnableClothBindings`
- `DisableClothBindings`
- `DestroyClothBindings`

安装 Hook 前逐项核对程序集、类型、完整参数与返回类型、InternalCall 标志、resolver 返回值及可执行页；
不使用固定 RVA。Hook 只转发游戏原调用并记录进入/返回。两个 Create 入口在原调用正常返回后读取
`AnimationTransformRWBufferHandle` 已由本次元数据确认的前两个 `UInt16` 字段 `count/invalidCount`；
它不遍历输入数组、有效索引表或 Transform，也不自行调用创建、启用、禁用和销毁接口。

`test_native_physics_trace`、`test_native_physics_contract_probe` 和 `test_hotkey_worker` 共 20 项通过，
覆盖完整参数转发、两个 Create 返回计数、InternalCall 契约失败、Hook 部分安装失败、异常展开、并发及
自动入口仍与 Dump 隔离。完整 `build.bat` 构建成功。v54 `bin/eiem.dll` 为 5505536 字节，SHA256：
`937448ACD0D3B778FD887B5D1ACB42AB920A2A8A7F9B8A6207E48B8690B573BF`。

确认游戏退出后已将 v53 备份到
`E:\EIEM_Workspace\plugin-releases\before-v54-physics-binding-trace-20260907-193945`，随后部署 v54；
游戏目录 DLL 与构建产物哈希一致。

PID 41236 随后实际加载 v54，但日志在安装任何物理 Hook 前报告
`Physics Animator icall signature unavailable: CreateClothBindings_Injected`。退出快照
`bin/diagnostics/v54-physics-binding-trace/runtime-start-failure.json` 为 375931 字节，SHA256
`8F32C9BF2078331A66DCDC608C3CFFDB9EAD0AAA24A2EF4035665F455F7DCA6C`；其中 lifecycle
`session=0/sequence=0/events=[]`，证明本轮没有 binding 或 ClothProcess 跟踪事件。

根因是 v54 Hook 安装器把 Animator 程序集误写成 `UnityEngine.CoreModule.dll`，而实际探针一直从
`UnityEngine.AnimationModule.dll` 读取 Animator。同一失败快照仍从正确程序集记录到精确的
`CreateClothBindings_Injected(Transform[], AnimationTransformRWBufferHandle&)` 元数据，并把五个 icall
全部解析到 UnityPlayer 可执行页。这是诊断实现错误，不是游戏缺少 binding 接口；v54 不能作为这些接口
没有被游戏调用的证据。

### 20.5 v55 统一 Animator 程序集契约（已构建部署并成功启动跟踪）

v55 在 `eiem_native_physics_api.h` 定义唯一的 `EiemPhysicsAnimatorImage` 常量，探针与 Hook 安装器共同使用
`UnityEngine.AnimationModule.dll`，删除两处独立字符串造成的漂移。宿主测试也使用同一常量，并静态拒绝
物理探针或跟踪器再次引用 `UnityEngine.CoreModule.dll`。

修复后第 20.4 节的 20 项检查再次全部通过，完整 `build.bat` 构建成功。v55 `bin/eiem.dll` 为
5505024 字节，SHA256：`514A9E661E8C274BB891A545A6C2E41786706CC15F54D0A69ACDB1C8067869BB`。
游戏退出后将 v54 备份到
`E:\EIEM_Workspace\plugin-releases\before-v55-physics-binding-trace-20260907-194722`，随后部署 v55；安装文件
与构建产物哈希一致。

用户随后重启游戏，PID 37728 实际加载该 v55；`plugin/eiem_log.txt` 同时记录构建标识和
`[PHYSICS-DIAGNOSTIC] automatic native physics trace started`，确认程序集修复后跟踪安装成功。
正常退出后生成的快照已复制为
`bin/diagnostics/v55-physics-binding-trace/runtime.json`，8082130 字节，SHA256：
`022893B718A4A14CEED022C1620A148A8CC3D1DD246C78BF45DF03A6472EA5F9`；机器可读统计见同目录
`summary.json`。记录跨度 1164453 ms，共 32236 个事件、16118 次调用，全部来自线程 24856，全部调用
进入/返回成对，`dropped=0`、`inFlightCalls=0`。

其中 `CompleteMasterJob=14592`、`Init=446`、`StartRuntimeBuild=181`（全部返回 true）、
`RemoveMonitoringProcess=465`、`Dispose/DisposeInternal=217/217`。217 次 Dispose 仍都是
`Dispose → DisposeInternal → RemoveMonitoringProcess → return` 的同步嵌套；它仍不是异步退休栅栏。
快照枚举到 138 个组件，其中 61 个 IsValid/IsRunning 为 true，并关联 6 个非零 Animator 身份。

五个 Animator binding Hook 的调用数全部为 **0**。这不是“新增 Transform 已被接纳”的证据，也不能证明
接口在跟踪窗口外从未使用；它说明本次长时间记录即使覆盖 181 次运行时构建和 217 次注销，仍没有触发
`Create/Enable/Disable/DestroyClothBindings`。因此不能把静态定位到的 `TeamManager.UpdateTeamAnimatorData`
分支当成每个 ClothProcess 构建必经步骤，下一轮需要直接观察 TeamManager 的登记、dirty 和汇总更新入口。

### 20.6 v56 工作区：Render 命中与角色 UI 的模型级 Physics 意图

用户指出物理骨骼的增删必须沿用 Mesh 已有 INI 命中语法，并要求核对角色 UI。源码复核结果如下：

- `EiemSetRenderField` 已让 `physics=` 进入 Render 条件语句的普通求值；`asset`、相对 `path` 和 Mesh
  形状条件仍由 `EiemRenderRuleMatches` 统一判断，首个匹配规则仍优先。
- `CharUIModelMono` 能以自身 GameObject 注册具体 UI 模型，但没有 PFB 路径；因此顶层 `asset=` Render
  是角色展示 UI 的必要入口。`OnRelease` 在游戏原释放前移除 owner，`SetVisible(false)` 此前没有独立状态。
- Renderer setter Hook 可以只拿到单个 Renderer/Mesh，未必能得到模型根；它不能承担模型级物理创建或销毁。

v56 工作区据此增加了不执行原生调用的 Physics 意图层。Renderer 在注册模型根遍历中赢得现有首个命中后，
将规则中的 Physics 快照加入 `EiemModelInstanceState`；同一 Mod 内引用同一准备快照的多个资源别名、Renderer
或 LOD 按快照身份去重。每次注册、池化复用、F10 或条件/UI 事务 reconcile 都重建当前模型计划；收集发生在
受影响 Mod 过滤之前，所以较高优先级的未改动规则仍会阻止低优先级规则被错误提升。`physics=` 被清空或条件
切换后，意图从新计划消失，但当前代码没有把这一变化当作可直接释放 Transform 的完成凭证。

角色 UI owner 现另存 active 状态：`OnAwake` / `SetVisible(true)` 激活并复用计划，`SetVisible(false)` 保留计划但
标记不活跃，`OnRelease` 才移除 owner。模型是否仍有活跃 owner 独立计算，给后续 native adapter 的
enable/disable/retire 状态机提供输入；本轮没有实现该状态机。

针对性验证为 Physics 资源/意图 13 项和 Render/角色 UI 契约 33 项，共 46 项通过；完整 `build.bat` 通过。
本地 v56 `bin/eiem.dll` 为 5515264 字节，SHA256：
`34203F39C109222223C60D1C680231606BAE583A25F01C8240762AAB1611EA16`。该独立 v56 构建没有部署；其规划代码
随后保留在第 20.7 节已部署的 v57 中。由于生产加载器仍整份拒绝带 Physics 动作的 Mod，部署包含这些代码
不等于计划层已在游戏中接收 Physics Mod 或执行原生物理。

生产加载器的整 Mod Physics 拒绝分支保持不变；本轮没有创建组件、启动模拟、调用 Animator binding、
修改骨骼释放或证明原生退休边界。v55 的零 binding 结果直接产生第 20.7 节的下一轮管理器跟踪；模型意图
仍需等待可验证的 native adapter 状态机，不能越过该证据缺口直接执行。

### 20.7 v57 自动跟踪 TeamManager Animator 汇总入口（已部署，待实机加载）

根据 v55 的零 binding 结果和静态方法映射，v57 在原 12 个 Hook 上增加五个精确签名的 TeamManager 入口：

- `UpdateTeamAnimatorData(ExNativeArray<Int16>, TransformAccessArray)`；
- `ClearTeamAnimatorData(Int32)`；
- `AddTeamAnimatorData(Int32, ClothProcess)`；
- `AddAnimatorTransform(Int32, Transform)`；
- `MarkAnimatorTransformDirty(Int32, Transform)`。

这些 wrapper 只原样转发参数并记录 manager、关联对象和 teamId；不读取值类型内容，不调用 Animator，
也不改变团队、Transform 或 binding 状态。这样下一次记录可以区分“团队从未进入汇总更新”、
“进入更新但无需重建 binding”和“进入更新且实际调用 Animator icall”三种情况。

`test_native_physics_trace`、`test_native_physics_contract_probe` 与 `test_hotkey_worker` 共 20 项通过，完整
`build.bat` 成功。v57 DLL 为 6043136 字节，SHA256：
`3AB2482DCF33270A0D4165B6E8B508709E864A68A8EB0FAC5381D72BF9BBE542`。确认游戏进程全部退出后，已把 v55
备份到 `E:\EIEM_Workspace\plugin-releases\before-v57-physics-team-animator-trace-20260907-202605` 并部署 v57；
游戏目录与构建产物哈希一致，未修改 Mod 或全局配置。v57 尚待下一次游戏启动与正常退出生成记录。

该部署仍是被动测试 DLL。生产 Physics Mod 拒绝分支、原生组件创建、SelectionData 转换、模拟启停和骨骼
释放均未连接或改变。

### 20.8 v58 统一 Mesh 消费者语义并补齐 NPC 最终 Render 边界（已部署，待实机加载）

现行领域约定是 Render 按 Mesh 身份作用于所有消费者，PFB 只保存资源关系和实例来源。复核发现
`EiemCompileModProgram` 曾把 `Prefab.render.N` 与 `partner.N` 一起视为模板引用，前者会把源 Render 排除出
全局规则；运行时随后又只在精确 PFB path 命中时执行它。这与上述约定冲突。v58 改为只有 `partner.N`
指向的额外 Renderer 模板不参与源 Mesh 匹配；PFB 引用的 Render 仍进入全局规则，并删除按 PFB path 的
第二次 Render 执行，避免同一消费者重复应用。`physics=` 的规划使用同一 Render 命中，因此已注册模型根上
也采用该全局语义。

实机旧日志同时确认 NPC 的 `CreateSMSGO → AssignSkin → SetSMRRootBone` 顺序和后者收到的具体 Renderer 数组。
v58 在游戏原 `SetSMRRootBone` 返回后，对该数组重放 mesh、material、skip、partner 等全局 Render 动作，
用于覆盖 NPC 最终组装阶段。该数组没有可靠模型根，所以此入口不创建或删除 Physics，也不单独写入模型级
Physics 意图；NPC 的 Physics 所有权仍需后续用明确模型 owner 接入。

解析/编译、真实多模型登记、重载生命周期、Render 合同和 Physics 资源检查共 62 项在 MSVC 环境通过，
完整 `build.bat` 通过。v58 DLL 为 6040064 字节，SHA256：
`CF0AFAA0972E12C0583332438A27514DD11E6F153F88BF150CD022161FC3EF39`。确认游戏进程未运行后，v57 备份到
`E:\EIEM_Workspace\plugin-releases\before-v58-global-mesh-consumers-20260907-210846`，v58 已部署且游戏目录
哈希一致。部署前的最新游戏日志仍标记 v55，说明 v57 没有获得实机加载记录；v58 保留 v57 的自动物理跟踪。

本节没有移除生产 Physics Mod 拒绝分支，没有连接原生组件创建、执行、取消或骨骼释放。v58 的 Mesh/NPC
行为和保留的物理跟踪都尚待下一次游戏启动验证。

### 20.9 v58 NPC 回放被实机否定，v59 改用具体 Renderer 初始化入口（实机验证）

v58 已由游戏进程加载。PFB 引用不再限制 Render 的全局 Mesh 身份语义，这部分由启动时 4 条源规则以及
普通模型、角色 UI 的 Typhoea 命中记录确认。但 52 次 `SetSMRRootBone` 记录的 `applied` 均为 0，20.8
提出的最终骨骼数组回放在本次实机中没有覆盖目标实例，不能继续作为 NPC 接入依据。

日志中的 Renderer `00000011E4D09F40` 和 `0000001022369BA0` 均以 917 顶点的原 Typhoea body Mesh
进入 `EntityRenderHelperMaterialController.RendererInfo._Init`；对应 cloth 仍启用。它们没有经过公开
`SkinnedMeshRenderer.set_sharedMesh` setter，也没有进入已登记的模型根执行器或任何成功的最终骨骼回放。
因此本次 NPC 漏应用的实证原因是具体 Renderer 构造旁路缺少执行入口，不是 PFB 或 INI 匹配错误。

v59 将 `SetSMRRootBone` 恢复为顺序观察。在游戏 `_Init` 保存干净源材质之后，具体
SkinnedMeshRenderer 进入现有全局 Mesh 身份执行器；不按 NPC 类型、角色身份或 PFB 分叉规则。
相关 63 项 MSVC/宿主检查及完整构建通过。DLL 为 6040064 字节，SHA256：
`E5E11A9A51FDDBC1CD2BF8EBF7A0D9A39392F0268E1608367CFBFFA4813DA890`；v58 备份到
`E:\EIEM_Workspace\plugin-releases\before-v59-npc-renderer-init-20260907-213635` 后已部署，尚待新进程验证
`[MOD-RENDERER-INIT]`、14664 顶点读回和 NPC 画面。

2026-09-08 的新进程已加载 `[BUILD] resource-runtime-v59-npc-renderer-init`。Typhoea body 在多个直接构造实例上
记录 `resource mesh replaced ... applied=true`，紧随其后的 `[MOD-RENDERER-INIT]` 明确给出各 Renderer；
cloth 01/02 也在相同入口执行 `skip`。用户同时确认 NPC 画面已经应用。因此本节的 NPC Mesh/skip 入口完成
实机验收；该结论不扩大到未声明的 lod1/2/3，也不扩大到下段列出的原生 Physics 状态。

本节同样没有移除生产 Physics Mod 拒绝分支，也没有连接原生组件创建、执行、取消、Animator 写回或骨骼释放。

### 20.10 v60 Mesh 命中实例的 Physics owner/Animator 定向调查（已部署，待实机采样）

针对“同一 Mesh 命中后的附加 Physics 是否能覆盖主角、NPC 与角色 UI”，本轮先重新核对当前
GameAssembly 元数据。NPC 路径已经存在明确对象关系，不需要继续由 Renderer 父链猜测所有者：

- `Beyond.NPC.Avatar.NPCAvatarManager._BuildBeyondCloth` 的参数同时包含
  `NPCAvatarMeshAssetsSO`、`UnityEngine.Animator`、`FNPCAvatarGOReference&` 与模型
  `GameObject`；这是游戏自身为 NPC 建立 BeyondCloth 的精确构建边界。
- `NPCAvatar.avatarGoRef` 是内嵌值类型；其中保存 `animator`、`go` 与
  `List<BeyondBoneCloth> boneCloths`。其成员偏移必须按 IL2CPP 非装箱值类型解释，不能把
  `avatarGoRef` 字段本身当成托管对象指针。
- `NPCAvatar.StartNPC(NPCCrowdEntityComponent)` 可把已构建模型绑定到稳定 Avatar owner；
  `NPCAvatarManager.ReleaseAvatar(NPCCrowdEntityComponent)` 与
  `NPCCrowdEntityComponent.OnRelease()` 提供 NPC 池化释放边界。`NPCCrowdEntityComponent.avatar`
  能把释放参数映射回同一 `NPCAvatar`。

据此新增独立测试探针（现位于 `eiem_npc_model_owner.h`，当时名为 `eiem_physics_owner_probe.h`）。它随资源 Hook 自动安装，没有 Dump/ImGui/
热键入口，只在现有 Render 规则已经命中 Mesh 后观察：注册模型中的 Animator 与现有
BeyondBoneCloth 数量、NPC 构建时游戏实际传入的 Animator/模型根、StartNPC owner，以及对应释放入口。
探针先调用游戏原 `_BuildBeyondCloth`/`StartNPC`，再读取返回后的关联；不调用 AddComponent、
BuildAndRun、Animator binding、Dispose 或 Destroy，也不收集/执行 Physics intent。

完整 `build.bat` 已通过；`python -m unittest discover -s tests` 发现 80 项，其中 35 项执行通过、
45 项因当前环境条件跳过。另一次定向调用覆盖 42 项，其中 37 项通过、5 项跳过。两次集合重叠，
不相加计数。v60 `bin/eiem.dll` 为 6201344 字节，SHA256：
`9DAB7BEE2ECFCACEDCB7FEF20C5B8530AE12480D7CB18560B6F14C92A476A4F5`。确认游戏进程退出后，
v59 已备份到
`E:\EIEM_Workspace\plugin-releases\before-v60-physics-owner-probe-20260908-021212`，随后部署 v60；
安装文件与构建产物哈希一致。

v60 随后由新游戏进程实际加载。四个 NPC Hook 均安装成功，元数据解析得到
`avatarGoRef=0x120`、`component.avatar=0x140`、`goRef.animator=0x10`、`goRef.go=0x30` 和
`goRef.boneCloths=0x48`。7 次 `StartNPC` 记录均满足 `component.avatar == avatar`、
`GetModelGo() == avatarGoRef.go`，且内嵌 Animator 非空，确认这条 owner 映射可直接使用。

主场景中命中的 Wulfa 与 Typhoea 模型各枚举到 1 个 Animator 和 11 个现有 BeyondBoneCloth。
Typhoea 角色 UI 在 `OnAwake`/首次可见时枚举到 1 个 Animator，随后一次 `SetVisible` 枚举到
5 个 Animator；两个 UI 实例均为 0 个现有 BeyondBoneCloth。日志同时记录到
`SetVisible(false)` 和 `UIModelLoader.UnloadModel`。因此 UI 不能复用场景模型已有的 Cloth，且不能在
5 个 Animator 中任取一个；必须按命中 Renderer 的实际祖先关系选出对应 Animator。

自动生命周期文件记录 2510 个成对事件、1255 次调用，`dropped=0`、`inFlightCalls=0`：
`AddAnimatorTransform=902`、`AddTeamAnimatorData=50`、`ClothProcess.Init=60`、
`StartRuntimeBuild=47`、`RemoveMonitoringProcess=50`、`CompleteMasterJob=146`。这证明游戏原有 Cloth
路径会进入 TeamManager 的 Animator 数据入口；该记录没有 EIEM 自建组件，也没有 Animator binding
icall 事件，不能据此声称新增骨骼已被接纳。

v60 的 `_BuildBeyondCloth` wrapper 被实机否定：它按实例方法多声明了一个 `self`，导致唯一一条日志的
参数整体错位。对 GameAssembly 中该方法入口的反汇编显示它是静态方法，真实原生参数依次为
`meshConfig, animator, goRef&, model, MethodInfo*`。因此 v60 的 Build 日志不作为正确 ABI 证据；
运行日志和生命周期 JSON 已归档到 `bin/diagnostics/v60-physics-owner-probe/`。

### 20.11 v61 修正 NPC 构建 ABI 与 Renderer/Animator 关联（已实机验证）

v61 将 `_BuildBeyondCloth` detour 改为上述 5 参数静态函数布局。`StartNPC` 现在即使此前没有发生
`_BuildBeyondCloth`，也会用已经实测一致的 `GetModelGo()`、内嵌 Animator 和 owner 建立 NPC 记录，
再把此前命中的 Renderer 与模型根关联。这样不会再把“该 NPC 没有 BuildCloth 调用”误写成
“该 Renderer 不是 NPC”。

同一探针还会在普通场景模型和角色 UI 模型中枚举 Animator，并沿命中 Renderer 的 Transform 父链计算
每个 Animator 是否为祖先，记录祖先数、最近 Animator 和层级距离。这个结果用于回答主角、NPC 与 UI
分别应把新增 Physics 接到哪个 Animator；它不预设 UI 的第一个 Animator 就是正确对象。

本轮同时修正模型释放观察：调用方已经给出 owner 时只按 owner 移除记录，只有没有 owner 时才按模型根
移除，避免同一模型由 `PrefabProxy` 与 `CharUIModel` 双重登记时一次释放误删两项。新增探针契约共 9 项
通过；完整 `build.bat` 构建通过；普通测试发现 83 项，其中 38 项执行通过、45 项因当前环境条件跳过。
v61 `bin/eiem.dll` 为 6203904 字节，SHA256：
`DC6C1BE763C613B981CDFBAE7DDB3F8785DACCD2205512AE6CDA0191006280CE`。
确认相关游戏进程退出后，已将 v60 备份到
`E:\EIEM_Workspace\plugin-releases\before-v61-physics-owner-correlation-20260908-024409` 并部署 v61；
游戏目录 DLL 与构建产物哈希一致。

PID 23656 已实际加载 v61，四个 Hook 全部安装成功。正确 ABI 下记录到一次 `_BuildBeyondCloth`：
`refAnimator == animator`、`refModel == model`，确认修正后的 5 参数转发和内嵌值类型读取均正确。
该实例在原调用返回后的 `boneCloths=0`。

日志共记录 9 次 `StartNPC`。其中两个 Typhoea 目标 NPC 各自把 3 个命中 Renderer 关联到唯一的模型根、
`NPCAvatar`、`NPCCrowdEntityComponent` 和 `avatarGoRef.animator`，共 6 条 `npc-render-match`；两者都是
`buildSeen=0`、`embeddedCloths=0`。这证明目标 NPC 不会依赖 `_BuildBeyondCloth` 建立原有 Cloth，但
`StartNPC` 已提供附加 Physics 所需的稳定模型和 Animator 身份。两个目标随后都记录到完整嵌套释放顺序：
`NPCCrowdEntityComponent.OnRelease enter → NPCAvatarManager.ReleaseAvatar enter/return → OnRelease return`。

Typhoea UI 的三个命中 Renderer 在 `OnAwake` 时都位于同一个 Animator 下，距离均为 3；稍后的
`SetVisible` 虽然能从整个模型枚举出 5 个 Animator，但每个命中 Renderer 的 Animator 祖先数仍为 1，
最近 Animator 始终是 `OnAwake` 已出现的第一个 Animator。UI 模型中现有 BeyondBoneCloth 始终为 0。
`SetVisible(false)` 和 `UIModelLoader.UnloadModel` 均已记录；同一模型的 `CharUIModel` 与 `PrefabProxy`
两份 owner 记录分别释放，没有再因模型地址相同而被一次误删。

普通场景模型的命中 Renderer 同样各有且仅有一个 Animator 祖先，距离为 3；Typhoea 模型同时已有
11 个游戏原生 BeyondBoneCloth。由此可把三类实例统一为“按 Mesh 命中 Renderer，向上选择唯一最近
Animator，在模型实例上持有 Physics”，但 UI 必须按模型地址去重，并以隐藏和卸载分别驱动停用与销毁；
NPC 则由 `StartNPC` 建立 owner、在上述嵌套释放入口开始注销。

实时日志快照已保存为
`bin/diagnostics/v61-physics-owner-correlation/runtime-live-20260908-024929.log`。正常退出后的最终日志另存为同目录
`runtime.log`：包含 171 条 v61 记录、37 条模型/Renderer 关系、11 次 NPC Start、1 次 NPC Build、
12 条目标 NPC Renderer 关系、6 条 UI owner 释放记录，以及四个目标 NPC 的 16 条成对释放记录。

最终自动生命周期记录为 `physics_runtime.json`：5502 个事件、2751 次调用，全部来自 Unity 线程 20672，
`dropped=0`、`inFlightCalls=0`。调用数为 `AddAnimatorTransform=1537`、
`CompleteMasterJob=591`、`RemoveMonitoringProcess=130`、`MarkAnimatorTransformDirty=104`、
`ClothProcess.Init=101`、`AddTeamAnimatorData=81`、`StartRuntimeBuild=70`、
`Dispose/DisposeInternal=49/49`、`ClearTeamAnimatorData=39`。49 个不同 ClothProcess 都按
`Dispose enter → DisposeInternal enter → RemoveMonitoringProcess enter/return → DisposeInternal return → Dispose return`
同步嵌套。五个 Animator binding 入口均解析到 UnityPlayer 可执行地址，但本次调用数仍为 0。
快照枚举到 143 个游戏现有组件；上述记录仍只描述游戏原对象，不含 EIEM 自建组件。

v61 仍是自动启动的观察型测试构建。生产 Physics Mod 拒绝分支、原生组件创建、SelectionData 转换、
模拟启停、取消与骨骼释放均未连接或改变。本节现在确认的是实例 owner 与 Animator 选择规则，以及游戏
已有的建立/释放边界；它不代表 EIEM Physics 已经在游戏中创建或运行。

### 20.12 v62 原生工厂静态结论与一次性构建探针（已部署，待实机）

退出 v61 后继续复核当前 `GameAssembly.dll` 与元数据，证据保存在
`bin/diagnostics/v62-physics-factory-static/`。当前游戏文件 SHA256 为
`C24495E51B406F03B03890C4788EE618AE022C991405BE5D5B8B787CB775AE89`。本轮取得以下静态结论：

- `BeyondBoneCloth` 的关键入口为 `DisableAutoBuild`、`BuildAndRun`、`set_SerializeData` 与
  `GetSerializeData2`；构造函数会创建 `serializeData`、`serializeData2` 和 `process`。
- `BuildAndRun` 先禁止自动构建，再执行初始化。BoneCloth 在 SelectionData 为空、无效或未标记为
  用户编辑时调用 `GenerateBoneClothSelection`，随后进入运行时构建。
- `GenerateBoneClothSelection` 使用 `boneClothSetupData` 的真实 Transform/父索引生成位置与默认 Move
  属性，再按 `rootBones` 的实例 ID 映射固定点。因此 v1 新骨链无需把作者节点顺序直接写成
  SelectionData；作者 `FIXED` 根写入 `rootBones`，作者 `IGNORE` 节点写入
  `ignoreFromRootBones`，其余节点由原生拓扑生成。
- Typhoea 解包样本的 11 个组件均为 `clothType=1`、`connectionMode=0`。长发样本中 24 个
  `ignoreFromRootBones` 引用与 24 个无效选择点一致，支持上述 IGNORE 映射。v1 目前只接受单一固定根；
  非根 FIXED 不会被悄悄降级成 Move。
- `DisableAutoBuild` 设置 `ClothProcess` 的禁用自动构建状态；`AutoBuild` 会检查该状态。正常运行模式下
  `Start` 执行 Init/AutoBuild，因此动态 AddComponent 可以在同一 Unity 线程调用中先禁用自动构建、
  绑定配置，再主动 `BuildAndRun`。这仍是静态推断，需要本轮探针实机验证调用顺序与结果。

[配置准备](../src/eiem_native_physics_config.h) 已增加并回读 `clothType=1`、
`connectionMode=0`、`rootBones` 和 `ignoreFromRootBones`。MSVC 宿主测试 11 项通过，覆盖列表成员、枚举、
失败事务、节点存活与父子关系。

[一次性原生工厂探针](../src/eiem_native_physics_factory_probe.h) 已加入测试构建。它由真实
`S_actor_typhoea_body_01_lod0` Mesh 命中和已登记模型共同触发，只选择 Renderer 的唯一最近 Animator，
并要求模型已有游戏原生 Cloth，以排除角色 UI 预览。探针建立独立的三节点新 Transform 链和独立组件宿主：

1. 宿主设为 inactive；
2. `AddComponent(BeyondBoneCloth)`；
3. `DisableAutoBuild`；
4. 写入并回读两个配置对象；
5. 激活宿主并调用 `BuildAndRun`；
6. 记录 process、Team、SelectionData、BoneCloth setup、新节点数量及 interlocking Animator。

探针不调用 Dispose、Destroy 或 Animator binding icall，全部新对象保留到进程退出。这样本轮可以先回答
“新增 Transform 是否进入 BoneCloth setup、Team 与 Animator”，而不把尚未验证的返回值当作释放栅栏。
自动关闭路径会在导出原生调用记录前写最后一次探针状态。

当前安装的 `typhoeus/mod.ini` 只有 Mesh、Material、Texture 与三个 Render 段，没有 Skeleton、Physics
资源或 `Render.physics`。所以 v62 实验使用真实 Mesh 命中选择实例，但使用明确标记的诊断三节点链；
它不是作者资源加载验收，也不会修改该 Mod。生产加载器对非空 Physics 动作的整 Mod 拒绝分支保持不变。

本地完整 `build.bat` 已通过。`bin/eiem.dll` 为 6238720 字节，SHA256：
`5B6FD0EB40FDF82F3244A9E7B6061F1F5DEF457B68F8077A8D86C81B4D42FE43`。工厂/owner 静态检查
15 项通过；此处记录的是本地构建和宿主契约，尚无 v62 游戏加载、组件创建、BuildAndRun 返回、Team、
Animator 接纳或运行中销毁证据。

2026-09-08 03:34:28 已在游戏退出状态部署该 DLL，游戏目录副本的大小与 SHA256 均与构建产物一致。
被替换的 v61 DLL 及当时的配置/日志备份到
`E:\EIEM_Workspace\plugin-releases\before-v62-physics-factory-probe-20260908-033428`；其 SHA256 为
`DC6C1BE763C613B981CDFBAE7DDB3F8785DACCD2205512AE6CDA0191006280CE`。部署不等于运行时验证；
需要新游戏进程命中 Typhoea 场景模型并正常退出，才能读取本节列出的组件、配置、Team 与 Animator 证据。

### 20.13 v62 首次实机结果与 v63 NPC 触发修正（v63 已部署，待实机）

v62 的新进程 PID 37984 已正常导出自动诊断。原始日志和快照保存在
`bin/diagnostics/v62-physics-factory-runtime-37984/`。DLL 加载标记正确，记录到 3 个
`S_actor_typhoea_body_01_lod0` Renderer 命中，但没有 `candidate`、`build-return` 或 `poll` 记录，
因此 v62 没有创建组件，也没有调用 BuildAndRun。

本次命中的场景 Typhoea 是 NPC，`NPCAvatar.StartNPC` 已精确关联 model、唯一 Animator 与 Renderer，
但其 `embeddedCloths=0`；另一个命中是角色预览，也没有原生 Cloth。v62 工厂要求
`ownerKind=PrefabProxy && nativeCloths>0`，原意是排除预览，却同时排除了目标 NPC。这是触发条件错误，
不是原生 AddComponent 或 BuildAndRun 失败。该进程的主场景可玩角色是 Wulfa，因此具有 11 个现有 Cloth
的模型并不是 Typhoea Mesh 命中目标。

同一退出快照枚举到 143 个游戏组件，其中 52 个 `IsValid=true`，42 个 `IsRunning=true`；这 42 个都有
正 Team ID、Bone setup 与 interlocking Animator。调用记录包含 52 次 `ClothProcess.Init`、34 次
`StartRuntimeBuild`、42 次 `AddTeamAnimatorData`、811 次 `AddAnimatorTransform`、122 次
`CompleteMasterJob`，无丢失事件。这证明该进程存在运行中的游戏原生裙子/头发等 Cloth 及其 Team、
骨链和 Animator 登记，但不证明 v62 自建链成功。

v63 保留普通 `PrefabProxy` 的 `nativeCloths>0` 判据，并从已经确认的目标
`NPCAvatar.StartNPC` owner 关联单独调用工厂观察入口。只有 Mesh 命中、model/Renderer 归属一致且唯一最近
Animator 成立时才会建立一次诊断链。完整构建通过；聚焦检查 38 项通过，Physics 套件运行 108 项，
106 项通过，2 项因没有提供 Blender/外部样本而跳过，0 项失败。

v63 DLL 为 6238720 字节，SHA256：
`A36547F701BEC97DDA31378FD79C0736BCC5BAECEEA154BF221EDE9CC6F4B977`。2026-09-08 03:47:52 已在没有
游戏进程时部署，安装副本哈希一致。v62 备份在
`E:\EIEM_Workspace\plugin-releases\before-v63-physics-factory-npc-probe-20260908-034752`。生产加载器保护、
`Render.physics` 和 typhoeus Mod 均未改变；v63 的组件创建、配置回读、Team 与 Animator 接纳仍待下一次
实机记录。

### 20.14 v63 配置失败与 v64 构造列表修正（v64 已部署，待实机）

v63 新进程 PID 27012 的日志与自动快照保存在
`bin/diagnostics/v63-physics-factory-runtime-27012/`。它记录到目标 NPC candidate：Renderer、model、唯一最近
Animator 与 `NPCAvatar.StartNPC` owner 均成立，`animatorAncestors=1`、深度为 4。这确认 v63 的 NPC 触发
修正有效。随后探针在创建组件前报告：

`failed stage=prepare error=Cannot bind detached rootBones list`

因此 v63 没有调用 AddComponent 或 BuildAndRun。失败发生在把新建 `List<Transform>` 写入
`ClothSerializeData.rootBones` 后的身份回读。重新核对 `ClothSerializeData::.ctor` 反汇编可见，构造函数已经
分别创建列表并保存到引用字段；对运行时新对象也应沿用这个构造语义，不需要另建列表并替换引用。

v64 删除了根/IGNORE 列表字段替换：它读取构造函数产生的两个列表，确认非空、实际泛型类型正确、彼此
独立且初始为空，再填入 root 和 IGNORE Transform；填入后再次核对字段仍指向相同列表。宿主也改为真实
模拟该构造行为，避免继续用“构造后列表为空指针”的错误模型掩盖运行时差异。

v64 完整构建通过。聚焦测试 39 项通过；Physics 套件运行 109 项，107 项通过，2 项可选 Blender/外部
样本集成测试跳过，0 项失败。DLL 为 6239744 字节，SHA256：
`0CA50D307958924C79389C9BB8563FF8B3CB6B1C12949C521007BC4603014844`。2026-09-08 03:57:18 已在没有游戏
进程时部署，安装副本哈希一致；v63 备份在
`E:\EIEM_Workspace\plugin-releases\before-v64-physics-constructor-lists-20260908-035718`。生产加载器保护、
`Render.physics` 和 typhoeus Mod 仍未改变，v64 的配置通过、组件创建、Team 与 Animator 接纳等待下一
进程实证。

### 20.15 v64 实机边界与 v65 组件自带 Data2（v65 已部署，待实机）

v64 新进程 PID 33724 的日志和退出快照保存在
`bin/diagnostics/v64-physics-factory-runtime-33724/`。目标 NPC、唯一最近 Animator、配置 `Prepare` 和
`BeyondBoneCloth` 的 `AddComponent` 均已通过；组件地址为 `0x0000000FAE24B000`，其构造产生的
`ClothProcess` 地址为 `0x000000100367EA80`。失败点是 `config-readback`，因此 v64 没有调用
`BuildAndRun`。16 次后续轮询中组件保持存活，但 `isBuild=false`、`valid=false`、`running=false`、
`team=0`、Animator 为空且没有 Selection/Setup。这些结果只证明配置准备和组件创建成立。

静态反汇编再次确认：`set_SerializeData` 写入组件偏移 `0x98` 并执行写屏障，`get_SerializeData`
读取该偏移，`GetSerializeData2` 则读取偏移 `0xa0`。v64 同时用正式 setter 写入 Data，并用通用字段
写入替换 Data2；结合失败位置，Data2 替换没有形成 getter 返回身份是当前推断，尚不能作为运行时事实。
原生组件构造函数本来就创建自己的 `ClothSerializeData2`，而 v1 的 SelectionData 应由游戏根据骨链
生成，因此 v65 不再替换它：作者参数仍通过 `set_SerializeData` 注入；探针读取、校验、持有并轮询
组件构造产生的 Data2。失败日志会分别记录预期/实际 Data、detached Data2 和 component Data2 地址。

v65 聚焦测试 40 项全部通过；Physics 套件运行 110 项，108 项通过、2 项可选 Blender/外部样本
集成测试跳过、0 项失败；完整 DLL 构建通过。DLL 为 6240256 字节，SHA256：
`43582FEDF97B0D1BA29EA116ADE92254823BEE3059A95D45C9152CF0E54873FB`。2026-09-08 04:09:38 在游戏
进程退出后部署，安装副本哈希一致；v64 备份位于
`E:\EIEM_Workspace\plugin-releases\before-v65-physics-component-data2-20260908-040937`，静态验证记录位于
`bin/diagnostics/v65-physics-component-data2/validation.json`。生产加载器 guard、`Render.physics` 和
typhoeus Mod 均未改变；组件 Data2 身份、`BuildAndRun` 返回、Team/Selection/Animator 接纳仍等待
下一进程的实机日志。

### 20.16 v65 实机建立最小 BoneCloth Team（PID 15584，已归档）

v65 进程 PID 15584 的实时证据保存在
`bin/diagnostics/v65-physics-factory-runtime-15584/`。目标 Renderer 在 `NPCAvatar.StartNPC` 下命中，
模型只有一个祖先 Animator；原模型没有内置 Cloth。探针通过正式 setter 绑定
`SerializeData=0x000000100B53E6C0`，组件构造的
`Data2=0x000000100BBD9EA0` 与配置草稿中未使用的 detached Data2 地址不同，配置回读通过。

`BuildAndRun` 返回 1。紧随返回的第一次轮询记录构建仍在进行：`isBuild=1`、`valid=1`、
`running=0`、`team=0`；游戏已经生成 3 点 Selection（`userEdit=1`）、3 个 skin bone 和 4 个 setup
Transform，其中 3 个就是探针新增节点。下一次 NPC 事件时构建已经完成：`isBuild=0`、`valid=1`、
`running=1`、`team=39`，`interlockingAnimator` 等于预期最近 Animator。此后 14 次 settled 轮询保持
相同状态。

这证明了当前游戏版本中最小新增 BoneCloth 的创建链：在目标模型/Animator 下建立连续 Transform，
构造并填写 `ClothSerializeData`，把组件自身的 `ClothSerializeData2` 留给游戏生成 Selection，随后激活
宿主并调用 `BuildAndRun`；异步构建完成后组件进入有效运行 Team，并接入目标 Animator。它尚未证明
骨骼 Transform 实际逐帧位移、Mesh 蒙皮产生可见变形、碰撞体实例化或生产生命周期注销。

### 20.17 v65 专属 trace 序列与 v66 合并验证（v66 已部署，待实机）

PID 15584 退出后自动导出的 2230 个事件全部成对，`dropped=0`、`inFlightCalls=0`。其中共有
1115 次调用：`Init=54`、`RemoveMonitoringProcess=43`、`StartRuntimeBuild=35`、
`CompleteMasterJob=124`、`AddTeamAnimatorData=43`、`AddAnimatorTransform=815`，以及全进程唯一一次
`BeyondBoneCloth.BuildAndRun`。该唯一调用的对象就是 EIEM 组件 `0x000000100B1D3960`。

按组件、process `0x000000100BBDAA80` 和 team 39 过滤后，专属序列为：

1. `BuildAndRun enter → Init → StartRuntimeBuild(true) → BuildAndRun(true)`；
2. 31 ms 后二次 `Init → RemoveMonitoringProcess`；
3. 再过 78 ms，`CompleteMasterJob → AddTeamAnimatorData(team 39)`；
4. 同一 tick 为 team 39 调用四次 `AddAnimatorTransform`，与 setup 中三个骨链节点加组件宿主共四个
   Transform 一致；
5. 关闭快照仍记录组件存活、有效、运行中，team 39、构建成功且无销毁标记，骨链引用和长度均未变化。

游戏窗口关闭时探针先停止并导出 trace，随后才记录目标 NPC 的 `OnRelease/ReleaseAvatar`，所以这次
快照没有捕获目标 process 的 Dispose；不能据此判断释放行为。完整原始快照、过滤事件和结构化结论位于
`bin/diagnostics/v65-physics-factory-runtime-15584/physics_runtime.json` 与 `trace-summary.json`。

为了用下一次进程同时回答运动与释放问题，v66 增加两项自动诊断：窗口主线程每 500 ms 读取三个新增
节点的局部/世界位置和旋转，最多 32 次，并与 `BuildAndRun` 前基线比较；精确目标 NPC 的
`NPCCrowdEntityComponent.OnRelease` 和 `NPCAvatarManager.ReleaseAvatar` 均在原函数进入、返回两侧轮询
组件、process、Team 和姿态。它没有新增 Hook，也没有接入 Dump UI。

v66 聚焦测试 42 项全部通过；Physics 套件运行 112 项，110 项通过、2 项可选 Blender/外部样本测试
跳过、0 项失败；完整 DLL 构建通过。DLL 为 6243840 字节，SHA256：
`2D36DB8CCF6659C1AEA940CA211739EBF296C82FE02B29C3B10020A1B820E62E`。2026-09-08 04:30:46 在游戏
退出后部署且安装哈希一致；v65 备份位于
`E:\EIEM_Workspace\plugin-releases\before-v66-physics-motion-release-20260908-043046`。生产加载器 guard、
`Render.physics` 和 typhoeus Mod 仍未改变。

### 20.18 v66 实机确认新增节点发生局部运动（PID 32784）

v66 再次完成相同的最小工厂链并进入 team 39。`BuildAndRun` 返回时三个节点相对构建前基线均未变化；
异步构建完成后的连续采样中，三个节点均可读取，固定根保持不变，两个 MOVE 节点同时出现局部位置和
世界位置变化：最大局部位置差平方为 `6.29432179e-07`，最大世界位置差平方为
`1.2407545e-06`，旋转差为 0。因为判据包含相对父节点的局部位置，结论不依赖角色整体世界位移。

这证明游戏运行中的 BoneCloth Team 已经对两个新增可动物理节点产生 Transform 写回。当前三点竖直链在
静止后收敛到相同位置，因此这些采样还没有描述连续摆动轨迹，也没有把节点装入可见 Mesh 的骨骼调色板。
实时证据位于 `bin/diagnostics/v66-physics-motion-release-runtime-32784/`。

### 20.19 v66 场景卸载触发原生释放（PID 32784）

切换场景时，目标模型 `0000000FCD7D4520` 的 NPC owner 进入并返回 `ReleaseAvatar/OnRelease`。完整 trace
显示目标工厂组件 `0000001022A34E10` 对应的 process `0000001022EB1A80` 在创建后运行约 166 秒，并于
tick 65441546 发生以下嵌套调用：

1. `ClothProcess.Dispose` enter；
2. `ClothProcess.DisposeInternal` enter；
3. `TeamManager.ClearTeamAnimatorData` enter/return，返回 team 39；
4. `TeamManager.RemoveMonitoringProcess` enter/return，参数为目标 process；
5. `DisposeInternal` return；
6. `Dispose` return。

释放之后 trace 又记录 6388 个事件，目标 component/process 地址没有再次出现；退出时枚举的 401 个
BoneCloth 组件中也没有目标 component 或 process。因此可以确认，把新增 BoneCloth 组件及新增 Transform
挂在命中模型下面时，模型的自然场景卸载会进入游戏自身的组件释放链，并注销目标 Team 的 Animator 数据和
监控 process。

team 数字不能作为长期实例标识。目标 team 39 清除后仅 4719 ms，另一个 process
`0000001023046A80` 即获得 team 39；退出快照中的 team 39 属于 `MC_Hair` 组件
`0000001022A34960`。后续实例表必须使用 component/process 地址并维护代次，不能跨释放期仅凭 team 编号关联。

这次结果证明的是自然模型卸载下的组件注销，不把 `DisposeInternal` 或 `Dispose` 的返回值提升为任意异步
任务、Job 或 Animator 写回均已静止的通用栅栏。生产实现应让新增节点跟随模型层级销毁，并在组件/模型销毁
边界移除自己的实例记录，不应在观察到单次 `Dispose` 返回后独立销毁仍可能被原生组件引用的节点。结构化
证据见 `trace-summary.json`；该次 trace 为 8772 个事件、0 dropped、导出时 0 个 hook 调用仍在栈内。

至此，最小 v1 BoneCloth 的组件创建、Data 配置、异步 Team 构建、Animator 接纳、物理局部位置写回，以及
自然场景卸载下的原生注销均已有同一条实机链路证据。尚未验证的是新增节点进入替换 Mesh 的骨骼调色板后能否
产生可见变形、作者参数逐项对运动的影响、碰撞体构造，以及 `Render.physics` 的生产实例化与多实例状态管理。

### 20.20 v67 接入 `Render.physics` 生产实例适配器（已构建部署，待实机）

v67 不再执行 v62-v66 的 Typhoea 硬编码一次性工厂。旧工厂头文件保留为历史实验源码，但已从 DLL include
链移除。生产加载器现在发布包含有效 Physics 动作的 Mod；Physics 意图仍由普通 Render 第一命中规则产生，
并新增保存实际命中的 Renderer。适配器按“模型实例 + Mod + 不可变 Physics 资源快照”去重，因此相同 Mesh
身份的主角、NPC 和角色 UI 是不同实例，同一模型的多个 LOD/Renderer 命中不会重复创建同一份 Physics。

当前 v1 无碰撞体执行路径如下：

1. 在命中模型内验证 Renderer 仍存活，并要求它只有一个最近 Animator 祖先；
2. 通过 Physics 引用的 Skeleton 文档解析该模型自己的 Transform。若 Render 的 Mesh 同时使用相同 Skeleton
   资源，通用 Skeleton 实例键会复用同一批新增节点；
3. 先准备构造函数自带的 root/IGNORE 列表和五个标量，再创建模型子级的非激活
   `EIEM_Physics_<generation>` 宿主；
4. 对每组添加一个 `BeyondBoneCloth`，执行 `DisableAutoBuild`、写入/读回 SerializeData、保留组件自己的
   Data2，激活宿主后调用 `BuildAndRun`；
5. 异步轮询要求 process 同时满足 valid、running、有效 team，且 `interlockingAnimator` 为步骤 1 的
   Animator，之后才记录 ready；
6. 规则移除、资源换代或模型 owner 释放时只销毁 EIEM 自己的宿主。若回调不在 Unity 线程，销毁请求留到
   下一次窗口主线程轮询；实例继续保留配置和 Skeleton，直到宿主及全部组件的 Unity native 状态均为 0。
   该实现不调用 `DisposeInternal`，也不把它的返回当作完成栅栏。

为区分“Physics 建立了”与“命中了 Mesh 实际使用的骨骼”，v67 在构建前记录 Renderer palette 槽数、组节点数
和指针相等的 `paletteHits`。`typhoeus` 实机夹具使用 body Mesh 原有的左食指
`Bip001_L_Finger0 → Finger01 → Finger02`，三者在导出 Mesh 的 76 槽骨骼表内；Physics 参数与 v66 已运行
配置一致，为 gravity 10、stabilization 0.1、falloff 0、blend 1、animation pose 1，无碰撞体。夹具生成器位于
`tools/diagnostics/make_v67_typhoea_physics_fixture.py`，生成 781 字节 Physics 和 1774 字节 source-only
Skeleton；生产 C++ 读取器已读取该准确输出。

相关 Physics/Skeleton/Skin 套件共运行 118 项，全部通过，其中 4 项因当前命令行环境没有 Blender 而跳过；
完整 `build.bat` 通过。DLL SHA256 为
`D4237678B72483E532DC33CA81379A9DBAA123488CC8225118A61AB97FA2D715`（6251008 字节），已在游戏退出后安装，并给
`typhoeus` 的 body Render 增加 `physics=PhysicsTyphoeaLeftIndexV67`。部署前备份位于
`E:\EIEM_Workspace\plugin-releases\before-v67-physics-resource-adapter-20260908-052503`。

本节尚无 v67 游戏进程结果。待验证项是：Mod 解析、每种实际模型消费者的 build/ready、
`selected=3/paletteHits=3`、左食指 Transform/可见蒙皮响应，以及规则撤销和场景卸载后的 retire/原生注销。
碰撞体和 Physics v2 仍由配置准备器明确拒绝，不属于本次部署。

### 20.21 v67 `Render.physics` 实机结果与 v68 精确作者节点边界

v67 进程 PID 4292 首次走通生产资源链。加载器解析了包含 Physics 动作的 `typhoeus` Mod；同一条 body Mesh
规则先后为两个 `chr_0034_typhoea_uimodel(Clone)` 和一个
`chr_0034_typhoea_postmodel(Clone)#160` 建立独立实例。前两者由 `CharUIModelMono.OnAwake` 命中，第三者先由
`PrefabInstantiateProxy.OnCompleted` 命中，随后也在 `BaseModelViewPart.OnLoadFinish` 下被识别；最后一个实例因此
只能称为 `PrefabProxy + BaseModelPart` 消费者，本轮记录不足以把它进一步认定为主角或 NPC。

三次构建均记录 `palette=76 selected=3 paletteHits=3`，`BuildAndRun` 和内部 `StartRuntimeBuild` 均返回 true，
并分别进入 team 43、43、49；三个 process 的 `interlockingAnimator` 都等于命中 Renderer 的唯一最近 Animator。
每个 team 注册了五个 Animator Transform。第三个实例在自动 trace 导出时仍为 valid/running，原生
`boneClothSetupData` 明确给出四个 skin bones：作者声明的
`Bip001_L_Finger0 → Finger01 → Finger02`，以及实际模型中未出现在作者文件内的子节点
`Bip001_L_Finger0Nub`；第五个 Transform 是 `EIEM_Physics_3` 宿主。

这份证据修正了 v67 的一个映射假设。v1 作者约定明确说未选择的子骨骼不进入链，但只把作者 FIXED 节点写入
`rootBones` 会让原生 BoneCloth 递归展开该节点下的完整实际层级，因此 `paletteHits=3/3` 只能证明三个作者节点
都属于 Mesh palette，不能证明原生 Selection 只含这三个节点。v68 在准备配置时遍历每个非 IGNORE 作者节点
的直接子节点；凡不在同组作者节点集合内的子节点，均作为边界根追加到 `ignoreFromRootBones`。显式 IGNORE
节点仍按原语义写入，且它自身已排除整个子树，不重复枚举子级。对本轮食指夹具，预期自动边界 IGNORE 为
`Finger0Nub` 一项；这个预期尚需下一游戏进程读取 Selection 属性验证。

两个 UI 实例都在 `UIModelLoader.UnloadModel` 触发适配器销毁自有宿主。对应 process 随后分别执行完整的
`Dispose → DisposeInternal → ClearTeamAnimatorData(team 43) → RemoveMonitoringProcess → return`，且在各自
Dispose 返回之后直到 trace 结束均没有再次引用该 process；组件也不再以原身份出现在最终枚举中。第二个
EIEM 组件的裸地址后来被 `MBC_Lizhiyan_Ear_Upper_01` 复用，其 process 和 team 已变为另一组值，这再次实证
team 编号和裸对象地址都不能跨退休期作为实例身份。

窗口正常关闭时，自动 trace 先导出，之后第三个实例才在 `PrefabInstantiateProxy.Unload` 记录 retire 请求，
所以现有 trace 可以证明它在导出点仍运行，不能证明该次关闭期间的最终原生 Dispose；这不是崩溃记录。
完整运行日志、12934 个零丢失/零在栈调用事件的 trace 和结构化结论保存在
`bin/diagnostics/v67-physics-resource-runtime-4292/`。v68 另外让自动 trace 读取 managed
`SelectionData.attributes`，按原生 `VertexAttribute.IsFixed/IsMove/IsInvalid` 方法统计，并仅为 EIEM 组件输出
逐点状态；该功能仍不在 Dump UI 或生产交互入口内。

v68 的 Native Physics 宿主套件 75 项全部通过；Physics、Skeleton、Skin 合并套件共运行 118 项，114 项通过、
4 项因当前命令行环境没有 Blender/外部样本而跳过，0 项失败；完整 `build.bat` 通过。游戏退出状态下已部署
6257664 字节 DLL，构建与安装 SHA256 均为
`E92837AA6788F951E16E54EA452F6C4A1664004EFC5C642F68BCAE1A27AA5C28`；v67 备份位于
`E:\EIEM_Workspace\plugin-releases\before-v68-physics-selection-boundary-20260908-133817`。以上是 v68 部署时的
待验证状态；实际运行结果和 Selection 诊断错误见下一节，不能继续引用本段原预期作为实机结果。

### 20.22 v68 精确拓扑实证、Selection 诊断纠错与 v69

v68 进程 PID 22888 命中了一个 `PrefabInstantiateProxy.OnCompleted` / `BaseModelPart` 消费者和一个
`CharUIModelMono.OnAwake` 消费者。两者均记录 `palette=76 selected=3 paletteHits=3 boundaryIgnores=1`，
分别进入 team 17 和 team 48，且 process 的 Animator 与目标 Renderer 的最近 Animator 一致。
最终自动快照中的生产组件 `EIEM_Physics_1` 仍为 valid/running；其原生 `boneClothSetupData` 只有三个
skin bones：`Bip001_L_Finger0 → Finger01 → Finger02`，组件宿主位于第四个 Transform 槽。
v67 多选的 `Finger0Nub` 已不再出现。这直接验证了边界 IGNORE 能把原生递归拓扑收敛到作者节点集合。

角色 UI 实例在 `UIModelLoader.UnloadModel` 后完整记录
`ClothProcess.Dispose → DisposeInternal → ClearTeamAnimatorData(team 48) → RemoveMonitoringProcess`，
随后适配器记录 `retired`。主模型实例是在自动快照导出后才收到 `PrefabInstantiateProxy.Unload`，日志只记录
retire 请求，没有在该快照中记录最终 `retired`，因此不把这次主模型关闭写成完整注销实证。

v68 快照最初把生产组件的三个 Selection 点都统计为 invalid。交叉检查发现同一诊断也把其余 148 个游戏原生
组件的全部点都统计为 invalid，问题来自诊断器本身：`System.Array.GetValue` 返回装箱的值类型
`VertexAttribute`，v68 却直接把装箱对象传给实例谓词；对象头被错误当作结构体 `this`。同文件读取
`ResultCode` 时已经使用了正确的 pinned box → unboxed value 流程。宿主回归现已改为模拟真实对象头，旧代码
稳定失败；v69 固定每个装箱值并在 unboxed 地址上调用 `IsFixed/IsMove/IsInvalid`，该回归恢复通过。
所以 v68 的精确骨骼拓扑结果有效，其 Selection 属性统计无效，不能据此判断参数没有进入求解器。

v68 完整日志、自动快照和结构化结论保存在
`bin/diagnostics/v68-physics-selection-runtime-22888/`。v69 的原生物理测试 75 项、owner 契约 10 项均通过，
完整 `build.bat` 通过；游戏退出状态下已部署 6257664 字节 DLL，构建与安装 SHA256 均为
`C39752928198493E10AE0F3DDFD99A28FC677A01A472CC6A830C548A6FC7B2FA`。v68 安装备份位于
`E:\EIEM_Workspace\plugin-releases\before-v69-physics-selection-value-unbox-20260908-140521`。
v69 随后在 PID 10760 运行。一个 `BaseModelPart` 和一个角色 UI 实例均再次以
`palette=76 selected=3 paletteHits=3 boundaryIgnores=1` 进入运行 Team，分别为 19 和 52。自动快照中的
`EIEM_Physics_1` 为 valid/running，原生 setup 仍精确包含三个作者节点；Selection 可信读回为
`count=3 fixed=1 move=2 invalid=0 other=0 unreadable=0`，逐点顺序为 `fixed / move / move`。
边界外的 Nub 被从 setup 和 Selection 一起排除，而不是保留为一个 invalid 点。

作为诊断交叉核对，游戏原生 `MBC_Typhoea_Hair_Back_Ponytail_Long` 同时读回
`count=38 fixed=2 move=12 invalid=24`，与第 8 节的离线样本完全一致；这证明 v69 的值类型读取修正有效，
也证明 v1 的 FIXED/MOVE 作者角色已通过 `rootBones + ignoreFromRootBones` 进入原生 Selection 生成过程。
角色 UI 的 team 52 完整执行 Dispose、ClearTeamAnimatorData 和 RemoveMonitoringProcess 后被适配器移除。
主模型仍在自动快照导出后才记录 retire 请求，故最终 Dispose 继续只采用此前 UI/v66 的实证范围。
完整 v69 日志、快照和结构化结论保存在 `bin/diagnostics/v69-physics-selection-runtime-10760/`。

当前完成度应分层表述：原生后端、最小 BoneCloth、Team/Animator、Transform 写回、精确作者拓扑、
FIXED/MOVE Selection 生成和一个
角色 UI 的完整退役机制已经实证，足以继续做产品实现；在 v70 可见结果出现前，完整功能仍缺少新增 Skeleton 节点驱动替换 Mesh 的
可见结果、作者标量参数的受控 A/B 响应、碰撞体转换与响应、Physics v2 源图实例化，以及明确标识为 NPC 的
生产实例。不能把 `BaseModelPart` 自动称为 NPC。

### 20.23 v70 新增骨骼可见蒙皮夹具（角色 UI 已实机验证）

下一项实验只验证一个问题：Skeleton 新建的 Transform 能否同时进入 BoneCloth 和替换 Mesh 的骨骼数组，进而
让实际顶点随物理写回变形。它没有修改 DLL，也没有向 Dump 或主功能 UI 增加入口；仍由 v69 的自动运行记录
观察结果。

`tools/diagnostics/make_v70_typhoea_new_bone_fixture.py` 以当前 Typhoea Mod 为输入，在独立目录复制并联动改写
Mesh、Skeleton、Physics 与 INI。原 body Mesh 有 14664 个顶点和 76 个骨骼槽，槽 48 是
`Bip001_L_Finger02`，共有 202 个正权重影响。夹具在该骨骼下追加
`EIEM_PhysicsTip`（局部位置 `(-0.015, 0, 0)`），将这 202 个影响保持原权重改指向新槽 76，并按
`inverse(newLocal) @ Finger02Bindpose` 生成新 bindpose。这样静止姿态不因改绑本身偏移，后续画面变化才可归因
于新增 Transform 的姿态。

对应 Skeleton v2 含 80 个现有源节点和一个 `source=false` 新节点，覆盖 Mesh 的全部原骨骼路径及必要祖先；
Physics v1 链为 `Finger0 FIXED → Finger01 MOVE → Finger02 MOVE → EIEM_PhysicsTip MOVE`。同一 Render 同时声明
`mesh=`、`skeleton=` 与 `physics=`，Physics 内的相对 Skeleton 路径和 INI Skeleton 资源最终指向同一个文件。

离线结果为 palette `76 → 77`、remapped vertices/influences `202/202`、新 bindpose 最大重编码误差
`1.4305114759416426e-08`，其余 Mesh 字段逐项一致。新增生成器回归 3 项通过；现有生产 C++ Physics/Skeleton/
Mod 读取套件 14 项通过，实际生成的 v70 `mod.ini` 也由同一 C++ 读取器以 exit 0 接受。夹具和机器可读记录位于
`bin/diagnostics/v70-new-bone-visible-fixture/`。

游戏退出后已把原 Mod 备份到
`E:\EIEM_Workspace\plugin-releases\before-v70-visible-new-bone-20260908-142630`，随后部署夹具；7 个源文件与
安装文件逐一核对无哈希差异，安装 Mesh SHA256 为
`E40B2C0B4A025B81DA08F8AB1BB78CDF0A65599DF329893F4E7559CD86868E4B`。

PID 7100 随后加载 v69 DLL 和 v70 资源。主模型 generation 1 与角色 UI generation 2 均记录
`Skeleton nodes=81 added=1`、`palette=77 selected=4 paletteHits=4 boundaryIgnores=1`，分别进入 team 36 和
team 52。自动快照中 generation 1 的原生 setup 精确含 `Finger0 → Finger01 → Finger02 → EIEM_Bone_1` 四个
skin bones，Selection 为 `fixed/move/move/move`，没有 invalid/unreadable 点。第五个 Transform 是组件宿主。
角色 UI 的 team 52 在 `UIModelLoader.UnloadModel` 完整执行
`Dispose → ClearTeamAnimatorData → RemoveMonitoringProcess` 并记录 `retired`；主模型只在快照导出后记录
retire 请求，故本次仍不把其最终退役写成已观察事实。

用户在角色 UI 页面明确观察到左手拇指动作被改变。作者链的游戏名称是 `Bip001_L_Finger0/...`；结合上述
新增节点、四个 palette 命中、原生写回和改绑的 202 个顶点，这一结果验证了完整可见通路：
**Skeleton 新建 Transform → BoneCloth/Animator 写回 → 替换 Mesh 蒙皮变形**。这不是仅凭日志推断的画面结论。

同一运行还精确命中两个 Typhoea NPC 模型，各有三个 Renderer 完成 Mesh/Skeleton 应用，且
`StartNPC target=1`、唯一 Animator 和释放边界均成立；但没有任何以 NPC owner 为 stage 的 `PHYSICS-PLAN`、
build 或 ready。因而本次 NPC 的不确定不是视觉观察不足：加载中的 v69 确实没有为 NPC 创建附加 Physics。
完整日志、快照和结构化结论位于 `bin/diagnostics/v70-new-bone-visible-runtime-7100/`。

### 20.24 v70 将精确 NPC owner 接入共享 Physics 执行器（NPC 已实机验证）

20.23 的 NPC 缺口来自两个已存在但未连接的入口：`RendererInfo._Init` 已对具体 Renderer 执行 Mesh/Skeleton，
而模型级 Physics 只能在模型根和 Animator 都稳定后建立。v70 在游戏原 `NPCAvatar.StartNPC` 返回后读取其已确认的
model/component 关系，并以 `EiemModelOwnerKind::NpcAvatar` 调用现有
`EiemRegisterAndApplyModelInstance`。这里仍由普通 Render 的 Mesh 身份规则决定是否命中；没有 NPC 专用资源格式、
角色名判断或第二套原生组件工厂。

对应释放在游戏原 `NPCAvatarManager.ReleaseAvatar` 与 `NPCCrowdEntityComponent.OnRelease` 调用前，均以同一
component owner 调用 `EiemForgetModelOwner`。嵌套重复通知由已有 owner 表幂等处理，物理组件和 Skeleton 节点仍由
现有模型实例退役路径管理。本改动没有把 `DisposeInternal` 返回、计数或 trace 谓词当作异步完成栅栏。
适配器现位于 `src/eiem_npc_model_owner.h`；自动诊断继续随启动运行并写独立目录，没有进入 Dump 或主功能 UI。

NPC owner、资源、Skeleton、Skin、生命周期和原生探针定向套件共 79 项通过，0 项失败；完整 `build.bat` 通过。
测试夹具同时修正了 MSVC 中文输出的解码方式，只影响测试进程捕获错误文本。游戏退出后，v69 DLL 已备份到
`E:\EIEM_Workspace\plugin-releases\before-v70-physics-npc-owner-20260908-151543`，并部署 6257664 字节 v70 DLL；
构建与安装 SHA256 均为 `439251AFF50BFB44541A348F7F8381AF797575410F1128B4821845279F3B41CD`。
PID 5048 随后加载上述 v70 DLL。两个精确 Typhoea NPC 分别在 `NPCAvatar.StartNPC` 以 generation 2/3 建立
Physics：每个模型都有三个命中 Renderer、唯一 owner/model/Animator 关系，binding 均为
`palette=77 selected=4 paletteHits=4 boundaryIgnores=1`。generation 2 明确记录 ready team 48；generation 3
在快照中为 valid/running team 49。两个 team 均注册五个 Animator Transform，自动快照中的 setup 都是四个
skin bones 加一个组件宿主，Selection 都是 `fixed/move/move/move`。用户同时确认 NPC 画面的左手拇指物理已经
生效，因此 NPC 生产创建与可见蒙皮路径完成实机验证。

两个 NPC 随后都在 `NPCCrowdEntityComponent.OnRelease` 注销 owner 并记录 retire 请求。不过关闭时的自动快照
仍枚举到这两个 valid/running 组件，日志没有出现对应 `retired`；这只能证明 owner/retire 入口已触发，不能把
本次进程关闭当作延迟 Destroy 和原生任务最终完成的证据。该限制不推翻已经观察到的 NPC 创建、Team 接纳和
可见效果。完整日志、快照和结构化结论位于 `bin/diagnostics/v70-physics-npc-runtime-5048/`。

至此，“能否按 Mesh 身份在主模型、角色 UI 和 NPC 上新增骨骼并用游戏原生 BoneCloth 驱动替换 Mesh”的机制
实验已经完成。下一项独立实验应验证五个作者标量是否实际改变求解结果；优先以同一资源的
`blendWeight=1 ↔ 0` 做热重载 A/B，保持 Mesh、Skeleton、节点选择和其余参数不变。通过后再接入球体碰撞体，
避免把参数映射与碰撞转换同时引入一个实验。

### 20.25 v71 `blendWeight` 热重载双向 A/B 与 NPC 最终退役

PID 35576 先以原资源 `blendWeight=1` 建立主模型 generation 1/team 17 和 NPC generation 2/team 48。
实验从同一 Physics v1 文档生成两个 994 字节变体，只改变一个 float：A 为 `blendWeight=1`，B 为
`blendWeight=0`；document identity、Skeleton、组、节点、碰撞体列表、gravity、stabilization、falloff 和
animation pose ratio 均保持一致。A/B SHA256 分别为
`B7B4A70F5995A8AA6B32275A1C83BBB7BDB00615B3FCE16B6C1311C4E0E654BA` 与
`9ECA7935FC14F523483C028E0D4C7B39D5CF45B1AF283E81654634B674DEF0FF`。

第一次 F10 读取 B 后，旧 generation 1/2/3 全部记录 `retired`，再建立零权重的主模型 generation 4/team 54、
NPC generation 5/team 55 和角色 UI generation 6/team 56。用户观察到左手拇指恢复原动画。NPC generation 5
自然卸载时依次记录 retire、最终 `retired` 和 Skeleton 自有节点退休；UI generation 6 也完整退休。这补齐了
20.24 关闭快照未覆盖的 NPC 延迟销毁结果。

随后磁盘资源恢复 A，第二次 F10 让活跃 generation 4/7 完整退休并建立 generation 8/9；用户观察到物理偏移
按预期重新出现。由此确认 `blendWeight` 从 Physics v1 作者文件经过 F10 资源快照、配置构造、原生重建直至
最终 Animator/Mesh 输出的双向行为有效。该实验没有证明其余四个标量的完整单位或曲线语义。
当前游戏目录已保持 A，即 `blendWeight=1`。变体、日志摘录和结构化结论位于
`bin/diagnostics/v71-physics-parameter-ab/`。

原计划的球形碰撞体实验已按用户安排暂停；后续先完善真实解包物理数据在 Blender 中的导入、配置和增量导出，
待有实际物理骨骼、Mesh、碰撞体和参数组合后再回到 DLL 碰撞验证。

### 20.26 Blender 0.13.0 原生配置来源与 v1 增量组合导出

本轮没有修改或部署 DLL。解包器现有 `components.json`、逐组件原始字节、TypeTree schema、解码数据、完整
Transform 表和 PPtr 引用图继续作为 v2 来源；Blender 不再把 SelectionData 点序号猜成骨骼顺序。每个原生
BoneCloth 的物理 Transform 由真实 `rootBones` 后代减去 `ignoreFromRootBones` 分支得到，选择点及其属性仍保持
原数组配对，二者在面板分别计数。

原生碰撞体按源类型恢复可视化：球使用 center/radius，胶囊使用 direction、reverseDirection、
alignedOnCenter、radiusSeparation、两端半径和 length，平面显示局部 +Y 法线并标明无限语义。几何解释依据
MagicaCloth 公共 API 合同，仍不把它写成终末地游戏中的碰撞响应实证。Typhoea 样本的 27 个组件均保留：
25 胶囊、1 球、1 平面；19 个位于骨盆、脊柱、胸、颈、头、手臂和大腿等身体层级，8 个位于附件层级。
11 个物理组引用其中 25 个，Neck 与 Spine1 胶囊未引用，同一碰撞体最多被 4 组共享。Blender 面板直接显示
绑定骨骼、引用次数和组名，因此躯干碰撞体与附件碰撞体都能从原配置识别。

参数工作流分两层。原生 v2 组之间可复制全部匹配的可编辑数值和曲线字段，但不复制组件身份、根骨、
SelectionData 或碰撞引用；新增 v1 组可从当前原生模板复制五个已经映射的标量。新增骨骼使用“复制所选骨链为
新增物理链”：复制连续树的静止层级并把副本根接回原外部父级，所有副本标记为 Skeleton `source=false`；
网格权重不自动复制，避免源骨与副本同时影响同一顶点，由作者明确转移需要物理驱动的权重。

Mesh Mod 导出保持选择式增量语义。只选 Mesh 时仍为 Mesh-only；同时选中新增 v1 无碰撞体组时，导出器按
共享 Rig 汇总一份 Physics，自动写入同一 Skeleton，并给所有使用该 Rig 的所选 Render 写入相同 `physics=`。
由此主模型、NPC 和角色 UI 继续共享 DLL 现有的 Mesh 身份命中规则，PFB 不成为物理范围条件。原生 v2、带
碰撞体的 v1、缺失同 Rig 可见 Mesh 的物理组都在修改目标目录前拒绝，保留独立作者导出入口。

实现位于 `tools/Blender/eiem_physics_source.py`、`eiem_physics_native.py`、
`eiem_physics_authoring.py` 和 `eiem_blender_addon.py`。Blender 5.0.1 与 MSVC 环境运行 33 项针对性测试全部通过，
覆盖真实 Typhoea 源图、三种碰撞体、根/忽略闭包、完整参数模板、复制新增骨链、保存重开、v1 组合包、
生产 C++ Physics/Mod 读取器及 v2/碰撞体拒绝。本结果验证作者数据与当前 v1 无碰撞体输出，不验证 v2 或碰撞体
原生实例化。

源码版本已升至 0.13.0，七个运行文件逐一哈希同步到 `E:\vscode\EIEM_Blender`。安装包为
`bin/EIEM_Blender-0.13.0-native-physics-authoring.zip`，大小 70941 字节，SHA256：
`387B7CBD57FD2FB2056714C1117DC908BBF928A80AC6248630109902206533E0`。本轮未向游戏插件目录写文件。

### 20.27 正常解包包内嵌原生物理源图并与共享 Rig 合并

AnimeStudio 的正常 `Export Prefab as EIEM mod package` 已接入物理源写入器。所选 Prefab 存在
`BeyondBoneCloth`、球体、胶囊或平面组件时，输出目录增加 `physics/components.json` 及逐组件
`.bin`、`.schema.json`、`.data.json`；它们保留 VFS 指纹、CAB/PathID 身份、引用闭包、原始字节、
TypeTree、解码字段和 Prefab Transform 图。`mod.ini` 不引用该目录，因此解包得到的源图不会自行成为
运行时 Physics 动作。重复导出会先清理该包自己的 `physics` 子目录，避免旧 Prefab 数据残留。

真实 Typhoea 正常包导出成功，`components.json` 为 646496 字节，包含 38 个组件和 556 个源
Transform：11 个 BoneCloth、25 个胶囊、1 个球体、1 个平面。Blender 整包导入实测得到 62 个 Mesh、
1 个共享 Rig、11 个原生物理组和 27 个碰撞体。渲染 Skeleton 原本省略了 48 个物理链末端节点以及
组件/碰撞体 owner Transform；导入器现在把组件 owner、显式 Transform 引用、按
`rootBones - ignoreFromRootBones` 展开的物理链及其祖先合并进共享 Rig，共 192 个物理相关 Transform。
`GrounderIK`、`Mesh_all`、VFX 等无关 Prefab 节点不进入 Rig。合并后仍校验源路径、直接父级和局部 TRS。

AnimeStudio GUI 的 .NET 9 Release 构建为 0 错误，并发布到 `tools/AnimeStudio/dist/win-x64-vfs-next`；
发布版 `AnimeStudio.GUI.exe` 为 168960 字节，SHA256
`9B6ACC8139CDB3EF8B9DD15E11FC08DE57330CA58EA64DCF376CFA933670F004`。Blender 5.0.1 的真实源图往返测试
和正常整包导入测试均通过。以上验证覆盖正常解包、原生参数/碰撞体保留及 Blender 场景建立；本节没有
修改或部署 DLL，没有把 v2/碰撞体写入游戏运行时。

### 20.28 Blender 0.14.0 物理工作区、集合隔离与参数面板

本轮只修改 Blender 作者工具和文档，没有修改、构建或部署 DLL，也没有向游戏目录写文件。问题来源已经分别定位：
整包导入一直复用全局 `EIEM` 及同名 LOD 集合，导致多个 package 混在一起；原生物理对象和全部线框可视化同时显示，
导致 11 个组、27 个碰撞体和辅助曲线在视图中叠加；面板每次重绘都会重新读取并解析包含 556 个 Transform 的
`components.json` 文本，普通参数修改还会重建可视化几何，因此参数操作出现卡顿。

0.14.0 把一次导入建立为独立 package 工作区：`EIEM / EIEM <package> / Meshes、Skeletons、Physics`。
同一路径再次导入会生成不同的 import id 和唯一集合名，不再把对象并入前一次导入。每个 Physics 工作区继续分为
`Groups`、`Colliders`、`Visuals`；集合只负责场景整理和选择，不承担物理拓扑语义。旧场景可在物理面板执行
“整理当前 Rig”，把已有物理辅助对象迁入这三个分类；要获得完整的逐 package Mesh/Skeleton 层级，应使用 0.14.0
重新导入 package。

物理关系仍以真实源数据为准。一个 BoneCloth 组 Empty 保存组参数、`rootBones`、`ignoreFromRootBones`、
SelectionData 和碰撞体引用；它可以展开出多个根和分支，不强制解释成单条线性链。Collider Empty 独立保存类型、
绑定 Transform、形状参数和源组件身份，同一个碰撞体可以被多个组引用。骨骼本身不复制一套“所属物理”参数；
骨骼、组和碰撞体通过上述引用关联。Transform 层级和 SelectionData 点数组继续分别展示，在没有实证映射时不猜测
“第 N 个点等于第 N 根骨”。

参数的唯一可编辑来源继续放在组或碰撞体 Empty 的 Blender RNA PropertyGroup 中，而不是再镜像一份 ID 自定义属性。
面板增加五个常用组参数的直接编辑区，完整原生参数支持搜索并每页显示 32 项。源文档解析结果按 Text 数据块内容缓存，
普通求解参数修改不再销毁并重建曲线；只有 center、radius、length、direction 等影响碰撞体形状的字段才更新几何。
这样保存、撤销和导出仍使用同一份结构化数据，同时消除重复 JSON 解析和无关几何重建。

默认视图只显示当前物理组及其引用的碰撞体，并关闭穿透显示；还可切换为仅组、仅碰撞体、全部或全部隐藏。
组面板显示根数、物理 Transform 数和 Selection 点数，碰撞体面板显示绑定骨骼、被哪些组共享，并可从碰撞体跳转到
一个引用组。Typhoea 样本中的 19 个躯干/身体层级碰撞体与 8 个附件层级碰撞体都保留在 `Colliders` 中；11 个
模拟组属于头发、衣物、附件和尾部，没有独立的“躯干模拟组”。躯干在这个样本中主要提供动画锚点和共享碰撞环境，
其碰撞体参数仍可在对应 Collider Empty 上直接修改。

交互组织参考了开源 [RE Chain Editor](https://github.com/NSACloud/RE-Chain-Editor) 的做法：活动链文件、碰撞体集合、
Empty 参数面板、按类别隐藏和独立穿透开关。这里只采用工作区与可见性思路；没有复制 RE Engine Chain2 字段、节点几何
或物理单位，终末地数据语义仍来自当前 `components.json`、TypeTree 和已经验证的 EIEM 作者/运行时合同。

Blender 5.0.1 的完整 Blender 包装测试运行 12 项，其中 8 项通过，4 项因当前测试入口未配置 MSVC/外部样本而跳过，
0 项失败。针对性测试覆盖真实 Typhoea 物理源、默认当前组可见性、缓存隔离、普通参数不重建几何、逐 package 集合隔离、
重复导入不混合对象，以及插件注册、卸载和三轮重载。七个运行文件已逐一哈希同步到
`E:\vscode\EIEM_Blender`。安装包为 `bin/EIEM_Blender-0.14.0-physics-authoring-ui.zip`，大小 75252 字节，
SHA256：`AA5CF4B674404D14E4C22AEEFBDF76EC49A607ED1BE2B7DE511FFDC67B5151C9`；压缩包解压后也通过 Blender 注册与三轮重载测试。

### 20.29 当前 Typhoeus 工程的骨骼朝向审计

使用 Blender 5.0.1 独立后台进程只读加载用户当前打开并已保存的
`G:\zmd\typhoeus\Typhoeus_1.0.blend`，没有保存或修改该工程。场景中有两份 EIEM Rig：
`Skeletonchr_0034_typhoea_postmodel_0` 为 316 根骨，缺少源物理图要求的 48 个末端物理节点；
`Skeletonchr_0034_typhoea_postmodel_0.001` 为 397 根骨，完整覆盖 140 个实际模拟 Transform 和
192 个物理作者依赖 Transform。后续查看或创建物理应使用第二份完整 Rig，直到旧的重复导入被清理。

完整 Rig 的 397 根骨逐一用嵌入 `.blend` 的 `native-authoring` 源图重建游戏世界矩阵，再通过 EIEM 的
Unity 左手 Y-up 到 Blender Z-up 基变换比较。head 位置最大误差为 0，矩阵旋转最大误差为 0；保存的
`eiem_rest_display` 与当前 `matrix_local` 的最大误差也为 0。因此当前骨架不存在坐标轴交换、手性或四元数
转换错误。

Blender 外形确实容易被误认为朝向错误：397 根导入骨骼全部使用约 0.05 的显示长度，均未设置 connected。
140 个模拟 Transform 内有 112 条实际父子边；bone tail 与子节点 head 方向的中位夹角为
`89.99997°`，其中 110 条超过 45°。这是源 Transform 局部 `+Y` 轴与节点层级方向并非同一概念造成的，
不是导入误差。绿色“层级连线”连接父子 Transform 原点，才代表物理链静止方向；bone tail/roll 保留局部
旋转坐标系。

这一区别对作者操作有实际影响。原生源骨骼导出使用保存的局部 TRS，并拒绝已经改动的绑定姿态，所以当前外形
不会破坏导入的原生组、Selection 或碰撞体。新增骨骼则从 Blender `matrix_local` 生成新 Transform：head 和父级
决定局部平移，tail 方向及 roll 决定局部旋转；显示长度本身不写入 Physics 参数。把源骨骼强制 connect、统一
重算 roll，或为了让八面体指向子节点而移动 tail，会改变局部旋转并使源骨架导出被拒绝。复制新增物理链保留
原骨骼的 head/tail/roll，因此不会自行引入朝向变化。

0.14.0 面板据此增加“Blender 骨骼外形”视图选择，并在原生组中明确标出“绿色层级连线=物理链方向、
bone tail=源 Transform 局部 +Y”；新增组也提示 head/父级与 tail/roll 的不同导出作用。该选择只改变 Blender
视图画法，不修改 Skeleton 或 Physics 数据。本节没有修改或部署 DLL，也没有写入游戏目录。

### 20.30 原生胶囊端点公式与 Blender 0.14.1 修正

用户在 Blender 中发现 `Magica Capsule Collider (Bip001_L_Thigh)` 从髋部向上伸入躯干。复核确认原始
`center=(-0.02,0,0)`、`size=(0.1,0.105,0.497)`、`direction=X`、`reverseDirection=false`、
`alignedOnCenter=false` 均读取正确，Collider owner 的源 Transform 与 Blender 绑定矩阵也一致；错误来自
0.13.0/0.14.0 预览把 `size.z` 猜成两端球心距离，并把非居中胶囊画成 `center → center+axis*length`。

本机 `GameAssembly.dll` 的 BeyondDynamicBone 方法体给出了完整公式。`GetColliderType` `0x4563880` 把
居中的 X/Y/Z 胶囊编码为 2/3/4，非居中编码为 5/6/7；`GetSize` `0x44BFB70` 在
`radiusSeparation=false` 时用 startRadius 同时替代 endRadius；`GetLocalDir` `0x59DC020` 选择局部
X/Y/Z，并在 `reverseDirection=true` 时取反。`StartSimulationStepJob.Execute(int)` `0x5A66A3C`
进一步计算两端球心：

```text
axis = reverseDirection ? -localAxis : localAxis
if alignedOnCenter:
    start = center + axis * max(length / 2 - startRadius, 0)
    end   = center - axis * max(length / 2 - endRadius, 0)
else:
    start = center
    end   = center - axis * max(length - startRadius - endRadius, 0)
```

这证明 `length` 是包含两端圆头的胶囊总长，start/end 保存的是端部球心，并在半径之和超过长度时把球心间距
钳制为 0。官方 MagicaCloth2 文档也明确把 `size` 记为 `(start radius, end radius, length)`，并说明关闭
Aligned On Center 后以胶囊起点作为旋转中心；本机方法体补足了公开文档没有列出的具体算式。

Typhoea 的全部 19 个非居中胶囊也提供了独立几何交叉检查：默认方向的旧预览都背离宿主骨骼的后代；按原生
负端公式后，左右大腿朝小腿、左右上臂朝前臂、左右前臂朝手、脊柱朝上级躯干、颈朝头、两侧发束朝后续发骨。
其中左大腿的 start 球心仍在髋部附近，end 球心沿腿向下约 `0.497-0.1-0.105=0.292`，不再向上进入躯干。

Blender 0.14.1 已按上述球心公式生成胶囊线框，保留原字段、绑定、导出字节和增量规则。该修正只改变原生
碰撞体的作者预览，不修改 DLL、Physics v2 文件或游戏目录；碰撞响应仍需在以后恢复 DLL 碰撞实验时验证。
七个插件文件已同步到开发目录和 Blender 5.0 用户插件目录；安装包为
`bin/EIEM_Blender-0.14.1-native-collider-geometry.zip`，大小 75502 字节，SHA256：
`6F4961983C18CC82FEFA5A4E1C2686650D9689B9D5589A5F655021859C44ABFD`。

### 20.31 Blender 0.15.0 物理工作区、曲线与实体显示

按当前作者流程继续完善 Blender 侧，没有修改 Physics v1/v2 二进制合同、DLL 或游戏目录。Group 与 Collider
Empty 的 RNA PropertyGroup 仍是唯一参数真值；骨骼只承担稳定身份、父子层级和静止变换。原生 Group 现在把
每个 `rootBones` 项作为一条可单独选择的根分支列出，并在同一面板明确列出源组实际引用的 Collider Empty。
这与源数据的真实关系一致：一个 BeyondBoneCloth 组可含多个根和分支，组内共享一套参数、曲线与碰撞体列表；
同一个碰撞体仍可被多个组引用。需要不同参数或碰撞集合的骨链应划分为不同物理组。

原生组增加九类结构化曲线编辑入口：阻尼、节点半径、距离约束强度、角度恢复强度、角度限制、最大运动距离、
回挡距离、碰撞限制距离和自碰撞表面厚度。界面按源字段直接编辑基础值、`useCurve`、关键帧的链位置/倍率，
以及可展开的切线、权重模式与权重；不建立第二份自定义属性，也不把 Selection 点顺序猜成骨骼顺序。
当前实现是数值关键帧编辑器，尚未提供图形曲线画布或按骨骼着色的曲线采样预览。交互组织参考固定版本的
[RE Chain Editor 几何节点实现](https://github.com/NSACloud/RE-Chain-Editor/blob/54ed5d41a6360511b4b314c80e9b459032b32688/modules/re_chain_geoNodes.py)，
只借鉴实体节点、连接体与分类显示方式，不复用 RE Engine Chain2 字段或物理单位。

辅助显示默认从三环线框改为低面数实体：Selection 节点为彩色球，父子层级边为实体管，球/胶囊/平面碰撞体
为半透明网格；“视图显示 → 样式”可随时切回线框。默认范围仍为“当前组”，只显示该组及其引用的共享碰撞体；
穿透显示默认关闭。作者 v1 组同样使用实体节点与链段，球/等半径胶囊使用同一显示器。Blender Armature 自身的
黑色 bone tail/roll 仍由“Blender 骨骼外形”控制，它不是物理链连接体；绿色实体连接才表示父子 head 方向。

Blender 5.0.1 针对性验证通过插件发现及三轮注册/卸载、v1 作者创建/保存/重开、真实 Typhoea v2 源数据往返，
以及正常 package 整体导入。正常包结果仍为 62 Mesh、1 共享 Rig、11 组、27 碰撞体和 192 个物理依赖 Transform。
真实源数据的 11 个组均识别到九类曲线字段，显式 Collider 引用与源列表一致；实体/线框双向切换和三种原生
碰撞体均有断言。当前打开的未保存 Blender 场景已热重载到 0.15.0，保留 11 组和 27 碰撞体，重建为 61 个
Mesh 辅助体、0 个旧 Curve 辅助体，导入与导出菜单回调各保留一个；当前视图设为“当前组”、实体、关闭穿透。

七个运行文件已逐一哈希同步到 `E:\vscode\EIEM_Blender`。安装包为
`bin/EIEM_Blender-0.15.0-physics-solid-workspace.zip`，大小 80272 字节，SHA256：
`884121E2634B166A9E06FDD0FD085ACC398144497E7C1D0E54A94ABB56A5FA52`；压缩包完整性及解压后的三轮注册测试通过。

### 20.32 Blender 0.16.0：Empty 参数、F-Curve 与链刷新

0.15.0 的数值曲线区和侧栏同时承担参数、拓扑、选择、文件和显示操作，导致常用入口过多；新建组又只在创建
瞬间读取骨骼选择，后续新骨骼没有明确加入动作，预览刷新边界也不清楚。0.16.0 将作者数据所有权和快捷入口
分开：Group/Collider Empty 继续保存唯一 RNA 源参数，选中 Empty 后在对象属性的 **EIEM 物理参数**中编辑；
N 侧栏首层只显示**当前组、新建、复制、查看**，链结构、显示和文件工具默认折叠。选中另一个 Group Empty 时，
它通过 Blender RNA 消息总线成为当前组，不使用每帧轮询。

原生组的九类链位置倍率曲线现在投影为该 Group Empty 自己的 Blender Action/F-Curve。横轴 0～100 对应源位置
0～1，纵轴为倍率，曲线静音对应 `useCurve=false`；源切线转换为 Bezier 手柄。用户可从对象属性一键打开 Graph
Editor，日常曲线名使用短中文，原始 TypeTree 路径只在折叠的高级源字段中保留。Action 记录曲线签名；只有用户
实际修改曲线后，应用或导出才写回源字段，未改数据不会因 Blender float32 或手柄表示被重写。当前写回保持原
关键帧数量，新增或删除关键帧会明确拒绝，避免伪造尚未定义的 Unity 曲线字段。

新增 v1 拓扑继续遵守线格式合同：一个组是一棵具有单一固定根的连通树。创建时选择完整连续骨链；同一链新增
后代后用**将所选骨骼加入当前链**，独立根另建组。加入前运行完整 v1 拓扑校验，第二根不会留下半修改状态；
加入、移除与创建立即重建预览。Armature 静止数据变化由 depsgraph 标记，在退出编辑模式后合并重建该 Rig 的
作者组，避免编辑期间反复刷新。原生 v2 拓扑仍只读，没有从 SelectionData 猜写新骨骼映射。

Blender 5.0.1 回归已通过插件三轮注册/卸载、消息总线与 depsgraph 清理、v1 链加入/第二根拒绝/移除/刷新、
真实 Typhoea 11 组 27 碰撞体导入、九条 F-Curve 编辑写回、未改源精确往返以及 `.blend` 保存重开。
本节只修改 Blender 作者工具和文档，没有修改 DLL、Physics 二进制合同或游戏目录；v2 与碰撞体的游戏实例化
边界保持不变。七个运行文件已逐一哈希同步到 `E:\vscode\EIEM_Blender`，当前 Blender 会话已热重载为
0.16.0，仍为 11 组、27 碰撞体、61 个实体辅助 Mesh、0 个旧 Curve 辅助体，导入/导出菜单回调及
depsgraph 处理器各一个；没有保存用户 `.blend`。安装包为
`bin/EIEM_Blender-0.16.0-fcurve-object-ui.zip`，大小 85941 字节，SHA256：
`CA34E63A2445B6D77E700A9B80988ECE2CF3E89EEACF3311D018E1E13EF142E9`；解压后的包已通过三轮注册测试。

### 20.33 角度限制与 Chain2 锥体的差异

Blender 0.16.0 尚未生成三维角度锥；当前只把
`serializeData.angleLimitConstraint.limitAngle` 映射为 Group Empty 的 Blender F-Curve。曲线静音映射的是
`limitAngle.useCurve`，不代表整个约束的 `useAngleLimit`，因此只看 Graph Editor 可能把“保存了曲线但总开关
关闭”误认为角度限制正在生效。后续常用参数面板应把**启用角度限制、限制角、曲线和边界回弹强度**放在同一区域。

终末地源字段与 MagicaCloth2 的公开 `AngleConstraint.LimitSerializeData` 一致：组级配置包含
`useAngleLimit`、CurveSerializeData `limitAngle` 和 `stiffness`。它限制 baseline 中每条边相对父边/基准姿态
可弯曲的角度；最终角度为基础角乘以该顶点 depth 上的曲线倍率，`stiffness` 控制越界后的回弹软硬，
`animationPoseRatio` 还会影响使用初始姿态还是当前动画姿态作为基准。参考：
https://magicasoft.jp/en/mc2_magicacloth_anglelimit/ 与 https://magicasoft.jp/en/mc2_baseline/ 。

Typhoea 的 11 个 BoneCloth 组中有 4 个启用 `useAngleLimit`：裙摆 18°、长马尾 90°、侧长发 36°、尾巴
60°；其余 7 组虽然保留默认 60° 或曲线，但总开关关闭。样本全部 164 个 Selection 位置都能在组根展开的
Transform 原点中找到几何对应，最大误差约 `2.55e-6`；长马尾的 38 点包括 14 个活动节点与 24 个 IGNORE
分支点。这为按位置建立 Typhoea 样本映射提供了证据，但没有把数组序号证明为通用身份合同，也尚未取得原生
每点 depth 缓存。

怪猎荒野 Chain2 则把角度半径、模式和角度限制方向作为节点侧数据，并由独立 `_ANGLE_LIMIT` 对象控制每个锥体
朝向；RE Chain Editor 还提供对齐方向和沿链设置半径坡度的操作。其仓库记录也明确存在某些骨向/属性标志下锥体
方向相反的问题：https://github.com/NSACloud/RE-Chain-Editor 。因此 EIEM 可以采用类似的锥形交互，但不能复制
Chain2 的节点字段或方向对象。正确的 EIEM 预览应从终末地 baseline 父子边与基准姿态生成锥轴，以
`limitAngle.value × curve(depth)` 作为半角；关闭 `useAngleLimit` 时隐藏或灰显。精确显示前还需明确 BoneCloth
depth 的生成规则，并用原生场景 Gizmo 或求解结果交叉验证锥轴，不能仅按 Blender bone tail 猜方向。

### 20.34 Blender 0.17.0：角度限制锥与原生 depth 规则

本轮继续限定在 Blender 作者工具、只读二进制调查和文档，没有修改或部署 DLL，也没有写入游戏目录。
`VirtualMesh.CreateVertexRootAndDepth`（本机 `GameAssembly.dll` RVA `0x374B590`）会安排
`BaseLine_CalcMaxBaseLineLengthJob`；后者的 `Execute`（RVA `0x313FED0`）给出了 depth 的实际算法。它只对
MOVE 属性（位 2）的点工作：从当前点沿 `vertexParentIndices` 向上，累计 `localPositions` 的父子距离，包含到
首个非 MOVE 父点的最后一段；随后取全组最大累计长度，把所有点的累计值除以该最大值并钳制到 0～1。
所以 depth 是**组内统一的归一化累计骨段长度**，不是数组序号或骨骼层数；多个根和分叉会共同使用最长路径
作为分母。

0.17.0 据此为 `useAngleLimit=true` 的原生 BoneCloth 组生成黄色半透明角度锥。视图先用组局部静止位置把
Selection 点与 `rootBones` 展开的 Transform 原点做一对一几何匹配，再为每条非零 MOVE 父子边建立锥面：锥尖
位于父点，轴沿静止 baseline 父点→子点，球面半径取该骨段长度，半角为
`limitAngle.value × limitAngle.curve(depth)`。这种球面构造在 90° 时形成有限圆盘，不会产生无限锥底。
Typhoea 裙摆、长马尾、侧长发和尾巴四个启用组均能生成锥体；关闭总开关的七组不显示。几何匹配只服务视图，
没有把 Selection 数组下标改写成通用骨骼身份、Mesh palette、setupIndex 或 Animator 写回槽。

Group Empty 的对象属性现在把**启用角度限制、基础角度、限制刚度**放在同一区域，直接代理原字段。
`limitAngle.useCurve` 保持独立：Graph Editor 中角度曲线未静音，只能说明使用曲线，不能说明总约束已经开启。
Action/F-Curve 的修改通过 depsgraph 标记并短延迟合并刷新，只重建该组的角度锥；普通重力、阻尼等参数仍不
重建整套辅助几何。Unity 曲线的无权重和加权 Bezier 手柄都参与预览，并在应用/导出时把切线、
`weightedMode`、`inWeight/outWeight` 写回原字段，关键帧数量仍保持不变。

锥体表达静止作者基准，不是 Blender 内的物理求解。终末地运行时会依据 `animationPoseRatio` 在初始姿态和当前
动画姿态之间形成动态 baseline，因此角色运动时求解器使用的瞬时锥轴不会由静态辅助网格逐帧模拟。该限制不影响
编辑基础角、曲线和刚度，也避免为终末地组伪造 Chain2 独有的节点角度方向对象。

Blender 5.0.1 的真实 Typhoea 测试确认 4 个启用组共生成 54 个非零骨段锥体：裙摆 29、长马尾 12、侧长发 6、
尾巴 7；尾巴在 depth `0.5` 的 60° 基础角经线性曲线得到 30°。实时测试把裙绳末端曲线倍率从 1 改为 0.5，
无需手动刷新，其锥体末端半角从 60° 更新为 30°；随后已恢复曲线和关闭临时总开关。当前 Blender 会话热重载为
0.17.0，保留 11 组、27 碰撞体，辅助显示为 65 个 Mesh（其中 4 个锥体对象）、0 个 Curve；两个 Physics
depsgraph 处理器各一个，没有保存用户 `.blend`。

完整 Blender 测试运行 12 项，7 项通过，5 项因未提供正常整包样本或 MSVC 环境而跳过，0 项失败；真实源测试
覆盖开关分离、depth/角度采样、实体与线框锥体、曲线写回、源精确往返及保存重开。七个运行文件已逐一哈希同步到
`E:\vscode\EIEM_Blender`。安装包为 `bin/EIEM_Blender-0.17.0-angle-limit-cones.zip`，大小 90137 字节，
SHA256：`55EE280211E044B403B1F5874D80BA277AF1A529B363BBC528CB54F7C5CE63C5`；解压后的包通过插件发现和三轮重载。

### 20.35 Blender 0.18.0：物理作者代码整理

本轮只整理 Blender 作者工具和测试，没有修改 DLL、Physics 资源合同或游戏目录。审查范围为
`eiem_blender_addon.py` 与四个 Physics 模块，共由 6452 行降至 6364 行；行数不是目标，减少的 88 行均对应
已经确认的重复实现或无调用入口。

- 删除了从未被调用的旧 `collider_paths()`，把三种碰撞体的线框路径统一到实际使用的
  `collider_wire_paths()`；实体/线框预览继续覆盖球、胶囊和无限平面。
- Curve 与 Mesh 辅助对象原先各自重复集合链接、父级、不可渲染/不可选择、穿透和标记设置，现在统一经过
  `finish_visual()`。新增 v1 与原生 v2 的节点/连线也共用同一组样式分派函数，只保留半径和坐标基底差异。
- 原生组层级连线与角度锥匹配原先各自计算 Transform 世界矩阵和组件局部原点，现在统一由
  `component_local_graph()` 给出，避免同一源图出现两种局部空间解释。
- 对象属性中逐关键帧的“曲线参数”表与 Blender Graph Editor 编辑同一份投影，属于重复操作界面，已经删除。
  日常曲线入口只保留“查看 / 编辑 9 条曲线”；导出仍会检测 Action 签名并自动写回源 RNA 字段，高级源字段仍可
  筛选和分页查看完整 TypeTree 数据。没有删除曲线编解码、切线/权重往返或源字段保留。
- F-Curve 变化原先同时经过原生 depsgraph 处理器和 0.2 秒签名轮询。直接修改 F-Curve 在 Blender 中并不保证
  产生稳定的 Action depsgraph 更新，因此保留只在“启用角度限制且已有 EIEM Action”的组存在时运行的轮询，
  删除原生 depsgraph 处理器。v1 骨架静止姿态仍由作者模块唯一的 depsgraph 处理器合并刷新。
- “刷新当前链显示”与创建、加入、移除及 Armature 自动重建重复，按钮和操作分支已删除。碰撞体的“刷新骨骼
  绑定”保留，因为它会重算 Child Of 逆矩阵；“整理当前 Rig”保留用于旧 `.blend` 集合迁移。

以下能力经审查后明确保留：`Groups / Colliders / Visuals` 分别承载组数据、可共享碰撞组件和可随时重建的视图；
v1 新增作者格式与 v2 原配置往返具有不同合同；原生组完整模板复制与映射到 v1 的五标量复制目标不同；实体与
诊断线框是同一数据的两种视图。这些不是重复功能。

Blender 5.0.1 的 12 项宿主回归为 7 项通过、5 项因未提供正常整包样本或 MSVC 而跳过、0 项失败。真实
Typhoea 测试覆盖 11 组、27 碰撞体、54 个角度锥，新增了“直接改 F-Curve 后仅由轮询标记并重建当前组”的
验证；注册测试确认作者模块有 1 个 depsgraph 处理器、原生模块为 0，并通过三轮注册/卸载。七个运行文件已逐一
哈希同步到 `E:\vscode\EIEM_Blender`。安装包为 `bin/EIEM_Blender-0.18.0-code-cleanup.zip`，大小 90082 字节，
SHA256：`67090877CFD55662236EBE522B03872BF5AEB498835ABA912D983F1E11F08B2C`；解压后的包通过插件发现和三轮重载。
当前交互 Blender 的 MCP 监听端口仍存在，但两次调用均未在超时内返回，所以本轮不把该会话记为热重载完成，
也没有保存用户 `.blend`。

### 20.36 胸部骨骼驱动来源：离线边界与 v72 运行时局部变换采样

本轮先沿 Typhoea 模型 Prefab 的 Animator 引用继续追踪。新增的独立离线命令
`EndfieldVfsProbe --inspect-prefab-animation` 会从同一 VFS 指纹和依赖闭包中解析 Animator、
RuntimeAnimatorController、AnimationClip 与 Transform GenericBinding。对
`chr_0034_typhoea_postmodel.prefab` 的实际结果为：1 个 Animator，但
`m_Controller=null`、0 个可解析 Clip、0 条 Transform binding。因此，模型包本身不能回答
`breast_base_L/R_a_01/02_jnt` 是否有动画曲线；角色系统在运行时装配控制器。机器可读结果位于
`bin/diagnostics/typhoea-animation-bindings-20260909.json`。这项结果只确定离线边界，不等于
“胸骨没有动画”。

为区分“胸骨只继承 Spine2 的世界运动”与“胸骨局部 TRS 每帧被某个运行时系统写入”，v72 在
`eiem_native_physics_diagnostic.h` 中加入独立的胸部运动观察。它不进入 Dump 或主功能 UI，
随现有自动诊断启动，并在已有 `SolverManager.LateUpdate` 原调用返回后每 50 ms 采样
`Bip001_Spine2`、左右 `breast_base_*_a_01/02_jnt` 的局部/世界位置和旋转。每个模型最多记录
3600 次；退出时在同一 TSV 尾部写入各骨骼相对首帧的最大局部/世界位置差和四元数角差。输出路径为
`plugin/physics_diagnostics/chest_motion_<pid>_<tick>.tsv`。若胸骨局部变化接近零、世界变化明显，
证据支持继承躯干；若局部旋转或位置明显变化，则可确认存在局部写入，但仅凭这个采样点还不能把写入者
进一步断言为 Animator、IK 或其他运行时作业。

`test_native_physics_trace.py`、EndfieldVfsProbe .NET 9 Release 构建和完整 `build.bat` 均通过。
游戏未运行时已备份原安装 DLL/日志到
`E:\EIEM_Workspace\plugin-releases\before-v72-chest-motion-20260909-131214`，并部署 6266880 字节
诊断 DLL；构建和安装 SHA256 均为
`4F1679649F535C51329DF4469645347622A544BC83CCDB148B3A3E9A4AB199DC`。尚未启动游戏或取得胸骨采样，
所以本节暂不写运动来源结论。
#### v72 实机结果

PID 33520 正常退出并生成 8703 行、2822865 字节的胸部运动记录。日志把六个观察对象明确对应为：
角色模型、角色 UI、两个 NPCAvatar.StartNPC 所有者、能力实体和对话时间线对象。判断骨骼是否被独立驱动
必须看局部旋转，跨场景导致的 520.47 世界位置差不能当成胸部形变。

角色模型连续采样 626 次。相对首帧，Bip001_Spine2 的最大局部角差为 35.65°；左胸第一级/第二级分别为
43.38°/83.80°，右胸第一级/第二级分别为 40.03°/69.35°。角色 UI 连续采样 104 次，对应值为
Bip001_Spine2 23.41°、左胸 8.56°/18.79°、右胸 6.48°/18.15°。胸部第二级节点的局部位置近似不变，
但局部旋转明显改变，所以这些节点存在旋转通道；Bip001_Spine2 的世界变换继承无法单独解释这个结果。

两个 NPC 都由精确 owner 日志确认。P_actor_typhoea_01 在 211 次采样期间完全静止；另一个
chr_0034_typhoea_postmodel 的 Bip001_Spine2 局部角差为 3.02°，左右胸第一级局部角差约
0.11°，第二级在该时间窗口保持局部不变。这说明 NPC 是否出现胸部局部变化取决于当时是否实际播放了
相关动作、是否被更新及该动作是否含相应通道；不能据一个静止 NPC 推断 NPC 骨架没有胸部驱动。

原生配置关系保持不变：Typhoea 的 11 个 BeyondBoneCloth 中没有任何 rootBones 指向胸部节点。
胸部相关的三个组件均为胶囊碰撞体：Spine2_Breast、左胸一级和右胸一级；三者只被
MBC_Typhoea_Hair_Front_Side_Long 的 collider list 引用。因此，Blender 中不应虚构一个
“胸部 BoneCloth 物理组”。胸部骨骼自身会被运行时动画链路写入，而依附其上的碰撞体随骨骼移动并供
侧长发组碰撞。当前证据高度支持运行时 Animator/动画控制链路，但采样点只能证明帧末局部写入，
尚不能区分 AnimationClip Transform 曲线与另一个 Animator job，也不能声称每个 Clip 都给胸骨打了
关键帧。

完整证据归档于 bin/diagnostics/v72-chest-motion-runtime-33520/：
analysis.json、原始 TSV、完整 EIEM 日志、原生 Physics 运行快照及离线 Animator 报告。
原始 TSV SHA256 为
99A8B14251133B08E76D429BB57CA9C4E333D9A9196C1FF25ED595E9FF97D369。

### 20.37 Blender 0.19.0：物理链节点碰撞半径可视化

本轮只修改 Blender 原生物理链预览、测试和文档，没有修改 DLL、游戏目录或独立 Collider 编辑。此前原生组的
FIXED/MOVE Selection 点使用固定 0.006 米球，只能表达节点角色，不能表达 `serializeData.radius`。0.19.0 改为
按每个 Selection 点的原生 depth 计算 `radius.value × radius.curve(depth)`；`radius.useCurve=false` 时倍率为 1。
depth 继续使用 20.34 节已经从 `VirtualMesh.CreateVertexRootAndDepth` 方法体确认的组内最长累计骨段长度规则。
FIXED 与 MOVE 分别保留橙色和蓝色，实体模式生成真实尺寸低面数球，线框模式生成三个正交圆环；IGNORE 不是
模拟点，继续使用固定灰色拓扑标记，不虚构碰撞半径。

角度与半径共用同一个“基础值 × 可选曲线”求值函数。原来的角度专用刷新队列改为组预览刷新队列；修改基础
半径、`radius.useCurve`、半径关键帧/切线，或直接在 Graph Editor 调整半径 F-Curve，都会合并重建当前组的节点
半径与角度锥。普通阻尼、重力等不影响几何的字段仍不重建预览。F-Curve 轮询现在跟踪所有已建立 EIEM Action
的原生组，因此即使该组没有开启角度限制，半径曲线也会实时更新。

Blender 5.0.1 真实 Typhoea `components.json` 测试验证 11 个组、164 个 Selection 点的半径样本与原生曲线求值
一致；实体球首顶点到球心的距离等于样本半径，线框每点恰有三个圆环。测试还直接修改一个启用曲线的组末端
半径倍率，确认无需手动刷新即可重建节点球并在恢复后保持源数据精确往返。普通 package 的 Mesh/Rig/Physics
联合导入与插件三轮注册/卸载继续通过。本结果验证作者视图和序列化合同，不等同于 DLL 运行时碰撞响应验证。

七个运行文件已逐一哈希同步到 `E:\vscode\EIEM_Blender`。安装包为
`bin/EIEM_Blender-0.19.0-node-radius-preview.zip`，大小 89988 字节，SHA256：
`36C39A46ADA4D9C86E334019F4A16BEE480574F31B1A7EE834D11392660FECA3`；ZIP 内容完整性及从开发目录进行的三轮
Blender 插件注册/卸载均已验证。

### 20.38 Blender 0.20.0 / 作者 v3：新增链节点半径曲线与 DLL 配置映射

本轮修正了新增作者链与原生导入链之间的不一致。`BeyondBoneCloth.serializeData.radius` 是模拟点自身的
碰撞厚度配置；它与组引用的独立球、胶囊、平面 Collider 是两类数据。0.19.0 只给原生导入组显示
`radius.value × curve(depth)`，而新增作者组仍使用固定装饰点，作者文件也只保存五个标量。0.20.0 将节点半径
加入新增链的唯一作者数据源，独立 Collider 本轮保持原状。

二进制新增作者格式使用版本 3，因为版本 2 已被 `native-authoring` 源图占用。v3 在每个组的五个基础标量后保存：

- 基础半径 `value` 与 `useCurve`；
- 2～64 个关键帧，每帧含归一化时间、倍率、进入/离开切线、`weightedMode`、进入/离开权重；
- 源曲线的 pre/post infinity 与 rotation order 元数据。

Python 与 C++ 均严格校验关键帧位于 0～1、时间递增、有限数值和权重范围；不强制首末帧恰好为 0/1，因为
Typhoea 灯笼组的真实半径曲线末帧为 `0.9974365234375`，长马尾组的首末帧为
`0.001953125` / `0.9961351752281189`。旧 v1 文件仍可读取和原样重编码；
载入 Blender 后会获得 0.006 米、倍率恒为 1 的默认半径曲线，再次导出使用当前 v3。没有把原生源图 v2 强制
降级为作者 v3。

新增组 Empty 的对象属性现在直接显示“节点基础半径”和“使用半径曲线”。“在曲线编辑器查看”打开属于该
Empty 的 Blender Action/F-Curve；横轴 0～100 对应组内从固定根沿 MOVE 父链累计的骨段长度，再除以本组最远
末端长度，纵轴为半径倍率。用户可在 0～100 内增删关键帧并编辑 Bezier 手柄。曲线、静音状态或基础
半径变化由 0.2 秒签名轮询合并刷新，无需手动重建。FIXED/MOVE 点使用真实尺寸实体球或三个线框圆环，IGNORE
仍只是拓扑标记。复制作者组会复制半径曲线；从原生模板映射时会复制五个基础参数及原生节点半径完整关键帧。

DLL 配置映射依据当前游戏 `global-metadata.dat` 的可核对合同实现：`CurveSerializeData` 明确含
`value:System.Single`、`useCurve:System.Boolean`、`curve:UnityEngine.AnimationCurve`，并具有
`SetValue(System.Single, UnityEngine.AnimationCurve)`；`AnimationCurve` 具有
`.ctor(UnityEngine.Keyframe[])` 与 `get_keys()`；`Keyframe` 的七个字段顺序对应 time/value/in/out tangent、
weighted mode、in/out weight，值类型大小为 28 字节。运行时按程序集、命名空间、类名、方法签名和字段类型解析，
同时核对 `il2cpp_class_value_size==28`，没有加入游戏版本字段偏移。v3 配置创建 `Keyframe[]` 和
`AnimationCurve`，调用 `CurveSerializeData.SetValue`，写入 `useCurve`，随后从新建 `ClothSerializeData`
逐项回读基础值、开关、曲线引用及关键帧字节；任何不一致都不会发布该批草稿。曲线只在 0～1 depth 内求值，
本轮未把序列化 infinity/rotation 元数据猜写成 Unity 的公开 WrapMode。

最终针对性验证为 36 项通过、0 跳过、0 失败，覆盖 Python/C++ v1/v3 线格式、坏文件、资源依赖、配置事务、
Blender 实体半径、F-Curve、复制、保存重开、旧 v1 升级、组合 Mod 和 Typhoea 新骨 fixture。完整 `build.bat`
成功生成 `bin/eiem.dll` 及两个代理 DLL。全仓 219 项中 215 项通过、2 项因缺少真实 Physics 包环境变量而跳过；
`test_material_baseline_lifecycle` 与 `test_partner_controls` 两个既有宿主夹具因其独立缺失声明而编译失败，未涉及
本轮修改文件。开发目录七个运行文件逐一 SHA256 相同，并通过 BlenderMCP 热重载到 0.20.0；会话中的 11 个
原生组保留。安装包 `bin/EIEM_Blender-0.20.0-author-radius.zip` 为 94493 字节，SHA256：
`138C57B8BEA41C16D8D41DB108B2647C26B4B4E3B26F00E4314C39E53A465F36`，ZIP 完整性及解压后的三轮插件注册/卸载通过。

这些结果验证作者交互、序列化、元数据解析和未挂接配置草稿。它们还没有证明 v3 半径会在游戏中产生预期碰撞
响应；该结论需要部署后对同一链使用明显不同半径/曲线做 A/B，并观察碰撞距离。独立球、胶囊、平面 Collider
仍未接入新增组运行时。

### 20.39 Blender 0.21.0：作者组支持原生式多 FIXED 根分支

`maid.002` 的六条三骨裙摆分支暴露了作者 v3 的单根限制。首次修正曾把共同骨架父级 `Bip001_Spine1`
自动加入为唯一 FIXED，并把 18 根裙骨全部标为 MOVE；真实源数据核对证明这种结构不忠实。原生
`MBC_Typhoea_Cloth_Skirt` 的 `rootBones` 是七根 `skirt_base_*_01_jnt`，每根在 SelectionData 中均为 FIXED；
它们的骨架父级是 `Bip001_Spine1`，但 Spine1 不属于该物理组。后续 `02/03/04` 节点为 MOVE。

0.21.0 因此直接扩展作者组的拓扑语义而不改变 v3 二进制布局：一个组可包含一个或多个根，每个根必须为
FIXED，非根不得标为 FIXED，组内至少有一个 MOVE。Blender 选择多条分支时保留各分支根，不再补入共同父骨；
所有分支共享组参数、节点半径曲线和碰撞体引用。C++ 读取器采用相同校验；DLL 配置草稿收集全部根并逐项写入
构造器自带的 `rootBones` 列表，再核对数量、顺序和对象引用。两根宿主配置、两分支 Blender 创建、追加、保存
重开及 Python/C++ 往返均有回归覆盖。

当前 Blender 会话中的 `maid.002 Skirt Physics` 已改为 18 个节点：六根 `maid_skirt_*_a_jnt` 为 FIXED，
十二根 `_b/_c_jnt` 为 MOVE，`Bip001_Spine1` 只保留为六根骨骼的父级。独立碰撞体与 v3 半径的游戏内 A/B
边界不因本节变化。最终作者格式、资源、DLL 配置、Blender 创建/注册和新骨夹具共 38 项通过，完整
`build.bat` 通过。安装包 `bin/EIEM_Blender-0.21.0-multi-root-groups.zip` 为 94643 字节，SHA256：
`C3E2A3B5D760156768C7AF27B1F9925BC54399060E5E1EE403251DD6CB49D220`；ZIP 完整性及解压后的三轮插件注册/卸载通过。

### 20.40 Blender 0.22.0：显式参数剪贴板与切换按键录制

物理侧栏原有一个“复制”菜单，内部同时放置复制整个组、设原生模板、应用原生模板和从模板映射作者参数，
用户必须记住操作顺序。0.22.0 将首层改为两个确定动作：**复制参数**与**粘贴参数**。剪贴板保存复制时的
值快照，不是指向源 Empty 的实时引用。复制原生组时，先将其 Blender Graph Editor 中的九类 F-Curve 写回
RNA 源字段，再收集全部可编辑数值和曲线字段；Typhoea 真实组实测为 249 项，阻尼曲线等非基础字段也会在
原生组之间复制。根骨、SelectionData、组件身份、碰撞体引用和链节点不属于参数，保持目标组自己的结构。

新增作者 v3 组当前可表达五个基础参数和节点碰撞半径曲线。粘贴到作者组会一次写入全部这些字段；半径曲线
包括 value/useCurve、关键帧时间和值、进出切线、weightedMode、进出权重、pre/post infinity 与 rotation order。
其余原生字段作为完整快照保存在作者组 Empty 中，并会随下一次复制继续传递；对象参数面板显示快照字段数。
该快照当前不会写入作者 v3 二进制，因而也不会由 DLL 应用。没有把它描述成已可导出数据，也没有修改 DLL
配置合同或游戏目录。

网格切换面板原先要求手填 `eiem_key` 字符串。0.22.0 新增 Blender 模态**录制按键**：点击后按一个键或
Ctrl/Shift/Alt 组合键即可，Esc 取消；录制值经过与 C++ INI 解析器一致的有限键名规范化，并在写入前拒绝
组间重复。创建组仍自动分配可用 F 键，面板同时列出全部“游戏按键 → 切换组”关系。后台 Blender 回归覆盖
字母、数字、翻页键、修饰键排序、重复键不改写、作者完整半径曲线复制、真实源 249 字段复制和三轮注册卸载。

### 20.41 Blender 0.23.0 / 作者 v4：完整参数模板进入资源和 DLL 配置草稿

0.22.0 的参数剪贴板虽然保留了原生组的全部可编辑字段，但作者 v3 二进制只携带五个基础标量与节点半径曲线，
因此“复制 249 项”并不等于 DLL 能收到这些值。0.23.0 将新增作者格式升级为 v4：每组增加
`nativeParameters`，逐项记录 `serializeData.*` 字段路径、浮点/整数类别和值。真实 Typhoea
`ClothSerializeData` 样本在排除拓扑和运行时字段后得到 249 项；五项常用参数和半径曲线在导出前以 Empty
当前值覆盖模板，因而用户复制参数后仍可继续直接调整这些常用项。

资源校验只接受 `serializeData` 下的安全字段路径、有限 float32 和 int32。`sourceRenderers`、`paintMaps`、
`rootBones`、`ignoreFromRootBones`、`colliderList`、`verificationResult`、PathID、数组原始字节等引用、拓扑和
运行时数据明确禁止进入 v4 参数表。这样复制的是新增链可复用的求解参数模板，不会把源组件身份、原生对象引用
或 Selection 顺序伪装成可移植参数。

DLL 配置草稿不使用硬编码字段偏移，而是沿当前游戏 IL2CPP 元数据的字段名和声明类型逐级解析引用对象。
当前支持 `System.Single`、`System.Boolean`、32 位整数、4 字节枚举和完整三分量
`UnityEngine.Vector3 gravityDirection`。阻尼、节点半径、距离刚度、角度恢复刚度、角度限制、最大距离、
回挡距离、碰撞限制距离和自碰撞表面厚度九类 `CurveSerializeData` 会创建 `Keyframe[]` 与
`AnimationCurve`，写入基础值、useCurve、时间、值、切线、weightedMode 和权重，再逐项回读。
资源仍完整保留曲线的 pre/post infinity 与 rotation order；当前运行时写入没有把这三项猜映射为 Unity
公开 WrapMode，因此它们尚不影响配置草稿。任一对象、字段、类型或回读值不匹配时，整批暂存配置不发布。

最终 MSVC 宿主回归 88 项通过，覆盖 v1/v3/v4 跨语言读取、坏文件、标量、布尔、枚举、重力方向、嵌套对象、
曲线和事务失败。Blender 5.0.1 作者回归与 Typhoea 真实源回归均通过，后者确认 11 组、27 个有形碰撞体及
249 项模板从原生组复制到新增组并进入 v4 文档。`build.bat` 已完整生成本地 `bin/eiem.dll`、
`d3dcompiler_47.dll` 与 `vulkan-1.dll`。这些结果尚不包含游戏目录部署或 v4 参数的游戏内响应验证；独立球、
胶囊和平面 Collider 转换仍未接入组合 Mod。

七个插件运行文件已逐一同步到开发目录和 Blender 5.0 的当前安装目录。安装包
`bin/EIEM_Blender-0.23.0-full-physics-parameters.zip` 为 99431 字节，SHA256：
`87B6673F20051AEA5CCDAC1345EE86BD5C12A3A5CB244133D897CCB72F784E46`；ZIP 完整性和解压目录三轮
注册/卸载通过。BlenderMCP 已将正在编辑的会话热重载到 0.23.0；重载前后文件仍为
`G:\zmd\typhoeus\18324_autosave.blend`，174 个对象、1 个作者物理组和未保存状态均保持不变，未自动保存场景。

### 20.42 Blender 0.24.0：作者组完整参数编辑与碰撞集合

0.23.0 已把 `nativeParameters` 写入作者 v4，但 Blender 作者组只保留一份隐藏 JSON 快照，对象面板仍只显示
五项常用参数；参数剪贴板 v1 还明确排除了碰撞引用，且“加入当前物理组”只接受作者碰撞体。因而从原生裙摆组
复制到新增裙摆链后，文件内部虽有 249 项参数，用户却无法查看或修改大部分字段，也得不到原组实际使用的碰撞集合。

0.24.0 将复制来的每个原生字段装入作者组 Empty 自己的 RNA 字段集合。对象属性中的“完整物理参数”使用中文短名、
筛选和每页 32 项显示；修改普通字段会立即写回作者快照，修改五项常用字段或节点半径 F-Curve 会同步到同一字段表，
作者 v4 导出继续从这份当前数据生成 `nativeParameters`。旧 `.blend` 只有隐藏快照时，插件重载会迁移为可编辑字段。
迁移测试同时发现重建预览时修改 `bpy.data.objects` 会使 Blender 的 C 迭代器失效；现改为先固定收集作者组，
再逐组迁移与重建。

参数剪贴板升级为 v2，并附带组使用的碰撞体稳定身份。粘贴到作者组会事务式替换参数和碰撞集合；目标 Empty
可在“碰撞集合”中选择、移除，或从同一 Rig 的作者/游戏源碰撞体下拉框加入。当前组显隐会连同这些引用更新。
原生组之间粘贴仍只改参数，不改原始源图关系。游戏源 Collider 的关联会随 `.blend` 保存，但作者 v4 导出仍明确
拒绝它，因为源球/胶囊/平面到 DLL 配置的转换尚未实现；本节没有把 Blender 关联宣称为游戏碰撞已接入。

Blender 5.0.1 的作者测试、真实 Typhoea 源图测试和正常 package 测试均通过：后两者继续得到 62 个 Mesh、
1 个共享 Rig、11 个原生组、27 个碰撞体与 192 个所需 Transform；原生组的 249 项字段和碰撞集合复制到新增组，
旧快照迁移、字段修改进入 v4、移除/重新加入源碰撞体、带源碰撞体导出拒绝均有覆盖。C++ 作者 reader 也读取
Blender 的 v4 输出通过，插件源码目录和解压安装包各完成三轮注册/卸载。

七个运行文件已逐一哈希同步到 `E:\vscode\EIEM_Blender` 和 Blender 5.0 安装目录。安装包
`bin/EIEM_Blender-0.24.0-editable-physics-groups.zip` 为 102745 字节，SHA256：
`DC24C7D55503104006D805316389217BAC5A64C3A23121691240A18F1019E11C`。BlenderMCP 已把当前会话热重载到
0.24.0；文件仍为 `C:\Users\25487\AppData\Local\Temp\18324_autosave_27152_autosave_31644_autosave.blend`，
174 个对象和当前 `maid.002 Skirt Physics` 保持，旧快照照迁移为 249 个可编辑字段。旧剪贴板没有碰撞身份，
因此按其中唯一源名 `MBC_Typhoea_Cloth_Skirt` 为当前作者组补回了同 Rig 的左右大腿、骨盆和左右小前臂共 5 个
原生胶囊引用；没有调用保存。

本节未修改或部署 DLL，未写入游戏目录，也未验证这些 Collider 的游戏内实例化或响应。

### 20.43 Blender 0.25.0：作者组与原生组统一参数界面

0.24.0 虽已把原生模板的 249 项字段复制进作者组，但作者组仍使用独立的简化参数面板和单独的节点半径
Action；原生组则显示常用参数、角度限制、九条曲线和高级字段。两者数据能够往返，编辑入口和 Empty 自定义
属性却并不一致。节点半径字段的作者回调还会重建旧的单曲线投影，无法维持完整模板的九曲线表示。

0.25.0 以“是否具有完整九曲线字段集”区分完整模板和旧的最小作者数据。完整模板作者组与原生组共用相同的
常用参数、角度限制和高级字段绘制函数；九类 `CurveSerializeData` 投影到同一套九个中文 Empty ID 属性及
Action/F-Curve。复制、粘贴、作者 v4 导入和旧 `.blend` 迁移都会建立这份投影。字段表编辑会重建相应投影，
Graph Editor 编辑仍写回字段表与作者快照；作者预览同时使用节点半径曲线并显示角度限制锥。没有完整模板的
旧作者组继续使用五项最小参数和单独节点半径曲线，避免把缺失的原生字段猜成默认值。

Blender 5.0.1 的最小作者流程、真实 Typhoea 源图和正常 package 回归均通过。真实源图确认复制后的新增组有
249 个字段、9 条 F-Curve、9 个中文自定义曲线属性和原组碰撞集合；高级曲线字段修改能够进入 Action。正常
package 仍导入 62 个 Mesh、1 个共享 Rig、11 个原生组、27 个碰撞体和 192 个物理相关 Transform。插件发现、
作者状态测试和 ZIP 内三轮注册/卸载也通过；文档 Python 测试 15 项通过，2 项因当前命令行未配置 MSVC 而跳过。

七个运行文件已同步到 `E:\vscode\EIEM_Blender` 和 Blender 5.0 插件目录。安装包
`bin/EIEM_Blender-0.25.0-unified-physics-ui.zip` 为 104214 字节，SHA256：
`95D119309DDA9B5C1F665CF255E5B3B424A883A8812630DDBD95D6DD8921CC93`。BlenderMCP 已热重载当前会话；活动
对象仍为 `maid.002 Skirt Physics`，其 249 项字段、5 个碰撞体引用、9 条曲线和 9 个自定义曲线属性均保留，
角度限制预览生成 12 个锥体。文件仍为
`C:\Users\25487\AppData\Local\Temp\18324_autosave_27152_autosave_31644_autosave.blend`，重载前后的未保存状态
均为 true，本轮没有自动保存。

本节没有修改 DLL、写入游戏目录、部署 DLL 或进行游戏内 Physics 验收；源碰撞体转换仍属于后续 DLL 阶段。
