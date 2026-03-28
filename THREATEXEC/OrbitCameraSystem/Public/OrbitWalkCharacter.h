#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "OrbitWalkCharacter.generated.h"

class UCameraComponent;
class AOrbitCameraManagerBase;

UCLASS()
class ORBITCAMERASYSTEM_API AOrbitWalkCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOrbitWalkCharacter();

	void InitializeFromManager(
		AOrbitCameraManagerBase* InManager,
		float InWalkSpeed,
		float InSprintMultiplier,
		float InMinPitch,
		float InMaxPitch,
		bool bInAllowVerticalMovement,
		FKey InToggleWalkOutKey,
		FKey InMoveForwardKey,
		FKey InMoveBackwardKey,
		FKey InMoveRightKey,
		FKey InMoveLeftKey,
		FKey InMoveUpKey,
		FKey InMoveDownKey,
		FKey InSprintKey);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void SetMoveForwardPressed();
	void SetMoveForwardReleased();
	void SetMoveBackwardPressed();
	void SetMoveBackwardReleased();
	void SetMoveRightPressed();
	void SetMoveRightReleased();
	void SetMoveLeftPressed();
	void SetMoveLeftReleased();
	void SetMoveUpPressed();
	void SetMoveUpReleased();
	void SetMoveDownPressed();
	void SetMoveDownReleased();
	void SetSprintPressed();
	void SetSprintReleased();
	void ToggleWalkModeFromCharacter();
	void OnLookYaw(float Value);
	void OnLookPitch(float Value);
	void UpdateMovementInput();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OrbitCamera|Walk", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> WalkCamera;

	TObjectPtr<AOrbitCameraManagerBase> OwningManager = nullptr;

	float WalkSpeed = 350.0f;
	float SprintMultiplier = 2.0f;
	float MinPitch = -80.0f;
	float MaxPitch = 80.0f;
	bool bAllowVerticalMovement = false;

	FKey ToggleWalkOutKey = EKeys::Tab;
	FKey MoveForwardKey = EKeys::W;
	FKey MoveBackwardKey = EKeys::S;
	FKey MoveRightKey = EKeys::D;
	FKey MoveLeftKey = EKeys::A;
	FKey MoveUpKey = EKeys::E;
	FKey MoveDownKey = EKeys::Q;
	FKey SprintKey = EKeys::LeftShift;

	bool bMoveForwardPressed = false;
	bool bMoveBackwardPressed = false;
	bool bMoveRightPressed = false;
	bool bMoveLeftPressed = false;
	bool bMoveUpPressed = false;
	bool bMoveDownPressed = false;
	bool bSprintPressed = false;
};
