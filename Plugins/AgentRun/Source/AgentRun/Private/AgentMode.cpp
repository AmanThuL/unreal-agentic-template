#include "AgentMode.h"

#include "AgentRunLog.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/InputSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY(LogAgentRun);

namespace AgentModePrivate
{
	TOptional<bool> ForcedActive;
	TOptional<FVector2D> Pointer;
	TSharedPtr<FAgentInputFilter> Filter;
}

bool AgentMode::IsActive()
{
	if (AgentModePrivate::ForcedActive.IsSet())
	{
		return AgentModePrivate::ForcedActive.GetValue();
	}
#if UE_BUILD_SHIPPING
	return false;
#else
	static const bool bFlag = FParse::Param(FCommandLine::Get(), TEXT("AgentRun"));
	return bFlag && !GIsEditor;
#endif
}

void AgentMode::OverrideForTests(TOptional<bool> bActive)
{
	AgentModePrivate::ForcedActive = bActive;
}

void AgentMode::ApplyStartupSettings()
{
	if (!IsActive())
	{
		return;
	}
	// Read when the game viewport is made, before any game code runs.
	UInputSettings* Input = GetMutableDefault<UInputSettings>();
	Input->bCaptureMouseOnLaunch = false;
	// The viewport passes injected clicks to the game only in a capture mode; the capture itself never
	// happens, since the replies to injected viewport events are dropped and real clicks are filtered.
	Input->DefaultViewportMouseCaptureMode = EMouseCaptureMode::CaptureDuringMouseDown;
	Input->DefaultViewportMouseLockMode = EMouseLockMode::DoNotLock;
}

void AgentMode::Start(UGameViewportClient* Viewport)
{
	if (!IsActive())
	{
		return;
	}
	if (Viewport)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
		Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
		Viewport->SetHideCursorDuringCapture(false);
	}
	if (FSlateApplication::IsInitialized() && !AgentModePrivate::Filter.IsValid())
	{
		AgentModePrivate::Filter = MakeShared<FAgentInputFilter>();
		FSlateApplication::Get().RegisterInputPreProcessor(AgentModePrivate::Filter, 0);
		FSlateApplication::Get().ReleaseAllPointerCapture();
		UE_LOG(LogAgentRun, Display, TEXT("AgentRun: agent mode on. Real mouse and keyboard input is ignored, no mouse capture, rendering in the background."));
	}
	// Keep drawing while another app is in front.
	if (IConsoleVariable* Idle = IConsoleManager::Get().FindConsoleVariable(TEXT("t.IdleWhenNotForeground")))
	{
		Idle->Set(0, ECVF_SetByCode);
	}
}

bool AgentMode::GetPointer(FVector2D& OutViewPosition)
{
	if (!AgentModePrivate::Pointer.IsSet())
	{
		return false;
	}
	OutViewPosition = AgentModePrivate::Pointer.GetValue();
	return true;
}

void AgentMode::SetPointer(const FVector2D& ViewPosition)
{
	AgentModePrivate::Pointer = ViewPosition;
}

void AgentMode::ResetPointer()
{
	AgentModePrivate::Pointer.Reset();
}
