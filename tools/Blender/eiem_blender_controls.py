"""Author and compile Blender Shape Key and outfit-switch controls.

The public functions in this module operate on Blender data-blocks.  Panels,
operators, package I/O and add-on registration remain in the main add-on so
the authoring model can be tested without duplicating UI behavior.
"""

import json
import math
import re
import uuid

import bpy


def parse_json_property(owner, name, default):
    try:
        return json.loads(owner.get(name, json.dumps(default)))
    except (TypeError, ValueError, json.JSONDecodeError):
        return default


def author_identity(owner, peers):
    """Stable .blend identity; data copies become independent without user IDs."""
    identity = str(owner.get("eiem_control_id", ""))
    if not re.fullmatch(r"[0-9a-f]{16}", identity):
        identity = uuid.uuid4().hex[:16]
        owner["eiem_control_id"] = identity
        owner["eiem_control_owner"] = owner.name
    duplicates = [peer for peer in peers if peer.get("eiem_control_id") == identity]
    if len(duplicates) > 1:
        original = next(
            (peer for peer in duplicates
             if peer.name == peer.get("eiem_control_owner")),
            sorted(duplicates, key=lambda peer: peer.name)[0],
        )
        for peer in duplicates:
            if peer != original:
                peer["eiem_control_id"] = uuid.uuid4().hex[:16]
            peer["eiem_control_owner"] = peer.name
    owner["eiem_control_owner"] = owner.name
    return owner["eiem_control_id"]


def add_shape_control(obj, name, automatic=False):
    keys = obj.data.shape_keys
    key = keys.key_blocks.get(name) if keys else None
    if key is None or key == keys.reference_key:
        raise ValueError("请先选择一个非 Basis 的形态键")
    for control in obj.data.eiem_shape_controls:
        if control.shape == name:
            control.enabled = True
            control.automatic = automatic
            return control
    control = obj.data.eiem_shape_controls.add()
    control.shape = name
    control.automatic = automatic
    control.identity = uuid.uuid4().hex[:16]
    control.label = name
    control.default = key.value
    control.minimum = key.slider_min
    control.maximum = key.slider_max
    return control


def sync_new_shape_controls(obj):
    keys = obj.data.shape_keys
    if not keys:
        return
    native = {
        frame["key"]
        for channel in parse_json_property(
            obj.data, "eiem_blend_shapes_json", [])
        for frame in channel.get("frames", [])
    }
    declared = {control.shape for control in obj.data.eiem_shape_controls}
    # Removed automatic channels disappear; explicit broken bindings remain an error.
    for index in reversed(range(len(obj.data.eiem_shape_controls))):
        control = obj.data.eiem_shape_controls[index]
        if control.automatic and control.shape not in keys.key_blocks:
            obj.data.eiem_shape_controls.remove(index)
    for key in keys.key_blocks:
        if (key != keys.reference_key and key.name not in native
                and key.name not in declared):
            add_shape_control(obj, key.name, automatic=True)


def shape_channel_name(obj, key):
    channel_name = key.name
    for channel in parse_json_property(
            obj.data, "eiem_blend_shapes_json", []):
        if any(frame.get("key") == key.name
               for frame in channel.get("frames", [])):
            if len(channel["frames"]) != 1:
                raise ValueError(
                    "多帧形态键暂不支持切换控制：" + key.name)
            channel_name = channel["name"]
    if (not channel_name.strip() or channel_name != channel_name.strip()
            or any(char in channel_name for char in "=\r\n")
            or len(channel_name.encode("utf-8")) >= 192):
        raise ValueError(
            "形态键名称不能包含换行/等号/首尾空格，UTF-8 长度须小于 192："
            + channel_name)
    return channel_name


def switch_groups(scene=None):
    scene = scene or bpy.context.scene
    return sorted(
        (collection for collection in scene.collection.children_recursive
         if collection.get("eiem_switch_group")),
        key=lambda collection: collection.name,
    )


def switch_states(group):
    return [collection for collection in group.children
            if collection.get("eiem_switch_state")]


def switch_state_values(group):
    states = switch_states(group)
    counter = max(
        int(group.get("eiem_next_state", 0)),
        max((int(state.get("eiem_state_value", -1)) + 1
             for state in states), default=0),
    )
    used = set()
    for state in states:
        value = int(state.get("eiem_state_value", -1))
        if value < 0 or value in used:
            value = counter
            counter += 1
            state["eiem_state_value"] = value
        used.add(value)
    group["eiem_next_state"] = counter
    return [int(state["eiem_state_value"]) for state in states]


