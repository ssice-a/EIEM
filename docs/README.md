# EIEM 文档索引与当前状态

核对日期：2026-09-11。范围：仓库工作区和本节明确记录的部署/实机结果；Blender 同步状态以本节记录为准。

## 当前状态

| 能力 | 工作区事实 | 验证边界 |
|---|---|---|
| Mesh / Material / Texture、条件、Lua UI、持久化 | v59 统一按 Mesh 身份匹配全部消费者；PFB 只保留资源关系；直接构造的 NPC Renderer 在 `RendererInfo._Init` 进入同一执行器 | 63 项检查及完整构建通过；v59 实机记录多个 Typhoea body/cloth 在该入口成功执行，用户确认 NPC 画面已替换 |
| 共享骨架与新增节点 | Skeleton v1/v2 读取、实例节点创建和 Mesh bones 装配已实现；v70 夹具将 202 个 Typhoea `Finger0` 链权重改绑到一个新节点 | 文件、bindpose 与 C++ 读取通过；用户已在角色 UI 观察到左手拇指动作改变，验证新 Transform 经原生写回驱动替换 Mesh |
| Blender 作者工具 | 源码版本 0.30.3；物理侧栏保留当前组、新建、复制参数、粘贴参数、查看；作者 v5 保留球体、异径胶囊、中心对齐和共享组引用；带作者物理骨权重的所选 Mesh 自动携带物理组依赖 | 后台 Blender 已验证未选 Group Empty 的带权重 Mesh 仍输出 Physics；当前 03:46 Typhoea 游戏包是在修复前导出，仍需重导 |
| Physics 作者 v1/v3/v4/v5 | v1/v3/v4 兼容读取；v5 在完整参数模板上增加胶囊末端半径与中心对齐语义，球/胶囊可随 Rig Mesh 增量导出 `Render.physics` | v1 创建/运动/退休已有实机验证；v4 参数响应仅验证过 `blendWeight`；v74 已验证 v5 创建 5 个碰撞组件并装入组列表，实际碰撞响应仍未验证 |
| Physics 源作者 v2 | 正常解包输出源图、完整字段/曲线/原始字节、物理 Transform、共享引用、三种原生碰撞体外形、参数复制和 C++ 树读取 | AnimeStudio .NET 9 Release 与 Blender 5 正常整包导入通过；源球/胶囊可转换到作者 v5；无限平面与整份 v2 图仍不由 DLL 实例化 |
| DLL 配置准备 | v80 在既有参数写入、LOD 与蒙皮调度基础上，为纯 partner 列表变化增加差集更新，不再整 Mod 销毁/重建无关 Renderer | MSVC 宿主回归与完整 DLL 构建通过并已部署；按键后贴地是否消失仍待新进程实机验证，F10 仍是完整重建路径 |
| 原生物理研究 | v65 实机已完成最小 BoneCloth 的配置、`BuildAndRun`、异步 Team 构建和 Animator 接纳；v66 确认 MOVE 节点写回和场景卸载注销；v70 确认新增节点产生可见 Mesh 变形 | team 编号可复用；v71 已验证 `blendWeight` 响应；作者 v5 碰撞体和 v2 源图实例化仍未验证 |
| Physics 实例所有者调查 | v61 已按 Mesh 命中记录注册模型的 Animator/现有 Cloth，并在 NPC `_BuildBeyondCloth → StartNPC → ReleaseAvatar/OnRelease` 边界关联模型根、Animator 与 Avatar owner | v70 实机确认两个目标 NPC 创建 Physics；v71 F10 与自然卸载均观察到 NPC 最终 `retired` 和 Skeleton 自有节点退休 |
| Physics 生产适配器 | v67～v71 已按同一 Mesh 规则为主模型、角色 UI 和 NPC 建立独立实例；v72～v79 将作者碰撞体、参数、Skeleton 代际纳入相同实例所有权与退休收集 | 03:46 前日志确认 Team ready、12/12 MOVE 节点变化且 5 个碰撞体进入实例；03:46 后当前包已无 Physics，必须重导后再验收视觉强度 |

