"""EIEM Physics authoring interchange; contains no live native identifiers.

Capsule span is the distance between cap centres. Native SetSize length is
therefore span + start radius + end radius.
"""
import math
import re
import struct
from pathlib import Path

MAGIC = b"EIEPHYS\0"
LEGACY_VERSION = 1
RADIUS_VERSION = 3
NATIVE_PARAMETER_VERSION = 4
VERSION = 5  # Version 2 belongs to the native source-graph format.
COORDINATE = "unity-y-up-left-handed"
PARAMETERS = ("gravity", "stablizationTimeAfterReset", "gravityFalloff",
              "blendWeight", "animationPoseRatio")
DEFAULTS = dict(zip(PARAMETERS, (9.8, 0.1, 0.0, 1.0, 0.0)))
ROLES = ("FIXED", "MOVE", "IGNORE")
SHAPES = ("SPHERE", "CAPSULE")
LIMIT = 16 * 1024 * 1024
MAX_CURVE_KEYS = 64
MAX_NATIVE_PARAMETERS = 4096
_NATIVE_PATH = re.compile(r"serializeData(?:\.[A-Za-z_][A-Za-z0-9_]*|\.[0-9]+)+\Z")
_FORBIDDEN_NATIVE_ROOTS = {
    "sourceRenderers", "paintMaps", "rootBones", "ignoreFromRootBones",
    "colliderList", "verificationResult",
}


def default_radius():
    return {"value": 0.006, "useCurve": True, "keys": [
        {"time": 0.0, "value": 1.0, "inSlope": 0.0, "outSlope": 0.0,
         "weightedMode": 0, "inWeight": 1 / 3, "outWeight": 1 / 3},
        {"time": 1.0, "value": 1.0, "inSlope": 0.0, "outSlope": 0.0,
         "weightedMode": 0, "inWeight": 1 / 3, "outWeight": 1 / 3},
    ], "preInfinity": 2, "postInfinity": 2, "rotationOrder": 4}


def require(condition, message):
    if not condition:
        raise ValueError("Physics: " + message)


def keys(value, expected, label):
    require(isinstance(value, dict) and set(value) == set(expected.split()),
            label + " has missing or unsupported fields")


def string(value, label, empty=False):
    require(isinstance(value, str) and (empty or bool(value)) and
            len(value.encode("utf-8")) <= 4096 and "\0" not in value, label + " is invalid")


def identity(value):
    require(isinstance(value, str) and re.fullmatch(r"[a-f0-9]{32}", value), "invalid author identity")


def path(value, empty=False):
    string(value, "bone/resource path", empty)
    require("\\" not in value and ":" not in value and
            (not value or all(p not in ("", ".", "..") for p in value.split("/"))), "invalid relative path")


def number(value, minimum=0.0, maximum=3.4028234663852886e38):
    require(type(value) in (int, float) and math.isfinite(value) and minimum <= value <= maximum,
            "invalid numeric value")


def vector(value, count):
    require(isinstance(value, (list, tuple)) and len(value) == count, "invalid vector size")
    for item in value:
        number(item, -3.4028234663852886e38)


def native_parameter(value):
    keys(value, "path floating value", "native parameter")
    string(value["path"], "native parameter path")
    require(_NATIVE_PATH.fullmatch(value["path"]) is not None,
            "invalid native parameter path")
    root = value["path"].split(".", 2)[1]
    require(re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", root) is not None and
            root not in _FORBIDDEN_NATIVE_ROOTS and
            ".m_PathID" not in value["path"] and ".arrayBytes" not in value["path"],
            "native parameter cannot replace topology or runtime state")
    require(type(value["floating"]) is bool, "invalid native parameter type")
    if value["floating"]:
        number(value["value"], -3.4028234663852886e38)
    else:
        require(type(value["value"]) is int and -2147483648 <= value["value"] <= 2147483647,
                "invalid native integer parameter")


