# Game design document

The source of truth for design. Agents read this before making design or architecture decisions.
Replace every section; keep the headings that agents rely on (Pillars, Core loop, Visual checks,
Performance budget, Milestones, Out of scope). Split it by topic when it nears 300 lines.

## One-line pitch

<The game in one sentence.>

## Pillars

<Three statements that decide arguments. When two options conflict, the pillars pick one.>

1.
2.
3.

## Core loop

<What the player does minute to minute, and what changes as a result.>

## Player, camera and controls

<Perspective, movement model, the input actions the game needs.>

## Systems

| System | What it does | Owner |
|---|---|---|
| <name> | <one line> | <C++ or Blueprint, per docs/guidelines/04-project-conventions.md> |

## Art and audio direction

<References and the look to aim for. Name what to avoid as well, for example default engine
looks.>

## Visual checks

The two or three views that define the look. Agents capture these whenever rendering changes.

1. <view, camera position, what must read well>
2.

## Performance budget

<Target machine, resolution, frame rate, and the floor for the 1% low.>

## Milestones

| Milestone | Delivers | Plan |
|---|---|---|
| M0 | <the smallest playable thing> | `docs/plans/<...>_PLAN.md` |
| M1 | | |

## Out of scope

Applies to all work until this list changes. Agents do not build these, even when they would be
easy.

- <feature>
- <platform>
- <content type>
