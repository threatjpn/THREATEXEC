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
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->SetMaxUndoSteps(UndoMaxSteps);
	}

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
		bEnableRuntimeEditing = A3->bEnableRuntimeEditing;
		bActorVisibleInGame = A3->bActorVisibleInGame;
		bHideVisualsWhenNotEditing = A3->bHideVisualsWhenNotEditing;
		bShowControlPoints = A3->bShowControlPoints;
		bShowStrip = A3->bShowStripMesh;
		bSnapToGrid = A3->bSnapToGrid;
		bShowGrid = A3->bShowGrid;
		GridSizeCm = A3->GridSizeCm;
		bLockToLocalXY = A3->bLockToLocalXY;
		bForcePlanar = A3->bForcePlanar;
		ForcePlanarAxis = A3->ForcePlanarAxis;
		bShowControlPolygon = A3->bShowControlPolygon;
		DebugLineColor = A3->DebugLineColor;
		DebugLevelColor = A3->DebugLevelColor;
		DebugResultColor = A3->DebugResultColor;
		DebugSamplePointColor = A3->DebugSamplePointColor;
		bPulseDebugLines = A3->bPulseDebugLines;
		DebugPulseMinAlpha = A3->DebugPulseMinAlpha;
		DebugPulseMaxAlpha = A3->DebugPulseMaxAlpha;
		DebugPulseMinThickness = A3->DebugPulseMinThickness;
		DebugPulseMaxThickness = A3->DebugPulseMaxThickness;
		DebugPulseSpeed = A3->DebugPulseSpeed;
		bPulseControlPoints = A3->bPulseControlPoints;
		ControlPointPulseMinScale = A3->ControlPointPulseMinScale;
		ControlPointPulseMaxScale = A3->ControlPointPulseMaxScale;
		ControlPointPulseMinAlpha = A3->ControlPointPulseMinAlpha;
		ControlPointPulseMaxAlpha = A3->ControlPointPulseMaxAlpha;
		ControlPointPulseSpeed = A3->ControlPointPulseSpeed;
		bPulseStrip = A3->bPulseStrip;
		StripPulseMinWidth = A3->StripPulseMinWidth;
		StripPulseMaxWidth = A3->StripPulseMaxWidth;
		StripPulseMinThickness = A3->StripPulseMinThickness;
		StripPulseMaxThickness = A3->StripPulseMaxThickness;
		StripPulseMinAlpha = A3->StripPulseMinAlpha;
		StripPulseMaxAlpha = A3->StripPulseMaxAlpha;
		StripPulseSpeed = A3->StripPulseSpeed;
		bPulseGrid = A3->bPulseGrid;
		GridPulseMinAlpha = A3->GridPulseMinAlpha;
		GridPulseMaxAlpha = A3->GridPulseMaxAlpha;
		GridPulseSpeed = A3->GridPulseSpeed;
		GridPulseMinThickness = A3->GridPulseMinThickness;
		GridPulseMaxThickness = A3->GridPulseMaxThickness;
		GridThicknessScale = A3->GridThicknessScale;
		DebugThicknessScale = A3->DebugThicknessScale;
		bForceVisualsOnTop = A3->bForceVisualsOnTop;
		VisualTranslucencySortPriority = A3->VisualTranslucencySortPriority;
		bEnableVisualFade = A3->bEnableVisualFade;
		VisualFadeSpeed = A3->VisualFadeSpeed;
	}
	else if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(SourceActor))
	{
		bEnableEditMode = A2->UI_GetEditMode();
		bEnableRuntimeEditing = A2->bEnableRuntimeEditing;
		bActorVisibleInGame = A2->bActorVisibleInGame;
		bHideVisualsWhenNotEditing = A2->bHideVisualsWhenNotEditing;
		bShowControlPoints = A2->bShowControlPoints;
		bShowStrip = A2->bShowStripMesh;
		bSnapToGrid = A2->bSnapToGrid;
		bShowGrid = A2->bShowGrid;
		GridSizeCm = A2->GridSizeCm;
		bLockToLocalXY = A2->bLockToLocalXY;
		bForcePlanar = A2->bForcePlanar;
		ForcePlanarAxis = bForcePlanar ? EBezierPlanarAxis::XY : EBezierPlanarAxis::None;
		bShowControlPolygon = A2->bShowControlPolygon;
		DebugLineColor = A2->DebugLineColor;
		DebugLevelColor = A2->DebugLevelColor;
		DebugResultColor = A2->DebugResultColor;
		DebugSamplePointColor = A2->DebugSamplePointColor;
		bPulseDebugLines = A2->bPulseDebugLines;
		DebugPulseMinAlpha = A2->DebugPulseMinAlpha;
		DebugPulseMaxAlpha = A2->DebugPulseMaxAlpha;
		DebugPulseMinThickness = A2->DebugPulseMinThickness;
		DebugPulseMaxThickness = A2->DebugPulseMaxThickness;
		DebugPulseSpeed = A2->DebugPulseSpeed;
		bPulseControlPoints = A2->bPulseControlPoints;
		ControlPointPulseMinScale = A2->ControlPointPulseMinScale;
		ControlPointPulseMaxScale = A2->ControlPointPulseMaxScale;
		ControlPointPulseMinAlpha = A2->ControlPointPulseMinAlpha;
		ControlPointPulseMaxAlpha = A2->ControlPointPulseMaxAlpha;
		ControlPointPulseSpeed = A2->ControlPointPulseSpeed;
		bPulseStrip = A2->bPulseStrip;
		StripPulseMinWidth = A2->StripPulseMinWidth;
		StripPulseMaxWidth = A2->StripPulseMaxWidth;
		StripPulseMinThickness = A2->StripPulseMinThickness;
		StripPulseMaxThickness = A2->StripPulseMaxThickness;
		StripPulseMinAlpha = A2->StripPulseMinAlpha;
		StripPulseMaxAlpha = A2->StripPulseMaxAlpha;
		StripPulseSpeed = A2->StripPulseSpeed;
		bPulseGrid = A2->bPulseGrid;
		GridPulseMinAlpha = A2->GridPulseMinAlpha;
		GridPulseMaxAlpha = A2->GridPulseMaxAlpha;
		GridPulseSpeed = A2->GridPulseSpeed;
		GridPulseMinThickness = A2->GridPulseMinThickness;
		GridPulseMaxThickness = A2->GridPulseMaxThickness;
		GridThicknessScale = A2->GridThicknessScale;
		DebugThicknessScale = A2->DebugThicknessScale;
		bForceVisualsOnTop = A2->bForceVisualsOnTop;
		VisualTranslucencySortPriority = A2->VisualTranslucencySortPriority;
		bEnableVisualFade = A2->bEnableVisualFade;
		VisualFadeSpeed = A2->VisualFadeSpeed;
	}

	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		UndoMaxSteps = Subsystem->GetMaxUndoSteps();
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
		const EBezierPlanarAxis AxisToApply = (ForcePlanarAxis == EBezierPlanarAxis::None) ? EBezierPlanarAxis::XY : ForcePlanarAxis;
		CurveSet->UI_SetForcePlanarAxisForAll(AxisToApply);
	}
	else
	{
		CurveSet->UI_SetForcePlanarAxisForAll(EBezierPlanarAxis::None);
	}
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
	if (ABezierCurve3DActor* Curve3D = Cast<ABezierCurve3DActor>(CurveActor))
	{
		Curve3D->bEnableRuntimeEditing = bEnableRuntimeEditing;
		Curve3D->bActorVisibleInGame = bActorVisibleInGame;
		Curve3D->bHideVisualsWhenNotEditing = bHideVisualsWhenNotEditing;

		Curve3D->UI_SetActorVisibleInGame(bActorVisibleInGame);
		Curve3D->UI_SetEditMode(bEnableEditMode);
		Curve3D->UI_SetShowControlPoints(bShowControlPoints);
		Curve3D->UI_SetShowStrip(bShowStrip);
		Curve3D->UI_SetShowCubeStrip(bUseCubeStrip);
		if (bOverrideVisualSizes)
		{
			Curve3D->UI_SetControlPointSize(ControlPointSize);
			Curve3D->UI_SetStripSize(StripWidth, StripThickness);
		}
		if (bOverrideControlPointMaterial && ControlPointMaterial)
		{
			Curve3D->ControlPointMaterial = ControlPointMaterial;
			Curve3D->UI_SetControlPointSize(Curve3D->ControlPointVisualScale);
		}
		if (bOverrideStripMaterial && StripMaterial)
		{
			Curve3D->StripMaterial = StripMaterial;
			Curve3D->UI_SetShowCubeStrip(Curve3D->bUseCubeStrip);
		}
		Curve3D->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		Curve3D->UI_SetSnapToGrid(bSnapToGrid);
		Curve3D->UI_SetShowGrid(bShowGrid);
		Curve3D->UI_SetGridSizeCm(GridSizeCm);
		Curve3D->UI_SetGridExtentCm(GridExtentCm);
		Curve3D->UI_SetGridOriginWorld(GridOriginWorld);
		Curve3D->UI_SetGridColor(GridColor);
		Curve3D->UI_SetGridBaseAlpha(GridBaseAlpha);
		Curve3D->UI_SetShowGridXY(bShowGridXY);
		Curve3D->UI_SetShowGridXZ(bShowGridXZ);
		Curve3D->UI_SetShowGridYZ(bShowGridYZ);
		Curve3D->UI_SetLockToLocalXY(bLockToLocalXY);
		if (bForcePlanar)
		{
			const EBezierPlanarAxis AxisToApply = (ForcePlanarAxis == EBezierPlanarAxis::None) ? EBezierPlanarAxis::XY : ForcePlanarAxis;
			Curve3D->UI_SetForcePlanarAxis(AxisToApply);
		}
		else
		{
			Curve3D->UI_SetForcePlanarAxis(EBezierPlanarAxis::None);
		}
		Curve3D->bShowControlPolygon = bShowControlPolygon;
		Curve3D->DebugLineColor = DebugLineColor;
		Curve3D->DebugLevelColor = DebugLevelColor;
		Curve3D->DebugResultColor = DebugResultColor;
		Curve3D->DebugSamplePointColor = DebugSamplePointColor;
		if (bOverridePulseSettings)
		{
			Curve3D->bPulseDebugLines = bPulseDebugLines;
			Curve3D->DebugPulseMinAlpha = DebugPulseMinAlpha;
			Curve3D->DebugPulseMaxAlpha = DebugPulseMaxAlpha;
			Curve3D->DebugPulseMinThickness = DebugPulseMinThickness;
			Curve3D->DebugPulseMaxThickness = DebugPulseMaxThickness;
			Curve3D->DebugPulseSpeed = DebugPulseSpeed;
			Curve3D->bPulseControlPoints = bPulseControlPoints;
			Curve3D->ControlPointPulseMinScale = ControlPointPulseMinScale;
			Curve3D->ControlPointPulseMaxScale = ControlPointPulseMaxScale;
			Curve3D->ControlPointPulseMinAlpha = ControlPointPulseMinAlpha;
			Curve3D->ControlPointPulseMaxAlpha = ControlPointPulseMaxAlpha;
			Curve3D->ControlPointPulseSpeed = ControlPointPulseSpeed;
			Curve3D->bPulseStrip = bPulseStrip;
			Curve3D->StripPulseMinWidth = StripPulseMinWidth;
			Curve3D->StripPulseMaxWidth = StripPulseMaxWidth;
			Curve3D->StripPulseMinThickness = StripPulseMinThickness;
			Curve3D->StripPulseMaxThickness = StripPulseMaxThickness;
			Curve3D->StripPulseMinAlpha = StripPulseMinAlpha;
			Curve3D->StripPulseMaxAlpha = StripPulseMaxAlpha;
			Curve3D->StripPulseSpeed = StripPulseSpeed;
			Curve3D->bPulseGrid = bPulseGrid;
			Curve3D->GridPulseMinAlpha = GridPulseMinAlpha;
			Curve3D->GridPulseMaxAlpha = GridPulseMaxAlpha;
			Curve3D->GridPulseSpeed = GridPulseSpeed;
			Curve3D->GridPulseMinThickness = GridPulseMinThickness;
			Curve3D->GridPulseMaxThickness = GridPulseMaxThickness;
			Curve3D->GridThicknessScale = GridThicknessScale;
			Curve3D->DebugThicknessScale = DebugThicknessScale;
		}
		Curve3D->bForceVisualsOnTop = bForceVisualsOnTop;
		Curve3D->VisualTranslucencySortPriority = VisualTranslucencySortPriority;
		Curve3D->bEnableVisualFade = bEnableVisualFade;
		Curve3D->VisualFadeSpeed = VisualFadeSpeed;
		return;
	}

	if (ABezierCurve2DActor* Curve2D = Cast<ABezierCurve2DActor>(CurveActor))
	{
		Curve2D->bEnableRuntimeEditing = bEnableRuntimeEditing;
		Curve2D->bActorVisibleInGame = bActorVisibleInGame;
		Curve2D->bHideVisualsWhenNotEditing = bHideVisualsWhenNotEditing;

		Curve2D->UI_SetActorVisibleInGame(bActorVisibleInGame);
		Curve2D->UI_SetEditMode(bEnableEditMode);
		Curve2D->UI_SetShowControlPoints(bShowControlPoints);
		Curve2D->UI_SetShowStrip(bShowStrip);
		Curve2D->UI_SetShowCubeStrip(bUseCubeStrip);
		if (bOverrideVisualSizes)
		{
			Curve2D->UI_SetControlPointSize(ControlPointSize);
			Curve2D->UI_SetStripSize(StripWidth, StripThickness);
		}
		if (bOverrideControlPointMaterial && ControlPointMaterial)
		{
			Curve2D->ControlPointMaterial = ControlPointMaterial;
			Curve2D->UI_SetControlPointSize(Curve2D->ControlPointVisualScale);
		}
		if (bOverrideStripMaterial && StripMaterial)
		{
			Curve2D->StripMaterial = StripMaterial;
			Curve2D->UI_SetShowCubeStrip(Curve2D->bUseCubeStrip);
		}
		Curve2D->UI_SetControlPointColors(ControlPointNormal, ControlPointHover, ControlPointSelected);
		Curve2D->UI_SetSnapToGrid(bSnapToGrid);
		Curve2D->UI_SetShowGrid(bShowGrid);
		Curve2D->UI_SetGridSizeCm(GridSizeCm);
		Curve2D->UI_SetGridExtentCm(GridExtentCm);
		Curve2D->UI_SetGridOriginWorld(GridOriginWorld);
		Curve2D->UI_SetGridColor(GridColor);
		Curve2D->UI_SetGridBaseAlpha(GridBaseAlpha);
		Curve2D->UI_SetShowGridXY(bShowGridXY);
		Curve2D->UI_SetLockToLocalXY(bLockToLocalXY);
		Curve2D->UI_SetForcePlanar(bForcePlanar);
		Curve2D->bShowControlPolygon = bShowControlPolygon;
		Curve2D->DebugLineColor = DebugLineColor;
		Curve2D->DebugLevelColor = DebugLevelColor;
		Curve2D->DebugResultColor = DebugResultColor;
		Curve2D->DebugSamplePointColor = DebugSamplePointColor;
		if (bOverridePulseSettings)
		{
			Curve2D->bPulseDebugLines = bPulseDebugLines;
			Curve2D->DebugPulseMinAlpha = DebugPulseMinAlpha;
			Curve2D->DebugPulseMaxAlpha = DebugPulseMaxAlpha;
			Curve2D->DebugPulseMinThickness = DebugPulseMinThickness;
			Curve2D->DebugPulseMaxThickness = DebugPulseMaxThickness;
			Curve2D->DebugPulseSpeed = DebugPulseSpeed;
			Curve2D->bPulseControlPoints = bPulseControlPoints;
			Curve2D->ControlPointPulseMinScale = ControlPointPulseMinScale;
			Curve2D->ControlPointPulseMaxScale = ControlPointPulseMaxScale;
			Curve2D->ControlPointPulseMinAlpha = ControlPointPulseMinAlpha;
			Curve2D->ControlPointPulseMaxAlpha = ControlPointPulseMaxAlpha;
			Curve2D->ControlPointPulseSpeed = ControlPointPulseSpeed;
			Curve2D->bPulseStrip = bPulseStrip;
			Curve2D->StripPulseMinWidth = StripPulseMinWidth;
			Curve2D->StripPulseMaxWidth = StripPulseMaxWidth;
			Curve2D->StripPulseMinThickness = StripPulseMinThickness;
			Curve2D->StripPulseMaxThickness = StripPulseMaxThickness;
			Curve2D->StripPulseMinAlpha = StripPulseMinAlpha;
			Curve2D->StripPulseMaxAlpha = StripPulseMaxAlpha;
			Curve2D->StripPulseSpeed = StripPulseSpeed;
			Curve2D->bPulseGrid = bPulseGrid;
			Curve2D->GridPulseMinAlpha = GridPulseMinAlpha;
			Curve2D->GridPulseMaxAlpha = GridPulseMaxAlpha;
			Curve2D->GridPulseSpeed = GridPulseSpeed;
			Curve2D->GridPulseMinThickness = GridPulseMinThickness;
			Curve2D->GridPulseMaxThickness = GridPulseMaxThickness;
			Curve2D->GridThicknessScale = GridThicknessScale;
			Curve2D->DebugThicknessScale = DebugThicknessScale;
		}
		Curve2D->bForceVisualsOnTop = bForceVisualsOnTop;
		Curve2D->VisualTranslucencySortPriority = VisualTranslucencySortPriority;
		Curve2D->bEnableVisualFade = bEnableVisualFade;
		Curve2D->VisualFadeSpeed = VisualFadeSpeed;
		return;
	}
}
