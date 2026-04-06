#include "BezierCurveSetActor.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"
#include "BezierDebugActor.h"
#include "BezierEditSubsystem.h"

#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "THREATEXEC_FileUtils.h"
#include "TimerManager.h"
#include "EngineUtils.h"

namespace
{
	/** Converts a sampling mode enum into a stable JSON string. */
	static FString TE_SamplingModeToString(EBezierSamplingMode Mode)
	{
		return Mode == EBezierSamplingMode::ArcLength ? TEXT("arc_length") : TEXT("parametric");
	}

	/** Converts a JSON sampling-mode string back into an enum. */
	static EBezierSamplingMode TE_SamplingModeFromString(const FString& Value, EBezierSamplingMode Fallback)
	{
		if (Value.Equals(TEXT("arc_length"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("arclength"), ESearchCase::IgnoreCase))
		{
			return EBezierSamplingMode::ArcLength;
		}
		if (Value.Equals(TEXT("parametric"), ESearchCase::IgnoreCase))
		{
			return EBezierSamplingMode::Parametric;
		}
		return Fallback;
	}

	/** Formats a timestamp for the file menu list. */
	static FString TE_FormatTimestampDDMMYYYY_HHMMSS(const FDateTime& InTime)
	{
		return FString::Printf(
			TEXT("%02d-%02d-%04d %02d:%02d:%02d"),
			InTime.GetDay(),
			InTime.GetMonth(),
			InTime.GetYear(),
			InTime.GetHour(),
			InTime.GetMinute(),
			InTime.GetSecond());
	}

	/** Formats file size as a small readable label for the UI. */
	static FString TE_FormatFileSizeLabel(const int64 InBytes)
	{
		constexpr double BytesPerKilobyte = 1024.0;
		constexpr double BytesPerMegabyte = 1024.0 * 1024.0;

		if (InBytes >= static_cast<int64>(BytesPerMegabyte))
		{
			return FString::Printf(TEXT("%.2f MB"), static_cast<double>(InBytes) / BytesPerMegabyte);
		}

		return FString::Printf(TEXT("%.1f KB"), static_cast<double>(InBytes) / BytesPerKilobyte);
	}
}

/** Normalises a user-entered file name and ensures the result is a valid JSON file name. */
FString ABezierCurveSetActor::SanitizeCurveSetFileName(const FString& InFileName)
{
	FString FileName = InFileName;
	FileName.TrimStartAndEndInline();

	if (FileName.IsEmpty())
	{
		return FString();
	}

	FileName = FPaths::GetCleanFilename(FileName);
	if (FileName.IsEmpty())
	{
		return FString();
	}

	if (!FileName.EndsWith(TEXT(".json"), ESearchCase::IgnoreCase))
	{
		FileName += TEXT(".json");
	}

	FString BaseName = FPaths::GetBaseFilename(FileName, false);
	FString Extension = FPaths::GetExtension(FileName, false);

	FPaths::MakeValidFileName(BaseName);
	FPaths::MakeValidFileName(Extension);

	if (BaseName.IsEmpty())
	{
		return FString();
	}

	return Extension.IsEmpty() ? BaseName : FString::Printf(TEXT("%s.%s"), *BaseName, *Extension);
}

ABezierCurveSetActor::ABezierCurveSetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	if (GridSizeCycleValues.Num() == 0)
	{
		GridSizeCycleValues = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	}
}

void ABezierCurveSetActor::BeginPlay()
{
	Super::BeginPlay();

	RefreshSpawnedFromWorld();

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

	if (!Curve2DClass)
	{
		Curve2DClass = ABezierCurve2DActor::StaticClass();
	}

	if (!Curve3DClass)
	{
		Curve3DClass = ABezierCurve3DActor::StaticClass();
	}

	RefreshSpawnedFromWorld();
}

/** Resolves a curve-set file name according to the toolkit's Saved-folder IO rules. */
FString ABezierCurveSetActor::MakeAbs(const FString& FileName) const
{
	return TE_PathUtils::ResolveFile(IOPathAbsolute, FileName, TEXT("Bezier"));
}

bool ABezierCurveSetActor::ReadText(const FString& AbsPath, FString& Out) const
{
	return FFileHelper::LoadFileToString(Out, *AbsPath);
}

bool ABezierCurveSetActor::WriteText(const FString& AbsPath, const FString& In) const
{
	return FFileHelper::SaveStringToFile(In, *AbsPath);
}

