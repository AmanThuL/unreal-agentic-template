# AGENTS.md

Entry point for every AI coding agent (Claude Code, Codex, Cursor and others) and every person
working on this project. `CLAUDE.md` imports this file, so shared rules belong here. Read this
page, then open the guideline that covers the task before changing anything.

## Project

Fill this section in when you start from the template. Agents read it first.

- Game: `<one or two sentences: genre, camera, core loop, single or multiplayer>`
- Design source of truth: [docs/design/GDD.md](docs/design/GDD.md). Read it before making design
  or architecture decisions. Its "Out of scope" list applies to all work.
- Current target: `<milestone name and the plan that tracks it in docs/plans/>`
- Performance budget: `<for example 60 fps at 1920 x 1080 on the target machine, 1% low above 50>`

## Environment

| Item | Value |
|---|---|
| Engine | Unreal Engine 5.8 (launcher build). Set `UE_ENGINE_DIR` if it is not in the default place. |
| Engine source | `<engine>/Engine/Source`. Search it to confirm an API before using it. |
| Project file | `MyGame.uproject` |
| Code | C++ module `Source/MyGame` plus Blueprints. Targets `MyGame` and `MyGameEditor`. |
| Agent harness | Plugin `Plugins/AgentRun` (scripted runs of the packaged game). |
| Editor access | Epic's MCP server inside the editor, `http://127.0.0.1:8000/mcp` (see `.mcp.json`). |
| Version control | Git with Git LFS. Change this row if the project uses Diversion. |

## Non-negotiables

The rules agents break most often. Each one links to the guideline with the detail and the reason.

1. Never take the person's mouse, keyboard or window focus. Launch the editor and the game in the
   background with the scripts, and drive the game with virtual input only.
   [06](docs/guidelines/06-agent-runs.md)
2. Do not report a change as working until a check has passed. Name the check. If a check failed
   or was skipped, say so. [05](docs/guidelines/05-verification.md)
3. Verify through the path the player uses. Input and UI are checked with events that enter at
   the Slate layer, in the packaged build. Performance is measured in the packaged build at the
   target resolution, never in the editor. [05](docs/guidelines/05-verification.md)
4. Never edit `.uasset` or `.umap` files as text or with shell tools. Change assets through the
   editor over MCP. [02](docs/guidelines/02-editor-and-mcp.md)
5. Look names up, do not guess them. Read tool schemas with `describe_toolset`, list an object's
   properties before setting one, and grep the engine source for C++ APIs.
   [02](docs/guidelines/02-editor-and-mcp.md)
6. Stop PIE before editing assets, and save explicitly. Nothing persists until it is saved.
   [02](docs/guidelines/02-editor-and-mcp.md)
7. A C++ change means: save over MCP, close the editor, build, run the tests, reopen the editor.
   Use the scripts. Never kill the editor process. [03](docs/guidelines/03-cpp-build-test-loop.md)
8. Keep game logic testable without a world, and name tests `MyGame.<Area>.<Case>`. Never disable
   a failing test to get a green run. [03](docs/guidelines/03-cpp-build-test-loop.md)
9. C++ owns systems and math; Blueprints own flow, visuals and tuning. Tuning values live in Data
   Assets or `EditDefaultsOnly` properties, not in code.
   [04](docs/guidelines/04-project-conventions.md)
10. Project assets live under `/Game/MyGame/<Feature>/` with the asset name prefixes. Input uses
    Enhanced Input only. [04](docs/guidelines/04-project-conventions.md)
11. Before deleting or moving an asset, check its referencers and confirm with the person.
    [02](docs/guidelines/02-editor-and-mcp.md)
12. Commit only when the person asks. Never commit `Binaries/`, `Intermediate/`, `Saved/` or
    `DerivedDataCache/`. With Git, binary files go through Git LFS
    ([08](docs/guidelines/08-version-control.md)). With Diversion, follow
    [09](docs/guidelines/09-version-control-diversion.md).
13. Work in stages with a written plan, and close each stage with a results doc that lists the
    evidence and every decision you made on the person's behalf.
    [01](docs/guidelines/01-agent-workflow.md)
14. After three failed attempts at the same problem, stop. Write down what you tried and why it
    failed, then change the approach or ask. [01](docs/guidelines/01-agent-workflow.md)
15. A Markdown doc has at most 300 lines. Split by topic and link the parts.
    [10](docs/guidelines/10-docs-and-plans.md)

## Where things are

