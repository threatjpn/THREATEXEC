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

	UPROPERTY(EditDefaultsOnly, Category = "Input", meta=(ClampMin="0.05"))
	float DoubleClickTimeSeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugTrace = false;

	// Input
	void Input_PrimaryPressed();
	void Input_PrimaryReleased();
	void Input_Cancel();

	// Hover + trace
	void UpdateHover();
	bool TraceUnderCursor(FHitResult& OutHit) const;
	bool GetMouseRay(FVector& OutOrigin, FVector& OutDirection) const;

	// Drag
	void StartDrag(const FHitResult& Hit);
	void UpdateDrag();
	void StopDrag();

	bool DeprojectMouseToPlane(const FVector& PlanePoint, const FVector& PlaneNormal, FVector& OutWorldPoint) const;

	// Helpers
	void ClearHovered();
	void ClearSelectedOnActor(AActor* Actor) const;
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

	UPROPERTY(Transient)
	bool bDragAllControlPoints = false;

	UPROPERTY(Transient)
	TArray<FVector> DragStartWorldPoints;

	UPROPERTY(Transient)
	float LastPrimaryClickTimeSeconds = -1.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> LastPrimaryClickActor;

	UPROPERTY(Transient)
	int32 LastPrimaryClickIndex = -1;

	UPROPERTY(Transient)
	FString DebugLastMessage;
};
