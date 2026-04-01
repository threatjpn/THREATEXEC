#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationNameWidget.generated.h"

class UTextBlock;

UCLASS()
class THREATEXEC_API UPhotoLocationNameWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> TXT_LocationName;
};