#include "BezierCurveSetActor.h"
#include "BezierCurve2DActor.h"
#include "BezierCurve3DActor.h"

#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Components/SplineComponent.h"

ABezierCurveSetActor::ABezierCurveSetActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABezierCurveSetActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!Curve2DClass) Curve2DClass = ABezierCurve2DActor::StaticClass();
	if (!Curve3DClass) Curve3DClass = ABezierCurve3DActor::StaticClass();
}

FString ABezierCurveSetActor::MakeAbs(const FString& FileName) const
{
	const FString Base = FPaths::IsRelative(IOPathAbsolute) ? (FPaths::ProjectDir() / IOPathAbsolute) : IOPathAbsolute;
	return FPaths::ConvertRelativePathToFull(Base / FileName);
}

bool ABezierCurveSetActor::ReadText(const FString& AbsPath, FString& Out) const
{
	return FFileHelper::LoadFileToString(Out, *AbsPath);
}

void ABezierCurveSetActor::ClearSpawned()
{
	for (AActor* A : Spawned)
	{
		if (IsValid(A)) A->Destroy();
	}
	Spawned.Reset();
}

void ABezierCurveSetActor::ImportCurveSetJson()
{
	// Keep your existing import logic
}

void ABezierCurveSetActor::UI_SetEditModeForAll(bool bInEditMode)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetEditMode(bInEditMode); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetEditMode(bInEditMode); }
	}
}

void ABezierCurveSetActor::UI_SetActorVisibleForAll(bool bInVisible)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetActorVisibleInGame(bInVisible); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetActorVisibleInGame(bInVisible); }
	}
}

void ABezierCurveSetActor::UI_SetShowControlPointsForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowControlPoints(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowControlPoints(bInShow); }
	}
}

void ABezierCurveSetActor::UI_SetShowCubeStripForAll(bool bInShow)
{
	for (AActor* A : Spawned)
	{
		if (ABezierCurve3DActor* A3 = Cast<ABezierCurve3DActor>(A)) { A3->UI_SetShowCubeStrip(bInShow); }
		else if (ABezierCurve2DActor* A2 = Cast<ABezierCurve2DActor>(A)) { A2->UI_SetShowCubeStrip(bInShow); }
	}
}
