# 条件与按键配置（v32）

状态：已实现 DLL 解析、求值、按键调度与局部重应用。自动测试/构建通过不代表游戏画面已验收。
v32 实现时未部署；后续 v33 已部署 DLL 和 Blender 0.5，见 [Blender 切换流程与验证记录](blender-switches.md)。

## 固定的全局配置

游戏根目录下 **`plugin/eiem.ini`**，唯一入口，不再读取 `eiem_config.txt` 或搜索备用路径。
新 DLL 启动时若文件不存在，会创建默认配置。发布模板位于 `config/eiem.ini` 和构建后的 `bin/eiem.ini`。

```ini
[Hotkeys]
reload=F10
gui=INSERT

[Graphics]
disable_camera_fade=false
```

`reload` 重读全局设置及所有 mod，并将 mod 变量重置到 `[Constants]` 的初始值。
将其改成 `reload=Ctrl+F8` 后，按一次**旧快捷键**加载配置，随后使用新快捷键。
`gui` 控制原有面板。两者不得相同；mod 按键不能占用这两个全局组合。
ApplePie 的配置入口指向 `eiem.ini`，刷新接口同样只向 Unity 线程提交 Reload。

`disable_camera_fade` 控制相机渐隐，详见 [相机反虚化](camera-fade.md)，不改变 Mod 显隐规则。

全局配置写错时明确记录错误，保持最后有效的设置（含快捷键与相机反虚化），防止失去修正配置的入口；
不尝试别的文件，也不使用另一套解析路径。普通 mod 文件非法则整文件不发布，不保留其旧动作。

## 一个按键切换原版、修改版、隐藏

以下内容写入该 mod 的 `mod.ini`，资源路径需指向实际已导出的文件：

```ini
[Constants]
$outfit=1

[KeyOutfit]
key=F6
type=cycle
$outfit=0,1,2

[MeshCloth]
path=meshes/cloth.mesh

[MaterialCloth]
path=materials/cloth.mat

[RenderCloth]
asset=S_actor_typhoea_cloth_01_lod0
if $outfit == 1
    mesh=MeshCloth
    material.0=MaterialCloth
else if $outfit == 2
    handling=skip
endif
```

- 0：本规则不修改，撤销此前由该规则施加的修改，回到游戏原状态。
- 1：给原 Renderer 更换 Mesh 与材质槽 0；不新增 Renderer。
- 2：只隐藏原 Renderer。`skip` 不自动创建或删除 Mesh。
- 新增 Renderer 仍为独立 `[Render...]` 模板，通过条件内的 `partner.N=Render...` 引用。
  模板中的 `if` 同样可以使用当前 mod 的变量。
- `0,1` 就是普通开关；`0,1,2,...` 为多状态循环，不另加 toggle 别名。

## 语法约定

- `[Constants]` 的变量名为 `$` 加英文字母/下划线开头的名称，**区分大小写**，当前 mod 内共享。
  只初始化一次；进入 UI、换图、新实例出现不会重置。全局刷新会重置。
- 数值为有限数值，支持负数、小数及 `true/false`（对应 1/0）。
- `[Key...]` 必须有 `key`、`type=cycle`、至少一项已声明变量的值列表。
  多个变量可同一按键同步切换，列表长度必须相同且至少有两项。
  根据当前变量组合选择列表中的下一组；不在列表中则选择第一组。
- 目前按键支持 F1–F24、A–Z、0–9、INSERT/DELETE/HOME/END/PAGEUP/PAGEDOWN、方向键等，
  以及 `Ctrl+Shift+F6` 形式的组合。没有 hold、延时、手柄、持久化状态功能。
- `if / else if / else / endif` 允许嵌套，不能跨 section；不同时提供 elif 等别名。
- 表达式支持 `== != < <= > >= && || ! ()` 和一元正负号。
  比较优先于 `&&`，`&&` 优先于 `||`；逻辑操作短路求值。不支持四则运算、函数调用或赋值表达式。
- 条件只出现在 Render 动作里。资源声明和 `asset/path/match.*` 选择器必须保持静态。
- 同一 Render 内按顺序应用成立的赋值，同一字段最后一次有效赋值生效。
  `mesh=`、`handling=`、`material.N=`、`partner.N=`、`shape.名称=` 可显式清除此轮此前对该字段的设置。
- 每次求值从该 Render 的静态选择器和空动作开始，不继承上一状态的动作。
- 当前规则集仍为**选择器第一条命中**，条件不成立不意味着自动尝试第二条规则。
- 整行 `;` / `#` 注释有效；不支持行尾注释。

