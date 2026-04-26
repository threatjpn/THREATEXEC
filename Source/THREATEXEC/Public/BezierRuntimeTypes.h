/**
 * @file BezierRuntimeTypes.h
 * @brief Shared enums used by the runtime Bézier editing system.
 */

#pragma once

#include "CoreMinimal.h"
#include "BezierRuntimeTypes.generated.h"

/**
 * @brief Supported resampling modes for runtime curve generation.
 */
UENUM(BlueprintType)
enum class EBezierSamplingMode : uint8
{
	Parametric UMETA(DisplayName = "Parametric"),
	ArcLength  UMETA(DisplayName = "Arc Length")
};

/**
 * @brief Plane-selection options used when constraining edits to a specific plane.
 */
UENUM(BlueprintType)
enum class EBezierPlanarAxis : uint8
{
	None UMETA(DisplayName = "None"),
	XY   UMETA(DisplayName = "XY (Z = 0)"),
	XZ   UMETA(DisplayName = "XZ (Y = 0)"),
	YZ   UMETA(DisplayName = "YZ (X = 0)")
};



/**
 * @brief Render-priority presets used by runtime visual components and debug draw.
 */
UENUM(BlueprintType)
enum class EBezierVisualPriority : uint8
{
	World UMETA(DisplayName = "World"),
	Foreground UMETA(DisplayName = "Foreground"),
	Overlay UMETA(DisplayName = "Overlay")
};
