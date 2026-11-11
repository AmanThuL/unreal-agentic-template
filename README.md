# Unreal agentic template

A starting point for building a 3D game in Unreal Engine 5.8 with AI coding agents doing most of
the work and a person reviewing it. It is a small, buildable Unreal project plus the rules,
scripts and harness that let an agent plan, build, test, play and package the game on its own,
without getting in the way of the person at the machine.

It is not a game template. There is no character, level or art in it. It is the workflow around
the game.

## What you get

| Piece | What it is for |
|---|---|
| `AGENTS.md`, `CLAUDE.md` | The entry point every agent reads: 15 non-negotiable rules, the repo map, commands |
| `docs/guidelines/` | Eleven short guidelines: workflow, editor over MCP, the C++ loop, conventions, verification, agent runs, packaging, version control with Git or Diversion, docs, field notes |
| `.mcp.json` and config | Epic's MCP server starts with the editor, so agents can edit assets, levels and Blueprints |
| `Scripts/editor.sh` | Open and close the editor in the background, waiting for MCP |
| `Scripts/build-editor.sh`, `Scripts/run-tests.sh` | The C++ loop, with a test runner that cannot be fooled by a crashed run |
| `Scripts/package.sh` | Package for Windows, Linux and macOS |
| `Plugins/AgentRun` + `Scripts/run-agent.sh` | Scripted runs of the packaged game with virtual input, screenshots and frame benches, collected as evidence |
| `docs/plans/`, `docs/decisions/`, `docs/design/` | Templates for stage plans, results reports, decision records and the GDD |

## The idea

Agents fail at game work in a few predictable ways. The template is built around preventing them.

- They say it works when it does not. So there is an evidence ladder
  ([verification](docs/guidelines/05-verification.md)), and the scripts decide pass or fail, not
  a glance at a log.
- They test below the layer that is broken. So checks of menus and input send virtual events
  through Slate's own hit test in the packaged build, the way a real click arrives
  ([agent runs](docs/guidelines/06-agent-runs.md)).
- They take over the machine. So the editor and the game launch in the background, real input is
  ignored during a run, and nothing moves the real cursor or steals focus.
- They ask a question at every fork, or decide silently. So the workflow is: decide inside the
  stage's scope, then list every decision in a results doc the person can override
  ([agent workflow](docs/guidelines/01-agent-workflow.md)).
- They guess API and property names. So the rules say look it up, and say where.

## Requirements

- Unreal Engine 5.8 (launcher build) and the platform toolchain: Xcode on macOS, Visual Studio
  with the C++ game workload on Windows, the engine's clang toolchain on Linux.
- Git with Git LFS, or [Diversion](docs/guidelines/09-version-control-diversion.md).
- Bash. On Windows, use Git Bash.
- An agent that reads `AGENTS.md` and can talk to an MCP server over HTTP. Claude Code picks up
  `CLAUDE.md` and `.mcp.json` as they are.

## Quick start

1. Create your repository from this template ("Use this template" on GitHub), then clone it.
2. Rename the project. `Starfall` is an example; use a PascalCase name.

   ```sh
   git lfs install
   Scripts/init-project.sh Starfall
   ```

3. Build and test. Both must pass.

   ```sh
   Scripts/build-editor.sh
   Scripts/run-tests.sh
   ```

4. Check the agent-run path end to end.

   ```sh
   Scripts/package.sh
   Scripts/run-agent.sh Scripts/agent-runs/smoke.txt
   ```

5. Fill in the Project section of `AGENTS.md` and write `docs/design/GDD.md`.
6. Open the editor and start your agent in the repo.

   ```sh
   Scripts/editor.sh open
   ```

7. Ask the agent for a plan for the first milestone. It will write
   `docs/plans/<TOPIC>_PLAN.md` from the template; review that, then let it run a stage.

If the engine is not in the launcher's default location, set `UE_ENGINE_DIR` to its `Engine/`
folder.

## Using it in an existing project

You do not need the sample project. Copy what you want:

- `Plugins/AgentRun/` into your `Plugins/`, and enable `AgentRun` in your `.uproject`.
- `Scripts/`. They find the project from the `.uproject` file in the repo root.
- `AGENTS.md`, `CLAUDE.md`, `.mcp.json` and `docs/guidelines/`. Replace `MyGame` with your
  project's name and adjust the conventions that differ.
- From `MyGame.uproject`: the `ModelContextProtocol` and `AllToolsets` plugin entries. From
  `Config/`: `DefaultEditorPerProjectUserSettings.ini` and the asset manager rule in
  `DefaultGame.ini`.
- `.gitattributes`, and the agent tooling lines of `.gitignore`.

## Platform support

| | macOS | Windows | Linux |
|---|---|---|---|
| Build, tests, editor scripts | Verified (Apple Silicon, UE 5.8) | Written, not yet run | Written, not yet run |
| Packaging | Verified | Written, not yet run | Written, not yet run |
| Agent runs | Verified | Written, not yet run | Written, not yet run |

"Written, not yet run" means the code follows the engine's documented layout and its
AutomationTool source, but nobody has executed it on that platform. All platform differences
live in `Scripts/lib/common.sh` and a few `case` blocks, so they are quick to fix. See
[packaging](docs/guidelines/07-packaging.md) and [field notes](docs/guidelines/11-field-notes.md).

## Layout

```
MyGame.uproject         the sample project (rename with Scripts/init-project.sh)
AGENTS.md  CLAUDE.md    agent entry points
Config/                 project settings
Content/                assets (empty)
Source/MyGame/          the game module, with one smoke test
Plugins/AgentRun/       agent mode, virtual input, script runner, bench
Scripts/                editor, build, test, package and agent-run scripts
docs/                   guidelines, design, plans, decisions, captures
```

## Notes

- The Unreal MCP plugins are Experimental in 5.8. Tool names in the docs come from that version.
- `Scripts/link-ue-skills.sh` is optional. It links a community skill pack from a local clone,
  since that pack has no license and cannot be vendored.
- The engine itself is not part of this repository and is covered by Epic's own license.

## License

MIT No Attribution. See [LICENSE](LICENSE). Use it, change it, ship with it; no credit needed.