/** Returns true if any managed curve is currently in runtime edit mode. */
bool ABezierCurveSetActor::IsAnyEditModeActive() const
{
	for (AActor* A : Spawned)
	{
		if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			if (A3->UI_GetEditMode())
			{
				return true;
			}
		}
		else if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			if (A2->UI_GetEditMode())
			{
				return true;
			}
		}
	}
	return false;
}

/** Auto-save timer callback. */
void ABezierCurveSetActor::HandleAutoSave()
{
	if (bAutoSaveOnlyWhenEditing && !IsAnyEditModeActive())
	{
		return;
	}

	UI_ExportCurveSetJson();
}

/** Destroys all currently tracked spawned curve actors. */
void ABezierCurveSetActor::ClearSpawned()
{
	for (AActor* A : Spawned)
	{
		if (IsValid(A))
		{
			A->Destroy();
		}
	}

	Spawned.Reset();
}

/** Rebuilds the Spawned list by scanning the world for 2D/3D curve actors owned by this actor. */
void ABezierCurveSetActor::RefreshSpawnedFromWorld()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	Spawned.Reset();

	for (TActorIterator<ABezierCurve3DActor> It(World); It; ++It)
	{
		if (It->GetOwner() == this)
		{
			Spawned.Add(*It);
		}
	}

	for (TActorIterator<ABezierCurve2DActor> It(World); It; ++It)
	{
		if (It->GetOwner() == this)
		{
			Spawned.Add(*It);
		}
	}
}

/** Serialises the current set of managed curves into the standard curve_set JSON structure. */
TSharedRef<FJsonObject> ABezierCurveSetActor::BuildCurveSetJson() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Curves;

	const int32 CurrentVersion = 2;
	double ScaleValue = 100.0;
	bool bScaleSet = false;

	for (AActor* A : Spawned)
	{
		if (!IsValid(A))
		{
			continue;
		}

		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			if (!bScaleSet)
			{
				ScaleValue = A3->Scale;
				bScaleSet = true;
			}

			TSharedRef<FJsonObject> CurveObj = MakeShared<FJsonObject>();
			CurveObj->SetStringField(TEXT("name"), A3->GetName());
			CurveObj->SetStringField(TEXT("space"), TEXT("3D"));
			CurveObj->SetBoolField(TEXT("closed"), A3->UI_IsClosedLoop());
			CurveObj->SetStringField(TEXT("sampling_mode"), TE_SamplingModeToString(A3->UI_GetSamplingMode()));
			CurveObj->SetNumberField(TEXT("sample_count"), A3->UI_GetSampleCount());

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
			if (!bScaleSet)
			{
				ScaleValue = A2->Scale;
				bScaleSet = true;
			}

			TSharedRef<FJsonObject> CurveObj = MakeShared<FJsonObject>();
			CurveObj->SetStringField(TEXT("name"), A2->GetName());
			CurveObj->SetStringField(TEXT("space"), TEXT("2D"));
			CurveObj->SetBoolField(TEXT("closed"), A2->UI_IsClosedLoop());
			CurveObj->SetStringField(TEXT("sampling_mode"), TE_SamplingModeToString(A2->UI_GetSamplingMode()));
			CurveObj->SetNumberField(TEXT("sample_count"), A2->UI_GetSampleCount());

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

	Root->SetNumberField(TEXT("version"), CurrentVersion);
	Root->SetStringField(TEXT("point_space"), TEXT("local"));
	Root->SetNumberField(TEXT("scale"), ScaleValue);
	Root->SetArrayField(TEXT("curves"), Curves);

	return Root;
}

/** Writes the current curve set to disk, optionally writing a backup of the previous file contents first. */
bool ABezierCurveSetActor::WriteCurveSetJsonToFile(const FString& FileName, bool bWriteBackup) const
{
	const FString AbsPath = MakeAbs(FileName);
	const TSharedRef<FJsonObject> Root = BuildCurveSetJson();

	if (bWriteBackup && !BackupCurveSetFile.IsEmpty())
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
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("BezierCurveSetActor: Saved curve set to %s"), *AbsPath);
	return true;
}

/** Finds the next available export file name using the configured prefix and start index. */
FString ABezierCurveSetActor::FindNextExportCurveSetFileName() const
{
	const FString SafePrefix = ExportedCurveSetPrefix.IsEmpty()
		? TEXT("exported_curve_set_")
		: ExportedCurveSetPrefix;

	for (int32 Index = FMath::Max(0, ExportedCurveSetStartIndex); Index < 100000; ++Index)
	{
		const FString Candidate = FString::Printf(TEXT("%s%d.json"), *SafePrefix, Index);
		const FString AbsPath = MakeAbs(Candidate);

		if (!FPaths::FileExists(AbsPath))
		{
			return Candidate;
		}
	}

	return FString::Printf(TEXT("%s%d.json"), *SafePrefix, 99999);
}

