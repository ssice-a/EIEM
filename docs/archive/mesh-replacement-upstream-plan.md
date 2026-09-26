# Mesh 上游创建替换方案

更新时间：2026-09-26

## 当前结论

此前把资源返回值替换称为“上游创建替换”，这个命名不准确，现已撤回。`FAssetProxyHandle.Get`、`FAssetProxyUntrackedHandle.Get`、`FAssetProxyLoaderHandle.Get` 和 `SubMeshInfo.get_mesh` 都发生在 Unity Mesh 已经创建并返回之后；在那里生成 EIEM Mesh 仍然绕过游戏创建阶段的私有 native 元数据，不能作为 cloth01 躺地修复。

Typhoea 的 12 个源 Mesh 已确认都经过 `UnityPlayer+0x1EA2A0` 的二进制 Mesh 读取函数；读取名称后能识别逻辑 Mesh，读取后 `native+0x1C8`（`m_BonesPerVertex`）为 4。上游替换的**载荷格式、资源路径身份、流所有权及缓存重建**仍未解决，因此当前只有已验证的读取边界，没有可部署的替换实现。必须在游戏把顶点、索引和权重描述交给 Mesh 创建/注册函数之前替换完整 payload，让游戏自己的 native 分配、布局、私有元数据、LOD、skin 注册和 GPU 资源建立流程完整执行。

目标仍是：替换 Mesh、材质、贴图和多个子 Mesh，支持按键切换与 F10 热重载；物理骨骼接入放到创建链稳定后处理。

## 现有证据

| 现象 | 结论 |
| --- | --- |
| 新建 Mesh 后 `native + 0x1C8` 在不同对象上出现 0、随机整数或其他值 | 该字段是 `m_BonesPerVertex`，当前 setter 路径未稳定初始化它；不能把这些值当成应该硬编码的 flag。 |
| `InternalSetBoneWeights`、bindpose、UploadMeshData 等公开/内部 setter 前后字段不变 | 蒙皮数组写对不等于游戏私有 native 注册记录建立。 |
| 源 Mesh 的 `+0x1C8` 通常为 4，替换 Mesh 的值不稳定；C7 消费端再派生出 16/48/52 | 16/48/52 是消费端组合值，直接写入会掩盖根因。 |
| 克隆实验中 cloth01 源布局为 23,758 顶点，替换布局为 51,677 顶点；克隆后 setter 读回仍为源布局，替换被拒绝 | 克隆不能承载当前几何布局不同的衣服。 |
| Renderer 后置替换曾出现缓存代际不一致、撕裂和闪退风险 | 后置替换进入注册链太晚。 |
| 冷启动和 F10 都可能躺地 | 不能把问题归因于 F10；两种情况下替换 Mesh 均可能带入无效元数据，其他条件尚未排除。 |

## 主线实现

### 1. 找到资源创建边界

在资源管理器 Mesh 返回、Mesh 数据解码和原生 Mesh 注册之间增加只读关联记录，记录：

- 逻辑资源路径、asset 名和 Mesh/PFB/UI 资源类别；
- 原始资源描述对象、顶点/索引/子 Mesh 数量；
- 游戏创建出的 Unity Mesh、native 指针、`+0x110/+0x124/+0x1C8`；
- 创建函数返回地址和后续注册函数的调用链。

先覆盖普通角色 Mesh；NPC/UI 使用各自资源类别，但共用资源身份和代际记录，不假设它们共用同一个 PFB 结构。

### 2. 目标：替换资源描述，不替换已装配 Renderer

当资源身份命中 `mod.ini` 的 Mesh 规则时，在游戏原始 Mesh 创建函数消费描述对象之前替换：

- 顶点、法线、切线、UV、颜色；
- 索引、子 Mesh 数量和每个子 Mesh 的索引范围；
- bindpose、可变骨骼权重和材质槽描述。

游戏原始函数继续负责 native Mesh 分配、布局、私有蒙皮元数据、注册、缓存和 GPU 资源建立。任何校验失败都回退到源资源，不发布半成品。

