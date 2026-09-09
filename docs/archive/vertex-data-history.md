# 顶点数据历史实验记录

归档：2026-09-07。以下内容记录对应日期的调查和验证，不是当前故障报告。
当前导出行为与新回归入口见[顶点数据契约](../vertex-data-contract.md)。
原生解码已修复；Blender 0.11.1 已补齐缺失切线生成。历史证据保留供追溯，不用于覆盖现行规则。

## 历史调查证据及边界（解码修复前）

研究目录：`E:\EIEM_Workspace\shader-investigation`。

- `survey/README.md`、`raw-resources.json`、`summary.json`：120 个去重 Mesh、55 材质、7 Shader 的调查。
  115 个 Mesh 的标量 Float32 NORMAL 实际封装 N/T；旧解析器只返回法线，切线为空。
  不能将此缺口解释成原模型无切线。其余 5 个显式 N/T 网格不走压缩帧分支。
- 法线/切线压缩帧：bits 0–9/10–19 是有符号 10 位八面体法线，bits 20–29 是菱形映射切线方向，
  bit30 是格式标志，bit31 是手性符号。读取原始位模式，不把浮点数值转换成整数。
- `typhoea-cloth01/uv2-decoding.md`：已核对的 NPR/Hair/Skin 描边程序使用 UV2.xy 表示
  切线空间方向，`z = sqrt(max(0, 1-x*x-y*y))`，与压缩法线的八面体编码不是同一算法。
  原始 N/T 并非总是严格正交；简单解码后点乘重编码不保证无损。
- 反例 `M_fx_wulfa_toppotential_02` 保存 `_OutlineAverageNormal=1`，但其实际 VFX Shader
  不使用该字段。仅凭字段存在选描边算法的旧建议撤回。
- 当时新几何切线及材质驱动描边数据均待实现；切线生成现由[现行契约](../vertex-data-contract.md)中的 0.11.1 实现，描边数据生成仍独立处理。

## 其他已撤回的说法

- F10 是磁盘 Reload，重新读取全部 mod 并以 Constants 默认值初始化变量；不是保留其他变量的局部更新。
  将来 UI 控制应提交状态更新/Reapply，不靠改 INI 后模拟 F10。
- ImGui `PushID` 只隔离控件 ID，不隔离 Lua 全局变量或 INI 变量。现有 mod 状态本来就独立存储。
- `SetNextWindowBgAlpha(0)` 只影响 ImGui 背景绘制，不自动提供原生窗口透明或鼠标穿透。
- 找到 `Renderer.set_enableCameraDither` / `set_cameraDitherAlphaValue` 只证明候选接口存在，
  不证明所有近距离虚化都由它控制。该接口调查发生在反虚化实现之前；后续实现状态见
  [相机反虚化记录](../camera-fade.md)，不能把这条历史边界误读为当前仍未实现。
  不能连手动渐隐、技能效果或景深一起关闭。

## 原生切线解码修复及回归记录（2026-09-05）

- 修复前：`EndfieldTangentProbe` 直接读取原始 AB，在 Wulfa body LOD3 复现
  `decoded tangent components = 0, expected 2768`。不是 Blender 导入后的才丢失。
- 当次修复：压缩 NORMAL 分支一次返回 N/T；保留显式 Tangent 通道和既有 EIEMESH v3 布局，
  不增加统一重算或猜测性回退。测试输出仅放新目录，不覆盖游戏 mod、源包或用户场景。
- 修复后：相同三个原始 AB 共解析 60 个 Mesh；57 个压缩网格、154,479 个压缩顶点全部有完整 N/T，
  帧单位长度、N/T 正交性和源手性符号检查通过；3 个显式 N/T 网格继续走原分支。
- 解码固定样本：法线相同但切线方向不同的 10 个角度/手性组合通过；截断字、缺少标志和错误格式
  3 个非法输入明确报错。仅检查单位长度不能证明方向正确，因此不以它替代方向样本。
- 调用真正的 `EiemPackageWriter.WriteMesh`，再用独立后台 Blender 5.0.1 导入/导出：
  60 Mesh、179,053 顶点的法线、切线、颜色及全部 UV 与解析器数据按 float32 位一致。
  其中本批源颜色为空；这里不声称验证了非空顶点色。此测试没有验证骨架/动画往返或游戏 GPU 输出。
- 验证工具发现 JSON 的 `-0` 被 Python 当整数 0，曾产生伪位差；修正测试的 JSON 读取，保留负零，
  未通过放宽容差、重算法线或改生产写包器掩盖差异。
- 额外回归：真实后台 Blender 材质槽空隙测试通过；资源管线 31 项静态契约检查通过。
- AnimeStudio 在本机 .NET 9 SDK 编译通过，输出 `bin/tangent-repair`，未覆盖既有运行版本。
  构建仍报告已有 MessagePack 依赖安全警告及既有编译警告；本轮未擅自升级依赖。

复现入口（路径从参数传入，不将角色名或本机 AB 路径写进生产规则）：

