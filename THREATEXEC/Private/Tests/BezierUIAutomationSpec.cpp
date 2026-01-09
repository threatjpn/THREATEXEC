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
