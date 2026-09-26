# GPU 蒙皮与环形缓冲区证据

本文记录随机躺地问题目前已经确认的 GPU 侧事实、Mesh 数据差异和下一步取证边界。文中的“已证实”来自用户提供的渲染管线信息、`E:\XXMI\EFMI` 中的 shader 参考实现，以及工作区内 Mesh 解析结果；“假设”不会当成修复结论。

## 已证实的运行时现象

1. `body_01` 在现有测试中没有观察到躺地；`cloth_01`、`cloth_02` 的全部可见部分可能在冷启动或 F10 后整块躺地。
2. 冷启动和 F10 都可能复现，换地图不能可靠恢复；一旦出现，姿态会持续保持，不是下一帧自动恢复的短暂错误。
3. 延迟约 100 ms 可以降低复现概率，但不能根除，因此不能把固定延迟当作正确的装配边界。
4. 已有 CPU 侧探针曾确认：replacement Mesh 指针稳定、`bones[]` 非空且数量稳定、没有空槽或越界，逐顶点 CPU 蒙皮也可得到正常结果。这只能证明公开 Renderer 和 CPU 检查的数据正常，不能证明自定义渲染提交使用了同一代骨骼数据。
5. 游戏使用自定义渲染管线。蒙皮在 vertex shader 中完成；用户确认 GPU 侧看到的是 T-pose 顶点数据，骨骼矩阵来自 VS `t0`，CB 区域提供查找偏移，多个 Mesh 共用一个约 4 MB 的环形结构化缓冲区。
6. 用户已确认当前游戏的 CB 确实提供两段骨骼区偏移，分别用于当前帧和上一帧；两段数据是 TAA 所需的 current/previous skinning 结果。
7. 用户已确认当前游戏 VS 的骨骼索引寻址与 EFMI 参考一致：顶点索引按每骨骼三个 `float4` 条目换算，再加上 CB 提供的实例基址。

## EFMI shader 参考给出的结构

`E:\XXMI\EFMI` 中的以下文件展示了与用户描述一致的访问方式：

- `1bd1c63f1d168878-vs_replace.txt`
- `1479...-vs_replace.txt`
- `Mods\DISABLEDBL (1)\BL\hlsl\record_bones_cs.hlsl`
- `Mods\DISABLEDBL (1)\BL\hlsl\redirect_cb1_cs.hlsl`

这些文件不是当前 Endfield 进程的运行时证明，但用户已确认当前游戏采用同样的两段 offset 和索引寻址结构。它们明确显示：

```hlsl
StructuredBuffer<t0_t> t0 : register(t0);
// t0_t 内含 float val[4]，单项 stride 为 16 字节

r0.y = (uint)v9.x << 4;
r4.xy = int2(3, 3) + asint(cb3[r0.y + 5].xy);
r5.xyzw = mad((int4)v8.xyzw, int4(3, 3, 3, 3), (int4)r4.xxxx);
```

含义是：每个实例从常量缓冲区取得一组基址，顶点中的骨骼索引先乘以 3，再加到该基址；一个骨骼至少对应连续的三个 `float4` 条目。`record_bones_cs.hlsl` 进一步展示了 current/previous 两个偏移从 CB 字段读取，再从共享 `t0` 环形区复制到目标区；这与“同一大缓冲区、不同 Mesh 使用不同 offset”的描述一致。

因此，GPU 蒙皮不等价于“Renderer 的 `bones[]` 数组正确”。最终结果至少还取决于：

- 本次 draw 的实例 ID 和 CB 块；
- CB 中指向 current/previous 骨骼区的偏移；
- 顶点索引与该 draw 的 palette 是否来自同一 Mesh/LOD；
- 环形缓冲区对应区段是否仍是当前实例、当前帧的数据；
- 自定义渲染器是否在公开 Renderer 更新后重用了旧的 draw 记录。

## 当前 Mesh 槽位证据