/** Imports a curve set from a specific file, replacing or merging into the current world according to ImportMode. */
bool ABezierCurveSetActor::ImportCurveSetJsonFromFile(const FString& FileName)
{
	FString JsonText;
	const FString AbsPath = MakeAbs(FileName);

	if (!ReadText(AbsPath, JsonText))
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to read %s"), *AbsPath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Failed to parse %s"), *AbsPath);
		return false;
	}

	const int32 CurrentVersion = 2;
	int32 Version = 0;
	Root->TryGetNumberField(TEXT("version"), Version);

	if (Version > CurrentVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Curve set version %d is newer than supported %d."), Version, CurrentVersion);
	}

	FString PointSpace = TEXT("local");
	Root->TryGetStringField(TEXT("point_space"), PointSpace);
	const bool bWorldSpacePoints = PointSpace.Equals(TEXT("world"), ESearchCase::IgnoreCase);

	double FileScale = 100.0;
	Root->TryGetNumberField(TEXT("scale"), FileScale);

	const TArray<TSharedPtr<FJsonValue>>* CurvesArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("curves"), CurvesArray) || !CurvesArray)
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: No curves array in %s"), *AbsPath);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!Curve2DClass)
	{
		Curve2DClass = ABezierCurve2DActor::StaticClass();
	}

	if (!Curve3DClass)
	{
		Curve3DClass = ABezierCurve3DActor::StaticClass();
	}

	// World-authored control points should spawn at identity so the imported points land exactly where authored.
	const FTransform SpawnTransform = bWorldSpacePoints ? FTransform::Identity : GetActorTransform();

	if (ImportMode == EBezierCurveSetImportMode::ReplaceAll)
	{
		ClearSpawned();
	}

	TMap<FName, TWeakObjectPtr<AActor>> ExistingByName;
	if (ImportMode == EBezierCurveSetImportMode::ReplaceByName || ImportMode == EBezierCurveSetImportMode::SkipExisting)
	{
		for (AActor* A : Spawned)
		{
			if (!IsValid(A))
			{
				continue;
			}

			const FName NameKey = A->GetFName();
			if (!NameKey.IsNone())
			{
				ExistingByName.Add(NameKey, A);
			}
		}
	}

	for (const TSharedPtr<FJsonValue>& CurveValue : *CurvesArray)
	{
		const TSharedPtr<FJsonObject>* CurveObj = nullptr;
		if (!CurveValue.IsValid() || !CurveValue->TryGetObject(CurveObj) || !CurveObj || !CurveObj->IsValid())
		{
			continue;
		}

		FString CurveName;
		(*CurveObj)->TryGetStringField(TEXT("name"), CurveName);

		FString Space;
		(*CurveObj)->TryGetStringField(TEXT("space"), Space);

		bool bClosed = false;
		(*CurveObj)->TryGetBoolField(TEXT("closed"), bClosed);

		FString SamplingModeStr;
		(*CurveObj)->TryGetStringField(TEXT("sampling_mode"), SamplingModeStr);

		double SampleCountNumber = 0.0;
		(*CurveObj)->TryGetNumberField(TEXT("sample_count"), SampleCountNumber);
		const int32 SampleCount = FMath::RoundToInt(SampleCountNumber);

		FString SourceProfile;
		(*CurveObj)->TryGetStringField(TEXT("source_profile"), SourceProfile);

		const TArray<TSharedPtr<FJsonValue>>* ControlArray = nullptr;
		if (!(*CurveObj)->TryGetArrayField(TEXT("control"), ControlArray) || !ControlArray)
		{
			UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Curve '%s' missing control array."), *CurveName);
			continue;
		}

		const bool bIs3D = Space.Equals(TEXT("3D"), ESearchCase::IgnoreCase);
		const bool bIs2D = Space.Equals(TEXT("2D"), ESearchCase::IgnoreCase);

		if (!bIs3D && !bIs2D)
		{
			UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Curve '%s' has invalid space '%s'."), *CurveName, *Space);
			continue;
		}

		if (ControlArray->Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: Curve '%s' has too few control points."), *CurveName);
			continue;
		}

		if (!CurveName.IsEmpty())
		{
			const FName CurveFName(*CurveName);

			if (ImportMode == EBezierCurveSetImportMode::SkipExisting && ExistingByName.Contains(CurveFName))
			{
				continue;
			}

			if (ImportMode == EBezierCurveSetImportMode::ReplaceByName)
			{
				if (TWeakObjectPtr<AActor>* Existing = ExistingByName.Find(CurveFName))
				{
					if (Existing->IsValid())
					{
						Spawned.Remove(Existing->Get());
						Existing->Get()->Destroy();
					}
					ExistingByName.Remove(CurveFName);
				}
			}
		}

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

		if (!CurveName.IsEmpty())
		{
			Params.Name = MakeUniqueObjectName(World, bIs3D ? Curve3DClass : Curve2DClass, FName(*CurveName));
		}

		if (bIs3D)
		{
			ABezierCurve3DActor* A3 = World->SpawnActor<ABezierCurve3DActor>(Curve3DClass, SpawnTransform, Params);
			if (!A3)
			{
				continue;
			}

			A3->Scale = FileScale;
			A3->Control.Reset();

			for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
			{
				const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 3)
				{
					continue;
				}

				double X = 0.0;
				double Y = 0.0;
				double Z = 0.0;

				if (!(*PointArray)[0].IsValid() || !(*PointArray)[0]->TryGetNumber(X)
					|| !(*PointArray)[1].IsValid() || !(*PointArray)[1]->TryGetNumber(Y)
					|| !(*PointArray)[2].IsValid() || !(*PointArray)[2]->TryGetNumber(Z))
				{
					continue;
				}

				A3->Control.Add(FVector(X, Y, Z));
			}

			if (A3->Control.Num() < 2)
			{
				A3->Destroy();
				continue;
			}

			A3->UI_SetInitialControlFromCurrent();
			A3->OverwriteSplineFromControl();
			A3->UI_SetClosedLoop(bClosed);

			const EBezierSamplingMode SamplingMode = TE_SamplingModeFromString(SamplingModeStr, EBezierSamplingMode::Parametric);
			A3->UI_SetSamplingMode(SamplingMode);

			const int32 ImportedSampleCount = SampleCount > 0 ? SampleCount : A3->Control.Num();
			A3->UI_SetSampleCount(FMath::Clamp(ImportedSampleCount, 2, 2048));

			if (SourceProfile.Equals(TEXT("max_sampled"), ESearchCase::IgnoreCase))
			{
				A3->UI_SetSamplingMode(EBezierSamplingMode::ArcLength);
				A3->UI_SetSampleCount(FMath::Clamp(FMath::Max(ImportedSampleCount, A3->Control.Num()), 2, 2048));
			}

			UI_RegisterSpawned(A3);
		}
		else
		{
			ABezierCurve2DActor* A2 = World->SpawnActor<ABezierCurve2DActor>(Curve2DClass, SpawnTransform, Params);
			if (!A2)
			{
				continue;
			}

			A2->Scale = FileScale;
			A2->Control.Reset();

			for (const TSharedPtr<FJsonValue>& Entry : *ControlArray)
			{
				const TArray<TSharedPtr<FJsonValue>>* PointArray = nullptr;
				if (!Entry.IsValid() || !Entry->TryGetArray(PointArray) || !PointArray || PointArray->Num() < 2)
				{
					continue;
				}

				double X = 0.0;
				double Y = 0.0;

				if (!(*PointArray)[0].IsValid() || !(*PointArray)[0]->TryGetNumber(X)
					|| !(*PointArray)[1].IsValid() || !(*PointArray)[1]->TryGetNumber(Y))
				{
					continue;
				}

				A2->Control.Add(FVector2D(X, Y));
			}

			if (A2->Control.Num() < 2)
			{
				A2->Destroy();
				continue;
			}

			A2->UI_SetInitialControlFromCurrent();
			A2->OverwriteSplineFromControl();
			A2->UI_SetClosedLoop(bClosed);

			const EBezierSamplingMode SamplingMode = TE_SamplingModeFromString(SamplingModeStr, EBezierSamplingMode::Parametric);
			A2->UI_SetSamplingMode(SamplingMode);

			const int32 ImportedSampleCount = SampleCount > 0 ? SampleCount : A2->Control.Num();
			A2->UI_SetSampleCount(FMath::Clamp(ImportedSampleCount, 2, 2048));

			if (SourceProfile.Equals(TEXT("max_sampled"), ESearchCase::IgnoreCase))
			{
				A2->UI_SetSamplingMode(EBezierSamplingMode::ArcLength);
				A2->UI_SetSampleCount(FMath::Clamp(FMath::Max(ImportedSampleCount, A2->Control.Num()), 2, 2048));
			}

			UI_RegisterSpawned(A2);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("BezierCurveSetActor: Loaded curve set from %s"), *AbsPath);
	return true;
}

