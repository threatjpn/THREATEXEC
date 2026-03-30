// Copyright 2020 RealVisStudios. All Rights Reserved.

#include "OrbitCameraManagerBase.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
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
			ActiveOrbitCamera = InitialOrbitCamera;
			PlayerController->SetViewTarget(InitialOrbitCamera);
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

	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AOrbitCameraManagerBase::OnLookPressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AOrbitCameraManagerBase::OnLookReleased);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AOrbitCameraManagerBase::OnPanPressed);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AOrbitCameraManagerBase::OnPanReleased);
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitCameraManagerBase::OnMouseX);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitCameraManagerBase::OnMouseY);
}

void AOrbitCameraManagerBase::OnLookPressed()
{
	bLookHeld = true;
}

void AOrbitCameraManagerBase::OnLookReleased()
{
	bLookHeld = false;
}

void AOrbitCameraManagerBase::OnPanPressed()
{
	bPanHeld = true;
}

void AOrbitCameraManagerBase::OnPanReleased()
{
	bPanHeld = false;
}

void AOrbitCameraManagerBase::OnMouseX(float Value)
{
	PendingMouseX = Value;

	if (!ActiveOrbitCamera || !ActiveOrbitCamera->OrbitRoot)
	{
		return;
	}

	if (bEnableLookInput && bLookHeld && !FMath::IsNearlyZero(Value))
	{
		FRotator Rotation = ActiveOrbitCamera->OrbitRoot->GetRelativeRotation();
		Rotation.Yaw += Value * LookInputSensitivity;
		Rotation.Yaw = FMath::Clamp(Rotation.Yaw, ActiveOrbitCamera->MinYaw, ActiveOrbitCamera->MaxYaw);
		ActiveOrbitCamera->OrbitRoot->SetRelativeRotation(Rotation);
		ActiveOrbitCamera->Internal_TargetRotation = Rotation;
		ActiveOrbitCamera->Internal_CurrentRotation = Rotation;
	}

	if (bEnablePanInput && bPanHeld && !FMath::IsNearlyZero(Value))
	{
		const FVector Right = ActiveOrbitCamera->GetActorRightVector();
		const FVector Delta = (-Right * Value) * PanInputSpeed;
		const FVector NewLocation = ActiveOrbitCamera->OrbitRoot->GetComponentLocation() + Delta;
		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(NewLocation);
		ActiveOrbitCamera->Internal_TargetLocation = NewLocation;
		ActiveOrbitCamera->Internal_CurrentLocation = NewLocation;
	}
}

void AOrbitCameraManagerBase::OnMouseY(float Value)
{
	PendingMouseY = Value;

	if (!ActiveOrbitCamera || !ActiveOrbitCamera->OrbitRoot)
	{
		return;
	}

	if (bEnableLookInput && bLookHeld && !FMath::IsNearlyZero(Value))
	{
		FRotator Rotation = ActiveOrbitCamera->OrbitRoot->GetRelativeRotation();
		Rotation.Pitch = FMath::Clamp(Rotation.Pitch + (Value * LookInputSensitivity), ActiveOrbitCamera->MinPitch, ActiveOrbitCamera->MaxPitch);
		ActiveOrbitCamera->OrbitRoot->SetRelativeRotation(Rotation);
		ActiveOrbitCamera->Internal_TargetRotation = Rotation;
		ActiveOrbitCamera->Internal_CurrentRotation = Rotation;
	}

	if (bEnablePanInput && bPanHeld && !FMath::IsNearlyZero(Value))
	{
		const FVector Up = ActiveOrbitCamera->GetActorUpVector();
		const FVector Delta = (Up * Value) * PanInputSpeed;
		const FVector NewLocation = ActiveOrbitCamera->OrbitRoot->GetComponentLocation() + Delta;
		ActiveOrbitCamera->OrbitRoot->SetWorldLocation(NewLocation);
		ActiveOrbitCamera->Internal_TargetLocation = NewLocation;
		ActiveOrbitCamera->Internal_CurrentLocation = NewLocation;
	}
}
