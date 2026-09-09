# 历史验证与部署记录

状态：历史档案。以下记录按原版本保留，不代表当前工作区构建、部署或游戏验收状态。
当前功能与未完成项见[文档索引](../README.md)。历史备份路径仅供取证，不是当前安装指南。


## 作者状态与材质 v47

### 2026-09-06 交付

- MSVC 环境下完整回归 **94 项通过**，包含实际 Blender 5.0.1 后台测试。
  最后补充的材质元数据构建、导入操作注册检查也通过；DLL Release 构建通过，diff 空白检查通过。
- 中性临时工程验证新增/原生形态键区分、无 UI 自动变量、保存重开、独立副本、稳定款式编号、
  跨角色同名材质/贴图、增量材质重新导入、改贴图重新导出、坏图片回滚。未打开或编辑用户工程。
- 数值存储验证 F10、重启、跨 Mod 隔离、配置注释不会恢复动作、坏值拒绝、原子写入失败后的重试。
  这些是程序/文件测试，不表示地图与 GPU 管线已完成实机验收。
- 已在游戏未运行时部署 `plugin/eiem.dll`；SHA256：
  `2DC3590245DB6ED7A19C14F2C1EF5E3905A47078DB5AC7D61347D78AD7B74031`。
- Blender 三个发布文件同步至 `E:\vscode\EIEM_Blender`；5.0 用户 addons 中的同名目录为指向它的 junction。
  未强制重载正在运行的 Blender、未保存其偏好或场景。使用者先保存工程，再运行开发环境 Reload Addons。
- 更新前备份：`E:\EIEM_Workspace\plugin-releases\v47-author-state-20260906-2139`。
  现有三份 Mod INI 的哈希保持不变；未部署全局配置模板、代理 DLL 或任何模型/贴图。
- 待实机验收：新导出 Mod 的常量驱动形态键、玩家调整后换图/F10/重启保留、跨角色材质实际渲染。
  老 Mod 不会自动新增 persist 或 shape 绑定，需从 .blend 重新导出；F10 不能更新 DLL 本身。


## Lua UI v37 / v38

### 2026-09-05 v37 历史记录（窗口策略已由 v38 取代）

- 当次工作没有修改顶点数据；切线的现行契约与历史实验统一见 [顶点数据契约](../vertex-data-contract.md)。
- C++ 真实 Lua VM + ImGui 帧：变量隔离、重载重置脚本局部状态、原生滑块点击、× 关闭、
  错误后丢弃写入、死循环/内存超额、作用域不平衡后另一个窗口可继续绘制。
- 解析与发布：UI 条件、重复字段、路径、保留键冲突、跨 Mod 变量隔离、批量拒绝和旧代事件丢弃。
- 热键线程模拟 OS：Mod UI 有焦点时 UI/全局刷新仍可用，不注册模型循环键；跨 UI 共用键去重。
- 后台 Blender 保存重开作者工程，导出按钮/滑条 Lua；实际 C++ 解析器和 Lua VM 读取执行生成结果。
- 真实 DirectComposition 隐藏宿主：初始化、关闭、再次创建和关闭时刷新。测试发现并修正
  `CreateContext` 不自动替换已有当前 context 的问题，防止 Mod 后端绑定到插件主 context。
- 游戏内鼠标穿透、焦点/光标与透明合成尚待验收；新增形态键的游戏 GPU 变形也仍待独立验收。
  不把离线 UI 测试通过当作这两项已通过。

### v37 历史构建与部署

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

### 2026-09-05 v38 验证与部署

- 删除 DLL 的 Mod UI 开关状态、专用快捷键队列、强制单窗口限制及固定错误窗口；
  UI section 仅声明脚本路径，普通 Key 的 `scope` 选择游戏/UI 焦点范围，Lua 自己处理窗口状态。
