# EIEM 文档索引与当前状态

核对日期：2026-09-17。本文只记录当前工作区能力及明确的实机验收边界。版本试验、探针部署和旧问题诊断统一保存在[历史档案](archive/README.md)。

## 当前状态

| 能力 | 当前实现 | 验证边界 |
|---|---|---|
| 静态 Mesh 替换 | 按原 Mesh 身份命中；替换 Mesh 在游戏原生装配边界进入现有 Renderer；不创建 Partner | 已用 `body_01_lod0_0`、`cloth_01_lod0_2`、`cloth_02_lod0_3` 实机验证稳定 |
| 多 submesh / 多材质 | Blender 选择导出生成一个合并 Mesh，保留多个 submesh 和对应材质槽 | 当前 Typhoea 测试包已实机显示；材质和贴图随静态替换生效 |
| 三端实例 | 世界、角色 UI、NPC 共享资源规则，各自沿游戏原生 owner、装配和 LOD 生命周期运行 | 三条目标 Render 在同类实例中均经过 `set_sharedMesh -> MOD-SKIN -> 材质 -> RendererInfo._Init -> NPC AssignSkin -> LOD`；没有发现某条 Render 漏装配 |
| 蒙皮槽位 | 保留源 Renderer 的既有 `bones[]` 槽位顺序；只有追加骨骼槽位才按作者路径解析 | 已覆盖世界、UI、NPC PFB 中同槽位骨骼名称不同的情况；禁止按角色名硬编码映射 |
| 诊断 Hook | 身份与装配探针默认关闭；NPC 只保留注册和释放所需的生命周期适配器 | 完整本地测试与 DLL 构建通过 |
| 按键显隐 | Mod 按键使用独立的最小窗口调度器；在同一 Mesh 对象上原地隐藏/恢复目标 submesh 索引，不进入 Partner/Physics 生命周期 | 2026-09-17 已部署原地索引测试版，等待世界/UI/NPC 实机验收 |
| 按住形态键 | `[Key] type=hold` 按独立采样节奏向目标值连续移动；`type=cycle` 仍是一按一切换 | DLL、INI 解析和 Blender 形态键导出已实现，见 [按住按键](hold-keys.md) |
| F10 热重载 | 新配置先完整解析为候选代际；解析失败时不恢复 Renderer、不发布新代际 | 事务预检已通过宿主测试；有效资源的现有实例回放仍待实机验收和后续收敛 |
| Skeleton / Physics / 碰撞体 | 保留已有格式和研究证据，生产链等待原生工厂与三端能力验证 | 属于后续重构阶段，不纳入当前静态替换验收 |

当前稳定结论只覆盖静态 Mesh、Material、Texture 及其蒙皮装配。用户已确认连续进入游戏时当前替换稳定。F6 曾通过切换完整/隐藏两个 `sharedMesh` 对象实现显隐，并稳定复现 cloth 部件躺地；骨骼引用和矩阵保持有效，问题已缩小到 Mesh 身份交换。当前测试版改为保留同一 Mesh 身份、只原地更新 submesh 索引，实机结果尚未确认。

## 现行文档

| 主题 | 入口 | 职责 |
|---|---|---|
| 项目术语 | [CONTEXT](../CONTEXT.md) | 核心对象与跨模块约束 |
| 代码架构 | [DLL 代码模块](code-architecture.md) | 模块职责、依赖方向和分阶段整理顺序 |
| 重构基线 | [资源替换重构](refactor-resource-replacement.md) | 目标架构、验证顺序、禁止事项和当前进度 |
| Mod 装配 | [模型替换](model-replacement-design.md) | 资源、Render、实例和恢复契约 |
| 原生装配链 | [模型创建链路](model-assembly-chain.md) | 世界、UI、NPC 的装配边界与蒙皮条件 |
| 合并边界 | [合并的边界](merged-part-renderer-boundaries.md) | 合并 Mesh、submesh、UV、材质槽和 LOD 约束 |
| 输入与配置 | [条件与按键](conditional-keys.md) | INI、表达式、状态与更新顺序 |
| Blender 网格工具 | [切换作者流程](blender-switches.md) | 选择、显隐、状态和导出 |
| Blender Mesh-only | [Mesh-only 导出](blender-mesh-only.md) | 只导出选中的 Mesh、材质和贴图，跳过骨架、物理及其他作者控制检查 |
| 顶点通道 | [顶点数据契约](vertex-data-contract.md) | 原生通道保留与缺失切线生成 |
| 蒙皮 | [共享骨架绑定](shared-skeleton-binding.md) | 源骨骼槽位、追加路径、bind pose 和权重 |
| 骨架扩展 | [骨架驱动](skeleton-driving.md) | 新增节点、Skeleton 文件和实例持有 |
| 形态键 | [形态键控制](shape-controls.md) | 作者通道、实例权重与游戏通道归属 |
| 作者状态 | [默认值、持久化与材质](author-state-materials.md) | `.blend` 数据、`state.ini` 和材质导入 |
| Mod UI | [Lua UI](lua-ui.md) | 脚本 API、窗口和变量事务 |
| 相机 | [反虚化契约](camera-fade.md) | CameraMono 原评估后清理 |
| 物理设计 | [三端物理契约](physics-authoring-design.md) | 解包、Blender、DLL 的完整链路要求 |
| 物理文件 | [作者格式](physics-authoring-v1.md)、[源图格式](physics-authoring-v2.md) | 新增作者数据与原生源图的格式和限制 |
| 工具运行 | [资源浏览器](../tools/README.md)、[Blender 插件](../tools/Blender/README.md) | 启动、安装和开发操作 |

## 维护规则

1. “已实现”“构建通过”“已部署”“游戏内验收”是四种不同状态，文档必须明确区分。
2. 现行文档描述契约和当前能力，不追加每次 DLL 哈希、启动日志或失败尝试。
3. 有复用价值的探针结果、根因和部署记录移入[历史档案](archive/README.md)，不能再作为当前行为的直接依据。
4. 身份、装配和高频资源探针默认关闭，只在有明确问题和退出条件的验证阶段临时启用。
5. 静态替换、按键、F10、Skeleton 和 Physics 分阶段验收；一个阶段通过不能替代另一阶段的实机结果。