void ABezierCurveSetActor::UI_ImportCurveSetJson()
{
	ImportCurveSetJsonFromFile(CurveSetFile);
}

void ABezierCurveSetActor::UI_LoadDemoCurveSetJson()
{
	ImportCurveSetJsonFromFile(DemoCurveSetFile);
}

void ABezierCurveSetActor::UI_ExportCurveSetJson()
{
	WriteCurveSetJsonToFile(CurveSetFile, bWriteBackupOnExport);
}

void ABezierCurveSetActor::UI_SaveExportedCurveSetSnapshot()
{
	const FString NextFile = FindNextExportCurveSetFileName();
	WriteCurveSetJsonToFile(NextFile, false);
}

/** Builds the UMG file-menu list by scanning the IO folder for JSON files and sorting newest first. */
void ABezierCurveSetActor::UI_FileMenuListCurveSetJsonFiles(TArray<FBezierCurveSetFileListRowData>& OutFiles) const
{
	OutFiles.Reset();

	const FString Directory = TE_PathUtils::ResolveSavedDir(IOPathAbsolute, TEXT("Bezier"));
	TArray<FString> FoundFiles;
	IFileManager::Get().FindFiles(FoundFiles, *(Directory / TEXT("*.json")), true, false);

	struct FSortableFileRow
	{
		FBezierCurveSetFileListRowData Row;
		FDateTime SortTime = FDateTime::MinValue();
	};

	TArray<FSortableFileRow> Rows;
	Rows.Reserve(FoundFiles.Num());

	for (const FString& FileName : FoundFiles)
	{
		const FString FullPath = Directory / FileName;
		const FFileStatData StatData = IFileManager::Get().GetStatData(*FullPath);

		if (!StatData.bIsValid || StatData.bIsDirectory)
		{
			continue;
		}

		const FDateTime LocalModified = StatData.ModificationTime;

		FSortableFileRow SortableRow;
		SortableRow.Row.FileName = FileName;
		SortableRow.Row.FileSizeBytes = StatData.FileSize;
		SortableRow.Row.FileSize = TE_FormatFileSizeLabel(StatData.FileSize);
		SortableRow.Row.Timestamp = TE_FormatTimestampDDMMYYYY_HHMMSS(LocalModified);
		SortableRow.SortTime = LocalModified;
		Rows.Add(MoveTemp(SortableRow));
	}

	Rows.Sort([](const FSortableFileRow& A, const FSortableFileRow& B)
		{
			if (A.SortTime == B.SortTime)
			{
				return A.Row.FileName < B.Row.FileName;
			}
			return A.SortTime > B.SortTime;
		});

	OutFiles.Reserve(Rows.Num());
	for (const FSortableFileRow& Row : Rows)
	{
		OutFiles.Add(Row.Row);
	}
}

