#include "AgentRunSubsystem.h"

#include "AgentBench.h"
#include "AgentInput.h"
#include "AgentMode.h"
#include "AgentRunLog.h"
#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderTimer.h"
#include "UnrealClient.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace AgentRunPrivate
{
	/** Frames right after a teleport or a load are not what the player sees for long. */
	const double BenchSettleSeconds = 1.0;
	const float DefaultHoldSeconds = 0.1f;
}

FString UAgentRunSubsystem::GetOutputDir()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AgentRuns"));
}

bool UAgentRunSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return !IsRunningDedicatedServer();
#endif
}

void UAgentRunSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAgentRunSubsystem::Tick));

	FString Script;
	if (!GIsEditor && FParse::Value(FCommandLine::Get(), TEXT("AgentScript="), Script))
	{
		if (!RunScriptFile(Script))
		{
			// A run that cannot start must not sit there until the timeout.
			UE_LOG(LogAgentRun, Error, TEXT("AgentRun: script done, 0 steps, 1 failed"));
			FPlatformMisc::RequestExit(false);
		}
	}
}

void UAgentRunSubsystem::Deinitialize()
{
	Stop();
	FTSTicker::RemoveTicker(Ticker);
	Ticker.Reset();
	Super::Deinitialize();
}

UGameViewportClient* UAgentRunSubsystem::GetViewport() const
{
	return GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
}

APlayerController* UAgentRunSubsystem::GetController() const
{
	return GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
}

bool UAgentRunSubsystem::RunScriptFile(const FString& Path)
{
	const FString FullPath = FPaths::IsRelative(Path) ? FPaths::Combine(GetOutputDir(), Path) : Path;
	TArray<FString> FileLines;
	if (!FFileHelper::LoadFileToStringArray(FileLines, *FullPath))
	{
		UE_LOG(LogAgentRun, Error, TEXT("AgentRun: cannot read %s"), *FullPath);
		return false;
	}
	UE_LOG(LogAgentRun, Display, TEXT("AgentRun: script %s, %d lines"), *FullPath, FileLines.Num());
	RunScript(FileLines);
	return true;
}

void UAgentRunSubsystem::RunScript(const TArray<FString>& InLines)
{
	Stop();
	Lines = InLines;
	NextLine = 0;
	Steps = 0;
	Failures = 0;
	bWaitingForPlayer = true;
}

void UAgentRunSubsystem::RunStep(const FString& Line)
{
	Execute(AgentScript::ParseLine(Line));
}

void UAgentRunSubsystem::Stop()
{
	if (Mode == EMode::Hold)
	{
		// Never leave a key or the button down.
		HeldKey.IsValid() ? AgentInput::Key(GetViewport(), HeldKey, false) : AgentInput::LeftButton(GetViewport(), HeldScreen, false);
	}
	if (SelfTestRoot.IsValid())
	{
		EndSelfTest(false, TEXT("stopped"));
	}
	AgentInput::Reset();
	Lines.Reset();
	NextLine = INDEX_NONE;
	Mode = EMode::Idle;
	bWaitingForPlayer = false;
}

void UAgentRunSubsystem::Fail(const FString& Message)
{
	++Failures;
	UE_LOG(LogAgentRun, Error, TEXT("AgentRun: FAILED %s"), *Message);
}

void UAgentRunSubsystem::FinishScript()
{
	// Scripts/run-agent.sh reads this line to decide whether the run passed.
	UE_LOG(LogAgentRun, Display, TEXT("AgentRun: script done, %d steps, %d failed"), Steps, Failures);
	Lines.Reset();
	NextLine = INDEX_NONE;
}

bool UAgentRunSubsystem::FractionToScreen(const FAgentStep& Step, int32 FirstArg, FVector2D& OutScreen) const
{
	const FVector2D Fraction(Step.Number(FirstArg, 0.5f), Step.Number(FirstArg + 1, 0.5f));
	return AgentInput::ViewFractionToScreen(GetViewport(), Fraction, OutScreen);
}

