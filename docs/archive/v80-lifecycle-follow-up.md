# v80 lifecycle follow-up

This note records the first follow-up changes made on top of the recovered v80
baseline (`762f220`, `feat: 完成物理碰撞解析`). It is a source and offline-test
record; it is not gameplay acceptance.

## Update requests

The mod update bitmask is retained when the game window is temporarily
unavailable, when `PostMessage` fails, or when Unity renderer APIs have not
finished resolving. A short window timer retries the same request. Shutdown
clears the retry timer. This keeps F10, Reapply, and lifecycle reconciliation
from being silently dropped during a scene transition.

## LOD membership

Partner membership is reconciled in both directions. When a source Renderer is
present in a LOD level, its partner is added once. When the source is absent,
any stale partner is removed from that level. The change does not infer a
character, Mesh, LOD number, or switch name. The existing skin assembly
boundaries re-run this reconciliation after the game rebuilds its LOD arrays.

## Validation boundary

The offline suite passed after the change and `build.bat` completed. No claim is
made here about in-game teleport, hot reload, LOD switching, native Physics
completion, Animator writeback, or teardown safety. Those require a separate
runtime validation pass with the rebuilt DLL.
