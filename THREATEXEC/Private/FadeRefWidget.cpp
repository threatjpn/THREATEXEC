#include "FadeRefWidget.h"

#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"

void UFadeRefWidget::NativeConstruct()
{
    Super::NativeConstruct();

    FadeState = EFadeRefState::Idle;
    DelayedFadeState = EFadeRefState::Idle;
    FadeElapsedSeconds = 0.0f;
    FadeDelayRemainingSeconds = 0.0f;
    CurrentFadeAlpha = 0.0f;
    LoadingIconRotation = 0.0f;

    ApplyFadeAlpha(0.0f);
    SetLoadingIconVisible(false);
}

void UFadeRefWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (FadeDelayRemainingSeconds > 0.0f)
    {
        FadeDelayRemainingSeconds = FMath::Max(0.0f, FadeDelayRemainingSeconds - InDeltaTime);
        if (FadeDelayRemainingSeconds <= KINDA_SMALL_NUMBER && DelayedFadeState != EFadeRefState::Idle)
        {
            const EFadeRefState NextState = DelayedFadeState;
            DelayedFadeState = EFadeRefState::Idle;
            StartFade(NextState);
        }
    }

    if (FadeState != EFadeRefState::Idle && FadeState != EFadeRefState::LevelSwitchDelay)
    {
        FadeElapsedSeconds += InDeltaTime;

        const float SafeDuration = FMath::Max(0.01f, FadeDurationSeconds);
        const float Progress = FMath::Clamp(FadeElapsedSeconds / SafeDuration, 0.0f, 1.0f);

        switch (FadeState)
        {
        case EFadeRefState::FadingIn:
        case EFadeRefState::TransitionIn:
        case EFadeRefState::LevelSwitchIn:
            ApplyFadeAlpha(Progress);
            break;
        case EFadeRefState::FadingOut:
        case EFadeRefState::TransitionOut:
            ApplyFadeAlpha(1.0f - Progress);
            break;
        default:
            break;
        }

        if (Progress >= 1.0f)
        {
            FinishFadeStep();
        }
    }
    else if (FadeState == EFadeRefState::LevelSwitchDelay)
    {
        FadeElapsedSeconds += InDeltaTime;
        if (FadeElapsedSeconds >= LevelLoadDelaySeconds)
        {
            ExecutePendingLevelTravel();
            FadeState = EFadeRefState::Idle;
        }
    }

    if (LoadingIcon)
    {
        if (bShowLoadingIcon)
        {
            LoadingIconRotation = FMath::Fmod(LoadingIconRotation + (LoadingSpinDegreesPerSecond * InDeltaTime), 360.0f);
            FWidgetTransform Transform = LoadingIcon->GetRenderTransform();
            Transform.Angle = LoadingIconRotation;
            LoadingIcon->SetRenderTransform(Transform);
        }

        LoadingIcon->SetVisibility(bShowLoadingIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
}

void UFadeRefWidget::FadeIn()
{
    PendingLevelToLoad = NAME_None;
    SetLoadingIconVisible(false);
    StartFadeWithDelay(EFadeRefState::FadingIn, FadeInDelaySeconds);
}

void UFadeRefWidget::FadeOut()
{
    PendingLevelToLoad = NAME_None;
    SetLoadingIconVisible(false);
    StartFadeWithDelay(EFadeRefState::FadingOut, FadeOutDelaySeconds);
}

void UFadeRefWidget::FadeTransition()
{
    PendingLevelToLoad = NAME_None;
    SetLoadingIconVisible(false);
    StartFadeWithDelay(EFadeRefState::TransitionIn, FadeInDelaySeconds);
}

void UFadeRefWidget::FadeTransitionToLevel(FName LevelName)
{
    PendingLevelToLoad = LevelName;
    SetLoadingIconVisible(true);
    StartFadeWithDelay(EFadeRefState::LevelSwitchIn, FadeInDelaySeconds);
}

void UFadeRefWidget::SetLoadingIconVisible(bool bVisible)
{
    bShowLoadingIcon = bVisible;

    if (!bShowLoadingIcon)
    {
        LoadingIconRotation = 0.0f;
        if (LoadingIcon)
        {
            FWidgetTransform Transform = LoadingIcon->GetRenderTransform();
            Transform.Angle = 0.0f;
            LoadingIcon->SetRenderTransform(Transform);
        }
    }
}

void UFadeRefWidget::CancelFade()
{
    FadeState = EFadeRefState::Idle;
    DelayedFadeState = EFadeRefState::Idle;
    FadeElapsedSeconds = 0.0f;
    FadeDelayRemainingSeconds = 0.0f;
    PendingLevelToLoad = NAME_None;
    SetLoadingIconVisible(false);
    ApplyFadeAlpha(0.0f);
}

bool UFadeRefWidget::IsFadeBusy() const
{
    return FadeState != EFadeRefState::Idle || FadeDelayRemainingSeconds > 0.0f || DelayedFadeState != EFadeRefState::Idle;
}

void UFadeRefWidget::StartFadeWithDelay(EFadeRefState NewState, float DelaySeconds)
{
    DelayedFadeState = EFadeRefState::Idle;
    FadeDelayRemainingSeconds = 0.0f;

    if (DelaySeconds > KINDA_SMALL_NUMBER)
    {
        DelayedFadeState = NewState;
        FadeDelayRemainingSeconds = DelaySeconds;
        FadeState = EFadeRefState::Idle;
        FadeElapsedSeconds = 0.0f;
        return;
    }

    StartFade(NewState);
}

void UFadeRefWidget::StartFade(EFadeRefState NewState)
{
    FadeState = NewState;
    FadeElapsedSeconds = 0.0f;

    if (NewState == EFadeRefState::FadingOut || NewState == EFadeRefState::TransitionOut)
    {
        ApplyFadeAlpha(1.0f);
    }
    else if (NewState == EFadeRefState::FadingIn || NewState == EFadeRefState::TransitionIn || NewState == EFadeRefState::LevelSwitchIn)
    {
        ApplyFadeAlpha(0.0f);
    }
}

void UFadeRefWidget::ExecutePendingLevelTravel()
{
    if (!PendingLevelToLoad.IsNone())
    {
        UGameplayStatics::OpenLevel(this, PendingLevelToLoad);
    }
}

void UFadeRefWidget::ApplyFadeAlpha(float Alpha)
{
    CurrentFadeAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

    if (FadeLayer)
    {
        FadeLayer->SetRenderOpacity(CurrentFadeAlpha);
        FadeLayer->SetVisibility(CurrentFadeAlpha > KINDA_SMALL_NUMBER ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    }
}

void UFadeRefWidget::FinishFadeStep()
{
    switch (FadeState)
    {
    case EFadeRefState::FadingIn:
        FadeState = EFadeRefState::Idle;
        OnFadeInFinished.Broadcast();
        break;

    case EFadeRefState::FadingOut:
        FadeState = EFadeRefState::Idle;
        OnFadeOutFinished.Broadcast();
        break;

    case EFadeRefState::TransitionIn:
        OnFadeInFinished.Broadcast();
        StartFadeWithDelay(EFadeRefState::TransitionOut, TransitionHoldBlackSeconds + FadeOutDelaySeconds);
        break;

    case EFadeRefState::TransitionOut:
        FadeState = EFadeRefState::Idle;
        OnFadeOutFinished.Broadcast();
        break;

    case EFadeRefState::LevelSwitchIn:
        OnFadeInFinished.Broadcast();
        if (LevelLoadDelaySeconds > KINDA_SMALL_NUMBER)
        {
            FadeState = EFadeRefState::LevelSwitchDelay;
            FadeElapsedSeconds = 0.0f;
        }
        else
        {
            ExecutePendingLevelTravel();
            FadeState = EFadeRefState::Idle;
        }
        break;

    default:
        FadeState = EFadeRefState::Idle;
        break;
    }
}
