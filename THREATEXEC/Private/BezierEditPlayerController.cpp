#include "BezierEditPlayerController.h"

#include "BezierEditSubsystem.h"
#include "BezierEditable.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"

namespace BezierEditPlayerControllerHelpers
{
	const TCHAR* PivotHandleLabel(EBezierPivotHandle Handle)
	{
		switch (Handle)
		{
		case EBezierPivotHandle::TranslateX:
			return TEXT("Translate X");
		case EBezierPivotHandle::TranslateY:
			return TEXT("Translate Y");
		case EBezierPivotHandle::TranslateZ:
			return TEXT("Translate Z");
		case EBezierPivotHandle::RotateX:
			return TEXT("Rotate X");
		case EBezierPivotHandle::RotateY:
			return TEXT("Rotate Y");
		case EBezierPivotHandle::RotateZ:
			return TEXT("Rotate Z");
		default:
			return TEXT("None");
		}
	}

	bool ClosestPointRayLine(const FVector& RayOrigin, const FVector& RayDir, const FVector& LineOrigin, const FVector& LineDir, float& OutRayT, float& OutLineT)
	{
		const float A = FVector::DotProduct(RayDir, RayDir);
		const float B = FVector::DotProduct(RayDir, LineDir);
		const float C = FVector::DotProduct(LineDir, LineDir);
		const FVector W0 = RayOrigin - LineOrigin;
		const float D = FVector::DotProduct(RayDir, W0);
		const float E = FVector::DotProduct(LineDir, W0);
		const float Denom = A * C - B * B;
		if (FMath::IsNearlyZero(Denom))
		{
			return false;
		}
		OutRayT = (B * E - C * D) / Denom;
		OutLineT = (A * E - B * D) / Denom;
		return true;
	}
}

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

bool ABezierEditPlayerController::GetMouseRay(FVector& OutOrigin, FVector& OutDirection) const
{
	float ScreenX = 0.0f;
	float ScreenY = 0.0f;
	if (!GetMousePosition(ScreenX, ScreenY))
	{
		return false;
	}

	return DeprojectScreenPositionToWorld(ScreenX, ScreenY, OutOrigin, OutDirection);
}

bool ABezierEditPlayerController::UpdatePivotHover()
{
	if (!GetWorld())
	{
		return false;
	}

	FVector RayOrigin;
	FVector RayDirection;
	if (!GetMouseRay(RayOrigin, RayDirection))
	{
		ClearPivotHover();
		return false;
	}

	AActor* Focused = nullptr;
	if (UBezierEditSubsystem* Sub = GetWorld()->GetSubsystem<UBezierEditSubsystem>())
	{
		Focused = Sub->GetFocused();
	}

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Focused);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Focused);
	if (!A3 && !A2)
	{
		ClearPivotHover();
		return false;
	}

	EBezierPivotHandle Handle = EBezierPivotHandle::None;
	const bool bHasHandle = A3
		? A3->UI_FindPivotHandleFromRay(RayOrigin, RayDirection, Handle)
		: A2->UI_FindPivotHandleFromRay(RayOrigin, RayDirection, Handle);

	if (!bHasHandle)
	{
		ClearPivotHover();
		return false;
	}

	if (A3)
	{
		A3->UI_SetHoveredPivotHandle(Handle);
	}
	else if (A2)
	{
		A2->UI_SetHoveredPivotHandle(Handle);
	}

	HoveredPivotActor = Focused;
	HoveredPivotHandle = Handle;
	ClearHovered();
	ReportDebugMessage(FString::Printf(TEXT("Hover: pivot handle %s"), BezierEditPlayerControllerHelpers::PivotHandleLabel(Handle)));
	return true;
}

void ABezierEditPlayerController::ClearPivotHover()
{
	if (AActor* A = HoveredPivotActor.Get())
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			A3->UI_SetHoveredPivotHandle(EBezierPivotHandle::None);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			A2->UI_SetHoveredPivotHandle(EBezierPivotHandle::None);
		}
	}

	HoveredPivotActor = nullptr;
	HoveredPivotHandle = EBezierPivotHandle::None;
}