使用 `tools/mesh_skin_index_check.py` 读取 EIEMESH 的 `bonePaths`、bindpose 和顶点使用到的最大索引，得到以下结果。替换 Mesh 是当前 mod 使用的单一 Lod0 Mesh；原生 Mesh 来自 `E:\EIEM_Workspace\game-mod-backups\typhoea_mesh_material_test.full-backup-20260904\meshes`。

| 部位 | 原生 LOD 槽数 | 替换 Mesh 槽数 | 原生最大使用索引 | 替换最大使用索引 |
|---|---:|---:|---:|---:|
| body01 | LOD0 35；LOD1 39；LOD2 35；LOD3 17 | 76 | 34/38/34/16 | 75 |
| cloth01 | LOD0 120；LOD1 120；LOD2 120；LOD3 61 | 122 | 119/119/119/60 | 121 |
| cloth02 | LOD0 126；LOD1 126；LOD2 114；LOD3 24 | 126 | 125/125/113/23 | 125 |

这证明不同 LOD 的原生 Mesh 确实有不同的局部槽位数量和顶点索引。它还证明当前 INI 将一个 Lod0 replacement Mesh 复用于多个原生 LOD 时，不能只凭“同一完整骨架”推断 GPU 的局部 palette、索引和 CB 区段一定兼容。

但这张表**不是根因证明**：body replacement 也比原生 body LOD1/2/3 有更多槽位，却没有出现同样现象。因此还必须验证 body 与 cloth 是否走了不同的 draw/palette/物理提交路径，或者 cloth 的额外物理骨骼使错误的 palette/offset 更容易暴露。

## 目前可以排除与不能排除的内容

可以排除：

- 单纯的公开 `SkinnedMeshRenderer.bones[]` 为空、越界或被 F10 随机替换；
- 单纯的一帧延迟或下一帧会自动修复的动画竞争；
- 把 LOD 切换本身当作根因；用户已确认切换 LOD 后问题仍可持续。

尚不能排除：

1. replacement Mesh 的顶点索引仍按 Lod0 palette 编码，但自定义渲染器为当前原生 LOD 使用了另一套局部 palette 或另一组 CB offset。
2. cloth draw 记录中的 current/previous offset、实例 ID 或环形缓冲区区段与 Mesh/Renderer 代际不一致；错误记录写入后会持续渲染，所以表现为“整块躺地”。
3. body 与 cloth 在自定义渲染器中使用不同的材质/VS 变体、palette 构建或物理结果提交路径；body 稳定不能证明所有部位路径相同。
4. 公开 Renderer 更新已经完成，但自定义管线保存的 draw command 或 GPU buffer 仍引用旧的资源代际。这个分支需要在最终 CPU→自定义渲染提交边界取证，不能由当前 Renderer 探针推断。

## 当前日志的边界（2026-09-22）

本轮日志确认了探针安装状态，但没有取得最终 GPU draw 数据：

- `SkinnedMeshCaptureManager.RequestCapture`、`MaterialPropertyBlock` 的 buffer/constant-buffer、`CommandBuffer` 的全局 buffer、GPUDriven V1/V2 绑定和 dispatch 探针均报告安装成功；事务窗口内 `[SKIN-GPU-*]`、`[SKIN-CMD-*]`、`[SKIN-BUFFER-*]`、`[SKIN-NATIVE-*]` 记录数为 0。
- `[HG-CENSUS-v1]` 的目标 `HGMeshRenderer` 数量仍为 0，所以不能把 `HGMeshRendererData` 或 `partSubMeshGPU` 当成当前 Typhoea 主模型的提交入口。
- `[RENDER-REG-v1]` 和 `[BASEMODEL-CACHE-v1]` 能看到原生 Renderer 注册和公开 Mesh/bones 状态，但这仍停在自定义 draw 之前；最近事务中有记录显示 body/cloth02 仍为原 Mesh，cloth01 只有部分事务保留 replacement。这是资源注册/回放状态证据，不是 GPU 矩阵证据。

