# 本地项目管理状态（2026-09-26）

以下先记录仓库整理时的本地状态，后文列出发布候选提交。候选尚未推送或发布。

## 仓库与工作目录

| 产品 | 唯一日常工作树 | 分支 | 远端 |
|---|---|---|---|
| EIEM DLL、协议和集成测试 | 仓库根目录 | 见本地 Git 状态 | `ssice-a/EIEM` |
| Blender 作者插件 | `tools/Blender` | `main` | `ssice-a/EIEM-blender` |
| AnimeStudio 解包工具 | `tools/AnimeStudio` | `master` | `ssice-a/AnimeStudio` |

后两者是 EIEM 的 Git submodule，各自维护源码和提交历史。`E:\vscode\blender`
是另一套 Blender 插件，不属于 EIEM，不能当作重复目录清理。三个仓库都有未提交改动；
父仓库的 gitlink 目前不能代表全部本地源码。

## 已完成的核对

- 将原 `E:\vscode\AnimeStudio` 中较新的 7 个源码改动同步到 `tools/AnimeStudio`。
  对两个工作树逐文件比较：排除 Git、IDE 设置和生成目录后，1543 个文件只有
  `.gitignore` 不同；目标仓库额外忽略了 `.vscode/`。IDE 设置另行核对一致。
- `tools/AnimeStudio` 从 detached HEAD 切回跟踪 `origin/master` 的本地 `master`；
  未提交源码改动保留。
- 在 EIEM 的本地 Git 配置中初始化两个 submodule；`git submodule status` 现在正常
  显示 AnimeStudio `0aa4346` 和 Blender `97cecfb`，没有更新工作树或源码。
- 从原工作树创建并验证 Git 引用备份：
  `backups/AnimeStudio-old-checkout-20260926.bundle`。该目录被 EIEM 忽略，不参与提交。
- AnimeStudio GUI 用 .NET 9 `Rebuild` 通过：0 错误、20 警告。普通增量构建曾因
  旧产物报两处找不到方法；强制重建后均消失。机器当前没有 .NET 10 SDK。
- Blender 的纯 Python 格式与 LOD 单元测试通过，共 5 项。依赖 `bpy` 的测试需要
  Blender Python 环境，当前系统 Python 无法运行。
- `tools/README.md` 的开发、运行和构建路径已改为 `tools/AnimeStudio`。
- 原 `E:\vscode\AnimeStudio` 重复工作树已由用户手动删除。EIEM 工作区的
  `.vscode/settings.json` 通过 `git.scanRepositories` 显式列出 EIEM、Blender 和
  AnimeStudio 三个 Git 仓库，关闭自动仓库发现；AnimeStudio 的本地 CMake 路径也
  改到 `tools/AnimeStudio`。仓库内的两个临时调查 Git 目录仍在磁盘上，已加入
  `git.ignoredRepositories`，不显示在此工作区的源代码管理中。

## 待处理

1. 先前同步时的 `/PURGE` 删除了目标仓库原有、源目录没有的未跟踪文件
   `AnimeStudio.GUI/EndfieldMeshObjectDump.cs`（约 8 KB），并覆盖了目标的
   `Program.cs`、`Mesh.cs` 本地诊断改动。两个工作树及现有 Git 对象中尚未找到该
   未跟踪文件的副本；原诊断改动可从本地 Git 历史继续核查。不能称这次合并无损。
2. 分别审查三个仓库的未提交改动，做各自的测试和提交；然后在 EIEM 中更新两个
   gitlink，并验证跨端格式。当前不创建 tag、不推送、不发布 Release。

后续同步不得再用 `/PURGE` 覆盖有本地改动的目标目录。先比较内容与 Git 状态，
再逐项合并。

## 发布候选补记

- Blender 的源码、更新检查与发布工作流已作为本地提交 `8aba195` 保存，版本为 `v0.33.0`。
- AnimeStudio 的导出、更新检查与发布工作流已作为本地提交 `821167c` 保存，版本为 `v1.1.0`；MessagePack 已升级到 `3.1.10`。本机 .NET 9 GUI 构建通过，.NET 10 包由双 SDK 的 CI 工作流构建。
- EIEM DLL 的发布候选版本为 `v1.1.0`。发布 ZIP 已按游戏目录布局组织，`plugin/eiem.dll` 与 `plugin/eiem.ini` 位于正确位置；本地 DLL 和两个代理构建通过，仓库测试 140 项通过、50 项因可选环境缺失而跳过。
- 三仓库的候选提交目前仅在本地。推送分支和 tag 才会触发远端构建与 Release；这一步仍待执行。
