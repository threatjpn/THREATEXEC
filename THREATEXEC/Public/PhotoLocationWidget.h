#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PhotoLocationWidget.generated.h"

class UPanelWidget;
class UImage;
class UTexture2D;
class UWidgetAnimation;
class UWidget;
class UPhotoLocationEntryWidget;
class UTextBlock;

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

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> PreviewTextStackContainer;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> PreviewFade;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackShuffleDuration = 0.28f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOffsetX = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOffsetY = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float StackOpacityFalloff = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float MinStackOpacity = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float DepthScaleFalloff = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Stack Animation")
    float FrontCardLiftScale = 1.03f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    float TextOpacityFalloff = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    float MinTextOpacity = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    float TextStackOffsetX = 14.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    float TextStackOffsetY = 0.0f;

    UFUNCTION()
    void HandleEntryHovered(UTexture2D* Texture);

    UFUNCTION()
    void HandleEntryClicked(UTexture2D* Texture);

    UFUNCTION()
    void HandlePreviewFadeFinished();

private:
    bool bAnimationBound = false;
    bool bStackAnimationPlaying = false;
    float StackShuffleProgress = 1.0f;

    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> PreviewTextureStack;

    UPROPERTY()
    TArray<TObjectPtr<UPhotoLocationEntryWidget>> CachedEntries;

    UPROPERTY(Transient)
    TMap<TObjectPtr<UTexture2D>, TObjectPtr<UImage>> RuntimeStackImages;

    UPROPERTY(Transient)
    TMap<TObjectPtr<UTexture2D>, TObjectPtr<UTextBlock>> RuntimeStackTexts;

    UPROPERTY()
    TMap<TObjectPtr<UTexture2D>, FText> PreviewDescriptionByTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> LastPromotedTexture = nullptr;

    TMap<TObjectPtr<UTexture2D>, FWidgetTransform> StackStartTransforms;
    TMap<TObjectPtr<UTexture2D>, FWidgetTransform> StackTargetTransforms;
    TMap<TObjectPtr<UTexture2D>, float> StackStartOpacities;
    TMap<TObjectPtr<UTexture2D>, float> StackTargetOpacities;

    TMap<TObjectPtr<UTexture2D>, FVector2D> TextStartTranslations;
    TMap<TObjectPtr<UTexture2D>, FVector2D> TextTargetTranslations;
    TMap<TObjectPtr<UTexture2D>, float> TextStartOpacities;
    TMap<TObjectPtr<UTexture2D>, float> TextTargetOpacities;

    void BindEntries();
    void CollectEntriesRecursive(UWidget* RootWidget, TArray<UPhotoLocationEntryWidget*>& OutEntries);
    void BuildPreviewTextureStack();
    void EnsureRuntimeStackImages();
    void EnsureRuntimeStackTexts();
    void RefreshPreviewStackVisuals(bool bAnimateFrontSwap = false);
    void AnimateStackTowardTargets(float InDeltaTime);
    FWidgetTransform BuildTargetTransformForDepth(int32 DepthIndex) const;
    void BringTextureToFront(UTexture2D* Texture, bool bAnimateFrontSwap = false);
    void SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const;
};
