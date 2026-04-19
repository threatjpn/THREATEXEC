/**
 * File: PhotoLocationNameLibrary.h
 * Summary: Utility library that converts internal level names into user-facing photo location display names.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhotoLocationNameLibrary.generated.h"

/** Static helper library for converting internal map identifiers into presentation-friendly location names. */
UCLASS()
class THREATEXEC_API UPhotoLocationNameLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Photo Location", meta = (WorldContext = "WorldContextObject"))
    static FText GetCurrentPhotoLocationDisplayName(const UObject* WorldContextObject);

    UFUNCTION(BlueprintPure, Category = "Photo Location")
    static FText GetPhotoLocationDisplayNameFromLevelName(const FString& LevelName);
};