def switch_meshes(state):
    return [obj for obj in state.all_objects if obj.type == "MESH"]


def switch_members(group):
    """All Meshes controlled by a group; child links remain snapshot data."""
    result = []
    seen = set()
    candidates = list(group.objects) + [
        obj for state in switch_states(group) for obj in switch_meshes(state)
    ]
    for obj in candidates:
        if obj.type == "MESH" and obj.as_pointer() not in seen:
            seen.add(obj.as_pointer())
            result.append(obj)
    return result


def capture_switch_state(group, state, context=None):
    """Record the current viewport visibility as one outfit look."""
    context = context or bpy.context
    members = switch_members(group)
    if not members:
        raise ValueError("当前切换组没有网格")
    # Old author files stored membership only as the union of state children.
    # Recording a snapshot upgrades that relation to an explicit member pool.
    for obj in members:
        if obj.name not in group.objects:
            group.objects.link(obj)
    for obj in members:
        visible = (obj.name in context.view_layer.objects
                   and not obj.hide_get(view_layer=context.view_layer))
        if visible and obj.name not in state.objects:
            state.objects.link(obj)
        elif not visible and obj.name in state.objects:
            state.objects.unlink(obj)
    return state


def validate_switch_key(value):
    """Use the same finite key vocabulary as the runtime INI parser."""
    parts = [part.strip().upper() for part in str(value).split("+")]
    modifiers = parts[:-1]
    if (not parts
            or any(part not in {"CTRL", "SHIFT", "ALT"}
                   for part in modifiers)
            or len(set(modifiers)) != len(modifiers)):
        raise ValueError("快捷键无效：" + str(value))
    key = parts[-1]
    names = {
        "INSERT", "DELETE", "HOME", "END", "PAGEUP", "PAGEDOWN",
        "LEFT", "RIGHT", "UP", "DOWN", "SPACE", "ENTER", "ESC",
        "TAB", "BACKSPACE", "CAPSLOCK", "TILDE",
        "NUMPADPLUS", "NUMPADMINUS", "NUMPADMULTIPLY",
        "NUMPADDIVIDE", "NUMPADDECIMAL",
    }
    if not (re.fullmatch(r"[A-Z0-9]", key)
            or re.fullmatch(r"NUMPAD[0-9]", key) or key in names
            or re.fullmatch(r"F(?:[1-9]|1[0-9]|2[0-4])", key)):
        raise ValueError("快捷键无效：" + str(value))
    ordered = [modifier for modifier in ("CTRL", "SHIFT", "ALT")
               if modifier in modifiers]
    return "+".join(ordered + [key])


def switch_key_from_event(event):
    """Translate one Blender keyboard press into the runtime INI vocabulary."""
    if getattr(event, "value", "") != "PRESS":
        raise ValueError("请按下一个键")
    event_type = str(getattr(event, "type", "")).upper()
    aliases = {
        "ZERO": "0", "ONE": "1", "TWO": "2", "THREE": "3",
        "FOUR": "4", "FIVE": "5", "SIX": "6", "SEVEN": "7",
        "EIGHT": "8", "NINE": "9", "DEL": "DELETE", "RET": "ENTER",
        "NUMPAD_ENTER": "ENTER", "LEFT_ARROW": "LEFT",
        "NUMPAD_0": "NUMPAD0", "NUMPAD_1": "NUMPAD1",
        "NUMPAD_2": "NUMPAD2", "NUMPAD_3": "NUMPAD3",
        "NUMPAD_4": "NUMPAD4", "NUMPAD_5": "NUMPAD5",
        "NUMPAD_6": "NUMPAD6", "NUMPAD_7": "NUMPAD7",
        "NUMPAD_8": "NUMPAD8", "NUMPAD_9": "NUMPAD9",
        "NUMPAD_PLUS": "NUMPADPLUS", "NUMPAD_MINUS": "NUMPADMINUS",
        "NUMPAD_ASTERIX": "NUMPADMULTIPLY",
        "NUMPAD_SLASH": "NUMPADDIVIDE",
        "NUMPAD_PERIOD": "NUMPADDECIMAL",
        "RIGHT_ARROW": "RIGHT", "UP_ARROW": "UP", "DOWN_ARROW": "DOWN",
        "PAGE_UP": "PAGEUP", "PAGE_DOWN": "PAGEDOWN",
        "BACK_SPACE": "BACKSPACE", "CAPS_LOCK": "CAPSLOCK",
        "ACCENT_GRAVE": "TILDE",
    }
    key = aliases.get(event_type, event_type)
    modifiers = []
    if bool(getattr(event, "ctrl", False)):
        modifiers.append("CTRL")
    if bool(getattr(event, "shift", False)):
        modifiers.append("SHIFT")
    if bool(getattr(event, "alt", False)):
        modifiers.append("ALT")
    return validate_switch_key("+".join(modifiers + [key]))