bool ABezierCurveSetActor::UI_FileMenuLoadCurveSetJsonByFileName(const FString& InFileName)
{
	const FString NormalizedFileName = SanitizeCurveSetFileName(InFileName);
	if (NormalizedFileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: UI_FileMenuLoadCurveSetJsonByFileName received empty file name."));
		return false;
	}

	return ImportCurveSetJsonFromFile(NormalizedFileName);
}

bool ABezierCurveSetActor::UI_FileMenuSaveCurveSetJsonByFileName(const FString& InFileName, bool bWriteBackup)
{
	const FString NormalizedFileName = SanitizeCurveSetFileName(InFileName);
	if (NormalizedFileName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BezierCurveSetActor: UI_FileMenuSaveCurveSetJsonByFileName received empty file name."));
		return false;
	}

	return WriteCurveSetJsonToFile(NormalizedFileName, bWriteBackup);
}

// Deprecated wrappers kept so existing Blueprints do not break immediately.
void ABezierCurveSetActor::UI_ListCurveSetJsonFiles(TArray<FBezierCurveSetFileListRowData>& OutFiles) const
{
	UI_FileMenuListCurveSetJsonFiles(OutFiles);
}

bool ABezierCurveSetActor::UI_LoadCurveSetJsonByFileName(const FString& InFileName)
{
	return UI_FileMenuLoadCurveSetJsonByFileName(InFileName);
}

