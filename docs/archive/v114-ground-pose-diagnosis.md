# v114/v115 实机诊断记录：cloth_01 部件躺地上

本轮记录两次实机复现（v114 构建，2026-09-14）的结果。范围仅限本文件明确写出的实测证据；
推断与未知分开标注。这不是验收结论。

## 用户可见现象

- 只有 `MeshS_actor_typhoea_cloth_01_lod0_2`（`neiyi`）躺地上，其他部件正常。
- 同一 DLL 连续启动两次游戏：第一次无异常，第二次该 Mesh 躺地上。**随机**。

## 实测事实

### 三个场景实例，两条原生装配路径

单次 session 内场景里有三个 Typhoea 模型实例：

| 实例 | source Renderer | 模型 | 归属 | 装配路径 |
|---|---|---|---|---|
| 1 | `0FC9172000` / `0FC9172020` | `0FC91720A0` | `ownerKind=PrefabProxy` + `BaseModelPart`，`instanceUid=19185`，path=`.../chr_0034_typhoea_postmodel` | **BaseModelViewPart** |
| 2 | `0FC8A90B40` / `0FC8A90B60` | `0FC8A90C40` | owner `<unregistered>` | `NPCAvatarCreatorUtils.CreateSMSInfoForPostModel` |
| 3 | `1179C91BE0` / `1179C91C00` | 同上 | owner `<unregistered>` | `CreateSMSInfoForPostModel` |

实例 2、3 的 12 个 Partner 被成功并入 skin array：

```
[MOD-REG] stage=CreateSMSInfoForPostModel sourceCount=17 appended=12 newCount=29
```

实例 1 的 12 个 Partner **从未出现在任何 CreateSMS 数组里**，也从未得到 `MOD-REG`。
`0FC9172000` 作为 `skin-array-item` 出现次数：`CreateSMSGO` 0 次，`CreateSMSInfoForPostModel` 0 次。

`EiemRegisterPartnersInBaseModelArrays`（`src/il2cpp_trace.h`）是实例 1 应有的注册入口，
但 `[MOD-BASE-ARRAY]` 在整个 session 里出现 **0 次** —— 该函数在 8 个 `return false` 分支上
全部静默。v115 已为每个分支加上具名日志。

### 骨骼表长度不匹配（与"躺地上"直接相关）

`PARTNER-COMMIT-v102` 实测：

```
Part0  sourceBones=120  partnerBones=122  firstDiff=120
Part1  sourceBones=120  partnerBones=122  firstDiff=120
Part2  sourceBones=120  partnerBones=120  firstDiff=-1
Part3  sourceBones=120  partnerBones=120  firstDiff=-1
Part4  sourceBones=120  partnerBones=138  firstDiff=120
Part5  sourceBones=120  partnerBones=120  firstDiff=-1
Part6  sourceBones=120  partnerBones=122  firstDiff=120
Part7  sourceBones=120  partnerBones=122  firstDiff=120
Part8  sourceBones=120  partnerBones=120  firstDiff=-1
cloth_02 Part0/1/2  sourceBones=126  partnerBones=126  firstDiff=-1
```

`firstDiff` 处 source 侧为 `0000000000000000`（空槽）；partner 侧在 Part0/1/6/7 是
`Bip001_L_ForeTwist`，在 Part4 是 `EIEM_Bone_*`。

### ForeTwist 的来源

导出的 `.mesh` 文件里含 `ForeTwist` 的部件：

```
body_01_lod0_0            L=3 R=3
cloth_01_lod0_2   (Part0) L=1 R=1
cloth_01_lod0_2_2 (Part1) L=1 R=1
cloth_01_lod0_2_7 (Part6) L=1 R=1
cloth_01_lod0_2_8 (Part7) L=1 R=1
cloth_02_lod0_3*  (全部)   L=2 R=2
其余 Parts 2/3/4/5/9      L=0 R=0
```

Mod 自己的骨架文件含 `ForeTwist`：

```
SkeletonTyphoeaNewBoneV70.skeleton   ForeTwist=6
physics/.../skeleton.skeleton        ForeTwist=6
PhysicsSkeleton....physics           ForeTwist=0
```

`partner-skin-map` 显示 122 路径的 Partner 是 `private=0 unresolved=0 identityMismatch=0`
—— 即它们经 **Mod 自己的骨架**（`skeletonNodes=415`）全部解析成功，没有生成私有节点。
只有 138 路径的 Part4 报 `private=18`。

