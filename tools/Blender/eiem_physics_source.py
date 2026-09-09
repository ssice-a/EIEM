"""Portable native-authoring graph, independent of Blender and Unity objects.

The TypeTree and original component bytes are retained beside editable fields.
Reference identities are serialized CAB/PathID identities, never process pointers.
"""
import base64
import copy
import hashlib
import json
import math
import struct
from pathlib import Path

MAGIC = b"EIEPHYS\0"
LIMIT = 16 * 1024 * 1024
TYPES = ("BeyondBoneCloth", "BeyondBoneSphereCollider", "BeyondBoneCapsuleCollider", "BeyondBonePlaneCollider")
COLLIDER_REFERENCE = "$.serializeData.colliderCollisionConstraint.colliderList["
ROOT_REFERENCE = "$.serializeData.rootBones["
IGNORE_REFERENCE = "$.serializeData.ignoreFromRootBones["


def require(value, message):
    if not value:
        raise ValueError("Physics: " + message)


def identity(value):
    return hashlib.sha256(value.encode("utf-8")).hexdigest()[:32]


def dumps(value):
    return json.dumps(value, ensure_ascii=False, allow_nan=False, separators=(",", ":"))


def load_json(path):
    path = Path(path)
    require(path.stat().st_size <= LIMIT, "source file exceeds size limit")
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, "duplicate JSON field: " + key)
            result[key] = value
        return result
    return json.loads(path.read_text(encoding="utf-8-sig"), object_pairs_hook=pairs,
                      parse_constant=lambda value: require(False, "non-finite JSON number"))


def read_evidence(filename):
    filename = Path(filename).resolve()
    source = load_json(filename)
    require(source.get("purpose") == "raw-component-evidence-not-a-physics-package", "not a component export")
    selected = [c for c in source["components"] if c.get("inSelectedPrefab") and
                (c.get("script") or {}).get("type") in TYPES and
                c["script"].get("assembly") == "BeyondDynamicBone.dll"]
    require(bool(selected), "no supported native physics in this Prefab")
    records = []
    for component in selected:
        require(component["decoded"] and component["consumed"] == component["byteSize"],
                "component not completely decoded: " + component["ownerPath"])
        index = component["id"]
        require(isinstance(index, str) and index.isdigit(), "invalid component filename")
        def sibling(suffix):
            path = (filename.parent / (index + suffix)).resolve()
            require(path.parent == filename.parent, "component dependency escapes package")
            require(path.stat().st_size <= LIMIT, "oversized component dependency")
            return path
        raw = sibling(".bin").read_bytes()
        require(len(raw) == component["byteSize"] and
                hashlib.sha256(raw).hexdigest() == component["sha256"].lower(), "source component hash differs")
        refs = copy.deepcopy(component["references"])
        require(all(r["isNull"] or r["resolved"] for r in refs), "unresolved source component reference")
        records.append({"id": identity(component["identity"]), "source": component["identity"],
                        "type": component["script"]["type"], "name": component["ownerPath"].rsplit("/", 1)[-1],
                        "ownerPath": component["ownerPath"], "owner": component["owner"],
                        "operation": "override", "fields": load_json(sibling(".data.json")),
                        "schema": load_json(sibling(".schema.json")), "references": refs,
                        "raw": base64.b64encode(raw).decode("ascii"), "sha256": component["sha256"].lower()})
    transforms = copy.deepcopy(source["transforms"])
    roots = [t for t in transforms if t["inSelectedPrefab"] and t["parent"] is None]
    require(len(roots) == 1, "source Prefab must have one explicit Transform root")
    prefix = roots[0]["path"]
    def relative(path):
        require(path == prefix or path.startswith(prefix + "/"), "Transform is outside the selected Prefab")
        return path[len(prefix):].lstrip("/")
    for t in transforms:
        t["bone"] = relative(t["path"])
    for c in records:
        c["bone"] = relative(c["ownerPath"])
    return {"version": 2, "purpose": "native-authoring", "coordinate": "unity-y-up-left-handed",
            "backend": "BeyondDynamicBone", "id": identity(source["prefab"] + source["vfsFingerprint"]),
            "skeleton": "", "source": {"prefab": source["prefab"], "fingerprint": source["vfsFingerprint"]},
            "components": records, "transforms": transforms}


