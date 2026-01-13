// BezierCurveSetActor.h : Imports a whole set of curves from JSON, spawns child actors
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "BezierRuntimeTypes.h"
#include "BezierCurveSetActor.generated.h"

class ABezierCurve2DActor;
class ABezierCurve3DActor;

UCLASS()
class THREATEXEC_API ABezierCurveSetActor : public AActor
{
	GENERATED_BODY()

public:
	ABezierCurveSetActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category="BezierSet|Classes")
	TSubclassOf<ABezierCurve2DActor> Curve2DClass;

	UPROPERTY(EditAnywhere, Category="BezierSet|Classes")
	TSubclassOf<ABezierCurve3DActor> Curve3DClass;

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	FString IOPathAbsolute = TEXT("Content");

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	FString CurveSetFile = TEXT("curves.json");

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	bool bWriteBackupOnExport = true;

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	FString BackupCurveSetFile = TEXT("curves_backup.json");

	UPROPERTY(EditAnywhere, Category="BezierSet|IO|AutoSave")
	bool bEnableAutoSave = false;

	UPROPERTY(EditAnywhere, Category="BezierSet|IO|AutoSave", meta=(ClampMin="0.1"))
	float AutoSaveIntervalSeconds = 30.0f;

	UPROPERTY(EditAnywhere, Category="BezierSet|IO|AutoSave")
	bool bAutoSaveOnlyWhenEditing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="BezierSet|RuntimeEdit")
	TArray<float> GridSizeCycleValues;

	UPROPERTY(Transient, BlueprintReadOnly, Category="BezierSet|RuntimeEdit")
	int32 GridSizeCycleIndex = 0;

	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit", meta=(AutoCreateRefTerm="InValues")) void UI_SetGridSizeCycleValues(const TArray<float>& InValues);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_ResetGridSizeCycleIndex(int32 InIndex = 0);

	UFUNCTION(CallInEditor, Category="BezierSet|IO") void ImportCurveSetJson();
	UFUNCTION(BlueprintCallable, Category="BezierSet|IO") void UI_ImportCurveSetJson();
	UFUNCTION(BlueprintCallable, Category="BezierSet|IO") void UI_ExportCurveSetJson();
	UFUNCTION(CallInEditor, Category="BezierSet|Manage") void ClearSpawned();
	UFUNCTION(BlueprintCallable, Category="BezierSet|Manage") void UI_ClearSpawned();

	// Runtime helpers (easy to call from UMG)
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetEditModeForAll(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetActorVisibleForAll(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowControlPointsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowStripForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowCubeStripForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetStripSizeForAll(float InWidth, float InThickness);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetControlPointSizeForAll(float InVisualScale);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetControlPointColorsForAll(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetSnapToGridForAll(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetGridSizeForAll(float InGridSizeCm);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetGridOriginWorldForAll(FVector InOrigin);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetGridExtentForAll(float InGridExtentCm);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetGridColorForAll(FLinearColor InColor);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetGridBaseAlphaForAll(float InAlpha);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowGridXYForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowGridXZForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowGridYZForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_CycleGridSizeForAll();
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetForcePlanarForAll(bool bInForce);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetForcePlanarAxisForAll(EBezierPlanarAxis InAxis);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetLockToLocalXYForAll(bool bInLock);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowGridForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_ResetCurveStateForAll();
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetEditInteractionEnabledForAll(bool bEnabled, bool bShowControlPoints = true, bool bShowStrip = true);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Spawned;
	FTimerHandle AutoSaveTimerHandle;

	void RefreshSpawnedFromWorld();
	FString MakeAbs(const FString& FileName) const;
	bool ReadText(const FString& AbsPath, FString& Out) const;
	bool WriteText(const FString& AbsPath, const FString& In) const;
	bool IsAnyEditModeActive() const;
	void HandleAutoSave();
};
