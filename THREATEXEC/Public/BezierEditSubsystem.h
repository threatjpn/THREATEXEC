#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BezierEditable.h"
#include "BezierEditSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBezierFocusChanged, AActor*, FocusedActor);

UCLASS()
class THREATEXEC_API UBezierEditSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Registry (actors call these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void RegisterEditable(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void UnregisterEditable(AActor* Actor);

	// Focus
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void SetFocused(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	AActor* GetFocused() const;

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	bool HasFocused() const;

	UPROPERTY(BlueprintAssignable, Category="Bezier|Edit")
	FOnBezierFocusChanged OnFocusChanged;

	// Focus-only (UMG calls these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetEditMode(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleEditMode();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetActorVisibleInGame(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleActorVisibleInGame();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowControlPoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleShowControlPoints();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetShowStrip(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleShowStrip();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetControlPointSize(float InVisualScale);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetStripSize(float InWidth, float InThickness);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetControlPointColors(FLinearColor Normal, FLinearColor Hover, FLinearColor Selected);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetSnapToGrid(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ToggleSnapToGrid();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetGridSize(float InGridSizeCm);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_SetForcePlanar(bool bInForce);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|Focused") void Focus_ResetCurveState();

	// All curves (UMG calls these)
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetEditMode(bool bInEditMode);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleEditMode();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetActorVisibleInGame(bool bInVisible);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleActorVisibleInGame();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetShowControlPoints(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleShowControlPoints();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetShowStrip(bool bInShow);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleShowStrip();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetSnapToGrid(bool bInSnap);
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_ToggleSnapToGrid();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit|All") void All_SetGridSize(float InGridSizeCm);

	// Routing
	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void SetApplyAllToFocusedOnly(bool bInFocusedOnly);

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	void ToggleApplyAllToFocusedOnly();

	UFUNCTION(BlueprintCallable, Category="Bezier|Edit")
	bool GetApplyAllToFocusedOnly() const;

private:
	UPROPERTY(Transient) TWeakObjectPtr<AActor> FocusedActor;
	UPROPERTY(Transient) TArray<TWeakObjectPtr<AActor>> Editables;
	UPROPERTY(EditAnywhere, Category="Bezier|Edit")
	bool bApplyAllToFocusedOnly = true;

private:
	bool IsEditable(AActor* Actor) const;
	void CompactRegistry();
	void ForFocused(TFunctionRef<void(UObject* Obj)> Fn);
	void ForAll(TFunctionRef<void(UObject* Obj)> Fn);
};
