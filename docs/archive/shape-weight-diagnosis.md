# 形态键动作异常：只读诊断与证据

> 状态：历史档案。这是 v44～v46 的根因与修正时间线。v44/v45 临时探针已移除；当前通道归属见[形态键契约](../shape-controls.md)。

日期：2026-09-06。当前诊断构建标记：`resource-runtime-v45-native-shape-observation`。
v44 的实机结果和后续静态分析保留在下方历史记录中，不能与 v45 的观测覆盖混用。
目的：建立“动作发生 → 实际权重是否变化 → 哪条可观测调用路径写入”的反馈链。
本版不是形态键修复，不锁零、不添加控制规则、不修改作者资产。

## 已知事实与未验证部分

- 用户观察到身体模型扭腰时局部发生变形，动作结束后恢复；部署前尚无逐时刻权重证据。
  2026-09-06 的 v44 采集已记录实际权重变化；后续静态分析识别出具体游戏写入路径，
  v45 又捕获该路径写入目标 Renderer 的实机调用栈，并与后续 getter 读数对应，详见文末。
  **意外权重变化的直接写入来源已经确认，行为修复尚未实施。**
- 当前 Mod 没有 `shape.*` 控制。无配置且未拥有通道时，现有权重执行器直接返回。
- 离线原生身体 Mesh 的大世界、UI 导出副本均为 917 顶点、0 个形态键通道/帧。
  作者新增的形态键不能直接解释成原游戏同名形态键的正常动画。
- API/模拟原生接口测试不代表游戏自定义 GPU 管线验收。
- 不能仅凭画面确定 BlendShape 权重变化；骨骼、姿态修正、自定义 GPU 变形也会改变外观。

## 采集方式

独立输出 `plugin/eiem_shape_probe.txt`，每次进程启动重新创建；复现后先保存日志再重启。
当前记录使用 `[DEBUG-shape-v45]`，含序号、毫秒时间、线程、配置代数。

1. 只从现有 Render 命中登记和 partner 列表发现有形态键的 SMR，不全场景扫描，不按角色名称硬编码。
2. 主窗口线程的 50 ms 定时器读取真实 sharedMesh、通道名称、权重。首次、布局变化、权重变化、
   读取错误恢复时输出快照；不变时每两秒输出读数累计心跳。
3. 用运行时 `il2cpp_resolve_icall` 解析完整的 `SkinnedMeshRenderer::SetBlendShapeWeight(System.Int32,System.Single)`，
   观察原生 icall，三个参数为 managed Renderer、int、float，没有 MethodInfo 尾参。
   不再保留 v44 的公开包装层钩子。原参数、原函数调用次数及异常传播不变。
4. 钩子只对已观察 Renderer 记录原始索引、请求值、调用起止时间、线程、配置代数、调用栈。
   不在钩子内调用 Unity API 或写文件；有界队列交给主线程输出模块名和相对地址。
5. EIEM 自己的形态键写入使用线程局部作用域标记 `origin=EIEM`；其余标记 `external`。
   `external` 不代表一定是游戏 Animator，也可能是其他原生或插件调用方。
   另以运行时元数据解析 `Beyond.Gameplay.Core.SkeletalMorphCore._ApplyShaderPropDataToRenderers():void`，
   在该函数原样执行期间标记 `core` / `coreDepth`。这是同步调用来源，不是资源所有权判断。
   嵌套调用、C++ 异常退出恢复前一作用域，其他线程不继承该标记。core 地址只作为数值记录，之后不解引用。
6. 观察引用为弱引用，对象死亡后移除；递增 ticket 区分相同地址的不同生命周期。
   F10 打开新的日志额度、清除诊断去重基线，不改变任何资源恢复决策。

不需要修改 INI，不需要增加 UI。Mod 正常启用后复现一次动作，记录发生的大概时间；
可额外在复现前按一次 F10 形成 `RELOAD checkpoint` 时间标记，但不是采集启动的必要步骤。

## 怎样读记录

- `START nativeSetterHook=1 coreHook=1`：两个观察入口安装成功，并记录解析后的模块相对地址。
  `HOOK_ERROR` 或任一标志为 0 表示对应覆盖缺失；不回退到旧包装层。直接存储/GPU 写入仍可能绕过 icall。
