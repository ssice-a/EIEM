# CPU 蒙皮调查资料清理记录

更新时间：2026-09-25

## 保留

- [CPU 蒙皮随机躺地：当前唯一复盘](cpu-skin-random-pose-current-20260925.md)：唯一事实入口和后续计划。
- `analysis/native-mesh-ctor-trace-20260925/`：当前待验证的构造追踪 DLL、PDB 和基线备份。
- `analysis/unityplayer-20260923/`：UnityPlayer 原始二进制、IDA 数据库和静态反汇编文本；删除了其中旧的运行时探针、PDB、日志和抓帧产物。
- `src/` 与 `tools/`：源码和可复用诊断脚本暂不删除，直到写入/注册边界被证实。

## 已删除

- 已被当前复盘吸收的 C7、EF230、1C8 setter/clone/in-place/PFB/validation 等重复实验文档。
- `analysis/` 下除当前构造追踪和 UnityPlayer 静态工程外的旧实验目录。
- `backups/`、`bin/diagnostics/` 中与本问题无关的历史运行包和大日志。
- UnityPlayer 静态工程中的旧 Vulkan/flag 探针 DLL、PDB、日志、压缩日志和备份 DLL。
- 顶层临时文件 `$log`、`Hook.log`、`IDAHook.log`、`.tmp_dis.py`、`.tmp_parse.py`。

## 清理原则

删除的是可由当前复盘重建、且不会改变生产代码的实验产物。源代码、当前构造追踪版、原始 UnityPlayer 和静态分析数据库仍保留。以后每轮诊断只保留：一个明确问题、一个有界运行、一个结果摘要；不再把每次启动日志或 DLL 哈希追加到主文档。

## 清理过程中的恢复

旧目录包含指向 Git 子模块元数据的 Windows junction。第一次递归清理误触了顶层 Git 元数据；已从同仓库临时副本恢复 `HEAD/config/index`，并重建 Blender 子模块索引。`git status`、主仓库 `HEAD` 和子模块状态均已恢复可读；源码与当前 DLL 未被回滚或覆盖。
