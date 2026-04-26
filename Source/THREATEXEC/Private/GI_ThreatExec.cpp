// ============================================================================
// GI_ThreatExec.cpp
// Provides global fade and transition helpers through the game instance.
// ============================================================================

#include "GI_ThreatExec.h"
#include "FadeRefWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

// Handles init.
void UGI_ThreatExec::Init()
{
    Super::Init();
}

// Handles init fade widget.
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

// Runs the fade overlay toward visible opacity.
void UGI_ThreatExec::FadeIn()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeIn();
    }
}

// Runs the fade overlay toward hidden opacity.
void UGI_ThreatExec::FadeOut()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeOut();
    }
}

// Handles fade transition.
void UGI_ThreatExec::FadeTransition()
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeTransition();
    }
}

// Handles fade transition to level.
void UGI_ThreatExec::FadeTransitionToLevel(FName LevelName)
{
    InitFadeWidget();

    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->FadeTransitionToLevel(LevelName);
    }
}

// Checks whether cancel fade.
void UGI_ThreatExec::CancelFade()
{
    if (FadeWidget && IsValid(FadeWidget))
    {
        FadeWidget->CancelFade();
    }
}

// Checks whether is fade busy.
bool UGI_ThreatExec::IsFadeBusy() const
{
    return FadeWidget && IsValid(FadeWidget) && FadeWidget->IsFadeBusy();
}