## v117 性能回归（已回滚，v118 移除）

v117 为每个 model 边界新增了 `EiemTraceModelPartnerOwnership`，对 12 个 Partner
各做一次 source 与 partner 的父级链遍历。它用 `EiemRegistrationTraceFirst` 去重，
但该函数（`src/eiem_registration_trace.h:44`）是**环形缓冲**
（`s_next++ % _countof(slots)`，见第 58-59 行）。一次加载里模型数量上千，
槽位被挤掉后同一个 `(model, stage)` 会被**反复判为首次**，于是层级遍历反复执行
（实测该次启动 `MOD-PARTNER-OWNERSHIP` 160 条、`MOD-MODEL-PART` 157 条），
游戏在加载页面卡死。

**结论**：任何"以 model 为键、在一次 session 内可能上千个"的探针，
都不能依赖 `EiemRegistrationTraceFirst` 做去重。v118 已完全移除该探针。
运行时基线回到 v116 的诊断集合。

### 卡死那一次留下的观察（来自未完成加载，证据强度弱）

12 个 Partner 已创建（`PARTNER-COMMIT-v102` 12 条），但在 124 个已到达的装配边界上
`PARTNER-OWNERSHIP` 全部是 `sourcesUnderModel=0 partnersUnderModel=0` ——
即**没有任何一个 Partner 被挂在任何已完成装配的模型下**。
由于该次启动未完成加载，此观察不能作为结论，仅记录为待复现项：
**Partner 的 GameObject 是否真的被 parent 到对应模型的层级里。**

## 现行结论（v116 待验证）

用户补充：**除 `MeshS_actor_typhoea_body_01_lod0_0` 外，其余部件都躺过地上。**
body 是唯一一个走"直接替换源网格"的部件（`mesh=` + 有 `physics=`），
其余 12 个部件全部是 Partner（`partner.N=...`，没有 `mesh=`）。

因此这不是"某个 Mesh 的问题"，而是**类别差异**：源网格替换正常，Partner 全都不正常。
随机性只决定当前哪个实例、哪个被按键选中的部件可见。

### 机制

`RootBoneInfo` 是 16 字节值类型
`{ String rootBoneName; Int32 rootBoneID; Boolean deferEnableRestore; Boolean rendererEnabled }`。
游戏通过 `NPCAvatarCreatorUtils._FindTransformByBoneID(Animator, Int32, ...)`
把 `rootBoneID` 解析成 `SMR.rootBone`；`GetBonePath` / `GetBoneShortName`
以 `NPCAvatarTempletAssetsSO`（`List<String> bonePathsStr` + `List<BoneIdxList> bonePaths`）
为索引来源。即 `rootBoneID` 是**角色骨架全局骨表**里的编号，不是 Mesh 局部调色盘下标。

- 源网格替换：渲染器仍是游戏自己的那个，`RootBoneInfo` 由游戏按**该部件的源网格**填好 → 正确。
- Partner：`EiemCopyExpandedManagedArray`（`src/il2cpp_trace.h:1412`）
  把 **source 槽的 `RootBoneInfo` 字节** memcpy 给 Partner 槽。
  插件对 `rootBoneName`/`rootBoneID`/`deferEnableRestore`/`rendererEnabled`
  的读取次数为 **0**，所以这个身份从未按 Partner 自己的网格校验过。
  当 Partner 的骨表与 source 不同（本 mod 的 12 个部件全部声明了
  `skeleton=Skeletonchr_0034_typhoea_postmodel_0`），
  `_FindTransformByBoneID` 就可能解析到另一根骨骼，`SMR.rootBone` 随之落在错误的骨上
  —— 整体姿态跑偏，表现为躺地。

### v116 诊断

新增 `[MOD-ROOTBONE-INFO]`，对每个被追加的 Partner 同时记录
source 槽与 Partner 槽的 `sourceName/sourceID/sourceDefer/sourceEnabled`、
`partnerName/partnerID/partnerDefer/partnerEnabled`，
以及实际 `smrRootBone` 指针与其对象名。

判读方式：

- `sourceID != partnerID` 或 `sourceName != partnerName` → 复制确实产生了错误的 root 身份。
- `smrRootBone` 指向非本部件应有的根骨（或为 null）→ 机制成立，直接改成按 Partner
  自己的 Mesh 骨表推导 `RootBoneInfo`。
- 两者一致且 `smrRootBone` 正确 → 该假设也被推翻，转向骨骼调色盘/父级层级方向。

