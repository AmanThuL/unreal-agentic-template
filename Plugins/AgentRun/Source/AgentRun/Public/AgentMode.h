#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class UGameViewportClient;

/**
 * Agent runs (-AgentRun on the command line, never in the editor): scripted checks and benches that
 * share the machine with a person working on it. The game then never takes the OS mouse or keyboard:
 * no mouse capture, lock or cursor hiding, no cursor moves, and real mouse, keyboard and gamepad
 * input is dropped before Slate routes it, so the person's own typing cannot disturb the run.
 * Scripts drive the game with virtual input (AgentInput). Builds without the flag are unchanged.
 */
namespace AgentMode
{
	/** True when -AgentRun is on the command line outside the editor. Always false in Shipping. */
	AGENTRUN_API bool IsActive();

	/** For tests: forces IsActive (true or false) or, with no value, goes back to the command line. */
	AGENTRUN_API void OverrideForTests(TOptional<bool> bActive);

	/** Module startup: input settings so the viewport never captures the mouse at launch. */
	void ApplyStartupSettings();

	/** Once a viewport exists: the real-input filter, no capture or lock, rendering in the background. */
	AGENTRUN_API void Start(UGameViewportClient* Viewport);

	/**
	 * The virtual pointer in viewport pixels; false until a script has moved it. Game code that reads
	 * the OS cursor itself (a HUD drawn on the canvas, a custom cursor) should read this in agent runs.
	 */
	AGENTRUN_API bool GetPointer(FVector2D& OutViewPosition);
	AGENTRUN_API void SetPointer(const FVector2D& ViewPosition);
	AGENTRUN_API void ResetPointer();
}

/** Drops every real key, mouse and gamepad event before Slate routes it (agent runs). */
class AGENTRUN_API FAgentInputFilter : public IInputProcessor
{
public:
	/** Events dropped since the filter was made, for logs and tests. */
	int32 GetDropped() const { return Dropped; }

	//~ Begin IInputProcessor interface
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return Drop(); }
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override { return Drop(); }
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override { return Drop(); }
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return Drop(); }
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return Drop(); }
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return Drop(); }
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override { return Drop(); }
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override { return Drop(); }
	virtual const TCHAR* GetDebugName() const override { return TEXT("AgentInputFilter"); }
	//~ End IInputProcessor interface

private:
	bool Drop()
	{
		++Dropped;
		return true;
	}

	int32 Dropped = 0;
};
