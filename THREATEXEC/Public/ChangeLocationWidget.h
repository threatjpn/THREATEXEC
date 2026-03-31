#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChangeLocationWidget.generated.h"

class UButton;
class UPanelWidget;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLocationRequestedSignature, FName, VariantID);

UCLASS()
class THREATEXEC_API UChangeLocationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Change Location")
    FChangeLocationRequestedSignature OnChangeLocationRequested;

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    void RebindLocationButtons();

protected:
    virtual void NativeOnInitialized() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> CL_LIST;

    UFUNCTION(BlueprintImplementableEvent, Category = "Change Location")
    void BP_OnChangeLocationRequested(FName VariantID);

private:
    UPROPERTY()
    TArray<TObjectPtr<UButton>> CachedButtons;

    UPROPERTY()
    TObjectPtr<UButton> LastHoveredButton = nullptr;

    void CollectButtonsRecursive(UWidget* RootWidget, TArray<UButton*>& OutButtons);

    UFUNCTION()
    void HandleAnyButtonHovered();

    UFUNCTION()
    void HandleAnyButtonClicked();

    UButton* ResolveHoveredButton() const;
};