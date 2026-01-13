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
	ApplyCurveSetDebugAll();
}

void ABezierDebugActor::ExportCurveSet()
{
	if (!GetWorld()) return;
	for (TActorIterator<ABezierCurveSetActor> It(GetWorld()); It; ++It)
	{
		It->UI_ExportCurveSetJson();
	}
}

void ABezierDebugActor::ImportCurveSet()
{
	if (!GetWorld()) return;
	for (TActorIterator<ABezierCurveSetActor> It(GetWorld()); It; ++It)
	{
		It->UI_ImportCurveSetJson();
	}
}

void ABezierDebugActor::ResetAllCurves()
{
	if (!GetWorld()) return;
	for (TActorIterator<ABezierCurveSetActor> It(GetWorld()); It; ++It)
	{
		It->UI_ResetCurveStateForAll();
	}
}

void ABezierDebugActor::ApplyControllerDebug() const
{
	if (!GetWorld()) return;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ABezierEditPlayerController* EditPC = Cast<ABezierEditPlayerController>(PC))
		{
			EditPC->SetDebugTrace(bEnableMouseTraceDebug);
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
	if (bOverrideVisualSizes)
	{
		CurveSet->UI_SetControlPointSizeForAll(ControlPointSize);
		CurveSet->UI_SetStripSizeForAll(StripWidth, StripThickness);
	}
	CurveSet->UI_SetControlPointColorsForAll(ControlPointNormal, ControlPointHover, ControlPointSelected);
	CurveSet->UI_SetSnapToGridForAll(bSnapToGrid);
	CurveSet->UI_SetShowGridForAll(bShowGrid);
	CurveSet->UI_SetGridSizeForAll(GridSizeCm);
	CurveSet->UI_SetGridExtentForAll(GridExtentCm);
	CurveSet->UI_SetGridOriginWorldForAll(GridOriginWorld);
	CurveSet->UI_SetGridColorForAll(GridColor);
	CurveSet->UI_SetGridBaseAlphaForAll(GridBaseAlpha);
	CurveSet->UI_SetLockToLocalXYForAll(bLockToLocalXY);
	if (bForcePlanar)
	{
		CurveSet->UI_SetForcePlanarAxisForAll(ForcePlanarAxis);
	}
	else
	{
		CurveSet->UI_SetForcePlanarAxisForAll(EBezierPlanarAxis::None);
	}
	CurveSet->UI_SetEditInteractionEnabledForAll(bEnableEditMode, bShowControlPoints, bShowStrip);
}

void ABezierDebugActor::ApplyCurveSetDebugAll() const
{
	if (!GetWorld()) return;
	for (TActorIterator<ABezierCurveSetActor> It(GetWorld()); It; ++It)
	{
		ApplyCurveSetDebug(*It);
	}
}

