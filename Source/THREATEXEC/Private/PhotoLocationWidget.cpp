/**
 * File: PhotoLocationWidget.cpp
 * Summary: Implementation of the photo location browser widget, including preview image and text animation support.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "PhotoLocationWidget.h"
#include "PhotoLocationEntryWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationEvents.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
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
    PreviewStackItems.Empty();
    NextPreviewItemId = 1;

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

        FPhotoPreviewStackItem NewItem;
        NewItem.ItemId = NextPreviewItemId++;
        NewItem.Texture = Texture;
        NewItem.Description = Entry->GetPreviewDescription();
        PreviewStackItems.Add(MoveTemp(NewItem));
    }
}

void UPhotoLocationWidget::EnsureRuntimeStackImages()
{
    if (!PreviewStackContainer || !WidgetTree)
    {
        return;
    }

    for (const FPhotoPreviewStackItem& Item : PreviewStackItems)
    {
        if (!Item.Texture || RuntimeStackImages.Contains(Item.ItemId))
        {
            continue;
        }

        UImage* NewStackImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        if (!NewStackImage)
        {
            continue;
        }

        RuntimeStackImages.Add(Item.ItemId, NewStackImage);
    }
}

void UPhotoLocationWidget::EnsureRuntimeStackTexts()
{
    if (!PreviewTextStackContainer || !WidgetTree)
    {
        return;
    }

    for (const FPhotoPreviewStackItem& Item : PreviewStackItems)
    {
        if (!Item.Texture || RuntimeStackTexts.Contains(Item.ItemId))
        {
            continue;
        }

        UScrollBox* NewStackTextBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
        if (!NewStackTextBox)
        {
            continue;
        }

        UTextBlock* NewStackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        if (!NewStackText)
        {
            continue;
        }

        NewStackTextBox->AddChild(NewStackText);
        RuntimeStackTextBoxes.Add(Item.ItemId, NewStackTextBox);
        RuntimeStackTexts.Add(Item.ItemId, NewStackText);
    }
}

void UPhotoLocationWidget::RefreshActiveDescriptionWidget()
{
    if (!PreviewTextStackContainer)
    {
        return;
    }

    PreviewTextStackContainer->ClearChildren();

    ActiveDescriptionItemId = PreviewStackItems.Num() > 0 ? PreviewStackItems[0].ItemId : INDEX_NONE;
    if (ActiveDescriptionItemId == INDEX_NONE)
    {
        return;
    }

    TObjectPtr<UScrollBox>* ActiveTextBoxPtr = RuntimeStackTextBoxes.Find(ActiveDescriptionItemId);
    if (!ActiveTextBoxPtr || !*ActiveTextBoxPtr)
    {
        return;
    }

    UScrollBox* ActiveTextBox = ActiveTextBoxPtr->Get();
    ActiveTextBox->SetRenderTranslation(FVector2D::ZeroVector);
    ActiveTextBox->SetRenderOpacity(1.0f);
    ActiveTextBox->SetVisibility(ESlateVisibility::Visible);
    PreviewTextStackContainer->AddChild(ActiveTextBox);
}

/** Applies a texture to the supplied image widget while handling null-safety. */

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