void ABezierEditPlayerController::UpdateHover()
{
	if (UpdatePivotHover())
	{
		return;
	}

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
			A3->UI_SetHoveredPivotHandle(EBezierPivotHandle::None);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			A2->UI_ClearHoveredControlPoint();
			A2->UI_SetHoveredPivotHandle(EBezierPivotHandle::None);
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
	if (!TraceUnderCursor(Hit))
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (HoveredPivotHandle != EBezierPivotHandle::None && HoveredPivotActor.IsValid()
			&& GetMouseRay(RayOrigin, RayDirection))
		{
			StartPivotDrag(HoveredPivotActor.Get(), HoveredPivotHandle, RayOrigin, RayDirection);
			return;
		}

		if (UWorld* W = GetWorld())
		{
			if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
			{
				AActor* Focused = Sub->GetFocused();
				ClearSelectedOnActor(Focused);
			}
		}
		bLastPressWasDoubleClick = false;
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!HitActor) return;

	const float NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bHasControlPoint = Hit.Item != INDEX_NONE;
	const bool bSameClickTarget = (LastPrimaryClickActor.Get() == HitActor && LastPrimaryClickIndex == Hit.Item);
	const bool bIsDoubleClick = bHasControlPoint && bSameClickTarget
		&& (LastPrimaryClickTimeSeconds >= 0.0f)
		&& ((NowSeconds - LastPrimaryClickTimeSeconds) <= DoubleClickTimeSeconds);

	LastPrimaryClickTimeSeconds = NowSeconds;
	LastPrimaryClickActor = HitActor;
	LastPrimaryClickIndex = Hit.Item;

	if (bIsDoubleClick)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(HitActor))
		{
			A3->UI_SelectAllControlPoints();
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(HitActor))
		{
			A2->UI_SelectAllControlPoints();
		}
	}
	bLastPressWasDoubleClick = bIsDoubleClick;

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
	ClearPivotHover();

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

	if (HoveredPivotHandle != EBezierPivotHandle::None && HoveredPivotActor.Get() == HitActor)
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (GetMouseRay(RayOrigin, RayDirection))
		{
			StartPivotDrag(HitActor, HoveredPivotHandle, RayOrigin, RayDirection);
			return;
		}
	}

	const int32 CPIndex = Hit.Item;
	if (CPIndex == INDEX_NONE) return;

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(HitActor);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(HitActor);
	if (!A3 && !A2) return;

	const bool bAllSelected = A3 ? A3->UI_AreAllControlPointsSelected() : A2->UI_AreAllControlPointsSelected();
	const bool bAllowAllDrag = bAllSelected && bLastPressWasDoubleClick;
	if (!bAllSelected)
	{
		const bool bSelected = (A3 ? A3->UI_SelectFromHit(Hit) : A2->UI_SelectFromHit(Hit));
		if (!bSelected) return;
	}
	else if (!bAllowAllDrag)
	{
		const bool bSelected = (A3 ? A3->UI_SelectFromHit(Hit) : A2->UI_SelectFromHit(Hit));
		if (!bSelected) return;
	}

	DraggedActor = HitActor;
	DraggedIndex = CPIndex;
	bDragging = true;
	bDragAllControlPoints = false;
	DragStartWorldPoints.Reset();
	bLastPressWasDoubleClick = false;

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

	if (bAllowAllDrag)
	{
		bDragAllControlPoints = A3 ? A3->UI_GetAllControlPointsWorld(DragStartWorldPoints)
			: A2->UI_GetAllControlPointsWorld(DragStartWorldPoints);
		if (!bDragAllControlPoints)
		{
			StopDrag();
			return;
		}
	}

	ClearHovered();
}