```text
dotnet run --project tests/EndfieldTangentProbe -- <新构建目录> <全新输出目录> <原始AB> ...
blender --background --factory-startup --python-exit-code 1 --python tools/Blender/test_eiem_source_tangents.py -- tools/Blender/eiem_blender_addon.py <上述输出目录>
```

旧包/旧 `.blend` 中缺失的原生切线不会因升级解析器自动恢复。要找回原生数据，需从原始 AB 重新导出完整输入；
0.11.1 也可以为缺失数据生成切线，但不宣称与遗失的原生切线相同。
不能覆盖用户已编辑工程或对无顶点对应关系的网格盲目移植数据。当次没有重新部署 Mod 或修改用户场景。

## Blender 面角接缝导出修复（2026-09-06，0.8.2）

- 复现：0.8.1 对用户当前 body 网格连续两次在顶点 13088 报 `UV0 has a per-corner seam`。
  该顶点不同面角的 UV 约为 `(0.8694, 0.4835)` 与 `(0.3549, 0.5554)`，属于合法接缝。
  已核对 loop 索引未错位、已部署代码与源码一致。此前“缺 UV1 导致错位”的解释错误。
- 原因：旧 `corner_values_to_points` 把面角流压成每顶点一组数据，发现多值就拒绝导出；
  旧法线分支也有同类拒绝检查。不是 INI 选择问题，也不是源资产必须补 UV1。
- 修复：原顶点编号保留，对同一原顶点的不同 float32 面角属性组合追加输出顶点。
  所有非空 UV 的 XY、面角颜色和当前法线共同决定拆分；位置、切线/手性、UV Z/W、
  POINT 颜色、蒙皮和标准形态键位置/法线/切线增量都使用同一源顶点映射。
  三角形经 loop 映射更新索引，原材质槽空隙、骨骼调色板、bind pose 和零权重骨骼槽保持不变。
- 法线未编辑时仍保留源浮点备份，编辑后使用真实面角法线；不平均接缝、不重算源切线，
  不用临时操作符改 Blender 拓扑。新增顶点只存在于序列化结果。
- 游戏专用 `additionalNormals` 流仍没有已验证的索引语义；需要拓扑重映射时明确报错。
  本修复覆盖标准形态键增量，不宣称解决所有游戏的扩展流。
- 新测试先在旧实现复现 UV0 报错，再验证修复：稀疏 UV0/UV2、4D UV Z/W、CORNER 颜色、
  自定义法线接缝、POINT 颜色、原切线符号、权重与 3 槽骨骼调色板（含 Unused）、
  形态键增量复制、空材质槽、孤立顶点、确定性重复导出及未改资源 float32 往返。
  测试位于 `tools/Blender/test_eiem_corner_export.py`，由 `tests/test_blender_corner_export.py` 调用。
- 用户当前 body 网格临时导出：13,140 → 14,665 顶点，26,160 三角形，35 个骨骼槽。
  核对 78,480 个三角形面角的位置/UV 和 1,525 个副本的权重/切线；仍只有 UV0、UV2。
  这些是导出和数据一致性验证，不等同于新的游戏画面验收。
- 全量测试 84 项通过；完整 Typhoea 离线包额外完成 62 Mesh、1 骨架、31 材质、65 贴图的
  导入/导出回归，未编辑的数据及材质依赖行为继续通过原有断言。
- 已同步开发目录并在用户当前 Blender 5.0.1 中热重载 0.8.2；实际导出操作符连续两次返回
  `FINISHED`，每次 1 Mesh、0 Material、0 Texture、0 Skeleton。输出仅在临时目录验证，
  未覆盖游戏 Mod、INI、DLL 或用户 `.blend`，当前选择及模型顶点/面保持不变。
- 开发插件旧三文件备份：`E:\EIEM_Workspace\blender-addon-backups\before-corner-export-20260906-153426`。
- 独立后台往返测试输出保留在系统临时目录 `eiem-corner-roundtrip-5ba2a5b266754f5aa8ca3707ccc35433`；
  清理请求被环境策略拦截，未尝试绕过。它不是用户 Mod 的一部分。

## 共享骨架扩展（2026-09-06，0.8.3 / DLL v43）

- 修复了原 35 槽调色板之外的正权重被丢弃的问题，详见 [绑定契约与实证](../shared-skeleton-binding.md)。
  0.8.2 的接缝测试只证明副本与旧导出结果一致，不证明 Blender 中所有权重组均被导出；不能混淆这两项。
- 当次不更改源切线处理。当时用户编辑后的 body 临时导出没有切线数组，其 Mesh 也没有
  `EIEM_Tangent` / `EIEM_TangentSign`；这不能反推原始 AB 没有切线，也不是对历史解码故障的复现。
  当时尚未实现外部新增几何的切线生成；此待办现由[现行契约](../vertex-data-contract.md)中的 0.11.1 导出实现完成。
- 本轮确认的是骨骼绑定与权重映射的数据修复；不由此宣称切线着色或游戏 GPU 蒙皮已验收。
