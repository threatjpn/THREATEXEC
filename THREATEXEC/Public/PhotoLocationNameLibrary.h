#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhotoLocationNameLibrary.generated.h"

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