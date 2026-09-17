# 资源级替换重构设计

状态：设计基线，尚未作为生产实现完成。

本文档冻结 DLL 重构的目标、边界、验证顺序和验收标准。除非新的运行时证据推翻某项结论，否则后续实现必须遵循本文档。文档把事实、设计决定和待验证假设分开记录，避免把研究性 Hook 当成生产契约。

## 1. 核心需求

### 1.0 本轮验证范围

本轮只验证“资源级替换架构”是否可行：资源身份解析、原生装配输入、原生可见性输入、统一 Mod 状态和 F10 资源代际。此前 Partner/下游重绑路径造成的随机躺地、T-pose 等历史故障不作为本轮结论，也不允许成为本轮验证的成功或失败依据；如果新架构重新出现异常，必须以新架构的运行时证据单独登记。

验证顺序必须从无 Mod、无写入 Hook 的游戏基线开始，再逐项打开一个能力。旧实现中名称带 `Trace` 的函数不自动视为观察 Hook，必须按实际是否改变游戏对象或资源来分类。

EIEM 要修改的是游戏资源输入，而不是接管游戏模型运行时。目标资源包括：

- Mesh、材质、贴图和材质参数；
- 后续可扩展的 Skeleton、Physics、碰撞体和物理参数；
- 按键切换资源部件的显隐；
- F10 热重载已经发布的 Mod，并影响当时已经存在的模型实例。

替换必须按资源身份命中，而不能硬编码某个角色、某个 PFB 或某个场景。NPC、角色 UI 和大世界虽然可以由不同 PFB 或不同 owner 创建，但只要它们消费同一资源身份，就必须得到同一 Mod 规则和同一 Mod 状态。

游戏负责以下运行时职责：

- 创建和销毁 GameObject、Renderer、Animator、LOD 和 Physics；
- 将 Mesh 与游戏骨架、动画和物理系统装配；
- 根据距离和平台状态决定 LOD 与原生可见性；
- 维护实例生命周期和异步 Job 的完成顺序。

DLL 不应建立第二套模型生命周期。

## 2. 已验证事实

### 2.1 三条实例装配链存在

当前运行时已观察到三类入口：

| 场景 | 已观测入口 | 结论 |
|---|---|---|
| 大世界 | `BaseModelViewPart` | 有独立的模型 owner、资源句柄和 LOD/物理缓存 |
| 角色 UI | `UIModelLoader`、`CharUIModelMono` | 有独立的 UI owner、可见性和卸载入口 |
| NPC | `NPCAvatarCreatorUtils.CreateSMSGO`、`CreateSMSInfoForPostModel`、`NPCAvatar.StartNPC` | 可能直接从部件表创建 Renderer，不要求经过 PFB 实例化 |

因此，“一个统一资源替换引擎”是目标；“一个 Hook 覆盖全部创建路径”尚未被证明。

### 2.2 资源身份与实例身份必须分离

同名 Mesh 不足以证明是同一个资源。运行时规则应优先使用已观测的资源身份：路径 Hash、GUID、资源描述中的 Mesh 指针和必要的来源关系。PFB 只记录资源关系和实例来源，不能成为 Mesh 规则的唯一身份。

### 2.3 动画与材质职责不同

Animator 和 Physics 写入骨骼 Transform；SkinnedMeshRenderer 使用 `bones`、bindpose 和顶点权重完成蒙皮；材质和贴图只影响绘制。材质切换本身不能修复或造成骨骼绑定。

因此，Mesh-only 替换必须保持 skin contract，不能在未验证时改写 `bones` 或 `rootBone`。

### 2.4 原生 Physics 确实存在，但三端能力未统一证明

历史运行记录已经观察到大世界和部分 NPC 的 `BeyondBoneCloth`、`ClothProcess`、Team 与 Animator 数据链路。UI 实例存在没有原生 Cloth 的记录，可能是预览模式、主动禁用或另一种装配分支。不能把“有裙子”或“有动画”当成实时 Physics 的证据。

Physics 资源描述独立于 `SubMeshInfo`，新增 Physics 必须进入游戏原生装配会消费的描述对象，不能由 DLL 通过 `AddComponent` 建立独立 Team。

### 2.5 当前实现的已知危险点

现有 `EiemApplyResolvedRenderRule` 仍可能在模型已装配后：

- 写入 `sharedMesh`；
- 解析并写入 `SkinnedMeshRenderer.bones`；
- 跟踪或改写 `rootBone`；
- 创建或维护 Partner；
- 在 F10 中恢复旧覆盖，再重新播放资源规则。

当前 `submesh_visible` 按键路径也会重新进入资源规则应用。它在语义上应该只改变显隐，但现实现仍可能再次触发 Mesh/skin 相关逻辑。这是 T-pose、躺地和热重载后状态污染的重点嫌疑，必须在重构中拆除。

### 2.6 当前证据对重构可行性的判断

当前证据可以支持验证计划，但还不足以支持直接实现最终架构：

