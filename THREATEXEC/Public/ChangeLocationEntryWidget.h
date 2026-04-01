#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChangeLocationEntryWidget.generated.h"

class UButton;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLocationEntryClickedSignature, FName, VariantID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChangeLocationEntryHoveredSignature, FName, VariantID);

UCLASS()
class THREATEXEC_API UChangeLocationEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Change Location")
    FChangeLocationEntryClickedSignature OnEntryClicked;

    UPROPERTY(BlueprintAssignable, Category = "Change Location")
    FChangeLocationEntryHoveredSignature OnEntryHovered;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (ExposeOnSpawn = "true"))
    FName VariantID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> ButtonTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> HoveredButtonTexture = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Change Location", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<UTexture2D> PressedButtonTexture = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Change Location")
    FName GetVariantID() const { return VariantID; }

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