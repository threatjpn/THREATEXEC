/**
 * File: GI_ThreatExec.cpp
 * Summary: Implementation of the custom game instance fade and transition helpers.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "GI_ThreatExec.h"
#include "FadeRefWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"

void UGI_ThreatExec::Init()
{
    Super::Init();
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