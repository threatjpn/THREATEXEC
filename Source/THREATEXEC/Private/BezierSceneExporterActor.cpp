// ============================================================================
// BezierSceneExporterActor.cpp
// Gathers selected Bézier actors (2D/3D) and writes a single scene JSON.
// Editor-only usage; harmless in game builds (no-op without GEditor).
// ============================================================================

#include "BezierSceneExporterActor.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Components/SplineComponent.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "THREATEXEC_FileUtils.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Selection.h"
#endif

ABezierSceneExporterActor::ABezierSceneExporterActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABezierSceneExporterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

FString ABezierSceneExporterActor::MakeAbs(const FString& FileName) const
{
	// Match 2D/3D actors: IOPathAbsolute resolved under Saved/, with "Bezier" default.
	const FString Dir = TE_PathUtils::ResolveSavedDir(IOPathAbsolute, TEXT("Bezier"));
	return Dir / FileName;
}

bool ABezierSceneExporterActor::WriteStringToFile(const FString& AbsPath, const FString& Str) const
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	const FString Dir = FPaths::GetPath(AbsPath);
	if (!PF.DirectoryExists(*Dir))
	{
		PF.CreateDirectoryTree(*Dir);
	}
	return FFileHelper::SaveStringToFile(Str, *AbsPath);
}

