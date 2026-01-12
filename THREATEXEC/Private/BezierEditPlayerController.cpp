#include "BezierEditPlayerController.h"

#include "BezierEditSubsystem.h"
#include "BezierEditable.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"

ABezierEditPlayerController::ABezierEditPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	DefaultMouseCursor = EMouseCursor::Default;

	PrimaryActionNames = { "Secondary" };
}

void ABezierEditPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// These action names must exist in Project Settings -> Input
	if (InputComponent)
	{
		if (PrimaryActionNames.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("BezierEditPlayerController: no PrimaryActionNames configured."));
		}

		for (const FName& ActionName : PrimaryActionNames)
		{
			if (!ActionName.IsNone())
			{
				InputComponent->BindAction(ActionName, IE_Pressed, this, &ABezierEditPlayerController::Input_PrimaryPressed);
				InputComponent->BindAction(ActionName, IE_Released, this, &ABezierEditPlayerController::Input_PrimaryReleased);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("BezierEditPlayerController: PrimaryActionNames contains None."));
			}
		}

		if (!CancelActionName.IsNone())
		{
			InputComponent->BindAction(CancelActionName, IE_Pressed, this, &ABezierEditPlayerController::Input_Cancel);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BezierEditPlayerController: CancelActionName is None."));
		}
	}
}

void ABezierEditPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);

	if (bDragging)
	{
		UpdateDrag();
	}
	else
	{
		UpdateHover();
	}
}

bool ABezierEditPlayerController::TraceUnderCursor(FHitResult& OutHit) const
{
	OutHit = FHitResult();

	// Uses visibility traces. Your ControlPointISM must block ECC_Visibility when edit mode is enabled.
	return GetHitResultUnderCursor(ECC_Visibility, true, OutHit);
}

void ABezierEditPlayerController::UpdateHover()
{
	FHitResult Hit;
	const bool bHit = TraceUnderCursor(Hit);

	AActor* NewActor = bHit ? Hit.GetActor() : nullptr;
	int32 NewIndex = bHit ? Hit.Item : -1;

	if (!bHit || !NewActor)
	{
		ReportDebugMessage(TEXT("Hover: none"));
		ClearHovered();
		return;
	}

	// If this is an editable actor, set focus in the edit subsystem.
	if (UWorld* W = GetWorld())
	{
		if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
		{
			if (NewActor->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
			{
				Sub->SetFocused(NewActor);
			}
		}
	}

	// Hover only matters for control point instances (Hit.Item).
	if (NewIndex == INDEX_NONE)
	{
		ReportDebugMessage(FString::Printf(TEXT("Hover: %s (no control point)"), *NewActor->GetName()));
		ClearHovered();
		return;
	}

	if (HoveredActor.Get() != NewActor || HoveredIndex != NewIndex)
	{
		ReportDebugMessage(FString::Printf(TEXT("Hover: %s idx %d"), *NewActor->GetName(), NewIndex));
		ClearHovered();
		HoveredActor = NewActor;
		HoveredIndex = NewIndex;
		SetHoveredOnActor(NewActor, NewIndex);
	}
}

void ABezierEditPlayerController::ClearHovered()
{
	if (AActor* A = HoveredActor.Get())
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			A3->UI_ClearHoveredControlPoint();
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			A2->UI_ClearHoveredControlPoint();
		}
	}

	HoveredActor = nullptr;
	HoveredIndex = -1;
}

void ABezierEditPlayerController::SetHoveredOnActor(AActor* Actor, int32 ControlPointIndex) const
{
	if (!Actor || ControlPointIndex < 0) return;

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Actor))
	{
		A3->UI_SetHoveredControlPoint(ControlPointIndex);
	}
	else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Actor))
	{
		A2->UI_SetHoveredControlPoint(ControlPointIndex);
	}
}

void ABezierEditPlayerController::Input_PrimaryPressed()
{
	FHitResult Hit;
	if (!TraceUnderCursor(Hit)) return;

	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	// Focus editable actor on click.
	if (UWorld* W = GetWorld())
	{
		if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
		{
			if (HitActor->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
			{
				Sub->SetFocused(HitActor);
			}
		}
	}

	StartDrag(Hit);
}

void ABezierEditPlayerController::Input_PrimaryReleased()
{
	StopDrag();
}

void ABezierEditPlayerController::Input_Cancel()
{
	StopDrag();
	ClearHovered();

	if (UWorld* W = GetWorld())
	{
		if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
		{
			AActor* Focused = Sub->GetFocused();
			ClearSelectedOnActor(Focused);
		}
	}
}

void ABezierEditPlayerController::ClearSelectedOnActor(AActor* Actor) const
{
	if (!Actor) return;

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Actor))
	{
		A3->UI_ClearSelectedControlPoint();
	}
	else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Actor))
	{
		A2->UI_ClearSelectedControlPoint();
	}
}

void ABezierEditPlayerController::StartDrag(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	const int32 CPIndex = Hit.Item;
	if (CPIndex == INDEX_NONE) return;

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(HitActor);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(HitActor);
	if (!A3 && !A2) return;

	const bool bSelected = (A3 ? A3->UI_SelectFromHit(Hit) : A2->UI_SelectFromHit(Hit));
	if (!bSelected) return;

	DraggedActor = HitActor;
	DraggedIndex = CPIndex;
	bDragging = true;

	DragPlanePoint = Hit.ImpactPoint;

	// Plane normal:
	// - 2D: always XY plane (normal Up)
	// - 3D: screen-space dragging (plane normal = -camera forward)
	const FVector CamForward = PlayerCameraManager ? PlayerCameraManager->GetActorForwardVector() : FVector(1, 0, 0);

	if (A2)
	{
		DragPlaneNormal = FVector::UpVector;
	}
	else
	{
		DragPlaneNormal = -CamForward.GetSafeNormal();
		if (DragPlaneNormal.IsNearlyZero())
		{
			DragPlaneNormal = FVector::UpVector;
		}
	}

	ClearHovered();
}

void ABezierEditPlayerController::UpdateDrag()
{
	AActor* A = DraggedActor.Get();
	if (!A || DraggedIndex < 0)
	{
		StopDrag();
		return;
	}

	FVector WorldPoint;
	if (!DeprojectMouseToPlane(DragPlanePoint, DragPlaneNormal, WorldPoint))
	{
		return;
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
	{
		A3->UI_SetControlPointWorld(DraggedIndex, WorldPoint);
	}
	else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
	{
		A2->UI_SetControlPointWorld(DraggedIndex, WorldPoint);
	}
	else
	{
		StopDrag();
	}
}

void ABezierEditPlayerController::StopDrag()
{
	bDragging = false;
	DraggedActor = nullptr;
	DraggedIndex = -1;
}

bool ABezierEditPlayerController::DeprojectMouseToPlane(const FVector& PlanePoint, const FVector& PlaneNormal, FVector& OutWorldPoint) const
{
	OutWorldPoint = FVector::ZeroVector;

	FVector WorldOrigin, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection))
	{
		return false;
	}

	const FVector N = PlaneNormal.GetSafeNormal();
	const float Denom = FVector::DotProduct(WorldDirection, N);
	if (FMath::Abs(Denom) < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float T = FVector::DotProduct((PlanePoint - WorldOrigin), N) / Denom;
	if (T < 0.0f)
	{
		return false;
	}

	OutWorldPoint = WorldOrigin + WorldDirection * T;
	return true;
}
