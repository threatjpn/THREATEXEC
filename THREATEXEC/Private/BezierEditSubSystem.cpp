#include "BezierEditSubsystem.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierCurveSetActor.h"
#include "BezierDebugActor.h"
#include "BezierEditable.h"

#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
void ApplyDebugSettingsToCurve(UWorld* World, AActor* CurveActor)
{
	if (!World || !IsValid(CurveActor))
	{
		return;
	}

	for (TActorIterator<ABezierDebugActor> DebugIt(World); DebugIt; ++DebugIt)
	{
		DebugIt->SyncFromWorldState();
		DebugIt->ApplyDebugToCurve(CurveActor);
		break;
	}
}

bool ToggleSnapForEditable(UObject* Obj, bool& bOutNewSnapState)
{
	if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
	{
		bOutNewSnapState = !A2->bSnapToGrid;
		A2->UI_SetSnapToGrid(bOutNewSnapState);
		return true;
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
	{
		bOutNewSnapState = !A3->bSnapToGrid;
		A3->UI_SetSnapToGrid(bOutNewSnapState);
		return true;
	}

	if (Obj && Obj->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetSnapToGrid(Obj);
		bOutNewSnapState = !bCur;
		return true;
	}

	return false;
}

bool AddControlPointAfterSelectedForEditable(UObject* Obj)
{
	if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
	{
		return A2->UI_AddControlPointAfterSelected();
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
	{
		return A3->UI_AddControlPointAfterSelected();
	}

	return Obj && Obj->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()) &&
		IBezierEditable::Execute_BEZ_AddControlPointAfterSelected(Obj);
}

bool DeleteSelectedControlPointForEditable(UObject* Obj)
{
	if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
	{
		return A2->UI_DeleteSelectedControlPoint();
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
	{
		return A3->UI_DeleteSelectedControlPoint();
	}

	return Obj && Obj->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()) &&
		IBezierEditable::Execute_BEZ_DeleteSelectedControlPoint(Obj);
}

bool DuplicateSelectedControlPointForEditable(UObject* Obj)
{
	if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
	{
		return A2->UI_DuplicateSelectedControlPoint();
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
	{
		return A3->UI_DuplicateSelectedControlPoint();
	}

	return Obj && Obj->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()) &&
		IBezierEditable::Execute_BEZ_DuplicateSelectedControlPoint(Obj);
}
}


static bool NearlyEqualTransform(const FTransform& A, const FTransform& B)
{
	return A.GetLocation().Equals(B.GetLocation(), KINDA_SMALL_NUMBER)
		&& A.GetRotation().Equals(B.GetRotation(), KINDA_SMALL_NUMBER)
		&& A.GetScale3D().Equals(B.GetScale3D(), KINDA_SMALL_NUMBER);
}

static bool SnapshotArraysEqual(const TArray<FVector>& A, const TArray<FVector>& B)
{
	if (A.Num() != B.Num()) return false;
	for (int32 i = 0; i < A.Num(); ++i)
	{
		if (!A[i].Equals(B[i], KINDA_SMALL_NUMBER)) return false;
	}
	return true;
}

static bool SnapshotArraysEqual(const TArray<FVector2D>& A, const TArray<FVector2D>& B)
{
	if (A.Num() != B.Num()) return false;
	for (int32 i = 0; i < A.Num(); ++i)
	{
		if (!A[i].Equals(B[i], KINDA_SMALL_NUMBER)) return false;
	}
	return true;
}



bool UBezierEditSubsystem::CaptureCurveSnapshot(AActor* Actor, FBezierCurveActorSnapshot& Out) const
{
	if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Actor))
	{
		Out.Actor = A2;
		Out.ActorClass = A2->GetClass();
		Out.Owner = A2->GetOwner();
		Out.Transform = A2->GetActorTransform();
		Out.bIs2D = true;
		Out.Control2D = A2->Control;
		Out.Scale = A2->Scale;
		Out.ControlPointVisualScale = A2->ControlPointVisualScale;
		Out.bEditMode = A2->UI_GetEditMode();
		Out.bClosedLoop = A2->UI_IsClosedLoop();
		Out.bEnableRuntimeEditing = A2->bEnableRuntimeEditing;
		Out.bHideVisualsWhenNotEditing = A2->bHideVisualsWhenNotEditing;
		Out.bActorVisibleInGame = A2->bActorVisibleInGame;
		Out.bShowControlPoints = A2->bShowControlPoints;
		Out.bShowStrip = A2->bShowStripMesh;
		Out.bUseCubeStrip = A2->bUseCubeStrip;
		Out.bSnapToGrid = A2->bSnapToGrid;
		Out.bShowGrid = A2->bShowGrid;
		Out.GridSizeCm = A2->GridSizeCm;
		Out.GridExtentCm = A2->GridExtentCm;
		Out.GridOriginWorld = A2->GridOriginWorld;
		Out.GridColor = A2->GridColor;
		Out.GridBaseAlpha = A2->GridBaseAlpha;
		Out.bShowGridXY = A2->bShowGridXY;
		Out.bLockToLocalXY = A2->bLockToLocalXY;
		Out.bForcePlanar = A2->bForcePlanar;
		Out.ForcePlanarAxis = A2->bForcePlanar ? EBezierPlanarAxis::XY : EBezierPlanarAxis::None;
		Out.SamplingMode = A2->UI_GetSamplingMode();
		Out.SampleCount = A2->UI_GetSampleCount();
		Out.bShowSamplePoints = A2->UI_GetShowSamplePoints();
		Out.bShowDeCasteljauLevels = A2->UI_GetShowDeCasteljauLevels();
		Out.ProofT = A2->UI_GetProofT();
		Out.SelectedControlPointIndex = A2->UI_GetSelectedControlPointIndex();
		Out.bSelectAllControlPoints = A2->UI_AreAllControlPointsSelected();
		return true;
	}

	if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Actor))
	{
		Out.Actor = A3;
		Out.ActorClass = A3->GetClass();
		Out.Owner = A3->GetOwner();
		Out.Transform = A3->GetActorTransform();
		Out.bIs3D = true;
		Out.Control3D = A3->Control;
		Out.Scale = A3->Scale;
		Out.ControlPointVisualScale = A3->ControlPointVisualScale;
		Out.bEditMode = A3->UI_GetEditMode();
		Out.bClosedLoop = A3->UI_IsClosedLoop();
		Out.bEnableRuntimeEditing = A3->bEnableRuntimeEditing;
		Out.bHideVisualsWhenNotEditing = A3->bHideVisualsWhenNotEditing;
		Out.bActorVisibleInGame = A3->bActorVisibleInGame;
		Out.bShowControlPoints = A3->bShowControlPoints;
		Out.bShowStrip = A3->bShowStripMesh;
		Out.bUseCubeStrip = A3->bUseCubeStrip;
		Out.bSnapToGrid = A3->bSnapToGrid;
		Out.bShowGrid = A3->bShowGrid;
		Out.GridSizeCm = A3->GridSizeCm;
		Out.GridExtentCm = A3->GridExtentCm;
		Out.GridOriginWorld = A3->GridOriginWorld;
		Out.GridColor = A3->GridColor;
		Out.GridBaseAlpha = A3->GridBaseAlpha;
		Out.bShowGridXY = A3->bShowGridXY;
		Out.bShowGridXZ = A3->bShowGridXZ;
		Out.bShowGridYZ = A3->bShowGridYZ;
		Out.bLockToLocalXY = A3->bLockToLocalXY;
		Out.bForcePlanar = A3->bForcePlanar;
		Out.ForcePlanarAxis = A3->ForcePlanarAxis;
		Out.SamplingMode = A3->UI_GetSamplingMode();
		Out.SampleCount = A3->UI_GetSampleCount();
		Out.bShowSamplePoints = A3->UI_GetShowSamplePoints();
		Out.bShowDeCasteljauLevels = A3->UI_GetShowDeCasteljauLevels();
		Out.ProofT = A3->UI_GetProofT();
		Out.SelectedControlPointIndex = A3->UI_GetSelectedControlPointIndex();
		Out.bSelectAllControlPoints = A3->UI_AreAllControlPointsSelected();
		return true;
	}

	return false;
}