- 真实 Lua/ImGui 测试覆盖无快捷键、多窗口、`###` 稳定 ID、脚本关闭/重新打开、关闭不改模型变量；
  Blender 测试覆盖可选模板、不生成 UI、无快捷键常显及用户自定义按键，导出结果经实际解析器/Lua 执行。
- 完整 `unittest discover -s tests -q`：63 项通过，耗时 23.318 秒；`build.bat` 成功，
  构建标识 `resource-runtime-v38-script-owned-ui`；`git diff --check` 无空白错误。
- 确认游戏进程未运行后更新 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`，
  SHA256：`95B24C372E61871B4BA6DF138C96D888FC8589E43A8748E2104E70DE7AF5BB24`。
- `E:\vscode\EIEM_Blender` 的三个插件文件已同步为 0.8.0 并逐一核对哈希；未修改开发环境设置、
  已打开的 Blender 工程、现有 Mod INI、模型或贴图。全局 `eiem.ini` 哈希保持不变。
- 旧 DLL、全局配置副本及三个旧插件文件备份在
  `E:\EIEM_Workspace\plugin-releases\before-v38-script-owned-ui-20260905-211757`。
- 开发环境 Reload Addons 后，在 N → EIEM → 网格切换中按需勾选“生成简单 UI”；
  默认不生成、不预填按键，按键留空则生成常显窗口。自定义 Lua 布局不是 DLL 内置布局。
- 此次 DLL 需下次启动游戏加载；后续修改 INI/Lua 使用配置的全局刷新键，不自动轮询文件。
  旧 `[UI] key/condition` 写法不保留兼容分支，须重新导出或改为普通 Key 变量与 Lua 条件。
- 尚未进行游戏内透明显示、鼠标穿透及焦点交互验收；离线测试通过不代表这些已在游戏中验证。
  未提交 Git。


## Skeleton v48

## 6. 构建与部署

本轮只在仓库构建和测试，未覆盖游戏 DLL、开发目录插件、用户 Mod 或 `.blend`。
最终完整回归：MSVC 环境设置 `EIEM_BLENDER=G:\blender5.0\blender.exe` 后执行
`python -m unittest discover -s tests -q`，**97 项通过，无跳过**；`build.bat` 完整构建通过。
产物 `bin/eiem.dll`（1,721,344 字节），已核对其中的 v48 标识；SHA256：
`07B91B1E4EBAE3817BDD8146AFD77A00E3C5ABC43B203E46F241A1CDE985AD5D`。

需要升级 DLL 和 Blender 插件后重新导出包含新增骨骼的包；
F10 只能重读包，不能更新已加载的 DLL。


## 共享骨架绑定 v43

## 构建与部署记录

- 最终完整测试：`python -m unittest discover -s tests -q`，89 项通过，无跳过；包含 Blender 5.0.1、
  MSVC 实际解析器/绑定实现及 Lua 测试。DLL Release 编译通过。
- 已同步 `E:\vscode\EIEM_Blender` 三个插件文件。安装目录的 EIEM_Blender junction 指向该目录。
  MCP 热重载后版本 0.8.3；用户此时 1,251 个对象、选择“平面”，显隐、作者权重、控件数据均未变。
- 使用已安装导出器对 body 再次临时导出：14,665 顶点、76 槽、全零权重 0、原槽保持。
  输出目录：`C:\Users\25487\AppData\Local\Temp\eiem-live-shared-skin-vug0_15z`。
  没有把临时 Mesh 写进游戏 Mod，也没有改用户选择、INI 或 `.blend`。
- 确认 Endfield 进程未运行后，已部署 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。
  标识 `resource-runtime-v43-shared-skeleton-binding`，SHA256：
  `A0B666FC6B34A3F5F349947A5346168B0D406E05CD9103643E29AE0DB321B040`。
- 原 v42 DLL 与原 Blender 三文件备份：
  `E:\EIEM_Workspace\plugin-releases\before-v43-shared-skin-20260906-162758`。
- 尚未进行 v43 游戏画面验收；用户需重新导出模型，让新增骨骼与权重进入游戏资源文件。


## 相机反虚化 v35

### 2026-09-05 v35 验证与部署

- 先跑新 hook 契约测试：旧 adapter 上 3 项失败于 CameraMono 安装；反例及缺失接口拒绝安装测试通过。
  替换实现后，5 项真实 MinHook + 模拟 CameraMono 测试与 1 项全局配置测试全部通过。
- 全套 `py -X utf8 -m unittest discover -s tests -v`：57 项，55 通过、2 项 Blender 联测因未设置
  `EIEM_BLENDER` 跳过。本轮未修改 Blender 插件或用户场景。
- 首次构建命令在外层先调用 vcvars64，build.bat 内又调用一次，因本机 PATH 引号导致环境脚本报错；
  改为直接 `cmd /c build.bat` 后编译成功，未为此修改系统 PATH 或构建脚本。
- `git diff --check` 通过，仅有既有 LF/CRLF 提示。活动 src/tests 中旧恢复表、旧 setter hook、
  `EiemRefreshCameraFade` 和专用 `il2cpp_gchandle_new_weakref` 引用均已清除。
- 确认游戏未运行后，仅部署 `plugin/eiem.dll`（1,214,464 字节）。构建产物与已安装文件 SHA256 一致：
  `9C7D6E89E8C627D3DD3A3DB9D8DE89DF361B3CF5E190FC5766ACEDA7077F49D7`。
- 部署前 DLL 与全局配置备份在 `E:\EIEM_Workspace\plugin-releases\before-v35-camera-mono-fade-20260905`。
  原 v34 DLL SHA256 为 `E9EBD359C693BCF4F64279985455F59B29EB7FFF09CB2743F431AF449F004B04`。
- 已安装 `plugin/eiem.ini` 保留 `disable_camera_fade=true`、F10、INSERT，仅修正功能注释。
  未部署构建产生的代理加载器，未修改任何 mod.ini、Mesh、Material、Texture 或 Blender 文件。
- 尚未启动游戏，没有本版游戏内 observed/cleared 记录或画面验收；不得以本节自动测试宣布视觉效果成功。


## 形态键控制 v36

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


## Blender 切换组 v33 / 0.5～0.8.1

2026-09-05：

- 50 项自动测试全部通过，包含真实 Blender 5.2 后台与真实 C++ 解析器联测，没有跳过该联测。
- 当时的 Typhoea 离线包往返：62 Mesh、1 骨架资源，包内已有的 UV/顶点色/法线/切线/形态键检查通过。
  这不是原始 AB 通道完整性证明：后续调查发现压缩 NORMAL 中的切线未被旧解析器导出，
  空数组也能通过包级往返。原生解码与重新生成的区别见 [顶点数据契约](../vertex-data-contract.md)。
- 单 Mesh 增量回归：1 Mesh + 1 修改材质 + 1 PNG；空材质槽回归通过。
- `build.bat` 成功；部署到 `D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。
- DLL 日志标识 `resource-runtime-v33-blender-switch-groups`，SHA256
  `B6A23D3358E361FD6A237FD1930F84295EDB4610C7AC09477688CCB986654FD4`。
