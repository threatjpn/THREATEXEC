// Copyright 2020 RealVisStudios. All Rights Reserved.

#pragma once

// OrbitCameraSystem includes
#include "OrbitCameraBase.h"

// Unreal Includes
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "OrbitCameraManagerBase.generated.h"

class AOrbitWalkingPawn;

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

UENUM(BlueprintType)
enum class EOrbitCameraMode : uint8
{
	Orbit = 0 UMETA(DisplayName = "Orbit"),
	Walking UMETA(DisplayName = "Walking"),
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Transition")
	FOrbitTransitionParams TransitionParams;

#pragma endregion

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera")
	TObjectPtr<AOrbitCameraBase> ActiveOrbitCamera = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbitCamera|Modes")
	TSubclassOf<AOrbitWalkingPawn> WalkingPawnClass;

	UPROPERTY(BlueprintReadOnly, Category = "OrbitCamera|Modes")
	TObjectPtr<AOrbitWalkingPawn> WalkingPawn = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "OrbitCamera|Modes")
	EOrbitCameraMode ActiveMode = EOrbitCameraMode::Orbit;

	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Transition")
	bool TransitionToCamera(AOrbitCameraBase* TargetCamera, EOrbitCameraTransition TransitionType = EOrbitCameraTransition::OC_Transition);

	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Modes")
	bool EnterWalkingMode(bool bMatchCurrentCamera = true);

	UFUNCTION(BlueprintCallable, Category = "OrbitCamera|Modes")
	bool ExitWalkingMode(bool bMatchWalkCameraToOrbit = true);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