FBezierHistorySnapshot UBezierEditSubsystem::CaptureHistorySnapshot() const
{
	FBezierHistorySnapshot Snapshot;
	TArray<AActor*> Actors;
	for (const TWeakObjectPtr<AActor>& P : Editables)
	{
		if (AActor* A = P.Get())
		{
			Actors.Add(A);
		}
	}

	if (Actors.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It) Actors.Add(*It);
			for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It) Actors.Add(*It);
		}
	}

	Actors.Sort([](const AActor& L, const AActor& R){ return L.GetName() < R.GetName(); });

	for (AActor* Actor : Actors)
	{
		FBezierCurveActorSnapshot Curve;
		if (CaptureCurveSnapshot(Actor, Curve))
		{
			if (Actor == FocusedActor.Get())
			{
				Snapshot.FocusedIndex = Snapshot.Curves.Num();
			}
			Snapshot.Curves.Add(MoveTemp(Curve));
		}
	}

	return Snapshot;
}

bool UBezierEditSubsystem::AreHistorySnapshotsEquivalent(const FBezierHistorySnapshot& A, const FBezierHistorySnapshot& B) const
{
	if (A.FocusedIndex != B.FocusedIndex || A.Curves.Num() != B.Curves.Num())
	{
		return false;
	}

	for (int32 i = 0; i < A.Curves.Num(); ++i)
	{
		const FBezierCurveActorSnapshot& L = A.Curves[i];
		const FBezierCurveActorSnapshot& R = B.Curves[i];
		if (L.Actor != R.Actor || L.ActorClass != R.ActorClass || L.Owner != R.Owner || !NearlyEqualTransform(L.Transform, R.Transform)
			|| L.bIs2D != R.bIs2D || L.bIs3D != R.bIs3D
			|| !SnapshotArraysEqual(L.Control2D, R.Control2D) || !SnapshotArraysEqual(L.Control3D, R.Control3D)
			|| !FMath::IsNearlyEqual(L.Scale, R.Scale) || !FMath::IsNearlyEqual(L.ControlPointVisualScale, R.ControlPointVisualScale)
			|| L.bEditMode != R.bEditMode
			|| L.bClosedLoop != R.bClosedLoop || L.bEnableRuntimeEditing != R.bEnableRuntimeEditing
			|| L.bHideVisualsWhenNotEditing != R.bHideVisualsWhenNotEditing || L.bActorVisibleInGame != R.bActorVisibleInGame
			|| L.bShowControlPoints != R.bShowControlPoints || L.bShowStrip != R.bShowStrip || L.bUseCubeStrip != R.bUseCubeStrip
			|| L.bSnapToGrid != R.bSnapToGrid || L.bShowGrid != R.bShowGrid
			|| !FMath::IsNearlyEqual(L.GridSizeCm, R.GridSizeCm) || !FMath::IsNearlyEqual(L.GridExtentCm, R.GridExtentCm)
			|| !L.GridOriginWorld.Equals(R.GridOriginWorld, KINDA_SMALL_NUMBER) || !L.GridColor.Equals(R.GridColor)
			|| !FMath::IsNearlyEqual(L.GridBaseAlpha, R.GridBaseAlpha)
			|| L.bShowGridXY != R.bShowGridXY || L.bShowGridXZ != R.bShowGridXZ || L.bShowGridYZ != R.bShowGridYZ
			|| L.bLockToLocalXY != R.bLockToLocalXY || L.bForcePlanar != R.bForcePlanar || L.ForcePlanarAxis != R.ForcePlanarAxis
			|| L.SamplingMode != R.SamplingMode || L.SampleCount != R.SampleCount
			|| L.bShowSamplePoints != R.bShowSamplePoints || L.bShowDeCasteljauLevels != R.bShowDeCasteljauLevels
			|| !FMath::IsNearlyEqual(L.ProofT, R.ProofT)
			|| L.SelectedControlPointIndex != R.SelectedControlPointIndex || L.bSelectAllControlPoints != R.bSelectAllControlPoints)
		{
			return false;
		}
	}

	return true;
}

