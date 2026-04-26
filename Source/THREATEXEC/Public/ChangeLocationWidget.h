/**
 * @file ChangeLocationWidget.h
 * @brief Widget that binds change-location entry widgets and forwards variant selection requests to Blueprint and external listeners.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChangeLocationWidget.generated.h"

class UPanelWidget;
class UWidget;
class UChangeLocationEntryWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLocationRequestedSignature, FName, VariantID);

/** Widget that discovers location-entry children and routes their selection events to listeners. */
UCLASS()
class THREATEXEC_API UChangeLocationWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Change Location")
    FChangeLocationRequestedSignature OnChangeLocationRequested;

    /** Re-collects child entry widgets and binds their click/selection delegates. */
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