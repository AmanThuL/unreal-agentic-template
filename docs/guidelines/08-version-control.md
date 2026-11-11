# 08. Version control

This guideline covers Git. For a project versioned with Diversion, the same rules apply with
different commands: see [09](09-version-control-diversion.md).

## Rules at a glance

1. Commit only when the person asks. Finishing a stage is not permission to commit.
2. Never commit `Binaries/`, `Intermediate/`, `Saved/` or `DerivedDataCache/`.
3. Binary files (`.uasset`, `.umap`, images, meshes, audio) go through Git LFS. Before committing
   a binary type that is not in `.gitattributes`, add it there.
4. A commit compiles, passes the tests, and says why the change was made.
5. Stage files explicitly. Look at `git status` before and after; do not sweep in files you did
   not mean to change.
6. Never bypass hooks (`--no-verify`) and never rewrite shared history.
7. Two people or agents never edit the same binary asset at the same time. Binary assets cannot
   be merged.
8. Follow the project's attribution rules for commits and pull requests. Add nothing they do not
   ask for.

## What is in the repo

| Path | Kind | Notes |
|---|---|---|
| `Config/`, `Source/`, `Plugins/*/Source/`, `Scripts/`, `docs/` | Text | Normal diffs and merges |
| `Content/` | Binary, LFS | Changed only through the editor; one owner per asset at a time |
| `docs/captures/` | Binary, LFS | Only the images a reviewer needs |
| `Binaries/`, `Intermediate/`, `Saved/`, `DerivedDataCache/` | Generated | Ignored |

## Git LFS

Run `git lfs install` once per machine, ideally before cloning. If a binary file shows up as a
small text pointer, run `git lfs pull`. `.gitattributes` lists every tracked binary type; a new
type added without an entry there is stored as a plain blob and bloats the history for good.

## Commits

- Small, and each one leaves the project working: it builds, the tests pass, the editor opens.
- The message says what changed and why. Match the style already in `git log`.
- Assets and the code that needs them go in the same commit, so no commit references something
  that does not exist yet.
- The editor rewrites some files when it opens a project or saves a level. Commit that churn only
  when it belongs to your change; otherwise restore the files.
- Before committing docs, run `Scripts/check-docs.sh`.

## Binary assets and parallel work

A `.uasset` or `.umap` cannot be merged. When more than one person or agent works on the project:

- Split levels so that people work in different files (sublevels, or World Partition with One
  File Per Actor).
- Say which assets a stage will edit in its plan. A subagent's brief names the assets it owns.
- If two branches changed the same binary asset, one of the changes is redone by hand in the
  editor. Decide which before merging, not after.

## Branches

Keep a default branch that always builds, passes tests and opens in the editor. Do work on
short-lived branches named for the stage or the change. If you are on the default branch when
asked to commit work in progress, branch first.

## Before deleting or moving assets

Moving or renaming an asset outside the editor breaks every reference to it. Do it in the editor
(over MCP), check `get_referencers` first, confirm with the person, and fix up redirectors
afterwards. Deleting an asset with referencers is never a cleanup; it is a change to whatever
referenced it.
