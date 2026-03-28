#include "PhotoLocationButtonWidget.h"
#include "Components/Button.h"

void UPhotoLocationButtonWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (MainButton)
    {
        MainButton->OnHovered.AddDynamic(this, &UPhotoLocationButtonWidget::HandleHovered);
        MainButton->OnClicked.AddDynamic(this, &UPhotoLocationButtonWidget::HandleClicked);
    }
}

void UPhotoLocationButtonWidget::HandleHovered()
{
    OnPhotoButtonHovered.Broadcast(ButtonID);
}

void UPhotoLocationButtonWidget::HandleClicked()
{
    OnPhotoButtonClicked.Broadcast(ButtonID);
}

void UPhotoLocationButtonWidget::SetVisualHovered(bool bHovered)
{
    BP_SetVisualHovered(bHovered);
}

void UPhotoLocationButtonWidget::SetVisualSelected(bool bSelected)
{
    BP_SetVisualSelected(bSelected);
}