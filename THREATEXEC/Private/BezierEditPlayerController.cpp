#include "BezierEditPlayerController.h"

#include "BezierEditSubsystem.h"
#include "BezierEditable.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "InputCoreTypes.h"

ABezierEditPlayerController::ABezierEditPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	DefaultMouseCursor = EMouseCursor::Default;

	PrimaryActionNames = { "Secondary" };
}

ABezierEditPlayerController::~ABezierEditPlayerController() = default;

void ABezierEditPlayerController::CaptureDragBeforeSnapshot(UBezierEditSubsystem* Sub)
{
	if (!Sub)
	{
		bHasDragBeforeSnapshot = false;
		DragBeforeSnapshot.Reset();
		return;
	}

	DragBeforeSnapshot = MakeUnique<FBezierHistorySnapshot>(Sub->CaptureHistorySnapshot());
	bHasDragBeforeSnapshot = true;
}

void ABezierEditPlayerController::CommitDragSnapshotIfNeeded(UBezierEditSubsystem* Sub)
{
	if (!Sub || !bHasDragBeforeSnapshot || !DragBeforeSnapshot.IsValid())
	{
		return;
	}

	Sub->History_CommitInteractiveChange(*DragBeforeSnapshot);
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

		InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ABezierEditPlayerController::Input_Undo);
		InputComponent->BindKey(EKeys::Y, IE_Pressed, this, &ABezierEditPlayerController::Input_Redo);
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

bool ABezierEditPlayerController::FindClosestControlPointOnMouseRay(AActor*& OutActor, int32& OutIndex) const
{
	OutActor = nullptr;
	OutIndex = INDEX_NONE;

	FVector RayOrigin;
	FVector RayDirection;
	if (!GetMouseRay(RayOrigin, RayDirection))
	{
		return false;
	}

	const FVector RayDir = RayDirection.GetSafeNormal();
	if (RayDir.IsNearlyZero())
	{
		return false;
	}

	float BestDistSq = TNumericLimits<float>::Max();
	AActor* BestActor = nullptr;
	int32 BestIndex = INDEX_NONE;

	auto TestPoint = [&](AActor* OwnerActor, int32 PointIndex, const FVector& WorldPoint)
		{
			const FVector ToPoint = WorldPoint - RayOrigin;
			const float AlongRay = FVector::DotProduct(ToPoint, RayDir);

			if (AlongRay < 0.0f || AlongRay > HoverMaxRayDistance)
			{
				return;
			}

			const FVector ClosestPointOnRay = RayOrigin + RayDir * AlongRay;
			const float DistSq = FVector::DistSquared(WorldPoint, ClosestPointOnRay);

			if (DistSq <= FMath::Square(HoverWorldRadius) && DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestActor = OwnerActor;
				BestIndex = PointIndex;
			}
		};

	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<ABezierCurve3DActor> It(W); It; ++It)
		{
			const ABezierCurve3DActor* Curve = *It;
			if (!IsValid(Curve)) continue;
			if (!Curve->UI_GetEditMode()) continue;
			if (!Curve->bShowControlPoints) continue;

			for (int32 i = 0; i < Curve->Control.Num(); ++i)
			{
				const FVector WorldPoint = Curve->GetActorTransform().TransformPosition(Curve->Control[i] * Curve->Scale);
				TestPoint(const_cast<ABezierCurve3DActor*>(Curve), i, WorldPoint);
			}
		}

		for (TActorIterator<ABezierCurve2DActor> It(W); It; ++It)
		{
			const ABezierCurve2DActor* Curve = *It;
			if (!IsValid(Curve)) continue;
			if (!Curve->UI_GetEditMode()) continue;
			if (!Curve->bShowControlPoints) continue;

			for (int32 i = 0; i < Curve->Control.Num(); ++i)
			{
				const FVector LocalPoint(Curve->Control[i].X * Curve->Scale, Curve->Control[i].Y * Curve->Scale, 0.0f);
				const FVector WorldPoint = Curve->GetActorTransform().TransformPosition(LocalPoint);
				TestPoint(const_cast<ABezierCurve2DActor*>(Curve), i, WorldPoint);
			}
		}
	}

	if (BestActor && BestIndex != INDEX_NONE)
	{
		OutActor = BestActor;
		OutIndex = BestIndex;
		return true;
	}

	return false;
}