- **资源级命中：部分可行。** 运行时已经能在 `RendererInfo._Init`、三端 owner 和资源身份记录之间建立关联，说明可以继续追踪资源输入；但尚未证明存在一个覆盖三端的单一 Resolver。
- **原生装配前替换：待验证。** 现实现仍在 `PostDealLoadedModel`、`CreateSMSGO` 和 `set_sharedMesh` 等下游边界执行规则。必须先通过 V1 找到资源成为游戏装配输入的边界，再决定修改点。
- **按键显隐：方向可行，入口未定。** Mod 状态和实例共享关系已有基础，但 `SubMeshInfo.isActive`、VisibleController 或其他原生输入哪个真正控制绘制，必须由 V2 的单变量实验确认。
- **F10 热重载：协议可设计，原生刷新未证实。** 代际与旧资源退休规则可以先定义；在找到原生刷新完成栅栏前，不能把现有逐 Renderer 重放路径当作实现。
- **Skeleton/Physics：本轮不判定。** 三端是否消费同一种 Physics 描述，以及新增对象的原生工厂入口，留到 V6 单独取证。

因此，本计划的“可行”含义是：每个未知点都有隔离的验证阶段和失败回退，不是当前代码已经满足目标架构。任何阶段无法取得对应证据，都停止扩展能力，不用下游补丁替代缺失入口。

## 3. 已确定的设计方案

### 3.1 统一资源替换引擎

三端共享以下逻辑：

```text
Mod 文档
  -> 不可变 ModProgram
  -> 资源身份规则与资源代际
  -> 统一 Resolver / Cache
  -> 游戏原生装配
```

三端只保留轻量适配器：

```text
World owner adapter
UI owner adapter
NPC owner adapter
```

适配器只负责发现实例、传递原生资源输入、请求原生刷新和报告生命周期；不能各自实现 Mesh、材质、Partner 或 Physics 规则。

### 3.2 按键切换方案

按键状态属于单个 Mod，同一 Mod 的所有实例共享该状态。按键只生成一个可见性状态变化：

```text
按键事件
  -> 更新 Mod 状态
  -> 计算原生部件/submesh 可见性
  -> 调用原生可见性输入或可见性刷新
```

按键路径禁止：

- 修改 `sharedMesh`；
- 修改 `bones` 或 `rootBone`；
- 创建或销毁 Renderer、Partner、Animator、Physics；
- 触发完整 F10 资源代际切换。

优先级：

1. 游戏原生 `isActive`、VisibleController 或 submesh 可见性输入；
2. 游戏已有的部件刷新函数；
3. 最后才使用同一 Mesh 的隐藏索引变体。

隐藏索引变体必须保留顶点、骨骼、bindpose、Renderer 和 LOD 数据，只把目标 submesh 的索引数量置零。切换前后必须验证 `Mesh` 以外的 skin 和 Physics 引用未变化。

材质透明、材质替换和 Shader 参数不作为默认显隐机制，因为它们仍可能参与深度、阴影和排序。

### 3.3 F10 热重载方案

F10 使用一套统一的资源代际协议，不能为 Mesh、材质、贴图和 Physics 各写一套生命周期。

```text
读取并校验新 Mod
  -> 创建新的不可变 ResourceGeneration
  -> 让 Resolver/Cache 对新请求返回新代际
  -> 请求游戏原生模型刷新或原生工厂重装配
  -> 游戏创建/绑定新的 Renderer、Animator、LOD、Physics
  -> 原生完成回调到达
  -> DLL 观察并登记新实例
  -> 旧代际在游戏释放后退休
```

首选实现是游戏原生的模型实例双缓冲刷新：游戏先完成新实例，再切换 owner 并通过自己的生命周期释放旧实例。DLL 不自行创建或销毁这些对象。

只修改缓存不够，因为已经存在的 Renderer 可能直接持有旧 Unity 对象。直接修改共享缓存对象也会污染其他实例。

若暂时找不到原生刷新入口，不能静默退回逐 Renderer 重绑。安全行为是将新代际标记为待应用，等待下一次游戏原生卸载/重装配；整图重载只作为验证手段，不能作为常规 F10 实现。

F10 禁止使用当前的全局“恢复全部旧覆盖 -> 逐实例重放 -> 重新收集 Physics”路径作为最终设计。删除规则时只处理受影响的原生资源关系，并保持游戏原生 owner 的生命周期。

### 3.4 Mesh、材质和贴图

- Mesh 替换在原生装配消费资源之前完成；游戏负责后续 skin、LOD 和动画绑定。
- 材质由资源 Resolver 生成或替换；未声明的材质槽保留游戏原值。
- 贴图和材质参数属于材质资源代际，不直接修改其他 Mod 或游戏共享对象。
- 同一资源身份的所有实例消费同一 Mod 规则，但每个实例仍由游戏决定是否 active、处于哪个 LOD 和何时释放。
- Partner 不作为基本 Mesh 替换机制。多个视觉部件优先使用一个合并 Mesh 的 submesh；额外 Renderer 只有在明确声明且能进入游戏原生生命周期时才允许使用。

