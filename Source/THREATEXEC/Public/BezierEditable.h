/**
 * @file BezierEditable.h
 * @brief Common interface implemented by runtime-editable Bézier actors.
 *
 * The interface standardises the operations required by the selection, edit,
 * and debug systems so that both 2D and 3D curve actors can be driven through
 * the same higher-level tooling.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BezierEditable.generated.h"

/**
 * @brief Marker interface type for runtime-editable Bézier actors.
 */
UINTERFACE(BlueprintType)
class THREATEXEC_API UBezierEditable : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief Contract implemented by Bézier actors so subsystems can drive them generically.
 */
class THREATEXEC_API IBezierEditable
{
	GENERATED_BODY()

public:
	// Identification
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	FName BEZ_GetTypeName() const;

	// Edit mode
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetEditMode(bool bInEditMode);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_GetEditMode() const;

	// In-game visibility (whole actor)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetActorVisibleInGame(bool bInVisible);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_GetActorVisibleInGame() const;

	// Visual toggles
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetShowControlPoints(bool bInShow);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_GetShowControlPoints() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetShowStrip(bool bInShow);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_GetShowStrip() const;

	// Visual params
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetControlPointSize(float InVisualScale);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetStripSize(float InWidth, float InThickness);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetControlPointColors(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected);

	// Snap/grid
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetSnapToGrid(bool bInSnap);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_GetSnapToGrid() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetGridSize(float InGridSizeCm);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	float BEZ_GetGridSize() const;

	// Optional constraints
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_SetForcePlanar(bool bInForce);

	// Reset
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	void BEZ_ResetCurveState();

	// Control point editing
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_AddControlPointAfterSelected();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_DeleteSelectedControlPoint();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Bezier|Editable")
	bool BEZ_DuplicateSelectedControlPoint();
};
