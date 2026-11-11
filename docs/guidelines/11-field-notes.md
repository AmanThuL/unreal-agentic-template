# 11. Field notes

Things that cost time to find out. Each note says where it was observed. Add to this file when
the toolchain surprises you, and remove a note when an engine upgrade makes it untrue.

Observed on macOS (Apple Silicon) with UE 5.8 unless a note says otherwise.

## Rules at a glance

1. Check here before debugging a toolchain oddity.
2. When you find a new one, add a note: what happened, where it was seen, what to do.
3. After an engine upgrade, run the smoke agent run and re-check the notes that mention engine
   behavior.

## Editor and MCP

- The MCP plugins (`ModelContextProtocol`, `AllToolsets`) are Experimental. Tool names and
  schemas can change between engine versions; trust `describe_toolset` over any doc.
- `call_tool` takes the full toolset name and the short tool name. The full dotted name as
  `tool_name` fails with `Unknown tool`. Inside `execute_tool_script`, `execute_tool` takes the
  full dotted name.
- Setting a property with a wrong name can fail silently. List properties first.
- The MCP toolsets enable the GameFeatures plugin, which logs an error and can fail a cook
  unless the asset manager has a `GameFeatureData` rule. `Config/DefaultGame.ini` carries one.
- A crashed headless run can leave the crash reporter holding the MCP port. `Scripts/editor.sh
  open` reports which process owns the port instead of waiting for a timeout.
- The first editor launch after a renderer or engine change compiles shaders for many minutes.
  `Scripts/editor.sh open` waits up to 15 minutes by default.
- Closing the editor with unsaved assets shows a save prompt, and the graceful close then times
  out. Save over MCP first.

## Build and tests

- Live Coding is Windows-only; the engine's Live Coding toolset compiles out on macOS. C++
  changes need the close, build, reopen loop.
- On macOS a plain `-log` writes under `~/Library/Logs/Unreal Engine/`, not the project folder.
  The scripts pass `-abslog` to put the log where they can read it.
- The exit code of a headless test run is not enough. A crash mid-run can leave a log with only
  passing results. `Scripts/run-tests.sh` compares the number of results with the number of tests
  found.
- Headless runs use `-nullrhi`: no rendering, no game viewport. Tests that need either belong in
  an agent run.
- New target files use `BuildSettingsVersion.V7` and `EngineIncludeOrderVersion.Unreal5_8`. Bump
  both when upgrading the engine, then fix what the new include order surfaces.

## Packaged builds

- On macOS the `BuildCookRun` archive step can copy the `.app` without `Contents/UE` (the paks
  and dylibs), which leaves an app that cannot start. `Scripts/package.sh` copies the complete
  staged app when it sees this.
- A packaged macOS app is sandboxed. Its `Saved/` folder, saves and logs live under
  `~/Library/Containers/<bundle id>/Data/Library/`, and it cannot read files outside the
  container. `Scripts/run-agent.sh` copies scripts in and evidence out.
- `BuildCookRun` can exit 0 with nothing to launch. `Scripts/package.sh` checks for the game.
- Performance in the editor and in the packaged game can differ a lot, in either direction.
  Only packaged numbers count.

## Input and focus

- Synthetic OS-level mouse clicks may not reach a packaged game at all, while synthetic key
  presses and mouse moves do. Do not build checks on OS-level input; it also takes the person's
  cursor.
- Routing virtual clicks through `FSlateApplication::ProcessMouseButtonDownEvent` can activate
  the window and bring it to the front. The AgentRun plugin calls the widgets directly instead.
- A Slate button with the default click method fires on release only if it is hovered and holds
  mouse capture. A virtual click therefore has to send hover enter events and honor the capture
  request from the press. `selftest` checks exactly this.
- Injecting into the input system (Enhanced Input, or a player controller) skips Slate. It is
  useful for steering a character and useless as evidence that menus or bindings work.
- `GameGetsMouseControl=False` in the editor's play settings keeps PIE from capturing the mouse
  when it starts.

## Running in the background

- A game in the background may be throttled by the OS, and may mute or duck its audio. Agent
  mode sets `t.IdleWhenNotForeground` to 0 so the engine keeps rendering; OS-level throttling,
  if any, should be measured and reported, not avoided by taking focus.
- A packaged game has been seen to stall at startup while the screen is locked on one project
  and to run normally on another. If a run times out with a locked screen, try again unlocked
  before debugging the script.
- The frontmost-application check in `Scripts/run-agent.sh` proves nothing when the screen is
  locked, since no application is in front.

## Not yet exercised

- The Windows and Linux branches of `Scripts/lib/common.sh`, `Scripts/editor.sh`,
  `Scripts/package.sh` and `Scripts/run-agent.sh`. They follow the engine's documented layout.
  On Windows the scripts expect Git Bash. Update this note and [07](07-packaging.md) when
  someone has run them.
