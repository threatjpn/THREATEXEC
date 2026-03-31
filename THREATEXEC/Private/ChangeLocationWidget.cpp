#include "ChangeLocationWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"

void UChangeLocationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RebindLocationButtons();
}

void UChangeLocationWidget::RebindLocationButtons()
{
    CachedButtons.Empty();
    LastHoveredButton = nullptr;

    if (!CL_LIST)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationWidget: CL_LIST is null"));
        return;
    }

    TArray<UButton*> FoundButtons;
    CollectButtonsRecursive(CL_LIST, FoundButtons);

    for (UButton* Button : FoundButtons)
    {
        if (!Button)
        {
            continue;
        }

        CachedButtons.Add(Button);

        Button->OnHovered.RemoveDynamic(this, &UChangeLocationWidget::HandleAnyButtonHovered);
        Button->OnHovered.AddDynamic(this, &UChangeLocationWidget::HandleAnyButtonHovered);

        Button->OnClicked.RemoveDynamic(this, &UChangeLocationWidget::HandleAnyButtonClicked);
        Button->OnClicked.AddDynamic(this, &UChangeLocationWidget::HandleAnyButtonClicked);
    }

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationWidget: Bound %d plain Button widget(s) from CL_LIST"), CachedButtons.Num());
}

void UChangeLocationWidget::CollectButtonsRecursive(UWidget* RootWidget, TArray<UButton*>& OutButtons)
{
    if (!RootWidget)
    {
        return;
    }

    if (UButton* Button = Cast<UButton>(RootWidget))
    {
        OutButtons.Add(Button);
        return;
    }

    if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
    {
        const int32 ChildCount = Panel->GetChildrenCount();
        for (int32 i = 0; i < ChildCount; ++i)
        {
            CollectButtonsRecursive(Panel->GetChildAt(i), OutButtons);
        }
    }
}

UButton* UChangeLocationWidget::ResolveHoveredButton() const
{
    if (LastHoveredButton && LastHoveredButton->IsHovered())
    {
        return LastHoveredButton;
    }

    for (UButton* Button : CachedButtons)
    {
        if (Button && Button->IsHovered())
        {
            return Button;
        }
    }

    return nullptr;
}

void UChangeLocationWidget::HandleAnyButtonHovered()
{
    LastHoveredButton = ResolveHoveredButton();
}

void UChangeLocationWidget::HandleAnyButtonClicked()
{
    UButton* SourceButton = ResolveHoveredButton();

    if (!SourceButton)
    {
        SourceButton = LastHoveredButton;
    }

    if (!SourceButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationWidget: Click received but no hovered button could be resolved"));
        return;
    }

    const FName VariantID = SourceButton->GetFName();

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationWidget: Requested variant '%s'"), *VariantID.ToString());

    OnChangeLocationRequested.Broadcast(VariantID);
    BP_OnChangeLocationRequested(VariantID);
}