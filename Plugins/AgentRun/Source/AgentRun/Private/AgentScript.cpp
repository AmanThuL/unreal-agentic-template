#include "AgentScript.h"

FAgentStep AgentScript::ParseLine(const FString& Line)
{
	FAgentStep Step;
	const FString Trimmed = Line.TrimStartAndEnd();
	TArray<FString> Parts;
	Trimmed.ParseIntoArrayWS(Parts);
	if (Parts.IsEmpty() || Parts[0].StartsWith(TEXT("#")))
	{
		return Step;
	}

	static const TMap<FString, EAgentStep> BuiltIns = {
		{ TEXT("log"), EAgentStep::Log },
		{ TEXT("wait"), EAgentStep::Wait },
		{ TEXT("mouse"), EAgentStep::Mouse },
		{ TEXT("click"), EAgentStep::Click },
		{ TEXT("wheel"), EAgentStep::Wheel },
		{ TEXT("key"), EAgentStep::Key },
		{ TEXT("axis"), EAgentStep::Axis },
		{ TEXT("goto"), EAgentStep::Goto },
		{ TEXT("bench"), EAgentStep::Bench },
		{ TEXT("shot"), EAgentStep::Shot },
		{ TEXT("selftest"), EAgentStep::SelfTest }
	};
	const EAgentStep* BuiltIn = BuiltIns.Find(Parts[0].ToLower());
	Step.Type = BuiltIn ? *BuiltIn : EAgentStep::Console;
	Step.Text = Trimmed;
	if (BuiltIn)
	{
		Parts.RemoveAt(0);
		Step.Args = MoveTemp(Parts);
		if (Step.Type == EAgentStep::Log)
		{
			Step.Text = Trimmed.RightChop(3).TrimStart();
		}
	}
	return Step;
}
