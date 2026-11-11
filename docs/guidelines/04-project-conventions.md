# 04. Project conventions

Where code and content go, and how they are named.

## Rules at a glance

1. C++ owns systems, math, data formats and anything performance-critical. Blueprints own flow,
   visual states and tuning.
2. C++ exposes small, explicit APIs to Blueprint (`BlueprintCallable`,
   `BlueprintImplementableEvent`). No Blueprint reaches into C++ internals.
3. Tuning values live in Data Assets or `EditDefaultsOnly` properties. A number a designer might
   change is never a literal in code.
4. Project assets live under `/Game/MyGame/<Feature>/`. Levels live in `/Game/MyGame/Maps/`.
5. `/Game/Developers/<user>/` is scratch space. Project content never references it.
6. Asset names carry the prefix for their type (table below).
7. Input uses Enhanced Input. Do not add legacy axis or action mappings.
8. Settings changes go in `Config/Default*.ini` with a comment that says why and which stage
   made them.
9. For a risky system with no established pattern, read the engine source, pick an approach, and
   write a decision record before building on it.

## C++ and Blueprint split

| Put it in C++ | Put it in Blueprint |
|---|---|
| Simulation and math (movement models, procedural generation, damage rules) | Level and encounter flow |
| Data formats, save data, serialization | Visual and audio states, timelines, feedback |
| Anything per-frame over many objects | Tuning: speeds, curves, colors, timings |
| Subsystems and components other features depend on | Subclasses of C++ actors that assign assets |
| Anything that needs an automation test | UI layout and binding |

A common shape: a C++ base class holds the logic and declares events; a `BP_` subclass assigns
meshes, sounds and Data Assets and implements the cosmetic events. The game then references the
`BP_` class, set in a Data Asset or in project settings, never by a hard-coded path in C++.

## Source layout

```
Source/MyGame/
  MyGame.Build.cs
  Public/<Feature>/     headers other modules or features include
  Private/<Feature>/    implementation
  Private/Tests/        automation tests, one file per area
```

Group by feature (Player, Combat, World, Save, UI), not by class kind. Keep one responsibility
per class. Prefer components and subsystems over deep inheritance, and interfaces or delegates
over singletons, so features can be tested and swapped.

## Content layout

```
/Game/MyGame/
  Core/         game mode, game instance, shared Data Assets
  Player/       pawn, controller, input actions and mapping contexts
  <Feature>/    one folder per feature, holding its Blueprints, materials, meshes, effects
  UI/
  Audio/
  Maps/         levels
```

## Asset name prefixes

| Prefix | Asset | Prefix | Asset |
|---|---|---|---|
| `BP_` | Blueprint | `WBP_` | Widget Blueprint |
| `BPI_` | Blueprint Interface | `DA_`, `DT_` | Data Asset, Data Table |
| `M_`, `MI_`, `MF_` | Material, instance, function | `MPC_` | Material Parameter Collection |
| `T_` | Texture | `SM_`, `SK_` | Static mesh, skeletal mesh |
| `NS_` | Niagara System | `L_` | Level |
| `IA_`, `IMC_` | Input Action, Input Mapping Context | `MSS_` | MetaSound Source |
| `SMX_`, `SEP_` | Sound Submix, Submix Effect Preset | `A_` | Sound Wave |
| `ABP_`, `AM_`, `BS_` | Animation Blueprint, Montage, Blend Space | `PCG_` | PCG Graph |

## Imported assets

Record where an imported asset came from. Set `Source` and `License` metadata on the asset at
import, or keep a table in `docs/design/` that lists each pack with its source and license. An
asset with no recorded license does not ship.

Source art that is not ready to import stays out of `Content/`. `SourceArt/Incoming/` is
gitignored for that purpose.

## Config

`Config/Default*.ini` files are plain text and the editor reads most of them at startup, so a
config change needs an editor restart to take effect. When an agent changes a setting:

- Add a comment above it: why, and which stage or decision it belongs to.
- Prefer the ini file over clicking through Project Settings, so the change shows up in review.
- Renderer and platform changes can trigger a long shader compile on the next editor launch;
  say so in the results.

## Logging and errors

- One log category per feature (`LogMyGameCombat`), declared in the feature's header.
- Fail fast with a message that names the object and the value. Never swallow an error.
- `ensure` for conditions that should never happen but can be survived; `check` for those that
  cannot. Neither replaces handling bad data from disk or from the player.