因此，当前 Unity 公共渲染 Hook “安装成功但没有调用记录”本身就是证据：主路径不是这些公开接口，下一步必须沿游戏自己的 GPU 资源/实例数据查找；继续扩大这些 Unity Hook 只会增加日志，不会触及最终 `t0` offset。

## 下一步唯一必要的取证

不再继续增加延迟或修改骨骼解析规则。下一次探针应在自定义渲染器生成最终 draw 数据的 CPU 边界，一次性记录 body、cloth01、cloth02 的：

1. Renderer/原生 Mesh 身份和当前 LOD；
2. draw 的实例 ID、CB 块地址或索引，以及类似 `cb[* + 5].x/y` 的 current/previous offset；
3. 绑定的共享 `t0` buffer/view、stride、palette 起始位置和可读长度；
4. 顶点 blend index 的最大值与 `base + index * 3` 的最终范围；
5. 记录时的资源代际、线程/调用返回地址和是否为重建后的新 draw 记录。

判定规则：

- 如果躺地时 CPU 最终 draw 的 offset、palette 范围或实例 ID 与同一 Renderer 的正常样本不同，问题在自定义提交/缓存代际；
- 如果 CPU draw 数据一致但画面仍躺地，问题已进入 GPU buffer 写入、GPU 资源绑定或 shader 解释层；
- 如果 cloth 与 body 的 shader/CB/palette 路径不同，优先沿差异路径定位，不再把 body 当作 cloth 的验证替代品。

在拿到这组最终提交证据前，不修改 DLL 的写回时机，也不把“等待物理结束”写成解决方案。当前文档只记录证据和可证伪分支。

## 2026-09-22：自定义 GPU 类型候选（静态字符串 + 只读元数据探针）

离线扫描当前 `global-metadata.dat` 找到了一条比 Unity 公共渲染接口更接近最终提交的候选链。字符串只证明类型/方法名存在，不能单独证明 Typhoea 实例走这条路径；因此本轮没有安装调用 Hook，也没有读写对象字段。

- `HG.Rendering.Runtime.GpuClothManager`：`GetSkeletonBuffer`、`GetUploadData`、`GetRenderData`、`FlipSkeletonFlag`、`IsClothSkeletonFlipped`、`PipelineUpdateV2`、`RegisterClothGroup`、`_SetPerDrawData`、`_UpdateRuntimeBuffer`、`_InitStreamingGpuBuffer`。
- `HG.Rendering.Runtime.GpuClothRenderData`、`GpuClothGroupUploadData`、`GpuClothClearBufferData`、`GpuClothMatrixGenerator`、`GpuClothSimulationPassConstructor`：这些类型可能承载当前/上一帧骨骼上传、清理和渲染阶段数据。
- `Beyond.NPC.Animation.GpuAnimator`、`GpuAnimatorSampler`、`GpuAnimInfo`、`GpuAnimMgr`：候选动画采样与 GPU 动画实例链。
- `Beyond.Gameplay.Core.GpuResourceWithMat`、`NpcGpuResource`、`NpcGpuInstanceWithMat`、`NpcGpuInstanceMgr`：候选 GPU 资源、实例和材质/部件提交链。

本轮把上述类型一次性加入 `EiemDumpCustomSkinPipelineMetadata`，只在 IL2CPP 初始化后枚举真实字段、返回值和参数类型。DLL 已编译并部署；下一次启动日志应首先看 `[CUSTOM-SKIN-META]`、`[META-FIELD]`、`[META-METHOD]`。只有找到真实的骨骼缓冲区/上传数据签名和对象关联，才进入下一轮观察 Hook。

`Il2CppDumper` 对当前 `GameAssembly.dll` 报告 `MetadataRegistration=0` 并无法加载自定义 PE，因此没有生成 `dump.cs`；静态字符串结果不被当作字段偏移或根因证据。

