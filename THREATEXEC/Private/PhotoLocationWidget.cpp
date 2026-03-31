#include "PhotoLocationWidget.h"
#include "PhotoLocationEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationEvents.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

void UPhotoLocationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    BindEntries();
}

void UPhotoLocationWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BindEntries();

    if (PreviewFade && !bAnimationBound)
    {
        FWidgetAnimationDynamicEvent FinishedDelegate;
        FinishedDelegate.BindDynamic(this, &UPhotoLocationWidget::HandlePreviewFadeFinished);
        BindToAnimationFinished(PreviewFade, FinishedDelegate);
        bAnimationBound = true;
    }

    InitializeFirstPreview();
}

void UPhotoLocationWidget::BindEntries()
{
    CachedEntries.Empty();

    if (!PL_LIST)
    {
        UE_LOG(LogTemp, Warning, TEXT("PhotoLocationWidget: PL_LIST is null"));
        return;
    }

    const int32 ChildCount = PL_LIST->GetChildrenCount();
    for (int32 i = 0; i < ChildCount; ++i)
    {
        UWidget* Child = PL_LIST->GetChildAt(i);
        UPhotoLocationEntryWidget* Entry = Cast<UPhotoLocationEntryWidget>(Child);
        if (!Entry)
        {
            continue;
        }

        CachedEntries.Add(Entry);

        Entry->OnEntryHovered.RemoveDynamic(this, &UPhotoLocationWidget::HandleEntryHovered);
        Entry->OnEntryHovered.AddDynamic(this, &UPhotoLocationWidget::HandleEntryHovered);

        Entry->OnEntryClicked.RemoveDynamic(this, &UPhotoLocationWidget::HandleEntryClicked);
        Entry->OnEntryClicked.AddDynamic(this, &UPhotoLocationWidget::HandleEntryClicked);
    }
}

void UPhotoLocationWidget::InitializeFirstPreview()
{
    for (UPhotoLocationEntryWidget* Entry : CachedEntries)
    {
        if (Entry && Entry->GetPreviewTexture())
        {
            ShowPreview(Entry->GetPreviewTexture(), true);
            return;
        }
    }

    CurrentPreviewTexture = nullptr;
    PendingPreviewTexture = nullptr;

    if (PreviewImage_Current)
    {
        PreviewImage_Current->SetBrush(FSlateBrush());
        PreviewImage_Current->SetRenderOpacity(0.0f);
        PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);
    }

    if (PreviewImage_Next)
    {
        PreviewImage_Next->SetBrush(FSlateBrush());
        PreviewImage_Next->SetRenderOpacity(0.0f);
        PreviewImage_Next->SetRenderTranslation(FVector2D::ZeroVector);
    }
}

void UPhotoLocationWidget::SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const
{
    if (!ImageWidget)
    {
        return;
    }

    if (Texture)
    {
        ImageWidget->SetBrushFromTexture(Texture, true);
    }
    else
    {
        ImageWidget->SetBrush(FSlateBrush());
    }
}

void UPhotoLocationWidget::ShowPreview(UTexture2D* Texture, bool bInstant)
{
    if (!Texture)
    {
        return;
    }

    if (CurrentPreviewTexture == Texture && !bInstant)
    {
        return;
    }

    if (!PreviewImage_Current || !PreviewImage_Next)
    {
        return;
    }

    if (bTransitionPlaying)
    {
        PendingPreviewTexture = Texture;
        return;
    }

    if (bInstant || !PreviewFade)
    {
        SetImageTexture(PreviewImage_Current, Texture);
        PreviewImage_Current->SetRenderOpacity(1.0f);
        PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);

        PreviewImage_Next->SetRenderOpacity(0.0f);
        PreviewImage_Next->SetRenderTranslation(FVector2D::ZeroVector);

        CurrentPreviewTexture = Texture;
        PendingPreviewTexture = nullptr;
        return;
    }

    SetImageTexture(PreviewImage_Next, Texture);

    PreviewImage_Current->SetRenderOpacity(1.0f);
    PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);

    PreviewImage_Next->SetRenderOpacity(0.0f);
    PreviewImage_Next->SetRenderTranslation(FVector2D(30.0f, 0.0f));

    bTransitionPlaying = true;
    PendingPreviewTexture = nullptr;
    CurrentPreviewTexture = Texture;

    StopAnimation(PreviewFade);
    PlayAnimationForward(PreviewFade);
}

void UPhotoLocationWidget::HandleEntryHovered(UTexture2D* Texture)
{
    ShowPreview(Texture, false);
}

void UPhotoLocationWidget::HandleEntryClicked(UTexture2D* Texture)
{
    ShowPreview(Texture, false);
}

void UPhotoLocationWidget::HandlePreviewFadeFinished()
{
    if (!PreviewImage_Current || !PreviewImage_Next)
    {
        bTransitionPlaying = false;
        return;
    }

    PreviewImage_Current->SetBrush(PreviewImage_Next->GetBrush());
    PreviewImage_Current->SetRenderOpacity(1.0f);
    PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);

    PreviewImage_Next->SetRenderOpacity(0.0f);
    PreviewImage_Next->SetRenderTranslation(FVector2D::ZeroVector);

    bTransitionPlaying = false;

    if (PendingPreviewTexture && PendingPreviewTexture != CurrentPreviewTexture)
    {
        UTexture2D* QueuedTexture = PendingPreviewTexture;
        PendingPreviewTexture = nullptr;
        ShowPreview(QueuedTexture, false);
    }
    else
    {
        PendingPreviewTexture = nullptr;
    }
}