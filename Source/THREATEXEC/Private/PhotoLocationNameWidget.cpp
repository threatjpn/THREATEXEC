// ============================================================================
// PhotoLocationNameWidget.cpp
// Displays the active photo-location name in the UI.
//
// Comments are documentation-only and do not alter behaviour.
// ============================================================================

/**
 * File: PhotoLocationNameWidget.cpp
 * Summary: Implementation of the photo location name display widget.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "PhotoLocationNameWidget.h"

#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "PhotoLocationNameLibrary.h"

// Initialises widget state and binds required UI behaviour.
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

// Resolves paths, references, or settings used by this component.
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
