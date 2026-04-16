#include "FadeRefAsyncAction.h"

#include "FadeRefWidget.h"
#include "GI_ThreatExec.h"

#include "Engine/World.h"
#include "TimerManager.h"

UFadeRefAsyncAction* UFadeRefAsyncAction::FadeInAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::FadeIn);
}

UFadeRefAsyncAction* UFadeRefAsyncAction::FadeOutAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::FadeOut);
}

UFadeRefAsyncAction* UFadeRefAsyncAction::FadeTransitionAndWait(UFadeRefWidget* FadeWidget)
{
    return CreateAction(FadeWidget, EFadeRefAsyncMode::Transition);
}

UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeInAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::FadeIn);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::FadeIn);
}

UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeOutAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::FadeOut);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::FadeOut);
}

UFadeRefAsyncAction* UFadeRefAsyncAction::GIFadeTransitionAndWait(UGI_ThreatExec* ThreatExecGameInstance)
{
    if (!ThreatExecGameInstance)
    {
        return CreateAction(nullptr, EFadeRefAsyncMode::Transition);
    }

    ThreatExecGameInstance->InitFadeWidget();
    return CreateAction(ThreatExecGameInstance->GetFadeWidget(), EFadeRefAsyncMode::Transition);
}

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

void UFadeRefAsyncAction::HandleCompletionDelayElapsed()
{
    CompleteAction();
}