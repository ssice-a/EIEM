# 模型、材质、贴图替换：当前设计

现行职责、语法与生命周期契约。工作区版本和验证等级统一见[文档索引](README.md)。
共享骨架绑定见[蒙皮契约](shared-skeleton-binding.md)，新增节点见[骨架驱动](skeleton-driving.md)。
源材质初始化和缓存修正已经进入代码；[残留诊断档案](archive/map-reload-residue-diagnosis.md)保留当时证据，
不能将用户暂未观察到残留扩大为全部生命周期或 GPU 画面已验收。

全局配置与条件语法见[条件与按键](conditional-keys.md)，作者操作见[Blender 切换流程](blender-switches.md)。
完整物理目标见[三端设计契约](physics-authoring-design.md)。v67 已让 `EiemReloadMods` 发布通过校验的
Physics Mod，并由普通 Render 的 Mesh 命中按模型实例消费无碰撞体 v1 `Render.physics`。v70 资源已在角色 UI
验证四节点 `fixed/move/move/move` 链驱动替换 Mesh 可见变形；同版 DLL 又把精确 `NPCAvatar.StartNPC` owner
接入相同模型执行器。实机已确认两个 NPC 分别进入原生 Team 48/49并产生可见拇指效果，两个释放入口也注销了
owner；本次关闭前尚未观察到延迟销毁后的最终 `retired`。证据与限制见
[原生调查第 20.23～20.24 节](native-physics-investigation.md)。碰撞体和 v2 原生图仍不属于当前可执行范围。

## 1. 核心职责

替换按 Mesh 资源身份选中**所有匹配的 Renderer 实例**，操作在实例上执行。
不是重新打包，也不是覆盖 VFS 文件。PFB 可提供模型根、实例来源及资源关系，
但不限制 Render 的实例范围，也不是独立资源身份证明；不同 PFB 可以使用同一个 Mesh。

| Module | Interface：调用者需要知道的事 | Implementation |
|---|---|---|
| Mod document | INI 输入，得到资源声明、变量、按键和有序 Render 条件树，出错有行号 | `eiem_mod_document.h` / `eiem_expression.h`；无 Unity 或全局配置状态 |
| Mod program | 按 mod/section 查声明，取顶层 Render 集合 | `eiem_mods.h`；一次发布配置和预计算的引用索引 |
| Mod update | 提交 Reconcile / Reapply / Reload 请求 | `eiem_mod_update.h`；合并请求，明确恢复、发布、重应用顺序 |
| Render 执行 | 模型根或 Renderer + 规则集合 | `il2cpp_trace.h` 中唯一 `EiemApplyRenderRuleSet` 路径 |
| Resource backend | 根据声明构造 Mesh、材质克隆、贴图 | `eiem_resource_backend.h`；格式检查、Unity API、资源缓存 |
| Skeleton binding | 资源节点与实例 Transform 映射、增量创建和消费者生命周期 | `eiem_skeleton_document.h` / `eiem_skeleton_runtime.h`；不写源骨骼姿态 |
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
- `skeleton=` 为该 skinned Render 指定共享骨架依赖；原节点按路径引用，新节点按作者 TRS 创建。
  Mesh 的骨骼路径、bindpose 和顶点权重必须同时包含新增节点，Physics 才能让该节点的写回影响画面；
  原骨骼绑定姿态修改和整套 Avatar 替换仍不在现行执行范围内。
- `submesh.N=M` 可将 submesh N 映射到材质槽 M。槽号是资源布局，
  删除槽 0 的全部面不能把槽 1 自动重编号为 0。
- `match.vertices/indices/submeshes` 是可选附加条件，不是必须写的身份。
- 同一规则集合按配置顺序采用第一条命中的规则。文件夹按不区分大小写排序，
  根 `plugin/mods/mod.ini` 最后读取；不要依赖操作系统枚举顺序。
- 多条源 Render 都命中同一 Renderer 时仍按配置顺序取第一条；不要为同一 Mesh 声明相互矛盾的源规则，
  再假设它们会自动合成。

资源路径与 Transform 路径必须区分：

- `Mesh.path` / `Material.path` / `Texture.path`：mod 内磁盘文件。
- `Prefab.path`：游戏逻辑 PFB 路径。
- 现有 `Render.path`：相对模型根的 Transform 路径，不是逻辑 .asset 路径。
  普通 Blender 导出不再生成这个易混淆的字段，只生成 `Render.asset`。
- 仅靠名称不能证明跨所有包的资源身份唯一；运行时已观察到的逻辑路径
  是辅助来源信息，不能把它宣称为普遍可拦截的文件请求。

可选的 PFB 资源关系如下（不要求普通 mod 写）：

```ini
[PrefabExample]
path=assets/.../example.prefab
render.0=RenderBody
```

