// ============================================================================
// PhotoLocationNameLibrary.cpp
// Provides display-name helpers for photo-location identifiers.
// ============================================================================

#include "PhotoLocationNameLibrary.h"

#include "Kismet/GameplayStatics.h"

// Returns the display name for the current photo location.
FText UPhotoLocationNameLibrary::GetCurrentPhotoLocationDisplayName(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return FText::GetEmpty();
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return FText::GetEmpty();
    }

    const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
    return GetPhotoLocationDisplayNameFromLevelName(LevelName);
}

// Returns a photo-location display name from a level identifier.
FText UPhotoLocationNameLibrary::GetPhotoLocationDisplayNameFromLevelName(const FString& LevelName)
{
    if (LevelName.Equals(TEXT("TE_PL_AHRWEILER"), ESearchCase::IgnoreCase))
    {
        return FText::FromString(TEXT("Ahrweiler - Town Square, Germany"));
    }

    if (LevelName.Equals(TEXT("TE_PL_ABBEY"), ESearchCase::IgnoreCase))
    {
        return FText::FromString(TEXT("San Galgano Abbey, Italy"));
    }

    if (LevelName.Equals(TEXT("TE_PL_KYOTO_GION"), ESearchCase::IgnoreCase))
    {
        return FText::FromString(TEXT("Kyoto - Gion, Japan"));
    }

    if (LevelName.Equals(TEXT("TE_PL_MIRAGE"), ESearchCase::IgnoreCase))
    {
        return FText::FromString(TEXT("Secret Test Ground, UK"));
    }

    if (LevelName.Equals(TEXT("TE_PL_WIND_TUNNEL"), ESearchCase::IgnoreCase))
    {
        return FText::FromString(TEXT("Honda R&D Facility, Japan"));
    }

    return FText::FromString(LevelName);
}
