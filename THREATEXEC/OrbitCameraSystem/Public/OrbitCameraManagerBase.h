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
class AOrbitWalkCharacter;

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

	// Enables standalone walk mode usage with no orbit camera dependency.
	// When enabled the pawn starts in walk mode and does not transition back to orbit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bWalkModeOnly = false;

	// If true, starts the pawn in walk out mode on BeginPlay.
	// Ignored when bWalkModeOnly is enabled because walk mode is forced on.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bStartInWalkOutMode = false;

	// Camera that remains the source orbit camera when switching to walk out.
	// Edit this on placed level instances (not class defaults) so actor picking works as expected.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	TObjectPtr<AOrbitCameraBase> ActiveOrbitCamera = nullptr;

	// If true and ActiveOrbitCamera is unset, manager will auto-bind to the first matching OrbitCameraBase in the level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bAutoFindOrbitCameraIfUnset = true;

	// Optional tag filter for auto-discovery. If set, manager prefers cameras with this tag.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode",
		meta = (EditCondition = "bAutoFindOrbitCameraIfUnset"))
	FName AutoFindOrbitCameraTag = NAME_None;

	// If true, manager will possess itself with PlayerController(0) on BeginPlay for plug-and-play setup.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bAutoPossessPlayer0OnBeginPlay = true;

	// If true, manager updates player view target automatically when entering/exiting walk mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bAutoManageViewTarget = true;

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

	// If true, uses a first-person style acceleration/deceleration model instead of instant movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bUseFirstPersonWalkModel = true;

	// Horizontal acceleration in cm/s^2 for first-person walk model.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bUseFirstPersonWalkModel"))
	float WalkAcceleration = 2200.0f;

	// Horizontal braking deceleration in cm/s^2 when no movement input.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bUseFirstPersonWalkModel"))
	float WalkBrakingDeceleration = 2600.0f;

	// Vertical movement speed (when vertical input is enabled).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkVerticalSpeed = 300.0f;

	// Apply sweep collision when moving in walk mode (if root has collision enabled).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bWalkSweepCollision = true;

	// If true, walk movement uses only planar forward/right (character-like movement without flying when looking up/down).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bWalkPlanarMovement = true;

	// If false, vertical walk input (E/Q) is ignored for character-like ground movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bAllowWalkVerticalInput = false;

	// If true and the game starts in walk mode, spawn at manager placed transform instead of orbit camera transform.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bWalkSpawnFromManagerPlacement = true;

	// Character class used for walk mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	TSubclassOf<AOrbitWalkCharacter> WalkCharacterClass;

	// If true, walk character actor is destroyed when leaving walk mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut")
	bool bDestroyWalkCharacterOnExit = false;

	// Fast movement speed while sprint input is active.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkSprintMultiplier = 2.0f;

	// Mouse/controller yaw speed multiplier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkLookYawSpeed = 120.0f;

	// Mouse/controller pitch speed multiplier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|WalkOut", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WalkLookPitchSpeed = 120.0f;

	// Orbit mouse look speed while holding orbit-look key.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitLookYawSpeed = 100.0f;

	// Orbit mouse pitch speed while holding orbit-look key.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitLookPitchSpeed = 100.0f;

	// Orbit pan speed while holding orbit-pan key.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitPanSpeed = 0.6f;

	// Orbit zoom speed per mouse wheel tick.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float OrbitZoomStep = 25.0f;

	// Smoothly interpolate orbit look/pan/zoom instead of applying instant jumps.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls")
	bool bSmoothOrbitControls = true;

	// Interp speed for orbit look rotation.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bSmoothOrbitControls"))
	float OrbitLookSmoothingSpeed = 12.0f;

	// Interp speed for orbit panning movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bSmoothOrbitControls"))
	float OrbitPanSmoothingSpeed = 10.0f;

	// Interp speed for orbit wheel zoom movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|OrbitControls",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bSmoothOrbitControls"))
	float OrbitZoomSmoothingSpeed = 10.0f;

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

	// Hold to rotate orbit camera with mouse movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey OrbitLookHoldKey = EKeys::RightMouseButton;

	// Hold to pan orbit origin with mouse movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey OrbitPanHoldKey = EKeys::MiddleMouseButton;

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

	// Explicitly set walk out mode state (supports walk-only levels without orbit cameras).
	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Mode")
	void SetWalkOutModeEnabled(bool bEnable);

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
	void ApplyDesiredViewTarget();
	void EnsureOrbitTargetsInitialized();
	void UpdateOrbitCameraSmoothing(float DeltaTime);
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
	void SetOrbitLookPressed();
	void SetOrbitLookReleased();
	void SetOrbitPanPressed();
	void SetOrbitPanReleased();
	void OnOrbitMouseWheel(float Value);

	void OnLookYaw(float Value);
	void OnLookPitch(float Value);

	FVector WalkBoundsOrigin = FVector::ZeroVector;
	FVector WalkModeStartLocation = FVector::ZeroVector;
	FRotator WalkModeStartRotation = FRotator::ZeroRotator;
	FTransform ManagerPlacedTransform = FTransform::Identity;
	FVector WalkVelocity = FVector::ZeroVector;
	TObjectPtr<AOrbitWalkCharacter> WalkCharacterInstance = nullptr;

	bool bMoveForwardPressed = false;
	bool bMoveBackwardPressed = false;
	bool bMoveRightPressed = false;
	bool bMoveLeftPressed = false;
	bool bMoveUpPressed = false;
	bool bMoveDownPressed = false;
	bool bSprintPressed = false;
	bool bOrbitLookPressed = false;
	bool bOrbitPanPressed = false;
	bool bOrbitTargetsInitialized = false;

	FVector OrbitTargetRootLocation = FVector::ZeroVector;
	FRotator OrbitTargetRootRotation = FRotator::ZeroRotator;
	float OrbitTargetDistance = 0.0f;
	float OrbitTargetFocalLength = 0.0f;

	bool bTransitionInProgress = false;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
	FTransform TransitionStart = FTransform::Identity;
	FTransform TransitionTarget = FTransform::Identity;
};