## 2026-09-22：GpuClothManager 运行时边界探针（已部署，待重启取证）

元数据探针已确认 `HG.Rendering.Runtime.GpuClothManager` 的字段与方法存在，并得到当前版本的字段偏移：`m_characterMesh=0x110`、`clothSkeletonDataBuffer=0x148`、`isStreamingMode=0x290`、`skeletonFlipped=0x294`、`m_runtimeClothNum=0x2B8`、`m_runtimeClothGroupNum=0x2BC`。这些偏移只用于同一版本的只读观察，不写入对象。

本轮 DLL 新增了有界的 `[GPU-CLOTH-BOUNDARY-v1]` 观察钩子，覆盖：

- `Tick(float)`
- `RegisterClothGroup(ClothGroupData&)`
- `_SetPerDrawData()`
- `GetSkeletonBuffer()`
- `IsClothSkeletonValid()` / `IsClothSkeletonFlipped()`
- `PipelineUpdateV2(Transform)`

每次冷启动或 F10 事务最多记录 512 个事件，原函数先执行，探针随后读取指针、运行时数量、翻转/有效性结果、线程和调用返回地址。探针不读取 ComputeBuffer 内容、不调用 `GetData`、不改变 Mesh、bones、材质、缓冲区或调用时序，因此不会把 GPU 同步开销混入复现条件。

当前这次启动的日志文件时间早于 DLL 部署时间，尚未加载本轮钩子；需要下一次重启后查看 `[RES-TRACE] ... GpuClothManager ... observation hook installed` 与 `[GPU-CLOTH-BOUNDARY-v1]`。判断规则：

1. 若 `characterMesh` 命中目标角色且 `skeletonBuffer` 非空，说明已进入游戏自定义物理/骨骼上传拥有者；比较正常与躺地事务的事件顺序和 buffer 指针。
2. 若 `IsClothSkeletonValid=0` 或 `GetSkeletonBuffer` 为空，只能说明物理 GPU 资源当时未就绪，不能直接推断 replacement Mesh 错误。
3. 若 cloth 与 body 的 manager 状态一致而画面仍不同，问题继续向 RenderGraph/自定义 draw 提交边界定位；不能用公开 Renderer 的 bones[] 结论替代最终 GPU draw 数据。
4. 若完全没有事件，说明 Typhoea 实例没有走这条 manager 路径，下一步应转向 `GpuAnimator`/`NpcGpuInstanceWithMat` 或更下游自定义提交，不再重复扩展 Unity 公共 Hook。

## 2026-09-22：首次 GpuCloth 运行时取证结果与 ABI 修正

本次重启已确认新 DLL 生效，日志出现全部 `GpuClothManager` observation hook installed，并在 cold-start probe 中命中两次 `PipelineUpdateV2`。第一次记录的 manager 字段为 `characterMesh=0x40A00000`、`skeletonBuffer=null`、`clothNum=5701632`、`groupNum=7077960`，这些值不可能作为合法的 Mesh/ComputeBuffer/cloth 数量使用。

因此这不是“物理骨骼 GPU 缓冲为空”的证据，而是 `PipelineUpdateV2` 的 native ABI 或 static/instance 解释不正确的证据。原探针把第一个参数一律当作 `GpuClothManager*`，读取了错误对象的字段；不能据此判断游戏物理是否完成。

已修正探针：下一版在安装时记录 `PipelineUpdateV2` 的 method flags 和 static 标记；若为 static，使用单独的 `(Transform, MethodInfo)` ABI，只记录 Transform 类型，不读取 manager 字段；若为 instance，继续记录 manager 字段并同时记录 `self` 的实际 IL2CPP 类型。此次修正仍是只读观察，不改变原调用。

## 2026-09-22：PipelineUpdateV2 已确认不是上传边界

