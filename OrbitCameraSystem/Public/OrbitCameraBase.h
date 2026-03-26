// Copyright 2020 RealVisStudios. All Rights Reserved.

#pragma once

// OrbitCameraSystem includes

// Unreal Includes
#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "Math/Rotator.h"
#include "Delegates/IDelegateInstance.h"

#include "OrbitCameraBase.generated.h"

class USpringArmComponent;
class UCineCameraComponent;

DECLARE_LOG_CATEGORY_EXTERN(OrbitCamera, Log, All);

//////////////////////////////////////////////////////////////////////////


// Defines a OrbitCamera. Holds Transformations, Zoom and FOV values
USTRUCT(BlueprintType)
struct FOrbitCameraDefinition
{
	GENERATED_BODY()

public:
	// the OrbitCamera Actor World Transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	FTransform ActorTransform = FTransform::Identity;

	// the OrbitCamera position
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	FVector Position = FVector::ZeroVector;

	// the Cameras Local Rotation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	FRotator Rotation = FRotator::ZeroRotator;

	// the Cameras distance to the Center
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	float Distance = 100.f;

	// the Cameras Zoom value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	float Zoom = 100.f;

	// the Cameras Field of View value
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	float FoV = 45.f;
};

// Defines the Focus behavior of the OrbitCamera
UENUM(BlueprintType)
enum class EOrbitCameraFocus : uint8
{
	//Automatically focus on the OrbitCamera origin/center
	OC_FocusOrigin = 0 UMETA(DisplayName = "FocusOrigin"),

	//Automatically finds the nearest Surface
	OC_AutoFocus UMETA(DisplayName = "AutoFocus"),

	//Use CineCamera settings
	Off UMETA(DisplayName = "Off"),
};

// Editor Preview Modes for Visualisation purposes
UENUM(BlueprintType)
enum class EEditorPositionPreview : uint8
{
	OC_Initial = 0 UMETA(DisplayName = "Inital"),
	OC_Min UMETA(DisplayName = "Min"),
	OC_Max UMETA(DisplayName = "Max"),
};

/**
 *
 */
UCLASS()
class ORBITCAMERASYSTEM_API AOrbitCameraBase : public ACineCameraActor
{
	GENERATED_BODY()

public:
	// constructor
	AOrbitCameraBase(const FObjectInitializer& ObjectInitializer);


#pragma region Defaults

	//Holds the Cameras Scenes Name
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Defaults")
	FName CameraId;

#pragma endregion

#pragma region Position

	//Holds the Initial Actors Transform
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Location")
	FTransform Internal_ActorInitalTransform = FTransform::Identity;

	//Holds the Initial Root location
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Location")
	FVector Internal_InitalLocation = FVector::ZeroVector;

	//Holds the current Actors location
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Location")
	FVector Internal_CurrentLocation = FVector::ZeroVector;

	//Holds the target Actors location
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Location")
	FVector Internal_TargetLocation = FVector::ZeroVector;

	//Holds the location interpolation Speed, this can change dynamically
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Location")
	float Internal_LocationInterpolationSpeed = 5.0;

#pragma endregion

#pragma region Rotation

	//Yaw Editor Preview
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OrbitCamera|Rotation")
	EEditorPositionPreview YawPreview;

	//Minimal Yaw Rotation Limit in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-359.0", ClampMax = "359.0", UIMin = "-359.0", UIMax = "359.0"))
	float MinYaw = -359.0f;

	//Initial Yaw Rotation in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-359.0", ClampMax = "359.0", UIMin = "-359.0", UIMax = "359.0"))
	float InitialYaw = 0.0f;

	//Maximal Yaw Rotation Limit in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-359.0", ClampMax = "359.0", UIMin = "-359.0", UIMax = "359.0"))
	float MaxYaw = 359.0f;

	//Pitch  Editor Preview
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation")
	EEditorPositionPreview PitchPreview;

	//Minimal Pitch Rotation Limit in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0"))
	float MinPitch = 90.0f;

	//Initial Pitch Rotation in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0"))
	float InitialPitch = -25.0f;

	//Maximal Pitch Rotation Limit in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-90.0", ClampMax = "90.0", UIMin = "-90.0", UIMax = "90.0"))
	float MaxPitch = -90.0f;

	//Initial Roll in Degrees
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ExposeOnSpawn = "true", ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0"))
	float InitialRoll = 0.0f;

	// Internals

	//Holds the rotation interpolation speed, this can change dynamically
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Rotation",
		meta = (ClampMin = "0", UIMin = "0"))
	float Internal_RotationInterpolationSpeed = 10.0f;

	//Holds the target rotation
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation")
	FRotator Internal_TargetRotation = FRotator::ZeroRotator;

	//Holds the current rotation
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Rotation")
	FRotator Internal_CurrentRotation = FRotator::ZeroRotator;

