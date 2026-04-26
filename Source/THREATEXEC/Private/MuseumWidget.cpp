// ============================================================================
// MuseumWidget.cpp
// Implements the museum slideshow widget, including slide data, timed cycling, and fade transitions.
// ============================================================================

#include "MuseumWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"

// Initialises widget state and binds required UI behaviour.
void UMuseumWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ActiveSlideIndex = 0;
    PendingSlideIndex = INDEX_NONE;
    bTransitioning = false;
    StabilizedBodyWrapWidth = 0.0f;
    TimeUntilNextSlide = FMath::Max(0.1f, SlideIntervalSeconds);

    InitializeVisibleSlide();

    if (bStartCyclingOnConstruct)
    {
        StartCycling();
    }
    else
    {
        StopCycling();
    }
}

// Updates time-dependent widget behaviour each frame.
void UMuseumWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bTransitioning)
    {
        UpdateTransition(InDeltaTime);
        return;
    }

    if (!bCycling || GetSlideCount() <= 1)
    {
        return;
    }

    TimeUntilNextSlide -= InDeltaTime;
    if (TimeUntilNextSlide <= 0.0f)
    {
        const int32 NextSlideIndex = (ActiveSlideIndex + 1) % GetSlideCount();
        BeginTransitionTo(NextSlideIndex);
    }
}