void UPhotoLocationWidget::ApplyPreviewTextStyle(UTextBlock* TextWidget) const
{
    if (!TextWidget || !bOverridePreviewTextStyle)
    {
        return;
    }

    TextWidget->SetFont(PreviewTextFont);
    TextWidget->SetColorAndOpacity(FSlateColor(PreviewTextColor));
    TextWidget->SetJustification(PreviewTextJustification);

    if (PreviewTextWrapAt > 0.0f)
    {
        TextWidget->SetAutoWrapText(false);
        TextWidget->SetWrapTextAt(PreviewTextWrapAt);
    }
    else
    {
        TextWidget->SetAutoWrapText(true);
        TextWidget->SetWrapTextAt(0.0f);
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

        for (int32 TextureIndex = PreviewStackItems.Num() - 1; TextureIndex >= 0; --TextureIndex)
        {
            const FPhotoPreviewStackItem& LayerItem = PreviewStackItems[TextureIndex];
            UTexture2D* LayerTexture = LayerItem.Texture;
            const int32 LayerItemId = LayerItem.ItemId;

            TObjectPtr<UImage>* StackImagePtr = RuntimeStackImages.Find(LayerItemId);
            if (StackImagePtr && *StackImagePtr)
            {
                UImage* StackImage = StackImagePtr->Get();
                SetImageTexture(StackImage, LayerTexture);

                const float LayerDepth = static_cast<float>(TextureIndex);
                const float TargetOpacity = FMath::Max(MinStackOpacity, 1.0f - (LayerDepth * StackOpacityFalloff));
                const FWidgetTransform TargetTransform = BuildTargetTransformForDepth(TextureIndex);

                StackStartTransforms.Add(LayerItemId, StackImage->GetRenderTransform());
                StackStartOpacities.Add(LayerItemId, StackImage->GetRenderOpacity());
                StackTargetTransforms.Add(LayerItemId, TargetTransform);
                StackTargetOpacities.Add(LayerItemId, TargetOpacity);

                if (!bAnimateFrontSwap)
                {
                    StackImage->SetRenderTransform(TargetTransform);
                    StackImage->SetRenderOpacity(TargetOpacity);
                }

                PreviewStackContainer->AddChild(StackImage);
            }

            if (PreviewTextStackContainer)
            {
                TObjectPtr<UTextBlock>* StackTextPtr = RuntimeStackTexts.Find(LayerItemId);
                TObjectPtr<UScrollBox>* StackTextBoxPtr = RuntimeStackTextBoxes.Find(LayerItemId);
                if (StackTextPtr && *StackTextPtr && StackTextBoxPtr && *StackTextBoxPtr)
                {
                    UTextBlock* StackText = StackTextPtr->Get();
                    UScrollBox* StackTextBox = StackTextBoxPtr->Get();
                    StackText->SetText(LayerItem.Description);
                    ApplyPreviewTextStyle(StackText);

                }
            }
        }

        RefreshActiveDescriptionWidget();

        StackShuffleProgress = bAnimateFrontSwap ? 0.0f : 1.0f;
        bStackAnimationPlaying = bAnimateFrontSwap;
    }
    else
    {
        if (!PreviewImage_Current || !PreviewImage_Next)
        {
            return;
        }

        UTexture2D* FrontTexture = PreviewStackItems.Num() > 0 ? PreviewStackItems[0].Texture.Get() : nullptr;
        UTexture2D* BackTexture = PreviewStackItems.Num() > 1 ? PreviewStackItems[1].Texture.Get() : nullptr;

        SetImageTexture(PreviewImage_Current, FrontTexture);
        SetImageTexture(PreviewImage_Next, BackTexture);

        PreviewImage_Current->SetRenderOpacity(FrontTexture ? 1.0f : 0.0f);
        PreviewImage_Current->SetRenderTranslation(FVector2D::ZeroVector);

        const float BackOpacity = BackTexture ? 0.75f : 0.0f;
        PreviewImage_Next->SetRenderOpacity(BackOpacity);
        PreviewImage_Next->SetRenderTranslation(FVector2D(26.0f, 12.0f));

        RefreshActiveDescriptionWidget();
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

    for (const FPhotoPreviewStackItem& Item : PreviewStackItems)
    {
        const int32 ItemId = Item.ItemId;
        TObjectPtr<UImage>* StackImagePtr = RuntimeStackImages.Find(ItemId);
        const FWidgetTransform* StartTransform = StackStartTransforms.Find(ItemId);
        const FWidgetTransform* TargetTransform = StackTargetTransforms.Find(ItemId);
        const float* StartOpacity = StackStartOpacities.Find(ItemId);
        const float* TargetOpacity = StackTargetOpacities.Find(ItemId);

        if (StackImagePtr && *StackImagePtr && StartTransform && TargetTransform && StartOpacity && TargetOpacity)
        {
            UImage* StackImage = StackImagePtr->Get();

            FWidgetTransform InterpolatedTransform;
            InterpolatedTransform.Translation = FMath::Lerp(StartTransform->Translation, TargetTransform->Translation, EasedAlpha);
            InterpolatedTransform.Scale = FMath::Lerp(StartTransform->Scale, TargetTransform->Scale, EasedAlpha);
            InterpolatedTransform.Shear = FMath::Lerp(StartTransform->Shear, TargetTransform->Shear, EasedAlpha);
            InterpolatedTransform.Angle = 0.0f;

            if (ItemId == LastPromotedItemId)
            {
                InterpolatedTransform.Translation.X -= FrontShufflePulse * 7.0f;
                InterpolatedTransform.Translation.Y -= FrontShufflePulse * 3.0f;
            }

            StackImage->SetRenderTransform(InterpolatedTransform);
            StackImage->SetRenderOpacity(FMath::Lerp(*StartOpacity, *TargetOpacity, EasedAlpha));
        }
    }

    if (StackShuffleProgress >= 1.0f)
    {
        bStackAnimationPlaying = false;
    }
}

void UPhotoLocationWidget::BringTextureToFront(UTexture2D* Texture, bool bAnimateFrontSwap)
{
    if (!Texture || PreviewStackItems.Num() == 0)
    {
        return;
    }

    int32 FoundIndex = INDEX_NONE;
    for (int32 ItemIndex = 0; ItemIndex < PreviewStackItems.Num(); ++ItemIndex)
    {
        if (PreviewStackItems[ItemIndex].Texture == Texture)
        {
            FoundIndex = ItemIndex;
            break;
        }
    }

    if (FoundIndex == INDEX_NONE || FoundIndex == 0)
    {
        return;
    }

    const FPhotoPreviewStackItem PromotedItem = PreviewStackItems[FoundIndex];
    const FPhotoPreviewStackItem PreviousFront = PreviewStackItems[0];
    PreviewStackItems.RemoveAt(FoundIndex);
    PreviewStackItems.RemoveAt(0);
    PreviewStackItems.Insert(PromotedItem, 0);
    PreviewStackItems.Add(PreviousFront);

    LastPromotedItemId = PromotedItem.ItemId;
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
