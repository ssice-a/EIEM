# EIEM 文档索引与当前状态

核对日期：2026-09-09。范围：仓库工作区和本节明确记录的部署/实机结果；不表示正在运行的 Blender 已同步。

## 当前状态

| 能力 | 工作区事实 | 验证边界 |
|---|---|---|
| Mesh / Material / Texture、条件、Lua UI、持久化 | v59 统一按 Mesh 身份匹配全部消费者；PFB 只保留资源关系；直接构造的 NPC Renderer 在 `RendererInfo._Init` 进入同一执行器 | 63 项检查及完整构建通过；v59 实机记录多个 Typhoea body/cloth 在该入口成功执行，用户确认 NPC 画面已替换 |
| 共享骨架与新增节点 | Skeleton v1/v2 读取、实例节点创建和 Mesh bones 装配已实现；v70 夹具将 202 个 Typhoea `Finger0` 链权重改绑到一个新节点 | 文件、bindpose 与 C++ 读取通过；用户已在角色 UI 观察到左手拇指动作改变，验证新 Transform 经原生写回驱动替换 Mesh |
| Blender 作者工具 | 源码版本 0.25.0；侧栏首层只保留当前组、新建、复制参数、粘贴参数、查看；带完整模板的作者组与原生组共用常用参数、角度限制、九条曲线、Empty 自定义属性及高级字段界面，并管理同 Rig 碰撞集合 | 真实 Typhoea 正常包一次导入 62 Mesh、1 共享 Rig、11 组、27 碰撞体；249 项参数及组使用的碰撞集合可复制到新增作者组；v4 保存参数，源碰撞体转为 DLL 配置仍待后续阶段 |
| Physics 作者 v1/v3/v4 | v1/v3 兼容读取；v4 保存五项常用参数、节点半径曲线及按原生字段路径记录的完整参数模板，选中无独立碰撞体组可随同 Rig Mesh 增量导出 `Render.physics` | v1 创建/运动/退休已有实机验证；v4 的 249 项 Typhoea 参数往返、C++ 读取及元数据名称写入已完成 Blender/宿主验证，游戏内参数响应待验证 |
| Physics 源作者 v2 | 正常解包输出源图、完整字段/曲线/原始字节、物理 Transform、共享引用、三种原生碰撞体外形、参数复制和 C++ 树读取 | AnimeStudio .NET 9 Release 与 Blender 5 正常整包导入通过；尚未在终末地运行时实例化 v2 或碰撞体 |
| DLL 配置准备 | v65 为 v1 BoneCloth 写入 `clothType=1`、`connectionMode=0`、构造函数自带的根/IGNORE 列表和五个标量；Data2 使用组件构造实例 | v71 以 F10 完成 `blendWeight=1 → 0 → 1` 双向实机响应；其余参数语义、碰撞转换和 v2 实例化仍未完成 |
| 原生物理研究 | v65 实机已完成最小 BoneCloth 的配置、`BuildAndRun`、异步 Team 构建和 Animator 接纳；v66 确认 MOVE 节点写回和场景卸载注销；v70 确认新增节点产生可见 Mesh 变形 | team 编号可复用；参数响应、碰撞体和 v2 源图实例化仍未验证 |
| Physics 实例所有者调查 | v61 已按 Mesh 命中记录注册模型的 Animator/现有 Cloth，并在 NPC `_BuildBeyondCloth → StartNPC → ReleaseAvatar/OnRelease` 边界关联模型根、Animator 与 Avatar owner | v70 实机确认两个目标 NPC 创建 Physics；v71 F10 与自然卸载均观察到 NPC 最终 `retired` 和 Skeleton 自有节点退休 |
| Physics 生产适配器 | v67～v69 已按同一 Mesh 规则为角色 UI 和 `BaseModelPart` 建立独立实例；v70 又将相同执行器接到精确 `NPCAvatar.StartNPC` owner | 主模型、角色 UI、NPC 均有四节点原生 Team；UI/NPC 可见生效，v71 `blendWeight` 双向热重载有效。碰撞体和 v2 仍未实证 |

当前按用户安排暂停碰撞体 DLL 实验，优先完善解包数据到 Blender 的配置、复制和增量导出流程。
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
| 物理文件 | [Physics 作者 v1/v3/v4](physics-authoring-v1.md)、[源作者 v2](physics-authoring-v2.md) | 新增作者格式与原生源图格式、操作和限制 |
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