它把 PFB 与相关 Render 声明关联起来，供导入、资源整理和模型来源诊断使用，
不把 `RenderBody` 限制在这个 PFB。只要 Renderer 使用 `RenderBody.asset` 命中的 Mesh，
无论来自该 PFB、其他 PFB、重复角色、NPC、角色 UI 还是预览，都执行同一条 Render 动作。
`partner.N` 引用的 Render 是额外 Renderer 模板，仍不作为源 Mesh 的全局命中规则。

- 所有带 Mesh 选择器且不是 partner 模板的 `Render` 都按 Mesh 身份作用于每个已观察消费者。
- Hook 安装日志只会在进程启动时出现一次。判断某个实例是否执行规则，应查看该 Renderer 的
  `resource rule applied` 或模型的 `actions=N`，不能等待第二条 Hook 安装日志。
- LOD 是不同的 Mesh asset。只声明 `asset=..._lod0` 不会同时匹配 `lod1`、`lod2` 或 `lod3`。

普通 Blender 导出只需生成带 `asset=` 的 Render；保留或省略 PFB 关系都不改变运行时命中范围。

## 3. 解析与发布约束

- 文本解析与规则执行分开；字段解析不创建 Unity 对象。
- 支持 UTF-8 BOM、CRLF、整行分号/井号注释。
- 关键数字必须完整有效；`12garbage` 不当成 12。
- 重复 `material.N` 更新该槽，不追加重复槽记录。
- 同一文件内重复 section 报错，不再让资源查找“取第一条”而 Render 查找“取最后一条”。
- 不完整语句、未知 handling、坏数字不会静默变成无条件执行。
  非法文件不发布其前半部分；记录文件和行号，跳过该文件，不偷偷保留旧版动作。
- 条件语句、mod 内变量及 cycle 按键已实现；参见独立语法文档。错误条件不能当注释吞掉。
- 部分元数据键允许保持为离线信息；这不是完整 schema 验证器。
  Render 字段、跨节资源及 partner 引用均在发布前检查；后端容量与其他格式输入限制见审查记录。
- 现有固定数组容量是实现限制，不是已证明的 Unity 上限；后续语法设计不能继续随意加魔法数字。

## 4. 生命周期和热更新

