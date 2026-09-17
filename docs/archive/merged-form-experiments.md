# 合并形态（上游替换）实验记录

记录 2026-09-14 ~ 09-15 这一轮"把 Partner 换成上游替换"的调查与实验。
本文的目的不是描述现状，而是**避免重复推导已经被推翻的做法**，并记录几个
会让诊断静默失效的陷阱。

相关文档：
- `model-assembly-chain.md` —— 三端装配链路与改动点取舍
- `merged-part-renderer-boundaries.md` —— 源 asset 作为分组单位的硬约束
  （注意：该文第 28 行"已确认不再躺地"是**更早一次导出**的结论，现已失效）

---

## 一、Hook 结论

### 1.1 mesh 替换挂在哪

`TraceSkinnedMeshSetSharedMesh`（`src/il2cpp_trace.h:7980`）：

```cpp
TraceRememberMeshObservation(self, mesh, "SkinnedMeshRenderer");
void *sourceMesh = mesh;
void *retained = EiemReplacementForSourceMesh(self, sourceMesh);
if (retained) mesh = retained;
if (original) original(self, mesh, methodInfo);          // 游戏拿到的是替换后的
if (!retained && sourceMesh)
    EiemApplyStandaloneRenderRulesToRenderer(...);       // 规则在这里求值
```

**这是在"游戏把源 mesh 赋给渲染器"的那一刻替换，且在 `original()` 之前**，
所以游戏从未见过原 mesh。对静态 Prefab（玩家角色）等价于"实例化时替换"。

**这是资源赋值层，不是装配层**，因此与呈现路径无关：大世界 / UI / NPC 都要给
SkinnedMeshRenderer 赋 sharedMesh，都要过这个 hook。这一点比 Partner 模式
（要分别 hook `OnLoadFinish` / `CreateSMSGO` / UI 生命周期）根本不同。

### 1.2 装配链的四个容器，插件只写了三个

`BaseModelViewPart` 上游戏收集自己的渲染器的字段：

| 字段 | 偏移 | 插件写入 |
|---|---|---|
| `m_renderers` | 0x88 | ✅ `EiemRegisterPartnersInBaseModelArrays` |
| `m_meshes` | 0xA8 | ✅ 同上 |
| `m_lodGroups` | 0xC0 | ✅ `EiemSetPartnerLodMembership` |
| `m_boneCloths` | 0xB0 | ❌ **从未写入** |

`CharUIModelMono.m_boneCloths`（0xE8）同样从未写入。

`s_basePartBoneClothsOffset` 在整个 `src/` 里**只出现在第 255 行的声明**，
连查找字段的列表都没有它。

**结论**：Partner 模式模仿了游戏装配渲染器的四个容器中的三个。第四个
`m_boneCloths` 里的 `BeyondBoneCloth.ClothData` 持有
`baseSkinningDataList` / `baseSkinningBindPoseList`，是蒙皮/布料驱动的入口。
这是"数据全对但渲染成 T-pose"的结构性解释。

### 1.3 物理也是自建的一套，没接入游戏的装配

`eiem_native_physics_runtime.h`：

```cpp
389: EiemPhysicsRuntimeNewHost(...)
399:   void *host = il2cpp_object_new(api.gameObjectClass);     // 自建 GameObject
401:   const std::string name = "EIEM_Physics_" + generation;   // 自命名
415:   set_parent(transform, modelTransform, keepWorld=false);
935:   addComponent(host, clothClass);                          // 自建 BeyondBoneCloth
```

`EIEM_Physics_N` 与 Partner 的 `EIEM Partner Render*` 是同一个模式。**所以网格
和物理是同一个病，不是两个问题**：都绕过了游戏的装配链。

### 1.4 骨架解析的不对称

`EiemAcquireSkeleton`（`eiem_skeleton_runtime.h:489-495`）：

```cpp
std::unordered_set<std::string> virtualPaths;
if (rule.hasPhysics)                                  // ← 只有这条分支
    for (const auto &node : document.nodes)
        if (node.source) virtualPaths.insert(node.path);
```

`virtualPaths` **只在 `rule.hasPhysics` 时构建**。它决定
`EiemSkeletonSourceNodes:343` 遇到解析不到的 `source=true` 节点时是
`continue`（放过）还是 `return false`（整条链中止）：

```cpp
if (!resolved) {
    if (i != 0 && virtualPaths && virtualPaths->count(node.path)) continue;
    error = "Source bone is missing (not a new bone): " + node.path;
    return false;
}
```

**即 `physics=` 不只是"加物理"，它换掉了整套骨架解析策略。**

### 1.5 解析不到的节点会被换成空 GameObject

`EiemAcquireSkeletonDocument:426`：

