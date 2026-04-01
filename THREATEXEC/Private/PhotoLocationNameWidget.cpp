#include "PhotoLocationNameWidget.h"

#include "Components/TextBlock.h"
#include "PhotoLocationNameLibrary.h"

void UPhotoLocationNameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (TXT_LocationName)
    {
        TXT_LocationName->SetText(UPhotoLocationNameLibrary::GetCurrentPhotoLocationDisplayName(this));
    }
}