void ABezierEditPlayerController::UpdateHover()
{
	FHitResult Hit;
	const bool bHit = TraceUnderCursor(Hit);

	AActor* NewActor = nullptr;
	int32 NewIndex = INDEX_NONE;

	if (bHit)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Hit.GetActor()))
		{
			A3->UI_TryResolveControlPointFromHit(Hit, NewIndex);
			NewActor = (NewIndex != INDEX_NONE) ? A3 : nullptr;
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Hit.GetActor()))
		{
			A2->UI_TryResolveControlPointFromHit(Hit, NewIndex);
			NewActor = (NewIndex != INDEX_NONE) ? A2 : nullptr;
		}
	}

	if (!NewActor || NewIndex == INDEX_NONE)
	{
		AActor* FallbackActor = nullptr;
		int32 FallbackIndex = INDEX_NONE;
		if (FindClosestControlPointOnMouseRay(FallbackActor, FallbackIndex))
		{
			NewActor = FallbackActor;
			NewIndex = FallbackIndex;
		}
	}

	if (!NewActor || NewIndex == INDEX_NONE)
	{
		ReportDebugMessage(TEXT("Hover: none"));
		ClearHovered();
		return;
	}

	if (UWorld* W = GetWorld())
	{
		if (UBezierEditSubsystem* Sub = W->GetSubsystem<UBezierEditSubsystem>())
		{
			bool bHasExplicitFocusedSelection = false;
			if (AActor* FocusedActor = Sub->GetFocused())
			{
				if (ABezierCurve3DActor* Focused3D = Cast<ABezierCurve3DActor>(FocusedActor))
				{
					bHasExplicitFocusedSelection = Focused3D->UI_AreAllControlPointsSelected() || Focused3D->UI_GetSelectedControlPointIndex() != INDEX_NONE;
				}
				else if (ABezierCurve2DActor* Focused2D = Cast<ABezierCurve2DActor>(FocusedActor))
				{
					bHasExplicitFocusedSelection = Focused2D->UI_AreAllControlPointsSelected() || Focused2D->UI_GetSelectedControlPointIndex() != INDEX_NONE;
				}
			}

			if (!bHasExplicitFocusedSelection && NewActor->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
			{
				Sub->SetFocused(NewActor);
			}
		}
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

void ABezierEditPlayerController::StartDragFromControlPoint(AActor* HitActor, int32 ControlPointIndex, const FVector& ImpactPoint)
{
	if (!HitActor || ControlPointIndex == INDEX_NONE)
	{
		return;
	}

	ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(HitActor);
	ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(HitActor);
	if (!A3 && !A2)
	{
		return;
	}

	const bool bAllSelected = A3 ? A3->UI_AreAllControlPointsSelected() : A2->UI_AreAllControlPointsSelected();
	const bool bAllowAllDrag = bAllSelected;

	DraggedActor = HitActor;
	DraggedIndex = ControlPointIndex;
	bDragging = true;
	bDragAllControlPoints = false;
	DragStartWorldPoints.Reset();

	DragPlanePoint = ImpactPoint;

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

	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		CaptureDragBeforeSnapshot(Sub);
	}

	ClearHovered();
}

void ABezierEditPlayerController::Input_PrimaryPressed()
{
	FHitResult Hit;
	const bool bHit = TraceUnderCursor(Hit);

	AActor* HitActor = nullptr;
	int32 HitIndex = INDEX_NONE;
	FVector HitPoint = bHit ? Hit.ImpactPoint : FVector::ZeroVector;

	if (bHit)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Hit.GetActor()))
		{
			A3->UI_TryResolveControlPointFromHit(Hit, HitIndex);
			HitActor = (HitIndex != INDEX_NONE) ? A3 : nullptr;
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Hit.GetActor()))
		{
			A2->UI_TryResolveControlPointFromHit(Hit, HitIndex);
			HitActor = (HitIndex != INDEX_NONE) ? A2 : nullptr;
		}
	}

	// Fallback: if the cursor did not directly hit a control-point instance,
	// use the nearest control point near the mouse ray.
	if (!HitActor || HitIndex == INDEX_NONE)
	{
		AActor* FallbackActor = nullptr;
		int32 FallbackIndex = INDEX_NONE;
		if (FindClosestControlPointOnMouseRay(FallbackActor, FallbackIndex))
		{
			HitActor = FallbackActor;
			HitIndex = FallbackIndex;

			if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(FallbackActor))
			{
				if (A3->Control.IsValidIndex(FallbackIndex))
				{
					HitPoint = A3->GetActorTransform().TransformPosition(A3->Control[FallbackIndex] * A3->Scale);
				}
			}
			else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(FallbackActor))
			{
				if (A2->Control.IsValidIndex(FallbackIndex))
				{
					const FVector LocalPoint(
						A2->Control[FallbackIndex].X * A2->Scale,
						A2->Control[FallbackIndex].Y * A2->Scale,
						0.0f
					);
					HitPoint = A2->GetActorTransform().TransformPosition(LocalPoint);
				}
			}
		}
	}

	if (!HitActor || HitIndex == INDEX_NONE)
	{
		ClearSelectedOnAllActors();
		return;
	}

	const float NowSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const bool bHasControlPoint = HitIndex != INDEX_NONE;
	const bool bSameClickTarget = (LastPrimaryClickActor.Get() == HitActor && LastPrimaryClickIndex == HitIndex);
	const bool bIsDoubleClick = bHasControlPoint && bSameClickTarget
		&& (LastPrimaryClickTimeSeconds >= 0.0f)
		&& ((NowSeconds - LastPrimaryClickTimeSeconds) <= DoubleClickTimeSeconds);

	LastPrimaryClickTimeSeconds = NowSeconds;
	LastPrimaryClickActor = HitActor;
	LastPrimaryClickIndex = HitIndex;

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
	else
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(HitActor))
		{
			A3->UI_SelectControlPoint(HitIndex);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(HitActor))
		{
			A2->UI_SelectControlPoint(HitIndex);
		}
	}

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

	StartDragFromControlPoint(HitActor, HitIndex, HitPoint);
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

