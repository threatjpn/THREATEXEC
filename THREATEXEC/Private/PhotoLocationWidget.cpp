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

    while (RuntimeStackImages.Num() < PreviewTextureStack.Num())
    {
        UImage* NewStackImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        if (!NewStackImage)
        {
            break;
        }

        RuntimeStackImages.Add(NewStackImage);
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
        PreviewStackContainer->ClearChildren();

        constexpr float StackOffsetX = 18.0f;
        constexpr float StackOffsetY = 8.0f;
        constexpr float StackOpacityFalloff = 0.13f;
        constexpr float MinStackOpacity = 0.2f;

        for (int32 TextureIndex = PreviewTextureStack.Num() - 1; TextureIndex >= 0; --TextureIndex)
        {
            if (!RuntimeStackImages.IsValidIndex(TextureIndex))
            {
                continue;
            }

            UImage* StackImage = RuntimeStackImages[TextureIndex];
            UTexture2D* LayerTexture = PreviewTextureStack[TextureIndex];
            SetImageTexture(StackImage, LayerTexture);

            const float LayerDepth = static_cast<float>(TextureIndex);
            const float LayerOpacity = FMath::Max(MinStackOpacity, 1.0f - (LayerDepth * StackOpacityFalloff));

            StackImage->SetRenderOpacity(LayerOpacity);
            StackImage->SetRenderTranslation(FVector2D(LayerDepth * StackOffsetX, LayerDepth * StackOffsetY));

            PreviewStackContainer->AddChild(StackImage);
        }
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
