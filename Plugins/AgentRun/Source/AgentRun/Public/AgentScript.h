#pragma once

#include "CoreMinimal.h"

/** What one script line does. Anything that is not a built-in step runs as a console command. */
enum class EAgentStep : uint8
{
	/** A blank line or a # comment. */
	None,
	Log,
	Wait,
	Mouse,
	Click,
	Wheel,
	Key,
	Axis,
	Goto,
	Bench,
	Shot,
	SelfTest,
	Console
};

struct FAgentStep
{
	EAgentStep Type = EAgentStep::None;
	/** The words after the step name. */
	TArray<FString> Args;
	/** The whole trimmed line (console commands) or the text after "log". */
	FString Text;

	/** Argument Index as a number, or Default when it is missing. */
	float Number(int32 Index, float Default) const
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}
};

/** Script parsing, testable without a world. The step list is in Plugins/AgentRun/README.md. */
namespace AgentScript
{
	AGENTRUN_API FAgentStep ParseLine(const FString& Line);
}