void UBezierEditSubsystem::PushUndoSnapshotIfDifferent(const FBezierHistorySnapshot& Snapshot)
{
	if (UndoHistory.Num() == 0 || !AreHistorySnapshotsEquivalent(UndoHistory.Last(), Snapshot))
	{
		UndoHistory.Add(Snapshot);
	}
}

void UBezierEditSubsystem::TrimHistoryStacks()
{
	const int32 MaxStepsClamped = FMath::Max(1, MaxUndoSteps) * 2 + 1;
	if (UndoHistory.Num() > MaxStepsClamped)
	{
		UndoHistory.RemoveAt(0, UndoHistory.Num() - MaxStepsClamped, EAllowShrinking::No);
	}
	if (RedoHistory.Num() > MaxStepsClamped)
	{
		RedoHistory.RemoveAt(0, RedoHistory.Num() - MaxStepsClamped, EAllowShrinking::No);
	}
}

bool UBezierEditSubsystem::RestoreHistorySnapshot(const FBezierHistorySnapshot& Snapshot)
{
	CompactRegistry();
	TArray<AActor*> CurrentActors;
	for (const TWeakObjectPtr<AActor>& P : Editables)
	{
		if (AActor* A = P.Get())
		{
			CurrentActors.AddUnique(A);
		}
	}
	if (CurrentActors.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It) CurrentActors.AddUnique(*It);
			for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It) CurrentActors.AddUnique(*It);
		}
	}

	TArray<AActor*> RestoredActors;
	for (const FBezierCurveActorSnapshot& Curve : Snapshot.Curves)
	{
		AActor* TargetActor = Curve.Actor.Get();
		bool bSpawnedFromSnapshot = false;
		if (!IsValid(TargetActor))
		{
			UWorld* World = GetWorld();
			if (!World || !Curve.ActorClass)
			{
				continue;
			}

			FActorSpawnParameters Params;
			Params.Owner = Curve.Owner.Get();
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			TargetActor = World->SpawnActor<AActor>(Curve.ActorClass, Curve.Transform, Params);
			if (!TargetActor)
			{
				continue;
			}

			bSpawnedFromSnapshot = true;
			ApplyDebugSettingsToCurve(World, TargetActor);
		}

		TargetActor->SetActorTransform(Curve.Transform, false, nullptr, ETeleportType::TeleportPhysics);

		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(TargetActor))
		{
			A2->Scale = Curve.Scale;
			A2->ControlPointVisualScale = Curve.ControlPointVisualScale;
			A2->Control = Curve.Control2D;
			A2->bEnableRuntimeEditing = Curve.bEnableRuntimeEditing;
			A2->bHideVisualsWhenNotEditing = Curve.bHideVisualsWhenNotEditing;
			A2->UI_SetEditMode(Curve.bEditMode);
			A2->UI_SetActorVisibleInGame(Curve.bActorVisibleInGame);
			A2->UI_SetShowControlPoints(Curve.bShowControlPoints);
			A2->UI_SetShowStrip(Curve.bShowStrip);
			A2->UI_SetShowCubeStrip(Curve.bUseCubeStrip);
			A2->UI_SetSnapToGrid(Curve.bSnapToGrid);
			A2->UI_SetShowGrid(Curve.bShowGrid);
			A2->UI_SetGridSizeCm(Curve.GridSizeCm);
			A2->UI_SetGridExtentCm(Curve.GridExtentCm);
			A2->UI_SetGridOriginWorld(Curve.GridOriginWorld);
			A2->UI_SetGridColor(Curve.GridColor);
			A2->UI_SetGridBaseAlpha(Curve.GridBaseAlpha);
			A2->UI_SetShowGridXY(Curve.bShowGridXY);
			A2->UI_SetLockToLocalXY(Curve.bLockToLocalXY);
			A2->UI_SetForcePlanar(Curve.bForcePlanar);
			A2->OverwriteSplineFromControl();
			A2->UI_SetClosedLoop(Curve.bClosedLoop);
			A2->UI_SetSamplingMode(Curve.SamplingMode);
			A2->UI_SetSampleCount(Curve.SampleCount);
			A2->UI_SetShowSamplePoints(Curve.bShowSamplePoints);
			A2->UI_SetShowDeCasteljauLevels(Curve.bShowDeCasteljauLevels);
			A2->UI_SetProofT(Curve.ProofT);
			if (Curve.bSelectAllControlPoints) A2->UI_SelectAllControlPoints();
			else if (Curve.SelectedControlPointIndex != INDEX_NONE) A2->UI_SelectControlPoint(Curve.SelectedControlPointIndex);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(TargetActor))
		{
			A3->Scale = Curve.Scale;
			A3->ControlPointVisualScale = Curve.ControlPointVisualScale;
			A3->Control = Curve.Control3D;
			A3->bEnableRuntimeEditing = Curve.bEnableRuntimeEditing;
			A3->bHideVisualsWhenNotEditing = Curve.bHideVisualsWhenNotEditing;
			A3->UI_SetEditMode(Curve.bEditMode);
			A3->UI_SetActorVisibleInGame(Curve.bActorVisibleInGame);
			A3->UI_SetShowControlPoints(Curve.bShowControlPoints);
			A3->UI_SetShowStrip(Curve.bShowStrip);
			A3->UI_SetShowCubeStrip(Curve.bUseCubeStrip);
			A3->UI_SetSnapToGrid(Curve.bSnapToGrid);
			A3->UI_SetShowGrid(Curve.bShowGrid);
			A3->UI_SetGridSizeCm(Curve.GridSizeCm);
			A3->UI_SetGridExtentCm(Curve.GridExtentCm);
			A3->UI_SetGridOriginWorld(Curve.GridOriginWorld);
			A3->UI_SetGridColor(Curve.GridColor);
			A3->UI_SetGridBaseAlpha(Curve.GridBaseAlpha);
			A3->UI_SetShowGridXY(Curve.bShowGridXY);
			A3->UI_SetShowGridXZ(Curve.bShowGridXZ);
			A3->UI_SetShowGridYZ(Curve.bShowGridYZ);
			A3->UI_SetLockToLocalXY(Curve.bLockToLocalXY);
			A3->UI_SetForcePlanar(Curve.bForcePlanar);
			A3->UI_SetForcePlanarAxis(Curve.ForcePlanarAxis);
			A3->UI_OverwriteSplineFromControl();
			A3->UI_SetClosedLoop(Curve.bClosedLoop);
			A3->UI_SetSamplingMode(Curve.SamplingMode);
			A3->UI_SetSampleCount(Curve.SampleCount);
			A3->UI_SetShowSamplePoints(Curve.bShowSamplePoints);
			A3->UI_SetShowDeCasteljauLevels(Curve.bShowDeCasteljauLevels);
			A3->UI_SetProofT(Curve.ProofT);
			if (Curve.bSelectAllControlPoints) A3->UI_SelectAllControlPoints();
			else if (Curve.SelectedControlPointIndex != INDEX_NONE) A3->UI_SelectControlPoint(Curve.SelectedControlPointIndex);
		}

		RegisterEditable(TargetActor);
		if (bSpawnedFromSnapshot)
		{
			if (ABezierCurveSetActor* CurveSet = Cast<ABezierCurveSetActor>(Curve.Owner.Get()))
			{
				CurveSet->UI_RegisterSpawned(TargetActor);
			}
		}
		RestoredActors.Add(TargetActor);
	}

	for (AActor* Actor : CurrentActors)
	{
		if (IsValid(Actor) && !RestoredActors.Contains(Actor) && IsEditable(Actor))
		{
			Actor->Destroy();
		}
	}

	Editables.RemoveAll([&RestoredActors](const TWeakObjectPtr<AActor>& P)
	{
		const AActor* Actor = P.Get();
		return !IsValid(Actor) || !RestoredActors.Contains(Actor);
	});

	if (Snapshot.FocusedIndex != INDEX_NONE && RestoredActors.IsValidIndex(Snapshot.FocusedIndex))
	{
		SetFocused(RestoredActors[Snapshot.FocusedIndex]);
	}
	else
	{
		FocusedActor = nullptr;
		OnFocusChanged.Broadcast(nullptr);
	}

	return true;
}

