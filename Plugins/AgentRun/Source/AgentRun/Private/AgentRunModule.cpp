#include "AgentMode.h"
#include "AgentRunLog.h"
#include "AgentRunSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

class FAgentRunModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Agent runs (-AgentRun) must not capture the OS mouse when the viewport opens.
		AgentMode::ApplyStartupSettings();
		ViewportCreated = UGameViewportClient::OnViewportCreated().AddLambda([]()
		{
			AgentMode::Start(GEngine ? GEngine->GameViewport.Get() : nullptr);
		});
	}

	virtual void ShutdownModule() override
	{
		UGameViewportClient::OnViewportCreated().Remove(ViewportCreated);
	}

private:
	FDelegateHandle ViewportCreated;
};

IMPLEMENT_MODULE(FAgentRunModule, AgentRun);

#if !UE_BUILD_SHIPPING

namespace AgentRunCommands
{
	UAgentRunSubsystem* Find(const UWorld* World)
	{
		const UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
		return Instance ? Instance->GetSubsystem<UAgentRunSubsystem>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs Run(TEXT("Agent.Run"), TEXT("Agent.Run <file>: plays a script. Relative paths are under Saved/AgentRuns/."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UAgentRunSubsystem* Runner = Find(World); Runner && Args.Num() > 0)
			{
				Runner->RunScriptFile(Args[0]);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs Step(TEXT("Agent.Step"), TEXT("Agent.Step <script line>: runs one script step, for example Agent.Step click 0.5 0.4."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UAgentRunSubsystem* Runner = Find(World))
			{
				Runner->RunStep(FString::Join(Args, TEXT(" ")));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs Stop(TEXT("Agent.Stop"), TEXT("Agent.Stop: stops the running script and releases held input."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UAgentRunSubsystem* Runner = Find(World))
			{
				Runner->Stop();
			}
		}));
}

#endif // !UE_BUILD_SHIPPING
