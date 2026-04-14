// ============================================================================
// BezierIntegrationSpec.cpp
// Editor integration tests: spawn actors, set spline, sync, resample, export.
// Avoids touching private members; validates via spline state and JSON files.
// ============================================================================

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_EDITOR

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"

// Public actor headers
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierSceneExporterActor.h"

// ---------- helpers ----------------------------------------------------------
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

	static void SetLocalSplinePoints(USplineComponent* Spline, const TArray<FVector>& Points, bool bClosed = false)
	{
		check(Spline);
		Spline->ClearSplinePoints(false);
		for (const FVector& P : Points)
			Spline->AddSplinePoint(P, ESplineCoordinateSpace::Local, false);
		Spline->SetClosedLoop(bClosed, false);
		Spline->UpdateSpline();
	}

	static bool LoadText(const FString& AbsFile, FString& Out) { return FPaths::FileExists(AbsFile) && FFileHelper::LoadFileToString(Out, *AbsFile); }
	static bool JsonHasKey(const FString& Json, const TCHAR* Key) { return Json.Contains(FString::Printf(TEXT("\"%s\""), Key)); }

	// Find the first JSON file in Dir that contains "key" as a JSON key.
	static bool FindJsonWithKey(const FString& Dir, const TCHAR* Key, FString& OutJsonPath)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);
		for (const FString& Base : Files)
		{
			const FString Path = Dir / Base;
			FString J;
			if (LoadText(Path, J) && JsonHasKey(J, Key))
			{
				OutJsonPath = Path;
				return true;
			}
		}
		return false;
	}
}

// ---------- tests ------------------------------------------------------------

// 1) 2D actor: sync & export (validate files/keys)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_IT_2D_SyncAndExport,
	"Bezier/Integration/2D_SyncAndExport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_IT_2D_SyncAndExport::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked();
	const FString OutDir = MakeTempDir(TEXT("Bezier2D"));

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A)) return false;

	USplineComponent* Spline = A->FindComponentByClass<USplineComponent>();
	if (!TestNotNull(TEXT("Spline"), Spline)) return false;

	SetLocalSplinePoints(Spline, {
		FVector(-300,   0, 0),
		FVector(-100,  50, 0),
		FVector(0, 120, 0),
		FVector(150,  60, 0),
		FVector(300,   0, 0),
		FVector(400, -20, 0)
		});

	A->SyncControlFromSpline();
	A->IOPathAbsolute = OutDir;
	A->ExportControlPointsToJson();
	A->ExportCurveSamplesToJson();
	A->ExportDeCasteljauProofJson();

	FString Path;

	TestTrue(TEXT("found a control JSON (has 'control')"),
		FindJsonWithKey(OutDir, TEXT("control"), Path));

	TestTrue(TEXT("found a samples JSON (has 'samples')"),
		FindJsonWithKey(OutDir, TEXT("samples"), Path));

	TestTrue(TEXT("found a proof JSON (has 'levels')"),
		FindJsonWithKey(OutDir, TEXT("levels"), Path));

	A->Destroy();
	return true;
}

// 2) 3D actor: non-planar, sync & export (validate files/keys)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_IT_3D_SyncAndExport,
	"Bezier/Integration/3D_SyncAndExport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_IT_3D_SyncAndExport::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked();
	const FString OutDir = MakeTempDir(TEXT("Bezier3D"));

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve3DActor* A = World->SpawnActor<ABezierCurve3DActor>(P);
	if (!TestNotNull(TEXT("Spawn 3D"), A)) return false;

	USplineComponent* Spline = A->FindComponentByClass<USplineComponent>();
	if (!TestNotNull(TEXT("Spline"), Spline)) return false;

	SetLocalSplinePoints(Spline, {
		FVector(-300,   0,   0),
		FVector(-100,  70,  40),
		FVector(50, 120,  90),
		FVector(200,  40,  20),
		FVector(380,   0, 110)
		});

	A->SyncControlFromSpline();
	A->IOPathAbsolute = OutDir;
	A->ExportControlPointsToJson();
	A->ExportCurveSamplesToJson();
	A->ExportDeCasteljauProofJson();

	FString Path;

	TestTrue(TEXT("found a control JSON (has 'control')"),
		FindJsonWithKey(OutDir, TEXT("control"), Path));

	TestTrue(TEXT("found a samples JSON (has 'samples')"),
		FindJsonWithKey(OutDir, TEXT("samples"), Path));

	TestTrue(TEXT("found a proof JSON (has 'levels')"),
		FindJsonWithKey(OutDir, TEXT("levels"), Path));

	A->Destroy();
	return true;
}

