#pragma once

/**
 * @file BezierCurveSetActor.h
 * @brief Manager actor for importing, exporting, and coordinating groups of Bézier curves.
 *
 * The set actor acts as the bridge between JSON file IO, spawned curve actors,
 * and the UI-facing helpers used by the prototype file browser and bulk-edit tools.
 */
// BezierCurveSetActor.h
// Imports and exports whole sets of Bézier curves, manages spawned curve actors,
// and exposes editor / runtime helper functions for the UMG file menu and edit UI.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BezierRuntimeTypes.h"
#include "BezierCurveSetActor.generated.h"

class ABezierCurve2DActor;
class ABezierCurve3DActor;

/**
 * Controls how imported curve sets should be merged into the current scene.
 */
UENUM(BlueprintType)
enum class EBezierCurveSetImportMode : uint8
{
	ReplaceAll   UMETA(DisplayName = "Replace All"),
	ReplaceByName UMETA(DisplayName = "Replace By Name"),
	SkipExisting UMETA(DisplayName = "Skip Existing"),
	Append       UMETA(DisplayName = "Append")
};

/**
 * Row data used by the UMG file menu for displaying available curve-set JSON files.
 */
USTRUCT(BlueprintType)
struct FBezierCurveSetFileListRowData
{
	GENERATED_BODY()

	/** File name including extension, for example "curve_set_01.json". */
	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString FileName;

	/** Human-readable last modified time string. */
	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString Timestamp;

	/** Human-readable file size string, for example "2.1 KB". */
	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	FString FileSize;

	/** Raw file size in bytes. */
	UPROPERTY(BlueprintReadOnly, Category = "BezierSet|IO")
	int64 FileSizeBytes = 0;
};

/**
 * Actor responsible for importing, exporting, and managing sets of 2D/3D Bézier curves.
 * This is the bridge between JSON file IO, spawned curve actors, and the UMG file menu.
 */
/**
 * @brief Actor that owns high-level import/export and batch-edit operations for multiple curves.
 */
