#include "BezierDebugActor.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierCurveSetActor.h"
#include "BezierEditPlayerController.h"
#include "BezierEditSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"

ABezierDebugActor::ABezierDebugActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABezierDebugActor::BeginPlay()
{
	Super::BeginPlay();

	if (bApplyOnBeginPlay)
	{
		ApplyDebugSettings();
	}
}

void ABezierDebugActor::ApplyDebugSettings()
{
	ApplyControllerDebug();
	ApplySubsystemDebug();
	ApplyCurveActorDebug();

	if (ABezierCurveSetActor* CurveSet = FindCurveSetActor())
	{
		ApplyCurveSetDebug(CurveSet);
	}
}

void ABezierDebugActor::ExportCurveSet()
{
	if (ABezierCurveSetActor* CurveSet = FindCurveSetActor())
	{
		CurveSet->UI_ExportCurveSetJson();
	}
}

void ABezierDebugActor::ImportCurveSet()
{
	if (ABezierCurveSetActor* CurveSet = FindCurveSetActor())
	{
		CurveSet->UI_ImportCurveSetJson();
	}
}

void ABezierDebugActor::ResetAllCurves()
{
	if (ABezierCurveSetActor* CurveSet = FindCurveSetActor())
	{
		CurveSet->UI_ResetCurveStateForAll();
	}
}

ABezierCurveSetActor* ABezierDebugActor::FindCurveSetActor() const
{
	if (!GetWorld()) return nullptr;
	for (TActorIterator<ABezierCurveSetActor> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void ABezierDebugActor::ApplyControllerDebug() const
{
	if (!GetWorld()) return;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ABezierEditPlayerController* EditPC = Cast<ABezierEditPlayerController>(PC))
		{
			EditPC->bDebugTrace = bEnableMouseTraceDebug;
		}
	}
}

void ABezierDebugActor::ApplyCurveSetDebug(ABezierCurveSetActor* CurveSet) const
{
	if (!CurveSet) return;

	CurveSet->UI_SetEditModeForAll(bEnableEditMode);
	CurveSet->UI_SetShowControlPointsForAll(bShowControlPoints);
	CurveSet->UI_SetShowStripForAll(bShowStrip);
	CurveSet->UI_SetShowCubeStripForAll(bUseCubeStrip);
	CurveSet->UI_SetControlPointSizeForAll(ControlPointSize);
	CurveSet->UI_SetStripSizeForAll(StripWidth, StripThickness);
	CurveSet->UI_SetControlPointColorsForAll(ControlPointNormal, ControlPointHover, ControlPointSelected);
	CurveSet->UI_SetSnapToGridForAll(bSnapToGrid);
	CurveSet->UI_SetGridSizeForAll(GridSizeCm);
	CurveSet->UI_SetForcePlanarForAll(bForcePlanar);
	CurveSet->UI_SetEditInteractionEnabledForAll(bEnableEditMode, bShowControlPoints, bShowStrip);
}

void ABezierDebugActor::ApplyCurveActorDebug() const
{
	if (!GetWorld()) return;

	for (TActorIterator<ABezierCurve3DActor> It(GetWorld()); It; ++It)
	{
		It->bShowControlPolygon = bShowControlPolygon;
	}

	for (TActorIterator<ABezierCurve2DActor> It(GetWorld()); It; ++It)
	{
		It->bShowControlPolygon = bShowControlPolygon;
	}
}

void ABezierDebugActor::ApplySubsystemDebug() const
{
	if (!GetWorld()) return;
	if (UBezierEditSubsystem* Subsystem = GetWorld()->GetSubsystem<UBezierEditSubsystem>())
	{
		Subsystem->SetApplyAllToFocusedOnly(bApplyAllToFocusedOnly);
	}
}
