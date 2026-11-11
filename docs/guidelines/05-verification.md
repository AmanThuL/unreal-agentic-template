# 05. Verification

What counts as evidence that a change works, and which check a change needs.

## Rules at a glance

1. Do not report a change as working until a check has passed. Name the check and its result.
2. If a check failed, was skipped or could not be run, say so plainly. A skipped check is a fact
   the reviewer needs, not a detail to leave out.
3. Use the check that matches the claim (table below). Unit tests do not prove that a menu is
   clickable; an editor bench does not prove the frame rate players get.
4. Verify input and UI with events that enter at the Slate layer, in the packaged build.
5. Measure performance in the packaged build, at the target resolution, and report the 1% low
   and the hitches as well as the average.
6. Look at every screenshot you take. A capture that nobody examined is not a check.
7. Things only a person can judge (feel, fun, comfort, taste) are reported as pending the person.

## The evidence ladder

Each rung proves more and costs more. Climb as far as the change needs.

| Rung | Proves | How |
|---|---|---|
| 1. Build | The code compiles and links, no new warnings | `Scripts/build-editor.sh` |
| 2. Automation tests | Logic is correct in isolation | `Scripts/run-tests.sh` |
| 3. Editor log | Assets load, Blueprints compile, nothing new complains | `LogsToolset` `GetLogEntries`, pattern `Error|Warning` |
| 4. Viewport capture | The level looks right from a known camera | `EditorAppToolset` `CaptureViewport`, then read the image |
| 5. Agent run in the packaged build | The shipped game behaves: menus respond to clicks, the flow completes, frame rate holds | `Scripts/run-agent.sh` |
| 6. Person plays | Feel, fun, readability, audio mix | The person, with a short guide of what to try |

## Which check for which change

| Change | Minimum evidence |
|---|---|
| Pure logic in C++ | Rungs 1 and 2 |
| Gameplay behavior in a level | Rungs 1 to 3, plus an agent run that exercises it |
| Menu, HUD, input binding | Rungs 1 to 3, plus an agent run that clicks or presses through it with virtual input |
| Visual change (material, lighting, effects) | Capture before and after from the same camera; a packaged shot if the change is platform-sensitive |
| Performance-sensitive change (content, rendering settings, per-frame code) | Packaged bench before and after at the same spots |
| Save data change | A test that loads an old save, plus a packaged run that saves, quits and continues |
| Config change | Editor restart, log check, and whichever rung the setting affects |

## Why the packaged build

Three failures this workflow is built to prevent:

- A check that injects input below the layer that is broken will pass while the real thing
  fails. A menu walkthrough that injected into the input system reported success while real
  mouse clicks did nothing, because the bug was in the layer the injection skipped. Virtual input
  for checks therefore enters at the Slate widgets, through the same hit test a real click uses
  ([06](06-agent-runs.md)).
- The editor is not the game. Editor benches run at different settings, with different shader
  and streaming behavior, and have differed from the packaged game by a wide margin in both
  directions. Frame-rate claims come from the packaged build.
- PIE shares the editor's window, focus and input state. It is fine for watching logic, and it
  is not evidence for capture, focus or input behavior.

## Performance checks

- State the budget in `AGENTS.md` (for example 60 fps at the target resolution, 1% low above 50).
- Bench at fixed spots: the worst views in the game, not the nicest. Add a spot when a new area
  or effect arrives. `goto` plus `bench` in an agent script makes the spots repeatable.
- Run at the resolution and window mode players use (`AGENT_RES`).
- Compare like with like: same spots, same build configuration, same machine state. Thermal
  throttling after several minutes is real; a long soak run finds it.
- Report average fps, the 1% low, the slowest frame and the hitch count per spot. A spot under
  the floor is a failing check even when the average looks fine.
- If the OS throttles a background window, measure it and say so. Do not bring the game to the
  front to get better numbers.

## Visual checks

- Name the two or three views that define the game's look in the GDD, and capture those every
  time rendering changes.
- Keep the camera positions in the agent script, so before and after are the same frame.
- Copy the captures a reviewer needs into `docs/captures/<stage>/`. The full set stays in
  `Saved/AgentRuns/` and is not committed.
- Describe what the image shows, including what is wrong with it. "Partial: the reef reads well,
  the surface is a flat glow with no shafts" is a useful result.

## Reporting

In the results doc, one row per success criterion: the result (pass, partial, fail, pending the
person) and the evidence (test names, a log line, a file in `docs/captures/`, a bench row).
Partial and fail rows say what is missing. See the template in `docs/plans/`.
