#include "PhotoLocationNameWidget.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "PhotoLocationNameLibrary.h"

void UPhotoLocationNameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!TXT_LocationName)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        TXT_LocationName->SetText(FText::GetEmpty());
        return;
    }

    const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
    TXT_LocationName->SetText(ResolveDisplayNameForLevel(CurrentLevelName));
}

FText UPhotoLocationNameWidget::ResolveDisplayNameForLevel(const FString& LevelName) const
{
    for (const FPhotoLocationDisplayNameOverride& Override : LevelDisplayNameOverrides)
    {
        if (Override.LevelName.Equals(LevelName, ESearchCase::IgnoreCase))
        {
            return Override.DisplayName;
        }
    }

    return UPhotoLocationNameLibrary::GetPhotoLocationDisplayNameFromLevelName(LevelName);
}