// Adds an image entry to the slideshow data.
void UMuseumWidget::AddMuseumImage(UTexture2D* MuseumImage)
{
    MuseumImages.Add(MuseumImage);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Adds body text to the slideshow data.
void UMuseumWidget::AddMuseumText(const FText& MuseumTxt)
{
    MuseumTexts.Add(MuseumTxt);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Adds a basic museum slide from image and body text.
void UMuseumWidget::AddMuseumSlide(UTexture2D* MuseumImage, const FText& MuseumTxt)
{
    MuseumImages.Add(MuseumImage);
    MuseumTitles.Add(FText::GetEmpty());
    MuseumTexts.Add(MuseumTxt);
    MuseumPhotoCredits.Add(FText::GetEmpty());

    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Adds a title entry to the slideshow data.
void UMuseumWidget::AddMuseumTitle(const FText& MuseumTitle)
{
    MuseumTitles.Add(MuseumTitle);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Adds a credit entry to the slideshow data.
void UMuseumWidget::AddMuseumPhotoCredit(const FText& MuseumPhotoCredit)
{
    MuseumPhotoCredits.Add(MuseumPhotoCredit);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Adds a complete museum slide with image, title, text, and credit.
void UMuseumWidget::AddMuseumSlideExtended(UTexture2D* MuseumImage, const FText& MuseumTitle, const FText& MuseumTxt, const FText& MuseumPhotoCredit)
{
    MuseumImages.Add(MuseumImage);
    MuseumTitles.Add(MuseumTitle);
    MuseumTexts.Add(MuseumTxt);
    MuseumPhotoCredits.Add(MuseumPhotoCredit);

    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

// Clears all slide data and resets the visible widget state.
void UMuseumWidget::ClearMuseumSlides()
{
    MuseumImages.Empty();
    MuseumTitles.Empty();
    MuseumTexts.Empty();
    MuseumPhotoCredits.Empty();

    ActiveSlideIndex = 0;
    PendingSlideIndex = INDEX_NONE;
    bTransitioning = false;

    if (MuseumImage_Current)
    {
        MuseumImage_Current->SetBrush(FSlateBrush());
        MuseumImage_Current->SetRenderOpacity(1.0f);
    }

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetBrush(FSlateBrush());
        MuseumImage_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTxt_Current)
    {
        MuseumTxt_Current->SetText(FText::GetEmpty());
        MuseumTxt_Current->SetRenderOpacity(1.0f);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetText(FText::GetEmpty());
        MuseumTxt_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTitle_Current)
    {
        MuseumTitle_Current->SetText(FText::GetEmpty());
        MuseumTitle_Current->SetRenderOpacity(1.0f);
    }

    if (MuseumTitle_Next)
    {
        MuseumTitle_Next->SetText(FText::GetEmpty());
        MuseumTitle_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumPhotoCredit_Current)
    {
        MuseumPhotoCredit_Current->SetText(FText::GetEmpty());
        MuseumPhotoCredit_Current->SetRenderOpacity(1.0f);
    }

    if (MuseumPhotoCredit_Next)
    {
        MuseumPhotoCredit_Next->SetText(FText::GetEmpty());
        MuseumPhotoCredit_Next->SetRenderOpacity(0.0f);
    }
}

// Starts automatic slide cycling from the current slide.
void UMuseumWidget::StartCycling()
{
    bCycling = true;
    TimeUntilNextSlide = FMath::Max(0.1f, SlideIntervalSeconds);
}

// Stops automatic slide cycling without changing the active slide.
void UMuseumWidget::StopCycling()
{
    bCycling = false;
}

// Returns the largest populated slide-data count.
int32 UMuseumWidget::GetSlideCount() const
{
    int32 SlideCount = MuseumImages.Num();
    SlideCount = FMath::Max(SlideCount, MuseumTitles.Num());
    SlideCount = FMath::Max(SlideCount, MuseumTexts.Num());
    SlideCount = FMath::Max(SlideCount, MuseumPhotoCredits.Num());
    return SlideCount;
}

// Applies the current slide and resets hidden transition widgets.
void UMuseumWidget::InitializeVisibleSlide()
{
    if (GetSlideCount() <= 0)
    {
        return;
    }

    ActiveSlideIndex = FMath::Clamp(ActiveSlideIndex, 0, GetSlideCount() - 1);
    ApplySlideToWidgets(ActiveSlideIndex, MuseumImage_Current, MuseumTxt_Current, MuseumTitle_Current, MuseumPhotoCredit_Current);

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTitle_Next)
    {
        MuseumTitle_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumPhotoCredit_Next)
    {
        MuseumPhotoCredit_Next->SetRenderOpacity(0.0f);
    }
}

// Starts a fade transition to the requested slide index.
void UMuseumWidget::BeginTransitionTo(int32 NewSlideIndex)
{
    if (bTransitioning || GetSlideCount() <= 1)
    {
        return;
    }

    PendingSlideIndex = FMath::Clamp(NewSlideIndex, 0, GetSlideCount() - 1);
    FadeProgressSeconds = 0.0f;
    bTransitioning = true;
    StabilizedBodyWrapWidth = ResolveStabilizedWrapWidth();

    ApplyTextStyle(MuseumTxt_Current, StabilizedBodyWrapWidth);
    ApplySlideToWidgets(PendingSlideIndex, MuseumImage_Next, MuseumTxt_Next, MuseumTitle_Next, MuseumPhotoCredit_Next);

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetRenderOpacity(0.0f);
        MuseumTxt_Next->ForceLayoutPrepass();
    }

    if (MuseumTitle_Next)
    {
        MuseumTitle_Next->SetRenderOpacity(0.0f);
        MuseumTitle_Next->ForceLayoutPrepass();
    }

    if (MuseumPhotoCredit_Next)
    {
        MuseumPhotoCredit_Next->SetRenderOpacity(0.0f);
        MuseumPhotoCredit_Next->ForceLayoutPrepass();
    }
}

// Writes slide image, title, text, and credit into target widgets.
void UMuseumWidget::ApplySlideToWidgets(int32 SlideIndex, UImage* ImageWidget, UTextBlock* BodyTextWidget, UTextBlock* TitleTextWidget, UTextBlock* PhotoCreditTextWidget) const
{
    SetImageFromSlide(ImageWidget, SlideIndex);
    SetOptionalText(TitleTextWidget, MuseumTitles, SlideIndex);
    SetOptionalText(BodyTextWidget, MuseumTexts, SlideIndex);
    SetOptionalText(PhotoCreditTextWidget, MuseumPhotoCredits, SlideIndex);

    ApplyTextStyle(TitleTextWidget);
    ApplyTextStyle(BodyTextWidget, StabilizedBodyWrapWidth);
    ApplyTextStyle(PhotoCreditTextWidget);
}

// Applies derived settings to the relevant runtime objects.
void UMuseumWidget::ApplyTextStyle(UTextBlock* TextWidget, float OverrideWrapAt) const
{
    if (!TextWidget)
    {
        return;
    }

    if (bOverrideMuseumTextStyle)
    {
        TextWidget->SetFont(MuseumTextFont);
        TextWidget->SetColorAndOpacity(FSlateColor(MuseumTextColor));
        TextWidget->SetJustification(MuseumTextJustification);
    }

    const float EffectiveWrapAt = OverrideWrapAt > 0.0f ? OverrideWrapAt : MuseumTextWrapAt;
    if (EffectiveWrapAt > 0.0f)
    {
        TextWidget->SetAutoWrapText(false);
        TextWidget->SetWrapTextAt(EffectiveWrapAt);
    }
    else
    {
        TextWidget->SetAutoWrapText(true);
        TextWidget->SetWrapTextAt(0.0f);
    }
}

// Applies the slide image to the target image widget.
void UMuseumWidget::SetImageFromSlide(UImage* ImageWidget, int32 SlideIndex) const
{
    if (!ImageWidget)
    {
        return;
    }

    UTexture2D* Texture = MuseumImages.IsValidIndex(SlideIndex) ? MuseumImages[SlideIndex] : nullptr;
    if (Texture)
    {
        ImageWidget->SetBrushFromTexture(Texture, bMatchImageSize);
    }
    else
    {
        ImageWidget->SetBrush(FSlateBrush());
    }
}

// Advances the active slide fade and finalises completed transitions.
void UMuseumWidget::UpdateTransition(float InDeltaTime)
{
    FadeProgressSeconds += InDeltaTime;

    const float SafeDuration = FMath::Max(0.05f, CrossFadeDurationSeconds);
    const float Alpha = FMath::Clamp(FadeProgressSeconds / SafeDuration, 0.0f, 1.0f);

    if (MuseumImage_Current)
    {
        MuseumImage_Current->SetRenderOpacity(1.0f - Alpha);
    }

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetRenderOpacity(Alpha);
    }

    if (MuseumTxt_Current)
    {
        MuseumTxt_Current->SetRenderOpacity(1.0f - Alpha);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetRenderOpacity(Alpha);
    }

    if (MuseumTitle_Current)
    {
        MuseumTitle_Current->SetRenderOpacity(1.0f - Alpha);
    }

    if (MuseumTitle_Next)
    {
        MuseumTitle_Next->SetRenderOpacity(Alpha);
    }

    if (MuseumPhotoCredit_Current)
    {
        MuseumPhotoCredit_Current->SetRenderOpacity(1.0f - Alpha);
    }

    if (MuseumPhotoCredit_Next)
    {
        MuseumPhotoCredit_Next->SetRenderOpacity(Alpha);
    }

    if (Alpha >= 1.0f)
    {
        ActiveSlideIndex = PendingSlideIndex;
        PendingSlideIndex = INDEX_NONE;
        bTransitioning = false;
        StabilizedBodyWrapWidth = 0.0f;

        ApplySlideToWidgets(ActiveSlideIndex, MuseumImage_Current, MuseumTxt_Current, MuseumTitle_Current, MuseumPhotoCredit_Current);

        if (MuseumImage_Current)
        {
            MuseumImage_Current->SetRenderOpacity(1.0f);
        }

        if (MuseumImage_Next)
        {
            MuseumImage_Next->SetRenderOpacity(0.0f);
        }

        if (MuseumTxt_Current)
        {
            MuseumTxt_Current->SetRenderOpacity(1.0f);
        }

        if (MuseumTxt_Next)
        {
            MuseumTxt_Next->SetRenderOpacity(0.0f);
        }

        if (MuseumTitle_Current)
        {
            MuseumTitle_Current->SetRenderOpacity(1.0f);
        }

        if (MuseumTitle_Next)
        {
            MuseumTitle_Next->SetRenderOpacity(0.0f);
        }

        if (MuseumPhotoCredit_Current)
        {
            MuseumPhotoCredit_Current->SetRenderOpacity(1.0f);
        }

        if (MuseumPhotoCredit_Next)
        {
            MuseumPhotoCredit_Next->SetRenderOpacity(0.0f);
        }

        TimeUntilNextSlide = FMath::Max(0.1f, SlideIntervalSeconds);
    }
}

// Resolves paths, references, or settings used by this component.
float UMuseumWidget::ResolveStabilizedWrapWidth() const
{
    if (!bStabilizeAutoWrapDuringFade || MuseumTextWrapAt > 0.0f || !MuseumTxt_Current)
    {
        return 0.0f;
    }

    const float CurrentWidth = MuseumTxt_Current->GetCachedGeometry().GetLocalSize().X;
    if (CurrentWidth >= MinimumStabilizedWrapWidth)
    {
        return CurrentWidth;
    }

    return 0.0f;
}

// Applies optional slide text to the target text widget.
void UMuseumWidget::SetOptionalText(UTextBlock* TextWidget, const TArray<FText>& TextArray, int32 SlideIndex)
{
    if (!TextWidget)
    {
        return;
    }

    if (TextArray.IsValidIndex(SlideIndex))
    {
        TextWidget->SetText(TextArray[SlideIndex]);
    }
    else
    {
        TextWidget->SetText(FText::GetEmpty());
    }
}