def set_switch_group_key(group, key, scene=None):
    scene = scene or bpy.context.scene
    key = validate_switch_key(key)
    if any(candidate != group
           and validate_switch_key(candidate.get("eiem_key", "")) == key
           for candidate in switch_groups(scene)):
        raise ValueError("已有切换组使用快捷键 " + key)
    if any(str(getattr(control, attribute, "")).strip()
           and validate_switch_key(getattr(control, attribute)) == key
           for _, control in shape_controls_in_scene(scene)
           for attribute in ("hotkey_increase", "hotkey_decrease")):
        raise ValueError("已有形态键控制使用快捷键 " + key)
    ui_key = str(getattr(scene, "eiem_ui_key", "")).strip()
    if ui_key and validate_switch_key(ui_key) == key:
        raise ValueError("Mod UI 已使用快捷键 " + key)
    group["eiem_key"] = key
    return key


def shape_controls_in_scene(scene=None):
    """Yield each shared Mesh control once, together with one scene object."""
    scene = scene or bpy.context.scene
    seen = set()
    for obj in scene.objects:
        if obj.type != "MESH" or not hasattr(obj.data, "eiem_shape_controls"):
            continue
        identity = obj.data.as_pointer()
        if identity in seen:
            continue
        seen.add(identity)
        for control in obj.data.eiem_shape_controls:
            yield obj, control


def set_shape_control_hotkey(obj, control, key, direction, scene=None):
    """Assign one explicit increase/decrease Shape Key chord."""
    scene = scene or bpy.context.scene
    if direction not in {"INCREASE", "DECREASE"}:
        raise ValueError("形态键按键方向无效")
    attribute = ("hotkey_increase" if direction == "INCREASE"
                 else "hotkey_decrease")
    key = str(key).strip()
    if not key:
        setattr(control, attribute, "")
        return ""
    key = validate_switch_key(key)
    if any(validate_switch_key(group.get("eiem_key", "")) == key
           for group in switch_groups(scene)):
        raise ValueError("已有网格切换组使用快捷键 " + key)
    ui_key = str(getattr(scene, "eiem_ui_key", "")).strip()
    if ui_key and validate_switch_key(ui_key) == key:
        raise ValueError("Mod UI 已使用快捷键 " + key)
    target = (control.as_pointer(), attribute)
    for _, candidate in shape_controls_in_scene(scene):
        for candidate_attribute in ("hotkey_increase", "hotkey_decrease"):
            candidate_key = str(
                getattr(candidate, candidate_attribute, "")).strip()
            if ((candidate.as_pointer(), candidate_attribute) != target
                    and candidate_key
                    and validate_switch_key(candidate_key) == key):
                raise ValueError("已有形态键控制使用快捷键 " + key)
    setattr(control, attribute, key)
    return key


def add_switch_state(group, name):
    state = bpy.data.collections.new(name)
    group.children.link(state)
    state["eiem_switch_state"] = True
    state["eiem_default"] = len(switch_states(group)) == 1
    return state


def set_switch_state_order(group, ordered_states):
    """Apply one explicit cycle order without changing stable state values."""
    current = switch_states(group)
    ordered_states = list(ordered_states)
    if (len(ordered_states) != len(current)
            or {state.as_pointer() for state in ordered_states}
            != {state.as_pointer() for state in current}):
        raise ValueError("款式顺序与当前切换组不匹配")
    for state in current:
        group.children.unlink(state)
    for state in ordered_states:
        group.children.link(state)


def reorder_switch_state(group, state, target_index):
    """Move one state in the cycle and return its clamped new index."""
    states = switch_states(group)
    if state not in states:
        raise ValueError("款式不属于当前切换组")
    source_index = states.index(state)
    target_index = max(0, min(len(states) - 1, int(target_index)))
    if source_index == target_index:
        return source_index
    states.pop(source_index)
    states.insert(target_index, state)
    set_switch_state_order(group, states)
    return target_index


def set_switch_default(group, state):
    for candidate in switch_states(group):
        candidate["eiem_default"] = candidate == state


