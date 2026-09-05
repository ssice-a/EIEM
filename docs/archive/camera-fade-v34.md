# 相机反虚化 v34：历史失败实现

此文档仅保留实验历史，现行方案见 [CameraMono 调用契约](../camera-fade.md)。
2026-09-05 用户反馈仍有虚化；实际日志共 5119 行，v34 构建标识、开关开启、hook 安装各出现一次，
但 `[CAMERA-FADE] observed` 为零。安装成功不等于游戏经过此 setter。
v35 删除旧 Renderer setter hook、弱引用恢复表，不把它们保留为运行时回退。

## 本轮范围

去掉走 `UnityEngine.Renderer.enableCameraDither` 的相机渐隐。
不改变 `Renderer.enabled`、`handling=skip`、手动渐隐、技能溶解、材质参数、景深或运动模糊。
“相机拉近人物虚化”的实际画面尚未在本轮验收，不能声称此入口覆盖所有近距离效果。

## 配置及实现约定

唯一全局文件为游戏目录下 `plugin/eiem.ini`：

```ini
[Hotkeys]
reload=F10
gui=INSERT

[Graphics]
disable_camera_fade=true
```

缺省为 false；旧配置不自动开启新功能。修改后按 reload 对应的键（默认 F10）。
值为 `true` / `false`（不区分大小写）；`yes`、空值和其他拼写均拒绝。
非法值整份全局配置不发布，保留最后有效设置并记录错误；不静默把拼错的值当 false。
首次更新 DLL 需要重新启动游戏；配置刷新并不会替换正在运行的 DLL。

实现只使用一个入口：IL2CPP 动态解析的
`UnityEngine.Renderer::set_enableCameraDither(System.Boolean)` 原生调用。
不按角色名或模型类型枚举 hook，不使用静态分析中的地址作为运行时常量。

- 游戏请求值与实际送入值分开：开启反虚化时，实际送入 false，但保留游戏最后请求的 true/false。
- 配置关闭或插件关闭后，在 Unity 线程重放仍存活对象的最后请求；不能一律写 true。
- 每个已观察 Renderer 使用弱 GC handle。销毁对象不重放；遇到新对象时清理死亡记录。
  不以原生地址相同认定旧对象仍存活，不强引用整个角色，也不扫描全场景或轮询文件。
- 未观察到的既存 Renderer 没有可靠原始值，不能猜测并强行恢复；后续游戏调用入口时登记。
  新 DLL 在启动时安装 hook；开启前未观察到的状态属于明确的覆盖边界。
- 即使配置尚未开启，也记录请求，才能在本次会话里按键开启并正确恢复。弱句柄创建失败时明确记错，
  原请求不改写，不施加无法恢复的修改；这是拒绝未拥有的状态，不是另一条反虚化实现。
- 功能是全局 camera-dither 开关，不按角色名筛选。任何使用该接口的 Renderer 都会受到影响。
- 配置、恢复与 mod 的 mesh/material/skip 操作互不混用。F10 仍会按原有语义重载 mod 并重置变量。

## 证据

2026-09-05 静态读取当前磁盘 GameAssembly.dll（未执行）：

- 确认含完整原生调用名称 `UnityEngine.Renderer::set_enableCameraDither(System.Boolean)`。
- 包装函数保存 bool 参数，解析原生调用后以 Renderer 对象和 bool 转发。
- 已定位到 Lua `UnityEngineRendererWrap._s_set_enableCameraDither` 对包装函数的直接调用。
- 旧 IL2CPP 元数据同时列出 camera 和 manual 两组独立 setter。不能因此推论所有人物渐隐都走 camera。
- 本轮未运行游戏；画面验证及运行时命中信息待新 DLL 加载后采集。