bool UBezierEditSubsystem::History_CommitInteractiveChange(const FBezierHistorySnapshot& BeforeSnapshot)
{
	const FBezierHistorySnapshot AfterSnapshot = CaptureHistorySnapshot();
	if (AreHistorySnapshotsEquivalent(BeforeSnapshot, AfterSnapshot))
	{
		return false;
	}

	PushUndoSnapshotIfDifferent(BeforeSnapshot);
	PushUndoSnapshotIfDifferent(AfterSnapshot);
	RedoHistory.Reset();
	TrimHistoryStacks();
	return true;
}

void UBezierEditSubsystem::History_Clear()
{
	UndoHistory.Reset();
	RedoHistory.Reset();
}

bool UBezierEditSubsystem::History_Undo()
{
	if (UndoHistory.Num() < 2)
	{
		return false;
	}

	RedoHistory.Add(UndoHistory.Last());
	UndoHistory.RemoveAt(UndoHistory.Num() - 1, 1, EAllowShrinking::No);
	TrimHistoryStacks();
	return RestoreHistorySnapshot(UndoHistory.Last());
}

bool UBezierEditSubsystem::History_Redo()
{
	if (RedoHistory.Num() == 0)
	{
		return false;
	}

	const FBezierHistorySnapshot Target = RedoHistory.Last();
	RedoHistory.RemoveAt(RedoHistory.Num() - 1, 1, EAllowShrinking::No);
	PushUndoSnapshotIfDifferent(Target);
	TrimHistoryStacks();
	return RestoreHistorySnapshot(Target);
}

bool UBezierEditSubsystem::IsEditable(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Prefer interface check, but keep native curve actors editable even if interface metadata
	// is not available in a given automation/build context.
	if (Actor->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
	{
		return true;
	}

	return Cast<ABezierCurve2DActor>(Actor) != nullptr || Cast<ABezierCurve3DActor>(Actor) != nullptr;
}

void UBezierEditSubsystem::CompactRegistry()
{
	Editables.RemoveAll([](const TWeakObjectPtr<AActor>& P){ return !P.IsValid(); });
}

void UBezierEditSubsystem::RegisterEditable(AActor* Actor)
{
	if (!IsEditable(Actor)) return;

	CompactRegistry();
	for (const auto& P : Editables)
	{
		if (P.Get() == Actor) return;
	}
	Editables.Add(Actor);
}

void UBezierEditSubsystem::UnregisterEditable(AActor* Actor)
{
	if (!Actor) return;

	Editables.RemoveAll([Actor](const TWeakObjectPtr<AActor>& P)
	{
		return !P.IsValid() || P.Get() == Actor;
	});

	if (FocusedActor.Get() == Actor)
	{
		FocusedActor = nullptr;
		OnFocusChanged.Broadcast(nullptr);
	}
}

void UBezierEditSubsystem::SetFocused(AActor* Actor)
{
	if (Actor && !IsEditable(Actor)) return;

	if (FocusedActor.Get() == Actor) return;

	FocusedActor = Actor;
	OnFocusChanged.Broadcast(Actor);
	if (bIsolateFocusedCurve)
	{
		ApplyIsolateVisibility();
	}
}

AActor* UBezierEditSubsystem::GetFocused() const
{
	return FocusedActor.Get();
}

bool UBezierEditSubsystem::HasFocused() const
{
	return FocusedActor.IsValid();
}

void UBezierEditSubsystem::ForFocused(TFunctionRef<void(UObject* Obj)> Fn)
{
	AActor* A = FocusedActor.Get();
	if (!IsEditable(A) && bAutoFocusFirstEditable)
	{
		CompactRegistry();
		for (const auto& P : Editables)
		{
			A = P.Get();
			if (IsEditable(A))
			{
				FocusedActor = A;
				OnFocusChanged.Broadcast(A);
				break;
			}
		}

		// Recovery path for contexts where registry was not yet populated (automation/world init timing).
		if (!IsEditable(A))
		{
			if (UWorld* World = GetWorld())
			{
				for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It)
				{
					if (IsEditable(*It))
					{
						A = *It;
						FocusedActor = A;
						OnFocusChanged.Broadcast(A);
						break;
					}
				}

				if (!IsEditable(A))
				{
					for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It)
					{
						if (IsEditable(*It))
						{
							A = *It;
							FocusedActor = A;
							OnFocusChanged.Broadcast(A);
							break;
						}
					}
				}
			}
		}
	}

	if (!IsEditable(A)) return;
	Fn(A);
}