bool ABezierCurveSetActor::UI_SaveCurveSetJsonByFileName(const FString& InFileName, bool bWriteBackup)
{
	return UI_FileMenuSaveCurveSetJsonByFileName(InFileName, bWriteBackup);
}

void ABezierCurveSetActor::UI_ClearSpawned()
{
	ClearSpawned();
}

/** Registers a curve actor with this set actor and updates editor integration state where needed. */
void ABezierCurveSetActor::UI_RegisterSpawned(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	if (!Spawned.Contains(Actor))
	{
		Spawned.Add(Actor);
	}

	if (Actor->GetOwner() != this)
	{
		Actor->SetOwner(this);
	}

	if (UWorld* World = GetWorld())
	{
		if (UBezierEditSubsystem* Subsystem = World->GetSubsystem<UBezierEditSubsystem>())
		{
			Subsystem->RegisterEditable(Actor);
		}

		for (TActorIterator<ABezierDebugActor> It(World); It; ++It)
		{
			It->SyncFromWorldState();
			It->ApplyDebugToCurve(Actor);
			break;
		}
	}
}

bool ABezierCurveSetActor::UI_HasAnyCurves() const
{
	for (AActor* A : Spawned)
	{
		if (IsValid(A) && (Cast<ABezierCurve3DActor>(A) || Cast<ABezierCurve2DActor>(A)))
		{
			return true;
		}
	}
	return false;
}

int32 ABezierCurveSetActor::UI_GetCurveCount() const
{
	int32 Count = 0;
	for (AActor* A : Spawned)
	{
		if (IsValid(A) && (Cast<ABezierCurve3DActor>(A) || Cast<ABezierCurve2DActor>(A)))
		{
			++Count;
		}
	}
	return Count;
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

	UI_SetShowGridForAll(bInSnap);

	if (bInSnap)
	{
		UI_SetShowGridXYForAll(true);
		UI_SetShowGridXZForAll(true);
		UI_SetShowGridYZForAll(true);
	}
}

bool ABezierCurveSetActor::UI_ToggleSnapToGridForAll()
{
	bool bCurrent = false;
	bool bFound = false;

	for (AActor* A : Spawned)
	{
		if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			bCurrent = A3->bSnapToGrid;
			bFound = true;
			break;
		}
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			bCurrent = A2->bSnapToGrid;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return false;
	}

	const bool bNext = !bCurrent;
	UI_SetSnapToGridForAll(bNext);
	return bNext;
}

void ABezierCurveSetActor::UI_SetGridSizeForAll(float InGridSizeCm)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridSizeCm(InGridSizeCm); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridSizeCm(InGridSizeCm); }
	}
}

void ABezierCurveSetActor::UI_SetGridSizeCycleValues(const TArray<float>& InValues)
{
	GridSizeCycleValues.Reset();

	for (float Value : InValues)
	{
		if (Value > 0.0f)
		{
			GridSizeCycleValues.Add(Value);
		}
	}

	if (GridSizeCycleValues.Num() == 0)
	{
		GridSizeCycleValues = { 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 25.0f };
	}

	GridSizeCycleIndex = 0;
}

void ABezierCurveSetActor::UI_ResetGridSizeCycleIndex(int32 InIndex)
{
	if (GridSizeCycleValues.Num() > 0)
	{
		GridSizeCycleIndex = FMath::Clamp(InIndex, 0, GridSizeCycleValues.Num() - 1);
	}
	else
	{
		GridSizeCycleIndex = 0;
	}
}

void ABezierCurveSetActor::UI_SetGridOriginWorldForAll(FVector InOrigin)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridOriginWorld(InOrigin); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridOriginWorld(InOrigin); }
	}
}

void ABezierCurveSetActor::UI_SetGridExtentForAll(float InGridExtentCm)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridExtentCm(InGridExtentCm); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridExtentCm(InGridExtentCm); }
	}
}

void ABezierCurveSetActor::UI_SetGridColorForAll(FLinearColor InColor)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridColor(InColor); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridColor(InColor); }
	}
}

void ABezierCurveSetActor::UI_SetGridBaseAlphaForAll(float InAlpha)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetGridBaseAlpha(InAlpha); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetGridBaseAlpha(InAlpha); }
	}
}

void ABezierCurveSetActor::UI_SetShowGridXYForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowGridXY(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowGridXY(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetShowGridXZForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowGridXZ(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetShowGridYZForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowGridYZ(bInShow); }
	}
}

