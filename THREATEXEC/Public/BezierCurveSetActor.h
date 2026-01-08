// BezierCurveSetActor.h : Imports a whole set of curves from JSON, spawns child actors
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BezierCurveSetActor.generated.h"

class ABezierCurve2DActor;
class ABezierCurve3DActor;

UCLASS()
class THREATEXEC_API ABezierCurveSetActor : public AActor
{
	GENERATED_BODY()

public:
	ABezierCurveSetActor();

	UPROPERTY(EditAnywhere, Category="BezierSet|Classes")
	TSubclassOf<ABezierCurve2DActor> Curve2DClass;

	UPROPERTY(EditAnywhere, Category="BezierSet|Classes")
	TSubclassOf<ABezierCurve3DActor> Curve3DClass;

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	FString IOPathAbsolute = TEXT("Content");

	UPROPERTY(EditAnywhere, Category="BezierSet|IO")
	FString CurveSetFile = TEXT("curves.json");

	UFUNCTION(CallInEditor, Category="BezierSet|IO") void ImportCurveSetJson();
	UFUNCTION(CallInEditor, Category="BezierSet|Manage") void ClearSpawned();

	// Runtime helpers (easy to call from UMG)
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetEditModeForAll(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetActorVisibleForAll(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowControlPointsForAll(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="BezierSet|RuntimeEdit") void UI_SetShowCubeStripForAll(bool bInShow);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Spawned;

	FString MakeAbs(const FString& FileName) const;
	bool ReadText(const FString& AbsPath, FString& Out) const;
};
