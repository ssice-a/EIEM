# EIEM 领域模型

当前规范：[模型替换设计](docs/model-replacement-design.md)。
实验历史在 `docs/archive/`，不能把旧实验里的“当前方案”当作现行设计。

- **Resource（资源）**：离线 Mesh、Material、Texture、Skeleton 文件及其声明；与渲染实例分开。
- **Render action（渲染操作）**：选择原 Mesh 的消费者并组织 mesh、material、skip、partner；是动作，不是资源文件。
- **Model instance（模型实例）**：游戏中实际存在的模型根；拥有独立 Transform、骨架、Renderer 和生命周期。
- **Prefab（PFB）**：原游戏序列化对象图；运行时提供可选作用域和实例来源，不替代 Mesh 资源身份。
- **Partner**：通过独立 Render 模板显式创建的额外 Renderer，归源模型实例所有。
- **Mod program**：解析后的资源声明、Render 动作及预计算的作用域索引；不包含 Unity 对象。
- **Update request**：Reconcile 重试、Reapply 状态重应用、Reload 磁盘重载；只在 Unity 线程修改运行时对象。

原则：一个资源的所有匹配消费者都应受规则影响；mesh、skip 和 partner 相互独立。
同名 Mesh 不自动证明同一逻辑文件。不能用吞错、缺省骨骼索引或猜测 PFB 名称掩盖未知情况。

本仓库审查记录使用 `docs/architecture-review-v1.md`，本轮未创建远端 Issue 或修改项目管理配置。