void ABezierCurveSetActor::UI_CycleGridSizeForAll()
{
	const TArray<float> Defaults = { 5.0f, 10.0f, 25.0f, 50.0f, 100.0f };
	const TArray<float>& Values = GridSizeCycleValues.Num() > 0 ? GridSizeCycleValues : Defaults;

	if (Values.Num() == 0)
	{
		return;
	}

	GridSizeCycleIndex = (GridSizeCycleIndex + 1) % Values.Num();
	UI_SetGridSizeForAll(Values[GridSizeCycleIndex]);
}

void ABezierCurveSetActor::UI_SetForcePlanarForAll(bool bInForce)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetForcePlanar(bInForce); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetForcePlanar(bInForce); }
	}
}

void ABezierCurveSetActor::UI_SetForcePlanarAxisForAll(EBezierPlanarAxis InAxis)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			A3->UI_SetForcePlanarAxis(InAxis);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			A2->UI_SetForcePlanar(InAxis != EBezierPlanarAxis::None);
		}
	}
}

EBezierPlanarAxis ABezierCurveSetActor::UI_CycleForcePlanarAxisForAll()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		if (Subsystem->HasFocused())
		{
			return Subsystem->Focus_CycleForcePlanarAxis();
		}
	}

	EBezierPlanarAxis CurrentAxis = ForcePlanarAxisCycle;
	if (CurrentAxis == EBezierPlanarAxis::None)
	{
		for (AActor* A : Spawned)
		{
			if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
			{
				CurrentAxis = A3->bForcePlanar ? A3->ForcePlanarAxis : EBezierPlanarAxis::None;
				break;
			}
		}
	}

	EBezierPlanarAxis NextAxis = EBezierPlanarAxis::None;
	switch (CurrentAxis)
	{
	case EBezierPlanarAxis::None: NextAxis = EBezierPlanarAxis::XY; break;
	case EBezierPlanarAxis::XY:   NextAxis = EBezierPlanarAxis::XZ; break;
	case EBezierPlanarAxis::XZ:   NextAxis = EBezierPlanarAxis::YZ; break;
	case EBezierPlanarAxis::YZ:   NextAxis = EBezierPlanarAxis::None; break;
	default: break;
	}

	ForcePlanarAxisCycle = NextAxis;
	UI_SetForcePlanarAxisForAll(NextAxis);
	return NextAxis;
}

void ABezierCurveSetActor::UI_SetLockToLocalXYForAll(bool bInLock)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetLockToLocalXY(bInLock); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetLockToLocalXY(bInLock); }
	}
}

EBezierPlanarAxis ABezierCurveSetActor::UI_CycleLockAxisForAll()
{
	EBezierPlanarAxis CurrentAxis = LockAxisCycle;
	if (CurrentAxis == EBezierPlanarAxis::None)
	{
		for (AActor* A : Spawned)
		{
			if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
			{
				if (A3->bLockPlanar)
				{
					CurrentAxis = A3->LockPlanarAxis;
				}
				break;
			}
		}
	}

	EBezierPlanarAxis NextAxis = EBezierPlanarAxis::None;
	switch (CurrentAxis)
	{
	case EBezierPlanarAxis::None: NextAxis = EBezierPlanarAxis::XY; break;
	case EBezierPlanarAxis::XY:   NextAxis = EBezierPlanarAxis::XZ; break;
	case EBezierPlanarAxis::XZ:   NextAxis = EBezierPlanarAxis::YZ; break;
	case EBezierPlanarAxis::YZ:   NextAxis = EBezierPlanarAxis::None; break;
	default: break;
	}

	LockAxisCycle = NextAxis;

	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			A3->UI_SetLockAxis(NextAxis);
		}
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			const bool bLock2D = NextAxis != EBezierPlanarAxis::None;
			A2->UI_SetLockToLocalXY(bLock2D);
			A2->UI_SetForcePlanar(bLock2D);
		}
	}

	return NextAxis;
}

void ABezierCurveSetActor::UI_SetShowGridForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowGrid(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowGrid(bInShow); }
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

void ABezierCurveSetActor::UI_SetSamplingModeForAll(EBezierSamplingMode InMode)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetSamplingMode(InMode); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetSamplingMode(InMode); }
	}
}

void ABezierCurveSetActor::UI_SetSampleCountForAll(int32 InCount)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetSampleCount(InCount); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetSampleCount(InCount); }
	}
}