- 新建 `plugin/eiem.ini`，默认 `reload=F10`、`gui=INSERT`。未改现有 mod.ini、用户模型或 `.blend`。
- 原 DLL/开发插件备份：`E:\EIEM_Workspace\plugin-releases\before-v33-switch-groups-20260905`。
- 源资源读取和后台测试均在独立 Blender 进程完成；未连接/改动用户交互场景。
- 尚未验证游戏自定义渲染管线对新增 partner 的全部内部状态，以及大世界/UI/切图的实际图像。
  不把解析正确或后台 Blender 正确等同于游戏画面已经通过。

### 开发目录入口修正（2026-09-05）

- 症状：开发环境启动 Blender 后，导入菜单没有 EIEM。
- 已确认原因：此前只更新 `E:\vscode\eiem_blender_addon.py`，开发工作区实际上是
  `E:\vscode\EIEM_Blender`，其中仍是旧代码；其 `__init__.py` 也没有 `bl_info`。
  Blender Development 启动日志 `ADDONS_TO_LOAD: []`，没有尝试加载该插件。
- 修正：开发目录换成完整 0.5.0；增加带字面量 `bl_info` 的包入口，并以相对导入调用新版实现。
  没有保留旧 `eiem_format.py`、`eiem_export.py`、ZIP 或旧缓存作为加载分支。
