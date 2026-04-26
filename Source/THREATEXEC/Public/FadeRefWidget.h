/**
 * @file FadeRefWidget.h
 * @brief Reusable fade overlay widget for screen fades, transitions and level travel.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FadeRefWidget.generated.h"

class UWidget;

/**
 * Lightweight event used for fade lifecycle notifications.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFadeRefSimpleEvent);

/**
 * Internal state machine used by the fade widget.
 */
UENUM(BlueprintType)
enum class EFadeRefState : uint8
{
    /** No fade is currently active. */
    Idle,

    /** Internal state: fade layer opacity moves from 0 to 1. */
    FadingIn,

    /** Internal state: fade layer opacity moves from 1 to 0. */
    FadingOut,

    /** First phase of a transition sequence. */
    TransitionIn,

    /** Return phase of a transition sequence. */
    TransitionOut,

    /** Fade phase used before a level switch is executed. */
    LevelSwitchIn,

    /** Waiting state used before performing delayed level travel. */
    LevelSwitchDelay
};

/**
 * Reusable fade overlay widget for screen transitions and loading feedback.
 *
 * The widget drives a small internal state machine that can fade the screen
 * in or out, perform chained transition flows, and optionally coordinate a
 * delayed level travel while displaying a loading indicator.
 */
UCLASS()
class THREATEXEC_API UFadeRefWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Public meaning:
    // FadeIn  = reveal scene from black
    // FadeOut = cover scene to black

    /** Reveal the scene from black. */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeIn();

    /** Cover the scene to black. */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeOut();

    /** Execute the widget's built-in transition sequence. */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransition();

    /**
     * Fade to black and then travel to the requested level.
     *
     * @param LevelName Name of the map level to load.
     */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void FadeTransitionToLevel(FName LevelName);

    /** Show or hide the optional loading icon. */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void SetLoadingIconVisible(bool bVisible);

    /** Cancel any active fade sequence and return the widget to an idle state. */
    UFUNCTION(BlueprintCallable, Category = "Fade")
    void CancelFade();

    /**
     * Check whether the widget is currently processing a fade sequence.
     *
     * @return True when the widget is in any non-idle fade state.
     */
    UFUNCTION(BlueprintPure, Category = "Fade")
    bool IsFadeBusy() const;

    /**
     * Post-fade wait used by the async FadeInAndWait helper node.
     *
     * @return Additional delay in seconds after fade-in completes.
     */
    UFUNCTION(BlueprintPure, Category = "Fade|Async")
    float GetFadeInCompletedWaitSeconds() const { return FadeInCompletedWaitSeconds; }

    /**
     * Post-fade wait used by the async FadeOutAndWait helper node.
     *
     * @return Additional delay in seconds after fade-out completes.
     */
    UFUNCTION(BlueprintPure, Category = "Fade|Async")
    float GetFadeOutCompletedWaitSeconds() const { return FadeOutCompletedWaitSeconds; }

    /** Broadcast when the fade-in sequence fully completes. */
    UPROPERTY(BlueprintAssignable, Category = "Fade")
    FFadeRefSimpleEvent OnFadeInFinished;

    /** Broadcast when the fade-out sequence fully completes. */
    UPROPERTY(BlueprintAssignable, Category = "Fade")
    FFadeRefSimpleEvent OnFadeOutFinished;

protected:
    /** Initialise widget state and bound UI elements. */
    virtual void NativeConstruct() override;

    /** Advance the fade state machine and visual animation each frame. */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Optional overlay widget whose opacity represents the fade amount. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Fade")
    TObjectPtr<UWidget> FadeLayer;

    /** Optional loading indicator shown during selected transition flows. */
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Fade")
    TObjectPtr<UWidget> LoadingIcon;

    /** Total duration for an individual fade animation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.01"))
    float FadeDurationSeconds = 2.0f;

    /** Rotation speed applied to the loading icon while active. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade", meta = (ClampMin = "0.0"))
    float LoadingSpinDegreesPerSecond = 180.0f;

    /** Delay before the fade-in animation starts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float FadeInDelaySeconds = 0.0f;

    /** Delay before the fade-out animation starts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float FadeOutDelaySeconds = 0.0f;

    /** Time to remain fully black during automatic transition sequences. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float TransitionHoldBlackSeconds = 0.0f;

    /** Delay before executing an actual level load request. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Delay", meta = (ClampMin = "0.0"))
    float LevelLoadDelaySeconds = 0.0f;

    /** Additional wait after a fade-out completes before async completion fires. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Async", meta = (ClampMin = "0.0"))
    float FadeOutCompletedWaitSeconds = 2.0f;

    /** Additional wait after a fade-in completes before async completion fires. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fade|Async", meta = (ClampMin = "0.0"))
    float FadeInCompletedWaitSeconds = 0.0f;

private:
    /**
     * Queue a fade state change after an optional delay.
     *
     * @param NewState State to start once the delay expires.
     * @param DelaySeconds Time in seconds before the state is activated.
     */
    void StartFadeWithDelay(EFadeRefState NewState, float DelaySeconds);

    /**
     * Immediately enter a new fade state.
     *
     * @param NewState State to begin processing.
     */
    void StartFade(EFadeRefState NewState);

    /**
     * Apply the current fade alpha to the visual fade layer.
     *
     * @param Alpha Normalised fade value expected in the range [0, 1].
     */
    void ApplyFadeAlpha(float Alpha);

    /** Execute any deferred level travel request once the fade permits it. */
    void ExecutePendingLevelTravel();

    /** Finalise the current state and advance the fade sequence when needed. */
    void FinishFadeStep();

    /** Current active fade state. */
    EFadeRefState FadeState = EFadeRefState::Idle;

    /** State scheduled to begin once a start delay has elapsed. */
    EFadeRefState DelayedFadeState = EFadeRefState::Idle;

    /** Time accumulator for the active fade animation. */
    float FadeElapsedSeconds = 0.0f;

    /** Remaining time before a delayed fade state begins. */
    float FadeDelayRemainingSeconds = 0.0f;

    /** Cached fade alpha currently applied to the overlay. */
    float CurrentFadeAlpha = 0.0f;

    /** Runtime rotation accumulator for the loading indicator. */
    float LoadingIconRotation = 0.0f;

    /** Whether the loading icon should currently be visible. */
    bool bShowLoadingIcon = false;

    /** Optional level name to load after the black-screen phase completes. */
    FName PendingLevelToLoad = NAME_None;
};