- `WATCH`：观察已实际开始，包含 Renderer、ticket、规则和层级路径。
- `SOURCE_LAYOUT`：原 Mesh 的通道布局，不冒充替换前的运行时权重快照。
- `SAMPLE` / `WEIGHT`：采样时读到的实际权重；零也是有效观测，不用“没有日志”代表零。
- `SET_RETURNED`：原 setter 返回后的请求记录，**不是写入成功或最终权重的证明**；
  原始索引不自动套用之后才读取到的 Mesh 布局。
  `coreDepth>0` 表示调用处于被观察的 Apply 函数同步调用链内；仍应核对 stack 与后续 WEIGHT。
- `HEARTBEAT`：确认仍在持续读；`watches=0` 表示没有观察到符合条件的消费者，不能推断权重不变。
  `nativeCalls` / `coreCalls` 是探针启用期间两个入口的全部进入次数，含未被观察的对象和抛异常调用；
  `setterCalls` 仍仅统计队列取出的目标成功返回请求，三者不能直接互作等价比较。
- `READ_ERROR`、`LAYOUT_ERROR`：读取失败/接口缺失/超过诊断上限，值未知，不能补零。
- `OVERFLOW`、`TARGET_LIMIT`、`LIMIT`：覆盖不完整，不能用事件缺席排除某个写入来源。

可证伪的区分：

- 若 `WEIGHT` 变化且有对应 setter 记录，根据调用栈追踪该写入方。
- 若 `WEIGHT` 变化但没有对应 setter 记录，可能绕过当前 icall 或超出采集覆盖，不能断言无人写入。
- 若动作异常期间持续读到权重不变，下一步区分蒙皮/姿态修正/GPU 变形；
  50 ms 采样仍可能漏掉帧内短暂变化，不能证明所有渲染阶段都没使用过其他权重。

## 覆盖边界与资源成本

- 定时采样不是逐帧同步，也不是 GPU 读回；窗口消息繁忙时实际间隔可能更长。
- 第一份 WATCH 之前的写入不被记录。原 Mesh 在第一次采样前可能已经替换。
- 最多观察 128 个消费者，每个 Mesh 最多读取 256 通道；调用队列 2048 条，满时显式计丢失。
- 每次启动/F10 最多 20000 条详细日志，达到上限会明确暂停输出。
- 同一调用点/通道的相同请求值去重；心跳的 setterCalls 是已排队且取出的调用累计，不是全引擎统计。
- 任意形态键的数据或命名均不影响观察规则；没有模型、材质或角色白名单。

## 测试与后续清理

`tests/test_shape_probe.py` 编译并执行真实观察头文件并链接实际 MinHook，使用 Inflate/Sleeve 测试形态键：
覆盖参数直通、异常传播、只读采样、后台线程不调用 Unity、EIEM/外部来源、通道重排、
无 setter 的数值变化、读取失败与恢复、partner、地址复用、队列溢出、F10、销毁，
以及直接原生调用、嵌套 core、跨线程无上下文继承、core/原生 setter 异常、入口缺失和重复 Hook 不破坏已有 Hook。
这些测试验证诊断器，不宣称复现用户的游戏动作。

诊断结束后删除 `src/eiem_shape_probe.h`、`src/eiem_shape_probe_state.h`，及以下接入点：
`il2cpp_trace.h` 的 include、WriterScope、Checkpoint、InitShapeProbe；
`trojan.h` 的 EnsureTimer/Stop；相关测试 fixture stub。
保留最终证据与根因记录，不把临时探针变成资源加载或形态键控制的另一条实现路径。

## v44 验证与部署（历史）

- 2026-09-06：全量 91 项测试通过，包含独立后台 Blender，未跳过；`build.bat` 编译成功。
- `git diff --check` 通过。未修改作者模型、INI 或 Blender 场景，未提交 Git。
- 确认游戏退出后部署 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。
  SHA256：`5EFEEC02973C3F74E2B1A75B5CA079136FC382D0014BF6F6924DBD5E97F01BCB`。
- 旧 v43 DLL、部署前日志、新 DLL/PDB、诊断源文件存于
  `E:\EIEM_Workspace\plugin-releases\v44-shape-observation-20260906-175212`。