UCLASS()
class THREATEXEC_API ABezierCurveSetActor : public AActor
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	ABezierCurveSetActor();

	/** Starts any runtime services such as optional auto-save management. */
	virtual void BeginPlay() override;
	/** Stops timers and releases transient references during shutdown. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Class used when spawning imported 2D curve actors. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|Classes")
	TSubclassOf<ABezierCurve2DActor> Curve2DClass;

	/** Class used when spawning imported 3D curve actors. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|Classes")
	TSubclassOf<ABezierCurve3DActor> Curve3DClass;

	/** Base IO folder. Relative paths resolve under Project/Saved. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString IOPathAbsolute = TEXT("Bezier");

	/** Default curve-set file used by generic import/export functions. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString CurveSetFile = TEXT("curve_set.json");

	/** Fixed demo file used by the prototype Load button. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString DemoCurveSetFile = TEXT("curve_set_DEMO.json");

	/** Prefix used by the snapshot-style save button. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString ExportedCurveSetPrefix = TEXT("exported_curve_set_");

	/** Starting index when searching for the next numbered snapshot file. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO", meta = (ClampMin = "0"))
	int32 ExportedCurveSetStartIndex = 0;

	/** Import merge mode. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	EBezierCurveSetImportMode ImportMode = EBezierCurveSetImportMode::ReplaceAll;

	/** If true, exporting the main curve set writes a backup first when possible. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	bool bWriteBackupOnExport = true;

	/** Backup file used when bWriteBackupOnExport is enabled. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO")
	FString BackupCurveSetFile = TEXT("curves_backup.json");

	/** Enables timer-based auto-save. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave")
	bool bEnableAutoSave = false;

	/** Auto-save interval in seconds. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave", meta = (ClampMin = "0.1"))
	float AutoSaveIntervalSeconds = 30.0f;

	/** If true, auto-save only runs while any curve is in edit mode. */
	UPROPERTY(EditAnywhere, Category = "BezierSet|IO|AutoSave")
	bool bAutoSaveOnlyWhenEditing = true;

	/** Candidate grid sizes used by the "cycle grid size" action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BezierSet|RuntimeEdit")
	TArray<float> GridSizeCycleValues;

	/** Current index into GridSizeCycleValues. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	int32 GridSizeCycleIndex = 0;

	/** Cached lock-axis cycle state used by UI cycling helpers. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	EBezierPlanarAxis LockAxisCycle = EBezierPlanarAxis::None;

	/** Cached planar-axis cycle state used by UI cycling helpers. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BezierSet|RuntimeEdit")
	EBezierPlanarAxis ForcePlanarAxisCycle = EBezierPlanarAxis::None;

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit", meta = (AutoCreateRefTerm = "InValues"))
	void UI_SetGridSizeCycleValues(const TArray<float>& InValues);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit")
	void UI_ResetGridSizeCycleIndex(int32 InIndex = 0);

	/** Imports the default CurveSetFile. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_ImportCurveSetJson();

	/** Exports the current set to CurveSetFile. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_ExportCurveSetJson();

	// Final prototype helpers:
	// - Load always reads the fixed demo file
	// - Save always writes the next numbered exported_curve_set_N.json
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_LoadDemoCurveSetJson();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	void UI_SaveExportedCurveSetSnapshot();

	// UMG-friendly browser helpers:
	// - enumerate *.json files in the IO folder (default Saved/Bezier)
	// - import one selected file from that list
	// - save with a user-provided filename into the same folder
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	TArray<FString> UI_ListCurveSetJsonFiles(bool bSortAscending = true) const;

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	bool UI_ImportCurveSetJsonByFileName(const FString& FileName);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	bool UI_SaveCurveSetJsonAs(const FString& InFileName, bool bWriteBackup = false);

	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO")
	bool UI_DeleteCurveSetJsonByFileName(const FString& FileName);

	/** Returns all available JSON files for the UMG file menu. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	void UI_FileMenuListCurveSetJsonFiles(TArray<FBezierCurveSetFileListRowData>& OutFiles) const;

	/** Loads a specific JSON file selected from the UMG file menu. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	bool UI_FileMenuLoadCurveSetJsonByFileName(const FString& InFileName);

	/** Saves to a specific JSON file name entered from the UMG file menu. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|IO|FileMenu")
	bool UI_FileMenuSaveCurveSetJsonByFileName(const FString& InFileName, bool bWriteBackup = false);

	/** Destroys all currently managed spawned curve actors. */
	UFUNCTION(CallInEditor, Category = "BezierSet|Manage")
	void ClearSpawned();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|Manage")
	void UI_ClearSpawned();

	/** Registers an externally spawned curve actor into this set actor's managed list. */
	UFUNCTION(BlueprintCallable, Category = "BezierSet|Manage")
	void UI_RegisterSpawned(AActor* Actor);

	/** Returns true if the set currently has any valid 2D or 3D curve actors. */
	UFUNCTION(BlueprintPure, Category = "BezierSet|Manage")
	bool UI_HasAnyCurves() const;

	/** Returns the number of valid 2D/3D curve actors currently managed. */
	UFUNCTION(BlueprintPure, Category = "BezierSet|Manage")
	int32 UI_GetCurveCount() const;

	// Runtime helpers for bulk editing across all curves in the set.
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

	// Bulk sampling controls.
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetSamplingModeForAll(EBezierSamplingMode InMode);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetSampleCountForAll(int32 InCount);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetProofTForAll(double InT);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetShowSamplePointsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_ToggleShowSamplePointsForAll();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_SetShowDeCasteljauLevelsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_ToggleShowDeCasteljauLevelsForAll();

	// Focused-curve helpers routed through the edit subsystem.
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetSamplingMode(EBezierSamplingMode InMode);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") EBezierSamplingMode UI_FocusGetSamplingMode();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetSampleCount(int32 InCount);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") int32 UI_FocusGetSampleCount();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetProofT(double InT);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") double UI_FocusGetProofT();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetShowSamplePoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusToggleShowSamplePoints();
	UFUNCTION(BlueprintPure, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusGetShowSamplePoints();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") void UI_FocusSetShowDeCasteljauLevels(bool bInShow);
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusToggleShowDeCasteljauLevels();
	UFUNCTION(BlueprintPure, Category = "BezierSet|RuntimeEdit|Sampling") bool UI_FocusGetShowDeCasteljauLevels();

	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusAddControlPointAfterSelected();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusDeleteSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") bool UI_FocusDuplicateSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") void UI_FocusDuplicateCurve();
	UFUNCTION(BlueprintCallable, Category = "BezierSet|RuntimeEdit") EBezierPlanarAxis UI_FocusCycleForcePlanarAxis();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** Curves spawned and owned by this set actor. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Spawned;

	/** Timer handle used by optional auto-save. */
	FTimerHandle AutoSaveTimerHandle;

	/** Rebuilds the Spawned list by scanning the current world for owned curve actors. */
	void RefreshSpawnedFromWorld();

	/** Resolves a file name to an absolute path using the current IO folder rules. */
	FString MakeAbs(const FString& FileName) const;

	/** Reads a text file from disk. */
	bool ReadText(const FString& AbsPath, FString& Out) const;

	/** Writes a text file to disk. */
	bool WriteText(const FString& AbsPath, const FString& In) const;

	/** Returns true if any managed curve actor is currently in edit mode. */
	bool IsAnyEditModeActive() const;

	/** Auto-save timer callback. */
	void HandleAutoSave();

	// Internal helpers for final prototype save/load flow
	bool ImportCurveSetJsonFromFile(const FString& FileName);
	TSharedRef<FJsonObject> BuildCurveSetJson() const;
	bool WriteCurveSetJsonToFile(const FString& FileName, bool bWriteBackup) const;
	FString FindNextExportCurveSetFileName() const;
	FString SanitizeCurveSetFileName(const FString& InFileName) const;
};
