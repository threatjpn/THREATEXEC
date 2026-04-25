// ============================================================================
// BezierSelectionSubsystem.cpp
// Implements shared selection-state management for Bézier actors and their editable control points.
//
// Notes:
// - Comments in this file are documentation-only and do not alter behaviour.
// - Function signatures, ordering, and implementation logic are preserved.
// ============================================================================

#include "BezierSelectionSubsystem.h"

#include "GameFramework/Actor.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

// Sets default components, ticking, collision, and editable values.
UBezierSelectionSubsystem::UBezierSelectionSubsystem()
{
}

// Checks whether the actor can be selected by this subsystem.
bool UBezierSelectionSubsystem::IsAllowedActor(AActor* InActor) const
{
	if (!InActor) return true;
	if (!bOnlyAllowBezierActors) return true;

	return InActor->IsA<ABezierCurve2DActor>() || InActor->IsA<ABezierCurve3DActor>();
}

// Updates the currently focused Bezier actor and broadcasts the change.
void UBezierSelectionSubsystem::SetFocusedActor(AActor* InActor)
{
	if (!IsAllowedActor(InActor))
	{
		return;
	}

	AActor* Current = FocusedActor.Get();
	if (Current == InActor)
	{
		return;
	}

	FocusedActor = InActor;
	OnFocusedBezierActorChanged.Broadcast(InActor);
}

// Clears the current focused actor selection.
void UBezierSelectionSubsystem::ClearFocusedActor()
{
	SetFocusedActor(nullptr);
}

// Returns the currently focused actor, if one is still valid.
AActor* UBezierSelectionSubsystem::GetFocusedActor() const
{
	return FocusedActor.Get();
}

// Reports whether a valid focused actor exists.
bool UBezierSelectionSubsystem::HasFocusedActor() const
{
	return FocusedActor.IsValid();
}