void UBezierEditSubsystem::ForAll(TFunctionRef<void(UObject* Obj)> Fn)
{
	if (bApplyAllToFocusedOnly && HasFocused())
	{
		ForFocused(Fn);
		return;
	}

	CompactRegistry();
	bool bAppliedAny = false;
	for (const auto& P : Editables)
	{
		AActor* A = P.Get();
		if (IsEditable(A))
		{
			Fn(A);
			bAppliedAny = true;
		}
	}

	// Recovery path for contexts where registry was not yet populated.
	if (!bAppliedAny)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It)
			{
				if (IsEditable(*It))
				{
					Fn(*It);
					bAppliedAny = true;
				}
			}

			for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It)
			{
				if (IsEditable(*It))
				{
					Fn(*It);
					bAppliedAny = true;
				}
			}
		}
	}
}

// Focus-only
void UBezierEditSubsystem::Focus_SetEditMode(bool bInEditMode)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetEditMode(Obj, bInEditMode); });
}

void UBezierEditSubsystem::Focus_ToggleEditMode()
{
	ForFocused([&](UObject* Obj)
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetEditMode(Obj);
		IBezierEditable::Execute_BEZ_SetEditMode(Obj, !bCur);
	});
}

void UBezierEditSubsystem::Focus_SetActorVisibleInGame(bool bInVisible)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetActorVisibleInGame(Obj, bInVisible); });
}

void UBezierEditSubsystem::Focus_ToggleActorVisibleInGame()
{
	ForFocused([&](UObject* Obj)
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetActorVisibleInGame(Obj);
		IBezierEditable::Execute_BEZ_SetActorVisibleInGame(Obj, !bCur);
	});
}

void UBezierEditSubsystem::Focus_SetShowControlPoints(bool bInShow)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetShowControlPoints(Obj, bInShow); });
}

void UBezierEditSubsystem::Focus_ToggleShowControlPoints()
{
	ForFocused([&](UObject* Obj)
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetShowControlPoints(Obj);
		IBezierEditable::Execute_BEZ_SetShowControlPoints(Obj, !bCur);
	});
}

void UBezierEditSubsystem::Focus_SetShowStrip(bool bInShow)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetShowStrip(Obj, bInShow); });
}

void UBezierEditSubsystem::Focus_ToggleShowStrip()
{
	ForFocused([&](UObject* Obj)
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetShowStrip(Obj);
		IBezierEditable::Execute_BEZ_SetShowStrip(Obj, !bCur);
	});
}

void UBezierEditSubsystem::Focus_SetControlPointSize(float InVisualScale)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetControlPointSize(Obj, InVisualScale); });
}

void UBezierEditSubsystem::Focus_SetStripSize(float InWidth, float InThickness)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetStripSize(Obj, InWidth, InThickness); });
}

void UBezierEditSubsystem::Focus_SetControlPointColors(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetControlPointColors(Obj, Normal, Hover, Selected); });
}

void UBezierEditSubsystem::Focus_SetSnapToGrid(bool bInSnap)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetSnapToGrid(Obj, bInSnap); });
}

void UBezierEditSubsystem::Focus_ToggleSnapToGrid()
{
	ForFocused([&](UObject* Obj)
	{
		const bool bCur = IBezierEditable::Execute_BEZ_GetSnapToGrid(Obj);
		IBezierEditable::Execute_BEZ_SetSnapToGrid(Obj, !bCur);
	});
}

void UBezierEditSubsystem::Focus_SetGridSize(float InGridSizeCm)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetGridSize(Obj, InGridSizeCm); });
}

void UBezierEditSubsystem::Focus_SetForcePlanar(bool bInForce)
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetForcePlanar(Obj, bInForce); });
}

EBezierPlanarAxis UBezierEditSubsystem::Focus_CycleForcePlanarAxis()
{
	EBezierPlanarAxis Result = EBezierPlanarAxis::None;
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			Result = A3->UI_CycleForcePlanarAxis();
			return;
		}

		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			const bool bEnable = !A2->bForcePlanar;
			A2->UI_SetForcePlanar(bEnable);
			Result = bEnable ? EBezierPlanarAxis::XY : EBezierPlanarAxis::None;
		}
	});
	return Result;
}

void UBezierEditSubsystem::Focus_ResetCurveState()
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_ResetCurveState(Obj); });
}

bool UBezierEditSubsystem::Focus_AddControlPointAfterSelected()
{
	const FBezierHistorySnapshot Before = CaptureHistorySnapshot();
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = AddControlPointAfterSelectedForEditable(Obj); });
	if (bResult)
	{
		History_CommitInteractiveChange(Before);
	}
	return bResult;
}

bool UBezierEditSubsystem::Focus_DeleteSelectedControlPoint()
{
	const FBezierHistorySnapshot Before = CaptureHistorySnapshot();
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = DeleteSelectedControlPointForEditable(Obj); });
	if (bResult)
	{
		CompactRegistry();
		History_CommitInteractiveChange(Before);
	}
	return bResult;
}

bool UBezierEditSubsystem::Focus_DuplicateSelectedControlPoint()
{
	const FBezierHistorySnapshot Before = CaptureHistorySnapshot();
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = DuplicateSelectedControlPointForEditable(Obj); });
	if (bResult)
	{
		History_CommitInteractiveChange(Before);
	}
	return bResult;
}

void UBezierEditSubsystem::Focus_CenterCurve()
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_CenterCurve();
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_CenterCurve();
		}
	});
}