- 对 typhoeus Mod 全部文件及全局配置共 6 个文件核对部署前后哈希，均未改变。
- 部署时尚未采集此版本游戏内的扭腰动作；不能把诊断器测试通过写成根因已修复。

## 2026-09-06：重启后的实机采集结果

已保存完整会话至 `E:\EIEM_Workspace\shape-diagnostics\v44-20260906-182103`，
复制后核对源文件与副本 SHA256 一致：

- `eiem_shape_probe.txt`：`F6CAF2EE5E43777EF764AC46D52EF4AAB69548410CC6064F3DA206DDD2C44498`。
- `eiem_log.txt`：`712F8F0CBC728788BED9ED049E2EEE10E06A1732238E2D0ADFE59D2652DAF10E`。

实际观测：

1. 启动日志确认 v44 DLL，`setterHook=1`、`fileOK=1`。目标规则为
   `RenderS_actor_typhoea_body_01_lod0_0`，`WATCH ticket=1` 对应大世界实例。
2. `SOURCE_LAYOUT readable=1 channels=0`：被记录为源 Mesh 的运行时对象没有形态键通道。
   替换 Mesh 有两个通道，首次采样均为 0。
3. 目标始终为同一 Renderer/Mesh/ticket；`gen=1`，无 `RELOAD`。
   `layoutChanged=1` 仅首次出现一次，之后布局未发生被采样捕获的变化。
4. 最后心跳为 `reads=20907 setterCalls=0`。共有 450 份 `SAMPLE`、900 条 `WEIGHT`，
   两个通道各记录 450 个值，范围均为 0～100。出现多次升高后回落，不是仅首次初始化。
5. `SET_RETURNED`、`READ_ERROR`、`LAYOUT_ERROR`、`OBSERVER_ERROR`、`OVERFLOW`、
   `TARGET_LIMIT`、`LIMIT` 均为 0；会话有 `STOP`，归档时游戏进程已退出。

第一次捕获的明显变化（时间为日志单调毫秒时钟，不是墙钟，也不是逐帧采样）：

| ms | 通道 0 | 通道 1 |
| --- | ---: | ---: |
| 16198296 | 2.8592 | 2.8592 |
| 16198359 | 43.2490 | 42.4162 |
| 16198421 | 99.9647 | 99.9497 |
| 16198484 | 91.2774 | 91.2774 |
| 16198546 | 32.4618 | 32.4618 |
| 16198609 | 2.9962 | 2.9962 |
| 16198734 | 0.0012 | 0.0012 |

结论边界：

- 已确认 Unity getter 读到的实际形态键权重发生变化。不能再仅以“骨骼蒙皮造成视觉误判”解释本次观测。
- 原 Mesh 无通道，不能称为“正常继承原 Mesh 同名形态键动画”。这也不等于排除了更上层动画绑定或姿态控制器。
- 公开 setter 包装层未记录到目标的写入。具体写入可能绕过该入口，或该观测入口覆盖不足；
  尚无写入调用栈，不能确定是 Animator、自定义姿态修正、其他原生调用方或错误内存写入。
- 两个通道的读取不是原子快照，不能据小量数值差断言存在不同动画曲线。
- 日志没有动作时间标记或同步视频；它证明反复的数值变化，但不能给每次峰值指定某个动作。
- 本轮只归档和分析证据，未锁定权重、修改 Mod/INI/作者模型或再次部署 DLL；根因与修复均未完成。

下一次定位应先确定实际权重存储与原生写入入口，核验公开包装层的覆盖范围，再取得写入调用栈。
验证使用普通测试形态键；不能用每帧写零掩盖写入来源，也不能仅凭 `setterCalls=0` 宣布游戏没有更新权重。

## 2026-09-06：后续静态定位（未改 DLL）

用户要求继续调查。本节静态分析阶段只读取源码、已归档日志、磁盘 PE 与元数据；
新增离线分析工具和本记录，未附加游戏进程，未执行游戏 DLL，未修改模型、INI、Blender 场景或已部署插件。

### 候选与验证结果

调查前按以下顺序提出可证伪假设：

