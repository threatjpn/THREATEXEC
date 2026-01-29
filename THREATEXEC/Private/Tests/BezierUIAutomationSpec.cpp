// ============================================================================
// BezierUIAutomationSpec.cpp
// UI-focused automation tests for 2D/3D Bezier actors.
// Keeps existing specs untouched and validates UI/core wiring.
// ============================================================================

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_EDITOR

#include "Editor.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierCurveSetActor.h"
#include "BezierEditSubsystem.h"

#include "EngineUtils.h"
#include "Misc/FileHelper.h"

namespace
{
	static UWorld* GetEditorWorldChecked_UI()
	{
		check(GEditor);
		return GEditor->GetEditorWorldContext().World();
	}

	static FString MakeTempDir_UI(const FString& Name)
	{
		const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation") / Name);
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		if (PF.DirectoryExists(*Dir)) PF.DeleteDirectoryRecursively(*Dir);
		PF.CreateDirectoryTree(*Dir);
		return Dir;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_2D_Core,
	"Bezier/UI/2D_Core",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_2D_Core::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	const FString OutDir = MakeTempDir_UI(TEXT("BezierUI2D"));

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A)) return false;

	A->Control = { FVector2D(0,0), FVector2D(1,0), FVector2D(2,0) };
	A->OverwriteSplineFromControl();

	A->Control.Empty();
	A->SyncControlFromSpline();
	TestTrue(TEXT("Sync restored control"), A->Control.Num() == 3);

	A->ReverseControlOrder();
	TestTrue(TEXT("Reverse order"), A->Control[0].Equals(FVector2D(2,0), 1e-6));

	A->StripSegments = 5;
	A->ResampleBezierToSpline();
	TestTrue(TEXT("Resample count"), A->Control.Num() == 5);

	A->UI_SetEditMode(true);
	TestTrue(TEXT("UI_SetEditMode"), A->bEditMode);

	A->UI_SetActorVisibleInGame(false);
	TestTrue(TEXT("UI_SetActorVisibleInGame"), !A->bActorVisibleInGame);

	A->UI_SetShowStrip(false);
	TestTrue(TEXT("UI_SetShowStrip"), !A->bShowStripMesh);

	A->UI_SetShowCubeStrip(false);
	TestTrue(TEXT("UI_SetShowCubeStrip"), !A->bUseCubeStrip);

	A->UI_SetStripSize(12.0f, 3.5f);
	TestTrue(TEXT("UI_SetStripSize width"), FMath::IsNearlyEqual(A->StripWidth, 12.0f));
	TestTrue(TEXT("UI_SetStripSize thickness"), FMath::IsNearlyEqual(A->StripThickness, 3.5f));

	A->UI_SetShowControlPoints(false);
	TestTrue(TEXT("UI_SetShowControlPoints"), !A->bShowControlPoints);

	A->UI_SetControlPointSize(0.12f);
	TestTrue(TEXT("UI_SetControlPointSize"), FMath::IsNearlyEqual(A->ControlPointVisualScale, 0.12f));

	const FLinearColor Normal(0.1f, 0.2f, 0.3f, 1.0f);
	const FLinearColor Hover(0.4f, 0.5f, 0.6f, 1.0f);
	const FLinearColor Selected(0.7f, 0.8f, 0.9f, 1.0f);
	A->UI_SetControlPointColors(Normal, Hover, Selected);
	TestTrue(TEXT("UI_SetControlPointColors normal"), A->ControlPointColor.Equals(Normal));
	TestTrue(TEXT("UI_SetControlPointColors hover"), A->ControlPointHoverColor.Equals(Hover));
	TestTrue(TEXT("UI_SetControlPointColors selected"), A->ControlPointSelectedColor.Equals(Selected));

	A->UI_SetSnapToGrid(true);
	TestTrue(TEXT("UI_SetSnapToGrid"), A->bSnapToGrid);

	A->UI_SetGridSizeCm(15.0f);
	TestTrue(TEXT("UI_SetGridSizeCm"), FMath::IsNearlyEqual(A->GridSizeCm, 15.0f));

	A->UI_SetShowGrid(true);
	TestTrue(TEXT("UI_SetShowGrid"), A->bShowGrid);

	A->UI_SetGridExtentCm(250.0f);
	TestTrue(TEXT("UI_SetGridExtentCm"), FMath::IsNearlyEqual(A->GridExtentCm, 250.0f));

	const FVector GridOrigin2D(10.0f, 20.0f, 0.0f);
	A->UI_SetGridOriginWorld(GridOrigin2D);
	TestTrue(TEXT("UI_SetGridOriginWorld"), A->GridOriginWorld.Equals(GridOrigin2D));

	const FLinearColor GridColor2D(0.4f, 0.4f, 0.4f, 1.0f);
	A->UI_SetGridColor(GridColor2D);
	TestTrue(TEXT("UI_SetGridColor"), A->GridColor.Equals(GridColor2D));

	A->UI_SetGridBaseAlpha(0.12f);
	TestTrue(TEXT("UI_SetGridBaseAlpha"), FMath::IsNearlyEqual(A->GridBaseAlpha, 0.12f));

	A->UI_SetShowGridXY(true);
	TestTrue(TEXT("UI_SetShowGridXY"), A->bShowGridXY);

	A->UI_SetLockToLocalXY(false);
	TestTrue(TEXT("UI_SetLockToLocalXY"), !A->bLockToLocalXY);

	A->UI_SetForcePlanar(false);
	TestTrue(TEXT("UI_SetForcePlanar"), !A->bForcePlanar);

	A->bPulseDebugLines = false;
	A->DebugPulseMinAlpha = 0.0f;
	A->DebugPulseMaxAlpha = 0.5f;
	A->DebugPulseSpeed = 2.0f;
	TestTrue(TEXT("Debug pulse params"), !A->bPulseDebugLines && FMath::IsNearlyEqual(A->DebugPulseMaxAlpha, 0.5f));

	A->bPulseGrid = true;
	A->GridPulseMinAlpha = 0.1f;
	A->GridPulseMaxAlpha = 0.6f;
	A->GridPulseSpeed = 1.5f;
	A->GridPulseMinThickness = 0.25f;
	A->GridPulseMaxThickness = 1.25f;
	TestTrue(TEXT("Grid pulse params"), A->bPulseGrid && FMath::IsNearlyEqual(A->GridPulseMinAlpha, 0.1f));

	A->ControlPointPulseMinAlpha = 0.35f;
	A->ControlPointPulseMaxAlpha = 0.9f;
	TestTrue(TEXT("Control point pulse alpha"), FMath::IsNearlyEqual(A->ControlPointPulseMinAlpha, 0.35f));

	A->StripPulseMinAlpha = 0.3f;
	A->StripPulseMaxAlpha = 0.95f;
	TestTrue(TEXT("Strip pulse alpha"), FMath::IsNearlyEqual(A->StripPulseMaxAlpha, 0.95f));

	A->bEnableVisualFade = true;
	A->VisualFadeSpeed = 3.0f;
	TestTrue(TEXT("Visual fade params"), A->bEnableVisualFade && FMath::IsNearlyEqual(A->VisualFadeSpeed, 3.0f));

	A->IOPathAbsolute = OutDir;
	A->UI_ExportAllJson();
	TestTrue(TEXT("control.json"), FPaths::FileExists(OutDir / TEXT("control.json")));
	TestTrue(TEXT("samples.json"), FPaths::FileExists(OutDir / TEXT("samples.json")));
	TestTrue(TEXT("proof.json"), FPaths::FileExists(OutDir / TEXT("proof.json")));

	A->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_3D_Core,
	"Bezier/UI/3D_Core",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_3D_Core::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	const FString OutDir = MakeTempDir_UI(TEXT("BezierUI3D"));

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve3DActor* A = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A)) return false;

	A->Control = { FVector(0,0,0), FVector(1,0,0), FVector(2,0,0) };
	A->OverwriteSplineFromControl();

	A->Control.Empty();
	A->SyncControlFromSpline();
	TestTrue(TEXT("Sync restored control"), A->Control.Num() == 3);

	A->ReverseControlOrder();
	TestTrue(TEXT("Reverse order"), A->Control[0].Equals(FVector(2,0,0), 1e-6));

	A->StripSegments = 6;
	A->UI_ResampleParam();
	TestTrue(TEXT("Param resample count"), A->Control.Num() == 6);

	A->StripSegments = 4;
	A->UI_ResampleArcLength();
	TestTrue(TEXT("Arc resample count"), A->Control.Num() == 4);

	A->UI_SetEditMode(true);
	TestTrue(TEXT("UI_SetEditMode"), A->bEditMode);

	A->UI_SetActorVisibleInGame(false);
	TestTrue(TEXT("UI_SetActorVisibleInGame"), !A->bActorVisibleInGame);

	A->UI_SetShowStrip(false);
	TestTrue(TEXT("UI_SetShowStrip"), !A->bShowStripMesh);

	A->UI_SetShowCubeStrip(false);
	TestTrue(TEXT("UI_SetShowCubeStrip"), !A->bUseCubeStrip);

	A->UI_SetStripSize(8.0f, 1.5f);
	TestTrue(TEXT("UI_SetStripSize width"), FMath::IsNearlyEqual(A->StripWidth, 8.0f));
	TestTrue(TEXT("UI_SetStripSize thickness"), FMath::IsNearlyEqual(A->StripThickness, 1.5f));

	A->UI_SetShowControlPoints(false);
	TestTrue(TEXT("UI_SetShowControlPoints"), !A->bShowControlPoints);

	A->UI_SetControlPointSize(0.08f);
	TestTrue(TEXT("UI_SetControlPointSize"), FMath::IsNearlyEqual(A->ControlPointVisualScale, 0.08f));

	const FLinearColor Normal3D(0.15f, 0.25f, 0.35f, 1.0f);
	const FLinearColor Hover3D(0.45f, 0.55f, 0.65f, 1.0f);
	const FLinearColor Selected3D(0.75f, 0.85f, 0.95f, 1.0f);
	A->UI_SetControlPointColors(Normal3D, Hover3D, Selected3D);
	TestTrue(TEXT("UI_SetControlPointColors normal"), A->ControlPointColor.Equals(Normal3D));
	TestTrue(TEXT("UI_SetControlPointColors hover"), A->ControlPointHoverColor.Equals(Hover3D));
	TestTrue(TEXT("UI_SetControlPointColors selected"), A->ControlPointSelectedColor.Equals(Selected3D));

	A->UI_SetSnapToGrid(true);
	TestTrue(TEXT("UI_SetSnapToGrid"), A->bSnapToGrid);

	A->UI_SetGridSizeCm(12.5f);
	TestTrue(TEXT("UI_SetGridSizeCm"), FMath::IsNearlyEqual(A->GridSizeCm, 12.5f));

	A->UI_SetShowGrid(true);
	TestTrue(TEXT("UI_SetShowGrid"), A->bShowGrid);

	A->UI_SetGridExtentCm(250.0f);
	TestTrue(TEXT("UI_SetGridExtentCm"), FMath::IsNearlyEqual(A->GridExtentCm, 250.0f));

	const FVector GridOrigin3D(5.0f, 15.0f, -10.0f);
	A->UI_SetGridOriginWorld(GridOrigin3D);
	TestTrue(TEXT("UI_SetGridOriginWorld"), A->GridOriginWorld.Equals(GridOrigin3D));

	const FLinearColor GridColor3D(0.3f, 0.3f, 0.3f, 1.0f);
	A->UI_SetGridColor(GridColor3D);
	TestTrue(TEXT("UI_SetGridColor"), A->GridColor.Equals(GridColor3D));

	A->UI_SetGridBaseAlpha(0.1f);
	TestTrue(TEXT("UI_SetGridBaseAlpha"), FMath::IsNearlyEqual(A->GridBaseAlpha, 0.1f));

	A->UI_SetShowGridXY(true);
	A->UI_SetShowGridXZ(true);
	A->UI_SetShowGridYZ(true);
	TestTrue(TEXT("UI_SetShowGridXY"), A->bShowGridXY);

	A->UI_SetLockToLocalXY(true);
	TestTrue(TEXT("UI_SetLockToLocalXY"), A->bLockToLocalXY);

	A->UI_SetForcePlanar(true);
	TestTrue(TEXT("UI_SetForcePlanar"), A->bForcePlanar);

	A->bPulseDebugLines = false;
	A->DebugPulseMinAlpha = 0.0f;
	A->DebugPulseMaxAlpha = 0.5f;
	A->DebugPulseSpeed = 2.0f;
	TestTrue(TEXT("Debug pulse params"), !A->bPulseDebugLines && FMath::IsNearlyEqual(A->DebugPulseMaxAlpha, 0.5f));

	A->bPulseGrid = true;
	A->GridPulseMinAlpha = 0.2f;
	A->GridPulseMaxAlpha = 0.7f;
	A->GridPulseSpeed = 1.25f;
	A->GridPulseMinThickness = 0.3f;
	A->GridPulseMaxThickness = 1.3f;
	TestTrue(TEXT("Grid pulse params"), A->bPulseGrid && FMath::IsNearlyEqual(A->GridPulseMaxAlpha, 0.7f));

	A->ControlPointPulseMinAlpha = 0.25f;
	A->ControlPointPulseMaxAlpha = 0.85f;
	TestTrue(TEXT("Control point pulse alpha"), FMath::IsNearlyEqual(A->ControlPointPulseMaxAlpha, 0.85f));

	A->StripPulseMinAlpha = 0.2f;
	A->StripPulseMaxAlpha = 0.9f;
	TestTrue(TEXT("Strip pulse alpha"), FMath::IsNearlyEqual(A->StripPulseMinAlpha, 0.2f));

	A->bEnableVisualFade = true;
	A->VisualFadeSpeed = 4.0f;
	TestTrue(TEXT("Visual fade params"), A->bEnableVisualFade && FMath::IsNearlyEqual(A->VisualFadeSpeed, 4.0f));

	A->UI_MirrorCurveX();
	A->UI_MirrorCurveY();
	A->UI_MirrorCurveZ();

	A->IOPathAbsolute = OutDir;
	A->UI_ExportAllJson();
	TestTrue(TEXT("control3d.json"), FPaths::FileExists(OutDir / TEXT("control3d.json")));
	TestTrue(TEXT("samples3d.json"), FPaths::FileExists(OutDir / TEXT("samples3d.json")));
	TestTrue(TEXT("proof3d.json"), FPaths::FileExists(OutDir / TEXT("proof3d.json")));

	A->Control.Empty();
	A->UI_ImportFromJson();
	TestTrue(TEXT("Import restored control"), A->Control.Num() >= 2);

	A->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_PivotGizmo3D,
	"Bezier/UI/PivotGizmo3D",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_PivotGizmo3D::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve3DActor* A = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A)) return false;

	A->bEnableRuntimeEditing = true;
	A->bEditMode = true;
	A->bActorVisibleInGame = true;
	A->bSnapToGrid = false;
	A->bForcePlanar = false;
	A->Scale = 1.0;

	A->Control = { FVector(1, 0, 0), FVector(0, 1, 0) };
	A->OverwriteSplineFromControl();

	A->SelectedControlPointIndex = 0;
	A->bSelectAllControlPoints = false;

	FVector Pivot = FVector::ZeroVector;
	TestTrue(TEXT("Pivot world available"), A->UI_GetPivotWorld(Pivot));

	const FVector RayOriginAxis = Pivot + FVector(A->PivotGizmo.AxisLength * 0.5f, 2.0f, 0.0f);
	const FVector RayDirAxis = FVector(0.0f, -1.0f, 0.0f);
	EBezierPivotHandle Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find translate X handle"), A->UI_FindPivotHandleFromRay(RayOriginAxis, RayDirAxis, Handle));
	TestEqual(TEXT("Translate X handle"), Handle, EBezierPivotHandle::TranslateX);

	A->UI_SetHoveredPivotHandle(EBezierPivotHandle::TranslateX);
	TestEqual(TEXT("Hovered handle stored"), A->UI_GetHoveredPivotHandle(), EBezierPivotHandle::TranslateX);

	const FVector RayOriginAxisY = Pivot + FVector(2.0f, A->PivotGizmo.AxisLength + A->PivotGizmo.ArrowSize * 0.5f, 0.0f);
	const FVector RayDirAxisY = FVector(-1.0f, 0.0f, 0.0f);
	Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find translate Y handle"), A->UI_FindPivotHandleFromRay(RayOriginAxisY, RayDirAxisY, Handle));
	TestEqual(TEXT("Translate Y handle"), Handle, EBezierPivotHandle::TranslateY);

	const FVector RayOriginAxisZ = Pivot + FVector(2.0f, 0.0f, A->PivotGizmo.AxisLength + A->PivotGizmo.ArrowSize * 0.5f);
	const FVector RayDirAxisZ = FVector(-1.0f, 0.0f, 0.0f);
	Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find translate Z handle"), A->UI_FindPivotHandleFromRay(RayOriginAxisZ, RayDirAxisZ, Handle));
	TestEqual(TEXT("Translate Z handle"), Handle, EBezierPivotHandle::TranslateZ);

	FVector WorldPoint = FVector::ZeroVector;
	TestTrue(TEXT("Get control point world"), A->UI_GetControlPointWorld(0, WorldPoint));
	TestTrue(TEXT("Apply pivot translation"), A->UI_ApplyPivotTranslation(FVector(10.0f, 0.0f, 0.0f)));

	FVector WorldPointAfter = FVector::ZeroVector;
	TestTrue(TEXT("Get control point world after"), A->UI_GetControlPointWorld(0, WorldPointAfter));
	TestTrue(TEXT("Translation applied"), WorldPointAfter.Equals(WorldPoint + FVector(10.0f, 0.0f, 0.0f), 0.01f));

	A->bSelectAllControlPoints = true;
	A->SelectedControlPointIndex = -1;
	TestTrue(TEXT("Pivot world for all selected"), A->UI_GetPivotWorld(Pivot));

	const FVector RayOriginRing = Pivot + FVector(A->PivotGizmo.RotateRadius, 0.0f, 50.0f);
	const FVector RayDirRing = FVector(0.0f, 0.0f, -1.0f);
	Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find rotate Z handle"), A->UI_FindPivotHandleFromRay(RayOriginRing, RayDirRing, Handle));
	TestEqual(TEXT("Rotate Z handle"), Handle, EBezierPivotHandle::RotateZ);

	TestTrue(TEXT("Apply pivot rotation"), A->UI_ApplyPivotRotation(Pivot, FVector::UpVector, PI / 2.0f));

	TArray<FVector> WorldPoints;
	TestTrue(TEXT("Get all control points"), A->UI_GetAllControlPointsWorld(WorldPoints));
	TestEqual(TEXT("Two control points after rotate"), WorldPoints.Num(), 2);
	TestTrue(TEXT("Rotate point 0"), WorldPoints[0].Equals(FVector(1.0f, 1.0f, 0.0f), 0.05f));
	TestTrue(TEXT("Rotate point 1"), WorldPoints[1].Equals(FVector(0.0f, 0.0f, 0.0f), 0.05f));

	A->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_PivotGizmo2D,
	"Bezier/UI/PivotGizmo2D",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_PivotGizmo2D::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A)) return false;

	A->bEnableRuntimeEditing = true;
	A->bEditMode = true;
	A->bActorVisibleInGame = true;
	A->bSnapToGrid = false;
	A->bForcePlanar = false;
	A->Scale = 1.0;

	A->Control = { FVector2D(1, 0), FVector2D(0, 1) };
	A->OverwriteSplineFromControl();

	A->SelectedControlPointIndex = 0;
	A->bSelectAllControlPoints = false;

	FVector Pivot = FVector::ZeroVector;
	TestTrue(TEXT("Pivot world available 2D"), A->UI_GetPivotWorld(Pivot));

	const FVector RayOriginAxis = Pivot + FVector(A->PivotGizmo.AxisLength * 0.5f, 2.0f, 0.0f);
	const FVector RayDirAxis = FVector(0.0f, -1.0f, 0.0f);
	EBezierPivotHandle Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find translate X handle 2D"), A->UI_FindPivotHandleFromRay(RayOriginAxis, RayDirAxis, Handle));
	TestEqual(TEXT("Translate X handle 2D"), Handle, EBezierPivotHandle::TranslateX);

	const FVector RayOriginAxisY = Pivot + FVector(2.0f, A->PivotGizmo.AxisLength + A->PivotGizmo.ArrowSize * 0.5f, 0.0f);
	const FVector RayDirAxisY = FVector(-1.0f, 0.0f, 0.0f);
	Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find translate Y handle 2D"), A->UI_FindPivotHandleFromRay(RayOriginAxisY, RayDirAxisY, Handle));
	TestEqual(TEXT("Translate Y handle 2D"), Handle, EBezierPivotHandle::TranslateY);

	FVector WorldPoint = FVector::ZeroVector;
	TestTrue(TEXT("Get control point world 2D"), A->UI_GetControlPointWorld(0, WorldPoint));
	TestTrue(TEXT("Apply pivot translation 2D"), A->UI_ApplyPivotTranslation(FVector(10.0f, 0.0f, 0.0f)));

	FVector WorldPointAfter = FVector::ZeroVector;
	TestTrue(TEXT("Get control point world after 2D"), A->UI_GetControlPointWorld(0, WorldPointAfter));
	TestTrue(TEXT("Translation applied 2D"), WorldPointAfter.Equals(WorldPoint + FVector(10.0f, 0.0f, 0.0f), 0.01f));

	A->bSelectAllControlPoints = true;
	A->SelectedControlPointIndex = -1;
	TestTrue(TEXT("Pivot world for all selected 2D"), A->UI_GetPivotWorld(Pivot));

	const FVector RayOriginRing = Pivot + FVector(A->PivotGizmo.RotateRadius, 0.0f, 50.0f);
	const FVector RayDirRing = FVector(0.0f, 0.0f, -1.0f);
	Handle = EBezierPivotHandle::None;
	TestTrue(TEXT("Find rotate Z handle 2D"), A->UI_FindPivotHandleFromRay(RayOriginRing, RayDirRing, Handle));
	TestEqual(TEXT("Rotate Z handle 2D"), Handle, EBezierPivotHandle::RotateZ);

	TestTrue(TEXT("Apply pivot rotation 2D"), A->UI_ApplyPivotRotation(Pivot, FVector::UpVector, PI / 2.0f));

	TArray<FVector> WorldPoints;
	TestTrue(TEXT("Get all control points 2D"), A->UI_GetAllControlPointsWorld(WorldPoints));
	TestEqual(TEXT("Two control points after rotate 2D"), WorldPoints.Num(), 2);
	TestTrue(TEXT("Rotate point 0 2D"), WorldPoints[0].Equals(FVector(1.0f, 1.0f, 0.0f), 0.05f));
	TestTrue(TEXT("Rotate point 1 2D"), WorldPoints[1].Equals(FVector(0.0f, 0.0f, 0.0f), 0.05f));

	A->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_Focused_PointOps,
	"Bezier/UI/Focused_PointOps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_Focused_PointOps::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	UBezierEditSubsystem* Subsystem = World ? World->GetSubsystem<UBezierEditSubsystem>() : nullptr;
	if (!TestNotNull(TEXT("Edit subsystem"), Subsystem)) return false;

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABezierCurve2DActor* A2 = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A2)) return false;
	A2->Control = { FVector2D(0,0), FVector2D(1,0), FVector2D(2,0) };
	A2->OverwriteSplineFromControl();
	A2->SelectedControlPointIndex = 0;
	Subsystem->RegisterEditable(A2);
	Subsystem->SetFocused(A2);

	TestTrue(TEXT("Focus duplicate selected 2D"), Subsystem->Focus_DuplicateSelectedControlPoint());
	TestEqual(TEXT("2D duplicate count"), A2->Control.Num(), 4);

	TestTrue(TEXT("Focus add after selected 2D"), Subsystem->Focus_AddControlPointAfterSelected());
	TestEqual(TEXT("2D add after selected count"), A2->Control.Num(), 5);

	TestTrue(TEXT("Focus delete selected 2D"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestEqual(TEXT("2D delete selected count"), A2->Control.Num(), 4);

	ABezierCurveSetActor* SetActor = World->SpawnActor<ABezierCurveSetActor>(P);
	if (!TestNotNull(TEXT("Spawn set actor"), SetActor)) return false;
	TestTrue(TEXT("Set actor add after selected 2D"), SetActor->UI_FocusAddControlPointAfterSelected());
	TestEqual(TEXT("Set actor add count 2D"), A2->Control.Num(), 5);

	Subsystem->All_ToggleSnapToGrid();
	TestTrue(TEXT("All toggle snap on 2D"), A2->bSnapToGrid);
	Subsystem->All_ToggleSnapToGrid();
	TestTrue(TEXT("All toggle snap off 2D"), !A2->bSnapToGrid);

	A2->Destroy();
	SetActor->Destroy();

	ABezierCurve3DActor* A3 = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A3)) return false;
	A3->Control = { FVector(0,0,0), FVector(1,0,0), FVector(2,0,0) };
	A3->OverwriteSplineFromControl();
	A3->SelectedControlPointIndex = 0;
	Subsystem->RegisterEditable(A3);
	Subsystem->SetFocused(A3);

	TestTrue(TEXT("Focus duplicate selected 3D"), Subsystem->Focus_DuplicateSelectedControlPoint());
	TestEqual(TEXT("3D duplicate count"), A3->Control.Num(), 4);

	TestTrue(TEXT("Focus add after selected 3D"), Subsystem->Focus_AddControlPointAfterSelected());
	TestEqual(TEXT("3D add after selected count"), A3->Control.Num(), 5);

	TestTrue(TEXT("Focus delete selected 3D"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestEqual(TEXT("3D delete selected count"), A3->Control.Num(), 4);

	A3->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_CurveSet_IO,
	"Bezier/UI/CurveSet_IO",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_CurveSet_IO::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	const FString OutDir = MakeTempDir_UI(TEXT("BezierUICurveSet"));

	const FString JsonText =
		TEXT("{\"scale\":100.0,\"curves\":[")
		TEXT("{\"name\":\"CurveA\",\"space\":\"3D\",\"closed\":false,\"control\":[[0,0,0],[1,0,0],[2,0,0]]}")
		TEXT("]}");

	const FString CurvesPath = OutDir / TEXT("curves.json");
	if (!FFileHelper::SaveStringToFile(JsonText, *CurvesPath))
	{
		AddError(TEXT("Failed to write curves.json"));
		return false;
	}

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurveSetActor* SetActor = World->SpawnActor<ABezierCurveSetActor>(P);
	if (!TestNotNull(TEXT("Spawn curve set"), SetActor)) return false;

	SetActor->IOPathAbsolute = OutDir;
	SetActor->CurveSetFile = TEXT("curves.json");
	SetActor->BackupCurveSetFile = TEXT("curves_backup.json");
	SetActor->bWriteBackupOnExport = true;

	int32 CountBefore = 0;
	for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It) { ++CountBefore; }

	SetActor->UI_ImportCurveSetJson();

	int32 CountAfter = 0;
	for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It) { ++CountAfter; }
	TestTrue(TEXT("Import spawns 3D curve"), CountAfter > CountBefore);

	SetActor->UI_ExportCurveSetJson();
	TestTrue(TEXT("Export writes curves.json"), FPaths::FileExists(CurvesPath));

	const FString BackupPath = OutDir / TEXT("curves_backup.json");
	TestTrue(TEXT("Backup written"), FPaths::FileExists(BackupPath));

	FString BackupText;
	FFileHelper::LoadFileToString(BackupText, *BackupPath);
	TestTrue(TEXT("Backup matches original"), BackupText == JsonText);

	if (UBezierEditSubsystem* Subsystem = World->GetSubsystem<UBezierEditSubsystem>())
	{
		Subsystem->All_ExportCurveSetJson();
		TestTrue(TEXT("Subsystem export writes curves.json"), FPaths::FileExists(CurvesPath));
	}
	else
	{
		AddError(TEXT("Missing BezierEditSubsystem"));
	}

	SetActor->GridSizeCycleValues = { 5.0f, 10.0f };
	SetActor->UI_CycleGridSizeForAll();
	TestTrue(TEXT("Cycle grid size"), SetActor->GridSizeCycleIndex == 1);

	SetActor->Destroy();
	return true;
}

#endif // WITH_EDITOR
