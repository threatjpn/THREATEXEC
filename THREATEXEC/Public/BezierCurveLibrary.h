// ============================================================================
// BezierCurveLibrary.h : small utilities for Bezier curves
// ============================================================================

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BezierCurveLibrary.generated.h"

/**
 * Math helpers used by both 2D and 3D curves:
 * - Arc-length LUT building (piecewise linear on de Casteljau samples)
 * - Uniform-by-length resampling
 * - Simple hashing of control for change detection in-editor
 */
UCLASS()
class THREATEXEC_API UBezierCurveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Build cumulative arc-length (LUT) of a sampled polyline
	static void BuildArcLengthLUT(const TArray<FVector>& Pts, TArray<double>& OutCumulative);

	// Uniform-by-length resampling: returns N points with equal arc distance
	static void UniformResampleByArc(const TArray<FVector>& Pts, int32 Count, TArray<FVector>& OutPts);

	// Hash a set of control points (stable across editor runs)
	static uint32 HashControl(const TArray<FVector2D>& C);
	static uint32 HashControl3D(const TArray<FVector>& C);
};