## 被推翻的假设：索引错位

用户给出反例：`MeshS_actor_typhoea_body_01_lod0_0` 用的骨骼比它挂载的源模型多，
但**从未躺地上**。直接读取导出文件的骨路径表后确认：

```
body_01_lod0_0.mesh       bonePaths= 76 （含全部 4 个 ForeTwist：索引 6/8/26/27）
cloth_01_lod0_2.mesh      bonePaths=122 （ForeTwist 在 120/121）
cloth_01_lod0_2_2.mesh    bonePaths=122 （ForeTwist 在 120/121）
cloth_01_lod0_2_7.mesh    bonePaths=122 （ForeTwist 在 120/121）
cloth_01_lod0_2_3.mesh    bonePaths=120 （无 ForeTwist）
cloth_01_lod0_2_9.mesh    bonePaths=120 （无 ForeTwist）
cloth_02_lod0_3.mesh      bonePaths=126 （ForeTwist1 变体在 65/112）
```

因此"路径数多于 source 槽数"**单独不足以**导致躺地：body 的 76 项全部属于
原生骨集合，而 cloth 的 122 项尾部 2 项是同一骨集合之外的条目。
"索引错位导致整体塌陷"这一解释被 body 反例推翻，**不作为现行假设**。

## 现行假设（未证实）

1. `RootBoneInfo` 身份被整体复制。实测 `EiemCopyExpandedManagedArray`
   （`src/il2cpp_trace.h:1412`）把 **source 槽的 `RootBoneInfo` 字节** memcpy 给
   每个追加的 Partner 槽；全仓库对 `rootBoneName` / `rootBoneID` /
   `deferEnableRestore` / `rendererEnabled` 的**读取次数为 0**（唯一匹配是
   无关的 `EiemReadRendererEnabled`）。也就是 Partner 的 root bone 身份是**继承**
   而非由自己的 Mesh 推导。若该 ID 在 Partner 自己的骨表里指向另一根骨骼，
   `SMR.rootBone` 就会落在错误的骨上，整体姿态随之跑偏 —— 与"整体塌到地面"形状相符。
2. 为什么只有最小、且**只有它**被报告，以及随机性来自哪个边界，仍未确定。
   注意 Part0/1/6/7 的文件级特征（bonePaths=122、ForeTwist 在 120/121）完全相同，
   仅顶点数与文件大小不同，所以"文件特征"不足以区分。

## 未知

- `Bip001_L_ForeTwist` 到底是否存在于游戏原生层级（需要逐槽路径比对或层级查询）。
- 为什么只有 `cloth_01_lod0_2`（Part0，131 KB，最小）塌陷，而同样 122 路径的
  Part1/6/7（685/976/217 KB）没有报告异常。两者 `pathCount`、`private`、
  `firstDiff` 完全相同，仅文件不同。
- 实例 1（PostModel）为何完全没被 `EiemRegisterPartnersInBaseModelArrays` 处理；
  v115 的 `[MOD-BASE-ARRAY-FAIL]` 会给出具体 reject 原因。

## 已做的诊断改动（v114 / v115）

- `src/eiem_assembly_probe.h`（v114 新增）：hook `EntityRenderHelper._ValidRenderers`
  与 `InitAll`。实测 `InitAll` 全程 0 次调用；`_ValidRenderers` 仅 1 次且 `count=0`。
- `src/il2cpp_trace.h`（v114）：`EiemRegisterPartnersInSkinArrays` 的静默 `continue`
  改为 `[MOD-REG-GAP]`；扩容失败改为 `[MOD-REG-FAIL]`。
- `src/il2cpp_trace.h`（v115）：`EiemRegisterPartnersInBaseModelArrays` 的 8 个
  `return false` 全部改为具名 `[MOD-BASE-ARRAY-FAIL] reason=...`，并加
  `[MOD-BASE-ARRAY-PROBE]` 计数器转储。

## 下一步要求

1. 跑 v115，取得 `[MOD-BASE-ARRAY-FAIL]` / `[MOD-BASE-ARRAY-PROBE]`。
2. 新增"逐槽路径比对"记录：把 Partner mesh 的骨骼路径表与 source Renderer 的
   骨骼路径表按索引对齐，输出第一个不一致的索引和两侧路径名。这是区分
   "骨骼集合不同"与"顺序不同"的唯一直接证据。
3. 修掉静默失败后，再决定是约束导出（不导出原生不存在的骨骼）还是
   在运行时按路径重排/补齐调色盘。
