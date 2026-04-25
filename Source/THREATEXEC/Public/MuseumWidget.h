#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "MuseumWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * Cycles through museum images/text and cross-fades between slides.
 */
UCLASS()
class THREATEXEC_API UMuseumWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumImage(UTexture2D* MuseumImage);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumText(const FText& MuseumTxt);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumSlide(UTexture2D* MuseumImage, const FText& MuseumTxt);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumTitle(const FText& MuseumTitle);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumPhotoCredit(const FText& MuseumPhotoCredit);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void AddMuseumSlideExtended(UTexture2D* MuseumImage, const FText& MuseumTitle, const FText& MuseumTxt, const FText& MuseumPhotoCredit);

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void ClearMuseumSlides();

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void StartCycling();

    UFUNCTION(BlueprintCallable, Category = "Museum")
    void StopCycling();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> MuseumImage_Current;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> MuseumImage_Next;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumTxt_Current;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumTxt_Next;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumTitle_Current;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumTitle_Next;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumPhotoCredit_Current;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MuseumPhotoCredit_Next;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Data")
    TArray<TObjectPtr<UTexture2D>> MuseumImages;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Data")
    TArray<FText> MuseumTexts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Data")
    TArray<FText> MuseumTitles;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Data")
    TArray<FText> MuseumPhotoCredits;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Cycle", meta = (ClampMin = "0.1"))
    float SlideIntervalSeconds = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Cycle", meta = (ClampMin = "0.05"))
    float CrossFadeDurationSeconds = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Cycle")
    bool bStartCyclingOnConstruct = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Image")
    bool bMatchImageSize = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text")
    bool bOverrideMuseumTextStyle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text", meta = (EditCondition = "bOverrideMuseumTextStyle"))
    FSlateFontInfo MuseumTextFont;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text", meta = (EditCondition = "bOverrideMuseumTextStyle"))
    FLinearColor MuseumTextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text", meta = (EditCondition = "bOverrideMuseumTextStyle"))
    TEnumAsByte<ETextJustify::Type> MuseumTextJustification = ETextJustify::Left;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text", meta = (ClampMin = "0.0", EditCondition = "bOverrideMuseumTextStyle"))
    float MuseumTextWrapAt = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text")
    bool bStabilizeAutoWrapDuringFade = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Museum|Text", meta = (ClampMin = "10.0", EditCondition = "bStabilizeAutoWrapDuringFade"))
    float MinimumStabilizedWrapWidth = 200.0f;

private:
    int32 ActiveSlideIndex = 0;
    int32 PendingSlideIndex = INDEX_NONE;
    float TimeUntilNextSlide = 0.0f;
    float FadeProgressSeconds = 0.0f;
    bool bCycling = false;
    bool bTransitioning = false;
    float StabilizedBodyWrapWidth = 0.0f;

    int32 GetSlideCount() const;
    void InitializeVisibleSlide();
    void BeginTransitionTo(int32 NewSlideIndex);
    void ApplySlideToWidgets(int32 SlideIndex, UImage* ImageWidget, UTextBlock* BodyTextWidget, UTextBlock* TitleTextWidget, UTextBlock* PhotoCreditTextWidget) const;
    void ApplyTextStyle(UTextBlock* TextWidget, float OverrideWrapAt = -1.0f) const;
    void SetImageFromSlide(UImage* ImageWidget, int32 SlideIndex) const;
    float ResolveStabilizedWrapWidth() const;
    static void SetOptionalText(UTextBlock* TextWidget, const TArray<FText>& TextArray, int32 SlideIndex);
    void UpdateTransition(float InDeltaTime);
};