### 3.5 Skeleton、Physics 和碰撞体

当前重构先保证 Mesh-only 路径不触碰骨骼和物理。新增骨骼、Physics 或碰撞体必须经过单独取证：

```text
资源描述
  -> 游戏原生工厂
  -> 游戏原生 BoneCloth / Collider / Team
  -> Animator 与 Physics 的原生更新和释放
```

在三端都确认原生消费入口前，不实现生产级新增 Physics。隐藏 Mesh 不等于删除 Physics；原生系统仍然引用节点时必须由游戏决定其停用和释放。

## 4. 仍需验证的关键问题

### 4.1 资源 Resolver 的共同入口

需要定位 Mesh、材质和贴图在进入三条装配链前的真实读取点，确认是否存在共同的资源描述或缓存入口。历史上 `AssetBundle.LoadAsset` 和 `FAssetProxyHandle.GetAssetProxy` 曾出现零调用，说明不能凭命名猜测入口。

验收证据：同一资源命中时记录 path hash/GUID、描述对象指针、最终 Renderer 指针，证明替换发生在原生装配消费之前。

### 4.2 原生模型刷新入口

需要验证以下能力：

- 是否能按模型 owner 请求资源刷新；
- 是否会走与地图/模型自然重载相同的工厂路径；
- 是否有完成回调或可证明的完成栅栏；
- NPC、UI、大世界是否都能通过同一协议请求，而不需要 DLL 重建对象。

没有完成栅栏就不能在 F10 中释放旧代际。

### 4.3 原生可见性入口

需要验证 `SubMeshInfo.isActive`、`isRendererDisabled` 或 VisibleController 的实际写入点是否影响装配和绘制，并确认 LOD 切换时兄弟 Mesh 会自动遵循同一状态。

### 4.4 三端 Physics 能力矩阵

对同一角色分别记录大世界、UI、NPC 的：

```text
model owner
Animator
BeyondBoneCloth 数量
ClothProcess.Init / StartRuntimeBuild
Team 注册与释放
Physics active/enabled 状态
```

只有完成这张矩阵，才能决定哪些 Physics 描述能被三端共同消费。

### 4.5 多 Mod 冲突

需要确定多个 Mod 命中同一资源时的优先级、材质槽合并规则、submesh 显隐合并规则和资源代际提交顺序。禁止依赖文件系统枚举顺序。

## 4.6 验证配置与 Hook 策略

### 4.6.1 为什么必须关闭无关 Hook

Hook 不是被动日志。Detour 会改变调用时序、线程栈和原函数前后的可见状态；即使日志内容完整，只要 Hook 在原函数前后写入了 Mesh、Renderer、骨骼、Partner、Physics 或缓存，就已经改变了验证对象。因此“日志里找得到”不能替代关闭无关 Hook。

本轮使用三类 Hook：

| 类别 | 运行时要求 | 例子 | 是否进入 V0 基线 |
|---|---|---|---|
| 状态写入/替换 | 必须关闭；需要验证时一次只打开一个 | `EiemInstallPartTableTest`、`EiemApplyResolvedRenderRule`、`EiemApplyStandaloneRenderRules`、`SkinnedMeshRenderer.set_sharedMesh` 的替换分支、`set_bones`/`SetSMRRootBone` 的改写分支、Partner/Physics 创建与销毁、F10 恢复/重放 | 否 |
| 纯观察、原样透传 | 可保留；不得修改参数、返回值、缓存或调用顺序 | `PrefabInstantiateProxy` 生命周期、`UIModelLoader`/`CharUIModelMono` 生命周期、`BaseModelViewPart`/NPC 创建边界、资源身份读取 | 是，限流 |
| 高噪声观察 | 默认关闭；在指定阶段按目标资源临时打开 | 全量 `AssetBundle.LoadAsset*`、VFS 读写、逐 Renderer setter、逐帧 Transform/LOD 记录 | 否 |

“原样透传”必须满足：保存原参数，调用原函数一次，返回原返回值，不追加对象创建/销毁，不修改任何 Unity 或游戏托管字段；日志失败也不能改变控制流。凡是不满足的 Hook，归入第一类。

### 4.6.2 V0 的确定配置

- 部署目录的 `mod.ini` 先备份，再将 `[Constants]`、Mesh、Material、Texture、Render 和其他资源规则逐行注释；不删除原文。V0 不允许资源规则进入 Resolver。
- 关闭所有第一类 Hook，尤其是 `EiemInstallPartTableTest`。该 Hook 会直接把第一个 active `SubMeshInfo.isActive` 写成 `false`，不能用于基线。
- 保留三端 owner 的创建/释放边界、Renderer/Animator/LOD/Physics 的一次性摘要，以及资源身份读取；这些 Hook 只调用原函数并记录前后指针。
- 关闭全量资源加载和逐帧日志，只保留事务级日志：`owner-create/release`、`assembly-begin/end`、`reload-begin/end`、`visibility-change`、`generation-publish/retire`。
- 每次启动只记录一次 Hook 配置摘要，日志中必须能看出本次是 `V0-native-baseline`，避免把不同配置的日志混在一起。

