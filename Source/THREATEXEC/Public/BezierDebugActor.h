// BezierDebugActor.h : Runtime debug toggles for Bezier editing and IO
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierRuntimeTypes.h"
#include "BezierDebugActor.generated.h"

class ABezierCurveSetActor;
class UMaterialInterface;

UCLASS()
class THREATEXEC_API ABezierDebugActor : public AActor
{
	GENERATED_BODY()

public:
	ABezierDebugActor();

	UPROPERTY(EditAnywhere, Category="Bezier|Debug")
	bool bApplyOnBeginPlay = true;

	// Controller debug
	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Controller")
	bool bEnableMouseTraceDebug = false;

	// Curve visuals + runtime edit
	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bEnableEditMode = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bEnableRuntimeEditing = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bActorVisibleInGame = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bHideVisualsWhenNotEditing = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowControlPoints = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowStrip = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bUseCubeStrip = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.0001"))
	float ControlPointSize = 0.0003f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.001"))
	float StripWidth = 0.005f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.001"))
	float StripThickness = 0.005f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FLinearColor ControlPointNormal = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FLinearColor ControlPointHover = FLinearColor(0.2f, 0.6f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FLinearColor ControlPointSelected = FLinearColor(1.0f, 0.35f, 0.2f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bSnapToGrid = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowGrid = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.01"))
	float GridSizeCm = 0.25f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="1.0"))
	float GridExtentCm = 250.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FVector GridOriginWorld = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FLinearColor GridColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GridBaseAlpha = 0.15f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowGridXY = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowGridXZ = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowGridYZ = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bLockToLocalXY = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bForcePlanar = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	EBezierPlanarAxis ForcePlanarAxis = EBezierPlanarAxis::XY;

	// Debug draw in curve actors
	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bShowControlPolygon = true;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bShowSamplePoints = false;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bShowDeCasteljauLevels = false;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bOverridePulseSettings = true;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	FLinearColor DebugLineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	FLinearColor DebugLevelColor = FLinearColor(128.0f / 255.0f, 200.0f / 255.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	FLinearColor DebugResultColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	FLinearColor DebugSamplePointColor = FLinearColor(64.0f / 255.0f, 220.0f / 255.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bPulseDebugLines = true;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMinAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DebugPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float DebugPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float DebugPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float DebugPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bPulseControlPoints = false;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float ControlPointPulseMinScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float ControlPointPulseMaxScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ControlPointPulseMinAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ControlPointPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float ControlPointPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bPulseStrip = false;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float StripPulseMinWidth = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float StripPulseMaxWidth = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float StripPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.001"))
	float StripPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StripPulseMinAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StripPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float StripPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bPulseGrid = false;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bOverrideVisualSizes = true;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals")
	bool bEnableVisualFade = true;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float VisualFadeSpeed = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMinAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GridPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float GridPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float GridPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float GridPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Debug|Visuals", meta = (ClampMin = "0.01"))
	float GridThicknessScale = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float DebugThicknessScale = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bForceVisualsOnTop = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0"))
	int32 VisualTranslucencySortPriority = 1000;


	UPROPERTY(EditAnywhere, Category="Bezier|Debug|History", meta=(ClampMin="1", UIMin="1"))
	int32 UndoMaxSteps = 10;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bOverrideControlPointMaterial = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	UMaterialInterface* ControlPointMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bOverrideStripMaterial = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	UMaterialInterface* StripMaterial = nullptr;

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ApplyDebugSettings();

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void SyncFromWorldState();

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ApplyDebugToCurve(AActor* CurveActor);

protected:
	virtual void BeginPlay() override;

private:
	void ApplyControllerDebug() const;
	void ApplyCurveSetDebug(ABezierCurveSetActor* CurveSet) const;
	void ApplyCurveSetDebugAll() const;
	void ApplyToCurveActor(AActor* CurveActor) const;
	void ApplyCurveActorDebug() const;
};