def restore_switch_preview(objects=None, context=None):
    context = context or bpy.context
    objects = list(objects) if objects is not None else list(context.scene.objects)
    for obj in objects:
        saved = parse_json_property(obj, "eiem_preview_json", {})
        previous = saved.pop(context.view_layer.name, None)
        if previous is not None:
            obj.hide_set(previous, view_layer=context.view_layer)
            if saved:
                obj["eiem_preview_json"] = json.dumps(saved)
            else:
                del obj["eiem_preview_json"]


def preview_switch(group, state, context=None):
    context = context or bpy.context
    visible = set(switch_meshes(state))
    for obj in switch_members(group):
        if obj.name not in context.view_layer.objects:
            continue
        saved = parse_json_property(obj, "eiem_preview_json", {})
        saved.setdefault(
            context.view_layer.name,
            obj.hide_get(view_layer=context.view_layer),
        )
        obj["eiem_preview_json"] = json.dumps(saved)
        obj.hide_set(obj not in visible, view_layer=context.view_layer)


def assign_switch_meshes(state, objects, scene=None):
    scene = scene or bpy.context.scene
    objects = list(objects)
    if (not objects
            or any(obj.type != "MESH" or not obj.data.get("eiem_section")
                   for obj in objects)):
        raise ValueError("请在物体模式选择已绑定 EIEM 源资源的网格")
    restore_switch_preview(objects)
    target_group = next(
        (group for group in switch_groups(scene)
         if state in switch_states(group)), None) if state else None
    for obj in objects:
        for group in switch_groups(scene):
            if target_group is not None and group == target_group:
                continue
            if obj.name in group.objects:
                group.objects.unlink(obj)
            for previous in switch_states(group):
                if obj.name in previous.objects:
                    previous.objects.unlink(obj)
        if target_group is not None:
            if obj.name not in target_group.objects:
                target_group.objects.link(obj)
            if obj.name not in state.objects:
                state.objects.link(obj)
        if not obj.users_collection:
            scene.collection.objects.link(obj)


def create_switch_group(name, key, objects, scene=None):
    scene = scene or bpy.context.scene
    objects = list(objects)
    key = validate_switch_key(key)
    if (not objects
            or any(obj.type != "MESH" or not obj.data.get("eiem_section")
                   for obj in objects)):
        raise ValueError("请先选择 EIEM 网格部件")
    if any(validate_switch_key(group.get("eiem_key", "")) == key
           for group in switch_groups(scene)):
        raise ValueError("已有切换组使用快捷键 " + key)
    if any(str(getattr(control, attribute, "")).strip()
           and validate_switch_key(getattr(control, attribute)) == key
           for _, control in shape_controls_in_scene(scene)
           for attribute in ("hotkey_increase", "hotkey_decrease")):
        raise ValueError("已有形态键控制使用快捷键 " + key)
    ui_key = str(getattr(scene, "eiem_ui_key", "")).strip()
    if ui_key and validate_switch_key(ui_key) == key:
        raise ValueError("Mod UI 已使用快捷键 " + key)
    root = next(
        (collection for collection in scene.collection.children
         if collection.get("eiem_switch_root")), None)
    if root is None:
        root = bpy.data.collections.new("EIEM 切换")
        root["eiem_switch_root"] = True
        scene.collection.children.link(root)
    group = bpy.data.collections.new(name or "切换组")
    group["eiem_switch_group"] = True
    group["eiem_key"] = key
    root.children.link(group)
    for obj in objects:
        group.objects.link(obj)
    shown = add_switch_state(group, "款式 1")
    capture_switch_state(group, shown)
    add_switch_state(group, "款式 2")
    scene.eiem_switch_active = group
    return group


def delete_switch_group(group, scene=None, context=None):
    """Delete only the authoring group and snapshots; keep all Mesh objects."""
    scene = scene or bpy.context.scene
    context = context or bpy.context
    if not group or not group.get("eiem_switch_group"):
        raise ValueError("当前切换组不存在")
    members = switch_members(group)
    restore_switch_preview(members, context)
    parents = [collection for collection in
               [scene.collection, *scene.collection.children_recursive]
               if group.name in collection.children]
    for state in list(switch_states(group)):
        bpy.data.collections.remove(state)
    bpy.data.collections.remove(group)
    scene.eiem_switch_active = None
    for parent in parents:
        if (parent != scene.collection and parent.get("eiem_switch_root")
                and not parent.children and not parent.objects):
            bpy.data.collections.remove(parent)


