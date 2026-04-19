/**
 * File: PhotoLocationEntryWidget.cpp
 * Summary: Implementation of photo-location entry widget setup, hover feedback and selection forwarding.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "PhotoLocationEntryWidget.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

void UPhotoLocationEntryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (MainButton)
    {
        MainButton->OnHovered.RemoveDynamic(this, &UPhotoLocationEntryWidget::HandleHovered);
        MainButton->OnUnhovered.RemoveDynamic(this, &UPhotoLocationEntryWidget::HandleUnhovered);
        MainButton->OnClicked.RemoveDynamic(this, &UPhotoLocationEntryWidget::HandleClicked);

        MainButton->OnHovered.AddDynamic(this, &UPhotoLocationEntryWidget::HandleHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UPhotoLocationEntryWidget::HandleUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UPhotoLocationEntryWidget::HandleClicked);
    }
}

void UPhotoLocationEntryWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshVisuals();
}

void UPhotoLocationEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshVisuals();
}

void UPhotoLocationEntryWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    RefreshVisuals();
}

void UPhotoLocationEntryWidget::RefreshVisuals()
{
    if (!MainButton)
    {
        return;
    }

    FButtonStyle Style = MainButton->GetStyle();

    auto ApplyBrushTexture = [](FSlateBrush& Brush, UTexture2D* Texture)
        {
            if (Texture)
            {
                Brush.SetResourceObject(Texture);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                Brush.ImageSize = FVector2D(
                    static_cast<float>(Texture->GetSizeX()),
                    static_cast<float>(Texture->GetSizeY()));
            }
            else
            {
                Brush.SetResourceObject(nullptr);
            }
        };

    ApplyBrushTexture(Style.Normal, ButtonTexture);
    ApplyBrushTexture(
        Style.Hovered,
        HoveredButtonTexture ? HoveredButtonTexture : ButtonTexture);
    ApplyBrushTexture(
        Style.Pressed,
        PressedButtonTexture
        ? PressedButtonTexture
        : (HoveredButtonTexture ? HoveredButtonTexture : ButtonTexture));
    ApplyBrushTexture(Style.Disabled, ButtonTexture);

    MainButton->SetStyle(Style);
}

void UPhotoLocationEntryWidget::HandleHovered()
{
    OnEntryHovered.Broadcast(PreviewTexture);
}

void UPhotoLocationEntryWidget::HandleUnhovered()
{
}

void UPhotoLocationEntryWidget::HandleClicked()
{
    OnEntryClicked.Broadcast(PreviewTexture);
}