bool UAgentRunSubsystem::Tick(float DeltaTime)
{
	if (bWaitingForPlayer)
	{
		if (!GetController() || !GetViewport())
		{
			return true;
		}
		bWaitingForPlayer = false;
	}
	const double Now = FPlatformTime::Seconds();
	switch (Mode)
	{
	case EMode::Wait:
		if (Now >= ModeEndTime)
		{
			Mode = EMode::Idle;
		}
		break;
	case EMode::Hold:
		if (Now >= ModeEndTime)
		{
			HeldKey.IsValid() ? AgentInput::Key(GetViewport(), HeldKey, false) : AgentInput::LeftButton(GetViewport(), HeldScreen, false);
			Mode = EMode::Idle;
		}
		break;
	case EMode::Axis:
		AgentInput::Axis(GetViewport(), AxisKey, AxisValue, DeltaTime);
		if (Now >= ModeEndTime)
		{
			Mode = EMode::Idle;
		}
		break;
	case EMode::Bench:
		TickBench(DeltaTime);
		break;
	case EMode::SelfTest:
		TickSelfTest();
		break;
	default:
		break;
	}
	if (Mode == EMode::Idle && NextLine != INDEX_NONE)
	{
		RunNextLines();
	}
	return true;
}

void UAgentRunSubsystem::RunNextLines()
{
	while (Mode == EMode::Idle && Lines.IsValidIndex(NextLine))
	{
		const FAgentStep Step = AgentScript::ParseLine(Lines[NextLine++]);
		if (Step.Type == EAgentStep::None)
		{
			continue;
		}
		++Steps;
		// "quit" ends the game at once, so the summary has to be logged before it runs.
		if (Step.Type == EAgentStep::Console && (Step.Text.Equals(TEXT("quit"), ESearchCase::IgnoreCase) || Step.Text.Equals(TEXT("exit"), ESearchCase::IgnoreCase)))
		{
			FinishScript();
		}
		Execute(Step);
	}
	if (Mode == EMode::Idle && NextLine != INDEX_NONE && !Lines.IsValidIndex(NextLine))
	{
		FinishScript();
	}
}

