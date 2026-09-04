# 模型、材质、贴图替换：当前设计

状态：v31 架构清理。大世界替换有用户验证；v30 修复了 UI 回调崩溃，
v31 的完整游戏场景回归尚未验收。不能把赋值成功、编译成功当成画面正确。

## 1. 核心职责

替换按 Mesh 资源身份选中**所有匹配的 Renderer 实例**，操作在实例上执行。
不是重新打包，也不是覆盖 VFS 文件。PFB 可提供模型根和可选作用域，
但不是独立资源身份证明；不同 PFB 可以使用同一个 Mesh。

| Module | Interface：调用者需要知道的事 | Implementation |
|---|---|---|
| Mod document | INI 输入，得到资源声明与 Render 动作，出错有行号 | `eiem_mod_document.h`；无 Unity、线程或全局配置状态 |
| Mod program | 按 mod/section 查声明，取顶层 Render 集合 | `eiem_mods.h`；一次发布配置和预计算的引用索引 |
| Mod update | 提交 Reconcile / Reapply / Reload 请求 | `eiem_mod_update.h`；合并请求，明确恢复、发布、重应用顺序 |
| Render 执行 | 模型根或 Renderer + 规则集合 | `il2cpp_trace.h` 中唯一 `EiemApplyRenderRuleSet` 路径 |
| Resource backend | 根据声明构造 Mesh、材质克隆、贴图 | `eiem_resource_backend.h`；格式检查、Unity API、资源缓存 |
| Blender authoring | 导入离线资源、编辑、输出资源与 INI | Blender addon；不把运行时语法当作工程里唯一的数据源 |

`il2cpp_trace.h` 仍偏大，包含观察器、生命周期适配和执行 Implementation。
本轮没有为了目录好看把它拆成循环引用的几个头文件；后续应通过真实 Interface
提取实例/资源所有权，而非把同一组全局变量分散到不同文件。

## 2. 现在支持的 INI 语义

```ini
[MeshBody]
path=meshes/body.mesh
target.path=assets/.../body.asset
target.asset=Body

[MaterialCloth]
path=materials/cloth.mat

[TextureCloth]
path=textures/cloth.png
linear=false
mipmaps=true
filter=1
wrap=0
aniso=1
mip_bias=0

[RenderBody]
asset=Body
mesh=MeshBody
material.0=MaterialCloth
partner.0=RenderAccessory

[RenderAccessory]
mesh=MeshAccessory

[MeshAccessory]
path=meshes/accessory.mesh
```

- Mesh / Material / Texture / Skeleton 是资源声明，不执行替换。
  `source`、`asset`、`target.*` 是来源/创作信息，不暗中触发全局重定向。
- 未被 Prefab 或 partner 引用、且具有选择器的 Render 是顶层规则。
  `asset=` 匹配原 Mesh 对象名称，不是 Renderer 实例名。
- `Render.mesh` 给命中的**原 Renderer**换 Mesh；不是创建另一份绘制。
- `handling=skip` 仅禁用命中的原 Renderer，与 `mesh=` 独立。
  同时设置则绑定新 Mesh 后保持原 Renderer 禁用。
- `partner.N` 显式创建额外 Renderer，跟随原实例生命周期。
  partner 模板不会自动提升为顶层命中规则。
- `material.N` 是该 Renderer 的材质槽；没指定的槽保留源值。
  Mesh 文件记录 submesh，但不嵌入材质文件身份。
- `submesh.N=M` 可将 submesh N 映射到材质槽 M。槽号是资源布局，
  删除槽 0 的全部面不能把槽 1 自动重编号为 0。
- `match.vertices/indices/submeshes` 是可选附加条件，不是必须写的身份。
- 同一规则集合按配置顺序采用第一条命中的规则。文件夹按不区分大小写排序，
  根 `plugin/mods/mod.ini` 最后读取；不要依赖操作系统枚举顺序。
- 多个作用域都命中同一 Renderer 时的完整冲突/合并政策还未设计。
  当前不要把相互矛盾的规则分散到多个作用域，再假设它们会自动合成。

资源路径与 Transform 路径必须区分：

- `Mesh.path` / `Material.path` / `Texture.path`：mod 内磁盘文件。
- `Prefab.path`：游戏逻辑 PFB 路径。
- 现有 `Render.path`：相对模型根的 Transform 路径，不是逻辑 .asset 路径。
  普通 Blender 导出不再生成这个易混淆的字段，只生成 `Render.asset`。
- 仅靠名称不能证明跨所有包的资源身份唯一；运行时已观察到的逻辑路径
  是辅助来源信息，不能把它宣称为普遍可拦截的文件请求。

可选的 PFB 作用域如下（不要求普通 mod 写）：

```ini
[PrefabExample]
path=assets/.../example.prefab
render.0=RenderBody
```

其作用仅是选择规则集合，然后调用与顶层规则相同的执行器。
不再保留一套只处理 SkinnedMeshRenderer 的 PFB 专用替换循环。

## 3. 解析与发布约束

