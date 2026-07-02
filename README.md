# HBM V2 — Time Rewind

Geode mod for Geometry Dash 2.208x that adds a **Snapshot-based Time Rewind**
system to Normal Mode.

Hold the bound key (default: **N**) to smoothly rewind up to 5 seconds of
gameplay using the game's native checkpoint machinery, so triggers, saw
rotations, particles and the wave trail all rewind consistently.

## How it works

- `PlayLayer::update` is hooked and every N physics ticks the mod calls
  `PlayLayer::createCheckpoint()` and stores the resulting `CheckpointObject`
  in a ring buffer (`std::deque<Ref<CheckpointObject>>`).
- When the rewind keybind is held, `PlayLayer::update` stops advancing the
  simulation and instead pops checkpoints from the back of the buffer,
  calling `PlayLayer::loadFromCheckpoint()` on each one.
- Buffer depth, snapshot cadence and rewind speed are all Geode settings and
  can be tweaked at runtime through `Mod::get()->getSettingValue<...>()`.
- Hidden checkpoints are removed from `m_checkpointArray` so the practice
  mode UI never renders them.

## Settings

| Setting              | Default | Description                                       |
|----------------------|---------|---------------------------------------------------|
| Rewind Buffer Depth  | 5.0 s   | How far back you can rewind.                      |
| Snapshot Interval    | 4 ticks | Physics ticks between checkpoints.                |
| Rewind Speed         | 2.0×    | Snapshots consumed per rendered frame on rewind.  |

## Keybind

Registered via `geode.custom-keybinds` under the **HBM V2** category, default
`N`. Rebindable through Geode's settings UI.

## Building

The repository ships with a GitHub Actions workflow
(`.github/workflows/build.yml`) that uses
[`geode-sdk/build-geode-mod`](https://github.com/geode-sdk/build-geode-mod) to
produce a `.geode` file for Windows, macOS, Android32 and Android64 on every
push. Tagging a commit with `vX.Y.Z` also publishes a GitHub Release with the
combined `.geode` artefact attached.

Local build:

```sh
geode build
```

(requires the [Geode CLI](https://docs.geode-sdk.org/) and a configured
`GEODE_SDK` env var).
