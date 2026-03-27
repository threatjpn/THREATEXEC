// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "OrbitWalkingPawn.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

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

	DiscoverOrbitCamerasIfNeeded();

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

void AOrbitCameraManagerBase::DiscoverOrbitCamerasIfNeeded()
{
	if (!bAutoDiscoverOrbitCameras || !GetWorld())
	{
		return;
	}

	if (RegisteredOrbitCameras.Num() > 0)
	{
		return;
	}

	for (TActorIterator<AOrbitCameraBase> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It))
		{
			RegisteredOrbitCameras.Add(*It);
		}
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

int32 AOrbitCameraManagerBase::GetActiveCameraIndex() const
{
	if (!ActiveOrbitCamera)
	{
		return INDEX_NONE;
	}

	return RegisteredOrbitCameras.IndexOfByPredicate([this](const TObjectPtr<AOrbitCameraBase>& Camera)
	{
		return Camera == ActiveOrbitCamera;
	});
}

bool AOrbitCameraManagerBase::TransitionToNextCamera(EOrbitCameraTransition TransitionType)
{
	DiscoverOrbitCamerasIfNeeded();
	if (RegisteredOrbitCameras.Num() == 0)
	{
		return false;
	}

	int32 ActiveIndex = GetActiveCameraIndex();
	if (ActiveIndex == INDEX_NONE)
	{
		ActiveIndex = 0;
	}

	for (int32 Offset = 1; Offset <= RegisteredOrbitCameras.Num(); ++Offset)
	{
		const int32 CandidateIndex = (ActiveIndex + Offset) % RegisteredOrbitCameras.Num();
		AOrbitCameraBase* Candidate = RegisteredOrbitCameras[CandidateIndex];
		if (IsValid(Candidate))
		{
			return TransitionToCamera(Candidate, TransitionType);
		}
	}

	return false;
}

bool AOrbitCameraManagerBase::TransitionToPreviousCamera(EOrbitCameraTransition TransitionType)
{
	DiscoverOrbitCamerasIfNeeded();
	if (RegisteredOrbitCameras.Num() == 0)
	{
		return false;
	}

	int32 ActiveIndex = GetActiveCameraIndex();
	if (ActiveIndex == INDEX_NONE)
	{
		ActiveIndex = 0;
	}

	for (int32 Offset = 1; Offset <= RegisteredOrbitCameras.Num(); ++Offset)
	{
		const int32 CandidateIndex = (ActiveIndex - Offset + RegisteredOrbitCameras.Num()) % RegisteredOrbitCameras.Num();
		AOrbitCameraBase* Candidate = RegisteredOrbitCameras[CandidateIndex];
		if (IsValid(Candidate))
		{
			return TransitionToCamera(Candidate, TransitionType);
		}
	}

	return false;
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