```cpp
if (!node.source || !source[i]) {
    void *object = il2cpp_object_new(g_gameObjectClass);   // 空 GameObject
    ...name = "EIEM_Bone_" + serial...
    set_parent(transform, instance->nodes[node.parent].Target(), keepWorld=false);
}
```

这些私有对象不在 Animator 的 avatar 层级里，**除非物理按路径找到它们，否则没有
任何东西驱动**。`eiem_skeleton_runtime.h:245-252` 的注释自己写出了后果：

> "avoids a static private bone putting the **whole weighted section in a bind/T-pose**"

那个防护只在"节点在 SMR 调色板祖先链里、只是不在 Transform 子节点遍历里"这一种
情况生效，其余全部私有化。

### 1.6 只有源渲染器受蒙皮保护

`EiemPreserveSourceSkinning`（`il2cpp_trace.h:1898`）只作用于
`s_eiemOverrides` 里的渲染器，也就是**就地替换网格的源渲染器**：

```cpp
// TraceSkinnedMeshSetBones:2148
const size_t index = EiemFindOverrideLocked(self);
if (index != SIZE_MAX && ...) { tracked = true; binding = ...; }
if (tracked && binding && ...) EiemPreserveSourceSkinning(self, expected, ...);
```

**Partner 渲染器不在 `s_eiemOverrides` 里**（那是源渲染器的表），所以游戏对
Partner 调 `set_bones` 时插件不做任何修正。

### 1.7 运行时构建骨骼数组的顺序

`EiemSkeletonMeshBones`（`eiem_skeleton_runtime.h:505`）按 **mesh 自己的
`bonePaths` 顺序**逐个解析到骨架节点：

```cpp
for (const auto &node : instance.document.nodes) paths.push_back(node.path);
EiemResolveSkinPathIndices(skin.paths, paths, indices, error);   // skin.paths 的顺序
for (auto i : indices) values.push_back(instance.nodes[i].Target());
```

所以**渲染器的 `bones` 数组长度可以任意**，等于该 mesh 的 `bonePaths` 数。
"游戏 cloth_01 是 120 根所以你的 mesh 必须 120 根"是**错的**——Unity 没有这个
约束，游戏自己同一个骨架上有 `Bones=1 / 2 / 7 / 9 / 120 / 126` 的各种 mesh。

**唯一的要求是：mesh 的每条 `bonePath` 都能在它绑的那个骨架里解析到。**

---

## 二、各版本实验

### 版本 A：Partner 形态（十二个 partner）

```ini
[RenderS_actor_typhoea_cloth_01_lod0_2]
asset=S_actor_typhoea_cloth_01_lod0
handling=skip                       ; 关掉游戏自己的渲染器
if $switch_xxx == 0
    partner.0=...Part0
endif
...共 9 个 partner（cloth_01）+ 3 个（cloth_02）
```

每个 Part 段带 `mesh=` / `skeleton=` / `physics=` / `material.0=`。

**实测结论**：
- 12 个 partner 全部创建成功、全部注册进游戏皮肤数组、全部摆位正确
- 每次 skinning 采样 `nullBones=0 oob=0 degenPose=0 unweighted=0`
- 但**随机有 mesh 渲染成 T-pose（躺地）**
- 两次结构完全相同的运行（探针条数、注册索引、骨骼数全相同）结果不同

**注册缺口与"随机"的关系**：`EiemRegisterPartnersInSkinArrays:1496` 要求
partner 的**源渲染器必须在那一次装配的 renderer 数组里**，否则 `continue`
跳过。而游戏装配时构造的数组内容随启动不同，所以同一个 partner 有时进得去
有时进不去。

### 版本 B：注释 `physics=` + 剥离骨架（失败，被我误判过）

剥离骨架里的 20 个 `Magica Capsule Collider` 节点后，整份 ini 被拒：

```
PARSE FAILED: PhysicsSkeleton...: Physics collider 0993f9a2... references
              missing Skeleton bone: .../Magica Capsule Collider (Bip001_R_Thigh)
```

**根因不是"骨架被剥离"，而是"声明了物理资源"**。
`EiemPrepareModPhysics`（`eiem_mod_document.h:743`）**遍历所有声明的 Physics
资源无条件加载**，在任何规则被求值之前：

```cpp
for (auto &resource : doc.resources) if (EiemModEquals(resource.kind, "Physics")) {
    ...
    else if (!EiemLoadPhysicsAsset(path, asset, error)) { error = ...; return false; }
}
```

所以只注释规则里的 `physics=` **完全无效**；必须把
`[PhysicsSkeleton...]` **资源段本身**注释掉。

### 版本 C：Merged 形态（两个合并 mesh）

```ini
[RenderS_actor_typhoea_cloth_01_lod0_2]
asset=S_actor_typhoea_cloth_01_lod0
mesh=MeshCloth01Merged
submesh.0=0 ... submesh.7=7
material.0=... ... material.7=...
```