void ABezierEditPlayerController::StartPivotDrag(AActor* TargetActor, EBezierPivotHandle Handle, const FVector& RayOrigin, const FVector& RayDirection)
{
	if (!TargetActor || Handle == EBezierPivotHandle::None)
	{
		return;
	}

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(TargetActor);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(TargetActor);
	if (!A3 && !A2)
	{
		return;
	}

	FVector Pivot;
	const bool bHasPivot = A3 ? A3->UI_GetPivotWorld(Pivot) : A2->UI_GetPivotWorld(Pivot);
	if (!bHasPivot)
	{
		return;
	}

	const FTransform Xf = TargetActor->GetActorTransform();
	FVector AxisDir = FVector::ZeroVector;
	switch (Handle)
	{
	case EBezierPivotHandle::TranslateX:
	case EBezierPivotHandle::RotateX:
		AxisDir = Xf.GetUnitAxis(EAxis::X);
		break;
	case EBezierPivotHandle::TranslateY:
	case EBezierPivotHandle::RotateY:
		AxisDir = Xf.GetUnitAxis(EAxis::Y);
		break;
	case EBezierPivotHandle::TranslateZ:
	case EBezierPivotHandle::RotateZ:
		AxisDir = Xf.GetUnitAxis(EAxis::Z);
		break;
	default:
		break;
	}

	if (AxisDir.IsNearlyZero())
	{
		return;
	}

	PivotDragOrigin = Pivot;
	PivotDragAxis = AxisDir.GetSafeNormal();
	ActivePivotHandle = Handle;
	bDraggingPivot = true;
	bDragging = true;
	DraggedActor = TargetActor;
	bLastPressWasDoubleClick = false;

	if (A3)
	{
		A3->UI_SetActivePivotHandle(Handle);
	}
	else if (A2)
	{
		A2->UI_SetActivePivotHandle(Handle);
	}

	if (Handle == EBezierPivotHandle::TranslateX
		|| Handle == EBezierPivotHandle::TranslateY
		|| Handle == EBezierPivotHandle::TranslateZ)
	{
		float RayT = 0.0f;
		float LineT = 0.0f;
		if (!BezierEditPlayerControllerHelpers::ClosestPointRayLine(RayOrigin, RayDirection, PivotDragOrigin, PivotDragAxis, RayT, LineT))
		{
			StopDrag();
			return;
		}
		PivotDragStartAxisParam = LineT;
	}
	else
	{
		const float Denom = FVector::DotProduct(RayDirection, PivotDragAxis);
		if (FMath::IsNearlyZero(Denom))
		{
			StopDrag();
			return;
		}

		const float T = FVector::DotProduct(PivotDragOrigin - RayOrigin, PivotDragAxis) / Denom;
		if (T < 0.0f)
		{
			StopDrag();
			return;
		}

		PivotDragStartVector = (RayOrigin + RayDirection * T) - PivotDragOrigin;
		if (PivotDragStartVector.IsNearlyZero())
		{
			StopDrag();
			return;
		}
	}

	ClearHovered();
	ReportDebugMessage(FString::Printf(TEXT("Pivot drag start: %s"), BezierEditPlayerControllerHelpers::PivotHandleLabel(Handle)));
}