def validate(document, bone_paths=None):
    keys(document, "version purpose coordinate backend id skeleton groups colliders", "document")
    require(type(document["version"]) is int and document["version"] in
            (LEGACY_VERSION, RADIUS_VERSION, NATIVE_PARAMETER_VERSION, VERSION),
            "unsupported version")
    require(document["purpose"] == "authoring", "unsupported resource purpose")
    require(document["coordinate"] == COORDINATE and document["backend"] == "BeyondDynamicBone",
            "unsupported coordinate/backend")
    identity(document["id"])
    path(document["skeleton"])
    require(document["skeleton"].endswith(".skeleton"), "missing Skeleton dependency")
    groups, colliders = document["groups"], document["colliders"]
    require(isinstance(groups, list) and 1 <= len(groups) <= 1024, "invalid group count")
    require(isinstance(colliders, list) and len(colliders) <= 4096, "invalid collider count")
    all_ids, collider_ids = set(), set()

    def unique(item):
        identity(item["id"])
        require(item["id"] not in all_ids, "duplicate author identity")
        all_ids.add(item["id"])
        string(item["name"], "display name")

    def bone(value):
        path(value, empty=True)
        require(bone_paths is None or value in bone_paths, "missing Skeleton bone: " + value)

    for collider in colliders:
        keys(collider, "id name bone shape position rotation radius span" +
             (" endRadius alignedOnCenter" if document["version"] >= VERSION else ""), "collider")
        unique(collider)
        collider_ids.add(collider["id"])
        bone(collider["bone"])
        require(collider["shape"] in SHAPES, "unsupported collider type")
        vector(collider["position"], 3)
        vector(collider["rotation"], 4)
        require(abs(sum(x*x for x in collider["rotation"]) - 1) <= 0.001, "unnormalized rotation")
        number(collider["radius"], 1.401298464324817e-45)
        if document["version"] >= VERSION:
            number(collider["endRadius"], 1.401298464324817e-45)
            require(type(collider["alignedOnCenter"]) is bool,
                    "invalid collider alignment")
        number(collider["span"])
        require(collider["shape"] != "SPHERE" or
                (collider["span"] == 0 and collider.get("endRadius", collider["radius"]) == collider["radius"]),
                "sphere span and radii are invalid")
        if (document["version"] >= VERSION and collider["shape"] == "CAPSULE" and
                collider["alignedOnCenter"]):
            require(collider["span"] >= abs(collider["radius"] - collider["endRadius"]),
                    "centered capsule cannot represent the requested cap-centre span")
    used_colliders = set()
    for group in groups:
        keys(group, "id name nodes parameters colliders" +
             (" radius" if document["version"] >= RADIUS_VERSION else "") +
             (" nativeParameters" if document["version"] >= NATIVE_PARAMETER_VERSION else ""), "group")
        unique(group)
        nodes = group["nodes"]
        require(isinstance(nodes, list) and 2 <= len(nodes) <= 16384, "a chain needs 2..16384 nodes")
        node_paths = set()
        for node in nodes:
            keys(node, "bone role", "node")
            bone(node["bone"])
            require(node["bone"] not in node_paths and node["role"] in ROLES, "duplicate node or invalid role")
            node_paths.add(node["bone"])
        roots = [n for n in nodes if n["bone"] == "" or n["bone"].rsplit("/", 1)[0]
                 not in node_paths or "/" not in n["bone"] and "" not in node_paths]
        root_paths = {node["bone"] for node in roots}
        require(bool(roots) and all(node["role"] == "FIXED" for node in roots),
                "each chain root must be fixed")
        require(all(node["role"] != "FIXED" or node["bone"] in root_paths for node in nodes),
                "only chain roots can be fixed")
        require(any(n["role"] == "MOVE" for n in nodes), "a chain needs a moving node")
        keys(group["parameters"], " ".join(PARAMETERS), "parameters")
        for name, value in group["parameters"].items():
            number(value, maximum=1.0 if name in PARAMETERS[2:] else 3.4028234663852886e38)
        if document["version"] >= RADIUS_VERSION:
            radius = group["radius"]
            keys(radius, "value useCurve keys preInfinity postInfinity rotationOrder", "node radius")
            number(radius["value"], 1.401298464324817e-45)
            require(type(radius["useCurve"]) is bool, "invalid node radius curve switch")
            curve = radius["keys"]
            require(isinstance(curve, list) and 2 <= len(curve) <= MAX_CURVE_KEYS,
                    "node radius curve needs 2..%d keys" % MAX_CURVE_KEYS)
            times = []
            for key in curve:
                keys(key, "time value inSlope outSlope weightedMode inWeight outWeight", "node radius key")
                number(key["time"], 0.0, 1.0); times.append(key["time"])
                number(key["value"])
                number(key["inSlope"], -3.4028234663852886e38)
                number(key["outSlope"], -3.4028234663852886e38)
                require(type(key["weightedMode"]) is int and 0 <= key["weightedMode"] <= 3,
                        "invalid node radius weighted mode")
                number(key["inWeight"], 0.0, 1.0); number(key["outWeight"], 0.0, 1.0)
            require(all(a < b for a, b in zip(times, times[1:])),
                    "node radius key times must increase")
            for name in ("preInfinity", "postInfinity"):
                require(type(radius[name]) is int and -2147483648 <= radius[name] <= 2147483647,
                        "invalid node radius infinity mode")
            require(type(radius["rotationOrder"]) is int and 0 <= radius["rotationOrder"] <= 5,
                    "invalid node radius rotation order")
        if document["version"] >= NATIVE_PARAMETER_VERSION:
            parameters = group["nativeParameters"]
            require(isinstance(parameters, list) and len(parameters) <= MAX_NATIVE_PARAMETERS,
                    "invalid native parameter count")
            native_paths = set()
            for parameter in parameters:
                native_parameter(parameter)
                require(parameter["path"] not in native_paths, "duplicate native parameter path")
                native_paths.add(parameter["path"])
        refs = group["colliders"]
        require(isinstance(refs, list) and len(refs) <= 4096 and all(isinstance(r, str) for r in refs),
                "invalid collider references")
        require(len(set(refs)) == len(refs) and set(refs) <= collider_ids, "duplicate or missing collider reference")
        used_colliders.update(refs)
    require(used_colliders == collider_ids, "unreferenced collider in selected dependency closure")
    return document