**无 `handling=skip`、无 `partner.N`**，游戏自己的渲染器保持启用并就地换成
合并 mesh。

| 分组 | 部件 | 结果 |
|---|---|---|
| cloth_01 | 9 个 → 首次尝试 | ❌ 整条替换被拒（见下） |
| cloth_01 | **8 个**（排除 `_5` 裙子） | ✅ palette 122，全部游戏骨骼 |
| cloth_02 | 3 个 | ✅ palette 126，全部游戏骨骼 |

**做成 8 个的原因**：第 9 个是 `MeshS_actor_typhoea_cloth_01_lod0_2_5`，它是
唯一带那 18 根 `maid_skirt_*` 的 mesh，而活体骨架里裙摆叫 `skirt_base_*`。
一个解析不到的骨骼路径会让**整个 mesh 赋值失败**：

```
resource mesh replacement failed: asset=S_actor_typhoea_cloth_01_lod0
    mesh=MeshCloth01Merged
    error=Skeleton bone path not found:
          .../Bip001_Spine1/maid_skirt_01_a_jnt
```

失败后渲染器保留游戏原 mesh，规则里所有材质/子网格映射**全部无效**——
表现为"合并完全没生效"。

### 版本 D：`skeleton=` 也注释掉

`EiemResolveMeshBones`（无骨架实例时的路径）对**活体骨架**做严格匹配，且它
构建的祖先映射来自 `SkinnedMeshRenderer.bones` 的父链，**不是完整的
Transform 层级**。所以描述文档里"游戏有"的骨骼不一定能解析到。

**这是 `skeleton=` 不能随便注释的原因**：注释后解析目标从"415 节点的骨架
文档"变成"活体骨架"，后者的路径集合不同。

---

## 三、已被推翻的假设（不要重复）

| 假设 | 推翻依据 |
|---|---|
| 那 20 个碰撞体节点是错的（游戏没有） | 游戏 prefab **确实有** 20 个 `Magica Capsule Collider` GameObject |
| 注释规则里的 `physics=` 就能禁用物理 | 物理资源是**声明即加载**，跟规则无关 |
| 裙摆骨骼是静态的所以顶点塌陷 | 它们由物理驱动（`[PHYSICS-RUNTIME] uniqueHits=18`） |
| 裙摆骨骼没有顶点权重、"纯拓扑流" | 实测 **11,000+ 顶点**绑在上面（`tools/mesh_bone_influence.py`） |
| `SKIN-DISPLACED` verdict 是故障 | 是探针假阳性：骨骼矩阵是世界空间、mesh 顶点是模型空间，差值就是角色世界坐标（~10301） |
| 坐标系基底标错导致躺地 | 13 个 mesh 全部标记 `unity-y-up-left-handed`，且运行时**没有任何旋转代码**（`coordinateSpace` 只读不用） |
| 注册时序随机 | 两次运行的注册**完全相同**（同 section、同数组、同 index） |
| "120 vs 138 骨骼数不匹配导致 T-pose" | Unity 无此约束（见 §1.7） |
| `added=52` 说明有第二套骨骼 | 主骨架 415 节点里 397 `source=true` + 18 `source=false`，52 的差额来源**仍未查清**，但不能用它推断 |

---

## 四、会让诊断静默失效的陷阱

### 4.1 `TraceTakeBudget` 是恒假存根 → 探针被编译器删除

```cpp
static bool TraceTakeBudget(volatile LONG *counter, LONG limit) {
  (void)counter; (void)limit;
  // Asset-path exploration is complete. ...
  return false;
}
```

`if (TraceTakeBudget(...))` 与 `&&` 组合后条件恒假，`/O2` 会把整个块当死代码删除。
症状极具欺骗性：

| 现象 | 误判 |
|---|---|
| 日志里 0 条探针输出 | "探针跑了但没找到目标" |
| dll 大小与上次**完全相同** | "改动太小看不出来" |
| 编译 0 错误 0 警告 | "编译成功了" |

**确诊方法**：往块里插一个独一无二的字符串，然后按**字节**搜索二进制
（`tools/binary_marker_check.py`）。注意 `/OPT:REF` 会删掉未被引用的数据，
所以标记必须放在**被调用的代码路径**里。

**`src/il2cpp_trace.h` 里还有 25 处** `if (!flag && TraceTakeBudget(...))` 形态的
守卫，全部是死代码、永远不会输出。`tests/test_dead_diagnostic_guard.py` 钉住了
这个事实。

### 4.2 别用跨行正则改 `mod.ini`

这个文件被我改坏过两次（一次段重复、一次删掉了 MERGED 资源段）。
`tools/apply_merge2_ini.py` 改成**解析 section 列表**，并且拒绝在还有
`partner.N` 引用时写盘。