当前 03:46 生成的 Typhoea 包没有 `.physics`、`[Physics...]` 或 `physics=`；F10 日志已从
`previous=1 current=0` 退休旧实例。因此当前画面不能用于判断额外物理强弱。修复前的同一进程旧包曾到达
Team ready，18 个节点全命中且 12/12 MOVE 节点变化，证明链路曾实际运行。Blender 0.30.3 现按网格正权重
推导作者 Physics 依赖，开发目录已同步；重载插件和重导包后再做视觉 A/B。DLL v80 已部署，普通按键只更新
partner 差集；连续 F10 的完整重建仍需独立实机验收。
v53 自动跟踪改动相关 13 项通过，完整构建和部署成功；游戏记录确认自动跟踪、关闭导出、原组件
`Init → RemoveMonitoringProcess → StartRuntimeBuild` 及中途注销嵌套序列，详见[原生调查第 20 节](native-physics-investigation.md#20-2026-09-07运行时快照与-v53-自动跟踪)。
v54 将五个 Animator binding InternalCall 纳入相同的自动测试跟踪，但实机因错误使用 CoreModule 查找
Animator 而在安装 Hook 前失败；退出快照确认零跟踪事件。v55 已统一探针和安装器的 AnimationModule
契约，20 项宿主检查及完整构建通过并已部署；PID 37728 的正常退出快照共 16118 次成对调用，
但五个 Animator binding 接口均为 0 次，不能据此认定它们是每次物理构建的必经路径。
v56 新增 Render 命中到模型级 Physics 意图的去重规划，以及 CharUI 显隐 active 输入；46 项针对性检查与
完整构建通过。独立 v56 没有部署；代码保留在 v57，但生产加载保护未移除，所以游戏仍不会接收 Physics 动作。
v57 继续自动测试，增加 TeamManager 的团队登记、Transform dirty 与 Animator 汇总更新 Hook；20 项跟踪契约及
完整构建通过并部署，但在首次加载前被 v58 取代。v58 修正 PFB/Render 语义；62 项相关
MSVC/宿主检查及完整构建通过并部署。随后的实机日志中，两个 Typhoea Renderer 在
`RendererInfo._Init` 出现，但从未经过 setter、模型根或有效的 `SetSMRRootBone` Render 回放，仍保留 917 顶点原 Mesh。
v59 将该实证入口接入全局 Mesh 身份执行器，并把 `SetSMRRootBone` 恢复为顺序观察；63 项相关检查及完整构建通过。
新进程记录多个 Typhoea body 的 Mesh 替换和对应 cloth 的 skip 均由 `[MOD-RENDERER-INIT]` 完成，用户确认 NPC 画面已替换。
v60 进一步使用 NPC 元数据中的 `_BuildBeyondCloth`、`NPCAvatar.avatarGoRef`、`StartNPC` 与释放入口调查
Mesh 命中实例的精确模型 owner/Animator。观察 DLL 已构建并部署；尚无 v60 实机日志，因此不能据此声称
主角、NPC 或 UI 的附加 Physics 已建立。
此前 v51 的 82 项、v50 的 64 项和 v52 的 35 项分别保留在第 17、16、18 节，
不与本轮相加，也不证明原生异步退出。此前建议以 DisposeInternal 返回释放骨骼的对话结论同样已撤回。

## 现行文档

| 主题 | 入口 | 职责 |
|---|---|---|
| 项目术语 | [CONTEXT](../CONTEXT.md) | 核心对象与跨模块约束 |
| Mod 装配 | [模型替换](model-replacement-design.md) | 资源、Render、实例及恢复 |
| 输入与配置 | [条件与按键](conditional-keys.md) | INI、表达式、更新顺序 |
| Blender 网格工具 | [切换作者流程](blender-switches.md) | 选择、显隐、状态和导出 |
| 顶点通道 | [顶点数据契约](vertex-data-contract.md) | 原生保留与缺失切线生成 |
| 蒙皮 | [共享骨架绑定](shared-skeleton-binding.md) | 原骨骼表、路径、bind pose 和权重 |
| 骨架扩展 | [骨架驱动](skeleton-driving.md) | 新增节点、Skeleton 文件和实例持有 |
| 形态键 | [形态键控制](shape-controls.md) | 作者通道、实例权重与游戏通道归属 |
| 作者状态 | [默认值、持久化与材质](author-state-materials.md) | `.blend` 数据、state.ini、材质导入 |
| Mod UI | [Lua UI](lua-ui.md) | 脚本 API、窗口和变量事务 |
| 相机 | [反虚化契约](camera-fade.md) | CameraMono 原评估后清理 |
| 物理目标 | [三端设计契约](physics-authoring-design.md) | 解包、Blender、DLL 的完整链路要求 |
| 物理文件 | [Physics 作者 v1/v3/v4/v5](physics-authoring-v1.md)、[源作者 v2](physics-authoring-v2.md) | 新增作者格式与原生源图格式、操作和限制 |
| 物理证据 | [原生调查](native-physics-investigation.md) | 静态/宿主/实机证据与待确认项 |
| 工具运行 | [资源浏览器](../tools/README.md)、[Blender 开发插件](../tools/Blender/README.md) | 启动、安装与操作 |

## 阅读与维护

1. 当前能力先看本页，再看对应契约和代码。实现草稿、设计目标、历史构建不互相替代。
2. 原生调查标记为 authoritative，按章节时点保留证据；冲突以最新实测及其明确边界为准，计划不能提升为事实。
3. 有价值的根因、日志位置和部署哈希保留在[历史档案](archive/README.md)；现行文档不追加部署流水账。
4. 更新代码后同步对应契约和本页状态，注明是否测试、构建、部署或游戏验收，避免重复状态清单。
5. 删除重复或失效说明时同步引用；未完成的验证条件不能仅因与新计划冲突而删除。
6. “尚未验证”只限制依赖该结论的具体修改；不要求暂停独立的源码实现、编译、静态分析或测试。

本轮删除了含未经证明退出判据的独立 Physics 路线图，以及已被 CameraMono 契约替代的 v34 操作说明。
文档清理不代表原生生命周期问题已解决，也不改变宿主审查机制。