class Writer:
    def __init__(self):
        self.data = bytearray()

    def raw(self, value):
        self.data.extend(value)

    def value(self, fmt, *values):
        self.raw(struct.pack("<" + fmt, *values))

    def string(self, value):
        data = value.encode("utf-8")
        self.value("I", len(data))
        self.raw(data)


class Reader:
    def __init__(self, data):
        require(len(data) <= LIMIT, "resource exceeds size limit")
        self.data, self.offset = data, 0

    def raw(self, size):
        require(size >= 0 and self.offset + size <= len(self.data), "truncated resource")
        value = self.data[self.offset:self.offset + size]
        self.offset += size
        return value

    def value(self, fmt):
        return struct.unpack("<" + fmt, self.raw(struct.calcsize("<" + fmt)))

    def count(self, maximum):
        value, = self.value("I")
        require(value <= maximum, "count exceeds limit")
        return value

    def string(self):
        return self.raw(self.count(4096)).decode("utf-8", "strict")


def encode(document):
    validate(document)
    w = Writer()
    w.raw(MAGIC)
    w.value("I", document["version"])
    for key in ("purpose", "coordinate", "backend", "id", "skeleton"):
        w.string(document[key])
    w.value("I", len(document["colliders"]))
    for c in document["colliders"]:
        for key in ("id", "name", "bone"):
            w.string(c[key])
        w.value("B", SHAPES.index(c["shape"]))
        w.value("3f", *c["position"])
        w.value("4f", *c["rotation"])
        if document["version"] >= VERSION:
            w.value("3f", c["radius"], c["endRadius"], c["span"])
            w.value("B", int(c["alignedOnCenter"]))
        else:
            w.value("2f", c["radius"], c["span"])
    w.value("I", len(document["groups"]))
    for g in document["groups"]:
        w.string(g["id"])
        w.string(g["name"])
        w.value("I", len(g["nodes"]))
        for node in g["nodes"]:
            w.string(node["bone"])
            w.value("B", ROLES.index(node["role"]))
        w.value("5f", *(g["parameters"][key] for key in PARAMETERS))
        if document["version"] >= RADIUS_VERSION:
            radius = g["radius"]
            w.value("fB", radius["value"], int(radius["useCurve"]))
            w.value("I", len(radius["keys"]))
            for key in radius["keys"]:
                w.value("4fI2f", key["time"], key["value"], key["inSlope"], key["outSlope"],
                        key["weightedMode"], key["inWeight"], key["outWeight"])
            w.value("3i", radius["preInfinity"], radius["postInfinity"], radius["rotationOrder"])
        if document["version"] >= NATIVE_PARAMETER_VERSION:
            w.value("I", len(g["nativeParameters"]))
            for parameter in g["nativeParameters"]:
                w.string(parameter["path"])
                w.value("B", int(parameter["floating"]))
                w.value("f" if parameter["floating"] else "i", parameter["value"])
        w.value("I", len(g["colliders"]))
        for ref in g["colliders"]:
            w.string(ref)
    require(len(w.data) <= LIMIT, "resource exceeds size limit")
    return bytes(w.data)