void UAgentRunSubsystem::Execute(const FAgentStep& Step)
{
	using namespace AgentRunPrivate;
	UGameViewportClient* Viewport = GetViewport();
	APlayerController* Controller = GetController();
	const double Now = FPlatformTime::Seconds();
	FVector2D Screen;

	switch (Step.Type)
	{
	case EAgentStep::Log:
		UE_LOG(LogAgentRun, Display, TEXT("AgentRun: %s"), *Step.Text);
		break;
	case EAgentStep::Wait:
		Mode = EMode::Wait;
		ModeEndTime = Now + Step.Number(0, 1.0f);
		break;
	case EAgentStep::Mouse:
		if (!FractionToScreen(Step, 0, Screen) || !AgentInput::MoveTo(Viewport, Screen))
		{
			Fail(Step.Text);
		}
		break;
	case EAgentStep::Click:
		if (!FractionToScreen(Step, 0, Screen) || !AgentInput::MoveTo(Viewport, Screen) || !AgentInput::LeftButton(Viewport, Screen, true))
		{
			Fail(Step.Text);
			break;
		}
		HeldKey = FKey();
		HeldScreen = Screen;
		Mode = EMode::Hold;
		ModeEndTime = Now + Step.Number(2, DefaultHoldSeconds);
		break;
	case EAgentStep::Wheel:
		if (!FractionToScreen(Step, 1, Screen) || !AgentInput::Wheel(Viewport, Screen, Step.Number(0, 1.0f)))
		{
			Fail(Step.Text);
		}
		break;
	case EAgentStep::Key:
	{
		const FKey Key(Step.Args.IsValidIndex(0) ? FName(*Step.Args[0]) : NAME_None);
		if (!AgentInput::Key(Viewport, Key, true))
		{
			Fail(Step.Text);
			break;
		}
		HeldKey = Key;
		Mode = EMode::Hold;
		ModeEndTime = Now + Step.Number(1, DefaultHoldSeconds);
		break;
	}
	case EAgentStep::Axis:
	{
		AxisKey = FKey(Step.Args.IsValidIndex(0) ? FName(*Step.Args[0]) : NAME_None);
		AxisValue = Step.Number(1, 0.0f);
		if (!AgentInput::Axis(Viewport, AxisKey, AxisValue, 0.0f))
		{
			Fail(Step.Text);
			break;
		}
		Mode = EMode::Axis;
		ModeEndTime = Now + Step.Number(2, 0.0f);
		break;
	}
	case EAgentStep::Goto:
	{
		if (!Controller)
		{
			Fail(Step.Text);
			break;
		}
		const FVector Location(Step.Number(0, 0.0f), Step.Number(1, 0.0f), Step.Number(2, 0.0f));
		if (APawn* Pawn = Controller->GetPawn())
		{
			// Put the eyes, not the pawn's origin, at the location, so captures are repeatable.
			const FVector EyeOffset = Pawn->GetPawnViewLocation() - Pawn->GetActorLocation();
			Pawn->SetActorLocation(Location - EyeOffset, false, nullptr, ETeleportType::TeleportPhysics);
			if (const ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				Character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
			}
		}
		Controller->SetControlRotation(FRotator(Step.Number(4, 0.0f), Step.Number(3, 0.0f), 0.0f));
		UE_LOG(LogAgentRun, Display, TEXT("AgentRun: goto %s yaw %.1f pitch %.1f"), *Location.ToCompactString(), Step.Number(3, 0.0f), Step.Number(4, 0.0f));
		break;
	}
	case EAgentStep::Bench:
		StartBench(Step.Number(0, 5.0f), Step.Args.IsValidIndex(1) ? Step.Args[1] : TEXT("bench"));
		break;
	case EAgentStep::Shot:
	{
		const FString Name = Step.Args.IsValidIndex(0) ? Step.Args[0] : TEXT("shot");
		const FString File = FPaths::Combine(GetOutputDir(), TEXT("Shots"), Name + TEXT(".png"));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogAgentRun, Display, TEXT("AgentRun: shot %s"), *File);
		break;
	}
	case EAgentStep::SelfTest:
		if (!Viewport)
		{
			Fail(TEXT("selftest: no game viewport"));
			break;
		}
		bSelfTestClicked = false;
		SelfTestStage = 0;
		Viewport->AddViewportWidgetContent(
			SAssignNew(SelfTestRoot, SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SAssignNew(SelfTestButton, SButton)
				.ContentPadding(FMargin(48.0f, 24.0f))
				.OnClicked_Lambda([this]()
				{
					bSelfTestClicked = true;
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("AgentRun self test")))
				]
			], 10000);
		Mode = EMode::SelfTest;
		// The button needs a painted frame before Slate's hit test can find it.
		ModeEndTime = Now + 0.5;
		break;
	case EAgentStep::Console:
		if (Controller)
		{
			Controller->ConsoleCommand(Step.Text, true);
		}
		else if (GEngine)
		{
			GEngine->Exec(GetWorld(), *Step.Text);
		}
		break;
	default:
		break;
	}
}

void UAgentRunSubsystem::StartBench(float Seconds, const FString& Label)
{
	BenchLabel = Label;
	BenchFrames.Reset();
	BenchGameMs = 0.0;
	BenchRenderMs = 0.0;
	BenchGpuMs = 0.0;
	BenchSampleStart = FPlatformTime::Seconds() + AgentRunPrivate::BenchSettleSeconds;
	ModeEndTime = BenchSampleStart + FMath::Max(Seconds, 0.5f);
	Mode = EMode::Bench;
}

