# 03. C++ build and test loop

## Rules at a glance

1. For any C++ change: save over MCP, `Scripts/editor.sh close`, `Scripts/build-editor.sh`,
   `Scripts/run-tests.sh`, `Scripts/editor.sh open`.
2. Fix all errors and all new warnings before moving on.
3. A test run passes only when `Scripts/run-tests.sh` exits 0. Do not judge a run by eye from a
   log tail.
4. Name tests `MyGame.<Area>.<Case>`. Put them in `Source/MyGame/Private/Tests/`.
5. Keep logic testable without a world: math, parsing, state machines and data rules go in plain
   functions and structs that actors and components call.
6. Never disable or weaken a failing test to make a run pass. Fix the code or fix the test.
7. Confirm an engine API in the engine source before using it. Training data is often older than
   the engine.
8. Follow Epic's coding standard: type prefixes, PascalCase, `TObjectPtr` members, `UPROPERTY`
   and `UFUNCTION` with a `Category`, engine containers over `std`.

## Why the editor has to close

Live Coding is Windows-only. On macOS and Linux a running editor keeps the old module loaded and
locks the binaries, so C++ cannot be rebuilt underneath it. On Windows, Live Coding handles
function body changes but not header or reflection changes, so the close, build, reopen loop is
still the path that always works. The scripts refuse to build, test or package while the editor
has the project open.

MCP is offline while the editor is closed. Plan the stage so that C++ work and editor work come
in batches, not interleaved one call at a time.

## The build

```sh
Scripts/build-editor.sh
```

This calls the engine's build script for `MyGameEditor`, Development, on the host platform with
`-WaitMutex`. Extra arguments pass through to UnrealBuildTool. Set `UE_ENGINE_DIR` to the engine's
`Engine/` folder when the engine is not in the launcher's default location.

A new module dependency goes in `Source/MyGame/MyGame.Build.cs` with a comment naming the feature
that needs it.

## The tests

```sh
Scripts/run-tests.sh                # default filter: MyGame+AgentRun
Scripts/run-tests.sh MyGame.Combat  # one area
Scripts/run-tests.sh --check-log Saved/Logs/Tests.log
```

The script runs `UnrealEditor-Cmd` headless with `-nullrhi` and writes `Saved/Logs/Tests.log`.
It exits 1 unless all of these hold:

- at least one test ran,
- every test reports `Result={Success}`,
- the number of results equals the number of tests found (a crash or hang mid-run leaves fewer),
- the log has `TEST COMPLETE. EXIT CODE: 0` and the editor process exited with 0.

With the editor open, run tests through MCP instead (`AutomationTestToolset`).

## Writing tests

The smoke test in `Source/MyGame/Private/Tests/SmokeTests.cpp` shows the shape:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageFalloffTest, "MyGame.Combat.DamageFalloff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDamageFalloffTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Full damage at the center"), Combat::Falloff(0.0f, 500.0f), 1.0f);
	return true;
}
```

- Wrap test files in `#if WITH_DEV_AUTOMATION_TESTS`.
- Test behavior, not implementation. Give each check a message that reads as a sentence.
- Tests are deterministic: no wall-clock time, no unseeded randomness, no dependence on order.
- Headless runs use `-nullrhi`: there is no rendering and no game viewport. Guard anything that
  needs Slate with `FSlateApplication::IsInitialized()`.
- What needs a rendered frame or real input is checked with an agent run
  ([06](06-agent-runs.md)), not with an automation test.

The pattern that keeps logic testable: a namespace or struct of pure functions holds the rule
(`AgentBench::Summarize` in the AgentRun plugin is an example), and the actor, component or
subsystem only feeds it engine state and applies the result.

## Checking engine APIs

```sh
grep -rn "SetPointerCaptor" "$UE_ENGINE_DIR/Source/Runtime/Slate/Public"
```

Read the header before calling anything you have not used in this engine version. Signatures and
whole subsystems change between minor versions, and a confident guess costs a build cycle.