Unity 的托管对象可在原生对象销毁后仍存活；参考 [Unity 官方 Object 源码](https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Scripting/UnityEngineObject.bindings.cs)。
本实现同时验证弱句柄与 `m_CachedPtr`，字段偏移从当前游戏元数据读取，不使用官方源码版本或测试中的固定偏移。
找不到字段或弱句柄 API 时拒绝安装并记录 `Unavailable`，不猜地址。

## 验收

先用真实 C++ 状态模块测试：开/关重复切换、游戏中途改变请求、原请求 false、对象销毁、地址复用、
弱句柄分配失败、重复登记、关闭后恢复。另测全局配置解析与发布，不只查源码字符串。
运行时日志必须区分 hook 已安装、实际收到请求、实际压制和重放恢复，不能把安装成功写成效果成功。

2026-09-05 自动验证记录：

- 新状态测试先失败于功能头文件不存在，实现后通过。
- `tests/test_camera_fade.py`：4 项真实 C++ 测试，涵盖上述状态、弱引用与配置发布。
- `tests/test_camera_fade_hook.py`：2 项真实 MinHook 测试。独立进程中拦截模拟原生 setter，
  运行生产 adapter，校验 bool ABI、转发、重复初始化、开关恢复、销毁与缺失接口拒绝安装。
  模拟对象故意使用不同的字段偏移，验证代码确实使用元数据。
- 补测发现：插件快速关闭/开启、调度事件合并时，中途游戏请求会让“最终开关相同”误判为不用刷新。
  修复为：观察到中间模式的请求会标记待刷新；该场景在修复前真实失败，修复后通过。
- 全套 `py -X utf8 -m unittest discover -s tests -v`：57 项中 55 项通过，2 项 Blender 联测因未设置
  `EIEM_BLENDER` 跳过。本轮未改 Blender 或连接用户场景，不把跳过记作通过。
- 旧静态回归曾固定截取函数前 2200 字符，新增调用使后部被截掉；改为读取完整函数，保留原断言。

### 运行日志与画面验收

- `[BUILD] resource-runtime-v34-camera-fade`：确认实际加载的是本版 DLL。
- `[CAMERA-FADE] Installed ...`：仅表示入口安装成功。
- `observed ... requests=...`：确实经过入口；`suppressed` 是被改写为 false 的写入次数，含刷新。
- `refresh ... replayed=... restored=...`：F10 / 管理器切换时重放的写入数及恢复数。
  `tracked` 为仍持有弱引用的观察对象；`expired` 为已清理对象，`trackingFailures` 为拒绝修改次数。
- 安装失败为 `Unavailable`。若已安装但没有 `observed`，不能称为近距离效果已生效。

首次换 DLL 后进入游戏，拉近镜头观察；同一会话把开关改为 false 并按 F10，再改回 true 并按 F10。
记录近距离渐隐是否往返变化，同时确认 Mod 的 `skip`、服饰切换和技能渐隐不受影响。
大世界、角色 UI 与换图属于待测覆盖，不用“入口通用”代替实际画面结论。

### 本机部署（2026-09-05）

- `build.bat` 构建成功；确认游戏未运行后，已部署 `plugin/eiem.dll`。
- 本版 SHA256：`E9EBD359C693BCF4F64279985455F59B29EB7FFF09CB2743F431AF449F004B04`，
  构建产物与游戏目录内 DLL 逐项哈希一致。
- 本机 `plugin/eiem.ini` 已加入 `[Graphics] disable_camera_fade=true`，原 F10 / INSERT 不变。
  分发模板仍为 false，不替其他用户自动开启。
- 原 DLL 和原配置备份在 `E:\EIEM_Workspace\plugin-releases\before-v34-camera-fade-20260905`。
  原 DLL SHA256：`B6A23D3358E361FD6A237FD1930F84295EDB4610C7AC09477688CCB986654FD4`。
- 未修改任何 `mod.ini`、用户 Mesh/Material/Texture、Blender 场景或开发插件。
- 未启动游戏，尚无本版游戏内请求日志或画面验收结果。
