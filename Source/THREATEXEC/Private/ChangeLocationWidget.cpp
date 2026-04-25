// ============================================================================
// ChangeLocationWidget.cpp
// Builds and manages the change-location menu entries.
//
// Comments are documentation-only and do not alter behaviour.
// ============================================================================

/**
 * File: ChangeLocationWidget.cpp
 * Summary: Implementation of the change-location selection widget and its child-entry rebinding logic.
 * Note: Comments added for maintainability only. Behaviour and public API remain unchanged.
 */

#include "ChangeLocationWidget.h"
#include "ChangeLocationEntryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Widget.h"

/** Performs one-time widget initialisation. */

// Handles native on initialized.
void UChangeLocationWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    RebindLocationEntries();
}

/** Refreshes entry bindings when the widget is constructed. */

// Initialises widget state and binds required UI behaviour.
void UChangeLocationWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RebindLocationEntries();
}

/** Rebuilds widget entry bindings after construction or dynamic UI changes. */

// Handles rebind location entries.
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

// Handles collect entries recursive.
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

// Handles an event from UI, input, or runtime state.
void UChangeLocationWidget::HandleEntryHovered(FName VariantID)
{
    UE_LOG(LogTemp, Verbose, TEXT("ChangeLocationWidget: Hovered variant '%s'"), *VariantID.ToString());
}

// Handles a clicked location entry and forwards the selection.
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