void UAgentRunSubsystem::TickBench(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	if (Now < BenchSampleStart)
	{
		return;
	}
	BenchFrames.Add(DeltaTime);
	BenchGameMs += FPlatformTime::ToMilliseconds(GGameThreadTime);
	BenchRenderMs += FPlatformTime::ToMilliseconds(GRenderThreadTime);
	BenchGpuMs += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
	if (Now < ModeEndTime)
	{
		return;
	}
	Mode = EMode::Idle;

	const FAgentBenchResult Result = AgentBench::Summarize(BenchFrames);
	const double Count = FMath::Max(Result.Frames, 1);
	const FString Row = AgentBench::CsvRow(BenchLabel, Result, BenchGameMs / Count, BenchRenderMs / Count, BenchGpuMs / Count);
	UE_LOG(LogAgentRun, Display, TEXT("AgentRun: bench %s fps=%.1f low1=%.1f min=%.1f hitches=%d game=%.2f render=%.2f gpu=%.2f ms (%d frames)"),
		*BenchLabel, Result.AverageFps, Result.Low1Fps, Result.MinFps, Result.Hitches, BenchGameMs / Count, BenchRenderMs / Count,
		BenchGpuMs / Count, Result.Frames);

	const FString File = FPaths::Combine(GetOutputDir(), TEXT("bench.csv"));
	const bool bNew = !IFileManager::Get().FileExists(*File);
	const FString Text = (bNew ? AgentBench::CsvHeader() + LINE_TERMINATOR : FString()) + Row + LINE_TERMINATOR;
	if (!FFileHelper::SaveStringToFile(Text, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
	{
		Fail(FString::Printf(TEXT("bench: cannot write %s"), *File));
	}
}

void UAgentRunSubsystem::TickSelfTest()
{
	const double Now = FPlatformTime::Seconds();
	if (Now < ModeEndTime)
	{
		return;
	}
	const UGameViewportClient* Viewport = GetViewport();
	if (!Viewport || !SelfTestButton.IsValid())
	{
		EndSelfTest(false, TEXT("the game viewport went away"));
		return;
	}
	const FGeometry& Geometry = SelfTestButton->GetTickSpaceGeometry();
	const FVector2D Center(Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f));
	bool bSent = true;
	switch (SelfTestStage++)
	{
	case 0:
		bSent = AgentInput::MoveTo(Viewport, Center);
		break;
	case 1:
		if (!SelfTestButton->IsHovered())
		{
			EndSelfTest(false, TEXT("the virtual pointer did not hover the button"));
			return;
		}
		bSent = AgentInput::LeftButton(Viewport, Center, true);
		break;
	case 2:
		bSent = AgentInput::LeftButton(Viewport, Center, false);
		break;
	default:
		EndSelfTest(bSelfTestClicked, bSelfTestClicked ? TEXT("a virtual click pressed a Slate button") : TEXT("the virtual click did not press the button"));
		return;
	}
	if (!bSent)
	{
		EndSelfTest(false, TEXT("the event could not be sent"));
		return;
	}
	ModeEndTime = Now + 0.1;
}

void UAgentRunSubsystem::EndSelfTest(bool bOk, const FString& Message)
{
	AgentInput::Reset();
	if (UGameViewportClient* Viewport = GetViewport(); Viewport && SelfTestRoot.IsValid())
	{
		Viewport->RemoveViewportWidgetContent(SelfTestRoot.ToSharedRef());
	}
	SelfTestRoot.Reset();
	SelfTestButton.Reset();
	Mode = EMode::Idle;
	if (bOk)
	{
		UE_LOG(LogAgentRun, Display, TEXT("AgentRun: selftest ok, %s"), *Message);
	}
	else
	{
		Fail(FString::Printf(TEXT("selftest: %s"), *Message));
	}
}