- 完整旧目录备份：`E:\EIEM_Workspace\plugin-releases\before-blender-dev-entry-20260905-134201`。
- 回归：`tools/Blender/test_eiem_registration.py` 在旧目录稳定复现发现失败；新目录通过
  本机 Blender 5.0.1 的真实插件发现、注册、菜单检查及三轮 disable/清除模块/enable 生命周期。
  测试还检查包入口与实现版本一致、场景对象未改变。`tests/test_blender_registration.py` 将其纳入测试。
- 本轮没有交互 Blender 进程可连接，验证使用独立后台进程；用户下一次 **Blender: Start** 会重新发现包。
  未改 `.blend`、模型、材质或 DLL；此验证不代表游戏运行效果验收。

### 显式选择与游戏隐藏（2026-09-06，0.8.1）

- 原实现不读取对象相机开关。先前“已支持相机关转 skip”的口头说明不正确；本轮才实现。
- 使用原生对象 `hide_render`，不增加另一份含义相同的作者属性；眼睛和显示器不参与判断。
- 移除自动添加同源/同组物体；保留一个源对应一个命中 Render 的规则，避免同源规则互相竞争。
- 新增真实 Blender 后台测试：混合隐藏/替换、未选同源副本不加入、隐藏对象无资源依赖、
  眼睛/显示器不影响导出、重新导出移除旧生成资源、选择切换组子集、同源拆分混合显隐、
  相机状态保存重开。使用实际 C++ INI 解析器验证纯 skip 和 skip + partner。
- 现有插件注册、形态键/Lua 与两组按键切换测试通过；本轮不宣称新增操作已在游戏画面验收。
- 本轮全量 `unittest discover -s tests -v`：83 项通过，无跳过；包含 Blender 5.0.1 和 MSVC 原生解析器联测。
- 已同步 `E:\vscode\EIEM_Blender`（Blender addons 中为指向该目录的链接），通过 MCP 热重载为 0.8.1。
  当前工程 1248 个对象、选中物体、相机状态和形态键控制数据均未改变；未保存或覆盖用户 `.blend`。
  实际选中 1 个物体时计划也只有该 1 个物体，没有扩展到其余角色资源。
- 旧三文件备份：`E:\EIEM_Workspace\blender-addon-backups\before-selected-export-20260906-142927`。
  本轮未导出用户 Mod，未修改游戏目录的 INI、DLL 或已有资源。


## 条件与按键 v32

2026-09-05 验证结果：`python -m unittest discover -s tests -q` 全部 48 项通过；
`build.bat` 构建成功，`git diff --check` 无空白错误。
本次 `bin/eiem.dll`（`resource-runtime-v32-conditional-keys`）SHA256：
`29F777CF6BD7A4052CA9234F4542072DBC628CAC201B61B17BBD2DF1B3790A3E`。



## 已移除的相机反虚化 v34 方案

2026-09-05 的真实日志中，Renderer setter Hook 未命中；用户仍观察到虚化。
v35 改用 CameraMono 每次评估后的清理，移除了旧 Hook、逐 Renderer 弱引用恢复表和对应刷新调度。
旧方案的配置和操作说明已经删除，当前契约见[相机反虚化](../camera-fade.md)。
