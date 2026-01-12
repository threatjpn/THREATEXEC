// BezierDebugActor.h : Runtime debug toggles for Bezier editing and IO
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit", meta=(ClampMin="0.1"))
	float GridSizeCm = 10.0f;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bLockToLocalXY = false;

	UPROPERTY(EditAnywhere, Category="Bezier|Debug|RuntimeEdit")
	bool bForcePlanar = false;

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
	ABezierCurveSetActor* FindCurveSetActor() const;
	void ApplyControllerDebug() const;
	void ApplyCurveSetDebug(ABezierCurveSetActor* CurveSet) const;
	void ApplyCurveActorDebug() const;
	void ApplySubsystemDebug() const;
};
