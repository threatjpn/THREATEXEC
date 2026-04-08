#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Fonts/SlateFontInfo.h"
#include "PhotoLocationWidget.generated.h"

class UPanelWidget;
class UImage;
class UTexture2D;
class UWidgetAnimation;
class UWidget;
class UPhotoLocationEntryWidget;
class UScrollBox;

USTRUCT()
struct FPhotoPreviewStackItem
{
    GENERATED_BODY()

    UPROPERTY()
    int32 ItemId = INDEX_NONE;

    UPROPERTY()
    TObjectPtr<UTexture2D> Texture = nullptr;

    UPROPERTY()
    FText Description;
};

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    bool bOverridePreviewTextStyle = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack", meta = (EditCondition = "bOverridePreviewTextStyle"))
    FSlateFontInfo PreviewTextFont;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack", meta = (EditCondition = "bOverridePreviewTextStyle"))
    FLinearColor PreviewTextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack", meta = (ClampMin = "0.0", EditCondition = "bOverridePreviewTextStyle"))
    float PreviewTextWrapAt = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Photo Location|Text Stack")
    TEnumAsByte<ETextJustify::Type> PreviewTextJustification = ETextJustify::Left;

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
    int32 NextPreviewItemId = 1;

    UPROPERTY()
    TArray<FPhotoPreviewStackItem> PreviewStackItems;

    UPROPERTY()
    TArray<TObjectPtr<UPhotoLocationEntryWidget>> CachedEntries;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UImage>> RuntimeStackImages;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UTextBlock>> RuntimeStackTexts;

    UPROPERTY(Transient)
    TMap<int32, TObjectPtr<UScrollBox>> RuntimeStackTextBoxes;

    UPROPERTY(Transient)
    int32 LastPromotedItemId = INDEX_NONE;

    TMap<int32, FWidgetTransform> StackStartTransforms;
    TMap<int32, FWidgetTransform> StackTargetTransforms;
    TMap<int32, float> StackStartOpacities;
    TMap<int32, float> StackTargetOpacities;

    TMap<int32, FVector2D> TextStartTranslations;
    TMap<int32, FVector2D> TextTargetTranslations;
    TMap<int32, float> TextStartOpacities;
    TMap<int32, float> TextTargetOpacities;

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
    void ApplyPreviewTextStyle(UTextBlock* TextWidget) const;
};
