#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UGameViewportClient;

/**
 * Virtual input for scripted runs. Events enter at the Slate widgets of the game window, found with
 * Slate's own hit test, so UMG and Slate menus and the game viewport all receive them the way they
 * receive real input. Nothing goes through the OS or through FSlateApplication's event routing: the
 * real cursor never moves, the window is never activated or brought to the front, and the game never
 * captures the mouse. Positions are fractions of the game viewport (0 to 1, origin top left).
 *
 * Limits: one left button, no drag and drop, no double clicks, no text typing. A widget that reads
 * the OS cursor position itself needs AgentMode::GetPointer in agent runs.
 */
namespace AgentInput
{
	/** The desktop (Slate screen space) position of a point given as fractions of the game viewport. */
	AGENTRUN_API bool ViewFractionToScreen(const UGameViewportClient* Viewport, const FVector2D& Fraction, FVector2D& OutScreen);

	/** Moves the virtual pointer: hover enter and leave, then a mouse move, on the widgets under it. */
	AGENTRUN_API bool MoveTo(const UGameViewportClient* Viewport, const FVector2D& Screen);

	/** Presses (bDown) or releases the left button at a desktop position. */
	AGENTRUN_API bool LeftButton(const UGameViewportClient* Viewport, const FVector2D& Screen, bool bDown);

	/** Turns the mouse wheel by Delta notches (positive away from the user). */
	AGENTRUN_API bool Wheel(const UGameViewportClient* Viewport, const FVector2D& Screen, float Delta);

	/** Presses or releases a key: first the focused widget and its parents, then the game viewport. */
	AGENTRUN_API bool Key(const UGameViewportClient* Viewport, const FKey& InKey, bool bDown);

	/** Sends one axis sample (MouseX, MouseY, a gamepad stick) to the game viewport. */
	AGENTRUN_API bool Axis(UGameViewportClient* Viewport, const FKey& InKey, float Value, float DeltaTime);

	/** Forgets the hovered widgets and releases the virtual capture. */
	AGENTRUN_API void Reset();
}
