// Copyright 2020 RealVisStudios. All Rights Reserved.

#pragma once

// OrbitCameraSystem includes
#include "OrbitCameraBase.h"

// Unreal Includes
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputCoreTypes.h"

class UBoxComponent;

#include "OrbitCameraManagerBase.generated.h"

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

#pragma region OrbitMode

	// Camera currently controlled by the manager.
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

	// If true, manager updates player view target automatically to the active orbit camera.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode")
	bool bAutoManageViewTarget = true;

	// Duration used for camera transition blending.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Mode", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ModeTransitionDuration = 0.35f;

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

	// If true, the manager constrains orbit root/camera movement against the active camera bounds each frame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds")
	bool bEnforceOrbitBoundsInManager = true;

	// If true, the orbit root itself is clamped to the bounds volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (EditCondition = "bEnforceOrbitBoundsInManager"))
	bool bConstrainOrbitRootToBounds = true;

	// If true, the actual camera position is also kept inside the bounds volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (EditCondition = "bEnforceOrbitBoundsInManager"))
	bool bConstrainOrbitCameraToBounds = true;

	// If true, look-bound collisions apply a temporary push offset to the orbit root instead of sliding the camera forward.
	// The push offset is visual only and relaxes back to zero when the view rotates back into free space.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (EditCondition = "bEnforceOrbitBoundsInManager && bConstrainOrbitCameraToBounds"))
	bool bUseTransientBoundsPushForLook = true;

	// Speed used when pushing the camera away from a bound while rotating into it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseTransientBoundsPushForLook && bSmoothOrbitControls"))
	float LookBoundsPushInterpSpeed = 14.0f;

	// Speed used when returning from the temporary look-bounds push after rotating away from the bound.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseTransientBoundsPushForLook && bSmoothOrbitControls"))
	float LookBoundsReturnSpeed = 6.0f;

	// Small inset used when computing the pushed camera position so the view stays just inside the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseTransientBoundsPushForLook"))
	float LookBoundsPushInset = 2.0f;

	// If true, look-bound collisions compress orbit distance instead of pushing the root upward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (EditCondition = "bEnforceOrbitBoundsInManager && bConstrainOrbitCameraToBounds"))
	bool bUseDistanceCompressionForLookBounds = true;

	// Minimum distance allowed when compressing the camera inward to stay inside bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseDistanceCompressionForLookBounds"))
	float LookBoundsMinDistance = 25.0f;

	// Extra inset used for look-bound distance compression so the camera stays slightly inside the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseDistanceCompressionForLookBounds"))
	float LookBoundsDistanceInset = 2.0f;

	// Speed used when compressing orbit distance inward because a look rotation would leave bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseDistanceCompressionForLookBounds && bSmoothOrbitControls"))
	float LookBoundsCompressionSpeed = 14.0f;

	// Speed used when restoring orbit distance after rotating back into free space.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bUseDistanceCompressionForLookBounds && bSmoothOrbitControls"))
	float LookBoundsRecoverySpeed = 6.0f;

	// Extra padding applied by the manager when clamping against the active camera bounds.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager"))
	float ManagerBoundsPadding = 0.0f;

	// When smoothing is enabled, bounds correction also interpolates using this speed so the camera pushes back nicely.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bEnforceOrbitBoundsInManager && bSmoothOrbitControls"))
	float OrbitBoundsCorrectionSpeed = 10.0f;

#pragma endregion

#pragma region Keybinds

	// Default keybinds are coded here so BP/UE input setup stays straightforward.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey OrbitLookHoldKey = EKeys::RightMouseButton;

	// Hold to pan orbit origin with mouse movement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input|Keybinds")
	FKey OrbitPanHoldKey = EKeys::MiddleMouseButton;

#pragma endregion

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
	void UpdateModeTransition(float DeltaTime);
	void CaptureOrbitCameraTransform(FOrbitCameraDefinition& OutDefinition, AOrbitCameraBase* InCamera) const;

	const UBoxComponent* GetActiveBoundsBox() const;
	FVector ClampPointToActiveBounds(const FVector& Point) const;
	FVector CalculateLookBoundsPushOffset(const FVector& RootLocation, const FRotator& RootRotation, float CameraDistance) const;
	float ResolveMaxOrbitDistanceForBounds(const FVector& RootLocation, const FRotator& RootRotation, float DesiredDistance) const;
	FVector ConstrainOrbitRootLocationToBounds(
		const FVector& DesiredRootLocation,
		float DeltaTime,
		bool bApplyCorrectionSmoothing) const;

	void SetOrbitLookPressed();
	void SetOrbitLookReleased();
	void SetOrbitPanPressed();
	void SetOrbitPanReleased();
	void OnOrbitMouseWheel(float Value);

	void OnLookYaw(float Value);
	void OnLookPitch(float Value);

	bool bOrbitLookPressed = false;
	bool bOrbitPanPressed = false;
	bool bOrbitTargetsInitialized = false;

	FVector OrbitTargetRootLocation = FVector::ZeroVector;
	FRotator OrbitTargetRootRotation = FRotator::ZeroRotator;
	float OrbitTargetDistance = 0.0f;
	float OrbitTargetFocalLength = 0.0f;
	FVector OrbitLookBoundsPushOffset = FVector::ZeroVector;

	bool bTransitionInProgress = false;
	float TransitionElapsed = 0.0f;
	float TransitionDuration = 0.0f;
	FTransform TransitionStart = FTransform::Identity;
	FTransform TransitionTarget = FTransform::Identity;
};
