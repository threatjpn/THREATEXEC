#include "FadeRefAsyncAction.h"

#include "FadeRefWidget.h"
#include "GI_ThreatExec.h"

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
    return Action;
}

void UFadeRefAsyncAction::Activate()
{
    UFadeRefWidget* FadeWidget = TargetFadeWidget.Get();
    if (!FadeWidget)
    {
        Failed.Broadcast();
        SetReadyToDestroy();
        return;
    }

    CleanupBindings();

    switch (Mode)
    {
    case EFadeRefAsyncMode::FadeIn:
        FadeWidget->OnFadeInFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeInFinished);
        FadeWidget->FadeIn();
        break;

    case EFadeRefAsyncMode::FadeOut:
        FadeWidget->OnFadeOutFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeOutFinished);
        FadeWidget->FadeOut();
        break;

    case EFadeRefAsyncMode::Transition:
        // For wait-style transition flows we complete when full black is reached,
        // so caller logic can continue while still covered by fade.
        FadeWidget->OnFadeInFinished.AddDynamic(this, &UFadeRefAsyncAction::HandleFadeInFinished);
        FadeWidget->FadeTransition();
        break;

    default:
        Failed.Broadcast();
        SetReadyToDestroy();
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
}

void UFadeRefAsyncAction::HandleFadeInFinished()
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

void UFadeRefAsyncAction::HandleFadeOutFinished()
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
