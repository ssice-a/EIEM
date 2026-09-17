# Hold Keys

`[Key...]` keeps the existing edge-triggered `type=cycle` behavior. A
`type=hold` key is sampled while its chord is held down and moves each assigned
variable toward one target value at `speed` units per second.

```ini
[Constants]
$shape=0

[KeyShapeIncrease]
key=UP
type=hold
speed=0.5
$shape=1

[KeyShapeDecrease]
key=DOWN
type=hold
speed=0.5
$shape=0
```

Hold assignments must contain exactly one target value and `speed` must be
positive. The DLL samples hold keys independently of the operating system's
keyboard repeat rate, while cycle keys remain one event per press.

The Blender shape-key exporter emits this form for its increase/decrease
hotkeys. Its `hotkey_speed` property is therefore the variable movement speed;
existing hand-written `shape_speed.*` Render fields remain supported for
independent Renderer smoothing.
