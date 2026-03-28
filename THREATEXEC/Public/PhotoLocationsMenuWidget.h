#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationsMenuWidget.generated.h"

class UPanelWidget;
class UImage;
class UWidgetAnimation;
class UTexture2D;
class UPhotoLocationButtonWidget;

USTRUCT(BlueprintType)
struct FPhotoLocationPreviewEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location")
    FName ButtonID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location")
    TObjectPtr<UTexture2D> PreviewTexture = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPhotoLocationSelectionEvent, FName, ButtonID);

UCLASS()
class YOURPROJECT_API UPhotoLocationsMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Photo Locations")
    FPhotoLocationSelectionEvent OnLocationClicked;

    UFUNCTION(BlueprintCallable, Category = "Photo Locations")
    void RebuildBindings();

    UFUNCTION(BlueprintCallable, Category = "Photo Locations")
    void ShowPreviewForID(FName InButtonID, bool bInstant = false);

    UFUNCTION(BlueprintCallable, Category = "Photo Locations")
    void FocusNext();

    UFUNCTION(BlueprintCallable, Category = "Photo Locations")
    void FocusPrevious();

    UFUNCTION(BlueprintCallable, Category = "Photo Locations")
    void ConfirmCurrentSelection();

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> PL_LIST;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Current;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Next;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> PreviewSwapAnim;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Locations")
    TArray<FPhotoLocationPreviewEntry> PreviewEntries;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Locations")
    bool bSelectFirstEntryOnInit = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Locations")
    bool bEnableKeyboardNavigation = true;

    UPROPERTY(BlueprintReadOnly, Category = "Photo Locations")
    FName CurrentButtonID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Photo Locations")
    int32 CurrentIndex = INDEX_NONE;

private:
    TMap<FName, TObjectPtr<UTexture2D>> PreviewMap;
    TArray<TObjectPtr<UPhotoLocationButtonWidget>> CachedButtons;

    bool bUsingCurrentAsFront = true;
    bool bTransitionPlaying = false;
    FName PendingButtonID = NAME_None;

    void BuildPreviewMap();
    void BindButtonsFromList();
    void ApplyButtonStates();
    void SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const;
    int32 FindButtonIndexByID(FName InButtonID) const;
    void FinalizeTransitionIfNeeded();

    UFUNCTION()
    void HandleButtonHovered(FName ButtonID);

    UFUNCTION()
    void HandleButtonClicked(FName ButtonID);

    UFUNCTION()
    void HandlePreviewSwapFinished();
};