def validate(value, skeleton_required=True):
    require(isinstance(value, dict) and value.get("version") == 2 and value.get("purpose") == "native-authoring",
            "unsupported native authoring version")
    require(value.get("backend") == "BeyondDynamicBone" and value.get("coordinate") == "unity-y-up-left-handed",
            "unsupported native backend or coordinates")
    import re
    require(re.fullmatch("[0-9a-f]{32}", value.get("id", "")), "invalid author identity")
    if skeleton_required:
        path = value.get("skeleton", "")
        require(path.endswith(".skeleton") and not any(c in path for c in "\\:\0") and
                all(p not in ("", ".", "..") for p in path.split("/")), "invalid Skeleton dependency")
    components, transforms = value.get("components"), value.get("transforms")
    require(isinstance(components, list) and 1 <= len(components) <= 4096, "invalid component count")
    require(isinstance(transforms, list) and 1 <= len(transforms) <= 16384, "invalid Transform count")
    sources, ids, owners = set(), set(), set()
    transform_ids = {t["identity"] for t in transforms}
    require(len(transform_ids) == len(transforms), "duplicate Transform identity")
    for t in transforms:
        require(t["parent"] is None or t["parent"] in transform_ids, "missing Transform ancestor")
        require(t["owner"] not in owners, "duplicate Transform owner")
        owners.add(t["owner"])
        for key, size in (("localPosition", 3), ("localRotation", 4), ("localScale", 3)):
            require(len(t[key]) == size and all(type(x) in (int, float) and math.isfinite(x) for x in t[key]),
                    "invalid Transform " + key)
        require(abs(sum(x*x for x in t["localRotation"])-1) < .002, "invalid Transform quaternion")
        require(all(x > 0 for x in t["localScale"]), "invalid Transform scale")
    parents = {t["identity"]: t["parent"] for t in transforms}
    finished = set()
    for key in parents:
        visiting = set()
        while key is not None and key not in finished:
            require(key not in visiting, "cyclic Transform hierarchy")
            visiting.add(key)
            key = parents[key]
        finished.update(visiting)
    for c in components:
        require(c["type"] in TYPES and c["operation"] in ("override", "create", "disable"), "unknown component operation/type")
        require(re.fullmatch("[0-9a-f]{32}", c["id"]) and c["id"] not in ids and c["source"] not in sources,
                "duplicate component identity")
        ids.add(c["id"]); sources.add(c["source"])
        require(c["owner"] in owners and isinstance(c["fields"], dict), "missing component owner/fields")
        raw = base64.b64decode(c["raw"], validate=True)
        require(hashlib.sha256(raw).hexdigest() == c["sha256"], "original component bytes differ")
        for ref in c["references"]:
            require(ref["isNull"] or ref["resolved"], "unresolved native reference")
            if ref["type"] == "Transform" and not ref["isNull"]:
                require(ref["identity"] in transform_ids, "missing referenced Transform")
        if c["type"] == "BeyondBoneCloth":
            selection = c["fields"]["serializeData2"]["selectionData"]
            require(len(selection["positions"]) == len(selection["attributes"]), "selection arrays differ")
    for c in components:
        for ref in c["references"]:
            if not ref["isNull"] and ref["type"] == "MonoBehaviour":
                require(ref["identity"] in sources, "missing referenced physics component: " + ref["field"])
    return value


def closure(value, selected):
    by_source = {c["source"]: c for c in value["components"]}
    selected = set(selected)
    require(bool(selected) and selected <= set(by_source), "select physics components to export")
    pending = list(selected)
    while pending:
        for ref in by_source[pending.pop()]["references"]:
            target = ref["identity"]
            if target in by_source and target not in selected:
                selected.add(target); pending.append(target)
    out = copy.deepcopy(value)
    out["components"] = [c for c in out["components"] if c["source"] in selected]
    return out


def referenced_sources(component, field_prefix, reference_type=None):
    """Return ordered source identities for one serialized reference list."""
    result = []
    for ref in component["references"]:
        if (not ref["isNull"] and ref["field"].startswith(field_prefix) and
                (reference_type is None or ref["type"] == reference_type)):
            result.append(ref["identity"])
    return result


def group_collider_sources(component):
    require(component["type"] == "BeyondBoneCloth", "component is not a native physics group")
    return referenced_sources(component, COLLIDER_REFERENCE, "MonoBehaviour")


