// ============================================================================
// FadeRefAsyncAction.cpp
// Wraps fade-widget transitions as Blueprint async actions.
// Comments are documentation-only and do not alter behaviour.
// ============================================================================

#include "FadeRefAsyncAction.h"

#include "FadeRefWidget.h"
#include "GI_ThreatExec.h"

#include "Engine/World.h"
#include "TimerManager.h"

// Runs a fade-in transition and exposes completion to Blueprint.
UFadeRefAsyncAction* UFadeRefAsyncAction::FadeInAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::FadeIn);
}

// Runs a fade-out transition and exposes completion to Blueprint.
UFadeRefAsyncAction* UFadeRefAsyncAction::FadeOutAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::FadeOut);
}

// Handles fade transition and wait.
UFadeRefAsyncAction* UFadeRefAsyncAction::FadeTransitionAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::Transition);
}

// Handles gifade in and wait.
UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeInAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::FadeIn);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::FadeIn);
}

// Handles gifade out and wait.
UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeOutAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::FadeOut);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::FadeOut);
}

// Handles gifade transition and wait.
UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeTransitionAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::Transition);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::Transition);
}

// Creates the requested runtime object or widget.
UFadeRefAsyncAction* UFadeRefAsyncAction::CreateAction(UFadeRefWidget* FadeWidget, EFadeRefAsyncMode InMode)
{
    UFadeRefAsyncAction* Action = NewObject<UFadeRefAsyncAction>();
    Action->TargetFadeWidget = FadeWidget;
    Action->Mode = InMode;

    if (FadeWidget)
    {
        Action->RegisterWithGameInstance(FadeWidget);
    }

    return Action;
}

// Handles activate.
void UFadeRefAsyncAction::Activate()
{
    UFadeRefWidget* FadeWidget = TargetFadeWidget.Get();
    if (!FadeWidget)
    {
        FailAction();
        return;
    }

    CleanupBindings();

    switch (Mode)
    {
    case EFadeRefAsyncMode::FadeIn:
        FadeWidget->OnFadeOutFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeOutFinished);
        FadeWidget->FadeIn();
        break;

    case EFadeRefAsyncMode::FadeOut:
        FadeWidget->OnFadeInFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeInFinished);
        FadeWidget->FadeOut();
        break;

    case EFadeRefAsyncMode::Transition:
        FadeWidget->OnFadeInFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeInFinished);
        FadeWidget->FadeTransition();
        break;

    default:
        FailAction();
        break;
    }
}

// Handles cleanup bindings.
void UFadeRefAsyncAction::CleanupBindings()
{
    if (UFadeRefWidget* FadeWidget = TargetFadeWidget.Get())
    {
        FadeWidget->OnFadeInFinished.RemoveDynamic(this, &UFadeRefAsyncAction::HandleFadeInFinished);
        FadeWidget->OnFadeOutFinished.RemoveDynamic(this, &UFadeRefAsyncAction::HandleFadeOutFinished);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CompletionDelayHandle);
    }
}

// Starts the named operation and initialises its state.
void UFadeRefAsyncAction::BeginCompletionAfterDelay(float DelaySeconds)
{
    if (bCompleted)
    {
        return;
    }

    if (DelaySeconds <= KINDA_SMALL_NUMBER)
    {
        CompleteAction();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        CompleteAction();
        return;
    }

    World->GetTimerManager().SetTimer(
        CompletionDelayHandle,
        this,
        &UFadeRefAsyncAction::HandleCompletionDelayElapsed,
        DelaySeconds,
        false
    );
}

// Handles complete action.
void UFadeRefAsyncAction::CompleteAction()
{
    if (bCompleted)
    {
        return;
    }

    bCompleted = true;
    CleanupBindings();
    Completed.Broadcast();
    SetReadyToDestroy();
}

// Handles fail action.
void UFadeRefAsyncAction::FailAction()
{
    if (bCompleted)
    {
        return;
    }

    bCompleted = true;
    CleanupBindings();
    Failed.Broadcast();
    SetReadyToDestroy();
}

// Handles an event from UI, input, or runtime state.
void UFadeRefAsyncAction::HandleFadeInFinished()
{
    UFadeRefWidget* FadeWidget = TargetFadeWidget.Get();
    if (!FadeWidget)
    {
        FailAction();
        return;
    }

    switch (Mode)
    {
    case EFadeRefAsyncMode::FadeOut:
        BeginCompletionAfterDelay(FadeWidget->GetFadeOutCompletedWaitSeconds());
        break;

    case EFadeRefAsyncMode::Transition:
        CompleteAction();
        break;

    default:
        CompleteAction();
        break;
    }
}

// Handles an event from UI, input, or runtime state.
void UFadeRefAsyncAction::HandleFadeOutFinished()
{
    UFadeRefWidget* FadeWidget = TargetFadeWidget.Get();
    if (!FadeWidget)
    {
        FailAction();
        return;
    }

    switch (Mode)
    {
    case EFadeRefAsyncMode::FadeIn:
        BeginCompletionAfterDelay(FadeWidget->GetFadeInCompletedWaitSeconds());
        break;

    default:
        CompleteAction();
        break;
    }
}

// Handles an event from UI, input, or runtime state.
void UFadeRefAsyncAction::HandleCompletionDelayElapsed()
{
    CompleteAction();
}
