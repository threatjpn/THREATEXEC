#include "PhotoLocationsMenuWidget.h"
#include "PhotoLocationButtonWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

void UPhotoLocationsMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    RebuildBindings();

    if (PreviewSwapAnim)
    {
        BindToAnimationFinished(PreviewSwapAnim, FWidgetAnimationDynamicEvent::CreateUFunction(this, FName("HandlePreviewSwapFinished")));
    }

    SetIsFocusable(true);
}

void UPhotoLocationsMenuWidget::RebuildBindings()
{
    BuildPreviewMap();
    BindButtonsFromList();

    if (bSelectFirstEntryOnInit && CachedButtons.Num() > 0 && CachedButtons[0])
    {
        CurrentIndex = 0;
        CurrentButtonID = CachedButtons[0]->ButtonID;
        ShowPreviewForID(CurrentButtonID, true);
        ApplyButtonStates();
    }
}

void UPhotoLocationsMenuWidget::BuildPreviewMap()
{
    PreviewMap.Empty();

    for (const FPhotoLocationPreviewEntry& Entry : PreviewEntries)
    {
        if (Entry.ButtonID != NAME_None && Entry.PreviewTexture)
        {
            PreviewMap.Add(Entry.ButtonID, Entry.PreviewTexture);
        }
    }
}

void UPhotoLocationsMenuWidget::BindButtonsFromList()
{
    CachedButtons.Empty();

    if (!PL_LIST)
    {
        UE_LOG(LogTemp, Warning, TEXT("PhotoLocationsMenuWidget: PL_LIST is null"));
        return;
    }

    const int32 ChildCount = PL_LIST->GetChildrenCount();
    for (int32 i = 0; i < ChildCount; ++i)
    {
        UWidget* Child = PL_LIST->GetChildAt(i);
        UPhotoLocationButtonWidget* ButtonWidget = Cast<UPhotoLocationButtonWidget>(Child);
        if (!ButtonWidget)
        {
            continue;
        }

        CachedButtons.Add(ButtonWidget);

        ButtonWidget->OnPhotoButtonHovered.RemoveDynamic(this, &UPhotoLocationsMenuWidget::HandleButtonHovered);
        ButtonWidget->OnPhotoButtonClicked.RemoveDynamic(this, &UPhotoLocationsMenuWidget::HandleButtonClicked);

        ButtonWidget->OnPhotoButtonHovered.AddDynamic(this, &UPhotoLocationsMenuWidget::HandleButtonHovered);
        ButtonWidget->OnPhotoButtonClicked.AddDynamic(this, &UPhotoLocationsMenuWidget::HandleButtonClicked);
    }
}

void UPhotoLocationsMenuWidget::SetImageTexture(UImage* ImageWidget, UTexture2D* Texture) const
{
    if (ImageWidget && Texture)
    {
        ImageWidget->SetBrushFromTexture(Texture, true);
    }
}