def group_transform_paths(value, component):
    """Resolve the BoneCloth root/ignore hierarchy without mapping selection points.

    These paths are the Transform input graph.  SelectionData remains a distinct
    serialized point array and is deliberately not zipped to this result.
    """
    require(component["type"] == "BeyondBoneCloth", "component is not a native physics group")
    by_id = {item["identity"]: item for item in value["transforms"]}
    children = {}
    for item in value["transforms"]:
        children.setdefault(item["parent"], []).append(item["identity"])
    roots = referenced_sources(component, ROOT_REFERENCE, "Transform")
    ignores = set(referenced_sources(component, IGNORE_REFERENCE, "Transform"))
    require(bool(roots), "native group has no root Transform")
    require(all(item in by_id for item in roots) and all(item in by_id for item in ignores),
            "native group references a missing Transform")
    result, seen = [], set()
    def visit(identity_value):
        if identity_value in ignores or identity_value in seen:
            return
        seen.add(identity_value)
        result.append(by_id[identity_value]["bone"])
        for child in children.get(identity_value, []):
            visit(child)
    for identity_value in roots:
        visit(identity_value)
    return result


def required_transforms(value):
    """Return the source Transform closure needed to author native physics.

    Prefab evidence also contains renderer, IK, VFX and other unrelated
    Transforms. Native authoring needs component owners, explicitly referenced
    Transforms, every root/ignore-resolved BoneCloth node, and their ancestors.
    """
    by_id = {item["identity"]: item for item in value["transforms"]}
    by_path = {item["bone"]: item for item in value["transforms"]}
    selected = set()
    for component in value["components"]:
        require(component["bone"] in by_path, "component owner Transform is missing")
        selected.add(by_path[component["bone"]]["identity"])
        for ref in component["references"]:
            if ref["type"] == "Transform" and not ref["isNull"]:
                require(ref["identity"] in by_id, "referenced Transform is missing")
                selected.add(ref["identity"])
        if component["type"] == "BeyondBoneCloth":
            for path in group_transform_paths(value, component):
                selected.add(by_path[path]["identity"])
    pending = list(selected)
    while pending:
        parent = by_id[pending.pop()]["parent"]
        if parent is not None and parent not in selected:
            require(parent in by_id, "physics Transform ancestor is missing")
            selected.add(parent)
            pending.append(parent)
    remaining = set(selected)
    result = []
    while remaining:
        ready = sorted(
            (identity_value for identity_value in remaining
             if by_id[identity_value]["parent"] not in remaining),
            key=lambda identity_value: by_id[identity_value]["bone"],
        )
        require(bool(ready), "cyclic required Transform hierarchy")
        for identity_value in ready:
            result.append(by_id[identity_value])
            remaining.remove(identity_value)
    return result


def collider_geometry(component):
    """Decode the sphere centers used by the native collider simulation.

    BeyondDynamicBone stores capsule length as the complete outside length,
    including both rounded ends.  StartSimulationStepJob places the start
    sphere along +GetLocalDir and the end sphere along -GetLocalDir; when the
    capsule is not centered, the start sphere center is the component center.
    """
    kind, fields = component["type"], component["fields"]
    require(kind in TYPES[1:], "component is not a native collider")
    center = tuple(float(fields["center"][axis]) for axis in "xyz")
    if kind == "BeyondBonePlaneCollider":
        return {"shape": "PLANE", "center": center, "normal": (0.0, 1.0, 0.0)}
    size = tuple(float(fields["size"][axis]) for axis in "xyz")
    if kind == "BeyondBoneSphereCollider":
        require(size[0] >= 0, "invalid native sphere radius")
        return {"shape": "SPHERE", "center": center, "radius": size[0]}
    direction = int(fields["direction"])
    require(direction in (0, 1, 2) and min(size) >= 0, "invalid native capsule size/direction")
    axis = [0.0, 0.0, 0.0]
    axis[direction] = -1.0 if bool(fields["reverseDirection"]) else 1.0
    length = size[2]
    start_radius = size[0]
    end_radius = size[1] if bool(fields["radiusSeparation"]) else start_radius
    if bool(fields["alignedOnCenter"]):
        start_distance = max(0.0, length * .5 - start_radius)
        end_distance = max(0.0, length * .5 - end_radius)
    else:
        start_distance = 0.0
        end_distance = max(0.0, length - start_radius - end_radius)
    start = tuple(center[i] + axis[i] * start_distance for i in range(3))
    end = tuple(center[i] - axis[i] * end_distance for i in range(3))
    return {"shape": "CAPSULE", "center": center, "axis": direction,
            "reverse": bool(fields["reverseDirection"]),
            "alignedOnCenter": bool(fields["alignedOnCenter"]),
            "start": start, "end": end, "startRadius": start_radius,
            "endRadius": end_radius, "length": length,
            "segmentLength": start_distance + end_distance}