void ABezierCurveSetActor::UI_SetProofTForAll(double InT)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetProofT(InT); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetProofT(InT); }
	}
}

void ABezierCurveSetActor::UI_SetShowSamplePointsForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowSamplePoints(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowSamplePoints(bInShow); }
	}
}

bool ABezierCurveSetActor::UI_ToggleShowSamplePointsForAll()
{
	bool bCurrent = false;
	bool bFound = false;

	for (AActor* A : Spawned)
	{
		if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			bCurrent = A3->UI_GetShowSamplePoints();
			bFound = true;
			break;
		}
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			bCurrent = A2->UI_GetShowSamplePoints();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return false;
	}

	const bool bNext = !bCurrent;
	UI_SetShowSamplePointsForAll(bNext);
	return bNext;
}

void ABezierCurveSetActor::UI_SetShowDeCasteljauLevelsForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowDeCasteljauLevels(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowDeCasteljauLevels(bInShow); }
	}
}

bool ABezierCurveSetActor::UI_ToggleShowDeCasteljauLevelsForAll()
{
	bool bCurrent = false;
	bool bFound = false;

	for (AActor* A : Spawned)
	{
		if (const ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A))
		{
			bCurrent = A3->UI_GetShowDeCasteljauLevels();
			bFound = true;
			break;
		}
		if (const ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A))
		{
			bCurrent = A2->UI_GetShowDeCasteljauLevels();
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return false;
	}

	const bool bNext = !bCurrent;
	UI_SetShowDeCasteljauLevelsForAll(bNext);
	return bNext;
}

void ABezierCurveSetActor::UI_FocusSetSamplingMode(EBezierSamplingMode InMode)
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_SetSamplingMode(InMode);
	}
}

EBezierSamplingMode ABezierCurveSetActor::UI_FocusGetSamplingMode()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_GetSamplingMode();
	}
	return EBezierSamplingMode::Parametric;
}

void ABezierCurveSetActor::UI_FocusSetSampleCount(int32 InCount)
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_SetSampleCount(InCount);
	}
}

int32 ABezierCurveSetActor::UI_FocusGetSampleCount()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_GetSampleCount();
	}
	return 0;
}

void ABezierCurveSetActor::UI_FocusSetProofT(double InT)
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_SetProofT(InT);
	}
}

double ABezierCurveSetActor::UI_FocusGetProofT()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_GetProofT();
	}
	return 0.0;
}

void ABezierCurveSetActor::UI_FocusSetShowSamplePoints(bool bInShow)
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_SetShowSamplePoints(bInShow);
	}
}

bool ABezierCurveSetActor::UI_FocusToggleShowSamplePoints()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		const bool bNext = !Subsystem->Focus_GetShowSamplePoints();
		Subsystem->Focus_SetShowSamplePoints(bNext);
		return bNext;
	}
	return false;
}

bool ABezierCurveSetActor::UI_FocusGetShowSamplePoints()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_GetShowSamplePoints();
	}
	return false;
}

void ABezierCurveSetActor::UI_FocusSetShowDeCasteljauLevels(bool bInShow)
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_SetShowDeCasteljauLevels(bInShow);
	}
}

bool ABezierCurveSetActor::UI_FocusToggleShowDeCasteljauLevels()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		const bool bNext = !Subsystem->Focus_GetShowDeCasteljauLevels();
		Subsystem->Focus_SetShowDeCasteljauLevels(bNext);
		return bNext;
	}
	return false;
}

bool ABezierCurveSetActor::UI_FocusGetShowDeCasteljauLevels()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_GetShowDeCasteljauLevels();
	}
	return false;
}

bool ABezierCurveSetActor::UI_FocusAddControlPointAfterSelected()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_AddControlPointAfterSelected();
	}
	return false;
}

bool ABezierCurveSetActor::UI_FocusDeleteSelectedControlPoint()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_DeleteSelectedControlPoint();
	}
	return false;
}

bool ABezierCurveSetActor::UI_FocusDuplicateSelectedControlPoint()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_DuplicateSelectedControlPoint();
	}
	return false;
}

void ABezierCurveSetActor::UI_FocusDuplicateCurve()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		Subsystem->Focus_DuplicateCurve();
	}
}

EBezierPlanarAxis ABezierCurveSetActor::UI_FocusCycleForcePlanarAxis()
{
	if (UBezierEditSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBezierEditSubsystem>() : nullptr)
	{
		return Subsystem->Focus_CycleForcePlanarAxis();
	}
	return EBezierPlanarAxis::None;
}