void ABezierEditPlayerController::UpdateDrag()
{
	if (bDraggingPivot)
	{
		FVector RayOrigin;
		FVector RayDirection;
		if (GetMouseRay(RayOrigin, RayDirection))
		{
			UpdatePivotDrag(RayOrigin, RayDirection);
		}
		return;
	}

	AActor* A = DraggedActor.Get();
	if (!A || (!bDragAllControlPoints && DraggedIndex < 0))
	{
		StopDrag();
		return;
	}

	FVector WorldPoint;
	if (!DeprojectMouseToPlane(DragPlanePoint, DragPlaneNormal, WorldPoint))
	{
		return;
	}

	if (bDragAllControlPoints)
	{
		const FVector Delta = WorldPoint - DragPlanePoint;
		TArray<FVector> NewWorldPoints = DragStartWorldPoints;
		for (FVector& P : NewWorldPoints)
		{
			P += Delta;
		}

		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			A3->UI_SetAllControlPointsWorld(NewWorldPoints);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			A2->UI_SetAllControlPointsWorld(NewWorldPoints);
		}
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

void ABezierEditPlayerController::UpdatePivotDrag(const FVector& RayOrigin, const FVector& RayDirection)
{
	AActor* TargetActor = DraggedActor.Get();
	if (!TargetActor)
	{
		StopDrag();
		return;
	}

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(TargetActor);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(TargetActor);
	if (!A3 && !A2)
	{
		StopDrag();
		return;
	}

	if (ActivePivotHandle == EBezierPivotHandle::TranslateX
		|| ActivePivotHandle == EBezierPivotHandle::TranslateY
		|| ActivePivotHandle == EBezierPivotHandle::TranslateZ)
	{
		float RayT = 0.0f;
		float LineT = 0.0f;
		if (!BezierEditPlayerControllerHelpers::ClosestPointRayLine(RayOrigin, RayDirection, PivotDragOrigin, PivotDragAxis, RayT, LineT))
		{
			return;
		}

		const float DeltaParam = LineT - PivotDragStartAxisParam;
		if (FMath::IsNearlyZero(DeltaParam))
		{
			return;
		}

		const FVector Delta = PivotDragAxis * DeltaParam;
		const bool bApplied = A3 ? A3->UI_ApplyPivotTranslation(Delta) : A2->UI_ApplyPivotTranslation(Delta);
		if (!bApplied)
		{
			StopDrag();
			return;
		}

		PivotDragStartAxisParam = LineT;
		return;
	}

	const float Denom = FVector::DotProduct(RayDirection, PivotDragAxis);
	if (FMath::IsNearlyZero(Denom))
	{
		return;
	}

	const float T = FVector::DotProduct(PivotDragOrigin - RayOrigin, PivotDragAxis) / Denom;
	if (T < 0.0f)
	{
		return;
	}

	const FVector CurrentVector = (RayOrigin + RayDirection * T) - PivotDragOrigin;
	if (CurrentVector.IsNearlyZero() || PivotDragStartVector.IsNearlyZero())
	{
		return;
	}

	const FVector Axis = PivotDragAxis.GetSafeNormal();
	const FVector V0 = PivotDragStartVector.GetSafeNormal();
	const FVector V1 = CurrentVector.GetSafeNormal();
	const float Angle = FMath::Atan2(FVector::DotProduct(Axis, FVector::CrossProduct(V0, V1)), FVector::DotProduct(V0, V1));

	if (FMath::IsNearlyZero(Angle))
	{
		return;
	}

	const bool bApplied = A3
		? A3->UI_ApplyPivotRotation(PivotDragOrigin, Axis, Angle)
		: A2->UI_ApplyPivotRotation(PivotDragOrigin, Axis, Angle);
	if (!bApplied)
	{
		StopDrag();
		return;
	}

	PivotDragStartVector = CurrentVector;
}

void ABezierEditPlayerController::StopDrag()
{
	AActor* Dragged = DraggedActor.Get();
	if (Dragged)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Dragged))
		{
			A3->UI_SetActivePivotHandle(EBezierPivotHandle::None);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Dragged))
		{
			A2->UI_SetActivePivotHandle(EBezierPivotHandle::None);
		}
	}

	bDragging = false;
	bDraggingPivot = false;
	DraggedActor = nullptr;
	DraggedIndex = -1;
	bDragAllControlPoints = false;
	DragStartWorldPoints.Reset();
	ActivePivotHandle = EBezierPivotHandle::None;
	PivotDragAxis = FVector::ZeroVector;
	PivotDragOrigin = FVector::ZeroVector;
	PivotDragStartAxisParam = 0.0f;
	PivotDragStartVector = FVector::ZeroVector;
	ClearPivotHover();
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