1. 游戏动画/姿态路径绕过公开 setter：预测存在直接访问同一原生权重存储的调用路径。
2. EIEM 另有赋值路径：预测可在插件中找到绕过 `EiemUnityShapes::Write` 的直接形态权重赋值。
3. getter 接口/解包错误：预测接口签名、参数转发或返回值读取与实际实现不一致。

结果：第一项有直接静态证据。当前源码中未找到第二条直接形态权重赋值路径；
这不等于证明插件其他骨骼/表情功能不会间接影响游戏的上游计算。
getter 的元数据、包装层和原生候选实现均与“Renderer + int 索引 → float”相符，第三项未获得支持。

### 游戏函数已识别，不是凭名字猜测

解析 `global-metadata.dat` 的类型/方法所属关系与方法 token，再核对 `Gameplay.Beyond.dll`
的 `Il2CppCodeGenModule` 方法指针表，得到以下当前文件 RVA：

| 类/方法 | GameAssembly RVA | 确认内容 |
| --- | --- | --- |
| `SkeletalMorphCore._InitializeBlendShapeResources` | `0x37AB990` | 建立形态键目标列表 |
| `SkeletalMorphCore.Update` | `0x3167900` | `0x3168B2F` 调用下一行函数 |
| `SkeletalMorphCore._ApplyShaderPropDataToRenderers` | `0x31661C0` | 包含形态键权重写入，不只是 Shader 参数 |
| `SkinnedMeshRenderer.SetBlendShapeWeight` 包装层 | `0x4439810` | v44 观察的公开入口 |
| `SkinnedMeshRenderer.GetBlendShapeWeight` 包装层 | `0x4486500` | getter 包装层 |

归档日志第 6029 行起的真实 `SkeletalMorphCore` 字段枚举同时确认：

- `0x38`：`m_animator`。
- `0x170`：`m_eyebrowBSmesh`。
- `0x198` / `0x1A8`：`m_dirtyEyebrowBSmesh` / `m_dirtyEyebrowBSmeshDoubleBuffer`。
- `0x410`：`m_enableCrossFrameTick`。

这些偏移只用于核对本次反汇编，不写进运行时加载/替换代码。

### 初始化筛选与数字索引写入

`_InitializeBlendShapeResources` 中可观察到：

1. `0x37ABDE4` 写入新建列表至 `this+0x170`。
2. `0x37ABE33` 起从 `m_animator` 获取组件集合并遍历。
3. 对每个候选检查对象与 sharedMesh 是否存在，然后读取 `Mesh.get_blendShapeCount()`。
4. `0x37ABFC4` / `0x37ABFC6`：数量不大于零就跳过。
5. 数量大于零则在 `0x37ABFCC`～`0x37ABFE6` 将该 Renderer 加入 `m_eyebrowBSmesh`。
   这段加入逻辑没有按新通道名称、原 Mesh 通道布局或材质筛选。

`_ApplyShaderPropDataToRenderers` 中可观察到：

1. 根据 `m_enableCrossFrameTick` 选择普通或双缓冲的 dirty 表。
2. 从 dirty 表取得数字索引和权重；遍历 `m_eyebrowBSmesh`。
3. `0x316672E`～`0x3166736`：仅以当前 Mesh 的通道数检查该数字索引是否可用。
4. 权重限定到 0～1，再乘以常量 100；与旧值差大于常量约 0.01 时写入。
5. `0x31667CC` 解析 `UnityEngine.SkinnedMeshRenderer::SetBlendShapeWeight(System.Int32,System.Single)`；
   `0x31667F1` 经缓存的 icall 指针直接调用。没有经过 `GameAssembly+0x4439810`。

这确立了一个具体的冲突机制：**新增形态键改变了游戏初始化时的目标筛选结果，而游戏后续仍按自己的数字索引驱动这些目标。**
例如原先零通道的普通部件增加 `Inflate` / `Sleeve` 后，若替换发生在上述初始化之前，
便满足“有形态键”的收集条件。游戏索引 0/1 的值可能被写到这两个不相关的新增通道，
不要求同名，也不要求 Mod 声明 UI 或 `shape.*`。

这与 v44 两个通道 0～100 反复变化、公开 setter 没有事件的记录高度吻合。
**尚未实机确认目标就在该列表内，也未取得该函数写入目标时的调用栈，不能把机制吻合写成根因验收完成。**
日志没有动作同步标记，尤其不能断言“扭腰本身驱动了眉部”；动作期间可能伴随其他表情更新。