#pragma endregion

#pragma region Zoom

	//Zoom Editor Preview. (for FocalLength preview is inverted Max <=> Min)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom")
	EEditorPositionPreview ZoomPreview;

	//Minimal Distance Limit in Meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float MinDistance = 100.0f;

	//Initial  Distance in Meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float InitialDistance = 300.0f;

	//Maximal Distance Limit in Meters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float MaxDistance = 500.0f;


	//Minimal FocalLength Limit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float MinFocalLength = 25.0f;

	//Initial  FocalLength
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float InitialFocalLength = 35.0f;

	//Maximal FocalLength Limit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom",
		meta = (ExposeOnSpawn = "true", ClampMin = "0", UIMin = "0"))
	float MaxFocalLength = 50.0f;

	//internals

	//Holds the Distance (Dolly) interpolation Speed, this can change dynamically
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_DistanceInterpolationSpeed = 10.0;

	//Holds the Zoom/FocalLength interpolation Speed, this can change dynamically
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_ZoomInterpolationSpeed = 10.0;

	//Holds the target Distance
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_TargetDistance;

	//Holds current Distance
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_CurrentDistance;

	//Holds the target Distance
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_TargetFocalLength;

	//Holds current FocalLength
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Zoom", meta = (ClampMin = "0", UIMin = "0"))
	float Internal_CurrentFocalLength;

#pragma endregion

#pragma region Focus

	//Sets the Focus behavior.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus", meta = (ExposeOnSpawn = "true"))
	EOrbitCameraFocus FocusBehavior;

	//Will add this value to FocalDistance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus",
		meta = (ExposeOnSpawn = "true", UIMin = "-100.0", UIMax = "100.0"))
	float FocusOffset;

	//This defines the tolerance for finding objects to focus on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus",
		meta = (ClampMin = "0", UIMin = "0.0", UIMax = "25.0"))
	float AutoFocus_TraceRadius = 10.0f;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus",
		meta = (ClampMin = "0", UIMin = "0.0", UIMax = "100.0"))
	float MinAutoFocusDistance = 1.0f;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus",
		meta = (ClampMin = "0", UIMin = "0.0", UIMax = "1000.0"))
	float MaxAutoFocusDistance = 1000.0f;

	//Defines how fast the Focus can change. Higher values == faster
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Focus",
		meta = (ClampMin = "0", ClampMax = "100", UIMin = "1", UIMax = "20"))
	float AutoFocus_InterpolationSpeed = 5.0f;

	//Sets the Objects to Focus on, only applies if FocusBehavior == AutoFocus. See ProjectSettings/Collisions for Object Types.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "OrbitCamera|Focus")
	TArray<TEnumAsByte<EObjectTypeQuery>> FocusOnObjectTypes;

	//Use complex collision for detecting focus point. If false simple collision will be used. Only applies if FocusBehavior == AutoFocus.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, AdvancedDisplay, Category = "OrbitCamera|Focus")
	bool bUseComplexCollisions = true;

	// Internals

	//
	UPROPERTY(VisibleAnyWhere, BlueprintReadWrite, Category = "OrbitCamera|Focus")
	float Internal_TargetFocusDistance;

#pragma endregion

#pragma region Bounds

	// Enable world-space clamping for OrbitRoot using a BoxComponent on CameraBoundsActor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds")
	bool bClampToBounds = false;

	// Set this to your BP_CameraBounds actor (it must have a BoxComponent).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds", meta = (EditCondition = "bClampToBounds"))
	AActor* CameraBoundsActor = nullptr;

	// Shrinks usable bounds on each side (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bClampToBounds"))
	float BoundsPadding = 30.0f;

#pragma endregion

#pragma region Debugging

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Debugging")
	bool EnableDebugging = false;

#pragma endregion

	UPROPERTY(BlueprintReadOnly, Category = "Internal")
	class UCineCameraComponent* CineCamRef;

	UPROPERTY(BlueprintReadOnly, Category = "Internal")
	class USceneComponent* OrbitRoot;

	// Will be ture if you are in Editor mode / not in any Play mode / not at runtime
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Internal", meta = (WorldContext = "WorldContextObject"))
	bool IsInEditorMode(UObject* WorldContextObject);

#if WITH_EDITOR

	// InEditor Only ! It will reset the *Preview variables to its inital state.
	UFUNCTION()
	void OnSelectionChanged(UObject* InObject);
#endif

protected:
	// We clamp after everything else moved the root this frame.
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR

	FDelegateHandle OnSelectionChangedDelegateHandle;
	virtual void PostRegisterAllComponents() override;
	virtual void BeginDestroy() override;

	//indicates if the Camera was selected in Editor. Is used to identify deselection
	bool bWasSelected = false;

#endif

private:
	void ClampOrbitRootToBounds();
	void ClampCameraToBounds();
};
