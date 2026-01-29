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

UENUM(BlueprintType)
enum class EBezierPivotHandle : uint8
{
	None UMETA(DisplayName = "None"),
	TranslateX UMETA(DisplayName = "Translate X"),
	TranslateY UMETA(DisplayName = "Translate Y"),
	TranslateZ UMETA(DisplayName = "Translate Z"),
	RotateX UMETA(DisplayName = "Rotate X"),
	RotateY UMETA(DisplayName = "Rotate Y"),
	RotateZ UMETA(DisplayName = "Rotate Z")
};
