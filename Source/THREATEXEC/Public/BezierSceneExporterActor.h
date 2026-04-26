/**
 * @file BezierSceneExporterActor.h
 * @brief Utility actor for exporting selected Bézier actors into one scene JSON file.
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierSceneExporterActor.generated.h"

#ifndef THREATEXEC_API
#define THREATEXEC_API
#endif

/**
 * Exports selected Bézier curve actors to a scene-level JSON payload.
 */
UCLASS()
class THREATEXEC_API ABezierSceneExporterActor : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the exporter actor and disables ticking. */
	ABezierSceneExporterActor();

	/** Optional output folder. Relative paths resolve under Project/Saved. */
	UPROPERTY(EditAnywhere, Category="Bezier|Scene Export")
	FString IOPathAbsolute;

	/** Number of samples written for each exported curve. */
	UPROPERTY(EditAnywhere, Category="Bezier|Scene Export", meta=(ClampMin="2", ClampMax="8192"))
	int32 ExportSamples = 256;

	/** Exports the currently selected 2D and 3D Bézier actors to JSON. */
	UFUNCTION(CallInEditor, Category="Bezier|Scene Export")
	void ExportSelectedToJson();

private:
	/** Resolves an output file name to an absolute save path. */
	FString MakeAbs(const FString& FileName) const;

	/** Writes text to disk, creating the target directory if needed. */
	bool WriteStringToFile(const FString& AbsPath, const FString& Str) const;
};
