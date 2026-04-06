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
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UPanelWidget> PL_LIST;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Current;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> PreviewImage_Next;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> PreviewStackContainer;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> PreviewFade;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackAnimationSpeed = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOffsetX = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOffsetY = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOpacityFalloff = 0.13f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float MinStackOpacity = 0.2f;

    UFUNCTION()
    void HandleEntryHovered(UTexture2D* Texture);

    UFUNCTION()
    void HandleEntryClicked(UTexture2D* Texture);

    UFUNCTION()
    void HandlePreviewFadeFinished();

private:
    bool bAnimationBound = false;
    bool bStackAnimationPlaying = false;

    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> PreviewTextureStack;

    UPROPERTY()
    TArray<TObjectPtr<UPhotoLocationEntryWidget>> CachedEntries;

    UPROPERTY(Transient)
    TMap<TObjectPtr<UTexture2D>, TObjectPtr<UImage>> RuntimeStackImages;

    TMap<TObjectPtr<UTexture2D>, FVector2D> StackTargetTranslations;
    TMap<TObjectPtr<UTexture2D>, float> StackTargetOpacities;

    void BindEntries();
    void BuildPreviewTextureStack();
    void EnsureRuntimeStackImages();
    void RefreshPreviewStackVisuals(bool bAnimateFrontSwap = false);
    void AnimateStackTowardTargets(float InDeltaTime);
    void BringTextureToFront(UTexture2D* Texture, bool bAnimateFrontSwap = false);
    void SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const;
};
