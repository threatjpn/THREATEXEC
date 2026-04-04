#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TEPlayerController.generated.h"

class UUserWidget;
class APawn;
class AActor;

UCLASS()
class THREATEXEC_API ATEPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATEPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetUIOnlyInput(UUserWidget* FocusWidget = nullptr, bool bShowCursor = true);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameOnlyInput(bool bShowCursor = false);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameAndUIInput(UUserWidget* FocusWidget = nullptr, bool bShowCursor = true);

	/** Manually sets a cinematic view target, useful for static cameras or level-sequence cameras. */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	void SetCinematicViewTarget(AActor* NewViewTarget, float BlendTime = 0.5f);

	/**
	 * Switches from the current cinematic or gameplay camera to a dedicated photo pawn.
	 * Optionally possesses the photo pawn so free camera controls work immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool EnterPhotoMode(APawn* InPhotoPawn, float BlendTime = 0.5f, bool bPossessPhotoPawn = true);

	/**
	 * Returns from the photo pawn back to the previously active camera target and original pawn.
	 */
	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool ExitPhotoMode(float BlendTime = 0.5f);

	UFUNCTION(BlueprintPure, Category = "Photo Mode")
	bool IsInPhotoMode() const { return bPhotoModeActive; }

	UFUNCTION(BlueprintPure, Category = "Photo Mode")
	APawn* GetPhotoPawn() const { return PhotoPawn.Get(); }

	UFUNCTION(BlueprintPure, Category = "Photo Mode")
	AActor* GetPreviousViewTarget() const { return PreviousViewTarget.Get(); }

protected:
	/**
	 * Implement this in your PlayerController Blueprint.
	 * From ActivePhotoPawn, get the plugin photo mode component and call its open photo mode logic.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode|Plugin")
	void BP_OpenPhotoModePlugin(APawn* ActivePhotoPawn);

	/**
	 * Optional close hook for the plugin UI.
	 * Safe to leave empty if your plugin handles closing internally.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode|Plugin")
	void BP_ClosePhotoModePlugin(APawn* ActivePhotoPawn);

private:
	void CachePrePhotoModeState();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> OriginalPawn;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousViewTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> PhotoPawn;

	UPROPERTY(Transient)
	bool bPhotoModeActive = false;
};