# EIEM Importing Endfield MMD

Mod authoring tools, current contracts and implementation status: [documentation index](docs/README.md).

English | [中文](README.md)

Brings MMD animation playback to *Arknights: Endfield*. Supports muscle-driven body motion, facial expressions, finger animation, camera motion, and synced background music, all controlled through an in-game GUI panel.

example: [bilibili](https://www.bilibili.com/video/BV1YdEC6bEfP/)

## v1.0.0 Resource Replacement

v1.0.0 is the first resource-replacement release. Mesh, material, and texture rules enter the game's existing Renderer assembly path, leaving animation, skinning, LOD, and instance lifetime under the game's own systems.

### Available now

- Replace Mesh, materials, textures, and declared material parameters by resource identity.
- Attach one merged Mesh to the target Renderer with multiple submeshes and material slots.
- Use the same resource rules for world characters, character UI, and NPC instances without creating an independent Partner.
- F10 hot reload updates existing instances and keeps the previous valid generation when parsing fails.
- `cycle` toggles once per press; `hold` continuously moves shape values while held. Mod state is isolated per Mod.
- Blender 0.32.0 supports LOD0-4 selection/template replication and selected Mesh-only export with materials and textures while skipping skeleton and physics export.
- LOD export only targets levels discovered in the current project; each level gets an independent Mesh/Render rule while sharing the same switch state.

### TODO

- Unified registration of added skeletons across world, UI, and NPC instances.
- Native-factory integration for physics bones, colliders, and physics parameters.
- Regression coverage and automated release validation across more game versions.

### Upstream acknowledgements

- [AnimeStudio](https://github.com/Escartem/AnimeStudio) and its contributors provide the Unity asset browser, VFS access, dependency resolution, and export foundation. EIEM maintains the Endfield integration plus the Blender and DLL layers on top of it. AnimeStudio is MIT-licensed and its license is included with the extractor package.
- Copyright and license information for [MinHook](https://github.com/TsudaKageyu/minhook), [Dear ImGui](https://github.com/ocornut/imgui), and other dependencies is in [THIRD_PARTY_NOTICES](THIRD_PARTY_NOTICES).

Release packages:

- `EIEM_v1.0.0_dll.zip`: DLL, proxy loaders, and configuration template.
- `EIEM_Blender_v0.32.0.zip`: Blender add-on.
- `EIEM_Extractor_v1.0.0_win-x64.zip`: AnimeStudio extractor and EIEM extraction helpers.

## User Agreement & Disclaimer

<details>
<summary>Please read this agreement carefully before downloading, installing, or using this plugin (EIEM). <b>By using this plugin, you acknowledge that you have fully read, understood, and agreed to all of the following terms.</b></summary>

### 1. Open Source License & End User Rights
- This plugin is fully open-sourced on GitHub under the **AGPL-3.0** license. Users may freely use, modify, and distribute the source code of this plugin in compliance with the license.
- End Users may use and distribute this plugin **without any restrictions**, provided they do not modify it. This right is not affected by whether the user violates this agreement.

### 2. Anti-Fraud Statement
- You **must not** openly sell this plugin **itself** on online retail platforms without providing the GitHub repository address and after-sales service.
- This plugin is entirely free and open-source on GitHub. If you obtained it through a paid purchase, please be aware that it is freely available on GitHub.

### 3. Content Compliance & Conduct
- This plugin does not contain any game art assets. Users acknowledge and agree that the official animations, scenes, models, and other assets built into *Arknights: Endfield* are copyrighted by Hypergryph and are not covered by the AGPL-3.0 license. You **should not and must not** use this plugin, or any in-game official assets, to create, play, or distribute any inappropriate motions/animations (including but not limited to pornographic, violent, politically sensitive, or other content that violates laws and regulations or causes community discomfort).

### 4. Risk & Disclaimer
- This project is for educational, technical research, and communication purposes only. All Arknights game data assets used in this plugin are copyrighted by Hypergryph. Using this tool may violate the game's terms of service and carries a risk of account suspension. For any loss directly or indirectly caused by using this plugin (including but not limited to account bans, game data corruption, etc.), **this project assumes no legal or financial liability**. Users bear all risks and are strongly advised to use it on a test account.

</details>

## Features

### Implemented
- **Muscle-driven motion**: Drives full-body animation via 95 muscle values
- **Finger animation**: Independent rotation control for 30 finger bones
- **Facial expressions**: Basic expressions including AIUEO, blinks, and smiling eyes
- **Camera motion**: VMD camera keyframes (with character-facing alignment)
- **Audio sync**: MCI backend plays WAV/MP3 BGM
- **Terrain & staircase stepping**: Real-time ground collision probing, adaptively snapping to slopes and stairs
- **Native IK**: Drives the game's native BipedIK solver using VMD foot IK data

### In Progress
- **Direct VMD playback mode**

### Planned
- Multi-character screen playback
- ...

## Download

You can download the latest release from [Releases](https://github.com/Sasye/EIEM/releases) or compile it yourself from source.

> Use [Applepie Manager](https://github.com/Sasye/ApplepieManager) to easily manage and configure this plugin.

## Installation

Copy the following files into the game directory (the folder containing `Endfield.exe`):

```
bin/eiem.dll             → game_dir/plugin/eiem.dll
bin/vulkan-1.dll         → game_dir/vulkan-1.dll
bin/d3dcompiler_47.dll   → game_dir/d3dcompiler_47.dll
```

> **Note**: `d3dcompiler_47.dll` (DX environment) and `vulkan-1.dll` (Vulkan environment) are proxy loaders. You can place either one or both. If you already use another plugin that shares a proxy loader (such as [AntiKick](https://github.com/Sasye/EndFieldAntiKick), [SynchroFocus](https://github.com/Sasye/EndfieldSynchroFocus), or [EndfieldCombatHUD](https://github.com/Sasye/EndfieldCombatHUD), etc.), there is no need to place the proxy loader again.

> If you have **never installed a plugin of this type before**, you may need to create the `plugin` folder yourself.

## Preparing Resource Files

Auto-scans `game_dir/plugin/` or manually specify the following files:

| File | Description | Required |
|------|-------------|----------|
| `muscle_anim.bin` | MUS4-format motion data (exported via ExportMuscleAnimation.cs) | **Yes** |
| `*.vmd` | VMD file (facial expression morph data) | Optional (auto-scans .vmd in plugin dir) |
| `camera.vmd` | Camera motion data | Optional |
| `bgm.wav` or `bgm.mp3` | Background music | Optional |

## Usage

1. Install as described above, launch the game, and enter the game.
2. Press **Insert** to open the GUI panel.
3. Load the desired files, then click the **Play** button on the "Control" tab to start playback.

## Motion Export (VMD → MUS4)

You must first convert VMD animations to MUS4 format (`muscle_anim.bin`) using the Unity editor.

### Prerequisites

- Unity Editor
- [MMD4Mecanim](https://stereoarts.jp/#:~:text=MMD4Mecanim_Beta_20200105.zip) — converts VMD to Unity AnimationClip
- Any Humanoid MMD model

### Steps

1. Place `ExportMuscleAnimation.cs` into your Unity project's `Assets/Editor/` folder
2. Import an MMD model (set to Humanoid Rig) and use MMD4Mecanim to convert a VMD file into an AnimationClip
3. Create an Animator Controller and add the converted AnimationClip as the default state
4. Assign the Animator Controller to the model in the scene
5. **Select the model**, then click `Tools > Export Muscle Animation` in the menu bar
6. The exported file `Assets/muscle_anim.bin` will be created — copy it to the game's `plugin/` directory

> Facial expressions do not go through this export — simply place the original `.vmd` file in the `plugin/` directory and the plugin will parse the morph data automatically.
