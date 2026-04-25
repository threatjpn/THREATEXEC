// ============================================================================
// TEPlayerController.cpp
// Implements player-controller helpers for input-mode management, view transitions,
// photo-mode flow, and global Bezier undo/redo shortcuts.
// ============================================================================

#include "TEPlayerController.h"

#include "BezierEditSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

ATEPlayerController::ATEPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	DefaultMouseCursor = EMouseCursor::Default;
}

void ATEPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	// Chord bindings for normal game input focus.
	FInputKeyBinding UndoChord(FInputChord(EKeys::Z, true, false, false, false), IE_Pressed);
	UndoChord.KeyDelegate.BindDelegate(this, &ATEPlayerController::Input_BezierUndo);
	InputComponent->KeyBindings.Add(MoveTemp(UndoChord));

	FInputKeyBinding RedoChord(FInputChord(EKeys::Y, true, false, false, false), IE_Pressed);
	RedoChord.KeyDelegate.BindDelegate(this, &ATEPlayerController::Input_BezierRedo);
	InputComponent->KeyBindings.Add(MoveTemp(RedoChord));

	// Plain-key fallback. The handlers still require Ctrl, so Z/Y alone do nothing.
	InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ATEPlayerController::Input_BezierUndo);
	InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ATEPlayerController::Input_BezierRedo);
}

void ATEPlayerController::Input_BezierUndo()
{
	const bool bCtrlDown = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
	const bool bShiftDown = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

	if (!bCtrlDown || bShiftDown)
	{
		return;
	}

	BezierUndo();
}

void ATEPlayerController::Input_BezierRedo()
{
	const bool bCtrlDown = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);

	if (!bCtrlDown)
	{
		return;
	}

	BezierRedo();
}

bool ATEPlayerController::BezierUndo()
{
	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Sub->History_Undo();
	}

	return false;
}

bool ATEPlayerController::BezierRedo()
{
	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Sub->History_Redo();
	}

	return false;
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