### 3. F10 热重载

F10 只递增替换资源 generation 并使目标资源缓存失效，然后调用游戏已有的资源重建/模型重载入口。不会在已渲染的 Renderer 上直接交换 Mesh 指针。

如果某类资源没有公开失效入口，就在该类资源的原生创建边界重新走一次游戏自己的实例重建；不为 NPC/UI 强行复用普通角色 PFB 流程。

材质、贴图和子 Mesh 显隐都挂在同一个资源 generation 上，避免 Mesh 已更新而材质或 GPU 索引仍来自旧代际。

### 4. 物理阶段

先确认创建后的 Mesh、bones、bindpose 和游戏自有注册记录稳定，再接入物理骨骼。物理代码不负责修正渲染 Mesh 的私有 flag。

## 只读追踪顺序

1. 资源身份和创建返回关联；
2. 原生 Mesh 创建后第一处 `+0x110/+0x124/+0x1C8` 写入；
3. 注册/校验函数对同一 native 指针的读写；
4. Renderer、缓存 generation 和 GPU 描述符的绑定；
5. 冷启动与 F10 的两条链逐项对比。

追踪器只记录目标资源的 native 指针和调用链，窗口按一次创建事务结束，不扫描所有高面数 draw，也不按角色名或固定 flag 值猜测。

## 验收门槛

- 冷启动多轮：cloth01/cloth02 不躺地，Mesh 不消失，body 材质不短暂回源；
- 手动 F10 至少 100 次：无躺地、撕裂、闪退和卡死；
- 普通角色、NPC、UI 资源类别分别验证；
- 日志只保留每次资源事务的摘要和异常，禁止再次产生无限增长的大日志。

当前已恢复基线硬件追踪 DLL，克隆版未保留在游戏插件目录。
## 本轮已撤回

- 新增过 `EIEM_UPSTREAM_MESH_BOUNDARY_BUILD` 构建开关。
- 在 `FAssetProxyHandle.Get`、`FAssetProxyUntrackedHandle.Get` 和
  `FAssetProxyLoaderHandle.Get` 返回 `UnityEngine.Mesh` 时，按资源路径或
  Mesh 逻辑名匹配 `Render` 规则，并调用现有 Mesh payload 生成器后返回生成
  对象。没有写 Renderer、bones、GPU 缓冲或 flag。
- 在 `SubMeshInfo.get_mesh` 增加同样的替换入口，覆盖 NPC LOD 描述器链路。
- 该版本关闭过渲染器阶段的 Mesh setter；材质、形状和 `handling=skip` 仍走原有
  生命周期边界。普通基线 DLL 仍关闭上游开关。
- 构建产物：
  `analysis/upstream-mesh-boundary-20260926/eiem-upstream-mesh-boundary.dll`
  （SHA-256 `1626C27EDCDF9D16C6E99F409E650A2E12EEE6464AD24F3D106CCFC2B7435229`）。

## 2026-09-26 创建前边界取证

旧文档尾部声称“上游入口已经接入”是不准确的。上述 `UPSTREAM-MESH`
getter 实验已撤回，运行时开关恒为 `false`；生产逻辑仍自行创建 Mesh，
并在已有 Renderer 上赋给 `sharedMesh`。F10 仍重放现存实例，未使原生资源
解码缓存失效。

离线 IDA 发现 `UnityPlayer+0xDA400` 接收 native 对象和读取流，逐项读取
Mesh 数据，并调用后续数据处理；`+0x47F140` 是直接跳板，
`+0x535260 → +0x53ECE0` 则在读取后追加字段。两种入口都位于各自虚表
的 `+0x30` 槽。静态证据尚不能证明 Typhoea 的源 Mesh 实际经过这些入口，
也不能确定流中的资源身份或可替换的数据格式。分析输入
`UnityPlayer.dll` 与游戏目录当前 DLL 的 SHA-256 相同：
`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`。

