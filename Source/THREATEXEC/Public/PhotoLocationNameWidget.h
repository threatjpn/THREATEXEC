/**
 * File: PhotoLocationNameWidget.h
 * Summary: Widget responsible for displaying the resolved photo location name for the active map or selection.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationNameWidget.generated.h"

class UTextBlock;

USTRUCT(BlueprintType)
struct FPhotoLocationDisplayNameOverride
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo Location")
    FString LevelName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Photo Location")
    FText DisplayName;
};

/** Widget that resolves and displays the current photo location name. */
UCLASS()
class THREATEXEC_API UPhotoLocationNameWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location")
    TArray<FPhotoLocationDisplayNameOverride> LevelDisplayNameOverrides;

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TXT_LocationName;

private:
    FText ResolveDisplayNameForLevel(const FString& LevelName) const;
};
