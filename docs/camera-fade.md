# 相机反虚化：CameraMono 调用契约

## 当前方案

只改变相机渐隐，不做 UI 绘制、景深处理或模型资源替换。
旧 Renderer setter hook 在真实日志中未命中，已删除；失败证据保存在
[v34 实验记录](archive/camera-fade-v34.md)。

运行时从 IL2CPP 元数据查找 `Beyond.Gameplay.View.CameraMono` 的两个零参数实例方法：
`_ProcessDitherByPitch()` 与 `ForceClearDither()`。不硬编码地址、偏移或角色名字。

每次游戏调用 `_ProcessDitherByPitch`：

1. 先调用原函数，原样传递当前实例与 MethodInfo。
2. 插件启用且 `disable_camera_fade=true` 时，在同一个实例调用 `ForceClearDither`，传入它自己的 MethodInfo。
3. 关闭功能时只执行原函数。原函数失败不吞异常，也不继续清理。

**纠正旧说法：不能把 `ForceClearDither` 理解为一次调用即可永久关闭渐隐。**
下一次相机评估可以重新施加渐隐，因此采用每次原评估之后清理，而非只在初始化或 F10 时调用。
不持有 CameraMono/Renderer 对象，不建弱引用表，不扫描场景，也不增加相机创建/销毁 hook。

## 配置与刷新

唯一全局文件仍为游戏目录下 `plugin/eiem.ini`：

```ini
[Hotkeys]
reload=F10
gui=INSERT

[Graphics]
disable_camera_fade=true
```

模板默认 false。修改后按配置的 reload 键发布设置，下一次相机评估读取新值。
F10 不重装 hook，也不在按键线程直接调用游戏相机方法；它不具有 DLL 热替换能力。
更换 DLL 首次需重启游戏。关闭功能后由后续游戏评估恢复原行为；如果相机暂停评估，
不能承诺设置发布的一刻就恢复画面。

非法配置不发布，保留上一份有效设置并报错。Mod 原有 F10 重载、mesh/skip 语义不变。
管理器开关与配置共同决定是否清理，不需要逐对象恢复调度。

## 依据与边界

参考项目在固定提交中采用原评估之后清理的调用顺序：
[Endfield-Uncensored CameraMono hook](https://github.com/DynamiByte/Endfield-Uncensored/blob/509d1b8b6ad0cec9787af7b055984d955c1f0461/src/dll.zig#L275-L293)。
仅参考游戏方法与调用契约；不复制其手写跳转补丁、快捷键轮询或 UI，使用本项目 MinHook 与全局配置。

本机 `eiem_il2cpp_classes.txt` 列出 Gameplay.Beyond.dll 下的 CameraMono；
当前 global-metadata.dat 含 `_ProcessDitherByPitch`、`ForceClearDither` 名称。
资源详情 dump 因资源类过滤不列出 CameraMono，不能据此推断相机类不存在。
元数据名称存在只证明可继续解析，不证明运行时一定经过该入口。

本方案不直接写 Renderer.enabled、Mod handling 或材质槽。
但游戏原生 ForceClearDither 对遮挡、技能或不同 UI 相机的作用范围仍需画面核验，
不能从方法名或模拟测试推断所有场景都覆盖且完全无副作用。景深、运动模糊、TAA 不属于此方案。

## 清理与测试

删除 v34 的 Renderer enableCameraDither setter hook、逐 Renderer 弱引用恢复表及专用 weakref API 绑定。
移除 F10/管理器对旧恢复表的调度，仅保留配置发布与状态日志。

`tests/test_camera_fade_hook.py` 使用真实 MinHook、生产 adapter 与模拟 CameraMono 方法验证：

- 一次清理后再评估会重新渐隐的反例。
- 两个相机连续评估，同实例 original → clear 顺序及各自 MethodInfo；地址复用无旧对象记录。
- 配置反复开关、非法配置保留、管理器开关。
- 原评估失败不会吞异常或继续清理。
- 类、方法、参数个数或地址缺失时拒绝安装，不猜地址、不转旧实现。

新契约测试首先在 v34 adapter 上失败于安装 CameraMono 入口；修复后应全部通过。
模拟对象里的 skip/manual 字段不变，只验证 adapter 未直接修改它们，不证明真实游戏副作用。
`tests/test_camera_fade.py` 保留真实全局配置解析与发布测试。

### 日志验收

- `[BUILD] resource-runtime-v35-camera-mono-fade`：实际加载版本。
- `Installed`：MinHook 安装完成，不等于画面成功。
- `observed`：真实原评估返回，首次记录。
- `cleared`：真实 ForceClearDither 返回，首次记录。
- `status ... evaluations=... clears=...`：初始化、F10、管理器切换时报告累计次数。
- `Unavailable`：元数据或 hook 安装失败。

游戏画面需要核验拉近镜头、开关往返、换地图、角色 UI，并与日志一起判断。
编译及模拟 MinHook 测试不能替代该验收。本轮自动验证与部署结果追加于下方。

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