已新增编译开关 `EIEM_NATIVE_MESH_DESERIALIZE_TRACE_BUILD` 的只读探针。
它最多暂存 16,384 次 `+0xDA400` 调用，只在 Render 规则实际命中源 Mesh
时，以 native 指针回查并输出资源身份、流指针/游标、调用链和字段快照。
本诊断构建的总日志硬上限为 16 MiB。
普通 DLL 不安装该 Hook。此探针只验证目标是否走该边界，不替换 payload，
也不宣称修复躺地。若目标未命中且记录未溢出，就回到上一级资源解码链
寻找实际入口；若命中，再研究流的所有权、格式与事务回滚，之后才做
创建前替换和 F10 原生资源重建。

诊断 DLL 已在游戏退出时部署至 `plugin/eiem.dll`，SHA-256 为
`053B7D501D0FD245723902A9447F58BB7040AC3921D3EC5EDD71DADB332C0D57`。
部署前 DLL 保存在
`analysis/native-mesh-deserialize-trace-20260926/eiem-before-deserialize-trace.dll`。
下一次验证只需进同一场景，记录 Typhoea 的冷启动姿态，手动按一次 F10，
记录姿态后退出；无需 F12 或重复按键。读取 `plugin/eiem_log.txt` 中的
`MESH-DESERIALIZE-v1` 行，并检查 `meshDeserializeStatus=0`。

### 首轮运行结果

日志保存在 `analysis/native-mesh-deserialize-trace-20260926/eiem-native-mesh-deserialize-run.log`。
Hook 安装成功（`meshDeserializeStatus=0`）。冷启动命中 Render 规则时，
`+0xDA400` 已记录 7,238–7,326 次调用；Typhoea body/cloth01/cloth02
的 LOD0–LOD3 共 12 个源 Mesh 均为 `match=none`，且 `truncated=0`。
一次 F10 后调用总数为 9,566，12 个源 Mesh 仍全部未匹配；各源 Mesh 的
managed/native 指针与 F10 前相同。这证明当前 F10 重放没有重新创建这些
源 Mesh。未匹配尚不能区分“源 Mesh 早于 Hook 安装便已反序列化”和“源
Mesh 走了另一条读取路径”。因此不能把 `+0xDA400` 直接用作替换入口。
用户报告本轮 cloth01 冷启动直立、一次 F10 后仍直立；这轮不是躺地
复现，不能用它推断姿态异常的直接成因。
游戏退出后已恢复部署前 DLL，SHA-256
`18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。

### 下一轮针对性判别

因为首轮无法区分“太早加载”和“另一路径”，只读探针现同时暂存
`+0xD9E70` Mesh 构造记录，并在命中规则的源 Mesh 出现时用同一 native
指针回查。若构造命中而反序列化不命中，就沿该构造调用栈找另一条载荷
路径；两者都不命中，则优先查插件 Hook 安装前的预加载/缓存，再决定是
提前安装还是经游戏资源管理器触发真正的卸载与重建。两项记录均限 16,384
条，总日志仍限 16 MiB，不改原生对象。构建通过，已部署 v3 诊断 DLL，
SHA-256 `4D7BFF33CE65DAC4895CB90104DAC31ECED348BEC7A649AA9D64E817C39528DC`。
首轮日志已保存，部署前 DLL 仍可从上述备份恢复。

### 2026-09-26 冷启动躺地样本与构造后路径

用户报告本轮 Typhoea cloth01 **冷启动即 T pose 躺地**，未以 F10 作为触发条件。
只读 v3 日志保存在
`analysis/native-mesh-deserialize-trace-20260926/eiem-native-mesh-deserialize-v3-ground-run.log`。
body、cloth01、cloth02 的 LOD0–LOD3 共 12 个源 Mesh，均以相同 native 指针
命中 `UnityPlayer+0xD9E70` 构造，构造调用栈从近到远为
`+0x1335E4 ← +0x133181 ← +0x131F9F ← +0x132A7C ← +0x130DBC ← +0x1C39FF`。
同时全部未命中 `+0xDA400`；两个 Hook 均安装成功，暂存记录未溢出。
所以目标源 Mesh 不是探针安装前就已创建；`+0xDA400` 也不是这批目标的
实际读取入口。当前仍无法据此确定姿态异常的直接原因。

离线 IDA 导出在
`analysis/unityplayer-20260923/UnityPlayer.typhoea-mesh-factory-stack.ida.txt`
和 `UnityPlayer.typhoea-mesh-payload-path.ida.txt`。`+0x1330F0` 经通用工厂
创建对象；`+0x303E20` 将对象插入哈希表，不能作为 Mesh payload 入口。
`+0x133800` 处理后续字节流并经对象虚表分派；Mesh 源对象的虚表
`+0x78` 槽通向 `+0x170A12C`，再通向包含 `m_BindPose`、`m_IndexBuffer`、
`m_BonesPerVertex` 等字段访问的 `+0x1706F04`；`+0x90` 槽通向
`+0x4710D0 → +0x1EA2A0`。后两条是否在 Typhoea 的实际加载事务中执行，
以及访问方向是读入还是写出，**尚需以同一 native 指针做运行时关联**。
仅凭虚表和静态伪代码不能把它们当作安全的替换入口。

本轮源 cloth01 `native+0x1C8=4`，EIEM 新建的替换 Mesh 在首次绑定时为 0；
此前直立样本的替换 Mesh 该字段为其他值。它与姿态有相关迹象，尚无写入点
或因果证明，不能复制或硬编码该值。游戏插件 DLL 已恢复诊断前版本，
SHA-256 `18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。

