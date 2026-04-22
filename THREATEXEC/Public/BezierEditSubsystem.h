#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "BezierEditable.h"
#include "BezierRuntimeTypes.h"
#include "BezierEditSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBezierFocusChanged, AActor*, FocusedActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBezierMirrorAxisCycleReset);


struct FBezierCurveActorSnapshot
{
	TWeakObjectPtr<AActor> Actor;
	TSubclassOf<AActor> ActorClass;
	TWeakObjectPtr<AActor> Owner;
	FTransform Transform = FTransform::Identity;

	bool bIs2D = false;
	bool bIs3D = false;
	TArray<FVector2D> Control2D;
	TArray<FVector> Control3D;

	float Scale = 1.0f;
	float ControlPointVisualScale = 1.0f;
	bool bClosedLoop = false;
	bool bEnableRuntimeEditing = true;
	bool bHideVisualsWhenNotEditing = true;
	bool bActorVisibleInGame = true;
	bool bShowControlPoints = true;
	bool bShowStrip = true;
	bool bUseCubeStrip = true;
	bool bSnapToGrid = false;
	bool bShowGrid = false;
	float GridSizeCm = 10.0f;
	float GridExtentCm = 250.0f;
	FVector GridOriginWorld = FVector::ZeroVector;
	FLinearColor GridColor = FLinearColor::White;
	float GridBaseAlpha = 0.15f;
	bool bShowGridXY = true;
	bool bShowGridXZ = true;
	bool bShowGridYZ = true;
	bool bLockToLocalXY = false;
	bool bForcePlanar = false;
	EBezierPlanarAxis ForcePlanarAxis = EBezierPlanarAxis::None;
	EBezierSamplingMode SamplingMode = EBezierSamplingMode::Parametric;
	int32 SampleCount = 64;
	bool bShowSamplePoints = false;
	bool bShowDeCasteljauLevels = false;
	double ProofT = 0.5;
	int32 SelectedControlPointIndex = INDEX_NONE;
	bool bSelectAllControlPoints = false;
};

struct FBezierHistorySnapshot
{
	TArray<FBezierCurveActorSnapshot> Curves;
	int32 FocusedIndex = INDEX_NONE;
};