void ABezierDebugActor::ApplyCurveActorDebug() const
{
	if (!GetWorld()) return;

	for (TActorIterator<ABezierCurve3DActor> It(GetWorld()); It; ++It)
	{
		It->UI_SetEditMode(bEnableEditMode);
		It->UI_SetShowControlPoints(bShowControlPoints);
		It->UI_SetShowStrip(bShowStrip);
		It->UI_SetShowCubeStrip(bUseCubeStrip);
		if (bOverrideVisualSizes)
		{
			It->UI_SetControlPointSize(ControlPointSize);
			It->UI_SetStripSize(StripWidth, StripThickness);
		}
		It->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		It->UI_SetSnapToGrid(bSnapToGrid);
		It->UI_SetShowGrid(bShowGrid);
		It->UI_SetGridSizeCm(GridSizeCm);
		It->UI_SetGridExtentCm(GridExtentCm);
		It->UI_SetGridOriginWorld(GridOriginWorld);
		It->UI_SetGridColor(GridColor);
		It->UI_SetGridBaseAlpha(GridBaseAlpha);
		It->UI_SetLockToLocalXY(bLockToLocalXY);
		if (bForcePlanar)
		{
			It->UI_SetForcePlanarAxis(ForcePlanarAxis);
		}
		else
		{
			It->UI_SetForcePlanarAxis(EBezierPlanarAxis::None);
		}
		It->bShowControlPolygon = bShowControlPolygon;
		It->bPulseDebugLines = bPulseDebugLines;
		It->DebugPulseMinAlpha = DebugPulseMinAlpha;
		It->DebugPulseMaxAlpha = DebugPulseMaxAlpha;
		It->DebugPulseSpeed = DebugPulseSpeed;
		It->bPulseControlPoints = bPulseControlPoints;
		It->ControlPointPulseMinScale = ControlPointPulseMinScale;
		It->ControlPointPulseMaxScale = ControlPointPulseMaxScale;
		It->ControlPointPulseMinAlpha = ControlPointPulseMinAlpha;
		It->ControlPointPulseMaxAlpha = ControlPointPulseMaxAlpha;
		It->ControlPointPulseSpeed = ControlPointPulseSpeed;
		It->bPulseStrip = bPulseStrip;
		It->StripPulseMinWidth = StripPulseMinWidth;
		It->StripPulseMaxWidth = StripPulseMaxWidth;
		It->StripPulseMinThickness = StripPulseMinThickness;
		It->StripPulseMaxThickness = StripPulseMaxThickness;
		It->StripPulseMinAlpha = StripPulseMinAlpha;
		It->StripPulseMaxAlpha = StripPulseMaxAlpha;
		It->StripPulseSpeed = StripPulseSpeed;
		It->bPulseGrid = bPulseGrid;
		It->GridPulseMinAlpha = GridPulseMinAlpha;
		It->GridPulseMaxAlpha = GridPulseMaxAlpha;
		It->GridPulseSpeed = GridPulseSpeed;
		It->GridPulseMinThickness = GridPulseMinThickness;
		It->GridPulseMaxThickness = GridPulseMaxThickness;
		It->bEnableVisualFade = bEnableVisualFade;
		It->VisualFadeSpeed = VisualFadeSpeed;
	}

	for (TActorIterator<ABezierCurve2DActor> It(GetWorld()); It; ++It)
	{
		It->UI_SetEditMode(bEnableEditMode);
		It->UI_SetShowControlPoints(bShowControlPoints);
		It->UI_SetShowStrip(bShowStrip);
		It->UI_SetShowCubeStrip(bUseCubeStrip);
		if (bOverrideVisualSizes)
		{
			It->UI_SetControlPointSize(ControlPointSize);
			It->UI_SetStripSize(StripWidth, StripThickness);
		}
		It->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		It->UI_SetSnapToGrid(bSnapToGrid);
		It->UI_SetShowGrid(bShowGrid);
		It->UI_SetGridSizeCm(GridSizeCm);
		It->UI_SetGridExtentCm(GridExtentCm);
		It->UI_SetGridOriginWorld(GridOriginWorld);
		It->UI_SetGridColor(GridColor);
		It->UI_SetGridBaseAlpha(GridBaseAlpha);
		It->UI_SetLockToLocalXY(bLockToLocalXY);
		It->UI_SetForcePlanar(bForcePlanar);
		It->bShowControlPolygon = bShowControlPolygon;
		It->bPulseDebugLines = bPulseDebugLines;
		It->DebugPulseMinAlpha = DebugPulseMinAlpha;
		It->DebugPulseMaxAlpha = DebugPulseMaxAlpha;
		It->DebugPulseSpeed = DebugPulseSpeed;
		It->bPulseControlPoints = bPulseControlPoints;
		It->ControlPointPulseMinScale = ControlPointPulseMinScale;
		It->ControlPointPulseMaxScale = ControlPointPulseMaxScale;
		It->ControlPointPulseMinAlpha = ControlPointPulseMinAlpha;
		It->ControlPointPulseMaxAlpha = ControlPointPulseMaxAlpha;
		It->ControlPointPulseSpeed = ControlPointPulseSpeed;
		It->bPulseStrip = bPulseStrip;
		It->StripPulseMinWidth = StripPulseMinWidth;
		It->StripPulseMaxWidth = StripPulseMaxWidth;
		It->StripPulseMinThickness = StripPulseMinThickness;
		It->StripPulseMaxThickness = StripPulseMaxThickness;
		It->StripPulseMinAlpha = StripPulseMinAlpha;
		It->StripPulseMaxAlpha = StripPulseMaxAlpha;
		It->StripPulseSpeed = StripPulseSpeed;
		It->bPulseGrid = bPulseGrid;
		It->GridPulseMinAlpha = GridPulseMinAlpha;
		It->GridPulseMaxAlpha = GridPulseMaxAlpha;
		It->GridPulseSpeed = GridPulseSpeed;
		It->GridPulseMinThickness = GridPulseMinThickness;
		It->GridPulseMaxThickness = GridPulseMaxThickness;
		It->bEnableVisualFade = bEnableVisualFade;
		It->VisualFadeSpeed = VisualFadeSpeed;
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
