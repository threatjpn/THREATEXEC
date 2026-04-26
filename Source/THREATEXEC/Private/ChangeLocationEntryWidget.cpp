// ============================================================================
// ChangeLocationEntryWidget.cpp
// Implements one entry in the change-location menu.
// ============================================================================

#include "ChangeLocationEntryWidget.h"
#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

// Handles native on initialized.
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

// Handles native pre construct.
void UChangeLocationEntryWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    RefreshVisuals();
}

// Initialises widget state and binds required UI behaviour.
void UChangeLocationEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshVisuals();
}

// Synchronises internal state with the engine representation.
void UChangeLocationEntryWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    RefreshVisuals();
}

// Rebuilds cached output from the current source data.
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

// Handles an event from UI, input, or runtime state.
void UChangeLocationEntryWidget::HandleHovered()
{
    OnEntryHovered.Broadcast(VariantID);
}

// Handles an event from UI, input, or runtime state.
void UChangeLocationEntryWidget::HandleUnhovered()
{
}

// Handles an event from UI, input, or runtime state.
void UChangeLocationEntryWidget::HandleClicked()
{
    OnEntryClicked.Broadcast(VariantID);
}
