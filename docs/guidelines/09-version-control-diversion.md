# 09. Version control with Diversion

Use this guideline instead of the Git commands in [08](08-version-control.md) when the project
is versioned with [Diversion](https://docs.diversion.dev/), a cloud version control system built
for game projects with large binary assets. The principles in 08 still hold: commit only when
asked, never commit generated folders, one owner per binary asset.

Status: written from Diversion's documentation as read on 2026-11-11. Nothing here has been run
against this template. Confirm syntax with `dv help <command>` before relying on it, and correct
this file when the tool disagrees.

## Rules at a glance

1. Pick one system as the source of truth for the project and write it in `AGENTS.md`. Do not
   commit the same work through Git and Diversion by turns.
2. Always pass a subcommand. A bare `dv` opens an interactive shell and hangs an agent.
3. Everything written into the workspace is uploaded to the cloud before any commit, counts
   toward storage, and is visible to teammates. Check the ignore rules before generating files,
   and never put secrets in the workspace.
4. Commit only when the person asks. A commit is shared with everyone on the branch at once;
   there is no local-only commit and no push step to hold it back.
5. Do stage work on a branch, not on the main branch.
6. Each agent gets its own workspace on its own branch. Two agents on one branch sync into each
   other.
7. Before editing a binary asset, check whether someone else has it locked or changed. Lock it
   if the plan supports hard locks, and release the lock when the work is committed.
8. Never discard work without the person's consent: no `dv reset`, no `--discard-changes`, no
   `accept-incoming`, `keep-current` or `keep-destination` conflict strategy, no forced unlock.
9. After a checkout or update that changes assets, reload them in the editor or restart it
   before editing.
10. If you find `*.dv-conflict.*` files, stop and report them. They are backups of local work
    that a sync overwrote.

## How Diversion differs from Git

| Git | Diversion |
|---|---|
| Local commits, then `git push` | No push or pull. A commit replicates to the branch for everyone |
| Uncommitted work exists only on your disk | Uncommitted work is synced to your cloud workspace as you save |
| `git worktree` for parallel work | A second workspace: a full clone on its own branch |
| Git LFS and `.gitattributes` for binaries | Binaries are handled natively; no LFS setup |
| No locking | Soft locks on every plan; hard locks on some plans |
| `.gitignore` | `.dvignore`, and `.gitignore` files are honored too |

## Setup

Once per machine, by a person:

```sh
brew install --cask diversion-app                       # macOS desktop app
curl -sL https://get.diversion.dev/unix | bash          # macOS or Linux CLI
export PATH="$HOME/.diversion/bin:$PATH"                # add to your shell profile
dv version
dv login                                                # opens a browser; agents reuse the session
```

On Windows, install the desktop app from Diversion's site. For CI and unattended agents, create
an API token in the web app (Organization settings, Integrations), store it as a secret, start
the sync agent and authenticate:

```sh
dv --agent --bkg
dv authenticate "$DIVERSION_API_TOKEN"
```

Put the project under Diversion, or get a copy of it:

```sh
dv init . MyGame            # in the folder that holds MyGame.uproject
dv clone <repo_id> [path] --new-workspace
dv status                   # after a clone this waits for the first sync to finish
```

## Ignore rules

Diversion reads `.dvignore` at the root and in subfolders, and treats `.gitignore` files the
same way. When both exist in a folder the patterns are merged and `.dvignore` wins. This
template's `.gitignore` already covers `Binaries/`, `Intermediate/`, `Saved/` and
`DerivedDataCache/`, so a new Diversion repo starts with the right exclusions.

- Run `dv status` right after `dv init` and read the list before the first commit. If generated
  folders or a `.git/` folder show up, fix the ignore rules first.
- Ignoring a path that is already committed has no effect. Delete it, commit the deletion, and
  warn collaborators: their copies are deleted on update.
- Every committed version of a file counts toward storage, and so do uncommitted changes and
  shelves. Commit only the captures a reviewer needs, as [10](10-docs-and-plans.md) says.

## Daily commands

| Task | Command |
|---|---|
| What changed | `dv status` |
| History | `dv log -n 20 --oneline` |
| New branch (uncommitted changes move with you) | `dv branch -c <name>` |
| Switch | `dv checkout <branch>` |
| Commit everything | `dv commit -a -m "<what and why>"` |
| Commit chosen files | `dv commit <file> <file> -m "<what and why>"` |
| Bring the branch's new commits into the workspace | `dv update` |
| Merge a branch | `dv merge <branch> --into <target>` |
| List, take, release a lock | `dv lock`, `dv lock <path>`, `dv lock -d <path>` |

`dv checkout`, `dv update` and `dv merge` take options that drop changes
(`--discard-changes`, `--conflict_resolution accept-incoming` and the like). Rule 8 applies: an
agent uses the defaults and reports a conflict instead of choosing a side.

## Working in the Unreal editor

Diversion's Unreal plugin (on Fab, installed to the engine, enabled under Edit > Plugins)
connects the editor's Revision Control to the repo. It needs the desktop app installed and signed
in. Diversion documents support for Windows and macOS; Linux is not officially supported.

- Status icons and soft-lock warnings appear on assets. A warning on open or save means the
  asset has uncommitted changes in another branch or workspace.
- The plugin cannot switch branches. Switch with `dv checkout`, then reload the affected assets
  (Asset Actions > Reload) or restart the editor.
- Agents save assets over MCP. A lock prompt in the editor is a modal dialog, and an open dialog
  can stall MCP calls. If a save hangs, look for a Diversion prompt. The prompts can be turned
  off in Project Settings > Plugins > Diversion, which edits a committed config file and so
  changes the behavior for the whole team; treat that as the person's decision. This interaction
  is expected from how the pieces work and has not been observed in this template.

## Binary assets and locks

`.uasset` and `.umap` files cannot be merged; one version wins. Diversion gives two tools:

- Soft locks (all plans): a warning badge when a file has uncommitted edits elsewhere. It
  informs, it does not block.
- Hard locks (Studio and Enterprise plans): only the lock holder can commit the path. Admins can
  auto-lock by extension (`.uasset`, `.umap`) and auto-release on commit to chosen branches.

For agents: name the assets a stage will edit in its plan, lock them before editing when hard
locks are available, and release them when the stage's commit lands. Never force another
person's lock open.

## Conflicts

Text conflicts merge line by line. A binary conflict has to be settled by choosing a version,
and Blueprints can go through Unreal's merge tool with the plugin. Manual resolution happens in
the desktop app's Merge Conflicts window, not in the CLI.

When `dv update` or `dv merge` reports conflicts, an agent stops and reports: which files, which
branches, and what each side changed. The person picks. The same goes for a sync conflict, where
Diversion keeps the local file as `<name>.dv-conflict.<ext>`.

## Keeping a Git remote as well

- Import is one-way: `dv import <git_url> [repo-name]` brings in the latest commit of the default
  branch, or everything with `--with-history`. GitHub repos need the Diversion Sync GitHub app.
- Continuous two-way sync with Git is an Enterprise feature.
- If a Git repo stays alongside (for example to publish source on GitHub), Git LFS rules and
  `.gitattributes` remain Git's concern, and `.git/` must be ignored by Diversion. Decide who
  commits where, and record it as a decision.

## Agents and CI

- One workspace per agent, each on its own branch, merged when the stage is accepted. A
  workspace is a full copy including binaries, so budget disk and sync time.
- Because uncommitted work is synced, an agent's scratch files are visible to the team. Keep
  scratch output in ignored folders (`Saved/`).
- CI: install the CLI, `dv --agent --bkg`, `dv authenticate`, `dv clone`, then run the same
  scripts as in [07](07-packaging.md). Diversion's docs show small differences in flag spelling
  between pages; trust `dv help clone`.
- Diversion publishes an agent quickstart and a Claude Code plugin that links commits to the
  conversation that produced them. Read the quickstart before the first unattended run.
