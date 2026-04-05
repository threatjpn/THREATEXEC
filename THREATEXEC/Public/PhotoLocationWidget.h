#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationWidget.generated.h"

class UPanelWidget;
class UImage;
class UTexture2D;
class UWidgetAnimation;
class UPhotoLocationEntryWidget;

UCLASS()
class THREATEXEC_API UPhotoLocationWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> PL_LIST;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Current;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Next;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> PreviewFade;

    UFUNCTION()
    void HandleEntryHovered(UTexture2D* Texture);

    UFUNCTION()
    void HandleEntryClicked(UTexture2D* Texture);

    UFUNCTION()
    void HandlePreviewFadeFinished();

private:
    bool bAnimationBound = false;

    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> PreviewTextureStack;

    UPROPERTY()
    TArray<TObjectPtr<UPhotoLocationEntryWidget>> CachedEntries;

    void BindEntries();
    void BuildPreviewTextureStack();
    void RefreshPreviewStackVisuals(bool bAnimateFrontSwap = false);
    void BringTextureToFront(UTexture2D* Texture, bool bAnimateFrontSwap = false);
    void SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const;
};
