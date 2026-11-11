@AGENTS.md

## Claude Code notes

- The Unreal MCP tools are `mcp__unreal-mcp__list_toolsets`, `mcp__unreal-mcp__describe_toolset`
  and `mcp__unreal-mcp__call_tool`. They may be deferred; load them with ToolSearch before calling.
  If they fail to connect, the editor is closed: run `Scripts/editor.sh open`.
- The `ue-*` skills load from `.claude/skills/`. If they are missing and you want them, run
  `Scripts/link-ue-skills.sh`.
- `.claude/settings.json` allows the project's own scripts without a prompt. Personal settings go
  in `.claude/settings.local.json` and personal notes in `CLAUDE.local.md`; both are gitignored.
