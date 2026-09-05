# 形态键与 ImGui 控制

状态：v36 建立作者入口、导出、INI 求值和 Renderer 权重接口；v37 将固定滑条页迁移为独立 Lua UI。
现行窗口与脚本约定见 [Lua UI](lua-ui.md)。下方 v36 测试/部署段为历史记录，不表示 v37 游戏验收。
形态键进入 EIEMESH 不代表终末地自定义 GPU 管线已参与变形；API 写入/回读与游戏画面验收分开记录。
本轮不修改用户现有模型、Mod 配置或场景。游戏画面尚未验收。

## 职责

- Mesh 资源包含形态键的名称、帧、顶点/法线/切线增量；不包含运行时实例权重。
- Render 的 `shape.名称=数值或变量` 控制命中实例或 partner 的形态键。名称区分大小写。
- UI `[UI...]` 指定 Lua 文件与独立开关键；Lua 只写当前 Mod 的变量，不持有 Unity 对象，不独立实现资源替换。
- 快捷键与滑条进入同一有序输入队列。F10 重载会丢弃旧代事件，恢复旧效果并重置默认值。
- 纯权重变化不重载磁盘，不重设 sharedMesh、骨架或材质；如果同一变量使 if 分支改变装配，走现有重应用流程。
- 通道新增、移除、改名同样重应用，以覆盖之前仅登记命中、尚未持有形态键状态的实例。
- 同一 Mesh 被多个实例共享时，权重逐 Renderer 设置；不会往共享 Mesh 写一个全局权重。

## 最小语法

```ini
[Constants]
$size=0

[UISize]
path=ui.lua
key=F8

[MeshCloth]
path=meshes/cloth.mesh

[RenderCloth]
asset=SourceCloth
mesh=MeshCloth
shape.Inflate=$size
```

权重按 Blender 习惯使用 0–1，1 对应 Unity 的 100。可声明其他有限范围。
滑条范围在 Lua `imgui.SliderFloat` 中给出；导出器仍校验作者设置的范围和默认值。
形态键赋值支持现有数值表达式语法，变量须在 Constants 声明；不引入另一套表达式语言。
支持比较、逻辑和一元正负号，不包含四则运算。单 Render 最多 64 个不同通道；通道名称 UTF-8 小于 192 字节。
`shape.Inflate=` 撤销本规则对此通道的控制；可以放在 if/endif 中。
INI 不重新描述形态键的几何数据。FrameWeight 是形态帧的位置，不是滑条当前权重。

F8（上例配置）打开独立 Mod 窗口；INSERT 只打开插件管理面板。相同变量名在不同 Mod 中隔离。
`ui.lua` 的完整绘制例子见 [Lua UI](lua-ui.md)。旧 `[Slider...]` 不再解析，旧工程重新导出即可。
所有 Unity 调用仅在现有 Unity 线程调度中执行；ImGui 绘制线程仅提交变量值。
按键可以继续用 `type=cycle` 写同一变量；当前输入顺序决定最终值。

## 生命周期与错误

只恢复本规则拥有的形态键通道。新增实例使用当前变量；销毁随现有实例清理；
换 Mesh 后按名称重新解析索引，不跨 Mesh 复用索引。
源游戏没有对应形态键不妨碍替换 Mesh 新增通道。静态 MeshFilter 不提供 SMR 权重接口，明确报错。
找不到通道、调用失败、回读不一致要报告，不能回退成索引 0，不能报告画面成功。
Mesh 构建使用显式异常通道检查 AddBlendShapeFrame；完成后核对通道数、名称与帧数。
Renderer 权重写入后也要读回验证。原生权重错误写入日志 `[SHAPE]`；Lua UI 自身错误在独立窗口与 `[UI]` 日志报告。
替换 Mesh 且带形态键控制时先保存源 Renderer 权重；撤销时分别恢复替换 Mesh 的受控通道和源 Mesh 的原权重。
形态键可能改变包围盒；超出原包围盒、游戏自定义形态键 GPU 通道及游戏动画后续覆写仍需真实渲染验证。

