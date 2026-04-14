#include "ChangeLocationWidget.h"
#include "ChangeLocationEntryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"

void UChangeLocationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RebindLocationEntries();
}

void UChangeLocationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RebindLocationEntries();
}

void UChangeLocationWidget::RebindLocationEntries()
{
    CachedEntries.Empty();

    if (!CL_LIST)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationWidget: CL_LIST is null"));
        return;
    }

    TArray<UChangeLocationEntryWidget*> FoundEntries;
    CollectEntriesRecursive(CL_LIST, FoundEntries);

    for (UChangeLocationEntryWidget* Entry : FoundEntries)
    {
        if (!Entry)
        {
            continue;
        }

        CachedEntries.Add(Entry);

        Entry->OnEntryHovered.RemoveDynamic(this, &UChangeLocationWidget::HandleEntryHovered);
        Entry->OnEntryHovered.AddDynamic(this, &UChangeLocationWidget::HandleEntryHovered);

        Entry->OnEntryClicked.RemoveDynamic(this, &UChangeLocationWidget::HandleEntryClicked);
        Entry->OnEntryClicked.AddDynamic(this, &UChangeLocationWidget::HandleEntryClicked);
    }

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationWidget: Bound %d ChangeLocationEntryWidget instance(s) from CL_LIST"), CachedEntries.Num());
}

void UChangeLocationWidget::CollectEntriesRecursive(UWidget* RootWidget, TArray<UChangeLocationEntryWidget*>& OutEntries)
{
    if (!RootWidget)
    {
        return;
    }

    if (UChangeLocationEntryWidget* Entry = Cast<UChangeLocationEntryWidget>(RootWidget))
    {
        OutEntries.Add(Entry);
        return;
    }

    if (UPanelWidget* Panel = Cast<UPanelWidget>(RootWidget))
    {
        const int32 ChildCount = Panel->GetChildrenCount();
        for (int32 i = 0; i < ChildCount; ++i)
        {
            CollectEntriesRecursive(Panel->GetChildAt(i), OutEntries);
        }
    }
}

void UChangeLocationWidget::HandleEntryHovered(FName VariantID)
{
    UE_LOG(LogTemp, Verbose, TEXT("ChangeLocationWidget: Hovered variant '%s'"), *VariantID.ToString());
}

void UChangeLocationWidget::HandleEntryClicked(FName VariantID)
{
    if (VariantID == NAME_None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeLocationWidget: Clicked entry has invalid VariantID"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ChangeLocationWidget: Requested variant '%s'"), *VariantID.ToString());

    OnChangeLocationRequested.Broadcast(VariantID);
    BP_OnChangeLocationRequested(VariantID);
}