#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationEntryWidget.generated.h"

class UButton;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPhotoLocationEntryHovered, UTexture2D*, PreviewTexture);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPhotoLocationEntryClicked, UTexture2D*, PreviewTexture);

UCLASS()
class THREATEXEC_API UPhotoLocationEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Photo Location")
    FPhotoLocationEntryHovered OnEntryHovered;

    UPROPERTY(BlueprintAssignable, Category = "Photo Location")
    FPhotoLocationEntryClicked OnEntryClicked;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> PreviewTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> ButtonTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> HoveredButtonTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> PressedButtonTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location", meta = (ExposeOnSpawn = "true"))
    FText PreviewDescription;

    UFUNCTION(BlueprintCallable, Category = "Photo Location")
    UTexture2D* GetPreviewTexture() const { return PreviewTexture; }

    UFUNCTION(BlueprintCallable, Category = "Photo Location")
    FText GetPreviewDescription() const { return PreviewDescription; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void SynchronizeProperties() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> MainButton;

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();

    UFUNCTION()
    void HandleClicked();

private:
    void RefreshVisuals();
};