// 3) Resample-to-spline: count>2 and endpoints preserved (local space)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_IT_ResampleButtons,
	"Bezier/Integration/ResampleButtons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_IT_ResampleButtons::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked();

	FActorSpawnParameters P; P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>(P);
	if (!TestNotNull(TEXT("Spawn 2D"), A)) return false;

	USplineComponent* Spline = A->FindComponentByClass<USplineComponent>();
	if (!TestNotNull(TEXT("Spline"), Spline)) return false;

	SetLocalSplinePoints(Spline, {
		FVector(-200,  0, 0),
		FVector(0, 80, 0),
		FVector(150, 60, 0),
		FVector(300,  0, 0)
		});

	A->SyncControlFromSpline();

	// Remember endpoints before resample
	const FVector FirstBefore = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
	const FVector LastBefore = Spline->GetLocationAtSplinePoint(Spline->GetNumberOfSplinePoints() - 1, ESplineCoordinateSpace::Local);

	A->ResampleBezierToSpline();

	const int32 N = Spline->GetNumberOfSplinePoints();
	TestTrue(TEXT("resampled count > 2"), N > 2);

	const FVector FirstAfter = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
	const FVector LastAfter = Spline->GetLocationAtSplinePoint(N - 1, ESplineCoordinateSpace::Local);

	// Endpoints should be preserved (within viewport-scale tolerance)
	TestTrue(TEXT("first preserved"), FirstAfter.Equals(FirstBefore, 0.5f));
	TestTrue(TEXT("last preserved"), LastAfter.Equals(LastBefore, 0.5f));

	A->Destroy();
	return true;
}

// 4) Scene exporter: gather selected curves into a single JSON
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_IT_SceneExporter,
	"Bezier/Integration/SceneExporter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FBezier_IT_SceneExporter::RunTest(const FString&)
{
	UWorld* World = GetEditorWorldChecked();
	const FString OutDir = MakeTempDir(TEXT("BezierScene"));

	ABezierCurve2DActor* A2D = World->SpawnActor<ABezierCurve2DActor>();
	ABezierCurve3DActor* A3D = World->SpawnActor<ABezierCurve3DActor>();
	if (!TestNotNull(TEXT("Spawn 2D"), A2D) || !TestNotNull(TEXT("Spawn 3D"), A3D)) return false;

	USplineComponent* S2 = A2D->FindComponentByClass<USplineComponent>();
	USplineComponent* S3 = A3D->FindComponentByClass<USplineComponent>();
	if (!TestNotNull(TEXT("S2"), S2) || !TestNotNull(TEXT("S3"), S3)) return false;

	SetLocalSplinePoints(S2, { FVector(-100,0,0), FVector(0,50,0), FVector(100,0,0) });
	SetLocalSplinePoints(S3, { FVector(-80,0,10), FVector(0,40,30), FVector(90,0,5) });

	A2D->SyncControlFromSpline();
	A3D->SyncControlFromSpline();

	ABezierSceneExporterActor* Exporter = World->SpawnActor<ABezierSceneExporterActor>();
	if (!TestNotNull(TEXT("Spawn Exporter"), Exporter)) return false;
	Exporter->IOPathAbsolute = OutDir;

	// Select and export
	GEditor->SelectNone(false, true, false);
	Exporter->ExportSelectedToJson();

	// Any JSON with 'actors' is accepted (filename may vary)
	FString ScenePath;
	TestTrue(TEXT("found scene export JSON (has 'actors')"),
		FindJsonWithKey(OutDir, TEXT("actors"), ScenePath));

	Exporter->Destroy();
	A2D->Destroy();
	A3D->Destroy();
	return true;
}

#endif // WITH_EDITOR
