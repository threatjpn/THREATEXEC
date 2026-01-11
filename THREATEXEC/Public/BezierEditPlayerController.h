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

protected:
	// Input mappings (edit in defaults if your camera uses primary/middle click)
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<FName> PrimaryActionNames;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FName CancelActionName = "Cancel";

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDebugTrace = false;

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

	void ReportDebugMessage(const FString& Message)
	{
		if (!bDebugTrace || Message == DebugLastMessage)
		{
			return;
		}

		DebugLastMessage = Message;

		if (GEngine)
		{
			const uint64 Key = static_cast<uint64>(reinterpret_cast<UPTRINT>(this));
			GEngine->AddOnScreenDebugMessage(Key, 2.0f, FColor::Yellow, Message);
		}

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
	FString DebugLastMessage;
};
