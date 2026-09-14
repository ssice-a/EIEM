# 合并兄弟 mesh 到单个 Renderer 的边界

记录 2026-09-14 合并方案的实测结论，避免后续重复推导已经被推翻的做法。

## 架构硬约束：源 asset 是分组单位

mod.ini 的挂载由 Render 段的 `asset=` 决定，不是由资源段的字段决定：

```ini
[RenderS_actor_typhoea_cloth_01_lod0_2]
asset=S_actor_typhoea_cloth_01_lod0     ; ← 这个 Render 只能挂这一个源 asset
```

`target.path` / `target.asset` **不参与挂载**。实测：`targetPath` 在 `src/` 内除了
解析处没有任何读取点；`targetAsset` 只在 `eiem_resource_backend.h` 里被当作
Unity 对象的显示名使用。

**结论**：源 asset 是分组单位，**不是骨架，也不是物理**。
Typhoea 的 body_01 / cloth_01 / cloth_02 虽然共用同一套
`Skeletonchr_0034_typhoea_postmodel_0` 和
`PhysicsSkeletonchr_0034_typhoea_postmodel_0`，但它们是三个源 asset，
所以**至少需要三个 Render 段，不能合并成一个**。

可合并的范围 = **同一个源 asset 下的全部兄弟部件**。

| 分组 | 部件数 | 合并结果 | 状态 |
|---|---|---|---|
| cloth_01 | 9 | `MeshS_actor_typhoea_cloth_01_lod0_2_MERGED.mesh` | 已部署，已确认不再躺地 |
| cloth_02 | 3 | 待合并 | 仍走 Partner 路径 |
| body_01 | 1 | 无需合并（本来就是就地替换） | 已部署 |

## 跨源 asset 合并会撞上的两个问题

把 12 个部件（cloth_01 + cloth_02）合成一个 mesh 时实测到：

1. **UV 通道宽度不一致**。cloth_01 全部是 `uv0=2, uv2=2`；cloth_02 全部是
   `uv0=2, uv2=4`。同一个合并通道只能有一个宽度，向宽的一侧补齐需要显式
   `--pad-uv`，且被补的顶点后半段必须是零。
2. **即使 UV 兼容也没有意义**，因为源 asset 边界决定了它无法挂到一个 Render 下。

## 材质槽填充

合并后的 mesh 子网格数（9）大于源 Renderer 的材质数（1）。插件的材质数组
原本只填 `sourceCount` 个槽，其余为 `null`，Unity 用 null 材质绘制子网格时
落到 magenta error shader —— 这就是合并后出现紫色的原因。

修法：未声明的槽继承源槽 0 的材质，等价于合并前"每个部件各自一个 Renderer、
各自槽 0 拿到源材质"的表现。配置显式声明的槽仍然覆盖在其上。

`submesh.N=<材质槽>` 只在显式声明时才重排；`submeshSlots` 默认全 `-1`，
此时子网格 i 直接用材质槽 i，所以 `submesh.0=0 … submesh.8=8` 是恒等映射。

## LOD

同一源 asset 的不同 LOD 级别**不能合进同一个子网格集合**：游戏切换 LOD 时
会整组换 renderer 或换 mesh，混在一起的几何会跟着一起消失或一起露出。

当前 mod 只供应 LOD0，所以尚未触发。加入 LOD1/2/3 之后，合并必须按
"每个 LOD 级别内部合并"来做。
