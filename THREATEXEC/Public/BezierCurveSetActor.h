// BezierCurveSetActor.h : Imports a whole set of curves from JSON, spawns child actors
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BezierRuntimeTypes.h"
#include "BezierCurveSetActor.generated.h"

class ABezierCurve2DActor;
class ABezierCurve3DActor;

UENUM(BlueprintType)
enum class EBezierCurveSetImportMode : uint8
{
	ReplaceAll UMETA(DisplayName = "Replace All"),
	ReplaceByName UMETA(DisplayName = "Replace By Name"),
	SkipExisting UMETA(DisplayName = "Skip Existing"),
	Append UMETA(DisplayName = "Append")
};

USTRUCT(BlueprintType)
struct FBezierCurveSetFileRow
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString FileName;

	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString FileSize;

	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	int64 FileSizeBytes = 0;
};

UCLASS()
class THREATEXEC_API ABezierCurveSetActor : public AActor
{
	GENERATED_BODY()

public:
	ABezierCurveSetActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "BezierSet|Classes")
	TSubclassOf<ABezierCurve2DActor> Curve2DClass;

	UPROPERTY(EditAnywhere, Category = "BezierSet|Classes")
	TSubclassOf<ABezierCurve3DActor> Curve3DClass;

	// Base IO folder. Relative paths are resolved under Project/Saved.
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString IOPathAbsolute = TEXT("Bezier");

	// Generic/manual export target for the full curve set.
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString CurveSetFile = TEXT("curve_set.json");

	// Fixed demo file used by the final prototype Load button.
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString DemoCurveSetFile = TEXT("curve_set.json");

	// Prefix used by the final prototype Save button.
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString ExportedCurveSetPrefix = TEXT("exported_curve_set_");

	// Starting index when searching for the next numbered export file.
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO", meta = (ClampMin = "0"))
	int32 ExportedCurveSetStartIndex = 0;

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	EBezierCurveSetImportMode ImportMode = EBezierCurveSetImportMode::ReplaceAll;

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	bool bWriteBackupOnExport = true;

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString BackupCurveSetFile = TEXT("curves_backup.json");

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave")
	bool bEnableAutoSave = false;

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave", meta = (ClampMin = "0.1"))
	float AutoSaveIntervalSeconds = 30.0f;

	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave")
	bool bAutoSaveOnlyWhenEditing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierSet|RuntimeEdit")
	TArray<float> GridSizeCycleValues;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	int32 GridSizeCycleIndex = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	EBezierPlanarAxis LockAxisCycle = EBezierPlanarAxis::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	EBezierPlanarAxis ForcePlanarAxisCycle = EBezierPlanarAxis::None;

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit", meta = (AutoCreateRefTerm = "InValues"))
	void UI_SetGridSizeCycleValues(const TArray<float>& InValues);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit")
	void UI_ResetGridSizeCycleIndex(int32 InIndex = 0);

	UFUNCTION(CallInEditor, Category = "BezierSet|IO")
	void ImportCurveSetJson();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_ImportCurveSetJson();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_ExportCurveSetJson();

	// Final prototype helpers:
	// - Load always reads the fixed demo file
	// - Save always writes the next numbered exported_curve_set_N.json
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_LoadDemoCurveSetJson();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_SaveExportedCurveSetSnapshot();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	void UI_ListCurveSetJsonFiles(TArray<FBezierCurveSetFileRow>& OutFiles) const;

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	bool UI_LoadCurveSetJsonByFileName(const FString& InFileName);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	bool UI_SaveCurveSetJsonByFileName(const FString& InFileName, bool bWriteBackup = false);

	UFUNCTION(CallInEditor, Category = "BezierSet|Manage")
	void ClearSpawned();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|Manage")
	void UI_ClearSpawned();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|Manage")
	void UI_RegisterSpawned(AActor* Actor);

	UFUNCTION(BlueprintPure, Category = "BezierSet|Manage")
	bool UI_HasAnyCurves() const;

	UFUNCTION(BlueprintPure, Category = "BezierSet|Manage")
	int32 UI_GetCurveCount() const;

	// Runtime helpers (easy to call from UMG)
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetEditModeForAll(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetActorVisibleForAll(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowControlPointsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowStripForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowCubeStripForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetStripSizeForAll(float InWidth, float InThickness);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetControlPointSizeForAll(float InVisualScale);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetControlPointColorsForAll(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetSnapToGridForAll(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_ToggleSnapToGridForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetGridSizeForAll(float InGridSizeCm);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetGridOriginWorldForAll(FVector InOrigin);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetGridExtentForAll(float InGridExtentCm);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetGridColorForAll(FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetGridBaseAlphaForAll(float InAlpha);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowGridXYForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowGridXZForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowGridYZForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_CycleGridSizeForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetForcePlanarForAll(bool bInForce);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetForcePlanarAxisForAll(EBezierPlanarAxis InAxis);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") EBezierPlanarAxis UI_CycleForcePlanarAxisForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetLockToLocalXYForAll(bool bInLock);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") EBezierPlanarAxis UI_CycleLockAxisForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetShowGridForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_ResetCurveStateForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_SetEditInteractionEnabledForAll(bool bEnabled, bool bShowControlPoints = true, bool bShowStrip = true);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetSamplingModeForAll(EBezierSamplingMode InMode);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetSampleCountForAll(int32 InCount);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetProofTForAll(double InT);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetShowSamplePointsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_ToggleShowSamplePointsForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetShowDeCasteljauLevelsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_ToggleShowDeCasteljauLevelsForAll();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetSamplingMode(EBezierSamplingMode InMode);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") EBezierSamplingMode UI_FocusGetSamplingMode();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetSampleCount(int32 InCount);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") int32 UI_FocusGetSampleCount();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetProofT(double InT);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") double UI_FocusGetProofT();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetShowSamplePoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusToggleShowSamplePoints();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusGetShowSamplePoints();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetShowDeCasteljauLevels(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusToggleShowDeCasteljauLevels();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusGetShowDeCasteljauLevels();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusAddControlPointAfterSelected();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusDeleteSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusDuplicateSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_FocusDuplicateCurve();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") EBezierPlanarAxis UI_FocusCycleForcePlanarAxis();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Spawned;

	FTimerHandle AutoSaveTimerHandle;

	void RefreshSpawnedFromWorld();
	FString MakeAbs(const FString& FileName) const;
	bool ReadText(const FString& AbsPath, FString& Out) const;
	bool WriteText(const FString& AbsPath, const FString& In) const;
	bool IsAnyEditModeActive() const;
	void HandleAutoSave();
	static FString SanitizeCurveSetFileName(const FString& InFileName);

	// Internal helpers for final prototype save/load flow
	bool ImportCurveSetJsonFromFile(const FString& FileName);
	TSharedRef<FJsonObject> BuildCurveSetJson() const;
	bool WriteCurveSetJsonToFile(const FString& FileName, bool bWriteBackup) const;
	FString FindNextExportCurveSetFileName() const;
};
