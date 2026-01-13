#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierRuntimeTypes.h"
#include "BezierEditable.h"
#include "BezierCurve3DActor.generated.h"

class USplineComponent;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class THREATEXEC_API ABezierCurve3DActor : public AActor, public IBezierEditable
{
	GENERATED_BODY()

public:
	ABezierCurve3DActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	// --- Data & Scaling ---
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Data")
	double Scale = 100.0;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|Data")
	TArray<FVector> Control;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|IO")
	FString IOPathAbsolute = TEXT("Saved/Bezier");

	// ---------------------------------------------------------------------
	// Runtime edit mode and visibility
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bezier3D|RuntimeEdit|Mode")
	bool bEnableRuntimeEditing = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|RuntimeEdit|Mode")
	bool bEditMode = false;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|RuntimeEdit|Mode")
	bool bHideVisualsWhenNotEditing = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|RuntimeEdit|Mode")
	bool bActorVisibleInGame = true;

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetEditMode(bool bInEditMode);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetActorVisibleInGame(bool bInVisible);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_GetEditMode() const { return bEditMode; }

	// ---------------------------------------------------------------------
	// Visual: strip
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	bool bShowStripMesh = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	bool bPulseStrip = false;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.001"))
	float StripPulseMinWidth = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.001"))
	float StripPulseMaxWidth = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.001"))
	float StripPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.001"))
	float StripPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StripPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StripPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta=(ClampMin="0.01"))
	float StripPulseSpeed = 1.0f;

	// True = instanced cube strip. False = procedural ribbon.
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	bool bUseCubeStrip = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip", meta = (ClampMin = "1", ClampMax = "2048"))
	int32 StripSegments = 64;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	float StripWidth = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	float StripThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Strip")
	UMaterialInterface* StripMaterial = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowStrip(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowCubeStrip(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetStripSize(float InWidth, float InThickness);

	// ---------------------------------------------------------------------
	// Visual: control points
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	bool bShowControlPoints = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	bool bPulseControlPoints = false;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints", meta=(ClampMin="0.001"))
	float ControlPointPulseMinScale = 0.333f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints", meta=(ClampMin="0.001"))
	float ControlPointPulseMaxScale = 1.333f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ControlPointPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ControlPointPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints", meta=(ClampMin="0.01"))
	float ControlPointPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	float ControlPointVisualScale = 0.06f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	UMaterialInterface* ControlPointMaterial = nullptr;

	// Requires material reading PerInstanceCustomData (RGB at indices 0..2)
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	FLinearColor ControlPointColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	FLinearColor ControlPointHoverColor = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|ControlPoints")
	FLinearColor ControlPointSelectedColor = FLinearColor(1.0f, 0.35f, 0.2f, 1.0f);

	// ---------------------------------------------------------------------
	// Visual: Fade
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Fade")
	bool bEnableVisualFade = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Visual|Fade", meta=(ClampMin="0.01"))
	float VisualFadeSpeed = 6.0f;

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowControlPoints(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetControlPointSize(float InVisualScale);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetControlPointColors(FLinearColor InNormal, FLinearColor InHover, FLinearColor InSelected);

	// Hover/selection helpers (controller can call, UI does not need casting)
	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetHoveredControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_ClearHoveredControlPoint();

	// --------------------------------------------------------------------
	// Runtime UI API (BlueprintCallable)
	// --------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ToggleClosedLoop();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_SetClosedLoop(bool bInClosed);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	bool UI_IsClosedLoop() const;

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ResampleParam();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ResampleArcLength();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ReverseControlOrder();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_SyncControlFromSpline();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_OverwriteSplineFromControl();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ExportAllJson();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ImportFromJson();

	// Control point editing
	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_ResetCurveState();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_SetInitialControlFromCurrent();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_CenterCurve();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_MirrorCurveX();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_MirrorCurveY();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI")
	void UI_MirrorCurveZ();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetForcePlanar(bool bInForce);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetForcePlanarAxis(EBezierPlanarAxis InAxis);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_SelectFromHit(const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_AddControlPoint(FVector ModelPos, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_AddControlPointAfterSelected();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_DeleteControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_DeleteSelectedControlPoint();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_DuplicateControlPoint(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_DuplicateSelectedControlPoint();

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetSnapToGrid(bool bInSnap);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowGrid(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetGridSizeCm(float InGridSizeCm);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetGridExtentCm(float InGridExtentCm);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetGridOriginWorld(FVector InOrigin);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetGridColor(FLinearColor InColor);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetGridBaseAlpha(float InAlpha);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowGridXY(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowGridXZ(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetShowGridYZ(bool bInShow);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	void UI_SetLockToLocalXY(bool bInLock);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_GetControlPointWorld(int32 Index, FVector& OutWorld) const;

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
	bool UI_SetControlPointWorld(int32 Index, const FVector& WorldPos);

	UFUNCTION(BlueprintCallable, Category = "Bezier3D|UI|RuntimeEdit")
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

	// Grid flags (exist in your cpp usage)
	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bSnapToGrid = false;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bShowGrid = false;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	float GridSizeCm = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit", meta=(ClampMin="1.0"))
	float GridExtentCm = 250.0f;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	FVector GridOriginWorld = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	FLinearColor GridColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GridBaseAlpha = 0.15f;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bShowGridXY = true;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bShowGridXZ = true;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bShowGridYZ = true;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bLockToLocalXY = false;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	bool bForcePlanar = false;

	UPROPERTY(EditAnywhere, Category="Bezier3D|RuntimeEdit")
	EBezierPlanarAxis ForcePlanarAxis = EBezierPlanarAxis::None;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|RuntimeEdit")
	int32 SelectedControlPointIndex = -1;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|RuntimeEdit")
	int32 HoveredControlPointIndex = -1;

	// Debug
	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug")
	bool bShowControlPolygon = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug")
	bool bPulseDebugLines = true;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMaxAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.01"))
	float DebugPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug")
	bool bPulseGrid = false;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMaxAlpha = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.01"))
	float GridPulseMaxThickness = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug")
	bool bShowLevelsAtT = false;

	UPROPERTY(EditAnywhere, Category = "Bezier3D|Debug", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	double ProofT = 0.5;

	// Math
	UFUNCTION(BlueprintCallable, Category = "Bezier3D|Math")
	FVector Eval(double T) const;

protected:
	void EnsureSpline();
	void ReadSplineToControl();
	void WriteControlToSpline();
	void DrawDebugView();
	void UniformArcLengthSample3D(int32 TargetCount, TArray<FVector>& Out) const;

	bool WriteJson(const FString& AbsPath, const FString& Str) const;
	bool ReadJson(const FString& AbsPath, FString& OutStr) const;
	FString MakeAbs(const FString& FileName) const;

	void RefreshControlPointVisuals();
	void ApplyRuntimeEditVisibility();
	void UpdateControlPointInstanceColors();
	void UpdateStripMesh();
	void UpdateCubeStrip();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|Components")
	USplineComponent* Spline = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|Components")
	UInstancedStaticMeshComponent* ControlPointISM = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|Components")
	UProceduralMeshComponent* StripMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Bezier3D|Components")
	UInstancedStaticMeshComponent* CubeStripISM = nullptr;

private:
	UPROPERTY()
	TArray<FVector> InitialControl;

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

	// ---- Interface (zero casting in UMG) ----
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