## Blender 作者流程

在原生形态键面板制作 Basis 与相对 Basis 的形态键，选中要控制的键（不能是 Basis）。
进入网格数据属性（绿色三角图标）→“EIEM 形态键控制”→“为当前形态键生成控制滑条”。
面板显式提供通道选择、标签、默认值和范围；不要求用户手写 INI。
保存 .blend 保留作者数据；导出选中 Mesh 时生成 Constants、UI、ui.lua 和对应 Render 的 shape 绑定。
同源拆分自动生成 partner 时，绑定属于使用该 Mesh 的 partner，而非被 skip 的源 Render。
共享 Mesh 数据只导出一份资源/滑条；不同 Mesh 的同名形态键分别控制。
V1 不伪装支持绝对形态键、非 Basis 相对键或顶点组遮罩：遇到这些明确拒绝导出，避免静默变形错误。
新形态键顶点增量必须与 Basis 使用相同顶点数/顺序；不删除零权重骨骼组。
原生多帧形态键数据继续保留，但 Blender 0.6 暂不为其生成控制滑条，选择时明确报错。
新增键导出顶点增量；源形态键已有法线/切线增量保留。新增键不会自动生成游戏专用法线/切线增量，
因此当前不能承诺大幅变形后的光照与游戏一致。不要把顶点位置成功当成着色全部通过。
控制数据保存在 Mesh 数据块，旧 .blend 更新插件后重新导出即可；无需用户手写 INI。

## 验证记录（2026-09-05）

- `tests/test_shape_controls.py`：实际解析/求值、跨 Mod 隔离、按键/滑条输入顺序、旧代事件丢弃、
  通道归属、原始权重恢复、Mesh 换索引、部分写入失败可恢复、极小数值精度、坏字段/坏范围拒绝。
- `tests/test_shape_runtime.py`：抽取实际 IL2CPP 适配代码，模拟原生对象测试写入/回读、线程限制、
  静态 MeshFilter 拒绝、异常与空返回、静默写入失败、通道元数据不一致。
- `tests/test_blender_shapes.py`：独立后台 Blender 5.0 临时工程新增形态键，绘制控制面板，保存重开，
  导出形态键顶点增量/INI，再由 DLL 的真实解析器读取；覆盖共享资源、拆分 partner、同名键隔离。
- `build.bat`：编译 `resource-runtime-v36-shape-controls`。这不是 GPU 画面测试。

待游戏验收：0 → 1 → 0、同变量快捷键/if 分支、F10、同资源多实例、地图/UI 切换。
如果权重回读正确而画面不动，说明还未证明游戏自定义 GPU 变形阶段消费这些数据；
下一步必须查实际变形输入，而不是重复添加 setter、换骨架或猜测索引。

## 部署（2026-09-05）

- 全量 `unittest discover -s tests -q`：61 项通过（设置 `EIEM_BLENDER=G:\blender5.0\blender.exe`，含后台 Blender）。
- `build.bat` 构建成功；`git diff --check` 无空白错误。
- 已确认游戏进程未运行，更新 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。
  SHA256：`B4BFA0CBA05D7BD9DFD1DDF545459819A715057862CFEE914B239F345309450F`。
- `E:\vscode\EIEM_Blender` 的 `__init__.py`、`eiem_blender_addon.py`、`README.md` 已同步 0.6.0 并逐文件核对哈希。
- 旧 DLL、全局 INI 及上述三个旧插件文件备份到
  `E:\EIEM_Workspace\plugin-releases\before-v36-shape-controls-20260905`。
- 全局 INI 哈希未变，未覆盖用户 Mod、模型或 Blender 工程；没有提交 Git。
- 更新 DLL 后启动游戏加载；Blender 开发环境执行 Reload Addons。
  Mod 文件修改使用全局刷新（默认 F10）；运行时拖动滑条不需要刷新。