### 原生存储与观测盲点

当前 UnityPlayer 的名称表与平行函数表提供如下候选；反汇编语义与 getter/setter 一致：

- getter icall 候选 `0x4F4EA0` 转到 `0x4F4F60`。
- setter icall 候选 `0x3CA6F0` 转到 `0x3CA7C0`。
- 两者使用原生 Renderer 的 `+0x330` 数组指针和 `+0x340` 长度；getter 按索引读 float，setter 写对应 float。
- UnityPlayer 另有属性访问 thunk 直接转到原生 getter/setter，同样不经过 GameAssembly 包装层。

这些是磁盘静态地址，不是本次运行时 `il2cpp_resolve_icall` 的回读结果，不能直接硬编码拿去 Hook。
v44 的观测边界现已明确：只观察公开包装层，不足以覆盖编译后直接使用 icall 的游戏方法。

### 剩余验证与修复边界

剩余最小证据是：在原生 icall 的只读观测中捕获目标 Renderer 的一次写入及其调用栈，
核对调用方是否为 `_ApplyShaderPropDataToRenderers`，并在 Unity 线程核对目标列表成员关系。
原生入口须由当前运行时解析，不使用本记录中的地址作为跨版本契约。

该静态分析阶段未实施新探针或行为修复；后续 v45 探针见文末。若得到实机确认，修复应区分原生通道和作者新增通道的控制归属：

- 不因“作者新增通道编号也为 0/1”就接受游戏原生通道的更新。
- 不关闭整个人物的 `SkeletalMorphCore.Update` 或整个 `_ApplyShaderPropDataToRenderers`，避免破坏原表情、骨骼与 Shader 更新。
- 不强制所有通道每帧归零；作者控制和原生表情都必须保留各自的正常用途。
- 不改全局眉部资源、作者模型或特定角色名单来绕过问题。
- 使用普通测试形态键验证调用路径与生命周期，不能仅靠离线模拟宣称游戏回归通过。

### 工具、资料与复现证据

