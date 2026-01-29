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
enum class EBezierTransformGizmoMode : uint8
{
	Translate UMETA(DisplayName = "Translate"),
	Rotate UMETA(DisplayName = "Rotate"),
	Scale UMETA(DisplayName = "Scale"),
	Pivot UMETA(DisplayName = "Edit Pivot")
};

UENUM(BlueprintType)
enum class EBezierTransformGizmoSpace : uint8
{
	World UMETA(DisplayName = "World"),
	Local UMETA(DisplayName = "Local")
};

UENUM(BlueprintType)
enum class EBezierTransformHandle : uint8
{
	None UMETA(DisplayName = "None"),
	TranslateX UMETA(DisplayName = "Translate X"),
	TranslateY UMETA(DisplayName = "Translate Y"),
	TranslateZ UMETA(DisplayName = "Translate Z"),
	TranslateXY UMETA(DisplayName = "Translate XY"),
	TranslateXZ UMETA(DisplayName = "Translate XZ"),
	TranslateYZ UMETA(DisplayName = "Translate YZ"),
	RotateX UMETA(DisplayName = "Rotate X"),
	RotateY UMETA(DisplayName = "Rotate Y"),
	RotateZ UMETA(DisplayName = "Rotate Z"),
	ScaleX UMETA(DisplayName = "Scale X"),
	ScaleY UMETA(DisplayName = "Scale Y"),
	ScaleZ UMETA(DisplayName = "Scale Z"),
	ScaleUniform UMETA(DisplayName = "Scale Uniform")
};

USTRUCT(BlueprintType)
struct FBezierTransformGizmoSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "1.0"))
	float AxisLength = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float AxisThickness = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float ArrowSize = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float PlaneHandleSize = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.0"))
	float PlaneHandleOffset = 16.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "1.0"))
	float RotateRadius = 28.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float RotateThickness = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float ScaleHandleSize = 4.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.0"))
	float ScaleHandleOffset = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float UniformScaleRadius = 4.5f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.1"))
	float CenterRadius = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "1.0"))
	float ViewScaleDistance = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.05"))
	float MinViewScale = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Gizmo", meta = (ClampMin = "0.05"))
	float MaxViewScale = 3.0f;
};
