// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AOrbitCameraManagerBase::AOrbitCameraManagerBase()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;

}

// Called when the game starts or when spawned
void AOrbitCameraManagerBase::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (bAutoPossessPlayer0OnBeginPlay && PlayerController->GetPawn() != this)
	{
		PlayerController->Possess(this);
	}

	if (bAutoSetViewTargetOnBeginPlay)
	{
		if (AOrbitCameraBase* InitialOrbitCamera = Cast<AOrbitCameraBase>(UGameplayStatics::GetActorOfClass(this, AOrbitCameraBase::StaticClass())))
		{
			PlayerController->SetViewTarget(InitialOrbitCamera);
			CurrentCameraDefinition = InitialOrbitCamera->GetCurrentDefinition();
		}
	}
}

// Called every frame
void AOrbitCameraManagerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AOrbitCameraManagerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
