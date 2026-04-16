#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "FadeRefAsyncAction.generated.h"

class UFadeRefWidget;
class UGI_ThreatExec;

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

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade In And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeInAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Out And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeOutAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Transition And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeTransitionAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    virtual void Activate() override;

private:
    static UFadeRefAsyncAction* CreateAction(UFadeRefWidget* FadeWidget, EFadeRefAsyncMode InMode);

    void CleanupBindings();
    void BeginCompletionAfterDelay(float DelaySeconds);
    void CompleteAction();
    void FailAction();

    UFUNCTION()
    void HandleFadeInFinished();

    UFUNCTION()
    void HandleFadeOutFinished();

    UFUNCTION()
    void HandleCompletionDelayElapsed();

    TWeakObjectPtr<UFadeRefWidget> TargetFadeWidget;
    EFadeRefAsyncMode Mode = EFadeRefAsyncMode::FadeIn;
    bool bCompleted = false;
    FTimerHandle CompletionDelayHandle;
};