### 4.6.3 V0 的退出条件

V0 不是为了证明替换成功，而是为了证明验证环境干净：

1. 大世界、角色 UI、NPC 冷启动、传送和自然卸载完成；
2. 没有 Mod 资源命中、没有 DLL 写入 Mesh/bones/rootBone/Renderer/Partner/Physics 的记录；
3. 每个 owner 的创建和释放成对，且没有未完成的 reload 事务；
4. 日志没有逐帧洪水，能在一次运行中定位每个 owner 和资源代际。

任一条件不满足，都先修正验证配置，不能进入 V1。

### 4.6.4 分阶段验证顺序

| 阶段 | 唯一变量 | 允许打开的 Hook/能力 | 通过条件 |
|---|---|---|---|
| V0 原生基线 | 无 Mod、无写入 | 低噪声 owner/装配/释放观察 | 三端生命周期稳定，日志干净 |
| V1 资源身份 | 只打开资源身份观察 | 目标 Mesh/材质/贴图 Resolver 入口观察 | 能证明资源在原生装配消费前被解析，未写 Renderer |
| V2 原生显隐 | 只改变一个部件可见性 | 原生 `isActive`/VisibleController/submesh 输入 | 按键只改变显隐，Mesh/bones/rootBone/Animator/Physics 指针不变 |
| V3 单资源代际 | 只替换一个材质或贴图 | 新代际 Resolver + 已验证原生刷新观察 | 现有实例使用新资源，旧代际按原生释放退休 |
| V4 Mesh 代际 | 只替换一个 skin contract 相同的 Mesh | 新代际 Resolver + 原生刷新 | 游戏完成骨骼/LOD/动画装配，DLL 不写 bones/rootBone |
| V5 三端一致性 | 同一资源同时出现在世界/UI/NPC | 三个 owner adapter | 三端命中同一规则和 Mod 状态，各自遵循原生生命周期 |
| V6 Skeleton/Physics | 新增一类原生描述 | 仅打开对应原生工厂观察 | 先证明三端消费入口，再实现创建/释放 |

每阶段只允许改变一个变量。失败时保留该阶段日志和配置摘要，回到上一阶段，不跨阶段打补丁。

## 5. 分阶段重构计划

### 阶段 0：冻结证据和清理研究 Hook

- 删除或隔离会主动修改 `isActive`、Renderer、骨骼或 Physics 的研究性 Hook。
- 保留资源身份、owner、Renderer、Animator、LOD、Physics 生命周期日志。
- 日志按一次装配、一次按键、一次 F10 事务分组，避免逐帧轮询。

本阶段的验证配置采用 4.6.2 的 `V0-native-baseline`，旧躺地问题不作为验收项；若出现新异常，仅记录其 owner、资源代际和触发事务。

完成标准：研究 Hook 不再改变游戏状态；每次状态变化都能追踪到一个明确的输入或原生回调。

### 阶段 1：统一资源模型和 Resolver

- 将 ModProgram、资源代际、资源身份和冲突优先级整理成独立模块。
- 找到共同 Resolver 或为三条原生装配链建立薄适配器。
- Mesh-only 规则只产生资源描述，不直接写 Renderer 骨骼。

完成标准：同一 Mesh 身份在世界、UI、NPC 三端得到同一规则；当前实例仍由游戏装配。

### 阶段 2：拆出纯按键可见性执行器

- 按键只更新 Mod 状态和原生可见性输入。
- 禁止按键路径进入 Mesh、bones、Partner 或 Physics 执行器。
- 验证 LOD、兄弟 Mesh 和多个实例共享状态。

完成标准：连续按键不会改变 Mesh/bones/rootBone/Animator/Physics 指针，不出现 T-pose 或躺地。

### 阶段 3：实现统一 F10 资源代际协议

- 实现配置校验、代际发布、Resolver 缓存隔离和旧代际退休。
- 接入已验证的原生模型刷新/双缓冲入口。
- 完成回调到达前不得释放旧资源。
- 删除当前 F10 的全局恢复和逐 Renderer 重放作为默认路径。

完成标准：Mesh、材质、贴图变化能影响已有实例；游戏原生负责重新装配；没有随机 T-pose、躺地、LOD 重叠或 Physics 泄漏。

### 阶段 4：材质、贴图和材质参数热更新

- 在统一代际协议下实现材质资源和贴图资源替换。
- 只更新声明的槽和参数，不污染游戏共享材质。
- 验证按键状态与材质资源代际互不覆盖。

### 阶段 5：新增 Skeleton、Physics、碰撞体

- 先完成三端 Physics 能力矩阵和原生工厂入口验证。
- 设计 Physics/Skeleton 描述格式与游戏原生字段的映射。
- 只通过游戏原生工厂创建和释放新增对象。
- 完成动画、物理、碰撞体和模型卸载的完整生命周期测试。

### 阶段 6：Blender 适配

