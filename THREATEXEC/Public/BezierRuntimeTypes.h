#pragma once

#include "CoreMinimal.h"
#include "BezierRuntimeTypes.generated.h"

UENUM(BlueprintType)
enum class EBezierSamplingMode : uint8
{
	Parametric UMETA(DisplayName = "Parametric"),
	ArcLength  UMETA(DisplayName = "Arc Length")
};

UENUM(BlueprintType)
enum class EBezierPlanarAxis : uint8
{
	None UMETA(DisplayName = "None"),
	XY   UMETA(DisplayName = "XY (Z = 0)"),
	XZ   UMETA(DisplayName = "XZ (Y = 0)"),
	YZ   UMETA(DisplayName = "YZ (X = 0)")
};
