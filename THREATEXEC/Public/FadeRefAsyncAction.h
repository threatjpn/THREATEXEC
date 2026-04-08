#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "FadeRefAsyncAction.generated.h"

class UFadeRefWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFadeRefAsyncOutput);

UENUM()
enum class EFadeRefAsyncMode : uint8
{
    FadeIn,
    FadeOut,
    Transition
};

UCLASS()
class THREATEXEC_API UFadeRefAsyncAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable)
    FFadeRefAsyncOutput Completed;

    UPROPERTY(BlueprintAssignable)
    FFadeRefAsyncOutput Failed;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade In And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeInAndWait(UFadeRefWidget* FadeWidget);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade Out And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeOutAndWait(UFadeRefWidget* FadeWidget);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade Transition And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeTransitionAndWait(UFadeRefWidget* FadeWidget);

    virtual void Activate() override;

private:
    static UFadeRefAsyncAction* CreateAction(UFadeRefWidget* FadeWidget, EFadeRefAsyncMode InMode);
    void CleanupBindings();

    UFUNCTION()
    void HandleFadeInFinished();

    UFUNCTION()
    void HandleFadeOutFinished();

    TWeakObjectPtr<UFadeRefWidget> TargetFadeWidget;
    EFadeRefAsyncMode Mode = EFadeRefAsyncMode::FadeIn;
    bool bCompleted = false;
};