- 文本解析与规则执行分开；字段解析不创建 Unity 对象。
- 支持 UTF-8 BOM、CRLF、整行分号/井号注释。
- 关键数字必须完整有效；`12garbage` 不当成 12。
- 重复 `material.N` 更新该槽，不追加重复槽记录。
- 同一文件内重复 section 报错，不再让资源查找“取第一条”而 Render 查找“取最后一条”。
- 不完整语句、未知 handling、坏数字不会静默变成无条件执行。
  非法文件不发布其前半部分；记录文件和行号，跳过该文件，不偷偷保留旧版动作。
- 当前尚无条件语句、变量、按键配置 DSL。不能把 `if` 当注释吞掉。
- 部分元数据键允许保持为离线信息；这不是完整 schema 验证器。
  长度截断、跨节引用和循环引用诊断仍有待完善，见审查记录。
- 现有固定数组容量是实现限制，不是已证明的 Unity 上限；后续语法设计不能继续随意加魔法数字。

## 4. 生命周期和热更新

```text
启动：读取并发布 Mod program → 安装 hooks
模型完成/重新启用：模型根 → 规则集合 → 匹配原 Mesh → Render 操作
共享 Mesh 重新赋值：保留已有绑定，或执行相同的顶层 Mesh 规则
材质控制器提交：对已绑定 Renderer 重应用材质部分

F10/管理器：提交 Reload 请求
Unity 主线程：恢复旧效果 → 读取/发布新 program → 重应用已登记实例

后续状态变化：提交 Reapply 请求
Unity 主线程：恢复旧效果 → 用当前 program 重应用（不读磁盘）
```

- Reconcile 只重试已登记模型；不会因初始 generation 不同而意外恢复启动 hooks 的成果。
- 热键线程不再直接发布新配置。新配置不能在旧状态恢复前被 setter/材质 hook 看见。
- 合并多个请求时，生命周期请求不能吞掉同时到达的 Reload。
- 首次资源构造、恢复和实例操作仍限定在 Unity 线程。安全检查不是全部冗余：
  失效对象、数组范围、类型/返回值、主线程约束不能为了少分支直接删除。
- 无自动文件轮询，无全场景每帧扫描；Dump 页显式刷新是独立的观察功能。
- UIModelLoader 异步调用只观察，原样传递游戏委托。不得再将原生代码地址当作
  托管 MethodInfo 创建 delegate。UI 根由实际触发的 PFB/组件生命周期提交。
- 所有权以具体模型实例记账；重复实例各自保留骨架、材质和状态。
  这不表示已经覆盖所有未知游戏模型创建/销毁方式。

## 5. 资源与 Blender

- 离线包、Blender、DLL 使用 EIEMESH 与独立 .mat、贴图；不要求 FBX 中转。
- 材质 .mat 指明游戏逻辑 `source=`，运行时加载该源材质再克隆。
  例如 `texture._BaseMap=TextureCloth`、`float._SomeParameter=0.2`；
  参数名来自实际材质，不编造通用游戏 Shader。
- PNG 创建纹理时用声明的 linear/mipmap/sampler 设置；用户无需手工 DDS 压缩。
- Blender 按资源去重，不为每个 Renderer 实例生成重复 Mesh/骨架；LOD 分集合便于编辑。
  材质贴图路径优先显示，UV、颜色、形态键使用对应 Blender 数据。
- 导出选中且修改的资源及依赖；新增网格通过明确 Render/partner 关系表达，
  不靠合并模型继承一堆不透明自定义属性。
- 旧 EIEMESH 输入版本及来源元数据不是旧运行时替换引擎。仍在使用的离线包
  Reader 不因为“清理兼容”就任意删除，除非有迁移和等价验证。
- 骨架编辑、同路径改像素的增量依赖、导出失败时保护旧输出等仍存在已记录限制；
  本文不宣称这些已完整实现。

## 6. 后续 if/endif、按键设计应落在哪里

这次只搭接口，不预定用户尚未确认的语法：

1. 解析器保留有顺序、可嵌套的语句信息；不能在读文件时简单删除“不成立”的行，
   否则按键后无法重新求值。
2. 变量/按键改变求值状态；状态层不直接调用 Unity，也不复制每种 hook 的逻辑。
3. 求值结果交给现有 Render 执行模块；恢复与重应用走 Reapply，
   修改磁盘文件才走 Reload。
4. 资源声明与条件动作分离。Blender 项目保留语义数据，更新 exporter 后再生成新配置。
5. 实例所有权、资源退休、规则冲突要先收敛，不能继续增加自动猜测或静默回退。

本轮没有引入 VM、事件总线、插件接口层或第二套执行引擎。

## 7. 验证与记录

- `tests/test_mod_program.py`：真实文本解析/编译/排序，动作独立性，坏文件不部分发布，
  更新请求合并及恢复→发布→应用顺序。
- `tests/test_mesh_resource_cache.py`：真实缓存函数的七个状态场景。
- `tests/test_ui_async_passthrough.py`：UI 回调原样传递的六个场景。
- `tools/Blender/test_eiem_material_slot_gaps.py`：空材质槽、INI 槽号、重新导入一致。
- 其余静态合同检查是代码组织护栏，不等同于游戏行为测试。
- [架构审查与剩余风险](architecture-review-v1.md)
- [UI 回调崩溃实证](debugging/2026-09-05-ui-callback-crash.md)
- [历史设计与全部实验记录](archive/model-replacement-experiments.md)