第二次重启记录了 `GpuClothManager.PipelineUpdateV2 flags=0x96 static=1`。修正 ABI 后，它接收到的对象类型是 `UnityEngine.Transform`，名称为 `PlayerCenterProxy`。因此该方法只是物理中心/玩家位置更新入口，不承载 manager 实例字段，也不是最终 cloth skeleton buffer 或 draw 提交入口。

本轮已将探针扩展为：进程启动阶段最多记录 128 个 GpuCloth 事件，冷启动/F10 窗口继续最多记录 512 个；并补充 `_SetCharacterProxyMesh(Mesh)` 与 `FlipSkeletonFlag()` 观察。下一轮应以 `RegisterClothGroup`、`_SetCharacterProxyMesh`、`GetSkeletonBuffer`、`FlipSkeletonFlag` 和 `SetPerDrawData` 的真实调用顺序为主。若启动窗口仍只有 static `PipelineUpdateV2`，则 Typhoea 当前实例没有经过 `GpuClothManager` 的 manager 实例路径，需要转查 `GpuAnimator`/`NpcGpuInstanceWithMat` 或更下游的自定义 draw 数据。
## 2026-09-22：本轮重启实测增量证据（只读）

本轮日志文件为 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem_log.txt`，末尾记录了 `Game window closed`，因此以下结论来自本轮已完成的采样窗口，不代表当前进程仍在运行。

### 已确认

- Typhoea 的公开 `SkinnedMeshRenderer` 快照在两个冷启动窗口中保持 replacement Mesh：`body_01`、`cloth_01`、`cloth_02` 的 replacement 指针分别稳定为 `0000000FCB34C760`、`0000000FCA1C13C0`、`0000000FCE8FBD60`。对应 `bones[]` 数量为 76、122、126；`nullBones=0`，越界检查没有报错。
- 同一批 Renderer 的 bounds 中心 Y 约为 283，说明 CPU 侧 Mesh 对象和公开 Renderer 状态本身没有变成贴地几何。这个证据不能证明自定义 GPU draw 使用了相同的矩阵。
- `GpuClothManager` 探针只命中了 `Capsule` 和 `PlayerCenterProxy` 的静态 `PipelineUpdateV2` 调用；本轮没有看到 Typhoea 的 `RegisterClothGroup`、`GetSkeletonBuffer`、`SetPerDrawData` 或实例 `Tick`。因此这条 GpuClothManager 路径不是当前 Typhoea 主模型的最终上传证据。
- 原生物理诊断文件为 `plugin/physics_diagnostics/physics_runtime_35976_54231765.json`：7008 个观测调用，`dropped=0`，但文件明确声明这些是调用观测，不是 Job 完成栅栏。活动 Typhoea 实例有 11 个有效且运行中的 BeyondDynamicBone 组件，Team ID 为 36–46。
- 这 11 个活动组件的 `ClothProcess.Init` 与 `StartRuntimeBuild` 发生在 tick `54212000`；`TeamManager.AddTeamAnimatorData` 随后注册 Team；`DynamicBoneTransformManager.WriteDoubleBufferTransform`/`CopyDoubleBuffer` 从 tick `54207890` 持续到 `54231750`。这证明物理双缓冲在运行期持续写入/复制，但还没有证明它就是 VS `t0` 的最终 skin palette。
- 本轮 `PHYSICS-ORDER-PROBE` 的 render commit 位于 tick `54211859`/`54212000`，而 `CompleteMasterJob` 的全局候选完成 tick 最晚为 `54218687`。这个全局候选不能当作自定义渲染提交栅栏；它只说明公开物理观测与模型注册时间存在交叠。

### 尚未确认

最终需要的是自定义渲染器生成 draw instance 时的 CPU 记录：本次 draw 使用的实例 ID、CB 中 current/previous 两个偏移、共享 `t0` 的起始地址/可读范围、顶点 blend index 的最终范围，以及该记录是否在资源重建后更新。当前探针还没有拿到这些字段，所以不能把 `bones[]` 正确解释成“GPU 矩阵正确”，也不能把 `body` 没出问题解释成 `cloth` 的根因。

下一步只应围绕自定义 RenderGraph/GPU 资源提交边界增加只读关联探针；不再增加 Unity 公共 Renderer Hook，也不把固定延迟写成修复方案。
## 2026-09-22：重新发现的骨骼候选冲突（与 GPU 身份分开）

本轮日志还暴露了一个独立的问题：`body_01` 的 LOD1/2/3 在第一次组装时被拒绝，日志是 `Replacement bone source candidates disagree in model instance`。冲突记录中，目标 body 分支的 `rootBone=0000000FCB8D7EA0`，另一个 cloth 分支的 `rootBone=0000000FCB9504E0`，但它们共享同一个 `skinningRoot=0000000FCB8D7E80`。

这说明当前解析器虽然写了“按实例统一骨架”的意图，实际仍把同一个实例内不同分支的候选当成冲突并整体拒绝。后续重试中布局又能让这些 LOD 出现 replacement，这是组装时机和候选集合不稳定的证据，但它不能单独解释持续身体贴地。

这条问题应先修成“当前 Renderer 的本地原始 bones[] 有记录时优先它；需要扩展时按同一 skinningRoot 的层级索引映射；不以场景名称、LOD 索引或固定延迟代替证据。在这条规则改好之前，不应把任何新的 GPU 结论归因于骨骼解析。
## 2026-09-22：EFMI 静态 shader 对提交边界的补充证据

本节只记录 `E:\XXMI\EFMI` 中的静态文件，不代表本次 Endfield 进程已经捕获到同样的运行时数值。

- `784f11ae11c97112-vs_replace.txt` 声明 `StructuredBuffer<t0_t> t0 : register(t0)`，结构步长为 16 字节；同一个 VS 还声明了 `cb2`，每行也是 `float4`。
- VS 先用 `SV_InstanceID << 4` 选取当前实例的 16 行记录，再从 `cb2[instanceBase + 5].xy` 取 current/previous 的 palette 基址，并给基址加 3 行保留区。
- 顶点的 `BLENDINDICES0` 会乘以 3；随后 VS 按 `base + index * 3 + row` 从 `t0` 读取每根骨骼的三行数据。也就是说，最终蒙皮取决于 draw 的实例记录、CB 偏移、顶点索引和同一时刻绑定的 t0 视图。
- `Mods\DISABLEDhg\hlsl\record_bones_cs.hlsl` 明确使用 `cs-t0 = vs-t0`，把原生 VS t0 按 CB 给出的 current/previous 窗口复制到另一个骨骼池；`gather_local_bones_cs.hlsl` 再按局部到全局骨骼映射生成局部 palette；`redirect_cb1_cs.hlsl` 重写每个实例的 current/previous 偏移。

因此，公开 Renderer 的 `mesh`、`bones[]`、bounds 或 CPU 逐顶点结果，不能证明最终自定义 draw 使用了正确的 t0 区段。真正能区分“CPU 绑定正确但画面躺地”和“绑定阶段已经错误”的只读探针必须同时记录：VS/实例标识、CB 中的两个偏移、绑定的 t0 资源与 stride、该 draw 的顶点索引范围，以及资源重建前后的变化。

当前 EIEM 的 IL2CPP/Unity GPU hook 在已有会话中没有捕获到这些字段（`SKIN-GPU-*`、`SKIN-CMD-*`、`SKIN-BUFFER-*` 计数为 0），所以还没有拿到 Endfield 运行时的最终 t0 内容。下一步应先确认游戏实际使用的图形后端，再在对应的原生提交边界做一次性只读采样；不能把 EFMI 的静态 shader 文件当作本次运行时证明，也不能用固定延迟替代该边界。

## Vulkan native submit probe (2026-09-22)

Endfield is confirmed to use Vulkan. The new `src/vulkan_skin_probe.h` is compiled into the `vulkan-1.dll` proxy. It only observes calls and then invokes the original function; it does not submit extra commands, read GPU memory, or use RenderDoc.

The bounded log is `plugin\eiem_vk_skin_probe.log`. It records:

- `VK-SKIN-BUFFER/MEMORY/BIND/MAP/FLUSH`: buffer size/usage, memory binding, CPU mappings, and flush range/hash/prefix. A host-visible shared bone ring will appear here.
- `VK-SKIN-DESC`: descriptor-set storage/uniform buffer handles, base offsets, and ranges.
- `VK-SKIN-DRAW` and `VK-SKIN-DRAW-INDIRECT(-COUNT)`: pipeline, descriptor sets, dynamic offsets, index/indirect ranges for actual draws.
- `VK-SKIN-SUBMIT`: queue submit batches and command buffers.
- `VK-SKIN-STACK`: return-address frames for the first 128 draws, usable as module+offset addresses in IDA.

Interpretation: compare cloth/body descriptor resource handles first, then their dynamic offsets. Same t0 resource with different flush contents indicates a CPU ring-buffer/lifetime issue; correct flush with a wrong draw offset indicates descriptor/dynamic-offset binding; both correct while the picture is still prone indicates shader-local bone indices or another VS input. An empty probe log only means the proxy/entry path was not loaded; it is not evidence that bones are correct.

## Vulkan probe v2: draw-to-palette evidence path (2026-09-22)

The first native Vulkan run proved that direct indexed draws are intercepted, but its log cap was consumed by unrelated transient buffers. It produced 128 draw call stacks while recording zero descriptor updates because the proxy had only wrapped the ordinary `vkUpdateDescriptorSets` path. The game may use descriptor update templates, push descriptors, or the newer `*2` entry points; zero ordinary updates therefore was not evidence of an empty GPU binding.

The next build keeps resource creation and mapping state but adds the missing descriptor paths and suppresses repetitive 4,406,400-byte transient-buffer lines. It recognizes the known Typhoea replacement draw index counts from the EIEM mesh payloads: body `78480` (76 bones), cloth01 submeshes `4194, 32607, 31365, 45486, 13008, 49632, 9480, 62370` (122 bones), and cloth02 submeshes `7554, 3564, 11952` (126 bones). A target draw starts a bounded sampling burst at cold start and on an observed F10 key-down edge.

The proxy also records `VK_EXT_descriptor_buffer` binding and offset calls. Those records identify the alternate path if the game does not use descriptor sets; decoding the descriptor bytes themselves requires the corresponding layout offsets and device-address ranges, so a hit there is reported as a separate binding branch rather than misclassified as an empty set.

For each target draw, the probe records the command buffer, pipeline, index range, instance ID, descriptor set/binding, resource handle, base/dynamic offsets, CPU mapping sample, and a stack with module plus RVA. It then applies the shader contract in `E:\XXMI\EFMI\1479b2b594b9c91a-vs.txt`: `cb2[(SV_InstanceID << 4) + 5].x/y` are read as the current/previous `t0` palette row bases; the shader adds three rows before the first 3x4 matrix. The probe reads both windows when the Vulkan allocation is CPU mapped and reports:

- `current`/`previous` palette base from the exact CB row;
- number of matrices that are identity, all-zero, or non-finite;
- whole-window hash and first 3x4 matrix values;
- whether the final `t0` range was CPU-readable at all.

This is the required evidence chain. If cloth and body have the same draw/CB offsets but different palette contents, the shared ring write or lifetime is wrong. If the final draw has a different offset or descriptor resource, the custom renderer's instance binding is wrong. If both have the same non-identity palette data and cloth still lies, the remaining branch is the vertex input/index interpretation or shader-side conditional, not the public `bones[]` array. If the final `t0` allocation is not mapped, the log must be extended with the buffer-copy/compute producer before claiming the GPU contents are known.
