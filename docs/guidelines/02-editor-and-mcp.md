# 02. Editor and MCP

How agents work inside the Unreal editor. Tool and toolset names below are from UE 5.8; confirm
them with `list_toolsets` and `describe_toolset`, since the plugins are Experimental and can
change between engine versions.

## Rules at a glance

1. The editor must be open for MCP. Open and close it yourself with `Scripts/editor.sh`; do not
   ask the person to do it.
2. Load the editor's own skills before Blueprint, material, Niagara, PCG or lighting work.
3. Look names up: `describe_toolset` for tool schemas, `list_properties` before setting a
   property. A wrong property name can fail silently.
4. Stop PIE before editing assets.
5. Save explicitly: assets with `save_assets`, level actors with `save_actor`. Check `is_dirty`.
6. Never edit `.uasset` or `.umap` files outside the editor.
7. Before deleting or moving an asset, check `get_referencers` and confirm with the person.
8. Never call `CreateSkill` or `UpdateSkill` without the person's permission.
9. After editor work, read the log for new errors and warnings.

## The server

The project enables two engine plugins in `MyGame.uproject`: `ModelContextProtocol` (the server)
and `AllToolsets` (the tools). `Config/DefaultEditorPerProjectUserSettings.ini` starts the server
with the editor and pins it to `http://127.0.0.1:8000/mcp`; `.mcp.json` points agents at the same
address. To change the port, change it in the ini file, in `.mcp.json` and in `UE_MCP_PORT` for
the scripts.

`Scripts/editor.sh open` launches the editor in the background and returns when the editor
process itself is listening on the port. The first launch after a renderer or engine change
compiles shaders for a long time; the default timeout is 15 minutes.

`Scripts/editor.sh close` asks the editor to quit and waits. It never kills the process. If a
save prompt or a hang keeps the editor open, the script fails and you report that instead of
forcing it. So before closing: stop PIE, then save everything (`AssetTools` `save_assets` with an
empty list saves all dirty assets).

## The three tools

The server runs in tool-search mode and exposes three tools:

1. `list_toolsets`: about 50 toolsets (scene, actors, assets, Blueprints, materials, Niagara,
   PCG, UMG, logs, automation tests and more).
2. `describe_toolset`: tool names with input and output schemas for one toolset.
3. `call_tool`: `toolset_name` is the full toolset name and `tool_name` is the short name, for
   example `{"toolset_name": "EditorToolset.LogsToolset", "tool_name": "GetLogEntries"}`.
   Passing the full dotted name as `tool_name` fails with `Unknown tool`.

## Working patterns

| Task | How |
|---|---|
| Learn a toolset | `describe_toolset`, then the matching editor skill: `ListSkills` and `GetSkills` in `ToolsetRegistry.AgentSkillToolset` |
| Set object properties | `ObjectTools` `list_properties` first; names vary by class |
| Edit a Blueprint graph | `BlueprintTools` `get_graph_dsl_docs` once per session, then `read_graph_dsl` and `write_graph_dsl` (which also compiles) |
| Many calls at once | `ProgrammaticToolset` `execute_tool_script`. Call `get_execution_environment` once first. Inside a script, `execute_tool` takes the full dotted tool name |
| PIE state | `EditorAppToolset` `IsPIERunning`, `StopPIE` |
| Save | `AssetTools` `save_assets`; `SceneTools` `save_actor`; `is_dirty` to check |
| Read the log | `EditorToolset.LogsToolset` `GetLogEntries` with pattern `Error|Warning` |
| See the result | `EditorAppToolset` `CaptureViewport`, then look at the image |
| Run tests with the editor open | `AutomationTestToolset`: `DiscoverTests`, then `ListTests` and `RunTestsByFilter` |

Longer editor jobs (building a level, importing a batch of assets) are easier to review and
repeat as a script than as hundreds of single calls. Keep such scripts in `Scripts/editor/` named
after the stage that needs them, and delete them when they stop being useful.

## PIE and the person's mouse

`Config/DefaultEditorPerProjectUserSettings.ini` sets `GameGetsMouseControl=False`, so starting
PIE never captures the mouse. Keep it that way. PIE is fine for a quick look at logic; it is not
evidence for input, UI or performance (see [05](05-verification.md)).

## After editor work

- Blueprints compile without errors.
- `is_dirty` reports nothing unsaved.
- The log has no new `Error` or `Warning` lines compared with before the work.
- For visual changes, a viewport capture was taken and looked at, not only taken.
