#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FadeRefWidget.generated.h"

class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFadeRefSimpleEvent);

UENUM(BlueprintType)
enum class EFadeRefState : uint8
{
    Idle,
    FadingIn,
    FadingOut,
    TransitionIn,
    TransitionOut,
    LevelSwitchIn,
    LevelSwitchDelay
};

UCLASS()
class THREATEXEC_API UFadeRefWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeIn();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeOut();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransition();

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransitionToLevel(FName LevelName);

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void SetLoadingIconVisible(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "Fade")
    void CancelFade();

    UFUNCTION(BlueprintPure, Category = "Fade")
    bool IsFadeBusy() const;

    UPROPERTY(BlueprintAssignable, Category = "Fade")
    FFadeRefSimpleEvent OnFadeInFinished;

    UPROPERTY(BlueprintAssignable, Category = "Fade")
    FFadeRefSimpleEvent OnFadeOutFinished;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Fade")
    TObjectPtr<UWidget> FadeLayer;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Fade")
    TObjectPtr<UWidget> LoadingIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.01"))
    float FadeDurationSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.0"))
    float LoadingSpinDegreesPerSecond = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float FadeInDelaySeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float FadeOutDelaySeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float TransitionHoldBlackSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float LevelLoadDelaySeconds = 0.0f;

private:
    void StartFadeWithDelay(EFadeRefState NewState, float DelaySeconds);
    void StartFade(EFadeRefState NewState);
    void ApplyFadeAlpha(float Alpha);
    void ExecutePendingLevelTravel();
    void FinishFadeStep();

    EFadeRefState FadeState = EFadeRefState::Idle;
    EFadeRefState DelayedFadeState = EFadeRefState::Idle;
    float FadeElapsedSeconds = 0.0f;
    float FadeDelayRemainingSeconds = 0.0f;
    float CurrentFadeAlpha = 0.0f;
    float LoadingIconRotation = 0.0f;
    bool bShowLoadingIcon = false;
    FName PendingLevelToLoad = NAME_None;
};
