#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GI_ThreatExec.generated.h"

class UFadeRefWidget;
class UFadeRefAsyncAction;

UCLASS()
class THREATEXEC_API UGI_ThreatExec : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void InitFadeWidget();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeIn();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeOut();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransition();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransitionToLevel(FName LevelName);

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void CancelFade();

    UFUNCTION(BlueprintPure, Category = "Fade")
    bool IsFadeBusy() const;

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade In And Wait"), Category = "Fade|Async")
    UFadeRefAsyncAction* FadeInAndWait();

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Out And Wait"), Category = "Fade|Async")
    UFadeRefAsyncAction* FadeOutAndWait();

    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Transition And Wait"), Category = "Fade|Async")
    UFadeRefAsyncAction* FadeTransitionAndWait();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    UFadeRefWidget* GetFadeWidget() const { return FadeWidget; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
    TSubclassOf<UFadeRefWidget> FadeWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fade")
    int32 FadeWidgetZOrder = 9999;

private:
    UPROPERTY(Transient)
    TObjectPtr<UFadeRefWidget> FadeWidget = nullptr;
};
