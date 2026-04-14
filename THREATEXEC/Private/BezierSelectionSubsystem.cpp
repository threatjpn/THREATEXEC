#include "BezierSelectionSubsystem.h"

#include "GameFramework/Actor.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

UBezierSelectionSubsystem::UBezierSelectionSubsystem()
{
}

bool UBezierSelectionSubsystem::IsAllowedActor(AActor* InActor) const
{
	if (!InActor) return true;
	if (!bOnlyAllowBezierActors) return true;

	return InActor->IsA<ABezierCurve2DActor>() || InActor->IsA<ABezierCurve3DActor>();
}

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

void UBezierSelectionSubsystem::ClearFocusedActor()
{
	SetFocusedActor(nullptr);
}

AActor* UBezierSelectionSubsystem::GetFocusedActor() const
{
	return FocusedActor.Get();
}

bool UBezierSelectionSubsystem::HasFocusedActor() const
{
	return FocusedActor.IsValid();
}
