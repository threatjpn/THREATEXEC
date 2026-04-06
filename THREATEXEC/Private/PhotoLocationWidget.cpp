#include "PhotoLocationWidget.h"
#include "PhotoLocationEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationEvents.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Components/WidgetTree.h"
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

    BuildPreviewTextureStack();
    RefreshPreviewStackVisuals(false);
}

void UPhotoLocationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bStackAnimationPlaying)
    {
        AnimateStackTowardTargets(InDeltaTime);
    }
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

void UPhotoLocationWidget::BuildPreviewTextureStack()
{
    PreviewTextureStack.Empty();

    for (UPhotoLocationEntryWidget* Entry : CachedEntries)
    {
        if (!Entry)
        {
            continue;
        }

        UTexture2D* Texture = Entry->GetPreviewTexture();
        if (!Texture)
        {
            continue;
        }

        if (!PreviewTextureStack.Contains(Texture))
        {
            PreviewTextureStack.Add(Texture);
        }
    }
}

void UPhotoLocationWidget::EnsureRuntimeStackImages()
{
    if (!PreviewStackContainer || !WidgetTree)
    {
        return;
    }

    for (UTexture2D* Texture : PreviewTextureStack)
    {
        if (!Texture || RuntimeStackImages.Contains(Texture))
        {
            continue;
        }

        UImage* NewStackImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        if (!NewStackImage)
        {
            continue;
        }

        RuntimeStackImages.Add(Texture, NewStackImage);
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

void UPhotoLocationWidget::RefreshPreviewStackVisuals(bool bAnimateFrontSwap)
{
    if (PreviewStackContainer)
    {
        EnsureRuntimeStackImages();

        // Rebuild child order so the visual stack is back-to-front.
        PreviewStackContainer->ClearChildren();

        StackTargetTranslations.Empty();
        StackTargetOpacities.Empty();

        for (int32 TextureIndex = PreviewTextureStack.Num() - 1; TextureIndex >= 0; --TextureIndex)
        {
            UTexture2D* LayerTexture = PreviewTextureStack[TextureIndex];
            UImage* const* StackImagePtr = RuntimeStackImages.Find(LayerTexture);
            if (!StackImagePtr || !(*StackImagePtr))
            {
                continue;
            }

            UImage* StackImage = *StackImagePtr;
            SetImageTexture(StackImage, LayerTexture);

            const float LayerDepth = static_cast<float>(TextureIndex);
            const FVector2D TargetTranslation(LayerDepth * StackOffsetX, LayerDepth * StackOffsetY);
            const float TargetOpacity = FMath::Max(MinStackOpacity, 1.0f - (LayerDepth * StackOpacityFalloff));

            StackTargetTranslations.Add(LayerTexture, TargetTranslation);
            StackTargetOpacities.Add(LayerTexture, TargetOpacity);

            if (!bAnimateFrontSwap)
            {
                StackImage->SetRenderTranslation(TargetTranslation);
                StackImage->SetRenderOpacity(TargetOpacity);
            }

            PreviewStackContainer->AddChild(StackImage);
        }

        // Hide any runtime image no longer represented in the stack.
        for (const TPair<TObjectPtr<UTexture2D>, TObjectPtr<UImage>>& Pair : RuntimeStackImages)
        {
            UTexture2D* Texture = Pair.Key;
            UImage* StackImage = Pair.Value;
            if (!Texture || !StackImage)
            {
                continue;
            }

            if (!PreviewTextureStack.Contains(Texture))
            {
                StackImage->SetRenderOpacity(0.0f);
            }
        }

        bStackAnimationPlaying = bAnimateFrontSwap;
    }
    else
    {
        if (!PreviewImage_Current || !PreviewImage_Next)
        {
            return;
        }

        UTexture2D* FrontTexture = PreviewTextureStack.Num() > 0 ? PreviewTextureStack[0] : nullptr;
        UTexture2D* BackTexture = PreviewTextureStack.Num() > 1 ? PreviewTextureStack[1] : nullptr;

        SetImageTexture(PreviewImage_Current, FrontTexture);
        SetImageTexture(PreviewImage_Next, BackTexture);

        PreviewImage_Current->SetRenderOpacity(FrontTexture ? 1.0f : 0.0f);
        PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);

        const float BackOpacity = BackTexture ? 0.75f : 0.0f;
        PreviewImage_Next->SetRenderOpacity(BackOpacity);
        PreviewImage_Next->SetRenderTranslation(FVector2D(26.0f, 12.0f));
    }

    if (bAnimateFrontSwap && PreviewFade)
    {
        StopAnimation(PreviewFade);
        PlayAnimationForward(PreviewFade);
    }
}

void UPhotoLocationWidget::AnimateStackTowardTargets(float InDeltaTime)
{
    bool bAnyStillMoving = false;

    for (UTexture2D* Texture : PreviewTextureStack)
    {
        UImage* const* StackImagePtr = RuntimeStackImages.Find(Texture);
        const FVector2D* TargetTranslation = StackTargetTranslations.Find(Texture);
        const float* TargetOpacity = StackTargetOpacities.Find(Texture);

        if (!StackImagePtr || !(*StackImagePtr) || !TargetTranslation || !TargetOpacity)
        {
            continue;
        }

        UImage* StackImage = *StackImagePtr;

        const FWidgetTransform CurrentTransform = StackImage->GetRenderTransform();
        const FVector2D CurrentTranslation = CurrentTransform.Translation;
        const FVector2D NewTranslation = FMath::Vector2DInterpTo(
            CurrentTranslation,
            *TargetTranslation,
            InDeltaTime,
            StackAnimationSpeed);

        const float CurrentOpacity = StackImage->GetRenderOpacity();
        const float NewOpacity = FMath::FInterpTo(CurrentOpacity, *TargetOpacity, InDeltaTime, StackAnimationSpeed);

        FWidgetTransform UpdatedTransform = CurrentTransform;
        UpdatedTransform.Translation = NewTranslation;

        StackImage->SetRenderTransform(UpdatedTransform);
        StackImage->SetRenderOpacity(NewOpacity);

        if (!NewTranslation.Equals(*TargetTranslation, 0.25f) || !FMath::IsNearlyEqual(NewOpacity, *TargetOpacity, 0.01f))
        {
            bAnyStillMoving = true;
        }
    }

    bStackAnimationPlaying = bAnyStillMoving;
}

void UPhotoLocationWidget::BringTextureToFront(UTexture2D* Texture, bool bAnimateFrontSwap)
{
    if (!Texture || PreviewTextureStack.Num() == 0)
    {
        return;
    }

    if (!PreviewTextureStack.Contains(Texture))
    {
        return;
    }

    if (PreviewTextureStack[0] == Texture)
    {
        return;
    }

    UTexture2D* PreviousFront = PreviewTextureStack[0];

    PreviewTextureStack.RemoveSingle(Texture);
    PreviewTextureStack.RemoveSingle(PreviousFront);

    PreviewTextureStack.Insert(Texture, 0);
    PreviewTextureStack.Add(PreviousFront);

    RefreshPreviewStackVisuals(bAnimateFrontSwap);
}

void UPhotoLocationWidget::HandleEntryHovered(UTexture2D* Texture)
{
    BringTextureToFront(Texture, true);
}

void UPhotoLocationWidget::HandleEntryClicked(UTexture2D* Texture)
{
    BringTextureToFront(Texture, true);
}

void UPhotoLocationWidget::HandlePreviewFadeFinished()
{
    RefreshPreviewStackVisuals(false);
}
