#include "MuseumWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UMuseumWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ActiveSlideIndex = 0;
    PendingSlideIndex = INDEX_NONE;
    bTransitioning = false;
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

void UMuseumWidget::AddMuseumImage(UTexture2D* MuseumImage)
{
    MuseumImages.Add(MuseumImage);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

void UMuseumWidget::AddMuseumText(const FText& MuseumTxt)
{
    MuseumTexts.Add(MuseumTxt);
    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

void UMuseumWidget::AddMuseumSlide(UTexture2D* MuseumImage, const FText& MuseumTxt)
{
    MuseumImages.Add(MuseumImage);
    MuseumTexts.Add(MuseumTxt);

    if (GetSlideCount() == 1)
    {
        InitializeVisibleSlide();
    }
}

void UMuseumWidget::ClearMuseumSlides()
{
    MuseumImages.Empty();
    MuseumTexts.Empty();

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
}

void UMuseumWidget::StartCycling()
{
    bCycling = true;
    TimeUntilNextSlide = FMath::Max(0.1f, SlideIntervalSeconds);
}

void UMuseumWidget::StopCycling()
{
    bCycling = false;
}

int32 UMuseumWidget::GetSlideCount() const
{
    return FMath::Max(MuseumImages.Num(), MuseumTexts.Num());
}

void UMuseumWidget::InitializeVisibleSlide()
{
    if (GetSlideCount() <= 0)
    {
        return;
    }

    ActiveSlideIndex = FMath::Clamp(ActiveSlideIndex, 0, GetSlideCount() - 1);
    ApplySlideToWidgets(ActiveSlideIndex, MuseumImage_Current, MuseumTxt_Current);

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetRenderOpacity(0.0f);
    }
}

void UMuseumWidget::BeginTransitionTo(int32 NewSlideIndex)
{
    if (bTransitioning || GetSlideCount() <= 1)
    {
        return;
    }

    PendingSlideIndex = FMath::Clamp(NewSlideIndex, 0, GetSlideCount() - 1);
    FadeProgressSeconds = 0.0f;
    bTransitioning = true;

    ApplySlideToWidgets(PendingSlideIndex, MuseumImage_Next, MuseumTxt_Next);

    if (MuseumImage_Next)
    {
        MuseumImage_Next->SetRenderOpacity(0.0f);
    }

    if (MuseumTxt_Next)
    {
        MuseumTxt_Next->SetRenderOpacity(0.0f);
    }
}

void UMuseumWidget::ApplySlideToWidgets(int32 SlideIndex, UImage* ImageWidget, UTextBlock* TextWidget) const
{
    SetImageFromSlide(ImageWidget, SlideIndex);

    if (TextWidget)
    {
        if (MuseumTexts.IsValidIndex(SlideIndex))
        {
            TextWidget->SetText(MuseumTexts[SlideIndex]);
        }
        else
        {
            TextWidget->SetText(FText::GetEmpty());
        }

        ApplyTextStyle(TextWidget);
    }
}

void UMuseumWidget::ApplyTextStyle(UTextBlock* TextWidget) const
{
    if (!TextWidget || !bOverrideMuseumTextStyle)
    {
        return;
    }

    TextWidget->SetFont(MuseumTextFont);
    TextWidget->SetColorAndOpacity(FSlateColor(MuseumTextColor));
    TextWidget->SetJustification(MuseumTextJustification);

    if (MuseumTextWrapAt > 0.0f)
    {
        TextWidget->SetAutoWrapText(false);
        TextWidget->SetWrapTextAt(MuseumTextWrapAt);
    }
    else
    {
        TextWidget->SetAutoWrapText(true);
        TextWidget->SetWrapTextAt(0.0f);
    }
}

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

    if (Alpha >= 1.0f)
    {
        ActiveSlideIndex = PendingSlideIndex;
        PendingSlideIndex = INDEX_NONE;
        bTransitioning = false;

        ApplySlideToWidgets(ActiveSlideIndex, MuseumImage_Current, MuseumTxt_Current);

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

        TimeUntilNextSlide = FMath::Max(0.1f, SlideIntervalSeconds);
    }
}
