#include "TEPlayerController.h"

#include "Blueprint/UserWidget.h"

ATEPlayerController::ATEPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	DefaultMouseCursor = EMouseCursor::Default;
}

void ATEPlayerController::SetUIOnlyInput(UUserWidget* FocusWidget, bool bShowCursor)
{
	FInputModeUIOnly InputMode;
	if (FocusWidget)
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bShowMouseCursor = bShowCursor;
}

void ATEPlayerController::SetGameOnlyInput(bool bShowCursor)
{
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = bShowCursor;
}

void ATEPlayerController::SetGameAndUIInput(UUserWidget* FocusWidget, bool bShowCursor)
{
	FInputModeGameAndUI InputMode;
	if (FocusWidget)
	{
		InputMode.SetWidgetToFocus(FocusWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bShowMouseCursor = bShowCursor;
}
