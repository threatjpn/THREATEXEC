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
	ApplyCurveActorDebug();
	ApplyCurveSetDebugAll();
}

void ABezierDebugActor::ApplyDebugToCurve(AActor* CurveActor)
{
	ApplyToCurveActor(CurveActor);
}

void ABezierDebugActor::SyncFromWorldState()
{
	if (!GetWorld())
	{
		return;
	}

	AActor* SourceActor = nullptr;
	if (UBezierEditSubsystem* Subsystem = GetWorld()->GetSubsystem<UBezierEditSubsystem>())
	{
		SourceActor = Subsystem->GetFocused();
	}

	if (!SourceActor)
	{
		if (TActorIterator<ABezierCurve3DActor> It(GetWorld()); It)
		{
			SourceActor = *It;
		}
		else if (TActorIterator<ABezierCurve2DActor> It2(GetWorld()); It2)
		{
			SourceActor = *It2;
		}
	}

	if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(SourceActor))
	{
		bEnableEditMode = A3->UI_GetEditMode();
		bShowControlPoints = A3->bShowControlPoints;
		bShowStrip = A3->bShowStripMesh;
		bSnapToGrid = A3->bSnapToGrid;
		bShowGrid = A3->bShowGrid;
		GridSizeCm = A3->GridSizeCm;
		bLockToLocalXY = A3->bLockToLocalXY;
		bForcePlanar = A3->bForcePlanar;
		ForcePlanarAxis = A3->ForcePlanarAxis;
	}
	else if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(SourceActor))
	{
		bEnableEditMode = A2->UI_GetEditMode();
		bShowControlPoints = A2->bShowControlPoints;
		bShowStrip = A2->bShowStripMesh;
		bSnapToGrid = A2->bSnapToGrid;
		bShowGrid = A2->bShowGrid;
		GridSizeCm = A2->GridSizeCm;
		bLockToLocalXY = A2->bLockToLocalXY;
		bForcePlanar = A2->bForcePlanar;
		ForcePlanarAxis = bForcePlanar ? EBezierPlanarAxis::XY : EBezierPlanarAxis::None;
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
	CurveSet->UI_SetShowGridXYForAll(bShowGridXY);
	CurveSet->UI_SetShowGridXZForAll(bShowGridXZ);
	CurveSet->UI_SetShowGridYZForAll(bShowGridYZ);
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
		ApplyToCurveActor(*It);
	}

	for (TActorIterator<ABezierCurve2DActor> It(GetWorld()); It; ++It)
	{
		ApplyToCurveActor(*It);
	}
}

void ABezierDebugActor::ApplyToCurveActor(AActor* CurveActor) const
{
	if (ABezierCurve3DActor* It = Cast<ABezierCurve3DActor>(CurveActor))
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
		if (bOverrideControlPointMaterial && ControlPointMaterial)
		{
			It->ControlPointMaterial = ControlPointMaterial;
			It->UI_SetControlPointSize(It->ControlPointVisualScale);
		}
		It->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		It->UI_SetSnapToGrid(bSnapToGrid);
		It->UI_SetShowGrid(bShowGrid);
		It->UI_SetGridSizeCm(GridSizeCm);
		It->UI_SetGridExtentCm(GridExtentCm);
		It->UI_SetGridOriginWorld(GridOriginWorld);
		It->UI_SetGridColor(GridColor);
		It->UI_SetGridBaseAlpha(GridBaseAlpha);
		It->UI_SetShowGridXY(bShowGridXY);
		It->UI_SetShowGridXZ(bShowGridXZ);
		It->UI_SetShowGridYZ(bShowGridYZ);
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
		It->DebugPulseMinThickness = DebugPulseMinThickness;
		It->DebugPulseMaxThickness = DebugPulseMaxThickness;
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
		It->GridThicknessScale = GridThicknessScale;
		It->DebugThicknessScale = DebugThicknessScale;
		It->bForceVisualsOnTop = bForceVisualsOnTop;
		It->VisualTranslucencySortPriority = VisualTranslucencySortPriority;
		It->bEnableVisualFade = bEnableVisualFade;
		It->VisualFadeSpeed = VisualFadeSpeed;
		return;
	}

	if (ABezierCurve2DActor* It = Cast<ABezierCurve2DActor>(CurveActor))
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
		if (bOverrideControlPointMaterial && ControlPointMaterial)
		{
			It->ControlPointMaterial = ControlPointMaterial;
			It->UI_SetControlPointSize(It->ControlPointVisualScale);
		}
		It->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		It->UI_SetSnapToGrid(bSnapToGrid);
		It->UI_SetShowGrid(bShowGrid);
		It->UI_SetGridSizeCm(GridSizeCm);
		It->UI_SetGridExtentCm(GridExtentCm);
		It->UI_SetGridOriginWorld(GridOriginWorld);
		It->UI_SetGridColor(GridColor);
		It->UI_SetGridBaseAlpha(GridBaseAlpha);
		It->UI_SetShowGridXY(bShowGridXY);
		It->UI_SetLockToLocalXY(bLockToLocalXY);
		It->UI_SetForcePlanar(bForcePlanar);
		It->bShowControlPolygon = bShowControlPolygon;
		It->bPulseDebugLines = bPulseDebugLines;
		It->DebugPulseMinAlpha = DebugPulseMinAlpha;
		It->DebugPulseMaxAlpha = DebugPulseMaxAlpha;
		It->DebugPulseMinThickness = DebugPulseMinThickness;
		It->DebugPulseMaxThickness = DebugPulseMaxThickness;
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
		It->GridThicknessScale = GridThicknessScale;
		It->DebugThicknessScale = DebugThicknessScale;
		It->bForceVisualsOnTop = bForceVisualsOnTop;
		It->VisualTranslucencySortPriority = VisualTranslucencySortPriority;
		It->bEnableVisualFade = bEnableVisualFade;
		It->VisualFadeSpeed = VisualFadeSpeed;
	}
}
