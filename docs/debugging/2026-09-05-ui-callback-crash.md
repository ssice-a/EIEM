# v29：进入角色 UI 闪退（v30 修复）

## 症状和范围

用户确认：大世界替换正常，进入角色 UI 闪退。此次不改 INI、Mesh、
材质或蒙皮。不要把异常归咎于用户的几何体，也不要以关闭 UI 替换作为修复。

- 出错 DLL：`renderer-mesh-identity-v29`。
- SHA256：`AF29EFFF66E8D6A296C3AF58F9CBC2E6BA1BE80ABC0BD20C36A53693904AF25A`。
- 本地异常时间：2026-09-05 04:08:31 左右；Windows 事件记录于 04:08:33。
- PID 10916，异常线程 20172（Unity 主线程）。
- 异常：`0xC0000005`，`GameAssembly.dll + 0x440D5D8`。
- 用户不需要为收集本次崩溃原因再次进游戏：已有两份转储。

原始证据（本机）：

- `%LOCALAPPDATA%\CrashDumps\Endfield.exe.10916.dmp`
- `%LOCALAPPDATA%\Temp\Hypergryph\Endfield\Crashes\Crash_2026-09-04_200831254\crash.dmp`
- 同目录 `Player.log`，以及游戏 `plugin\eiem_log.txt`。

## 假设与现场核对

依次检查：异步回调 ABI 错误、初始化阶段重复修改状态、替换资源提前失效。
不是仅凭最后一条日志推测崩溃位置，而是读取 MINIDUMP ExceptionStream
中的 ThreadContext（不能用 ThreadList 中已进入崩溃处理器的线程上下文）。

1. 日志出现 `[MOD-UI] async callback wrapped`，加载
   `Assets/Beyond/DynamicAssets/Gameplay/Prefabs/UIModels/chr_0034_typhoea_uimodel.prefab`，
   包装委托地址 `0x1232DDB200`。
2. UI 根 `0x12330ABE60` 已通过 `CharUIModelMono.OnAwake` 和
   `PrefabInstantiateProxy.OnCompleted` 分别送入同一 Render 执行器。
   目标 Renderer 的替换 Mesh 为 `0x12330BCB00`，`applied=true`。
   `payload=120/live=120`，bindpose 读回无字节差异，原骨骼引用未改变。
   **这些只证明写入/读回，不能等同于屏幕渲染成功。**
3. 随后崩溃，未出现 `[MOD-UI] async completed`。实际异常指令如下
   （代码来自崩溃转储，不用磁盘 GameAssembly 的字节代替运行时字节）：

```text
GameAssembly+440D5C4  mov rax, [rdi+40h]  ; callback
GameAssembly+440D5C8  test rax, rax
GameAssembly+440D5CB  je ...
GameAssembly+440D5CD  mov r8, [rax+28h]   ; method
GameAssembly+440D5D1  mov rdx, rsi        ; completed model
GameAssembly+440D5D4  mov rcx, [rax+40h]  ; invoke context
GameAssembly+440D5D8  call qword ptr [rax+18h]  ; invoke_impl: exception

RAX = RCX = 0000001232DDB200  (matches our wrapper in log)
RDX       = 00000012330ABE60  (matches completed UI root)
R8        = 00007FFE61178E00  (eiem.dll + 38E00, native callback code)
```

已定位根因：`EiemCreateUIModelCallback` 调用 Action 构造函数时把
`&EiemUIModelLoadedCallback` 的**原生代码地址**填入托管方法参数，
没有正确的 MethodInfo/委托调用入口。现场 method 字段也确实为该代码地址。
游戏是在调用这个错误委托时崩溃，尚未进入我们的完成函数。
小转储不含整个托管堆，不能声称已经读到了委托所有字段；上面 method/context
取自异常寄存器与紧邻异常的实际指令。

因此，“只是 Mesh 未命中”无法解释这次异常；重新排列骨骼或增加资源回退
也不能修复这一调用 ABI。其他潜在渲染问题仍需独立验证，不能用本结论排除。

## v30 改动

- 删除原生 Action 包装、pending callback 表、手工 GCHandle 管理及专用 Cancel hook。
- `TraceUIModelLoaderLoadModelAsync` 将原始 callback、self、path、parent、
  MethodInfo 全部原样传给游戏；原样返回 request ID。完成时间和取消归游戏管理。
- 复用已存在、此次确实调用的 `PrefabInstantiateProxy.OnCompleted`、
  `CharUIModelMono.OnAwake/SetVisible` 处理完成的模型。不新增按角色枚举的 hook，
  不加扫描、不加异常吞掉后的回退，不改变 Render 语义。
- 同步加载的 UIModelLoader 仍可登记返回的模型；Unload/Clear/Dispose 保留模型释放逻辑。
- 修正旧注释“角色 UI 不调用 UIModelLoader”：此次现场明确反证。
- 优化构建保留本地 PDB，便于下次解析我们自己的异常栈；不把 PDB 放到游戏 mod 中。

## 回归和验收边界

`tests/test_ui_async_passthrough.py` 编译**生产代码中的真实异步 hook 函数体**，
将其接入可控游戏加载器夹具。它验证 callback 指针不变、全部参数和返回值不变，
以及延迟完成、同步完成、空回调、失败、取消、原入口不可用六种情况。
旧包装 helper 在夹具中作为检测哨兵，绝不模拟 IL2CPP 布局或执行非法地址。

- 修复前：六个场景全部失败（调用了包装 helper）。
- 修复后：六个场景通过，原有 28 项静态合同检查也通过。
- 这覆盖造成问题的回调传递边界，**不是**对整个 IL2CPP/游戏渲染的集成验证。
- 本次仍待真实游戏确认：进入角色 UI 不崩溃，修改后的网格/贴图可见；随后退出 UI、
  重开 UI、换图后重开 UI，以及 F10 还原/重载。不能仅以编译或静态测试通过宣称完成验收。

防止重犯：原生函数指针与托管 MethodInfo 不可互换。优先观察已有的完成/组件生命周期，
不要为了取得已能观察到的根对象而重新制造托管委托。仅检测源码包含某个回调名称的
字符串测试不能验证 ABI；旧的这类测试已替换为原样传递合同及可执行边界测试。

## 部署记录

2026-09-05 04:23：MSVC 优化构建通过，游戏进程已退出，v30 已复制至
`D:\Hypergryph Launcher\games\Endfield Game\plugin\eiem.dll`。
构建产物与部署文件 SHA256 一致：
`33A16187B58BD1094D2B3E20184A89334F4728563F588A954391E9C65960A174`。
二进制包含 `renderer-mesh-identity-v30-ui-callback-fix`，不再包含旧包装日志。
旧 DLL/日志保存在忽略跟踪的 `bin/diagnostics/v29-ui-crash/`，可恢复。
本轮未更改 mod.ini 或资源文件；本地 PDB 留在 `bin/eiem.pdb`。
最终复跑：29 项测试通过（28 项静态合同 + 1 项含六场景的原生边界测试）。