void ABezierEditPlayerController::Input_Undo()
{
	const bool bCtrlDown = PlayerInput && (PlayerInput->IsPressed(EKeys::LeftControl) || PlayerInput->IsPressed(EKeys::RightControl));
	const bool bShiftDown = PlayerInput && (PlayerInput->IsPressed(EKeys::LeftShift) || PlayerInput->IsPressed(EKeys::RightShift));
	if (!bCtrlDown || bShiftDown)
	{
		return;
	}

	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		StopDrag(false);
		Sub->History_Undo();
	}
}

void ABezierEditPlayerController::Input_Redo()
{
	const bool bCtrlDown = PlayerInput && (PlayerInput->IsPressed(EKeys::LeftControl) || PlayerInput->IsPressed(EKeys::RightControl));
	if (!bCtrlDown)
	{
		return;
	}

	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		StopDrag(false);
		Sub->History_Redo();
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

void ABezierEditPlayerController::ClearSelectedOnAllActors() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It)
	{
		It->UI_ClearSelectedControlPoint();
	}

	for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It)
	{
		It->UI_ClearSelectedControlPoint();
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

	const bool bAllSelected = A3 ? A3->UI_AreAllControlPointsSelected() : A2->UI_AreAllControlPointsSelected();
	const bool bAllowAllDrag = bAllSelected;
	if (!bAllSelected)
	{
		const bool bSelected = (A3 ? A3->UI_SelectFromHit(Hit) : A2->UI_SelectFromHit(Hit));
		if (!bSelected) return;
	}
	DraggedActor = HitActor;
	DraggedIndex = CPIndex;
	bDragging = true;
	bDragAllControlPoints = false;
	DragStartWorldPoints.Reset();

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

	if (UBezierEditSubsystem* Sub = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		CaptureDragBeforeSnapshot(Sub);
	}

	ClearHovered();
}

void ABezierEditPlayerController::UpdateDrag()
{
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

void ABezierEditPlayerController::StopDrag(bool bCommitHistory)
{
	if (bCommitHistory)
	{
		CommitDragSnapshotIfNeeded(GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr);
	}

	bDragging = false;
	DraggedActor = nullptr;
	DraggedIndex = -1;
	bDragAllControlPoints = false;
	DragStartWorldPoints.Reset();
	bHasDragBeforeSnapshot = false;
	DragBeforeSnapshot.Reset();
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
