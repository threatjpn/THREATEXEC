#include "GI_ThreatExec.h"
#include "FadeRefAsyncAction.h"
#include "FadeRefWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UGI_ThreatExec::Init()
{
    Super::Init();

    // Optional:
    // You can leave this here, but in many projects it is safer to also call
    // InitFadeWidget from PlayerController BeginPlay so it always exists after level load.
}

void UGI_ThreatExec::InitFadeWidget()
{
    if (FadeWidget && IsValid(FadeWidget))
    {
        if (!FadeWidget->IsInViewport())
        {
            FadeWidget->AddToViewport(FadeWidgetZOrder);
        }
        return;
    }

    if (!FadeWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGI_ThreatExec::InitFadeWidget - FadeWidgetClass is not set."));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGI_ThreatExec::InitFadeWidget - World is invalid."));
        return;
    }

    FadeWidget = CreateWidget<UFadeRefWidget>(World, FadeWidgetClass);
    if (!FadeWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("UGI_ThreatExec::InitFadeWidget - Failed to create FadeWidget."));
        return;
    }

    FadeWidget->AddToViewport(FadeWidgetZOrder);
}

void UGI_ThreatExec::FadeIn()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeIn();
    }
}

void UGI_ThreatExec::FadeOut()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeOut();
    }
}

void UGI_ThreatExec::FadeTransition()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeTransition();
    }
}

void UGI_ThreatExec::FadeTransitionToLevel(FName LevelName)
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeTransitionToLevel(LevelName);
    }
}

void UGI_ThreatExec::CancelFade()
{
    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->CancelFade();
    }
}

bool UGI_ThreatExec::IsFadeBusy() const
{
    return FadeWidget && IsValid(FadeWidget) && FadeWidget->IsFadeBusy();
}

UFadeRefAsyncAction* UGI_ThreatExec::FadeInAndWait()
{
    InitFadeWidget();
    return UFadeRefAsyncAction::FadeInAndWait(FadeWidget);
}

UFadeRefAsyncAction* UGI_ThreatExec::FadeOutAndWait()
{
    InitFadeWidget();
    return UFadeRefAsyncAction::FadeOutAndWait(FadeWidget);
}

UFadeRefAsyncAction* UGI_ThreatExec::FadeTransitionAndWait()
{
    InitFadeWidget();
    return UFadeRefAsyncAction::FadeTransitionAndWait(FadeWidget);
}
