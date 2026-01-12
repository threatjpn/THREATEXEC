#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierRuntimeTypes.h"
#include "BezierEditable.h"
#include "BezierCurve2DActor.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class THREATEXEC_API ABezierCurve2DActor : public AActor, public IBezierEditable
{
	GENERATED_BODY()

public:
	ABezierCurve2DActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	// --- Data & Scaling ---
	UPROPERTY(EditAnywhere, Category = "Bezier2D|Data")
	double Scale = 100.0;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|Data")
	TArray<FVector2D> Control;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|IO")
	FString IOPathAbsolute = TEXT("Saved/Bezier");

	// ---------------------------------------------------------------------
	// Runtime edit mode and visibility (added)
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category="Bezier2D|RuntimeEdit|Mode")
	bool bEnableRuntimeEditing = true;

	UPROPERTY(EditAnywhere, Category="Bezier2D|RuntimeEdit|Mode")
	bool bEditMode = false;

	UPROPERTY(EditAnywhere, Category="Bezier2D|RuntimeEdit|Mode")
	bool bHideVisualsWhenNotEditing = true;

	UPROPERTY(EditAnywhere, Category="Bezier2D|RuntimeEdit|Mode")
	bool bActorVisibleInGame = true;

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	void UI_SetEditMode(bool bInEditMode);

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	void UI_SetActorVisibleInGame(bool bInVisible);

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	bool UI_GetEditMode() const { return bEditMode; }

	// --- Visual: Strip ---
	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	bool bShowStripMesh = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	bool bPulseStrip = false;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.001"))
	float StripPulseMinWidth = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.001"))
	float StripPulseMaxWidth = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.001"))
	float StripPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.001"))
	float StripPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float StripPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float StripPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta=(ClampMin = "0.01"))
	float StripPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	bool bUseCubeStrip = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	float StripWidth = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	float StripThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip")
	UMaterialInterface* StripMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Strip", meta = (ClampMin = "2", ClampMax = "2048"))
	int32 StripSegments = 64;

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	void UI_SetShowStrip(bool bInShow);

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	void UI_SetShowCubeStrip(bool bInShow);

	UFUNCTION(BlueprintCallable, Category="Bezier2D|UI|RuntimeEdit")
	void UI_SetStripSize(float InWidth, float InThickness);

	// --- Visual: Control Points ---
	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	bool bShowControlPoints = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	bool bPulseControlPoints = false;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints", meta=(ClampMin = "0.001"))
	float ControlPointPulseMinScale = 0.333f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints", meta=(ClampMin = "0.001"))
	float ControlPointPulseMaxScale = 1.333f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints", meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float ControlPointPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints", meta=(ClampMin = "0.0", ClampMax = "1.0"))
	float ControlPointPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints", meta=(ClampMin = "0.01"))
	float ControlPointPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	float ControlPointVisualScale = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	UMaterialInterface* ControlPointMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	FLinearColor ControlPointColor = FLinearColor(0.9f,0.9f,0.9f,1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	FLinearColor ControlPointHoverColor = FLinearColor(0.2f,0.6f,1.0f,1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|ControlPoints")
	FLinearColor ControlPointSelectedColor = FLinearColor(1.0f,0.35f,0.2f,1.0f);

	// --- Visual: Fade ---
	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Fade")
	bool bEnableVisualFade = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Visual|Fade", meta = (ClampMin = "0.01"))
	float VisualFadeSpeed = 6.0f;

	// --- Constraints & Snapping ---
	UPROPERTY(EditAnywhere, Category = "Bezier2D|RuntimeEdit")
	bool bSnapToGrid = false;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|RuntimeEdit")
	bool bShowGrid = false;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|RuntimeEdit")
	float GridSizeCm = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|RuntimeEdit")
	bool bLockToLocalXY = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|RuntimeEdit")
	bool bForcePlanar = true;

	// Debug
	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug")
	bool bShowControlPolygon = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug")
	bool bPulseDebugLines = true;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMaxAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.01"))
	float DebugPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug")
	bool bPulseGrid = false;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMaxAlpha = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseMaxThickness = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier2D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double ProofT = 0.5;

	// --- UI Callbacks ---
	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_ResetCurveState();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_SetInitialControlFromCurrent();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_CenterCurve();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_ToggleClosedLoop();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_SetClosedLoop(bool bInClosed);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	bool UI_IsClosedLoop() const;

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_ReverseControlOrder();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_ExportAllJson();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI")
	void UI_ImportFromJson();

	// Runtime edit setters
	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetSnapToGrid(bool bInSnap);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetShowGrid(bool bInShow) { bShowGrid = bInShow; }

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetGridSizeCm(float InGridSizeCm) { GridSizeCm = FMath::Max(0.01f, InGridSizeCm); }

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetLockToLocalXY(bool bInLock) { bLockToLocalXY = bInLock; }

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetForcePlanar(bool bInForce) { bForcePlanar = bInForce; }

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_SelectFromHit(const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_AddControlPoint(FVector2D ModelPos, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_AddControlPointAfterSelected();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_DeleteControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_DeleteSelectedControlPoint();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_DuplicateControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_DuplicateSelectedControlPoint();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetShowControlPoints(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetControlPointSize(float InVisualScale);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetControlPointColors(FLinearColor InNormal, FLinearColor InHover, FLinearColor InSelected);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_SetHoveredControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_ClearHoveredControlPoint();

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_GetControlPointWorld(int32 Index, FVector& OutWorld) const;

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	bool UI_SetControlPointWorld(int32 Index, const FVector& WorldPos);

	UFUNCTION(BlueprintCallable, Category = "Bezier2D|UI|RuntimeEdit")
	void UI_ClearSelectedControlPoint();

	// --------------------------------------------------------------------
	// Core operations (used by tests and tools)
	// --------------------------------------------------------------------
	void SyncControlFromSpline();
	void OverwriteSplineFromControl();
	void ReverseControlOrder();
	void ResampleBezierToSpline();

	bool ExportControlPointsToJson() const;
	bool ExportCurveSamplesToJson() const;
	bool ExportDeCasteljauProofJson() const;

	// Math
	UFUNCTION(BlueprintCallable, Category = "Bezier2D|Math")
	FVector2D Eval(double T) const;

	void RefreshControlPointVisuals();
	void ApplyRuntimeEditVisibility();
	void UpdateControlPointInstanceColors();
	void UpdateStripMesh();
	void UpdateCubeStrip();

protected:
	void EnsureSpline();
	void ReadSplineToControl();
	void WriteControlToSpline();
	FString MakeAbs(const FString& FileName) const;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|Components")
	USplineComponent* Spline = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|Components")
	UInstancedStaticMeshComponent* ControlPointISM = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|Components")
	UProceduralMeshComponent* StripMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|Components")
	UInstancedStaticMeshComponent* CubeStripISM = nullptr;

public:
	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|RuntimeEdit")
	int32 SelectedControlPointIndex = -1;

	UPROPERTY(VisibleAnywhere, Category = "Bezier2D|RuntimeEdit")
	int32 HoveredControlPointIndex = -1;

private:
	UPROPERTY()
	TArray<FVector2D> InitialControl;

	void UpdateVisualFadeTargets();
	void UpdateVisualFade(float DeltaSeconds);
	void UpdateControlPointPulse();
	void UpdateControlPointInstanceScale(float InScale);
	float GetControlPointPulseScale() const;
	float GetControlPointPulseOpacity() const;
	float GetStripPulseAlpha() const;
	float GetStripWidthForRender() const;
	float GetStripThicknessForRender() const;
	float GetStripPulseOpacity() const;
	void UpdateStripPulseOpacity();

	float CachedControlPointScale = -1.0f;
	float CachedControlPointOpacity = -1.0f;
	float CachedStripOpacity = -1.0f;
	float ControlPointFadeAlpha = 1.0f;
	float StripFadeAlpha = 1.0f;
	float TargetControlPointFade = 1.0f;
	float TargetStripFade = 1.0f;

	// Interface
public:
	virtual FName BEZ_GetTypeName_Implementation() const override;

	virtual void BEZ_SetEditMode_Implementation(bool bInEditMode) override;
	virtual bool BEZ_GetEditMode_Implementation() const override;

	virtual void BEZ_SetActorVisibleInGame_Implementation(bool bInVisible) override;
	virtual bool BEZ_GetActorVisibleInGame_Implementation() const override;

	virtual void BEZ_SetShowControlPoints_Implementation(bool bInShow) override;
	virtual bool BEZ_GetShowControlPoints_Implementation() const override;

	virtual void BEZ_SetShowStrip_Implementation(bool bInShow) override;
	virtual bool BEZ_GetShowStrip_Implementation() const override;

	virtual void BEZ_SetControlPointSize_Implementation(float InVisualScale) override;
	virtual void BEZ_SetStripSize_Implementation(float InWidth, float InThickness) override;
	virtual void BEZ_SetControlPointColors_Implementation(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected) override;

	virtual void BEZ_SetSnapToGrid_Implementation(bool bInSnap) override;
	virtual bool BEZ_GetSnapToGrid_Implementation() const override;

	virtual void BEZ_SetGridSize_Implementation(float InGridSizeCm) override;
	virtual float BEZ_GetGridSize_Implementation() const override;

	virtual void BEZ_SetForcePlanar_Implementation(bool bInForce) override;
	virtual void BEZ_ResetCurveState_Implementation() override;
};
