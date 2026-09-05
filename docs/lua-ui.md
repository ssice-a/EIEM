# Mod Lua UI（v37 / Blender 0.7）

本轮只接入独立 Mod UI、Lua 与现有变量系统；不修改用户模型、现有 Mod 配置或游戏资源替换算法。
INSERT 仅控制插件管理面板。原来的内置 Mod 滑条标签页及 Slider 配置不再作为运行时 UI 引擎保留；
Blender 形态键作者数据保留，更新导出器重新导出为 Lua UI。

## 文件与语法

```ini
[Constants]
$size=0

[UICloth]
path=ui.lua
key=F8
; 可选，使用已有变量表达式；不代表已支持视锥/可见 Mesh 探测
condition=$size >= 0
```

一个 UI section 对应一个可独立开关的 ImGui 窗口、一个 Lua 环境。多个 section 可共用键，
仅条件成立的窗口响应。UI 键不能用 INSERT、不能与本 Mod 的循环按键重复；全局键冲突明确报错。
条件变为假会关闭窗口，不会等条件恢复后自动弹出。多个窗口共用快捷键时，该键切换每个符合条件窗口自己的开关。
没有自动轮询磁盘。默认 F10 重读 INI 与 Lua，关闭 UI 只隐藏窗口，不改变模型变量。
相对 Lua 路径限制在 Mod 目录内；不允许绝对路径、上级目录或重解析点越出 Mod 目录。

```lua
return function()
    imgui.SetNextWindowBgAlpha(0.8)
    if imgui.Begin("衣服") then
        local changed, value = imgui.SliderFloat("鼓起", mod.get("$size"), 0, 1)
        if changed then mod.set("$size", value) end
    end
    imgui.End()
end
```

Lua 返回每帧绘制函数。窗口名与控件 ID 由宿主按 Mod/section 隔离。每 section 每帧一个根窗口，
复杂布局用子窗口、分组、表格等；多个独立窗口用多个 section。
绑定常用公共 ImGui 接口，保持原生名称：Begin/End、Button、Checkbox、SliderFloat/DragFloat、
InputFloat、Text、布局、子窗口、表格、颜色/样式等。未绑定接口报错，不假装暴露完整 C++ API。

### 当前绑定签名

参数遵循 Lua 数值/字符串传值，不传 C++ 指针。控件 label 支持原生 `##id` 隐藏 ID。
Begin/End、BeginChild/EndChild 必须配对，即使 Begin 返回 false；BeginTable 返回 true 才 EndTable。
根窗口默认有 × 关闭按钮；主动设置 NoTitleBar 会隐藏标题栏，此时仍可用配置快捷键关闭。

| 接口 | 参数及返回值 |
| --- | --- |
| `Begin` / `End` | `Begin(title, flags=0) → visible`；每 section 每帧最多一个根窗口 |
| `BeginChild` / `EndChild` | `BeginChild(id, width=0, height=0, border=false) → visible` |
| `Button` / `RadioButton` | `(label, width=0, height=0) → clicked` / `(label, active) → clicked` |
| `Checkbox` | `(label, value) → changed, newValue`，value 是布尔值 |
| `SliderFloat` | `(label, value, min=0, max=1) → changed, newValue`，min < max |
| `DragFloat` / `InputFloat` | `(label, value, speed=.01, min=0, max=0)` / `(label, value, step=0)`，均返回 changed, newValue |
| `ColorEdit4` | `(label, r, g, b, a=1) → changed, r, g, b, a` |
| `Text` / `SetTooltip` | `(text)`，按原样文本绘制，不解析 printf 格式 |
| `SameLine` / `Dummy` | `(offset=0, spacing=-1)` / `(width=0, height=0)` |
| `Separator` / `Spacing` / `NewLine` | 无参数 |
| `SetNextWindowSize` / `SetNextWindowPos` | `(x, y)`，FirstUseEver，允许用户后续拖动/缩放 |
| `SetNextWindowBgAlpha` / `SetNextItemWidth` | `(alpha=1)` / `(width)` |
| `PushStyleColor` / `PopStyleColor` | `(imgui.Col.xxx, r, g, b, a=1)` / `(count=1)` |
| `PushStyleVar` / `PopStyleVar` | `(imgui.StyleVar.xxx, value[, value2])` / `(count=1)` |
| `PushID` / `PopID` | `(string)` / 无参数，作用域内配对 |
| `BeginDisabled` / `EndDisabled` | `(disabled=true)` / 无参数，作用域内配对 |
| `BeginGroup` / `EndGroup` | 无参数，作用域内配对 |
| `BeginTable` / `EndTable` | `(id, columns)`，1–64 列 / 无参数 |
| `TableNextRow` / `TableNextColumn` | 无参数；后者返回列是否可见 |
| `CollapsingHeader` | `(label) → open`，不需要 TreePop |
| `GetItemRectMin` / `GetItemRectMax` | 无参数 → x, y |
| `IsItemHovered` | 无参数 → hovered |

枚举：`imgui.WindowFlags` 提供 NoTitleBar、NoResize、NoMove、NoBackground、AlwaysAutoResize，可用 Lua `|` 组合。
`imgui.Col` 提供 Text、WindowBg、Button、ButtonHovered、ButtonActive、FrameBg；
`imgui.StyleVar` 提供 Alpha、WindowPadding、WindowRounding、FrameRounding、FramePadding、ItemSpacing。
未提供 Image/外部纹理句柄、FFI 或所有 ImGui 重载；需要新绑定时增加这一个边界，不修改 Render 语义。