def decode(data):
    r = Reader(data)
    require(r.raw(8) == MAGIC, "invalid magic")
    version, = r.value("I")
    require(version in (LEGACY_VERSION, RADIUS_VERSION, NATIVE_PARAMETER_VERSION, VERSION), "unsupported version")
    d = {"version": version}
    for key in ("purpose", "coordinate", "backend", "id", "skeleton"):
        d[key] = r.string()
    d["colliders"] = []
    for _ in range(r.count(4096)):
        c = {key: r.string() for key in ("id", "name", "bone")}
        shape, = r.value("B")
        require(shape < len(SHAPES), "unsupported collider type")
        c.update(shape=SHAPES[shape], position=list(r.value("3f")), rotation=list(r.value("4f")))
        if version >= VERSION:
            c["radius"], c["endRadius"], c["span"] = r.value("3f")
            aligned, = r.value("B")
            require(aligned <= 1, "invalid collider alignment")
            c["alignedOnCenter"] = bool(aligned)
        else:
            c["radius"], c["span"] = r.value("2f")
        d["colliders"].append(c)
    d["groups"] = []
    for _ in range(r.count(1024)):
        g = {"id": r.string(), "name": r.string(), "nodes": []}
        for _ in range(r.count(16384)):
            bone, role = r.string(), r.value("B")[0]
            require(role < len(ROLES), "invalid node role")
            g["nodes"].append({"bone": bone, "role": ROLES[role]})
        g["parameters"] = dict(zip(PARAMETERS, r.value("5f")))
        if version >= RADIUS_VERSION:
            value, use_curve = r.value("fB")
            radius = {"value": value, "useCurve": bool(use_curve), "keys": []}
            require(use_curve <= 1, "invalid node radius curve switch")
            for _ in range(r.count(MAX_CURVE_KEYS)):
                time, value, in_slope, out_slope, weighted_mode, in_weight, out_weight = r.value("4fI2f")
                radius["keys"].append({"time": time, "value": value, "inSlope": in_slope,
                    "outSlope": out_slope, "weightedMode": weighted_mode,
                    "inWeight": in_weight, "outWeight": out_weight})
            radius["preInfinity"], radius["postInfinity"], radius["rotationOrder"] = r.value("3i")
            g["radius"] = radius
        if version >= NATIVE_PARAMETER_VERSION:
            g["nativeParameters"] = []
            for _ in range(r.count(MAX_NATIVE_PARAMETERS)):
                path, floating = r.string(), r.value("B")[0]
                require(floating <= 1, "invalid native parameter type")
                value, = r.value("f" if floating else "i")
                g["nativeParameters"].append({"path": path, "floating": bool(floating), "value": value})
        g["colliders"] = [r.string() for _ in range(r.count(4096))]
        d["groups"].append(g)
    require(r.offset == len(data), "trailing resource data")
    return validate(d)


def read(pathname):
    path = Path(pathname)
    require(path.stat().st_size <= LIMIT, "resource exceeds size limit")
    return decode(path.read_bytes())
