# 06. Agent runs

How an agent plays the packaged game: scripted, in the background, with virtual input, without
disturbing the person at the machine. The engine side is the `AgentRun` plugin
([Plugins/AgentRun/README.md](../../Plugins/AgentRun/README.md)); the shell side is
`Scripts/run-agent.sh`.

## Rules at a glance

1. Never take the person's mouse, keyboard or window focus. No OS-level input (CGEvent,
   AppleScript, SendInput, xdotool), no cursor warps, no bringing the game to the front.
2. Run the packaged game through `Scripts/run-agent.sh`. It starts the game in the background
   with `-AgentRun`.
3. Drive the game with script steps only: `mouse`, `click`, `wheel`, `key`, `axis`.
4. End every script with `quit`. A run that has to be timed out is a failed run.
5. Back up the player's save data before a run that starts a new game or writes saves, and
   restore it afterwards.
6. A run passes only when `run-agent.sh` exits 0. Read the evidence it collected before you
   report.
7. If a check truly needs the real cursor or the game in front, ask the person first.

## What agent mode does

With `-AgentRun` on the command line (never in the editor, never in Shipping builds):

- The viewport does not capture or lock the mouse and does not hide the cursor.
- A Slate input pre-processor drops every real key, mouse and gamepad event, so the person's own
  typing and clicking cannot change the run, and the run's state cannot leak into their work.
- The game keeps rendering while another application is in front.
- A virtual pointer replaces the OS cursor. Game code that reads the cursor position itself
  should read `AgentMode::GetPointer` in agent runs.

Virtual input is delivered straight to the widgets under the virtual pointer, found with Slate's
own hit test. UMG and Slate menus and the game viewport receive it as they would real input, and
nothing passes through the OS or through the parts of Slate that activate windows.

## Running a script

```sh
Scripts/package.sh                                   # once per code or content change
Scripts/run-agent.sh Scripts/agent-runs/smoke.txt    # default timeout 600 s
AGENT_RES=2560x1440 Scripts/run-agent.sh Scripts/agent-runs/bench_main.txt 900
```

The script file is copied into the packaged game's own `Saved/AgentRuns/` folder (on macOS that
is inside the app's sandbox container). When the game exits, the evidence is copied to the
project's `Saved/AgentRuns/<script name>/`:

- `<script name>.log`: the game log. Every line the plugin writes starts with `AgentRun:`.
- `bench.csv`: one row per `bench` step.
- `Shots/<name>.png`: one file per `shot` step.

`run-agent.sh` exits 0 only if the log has `AgentRun: script done, N steps, 0 failed` and the
game quit by itself before the timeout. On macOS it also warns if the game was ever the
frontmost application during the run.

## Script steps

One step per line. `#` starts a comment. Positions are fractions of the game viewport, origin
top left.

| Step | Does |
|---|---|
| `log <text>` | Writes a marker to the log |
| `wait <seconds>` | Waits in real time, also while the game is paused |
| `mouse <x> <y>` | Moves the virtual pointer (hover) |
| `click <x> <y> [hold]` | Moves, presses the left button, releases after `hold` seconds (0.1) |
| `wheel <delta> [x y]` | Turns the wheel by `delta` notches at a position (default center) |
| `key <Key> [hold]` | Presses a key by its engine name (`SpaceBar`, `Escape`, `W`), releases after `hold` |
| `axis <Key> <value> [seconds]` | Sends an axis sample every frame (`MouseX`, `Gamepad_LeftY`) |
| `goto <x> <y> <z> [yaw] [pitch]` | Puts the player's eyes at a world position in cm (a cheat; for captures and benches) |
| `bench <seconds> [label]` | Settles one second, then records frame times |
| `shot <name>` | Saves a screenshot with the UI |
| `selftest` | Shows a Slate button and clicks it with virtual input; fails the run if the click does not land |
| `quit` | Logs the summary and exits the game |
| anything else | Runs as a console command, including your own cheat commands |

With the game running, the same steps work from the console: `Agent.Step click 0.5 0.4`,
`Agent.Run mycheck.txt`, `Agent.Stop`.

## Writing good scripts

- Keep them in `Scripts/agent-runs/`, named for what they check. Start each with a comment that
  says what it proves and what state it needs (a fresh save, a particular level).
- `log` before each phase. The log is how you, and the reviewer, find out where a run went wrong.
- Use real input for the thing under test and cheats for the setup. To check a menu, click it.
  To bench a cave, `goto` it.
- Give the game time: `wait` after a click that loads or animates, and before a `shot`.
- Menu positions depend on resolution and layout. Record the resolution the positions were
  measured at in the script's header, and run with that `AGENT_RES`.
- Add project cheats as console commands (an `Exec` function on a cheat manager, or an
  `FAutoConsoleCommand`) that print their result to the log, so a script can set up state and a
  reader can see what happened.
- Prefer a state dump over a screenshot when a fact can be logged. "Menu page: Settings,
  hovered row 2" in the log is cheaper to check than a picture.

## Limits

- One left mouse button; no right or middle button, double click, drag and drop, or text typing.
  Extend `AgentInput` when a check needs one.
- A widget that polls the OS cursor instead of handling pointer events will not see the virtual
  pointer unless it reads `AgentMode::GetPointer`.
- Audio: some platforms mute or duck a background application. Judge the audio mix with the
  person, or capture the game's own audio output with a tool that does not need focus.
- Verified on macOS with UE 5.8. On Windows and Linux the plugin code is platform-neutral, but
  the launch and path handling in `run-agent.sh` has not been exercised; see
  [11](11-field-notes.md).

## The smoke run

`Scripts/agent-runs/smoke.txt` runs `selftest`, takes a shot and benches the start map. Run it
after an engine upgrade and after any change to the plugin. If `selftest` fails, virtual clicks
are not reaching Slate widgets on this engine version, and no UI check from an agent run can be
trusted until it is fixed.
