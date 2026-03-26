#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "OrbitWalkingPawn.generated.h"

class UFloatingPawnMovement;
class UCapsuleComponent;
class UCameraComponent;
class AActor;

UCLASS()
class ORBITCAMERASYSTEM_API AOrbitWalkingPawn : public APawn
{
	GENERATED_BODY()

public:
	AOrbitWalkingPawn();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	float MoveSpeed = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	float LookSensitivityYaw = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	float LookSensitivityPitch = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	float MinPitch = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	float MaxPitch = 80.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Walk")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Walk")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk|Bounds")
	bool bClampToBounds = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk|Bounds", meta = (EditCondition = "bClampToBounds"))
	TObjectPtr<AActor> BoundsActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk|Bounds", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bClampToBounds"))
	float BoundsPadding = 30.0f;

	UFUNCTION(BlueprintCallable, Category = "Walk|Bounds")
	void ConfigureMovementBounds(bool bEnableBounds, AActor* InBoundsActor, float InBoundsPadding);

	virtual UPawnMovementComponent* GetMovementComponent() const override;

protected:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	float LookPitch = 0.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void LookYaw(float Value);
	void LookPitchInput(float Value);
	void ClampActorToBounds();
};
