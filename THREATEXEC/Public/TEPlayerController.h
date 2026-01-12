#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TEPlayerController.generated.h"

class UUserWidget;

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
};
