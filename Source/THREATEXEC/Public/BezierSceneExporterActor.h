#pragma once

/**
 * @file BezierSceneExporterActor.h
 * @brief Utility actor for exporting selected Bézier actors into a single scene JSON payload.
 */
// ============================================================================
// BezierSceneExporterActor.h
// Editor-only utility actor that exports selected Bézier actors (2D/3D)
// into a single scene JSON with per-actor blocks:

// The actor does not modify the scene. It reads current Control arrays by
// asking each curve actor to sync from its spline, then sampling Eval(t).
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierSceneExporterActor.generated.h"

/**
 * @brief Utility actor that exports selected Bézier actors into a single scene-level JSON file.
 */
UCLASS()
class THREATEXEC_API ABezierSceneExporterActor : public AActor
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	ABezierSceneExporterActor();

	// Destination folder (absolute or relative to ProjectDir()).
	UPROPERTY(EditAnywhere, Category="Bezier|Scene Export")
	FString IOPathAbsolute;

	// Number of per-curve samples to export along each curve.
	UPROPERTY(EditAnywhere, Category="Bezier|Scene Export", meta=(ClampMin="2", ClampMax="8192"))
	int32 ExportSamples = 256;

	// Export currently selected ABezierCurve* actors into a single scene JSON.
	UFUNCTION(CallInEditor, Category="Bezier|Scene Export")
	/** Exports the currently selected supported curve actors to JSON. */
	void ExportSelectedToJson();

protected:
	/** Standard actor tick, retained for parity with other toolkit actors. */
	virtual void Tick(float DeltaSeconds) override;

private:
	// Helpers
	FString MakeAbs(const FString& FileName) const;
	bool WriteStringToFile(const FString& AbsPath, const FString& Str) const;
};
