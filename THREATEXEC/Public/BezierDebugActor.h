// BezierDebugActor.h : Runtime debug toggles for Bezier editing and IO
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierRuntimeTypes.h"
#include "BezierDebugActor.generated.h"

class ABezierCurveSetActor;

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
	bool bShowControlPoints = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bShowStrip = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bUseCubeStrip = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.001"))
	float ControlPointSize = 0.06f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.001"))
	float StripWidth = 10.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.001"))
	float StripThickness = 2.0f;

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

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	FVector GridOriginWorld = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bLockToLocalXY = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bForcePlanar = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	EBezierPlanarAxis ForcePlanarAxis = EBezierPlanarAxis::XY;

	// Debug draw in curve actors
	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bShowControlPolygon = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bPulseDebugLines = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DebugPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DebugPulseMaxAlpha = 0.5f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float DebugPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bPulseControlPoints = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float ControlPointPulseMinScale = 0.333f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float ControlPointPulseMaxScale = 1.333f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ControlPointPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ControlPointPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float ControlPointPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bPulseStrip = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float StripPulseMinWidth = 0.2f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float StripPulseMaxWidth = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float StripPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.001"))
	float StripPulseMaxThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StripPulseMinAlpha = 0.4f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float StripPulseMaxAlpha = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float StripPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bPulseGrid = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bOverrideVisualSizes = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals")
	bool bEnableVisualFade = true;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float VisualFadeSpeed = 6.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GridPulseMinAlpha = 0.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.0", ClampMax="1.0"))
	float GridPulseMaxAlpha = 0.15f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float GridPulseSpeed = 1.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float GridPulseMinThickness = 0.5f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Visuals", meta=(ClampMin="0.01"))
	float GridPulseMaxThickness = 1.5f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|Subsystem")
	bool bApplyAllToFocusedOnly = true;

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ApplyDebugSettings();

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ExportCurveSet();

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ImportCurveSet();

	UFUNCTION(BlueprintCallable, Category="Bezier|Debug")
	void ResetAllCurves();

protected:
	virtual void BeginPlay() override;

private:
	void ApplyControllerDebug() const;
	void ApplyCurveSetDebug(ABezierCurveSetActor* CurveSet) const;
	void ApplyCurveSetDebugAll() const;
	void ApplyCurveActorDebug() const;
	void ApplySubsystemDebug() const;
};