### 4.3 日志计数会被去重和缺省字段污染

- `EiemRegistrationTraceFirst` 会按 (boundary, object, generation) 去重，
  所以某些事件的**真实调用次数远多于日志行数**
- 正则 `(\w+)=([^\s]+)` 会把**渲染器名字里的 "partner"** 当成 `partner=` 字段；
  缺省值是字面量 `0`，会被误当成一个指针
- 一行里可能有多个 `source=`，按行计数会虚高

### 4.4 托管数组的布局

```
偏移 +16 : 对象头之后
偏移 +24 : 元素个数 (int32)
偏移 +32 : 元素数据
```

`IL2CPP_ARRAY_DATA` = 0x20。

### 4.5 测量蒙皮不能看 `localBounds`

`SkinnedMeshRenderer.localBounds` 是**导出时定死的数据**，绑定时坏了它也不会动。
要看真实结果必须自己算：

```
skin(i) = bones[i].localToWorldMatrix * mesh.bindposes[i]
p'      = Σ(k) weight[k] * skin(boneIndex[k]) * p
```

数学在 `src/eiem_skin_math.h`（不依赖 Windows/IL2CPP，可独立编译），
由 `tests/test_skin_math.py` 覆盖，钉住了列主序 `m[col*4+row]`、
`p' = M*p` 的应用顺序、退化 bindpose 的行列式、以及权重和不为 1 时的归一化。

---

## 五、当前状态与未决问题

### 已部署（2026-09-15 00:54）

| 项 | 内容 |
|---|---|
| `MeshCloth01Merged.mesh` | 8 submesh（排除 `_5` 裙子），palette 122，全部游戏骨骼 |
| `MeshCloth02Merged.mesh` | 3 submesh，palette 126，全部游戏骨骼 |
| `mod.ini` | rules=4 / resources=28 / missing=0 / `partner.` 引用=0 |
| `skeleton=` / `physics=` | 全部注释（资源段 + 规则指令） |
| 旧 12 个 part mesh | 在 `meshes\fallback-parts\`，未删除 |

### 未决

1. **`_5` 裙子（含 18 根 `maid_skirt_*`）无法进入合并**。要么改绑游戏的
   `skirt_base_*`，要么单独用 `skeleton=` 提供那份 415 节点文档。前者更干净
   （改绑后所有 mesh 的 palette 都是纯游戏骨骼，物理也能回到游戏自己的链路）。
2. **按键切换在合并形态下不成立**。`submesh.N=M` 只做材质重映射，没有隐藏
   submesh 的能力；ini 的材质段只接受 `texture.*`，没有 `float.*` / `color.*`，
   做不出全透明材质。可行方案是为每个开关状态生成一个变体 mesh 走条件
   `mesh=`，或给运行时加"隐藏 submesh"的能力。
3. **`added=52` 的差额来源未查清**（文档 415 节点里只有 18 个 `source=false`）。
4. **UI / NPC 两条呈现路径未实测**。`mesh=` 在赋值层，理论上与路径无关，但
   UI 路径在插件里只有生命周期 hook，需要单独验证。
5. **body 渲染器出现 `visible=false`**（`[DEBUG-DRAW-STATE]`），原因未查。

### 可用的诊断工具

| 工具 | 用途 |
|---|---|
| `tools/binary_marker_check.py` | 按字节确认代码是否进了二进制 |
| `tools/mesh_palette_resolution.py` | mesh 的 bonePaths 能否在骨架里全部解析 |
| `tools/mesh_bone_influence.py` | 每根骨骼承载多少顶点（区分"没用到"和"没查对"） |
| `tools/mesh_bone_palette_diff.py` | 两个 mesh 的 palette 差集 |
| `tools/mesh_skin_compare.py` | 多 mesh 的骨骼/权重对照表 |
| `tools/mesh_section_identify.py` | 导出 mesh 对应哪个 Blender 对象 |
| `tools/eiem_container_markers.py` | 容器的坐标标记与 source asset |
| `tools/run_sequence_diff.py` | 两次运行的事件序列对比（指针归一化） |
| `tools/partner_array_diff.py` | 两次运行的 partner 注册差异 |
| `tools/skin_probe_report.py` | `[SKIN-PROBE]` 结果的 verdict 分布 |
| `tools/comment_physics_decl.py` | 注释 mod.ini 里的 Physics 资源声明 |
| `tools/apply_merge2_ini.py` | 改写 mod.ini 为合并形态（按 section，非正则） |
| `tools/merge_eiem_mesh.py` / `verify_eiem_merge.py` | 合并 / 无损反解验证 |
| `tools/inspect_eiem_skeleton.py` | 骨架节点、tree、source 标记 |
