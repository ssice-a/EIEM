# Blender LOD 导出

Blender 导出器的 LOD 选项只作用于当前选中的 EIEM Mesh。导出器从导入资源元数据读取 LOD，不根据 Blender 对象显示名称或手写文件名猜测游戏资源。

导出面板提供 `LOD0` 到 `LOD4` 复选框以及“全部已发现 LOD”。“全部”表示当前 `.blend` 中同一资源族已经导入的全部 LOD；未导入、未观察到的级别不会生成 Mesh、Render 或其他 INI 规则。

当选中一个 LOD Mesh 并勾选多个目标级别时，选中的 Mesh 作为模板复制到每个已发现的目标级别。复制只创建导出期间的只读资源视图，不改写 `.blend` 中的 Mesh、材质、骨骼或切换状态。每个目标级别仍生成独立的精确 `asset=` Render 规则，例如：

```ini
[RenderBody_lod0]
asset=S_actor_body_lod0
mesh=MeshBody_lod0

[RenderBody_lod1]
asset=S_actor_body_lod1
mesh=MeshBody_lod1
```

按键切换变量只声明一次。每个 LOD 的 Render 规则复用同一个变量和相同的 submesh 条件，因此世界、NPC、UI 实例以及不同 LOD 共享同一 Mod 状态。导出器不会创建 `asset=*lod*` 通配规则，也不会新增 DLL LOD Hook。

同一源资源的多个部件仍会合并为一个 Mesh，但合并范围严格限制在同一个 LOD。LOD0 与 LOD1 不会共用顶点缓冲或子网格槽位。不同 LOD 的骨骼、bindpose、材质槽和 UV 契约仍由现有 Mesh 导出校验；不兼容数据会在导出时明确报错。

如果显式勾选的目标级别在当前工程没有发现，导出会失败并提示没有可用目标，不会静默生成可能无法命中的规则。Mesh-only 导出使用相同的 LOD 选择逻辑，但仍跳过 Skeleton、Physics 和作者控制依赖。