UCLASS()
class THREATEXEC_API UBezierEditSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Registry (actors call these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void RegisterEditable(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void UnregisterEditable(AActor* Actor);

	// Focus
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void SetFocused(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	AActor* GetFocused() const;

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	bool HasFocused() const;

	UPROPERTY(BlueprintAssignable, Category="Bezier|Edit")
	FOnBezierFocusChanged OnFocusChanged;

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|History")
	bool History_Undo();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|History")
	bool History_Redo();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|History")
	void History_Clear();

	FBezierHistorySnapshot CaptureHistorySnapshot() const;
	bool History_CommitInteractiveChange(const FBezierHistorySnapshot& BeforeSnapshot);

	// Focus-only (UMG calls these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetEditMode(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleEditMode();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetActorVisibleInGame(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleActorVisibleInGame();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowControlPoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleShowControlPoints();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowStrip(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleShowStrip();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetControlPointSize(float InVisualScale);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetStripSize(float InWidth, float InThickness);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetControlPointColors(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetSnapToGrid(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleSnapToGrid();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetGridSize(float InGridSizeCm);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetForcePlanar(bool bInForce);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") EBezierPlanarAxis Focus_CycleForcePlanarAxis();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ResetCurveState();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_AddControlPointAfterSelected();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_DeleteSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_DuplicateSelectedControlPoint();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_CenterCurve();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_MirrorCurve();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ReverseControlOrder();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleClosedLoop();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetClosedLoop(bool bInClosed);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_IsClosedLoop();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_DuplicateCurve();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_IsolateCurve(bool bInIsolate);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleIsolateCurve();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetEditInteractionEnabled(bool bEnabled, bool bShowControlPoints = true, bool bShowStrip = true);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetSamplingMode(EBezierSamplingMode InMode);
UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") EBezierSamplingMode Focus_GetSamplingMode();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetSampleCount(int32 InCount);
UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") int32 Focus_GetSampleCount();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetProofT(double InT);
UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") double Focus_GetProofT();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowSamplePoints(bool bInShow);
UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_GetShowSamplePoints();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowDeCasteljauLevels(bool bInShow);
UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_GetShowDeCasteljauLevels();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") bool Focus_EnsureFocused();

	// Mirror cycle helpers
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ResetMirrorAxisCycle();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") float Focus_GetMirrorAxisCycleSecondsRemaining() const;

	UPROPERTY(BlueprintAssignable, Category="Bezier|Edit|Focused")
	FOnBezierMirrorAxisCycleReset OnMirrorAxisCycleReset;

	// All curves (UMG calls these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetEditMode(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleEditMode();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetActorVisibleInGame(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleActorVisibleInGame();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetShowControlPoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleShowControlPoints();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetShowStrip(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleShowStrip();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetSnapToGrid(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleSnapToGrid();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetGridSize(float InGridSizeCm);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetSampleCount(int32 InCount);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetProofT(double InT);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetEditInteractionEnabled(bool bEnabled, bool bShowControlPoints = true, bool bShowStrip = true);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ExportCurveSetJson();
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ImportCurveSetJson();

	// Routing
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void SetApplyAllToFocusedOnly(bool bInFocusedOnly);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void ToggleApplyAllToFocusedOnly();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	bool GetApplyAllToFocusedOnly() const;

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void SetAutoFocusFirstEditable(bool bInAutoFocus);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	bool GetAutoFocusFirstEditable() const;

	void SetMaxUndoSteps(int32 InMaxUndoSteps) { MaxUndoSteps = FMath::Max(1, InMaxUndoSteps); TrimHistoryStacks(); }
	int32 GetMaxUndoSteps() const { return MaxUndoSteps; }

private:
	UPROPERTY(Transient) TWeakObjectPtr<AActor> FocusedActor;
	UPROPERTY(Transient) TArray<TWeakObjectPtr<AActor>> Editables;
	UPROPERTY(EditAnywhere, Category="Bezier|Edit")
	bool bApplyAllToFocusedOnly = true;
	UPROPERTY(EditAnywhere, Category="Bezier|Edit")
	bool bAutoFocusFirstEditable = true;
	UPROPERTY(EditAnywhere, Category="Bezier|Edit|History", meta=(ClampMin="1", UIMin="1"))
	int32 MaxUndoSteps = 10;
	UPROPERTY(Transient)
	bool bIsolateFocusedCurve = false;
	UPROPERTY(Transient)
	int32 MirrorAxisCycleIndex = 0;
	UPROPERTY(EditAnywhere, Category="Bezier|Edit")
	float MirrorAxisCycleResetDelaySeconds = 10.0f;
	UPROPERTY(Transient)
	float LastMirrorAxisCycleTimeSeconds = -1.0f;
	FTimerHandle MirrorAxisCycleResetHandle;

private:
	bool IsEditable(AActor* Actor) const;
	void CompactRegistry();
	void ForFocused(TFunctionRef<void(UObject* Obj)> Fn);
	void ForAll(TFunctionRef<void(UObject* Obj)> Fn);
	void ApplyIsolateVisibility();

	bool CaptureCurveSnapshot(AActor* Actor, FBezierCurveActorSnapshot& Out) const;
	bool RestoreHistorySnapshot(const FBezierHistorySnapshot& Snapshot);
	bool AreHistorySnapshotsEquivalent(const FBezierHistorySnapshot& A, const FBezierHistorySnapshot& B) const;
	void PushUndoSnapshotIfDifferent(const FBezierHistorySnapshot& Snapshot);
	void TrimHistoryStacks();

	TArray<FBezierHistorySnapshot> UndoHistory;
	TArray<FBezierHistorySnapshot> RedoHistory;
};
