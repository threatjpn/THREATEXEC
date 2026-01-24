#include "BezierEditSubsystem.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierCurveSetActor.h"
#include "BezierEditable.h"

#include "Engine/World.h"
#include "EngineUtils.h"

bool UBezierEditSubsystem::IsEditable(AActor* Actor) const
{
	return Actor && Actor->GetClass()->ImplementsInterface(UBezierEditable::StaticClass());
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
	for (const auto& P : Editables)
	{
		AActor* A = P.Get();
		if (IsEditable(A)) Fn(A);
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
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = IBezierEditable::Execute_BEZ_AddControlPointAfterSelected(Obj); });
	return bResult;
}

bool UBezierEditSubsystem::Focus_DeleteSelectedControlPoint()
{
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = IBezierEditable::Execute_BEZ_DeleteSelectedControlPoint(Obj); });
	return bResult;
}

bool UBezierEditSubsystem::Focus_DuplicateSelectedControlPoint()
{
	bool bResult = false;
	ForFocused([&](UObject* Obj){ bResult = IBezierEditable::Execute_BEZ_DuplicateSelectedControlPoint(Obj); });
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
	UWorld* World = GetWorld();
	if (!World) return;

	ForFocused([&](UObject* Obj)
	{
		AActor* Actor = Cast<AActor>(Obj);
		if (!Actor) return;

		AActor* Owner = Actor->GetOwner();
		FActorSpawnParameters Params;
		Params.Template = Actor;
		Params.Owner = Owner;
		Params.OverrideLevel = Actor->GetLevel();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* NewActor = World->SpawnActor<AActor>(Actor->GetClass(), Actor->GetActorTransform(), Params);
		if (NewActor)
		{
			if (ABezierCurveSetActor* CurveSet = Cast<ABezierCurveSetActor>(Owner))
			{
				CurveSet->UI_RegisterSpawned(NewActor);
			}

			FBox Bounds(EForceInit::ForceInit);
			const FTransform ActorXf = Actor->GetActorTransform();
			if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(Actor))
			{
				for (const FVector2D& P : A2->Control)
				{
					Bounds += ActorXf.TransformPosition(FVector(P.X * A2->Scale, P.Y * A2->Scale, 0.0f));
				}
			}
			else if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(Actor))
			{
				for (const FVector& P : A3->Control)
				{
					Bounds += ActorXf.TransformPosition(P * A3->Scale);
				}
			}
			else
			{
				FVector Origin = FVector::ZeroVector;
				FVector Extents = FVector::ZeroVector;
				Actor->GetActorBounds(true, Origin, Extents);
				Bounds += Origin - Extents;
				Bounds += Origin + Extents;
			}

			const float BoundsSize = Bounds.IsValid ? Bounds.GetSize().Size() : 0.0f;
			const float OffsetDistance = FMath::Max(100.0f, BoundsSize * 0.25f);
			const FVector Offset = (Actor->GetActorRightVector() + Actor->GetActorForwardVector() * 0.5f) * OffsetDistance;
			NewActor->AddActorWorldOffset(Offset, false, nullptr, ETeleportType::TeleportPhysics);
		}
		if (IsEditable(NewActor))
		{
			RegisterEditable(NewActor);
			SetFocused(NewActor);
		}
	});
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
	ForAll([&](UObject* Obj){ IBezierEditable::Execute_BEZ_SetSnapToGrid(Obj, bInSnap); });
}

void UBezierEditSubsystem::All_ToggleSnapToGrid()
{
	if (HasFocused())
	{
		ForFocused([&](UObject* Obj)
		{
			const bool bCur = IBezierEditable::Execute_BEZ_GetSnapToGrid(Obj);
			All_SetSnapToGrid(!bCur);
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
