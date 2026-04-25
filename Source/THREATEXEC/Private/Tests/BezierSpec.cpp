// ============================================================================
// BezierSpec.cpp
// Comprehensive automation tests for THREATEXEC Bezier system.
// Tests math, robustness, JSON shape, sampling, and actor-level integration.
// ============================================================================

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

#include "DeCasteljau.h"
#include "THREATEXEC_FileUtils.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

using namespace TEBezier;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static UWorld* GetTestWorld()
{
	// Editor automation tests run with a special PIE/Editor world.
	// This pulls the current test world safely.
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

static bool NearlyEqual(double A, double B, double eps = 1e-6)
{
	return FMath::Abs(A - B) <= eps;
}

// ---------------------------------------------------------------------------
// 1D EVAL (double-only tests)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_Linear1D,
"Bezier.Math.1D.Linear",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_Linear1D::RunTest(const FString& Params)
{
	TArray<double> C = { 0.0, 10.0 };

	TestTrue("Eval(0)==0", NearlyEqual(DeCasteljauEval(C,0.0),0.0));
	TestTrue("Eval(0.5)==5",NearlyEqual(DeCasteljauEval(C,0.5),5.0));
	TestTrue("Eval(1)==10",NearlyEqual(DeCasteljauEval(C,1.0),10.0));

	return true;
}

// ---------------------------------------------------------------------------
// 2D Tests
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_Linear2D,
"Bezier.Math.2D.Linear",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_Linear2D::RunTest(const FString& Params)
{
	TArray<FVector2D> C = { FVector2D(0,0), FVector2D(1,1) };
	const FVector2D R = DeCasteljauEval(C,0.25);

	TestTrue("x==0.25", NearlyEqual(R.X,0.25));
	TestTrue("y==0.25", NearlyEqual(R.Y,0.25));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_QuadSymmetry2D,
"Bezier.Math.2D.QuadraticSymmetry",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_QuadSymmetry2D::RunTest(const FString& Params)
{
	// symmetric about x-axis → midpoint must lie near (0.5*peak)
	TArray<FVector2D> C = {
		FVector2D(0,0),
		FVector2D(1,10),
		FVector2D(2,0)
	};

	const FVector2D Mid = DeCasteljauEval(C,0.5);
	TestTrue("Mid.x approx 1", NearlyEqual(Mid.X,1.0));
	TestTrue("Mid.y approx 5", NearlyEqual(Mid.Y,5.0));

	return true;
}

// cubic shape test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_CubicLevelsShape,
"Bezier.Math.2D.CubicLevels",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_CubicLevelsShape::RunTest(const FString& Params)
{
	TArray<FVector2D> C = {
		FVector2D(0,0),
		FVector2D(1,2),
		FVector2D(2,2),
		FVector2D(3,0)
	};

	TArray<TArray<FVector2D>> L;
	DeCasteljauLevels(C,0.5,L);

	TestTrue("levels==4", L.Num()==4);
	TestTrue("top is size 1", L.Last().Num()==1);

	const FVector2D Direct = DeCasteljauEval(C,0.5);
	const FVector2D Last = L.Last()[0];

	TestTrue("Level last equals Eval", Last.Equals(Direct,1e-6));

	return true;
}

// ---------------------------------------------------------------------------
// 3D Tests
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_3D_AffineConsistency,
"Bezier.Math.3D.AffineConsistency",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_3D_AffineConsistency::RunTest(const FString& Params)
{
	TArray<FVector> C3 = {
		FVector(0,0,0),
		FVector(1,0,0),
		FVector(1,1,0)
	};

	const FVector R = DeCasteljauEval(C3,0.5);

	// project to XY → should match a 2D version's XY
	TArray<FVector2D> C2 = {
		FVector2D(0,0),
		FVector2D(1,0),
		FVector2D(1,1)
	};

	const FVector2D R2 = DeCasteljauEval(C2,0.5);
	TestTrue("XY matches 2D", NearlyEqual(R.X,R2.X) && NearlyEqual(R.Y,R2.Y));

	return true;
}

// ---------------------------------------------------------------------------
// Degenerate & NaN tests
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_DegenerateSinglePoint,
"Bezier.Robust.Degenerate.SinglePoint",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_DegenerateSinglePoint::RunTest(const FString& Params)
{
	TArray<FVector2D> C = { FVector2D(5,6) };

	for (double t : {0.0,0.3,0.5,1.0})
	{
		const FVector2D R = DeCasteljauEval(C,t);
		TestTrue("constant result", R.Equals(FVector2D(5,6),1e-6));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_DegenerateIdenticalPoints,
"Bezier.Robust.Degenerate.Identical",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_DegenerateIdenticalPoints::RunTest(const FString& Params)
{
	TArray<FVector2D> C = {
		FVector2D(1,1),
		FVector2D(1,1),
		FVector2D(1,1)
	};

	TArray<FVector2D> Arc;
	UniformArcLengthSample(C, 10, Arc);

	TestTrue("ArcLength size >= 2", Arc.Num()>=2);

	for (const FVector2D& P : Arc)
	{
		TestTrue("equal point", P.Equals(FVector2D(1,1),1e-6));
	}

	return true;
}

// ---------------------------------------------------------------------------
// Arc-Length Tests (2D + 3D)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_ArcLengthUniform2D,
"Bezier.Sampling.2D.ArcLength",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_ArcLengthUniform2D::RunTest(const FString& Params)
{
	TArray<FVector2D> C = {
		FVector2D(0,0),
		FVector2D(1,2),
		FVector2D(2,0)
	};

	TArray<FVector2D> Out;
	UniformArcLengthSample(C, 20, Out);

	TestTrue("count==20", Out.Num()==20);
	TestTrue("no NaN", !Out[10].ContainsNaN());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_ArcLengthUniform3D,
"Bezier.Sampling.3D.ArcLength",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_ArcLengthUniform3D::RunTest(const FString& Params)
{
	TArray<FVector> C = {
		FVector(0,0,0),
		FVector(1,0,1),
		FVector(2,0,0)
	};

	TArray<FVector> Out;
	UniformArcLengthSample(C, 30, Out);

	TestTrue("count==30", Out.Num()==30);
	TestTrue("no NaN", !Out[15].ContainsNaN());

	return true;
}

// ---------------------------------------------------------------------------
// JSON Shape Test (2D)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_JSONShape2D,
"Bezier.JSON.Shape2D",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_JSONShape2D::RunTest(const FString& Params)
{
	// minimal JSON shape test
	FString J = TEXT("{\"control\":[[0,1],[2,3]]}");
	TSharedPtr<FJsonObject> Root;

	auto Reader = TJsonReaderFactory<>::Create(J);
	TestTrue("json parse", FJsonSerializer::Deserialize(Reader, Root));

	TestTrue("has control", Root->HasTypedField<EJson::Array>(TEXT("control")));

	return true;
}

// ---------------------------------------------------------------------------
// Integration Tests with Actors
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_Integration_2D_SyncAndExport,
"Bezier.Integration.2D.SyncExport",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_Integration_2D_SyncAndExport::RunTest(const FString& Params)
{
	UWorld* World = GetTestWorld();
	TestNotNull("Have world", World);

	FActorSpawnParameters P;
	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>(P);
	TestNotNull("Spawned 2D actor", A);

	A->Control = { FVector2D(0,0), FVector2D(1,0) };
	A->OverwriteSplineFromControl();
	A->ExportControlPointsToJson();
	A->ExportCurveSamplesToJson();
	A->ExportDeCasteljauProofJson();

	// Confirm files exist in Saved/Bezier
	const FString Dir = TE_PathUtils::ResolveSavedDir(A->IOPathAbsolute,TEXT("Bezier"));
	const FString F1 = Dir / TEXT("control.json");

	TestTrue("control.json exists", FPaths::FileExists(F1));

	A->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_Integration_3D_SyncAndExport,
"Bezier.Integration.3D.SyncExport",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_Integration_3D_SyncAndExport::RunTest(const FString& Params)
{
	UWorld* World = GetTestWorld();
	TestNotNull("Have world", World);

	FActorSpawnParameters P;
	ABezierCurve3DActor* A = World->SpawnActor<ABezierCurve3DActor>(P);
	TestNotNull("Spawned 3D actor", A);

	A->Control = {
		FVector(0,0,0),
		FVector(1,0,0),
		FVector(1,1,1)
	};
	A->OverwriteSplineFromControl();
	A->ExportControlPointsToJson();
	A->ExportCurveSamplesToJson();
	A->ExportDeCasteljauProofJson();

	const FString Dir = TE_PathUtils::ResolveSavedDir(A->IOPathAbsolute,TEXT("Bezier"));
	const FString F1 = Dir / TEXT("control3d.json");

	TestTrue("control3d.json exists", FPaths::FileExists(F1));

	A->Destroy();
	return true;
}

// reverse order identity test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_ReverseOrderIdentity,
"Bezier.Integration.ReverseOrder",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_ReverseOrderIdentity::RunTest(const FString& Params)
{
	UWorld* World = GetTestWorld();
	TestNotNull("Have world", World);

	ABezierCurve2DActor* A = World->SpawnActor<ABezierCurve2DActor>();
	A->Control = {
		FVector2D(0,0),
		FVector2D(1,2),
		FVector2D(2,0)
	};

	const FVector2D Before = A->Eval(0.3);
	A->OverwriteSplineFromControl();
	A->ReverseControlOrder();
	const FVector2D After = A->Eval(0.7); // reversed t → 1-t

	TestTrue("Reverse identity", Before.Equals(After,1e-5));

	A->Destroy();
	return true;
}

// ---------------------------------------------------------------------------
// Performance Smoke Test
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_PerfSmoke1024,
"Bezier.Performance.Smoke.1024Samples",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_PerfSmoke1024::RunTest(const FString& Params)
{
	TArray<FVector> C = {
		FVector(0,0,0),
		FVector(2,1,0),
		FVector(4,0,0)
	};

	TArray<FVector> Out;
	UniformArcLengthSample(C,1024,Out);

	TestTrue("1024 ok", Out.Num()==1024);
	return true;
}

// ---------------------------------------------------------------------------
// Boundary / clamp behavior tests
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_EvalTClamp,
"Bezier.Math.ClampT",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_EvalTClamp::RunTest(const FString& Params)
{
	TArray<FVector2D> C = { FVector2D(0,0), FVector2D(10,0) };

	const FVector2D Neg = DeCasteljauEval(C, -5.0);
	const FVector2D Over = DeCasteljauEval(C, 5.0);

	TestTrue("t<0 clamps to first", Neg.Equals(FVector2D(0,0), 1e-6));
	TestTrue("t>1 clamps to last", Over.Equals(FVector2D(10,0), 1e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_LevelsTClamp,
"Bezier.Math.Levels.ClampT",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_LevelsTClamp::RunTest(const FString& Params)
{
	TArray<FVector2D> C = { FVector2D(1,1), FVector2D(3,1), FVector2D(5,1) };
	TArray<TArray<FVector2D>> LNeg;
	TArray<TArray<FVector2D>> LOver;

	DeCasteljauLevels(C, -1.0, LNeg);
	DeCasteljauLevels(C, 2.0, LOver);

	TestTrue("levels exist for t<0", LNeg.Num() == 3 && LNeg.Last().Num() == 1);
	TestTrue("levels exist for t>1", LOver.Num() == 3 && LOver.Last().Num() == 1);
	TestTrue("t<0 final equals first", LNeg.Last()[0].Equals(C[0], 1e-6));
	TestTrue("t>1 final equals last", LOver.Last()[0].Equals(C.Last(), 1e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_SampleUniform_SingleControl,
"Bezier.Sampling.Param.SingleControl",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_SampleUniform_SingleControl::RunTest(const FString& Params)
{
	TArray<FVector2D> C = { FVector2D(7, 9) };
	TArray<FVector2D> Out;
	SampleUniform(C, 5, Out);

	TestTrue("count==5", Out.Num() == 5);
	for (const FVector2D& P : Out)
	{
		TestTrue("all equal control", P.Equals(C[0], 1e-6));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_ArcLength_ClampAndInvalidInput,
"Bezier.Sampling.ArcLength.ClampAndInvalidInput",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_ArcLength_ClampAndInvalidInput::RunTest(const FString& Params)
{
	TArray<FVector2D> EmptyControl;
	TArray<FVector2D> Out;
	UniformArcLengthSample(EmptyControl, 10, Out);
	TestTrue("empty control -> no output", Out.Num() == 0);

	TArray<FVector2D> C2 = { FVector2D(0,0), FVector2D(10,0) };
	UniformArcLengthSample(C2, 1, Out); // clamps to min 2
	TestTrue("target<2 clamps to 2", Out.Num() == 2);

	UniformArcLengthSample(C2, 5000, Out); // clamps to max 4096
	TestTrue("target>4096 clamps to 4096", Out.Num() == 4096);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBezier_JSON_InvalidShape,
"Bezier.JSON.InvalidShape",
EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBezier_JSON_InvalidShape::RunTest(const FString& Params)
{
	const FString BadJ = TEXT("{\"control\": [ [0,1], }"); // malformed
	TSharedPtr<FJsonObject> Root;
	const auto Reader = TJsonReaderFactory<>::Create(BadJ);
	const bool bParsed = FJsonSerializer::Deserialize(Reader, Root);
	TestTrue("invalid json should fail parse", !bParsed);
	return true;
}

#endif // WITH_EDITOR