void ABezierSceneExporterActor::ExportSelectedToJson()
{
#if WITH_EDITOR
	if (!GEditor) return;

	// Scene root JSON
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> JActors;

	// Iterate selected actors
	USelection* Sel = GEditor->GetSelectedActors();
	if (!Sel) return;

	for (FSelectionIterator It(*Sel); It; ++It)
	{
		AActor* A = Cast<AActor>(*It);
		if (!A) continue;

		// 2D
		if (auto* C2D = Cast<ABezierCurve2DActor>(A))
		{
			// Ensure latest control state
			C2D->SyncControlFromSpline();

			TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
			J->SetStringField(TEXT("name"), C2D->GetName());
			J->SetStringField(TEXT("class"), TEXT("ABezierCurve2DActor"));
			J->SetNumberField(TEXT("scale"), C2D->Scale);

			// Transform (actor)
			const FTransform Xf = C2D->GetActorTransform();
			const FVector   Loc = Xf.GetLocation();
			const FRotator  Rot = Xf.Rotator();
			const FVector   Scl = Xf.GetScale3D();

			TSharedRef<FJsonObject> JT = MakeShared<FJsonObject>();
			{
				TArray<TSharedPtr<FJsonValue>> L; L.Add(MakeShared<FJsonValueNumber>(Loc.X)); L.Add(MakeShared<FJsonValueNumber>(Loc.Y)); L.Add(MakeShared<FJsonValueNumber>(Loc.Z));
				TArray<TSharedPtr<FJsonValue>> R; R.Add(MakeShared<FJsonValueNumber>(Rot.Pitch)); R.Add(MakeShared<FJsonValueNumber>(Rot.Yaw)); R.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
				TArray<TSharedPtr<FJsonValue>> S; S.Add(MakeShared<FJsonValueNumber>(Scl.X)); S.Add(MakeShared<FJsonValueNumber>(Scl.Y)); S.Add(MakeShared<FJsonValueNumber>(Scl.Z));
				JT->SetArrayField(TEXT("location"), L);
				JT->SetArrayField(TEXT("rotation"), R);
				JT->SetArrayField(TEXT("scale"), S);
			}
			J->SetObjectField(TEXT("transform"), JT);

			// Closed?
			bool bClosed = false;
			if (auto* Spline = C2D->FindComponentByClass<USplineComponent>())
			{
				bClosed = Spline->IsClosedLoop();
			}
			J->SetBoolField(TEXT("closed"), bClosed);

			// Control (2D)
			{
				TArray<TSharedPtr<FJsonValue>> Arr;
				for (const FVector2D& P : C2D->Control)
				{
					TArray<TSharedPtr<FJsonValue>> XY;
					XY.Add(MakeShared<FJsonValueNumber>(P.X));
					XY.Add(MakeShared<FJsonValueNumber>(P.Y));
					Arr.Add(MakeShared<FJsonValueArray>(XY));
				}
				J->SetArrayField(TEXT("control"), Arr);
			}

			// Samples (2D)
			{
				TArray<TSharedPtr<FJsonValue>> Arr;
				const int32 N = FMath::Max(ExportSamples, 2);
				for (int32 i = 0; i < N; ++i)
				{
					const double    t = (N == 1) ? 0.0 : (double)i / (double)(N - 1);
					const FVector2D P = C2D->Eval(t);
					TArray<TSharedPtr<FJsonValue>> XY;
					XY.Add(MakeShared<FJsonValueNumber>(P.X));
					XY.Add(MakeShared<FJsonValueNumber>(P.Y));
					Arr.Add(MakeShared<FJsonValueArray>(XY));
				}
				J->SetArrayField(TEXT("samples"), Arr);
			}

			JActors.Add(MakeShared<FJsonValueObject>(J));
		}

		// 3D
		else if (auto* C3D = Cast<ABezierCurve3DActor>(A))
		{
			C3D->SyncControlFromSpline();

			TSharedRef<FJsonObject> J = MakeShared<FJsonObject>();
			J->SetStringField(TEXT("name"), C3D->GetName());
			J->SetStringField(TEXT("class"), TEXT("ABezierCurve3DActor"));
			J->SetNumberField(TEXT("scale"), C3D->Scale);

			const FTransform Xf = C3D->GetActorTransform();
			const FVector   Loc = Xf.GetLocation();
			const FRotator  Rot = Xf.Rotator();
			const FVector   Scl = Xf.GetScale3D();

			TSharedRef<FJsonObject> JT = MakeShared<FJsonObject>();
			{
				TArray<TSharedPtr<FJsonValue>> L; L.Add(MakeShared<FJsonValueNumber>(Loc.X)); L.Add(MakeShared<FJsonValueNumber>(Loc.Y)); L.Add(MakeShared<FJsonValueNumber>(Loc.Z));
				TArray<TSharedPtr<FJsonValue>> R; R.Add(MakeShared<FJsonValueNumber>(Rot.Pitch)); R.Add(MakeShared<FJsonValueNumber>(Rot.Yaw)); R.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
				TArray<TSharedPtr<FJsonValue>> S; S.Add(MakeShared<FJsonValueNumber>(Scl.X)); S.Add(MakeShared<FJsonValueNumber>(Scl.Y)); S.Add(MakeShared<FJsonValueNumber>(Scl.Z));
				JT->SetArrayField(TEXT("location"), L);
				JT->SetArrayField(TEXT("rotation"), R);
				JT->SetArrayField(TEXT("scale"), S);
			}
			J->SetObjectField(TEXT("transform"), JT);

			bool bClosed = false;
			if (auto* Spline = C3D->FindComponentByClass<USplineComponent>())
			{
				bClosed = Spline->IsClosedLoop();
			}
			J->SetBoolField(TEXT("closed"), bClosed);

			// Control (3D)
			{
				TArray<TSharedPtr<FJsonValue>> Arr;
				for (const FVector& P : C3D->Control)
				{
					TArray<TSharedPtr<FJsonValue>> XYZ;
					XYZ.Add(MakeShared<FJsonValueNumber>(P.X));
					XYZ.Add(MakeShared<FJsonValueNumber>(P.Y));
					XYZ.Add(MakeShared<FJsonValueNumber>(P.Z));
					Arr.Add(MakeShared<FJsonValueArray>(XYZ));
				}
				J->SetArrayField(TEXT("control"), Arr);
			}

			// Samples (3D)
			{
				TArray<TSharedPtr<FJsonValue>> Arr;
				const int32 N = FMath::Max(ExportSamples, 2);
				for (int32 i = 0; i < N; ++i)
				{
					const double  t = (N == 1) ? 0.0 : (double)i / (double)(N - 1);
					const FVector P = C3D->Eval(t);
					TArray<TSharedPtr<FJsonValue>> XYZ;
					XYZ.Add(MakeShared<FJsonValueNumber>(P.X));
					XYZ.Add(MakeShared<FJsonValueNumber>(P.Y));
					XYZ.Add(MakeShared<FJsonValueNumber>(P.Z));
					Arr.Add(MakeShared<FJsonValueArray>(XYZ));
				}
				J->SetArrayField(TEXT("samples"), Arr);
			}

			JActors.Add(MakeShared<FJsonValueObject>(J));
		}
	}

	Root->SetArrayField(TEXT("actors"), JActors);

	// Write file
	FString Out;
	auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);

	WriteStringToFile(MakeAbs(TEXT("scene_export.json")), Out);
#endif // WITH_EDITOR
}