- DLL 资源契约稳定后再调整 Blender 导出。
- 导出显式选择的 Mesh、材质和贴图依赖；未选择的源 Mesh 不生成规则。
- Skeleton、Physics、碰撞体导出等到阶段 5 的原生契约确定后再接入。

## 6. 验证矩阵

| 场景 | 操作 | 必须保持不变 | 必须变化 |
|---|---|---|---|
| 冷启动 | 进入世界/UI/NPC | 游戏原生生命周期、LOD、Animator、Physics | 命中资源使用 Mod 资源 |
| 按键 | 连续切换显隐 | Mesh、bones、rootBone、Animator、Physics | 目标部件可见性 |
| F10 材质 | 替换材质/贴图 | Mesh、bones、Physics | 材质和贴图 |
| F10 Mesh | 替换兼容 Mesh | 游戏 owner 和原生装配顺序 | Mesh 及其原生绑定结果 |
| F10 + 按键 | 先热重载再切换 | 代际、骨骼、Physics 生命周期 | 资源与显隐状态 |
| 卸载/传送 | 离开并返回场景 | 无残留引用、无卡死 | 新实例重新消费当前代际 |
| 多实例 | 世界、UI、NPC 同时存在 | 同一 Mod 状态一致 | 各自遵循原生 LOD/owner |

每次验收至少记录：资源身份、实例 owner、Renderer、Mesh、材质数组、bones 数组、rootBone、Animator、LOD、Physics owner、代际和事务阶段。

## 7. 明确禁止事项

- 不把 `sharedMesh` 事后重绑当作通用热重载方案。
- Mesh-only 替换不得主动写 `bones`、`rootBone` 或 Animator 数据。
- 不由 DLL 创建独立 Partner、BoneCloth、Team 或碰撞体来补齐游戏链路。
- 不用逐帧轮询维护 Physics 候选。
- 不用全局场景扫描代替 owner 生命周期。
- 不把日志、构建产物或历史版本号当成运行时能力证明。

## 8. 当前实现与文档的关系

当前 `src/il2cpp_trace.h`、`src/eiem_resource_backend.h` 和相关测试仍包含历史下游替换、Partner、Skeleton 和 Physics 试验代码。阶段 0 完成前，这些代码只能视为迁移对象，不能视为本文档的实现依据。

本文档确定的是目标架构和验证门槛，不宣称原生 Resolver、原生刷新入口或三端 Physics 已经全部接通。每个阶段完成后必须补充运行时证据，再进入下一阶段。

## 9. 本轮开始前需要的外部信息

静态设计不需要额外信息即可开始。进入 V0 实机验证前，只需要确认两件事：

1. 游戏当前未运行，或可以接受先备份并注释部署目录的 `mod.ini`；
2. 运行一次后，分别报告大世界、角色 UI、NPC、传送/卸载是否完成，以及日志中是否出现第一类 Hook 的写入记录。

不需要用户先判断某个 Hook 是否“看起来无关”；由 Hook 分类表和日志中的配置摘要决定是否进入下一阶段。

## 10. V0 准备记录

2026-09-15 已完成静态审查、V0 部署和一次冷启动运行：

- 部署的 `plugin/mods/typhoeus/mod.ini` 已全部注释，原文件备份为 `mod.ini.v0-baseline-20260915-194352.bak`。
- V0 构建关闭了启动时的元数据全量枚举和 `SubMeshInfo.isActive` 研究性变更 Hook，并输出 `[VALIDATION] mode=V0-native-baseline`。
- 本轮 DLL 已重新编译并部署为 `plugin/eiem.dll`，SHA-256 为 `D62A0F41CBBD3C0EA13EE4C242B32A6F03BEF71E22DCF0C31468D7589447E112`。
- 被替换前的 DLL 备份为 `eiem.dll.v0-before-20260915-195100.bak`，SHA-256 为 `2AB258AE250F34BF8282F8AFA9DCD80F0A31DF196A18C550E969B2E2DDC096F5`。
- 本轮日志确认 `[VALIDATION] CameraFade hook disabled`、`HumanPoseHandler/Trojan hooks disabled`、`Player/face/IK hooks disabled` 和 `Hotkey/animation/GUI/update workers disabled`；没有 `MOD-MESH`、资源替换、`SubMeshInfo.isActive` 写入或旧 MMD/姿态 Hook 记录。
- 空规则状态为 `0 Prefab declarations, 0 Render rules, 0 resources`，所有模型生命周期记录的 `applied=0`。本轮仍保留资源身份和 owner 边界观察 Hook，它们调用原函数并维护插件内部登记，不向游戏 Renderer、Mesh、bones、rootBone、Partner 或 Physics 提交变更。
- 日志共 8,053 行、约 1.4 MB；其中 `MOD-MODEL-PART` 486 条、VFS 片段捕获 32 条，说明日志限流尚未完成。这是下一步清理项，不影响本轮“无资源规则写入”的判定。
- 随后加入低噪声开关：关闭 PFB 逐 Renderer 扫描、VFS 二进制捕获和未命中的逐模型/LOD/Partner 数组细节；V1 需要时只打开独立的 `kEiemValidationIdentityProbe`，不重新打开旧动画或写入 Hook。低噪声 DLL 已部署，SHA-256 为 `16E46589C4AC2A859A0F7345FC5B9F43864B743B61B3B485A665F0E1FFA32827`。

