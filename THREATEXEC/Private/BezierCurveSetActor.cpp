#include "BezierCurveSetActor.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Components/SplineComponent.h"
#include "THREATEXEC_FileUtils.h"
#include "TimerManager.h"

ABezierCurveSetActor::ABezierCurveSetActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABezierCurveSetActor::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableAutoSave && AutoSaveIntervalSeconds > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			AutoSaveTimerHandle,
			this,
			&ABezierCurveSetActor::HandleAutoSave,
			AutoSaveIntervalSeconds,
			true
		);
	}
}

void ABezierCurveSetActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutoSaveTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ABezierCurveSetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Curve2DClass) Curve2DClass = ABezierCurve2DActor::StaticClass();
	if (!Curve3DClass) Curve3DClass = ABezierCurve3DActor::StaticClass();
}

FString ABezierCurveSetActor::MakeAbs(const FString& FileName) const
{
	const FString Base = FPaths::IsRelative(IOPathAbsolute) ? (FPaths::ProjectDir() / IOPathAbsolute) : IOPathAbsolute;
	return FPaths::ConvertRelativePathToFull(Base / FileName);
}

bool ABezierCurveSetActor::ReadText(const FString& AbsPath, FString& Out) const
{
	return FFileHelper::LoadFileToString(Out, *AbsPath);
}

bool ABezierCurveSetActor::WriteText(const FString& AbsPath, const FString& In) const
{
	return FFileHelper::SaveStringToFile(In, *AbsPath);
}

bool ABezierCurveSetActor::IsAnyEditModeActive() const
{
	for (AActor* A : Spawned)
	{
		if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			if (A3->UI_GetEditMode()) return true;
		}
		else if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			if (A2->UI_GetEditMode()) return true;
		}
	}
	return false;
}

void ABezierCurveSetActor::HandleAutoSave()
{
	if (bAutoSaveOnlyWhenEditing && !IsAnyEditModeActive()) return;
	UI_ExportCurveSetJson();
}

void ABezierCurveSetActor::ClearSpawned()
{
	for (AActor* A : Spawned)
	{
		if (IsValid(A)) A->Destroy();
	}
	Spawned.Reset();
}

void ABezierCurveSetActor::ImportCurveSetJson()
{
	ClearSpawned();

	FString JsonText;
	const FString AbsPath = MakeAbs(CurveSetFile);
	if (!ReadText(AbsPath, JsonText))
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to read %s"), *AbsPath);
		return;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to parse %s"), *AbsPath);
		return;
	}

	double FileScale = 100.0;
	Root->TryGetNumberField(TEXT("scale"), FileScale);
	const TArray<TSharedPtr<FJsonValue>>* CurvesArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("curves"), CurvesArray) || !CurvesArray)
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: No curves array in %s"), *AbsPath);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	if (!Curve2DClass) Curve2DClass = ABezierCurve2DActor::StaticClass();
	if (!Curve3DClass) Curve3DClass = ABezierCurve3DActor::StaticClass();

	for (const TSharedPtr<FJsonValue>& CurveValue : *CurvesArray)
	{
		const TSharedPtr<FJsonObject>* CurveObj = nullptr;
		if (!CurveValue.IsValid() || !CurveValue->TryGetObject(CurveObj) || !CurveObj || !CurveObj->IsValid()) continue;

		FString CurveName;
		(*CurveObj)->TryGetStringField(TEXT("name"), CurveName);
		FString Space;
		(*CurveObj)->TryGetStringField(TEXT("space"), Space);
		bool bClosed = false;
		(*CurveObj)->TryGetBoolField(TEXT("closed"), bClosed);

		const TArray<TSharedPtr<FJsonValue>>* ControlArray = nullptr;
		if (!(*CurveObj)->TryGetArrayField(TEXT("control"), ControlArray) || !ControlArray) continue;

		const bool bIs3D = Space.Equals(TEXT("3D"), ESearchCase::IgnoreCase);
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (!CurveName.IsEmpty())
		{
			Params.Name = MakeUniqueObjectName(World, bIs3D ? Curve3DClass : Curve2DClass, FName(*CurveName));
		}

		if (bIs3D)
		{
			ABezierCurve3DActor* A3 = World->SpawnActor<ABezierCurve3DActor>(Curve3DClass, GetActorTransform(), Params);
			if (!A3) continue;

			A3->Scale = FileScale;
			A3->Control.Reset();
			for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
			{
				const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 3) continue;
				const double X = (*PointArray)[0]->AsNumber();
				const double Y = (*PointArray)[1]->AsNumber();
				const double Z = (*PointArray)[2]->AsNumber();
				A3->Control.Add(FVector(X, Y, Z));
			}
			A3->UI_SetInitialControlFromCurrent();
			A3->OverwriteSplineFromControl();
			A3->UI_SetClosedLoop(bClosed);
			Spawned.Add(A3);
		}
		else
		{
			ABezierCurve2DActor* A2 = World->SpawnActor<ABezierCurve2DActor>(Curve2DClass, GetActorTransform(), Params);
			if (!A2) continue;

			A2->Scale = FileScale;
			A2->Control.Reset();
			for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
			{
				const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 2) continue;
				const double X = (*PointArray)[0]->AsNumber();
				const double Y = (*PointArray)[1]->AsNumber();
				A2->Control.Add(FVector2D(X, Y));
			}
			A2->UI_SetInitialControlFromCurrent();
			A2->OverwriteSplineFromControl();
			A2->UI_SetClosedLoop(bClosed);
			Spawned.Add(A2);
		}
	}
}