def mesh_source_identity(obj):
    asset = str(obj.get("eiem_render_asset", "") or obj.data.get(
        "eiem_target_asset", obj.data.get("eiem_asset", ""))).strip()
    source = str(obj.data.get(
        "eiem_target_path", "") or obj.data.get("eiem_source", ""))
    package = str(obj.get("eiem_author_package", ""))
    return (package.replace("\\", "/").lower(),
            source.replace("\\", "/").lower(), asset.lower())


def plan_switch_export(mesh_objects, scene=None):
    """Plan exactly the selected objects; visibility is an explicit action."""
    scene = scene or bpy.context.scene
    selected = set(mesh_objects)
    if not selected:
        raise ValueError("No EIEM mesh objects selected")
    if any(obj.type != "MESH" or not obj.data.get("eiem_section")
           for obj in selected):
        raise ValueError("所选包含未绑定 EIEM 的网格")
    sources = {obj: mesh_source_identity(obj) for obj in selected}
    hidden = {obj for obj in selected if obj.hide_render}
    for obj in selected:
        if not sources[obj][2]:
            raise ValueError("网格 %s 缺少原 Mesh 命中名称" % obj.name)
    groups = switch_groups(scene)
    memberships = {}
    for group in groups:
        for obj in switch_members(group):
            if obj in selected and obj not in hidden:
                memberships.setdefault(obj, []).append(group)
    used_groups = {
        group for owned_groups in memberships.values() for group in owned_groups
    }
    group_defs = []
    bindings = {}
    keys = set()
    for group in groups:
        if group not in used_groups:
            continue
        states = switch_states(group)
        if len(states) < 2:
            raise ValueError(
                "切换组 %s 至少需要两个状态（允许空状态）" % group.name)
        defaults = [index for index, state in enumerate(states)
                    if state.get("eiem_default")]
        if len(defaults) != 1:
            raise ValueError("请为切换组 %s 指定一个初始状态" % group.name)
        key = validate_switch_key(group.get("eiem_key", ""))
        if key in keys:
            raise ValueError("导出的多个切换组使用同一快捷键：" + key)
        keys.add(key)
        group_identity = author_identity(group, bpy.data.collections)
        variable = "$switch_" + group_identity
        state_values = switch_state_values(group)
        group_defs.append(
            (group, states, state_values[defaults[0]], key, variable))
        for obj in switch_members(group):
            if obj not in selected or obj in hidden:
                continue
            if len(memberships.get(obj, [])) != 1:
                raise ValueError("网格 %s 同时属于多个切换组" % obj.name)
            visible = tuple(
                state_values[index]
                for index, state in enumerate(states)
                if obj in switch_meshes(state)
            )
            bindings[obj] = (variable, visible)

    grouped = {}
    selectors = {}
    for obj in sorted(selected, key=lambda item: (sources[item], item.name)):
        identity = sources[obj]
        if identity[2] in selectors and selectors[identity[2]] != identity:
            raise ValueError(
                "同名 Mesh 来自不同源资源/作者包，不能安全合并："
                + identity[2])
        selectors[identity[2]] = identity
        grouped.setdefault(identity, []).append(obj)
    return {
        "objects": [obj for objects in grouped.values() for obj in objects],
        "sources": list(grouped.values()),
        "groups": group_defs,
        "bindings": bindings,
        "hidden": hidden,
    }