int32 UPhotoLocationsMenuWidget::FindButtonIndexByID(FName InButtonID) const
{
    for (int32 i = 0; i < CachedButtons.Num(); ++i)
    {
        if (CachedButtons[i] && CachedButtons[i]->ButtonID == InButtonID)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

void UPhotoLocationsMenuWidget::ApplyButtonStates()
{
    for (int32 i = 0; i < CachedButtons.Num(); ++i)
    {
        UPhotoLocationButtonWidget* Button = CachedButtons[i];
        if (!Button)
        {
            continue;
        }

        const bool bIsHovered = (Button->ButtonID == CurrentButtonID);
        const bool bIsSelected = (i == CurrentIndex);

        Button->SetVisualHovered(bIsHovered);
        Button->SetVisualSelected(bIsSelected);
    }
}

void UPhotoLocationsMenuWidget::HandleButtonHovered(FName ButtonID)
{
    ShowPreviewForID(ButtonID, false);
}

void UPhotoLocationsMenuWidget::HandleButtonClicked(FName ButtonID)
{
    const int32 FoundIndex = FindButtonIndexByID(ButtonID);
    if (FoundIndex != INDEX_NONE)
    {
        CurrentIndex = FoundIndex;
        CurrentButtonID = ButtonID;
        ApplyButtonStates();
        OnLocationClicked.Broadcast(ButtonID);
    }
}

void UPhotoLocationsMenuWidget::FinalizeTransitionIfNeeded()
{
    if (!PreviewImage_Current || !PreviewImage_Next)
    {
        return;
    }

    UImage* FrontImage = bUsingCurrentAsFront ? PreviewImage_Current : PreviewImage_Next;
    UImage* BackImage = bUsingCurrentAsFront ? PreviewImage_Next : PreviewImage_Current;

    FrontImage->SetRenderOpacity(1.0f);
    BackImage->SetRenderOpacity(0.0f);
    BackImage->SetRenderTranslation(FVector2D::ZeroVector);
    FrontImage->SetRenderTranslation(FVector2D::ZeroVector);
}

void UPhotoLocationsMenuWidget::ShowPreviewForID(FName InButtonID, bool bInstant)
{
    if (InButtonID == NAME_None)
    {
        return;
    }

    UTexture2D* const* FoundTexture = PreviewMap.Find(InButtonID);
    if (!FoundTexture || !(*FoundTexture))
    {
        UE_LOG(LogTemp, Warning, TEXT("PhotoLocationsMenuWidget: No preview found for ButtonID '%s'"), *InButtonID.ToString());
        return;
    }

    const int32 FoundIndex = FindButtonIndexByID(InButtonID);
    if (FoundIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("PhotoLocationsMenuWidget: No button found for ButtonID '%s'"), *InButtonID.ToString());
        return;
    }

    if (bTransitionPlaying)
    {
        PendingButtonID = InButtonID;
        return;
    }

    if (CurrentButtonID == InButtonID && !bInstant)
    {
        return;
    }

    CurrentButtonID = InButtonID;
    CurrentIndex = FoundIndex;
    ApplyButtonStates();

    if (!PreviewImage_Current || !PreviewImage_Next)
    {
        return;
    }

    UImage* FrontImage = bUsingCurrentAsFront ? PreviewImage_Current : PreviewImage_Next;
    UImage* BackImage = bUsingCurrentAsFront ? PreviewImage_Next : PreviewImage_Current;

    SetImageTexture(BackImage, *FoundTexture);

    if (bInstant || !PreviewSwapAnim)
    {
        SetImageTexture(FrontImage, *FoundTexture);
        FrontImage->SetRenderOpacity(1.0f);
        BackImage->SetRenderOpacity(0.0f);
        FrontImage->SetRenderTranslation(FVector2D::ZeroVector);
        BackImage->SetRenderTranslation(FVector2D::ZeroVector);
        return;
    }

    StopAnimation(PreviewSwapAnim);

    FrontImage->SetRenderOpacity(1.0f);
    BackImage->SetRenderOpacity(0.0f);
    FrontImage->SetRenderTranslation(FVector2D::ZeroVector);
    BackImage->SetRenderTranslation(FVector2D(30.0f, 0.0f));

    bTransitionPlaying = true;
    PlayAnimationForward(PreviewSwapAnim);
}

void UPhotoLocationsMenuWidget::HandlePreviewSwapFinished()
{
    if (!PreviewImage_Current || !PreviewImage_Next)
    {
        bTransitionPlaying = false;
        return;
    }

    UImage* FrontImage = bUsingCurrentAsFront ? PreviewImage_Current : PreviewImage_Next;
    UImage* BackImage = bUsingCurrentAsFront ? PreviewImage_Next : PreviewImage_Current;

    FrontImage->SetBrush(BackImage->GetBrush());
    FrontImage->SetRenderOpacity(1.0f);
    FrontImage->SetRenderTranslation(FVector2D::ZeroVector);

    BackImage->SetRenderOpacity(0.0f);
    BackImage->SetRenderTranslation(FVector2D::ZeroVector);

    bUsingCurrentAsFront = !bUsingCurrentAsFront;
    bTransitionPlaying = false;

    if (PendingButtonID != NAME_None && PendingButtonID != CurrentButtonID)
    {
        const FName QueuedID = PendingButtonID;
        PendingButtonID = NAME_None;
        ShowPreviewForID(QueuedID, false);
    }
    else
    {
        PendingButtonID = NAME_None;
    }
}

void UPhotoLocationsMenuWidget::FocusNext()
{
    if (CachedButtons.Num() == 0)
    {
        return;
    }

    int32 NewIndex = CurrentIndex;
    if (NewIndex == INDEX_NONE)
    {
        NewIndex = 0;
    }
    else
    {
        NewIndex = FMath::Clamp(NewIndex + 1, 0, CachedButtons.Num() - 1);
    }

    if (CachedButtons.IsValidIndex(NewIndex) && CachedButtons[NewIndex])
    {
        ShowPreviewForID(CachedButtons[NewIndex]->ButtonID, false);
    }
}

void UPhotoLocationsMenuWidget::FocusPrevious()
{
    if (CachedButtons.Num() == 0)
    {
        return;
    }

    int32 NewIndex = CurrentIndex;
    if (NewIndex == INDEX_NONE)
    {
        NewIndex = 0;
    }
    else
    {
        NewIndex = FMath::Clamp(NewIndex - 1, 0, CachedButtons.Num() - 1);
    }

    if (CachedButtons.IsValidIndex(NewIndex) && CachedButtons[NewIndex])
    {
        ShowPreviewForID(CachedButtons[NewIndex]->ButtonID, false);
    }
}

void UPhotoLocationsMenuWidget::ConfirmCurrentSelection()
{
    if (CurrentButtonID != NAME_None)
    {
        OnLocationClicked.Broadcast(CurrentButtonID);
    }
}

FReply UPhotoLocationsMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (!bEnableKeyboardNavigation)
    {
        return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
    }

    const FKey Key = InKeyEvent.GetKey();

    if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up)
    {
        FocusPrevious();
        return FReply::Handled();
    }

    if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down)
    {
        FocusNext();
        return FReply::Handled();
    }

    if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept || Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        ConfirmCurrentSelection();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}