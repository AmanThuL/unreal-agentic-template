# Documentation index

Start with [AGENTS.md](../AGENTS.md). It holds the non-negotiable rules and links here.

## Guidelines

How to work in this repo. Each opens with a short list of rules.

| # | Guideline | Owns |
|---|---|---|
| 01 | [Agent workflow](guidelines/01-agent-workflow.md) | Stages, plans, deciding and reporting, subagent briefs, the three-attempt rule |
| 02 | [Editor and MCP](guidelines/02-editor-and-mcp.md) | The editor's MCP server, its three tools, saving, PIE, editor skills |
| 03 | [C++ build and test loop](guidelines/03-cpp-build-test-loop.md) | Close, build, test, reopen; automation tests; checking engine APIs |
| 04 | [Project conventions](guidelines/04-project-conventions.md) | C++ and Blueprint split, source and content layout, asset prefixes, config |
| 05 | [Verification](guidelines/05-verification.md) | The evidence ladder; which check a change needs; performance and visual checks |
| 06 | [Agent runs](guidelines/06-agent-runs.md) | Scripted runs of the packaged game with virtual input; script steps |
| 07 | [Packaging](guidelines/07-packaging.md) | Packaging for Windows, Linux and macOS; hosts, outputs, CI |
| 08 | [Version control](guidelines/08-version-control.md) | Commits, Git LFS, binary assets, branches |
| 09 | [Version control with Diversion](guidelines/09-version-control-diversion.md) | The same rules with Diversion: workspaces, cloud sync, locks, the `dv` commands |
| 10 | [Docs and plans](guidelines/10-docs-and-plans.md) | Doc rules, plan and results templates, decision records |
| 11 | [Field notes](guidelines/11-field-notes.md) | Toolchain surprises and what to do about them |

## Working docs

| Folder | Holds |
|---|---|
| [design/](design/GDD.md) | The game design document and per-feature design notes |
| [plans/](plans/PLAN_TEMPLATE.md) | Stage plans and results, with templates |
| [decisions/](decisions/0000-template.md) | One record per engineering decision |
| `captures/` | Evidence images referenced by results docs |

## Other

- [Plugins/AgentRun/README.md](../Plugins/AgentRun/README.md): the agent harness plugin.
- [README.md](../README.md): what the template is and how to start from it.
