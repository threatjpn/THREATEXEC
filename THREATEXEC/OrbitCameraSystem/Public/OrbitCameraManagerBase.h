// Copyright 2020 RealVisStudios. All Rights Reserved.

#pragma once

// OrbitCameraSystem includes
#include "OrbitCameraBase.h"

// Unreal Includes
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

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

	// Automatically possess this manager with PlayerController(0) at runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Startup")
	bool bAutoPossessPlayer0OnBeginPlay = true;

	// Automatically set PlayerController(0) view target to the first found OrbitCamera actor.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Startup")
	bool bAutoSetViewTargetOnBeginPlay = true;

	// Hold RMB ("Secondary") to look around orbit root.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	bool bEnableLookInput = true;

	// Hold MMB to pan orbit root in camera local right/up plane.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input")
	bool bEnablePanInput = true;

	// Mouse sensitivity applied to look input.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LookInputSensitivity = 0.2f;

	// Pan speed scalar in world units per mouse unit.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Input", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PanInputSpeed = 2.0f;

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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UFUNCTION()
	void OnLookPressed();

	UFUNCTION()
	void OnLookReleased();

	UFUNCTION()
	void OnPanPressed();

	UFUNCTION()
	void OnPanReleased();

	UFUNCTION()
	void OnMouseX(float Value);

	UFUNCTION()
	void OnMouseY(float Value);

private:
	TObjectPtr<AOrbitCameraBase> ActiveOrbitCamera = nullptr;
	bool bLookHeld = false;
	bool bPanHeld = false;
	float PendingMouseX = 0.0f;
	float PendingMouseY = 0.0f;
};