def numeric_fields(value, path=()):
    """Keep source references and prebuild arrays out of numeric editing."""
    if isinstance(value, dict):
        if "m_PathID" in value or "arrayBytes" in value:
            return
        for key, item in value.items():
            yield from numeric_fields(item, path + (key,))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            yield from numeric_fields(item, path + (index,))
    elif type(value) in (float, int, bool):
        yield path, value


def get_field(value, path):
    for part in path:
        value = value[part]
    return value


def schema_types(nodes):
    result, stack = {}, []
    for node in nodes:
        level = node["level"]
        stack = stack[:level] + [node["name"]]
        parts = stack[1:]
        normalized = []
        for part in parts:
            if part == "Array": continue
            normalized.append("*" if part == "data" and "Array" in parts else part)
        result[tuple(normalized)] = node["type"]
    return result


def set_field(value, path, replacement, floating=False):
    require(bool(path), "cannot replace the source record")
    parent = get_field(value, path[:-1])
    original = parent[path[-1]]
    require((type(original) is type(replacement) or floating and type(original) in (int,float) and type(replacement) is float) and
            (type(replacement) is not float or math.isfinite(replacement)), "field type/value differs")
    parent[path[-1]] = replacement


def encode(value):
    validate(value)
    data = bytearray(MAGIC + struct.pack("<I", 2))
    nodes = 0
    def emit(item, depth=0):
        nonlocal nodes
        nodes += 1
        require(nodes <= 524288, "native tree has too many values")
        require(depth <= 64, "native data is too deeply nested")
        if item is None: data.append(0)
        elif type(item) is bool: data.append(2 if item else 1)
        elif type(item) is int:
            data.append(3); data.extend(struct.pack("<q", item))
        elif type(item) is float:
            require(math.isfinite(item), "non-finite native field")
            data.append(4); data.extend(struct.pack("<d", item))
        elif isinstance(item, str):
            raw = item.encode("utf-8")
            require(len(raw) <= LIMIT and "\0" not in item, "invalid native string")
            data.append(5); data.extend(struct.pack("<I", len(raw))); data.extend(raw)
        elif isinstance(item, list):
            require(len(item) <= 262144, "native array is too large")
            data.append(6); data.extend(struct.pack("<I", len(item)))
            for child in item: emit(child, depth+1)
        elif isinstance(item, dict):
            data.append(7); data.extend(struct.pack("<I", len(item)))
            for key, child in item.items():
                require(isinstance(key, str), "native key must be a string")
                emit(key, depth+1); emit(child, depth+1)
        else: raise ValueError("unsupported native field")
        require(len(data) <= LIMIT, "Physics package exceeds size limit")
    emit(value)
    return bytes(data)


def decode(data):
    require(len(data) <= LIMIT and data[:12] == MAGIC + struct.pack("<I", 2), "not Physics v2")
    offset = 12
    nodes = 0
    def raw(count):
        nonlocal offset
        require(count <= len(data)-offset, "truncated native authoring file")
        result = data[offset:offset+count]; offset += count
        return result
    def count():
        value, = struct.unpack("<I", raw(4))
        require(value <= LIMIT, "oversized native field")
        return value
    def read(depth=0):
        nonlocal nodes
        nodes += 1
        require(nodes <= 524288, "native tree has too many values")
        require(depth <= 64, "native data is too deeply nested")
        tag, = raw(1)
        if tag < 3: return (None, False, True)[tag]
        if tag in (3, 4):
            v, = struct.unpack("<q" if tag == 3 else "<d", raw(8))
            require(tag == 3 or math.isfinite(v), "non-finite native field")
            return v
        if tag == 5:
            value = raw(count()).decode("utf-8", "strict")
            require("\0" not in value, "invalid native string")
            return value
        require(tag in (6, 7), "unknown native field tag")
        size = count()
        require(size <= 262144 and size <= len(data)-offset, "invalid native collection size")
        if tag == 6: return [read(depth+1) for _ in range(size)]
        result = {}
        for _ in range(size):
            key = read(depth+1)
            require(isinstance(key, str) and key not in result, "duplicate or non-string native key")
            result[key] = read(depth+1)
        return result
    result = read()
    require(offset == len(data), "trailing native authoring data")
    return validate(result)


def read(path):
    path = Path(path)
    require(path.stat().st_size <= LIMIT, "oversized Physics file")
    return decode(path.read_bytes())
