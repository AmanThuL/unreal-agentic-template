# AgentRun plugin

Scripted runs of the game for AI agents and CI. The game is driven with virtual input that never
takes the real mouse, keyboard or window focus, so a person can keep working on the same machine
while a run is in progress.

The plugin has no dependency on the game module. To use it in another project, copy this folder
into that project's `Plugins/` and enable `AgentRun` in the `.uproject`.

## What it adds

| Piece | File | Role |
|---|---|---|
| Agent mode | `AgentMode.h` | With `-AgentRun`: no mouse capture or lock, real input dropped by a Slate pre-processor, rendering in the background, a virtual pointer |
| Virtual input | `AgentInput.h` | Pointer, wheel, key and axis events delivered to the widgets under the virtual pointer |
| Script runner | `AgentRunSubsystem.h` | A game instance subsystem that plays a script one step per line |
| Script parser | `AgentScript.h` | Turns a line into a step; pure, tested |
| Bench math | `AgentBench.h` | Average, 1% low, slowest frame, hitch count; pure, tested |

In Shipping builds agent mode is always off, the subsystem is not created and the console
commands do not exist.

## Starting a run

From the shell, `Scripts/run-agent.sh <script.txt>` launches the packaged
game with the right arguments and collects the evidence. By hand:

```
<Game> -AgentRun -AgentScript=smoke.txt -windowed -ResX=1920 -ResY=1080
```

`-AgentScript` paths are relative to the game's `Saved/AgentRuns/`. Without `-AgentRun` a script
still plays, but the game behaves as a normal build (it may capture the mouse).

From the in-game console, in any non-Shipping build including PIE:

```
Agent.Run mycheck.txt
Agent.Step click 0.5 0.4
Agent.Stop
```

## Script steps

One step per line; `#` starts a comment; positions are fractions of the game viewport (0 to 1,
origin top left).

| Step | Does |
|---|---|
| `log <text>` | Writes `AgentRun: <text>` to the log |
| `wait <seconds>` | Waits in real time (default 1), also while paused |
| `mouse <x> <y>` | Moves the virtual pointer |
| `click <x> <y> [hold]` | Moves, presses the left button, releases after `hold` seconds (default 0.1) |
| `wheel <delta> [x y]` | Turns the wheel at a position (default the center) |
| `key <Key> [hold]` | Presses a key by engine name, releases after `hold` seconds (default 0.1) |
| `axis <Key> <value> [seconds]` | Sends an axis sample each frame for a duration (default one frame) |
| `goto <x> <y> <z> [yaw] [pitch]` | Puts the player's eyes at a world position in cm |
| `bench <seconds> [label]` | Settles 1 s, records frame times, appends a row to `bench.csv` |
| `shot <name>` | Saves `Shots/<name>.png` with the UI |
| `selftest` | Adds a Slate button, clicks it with virtual input, fails the step if it was not pressed |
| `quit` | Logs the summary, then exits |
| anything else | A console command |

Outputs go to the game's `Saved/AgentRuns/`. When a script ends the log has
`AgentRun: script done, <N> steps, <F> failed`; a failed step logs `AgentRun: FAILED <reason>`.

## How virtual input is delivered

`AgentInput` finds the widgets under the virtual pointer with Slate's own hit test
(`LocateWindowUnderMouse`) and calls their event handlers directly, bubbling from the leaf to the
root as Slate does:

- Move: `OnMouseLeave` for widgets no longer under the pointer, `OnMouseEnter` for those now
  under it, then `OnMouseMove`.
- Press: `OnPreviewMouseButtonDown` from the root down, then `OnMouseButtonDown` from the leaf
  up. If a menu widget's reply asks for mouse capture or keyboard focus, that is applied inside
  Slate only. The game viewport's reply (which asks for OS focus, capture and cursor moves) is
  dropped.
- Release: to the capturing widget if there is one, otherwise to the widgets under the pointer.
- Keys: to the focused widget and its parents, then to the game viewport.
- Axis: to the game viewport client.

It does not use `FSlateApplication::ProcessMouseButtonDownEvent` and friends, because that
routing can activate the window and bring it to the front.

## Reading the virtual pointer in game code

Widgets that handle pointer events need nothing. Code that polls the OS cursor (a HUD drawn on
the canvas, a custom cursor, a world-space pick under the mouse) should branch:

```cpp
FVector2D Position;
const bool bHavePosition = AgentMode::IsActive()
	? AgentMode::GetPointer(Position)
	: PlayerController->GetMousePosition(Position.X, Position.Y);
```

## Extending it

- A project cheat a script can call: add a console command or an `Exec` function that logs its
  result; any line that is not a built-in step runs as a console command.
- A new built-in step: add it to `EAgentStep` and the table in `AgentScript.cpp`, handle it in
  `UAgentRunSubsystem::Execute`, and add a parse test.
- More input (right button, drag, text): extend `AgentInput` and keep the rule that nothing goes
  through the OS.

## Tests

`Scripts/run-tests.sh AgentRun` runs the plugin's automation tests (mode, parser, bench math).
What needs a rendered frame is covered by the `selftest` step in `Scripts/agent-runs/smoke.txt`.

## Limits

One left mouse button; no double click, drag and drop or text typing. Verified on macOS with
UE 5.8; the code is platform-neutral but has not been run on Windows or Linux.
