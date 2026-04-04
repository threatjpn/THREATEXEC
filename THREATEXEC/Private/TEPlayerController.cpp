#include "TEPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"

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

void ATEPlayerController::CachePrePhotoModeState()
{
	OriginalPawn = GetPawn();
	PreviousViewTarget = GetViewTarget();
}

void ATEPlayerController::SetCinematicViewTarget(AActor* NewViewTarget, float BlendTime)
{
	if (!IsValid(NewViewTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("ATEPlayerController::SetCinematicViewTarget - NewViewTarget is null."));
		return;
	}

	SetViewTargetWithBlend(NewViewTarget, FMath::Max(0.0f, BlendTime));
}

bool ATEPlayerController::EnterPhotoMode(APawn* InPhotoPawn, float BlendTime, bool bPossessPhotoPawn)
{
	if (!IsValid(InPhotoPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("ATEPlayerController::EnterPhotoMode - InPhotoPawn is null."));
		return false;
	}

	if (bPhotoModeActive && PhotoPawn.Get() == InPhotoPawn)
	{
		BP_OpenPhotoModePlugin(InPhotoPawn);
		return true;
	}

	CachePrePhotoModeState();

	PhotoPawn = InPhotoPawn;

	SetViewTargetWithBlend(InPhotoPawn, FMath::Max(0.0f, BlendTime));

	if (bPossessPhotoPawn)
	{
		Possess(InPhotoPawn);
	}

	SetGameAndUIInput(nullptr, true);

	bPhotoModeActive = true;

	BP_OpenPhotoModePlugin(InPhotoPawn);

	UE_LOG(LogTemp, Log, TEXT("ATEPlayerController::EnterPhotoMode - switched to %s"), *InPhotoPawn->GetName());
	return true;
}

bool ATEPlayerController::ExitPhotoMode(float BlendTime)
{
	if (!bPhotoModeActive)
	{
		return false;
	}

	APawn* ActivePhotoPawn = PhotoPawn.Get();
	if (ActivePhotoPawn)
	{
		BP_ClosePhotoModePlugin(ActivePhotoPawn);
	}

	APawn* SavedOriginalPawn = OriginalPawn.Get();
	AActor* SavedViewTarget = PreviousViewTarget.Get();

	if (SavedOriginalPawn && GetPawn() != SavedOriginalPawn)
	{
		Possess(SavedOriginalPawn);
	}

	if (SavedViewTarget)
	{
		SetViewTargetWithBlend(SavedViewTarget, FMath::Max(0.0f, BlendTime));
	}
	else if (SavedOriginalPawn)
	{
		SetViewTargetWithBlend(SavedOriginalPawn, FMath::Max(0.0f, BlendTime));
	}

	SetGameAndUIInput(nullptr, true);

	bPhotoModeActive = false;
	PhotoPawn = nullptr;
	OriginalPawn = nullptr;
	PreviousViewTarget = nullptr;

	UE_LOG(LogTemp, Log, TEXT("ATEPlayerController::ExitPhotoMode - restored previous camera state."));
	return true;
}