## 状态与执行

- UI 只通过 `mod.get("$变量")` / `mod.set("$变量", 数值)` 访问本 Mod 已声明变量。
- 一帧脚本写入先暂存，绘制成功才通过统一输入队列提交；多个变量作为一批提交。
- ImGui 线程不调用 Unity；现有 Unity 线程求值 Render，保留形态键纯权重更新路径。
- 独立透明原生宿主承载所有 Mod ImGui 窗口，不嵌入插件面板，不 hook 游戏图形管线。
  在现有 GUI 线程调度，使用独立 ImGui context 和绘制资源；鼠标不在 Mod 窗口时穿透到游戏。
- 全局刷新在帧边界销毁旧 Lua 状态、清除旧交互；事件带配置代号，旧代事件不可落入新配置。
  保留仍存在窗口的开关状态；变量默认值沿用 F10 既有重置语义。

## 错误与权限

每 UI 独立 Lua 状态，限定内存/每帧指令预算。仅开放基础运算、math/string/table/utf8；
不开放 io/os/package/debug、文件加载、动态库或 Unity 指针。不把它宣传成进程级安全沙箱。
Lua 错误或 ImGui 栈不平衡会停止该窗口脚本并显示文件/行号；清理当前帧栈，不执行半帧变量写入。
其他窗口继续工作。F10 修正并重载，不保留旧脚本静默回退。
当前 Lua 5.4.9：脚本文件最多 256 KiB、Lua 分配预算 16 MiB、每次执行约 200,000 条 VM 指令。
不开放 Lua 侧 pcall/xpcall/coroutine，避免脚本反复捕获指令预算错误。宿主用受保护调用和 C++ 异常展开。
指令限额不是原生库函数的硬实时中断；不承诺恶意脚本在同进程绝对安全或任意复杂 UI 都无帧耗。

## Blender

保留现有网格切换组和形态键控制面板；增加 Mod UI 开关键/标题设置。
有切换组或形态键控制时自动生成 `ui.lua`、UI section，按钮控制切换组，滑条控制形态键。
旧 .blend 无需修改格式；生成脚本属于导出产物，再导出会更新生成脚本。

## 验收

真实解析器与 Lua/ImGui 运行测试：变量隔离、滑块/拖动、批量提交、关闭/独立开关、条件、
全局刷新代际、语法错误/超预算/不平衡栈/路径拒绝、另一个 UI 不受影响。
后台 Blender：作者数据保存重开、生成 Lua、INI 由实际解析器读取、脚本由真实 Lua VM 执行。
原生宿主进行独立隐藏窗口测试，不启动或修改用户游戏；真实游戏输入/透明显示结果另行记录。

### 2026-09-05 本轮记录

- 保留源切线的现行结论已归一到顶点数据契约；115/120 是修复前调查。本轮没有修改顶点数据。
- C++ 真实 Lua VM + ImGui 帧：变量隔离、重载重置脚本局部状态、原生滑块点击、× 关闭、
  错误后丢弃写入、死循环/内存超额、作用域不平衡后另一个窗口可继续绘制。
- 解析与发布：UI 条件、重复字段、路径、保留键冲突、跨 Mod 变量隔离、批量拒绝和旧代事件丢弃。
- 热键线程模拟 OS：Mod UI 有焦点时 UI/全局刷新仍可用，不注册模型循环键；跨 UI 共用键去重。
- 后台 Blender 保存重开作者工程，导出按钮/滑条 Lua；实际 C++ 解析器和 Lua VM 读取执行生成结果。
- 真实 DirectComposition 隐藏宿主：初始化、关闭、再次创建和关闭时刷新。测试发现并修正
  `CreateContext` 不自动替换已有当前 context 的问题，防止 Mod 后端绑定到插件主 context。
- 游戏内鼠标穿透、焦点/光标与透明合成尚待验收；新增形态键的游戏 GPU 变形也仍待独立验收。
  不把离线 UI 测试通过当作这两项已通过。

### 构建与部署

- 本轮完整 `unittest discover -s tests -q`：63 项通过，包含 MSVC、真实 Lua/ImGui 和后台 Blender。
- `build.bat` 构建成功，标识 `resource-runtime-v37-lua-ui`；`git diff --check` 无空白错误。
- 确认游戏进程未运行后，已更新 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，
  SHA256：`FF2EBA6AFB41A833FFBAABCAA492876002550C33540A61AF6AB969465A336CE8`。
- 同步 `E:\vscode\EIEM_Blender` 的三个插件文件，逐个核对哈希；没有修改开发环境设置或打开的工程。
- 旧 DLL、全局配置及三个旧插件文件备份在
  `E:\EIEM_Workspace\plugin-releases\before-v37-lua-ui-20260905-203541`。
- 全局配置哈希未变；现有 Mod 配置与模型未修改，没有重新导出用户资源，没有提交 Git。
  Lua 许可证随 DLL 放置为 `LUA-LICENSE.txt`。
- Blender 执行 Reload Addons，N → EIEM 设置 UI 开关键/标题，重新导出需要按钮或滑条的 Mod。
  DLL 更新需要下次启动游戏加载；随后 Lua/INI 文件编辑才使用全局刷新（默认 F10）。
