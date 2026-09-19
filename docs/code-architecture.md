# DLL 代码架构

本文记录现行 DLL 的模块边界。实验过程和已被推翻的实现不再放在当前文档中；需要追溯时使用 Git 历史。

## 三端边界

| 仓库 | 职责 | 不负责 |
|---|---|---|
| EIEM | DLL、INI 运行时、游戏实例适配、资源提交与恢复 | 编辑作者数据、解析游戏 VFS |
| EIEM-blender（`tools/Blender`） | Blender 导入、编辑、校验与导出 EIEM 资源 | 游戏对象生命周期、Unity API |
| AnimeStudio（开发目录 `E:\vscode\AnimeStudio`） | VFS 解包、资源索引与离线检查 | Mod 运行时替换 |

三端只通过有版本的文件格式交接。DLL 不读取 `.blend`，Blender 不调用游戏 Hook，AnimeStudio 不参与游戏运行时。

## DLL 分层

```text
Windows / ApplePie host
  -> IL2CPP 与 Hook adapter
      -> Model lifecycle registry
          -> Render replacement transaction
              -> Resource backend / Unity ownership

mod.ini
  -> parser + expression evaluator
      -> update coordinator
          -> registered model instances
```

### 平台与 Hook

`eiem.cpp`、`il2cpp_api.h`、`globals.h`、`init.h` 只负责加载 DLL、解析 Unity/IL2CPP API、安装 Hook 和把回调转换成生命周期事件。Hook 入口不能另写一套 Mesh/Material 替换逻辑。

### Mod 前端与更新

`eiem_mod_document.h`、`eiem_expression.h`、`eiem_mods.h`、`eiem_keys.h`、`eiem_persistent_state.h`、`eiem_mod_dispatcher.h` 负责解析、条件、状态、按键与 F10 请求。解析层不调用 Unity API。

`partner.N` 已停止支持。多部件由 Blender 合并成一个 Mesh 的多个 submesh，显隐由 `submesh_visible.N` 表达。输入状态按 Mod 隔离，世界、UI、NPC 的同一 Mod 实例共享同一状态。

### Model lifecycle

`eiem_model_lifecycle.h` 保存通用 owner 类型与实例状态；`eiem_npc_model_owner.h` 及 `il2cpp_trace.h` 中的 world/UI adapters 只负责登记、激活、失活和释放模型实例。它们都调用同一个 Render executor。

生命周期状态必须按具体模型实例保存。不同 PFB 可以有不同 Transform 对象，不能跨世界、UI、NPC 实例借用骨骼。

### Render replacement

`eiem_resource_backend.h`、`eiem_skin_binding.h`、`eiem_render_state.h` 和 `il2cpp_trace.h` 中尚未拆出的 executor 负责 Mesh、Material、Texture、骨骼 palette、submesh 显隐及恢复。

每次模型事务先对 SkinnedMeshRenderer 和 MeshFilter 各做一次层级快照。骨骼解析顺序为：

1. 当前实例中的完整名称路径；
2. EIEMESH v5 记录的“源 Mesh 身份 + 原始 bones[] 槽号”；
3. 当前实例内的层级索引路径。

第二步解决同一原 Mesh 在不同 PFB 中骨骼名称不同的问题。映射不包含角色名、Mesh 名或裙骨等特例。

### 可选功能

- `eiem_camera_fade.h`：独立反虚化功能。
- `eiem_shape_*`：形态键状态与运行时绑定。
- `eiem_skeleton_*`、`eiem_native_physics_*`、`eiem_physics_*`：尚未完成实机验收的骨架与 Physics 工作。
- `eiem_ui_host.h`、`eiem_lua_ui.h`、`gui.h`：Mod 管理器和 Lua UI。
- `eiem_registration_trace.h`、`eiem_skin_probe.h`、`eiem_metadata_probe.h`：有界诊断，不得决定生产行为。

## 已完成的清理

- Blender 和 AnimeStudio 使用独立仓库；EIEM 只保存 submodule 固定版本。
- Model lifecycle 公共类型已从 `il2cpp_trace.h` 提取。
- 一次 Render 事务复用同一批 Renderer 快照，避免重复遍历和两个不同层级时刻之间的竞态。
- `partner.N` 已从解析器、编译器、按键/F10 执行入口和 Hook 安装路径移除。
- 旧 Partner 测试及历史诊断文档已删除。
- 旧 Partner 实现块目前没有生产入口，后续应在拆出 Render executor 时物理删除，不能重新接回运行链。

## 下一步拆分顺序

1. 从 `il2cpp_trace.h` 提取 update coordinator。
2. 提取 model registry 和 world/UI lifecycle adapters。
3. 提取 Renderer override ownership 与 Render executor。
4. 删除无调用者的旧 Partner 实现及它专用的 Unity API 解析。
5. 最后整理 Hook 安装与可选诊断。

每一步都必须保持三端同一 Mesh 身份规则、F10 事务顺序和世界/UI/NPC 实例隔离，并运行完整单元测试与 DLL 构建。涉及 Unity 所有权或 Hook 顺序时，还需分别做世界、UI、NPC 的实机回归。
