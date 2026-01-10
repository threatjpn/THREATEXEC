#include "BezierEditSubsystem.h"

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

void UBezierEditSubsystem::Focus_ResetCurveState()
{
	ForFocused([&](UObject* Obj){ IBezierEditable::Execute_BEZ_ResetCurveState(Obj); });
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
