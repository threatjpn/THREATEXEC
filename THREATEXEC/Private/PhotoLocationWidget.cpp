#include "PhotoLocationWidget.h"
#include "PhotoLocationEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationEvents.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
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

    TArray<UPhotoLocationEntryWidget*> FoundEntries;
    CollectEntriesRecursive(PL_LIST, FoundEntries);

    for (UPhotoLocationEntryWidget* Entry : FoundEntries)
    {
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


void UPhotoLocationWidget::CollectEntriesRecursive(UWidget* RootWidget, TArray<UPhotoLocationEntryWidget*>& OutEntries)
{
    if (!RootWidget)
    {
        return;
    }

    if (UPhotoLocationEntryWidget* Entry = Cast<UPhotoLocationEntryWidget>(RootWidget))
    {
        OutEntries.Add(Entry);
        return;
    }

    if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
    {
        const int32 ChildCount = Panel->GetChildrenCount();
        for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
        {
            CollectEntriesRecursive(Panel->GetChildAt(ChildIndex), OutEntries);
        }
    }
}

void UPhotoLocationWidget::BuildPreviewTextureStack()
{
    PreviewTextureStack.Empty();
    PreviewDescriptionByTexture.Empty();

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
            PreviewDescriptionByTexture.Add(Texture, Entry->GetPreviewDescription());
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

void UPhotoLocationWidget::EnsureRuntimeStackTexts()
{
    if (!PreviewTextStackContainer || !WidgetTree)
    {
        return;
    }

    for (UTexture2D* Texture : PreviewTextureStack)
    {
        if (!Texture || RuntimeStackTexts.Contains(Texture))
        {
            continue;
        }

        UTextBlock* NewStackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!NewStackText)
        {
            continue;
        }

        RuntimeStackTexts.Add(Texture, NewStackText);
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
    Transform.Angle = 0.0f;

    const float DepthScale = FMath::Max(0.8f, 1.0f - (Depth * DepthScaleFalloff));
    Transform.Scale = FVector2D(DepthScale, DepthScale);

    if (DepthIndex == 0)
    {
        Transform.Scale = FVector2D(FrontCardLiftScale, FrontCardLiftScale);
    }

    return Transform;
}

void UPhotoLocationWidget::RefreshPreviewStackVisuals(bool bAnimateFrontSwap)
{
    if (PreviewStackContainer)
    {
        EnsureRuntimeStackImages();
        EnsureRuntimeStackTexts();

        PreviewStackContainer->ClearChildren();
        if (PreviewTextStackContainer)
        {
            PreviewTextStackContainer->ClearChildren();
        }

        StackStartTransforms.Empty();
        StackTargetTransforms.Empty();
        StackStartOpacities.Empty();
        StackTargetOpacities.Empty();

        TextStartTranslations.Empty();
        TextTargetTranslations.Empty();
        TextStartOpacities.Empty();
        TextTargetOpacities.Empty();

        for (int32 TextureIndex = PreviewTextureStack.Num() - 1; TextureIndex >= 0; --TextureIndex)
        {
            UTexture2D* LayerTexture = PreviewTextureStack[TextureIndex];

            TObjectPtr<UImage>* StackImagePtr = RuntimeStackImages.Find(LayerTexture);
            if (StackImagePtr && *StackImagePtr)
            {
                UImage* StackImage = StackImagePtr->Get();
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

            if (PreviewTextStackContainer)
            {
                TObjectPtr<UTextBlock>* StackTextPtr = RuntimeStackTexts.Find(LayerTexture);
                if (StackTextPtr && *StackTextPtr)
                {
                    UTextBlock* StackText = StackTextPtr->Get();
                    const FText* DescriptionText = PreviewDescriptionByTexture.Find(LayerTexture);
                    StackText->SetText(DescriptionText ? *DescriptionText : FText::GetEmpty());

                    const float LayerDepth = static_cast<float>(TextureIndex);
                    const FVector2D TextTargetTranslation(LayerDepth * TextStackOffsetX, LayerDepth * TextStackOffsetY);
                    const float TextTargetOpacity = FMath::Max(MinTextOpacity, 1.0f - (LayerDepth * TextOpacityFalloff));

                    TextStartTranslations.Add(LayerTexture, StackText->GetRenderTransform().Translation);
                    TextTargetTranslations.Add(LayerTexture, TextTargetTranslation);
                    TextStartOpacities.Add(LayerTexture, StackText->GetRenderOpacity());
                    TextTargetOpacities.Add(LayerTexture, TextTargetOpacity);

                    if (!bAnimateFrontSwap)
                    {
                        StackText->SetRenderTranslation(TextTargetTranslation);
                        StackText->SetRenderOpacity(TextTargetOpacity);
                    }

                    PreviewTextStackContainer->AddChild(StackText);
                }
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
        TObjectPtr<UImage>* StackImagePtr = RuntimeStackImages.Find(Texture);
        const FWidgetTransform* StartTransform = StackStartTransforms.Find(Texture);
        const FWidgetTransform* TargetTransform = StackTargetTransforms.Find(Texture);
        const float* StartOpacity = StackStartOpacities.Find(Texture);
        const float* TargetOpacity = StackTargetOpacities.Find(Texture);

        if (StackImagePtr && *StackImagePtr && StartTransform && TargetTransform && StartOpacity && TargetOpacity)
        {
            UImage* StackImage = StackImagePtr->Get();

            FWidgetTransform InterpolatedTransform;
            InterpolatedTransform.Translation = FMath::Lerp(StartTransform->Translation, TargetTransform->Translation, EasedAlpha);
            InterpolatedTransform.Scale = FMath::Lerp(StartTransform->Scale, TargetTransform->Scale, EasedAlpha);
            InterpolatedTransform.Shear = FMath::Lerp(StartTransform->Shear, TargetTransform->Shear, EasedAlpha);
            InterpolatedTransform.Angle = 0.0f;

            if (Texture == LastPromotedTexture)
            {
                InterpolatedTransform.Translation.X -= FrontShufflePulse * 7.0f;
                InterpolatedTransform.Translation.Y -= FrontShufflePulse * 3.0f;
            }

            StackImage->SetRenderTransform(InterpolatedTransform);
            StackImage->SetRenderOpacity(FMath::Lerp(*StartOpacity, *TargetOpacity, EasedAlpha));
        }

        if (PreviewTextStackContainer)
        {
            TObjectPtr<UTextBlock>* StackTextPtr = RuntimeStackTexts.Find(Texture);
            const FVector2D* StartTranslation = TextStartTranslations.Find(Texture);
            const FVector2D* TargetTranslation = TextTargetTranslations.Find(Texture);
            const float* StartTextOpacity = TextStartOpacities.Find(Texture);
            const float* TargetTextOpacity = TextTargetOpacities.Find(Texture);

            if (StackTextPtr && *StackTextPtr && StartTranslation && TargetTranslation && StartTextOpacity && TargetTextOpacity)
            {
                UTextBlock* StackText = StackTextPtr->Get();
                FVector2D NewTextTranslation = FMath::Lerp(*StartTranslation, *TargetTranslation, EasedAlpha);

                if (Texture == LastPromotedTexture)
                {
                    NewTextTranslation.X -= FrontShufflePulse * 4.0f;
                }

                StackText->SetRenderTranslation(NewTextTranslation);
                StackText->SetRenderOpacity(FMath::Lerp(*StartTextOpacity, *TargetTextOpacity, EasedAlpha));
            }
        }
    }

    if (StackShuffleProgress >= 1.0f)
    {
        bStackAnimationPlaying = false;
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