```text
启动：读取并发布 Mod program → 安装 hooks
模型完成/重新启用：模型根 → 规则集合 → 匹配原 Mesh → Render 操作
共享 Mesh 重新赋值：保留已有绑定，或执行相同的顶层 Mesh 规则
NPC 直接 Renderer：游戏完成 RendererInfo._Init 的干净源材质采样 → 对具体 Renderer 执行全局 Mesh 规则
NPC 最终骨骼装配：SetSMRRootBone 只记录装配顺序，不承担 Render 动作
NPC 模型 Physics：游戏完成 NPCAvatar.StartNPC → 以 component/model 注册同一模型执行器
NPC 模型释放：ReleaseAvatar/OnRelease 调用游戏原函数前 → 注销同一 component owner
材质控制器提交：对已绑定 Renderer 重应用材质部分
材质控制器初始化：临时暴露该 Renderer 的原材质槽 → 游戏记录干净源表 → 重应用当前 Mod 材质

eiem.ini 的 reload 快捷键（默认 F10）/管理器：提交 Reload 请求
Unity 主线程：恢复旧效果 → 读取/发布新 program → 重应用已登记实例

mod Key：有序按键事件 → 提交 Reapply 请求
Unity 主线程：求值新状态 → 恢复受影响 mod 的旧效果 → 发布新动作并重应用（不重读 INI）
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
- v40：现有生命周期入口观察到的模型先登记弱引用，再判断规则；整个 INI 为空也不会丢掉观察信息。
  F10 检查已观察模型的弱引用及原生存活状态；无效目标移出，未知状态明确报告并跳过。
  不通过强引用保活所有模型，也不增加每帧全场景扫描。
- Mesh 写入前记录恢复责任，不以 setter 返回成功作为唯一依据。未进入 setter 的构造失败不拥有 Mesh 写入。
  源 Mesh 快照在恢复期间保留托管引用，但仍须检查原生有效性；GC 引用不等于禁止游戏卸载原生对象。
- 恢复记录仅在读回成功或目标失效后释放；失败保留基线并标记待恢复，阻止此对象的新规则覆盖基线。
  仅用户重载/状态重应用再次尝试，没有后台无限重试或备用替换路线。
- v41：Mesh、Material、Texture 共用生成资源缓存的原生有效性判断。托管句柄非空不是命中条件；
  原生有效才复用，失效则淘汰该条目并按声明重新构建，未知状态明确报错，不返回给 Renderer。
  同文件时间戳重建也替换旧条目。v43 的 Mesh 保持文件里的骨骼槽顺序，缓存仅保存资源与
  不可变骨骼路径/哈希；每个 Renderer 的 Transform 数组独立构建，禁止缓存或混用实例指针。
  缓存快照共享托管引用，原生有效性检查在锁外执行；这不阻止游戏卸载原生资源。
- 材质缓存命中前仍解析配置引用的 Texture，核对实际贴图引用；贴图重建而文件未变时，
  重新克隆并装配材质，不原地修改已有消费者共享的旧材质。资源有效并成功保留后才输出构造结果。
  不增设整表强制清空、后台轮询或自动保活所有游戏资源；未覆盖的归池持有者问题仍须另行验证。
- v42：`RendererInfo._Init` 调用前，只用该实例已保存的材质基线恢复 EIEM 拥有的槽，
  不改其他槽；游戏原初始化照常执行，从 Renderer 读取干净源材质。
  初始化期间对同一 Renderer 暂停 EIEM 材质重应用，退出后按当前有效规则重新应用。
  嵌套初始化按 Renderer 区分，不全局暂停其他模型，不写控制器原生字段或猜测偏移。
  暴露基线不释放原有恢复责任；失败记录错误、保留基线，不跳过游戏初始化，也不伪称成功。
  这修复已实证的材质源表污染，不宣称已修复未知的绘制端 Mesh 缓存。
- v59：实机发现两个 Typhoea Renderer 只经过 `RendererInfo._Init`，没有经过 `sharedMesh` setter、
  已注册模型根或能命中的 `SetSMRRootBone` 数组。游戏完成 `_Init`、保存干净源材质之后，
  具体 SkinnedMeshRenderer 进入现有全局 Mesh 身份执行器；PFB、NPC 类型和角色身份均不参与匹配。
  后续实机记录多个 Typhoea body/cloth 在该入口分别完成 Mesh 替换与 skip，用户确认 NPC 画面已生效。

## 5. 资源与 Blender

顶点数据的现行约束及已撤回建议见 [顶点数据契约](vertex-data-contract.md)。

- 离线包、Blender、DLL 使用 EIEMESH 与独立 .mat、贴图；不要求 FBX 中转。
- 材质 .mat 指明游戏逻辑 `source=`，运行时加载该源材质再克隆。
  例如 `texture._BaseMap=TextureCloth`、`float._SomeParameter=0.2`；
  参数名来自实际材质，不编造通用游戏 Shader。
- PNG 创建纹理时用声明的 linear/mipmap/sampler 设置；用户无需手工 DDS 压缩。
- Blender 按资源去重，不为每个 Renderer 实例生成重复 Mesh/骨架；LOD 分集合便于编辑。
  材质贴图路径优先显示，UV、颜色、形态键使用对应 Blender 数据。
- 导出显式选中的资源及依赖；相机关闭的物体只表达游戏隐藏，不写其网格和材质/贴图依赖。
  未选中的独立源资源不生成规则，眼睛/显示器不影响游戏显隐。新增网格通过明确 Render/partner 关系表达，
  不靠合并模型继承一堆不透明自定义属性。
- Blender 0.5 用集合表达切换组/状态，一组一个按键，多组独立。同源拆分部件汇总为一个命中 Render，
  各部件以条件 partner 装配。0.8.1 移除物体范围的自动闭包，所需款式/同源拆分部件必须显式选择。
  同源几块仍是一个游戏目标，不选某一块不代表能保留原 Mesh 中对应的部分面。
- Blender 0.6 在 Mesh 数据块保存形态键滑条作者数据，导出 `Slider`/`shape.名称` 绑定。
  权重属于 Renderer 实例；ImGui 与按键只写 Mod 变量，纯权重更新不重建资源。
  [形态键控制](shape-controls.md)记录实现和未验收的游戏自定义 GPU 阶段。
- 旧 EIEMESH 输入版本及来源元数据不是旧运行时替换引擎。仍在使用的离线包
  Reader 不因为“清理兼容”就任意删除，除非有迁移和等价验证。
- 输出先在临时目录完成校验和文件构建，再更新目标；输入或构建出错不清空旧包。
  最后复制阶段的磁盘故障并非跨文件原子提交，仍须另行处理。
- 骨架编辑、同路径改像素的增量依赖等仍存在已记录限制；
  本文不宣称这些已完整实现。

## 6. if/endif、按键实现位置

实现采用用户确认的职责，具体语法见 [conditional-keys.md](conditional-keys.md)：

1. 解析器保留有顺序、可嵌套的语句信息；不在读文件时简单删除“不成立”的行，
   否则按键后无法重新求值。
2. 变量/按键改变求值状态；状态层不直接调用 Unity，也不复制每种 hook 的逻辑。
3. 求值结果交给现有 Render 执行模块；实例观察登记不依赖当前是否有规则，修改记录只在命中并准备修改时建立。恢复与重应用走 Reapply，
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
- `tests/test_blender_switches.py`：实际 Blender P 分离、作者工程保存/重开、导出，再由真实 DLL 解析器切换六次。
- `tests/test_partner_controls.py`：实际退休与排除函数在模拟 Unity 调用下的顺序/所有权测试。
- 其余静态合同检查是代码组织护栏，不等同于游戏行为测试。
- [v31 历史架构快照](archive/architecture-review-v1.md)
- [UI 回调崩溃实证](archive/ui-callback-crash-v29.md)
- [历史设计与全部实验记录](archive/model-replacement-experiments.md)
