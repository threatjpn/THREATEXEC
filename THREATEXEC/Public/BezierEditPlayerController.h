#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BezierEditPlayerController.generated.h"

UCLASS()
class THREATEXEC_API ABezierEditPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABezierEditPlayerController();

	virtual void PlayerTick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

protected:
	// Input
	void Input_PrimaryPressed();
	void Input_PrimaryReleased();
	void Input_Cancel();

	// Hover + trace
	void UpdateHover();
	bool TraceUnderCursor(FHitResult& OutHit) const;

	// Drag
	void StartDrag(const FHitResult& Hit);
	void UpdateDrag();
	void StopDrag();

	bool DeprojectMouseToPlane(const FVector& PlanePoint, const FVector& PlaneNormal, FVector& OutWorldPoint) const;

	// Helpers
	void ClearHovered();
	void ClearSelectedOnActor(AActor* Actor) const;
	void SetHoveredOnActor(AActor* Actor, int32 ControlPointIndex) const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> HoveredActor;

	UPROPERTY(Transient)
	int32 HoveredIndex = -1;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> DraggedActor;

	UPROPERTY(Transient)
	int32 DraggedIndex = -1;

	UPROPERTY(Transient)
	bool bDragging = false;

	UPROPERTY(Transient)
	FVector DragPlanePoint = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector DragPlaneNormal = FVector::UpVector;
};