下一步只读验证限于上述字段访问入口，记录同一 native 指针的命中、执行顺序
及对象前后状态；确认真实读入边界后，再设计完整 Mesh payload 替换与
F10 原生资源缓存失效。当前没有实现上游替换，也未达到稳定性验收门槛。

已实现只读 v4 探针：在 `+0x1706F04` 与 `+0x1EA2A0` 返回后，各记录最多
16,384 次调用；仅当 Render 规则命中源 Mesh 时，按同一 native 指针输出
`MESH-PAYLOAD-PATH-v1` 对照行。它不修改 Mesh 或字节流。已编译并部署到
`plugin/eiem.dll`，SHA-256
`B3C841F279FCE21BE323EB5560E50BDEA12777C981E296E63C7550FC8F6D8750`；
原基线 DLL 仍在备份中。验证只需一次冷启动，记录 cloth01 姿态并退出，
无需按 F10。若 Hook 安装失败、目标无命中或记录溢出，不能据此选择替换入口。

### 2026-09-26 v4 实测与当前边界

用户确认 v4 是 **冷启动直立、未按 F10、已退出**。完整日志：
`analysis/native-mesh-deserialize-trace-20260926/eiem-native-mesh-payload-v4-run.log`。
两个 Hook 均成功（`status=0`），记录未溢出。Typhoea 的 body、cloth01、
cloth02 各 LOD0–LOD3 共 12 个源 Mesh，都以与构造及后续 Render 规则命中时
相同的 native 指针命中 `UnityPlayer+0x1EA2A0`；返回地址都是 `+0x133ADE`。
`+0x1706F04` 调用数为 0，旧候选 `+0xDA400` 对这 12 个对象仍无命中。
因此 `+0x1EA2A0` 是这批源 Mesh 的实际二进制读取路径；**命中读取函数
不等于已找到可安全替换的载荷格式或目标识别点**。

IDA 显示 `+0x1EA2A0` 从流逐项读入 native Mesh，末尾通过
`+0xF50A0C(stream, native+0x1C8)` 写入 4 字节。命名字段访问路径把同一
偏移称作 `m_BonesPerVertex`。v4 直立样本的源 cloth01 LOD0 在读取前为
260638、读取后为 4；其余 11 个源 Mesh 读取后也为 4。读取前的值不能
解释为 flag。现有替换 Mesh 的 `+0x1C8` 在 v4 直立样本为 910569797，
v3 躺地样本为 0。`UnityPlayer+0xC7B750` 最多取该字段的 4，分别得到
4 与 0，再组成绘制记录。这个差异与姿态强相关，但尚未证明它是唯一
原因；不能硬编码 4，也不能把派生的 16/48/52 写进对象。

