// Copyright 2020 RealVisStudios. All Rights Reserved.

#pragma once

// OrbitCameraSystem includes
#include "OrbitCameraBase.h"

// Unreal Includes
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

#include "OrbitCameraManagerBase.generated.h"

//////////////////////////////////////////////////////////////////////////

// Defines the Transition for selecting an OrbitCamera
UENUM(BlueprintType)
enum class EOrbitCameraTransition : uint8
{
	//Hard Cut without effects
	OC_HardCut = 0 UMETA(DisplayName = "HardCut"),

	//Transition Effects (Positional Transition, Blur, Fade)
	OC_Transition UMETA(DisplayName = "Transition"),
};

// A OrbitCamera Transition State
UENUM(BlueprintType)
enum class ETransitionState : uint8
{
	//Transition is not in Progress
	Transition_Finished = 0 UMETA(DisplayName = "Finished"),

	//Transition is in Progress
	Transition_InProgress UMETA(DisplayName = "InProgress"),
};

// Holds OrbitCamera Tags
USTRUCT(BlueprintType)
struct FOrbitCameraTags
{
	GENERATED_BODY()

public:
	// The Tags which should be included when searching for a OrbitCamera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	TArray<FName> IncludingTags;

	// The Tags which should not be included when searching for a OrbitCamera
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	TArray<FName> ExcludingTags;
};

// The Base OrbitCamera Manager Pawn
// This is used to control the OrbitCameras and transition between them.
UCLASS()
class ORBITCAMERASYSTEM_API AOrbitCameraManagerBase : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AOrbitCameraManagerBase();

#pragma region Transition

	// Holds the camera definition for the start of the transition
	UPROPERTY(BlueprintReadWrite, Category = "Internal|Transition")
	FOrbitCameraDefinition StartCameraDefinition;

	// Holds the camera definition for the target/end of the transition
	UPROPERTY(BlueprintReadWrite, Category = "Internal|Transition")
	FOrbitCameraDefinition TargetCameraDefinition;

	// Holds the camera definition for the current state of the transition
	UPROPERTY(BlueprintReadWrite, Category = "Internal|Transition")
	FOrbitCameraDefinition CurrentCameraDefinition;

#pragma endregion

#pragma region OrbitAndWalkMode

	// Camera that remains the source orbit camera when switching to walk out.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	AOrbitCameraBase* ActiveOrbitCamera = nullptr;

	// True while player is in walk out mode.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OrbitCamera|Mode")
	bool bIsWalkOutMode = false;

	// When true, player can toggle between orbit and walk out mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bAllowWalkOutMode = true;

	// Duration used for mode transition blending.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ModeTransitionDuration = 0.35f;

	// Movement speed in walk out mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkMoveSpeed = 350.0f;

	// Fast movement speed while sprint input is active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkSprintMultiplier = 2.0f;

	// Mouse/controller yaw speed multiplier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkLookYawSpeed = 120.0f;

	// Mouse/controller pitch speed multiplier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkLookPitchSpeed = 120.0f;

	// Clamp pitch while walking to keep controls predictable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float WalkMinPitch = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float WalkMaxPitch = 80.0f;

	// Optional local bounds around walk origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut|Bounds")
	bool bEnableWalkBounds = false;

	// Axis-aligned extents in centimeters around walk origin.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnableWalkBounds"))
	FVector WalkBoundsExtent = FVector(1000.0f, 1000.0f, 500.0f);

	// Optional hard actor bounds for walk mode. Requires a BoxComponent on actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut|Bounds",
		meta = (EditCondition = "bEnableWalkBounds"))
	AActor* WalkBoundsActor = nullptr;

#pragma endregion

#pragma region Keybinds

	// Default keybinds are coded here so BP/UE input setup stays straightforward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey ToggleWalkOutKey = EKeys::Tab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveForwardKey = EKeys::W;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveBackwardKey = EKeys::S;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveRightKey = EKeys::D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveLeftKey = EKeys::A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveUpKey = EKeys::E;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey MoveDownKey = EKeys::Q;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey SprintKey = EKeys::LeftShift;

#pragma endregion

	// Switches between orbit and walk out mode.
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Mode")
	void ToggleWalkOutMode();

	// Enter walk out mode without changing ActiveOrbitCamera settings.
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Mode")
	void EnterWalkOutMode();

	// Return to orbit camera mode.
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Mode")
	void ExitWalkOutMode();

	// Assign active orbit camera and move manager to it.
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Transition")
	void CutToOrbitCamera(AOrbitCameraBase* NewOrbitCamera);

	// Transition manager transform to orbit camera over duration.
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Transition")
	void TransitionToOrbitCamera(AOrbitCameraBase* NewOrbitCamera, float Duration = 0.5f);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void UpdateWalkOutMovement(float DeltaTime);
	void UpdateModeTransition(float DeltaTime);
	void ClampWalkLocation(FVector& InOutLocation) const;
	void CaptureOrbitCameraTransform(FOrbitCameraDefinition& OutDefinition, AOrbitCameraBase* InCamera) const;

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

	void OnLookYaw(float Value);
	void OnLookPitch(float Value);

	FVector WalkBoundsOrigin = FVector::ZeroVector;
	FVector WalkModeStartLocation = FVector::ZeroVector;
	FRotator WalkModeStartRotation = FRotator::ZeroRotator;

	bool bMoveForwardPressed = false;
	bool bMoveBackwardPressed = false;
	bool bMoveRightPressed = false;
	bool bMoveLeftPressed = false;
	bool bMoveUpPressed = false;
	bool bMoveDownPressed = false;
	bool bSprintPressed = false;

	bool bTransitionInProgress = false;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
	FTransform TransitionStart = FTransform::Identity;
	FTransform TransitionTarget = FTransform::Identity;
};