V0 已通过，下一步进入 V1 资源身份观察：仍保持 `mod.ini` 全部注释，只保留目标 Mesh 的 path hash/GUID、描述对象、Renderer 和原生装配边界日志；不得写入 Mesh、材质、贴图、bones、rootBone 或显隐状态。V1 通过后再一次只打开一个资源替换能力。

## 11. V1 身份观察部署记录

2026-09-15 已在 V0 通过后打开唯一的 `kEiemValidationIdentityProbe`，其余 V0 开关保持不变：

- `mod.ini` 仍全部注释，资源规则数为 0；没有启用 Mesh、材质、贴图、bones、rootBone、Partner、Physics 或按键写入路径。
- 本地合同测试为 141 项通过、48 项因宿主环境跳过，`build.bat` 构建成功。
- V1 DLL 已部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，SHA-256 为 `D1DFEB38531136E82D214B9F742A7F0C42D991671EEA35704934C82C079A303A`，备份目录为 `backups/deploy_v1_identity_20260915_203934`。
- 下一次启动只用于记录同一资源在大世界、角色 UI、NPC 三条原生装配链的 path hash/GUID、资源描述对象、Renderer 指针、owner 和装配边界；不以日志数量或旧躺地问题作为 V1 结论。

## 12. V1 运行结果与 Resolver 缺口

V1 冷启动日志（`[BUILD] ... dll=Sep 15 2026 20:38:51`）确认：

- 大世界 `chr_0034_typhoea_postmodel.prefab` 和 UI `chr_0034_typhoea_uimodel.prefab` 的 `S_actor_typhoea_body_01_lod0`、`S_actor_typhoea_cloth_01_lod0` 使用相同的 Mesh 指针。这证明两条 PFB 实例链最终消费同一运行时 Mesh 对象，但还不是资源路径/GUID 证据。
- NPC 的 `CreateSMSGO`、`CreateSMSInfoForPostModel`、`AssignSkinGo` 和 `SetSMRRootBone` 边界均能观察到；本轮 NPC 实例是 `npc_1001_andrew_postmodel`，没有加载 Typhoea NPC，因此三端同一资源的结论仍缺一条实机记录。
- `AssetBundle.LoadAsset`、`FAssetProxyHandle.GetAssetProxy`、`BundleResourceManager._LoadAssetInternal`、`Asset._FinishWithAsset` 等当前观察 Hook 没有记录目标 Mesh 的实际调用。它们不能作为替换入口的依据；当前仍不知道 Mesh 是在哪个更早的缓存/描述对象边界解析出来的。
- `NPCAvatarCreatorUtils.CreateMeshAssetsInfoForPostModel.AssignSkin` Hook 仍失败，NPC post-model 的最终赋值边界尚未完整覆盖。
- 本轮没有 `MOD-MESH`、材质写入、骨骼写入、Partner 创建或 Physics 资源写入记录。

因此，当前核心缺口不是再增加 Renderer Hook，而是找到“资源描述对象/缓存 -> 原生装配输入”的真实 Resolver。下一轮只做一次性观察：

1. 优先验证 `BundleResourceManager.Load/LoadAsync/LoadSubAsset` 的 String、StringPathHash、GUIDProxy 重载，以及其内部 `BundleLoader.AssetProxy._LoadAssetInternal` 重载，确认哪一条返回目标 Mesh/Material/Texture 的描述对象。
2. 并行补齐 `CreateMeshAssetsInfoForPostModel.AssignSkin` 的正确签名，只用于证明 NPC post-model 是否消费同一描述对象。
3. 让大世界、UI、Typhoea NPC 同时记录 `resource identity -> descriptor pointer -> final Renderer` 的关联；在这条关联成立前，不实现资源替换。

## 13. 2026-09-15 重启后的取证结论

本次重启加载的是 `resource-runtime-submesh-hotreload-20260915`（DLL 编译时间
`Sep 15 2026 20:38:51`）。部署配置仍是空规则：`0 Prefab declarations`、`0 Render rules`、
`0 resources`。日志没有 `MOD-MESH`、`MOD-MATERIAL`、`PART-TABLE` 写入、Partner/Physics
写入或错误记录，因此这次运行没有改变游戏状态。

大世界和角色 UI 都成功记录了 Typhoea：

- `S_actor_typhoea_body_01_lod0` 的 Mesh 指针都是 `0000000FC4CAB640`；
- `S_actor_typhoea_cloth_01_lod0` 的 Mesh 指针都是 `0000000FC4CAB5E0`。

