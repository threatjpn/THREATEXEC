#include "ChangeLocationEntryWidget.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

void UChangeLocationEntryWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (MainButton)
    {
        MainButton->OnHovered.RemoveDynamic(this, &UChangeLocationEntryWidget::HandleHovered);
        MainButton->OnUnhovered.RemoveDynamic(this, &UChangeLocationEntryWidget::HandleUnhovered);
        MainButton->OnClicked.RemoveDynamic(this, &UChangeLocationEntryWidget::HandleClicked);

        MainButton->OnHovered.AddDynamic(this, &UChangeLocationEntryWidget::HandleHovered);
        MainButton->OnUnhovered.AddDynamic(this, &UChangeLocationEntryWidget::HandleUnhovered);
        MainButton->OnClicked.AddDynamic(this, &UChangeLocationEntryWidget::HandleClicked);
    }
}

void UChangeLocationEntryWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshVisuals();
}

void UChangeLocationEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshVisuals();
}

void UChangeLocationEntryWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    RefreshVisuals();
}

void UChangeLocationEntryWidget::RefreshVisuals()
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
    ApplyBrushTexture(Style.Hovered, HoveredButtonTexture ? HoveredButtonTexture : ButtonTexture);
    ApplyBrushTexture(
        Style.Pressed,
        PressedButtonTexture
            ? PressedButtonTexture
            : (HoveredButtonTexture ? HoveredButtonTexture : ButtonTexture));
    ApplyBrushTexture(Style.Disabled, ButtonTexture);

    MainButton->SetStyle(Style);
}

void UChangeLocationEntryWidget::HandleHovered()
{
    OnEntryHovered.Broadcast(VariantID);
}

void UChangeLocationEntryWidget::HandleUnhovered()
{
}

void UChangeLocationEntryWidget::HandleClicked()
{
    OnEntryClicked.Broadcast(VariantID);
}