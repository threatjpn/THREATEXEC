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

	A->Control = { FVector(1,2,3), FVector(3,4,5), FVector(5,6,7) };
	A->OverwriteSplineFromControl();
	A->UI_MirrorCurveX();
	TestTrue(TEXT("Mirror X keeps centroid"), A->Control[1].Equals(FVector(3,4,5), 1e-6));
	A->UI_MirrorCurveY();
	TestTrue(TEXT("Mirror Y keeps centroid"), A->Control[1].Equals(FVector(3,4,5), 1e-6));
	A->UI_MirrorCurveZ();
	TestTrue(TEXT("Mirror Z keeps centroid"), A->Control[1].Equals(FVector(3,4,5), 1e-6));

	A->Control = { FVector(0,0,1), FVector(2,3,4), FVector(5,8,13) };
	A->OverwriteSplineFromControl();
	A->UI_SetForcePlanar(false);
	A->UI_SetForcePlanarAxis(EBezierPlanarAxis::XY);
	const EBezierPlanarAxis Cycle1 = A->UI_CycleForcePlanarAxis();
	const EBezierPlanarAxis Cycle2 = A->UI_CycleForcePlanarAxis();
	const EBezierPlanarAxis Cycle3 = A->UI_CycleForcePlanarAxis();
	TestTrue(TEXT("Force planar cycle includes None"), Cycle1 == EBezierPlanarAxis::XZ && Cycle2 == EBezierPlanarAxis::YZ && Cycle3 == EBezierPlanarAxis::None);
	TestTrue(TEXT("Force planar none restores original control"),
		A->Control.Num() == 3 &&
		A->Control[0].Equals(FVector(0,0,1), 1e-6) &&
		A->Control[1].Equals(FVector(2,3,4), 1e-6) &&
		A->Control[2].Equals(FVector(5,8,13), 1e-6));

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
	A2->UI_SetEditMode(true);
	A2->UI_SelectControlPoint(0);
	Subsystem->RegisterEditable(A2);
	Subsystem->SetFocused(A2);

	TestTrue(TEXT("Focus duplicate selected 2D"), Subsystem->Focus_DuplicateSelectedControlPoint());
	TestEqual(TEXT("2D duplicate count"), A2->Control.Num(), 4);

	TestTrue(TEXT("Focus add after selected 2D"), Subsystem->Focus_AddControlPointAfterSelected());
	TestEqual(TEXT("2D add after selected count"), A2->Control.Num(), 5);

	TestTrue(TEXT("Focus delete selected 2D"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestEqual(TEXT("2D delete selected count"), A2->Control.Num(), 4);

	A2->UI_SelectAllControlPoints();
	TestTrue(TEXT("Focus delete selected all 2D destroys actor"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestTrue(TEXT("2D actor destroyed when all selected"), !IsValid(A2));

	ABezierCurveSetActor* SetActor = World->SpawnActor<ABezierCurveSetActor>(P);
	if (!TestNotNull(TEXT("Spawn set actor"), SetActor)) return false;
	ABezierCurve2DActor* A2b = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Respawn 2D"), A2b)) return false;
	A2b->Control = { FVector2D(0,0), FVector2D(1,0), FVector2D(2,0) };
	A2b->OverwriteSplineFromControl();
	A2b->UI_SetEditMode(true);
	A2b->UI_SelectControlPoint(0);
	Subsystem->RegisterEditable(A2b);
	Subsystem->SetFocused(A2b);
	TestTrue(TEXT("Set actor add after selected 2D"), SetActor->UI_FocusAddControlPointAfterSelected());
	TestEqual(TEXT("Set actor add count 2D"), A2b->Control.Num(), 4);

	Subsystem->All_ToggleSnapToGrid();
	TestTrue(TEXT("All toggle snap on 2D"), A2b->bSnapToGrid);
	Subsystem->All_ToggleSnapToGrid();
	TestTrue(TEXT("All toggle snap off 2D"), !A2b->bSnapToGrid);

	A2b->Destroy();
	SetActor->Destroy();

	ABezierCurve3DActor* A3 = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A3)) return false;
	A3->Control = { FVector(0,0,0), FVector(1,0,0), FVector(2,0,0) };
	A3->OverwriteSplineFromControl();
	A3->UI_SetEditMode(true);
	A3->UI_SelectControlPoint(0);
	Subsystem->RegisterEditable(A3);
	Subsystem->SetFocused(A3);

	TestTrue(TEXT("Focus duplicate selected 3D"), Subsystem->Focus_DuplicateSelectedControlPoint());
	TestEqual(TEXT("3D duplicate count"), A3->Control.Num(), 4);

	TestTrue(TEXT("Focus add after selected 3D"), Subsystem->Focus_AddControlPointAfterSelected());
	TestEqual(TEXT("3D add after selected count"), A3->Control.Num(), 5);

	TestTrue(TEXT("Focus delete selected 3D"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestEqual(TEXT("3D delete selected count"), A3->Control.Num(), 4);

	A3->UI_SelectAllControlPoints();
	TestTrue(TEXT("Focus delete selected all 3D destroys actor"), Subsystem->Focus_DeleteSelectedControlPoint());
	TestTrue(TEXT("3D actor destroyed when all selected"), !IsValid(A3));
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

	SetActor->UI_ClearSpawned();
	SetActor->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_CurveSet_FileMenuOps,
	"Bezier/UI/CurveSet_FileMenuOps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_CurveSet_FileMenuOps::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	const FString OutDir = MakeTempDir_UI(TEXT("BezierUICurveSetMenu"));

	const FString JsonA = TEXT("{\"scale\":100.0,\"curves\":[{\"name\":\"CurveA\",\"space\":\"2D\",\"closed\":false,\"control\":[[0,0],[1,0],[2,0]]}]}");
	const FString JsonB = TEXT("{\"scale\":100.0,\"curves\":[{\"name\":\"CurveB\",\"space\":\"2D\",\"closed\":false,\"control\":[[0,0],[0,1],[0,2]]}]}");

	const FString APath = OutDir / TEXT("a_curves.json");
	const FString BPath = OutDir / TEXT("b_curves.json");
	if (!FFileHelper::SaveStringToFile(JsonA, *APath) || !FFileHelper::SaveStringToFile(JsonB, *BPath))
	{
		AddError(TEXT("Failed to seed menu JSON files"));
		return false;
	}

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurveSetActor* SetActor = World->SpawnActor<ABezierCurveSetActor>(P);
	if (!TestNotNull(TEXT("Spawn curve set"), SetActor)) return false;

	SetActor->IOPathAbsolute = OutDir;

	// List files and ensure both seeded files are visible.
	TArray<FString> Listed = SetActor->UI_ListCurveSetJsonFiles(true);
	TestTrue(TEXT("List includes a_curves.json"), Listed.Contains(TEXT("a_curves.json")));
	TestTrue(TEXT("List includes b_curves.json"), Listed.Contains(TEXT("b_curves.json")));

	TArray<FBezierCurveSetFileListRowData> MenuRows;
	SetActor->UI_FileMenuListCurveSetJsonFiles(MenuRows);
	TestTrue(TEXT("FileMenu rows include seeded files"), MenuRows.Num() >= 2);

	// Load one via file-menu API.
	TestTrue(TEXT("Load by file menu name"), SetActor->UI_FileMenuLoadCurveSetJsonByFileName(TEXT("a_curves.json")));
	TestTrue(TEXT("Has any curves after load"), SetActor->UI_HasAnyCurves());
	TestTrue(TEXT("Curve count positive after load"), SetActor->UI_GetCurveCount() > 0);

	// Save under a new name and then delete it.
	const FString SavedName = TEXT("saved_from_menu.json");
	TestTrue(TEXT("Save by file menu name"), SetActor->UI_FileMenuSaveCurveSetJsonByFileName(SavedName, false));
	TestTrue(TEXT("Saved file exists"), FPaths::FileExists(OutDir / SavedName));

	TestTrue(TEXT("Delete by filename"), SetActor->UI_DeleteCurveSetJsonByFileName(SavedName));
	TestTrue(TEXT("Deleted file removed"), !FPaths::FileExists(OutDir / SavedName));

	SetActor->UI_ClearSpawned();
	SetActor->Destroy();
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_UI_SelectionAndEditOps_2D3D,
	"Bezier/UI/SelectionAndEditOps_2D3D",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_UI_SelectionAndEditOps_2D3D::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked_UI();
	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABezierCurve2DActor* A2 = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A2)) return false;

	A2->UI_SetEditMode(true);
	A2->Control = { FVector2D(0,0), FVector2D(1,0), FVector2D(2,0) };
	A2->OverwriteSplineFromControl();
	A2->UI_SelectAllControlPoints();
	TestTrue(TEXT("2D select-all active"), A2->UI_AreAllControlPointsSelected());

	TArray<FVector> All2D;
	TestTrue(TEXT("2D get all world points"), A2->UI_GetAllControlPointsWorld(All2D));
	TArray<FVector> Shifted2D = All2D;
	for (FVector& P2 : Shifted2D)
	{
		P2 += FVector(25.0f, -10.0f, 0.0f);
	}
	TestTrue(TEXT("2D set all world points"), A2->UI_SetAllControlPointsWorld(Shifted2D));

	TArray<FVector> AfterShift2D;
	TestTrue(TEXT("2D verify shifted points"), A2->UI_GetAllControlPointsWorld(AfterShift2D) && AfterShift2D.Num() == Shifted2D.Num());
	if (AfterShift2D.Num() == Shifted2D.Num())
	{
		for (int32 i = 0; i < AfterShift2D.Num(); ++i)
		{
			TestTrue(FString::Printf(TEXT("2D point %d shifted"), i), AfterShift2D[i].Equals(Shifted2D[i], 0.1f));
		}
	}

	const int32 Num2DBeforeAdd = A2->Control.Num();
	TestTrue(TEXT("2D add control point after selected/select-all"), A2->UI_AddControlPointAfterSelected());
	TestTrue(TEXT("2D add increased count"), A2->Control.Num() == Num2DBeforeAdd + 1);

	A2->UI_SelectControlPoint(1);
	const int32 Num2DBeforeDup = A2->Control.Num();
	TestTrue(TEXT("2D duplicate selected"), A2->UI_DuplicateSelectedControlPoint());
	TestTrue(TEXT("2D duplicate increased count"), A2->Control.Num() == Num2DBeforeDup + 1);

	A2->UI_SelectControlPoint(0);
	const int32 Num2DBeforeDelete = A2->Control.Num();
	TestTrue(TEXT("2D delete selected"), A2->UI_DeleteSelectedControlPoint());
	TestTrue(TEXT("2D delete reduced count"), A2->Control.Num() == Num2DBeforeDelete - 1);

	ABezierCurve3DActor* A3 = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A3)) return false;

	A3->UI_SetEditMode(true);
	A3->Control = { FVector(0,0,0), FVector(1,0,0), FVector(2,0,0) };
	A3->OverwriteSplineFromControl();
	A3->UI_SelectAllControlPoints();
	TestTrue(TEXT("3D select-all active"), A3->UI_AreAllControlPointsSelected());

	TArray<FVector> All3D;
	TestTrue(TEXT("3D get all world points"), A3->UI_GetAllControlPointsWorld(All3D));
	TArray<FVector> Shifted3D = All3D;
	for (FVector& P3 : Shifted3D)
	{
		P3 += FVector(-15.0f, 20.0f, 5.0f);
	}
	TestTrue(TEXT("3D set all world points"), A3->UI_SetAllControlPointsWorld(Shifted3D));

	const int32 Num3DBeforeAdd = A3->Control.Num();
	TestTrue(TEXT("3D add control point after selected/select-all"), A3->UI_AddControlPointAfterSelected());
	TestTrue(TEXT("3D add increased count"), A3->Control.Num() == Num3DBeforeAdd + 1);

	A3->UI_SelectControlPoint(1);
	const int32 Num3DBeforeDup = A3->Control.Num();
	TestTrue(TEXT("3D duplicate selected"), A3->UI_DuplicateSelectedControlPoint());
	TestTrue(TEXT("3D duplicate increased count"), A3->Control.Num() == Num3DBeforeDup + 1);

	A3->UI_SelectControlPoint(0);
	const int32 Num3DBeforeDelete = A3->Control.Num();
	TestTrue(TEXT("3D delete selected"), A3->UI_DeleteSelectedControlPoint());
	TestTrue(TEXT("3D delete reduced count"), A3->Control.Num() == Num3DBeforeDelete - 1);

	A3->Destroy();
	A2->Destroy();
	return true;
}


#endif // WITH_EDITOR
