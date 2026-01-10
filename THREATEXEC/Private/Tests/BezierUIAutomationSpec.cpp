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

namespace
{
	static UWorld* GetEditorWorldChecked()
	{
		check(GEditor);
		return GEditor->GetEditorWorldContext().World();
	}

	static FString MakeTempDir(const FString& Name)
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
	UWorld* World = GetEditorWorldChecked();
	const FString OutDir = MakeTempDir(TEXT("BezierUI2D"));

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

	A->UI_SetLockToLocalXY(false);
	TestTrue(TEXT("UI_SetLockToLocalXY"), !A->bLockToLocalXY);

	A->UI_SetForcePlanar(false);
	TestTrue(TEXT("UI_SetForcePlanar"), !A->bForcePlanar);

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
	UWorld* World = GetEditorWorldChecked();
	const FString OutDir = MakeTempDir(TEXT("BezierUI3D"));

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

	A->UI_SetForcePlanar(true);
	TestTrue(TEXT("UI_SetForcePlanar"), A->bForcePlanar);

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

#endif // WITH_EDITOR