这证明两个 PFB 的最终 Renderer 消费了同一个运行时 Mesh 对象，但还没有证明它们通过同一个
Resolver 或同一个资源描述对象得到该指针。当前运行中的 NPC 是 `npc_1001_andrew_postmodel`，
没有 Typhoea NPC，因此 NPC 三端一致性仍未验证。

资源候选 Hook 目前只确认“安装成功”，没有记录目标 Mesh 的实际 Load/Resolve 事件；此外
`CreateMeshAssetsInfoForPostModel.AssignSkin` 仍安装失败，NPC post-model 的最终赋值边界不完整。
`EntityRenderHelper._ValidRenderers` 虽然已安装，但本次只得到一次 `count=0`，不能把它当作已经
捕获完整 Renderer 名册的证据；需要确认签名和调用时机。

本次世界端记录还显示同一 LOD Renderer 曾出现 `active=0/enabled=0`，随后恢复为
`active=1/enabled=1`。这只是原生 LOD/可见性基线，不是插件写入；它为 V2 单变量显隐实验提供了
可比较的状态字段。

## 14. 当前必须保留、补齐和禁止的 Hook

### 必须保留的观察边界

- 三端 owner 的创建、完成、可见性和释放：`PrefabInstantiateProxy`、`UIModelLoader`/
  `CharUIModelMono`、`BaseModelViewPart`、NPC 创建与释放边界。
- 原生装配的一次性边界：`EntityRenderHelper._InitRenderAndMaterial`、正确签名的
  `_ValidRenderers`、NPC 的 `CreateSMS*`、`AssignSkin` 和 `SetSMRRootBone`。
- 只读资源身份：`SubMeshInfo` 的 `mesh`、`meshPathHash`、`meshGuid`、`meshName`、材质路径哈希，
  以及 `NPCAvatarMeshAssetsSO`/`NPCAvatarLodMeshAssets` 的描述对象指针。
- 资源缓存候选：`BundleResourceManager` 的全部 Load/LoadAsync/LoadSubAsset 重载、
  `BundleLoader.AssetProxy._LoadAssetInternal`、`FAssetProxyHandle` 和完成回调。

### 当前缺少的证据或 Hook

1. 找到真正返回 `Mesh`/`Material`/`Texture` 描述对象的 Resolver；当前 AssetBundle、
   `FAssetProxyHandle.GetAssetProxy` 和已观测的 String/hash `_LoadAssetInternal` 没有命中目标资源。
   资源转储已经确认下一轮应覆盖 `BundleResourceManager.Load/LoadAsync/LoadSubAsset` 的
   `GUIDProxy` 重载，以及 `_LoadAssetInternal(GUIDProxy, ...)`、`_LoadAssetInternal(StringPathHash, ...)`。
   NPC 还可能直接消费 `FAssetProxyLoaderHandle`，因此要观察它的 `Get`、`LoadImmediate` 和
   `AddOnProxyCompleted`，不能只盯 `FAssetProxyHandle`。
2. 补齐 `CreateMeshAssetsInfoForPostModel.AssignSkin` 的精确参数签名，并把大世界、UI、NPC
   的 `resource identity -> descriptor -> Renderer` 关联记录在同一条事务中。
3. 修正或重新定位 `_ValidRenderers`，使其能证明 Renderer 名册在何时冻结；`count=0` 不能作为成功证据。
4. 获取一个实际的 Typhoea NPC 实例，确认 NPC 是否消费同一个描述对象，以及它是否绕过了当前候选 Resolver。
5. 在 V2 前确认原生可见性输入究竟是 `SubMeshInfo.isActive`、`isRendererDisabled`、
   `VisibleController` 还是其他部件表；只做单变量读写实验。

### 必须继续关闭的写入路径

`EiemApplyResolvedRenderRule`、`EiemApplyStandaloneRenderRules` 的资源写入分支、
`set_sharedMesh`/`set_bones`/`SetSMRRootBone` 改写、Partner/Physics 创建销毁、F10 恢复重放和
逐帧物理轮询都不能参与上述取证。即使日志完整，这些 Hook 也会改变验证对象。

## 15. V1.1 描述符取证部署

2026-09-15 已部署 V1.1 取证 DLL。它新增的运行时观察点为：

- `FAssetProxyLoaderHandle.Get`、`LoadImmediate`、`AddOnProxyCompleted`；
- `SubMeshInfo.get_mesh/set_mesh`（使用 V1.1 专用 detour 名称，原函数仍先后各调用一次）；
- `NPCAvatarLodMeshAssets.GetSubMeshInfo`；
- `NPCAvatarMeshAssetsSO.GetAvatarSlotMeshAssets` 和 `GetAllAvatarSlotMeshAssets`。

这些探针只读取目标名称、Mesh 指针、路径哈希、active/disabled、rootBoneID 和 LOD 数组，
没有替换参数，也没有写入游戏对象。V1.1 DLL 已部署到
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，SHA-256 为
`7BF1E546A4B9D771B37C328D773F94DB98214647C187AD5B4B8FABA2044AE847`，备份目录为
`backups/deploy_v1_1_descriptor_final_20260915_212859`。

