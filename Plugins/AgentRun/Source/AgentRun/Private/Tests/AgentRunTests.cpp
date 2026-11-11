#include "AgentBench.h"
#include "AgentMode.h"
#include "AgentScript.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentModeTest, "AgentRun.Mode.FilterAndPointer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAgentModeTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("The editor never runs as an agent run"), AgentMode::IsActive());

	// The virtual pointer starts unset and keeps what scripts give it.
	AgentMode::OverrideForTests(true);
	TestTrue(TEXT("The override turns agent mode on"), AgentMode::IsActive());
	AgentMode::ResetPointer();
	FVector2D Pointer(-1.0, -1.0);
	TestFalse(TEXT("No pointer before a script moves it"), AgentMode::GetPointer(Pointer));
	AgentMode::SetPointer(FVector2D(1280.0, 720.0));
	TestTrue(TEXT("The pointer is set"), AgentMode::GetPointer(Pointer) && Pointer.Equals(FVector2D(1280.0, 720.0)));
	AgentMode::ResetPointer();
	AgentMode::OverrideForTests(TOptional<bool>());

	// The filter swallows every real event it is offered.
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication& App = FSlateApplication::Get();
		FAgentInputFilter Filter;
		const FKeyEvent Key(EKeys::W, FModifierKeysState(), 0, false, 0, 0);
		const FPointerEvent Mouse(0, 0, FVector2D(10.0, 10.0), FVector2D(0.0, 0.0), TSet<FKey>(), EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
		TestTrue(TEXT("A real key press is dropped"), Filter.HandleKeyDownEvent(App, Key));
		TestTrue(TEXT("A real key release is dropped"), Filter.HandleKeyUpEvent(App, Key));
		TestTrue(TEXT("A real mouse move is dropped"), Filter.HandleMouseMoveEvent(App, Mouse));
		TestTrue(TEXT("A real click is dropped"), Filter.HandleMouseButtonDownEvent(App, Mouse));
		TestTrue(TEXT("A real wheel turn is dropped"), Filter.HandleMouseWheelOrGestureEvent(App, Mouse, nullptr));
		TestEqual(TEXT("Five events dropped"), Filter.GetDropped(), 5);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentScriptParseTest, "AgentRun.Script.ParseLine",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAgentScriptParseTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A blank line is no step"), AgentScript::ParseLine(TEXT("   ")).Type == EAgentStep::None);
	TestTrue(TEXT("A comment is no step"), AgentScript::ParseLine(TEXT("# click 0.5 0.5")).Type == EAgentStep::None);

	const FAgentStep Click = AgentScript::ParseLine(TEXT("  Click 0.25 0.75  "));
	TestTrue(TEXT("Step names ignore case"), Click.Type == EAgentStep::Click);
	TestEqual(TEXT("Click has two arguments"), Click.Args.Num(), 2);
	TestEqual(TEXT("Click x"), Click.Number(0, 0.0f), 0.25f);
	TestEqual(TEXT("Click y"), Click.Number(1, 0.0f), 0.75f);
	TestEqual(TEXT("A missing argument falls back to the default"), Click.Number(2, 0.1f), 0.1f);

	const FAgentStep Log = AgentScript::ParseLine(TEXT("log menu opened, checking rows"));
	TestTrue(TEXT("Log is a step"), Log.Type == EAgentStep::Log);
	TestEqual(TEXT("Log keeps its text"), Log.Text, FString(TEXT("menu opened, checking rows")));

	const FAgentStep Key = AgentScript::ParseLine(TEXT("key SpaceBar 0.5"));
	TestTrue(TEXT("Key is a step"), Key.Type == EAgentStep::Key);
	TestEqual(TEXT("Key name"), Key.Args[0], FString(TEXT("SpaceBar")));

	const FAgentStep Console = AgentScript::ParseLine(TEXT("stat fps"));
	TestTrue(TEXT("Anything else is a console command"), Console.Type == EAgentStep::Console);
	TestEqual(TEXT("A console command keeps the whole line"), Console.Text, FString(TEXT("stat fps")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAgentBenchSummarizeTest, "AgentRun.Bench.Summarize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAgentBenchSummarizeTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No frames, no result"), AgentBench::Summarize(TConstArrayView<float>()).Frames, 0);

	// 198 frames at 10 ms and two at 50 ms: the average hides the hitches, the lows do not.
	TArray<float> Frames;
	Frames.Init(0.010f, 198);
	Frames.Add(0.050f);
	Frames.Add(0.050f);
	const FAgentBenchResult Result = AgentBench::Summarize(Frames);
	TestEqual(TEXT("Frame count"), Result.Frames, 200);
	TestEqual(TEXT("Average fps"), Result.AverageFps, 1.0 / 0.0104, 0.1);
	TestEqual(TEXT("The 1% low is the 99th percentile frame"), Result.Low1Fps, 100.0, 0.1);
	TestEqual(TEXT("The minimum is the slowest frame"), Result.MinFps, 20.0, 0.1);
	TestEqual(TEXT("Two frames over 25 ms"), Result.Hitches, 2);
	TestEqual(TEXT("Worst frame in ms"), Result.WorstMs, 50.0, 0.01);

	const FString Row = AgentBench::CsvRow(TEXT("spawn"), Result, 3.0, 4.0, 9.0);
	TestEqual(TEXT("A row has as many fields as the header"), Row.Len() - Row.Replace(TEXT(","), TEXT("")).Len(),
		AgentBench::CsvHeader().Len() - AgentBench::CsvHeader().Replace(TEXT(","), TEXT("")).Len());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
