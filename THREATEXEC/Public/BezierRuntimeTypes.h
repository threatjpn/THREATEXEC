#pragma once

#include "CoreMinimal.h"
#include "BezierRuntimeTypes.generated.h"

UENUM(BlueprintType)
enum class EBezierSamplingMode : uint8
{
	Parametric UMETA(DisplayName = "Parametric"),
	ArcLength  UMETA(DisplayName = "Arc Length")
};