void UBezierEditSubsystem::Focus_MirrorCurve()
{
	UWorld* World = GetWorld();
	const float NowSeconds = World ? World->GetTimeSeconds() : 0.0f;
	if (LastMirrorAxisCycleTimeSeconds >= 0.0f && World)
	{
		const float Elapsed = NowSeconds - LastMirrorAxisCycleTimeSeconds;
		if (Elapsed >= MirrorAxisCycleResetDelaySeconds)
		{
			Focus_ResetMirrorAxisCycle();
		}
	}

	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			const int32 AxisIndex = MirrorAxisCycleIndex % 2;
			if (AxisIndex == 0)
			{
				A2->UI_MirrorCurveX();
			}
			else
			{
				A2->UI_MirrorCurveY();
			}
			MirrorAxisCycleIndex = (MirrorAxisCycleIndex + 1) % 2;
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			const int32 AxisIndex = MirrorAxisCycleIndex % 3;
			if (AxisIndex == 0)
			{
				A3->UI_MirrorCurveX();
			}
			else if (AxisIndex == 1)
			{
				A3->UI_MirrorCurveY();
			}
			else
			{
				A3->UI_MirrorCurveZ();
			}
			MirrorAxisCycleIndex = (MirrorAxisCycleIndex + 1) % 3;
		}
	});

	if (World)
	{
		LastMirrorAxisCycleTimeSeconds = NowSeconds;
		World->GetTimerManager().ClearTimer(MirrorAxisCycleResetHandle);
		World->GetTimerManager().SetTimer(MirrorAxisCycleResetHandle, this, &UBezierEditSubsystem::Focus_ResetMirrorAxisCycle, MirrorAxisCycleResetDelaySeconds, false);
	}
}

void UBezierEditSubsystem::Focus_ReverseControlOrder()
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_ReverseControlOrder();
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_ReverseControlOrder();
		}
	});
}

void UBezierEditSubsystem::Focus_ToggleClosedLoop()
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_ToggleClosedLoop();
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_ToggleClosedLoop();
		}
	});
}

void UBezierEditSubsystem::Focus_SetClosedLoop(bool bInClosed)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetClosedLoop(bInClosed);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetClosedLoop(bInClosed);
		}
	});
}

bool UBezierEditSubsystem::Focus_IsClosedLoop()
{
	bool bClosed = false;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			bClosed = A2->UI_IsClosedLoop();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			bClosed = A3->UI_IsClosedLoop();
		}
	});
	return bClosed;
}

void UBezierEditSubsystem::Focus_DuplicateCurve()
{
	if (!Focus_EnsureFocused())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FBezierHistorySnapshot Before = CaptureHistorySnapshot();
	bool bDuplicated = false;

	ForFocused([&](UObject* Obj)
	{
		AActor* Actor = Cast<AActor>(Obj);
		if (!Actor) return;

		AActor* Owner = Actor->GetOwner();
		FActorSpawnParameters Params;
		Params.Owner = Owner;
		Params.OverrideLevel = Actor->GetLevel();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* NewActor = World->SpawnActor<AActor>(Actor->GetClass(), Actor->GetActorTransform(), Params);
		if (NewActor)
		{
			if (const ABezierCurve2DActor* Source2D = Cast<ABezierCurve2DActor>(Actor))
			{
				if (ABezierCurve2DActor* New2D = Cast<ABezierCurve2DActor>(NewActor))
				{
					New2D->Scale = Source2D->Scale;
					New2D->Control = Source2D->Control;
					New2D->ControlPointVisualScale = Source2D->ControlPointVisualScale;
					New2D->UI_SetClosedLoop(Source2D->UI_IsClosedLoop());
					New2D->OverwriteSplineFromControl();
					New2D->bEnableRuntimeEditing = Source2D->bEnableRuntimeEditing;
					New2D->bHideVisualsWhenNotEditing = Source2D->bHideVisualsWhenNotEditing;
					New2D->UI_SetActorVisibleInGame(Source2D->bActorVisibleInGame);
					New2D->UI_SetShowControlPoints(Source2D->bShowControlPoints);
					New2D->UI_SetShowStrip(Source2D->bShowStripMesh);
					New2D->UI_SetShowCubeStrip(Source2D->bUseCubeStrip);
					New2D->UI_SetStripSize(Source2D->StripWidth, Source2D->StripThickness);
					New2D->StripSegments = Source2D->StripSegments;
					New2D->ControlPointMaterial = Source2D->ControlPointMaterial;
					New2D->StripMaterial = Source2D->StripMaterial;
					New2D->ControlPointColor = Source2D->ControlPointColor;
					New2D->ControlPointHoverColor = Source2D->ControlPointHoverColor;
					New2D->ControlPointSelectedColor = Source2D->ControlPointSelectedColor;
					New2D->UI_SetControlPointColors(New2D->ControlPointColor, New2D->ControlPointHoverColor, New2D->ControlPointSelectedColor);
					New2D->UI_SetEditMode(Source2D->UI_GetEditMode());
					New2D->UI_SetInitialControlFromCurrent();
				}
			}
			else if (const ABezierCurve3DActor* Source3D = Cast<ABezierCurve3DActor>(Actor))
			{
				if (ABezierCurve3DActor* New3D = Cast<ABezierCurve3DActor>(NewActor))
				{
					New3D->Scale = Source3D->Scale;
					New3D->Control = Source3D->Control;
					New3D->ControlPointVisualScale = Source3D->ControlPointVisualScale;
					New3D->UI_SetClosedLoop(Source3D->UI_IsClosedLoop());
					New3D->UI_OverwriteSplineFromControl();
					New3D->bEnableRuntimeEditing = Source3D->bEnableRuntimeEditing;
					New3D->bHideVisualsWhenNotEditing = Source3D->bHideVisualsWhenNotEditing;
					New3D->UI_SetActorVisibleInGame(Source3D->bActorVisibleInGame);
					New3D->UI_SetShowControlPoints(Source3D->bShowControlPoints);
					New3D->UI_SetShowStrip(Source3D->bShowStripMesh);
					New3D->UI_SetShowCubeStrip(Source3D->bUseCubeStrip);
					New3D->UI_SetStripSize(Source3D->StripWidth, Source3D->StripThickness);
					New3D->StripSegments = Source3D->StripSegments;
					New3D->ControlPointMaterial = Source3D->ControlPointMaterial;
					New3D->StripMaterial = Source3D->StripMaterial;
					New3D->ControlPointColor = Source3D->ControlPointColor;
					New3D->ControlPointHoverColor = Source3D->ControlPointHoverColor;
					New3D->ControlPointSelectedColor = Source3D->ControlPointSelectedColor;
					New3D->UI_SetControlPointColors(New3D->ControlPointColor, New3D->ControlPointHoverColor, New3D->ControlPointSelectedColor);
					New3D->UI_SetEditMode(Source3D->UI_GetEditMode());
					New3D->UI_SetInitialControlFromCurrent();
				}
			}

			if (ABezierCurveSetActor* CurveSet = Cast<ABezierCurveSetActor>(Owner))
			{
				CurveSet->UI_RegisterSpawned(NewActor);
			}

			if (UWorld* LocalWorld = NewActor->GetWorld())
			{
				for (TActorIterator<ABezierDebugActor> DebugIt(LocalWorld); DebugIt; ++DebugIt)
				{
					DebugIt->ApplyDebugToCurve(NewActor);
					break;
				}
			}

			constexpr float DuplicateOffsetCm = 5.0f; // 0.05m
			const FVector Offset = Actor->GetActorRightVector().GetSafeNormal() * DuplicateOffsetCm;
			NewActor->AddActorWorldOffset(Offset, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (IsEditable(NewActor))
		{
			RegisterEditable(NewActor);
			SetFocused(NewActor);
			bDuplicated = true;
		}
	});

	if (bDuplicated)
	{
		History_CommitInteractiveChange(Before);
	}
}

void UBezierEditSubsystem::Focus_IsolateCurve(bool bInIsolate)
{
	if (bInIsolate && bIsolateFocusedCurve)
	{
		bIsolateFocusedCurve = false;
	}
	else
	{
		bIsolateFocusedCurve = bInIsolate;
	}
	ApplyIsolateVisibility();
}

void UBezierEditSubsystem::Focus_ToggleIsolateCurve()
{
	bIsolateFocusedCurve = !bIsolateFocusedCurve;
	ApplyIsolateVisibility();
}

void UBezierEditSubsystem::ApplyIsolateVisibility()
{
	AActor* Focused = FocusedActor.Get();
	const bool bHasFocused = IsEditable(Focused);
	CompactRegistry();
	for (const auto& P : Editables)
	{
		UObject* Obj = P.Get();
		if (!IsEditable(Cast<AActor>(Obj))) continue;

		const bool bIsFocused = bHasFocused && Obj == Focused;
		const bool bVisible = bIsolateFocusedCurve ? (bHasFocused ? bIsFocused : true) : true;
		IBezierEditable::Execute_BEZ_SetActorVisibleInGame(Obj, bVisible);
	}
}

void UBezierEditSubsystem::Focus_SetSamplingMode(EBezierSamplingMode InMode)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetSamplingMode(InMode);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetSamplingMode(InMode);
		}
	});
}

