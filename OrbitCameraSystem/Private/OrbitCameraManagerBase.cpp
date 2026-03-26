// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "OrbitWalkingPawn.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOrbitCameraManagerBase::AOrbitCameraManagerBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AOrbitCameraManagerBase::BeginPlay()
{
	Super::BeginPlay();

	if (!ActiveOrbitCamera)
	{
		ActiveOrbitCamera = Cast<AOrbitCameraBase>(UGameplayStatics::GetActorOfClass(this, AOrbitCameraBase::StaticClass()));
	}

	if (ActiveOrbitCamera)
	{
		CurrentCameraDefinition = ActiveOrbitCamera->GetCurrentDefinition();
	}
}

// Called every frame
void AOrbitCameraManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ActiveOrbitCamera)
	{
		CurrentCameraDefinition = ActiveOrbitCamera->GetCurrentDefinition();
	}
}

bool AOrbitCameraManagerBase::TransitionToCamera(AOrbitCameraBase* TargetCamera, EOrbitCameraTransition TransitionType)
{
	if (!TargetCamera)
	{
		return false;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	StartCameraDefinition = ActiveOrbitCamera ? ActiveOrbitCamera->GetCurrentDefinition() : TargetCamera->GetCurrentDefinition();
	TargetCameraDefinition = TargetCamera->GetCurrentDefinition();

	const float BlendTime = (TransitionType == EOrbitCameraTransition::OC_HardCut) ? 0.0f : TransitionParams.Duration;
	PC->SetViewTargetWithBlend(TargetCamera, BlendTime);

	ActiveOrbitCamera = TargetCamera;
	CurrentCameraDefinition = TargetCameraDefinition;
	return true;
}

bool AOrbitCameraManagerBase::EnterWalkingMode(bool bMatchCurrentCamera)
{
	if (ActiveMode == EOrbitCameraMode::Walking)
	{
		return true;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	if (!WalkingPawn)
	{
		if (!WalkingPawnClass)
		{
			WalkingPawnClass = AOrbitWalkingPawn::StaticClass();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		WalkingPawn = GetWorld()->SpawnActor<AOrbitWalkingPawn>(WalkingPawnClass, GetActorLocation(), GetActorRotation(), SpawnParams);
	}

	if (!WalkingPawn)
	{
		return false;
	}

	if (bMatchCurrentCamera && ActiveOrbitCamera)
	{
		WalkingPawn->SetActorLocation(ActiveOrbitCamera->GetActorLocation());
		WalkingPawn->SetActorRotation(ActiveOrbitCamera->GetActorRotation());
	}

	if (ActiveOrbitCamera)
	{
		WalkingPawn->ConfigureMovementBounds(
			ActiveOrbitCamera->bClampToBounds,
			ActiveOrbitCamera->CameraBoundsActor,
			ActiveOrbitCamera->BoundsPadding);
	}

	PC->Possess(WalkingPawn);
	PC->SetViewTargetWithBlend(WalkingPawn, TransitionParams.Duration);
	ActiveMode = EOrbitCameraMode::Walking;
	return true;
}

bool AOrbitCameraManagerBase::ExitWalkingMode(bool bMatchWalkCameraToOrbit)
{
	if (ActiveMode == EOrbitCameraMode::Orbit)
	{
		return true;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return false;
	}

	PC->Possess(this);
	if (ActiveOrbitCamera)
	{
		if (bMatchWalkCameraToOrbit && WalkingPawn)
		{
			FOrbitCameraDefinition NewDefinition = ActiveOrbitCamera->GetCurrentDefinition();
			NewDefinition.Position = WalkingPawn->GetActorLocation();
			NewDefinition.Rotation = WalkingPawn->GetActorRotation();
			ActiveOrbitCamera->StartTransitionToDefinition(NewDefinition, TransitionParams);
		}
		PC->SetViewTargetWithBlend(ActiveOrbitCamera, TransitionParams.Duration);
	}

	ActiveMode = EOrbitCameraMode::Orbit;
	return true;
}

// Called to bind functionality to input
void AOrbitCameraManagerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
