# 01. Agent workflow

How work is planned, carried out and reported when an agent does most of it and a person reviews.

## Rules at a glance

1. Work in stages. A stage has one deliverable, testable success criteria, and fits in one
   working session.
2. Write the plan before the work: `docs/plans/<TOPIC>_PLAN.md`, from the template. Update its
   status as work lands.
3. Read before you write. Find how the codebase already does the thing, and read the GDD section
   the stage serves.
4. Inside the stage's scope, make the calls yourself. Record each one so the person can override
   it. Ask first only for things that are hard to undo or outside the scope.
5. Close a stage with `docs/plans/<TOPIC>_RESULTS.md`: what exists now, each criterion with its
   result and evidence, decisions made, what was cut, what is pending the person.
6. After three failed attempts at the same problem, stop and reassess.
7. Leave the workspace clean: assets saved, PIE stopped, the editor open again, no stray files.

## The stage loop

```
plan  ->  build in small steps  ->  verify  ->  results  ->  review by the person
            ^                         |
            +---- fix and re-verify --+
```

Plan. Break the milestone into three to five stages. For each: goal, success criteria that a
check can decide, the tests and agent runs that will decide them, and what is out of scope. A
criterion like "swimming feels good" cannot be checked by an agent; write the measurable part
("holds depth with no input, 2 m/s cruise") and mark the feel as pending the person.

Build. Small changes that compile and pass tests. For C++, write the test first when the logic
can be tested without a world (see [03](03-cpp-build-test-loop.md)). For editor work, load the
editor's skills first and save as you go (see [02](02-editor-and-mcp.md)).

Verify. Climb the evidence ladder in [05](05-verification.md) as far as the change needs. A
change to player-facing behavior is not verified by unit tests alone.

Results. Write the results doc while the evidence is fresh. Copy the screenshots that matter into
`docs/captures/<stage>/`; the full set stays in `Saved/` and is not committed.

## Decide, then report

A person reviewing agent work wants a finished stage and a list of the calls that were made, not
a queue of questions. So:

- If a choice is inside the stage's scope and easy to change later, decide. Write one line in the
  results doc: what you chose, what the alternative was, and why.
- If a plan detail turns out to be impossible (a number that cannot be met, an asset that does not
  exist), choose the nearest thing that keeps the intent, and record the deviation.
- If a choice is expensive to reverse, changes the design, deletes or moves assets, or publishes
  anything outside the repo, ask before acting.
- Never widen the scope silently. Things you noticed but did not do go in the results doc under
  follow-ups.

## Briefing a subagent

When you hand part of a stage to another agent, the brief is all it knows. Include:

- The goal and the success criteria, copied from the plan.
- The files and docs to read first, by path.
- The rules it must not break. Always include rule 1 from `AGENTS.md` (never take the person's
  mouse, keyboard or focus) and the verification it must run before it reports.
- Whether the editor is open or closed right now, and who owns it. Only one agent at a time
  drives the editor or runs a build.

## When you are stuck

After three failed attempts at the same problem:

1. Write down what you tried, the exact error text, and why you think each attempt failed.
2. Look for two or three working examples: in this repo, in the engine source, in the engine's
   own plugins.
3. Question the approach. Is this the right level of abstraction? Can the problem be split? Is
   there a simpler way that drops a requirement nobody needs?
4. Try a different angle, or stop and ask with the write-up in hand.

Repeating the same fix with small variations is the failure mode this rule exists to stop.

## Definition of done for a stage

- The build succeeds with no new warnings and every automation test passes.
- Each success criterion has a result (pass, partial, fail, pending the person) and evidence.
- Assets are saved, the editor log has no new errors or warnings, PIE is stopped.
- The plan's status is updated and the results doc is written.
- `Scripts/check-docs.sh` passes.
- Nothing is committed unless the person asked for a commit.