本次新增的 `tools/diagnostics/inspect_native_shapes.py` 只读 PE，输出字符串、指令、候选引用及 unwind 范围；
候选引用来自字节扫描，不声明完整调用图或可靠函数边界。
`inspect_il2cpp_type.py` 核对所属程序集、方法 owner/token、指针表范围；本文件的 v29 类型记录为 92 字节，
需显式 `--type-stride 92`。标准 88 字节布局被拒绝，不静默猜测；新增四字节字段的语义未猜定。
基础结构参考 [Il2CppDumper 元数据定义](https://github.com/Perfare/Il2CppDumper/blob/master/Il2CppDumper/Il2Cpp/MetadataClass.cs)
和 [代码生成模块定义](https://github.com/Perfare/Il2CppDumper/blob/master/Il2CppDumper/Il2Cpp/Il2CppClass.cs)，
以本地方法 token/地址和真实字段日志交叉核验，而非仅按参考格式认定游戏实现。

`python tools/diagnostics/test_static_shape_diagnostics.py -v`：6 项中性夹具测试通过，
覆盖两种显式布局、错误版本/布局、方法 owner/token、错误程序集、命名空间及无 PE 时不伪造地址。
它们验证离线工具，不是异常动作的游戏回归测试。该静态分析阶段没有重编译或部署 DLL。

上述采集目录新增：`skeletal-morph-core-methods.json`、`static-shape-chain.json`、
`shape-resources-initializer.json`、`game-direct-shape-writer.json`、`native-shape-storage.json` 等只读分析输出。
对应磁盘输入 SHA256：

- GameAssembly.dll：`C24495E51B406F03B03890C4788EE618AE022C991405BE5D5B8B787CB775AE89`。
- UnityPlayer.dll：`BEE7BE52370ADDDD67BA61E4937CA51B7F272656841D187E95E505496DA798D1`。
- global-metadata.dat：`0076743397ACADF03D3B0064343A963C7C88863B8160526D397E4B3EFB96F02E`。

## 2026-09-06：v45 原生来源探针

基于 v44 的“getter 变化、包装层无事件”和已定位的直接 icall 写入链，替换观察入口，
不是增加另一个形态键执行器。运行时解析 icall 与 Apply 方法，不使用上述磁盘 RVA 或对象字段偏移。
不读取/修改游戏内部列表，不拦截赋值，不每帧写零，不修改模型、INI 或 Blender 工程。

中性测试通过真实 MinHook 复现 `Core -> 原生 setter` 调用，而不是直接调用探针来伪装 Hook 成功。
这验证参数转发、作用域和探针只读性，**不等于确认用户目标已被这条调用链写入**。
实机仍需捕获目标 ticket 的 `SET_RETURNED`、`coreDepth>0`、GameAssembly 调用栈与权重采样。
如 core 标记为空，先核对入口安装、调用线程和栈，不以缺席推断权重没有写入。

### v45 验证与部署

- 全量 `unittest discover -s tests -q`：91 项通过，含后台 Blender，无跳过。
  `tools/diagnostics/test_static_shape_diagnostics.py`：另 6 项通过。
- `build.bat` 构建成功。最初在已初始化 MSVC 的 shell 再调用该脚本，触发本机 PATH 引号的二次初始化错误；
  改为直接运行脚本自己的初始化流程后通过，未为此修改构建脚本或系统环境。
- 确认游戏进程退出后部署 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，
  SHA256：`26E99A6CA09441A3352E789109B5A273334DE74D62FCEABE3650812F30B28151`。
- 旧 v44 DLL、旧日志、新 DLL/PDB、诊断源文件与测试归档至
  `E:\EIEM_Workspace\plugin-releases\v45-native-shape-observation-20260906-190232`。
- 对全局配置及 typhoeus Mod 共 6 个文件核对前后 SHA256，均未改变；未更新加载器代理，未提交 Git。
  IDE 中的旧 `typhoea_mesh_material_test` 目录现已不存在，没有重建它。
- 下一次启动才会加载 v45 DLL，F10 不替换进程中的 DLL。启用现有 Mod 并复现一次异常动作即可采集，
  不需要新增 UI 或 shape 配置。

**当前仅完成诊断器验证与部署，没有形态键行为修复或游戏画面验收结论。**

## 2026-09-06：v45 重启后的实机结论

用户报告已重启后读取日志。启动标记为 `resource-runtime-v45-native-shape-observation`，
已部署 DLL SHA256 仍为 `26E99A6CA09441A3352E789109B5A273334DE74D62FCEABE3650812F30B28151`。
本轮只采集、核对并记录，不改 DLL、Mod、模型、贴图或 Blender 场景。

日志保存于 `E:\EIEM_Workspace\shape-diagnostics\v45-20260906-190843`。复制操作按可能仍在写入的活动文件处理，
随后确认游戏进程已经退出，副本含 `STOP`、末行完整，源文件与副本 SHA256 相同：

- `eiem_shape_probe.txt`：`5FD2FD471AF51CCFF317592BB1FA8BDEB68050D32ABC714A0DBF7B9FE0598F1A`。
- `eiem_log.txt`：`7169937CB3EAE41D4F091A5782F582B4F0D913563715435F47A1A044EFF7D097`。

### 安装、身份及调用来源

- `START nativeSetterHook=1 coreHook=1 fileOK=1`。运行时解析入口为
  `UnityPlayer+0x3CA6F0` 和 `GameAssembly+0x31661C0`，与上轮磁盘分析相符；不是按该 RVA 安装。
- `seq=29 WATCH ticket=1`：目标规则 `RenderS_actor_typhoea_body_01_lod0_0`，
  Renderer `0000000FCDFB0C20`，替换 Mesh `0000000FCDFCB240`。
- `seq=30 SOURCE_LAYOUT`：原 Mesh `0000000FCDFB02C0`，`readable=1 channels=0`。
  当前替换 Mesh 为 2 通道；整个采集只在首次出现 `layoutChanged=1`，配置始终为 gen=1，无 F10。
- 共 987 条 `SET_RETURNED`，全部 `origin=external`、`coreDepth=1`、
  core=`00000010EEA08000`，第一栈帧全部为 `GameAssembly+0x31667F3`。
  该返回地址正是静态定位 `0x31667F1 call rax` 的下一条指令，属于
  `SkeletalMorphCore._ApplyShaderPropDataToRenderers` 的形态键原生 setter 调用。
- 栈中还包含 EIEM 的透传观察/现有 SMC Update Hook，不能因为有 EIEM 栈帧就判定是 UI 在赋值；
  直接调用点和同步 core 作用域指向游戏 Apply 方法。本 Mod 未声明 `shape.*` 或 UI 块。

### 写入与后续读回相互对应

副本共 1354 条记录：1 START、1 TIMER、56 HEARTBEAT、1 WATCH、1 SOURCE_LAYOUT、
102 SAMPLE、204 WEIGHT、987 SET_RETURNED、1 STOP；无读取错误、溢出、额度上限或重载记录。

按同一 ticket、通道索引、配置代数，将每条 WEIGHT 与其日志序号之前最近的 SET_RETURNED 对应：
202 条读数有前置写入，打印的浮点值全部相同，数值绝对差均为 0，写入完成时间均不晚于读取时间。
其余 2 条是首次零值快照，之前尚无目标写入；不能为它们编造写入事件。
这不是逐 GPU 帧同步证明，但足以确认实际 setter 请求与采样到的权重变化一致。

| 写入 seq → 读回 seq | 通道索引 | 请求值 = 实际读数 | 写入完成 ms → 采样 ms |
| --- | ---: | ---: | --- |
| 75 → 78 | 0 | 97.5427551 | 20526421 → 20526437 |
| 76 → 79 | 1 | 96.5875626 | 20526421 → 20526437 |
| 90 → 93 | 0 | 99.4299774 | 20526484 → 20526500 |
| 91 → 94 | 1 | 99.4299774 | 20526484 → 20526500 |

每个通道各 102 份读数；通道 0 范围 0～99.9972458，通道 1 范围 0～99.9960785。
最后心跳早于最后一次队列排空，`setterCalls=953` 不是最终总数，不能因此与 987 条事件混淆。

### 结论及边界

**已确认本次意外权重变化的直接原因：游戏 SkeletalMorphCore 的 Apply 路径按数字索引，
向替换 Mesh 的新增形态键写入了游戏控制值。** 原 Mesh 无通道，因此不能解释成继承原 Mesh 的正常同名动画。
没有 UI 不代表无人写权重；上一版 `setterCalls=0` 是观察入口漏掉了直接 icall，不是未发生赋值。

静态初始化代码的“只看 blendShapeCount > 0 就加入列表”解释了此冲突如何形成。
本版没有逐事件记录初始化列表的加入时刻，故不声称捕获了整个初始化因果链；
但实际目标已被上述游戏写入路径驱动，不再只是静态候选。
日志未与视频/动作事件对时，不能进一步把上游控制值断言为扭腰、眨眼或某个特定表情。

后续工作的边界是通道控制归属：原生通道与新增通道不能只因数字索引相同而被视为同一对象。
不通过关闭整个 SkeletalMorphCore、每帧强制清零、改作者资产或按角色名称特判掩盖该问题。
本轮完成诊断，不宣称已修复；也未移除仍用于该诊断的探针或提交 Git。

## 2026-09-06：v46 通道归属实现

用户同意后实施。修正对象是 v45 已证明的游戏数字索引与作者新增通道发生碰撞，
不修改作者几何、形态键名称、默认配置、贴图或 Blender 场景。

### 实现边界

- `eiem_shape_binding.h`：每消费者的源名称/顺序 → 目标槽映射、游戏最新要求值和作者控制归属。
  原生通道缺失、重名或源权重无效时明确失败，不回退到索引 0。
- `eiem_shape_guard.h`：成对 Hook 原生权重 getter/setter，在已证实的
  `_ApplyShaderPropDataToRenderers` 同步调用中提供源通道视图；EIEM 自身读写显式使用目标索引。
  不关闭整个 SkeletalMorphCore，不每帧清零，不按角色或通道名称硬编码。
- `il2cpp_trace.h`：所有 SMR Mesh 替换在写入前保存源布局、建立绑定，写入后只初始化一次；
  源通道使用游戏权重，新增通道为 0，然后应用现有 `shape.*`。只控制权重的规则也受保护。
- 游戏重新给同一 Renderer 指派源 Mesh 时，现有消费者输入路径重新保存源通道，
  清掉旧绑定/旧控制声明，不把旧 Mod 的槽序当作新源资源的槽序。
- 原生通道被作者控制时保留游戏最新要求值，撤销控制恢复该值；F10 恢复源权重增加真实回读核验。
  setter 成功返回不再单独作为恢复完成的证据，失败记录继续保留，沿用现有恢复机制。
- 绑定归现有 override/partner 状态所有，Hook 注册表只持弱索引；退役先移除索引。
  游戏 Hook 不获取 override 锁，原生方法转发前释放查表/通道锁，避免新增反向锁依赖。

入口通过 icall 名称和方法元数据解析，不使用诊断 RVA。第三个 icall
`UnityEngine.SkinnedMeshRenderer::get_sharedMesh()` 的字符串也在本地 GameAssembly 中核对过。
缺少必要入口时拒绝无保护的形态键绑定，而非悄悄沿用旧观察器。
保护范围是已观察的同步 Apply 路径；意外工作线程调用会报告并抑制该受保护写入，
不会在工作线程查询 Unity Mesh。未确认的其他动画写入路径不宣称已经覆盖。

### 可自动复现的验证

`tests/test_shape_ownership.py` 使用中性通道和真实 MinHook，实际调用
`Core -> native getter/setter`，并抽取生产 IL2CPP 适配器与消费者输入代码：

1. 安装修正前先复现：仅按通道数量驱动的游戏逻辑把两个新增通道写成 60/40，未声明 UI 或 `shape.*`。
2. 安装后同一调用链不再写入新增通道；显式作者权重保留，不因后续初始化被反复清零。
3. 原生通道重排、新增通道插在前面时按名称映射；原生 getter 与 setter 使用同一视图。
4. 作者控制原生通道为 75，期间游戏要求 90，撤销控制恢复 90，而不是开始时的旧快照。
5. 同 Mesh 多消费者、partner、嵌套 Apply、C++ 异常传播、当前 Mesh 检查、对象地址重用、
   F10 式新绑定、仅形态键规则及其 Mesh 更换、缺失必要 icall 均有断言。
6. 抽取实际 `EiemPrepareRenderInput`，验证同一消费者的新源布局替代旧布局，以及恢复挂起时不重建旧效果。

主调用链分别用 `/Od` 与 `/O2` 编译运行；这是真实 Hook + 模拟引擎对象测试，
不是用测试结果冒充游戏 GPU 画面验收。一般恢复测试另增加“源形态权重 setter 静默失败”的用例，
核对回读失败时保留记录，再次正确写入后释放；partner 清理测试核对先退役绑定再销毁对象。

v44/v45 临时探针源代码、定时器调用及专用观察测试已移除。旧源码与已采集日志仍在此前版本归档，
不删除证据。与本问题无关的既有残留诊断器未在此轮清理。

本节记录实现与测试设计；构建、全量测试、部署及游戏验收结果在下方分别记录。

### v46 验证与部署结果

- 全量 `unittest discover -s tests -q`：92 项通过，无跳过；包含后台 Blender 测试。
- `tools/diagnostics/test_static_shape_diagnostics.py`：另 6 项通过。
- `build.bat` 构建成功，`git diff --check` 无空白错误（已有工作区 LF/CRLF 提示不属于测试失败）。
- 确认游戏退出后仅更新 `plugin/eiem.dll`，构建/部署 SHA256 一致：
  `3FE3E33E19935C6E2DF7A04FD618D90E4646E2D034F7449B200DEC8E1CB678A1`。
  启动标记：`resource-runtime-v46-shape-ownership`。
- 上一版 v45 DLL、旧日志、新 DLL/PDB、此次相关源码/测试/文档和部署清单保存至
  `E:\EIEM_Workspace\plugin-releases\v46-shape-ownership-20260906-200136`。
- 全局配置和 typhoeus Mod 共 6 个文件部署前后 SHA256 均未改变；没有覆盖代理 DLL，未提交 Git。
- **游戏验收待完成**：此次还没有 v46 实机权重/画面结果。启动游戏才会加载新 DLL，F10 不能替换已载入的 DLL。
  使用现有 Mod 即可核对原异常动作；不需要增加 UI 或重导模型。之后再核对正常动画、F10 撤销/启用及换图。