void ABezierCurveSetActor::UI_ImportCurveSetJson()
{
	ImportCurveSetJson();
}

void ABezierCurveSetActor::UI_ExportCurveSetJson()
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Curves;

	double ScaleValue = 100.0;
	bool bScaleSet = false;

	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			if (!bScaleSet) { ScaleValue = A3->Scale; bScaleSet = true; }
			TSharedRef<FJsonObject> CurveObj = MakeShared<FJsonObject>();
			CurveObj->SetStringField(TEXT("name"), A3->GetName());
			CurveObj->SetStringField(TEXT("space"), TEXT("3D"));
			CurveObj->SetBoolField(TEXT("closed"), A3->UI_IsClosedLoop());

			TArray<TSharedPtr<FJsonValue>> ControlValues;
			for (const FVector& P : A3->Control)
			{
				TArray<TSharedPtr<FJsonValue>> PointValues;
				PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
				PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
				PointValues.Add(MakeShared<FJsonValueNumber>(P.Z));
				ControlValues.Add(MakeShared<FJsonValueArray>(PointValues));
			}
			CurveObj->SetArrayField(TEXT("control"), ControlValues);
			Curves.Add(MakeShared<FJsonValueObject>(CurveObj));
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			if (!bScaleSet) { ScaleValue = A2->Scale; bScaleSet = true; }
			TSharedRef<FJsonObject> CurveObj = MakeShared<FJsonObject>();
			CurveObj->SetStringField(TEXT("name"), A2->GetName());
			CurveObj->SetStringField(TEXT("space"), TEXT("2D"));
			CurveObj->SetBoolField(TEXT("closed"), A2->UI_IsClosedLoop());

			TArray<TSharedPtr<FJsonValue>> ControlValues;
			for (const FVector2D& P : A2->Control)
			{
				TArray<TSharedPtr<FJsonValue>> PointValues;
				PointValues.Add(MakeShared<FJsonValueNumber>(P.X));
				PointValues.Add(MakeShared<FJsonValueNumber>(P.Y));
				ControlValues.Add(MakeShared<FJsonValueArray>(PointValues));
			}
			CurveObj->SetArrayField(TEXT("control"), ControlValues);
			Curves.Add(MakeShared<FJsonValueObject>(CurveObj));
		}
	}

	Root->SetNumberField(TEXT("scale"), ScaleValue);
	Root->SetArrayField(TEXT("curves"), Curves);

	const FString AbsPath = MakeAbs(CurveSetFile);
	if (bWriteBackupOnExport && !BackupCurveSetFile.IsEmpty())
	{
		const FString BackupPath = MakeAbs(BackupCurveSetFile);
		FString Existing;
		if (ReadText(AbsPath, Existing))
		{
			if (!WriteText(BackupPath, Existing))
			{
				UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to write backup %s"), *BackupPath);
			}
		}
	}

	if (!TE_FileUtils::SaveJson(AbsPath, Root))
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to write %s"), *AbsPath);
	}
}

void ABezierCurveSetActor::UI_ClearSpawned()
{
	ClearSpawned();
}

void ABezierCurveSetActor::UI_SetEditModeForAll(bool bInEditMode)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetEditMode(bInEditMode); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetEditMode(bInEditMode); }
	}
}

void ABezierCurveSetActor::UI_SetActorVisibleForAll(bool bInVisible)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetActorVisibleInGame(bInVisible); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetActorVisibleInGame(bInVisible); }
	}
}

void ABezierCurveSetActor::UI_SetShowControlPointsForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowControlPoints(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowControlPoints(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetShowStripForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowStrip(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowStrip(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetShowCubeStripForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowCubeStrip(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowCubeStrip(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetStripSizeForAll(float InWidth, float InThickness)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetStripSize(InWidth, InThickness); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetStripSize(InWidth, InThickness); }
	}
}

void ABezierCurveSetActor::UI_SetControlPointSizeForAll(float InVisualScale)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetControlPointSize(InVisualScale); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetControlPointSize(InVisualScale); }
	}
}

void ABezierCurveSetActor::UI_SetControlPointColorsForAll(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetControlPointColors(Normal, Hover, Selected); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetControlPointColors(Normal, Hover, Selected); }
	}
}

void ABezierCurveSetActor::UI_SetSnapToGridForAll(bool bInSnap)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetSnapToGrid(bInSnap); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetSnapToGrid(bInSnap); }
	}
}

void ABezierCurveSetActor::UI_SetGridSizeForAll(float InGridSizeCm)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridSizeCm(InGridSizeCm); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridSizeCm(InGridSizeCm); }
	}
}

void ABezierCurveSetActor::UI_SetForcePlanarForAll(bool bInForce)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetForcePlanar(bInForce); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetForcePlanar(bInForce); }
	}
}

void ABezierCurveSetActor::UI_ResetCurveStateForAll()
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_ResetCurveState(); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_ResetCurveState(); }
	}
}

void ABezierCurveSetActor::UI_SetEditInteractionEnabledForAll(bool bEnabled, bool bShowControlPoints, bool bShowStrip)
{
	UI_SetEditModeForAll(bEnabled);
	UI_SetShowControlPointsForAll(bEnabled && bShowControlPoints);
	UI_SetShowStripForAll(bEnabled && bShowStrip);
}
