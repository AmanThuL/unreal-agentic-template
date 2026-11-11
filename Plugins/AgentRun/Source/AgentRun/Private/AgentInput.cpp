#include "AgentInput.h"

#include "AgentMode.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "HAL/PlatformTime.h"
#include "InputKeyEventArgs.h"
#include "Layout/WidgetPath.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

namespace AgentInputPrivate
{
	/** Widgets the virtual pointer is over, root first. */
	TArray<TWeakPtr<SWidget>> Hovered;
	/** The widget that asked to capture the mouse on the last virtual press. */
	TWeakPtr<SWidget> Captor;

	const uint32 UserIndex = FSlateApplication::CursorUserIndex;
	const uint32 PointerIndex = FSlateApplication::CursorPointerIndex;

	FPointerEvent MakePointerEvent(const FVector2D& Screen, const TSet<FKey>& Pressed, FKey Effecting, float WheelDelta = 0.0f)
	{
		return FPointerEvent(UserIndex, PointerIndex, Screen, Screen, Pressed, Effecting, WheelDelta, FModifierKeysState());
	}

	/** The widgets of the game window under a desktop position, root first. Empty if nothing is hit. */
	FWidgetPath PathUnder(const UGameViewportClient* Viewport, const FVector2D& Screen)
	{
		const TSharedPtr<SWindow> Window = Viewport ? Viewport->GetWindow() : nullptr;
		if (!Window.IsValid())
		{
			return FWidgetPath();
		}
		TArray<TSharedRef<SWindow>> Windows;
		Windows.Add(Window.ToSharedRef());
		return FSlateApplication::Get().LocateWindowUnderMouse(Screen, Windows, false, UserIndex);
	}

	/**
	 * The path under Screen, or just the game viewport widget when the hit test finds nothing (for
	 * example before the first frame is painted).
	 */
	bool FindTargets(const UGameViewportClient* Viewport, const FVector2D& Screen, TArray<FArrangedWidget>& OutRootFirst)
	{
		OutRootFirst.Reset();
		const FWidgetPath Path = PathUnder(Viewport, Screen);
		if (Path.IsValid())
		{
			for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
			{
				OutRootFirst.Add(Path.Widgets[Index]);
			}
			return true;
		}
		const TSharedPtr<SViewport> Widget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
		if (!Widget.IsValid())
		{
			return false;
		}
		OutRootFirst.Add(FArrangedWidget(Widget.ToSharedRef(), Widget->GetTickSpaceGeometry()));
		return true;
	}

	/** Sets the virtual pointer (viewport pixels) from a desktop position. */
	void SetVirtualPointer(const UGameViewportClient* Viewport, const FVector2D& Screen)
	{
		const TSharedPtr<SViewport> Widget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
		if (!Widget.IsValid() || !Viewport->Viewport)
		{
			return;
		}
		const FGeometry& Geometry = Widget->GetTickSpaceGeometry();
		const FVector2D LocalSize(Geometry.GetLocalSize());
		if (LocalSize.X <= 0.0 || LocalSize.Y <= 0.0)
		{
			return;
		}
		const FVector2D Fraction = FVector2D(Geometry.AbsoluteToLocal(FVector2f(Screen))) / LocalSize;
		const FIntPoint Pixels = Viewport->Viewport->GetSizeXY();
		AgentMode::SetPointer(Fraction * FVector2D(Pixels.X, Pixels.Y));
	}

