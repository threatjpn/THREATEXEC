#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChangeLocationWidget.generated.h"

class UPanelWidget;
class UWidget;
class UChangeLocationEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLocationRequestedSignature, FName, VariantID);

UCLASS()
class THREATEXEC_API UChangeLocationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Change Location")
    FChangeLocationRequestedSignature OnChangeLocationRequested;

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    void RebindLocationEntries();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> CL_LIST;

    UFUNCTION(BlueprintImplementableEvent, Category = "Change Location")
    void BP_OnChangeLocationRequested(FName VariantID);

private:
    UPROPERTY()
    TArray<TObjectPtr<UChangeLocationEntryWidget>> CachedEntries;

    void CollectEntriesRecursive(UWidget* RootWidget, TArray<UChangeLocationEntryWidget*>& OutEntries);

    UFUNCTION()
    void HandleEntryHovered(FName VariantID);

    UFUNCTION()
    void HandleEntryClicked(FName VariantID);
};