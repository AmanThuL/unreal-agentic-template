#pragma once

#include "CoreMinimal.h"
#include "AgentScript.h"
#include "Containers/Ticker.h"
#include "InputCoreTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentRunSubsystem.generated.h"

class APlayerController;
class SButton;
class SWidget;
class UGameViewportClient;

/**
 * Plays a script, one step per line, against the running game. It lives on the game instance, so a
 * script keeps running across level loads, and it ticks in real time, so waits run while the game is
 * paused. Start one with -AgentScript=<file> on the command line (Scripts/run-agent.sh does this) or
 * with the console command Agent.Run <file>. Relative paths are under Saved/AgentRuns/.
 *
 * The step list is in Plugins/AgentRun/README.md. Not created in Shipping builds.
 */
UCLASS()
class AGENTRUN_API UAgentRunSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Loads a script and starts it once a local player controller exists. */
	bool RunScriptFile(const FString& Path);

	/** Starts a script from lines. */
	void RunScript(const TArray<FString>& InLines);

	/** Runs one step now, outside a script (the Agent.Step console command). */
	void RunStep(const FString& Line);

	void Stop();

	bool IsRunning() const { return Lines.IsValidIndex(NextLine) || Mode != EMode::Idle; }

	/** Steps that failed since the last script started. */
	int32 GetFailures() const { return Failures; }

	/** Saved/AgentRuns/, where scripts are read and benches and screenshots are written. */
	static FString GetOutputDir();

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

private:
	enum class EMode : uint8
	{
		Idle,
		Wait,
		/** A key or the left button is down until ModeEndTime. */
		Hold,
		Axis,
		Bench,
		SelfTest
	};

	bool Tick(float DeltaTime);
	void RunNextLines();
	void Execute(const FAgentStep& Step);
	void Fail(const FString& Message);
	void FinishScript();

	void StartBench(float Seconds, const FString& Label);
	void TickBench(float DeltaTime);
	void TickSelfTest();
	void EndSelfTest(bool bOk, const FString& Message);

	UGameViewportClient* GetViewport() const;
	APlayerController* GetController() const;
	bool FractionToScreen(const FAgentStep& Step, int32 FirstArg, FVector2D& OutScreen) const;

	TArray<FString> Lines;
	int32 NextLine = INDEX_NONE;
	int32 Steps = 0;
	int32 Failures = 0;
	/** A script loaded before the player existed. */
	bool bWaitingForPlayer = false;

	EMode Mode = EMode::Idle;
	double ModeEndTime = 0.0;

	/** Hold: what to release. */
	FKey HeldKey;
	FVector2D HeldScreen = FVector2D::ZeroVector;

	/** Axis: the sample sent every frame. */
	FKey AxisKey;
	float AxisValue = 0.0f;

	/** Bench: samples after the settle time. */
	FString BenchLabel;
	double BenchSampleStart = 0.0;
	TArray<float> BenchFrames;
	double BenchGameMs = 0.0;
	double BenchRenderMs = 0.0;
	double BenchGpuMs = 0.0;

	/** Self test: a button on the viewport that a virtual click must press. */
	TSharedPtr<SWidget> SelfTestRoot;
	TSharedPtr<SButton> SelfTestButton;
	int32 SelfTestStage = 0;
	bool bSelfTestClicked = false;

	FTSTicker::FDelegateHandle Ticker;
};