	/**
	 * Hover: leave for widgets no longer under the pointer, enter for every widget now under it.
	 * Enter is sent again on purpose before presses and releases, because Slate's own hover pass
	 * (driven by the real cursor) can clear a hover between two virtual events.
	 */
	void UpdateHover(const TArray<FArrangedWidget>& RootFirst, const FPointerEvent& Event)
	{
		for (const TWeakPtr<SWidget>& Old : Hovered)
		{
			const TSharedPtr<SWidget> Widget = Old.Pin();
			if (Widget.IsValid() && !RootFirst.ContainsByPredicate([&Widget](const FArrangedWidget& Arranged) { return Arranged.Widget == Widget; }))
			{
				Widget->OnMouseLeave(Event);
			}
		}
		Hovered.Reset();
		for (const FArrangedWidget& Arranged : RootFirst)
		{
			Arranged.Widget->OnMouseEnter(Arranged.Geometry, Event);
			Hovered.Add(Arranged.Widget);
		}
	}

	bool IsGameViewport(const UGameViewportClient* Viewport, const TSharedRef<SWidget>& Widget)
	{
		return Viewport && Viewport->GetGameViewportWidget() == Widget;
	}
}

void AgentInput::Reset()
{
	using namespace AgentInputPrivate;
	Hovered.Reset();
	if (Captor.IsValid() && FSlateApplication::IsInitialized())
	{
		if (const TSharedPtr<FSlateUser> User = FSlateApplication::Get().GetUser(UserIndex))
		{
			User->ReleaseCapture(PointerIndex);
		}
	}
	Captor.Reset();
}

bool AgentInput::ViewFractionToScreen(const UGameViewportClient* Viewport, const FVector2D& Fraction, FVector2D& OutScreen)
{
	const TSharedPtr<SViewport> Widget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	if (!Widget.IsValid())
	{
		return false;
	}
	const FGeometry& Geometry = Widget->GetTickSpaceGeometry();
	OutScreen = FVector2D(Geometry.LocalToAbsolute(FVector2f(Fraction * FVector2D(Geometry.GetLocalSize()))));
	return true;
}

bool AgentInput::MoveTo(const UGameViewportClient* Viewport, const FVector2D& Screen)
{
	using namespace AgentInputPrivate;
	TArray<FArrangedWidget> Targets;
	if (!FSlateApplication::IsInitialized() || !FindTargets(Viewport, Screen, Targets))
	{
		return false;
	}
	SetVirtualPointer(Viewport, Screen);
	const FPointerEvent Event = MakePointerEvent(Screen, TSet<FKey>(), EKeys::Invalid);
	UpdateHover(Targets, Event);
	for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
	{
		if (Targets[Index].Widget->OnMouseMove(Targets[Index].Geometry, Event).IsEventHandled())
		{
			break;
		}
	}
	return true;
}

bool AgentInput::LeftButton(const UGameViewportClient* Viewport, const FVector2D& Screen, bool bDown)
{
	using namespace AgentInputPrivate;
	TArray<FArrangedWidget> Targets;
	if (!FSlateApplication::IsInitialized() || !FindTargets(Viewport, Screen, Targets))
	{
		return false;
	}
	SetVirtualPointer(Viewport, Screen);
	TSet<FKey> Pressed;
	if (bDown)
	{
		Pressed.Add(EKeys::LeftMouseButton);
	}
	const FPointerEvent Event = MakePointerEvent(Screen, Pressed, EKeys::LeftMouseButton);
	UpdateHover(Targets, Event);
	const TSharedPtr<FSlateUser> User = FSlateApplication::Get().GetUser(UserIndex);

	if (!bDown)
	{
		// A widget that captured the press gets the release wherever the pointer is, as in Slate.
		if (const TSharedPtr<SWidget> Capturing = Captor.Pin())
		{
			Capturing->OnMouseButtonUp(Capturing->GetTickSpaceGeometry(), Event);
			if (User.IsValid())
			{
				User->ReleaseCapture(PointerIndex);
			}
			Captor.Reset();
			return true;
		}
		for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
		{
			if (Targets[Index].Widget->OnMouseButtonUp(Targets[Index].Geometry, Event).IsEventHandled())
			{
				break;
			}
		}
		return true;
	}

	// Preview events go from the root down; the first widget to handle one stops the press.
	for (const FArrangedWidget& Target : Targets)
	{
		if (Target.Widget->OnPreviewMouseButtonDown(Target.Geometry, Event).IsEventHandled())
		{
			return true;
		}
	}
	for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
	{
		const FReply Reply = Targets[Index].Widget->OnMouseButtonDown(Targets[Index].Geometry, Event);
		if (!Reply.IsEventHandled())
		{
			continue;
		}
		// The game viewport's reply asks for OS focus, capture and cursor moves; it is dropped on
		// purpose. A menu widget's reply is honored inside Slate only: capture (buttons click on
		// release only while they hold it) and keyboard focus. Neither touches the OS.
		if (!IsGameViewport(Viewport, Targets[Index].Widget))
		{
			const TSharedPtr<SWidget> Capturing = Reply.GetMouseCaptor();
			const FWidgetPath Path = PathUnder(Viewport, Screen);
			if (Capturing.IsValid() && User.IsValid() && Path.IsValid() && User->SetPointerCaptor(PointerIndex, Capturing.ToSharedRef(), Path))
			{
				Captor = Capturing;
			}
			if (const TSharedPtr<SWidget> Focus = Reply.GetUserFocusRecepient())
			{
				FSlateApplication::Get().SetUserFocus(UserIndex, Focus, EFocusCause::Mouse);
			}
		}
		break;
	}
	return true;
}