def plan_shape_controls(objects):
    declarations = []
    bindings = {}
    hotkeys = []
    shared = {}
    for obj in objects:
        identity = obj.data.as_pointer()
        if identity not in shared:
            sync_new_shape_controls(obj)
            mesh_id = author_identity(obj.data, bpy.data.meshes)
            actions = []
            used = set()
            for control in obj.data.eiem_shape_controls:
                if not control.enabled:
                    continue
                keys = obj.data.shape_keys
                key = keys.key_blocks.get(control.shape) if keys else None
                if (key is None or key == keys.reference_key
                        or key.name in used):
                    raise ValueError(
                        "%s 的形态键控制缺失、重复或指向 Basis：%s"
                        % (obj.name, control.shape))
                used.add(key.name)
                channel_name = shape_channel_name(obj, key)
                values = (
                    (key.value, key.slider_min, key.slider_max)
                    if control.automatic
                    else (control.default, control.minimum, control.maximum)
                )
                default, minimum, maximum = values
                if (not all(math.isfinite(value) for value in values)
                        or not minimum < maximum
                        or not minimum <= default <= maximum):
                    raise ValueError(
                        "形态键滑条的默认值或范围无效：" + key.name)
                if not control.identity:
                    control.identity = uuid.uuid4().hex[:16]
                variable = "$shape_%s_%s" % (mesh_id, control.identity)
                label = (control.label or key.name).replace(
                    "\r", " ").replace("\n", " ")
                declarations.append((variable, label, *values))
                actions.append("shape.%s=%s" % (channel_name, variable))
                configured_hotkeys = [
                    ("增加", str(getattr(
                        control, "hotkey_increase", "")).strip(), maximum),
                    ("减少", str(getattr(
                        control, "hotkey_decrease", "")).strip(), minimum),
                ]
                if any(key_value for _, key_value, _ in configured_hotkeys):
                    speed = float(control.hotkey_speed)
                    if not math.isfinite(speed) or speed <= 0:
                        raise ValueError(
                            "形态键 %s 的变化速度必须大于 0" % key.name)
                for direction_label, hotkey, target in configured_hotkeys:
                    if not hotkey:
                        continue
                    hotkeys.append({
                        "object": obj,
                        "shape": key.name,
                        "label": "%s：%s" % (direction_label, label),
                        "key": validate_switch_key(hotkey),
                        "variable": variable,
                        "type": "hold",
                        "speed": speed,
                        "target": target,
                    })
            shared[identity] = actions
        bindings[obj] = shared[identity]
    return declarations, bindings, hotkeys


def lua_string(value):
    # Lua decimal escapes preserve control characters without JSON unicode escapes.
    return '"' + ''.join(
        ('\\%03d' % ord(char) if ord(char) < 32
         else '\\' + char if char in {'"', '\\'}
         else char)
        for char in str(value)
    ) + '"'


def generate_mod_ui(groups, shape_controls, shape_hotkeys=None, scene=None):
    scene = scene or bpy.context.scene
    shape_hotkeys = shape_hotkeys or []
    key = (validate_switch_key(scene.eiem_ui_key)
           if scene.eiem_ui_key.strip() else "")
    if key.split('+')[-1] == 'INSERT':
        raise ValueError("Mod UI 不使用 INSERT，请修改 UI 开关键")
    if any(key == group[3] for group in groups):
        raise ValueError("Mod UI 开关键与切换组按键重复，请修改 UI 开关键")
    if any(key == control["key"] for control in shape_hotkeys):
        raise ValueError("Mod UI 开关键与形态键控制按键重复，请修改 UI 开关键")
    lines = [
        "-- Optional Blender template. All window behavior belongs to this Lua file.",
        "return function()",
    ]
    if key:
        lines.append('  if mod.get("$ui_open") == 0 then return end')
    lines.append("  imgui.SetNextWindowSize(360, 0, imgui.Cond.FirstUseEver)")
    if key:
        lines.extend([
            "  local visible, open = imgui.Begin(%s, true)"
            % lua_string(scene.eiem_ui_title),
            '  if not open then mod.set("$ui_open", 0) end',
            "  if visible then",
        ])
    else:
        lines.append("  if imgui.Begin(%s) then"
                     % lua_string(scene.eiem_ui_title))
    for group, states, default, group_key, variable in groups:
        lines.append("    imgui.Text(%s)" % lua_string(group.name))
        values = switch_state_values(group)
        for value, state in zip(values, states):
            lines.extend([
                "    if imgui.RadioButton(%s, mod.get(%s) == %d) then" % (
                    lua_string(state.name + "##" + variable + str(value)),
                    lua_string(variable), value),
                "      mod.set(%s, %d)" % (lua_string(variable), value),
            ])
            lines.append("    end")
        lines.append("    imgui.Separator()")
    for variable, label, default, minimum, maximum in shape_controls:
        lines.extend([
            "    do",
            "      local changed, value = imgui.SliderFloat(%s, mod.get(%s), %.9g, %.9g)" % (
                lua_string(label + "##" + variable), lua_string(variable),
                minimum, maximum),
            "      if changed then mod.set(%s, value) end"
            % lua_string(variable),
            "    end",
        ])
    variables = (
        [group[4] for group in groups]
        + [control[0] for control in shape_controls]
    )
    if variables:
        lines.append('    if imgui.Button("恢复默认值") then')
        for variable in variables:
            lines.append("      mod.set(%s, mod.default(%s))" % (
                lua_string(variable), lua_string(variable)))
        lines.append("    end")
    lines.extend(["  end", "  imgui.End()", "end", ""])
    return key, '\n'.join(lines)
