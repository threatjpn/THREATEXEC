/**
 * @file BezierSelectionSubsystem.h
 * @brief World subsystem that stores the currently focused Bézier actor.
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BezierSelectionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedBezierActorChanged, AActor*, NewFocusedActor);

// Stores currently focused Bezier actor (2D or 3D) for UI and controller.
/**
 * @brief Stores and broadcasts the current focused Bézier actor for UI and controller logic.
 */
UCLASS()
class THREATEXEC_API UBezierSelectionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Default constructor. */
	UBezierSelectionSubsystem();

	// Set focused actor (pass nullptr to clear).
	UFUNCTION(BlueprintCallable, Category="Bezier|Selection")
	void SetFocusedActor(AActor* InActor);

	// Clear focus.
	UFUNCTION(BlueprintCallable, Category="Bezier|Selection")
	void ClearFocusedActor();

	// Get focused actor (may be nullptr).
	UFUNCTION(BlueprintCallable, Category="Bezier|Selection")
	AActor* GetFocusedActor() const;

	// True if a focused actor exists.
	UFUNCTION(BlueprintCallable, Category="Bezier|Selection")
	bool HasFocusedActor() const;

	// Broadcast when focus changes.
	UPROPERTY(BlueprintAssignable, Category="Bezier|Selection")
	FOnFocusedBezierActorChanged OnFocusedBezierActorChanged;

	// Optional: enforce that focus must be a Bezier actor (2D or 3D).
	UPROPERTY(EditAnywhere, Category="Bezier|Selection")
	bool bOnlyAllowBezierActors = true;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FocusedActor;

	bool IsAllowedActor(AActor* InActor) const;
};
