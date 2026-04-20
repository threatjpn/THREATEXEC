#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BezierEditSubsystem.h"
#include "BezierEditPlayerController.generated.h"

UCLASS()
class THREATEXEC_API ABezierEditPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABezierEditPlayerController();

	virtual void PlayerTick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Debug")
	void SetDebugTrace(bool bInDebugTrace) { bDebugTrace = bInDebugTrace; }

	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugLastMessage() const { return DebugLastMessage; }

protected:
	// Input mappings (edit in defaults if your camera uses primary/middle click)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<FName> PrimaryActionNames;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FName CancelActionName = "Cancel";

	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.05"))
	float DoubleClickTimeSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugTrace = false;

	// More forgiving mouse hover against tiny control-point instances.
	UPROPERTY(EditAnywhere, Category = "Bezier|Trace", meta = (ClampMin = "0.1"))
	float HoverWorldRadius = 18.0f;

	UPROPERTY(EditAnywhere, Category = "Bezier|Trace", meta = (ClampMin = "1.0"))
	float HoverMaxRayDistance = 100000.0f;

	// Input
	void Input_PrimaryPressed();
	void Input_PrimaryReleased();
	void Input_Cancel();
	void Input_Undo();
	void Input_Redo();

	// Hover + trace
	void UpdateHover();
	bool TraceUnderCursor(FHitResult& OutHit) const;
	bool GetMouseRay(FVector& OutOrigin, FVector& OutDirection) const;
	bool FindClosestControlPointOnMouseRay(AActor*& OutActor, int32& OutIndex) const;

	// Drag
	void StartDrag(const FHitResult& Hit);
	void UpdateDrag();
	void StopDrag();
	void StartDragFromControlPoint(AActor* HitActor, int32 ControlPointIndex, const FVector& ImpactPoint);

	bool DeprojectMouseToPlane(const FVector& PlanePoint, const FVector& PlaneNormal, FVector& OutWorldPoint) const;

	// Helpers
	void ClearHovered();
	void ClearSelectedOnActor(AActor* Actor) const;
	void ClearSelectedOnAllActors() const;
	void SetHoveredOnActor(AActor* Actor, int32 ControlPointIndex) const;

	void ReportDebugMessage(const FString& Message)
	{
		if (!bDebugTrace || Message == DebugLastMessage)
		{
			return;
		}

		DebugLastMessage = Message;

		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	}

private:
	// Runtime-only state. Keep these non-UPROPERTY so Blueprint serialization
	// does not depend on transient editor/runtime interaction internals.
	TWeakObjectPtr<AActor> HoveredActor;
	int32 HoveredIndex = -1;
	TWeakObjectPtr<AActor> DraggedActor;
	int32 DraggedIndex = -1;
	bool bDragging = false;
	FVector DragPlanePoint = FVector::ZeroVector;
	FVector DragPlaneNormal = FVector::UpVector;
	bool bDragAllControlPoints = false;
	TArray<FVector> DragStartWorldPoints;
	float LastPrimaryClickTimeSeconds = -1.0f;
	TWeakObjectPtr<AActor> LastPrimaryClickActor;
	int32 LastPrimaryClickIndex = -1;
	bool bHasDragBeforeSnapshot = false;
	FBezierHistorySnapshot DragBeforeSnapshot;
	FString DebugLastMessage;
};