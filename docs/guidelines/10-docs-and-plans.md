# 10. Docs and plans

Docs are how context survives between sessions, for agents and for people. These rules apply to
every Markdown file in the repo: `docs/`, `AGENTS.md`, `CLAUDE.md`, `README.md` and skills.

## Rules at a glance

1. A doc has at most 300 lines. When a doc would pass the limit, split it by topic and link the
   parts from the doc that introduces them.
2. Run `Scripts/check-docs.sh` before handing over or committing docs. It checks the length
   limit and that relative links resolve.
3. Each stage has a plan in `docs/plans/` before work starts and a results doc when it ends.
4. Update a plan's status as work lands. A plan that says "Not started" for finished work is
   worse than no plan.
5. Record an engineering decision that would be expensive to rediscover in `docs/decisions/`.
6. Write for a reader with no context: full sentences, defined terms, exact names and paths.
7. Do not create planning or analysis documents nobody asked for. The plan, the results and the
   decision records are the working set.

## Why 300 lines

Agents read whole files. A long doc costs context every time it is opened, and the part that
matters is usually one section. Short, single-topic docs can be loaded when needed and skipped
when not. The limit is strict so that nobody has to judge it case by case.

## Where docs live

| Folder | Holds | Lifetime |
|---|---|---|
| `docs/guidelines/` | How to work in this repo | Long; change when a convention changes |
| `docs/design/` | The GDD and design notes per feature | Long; the GDD is the source of truth for design |
| `docs/plans/` | Stage plans and results | A plan is live until its stage is done, then it is history |
| `docs/decisions/` | One record per engineering decision | Permanent; superseded, not edited |
| `docs/captures/<stage>/` | Evidence images referenced by results docs | As long as the results doc |

When a convention changes, update the guideline that owns it in the same change, and
`AGENTS.md` too if a non-negotiable changed.

## Plans

Copy `docs/plans/PLAN_TEMPLATE.md` to `docs/plans/<TOPIC>_PLAN.md`. A plan has three to five
stages, each with:

- Goal: one specific deliverable.
- Success criteria: outcomes a check can decide. Put the measurable part in the criterion and
  mark the part only a person can judge as pending the person.
- Checks: the tests and agent runs that will decide the criteria.
- Out of scope: what this stage will not touch, so nobody widens it by accident.
- Status: Not started, In progress, Complete, or Complete with descopes.

## Results

Copy `docs/plans/RESULTS_TEMPLATE.md` to `docs/plans/<TOPIC>_RESULTS.md` when a stage ends. It
is the handover to the reviewer and to the next session:

- What exists now: classes, assets, maps, settings, by exact name.
- Success criteria: one row each, with the result and the evidence.
- Decisions made during the stage: every call the plan did not make, so the person can override
  any of them.
- Cut or changed: what the plan asked for that was not done, and why.
- Pending the person: playtests and judgment calls.
- Follow-ups: things noticed and left alone.

## Decision records

Copy `docs/decisions/0000-template.md`. Write one when you pick an approach for something risky
or easy to get wrong twice: a runtime mesh strategy, a save format, a rendering path, a plugin
adoption. Include what the engine source showed, since that is the part that is expensive to
find again. Keep it under a page.

## Writing style

- Lead with the answer or the rule. Put the reason after it.
- Use exact names: the class, the asset path, the console command, the file.
- Prefer a table when comparing, a numbered list for steps, and prose for reasoning.
- Say what was verified and how. Mark anything unverified as unverified.
- Use sentence-case headings. No decorative emphasis.
- Dates are absolute (2026-03-14), never "last week".