`+0x1EA2A0` 开头调用 `+0xB4160(native+0x30, stream)`，其伪代码从流
读取字符串再存入对象。下一步需确认该字符串是否为 Mesh 逻辑身份，并
记录同一次读取的流起止、字段位置和资源所有权。只有能按完整资源身份
匹配且能生成游戏接受的整份载荷时，才可在读入前替换；失败时整笔事务
保持原始资源。F10 目前只重放已缓存实例，没有触发这 12 个源 Mesh 的
重新读取；热重载仍需单独找到原生缓存失效及重建入口。

这次验证结束后游戏 `plugin/eiem.dll` 已恢复基线，SHA-256 为
`18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。
当前未实现上游载荷替换，稳定修复与手动 100 次 F10 验收均未完成。

### v5：读取期资源身份与流区间

基线 DLL 下的一次补充冷启动由用户确认 cloth01 直立、游戏已退出；
这不能充当上游修复验证。现将同一只读 `+0x1EA2A0` 探针扩展为：
在匹配的 12 个源 Mesh 上输出读取后的 `native+0x30` 名称指针及可读
ASCII 前缀、流对象 `+0x30/+0x38/+0x40` 在读取前后的原始值。
`MESH-PAYLOAD-IDENTITY-v1` 只在 Render 规则已按 managed/native 同一
指针确认源 Mesh 时输出；不转储全局流、不修改资源。新增字段先验证
字符串是否足以匹配目标及流跨度是否可界定，不能直接推定三个偏移的
具体语义。

v5 已编译并在游戏退出后部署到 `plugin/eiem.dll`，SHA-256
`F1CDED2CDD8CA0009B86D76956362FCDB6027C399DD1421AEAC4B55935F7EAD5`；
版本副本为 `analysis/native-mesh-deserialize-trace-20260926/`
`eiem-native-mesh-deserialize-trace-v5.dll`。基线备份 SHA-256 仍为
`18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。
下一次只需冷启动进入相同场景、确认 Typhoea 姿态后退出；无需 F10/F12。
读取日志后恢复基线并决定是否能进入载荷格式研究。

### v5 结果：冷启动躺地

用户确认 v5 一次冷启动 cloth01 躺地，未按 F10，游戏已退出。日志保存为
`analysis/native-mesh-deserialize-trace-20260926/eiem-native-mesh-payload-v5-cold-ground-run.log`
（135,298 字节）。两个 Hook 均成功，12 个目标源 Mesh 全部命中
`+0x1EA2A0`，记录未溢出。每个对象的 `native+0x30` 字符串都与
Render 规则中的 Mesh asset 名称逐字一致，包括 cloth01 LOD0 的
`S_actor_typhoea_cloth_01_lod0`。因此这条读取链在读完名称时已有
可用的逻辑 Mesh 名；但同名不证明资源路径相同，正式匹配仍需绑定
asset 记录及来源。

异常态中源 cloth01 LOD0 的 `native+0x1C8` 在该读取前为未初始化值，
读取后为 4，后续 Render 绑定时仍为 4；EIEM 新建替换 Mesh 绑定时
却为 0。正常态 v4 替换 Mesh 同一字段为 910569797。这个对照说明
现有公开 setter 并未建立稳定的原生 `m_BonesPerVertex`，与躺地强相关，
但不能据此宣称已证明完整因果或只改一个字段即可修复。

同一次 `+0x1EA2A0` 读取前后，流对象 `+0x30` 的指针及 `+0x38/+0x40`
所指范围均发生跳变。例如 cloth01 LOD0 的 `+0x30` 从
`0x2515B4280` 变为 `0x251C66AB8`。IDA 的 4 字节读取辅助函数在
超出当前 `+0x40` 时转入补充缓冲路径。因而这不是可以按一次连续
内存区间直接覆盖的载荷。下一步应先确认流补充、资产边界和完整
序列化格式，再设计游戏读取前的替换及 F10 的资源重建。