bool AgentInput::Wheel(const UGameViewportClient* Viewport, const FVector2D& Screen, float Delta)
{
	using namespace AgentInputPrivate;
	TArray<FArrangedWidget> Targets;
	if (!FSlateApplication::IsInitialized() || !FindTargets(Viewport, Screen, Targets))
	{
		return false;
	}
	const FPointerEvent Event = MakePointerEvent(Screen, TSet<FKey>(), EKeys::Invalid, Delta);
	for (int32 Index = Targets.Num() - 1; Index >= 0; --Index)
	{
		if (Targets[Index].Widget->OnMouseWheel(Targets[Index].Geometry, Event).IsEventHandled())
		{
			break;
		}
	}
	return true;
}

bool AgentInput::Key(const UGameViewportClient* Viewport, const FKey& InKey, bool bDown)
{
	const TSharedPtr<SViewport> ViewportWidget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	if (!FSlateApplication::IsInitialized() || !ViewportWidget.IsValid() || !InKey.IsValid())
	{
		return false;
	}
	const FKeyEvent Event(InKey, FModifierKeysState(), AgentInputPrivate::UserIndex, false, 0, 0);
	auto Send = [&Event, bDown](const TSharedPtr<SWidget>& Widget)
	{
		const FGeometry& Geometry = Widget->GetTickSpaceGeometry();
		return (bDown ? Widget->OnKeyDown(Geometry, Event) : Widget->OnKeyUp(Geometry, Event)).IsEventHandled();
	};

	// The focused widget first (a menu, a text box), then its parents, as Slate bubbles key events.
	bool bViewportVisited = false;
	TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetUserFocusedWidget(AgentInputPrivate::UserIndex);
	while (Widget.IsValid())
	{
		bViewportVisited |= Widget == ViewportWidget;
		if (Send(Widget))
		{
			return true;
		}
		Widget = Widget->GetParentWidget();
	}
	// Nothing focused handled it (in the background the game often has no focus): straight to the game.
	if (!bViewportVisited)
	{
		Send(ViewportWidget);
	}
	return true;
}

bool AgentInput::Axis(UGameViewportClient* Viewport, const FKey& InKey, float Value, float DeltaTime)
{
	if (!Viewport || !Viewport->Viewport || !InKey.IsValid())
	{
		return false;
	}
	const FInputDeviceId Device = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	Viewport->InputAxis(FInputKeyEventArgs(Viewport->Viewport, Device, InKey, Value, DeltaTime, 1, FPlatformTime::Cycles64()));
	return true;
}
