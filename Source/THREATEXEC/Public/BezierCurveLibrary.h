/**
 * @file BezierCurveLibrary.h
 * @brief Static Bézier helper functions for arc-length resampling and control hashing.
 */

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BezierCurveLibrary.generated.h"

#ifndef THREATEXEC_API
#define THREATEXEC_API
#endif

/**
 * Blueprint-accessible utility class for shared Bézier curve operations.
 */
UCLASS()
class THREATEXEC_API UBezierCurveLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Builds a cumulative arc-length lookup table from a sampled polyline. */
	static void BuildArcLengthLUT(const TArray<FVector>& Pts, TArray<double>& OutCumulative);

	/** Resamples a sampled polyline into evenly spaced arc-length points. */
	static void UniformResampleByArc(const TArray<FVector>& Pts, int32 Count, TArray<FVector>& OutPts);

	/** Produces a stable hash for 2D control points. */
	static uint32 HashControl(const TArray<FVector2D>& C);

	/** Produces a stable hash for 3D control points. */
	static uint32 HashControl3D(const TArray<FVector>& C);
};
