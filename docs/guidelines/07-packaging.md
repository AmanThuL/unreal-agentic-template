# 07. Packaging

How the game is built into something a player can run, on Windows, Linux and macOS.

## Rules at a glance

1. Package with `Scripts/package.sh`, with the editor closed. Do not package from the editor UI:
   the script's settings are the ones under review.
2. A packaging run passes only when the script exits 0. It checks that there is something to
   launch, since the engine's tool can report success without one.
3. Package on a host that can build the target (table below). macOS builds need a Mac; Windows
   builds need Windows.
4. Use Development for agent runs and day-to-day checks, Shipping for builds that go to players.
   The AgentRun plugin does nothing in Shipping.
5. After packaging a target you can run, run the smoke agent run before reporting the build as
   good.
6. Packaged builds are never committed. They land in `Saved/Packaged/`, which is ignored.

## The command

```sh
Scripts/package.sh                                        # this machine's platform, Development
Scripts/package.sh --platform Win64 --config Shipping
Scripts/package.sh --platform Linux
Scripts/package.sh --platform Mac -map=/Game/MyGame/Maps/L_Main   # extra BuildCookRun arguments
```

It runs the engine's `BuildCookRun` (build, cook, stage, pak, archive) and writes
`Saved/Logs/Package-<platform>.log`.

| Target | Output | Launch |
|---|---|---|
| `Mac` | `Saved/Packaged/Mac/MyGame.app` | `open Saved/Packaged/Mac/MyGame.app` |
| `Win64` | `Saved/Packaged/Windows/MyGame.exe` | run the `.exe` |
| `Linux` | `Saved/Packaged/Linux/MyGame.sh` | run the `.sh` |
| `LinuxArm64` | `Saved/Packaged/LinuxArm64/MyGame-Arm64.sh` | run the `.sh` |

## Which host builds which target

This is an engine limit. The script fails early with the reason when the host cannot build the
target.

| Host | Can package |
|---|---|
| macOS | `Mac` |
| Windows | `Win64`; `Linux` and `LinuxArm64` with the Linux target platform installed for the engine and Epic's Linux cross-compile toolchain (it sets `LINUX_MULTIARCH_ROOT`) |
| Linux | `Linux`, `LinuxArm64` |

To ship all three desktop platforms you need two machines at minimum: a Mac and a Windows
machine with the Linux toolchain. Three machines (or three CI runners) is the simpler setup.

## Status of each platform in this template

| Target | Status |
|---|---|
| `Mac` on macOS (Apple Silicon), UE 5.8 | Verified: package, launch and smoke agent run |
| `Win64` on Windows | Written from the engine's documented layout and its AutomationTool source; not yet run |
| `Linux` on Linux or Windows | Same as Win64 |

The first time you package on Windows or Linux, treat it as a check of the script as well as of
the game: run it, run the smoke agent run, and fix `Scripts/lib/common.sh` if a path differs.
Then update this table.

## Building all three in CI

Unreal builds need the engine installed, so they run on self-hosted runners. A starting point
for GitHub Actions, to adapt and test before relying on it:

```yaml
name: package
on: workflow_dispatch
jobs:
  package:
    strategy:
      fail-fast: false
      matrix:
        include:
          - { runner: [self-hosted, macOS], platform: Mac }
          - { runner: [self-hosted, Windows], platform: Win64 }
          - { runner: [self-hosted, Linux], platform: Linux }
    runs-on: ${{ matrix.runner }}
    steps:
      - uses: actions/checkout@v4
        with: { lfs: true }
      - run: Scripts/build-editor.sh
        shell: bash
      - run: Scripts/run-tests.sh
        shell: bash
      - run: Scripts/package.sh --platform ${{ matrix.platform }} --config Shipping
        shell: bash
      - uses: actions/upload-artifact@v4
        with:
          name: MyGame-${{ matrix.platform }}
          path: Saved/Packaged/
```

Set `UE_ENGINE_DIR` on each runner if the engine is not in the launcher's default location.

## What goes into the build

- Maps: by default the cook follows the maps named in `Config/DefaultEngine.ini`
  (`GameDefaultMap`) and what they reference. List extra maps under "List of maps to include in a
  packaged build" in Project Settings, or pass `-map=` to the script.
- Plugins: an enabled editor-only plugin can add content or checks to the cook. The asset manager
  rule in `Config/DefaultGame.ini` exists because the MCP toolsets enable GameFeatures, which
  fails a cook without it.
- Shipping builds drop console commands, cheats and the AgentRun plugin's behavior. Test the
  Shipping build by hand, or with a Test configuration build if you need the console.

## Platform notes

- macOS: packaged apps are sandboxed, so the game reads and writes under
  `~/Library/Containers/<bundle id>/Data/`, including saves and logs. Set a real bundle
  identifier and signing identity in Project Settings before distributing. The script works
  around an archive step that can copy the `.app` without its `Contents/UE` folder.
- Windows: Development builds keep `Saved/` next to the game. Shipping builds write to the
  user's local application data folder.
- Linux: the `.sh` launcher starts the binary under `MyGame/Binaries/Linux/`. Shipping to players
  usually also means choosing between Vulkan-only and a compatibility path; record that choice
  as a decision.
- Before distributing on any platform: set the project name, company, version, icons and splash
  in Project Settings, and package with `--config Shipping`.