`GUIDProxy` 的直接按值 detour 暂未启用。资源转储已确认其重载存在，但其原生值类型 ABI 尚未
通过安全方式确认；先用 Loader 和描述符探针验证是否已经能关联目标 Mesh，再决定是否需要补这个
入口。合同测试当前为 141 项通过、48 项环境跳过。

## 16. V1.1 重启证据：PostModel 的实际 Mesh 来源

2026-09-15 21:31 左右的重启日志进一步区分了资源描述与 PFB 实例输入：

- `chr_0034_typhoea_postmodel.prefab` 在 `PrefabInstantiateProxy.OnCompleted` 的扫描阶段已经有
  72 个 `SkinnedMeshRenderer`，其中 `S_actor_typhoea_body_01_lod0`、
  `S_actor_typhoea_cloth_01_lod0` 等 Renderer 已经持有非空 Mesh；此时 mod 配置仍为空，尚未执行
  任何替换。
- `chr_0034_typhoea_uimodel.prefab` 在 UI 加载完成阶段已经有 17 个 Renderer。它与世界 PFB 的
  `body_01_lod0`、`cloth_01_lod0` 等 Mesh 指针相同，并且 rootBone 名称也相同。
- 同一世界 PFB 随后进入 `CreateSMSInfoForPostModel -> AssignSkin`，最终 Renderer 数组继续使用
  这些 Mesh。两条链路之间没有观察到 `SubMeshInfo.set_mesh` 或 `FAssetProxyLoaderHandle` 调用。
- `NPCAvatarLodMeshAssets.GetSubMeshInfo` 返回的 `SubMeshInfo` 仍然包含 `meshPathHash`、
  `meshName`、`rootBoneID` 等描述信息，但 `mesh` 字段为空。对 PostModel 来说，这个对象不能被
  当成最终 Mesh 实例的唯一替换入口；它更接近资源索引与装配配置。

本轮结论：PostModel 的 Mesh 已在 PFB 实例完成时绑定，之后才进入游戏的原生装配和 Animator/LOD
链路。因此下一阶段的候选入口是“PFB 完成、原生装配开始前”的资源身份关联，而不是
`SubMeshInfo.mesh` setter 或 Renderer 末端重绑。该入口仍只做只读关联，不能据此直接启用 Mesh 替换。

下一轮验证顺序调整为：

1. 在 `PrefabInstantiateProxy.OnCompleted` 记录 PFB 路径、实例对象、Renderer 名称、Mesh 指针，并
   与 `CreateSMSInfoForPostModel` 的输出按同一实例关联。
2. 获取一个实际 Typhoea NPC 实例，确认 NPC 是否消费同一 PFB/同一 Mesh 身份；当前日志中的 NPC
   仍是 Andrew，不能代替这条证据。
3. 若三端关联成立，再验证在该完成边界替换一个 Mesh 是否仍由游戏原生 `AssignSkin`、LOD 和动画
   继续接管。此验证前保持 `mod.ini` 注释，禁止 Partner、骨骼、Physics、按键和 F10 写入。

## V1.2 Prefab Context Probe (2026-09-16)

The latest restart established that two Typhoea NPC instances can share one
`NPCAvatarLodMeshAssets` pointer (`0000000FA247C0C0`) while retaining different
Avatar, Model, parent and Renderer-array pointers. This pointer is shared
resource description state, not an instance key. The probe now keeps multiple
owner links and records ambiguity instead of overwriting the previous owner.

The deployed read-only probe also records `NPCAvatarMeshAssetsSO.prefabItems`,
the concrete loader object passed to `CreateSMSGO`, and the parent Transform
passed to both `CreateSMS*` methods. It still performs no Mesh, material,
Renderer, Partner, Skeleton, Physics, LOD or visibility writes.

Build SHA-256:
`981B273D6ABCDEF6D484D04CEA152028626881676389C3BFC4F71C86D87B8F95`

Deployment backup:
`E:\vscode\EIEM\backups\probe-prefab-context-20260916-082619`.

## 17. Direct PFB skeleton comparison (2026-09-17)

The exported V1 skeleton resources were compared directly, using the same
binary layout as the DLL reader:

- World: 316 nodes; UI: 316 nodes; NPC: 317 nodes.
- The NPC has one extra outer `chr_0034_typhoea_postmodel` transform. After
  removing that instance wrapper, its `Root` subtree has the same 315 nodes
  and parent relationships as UI.
- World and UI match in order and transforms except for one path at the skirt
  leaf: world uses `.../skirt_base_R_c_03_jnt`, while NPC/UI use
  `.../skirt_base_L_c_03_jnt`.
- `cloth_01_lod0` has 120 bindposes in the shared Mesh resource. The PFB
  skeleton name difference therefore does not justify rebuilding the existing
  Renderer `bones[]` array.

The runtime rule is consequently: preserve the source Renderer bone array for
all existing Mesh slots; only resolve authored paths appended after the source
slot count. A replacement with fewer slots is rejected. This handles the
three PFB variants without a role-specific name map.
