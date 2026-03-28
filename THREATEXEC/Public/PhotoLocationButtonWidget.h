#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationButtonWidget.generated.h"

class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPhotoLocationButtonEvent, FName, ButtonID);

UCLASS()
class YOURPROJECT_API UPhotoLocationButtonWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location")
    FName ButtonID = NAME_None;

    UPROPERTY(BlueprintAssignable, Category = "Photo Location")
    FPhotoLocationButtonEvent OnPhotoButtonHovered;

    UPROPERTY(BlueprintAssignable, Category = "Photo Location")
    FPhotoLocationButtonEvent OnPhotoButtonClicked;

    UFUNCTION(BlueprintCallable, Category = "Photo Location")
    void SetVisualHovered(bool bHovered);

    UFUNCTION(BlueprintCallable, Category = "Photo Location")
    void SetVisualSelected(bool bSelected);

protected:
    virtual void NativeOnInitialized() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainButton;

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Photo Location")
    void BP_SetVisualHovered(bool bHovered);

    UFUNCTION(BlueprintImplementableEvent, Category = "Photo Location")
    void BP_SetVisualSelected(bool bSelected);
};