EBezierSamplingMode UBezierEditSubsystem::Focus_GetSamplingMode()
{
	EBezierSamplingMode Mode = EBezierSamplingMode::Parametric;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			Mode = A2->UI_GetSamplingMode();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			Mode = A3->UI_GetSamplingMode();
		}
	});
	return Mode;
}

void UBezierEditSubsystem::Focus_SetSampleCount(int32 InCount)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetSampleCount(InCount);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetSampleCount(InCount);
		}
	});
}

int32 UBezierEditSubsystem::Focus_GetSampleCount()
{
	int32 Count = 0;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			Count = A2->UI_GetSampleCount();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			Count = A3->UI_GetSampleCount();
		}
	});
	return Count;
}

void UBezierEditSubsystem::Focus_SetProofT(double InT)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetProofT(InT);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetProofT(InT);
		}
	});
}

double UBezierEditSubsystem::Focus_GetProofT()
{
	double Value = 0.0;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			Value = A2->UI_GetProofT();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			Value = A3->UI_GetProofT();
		}
	});
	return Value;
}

void UBezierEditSubsystem::Focus_SetShowSamplePoints(bool bInShow)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetShowSamplePoints(bInShow);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetShowSamplePoints(bInShow);
		}
	});
}

bool UBezierEditSubsystem::Focus_GetShowSamplePoints()
{
	bool bShow = false;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			bShow = A2->UI_GetShowSamplePoints();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			bShow = A3->UI_GetShowSamplePoints();
		}
	});
	return bShow;
}

void UBezierEditSubsystem::Focus_SetShowDeCasteljauLevels(bool bInShow)
{
	ForFocused([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetShowDeCasteljauLevels(bInShow);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetShowDeCasteljauLevels(bInShow);
		}
	});
}

bool UBezierEditSubsystem::Focus_GetShowDeCasteljauLevels()
{
	bool bShow = false;
	ForFocused([&](UObject* Obj)
	{
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			bShow = A2->UI_GetShowDeCasteljauLevels();
		}
		else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			bShow = A3->UI_GetShowDeCasteljauLevels();
		}
	});
	return bShow;
}

bool UBezierEditSubsystem::Focus_EnsureFocused()
{
	if (HasFocused())
	{
		return true;
	}

	if (!bAutoFocusFirstEditable)
	{
		return false;
	}

	CompactRegistry();
	for (const auto& P : Editables)
	{
		AActor* A = P.Get();
		if (IsEditable(A))
		{
			FocusedActor = A;
			OnFocusChanged.Broadcast(A);
			return true;
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It)
		{
			if (IsEditable(*It))
			{
				FocusedActor = *It;
				OnFocusChanged.Broadcast(*It);
				return true;
			}
		}

		for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It)
		{
			if (IsEditable(*It))
			{
				FocusedActor = *It;
				OnFocusChanged.Broadcast(*It);
				return true;
			}
		}
	}

	return false;
}

void UBezierEditSubsystem::Focus_ResetMirrorAxisCycle()
{
	MirrorAxisCycleIndex = 0;
	LastMirrorAxisCycleTimeSeconds = -1.0f;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MirrorAxisCycleResetHandle);
	}
	OnMirrorAxisCycleReset.Broadcast();
}