补充 IDA 导出：`analysis/unityplayer-20260923/UnityPlayer.mesh-stream-boundary.ida.txt`
（脚本 `tools/ida_trace_mesh_stream_boundary.py`）。`+0xB4900` 在 Mesh
读取开头从流读取长度及同长度名称字节；`+0xB4160` 把结果存入
`native+0x30`。`+0xF50A0C` 将流 `+0x30` 当当前指针、`+0x40` 当
缓冲末端；不足四字节时调用 `+0x133D40` 跨缓冲读取。
`+0x133D40` 又调用 `+0x1341A0` 补充流缓冲，解释了 v5 的地址跳变。
由此可知**名称读取后**可以识别逻辑 Mesh，但完整载荷超过当前
缓冲区；不能修改现有缓冲的一小段就期望几何、索引和权重保持一致。

异常日志已归档，游戏 `plugin/eiem.dll` 已恢复基线 SHA-256
`18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。
当前没有部署实验性修改，也没有达到修复验收标准。

### 离线原始对象核对（2026-09-26）

用户确认 Typhoea cloth01 来自 AnimeStudio PFB 导出，并给出其索引身份：
`assets/beyond/arts/entity/actor/loli/typhoea/models/s_actor_typhoea_cloth_01_lod0.asset`，
Bundle `Bundles/Windows/main/fe9aca3b9ad63ab257f8236a.ab`，
PathID `4411933801208500242`。工作目录实际位于 `E:\EIEM_Workspace`。
其 `index/endfield_assets.eidx` 中还有同名但不同身份的
`sk_actor_typhoea_01.fbx` Mesh（PathID `9211237107134140270`）；
两者顶点数和 bind pose 数都相同，但原始对象大小与顶点布局不同。
因此按名称、顶点数或骨骼数匹配都不足以保证替换的是 PFB 所引用的对象。

从工作目录缓存的原 Bundle 按 PathID 只读提取了 cloth01 LOD0 的
2,291,768 字节原始序列化对象。AnimeStudio 以 Endfield 2021.3.34f5
格式完整解析，读取位置恰好到对象末尾：23,758 顶点、1 个 submesh、
120 个 bind pose、16 位索引、14 个通道、3 个顶点流，
`m_BonesPerVertex=4`，`m_VariableBoneCountWeights` 数组为空。
区段映射保存于 `analysis/native-mesh-deserialize-trace-20260926/`
`typhoea-cloth01-lod0.source-object.txt`；原始字节同目录 `.bin`。
body/cloth02 的独立 `.asset` 对象也完成同样的完整解析，末尾字段都为 4；
其对象大小分别为 48,836 和 508,456 字节，顶点数分别为 917 和 10,568。
游戏目录的 `plugin/eiem.dll` 仍是基线 SHA-256
`18B7AA697136E29F7B2BAE0C5E3FF1008911C60CB32316EA6B14EA10D9885D11`。

当前 EIEMESH v6 cloth01 替换件为 51,677 顶点、8 个 submesh、
248,142 个索引、122 个 bind pose，UV0/UV2 各 2 分量，最大顶点索引
51,676，最大骨骼索引 121。原对象只有 120 个 bind pose，说明上游
Mesh 数据替换还必须与新增骨骼的 Renderer palette 接入配合；仅改变
Mesh 序列化对象不能凭空扩充原生 Renderer 的 `bones[]`。

下一步先构造**离线**原生 Mesh 序列化写出器：以原对象为模板，重建
submesh/AABB、bind pose 与骨骼散列、每骨骼 AABB、索引缓冲和 3 个
顶点流，保留未知及非目标字段；按替换权重推导每顶点骨骼数。先要求
原对象无修改写出后字节完全相同，再要求替换对象被同一 AnimeStudio
解析器完整读取，所有索引/骨骼范围有效。之后才研究游戏读入前的
对象身份传递和完整载荷提交；`+0x1EA2A0` 的名称本身不足以识别
上述两个同名资产。F10 仍需独立的原生资源缓存失效与实例重建入口。
