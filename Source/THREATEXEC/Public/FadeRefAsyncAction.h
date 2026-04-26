/**
 * @file FadeRefAsyncAction.h
 * @brief Blueprint async action nodes that wrap fade and transition completion flows.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "FadeRefAsyncAction.generated.h"

class UFadeRefWidget;
class UGI_ThreatExec;

/**
 * Simple completion delegate used by the asynchronous fade helper nodes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFadeRefAsyncOutput);

/**
 * Internal operation mode for the async fade action.
 *
 * This is used to decide which fade workflow should be triggered when the
 * async action is activated.
 */
UENUM()
enum class EFadeRefAsyncMode : uint8
{
    /** Reveal the scene from black, then complete after any configured delay. */
    FadeIn,

    /** Fade the scene to black, then complete after any configured delay. */
    FadeOut,

    /** Run the widget's transition flow and complete when the flow finishes. */
    Transition
};

/**
 * Blueprint async action that wraps UFadeRefWidget and GI-driven fade flows.
 *
 * The class exposes latent-style Blueprint nodes such as Fade In And Wait,
 * Fade Out And Wait, and transition variants. It binds to fade completion
 * events, optionally waits for configured post-fade delays, then broadcasts
 * either Completed or Failed.
 */
UCLASS()
class THREATEXEC_API UFadeRefAsyncAction : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:
    /** Fired when the requested fade operation completes successfully. */
    UPROPERTY(BlueprintAssignable)
    FFadeRefAsyncOutput Completed;

    /** Fired when the action cannot start or complete correctly. */
    UPROPERTY(BlueprintAssignable)
    FFadeRefAsyncOutput Failed;

    /**
     * Start a widget-based fade-in workflow and complete once it is finished.
     *
     * @param FadeWidget Widget instance responsible for the fade presentation.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade In And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeInAndWait(UFadeRefWidget* FadeWidget);

    /**
     * Start a widget-based fade-out workflow and complete once it is finished.
     *
     * @param FadeWidget Widget instance responsible for the fade presentation.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade Out And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeOutAndWait(UFadeRefWidget* FadeWidget);

    /**
     * Start a widget-based transition workflow and complete when it finishes.
     *
     * @param FadeWidget Widget instance responsible for the fade presentation.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "Fade Transition And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* FadeTransitionAndWait(UFadeRefWidget* FadeWidget);

    /**
     * Start a GameInstance-managed fade-in workflow and complete on success.
     *
     * @param ThreatExecGameInstance Active game instance that owns the fade flow.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade In And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeInAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    /**
     * Start a GameInstance-managed fade-out workflow and complete on success.
     *
     * @param ThreatExecGameInstance Active game instance that owns the fade flow.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Out And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeOutAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    /**
     * Start a GameInstance-managed transition workflow and complete on success.
     *
     * @param ThreatExecGameInstance Active game instance that owns the fade flow.
     * @return Newly created async action object.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DisplayName = "GI Fade Transition And Wait"), Category = "Fade|Async")
    static UFadeRefAsyncAction* GIFadeTransitionAndWait(UGI_ThreatExec* ThreatExecGameInstance);

    /**
     * Begin execution of the configured fade workflow.
     */
    virtual void Activate() override;

private:
    /**
     * Shared helper used by the widget-based factory functions.
     *
     * @param FadeWidget Target widget that performs the fade.
     * @param InMode Requested async mode.
     * @return Newly configured action instance.
     */
    static UFadeRefAsyncAction* CreateAction(UFadeRefWidget* FadeWidget, EFadeRefAsyncMode InMode);

    /** Remove any active delegate bindings and timers owned by this action. */
    void CleanupBindings();

    /**
     * Delay final completion by a fixed interval.
     *
     * @param DelaySeconds Additional wait time before broadcasting Completed.
     */
    void BeginCompletionAfterDelay(float DelaySeconds);

    /** Mark the action as successful and broadcast the completion event. */
    void CompleteAction();

    /** Mark the action as failed and broadcast the failure event. */
    void FailAction();

    /** Callback for widget fade-in completion notifications. */
    UFUNCTION()
    void HandleFadeInFinished();

    /** Callback for widget fade-out completion notifications. */
    UFUNCTION()
    void HandleFadeOutFinished();

    /** Timer callback used for deferred completion after a wait period. */
    UFUNCTION()
    void HandleCompletionDelayElapsed();

    /** Weak reference to the widget that owns the fade presentation. */
    TWeakObjectPtr<UFadeRefWidget> TargetFadeWidget;

    /** Requested fade workflow for this async action instance. */
    EFadeRefAsyncMode Mode = EFadeRefAsyncMode::FadeIn;

    /** Guards against duplicate completion/failure broadcasts. */
    bool bCompleted = false;

    /** Timer used when completion must be delayed after the fade ends. */
    FTimerHandle CompletionDelayHandle;
};
