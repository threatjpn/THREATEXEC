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

FWidgetTransform UPhotoLocationWidget::BuildTargetTransformForDepth(int32 DepthIndex) const
{
    FWidgetTransform Transform;

    const float Depth = static_cast<float>(DepthIndex);
    Transform.Translation = FVector2D(Depth * StackOffsetX, Depth * StackOffsetY);

    const float TiltFraction = PreviewTextureStack.Num() > 1
        ? (Depth / static_cast<float>(PreviewTextureStack.Num() - 1))
        : 0.0f;
    const float AlternatingTiltSign = (DepthIndex % 2 == 0) ? -1.0f : 1.0f;
    Transform.Angle = MaxCardTiltDegrees * TiltFraction * AlternatingTiltSign;

    const float DepthScale = FMath::Max(0.8f, 1.0f - (Depth * DepthScaleFalloff));
    Transform.Scale = FVector2D(DepthScale, DepthScale);

    if (DepthIndex == 0)
    {
        Transform.Scale = FVector2D(FrontCardLiftScale, FrontCardLiftScale);
        Transform.Angle = 0.0f;
    }

    return Transform;
}

void UPhotoLocationWidget::RefreshPreviewStackVisuals(bool bAnimateFrontSwap)
{
    if (PreviewStackContainer)
    {
        EnsureRuntimeStackImages();

        PreviewStackContainer->ClearChildren();

        StackStartTransforms.Empty();
        StackTargetTransforms.Empty();
        StackStartOpacities.Empty();
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
            const float TargetOpacity = FMath::Max(MinStackOpacity, 1.0f - (LayerDepth * StackOpacityFalloff));
            const FWidgetTransform TargetTransform = BuildTargetTransformForDepth(TextureIndex);

            StackStartTransforms.Add(LayerTexture, StackImage->GetRenderTransform());
            StackStartOpacities.Add(LayerTexture, StackImage->GetRenderOpacity());
            StackTargetTransforms.Add(LayerTexture, TargetTransform);
            StackTargetOpacities.Add(LayerTexture, TargetOpacity);

            if (!bAnimateFrontSwap)
            {
                StackImage->SetRenderTransform(TargetTransform);
                StackImage->SetRenderOpacity(TargetOpacity);
            }

            PreviewStackContainer->AddChild(StackImage);
        }

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

        StackShuffleProgress = bAnimateFrontSwap ? 0.0f : 1.0f;
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
    if (StackShuffleDuration <= KINDA_SMALL_NUMBER)
    {
        StackShuffleProgress = 1.0f;
    }
    else
    {
        StackShuffleProgress = FMath::Clamp(StackShuffleProgress + (InDeltaTime / StackShuffleDuration), 0.0f, 1.0f);
    }

    const float EasedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, StackShuffleProgress, 2.4f);
    const float FrontShufflePulse = FMath::Sin(EasedAlpha * PI);

    for (UTexture2D* Texture : PreviewTextureStack)
    {
        UImage* const* StackImagePtr = RuntimeStackImages.Find(Texture);
        const FWidgetTransform* StartTransform = StackStartTransforms.Find(Texture);
        const FWidgetTransform* TargetTransform = StackTargetTransforms.Find(Texture);
        const float* StartOpacity = StackStartOpacities.Find(Texture);
        const float* TargetOpacity = StackTargetOpacities.Find(Texture);

        if (!StackImagePtr || !(*StackImagePtr) || !StartTransform || !TargetTransform || !StartOpacity || !TargetOpacity)
        {
            continue;
        }

        UImage* StackImage = *StackImagePtr;

        FWidgetTransform InterpolatedTransform;
        InterpolatedTransform.Translation = FMath::Lerp(StartTransform->Translation, TargetTransform->Translation, EasedAlpha);
        InterpolatedTransform.Scale = FMath::Lerp(StartTransform->Scale, TargetTransform->Scale, EasedAlpha);
        InterpolatedTransform.Shear = FMath::Lerp(StartTransform->Shear, TargetTransform->Shear, EasedAlpha);
        InterpolatedTransform.Angle = FMath::Lerp(StartTransform->Angle, TargetTransform->Angle, EasedAlpha);

        if (Texture == LastPromotedTexture)
        {
            // Adds a subtle shuffle arc for the promoted card.
            InterpolatedTransform.Translation.X -= FrontShufflePulse * 7.0f;
            InterpolatedTransform.Translation.Y -= FrontShufflePulse * 3.0f;
            InterpolatedTransform.Angle += FrontShufflePulse * 1.8f;
        }

        StackImage->SetRenderTransform(InterpolatedTransform);
        StackImage->SetRenderOpacity(FMath::Lerp(*StartOpacity, *TargetOpacity, EasedAlpha));
    }

    if (StackShuffleProgress >= 1.0f)
    {
        bStackAnimationPlaying = false;
        for (UTexture2D* Texture : PreviewTextureStack)
        {
            UImage* const* StackImagePtr = RuntimeStackImages.Find(Texture);
            const FWidgetTransform* TargetTransform = StackTargetTransforms.Find(Texture);
            const float* TargetOpacity = StackTargetOpacities.Find(Texture);

            if (StackImagePtr && *StackImagePtr && TargetTransform && TargetOpacity)
            {
                (*StackImagePtr)->SetRenderTransform(*TargetTransform);
                (*StackImagePtr)->SetRenderOpacity(*TargetOpacity);
            }
        }
    }
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

    LastPromotedTexture = Texture;
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