未知变量、错误表达式、缺失 endif、无效资源引用在加载时统一报错，包括当前不会执行的分支。
资源引用只能指向本 mod；partner 的静态引用关系不会因为分支关闭而改变。
当前运行时 partner 仅支持一层，嵌套或循环 partner 引用明确拒绝；条件嵌套与 partner 嵌套是两回事。
单 Render 最多 128 个条件节点，单表达式最多 256 个节点，超限报错而非截断执行。

## 执行与生命周期

1. 加载：解析有序条件树、校验全部分支、建立静态作用域索引，以初始变量求值得到 Render 动作。
2. 模型完成：只要选择器命中，即使条件关闭，也登记模型实例；不因没有执行修改而丢失目标。
3. 输入：前台窗口的真实按下事件与 ImGui 滑条共用有序队列，送到 Unity 窗口线程；
   只合并相邻的同一滑条采样，不跨越按键事件。
4. 重应用：先在独立数据中求得新状态；撤销受影响 mod 的旧效果，再发布新动作、应用已登记实例。
   不重新解析磁盘 INI；资源仍由同一后端按需构造并复用缓存。
   v36 增加纯形态键权重更新路径：通道集合和装配不变时只更新权重，不撤销并重建 Renderer。
5. 退出/换实例：沿用实例所有权清理；新实例使用当前变量值。
6. 全局刷新：撤销旧效果 → 重读 eiem.ini/mod.ini → 重置变量 → 应用已登记实例。
   同批次若有刷新，旧代按键及滑条事件丢弃，避免旧操作落到新配置。

键位随配置版本和前台窗口变化重新注册；离开游戏注销占用，不在桌面切换 mod。
注册失败明确记录键码和错误，不回退到另一套轮询输入逻辑。
只用循环按键，因此没有伪造松开事件或依赖连续按住重复触发。

恢复缩小为本次真正修改的字段：

- 没有成功写 Mesh，就不重设 Mesh；纯材质/skip 不捕获或恢复骨架。
- 材质只恢复被写入的槽位，保留其他槽的当前游戏值；插件扩展的尾部槽会撤销。
- Mesh 恢复时，骨骼数组/根骨骼与保存值相同则不重复调用 setter。
- partner 在销毁前先禁用绘制，避免 Unity 延后销毁时与新对象短暂叠加。
- 对不同 mod 的按键更新保持原有第一条命中次序，不先过滤规则再把次优规则提升成命中者。

## 验证与未完成项

`tests/test_mod_program.py` 执行真实解析/表达式/循环状态代码；`test_mod_controls.py` 执行全局配置、
按键队列发布、材质槽恢复以及真实模型登记函数；`test_hotkey_worker.py` 将实际快捷键线程放在模拟 OS
消息队列中，验证改键、连续按键、离开/返回前台和注销。未向真实桌面注入按键。

2026-09-05 验证结果：`python -m unittest discover -s tests -q` 全部 48 项通过；
`build.bat` 构建成功，`git diff --check` 无空白错误。
本次 `bin/eiem.dll`（`resource-runtime-v32-conditional-keys`）SHA256：
`29F777CF6BD7A4052CA9234F4542072DBC628CAC201B61B17BBD2DF1B3790A3E`。

已知边界不能隐去：

- 新语法需要 v32 或更新 DLL；此前的旧 DLL 不会识别这些条件。
- 大世界/UI/换图的真实渲染结果和内存曲线仍需游戏验收。
- 尚未识别游戏内聊天框等文本输入状态；游戏窗口在前台时 mod 快捷键仍会触发，建议使用功能键/组合键。
- 多 PFB/顶层作用域对同一 Renderer 的冲突合成，原生资源最终 Destroy 的完整所有权，以及新增陌生目标
  后仅靠刷新发现实例，仍是架构审查记录中的未完成项。不要配置相互覆盖的多个作用域。
- 同一受控材质槽后来被游戏改写时，恢复基线的持续同步尚未完整解决；本轮保证未受控槽不被旧快照覆盖。
- Blender 0.5 已提供切换组/状态 UI 与 INI 自动生成；不是任意表达式的可视化编辑器。
  Mesh/材质/贴图格式没有更改。

v36 / Blender 0.6 曾增加固定 Slider UI；v37 / Blender 0.7 改为 `[UI...]` + Lua，移除旧 Slider 解析。
`shape.名称=表达式` 不变，Lua、快捷键继续共用上述变量与输入顺序，见 [Lua UI](lua-ui.md)。
详见 [形态键与 ImGui 控制](shape-controls.md)，该文单独记录 API 验证与游戏 GPU 验收边界。
