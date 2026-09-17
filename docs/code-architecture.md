# DLL 代码模块与整理顺序

本文描述 DLL、Blender 和解包工具的现行职责及目标模块。它约束后续重构，
不把文件移动本身当成功能改进。

## 目标模块

| 模块 | 对外职责 | 当前状态 |
|---|---|---|
| Render 资源替换 | 读取并构造 Mesh、Material、Texture；对命中的原 Renderer 提交、恢复资源 | 核心功能已实现；执行器仍有较多代码留在 `il2cpp_trace.h` |
| Mod 前端与更新协调 | INI 解析、表达式、变量、按键、F10、状态持久化；发布 Reconcile/Reapply/Reload | 解析模块已独立；主线程事务仍需从 `il2cpp_trace.h` 提取 |
| Camera fade | 独立控制游戏相机的透明/淡化结果 | 已独立为 `eiem_camera_fade.h`，只保留小型 Hook adapter |
| UI、Shape、Skeleton、Physics | Mod UI 和尚未全部验收的作者功能 | 已按主题拆文件；不得阻塞静态资源替换 |
| Blender 作者端 | 选择、编辑、合并、材质贴图及资源导出 | 位于 `tools/Blender/`，继续与 DLL 格式契约对接 |
| 解包与资源索引 | VFS 提取、AnimeStudio/索引查询、离线检查 | 位于 `tools/` 和独立 C# 工具，不进入 DLL 运行时 |
| ImGui | 内置诊断/管理页面及 Mod UI 宿主 | `gui.h`、`eiem_ui_host.h`、`eiem_lua_ui.h`；后续拆页面，不与 Render executor 混合 |
| Legacy runtime | 旧 MMD、动画、Partner 和上游资源实验 | 暂时保留；核心链验收完成后统一隔离、删除或迁移 |

还需要三个独立模块，它们不能归入 Mesh 或 UI：

| 模块 | 原因 |
|---|---|
| Model lifecycle | 世界、UI、NPC 的入口不同，但都只负责发现、登记、激活和释放同一个 Model instance |
| Platform/Hook adapter | IL2CPP 元数据、Unity API、Hook 安装和线程调度是平台适配，不是业务规则 |
| Ownership and diagnostics | Unity 资源缓存、Renderer 恢复责任属于所有权；探针和 Dump 只提供证据，不能决定生产行为 |

## 依赖方向

```text
DLL/ApplePie host
  -> Platform and Hook adapters
      -> Model lifecycle registry
          -> Render executor
              -> Resource backend and Unity ownership

INI files
  -> Mod frontend
      -> Update coordinator
          -> Render executor

Blender/export tools -> versioned resource files -> Resource backend
VFS/extraction tools -> source evidence only
```

世界、UI、NPC 可以有不同的 lifecycle adapter，但不能各自实现 Mesh、材质、按键或
F10 规则。Hook 只提交事件，Render executor 是唯一的资源替换实现。

## 当前关键装配契约

1. `EntityRenderHelper` 正在遍历 Renderer 时，`RendererInfo._Init` 只让游戏完成干净源材质采样。
   Mesh 和材质在完整 helper 返回后统一提交，禁止逐 Renderer 中途替换。
2. 不经过 `EntityRenderHelper` 的直接 NPC/UI Renderer，仍在独立 `_Init` 完成后进入同一个
   Render executor。这是入口差异，不是第二套替换实现。
3. F10 在 Unity 线程按“完整解析候选配置、恢复旧输出、发布新 generation、回放已观察实例”执行。
4. 按键只改变当前选中 Mod 的状态；该状态仍由该 Mod 的世界、UI、NPC 实例共享。submesh 显隐保持 Mesh、Renderer、骨骼和 LOD 身份不变。
5. 诊断 Hook 默认关闭，临时探针必须有触发上限和删除条件。

## 分阶段整理

### 阶段 A：现行基线

- 删除没有调用点的历史 F6 蒙皮 timer/probe 状态。
- 建立本文和 [`src/README.md`](../src/README.md) 的职责索引。
- 保持已验证的 Mesh 提交时序，不在清理中改变资源行为。

### 阶段 B：输入与更新

- 已完成：将 Windows 消息、全局热键注册和按键/F10 事件投递迁入
  `eiem_mod_dispatcher.h`；原包含位置和执行顺序不变。
- 已完成：`INSERT` 打开最小 Mod 管理器；普通快捷键只为所选 Mod 注册，输入事件携带
  `modPath`，面板按钮还携带精确 Key section。F10 保留仍存在的选择。
- 从 `il2cpp_trace.h` 提取 Update coordinator。
- 对外只保留提交 Reconcile/Reapply/Reload、提交按键事件和 Unity 线程执行事务。
- F10 与按键的宿主测试改为直接覆盖该接口。

### 阶段 C：实例与 Render

- 提取 Model instance registry 和 world/UI/NPC adapters。
- 提取 Renderer override ownership、Render executor 和材质控制器 adapter。
- `il2cpp_trace.h` 最终只保留 Hook adapter/安装，不保存 Render 业务状态。

### 阶段 D：可选功能

- 已完成最小 Mod 管理页与旧诊断 GUI 的线程隔离；后续 ImGui 页面继续拆分，公共 UI host 保持小接口。
- Shape、Skeleton、Physics 分别按实机验收结果推进，不互相伪装为完成条件。
- Camera fade 保持独立，不并入通用 Render 规则。

### 阶段 E：Legacy

- 统计 `trojan.h`、MMD/动画/Partner 代码的真实调用者。
- 有消费者的迁入对应模块；无消费者的连同 Hook、全局状态和文档一起删除。
- 上游资源替换实验在现行 Render/F10/按键全部验收后统一处理，避免同时维护两条链。

## 验证要求

每次物理拆分至少执行完整单元测试和 DLL 构建。涉及 Hook 顺序、Unity 对象所有权或资源提交
时序时，还必须分别记录大世界、UI、NPC、F10 和按键的游戏验收，不能用“纯移动代码”跳过。