```
MyGame.uproject
AGENTS.md / CLAUDE.md      this file (CLAUDE.md imports it)
.mcp.json                  points agents at the editor's MCP server
Config/                    project settings (Default*.ini), plain text, read at editor startup
Content/                   assets (.uasset, .umap), binary, edited only through the editor
Source/MyGame/             the game's C++ module; tests in Private/Tests/
Plugins/AgentRun/          agent mode, virtual input, script runner, bench (has its own README)
Scripts/                   editor.sh, build-editor.sh, run-tests.sh, package.sh, run-agent.sh,
                           check-docs.sh, link-ue-skills.sh, agent-runs/*.txt
docs/
  guidelines/              the eleven guidelines indexed below
  design/                  the GDD and design notes
  plans/                   stage plans and results (templates included)
  decisions/               one short record per engineering decision
  captures/                screenshots kept as evidence for review
```

Generated, never committed: `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/`.

## Guidelines index

Each guideline opens with a short list of rules that is enough for most tasks.

| # | Read when you | Guideline |
|---|---|---|
| 01 | start a task, plan a stage, or report results | [Agent workflow](docs/guidelines/01-agent-workflow.md) |
| 02 | touch anything in the editor: assets, levels, Blueprints, materials | [Editor and MCP](docs/guidelines/02-editor-and-mcp.md) |
| 03 | write or change C++, or run tests | [C++ build and test loop](docs/guidelines/03-cpp-build-test-loop.md) |
| 04 | add a class, an asset, an input or a setting | [Project conventions](docs/guidelines/04-project-conventions.md) |
| 05 | are about to say something works | [Verification](docs/guidelines/05-verification.md) |
| 06 | need to play, click through, capture or bench the game | [Agent runs](docs/guidelines/06-agent-runs.md) |
| 07 | package the game for Windows, Linux or macOS | [Packaging](docs/guidelines/07-packaging.md) |
| 08 | commit, branch, or add a binary file type | [Version control](docs/guidelines/08-version-control.md) |
| 09 | use Diversion instead of Git for version control | [Version control with Diversion](docs/guidelines/09-version-control-diversion.md) |
| 10 | write a doc, a plan, a results report or a decision record | [Docs and plans](docs/guidelines/10-docs-and-plans.md) |
| 11 | hit something odd in the toolchain | [Field notes](docs/guidelines/11-field-notes.md) |

## Commands

Run from the repo root. All scripts find the project from the `.uproject` file.

```sh
Scripts/editor.sh status|open|close    # the editor, in the background; open waits for MCP
Scripts/build-editor.sh                # build the editor target (editor must be closed)
Scripts/run-tests.sh [filter]          # headless automation tests; default MyGame+AgentRun
Scripts/package.sh [--platform P]      # package for Mac, Win64 or Linux into Saved/Packaged/
Scripts/run-agent.sh <script.txt>      # play a script in the packaged game, collect evidence
Scripts/check-docs.sh                  # doc length and link check
```

The C++ loop in one line, once assets are saved over MCP:

```sh
Scripts/editor.sh close && Scripts/build-editor.sh && Scripts/run-tests.sh && Scripts/editor.sh open
```

## Skills

Agents get Unreal knowledge from three places:

1. Editor skills over MCP (`ListSkills`, `GetSkills` in `ToolsetRegistry.AgentSkillToolset`): how
   to use the editor's own toolsets. Load the relevant ones before Blueprint, material, Niagara,
   PCG or lighting work. Never call `CreateSkill` or `UpdateSkill` without the person's permission.
2. Optional `ue-*` skills in `.claude/skills/`, linked from a local clone by
   `Scripts/link-ue-skills.sh`: Unreal C++ and engine API knowledge. Where one disagrees with this
   file, follow this file.
3. This file and `docs/guidelines/`: project rules. Project-specific skills go in
   `.claude/skills/<project>-<topic>/` and are committed.

## Machine setup

Once per machine, for people and for agents that find something missing:

1. Install Unreal Engine 5.8 from the Epic Games Launcher, plus the platform toolchain (Xcode on
   macOS, Visual Studio with the C++ game workload on Windows).
2. Run `git lfs install`, then `git lfs pull` if binary files show up as small pointer files.
3. Run `Scripts/build-editor.sh`, then `Scripts/run-tests.sh`. Both must pass before real work.
4. Run `Scripts/editor.sh open` and connect your agent to the MCP server in `.mcp.json`.
5. Optional: `Scripts/link-ue-skills.sh` for the `ue-*` skills.

An agent that finds one of these missing tells the person instead of working around it.