float UBezierEditSubsystem::Focus_GetMirrorAxisCycleSecondsRemaining() const
{
	if (LastMirrorAxisCycleTimeSeconds < 0.0f)
	{
		return 0.0f;
	}

	if (const UWorld* World = GetWorld())
	{
		const float Elapsed = World->GetTimeSeconds() - LastMirrorAxisCycleTimeSeconds;
		return FMath::Max(0.0f, MirrorAxisCycleResetDelaySeconds - Elapsed);
	}

	return 0.0f;
}

static void ApplyEditInteraction(UObject* Obj, bool bEnabled, bool bShowControlPoints, bool bShowStrip)
{
	if (!Obj)
	{
		return;
	}

	IBezierEditable::Execute_BEZ_SetEditMode(Obj, bEnabled);
	IBezierEditable::Execute_BEZ_SetShowControlPoints(Obj, bEnabled && bShowControlPoints);
	IBezierEditable::Execute_BEZ_SetShowStrip(Obj, bEnabled && bShowStrip);
}

void UBezierEditSubsystem::Focus_SetEditInteractionEnabled(bool bEnabled, bool bShowControlPoints, bool bShowStrip)
{
	ForFocused([&](UObject* Obj){ ApplyEditInteraction(Obj, bEnabled, bShowControlPoints, bShowStrip); });
}

// All
void UBezierEditSubsystem::All_SetEditMode(bool bInEditMode)
{
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetEditMode(Obj, bInEditMode); });
}

void UBezierEditSubsystem::All_ToggleEditMode()
{
	// Toggle based on focused if available, else default to true
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			const bool bCur = IBezierEditable::Execute_BEZ_GetEditMode(Obj);
			All_SetEditMode(!bCur);
		});
	}
	else
	{
		All_SetEditMode(true);
	}
}

void UBezierEditSubsystem::All_SetActorVisibleInGame(bool bInVisible)
{
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetActorVisibleInGame(Obj, bInVisible); });
}

void UBezierEditSubsystem::All_ToggleActorVisibleInGame()
{
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			const bool bCur = IBezierEditable::Execute_BEZ_GetActorVisibleInGame(Obj);
			All_SetActorVisibleInGame(!bCur);
		});
	}
	else
	{
		All_SetActorVisibleInGame(true);
	}
}

void UBezierEditSubsystem::All_SetShowControlPoints(bool bInShow)
{
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetShowControlPoints(Obj, bInShow); });
}

void UBezierEditSubsystem::All_ToggleShowControlPoints()
{
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			const bool bCur = IBezierEditable::Execute_BEZ_GetShowControlPoints(Obj);
			All_SetShowControlPoints(!bCur);
		});
	}
	else
	{
		All_SetShowControlPoints(true);
	}
}

void UBezierEditSubsystem::All_SetShowStrip(bool bInShow)
{
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetShowStrip(Obj, bInShow); });
}

void UBezierEditSubsystem::All_ToggleShowStrip()
{
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			const bool bCur = IBezierEditable::Execute_BEZ_GetShowStrip(Obj);
			All_SetShowStrip(!bCur);
		});
	}
	else
	{
		All_SetShowStrip(true);
	}
}

void UBezierEditSubsystem::All_SetSnapToGrid(bool bInSnap)
{
	ForAll([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetSnapToGrid(bInSnap);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetSnapToGrid(bInSnap);
		}
		else if (Obj && Obj->GetClass()->ImplementsInterface(UBezierEditable::StaticClass()))
		{
			IBezierEditable::Execute_BEZ_SetSnapToGrid(Obj, bInSnap);
		}
	});
}

void UBezierEditSubsystem::All_ToggleSnapToGrid()
{
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			bool bNewSnapState = true;
			if (ToggleSnapForEditable(Obj, bNewSnapState))
			{
				All_SetSnapToGrid(bNewSnapState);
			}
			else
			{
				All_SetSnapToGrid(true);
			}
		});
	}
	else
	{
		All_SetSnapToGrid(true);
	}
}

void UBezierEditSubsystem::All_SetGridSize(float InGridSizeCm)
{
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetGridSize(Obj, InGridSizeCm); });
}

void UBezierEditSubsystem::All_SetSampleCount(int32 InCount)
{
	ForAll([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetSampleCount(InCount);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetSampleCount(InCount);
		}
	});
}

void UBezierEditSubsystem::All_SetProofT(double InT)
{
	ForAll([&](UObject* Obj)
	{
		if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Obj))
		{
			A2->UI_SetProofT(InT);
		}
		else if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Obj))
		{
			A3->UI_SetProofT(InT);
		}
	});
}

void UBezierEditSubsystem::All_SetEditInteractionEnabled(bool bEnabled, bool bShowControlPoints, bool bShowStrip)
{
	ForAll([&](UObject* Obj){ ApplyEditInteraction(Obj, bEnabled, bShowControlPoints, bShowStrip); });
}

void UBezierEditSubsystem::All_ExportCurveSetJson()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ABezierCurveSetActor> It(World); It; ++It)
	{
		It->UI_ExportCurveSetJson();
		break;
	}
}

void UBezierEditSubsystem::All_ImportCurveSetJson()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<ABezierCurveSetActor> It(World); It; ++It)
	{
		It->UI_ImportCurveSetJson();
		break;
	}
}

void UBezierEditSubsystem::SetApplyAllToFocusedOnly(bool bInFocusedOnly)
{
	bApplyAllToFocusedOnly = bInFocusedOnly;
}

void UBezierEditSubsystem::ToggleApplyAllToFocusedOnly()
{
	bApplyAllToFocusedOnly = !bApplyAllToFocusedOnly;
}

bool UBezierEditSubsystem::GetApplyAllToFocusedOnly() const
{
	return bApplyAllToFocusedOnly;
}

void UBezierEditSubsystem::SetAutoFocusFirstEditable(bool bInAutoFocus)
{
	bAutoFocusFirstEditable = bInAutoFocus;
}

bool UBezierEditSubsystem::GetAutoFocusFirstEditable() const
{
	return bAutoFocusFirstEditable;
}
