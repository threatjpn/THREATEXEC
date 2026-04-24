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

	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetUIOnlyInput(UUserWidget* FocusWidget, bool bShowCursor = true);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameOnlyInput(bool bShowCursor = true);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetGameAndUIInput(UUserWidget* FocusWidget, bool bShowCursor = true);

	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	void CachePrePhotoModeState();

	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCinematicViewTarget(AActor* NewViewTarget, float BlendTime = 0.35f);

	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool EnterPhotoMode(APawn* InPhotoPawn, float BlendTime = 0.35f, bool bPossessPhotoPawn = true);

	UFUNCTION(BlueprintCallable, Category = "Photo Mode")
	bool ExitPhotoMode(float BlendTime = 0.35f);

	// Use these for UI buttons too. They bypass keyboard-focus problems.
	UFUNCTION(BlueprintCallable, Category = "Bezier|History")
	bool BezierUndo();

	UFUNCTION(BlueprintCallable, Category = "Bezier|History")
	bool BezierRedo();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode")
	void BP_OpenPhotoModePlugin(APawn* InPhotoPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Photo Mode")
	void BP_ClosePhotoModePlugin(APawn* InPhotoPawn);

private:
	void Input_BezierUndo();
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
