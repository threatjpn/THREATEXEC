#pragma once

#include "CoreMinimal.h"
#include "BezierEditPlayerController.h"
#include "BEController.generated.h"

/**
 * Backward-compatibility shim for older Blueprint assets (e.g. BP_BEController)
 * that were authored against ABEController before the class was renamed.
 */
UCLASS(BlueprintType, Blueprintable)
class THREATEXEC_API ABEController : public ABezierEditPlayerController
{
	GENERATED_BODY()
};
