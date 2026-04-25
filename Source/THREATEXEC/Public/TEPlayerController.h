/**
 * File: TEPlayerController.h
 * Summary: Project player controller for input modes, photo mode transitions and Bézier undo/redo commands.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TEPlayerController.generated.h"

class UUserWidget;
class APawn;
class AActor;

/** Player controller that coordinates input mode changes, photo-mode camera flow and Bézier edit shortcuts. */
UCLASS()
class THREATEXEC_API ATEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Sets default controller behaviour for UI, camera and editor-style commands. */
	ATEPlayerController();

	/** Binds project input actions, including Bézier undo and redo shortcuts. */
	virtual void SetupInputComponent() override;

	/** Switches input to UI-only mode and optionally focuses a widget. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetUIOnlyInput(UUserWidget* FocusWidget, bool bShowCursor = true);

	/** Switches input back to game-only control while preserving cursor preference. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameOnlyInput(bool bShowCursor = true);

	/** Enables combined game and UI input for mixed interaction screens. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameAndUIInput(UUserWidget* FocusWidget, bool bShowCursor = true);

	/** Stores the active pawn and camera target before entering photo mode. */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	void CachePrePhotoModeState();

	/** Blends the camera to a cinematic target without changing the active pawn. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCinematicViewTarget(AActor* NewViewTarget, float BlendTime = 0.35f);

	/** Enters photo mode by blending to and optionally possessing the photo pawn. */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool EnterPhotoMode(APawn* InPhotoPawn, float BlendTime = 0.35f, bool bPossessPhotoPawn = true);

	/** Restores the cached pawn and view target after photo mode. */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool ExitPhotoMode(float BlendTime = 0.35f);

	// Use these for UI buttons too. They bypass keyboard-focus problems.
	/** Executes Bézier undo through the active edit subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Bezier|History")
	bool BezierUndo();

	/** Executes Bézier redo through the active edit subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Bezier|History")
	bool BezierRedo();

protected:
	/** Blueprint hook for opening the external photo mode plugin UI. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode")
	void BP_OpenPhotoModePlugin(APawn* InPhotoPawn);

	/** Blueprint hook for closing the external photo mode plugin UI. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode")
	void BP_ClosePhotoModePlugin(APawn* InPhotoPawn);

private:
	/** Input callback for keyboard-driven Bézier undo. */
	void Input_BezierUndo();
	/** Input callback for keyboard-driven Bézier redo. */
	void Input_BezierRedo();

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> OriginalPawn;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousViewTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> PhotoPawn;

	UPROPERTY(Transient)
	bool bPhotoModeActive = false;
};
